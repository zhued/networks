#!/bin/bash

# make sure to run 'chmod u+x test.sh' before runing this
make
echo "killing old processes..."
pkill -f dfs
pkill -f dfc

# # read -r -p "Do you want to start the dfs servers? [y/N] " response
# # if [[ $response =~ ^([yY][eE][sS]|[yY])$ ]]
# # then
#     # ./dfs "/DFS1" "10001" &
#     # ./dfs "/DFS2" "10002" &
#     # ./dfs "/DFS3" "10003" &
#     # ./dfs "/DFS4" "10004" &
#     # echo "done..."
# # else
#     # echo "done..."
# # fi

./dfs /DFS1 10001
# ./dfs /DFS2 10002