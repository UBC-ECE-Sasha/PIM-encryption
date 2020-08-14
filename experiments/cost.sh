#!/bin/bash

# Author: Jacob Grossbard
#
# This script executes pimcrypto on the host with a variable amount of data, and on the DPUs with a variable number of ranks and a variable amount of data
#

data_file="data/cost.csv"
make experiment  > /dev/null
:'
echo "Operation,Data size, execution time\n" > $data_file

data_size=16


while [ $data_size -le $((1 << 35)) ]
do
	echo "Testing with $data_size bytes on the host..."
	./experiment/pimcrypto host encrypt $data_size >> $data_file
	data_size=$(($data_size * 2))
done
'

echo "Tasklets,DPUs,Operation,Data size,Allocation time,Loading time,Data" \
" copy in,Parameter copy in,Launch,Data copy out,Performance count copy" \
" out,Free DPUs,Performance count min, max, average" > $data_file

for ranks in {1..9}; do
	data_per_dpu=16
	while [ $data_per_dpu -lt $((2 << 26)) ]
	do
		echo "Testing with $data_per_dpu bytes per DPU across $ranks ranks..."
		dpus=$((64 * $ranks))
		./experiment/pimcrypto dpu $dpus encrypt $(($ranks * 64 * $data_per_dpu)) >> $data_file
		data_per_dpu=$(($data_per_dpu * 2))
	done
done

