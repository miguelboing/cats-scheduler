# Changelog

## v1.0

First complete release of the simulator and its experiment harness. `v0.5` ran a single EDF scheduler over a static channel from a hardcoded `main.cpp`; this release is a config-driven, reproducible, five-scheduler comparison framework. 161 commits.

### Project rename

- Renamed from cats-scheduler to **rt-link-sim**, after the simulator rather than one of the schedulers it compares.

### Schedulers

- Added **CATS**, the project's contribution: belief about the channel state plus a utilization test, choosing both when to transmit and at what power.
- Added **CHARM**, a channel-aware baseline with accumulated-probability retransmission over a period-ordered queue.
- Added **CHEDF**, CHARM's policy over a deadline-ordered queue, isolating queue discipline from power policy.
- Renamed `smaller_period_first` to **Rate-Monotonic** to match the literature.
- Extended **EDF** with the base scheduler interface, radio-mode handling and power-tagged naming.
- All four baselines now run at 10 W and 25 W, the levels CATS predicts over, for comparable-energy comparisons.

### Experiment harness

- Added `run_simulation.py`: UUniFast task sets, parallel sweeps, schedulability and energy plots.
- Three modes: `sweep`, `error_sweep` and `tests`.
- Deadlines are drawn from a target utilization rather than the reverse.
- Every parameter affecting a run is snapshotted next to its plots.
- Added `hf_experiment/run.py`, the same machinery over recorded per-tick channel outcomes.

### Simulator

- `main.cpp` is now config driven: scheduler, channels and generators all come from `simulation_config.json`.
- Config and log paths are overridable from the command line.
- Added summary mode, which silences stdout and emits only an aggregate, making long sweeps affordable in memory.

### Channel and predictor

- Added finite-state Markov channel modelling through the vendored kovian library, with 20 m band state data.
- Added a replay channel that replays recorded success outcomes, for validation against measured data.
- Sigmoid channel gained saturation control, externalised FSMC state and more realistic values.
- The predictor now reports decode probabilities at several transmission powers.
- Added configurable prediction error, so predictor robustness can be swept.

### Metrics

- Added `sched_ratio_mk`, a weakly-hard (m,k)-firm constraint over sliding windows, which catches bursts that a whole-run average hides.
- Added `max_burst`, the longest run of consecutive undelivered instances.
- Added `total_energy`, summed transmission power over transmitted slots.
- Kept `sched_ratio` as the whole-run delivery rate against the requirement.

### Reproducibility

- Setting `SEED` derives every per-simulation seed deterministically and propagates it into the receiver, the channel and the Markov chain.
- The same seed now reproduces identical figures; leaving it unset keeps clock-based behaviour.

### Cluster support

- Added `run_slurm.sh` for SLURM systems, with resource sizing notes and mode validation.
- Added `SLURM.md` as a submission and monitoring reference.
- The script deliberately does not build, since concurrent jobs would race on the binary.

### Figures

- Added vector PDF output alongside PNG, with Type 42 fonts so IEEE and ACM submission checks pass.
- Labels now use the paper's nomenclature for reliability requirement, packet length and power.
- Moved legends outside the axes, where nine curves no longer cover the data.
- Removed percentile bands for the same reason; the spread is still recorded in the results.
- Changed gridlines to dotted light gray on both axes.

### Terminology

- **Success rate is now reliability requirement** across the C++ code, config format, harness and figures.
- Packet transmission length renamed from C to L.
- Configs written for `v0.5` need updating.

### Architecture and build

- `system_model/` now holds the buffer, radio interface, target receiver and predictor.
- The transmitter sits behind a `RadioInterface`; the receiver was renamed `TargetReceiver`.
- Physical channels moved out of `system_model/` into a top-level `physical_channels/` with a base class.
- Per-directory Makefiles throughout, plus `schedulers.hpp` as an umbrella include.
- Reduced the memory footprint of sweep workers and speeded up compilation.

### Fixes

- CHARM decided retransmissions from a hardcoded 10 W probability while transmitting at its configured power.
- A misspelled harness mode fell through to `tests`, the heaviest mode, and OOM-killed long cluster jobs; unknown modes are now rejected up front.
- Fixed-power schedulers embed their power in `get_name()`, so runs at different powers no longer overwrite each other's logs.
- Corrected the per-frame probability requirement in CATS and CHARM.
- Restored cppcheck compliance and made static analysis pass on a clean tree.

### Documentation

- Rewrote the README around the current architecture.
- Added `CLAUDE.md` with design rationale and known traps.
- Added `SLURM.md` as a cluster reference.

## v0.5

Prototype: EDF scheduler, sigmoid channel, JSON logging, hardcoded simulation in `main.cpp`.
