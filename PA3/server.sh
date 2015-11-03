#!/bin/bash

# make sure to run 'chmod u+x test.sh' before runing this
echo "killing old processes..."
pkill -f proxy

make clean
make

./proxy 10001