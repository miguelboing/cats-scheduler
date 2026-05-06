import json
import sys
import os
import copy
import random
import subprocess
import tempfile
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass
from typing import Optional
from collections import defaultdict
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

TESTS_DIR = "tests"

# ── UUniFast task-set generator ───────────────────────────────────────────────

def uunifast(n: int, U: float) -> list[float]:
    """Bini & Buttazzo UUniFast: returns n utilizations in (0,1) summing to U,
    uniformly distributed on the simplex."""
    utils = []
    s = U
    for i in range(1, n):
        nxt = s * random.random() ** (1.0 / (n - i))
        utils.append(s - nxt)
        s = nxt
    utils.append(s)
    return utils

def uunifast_packets(n: int, U: float, c_min: int, c_max: int,
                     sr_min: float, sr_max: float) -> list[dict]:
    """Turn a UUniFast draw into a fixed_rate packet list.
    Each task gets an independent frame count C ~ uniform_int[c_min, c_max]
    and success-rate requirement sr ~ uniform[sr_min, sr_max]. Deadline is
    derived from utilization: D = floor(C / u). Since UUniFast yields u in
    (0,1), we have D >= C, so the floored D keeps actual u' = C/D in (u, 1]
    — utilization stays at-or-slightly-above the requested level instead of
    drifting upward as it would when rounding C from a fixed D."""
    utils = uunifast(n, U)
    packets = []
    for i, u in enumerate(utils):
        C = random.randint(c_min, c_max)
        D = max(C, int(C / u))  # floor; clamp to C if u ≈ 1
        packets.append({
            "id":                i + 1,
            "relative_deadline": D,
            "frames":            C,
            "success_rate":      round(random.uniform(sr_min, sr_max), 2),
            "period":            D,
            "phase":             0,
        })
    return packets

def make_config(test: dict) -> dict:
    """Resolve a test definition into a concrete simulator config. If a
    'uunifast' spec is present, a fresh packet_generators block is drawn."""
    config = copy.deepcopy(test["config"])
    if "uunifast" in test:
        spec = test["uunifast"]
        config["packet_generators"] = [{
            "type":    "fixed_rate",
            "packets": uunifast_packets(spec["n"], spec["U"],
                                        spec["c_min"],  spec["c_max"],
                                        spec["sr_min"], spec["sr_max"]),
        }]
    return config

# ── Tests ─────────────────────────────────────────────────────────────────────

BASE_SCHEDULER = {
    "type": "CHARM",
    "tx_power": 10,
    "frequency": 14074000,
    "rx_period": 5
}

BASE_CATS_SCHEDULER = {
    "type": "CATS",
    "frequency": 14074000,
    "belief_threshold": 0.7,
    "margin": 0.1
}

BASE_CHANNELS = [
    {
        "type": "sigmoid",
        "name": "channel_20m",
        "frequency": 14074000
    }
]

BASE_SIM = { "duration": 5000 } # About the same amount of frames contained in a day

def scenario_label(n: int, c_min: int, c_max: int,
                   sr_min: float, sr_max: float, U: Optional[float] = None) -> str:
    """Render a scenario's parameters as a compact label used in plot titles
    and filenames. Always derived from the actual values so it can't drift."""
    parts = []
    if U is not None:
        parts.append(f"U={int(round(U * 100))}")
    parts.append(f"n={n}")
    parts.append(f"C=[{c_min},{c_max}]")
    parts.append(f"SR=[{sr_min},{sr_max}]")
    return ",".join(parts)

# Each scenario: (U, n, c_min, c_max, sr_min, sr_max)
SCENARIOS = [
    (0.10, 2,  1, 3, 0.50, 0.70),
    (0.25, 5,  1, 3, 0.50, 0.70),
    (0.50, 8,  1, 3, 0.50, 0.70),
]

SCHEDULERS = [
    ("Rate_M", { "type": "Rate_M", "tx_power": 10, "frequency": 14074000 }),
    ("CHARM",  BASE_SCHEDULER),
    ("CATS",   BASE_CATS_SCHEDULER),
]

TESTS = [
    {
        "name": f"{scenario_label(n, c_min, c_max, sr_min, sr_max, U)}_{sch_name}",
        "config": {
            "simulation": BASE_SIM,
            "scheduler":  scheduler,
            "channels":   BASE_CHANNELS,
        },
        "uunifast": {
            "U":      U,
            "n":      n,
            "c_min":  c_min,
            "c_max":  c_max,
            "sr_min": sr_min,
            "sr_max": sr_max,
        },
    }
    for U, n, c_min, c_max, sr_min, sr_max in SCENARIOS
    for sch_name, scheduler in SCHEDULERS
]

CONFIG_FILE = "simulation_config.json"
LOG_FILE    = "simulation_log.json"
SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
BINARY      = os.path.join(SCRIPT_DIR, "main.o")

# ── Data classes ──────────────────────────────────────────────────────────────

@dataclass
class Packet:
    id: int
    id_count: int
    deadline: int
    frames: int
    frame_count: int
    success_rate_req: float

@dataclass
class Transmission:
    packet: Packet
    tx_power: int
    frequency: int
    probability: float
    success_rate_req: float
    received: bool

@dataclass
class Prediction:
    powers: list[int]
    probs: list[float]
    frequency: int

@dataclass
class FsmcStatus:
    frequency: int
    state: int
    slope: float
    snr_50_db: float
    max_saturation: float
    noise_floor_dbm: float

@dataclass
class Frame:
    tick: int
    radio_mode: str
    buffer: list[Packet]
    fsmc: list[FsmcStatus]
    missed_packets: list[Packet]
    dropped_packets: list[Packet]
    transmission: Optional[Transmission] = None
    prediction: Optional[Prediction] = None

# ── Parsers ───────────────────────────────────────────────────────────────────

def parse_packet(d: dict) -> Packet:
    return Packet(
        id               = d["id"],
        id_count         = d["id_count"],
        deadline         = d["deadline"],
        frames           = d["frames"],
        frame_count      = d.get("frame_count", 0),
        success_rate_req = d["success_rate_req"]
    )

def parse_frame(d: dict) -> Frame:
    transmission = None
    if "transmission" in d:
        t = d["transmission"]
        transmission = Transmission(
            packet           = parse_packet(t["packet"]),
            tx_power         = t["tx_power"],
            frequency        = t["frequency"],
            probability      = t["probability"],
            success_rate_req = t["success_rate_req"],
            received         = t["received"]
        )

    prediction = None
    if "prediction" in d:
        p = d["prediction"]
        prediction = Prediction(
            powers    = p["powers"],
            probs     = p["probs"],
            frequency = p["frequency"]
        )

    return Frame(
        tick            = d["tick"],
        radio_mode      = d["radio_mode"],
        buffer          = [parse_packet(p) for p in d["buffer"]],
        fsmc            = [FsmcStatus(**ch) for ch in d["fsmc"]],
        missed_packets  = [parse_packet(p) for p in d["missed_packets"]],
        dropped_packets = [parse_packet(p) for p in d.get("dropped_packets", [])],
        transmission    = transmission,
        prediction      = prediction
    )

# ── Runner ────────────────────────────────────────────────────────────────────

def write_config(config: dict, path: str):
    with open(path, "w") as f:
        json.dump(config, f, indent=4)

def run_simulation(binary: str, config_file: str, log_file: Optional[str] = None):
    cmd = [binary, config_file] + ([log_file] if log_file else [])
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=SCRIPT_DIR)
    if result.returncode != 0:
        print("Simulation failed:")
        print(result.stdout)
        print(result.stderr)
        raise RuntimeError("Simulation exited with code", result.returncode)

def load_results(log_file: str) -> list[Frame]:
    with open(log_file) as f:
        raw = json.load(f)
    return [parse_frame(entry) for entry in raw]

def print_test_summary(test: dict):
    cfg = test["config"]
    print(f"\n── Test: {test['name']} ──")
    sched = cfg['scheduler']
    tx_power_str = f"  tx_power={sched['tx_power']}W" if 'tx_power' in sched else ""
    rx_period_str = f"  rx_period={sched['rx_period']}" if 'rx_period' in sched else ""
    belief_str = f"  belief_threshold={sched['belief_threshold']}" if 'belief_threshold' in sched else ""
    margin_str = f"  margin={sched['margin']}" if 'margin' in sched else ""
    print(f"   Scheduler  : {sched['type']}{tx_power_str}  freq={sched['frequency']}Hz{rx_period_str}{belief_str}{margin_str}")
    print(f"   Duration   : {cfg['simulation']['duration']} ticks")
    print(f"   Channels   : {', '.join(ch['name'] for ch in cfg['channels'])}")
    print(f"   Packets:")
    for gen in cfg["packet_generators"]:
        for p in gen["packets"]:
            print(f"     id={p['id']}  period={p['period']}  deadline={p['relative_deadline']}"
                  f"  frames={p['frames']}  success_rate={p['success_rate']}  phase={p['phase']}")

def run_test(test: dict) -> tuple[list["Frame"], dict]:
    config = make_config(test)
    print_test_summary({ "name": test["name"], "config": config })
    cfg_path = os.path.abspath(CONFIG_FILE)
    log_path = os.path.abspath(LOG_FILE)
    write_config(config, cfg_path)
    run_simulation(BINARY, cfg_path, log_path)
    frames = load_results(log_path)
    print(f"   Done — {len(frames)} frames loaded")
    return frames, config

# ── Metrics extraction ────────────────────────────────────────────────────────

def extract_metrics(frames: list[Frame], config: dict) -> dict:
    ticks = [f.tick for f in frames]

    fsmc_state  = [f.fsmc[0].state if f.fsmc else None for f in frames]
    tx_prob     = [f.transmission.probability if f.transmission else None for f in frames]
    tx_power    = [f.transmission.tx_power if f.transmission else 0 for f in frames]
    pred_prob   = [f.prediction.probs[0] if f.prediction else None for f in frames]
    missed_ticks  = [f.tick for f in frames if f.missed_packets]
    dropped_ticks = [f.tick for f in frames if f.dropped_packets]

    cumulative_missed = []
    cumulative_dropped = []
    total_missed = 0
    total_dropped = 0
    for f in frames:
        total_missed  += len(f.missed_packets)
        total_dropped += len(f.dropped_packets)
        cumulative_missed.append(total_missed)
        cumulative_dropped.append(total_dropped)

    cumulative_generated = []
    seen = set()
    total_gen = 0
    for f in frames:
        for p in f.buffer:
            key = (p.id, p.id_count)
            if key not in seen:
                seen.add(key)
                total_gen += 1
        cumulative_generated.append(total_gen)

    # Seed all packet IDs and requirements from config so untransmitted packets still appear
    per_id_ticks   = defaultdict(list)
    per_id_success = defaultdict(list)
    per_id_req     = {}
    id_total       = defaultdict(int)
    id_received    = defaultdict(int)
    for gen in config["packet_generators"]:
        for p in gen["packets"]:
            per_id_req[p["id"]] = p["success_rate"]

    for f in frames:
        if f.transmission:
            pid = f.transmission.packet.id
            id_total[pid]    += 1
            id_received[pid] += int(f.transmission.received)
            per_id_ticks[pid].append(f.tick)
            per_id_success[pid].append(id_received[pid] / id_total[pid])

    # Undelivered packets: instances where not all frame slots were successfully received.
    # Track per (id, id_count, frame_count) whether that frame slot ever had received=True.
    # This correctly handles retransmissions: a slot counts as delivered if received=True
    # at least once, regardless of how many times it was transmitted.
    instance_frames_needed = {}   # (id, id_count) -> frames
    frame_slot_received    = set()  # (id, id_count, frame_count) that had received=True

    for f in frames:
        if f.transmission:
            t   = f.transmission
            key = (t.packet.id, t.packet.id_count)
            instance_frames_needed[key] = t.packet.frames
            if t.received:
                frame_slot_received.add((t.packet.id, t.packet.id_count, t.packet.frame_count))

    # Also register instances that were missed or dropped without any transmission
    for f in frames:
        for p in f.missed_packets + f.dropped_packets:
            key = (p.id, p.id_count)
            if key not in instance_frames_needed:
                instance_frames_needed[key] = p.frames

    undelivered_per_id = defaultdict(int)
    generated_per_id   = defaultdict(int)
    for (pid, pid_count), needed in instance_frames_needed.items():
        generated_per_id[pid] += 1
        slots_received = sum(
            1 for slot in range(needed)
            if (pid, pid_count, slot) in frame_slot_received
        )
        if slots_received < needed:
            undelivered_per_id[pid] += 1

    total_transmissions = sum(1 for f in frames if f.transmission)

    return dict(
        ticks                = ticks,
        fsmc_state           = fsmc_state,
        tx_prob              = tx_prob,
        tx_power             = tx_power,
        pred_prob            = pred_prob,
        missed_ticks         = missed_ticks,
        dropped_ticks        = dropped_ticks,
        cumulative_missed    = cumulative_missed,
        cumulative_dropped   = cumulative_dropped,
        cumulative_generated = cumulative_generated,
        undelivered_per_id   = undelivered_per_id,
        generated_per_id     = generated_per_id,
        total_transmissions  = total_transmissions,
        per_id_ticks         = per_id_ticks,
        per_id_success       = per_id_success,
        per_id_req           = per_id_req,
    )

# ── Per-test plot ─────────────────────────────────────────────────────────────

def plot_test(m: dict, test_name: str, scheduler_type: str):
    ticks  = m["ticks"]
    colors = plt.cm.tab10.colors

    fig = plt.figure(figsize=(14, 16))
    gs  = gridspec.GridSpec(4, 2, figure=fig, hspace=0.45, wspace=0.35)

    # 1. FSMC state (states 0-2 = Excellent, 3-5 = Worst)
    quality = [None if s is None else (0 if s < 3 else 1) for s in m["fsmc_state"]]
    ax1 = fig.add_subplot(gs[0, :])
    ax1.plot(ticks, quality, color="steelblue", linewidth=0.8)
    ax1.set_ylabel("Channel Quality")
    ax1.set_xlabel("Tick")
    ax1.set_title("Channel Quality Evolution")
    ax1.set_yticks(range(2))
    ax1.set_yticklabels(["Excellent", "Worst"])
    ax1.invert_yaxis()
    ax1.grid(True, alpha=0.3)

    # 2. Actual vs predicted probability
    ax2 = fig.add_subplot(gs[1, 0])
    tx_ticks   = [ticks[i] for i, v in enumerate(m["tx_prob"])   if v is not None]
    pred_ticks = [ticks[i] for i, v in enumerate(m["pred_prob"]) if v is not None]
    ax2.scatter(tx_ticks,   [v for v in m["tx_prob"]   if v is not None], s=4, color="steelblue", label="Actual",    alpha=0.6)
    ax2.scatter(pred_ticks, [v for v in m["pred_prob"] if v is not None], s=4, color="orange",    label="Predicted", alpha=0.6)
    ax2.set_ylabel("Probability")
    ax2.set_xlabel("Tick")
    ax2.set_title("Actual vs Predicted Probability")
    ax2.set_ylim(0, 1.05)
    ax2.legend(markerscale=3)
    ax2.grid(True, alpha=0.3)

    # 3. Cumulative success rate per packet
    ax3 = fig.add_subplot(gs[1, 1])
    for i, pid in enumerate(sorted(m["per_id_req"].keys())):
        color = colors[i % len(colors)]
        if m["per_id_ticks"][pid]:
            ax3.plot(m["per_id_ticks"][pid], m["per_id_success"][pid], color=color, linewidth=1, label=f"id={pid}")
        else:
            ax3.scatter([], [], color=color, label=f"id={pid} (never scheduled)")
        ax3.axhline(y=m["per_id_req"][pid], color=color, linestyle="--", linewidth=0.8, label=f"req id={pid} ({m['per_id_req'][pid]})")
    ax3.set_ylabel("Success rate")
    ax3.set_xlabel("Tick")
    ax3.set_title("Cumulative TX Success Rate per Packet")
    ax3.set_ylim(0, 1.05)
    ax3.legend()
    ax3.grid(True, alpha=0.3)

    # 4. Cumulative generated vs missed vs dropped
    ax4 = fig.add_subplot(gs[2, 0])  # row 2
    ax4.plot(ticks, m["cumulative_generated"], color="steelblue", linewidth=1, label="Generated")
    ax4.plot(ticks, m["cumulative_missed"],    color="crimson",   linewidth=1, label="Missed")
    ax4.plot(ticks, m["cumulative_dropped"],   color="darkorange",linewidth=1, label="Dropped")
    if m["missed_ticks"]:
        ax4.scatter(m["missed_ticks"],
                    [m["cumulative_missed"][ticks.index(t)] for t in m["missed_ticks"]],
                    color="crimson", s=15, zorder=5)
    if m["dropped_ticks"]:
        ax4.scatter(m["dropped_ticks"],
                    [m["cumulative_dropped"][ticks.index(t)] for t in m["dropped_ticks"]],
                    color="darkorange", s=15, zorder=5)
    ax4.set_ylabel("Cumulative packets")
    ax4.set_xlabel("Tick")
    ax4.set_title("Cumulative Generated vs Missed vs Dropped")
    ax4.legend()
    ax4.grid(True, alpha=0.3)

    # 5. Undelivered packets per packet ID (generated vs not fully received)
    ax5 = fig.add_subplot(gs[3, :])
    pids     = sorted(set(list(m["generated_per_id"].keys()) + list(m["per_id_req"].keys())))
    x        = range(len(pids))
    bar_w    = 0.35
    gen_vals = [m["generated_per_id"].get(pid, 0)   for pid in pids]
    und_vals = [m["undelivered_per_id"].get(pid, 0)  for pid in pids]
    ax5.bar([i - bar_w/2 for i in x], gen_vals, bar_w, label="Generated",   color="steelblue", alpha=0.8)
    ax5.bar([i + bar_w/2 for i in x], und_vals, bar_w, label="Undelivered", color="crimson",   alpha=0.8)
    ax5.set_xticks(list(x))
    ax5.set_xticklabels([f"id={pid}" for pid in pids])
    ax5.set_ylabel("Packet instances")
    ax5.set_title("Generated vs Undelivered Packet Instances (not all frames received)")
    ax5.legend()
    ax5.grid(True, alpha=0.3, axis="y")

    # 6. Average power consumption (0W for IDLE/RX)
    ax6 = fig.add_subplot(gs[2, 1])
    pw_vals = m["tx_power"]
    cumulative_avg = [sum(pw_vals[:i+1]) / (i+1) for i in range(len(pw_vals))]
    final_avg = cumulative_avg[-1] if cumulative_avg else 0
    ax6.plot(ticks, pw_vals, color="steelblue", linewidth=0.6, alpha=0.4, label="Power per tick")
    ax6.plot(ticks, cumulative_avg, color="orange", linewidth=1.2, label=f"Cumulative avg = {final_avg:.2f}W")
    ax6.set_ylabel("Power (W)")
    ax6.set_xlabel("Tick")
    ax6.set_title("Power Consumption (0W = IDLE/RX)")
    ax6.legend()
    ax6.grid(True, alpha=0.3)

    plt.suptitle(f"{scheduler_type} — {test_name}", fontsize=13)
    os.makedirs(TESTS_DIR, exist_ok=True)
    out = os.path.join(TESTS_DIR, f"results_{test_name}.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"Plot saved to {out}")

# ── Comparison plot ───────────────────────────────────────────────────────────

def plot_comparison(results: list[tuple[str, dict]]):
    colors = plt.cm.tab10.colors
    n      = len(results)

    fig = plt.figure(figsize=(14, 10))
    gs  = gridspec.GridSpec(2, 2, figure=fig, hspace=0.45, wspace=0.35)

    # 1. Cumulative missed per test
    ax1 = fig.add_subplot(gs[0, 0])
    for i, (name, m) in enumerate(results):
        ax1.plot(m["ticks"], m["cumulative_missed"], color=colors[i], linewidth=1, label=name)
    ax1.set_ylabel("Cumulative missed")
    ax1.set_xlabel("Tick")
    ax1.set_title("Cumulative Missed Deadlines")
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    # 2. Miss ratio (missed / generated) per test
    ax2 = fig.add_subplot(gs[0, 1])
    for i, (name, m) in enumerate(results):
        ratio = [ms / gn if gn > 0 else 0
                 for ms, gn in zip(m["cumulative_missed"], m["cumulative_generated"])]
        ax2.plot(m["ticks"], ratio, color=colors[i], linewidth=1, label=name)
    ax2.set_ylabel("Miss ratio")
    ax2.set_xlabel("Tick")
    ax2.set_title("Miss Ratio (missed / generated)")
    ax2.set_ylim(0, 1.05)
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    # 3. Final cumulative success rate per packet per test
    ax3 = fig.add_subplot(gs[1, 0])
    x_pos  = 0
    bar_w  = 0.35
    for i, (name, m) in enumerate(results):
        for j, pid in enumerate(sorted(m["per_id_req"].keys())):
            final_sr = m["per_id_success"][pid][-1] if m["per_id_success"][pid] else 0
            req      = m["per_id_req"][pid]
            pos      = x_pos + j + i * bar_w
            ax3.bar(pos, final_sr, width=bar_w, color=colors[i], alpha=0.8,
                    label=name if j == 0 else "")
            ax3.plot([pos - bar_w / 2, pos + bar_w / 2], [req, req],
                     color="black", linewidth=1.2, linestyle="--")
        x_pos += len(m["per_id_req"]) + 1
    ax3.set_ylabel("Final success rate")
    ax3.set_title("Final Success Rate vs Requirement per Packet")
    ax3.set_ylim(0, 1.05)
    ax3.legend()
    ax3.grid(True, alpha=0.3, axis="y")

    # 4. FSMC state evolution per test (Excellent vs Worst)
    ax4 = fig.add_subplot(gs[1, 1])
    for i, (name, m) in enumerate(results):
        quality = [None if s is None else (0 if s < 3 else 1) for s in m["fsmc_state"]]
        ax4.plot(m["ticks"], quality, color=colors[i], linewidth=0.6, alpha=0.8, label=name)
    ax4.set_ylabel("Channel Quality")
    ax4.set_xlabel("Tick")
    ax4.set_title("FSMC State Evolution")
    ax4.set_yticks(range(2))
    ax4.set_yticklabels(["Excellent", "Worst"])
    ax4.invert_yaxis()
    ax4.legend()
    ax4.grid(True, alpha=0.3)

    plt.suptitle("Test Comparison", fontsize=13)
    os.makedirs(TESTS_DIR, exist_ok=True)
    out = os.path.join(TESTS_DIR, "results_comparison.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"Comparison plot saved to {out}")

# ── Summary table ─────────────────────────────────────────────────────────────

def print_summary_table(test_name: str, metrics: dict, config: dict):
    tx_power      = metrics["tx_power"]
    total_power   = sum(tx_power)
    average_power = total_power / len(tx_power) if tx_power else 0.0

    pids = sorted(metrics["per_id_req"].keys())

    # Build per-id req lookup from config
    id_req = {p["id"]: p["success_rate"] for gen in config["packet_generators"] for p in gen["packets"]}

    col_w = [6, 20, 16, 18, 18, 12, 12]
    header = (
        f"{'ID':<{col_w[0]}}"
        f"{'Undelivered':>{col_w[1]}}"
        f"{'Generated':>{col_w[2]}}"
        f"{'Success Ratio':>{col_w[3]}}"
        f"{'Success Req':>{col_w[4]}}"
        f"{'Avg Power (W)':>{col_w[5]}}"
        f"{'Total Power (W)':>{col_w[6]}}"
    )
    sep = "-" * sum(col_w)

    print(f"\n{'═' * sum(col_w)}")
    print(f"  Summary: {test_name}")
    print(f"{'═' * sum(col_w)}")
    print(header)
    print(sep)

    for pid in pids:
        undelivered   = metrics["undelivered_per_id"].get(pid, 0)
        generated     = metrics["generated_per_id"].get(pid, 0)
        success_ratio = (1 - undelivered / generated) if generated > 0 else 0.0
        req           = id_req.get(pid, float("nan"))
        print(
            f"{pid:<{col_w[0]}}"
            f"{undelivered:>{col_w[1]}.2f}"
            f"{generated:>{col_w[2]}.2f}"
            f"{success_ratio:>{col_w[3]}.2f}"
            f"{req:>{col_w[4]}.2f}"
            f"{average_power:>{col_w[5]}.2f}"
            f"{total_power:>{col_w[6]}.2f}"
        )

    print(sep)
    print(f"  (Avg/Total power are per-simulation, shared across all packet IDs)")

# ── Comparison table ──────────────────────────────────────────────────────────

def print_comparison_table(all_results: list[tuple[str, dict]]):
    name_w = max(len(name) for name, _ in all_results) + 2
    col_w  = 16

    header = (
        f"{'Test':<{name_w}}"
        f"{'Undelivered':>{col_w}}"
        f"{'Generated':>{col_w}}"
        f"{'Dropped':>{col_w}}"
        f"{'Transmissions':>{col_w}}"
        f"{'Met Criteria':>{col_w}}"
        f"{'Avg Pwr (W)':>{col_w}}"
        f"{'Tot Pwr (W)':>{col_w}}"
    )
    sep = "-" * len(header)

    print(f"\n{'═' * len(header)}")
    print("  Scheduler Comparison")
    print(f"{'═' * len(header)}")
    print(header)
    print(sep)

    for name, m in all_results:
        tx_power      = m["tx_power"]
        avg_power     = sum(tx_power) / len(tx_power) if tx_power else 0.0
        total_power   = sum(tx_power)
        total_undel   = sum(m["undelivered_per_id"].values())
        total_gen     = sum(m["generated_per_id"].values())
        total_dropped = sum(m["cumulative_dropped"][-1:] or [0])

        pids     = sorted(m["per_id_req"].keys())
        met      = sum(
            1 for pid in pids
            if m["generated_per_id"].get(pid, 0) > 0
            and (1 - m["undelivered_per_id"].get(pid, 0) / m["generated_per_id"][pid]) >= m["per_id_req"][pid]
        )
        criteria = f"{met}/{len(pids)}"

        print(
            f"{name:<{name_w}}"
            f"{total_undel:>{col_w}.2f}"
            f"{total_gen:>{col_w}.2f}"
            f"{total_dropped:>{col_w}.2f}"
            f"{m['total_transmissions']:>{col_w}.2f}"
            f"{criteria:>{col_w}}"
            f"{avg_power:>{col_w}.2f}"
            f"{total_power:>{col_w}.2f}"
        )

    print(sep)

# ── Success criteria table ────────────────────────────────────────────────────

def print_success_criteria_table(all_results: list[tuple[str, dict]]):
    name_w = max(len(name) for name, _ in all_results) + 2
    id_w   = 6
    col_w  = 18

    header = (
        f"{'Test':<{name_w}}"
        f"{'ID':<{id_w}}"
        f"{'Success Ratio':>{col_w}}"
        f"{'Success Req':>{col_w}}"
        f"{'Delta':>{col_w}}"
        f"{'Met':>{col_w}}"
    )
    sep = "-" * len(header)

    print(f"\n{'═' * len(header)}")
    print("  Success Criteria per Test/ID")
    print(f"{'═' * len(header)}")
    print(header)
    print(sep)

    for name, m in all_results:
        pids = sorted(m["per_id_req"].keys())
        for pid in pids:
            generated     = m["generated_per_id"].get(pid, 0)
            undelivered   = m["undelivered_per_id"].get(pid, 0)
            success_ratio = (1 - undelivered / generated) if generated > 0 else 0.0
            req           = m["per_id_req"][pid]
            delta         = success_ratio - req
            met           = "YES" if success_ratio >= req else "NO"
            print(
                f"{name:<{name_w}}"
                f"{pid:<{id_w}}"
                f"{success_ratio:>{col_w}.2f}"
                f"{req:>{col_w}.2f}"
                f"{delta:>+{col_w}.2f}"
                f"{met:>{col_w}}"
            )
        print(sep)

# ── Schedulability sweep ──────────────────────────────────────────────────────

# Each sweep scenario fixes (n, c_min, c_max, sr_min, sr_max); U is swept.
SWEEP_SCENARIOS = [
    (4,  1, 3, 0.50, 0.70),
    (10, 1, 3, 0.50, 0.70),
    (20, 1, 3, 0.50, 0.70),
]

U_VALUES = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]

def schedulability_ratio(metrics: dict) -> float:
    """Fraction of packet IDs whose final success ratio met the requirement."""
    pids = sorted(metrics["per_id_req"].keys())
    if not pids:
        return 0.0
    met = 0
    for pid in pids:
        gen = metrics["generated_per_id"].get(pid, 0)
        if gen <= 0:
            continue
        ratio = 1 - metrics["undelivered_per_id"].get(pid, 0) / gen
        if ratio >= metrics["per_id_req"][pid]:
            met += 1
    return met / len(pids)

# ── Parallel-safe single run ──────────────────────────────────────────────────

def _run_single(args: tuple) -> dict:
    """Run one simulation in a temp file pair and return extracted metrics.
    Designed to be called from a worker process. Resolves UUniFast specs
    fresh per call so rounding drift averages over runs."""
    test, run_id = args
    config = make_config(test)
    cfg_file = f"/tmp/sim_config_{os.getpid()}_{run_id}.json"
    log_file = f"/tmp/sim_log_{os.getpid()}_{run_id}.json"
    try:
        with open(cfg_file, "w") as f:
            json.dump(config, f)
        result = subprocess.run([BINARY, cfg_file, log_file],
                                capture_output=True, check=False,
                                cwd=SCRIPT_DIR)
        if result.returncode != 0 or not os.path.exists(log_file):
            raise RuntimeError(
                f"{BINARY} failed (exit={result.returncode}) "
                f"for run {run_id}.\n"
                f"stderr: {result.stderr.decode(errors='replace')[-2000:]}\n"
                f"stdout: {result.stdout.decode(errors='replace')[-500:]}"
            )
        frames = load_results(log_file)
        return extract_metrics(frames, config)
    finally:
        for path in (cfg_file, log_file):
            try:
                os.remove(path)
            except FileNotFoundError:
                pass

# ── Multi-run averaging ───────────────────────────────────────────────────────

def average_metrics(runs: list[dict]) -> dict:
    """Average scalar table metrics across runs; use last run for plot data."""
    n    = len(runs)
    last = runs[-1]

    # Collect all pids seen across runs
    all_pids = set()
    for m in runs:
        all_pids |= set(m["undelivered_per_id"].keys())
        all_pids |= set(m["generated_per_id"].keys())

    undelivered_per_id = {
        pid: sum(m["undelivered_per_id"].get(pid, 0) for m in runs) / n
        for pid in all_pids
    }
    generated_per_id = {
        pid: sum(m["generated_per_id"].get(pid, 0) for m in runs) / n
        for pid in all_pids
    }
    total_transmissions = sum(m["total_transmissions"] for m in runs) / n
    total_dropped_final = sum(m["cumulative_dropped"][-1] if m["cumulative_dropped"] else 0 for m in runs) / n

    # For power: average the per-run tx_power lists element-wise (same length assumed)
    avg_tx_power = [
        sum(m["tx_power"][i] for m in runs) / n
        for i in range(len(last["tx_power"]))
    ]

    # Success-rate requirements vary per run now (drawn from sr_min..sr_max),
    # so average them per-id alongside the other metrics.
    per_id_req = {
        pid: sum(m["per_id_req"].get(pid, 0) for m in runs) / n
        for pid in all_pids
    }

    averaged = dict(last)  # copy plot data from last run
    averaged["undelivered_per_id"]  = undelivered_per_id
    averaged["generated_per_id"]    = generated_per_id
    averaged["per_id_req"]          = per_id_req
    averaged["total_transmissions"] = total_transmissions
    averaged["tx_power"]            = avg_tx_power
    # Patch cumulative_dropped final value used in comparison table
    averaged["cumulative_dropped"]  = last["cumulative_dropped"][:-1] + [total_dropped_final]

    return averaged

# ── Sweep runner ──────────────────────────────────────────────────────────────

def run_sweep(n_runs: int, n_workers: int) -> dict:
    """For each (scenario, U, scheduler), run n_runs sims in a single big pool.
    Returns nested dict results[scen_name][sch_name][U] = avg_schedulability."""
    jobs = []
    for n, c_min, c_max, sr_min, sr_max in SWEEP_SCENARIOS:
        scen_name = scenario_label(n, c_min, c_max, sr_min, sr_max)
        for U in U_VALUES:
            for sch_name, scheduler in SCHEDULERS:
                test = {
                    "name": f"{scen_name}_U={U:.2f}_{sch_name}",
                    "config": {
                        "simulation": BASE_SIM,
                        "scheduler":  scheduler,
                        "channels":   BASE_CHANNELS,
                    },
                    "uunifast": {
                        "U":      U, "n":      n,
                        "c_min":  c_min, "c_max":  c_max,
                        "sr_min": sr_min, "sr_max": sr_max,
                    },
                }
                for _ in range(n_runs):
                    jobs.append(((scen_name, U, sch_name), test))

    total = len(jobs)
    print(f"  [sweep] dispatching {total} sims "
          f"({len(SWEEP_SCENARIOS)} scen x {len(U_VALUES)} U x "
          f"{len(SCHEDULERS)} sch x {n_runs} runs)", flush=True)

    combo_runs = defaultdict(list)
    with ProcessPoolExecutor(max_workers=n_workers) as executor:
        futures = {
            executor.submit(_run_single, (test, job_idx)): key
            for job_idx, (key, test) in enumerate(jobs)
        }
        done = 0
        for future in as_completed(futures):
            key = futures[future]
            combo_runs[key].append(future.result())
            done += 1
            if done % max(1, total // 50) == 0 or done == total:
                print(f"  [sweep] {done}/{total} done", end="\r", flush=True)
    print()

    def percentile(xs, p):
        s = sorted(xs)
        if not s:
            return 0.0
        k = (len(s) - 1) * p / 100
        f = int(k)
        c = min(f + 1, len(s) - 1)
        return s[f] + (s[c] - s[f]) * (k - f)

    results = {scenario_label(*scen): {sch_name: {} for sch_name, _ in SCHEDULERS}
               for scen in SWEEP_SCENARIOS}
    for (scen_name, U, sch_name), runs in combo_runs.items():
        ratios = [schedulability_ratio(m) for m in runs]
        powers = [sum(m["tx_power"]) for m in runs]
        results[scen_name][sch_name][U] = {
            "sched_ratio":     sum(ratios) / len(ratios),
            "sched_ratio_lo":  percentile(ratios, 10),
            "sched_ratio_hi":  percentile(ratios, 90),
            "total_power":     sum(powers) / len(powers),
            "total_power_lo":  percentile(powers, 10),
            "total_power_hi":  percentile(powers, 90),
        }
    return results

def plot_schedulability(results: dict):
    colors     = plt.cm.tab10.colors
    linestyles = ["-", "--", "-.", ":"]
    markers    = ["o", "s", "^", "D", "v", "P", "X"]
    scen_names = list(results.keys())
    n_scen     = len(scen_names)

    fig, axes = plt.subplots(2, n_scen, figsize=(6 * n_scen, 9), sharex="col")
    if n_scen == 1:
        axes = axes.reshape(2, 1)

    n_sch    = len(SCHEDULERS)
    dx_step  = 0.012  # horizontal dodge between schedulers (in U units)

    for col, scen_name in enumerate(scen_names):
        ax_top = axes[0, col]
        ax_bot = axes[1, col]
        for i, (sch_name, u_to_metrics) in enumerate(results[scen_name].items()):
            xs       = sorted(u_to_metrics.keys())
            xs_dodge = [x + (i - (n_sch - 1) / 2) * dx_step for x in xs]
            ratio    = [u_to_metrics[u]["sched_ratio"]    for u in xs]
            ratio_lo = [max(0, r - u_to_metrics[u]["sched_ratio_lo"]) for u, r in zip(xs, ratio)]
            ratio_hi = [max(0, u_to_metrics[u]["sched_ratio_hi"] - r) for u, r in zip(xs, ratio)]
            power    = [u_to_metrics[u]["total_power"]    for u in xs]
            power_lo = [max(0, p - u_to_metrics[u]["total_power_lo"]) for u, p in zip(xs, power)]
            power_hi = [max(0, u_to_metrics[u]["total_power_hi"] - p) for u, p in zip(xs, power)]
            color    = colors[i % len(colors)]
            ls       = linestyles[i % len(linestyles)]
            mk       = markers[i % len(markers)]
            # Mean line (no dodge, runs through the true U) for clean shape
            ax_top.plot(xs, ratio, color=color, linestyle=ls, marker=mk,
                        markersize=7, linewidth=1.5, label=sch_name)
            ax_bot.plot(xs, power, color=color, linestyle=ls, marker=mk,
                        markersize=7, linewidth=1.5, label=sch_name)
            # Dodged percentile bars on top, no connector line
            ax_top.errorbar(xs_dodge, ratio, yerr=[ratio_lo, ratio_hi], fmt="none",
                            ecolor=color, elinewidth=1.2, capsize=3, alpha=0.55)
            ax_bot.errorbar(xs_dodge, power, yerr=[power_lo, power_hi], fmt="none",
                            ecolor=color, elinewidth=1.2, capsize=3, alpha=0.55)
        ax_top.set_title(scen_name)
        ax_top.set_ylim(-0.05, 1.05)
        ax_top.grid(True, alpha=0.3)
        ax_top.legend()
        ax_bot.set_xlabel("Utilization U")
        ax_bot.grid(True, alpha=0.3)
        ax_bot.legend()

    axes[0, 0].set_ylabel("Schedulability ratio (met / total)")
    axes[1, 0].set_ylabel("Total power (W)")
    plt.suptitle("Schedulability and Total Power vs Utilization", fontsize=13)
    plt.tight_layout()
    os.makedirs(TESTS_DIR, exist_ok=True)
    out = os.path.join(TESTS_DIR, "results_schedulability.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"Schedulability plot saved to {out}")

# ── Main ──────────────────────────────────────────────────────────────────────

def format_duration(seconds: float) -> str:
    h, rem = divmod(int(seconds), 3600)
    m, s   = divmod(rem, 60)
    if h:
        return f"{h}h {m}m {s}s"
    if m:
        return f"{m}m {s}s"
    return f"{seconds:.2f}s"

if __name__ == "__main__":
    n_runs           = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    mode             = sys.argv[2] if len(sys.argv) > 2 else "tests"
    run_name         = sys.argv[3] if len(sys.argv) > 3 else None
    belief_threshold = float(sys.argv[4]) if len(sys.argv) > 4 else None
    margin           = float(sys.argv[5]) if len(sys.argv) > 5 else None
    n_workers        = len(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else (os.cpu_count() or 1)

    if belief_threshold is not None:
        BASE_CATS_SCHEDULER["belief_threshold"] = belief_threshold
        print(f"CATS belief_threshold overridden to {belief_threshold}")

    if margin is not None:
        BASE_CATS_SCHEDULER["margin"] = margin
        print(f"CATS margin overridden to {margin}")

    if run_name:
        os.makedirs(run_name, exist_ok=True)
        os.chdir(run_name)
        print(f"Outputs will be written under: {os.path.abspath('.')}")

    t_start = time.perf_counter()

    if mode == "sweep":
        print(f"Running schedulability sweep ({n_runs} runs/point, {n_workers} workers)")
        results = run_sweep(n_runs, n_workers)
        plot_schedulability(results)
        print(f"Total elapsed: {format_duration(time.perf_counter() - t_start)}")
        sys.exit(0)

    print(f"Running {n_runs} simulation(s) per test ({n_workers} workers)")

    all_results = []

    for test in TESTS:
        if n_runs == 1:
            frames, display_config = run_test(test)
            run_metrics = [extract_metrics(frames, display_config)]
        else:
            print(f"  [{test['name']}] dispatching {n_runs} runs ...", flush=True)
            args = [(test, i) for i in range(n_runs)]
            run_metrics = [None] * n_runs
            with ProcessPoolExecutor(max_workers=n_workers) as executor:
                futures = {executor.submit(_run_single, a): i for i, a in enumerate(args)}
                done = 0
                for future in as_completed(futures):
                    run_metrics[futures[future]] = future.result()
                    done += 1
                    print(f"  [{test['name']}] {done}/{n_runs} done", end="\r", flush=True)
            print()
            display_config = make_config(test)

        metrics        = average_metrics(run_metrics) if n_runs > 1 else run_metrics[0]
        scheduler_type = test["config"]["scheduler"]["type"]
        plot_test(run_metrics[0], test["name"], scheduler_type)
        print_summary_table(test["name"], metrics, display_config)
        all_results.append((test["name"], metrics))

    if len(all_results) > 1:
        plot_comparison(all_results)
        print_comparison_table(all_results)
        print_success_criteria_table(all_results)

    print(f"\nTotal elapsed: {format_duration(time.perf_counter() - t_start)}")

