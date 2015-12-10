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
#include <dirent.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>


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
    int server_sock;
    int read_size = 0;
    int len = 0;
    char snat_rules[BUFFER_SIZE];
    time_t time_date;
    char *log_file;
    char stuff_to_log[BUFFER_SIZE];
    // int server_fd;
    struct sockaddr_in server_socket_setup,server_addr,client_addr,destination_addr;
    socklen_t s_addrlen, client_addrlen, destination_addrlen; 
    s_addrlen = sizeof(struct sockaddr_in);
    client_addrlen = sizeof(struct sockaddr_in);         
    destination_addrlen = sizeof(struct sockaddr_in);

    server_sock = socket(AF_INET , SOCK_STREAM , 0);
    if (server_sock == -1)
    {
        printf("Could not create socket\n");
    }

    server_socket_setup.sin_family = AF_INET;
    server_socket_setup.sin_port = 8080;
    server_socket_setup.sin_addr.s_addr = INADDR_ANY; 
    bzero(&server_socket_setup.sin_zero, 8);

    if((bind(server_sock, (struct sockaddr *) &server_socket_setup, sizeof(server_socket_setup))) < 0){
        perror("bind failed. Error");
        exit(-1);
    }
    if((getsockname(server_sock, (struct sockaddr *) &server_addr, &s_addrlen)) < 0){
        perror("getsockname failed. Error");
        exit(-1);       
    }
    if((getpeername(sock, (struct sockaddr *) &client_addr, &client_addrlen)) < 0){
        perror("getpeername: ");
        exit(-1);       
    }
    if((getsockopt(sock, SOL_IP, 80, (struct sockaddr *) &destination_addr, &destination_addrlen)) < 0){
        perror("getsockopt: ");
        exit(-1);
    }


    
    sprintf(snat_rules, "iptables -t nat -A POSTROUTING -p tcp -j SNAT --sport %d --to-source %s", ntohs(server_addr.sin_port), inet_ntoa(client_addr.sin_addr));
    printf("%s\n", snat_rules);  
    system(snat_rules);
    
    // server_fd = connect(server_sock, (struct sockaddr *) &destination_addr, destination_addrlen);
    // if (server_fd == -1){
    //     exit(1);
    // }

    time_date = time(NULL);
    log_file = ctime(&time_date);
    log_file[strlen(log_file)-1] = ' ';
    strcat(log_file, inet_ntoa(client_addr.sin_addr));
    sprintf(stuff_to_log, " %d ", ntohs(client_addr.sin_port));
    strcat(log_file, stuff_to_log);
    strcat(log_file, inet_ntoa(destination_addr.sin_addr));
    memset(stuff_to_log, 0, BUFFER_SIZE);
    sprintf(stuff_to_log, " %d ", ntohs(destination_addr.sin_port));
    strcat(log_file, stuff_to_log);

    FILE * file_to_log = fopen("logs.txt", "a+");
    fprintf(file_to_log, "%s\n", log_file);
    fclose(file_to_log);

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
    if (sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
    Opens a sockets and listens on that port.
    Taken from my PA2 assignment.
*/
int open_port(){
    int sockfd , client_sock, c, *new_sock;
    struct sockaddr_in client;
    char dnat_rules[BUFFER_SIZE];
    char s[INET6_ADDRSTRLEN];
    int file_desc;
    struct ifreq ifr;

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

    file_desc = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
    ioctl(file_desc, SIOCGIFADDR, &ifr);
    close(file_desc);

    sprintf(dnat_rules, "iptables -t nat -A PREROUTING -p tcp -i eth1 -j DNAT --to %s:%d", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), ntohs(client.sin_port));
    printf("DNAT RULES: %s\n", dnat_rules);
    system(dnat_rules);
    printf("\nserver port %d: waiting for connections...\n", ntohs(client.sin_port));
     
    //Accept and incoming connection
    printf("Waiting for incoming connections..\n");
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(sockfd, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        printf("Connection accepted\n");

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