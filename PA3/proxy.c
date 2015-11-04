#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

int BUFFER_SIZE = 2048;

int errexit(const char *, ...);

/*
Used to show error and also exit at the same time
So I don't have to perror and exit and all that stuff anymore
*/
int errexit(const char *format, ...) {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

/*
    Checked if HTTP version is 1.0, then parces IP address

    Reference:
        http://www.beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
*/
void get_request(int sock, char *url, char *http_version, char *request){
    // int status;
    struct hostent *server;
    struct sockaddr_in serveraddr;
    char response[2048];

    if(strncmp(http_version, "HTTP/1.0", 8) == 0){
        printf("HTTP Version is correct!\n");

        server = gethostbyname(url);

        if (server == NULL){
            printf("gethostbyname() failed\n");
        }

    printf("Official name is: %s\n", server->h_name);
    printf("IP address: %s\n", inet_ntoa(*(struct in_addr*)server->h_addr));


        // connect to the server
        bzero((char *) &serveraddr, server->h_length);

        serveraddr.sin_family = AF_INET;
        // server.sin_addr.s_addr = inet_addr(host);
        bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length );
        serveraddr.sin_port = htons(80);

        int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (connect(tcpSocket, (struct sockaddr *) &serveraddr, sizeof serveraddr ) < 0){
            printf("\nError Connecting");
        }

        strcat(request, "\r\n");
        // Send request to socket
        if (send(tcpSocket, request, strlen(request), 0) < 0){
            printf("Error with send()");
        }
        bzero(response, 1000);
        recv(tcpSocket, response, 999, 0);
        write(sock, response, strlen(response));

    } else {
        printf("HTTP Version is NOT correct!\n");
    }
}
// 
// THIS SHT DONT WORK, I HATE STRING FORMATING IN C HOLY MOTHER OF HESUS
// 
// void get_request(char *url, char *http_version, char *request){
//     struct addrinfo hints, *res, *p;
//     int status;
//     char ipstr[INET6_ADDRSTRLEN];
//     struct sockaddr_in serveraddr;
//     // char response[1000];

//     if(strncmp(http_version, "HTTP/1.0", 8) == 0){
//         printf("HTTP Version is correct!\n");

//         // Parse the IP address from the host given from client
        // memset(&hints, 0, sizeof hints);
        // hints.ai_family = AF_UNSPEC;
        // hints.ai_socktype = SOCK_STREAM; 

        // if ((status = getaddrinfo(url, "http", &hints, &res)) != 0) {
        //     fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        //     exit(1);
        // }

        // p = res;
        // void *addr = NULL;
        // if (p->ai_family == AF_INET) { // IPv4
        //     struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        //     addr = &(ipv4->sin_addr);
        // } else {
        //     printf("IPv6 not supported!\n");
        // }

        // // convert the IP to a string and print it:
        // inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        // freeaddrinfo(res);

//         printf("IPv4 address for %s: %s\n", url, ipstr);

//         // connect to the server
        
//         bzero((char *) &serveraddr, sizeof ipstr );

//         serveraddr.sin_family = AF_INET;
//         // server.sin_addr.s_addr = inet_addr(host);
//         bcopy((char *)ipstr, (char *)&serveraddr.sin_addr.s_addr, sizeof ipstr );
//         serveraddr.sin_port = htons(80);

//         int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

//         if (connect(tcpSocket, (struct sockaddr *) &serveraddr, sizeof serveraddr ) < 0){
//             printf("\nError Connecting");
//         }

//         strcat(request, "\r\n");
//         // Send request to socket
//         if (send(tcpSocket, request, strlen(request), 0) < 0){
//             printf("Error with send()");
//         }

//     } else {
//         printf("HTTP Version is NOT correct!\n");
//     }
// }

/*
    Process the request that a client requests
    Example:
        GET http://www.google.com HTTP/1.0
*/
void process_request(int socket){
    char buf[BUFFER_SIZE];
    char *token;
    int read_size = 0;
    int len = 0;
    char command[64], full_url[256], http_version[64], url[128], service[64];

    // Keep reading in what client sends
    while ((read_size = recv(socket, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("Found:  %s\n", line);

        // Find the url that doens't have http in front of it
        sscanf(line, "%s %s %s", command, full_url, http_version);
        if (strncmp(full_url, "http", 4) == 0){
            token = strtok(full_url, "://");
            strcpy(service, token);
            token = strtok(NULL, "//");
            strcpy(url, token);
        } else {
            printf("HTTP not founded in url\n");
            strcpy(url, full_url);
        }

        // We only support GET
        if(strncmp(command, "GET", 3) == 0) {
            get_request(socket, url, http_version, line);
        } else {
            printf("Unsupported Command: %s\n", command);
        }
    } 

    if(read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if(read_size == -1){
        perror("recv failed");
    }
    return;
}


/*
    Opens a sockets and listens on that port.
    Taken from my PA2 assignment.
*/
int open_port(int port){
    int sockfd , client_sock, c, *new_sock;
    struct sockaddr_in server , client;
     
    //Create socket
    sockfd = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (sockfd == -1)
    {
        printf("Could not create socket\n");
    }
    printf("Socket created\n");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // timeouts on socket
    struct timeval timeout;      
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        perror("setsockopt failed\n");

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        perror("setsockopt failed\n");
     
    //Bind socket to address
    if( bind(sockfd,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }
    printf("Bind Complete\n");
     
    //Listen on socket
    listen(sockfd , 5);
     
    //Accept and incoming connection
    printf("Waiting for incoming connections..\n");
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(sockfd, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        printf("Connection accepted\n");
        new_sock = malloc(1);
        *new_sock = client_sock;
         
        // fork it so multiple connections can come in
        if(fork() == 0){
            printf("Connected! %d\n", port);         
            process_request(client_sock);    
            exit(0);        
        }
    }
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    return 0;
}

/*
    Main functions
*/
int main(int argc, char *argv[]) {
    int port;

    // If no parameter given, send error
    // If it is given, then set conf_file to argv[1]
    if (argc != 2) {
        fprintf(stderr, "Please give a port number");
        exit(0);
    }

    port = atoi(argv[1]);

    open_port(port);


    return EXIT_SUCCESS;

}