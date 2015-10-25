/*
    Edward Zhu
    Programming Assignment 2
    Distributive Filesystem - in C

    dfc.c
    Distributive Filesystem Client
*/

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

#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/errno.h> // for errexit
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>

int BUFFER_SIZE = 1024;
int MAXSIZE = 8192;

struct Config {
    char    dfs_host[5][20];
    int     dfs_port[5];
    int     dfs_fd[5];
    char    Username[128];
    char    Password[128];
} config_dfc;

int parse_config(const char *);
void process_request_client(char *);
int connect_to_server(int, const char *);



int parse_config(const char *filename) {
    char *line;
    char head[BUFFER_SIZE], second[BUFFER_SIZE], host_port[BUFFER_SIZE];
    size_t len = 0;
    int read_len = 0;
    int num_servers = 0;

    FILE* conf_file = fopen(filename,"r");
    while((read_len = getline(&line, &len, conf_file)) != -1) {
        // Remove endline character
        line[read_len-1] = '\0';

        sscanf(line, "%s %s 127.0.0.1:%s", head, second, host_port);

        // If the head is Server, then parse each server created
        // and set it in the config_dfc object
        if (!strcmp(head, "Server")) {
            if (!strcmp(second, "DFS1")){
                strcat(config_dfc.dfs_host[1], second);
                // strcat(config_dfc.dfs_port[1], host_port);
                config_dfc.dfs_port[1] = atoi(host_port);
            }
            if (!strcmp(second, "DFS2")){
                strcat(config_dfc.dfs_host[2], second);
                // strcat(config_dfc.dfs_port[2], host_port);
                config_dfc.dfs_port[2] = atoi(host_port);
            }
            if (!strcmp(second, "DFS3")){
                strcat(config_dfc.dfs_host[3], second);
                // strcat(config_dfc.dfs_port[3], host_port);
                config_dfc.dfs_port[3] = atoi(host_port);
            }
            if (!strcmp(second, "DFS4")){
                strcat(config_dfc.dfs_host[4], second);
                // strcat(config_dfc.dfs_port[4], host_port);
                config_dfc.dfs_port[4] = atoi(host_port);
            }
            num_servers++;
        } 

        // Parce/Set Username
        if (!strcmp(head, "Username:")) {
            strcat(config_dfc.Username, second);
        } 

        // Parce/Set Password
        if (!strcmp(head, "Password:")) {
            strcat(config_dfc.Password, second);
        } 
    }

    // Debugging
    // printf("dfsport1: %d\n", config_dfc.dfs_port[1]);
    // printf("dfsport2: %d\n", config_dfc.dfs_port[2]);
    // printf("dfsport3: %d\n", config_dfc.dfs_port[3]);
    // printf("dfsport4: %d\n", config_dfc.dfs_port[4]);
    // printf("Username: %s\n", config_dfc.Username);
    // printf("Password: %s\n", config_dfc.Password);

    fclose(conf_file);
    return num_servers;
}

void process_request_client(char * buf){
    if (strncmp(buf, "GET", 3) == 0) {
        printf("GET CALLED!\n");
    } else if(strncmp(buf, "LIST", 4) == 0) {
        printf("LIST CALLED!\n");
    } else if(strncmp(buf, "PUT", 3) == 0) {
        printf("PUT CALLED!\n");
    } else {
        printf("Unsupported Command: %s\n", buf);
    }

}

// int send_request(char * request, const int server_num){

// }

int connect_to_server(int port, const char *host){
    int sock;
    struct sockaddr_in server;
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    // puts("Socket created");
    printf("port num %d\n", port);
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
 
    //Connect to remote server
    if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    // authenticateUser(sock, username, password);

    printf("Socket %d connected on port %d\n", sock, ntohs(server.sin_port));

    return sock;
}

int main(int argc, char *argv[]) {
    char *conf_file, buf[MAXSIZE];
    int server_num, i;

    // If no parameter given, send error
    // If it is given, then set conf_file to argv[1]
    if (argc != 2) {
        fprintf(stderr, "Config file required.");
        exit(0);
    }
    conf_file = argv[1];

    // Parse through wsconf file and set variables when needed.
    server_num = parse_config(conf_file);

    // create connection to all servers
    printf("There are %d servers in the config file.\n", server_num);
    printf("Attempting to connect...\n\n");
    config_dfc.dfs_fd[1]= connect_to_server(10001, config_dfc.dfs_host[1]);
    printf("fd is %d\n", config_dfc.dfs_fd[1]);
    // for (i = 1; i < server_num+1; ++i){
    //     config_dfc.dfs_fd[i]= connect_to_server(config_dfc.dfs_port[i], config_dfc.dfs_host[i]);
    //     // printf("fd is %d\n", fd);
    // }

    // for (i = 1; i < server_num+1; ++i){
    //     printf("fd is %d\n", config_dfc.dfs_fd[i]);
    // }

    // Open up stdin to take in commands
    printf("Enter 'q' to quit.");
    while(1){
        printf("\nEnter a command: ");
        if(fgets(buf, MAXSIZE, stdin) != NULL) {
            if (!strncmp(buf, "q", 1)){
                printf("Quitting out.");
                break;
            }
            process_request_client(buf);
        }
    }



    return EXIT_SUCCESS;
}