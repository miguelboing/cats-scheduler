"""Real-life-replay sweep experiment.

Drives the same sweep machinery as `run_simulation.py` (UUniFast + parallel
worker pool + schedulability/energy plots) but on top of the ReplayChannel,
which feeds the predictor a fixed 0.07/0.58/0.80 view of 1/10/25 W (consumed
by CATS and by CHARM at its own tx_power) and replays the per-tick success
outcomes recorded in frame_success.csv.

Three scenarios (varying task count `n`), U swept over [0.1 .. 1.0], N runs
per (scenario, U, scheduler) point. Output PNGs land under
real_life_experiment/.

Two modes, mirroring run_simulation.py:
  sweep        — one pass at BASE_SIM["predict_error"], one curve per scheduler.
  error_sweep  — the same pass repeated at each level in rs.PREDICT_ERRORS,
                 so predictor-sensitive schedulers get one curve per level.

Usage:
  SEED=42 python real_life_experiment/run.py [n_runs] [sweep|error_sweep]
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
rs.BASE_SIM = {
    "duration":      160,   # capped by CSV length
    "predict_error": 0.20,  # ±half-width of uniform noise on each predicted decode probability
}

# error_sweep mode overrides predict_error per level and so ignores the 0.20
# above. Levels are inherited from run_simulation (PREDICT_ERRORS, currently
# [0.0, 0.15, 0.30]) to keep both experiments on the same axis; patch
# rs.PREDICT_ERRORS here if the replay experiment needs its own. The first
# level must be 0.0 — run_error_sweep asserts it, then dispatches the
# predictor-independent schedulers only at that level and replicates them
# across the rest.

rs.BASE_CHANNELS = [{
    "type":      "replay",
    "frequency": FREQ_HZ,
    # Resolved relative to SCRIPT_DIR (repo root) because the binary runs
    # with cwd=SCRIPT_DIR.
    "csv_path":  "real_life_experiment/frame_success.csv",
}]

# Fixed-power baselines run at both 10 W and 25 W — the two upper power levels
# CATS picks from — so a CATS curve can be read against a baseline spending
# comparable per-frame energy. The ReplayChannel resolves all three powers from
# the CSV, so 25 W replays real recorded outcomes rather than an extrapolation.
# Labels mirror run_simulation.SCHEDULERS so curves are named the same across
# both experiments.
rs.SCHEDULERS = [
    ("Rate_M_10W", {"type": "Rate_M", "tx_power": 10, "frequency": FREQ_HZ}),
    ("Rate_M_25W", {"type": "Rate_M", "tx_power": 25, "frequency": FREQ_HZ}),
    ("CHARM_10W",  {"type": "CHARM",  "tx_power": 10, "frequency": FREQ_HZ, "rx_period": 5}),
    ("CHARM_25W",  {"type": "CHARM",  "tx_power": 25, "frequency": FREQ_HZ, "rx_period": 5}),
    ("CATS",       {"type": "CATS",   "frequency": FREQ_HZ, "belief_threshold": 0.7, "utilization_threshold": 0.95}),
]

# Three scenarios — same axes as run_simulation's SWEEP_SCENARIOS, just
# with the 0.6-0.9 SR range and three task counts.
rs.SWEEP_SCENARIOS = [
    (4,  1, 3, 0.50, 0.90),
    (10, 1, 3, 0.50, 0.90),
    (20, 1, 3, 0.50, 0.90),
]

rs.U_VALUES = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]

# Plots land in real_life_experiment/ instead of tests/.
rs.TESTS_DIR = HERE


MODES = ("sweep", "error_sweep")


def main() -> None:
    n_runs    = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    mode      = sys.argv[2] if len(sys.argv) > 2 else "sweep"
    n_workers = (len(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity")
                 else (os.cpu_count() or 1))

    if mode not in MODES:
        sys.exit(f"unknown mode {mode!r} — expected one of {', '.join(MODES)}")

    if rs.MASTER_SEED is not None:
        print(f"SEED={rs.MASTER_SEED} — runs are reproducible")
    else:
        print("SEED unset — runs are non-deterministic "
              "(set SEED=<int> env var for reproducibility)")

    print(f"Replay {mode} — {n_runs} runs/point   {n_workers} workers   "
          f"duration={rs.BASE_SIM['duration']}   "
          f"scenarios={len(rs.SWEEP_SCENARIOS)}   U-points={len(rs.U_VALUES)}   "
          f"schedulers={len(rs.SCHEDULERS)}"
          + (f"   errors={rs.PREDICT_ERRORS}" if mode == "error_sweep"
             else f"   predict_error={rs.BASE_SIM['predict_error']}"))

    t_start = time.perf_counter()
    if mode == "error_sweep":
        rs.plot_error_sweep(rs.run_error_sweep(n_runs, n_workers))
    else:
        rs.plot_schedulability(rs.run_sweep(n_runs, n_workers))
    print(f"Total elapsed: {rs.format_duration(time.perf_counter() - t_start)}")


if __name__ == "__main__":
    main()
