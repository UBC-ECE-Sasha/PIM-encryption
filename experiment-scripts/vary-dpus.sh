#!/bin/bash

# Author: Jacob Grossbard
#
# This script executes pimcrypto with 64MB of data on a variable number of DPUs.
# It is designed to show how the speed of encryption scales with the number of
# DPUs, until the overhead of extra DPUs outweighs the speed benefit when there
# is too little data per DPU.
#

data_file="data/vary_dpus.csv"

echo "Tasklets,DPUs,Operation,Data size,Allocation time,Loading time,Data" \
" copy in,Parameter copy in,Launch,Data copy out,Performance count copy" \
" out,Free DPUs,Performance count min, max, average" > $data_file

dpu_numbers=( 1 2 4 8 16 32 64 128 256 512 )

for dpus in "${dpu_numbers[@]}"
do
	echo "Testing with $dpus DPUs..."
	make experiment  > /dev/null
	./experiment/pimcrypto 64M $dpus >> $data_file
done

