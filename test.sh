#!/bin/bash

echo "Hello, world!" > test.txt
truncate -s 64M test.txt
./$1 encrypt test.txt test.txt.enc
if ! cmp --silent test.txt test.txt.enc; then
	echo "Pass: input and encrypted files differ"
else
	echo "Fail: input and encrypted files are the same"
fi
./$1 decrypt test.txt.enc test.txt.dec
if cmp --silent test.txt test.txt.dec; then
	echo "Pass: input and decrypted files are the same"
else
	echo "Fail: input and decrypted files differ"
fi
rm test.txt test.txt.enc test.txt.dec

