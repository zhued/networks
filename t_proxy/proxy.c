/*
    Edward Zhu
    Programming Assignment 3
    Proxy Server - in C

    proxy.c
    Setting up a Proxy Server

*/

#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int BUFFER_SIZE = 2048;

/*
    Checked if HTTP version is 1.0, then parces IP address

    Reference:
        http://www.beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
*/
// void get_request(int sock, char *url, char *http_version, char *request){
//     // int status;
//     struct hostent *server;
//     struct sockaddr_in serveraddr;
//     char response[2048];

//     if(strncmp(http_version, "HTTP/1.0", 8) == 0){
//         printf("HTTP Version is correct!\n");

//         server = gethostbyname(url);

//         if (server == NULL){
//             printf("gethostbyname() failed\n");
//             printf("URL given is probabaly garbage.\n");
//         }

//     printf("Official name is: %s\n", server->h_name);
//     printf("IP address: %s\n", inet_ntoa(*(struct in_addr*)server->h_addr));


//         // connect to the server
//         bzero((char *) &serveraddr, server->h_length);

//         serveraddr.sin_family = AF_INET;
//         // server.sin_addr.s_addr = inet_addr(host);
//         bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length );
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
//         bzero(response, 1000);
//         recv(tcpSocket, response, 999, 0);
//         write(sock, response, strlen(response));

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
    // char *token;
    int read_size = 0;
    int len = 0;
    // char command[64], full_url[256], http_version[64], url[128], service[64];

    // Keep reading in what client sends
    while ((read_size = recv(socket, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("Found:  %s\n", line);

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
int open_port(){
    int sockfd , client_sock, c, *new_sock;
    struct sockaddr_in server , client;
    char DnatRules[BUFFER_SIZE];

    // struct sockaddr_storage their_addr;
    char *port = NULL;

    port = "8080"; 
    //Create socket
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1)
    {
        printf("Could not create socket\n");
    }
    printf("Socket created\n");
     
    //Prepare the sockaddr_in structure
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = INADDR_ANY;
    client.sin_port = htons(atoi(port));
    bzero(&client.sin_zero, 8);
     
    //Bind socket to address
    if( bind(sockfd,(struct sockaddr *)&client , sizeof(client)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }
    printf("Bind Complete\n");
     
    //Listen on socket
    listen(sockfd , 5);

    int fd;
    struct ifreq ifr;
    //This grabs the information from ifconfig for eth1, in order to write the DNAT rule
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);

    sprintf(DnatRules, "iptables -t nat -A PREROUTING -p tcp -i eth1 -j DNAT --to %s:%d", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), ntohs(client.sin_port));
    printf("DNAT RULES: %s\n", DnatRules);
    system(DnatRules);
    printf("\nserver port %d: waiting for connections...\n", ntohs(client.sin_port));
     
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
            printf("Connected! %s\n", port);         
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
    Main function
*/
int main(int argc, char *argv[]) {

    open_port();


    return EXIT_SUCCESS;

}