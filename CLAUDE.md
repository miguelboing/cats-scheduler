# CATS Scheduler

C++ simulator for wireless packet scheduling under deadline + channel-quality constraints. Compares multiple schedulers (CATS, CHARM, CHEDF, EDF, Rate-Monotonic) against synthetic task sets, driven by a Python harness.

## Entry points

- `main.cpp` — single-run simulator. Reads `simulation_config.json` (or `argv[1]`), writes `simulation_log.json` (or `argv[2]`).
- `run_simulation.py` — Python harness that generates task sets via UUniFast, runs the C++ binary repeatedly across (scenario × utilization × scheduler × seed) points in parallel, and produces plots.
  - Usage: `python run_simulation.py <n_runs> <mode: tests|sweep> <run_name> [belief_threshold] [utilization_threshold]`
- `run_aire.sh` — SLURM submission script for the AIRE cluster (Leeds). Build must happen on a login node first; concurrent jobs would race on `main.o`.
- `analyze_low_u.py` — ad-hoc analysis script for low-utilization corner cases.
- `hf_experiment/run.py` — replay sweep. Reuses `run_simulation.run_sweep`/`plot_schedulability` but monkey-patches the module globals (`SCHEDULERS`, `ERROR_SWEEP_SCHEDULERS`, `BASE_CHANNELS`, `SWEEP_SCENARIOS`, `U_VALUES`, `TESTS_DIR`) to run over `ReplayChannel`, which replays per-tick outcomes from `frame_success.csv` and shows the predictor a fixed 0.07/0.58/0.80 view of 1/10/25 W. Keep both its rosters and labels in sync with `run_simulation.py` — it builds them from the `BASE_*_SCHEDULER` dicts with only `frequency` rebound, so they can't silently diverge.
  - Usage: `SEED=42 python hf_experiment/run.py <n_runs> [sweep|error_sweep]` (defaults: 5 runs, `sweep`). `error_sweep` overrides `predict_error` per level from `rs.PREDICT_ERRORS`, so `BASE_SIM["predict_error"]` only applies in `sweep` mode.

## Build

```bash
make            # produces ./main.o
make clean
```

The Makefile recurses into `system_model/`, `packet_generators/`, `schedulers/`, `physical_channels/`, each compiling `*.o` into the repo root, then links everything into `main.o`. After C++ edits, rebuild before running the Python harness.

Compiler: `g++` with `-Wall`, C++17 (uses `std::optional`). Headers from `libs/` (nlohmann/json, kovian).

## Layout

```
main.cpp                         # config-driven simulator entry
schedulers.hpp                   # umbrella include for all schedulers
schedulers/
  base_scheduler.hpp             # interface: schedule_frame()
  cats/                          # CATS — channel/belief-aware, this project's contribution
  charm/                         # CHARM baseline
  chedf/                         # CHARM's policy over an EDF queue (standalone copy)
  earliest_deadline_first/       # EDF baseline
  rate_monotonic/                # RM baseline
system_model/
  system_model.hpp               # shared types
  buffer_packet/                 # packet buffer + deadline enforcement
  radio_interface/               # transmitter side
  target_receiver/               # receiver side
  ml_predictor/                  # channel-state predictor used by CATS
packet_generators/
  base_packet_generator.hpp
  fixed_rate/                    # only generator currently wired in
physical_channels/
  base_physical_channel.hpp
  sigmoid_channel/               # SNR→success-prob sigmoid
  channel_20m/fsmc_states.json   # FSMC state data for the 20m band
libs/                            # vendored deps (nlohmann, kovian)
```

Each component subdir has its own Makefile invoked from the top-level one. To add a scheduler/generator/channel: subclass the matching `base_*.hpp`, drop a Makefile in a new subdir, and wire it into the parent Makefile (and `schedulers.hpp` for schedulers, plus the `sched_type` chain in `main.cpp`).

## Config format

`simulation_config.json` has four sections: `simulation` (duration in frames; optional `seed`), `scheduler` (type-specific params), `channels`, `packet_generators`. The Python harness rewrites this per run via `make_config()` in `run_simulation.py`. CATS-specific knobs: `belief_threshold`, `utilization_threshold`. CHARM and CHEDF both use `tx_power`, `rx_period`.

Packet fields: `id`, `period`, `relative_deadline`, `frames` (transmission length), `reliability` (required reception probability), `phase`.

A roster is a list of `(label, config)` pairs, where the label names the curve in plots and the config is what gets written to `simulation_config.json`. The same scheduler type can appear more than once at different parameters; the fixed-power baselines are run at both 10 W and 25 W (the two upper power levels CATS predicts over) so CATS can be compared against a baseline burning comparable energy. Labels must be unique — they key the results dicts.

**There are two rosters, and they are deliberately different.**

- `SCHEDULERS` (9) drives `sweep` and `tests`: `RM_10W`, `RM_25W`, `EDF_10W`, `EDF_25W`, `CHARM_10W`, `CHARM_25W`, `CHEDF_10W`, `CHEDF_25W`, `CATS`. `sweep` runs at `predict_error = 0` (`BASE_SIM`), so no gap on that figure can be blamed on prediction noise. The roster is the full 2×2 of the two effects under study — retransmission/power policy (RM/EDF vs CHARM/CHEDF) crossed with queue discipline (RM/CHARM vs EDF/CHEDF) — so each gap has its own control.
- `ERROR_SWEEP_SCHEDULERS` (5) drives `error_sweep`: `EDF_10W`, `EDF_25W`, `CHEDF_10W`, `CHEDF_25W`, `CATS`. That figure draws one curve per predictor-sensitive scheduler *per error level*, so the 9-roster would put ~21 curves on each axis. It uses the deadline-ordered variants throughout (CHEDF, EDF) so the surviving gaps are the power policy, not the queue discipline.

Anything reading a roster must pick the right one — `run_error_sweep`, `_draw_error_sweep_panel`, and `write_experiment_params` all switch on the mode. `_draw_scenario_panel` takes color/linestyle/marker straight from enumeration order of the data it is handed, not from `SCHEDULERS`, so it renders whatever roster it is given; its `linestyles`/`markers` lists must stay at least as long as the longest roster or two curves share a style.

Neither figure draws the 10th/90th-percentile bands any more — with 9 curves per axis the bands overlapped into an unreadable smear. `aggregate_runs` still records `<metric>_lo` / `<metric>_hi` in the results dict, so the spread is available offline.

**CHEDF is its own scheduler type and its own standalone class** (`schedulers/chedf/`, subclassing `BaseScheduler` directly), taking the same config parameters as CHARM: `tx_power`, `frequency`, `rx_period`.

⚠️ **`CHEDF_scheduler` is a deliberate copy of `CHARM_scheduler`, differing only in the `min_element` comparator.** Everything else — the accumulated-probability retransmission rule, the `rx_period` listening schedule, `receive_prediction()`, `get_prediction_powers()` — is duplicated verbatim, and the CHARM/CHEDF comparison is only interpretable while it stays that way. **Any change to that shared logic must be applied to both files.** Nothing in the build enforces this; the header on each class says so, and it is the one maintenance hazard of the split. (An earlier version had CHEDF subclass CHARM behind a virtual `select_packet()` hook, which made drift impossible; standalone classes were preferred for clarity.)

CHARM's comparator keeps its `is_periodic` guard (an aperiodic packet has no period to rank by, so CHARM declines to send when only such packets are queued); CHEDF's needs none, since a deadline is well defined for every packet.

Under overload the ordering difference is large and mostly shows up in `max_burst`: fixed-priority order starves long-period tasks indefinitely (RM at U=1.0 reached a ~1000-instance burst, and period-ordered CHARM ~3900, versus ~46 and ~93 for the deadline-ordered equivalents on the same task sets).

## Schedulability metrics

Four metrics are computed in `main.cpp`'s summary block from one pass over the per-instance delivered/not sequence, then aggregated by `extract_sweep_metrics()`. `SWEEP_METRICS` is what gets aggregated; `PANEL_METRICS` is the subset that gets *plotted*, and it drives both the sweep and error-sweep figures so their row layout can't drift apart. Rows, in order: **schedulability ratio, windowed schedulability, energy**. `hf_experiment/run.py` rebinds `PANEL_METRICS` to drop the windowed row (see below); the figures size themselves from `len(PANEL_METRICS)`, and `_window_k_suffix()` drops the `(m,k window k=…)` note from the title when that row is absent.

- `sched_ratio` — fraction of ids whose **whole-run** delivery rate meets `reliability`. Insensitive to *when* misses land: an early burst gets averaged away by a long clean tail.
- `sched_ratio_mk` — fraction of ids meeting the weakly-hard **(m,k)-firm** constraint for the *whole run*: at least `m = ceil(reliability * k)` of every `k` consecutive instances, **sliding** (not tumbling, so a burst straddling a boundary can't be masked in both halves). One violated window anywhere fails the id. Burst-sensitive; converges to `sched_ratio` as k grows.
- `max_burst` — longest run of consecutive undelivered instances. Parameter-free, so it can't be tuned; often the quantity a control loop actually cares about. **Aggregated but no longer plotted**: it spans three orders of magnitude across U (single digits at U=0.2, thousands under overload), which flattens the low-U end to an unreadable line on a shared linear axis. Read it from the raw results, or add it back to `PANEL_METRICS` with a log scale.
- `total_energy` — unchanged, sum of tx_power over all transmitted time-slots (W·time-slot).

`window_k` lives in `BASE_SIM` (**default 100**) and is overridable per run with the `WINDOW_K` env var.

**Why 100.** `make_test_set` draws `reliability` as a 2-decimal float, so at k=100 the threshold `m = ceil(req*100)` is exactly `req*100` — `m/k` reproduces the requirement with no rounding, and each task is judged against the rate it actually asked for. Deriving a *per-task* k from the fraction instead (`0.50 → 1/2`, `0.51 → 51/100`) was considered and rejected: the denominator is an artifact of the decimal representation, so a 0.01 change in the requirement would swing burst tolerance by ~50× (1 tolerated miss in 2 vs. 49 in 100).

**`m` must be computed with an epsilon.** `window_m()` in `run_simulation.py` and the matching line in `main.cpp` both use `ceil(req*k - 1e-9)`. A bare `ceil` overshoots by one wherever the 2-decimal float rounds up — `ceil(0.55*100) == 56` and `ceil(0.56*100) == 57`, both inside the sweep's RR band — silently holding those tasks to a stricter rate than requested. The epsilon is far below the smallest legitimate gap (1/100 at k=1). Keep the two implementations in sync.

Two constraints bound `k`, and `warn_window_k()` / `warn_vacuous()` report violations at runtime:

- **`k >= 1/(1 - reliability)`** or `m == k` and the window silently degenerates to a zero-miss requirement — much stricter than the long-run rate it was derived from. At `rr_max = 0.9` the floor is k=10, so k=100 clears it comfortably; this only bites on a `WINDOW_K` override.
- **`k <= instances per id`** or the id has no complete window. Such ids are scored **vacuously met** (never as a violation), which is the *only* way `sched_ratio_mk` can exceed `sched_ratio`. This is the binding constraint at k=100: at the default 250k duration it reaches **~22% of ids** in the worst cell (n=20, U=0.1) versus ~3% at k=20, and `warn_vacuous()` prints the grid-wide worst case every run. At short durations it dominates entirely: the replay experiment's 160 ticks give every id fewer than k instances, so the metric there is vacuously 1.0 everywhere and carries no information. **`hf_experiment/run.py` therefore drops the windowed row from its `PANEL_METRICS` and raises `VACUOUS_WARN_THRESHOLD` out of reach** (the warning is about a windowed curve it no longer draws). `sched_ratio_mk` is still aggregated into its results dict, just not plotted; if `BASE_SIM["duration"]` there ever grows past ~100 instances per id, restore both.

**`sched_ratio_mk` is not run-length invariant.** An id passes only if *every* window holds, so extending a run can only add chances to fail — per-id, the verdict is monotone non-increasing in `duration`. Comparisons between schedulers are therefore only valid at equal `duration`, and the absolute level of the curve is not portable across durations. `window_violation_rate` (fraction of all windows violated, already in `SWEEP_METRICS` though not plotted) is the run-length-robust companion if that matters.

`window_k` remains a free parameter — sweep it before drawing a conclusion from a gap between two schedulers.

## Reproducibility

Set `SEED=<int>` env var before running the harness: `SEED=42 python run_simulation.py ...`. A per-sim seed is then derived deterministically (blake2b of master seed + test name + run index), seeded into Python's `random` for UUniFast and propagated via `config["simulation"]["seed"]` to the C++ side, where `TargetReceiver`, `SigmoidChannel`, and the kovian `MarkovChain` each get distinct sub-seeds. Same `SEED` → bit-identical output PNGs; unset → clock-based (non-deterministic).

## Outputs

- `simulation_log.json` — per-tick log (large, ~8 MB for 5000-frame runs).
- `<scheduler>_scheduled_packets.json`, `generated_packets.json`, `received_packets.json` — per-component logs from `main.cpp`.
- `results_*.png` — plots produced by `run_simulation.py`.
- `*.json` and `*.png` are gitignored; don't commit them.

## Figure labels

Plots use the paper's symbols, spelled once as module constants at the top of `run_simulation.py` — `SYM_RR` (`$\theta_n$`, reliability requirement), `SYM_N` (`$N$`), `SYM_L` (`$L$`), `SYM_U` (`$U$`), `SYM_M` (`$m$`), `SYM_KWIN` (`$K_{\mathrm{win}}$`), `SYM_ERR` (`$\varepsilon$`, predictor error). Use the constant, never an inline `$…$`.

Rendering is matplotlib **mathtext** (STIX serif to match the text face), not a TeX install, so figures still draw on AIRE. `MPL_USETEX=1` switches the same strings to real LaTeX; that path additionally needs `type1ec.sty` (texlive-cm-super), which is *not* installed on the dev box — without it every `savefig` dies inside latex. `tex_safe()` escapes `_` etc. in scheduler labels and test names for that path and is a no-op otherwise; only ever pass it a plain string, never one that already contains a `SYM_*`.

**`scenario_label()` stays ASCII** — it keys the results dicts, names the test directories and names the PNGs. `scenario_title()` is the display-only form: it rewrites `U=`/`n=`/`L=`/`RR=` to the symbols at draw time and leaves anything else (e.g. a trailing `_CATS` on a test name) alone. Titles must go through it; filenames must not.

## Conventions

- Code is **cppcheck-compliant** (warning + style + performance + portability). CI runs static analysis from `.github/workflows/static-analysis.yml`. Don't introduce warnings.
- Shared mutable state is passed via `std::shared_ptr` (see `system_tick`, `buffer_packet`, `spawn_log` in `main.cpp`).
- Branch convention: `dev` for active work, PRs land on `main`.

## Things that have bitten in the past

- `run_aire.sh` does **not** build — concurrent SLURM jobs race on `main.o` (the Makefile `rm`s then recreates it, leaving a window where the binary is missing). Always `make` on a login node before submitting.
- UUniFast in `run_simulation.py` generates **deadlines from utilization** (D = floor(C/u)), not the other way around. Don't "fix" it back to fixed-D + computed-C — that drifts utilization upward.
- A scheduler's `get_prediction_powers()` must match the power it actually transmits at. CHARM used to hardcode `{10}` while transmitting at its configured `tx_power`, which silently made a 25 W CHARM decide retransmissions from the 10 W decode probability. It now returns `{tx_power}`. Any new fixed-power scheduler needs the same coupling.
- `get_name()` on CHARM, CHEDF, RM and EDF embeds the power (`CHARM_25W`) because those types run at two power levels and would otherwise overwrite each other's `*_scheduled_packets.json`. EDF returned a bare `"EDF"` until it was promoted to the roster at both powers. CHEDF is a separate class, but since it was copied from CHARM its `get_name()` is the easiest line to forget to change — the pair then collides on the same log.
- **Module globals do not reach the sweep workers.** `run_sweep` uses a multiprocessing *spawn* pool, so children re-import `run_simulation` from disk and see the file's values, not the parent's. Patching `rs.BINARY` (or any other global `_run_single_sweep` reads) from a driver script silently has no effect — both halves of an A/B run then execute the same binary and produce a fake null result. Swap the binary on disk instead. This is why `hf_experiment/run.py` only patches globals that the *parent* consults when building the `test` dicts.
- Anything special-casing a scheduler by *label* (e.g. the error sweep skipping predictor-independent schedulers at non-zero error) breaks the moment a variant is added. Key on `config["type"]` via `is_predictor_independent()` instead.
