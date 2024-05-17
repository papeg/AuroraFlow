#!/bin/bash

#SBATCH -t 02:00:00
#SBATCH -n 1
#SBATCH -N 1
#SBATCH -J "arc_hw_emu"
#SBATCH -p normal
#SBATCH --cpus-per-task 16
#SBATCH -A hpc-lco-kenter

## Load environment modules
source env.sh

XCL_EMULATION_MODE=hw_emu ./build/host_test_collectives
