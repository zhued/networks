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
#include <sys/errno.h>

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
void process_request_client(int);
int connect_to_server(int, const char *);
// void authenticateUser(int, char *, char *);
void list();
int get(char *name);
int put(char *name);

// int errexit(const char *format, ...) {
//         va_list args;
//         va_start(args, format);
//         vfprintf(stderr, format, args);
//         va_end(args);
//         exit(1);
// }

void auth_user(int sock, char * username, char * password){
    char *result = malloc(strlen(username)+strlen(password));//+1 for the zero-terminator
    strcpy(result, "AUTH:");
    strcat(result, username);
    strcat(result, " ");
    strcat(result, password);
    if (write(sock, result, strlen(result)) < 0){
        printf("Authentication failed\n");
        // errexit("Error in Authentication: %s\n", strerror(errno));
    }
    // printf("Authentication success!\n");
}

void list() {
    
}

int get(char *name) {

    return 0;
}

int put(char *name) {

    return 0;
}

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

/*
Referenced this to make interactive commands:
http://stephen-brennan.com/2015/01/16/write-a-shell-in-c/
*/
void process_request_client(int sock){
    char *line = NULL, command[8], arg[64];
    // char *token;
    ssize_t len = 0;
    ssize_t read;
    int status = 1;

    do {
        printf("Enter Commands> ");
        read = getline(&line, &len, stdin);
        line[read-1] = '\0';

        sscanf(line, "%s %s", command, arg);


        if (!strncasecmp(command, "LIST", 4)) {
            char reply[BUFFER_SIZE];
            if(write(sock, command, strlen(command)) < 0) {
                puts("List failed");
                // errexit("Error in List: %s\n", strerror(errno));
            }
            if( recv(sock, reply, 2000, 0) < 0)
            {
                // errexit("Error in recv: %s\n", strerror(errno));
                puts("recv failed");
            } else {
                printf("%s\n", reply);
            }
        } else if (!strncasecmp(command, "GET", 3)) {
            if (strlen(line) <= 4)
                printf("GET needs an argument\n");
            else
                printf("GET CALLED!\n");
        } else if (!strncasecmp(command, "PUT", 3)) {
            if (strlen(line) <= 4)
                printf("PUT needs an argument\n");
            else
                printf("PUT CALLED!\n");
        } else if (!strncasecmp(command, "q", 1)) {
            status = 0;
        } else {
            printf("Unsupported Command: %s\n", command);
        }
        //printf("%s\n", command);
    } while(status);

    printf("Quitting out...\n");
}

/*
 Connect to a certain socket
 Reference from:
 http://www.binarytides.com/server-client-example-c-sockets-linux/
*/
int connect_to_server(int port, const char *host){
    int sock;
    struct sockaddr_in server;
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (sock == -1)
    {
        printf("Could not create socket");
    }

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

    auth_user(sock, config_dfc.Username, config_dfc.Password);

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
    printf("%d servers in conf file\n\n", server_num);

    for (i = 1; i < server_num+1; ++i){
        config_dfc.dfs_fd[i]= connect_to_server(config_dfc.dfs_port[i], config_dfc.dfs_host[i]);
        // printf("fd is %d\n", fd);
    }

    // DEBUGGING
    // for (i = 1; i < server_num+1; ++i){
    //     printf("fd is %d\n", config_dfc.dfs_fd[i]);
    // }

    // for (i = 1; i < server_num+1; ++i){
    //     printf("Listening on Socket %d\n", config_dfc.dfs_fd[i]);
    //     process_request_client(config_dfc.dfs_fd[i]);
    // }
    process_request_client(config_dfc.dfs_fd[1]);


    return EXIT_SUCCESS;

}