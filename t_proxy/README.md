# Transparent Proxy Server - Edward Zhu

Created a transparent proxy server between a client and server that will implement iptables rules and allow telneting between two hosts on different hosts/ports.

**NOTE:** You will have to set up 3 VMS, each configured specifically in a way so it all connects together. That took us over 20 hours to make it work. I hated every second of it. Then the coding portion, will take another 20+ hours, without even knowing what the end goal of the assignment is. Was not a fun time. Entire assignment was very confusing.

## To Run:

```bash
run 'make'

#Build Proxy: 
./proxy

```

## How to Connect to proxy:  
```bash
#Be on a client server, then run
telnet 10.0.0.2
# Start writing stuff and it should start showing up on the proxy server and also log everything out.
```


## Design Decisions:

**Main Idea**: 

- Built a transparent proxy that can telnet between client and server of different hosts with something in the middle the forward and set the telnet up.

#### open_port(int port)
- Same function from previous assignments. Basically build/opens a socket and binds the port to it on localhost
- Listens for any connects onto the socket, if there is, then it will fork to allow multiple connections to the proxy server

#### process_request(int socket)
- Sets up a new socket to connect to the server
- Parses all information of the addressses from client and server and logs it
- Sets up DNAT and SNAT stuff or something



## Resources used:

http://www.binarytides.com/server-client-example-c-sockets-linux/
http://beej.us/guide/bgnet/output/html/multipage/index.html