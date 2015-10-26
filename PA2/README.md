# Distributed File System

Unfortunatly I only programmed one server to work at a time, so it isn't distributed :(

## To Run:

run 'make'

Server: 
./dfs /DFS1 10001 &
./dfs /DFS1 10002 &
./dfs /DFS1 10003 &
./dfs /DFS1 10004 &
OR
./test.sh

Client: 
./dfc dfc.conf or ./dfc dfc1.conf

##Commands:  
LIST
GET <filename>
PUT <filename>

Replace File Name with the desired file.


##Resources used:

http://www.binarytides.com/server-client-example-c-sockets-linux/
http://beej.us/guide/bgnet/output/html/multipage/index.html