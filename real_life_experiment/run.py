"""Real-life-replay sweep experiment.

Drives the same sweep machinery as `run_simulation.py` (UUniFast + parallel
worker pool + schedulability/energy plots) but on top of the ReplayChannel,
which feeds CATS's predictor a fixed 0.07/0.58/0.80 view of 1/10/25 W and
replays the per-tick success outcomes recorded in frame_success.csv.

Three scenarios (varying task count `n`), U swept over [0.1 .. 1.0], N runs
per (scenario, U, scheduler) point. Output PNGs land under
real_life_experiment/.

Usage:
  SEED=42 python real_life_experiment/run.py [n_runs]
"""

import os
import sys
import time

HERE      = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(HERE)
sys.path.insert(0, REPO_ROOT)

# Importing run_simulation reads SEED env var at import time, so SEED must
# be set before this line for reproducibility.
import run_simulation as rs  # noqa: E402

FREQ_HZ = 14_074_000

# ── Sweep configuration ──────────────────────────────────────────────────
# Monkey-patch the run_simulation module globals that `run_sweep` and
# `plot_schedulability` read by name at call time. Workers (spawn-context
# children) re-import the module fresh, but they only execute
# `_run_single_sweep` on a fully-formed `test` dict — they don't consult
# any of the patched globals — so the overrides only need to live in the
# parent process.
rs.BASE_SIM = {"duration": 160}  # capped by CSV length

rs.BASE_CHANNELS = [{
    "type":      "replay",
    "frequency": FREQ_HZ,
    # Resolved relative to SCRIPT_DIR (repo root) because the binary runs
    # with cwd=SCRIPT_DIR.
    "csv_path":  "real_life_experiment/frame_success.csv",
}]

rs.SCHEDULERS = [
    ("Rate_M", {"type": "Rate_M", "tx_power": 10, "frequency": FREQ_HZ}),
    ("CHARM",  {"type": "CHARM",  "tx_power": 10, "frequency": FREQ_HZ, "rx_period": 5}),
    ("CATS",   {"type": "CATS",   "frequency": FREQ_HZ, "belief_threshold": 0.7, "utilization_threshold": 0.95}),
]

# Three scenarios — same axes as run_simulation's SWEEP_SCENARIOS, just
# with the 0.6-0.9 SR range and three task counts.
rs.SWEEP_SCENARIOS = [
    (4,  1, 3, 0.60, 0.90),
    (10,  1, 3, 0.60, 0.90),
    (20, 1, 3, 0.60, 0.90),
]

rs.U_VALUES = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]

# Plots land in real_life_experiment/ instead of tests/.
rs.TESTS_DIR = HERE


def main() -> None:
    n_runs    = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    n_workers = (len(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity")
                 else (os.cpu_count() or 1))

    if rs.MASTER_SEED is not None:
        print(f"SEED={rs.MASTER_SEED} — runs are reproducible")
    else:
        print("SEED unset — runs are non-deterministic "
              "(set SEED=<int> env var for reproducibility)")

    print(f"Replay sweep — {n_runs} runs/point   {n_workers} workers   "
          f"duration={rs.BASE_SIM['duration']}   "
          f"scenarios={len(rs.SWEEP_SCENARIOS)}   U-points={len(rs.U_VALUES)}   "
          f"schedulers={len(rs.SCHEDULERS)}")

    t_start = time.perf_counter()
    results = rs.run_sweep(n_runs, n_workers)
    rs.plot_schedulability(results)
    print(f"Total elapsed: {rs.format_duration(time.perf_counter() - t_start)}")


if __name__ == "__main__":
    main()
