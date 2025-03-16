#!/usr/bin/bash

source env.sh

make clean

base_path=`pwd`

for mode in 0 1; do
    for width in 32 64; do
        path=${base_path}_${mode}_${width}
        rm -rf ${path}
        cp -r ${base_path} ${path}

        cd ${path}
        sbatch ./scripts/synth.sh xclbin USE_FRAMING=${mode} FIFO_WIDTH=${width}
    done
done
