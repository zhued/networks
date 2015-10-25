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


/*
Parses the config file
Taken from my first webserver assignment
*/
int parse_config(const char *filename){
    char *line, *token;
    char head[64], second[256], host[256];
    size_t len = 0;
    int read_len = 0;
    int num_servers = 0;

    FILE* conf_file = fopen(filename,"r");
    while((read_len = getline(&line, &len, conf_file)) != -1) {
        // Remove endline character
        line[read_len-1] = '\0';

        sscanf(line, "%s %s %s", head, second, host);

        // If the head is Server, then parse each server created
        // and set it in the config_dfc object
        if (!strcmp(head, "Server")) {
            token = strtok(host, ":");
            strncpy(config_dfc.dfs_host[num_servers+1],token, 20);
            token = strtok(NULL, " ");
            config_dfc.dfs_port[num_servers+1]= atoi(token);
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

// Connect to a certain socket
int connect_to_server(int port, const char *hostname){
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
    server.sin_addr.s_addr = inet_addr(hostname);
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
    int server_num, i;

    // If no parameter given, send error
    // If it is given, then set conf_file to argv[1]
    if (argc != 2) {
        fprintf(stderr, "Config file required.");
        exit(0);
    }

    // Parse through wsconf file and set variables when needed.
    server_num = parse_config(argv[1]);

    // create connection to all servers
    printf("There are %d servers in the config file.\n", server_num);
    printf("Attempting to connect...\n\n");

    for (i = 1; i < server_num+1; ++i){
        config_dfc.dfs_fd[i]= connect_to_server(config_dfc.dfs_port[i], config_dfc.dfs_host[i]);
        // printf("fd is %d\n", fd);
    }

    // for (i = 1; i < server_num+1; ++i){
    //     printf("fd is %d\n", config_dfc.dfs_fd[i]);
    // }

    // // Try to connect to one of the servers
    // for (i = 0; i < num_servers; i++)
    // {
    //     if(config_dfc.dfs_fd[i] != 1)
    //     {
    //         printf("%d\n", config_dfc.dfs_fd[i]);
    //         readUserInput(config_dfc.dfs_fd[i]);
    //         // connection found, break out of loop.
    //         break;
    //     }
    // }

    return EXIT_SUCCESS;




    // return EXIT_SUCCESS;

}