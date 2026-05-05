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
#     sbatch run_aire.sh <run_name>
# Override defaults via --export, e.g.:
#     sbatch --export=N_RUNS=100,MODE=sweep,ALL run_aire.sh <run_name>

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
make

# --- Run --------------------------------------------------------------------
# Stop BLAS/OMP from oversubscribing cores already taken by the worker pool.
export OMP_NUM_THREADS=1
export MKL_NUM_THREADS=1
export OPENBLAS_NUM_THREADS=1

N_RUNS="${N_RUNS:-50}"   # runs averaged per (scenario, U, scheduler) point
MODE="${MODE:-sweep}"    # tests | sweep
RUN_NAME="${1:-run-${SLURM_JOB_ID}}"

echo "Job ${SLURM_JOB_ID} | ${SLURM_CPUS_PER_TASK} CPUs | mode=${MODE} | n_runs=${N_RUNS} | name=${RUN_NAME}"
python run_simulation.py "${N_RUNS}" "${MODE}" "${RUN_NAME}"
