#!/bin/bash

# Author: Jacob Grossbard
#
# This script executes pimcrypto on the host with amounts of data between 16B and 40GB
#

data_file="data/vary_data_host.csv"

echo "Operation,Data size, execution time\n" > $data_file

data_size=16

make experiment  > /dev/null

while [ $data_size -le $((1 << 30)) ]
do
	echo "Testing with $data_size bytes on the host..."
	./experiment/pimcrypto host encrypt $data_size >> $data_file
	data_size=$(($data_size * 2))
done

