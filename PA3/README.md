# Proxy Server

Created a proxy server that parses GET requests from the client and sends back to the client the results from the HTTP request from the web servers.

## To Run:

```bash
run 'make'

#Build Proxy: 
./proxy 10001
#OR JUST RUN
./server.sh

```

## How to Connect to proxy:  
```bash
telnet localhost 10001
#Once connected, type in a GET command to a http site with HTTP/1.0 as the version
GET http://www.google.com HTTP/1.0
#Replace http://www.google.com with any http site you want
#Once requested, it should throw back requests
```


**NOTE:** You can also set up a browser to connect to this proxy server, but make sure it is setup correctly. Once set up, try a site without https in it like 'http://www.edwardzhu.me' in the browser and you should get the header files of the webserver displayed on the terminal! (https websites may trigger 'CONNECT unimplemented' errors if browser not set up correctly.)


## Design Decisions:

**Main Idea**: 

- Parse from the command line the port number that the proxy server will live on
- Open and bind specified port to a socket and listen on it
- If connection is made to socket, fork a process request
- Parse specified request for GET and other variables
- If correctly specified, open another socket and send the request to the actual HTTP webserver
- Proxy server will listen for a response, then it will send back the response back to the client


#### open_port(int port)
- Same function from previous assignments. Basically build/opens a socket and binds the port to it on localhost
- Listens for any connects onto the socket, if there is, then it will fork to allow multiple connections to the proxy server

#### process_request(int socket)
- String parsing taken from pervious assignments as well
- Parses for GET, an http in the url path given, and also the http_version
- If http is in the url and a GET is called, it will send a get_request


#### get_request(int sock, char *url, char *http_version, char *request)
- Does a dnslookup with gethostbyname(url) to get the IP address of the url given.
- Once it finds a viable IP addr, it will open another socket that connects to the actual webserver and send the GET request to the server
- Once it receives a response back, it will write it to the client


## Resources used:

http://www.binarytides.com/server-client-example-c-sockets-linux/
http://beej.us/guide/bgnet/output/html/multipage/index.html

Reused many of my previous functions from the distributed file system as well as the http server programming assignments.