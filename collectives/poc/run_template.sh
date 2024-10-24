#!/bin/bash

#SBATCH -t 02:00:00
#SBATCH -n {0}
#SBATCH --ntasks-per-node 1
#SBATCH -J "poc_n{0}"
#SBATCH -o poc_n{0}_%j.out
#SBATCH -p fpga
#SBATCH -A hpc-lco-kenter
#SBATCH --constraint xilinx_u280_xrt2.15
#SBATCH -w n2fpga08,n2fpga04,n2fpga05

## Load environment modules
source env.sh

#https://pc2.github.io/fpgalink-gui/index.html?import={3}
changeFPGAlinksXilinx {2}
srun -l -n {1} --spread-job ../../scripts/reset.sh

srun -l -n {0} --spread-job ./poc
