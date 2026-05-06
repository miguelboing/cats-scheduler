#!/bin/bash
#SBATCH --mail-type=BEGIN,END,FAIL
#SBATCH --mail-user=gcsv9491@leeds.ac.uk
#SBATCH --job-name=cats-sweep
#SBATCH --output=cats-%j.out
#SBATCH --error=cats-%j.err
#SBATCH --time=04:00:00
#SBATCH --mem-per-cpu=2G
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32

# Submit with:
#     sbatch run_aire.sh <run_name> [belief_threshold] [margin]
# Override defaults via --export, e.g.:
#     sbatch --export=N_RUNS=100,MODE=sweep,ALL run_aire.sh <run_name> [belief_threshold] [margin]

set -euo pipefail

# --- Modules ----------------------------------------------------------------
# Adjust names to whatever AIRE actually exposes (`module avail` to check).
module load miniforge
module load gcc

# --- Python env -------------------------------------------------------------
# Create once on a login node:  conda create -n cats-scheduler python=3.12 numpy matplotlib
conda activate cats-scheduler

# --- Workdir ----------------------------------------------------------------
cd "${SLURM_SUBMIT_DIR}"

# --- Build ------------------------------------------------------------------
# Build is intentionally NOT done here — concurrent jobs would race on main.o
# (the Makefile rm's then re-creates it, leaving a window where it's missing).
# Run `make` on a login node before submitting, and after any C++ edits.
if [ ! -x main.o ]; then
    echo "ERROR: main.o not found. Run 'make' on a login node before submitting." >&2
    exit 1
fi

# --- Run --------------------------------------------------------------------
# Stop BLAS/OMP from oversubscribing cores already taken by the worker pool.
export OMP_NUM_THREADS=1
export MKL_NUM_THREADS=1
export OPENBLAS_NUM_THREADS=1

N_RUNS="${N_RUNS:-50}"   # runs averaged per (scenario, U, scheduler) point
MODE="${MODE:-sweep}"    # tests | sweep
RUN_NAME="${1:-run-${SLURM_JOB_ID}}"
BELIEF_THRESHOLD="${2:-}"   # optional; if empty, run_simulation.py uses its default
MARGIN="${3:-}"             # optional; if empty, run_simulation.py uses its default
# Note: passing margin requires belief_threshold to be set (positional args).

echo "Job ${SLURM_JOB_ID} | ${SLURM_CPUS_PER_TASK} CPUs | mode=${MODE} | n_runs=${N_RUNS} | name=${RUN_NAME} | belief=${BELIEF_THRESHOLD:-default} | margin=${MARGIN:-default}"
python run_simulation.py "${N_RUNS}" "${MODE}" "${RUN_NAME}" ${BELIEF_THRESHOLD:+"${BELIEF_THRESHOLD}"} ${MARGIN:+"${MARGIN}"}
