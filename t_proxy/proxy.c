/*
    Edward Zhu
    Programming Assignment 4
    Transparent Proxy Server - in C

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
// For mac to make sol_ip working
// #ifndef SOL_IP
// #define SOL_IP IPPROTO_IP
// #endif


/*
    Process the request that a client requests
    Example:
        GET http://www.google.com HTTP/1.0
*/
void process_request(int sock){
    char buf[BUFFER_SIZE];
    int server_sock, bytes_sent;
    int read_size = 0;
    int len = 0;
    // char command[64], full_url[256], http_version[64], url[128], service[64];

    struct sockaddr_in s_hints;
    struct sockaddr_in server_addr;
    struct sockaddr_in p_addr;
    struct sockaddr_in d_addr;
    socklen_t s_addrlen; 
    socklen_t p_addrlen; 
    socklen_t d_addrlen; 
    s_addrlen = sizeof(struct sockaddr_in);
    p_addrlen = sizeof(struct sockaddr_in);         
    d_addrlen = sizeof(struct sockaddr_in);

    server_sock = socket(AF_INET , SOCK_STREAM , 0);
    if (server_sock == -1)
    {
        printf("Could not create socket\n");
    }

    s_hints.sin_family = AF_INET;
    s_hints.sin_port = 8080;
    s_hints.sin_addr.s_addr = INADDR_ANY; 
    bzero(&s_hints.sin_zero, 8);

    if((bind(server_sock, (struct sockaddr *) &s_hints, sizeof(s_hints))) < 0){
        perror("bind failed. Error");
        exit(-1);
    }
    //Need information for SNAT rules, getting the server info
    if((getsockname(server_sock, (struct sockaddr *) &server_addr, &s_addrlen)) < 0){
        perror("getsockname failed. Error");
        exit(-1);       
    }
    //Need information for SNAT rules, getting the client info
    if((getpeername(sock, (struct sockaddr *) &p_addr, &p_addrlen)) < 0){
        perror("getpeername: ");
        exit(-1);       
    }
    //Need information for SNAT rules, getting the original destination (in write up)
    if((getsockopt(sock, SOL_IP, 80, (struct sockaddr *) &d_addr, &d_addrlen)) < 0){
        perror("getsockopt: ");
        exit(-1);
    }


    char SnatRules[BUFFER_SIZE];
    sprintf(SnatRules, "iptables -t nat -A POSTROUTING -p tcp -j SNAT --sport %d --to-source %s", ntohs(server_addr.sin_port), inet_ntoa(p_addr.sin_addr));
    printf("%s\n", SnatRules);  
    system(SnatRules);

    // int server_fd;
    // //Finally, connect to the server
    // server_fd = connect(server_sock, (struct sockaddr *) &d_addr, d_addrlen);
    // if (server_fd == -1){
    //     exit(1);
    // }

    char *logfile;
    char logs[BUFFER_SIZE];
    time_t currentTime;
    currentTime = time(NULL);
    //Getting the date and time
    logfile = ctime(&currentTime);
    //Replacing the new line character
    logfile[strlen(logfile)-1] = ' ';
    //Add the client's IP address to the logfile
    strcat(logfile, inet_ntoa(p_addr.sin_addr));
    //Convert the client's port number to a string
    sprintf(logs, " %d ", ntohs(p_addr.sin_port));
    strcat(logfile, logs);
    //Add server's IP address to the logfile
    strcat(logfile, inet_ntoa(d_addr.sin_addr));
    memset(logs, 0, BUFFER_SIZE);
    //Convert the server's port number to a string
    sprintf(logs, " %d ", ntohs(d_addr.sin_port));
    strcat(logfile, logs);

    FILE * FileLog = fopen("logs.txt", "a+");
    fprintf(FileLog, "%s\n", logfile);
    fclose(FileLog);

    // Keep reading in what client sends
    while ((read_size = recv(sock, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
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

void *find_which_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
    Opens a sockets and listens on that port.
    Taken from my PA2 assignment.
*/
int open_port(){
    int sockfd , client_sock, c, *new_sock;
    struct sockaddr_in client;
    char DnatRules[BUFFER_SIZE];
    char s[INET6_ADDRSTRLEN];

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

        // inet_ntop(client.ss_family, find_which_addr((struct sockaddr *)&client), s, sizeof s);

        printf("server port %s: got connection from %s\n", port, s);

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