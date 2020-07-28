#!/bin/bash

# Author: Jacob Grossbard
#
# This script executes pimcrypto with a constant number of DPUs and a variable
# amount of data per DPU. It is designed to show the growth in throughput per
# DPU as the amount of data per DPU increases.
#

data_file="data/vary_data_per_dpu.csv"

echo "Tasklets,DPUs,Operation,Data size,Allocation time,Loading time,Data" \
" copy in,Parameter copy in,Launch,Data copy out,Performance count copy" \
" out,Free DPUs,Performance count min, max, average" > $data_file

dpus=64
data_per_dpu=128

while [ $data_per_dpu -lt $((2 << 26)) ]
do
	echo "Testing with $data_per_dpu bytes per DPU across $dpus DPUs..."
	make experiment  > /dev/null
	./experiment/pimcrypto $(($dpus * $data_per_dpu)) $dpus >> $data_file
	data_per_dpu=$(($data_per_dpu * 2))
done

