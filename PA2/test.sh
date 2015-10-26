#!/bin/bash

# make sure to run 'chmod u+x test.sh' before runing this
echo "killing old processes..."
pkill -f dfs
pkill -f dfc

make

./dfs /DFS1 10001 &
./dfs /DFS2 10002 &
./dfs /DFS3 10003 &
./dfs /DFS4 10004 &