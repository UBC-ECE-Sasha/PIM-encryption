#!/bin/bash

# Author: Jacob Grossbard
#
# This script executes pimcrypto with various numbers of ranks and a variable
# amount of data per DPU.
#

data_file="data/vary_ranks.csv"

echo "Tasklets,DPUs,Operation,Data size,Allocation time,Loading time,Data" \
" copy in,Parameter copy in,Launch,Data copy out,Performance count copy" \
" out,Free DPUs,Performance count min, max, average" > $data_file

make experiment  > /dev/null

for operation in encrypt decrypt;
do
	for ranks in {1..8}
	do
		data_per_dpu=16
		while [ $data_per_dpu -lt $((2 << 26)) ]
		do
			echo "Testing $operation with $data_per_dpu bytes per DPU across $ranks ranks..."
			./experiment/pimcrypto dpu $(($ranks * 64)) $operation $((64 * $ranks * $data_per_dpu)) >> $data_file
			data_per_dpu=$(($data_per_dpu * 2))
		done
	done
done
