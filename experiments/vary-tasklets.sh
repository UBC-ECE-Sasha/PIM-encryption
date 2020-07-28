#!/bin/bash

# Author: Jacob Grossbard
#
# This script recompiles pimcrypto to use 2-24 tasklets, taking data for
# each.

data_file="data/vary_tasklets.csv"

echo "Tasklets,DPUs,Operation,Data size,Allocation time,Loading time,Data" \
" copy in,Parameter copy in,Launch,Data copy out,Performance count copy" \
" out,Free DPUs,Performance count min, max, average" > $data_file

for tasklets in {2..24}
do
	echo "Testing with $tasklets tasklets..."
	NR_TASKLETS=$tasklets make experiment > /dev/null
	./experiment/pimcrypto 64M 1 >> $data_file
done

