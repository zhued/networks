# HTTP Web Server

## Edward Zhu - 9/20/15

Compiled on a mac with gcc. 

## How to Run
    `make clean`
    `make`
    `./webserver`

## webserver.c 
    Main file where all the code lives!

## ws.config
    Configurations that can be parsed into the server.


## Handles:
    png, text, jpg, and html files
    500, 501, 404, and 400 errors


Socket Programming learned and inspired from: tinyhttpd-0.1.0 and rosetta code.

To test pipeline support, set the port to any number higher than 1024 and then run the
following command replacing <your port number> with the one you chose:
    (echo -en "GET /index.html HTTP/1.1\n Host: localhost \nConnection: keep-alive\n\nGET
    /index.html HTTP/1.1\nHost: localhost\n\n"; sleep 10) | telnet localhost <your port number>
