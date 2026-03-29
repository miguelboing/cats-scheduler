import json
import subprocess
from dataclasses import dataclass, field
from typing import Optional

# ── Configuration ─────────────────────────────────────────────────────────────

CONFIG = {
    "simulation": {
        "duration": 500
    },
    "scheduler": {
        "type": "CHASPF",
        "tx_power": 10,
        "frequency": 14074000,
        "rx_period": 5
    },
    "packet_generators": [
        {
            "type": "fixed_rate",
            "packets": [
                { "id": 1, "relative_deadline": 5, "frames": 2, "success_rate": 0.9, "period": 5, "phase": 0 },
                { "id": 2, "relative_deadline": 4, "frames": 1, "success_rate": 0.7, "period": 4, "phase": 0 }
            ]
        }
    ],
    "channels": [
        {
            "type": "sigmoid",
            "name": "channel_20m",
            "frequency": 14074000
        }
    ]
}

CONFIG_FILE  = "simulation_config.json"
LOG_FILE     = "simulation_log.json"
BINARY       = "./main.o"

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
    probability: float
    tx_power: int
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
            probability = p["probability"],
            tx_power    = p["tx_power"],
            frequency   = p["frequency"]
        )

    return Frame(
        tick            = d["tick"],
        radio_mode      = d["radio_mode"],
        buffer          = [parse_packet(p) for p in d["buffer"]],
        fsmc            = [FsmcStatus(**ch) for ch in d["fsmc"]],
        missed_packets  = [parse_packet(p) for p in d["missed_packets"]],
        transmission    = transmission,
        prediction      = prediction
    )

# ── Main ──────────────────────────────────────────────────────────────────────

def write_config(config: dict, path: str):
    with open(path, "w") as f:
        json.dump(config, f, indent=4)
    print(f"Config written to {path}")

def run_simulation(binary: str, config_file: str):
    result = subprocess.run([binary, config_file], capture_output=True, text=True)
    if result.returncode != 0:
        print("Simulation failed:")
        print(result.stdout)
        print(result.stderr)
        raise RuntimeError("Simulation exited with code", result.returncode)
    print("Simulation completed successfully")

def load_results(log_file: str) -> list[Frame]:
    with open(log_file) as f:
        raw = json.load(f)
    return [parse_frame(entry) for entry in raw]

if __name__ == "__main__":
    write_config(CONFIG, CONFIG_FILE)
    run_simulation(BINARY, CONFIG_FILE)
    frames = load_results(LOG_FILE)

    print(f"\nLoaded {len(frames)} frames")
    for frame in frames:
        print(f"  Tick {frame.tick:3d} | {frame.radio_mode:<10}", end="")
        if frame.transmission:
            t = frame.transmission
            print(f" | pkt id={t.packet.id} id_count={t.packet.id_count}"
                  f" prob={t.probability:.3f} received={t.received}", end="")
        if frame.prediction:
            print(f" | predicted prob={frame.prediction.probability:.3f}", end="")
        if frame.missed_packets:
            ids = [(p.id, p.id_count) for p in frame.missed_packets]
            print(f" | MISSED={ids}", end="")
        fsmc_states = [f"freq={ch.frequency} state={ch.state}" for ch in frame.fsmc]
        print(f" | FSMC: {', '.join(fsmc_states)}")
