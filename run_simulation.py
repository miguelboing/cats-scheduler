import json
import sys
import os
import subprocess
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass
from typing import Optional
from collections import defaultdict
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

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
    "rx_period": 5
}

BASE_CHANNELS = [
    {
        "type": "sigmoid",
        "name": "channel_20m",
        "frequency": 14074000
    }
]

TESTS = [
    # ── Very relaxed ──────────────────────────────────────────────────────────
    {
        "name": "2_packets_very_relaxed_Rate_M",
        "config": {
            "simulation": { "duration": 500 },
            "scheduler": { "type": "Rate_M", "tx_power": 10, "frequency": 14074000 },
            "channels": BASE_CHANNELS,
            "packet_generators": [
                {
                    "type": "fixed_rate",
                    "packets": [
                        { "id": 1, "relative_deadline": 10, "frames": 2, "success_rate": 0.6, "period": 10, "phase": 0 },
                        { "id": 2, "relative_deadline": 8,  "frames": 1, "success_rate": 0.5, "period": 8,  "phase": 0 }
                    ]
                }
            ]
        }
    },
    {
        "name": "2_packets_very_relaxed_CHARM",
        "config": {
            "simulation": { "duration": 500 },
            "scheduler": BASE_SCHEDULER,
            "channels": BASE_CHANNELS,
            "packet_generators": [
                {
                    "type": "fixed_rate",
                    "packets": [
                        { "id": 1, "relative_deadline": 10, "frames": 2, "success_rate": 0.6, "period": 10, "phase": 0 },
                        { "id": 2, "relative_deadline": 8,  "frames": 1, "success_rate": 0.5, "period": 8,  "phase": 0 }
                    ]
                }
            ]
        }
    },
    {
        "name": "2_packets_very_relaxed_CATS",
        "config": {
            "simulation": { "duration": 500 },
            "scheduler": BASE_CATS_SCHEDULER,
            "channels": BASE_CHANNELS,
            "packet_generators": [
                {
                    "type": "fixed_rate",
                    "packets": [
                        { "id": 1, "relative_deadline": 10, "frames": 2, "success_rate": 0.6, "period": 10, "phase": 0 },
                        { "id": 2, "relative_deadline": 8,  "frames": 1, "success_rate": 0.5, "period": 8,  "phase": 0 }
                    ]
                }
            ]
        }
    },
    # ── Relaxed ───────────────────────────────────────────────────────────────
    {
        "name": "2_packets_relaxed_Rate_M",
        "config": {
            "simulation": { "duration": 500 },
            "scheduler": { "type": "Rate_M", "tx_power": 10, "frequency": 14074000 },
            "channels": BASE_CHANNELS,
            "packet_generators": [
                {
                    "type": "fixed_rate",
                    "packets": [
                        { "id": 1, "relative_deadline": 5, "frames": 2, "success_rate": 0.9, "period": 5, "phase": 0 },
                        { "id": 2, "relative_deadline": 4, "frames": 1, "success_rate": 0.7, "period": 4, "phase": 0 }
                    ]
                }
            ]
        }
    },
    {
        "name": "2_packets_relaxed_CHARM",
        "config": {
            "simulation": { "duration": 500 },
            "scheduler": BASE_SCHEDULER,
            "channels": BASE_CHANNELS,
            "packet_generators": [
                {
                    "type": "fixed_rate",
                    "packets": [
                        { "id": 1, "relative_deadline": 5, "frames": 2, "success_rate": 0.9, "period": 5, "phase": 0 },
                        { "id": 2, "relative_deadline": 4, "frames": 1, "success_rate": 0.7, "period": 4, "phase": 0 }
                    ]
                }
            ]
        }
    },
    {
        "name": "2_packets_relaxed_CATS",
        "config": {
            "simulation": { "duration": 500 },
            "scheduler": BASE_CATS_SCHEDULER,
            "channels": BASE_CHANNELS,
            "packet_generators": [
                {
                    "type": "fixed_rate",
                    "packets": [
                        { "id": 1, "relative_deadline": 5, "frames": 2, "success_rate": 0.9, "period": 5, "phase": 0 },
                        { "id": 2, "relative_deadline": 4, "frames": 1, "success_rate": 0.7, "period": 4, "phase": 0 }
                    ]
                }
            ]
        }
    },
    # ── Stressed ──────────────────────────────────────────────────────────────
    {
        "name": "3_packets_stressed_Rate_M",
        "config": {
            "simulation": { "duration": 500 },
            "scheduler": { "type": "Rate_M", "tx_power": 10, "frequency": 14074000 },
            "channels": BASE_CHANNELS,
            "packet_generators": [
                {
                    "type": "fixed_rate",
                    "packets": [
                        { "id": 1, "relative_deadline": 3, "frames": 2, "success_rate": 0.95, "period": 3, "phase": 0 },
                        { "id": 2, "relative_deadline": 4, "frames": 1, "success_rate": 0.85, "period": 4, "phase": 1 },
                        { "id": 3, "relative_deadline": 6, "frames": 3, "success_rate": 0.75, "period": 6, "phase": 2 }
                    ]
                }
            ]
        }
    },
    {
        "name": "3_packets_stressed_CHARM",
        "config": {
            "simulation": { "duration": 500 },
            "scheduler": BASE_SCHEDULER,
            "channels": BASE_CHANNELS,
            "packet_generators": [
                {
                    "type": "fixed_rate",
                    "packets": [
                        { "id": 1, "relative_deadline": 3, "frames": 2, "success_rate": 0.95, "period": 3, "phase": 0 },
                        { "id": 2, "relative_deadline": 4, "frames": 1, "success_rate": 0.85, "period": 4, "phase": 1 },
                        { "id": 3, "relative_deadline": 6, "frames": 3, "success_rate": 0.75, "period": 6, "phase": 2 }
                    ]
                }
            ]
        }
    },
    {
        "name": "3_packets_stressed_CATS",
        "config": {
            "simulation": { "duration": 500 },
            "scheduler": BASE_CATS_SCHEDULER,
            "channels": BASE_CHANNELS,
            "packet_generators": [
                {
                    "type": "fixed_rate",
                    "packets": [
                        { "id": 1, "relative_deadline": 3, "frames": 2, "success_rate": 0.95, "period": 3, "phase": 0 },
                        { "id": 2, "relative_deadline": 4, "frames": 1, "success_rate": 0.85, "period": 4, "phase": 1 },
                        { "id": 3, "relative_deadline": 6, "frames": 3, "success_rate": 0.75, "period": 6, "phase": 2 }
                    ]
                }
            ]
        }
    }
]

CONFIG_FILE = "simulation_config.json"
LOG_FILE    = "simulation_log.json"
BINARY      = "./main.o"

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

def run_simulation(binary: str, config_file: str):
    result = subprocess.run([binary, config_file], capture_output=True, text=True)
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
    print(f"   Scheduler  : {sched['type']}{tx_power_str}  freq={sched['frequency']}Hz{rx_period_str}")
    print(f"   Duration   : {cfg['simulation']['duration']} ticks")
    print(f"   Channels   : {', '.join(ch['name'] for ch in cfg['channels'])}")
    print(f"   Packets:")
    for gen in cfg["packet_generators"]:
        for p in gen["packets"]:
            print(f"     id={p['id']}  period={p['period']}  deadline={p['relative_deadline']}"
                  f"  frames={p['frames']}  success_rate={p['success_rate']}  phase={p['phase']}")

def run_test(test: dict) -> list[Frame]:
    print_test_summary(test)
    write_config(test["config"], CONFIG_FILE)
    run_simulation(BINARY, CONFIG_FILE)
    frames = load_results(LOG_FILE)
    print(f"   Done — {len(frames)} frames loaded")
    return frames

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

    # 1. FSMC state (aggregate paired states to channel quality: 0=best .. 3=worst)
    quality = [(6 - s) if s is not None and s > 3 else s for s in m["fsmc_state"]]
    ax1 = fig.add_subplot(gs[0, :])
    ax1.plot(ticks, quality, color="steelblue", linewidth=0.8)
    ax1.set_ylabel("Channel Quality")
    ax1.set_xlabel("Tick")
    ax1.set_title("FSMC State Evolution")
    ax1.set_yticks(range(4))
    ax1.set_yticklabels(["Best", "2nd Best", "2nd Worst", "Worst"])
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
    out = f"results_{test_name}.png"
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

    # 4. FSMC state evolution per test (aggregated to channel quality)
    ax4 = fig.add_subplot(gs[1, 1])
    for i, (name, m) in enumerate(results):
        quality = [(6 - s) if s is not None and s > 3 else s for s in m["fsmc_state"]]
        ax4.plot(m["ticks"], quality, color=colors[i], linewidth=0.6, alpha=0.8, label=name)
    ax4.set_ylabel("Channel Quality")
    ax4.set_xlabel("Tick")
    ax4.set_title("FSMC State Evolution")
    ax4.set_yticks(range(4))
    ax4.set_yticklabels(["Best", "2nd Best", "2nd Worst", "Worst"])
    ax4.invert_yaxis()
    ax4.legend()
    ax4.grid(True, alpha=0.3)

    plt.suptitle("Test Comparison", fontsize=13)
    plt.savefig("results_comparison.png", dpi=150, bbox_inches="tight")
    plt.close()
    print("Comparison plot saved to results_comparison.png")

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

# ── Parallel-safe single run ──────────────────────────────────────────────────

def _run_single(args: tuple) -> dict:
    """Run one simulation in a temp file pair and return extracted metrics.
    Designed to be called from a worker process."""
    config, run_id = args
    cfg_file = f"/tmp/sim_config_{os.getpid()}_{run_id}.json"
    log_file = f"/tmp/sim_log_{os.getpid()}_{run_id}.json"
    try:
        with open(cfg_file, "w") as f:
            json.dump(config, f)
        result = subprocess.run([BINARY, cfg_file, log_file],
                                capture_output=True, check=False)
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

    averaged = dict(last)  # copy plot data from last run
    averaged["undelivered_per_id"]  = undelivered_per_id
    averaged["generated_per_id"]    = generated_per_id
    averaged["total_transmissions"] = total_transmissions
    averaged["tx_power"]            = avg_tx_power
    # Patch cumulative_dropped final value used in comparison table
    averaged["cumulative_dropped"]  = last["cumulative_dropped"][:-1] + [total_dropped_final]

    return averaged

# ── Main ──────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    n_runs   = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    n_workers = os.cpu_count() or 1
    print(f"Running {n_runs} simulation(s) per test ({n_workers} workers)")

    all_results = []

    for test in TESTS:
        if n_runs == 1:
            frames     = run_test(test)
            run_metrics = [extract_metrics(frames, test["config"])]
        else:
            print(f"  [{test['name']}] dispatching {n_runs} runs ...", flush=True)
            args = [(test["config"], i) for i in range(n_runs)]
            run_metrics = [None] * n_runs
            with ProcessPoolExecutor(max_workers=n_workers) as executor:
                futures = {executor.submit(_run_single, a): i for i, a in enumerate(args)}
                done = 0
                for future in as_completed(futures):
                    run_metrics[futures[future]] = future.result()
                    done += 1
                    print(f"  [{test['name']}] {done}/{n_runs} done", end="\r", flush=True)
            print()

        metrics        = average_metrics(run_metrics) if n_runs > 1 else run_metrics[0]
        scheduler_type = test["config"]["scheduler"]["type"]
        plot_test(run_metrics[0], test["name"], scheduler_type)
        print_summary_table(test["name"], metrics, test["config"])
        all_results.append((test["name"], metrics))

    if len(all_results) > 1:
        plot_comparison(all_results)
        print_comparison_table(all_results)
        print_success_criteria_table(all_results)

