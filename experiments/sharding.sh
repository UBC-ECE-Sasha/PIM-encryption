#!/bin/bash

# Author: Jacob Grossbard
#
#

data_file="data/sharding.csv"

echo "Tasklets,DPUs,Operation,Data size,Allocation time,Loading time,Data" \
" copy in,Parameter copy in,Launch,Data copy out,Performance count copy" \
" out,Free DPUs,Performance count min, max, average" > $data_file

make experiment  > /dev/null

for ranks in {1..8}
do
	max_data=$(($ranks*(1 << 32)))
	data=8192

	while [ $data -le $max_data ]
	do
		echo "Testing $operation with $data bytes across $ranks ranks..."
		./experiment/pimcrypto dpu $(($ranks * 64)) encrypt $data >> $data_file
		data=$(($data * 2))
	done
done
