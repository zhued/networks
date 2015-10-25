/*
    Edward Zhu
    Programming Assignment 2
    Distributive Filesystem - in C

    dfc.c
    Distributive Filesystem Server

    Code and inspiration based on: http://www.binarytides.com/server-client-example-c-sockets-linux/

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
// #include <err.h>
// #include <fcntl.h>

// #include <ctype.h>
// #include <strings.h>
#include <string.h>
// #include <sys/stat.h>

int BUFFER_SIZE = 1024;

struct Config {
    // char    dir[128];
    char    users[8][128];
    char    passwords[8][128];
    // int     port;
} server_conf;

void parse_config(const char *);
void list();
int get(char *name);
int put(char *name);

void parse_config(const char *filename) {
    char *line;
    char Name[BUFFER_SIZE], Pass[BUFFER_SIZE];
    size_t len = 0;
    int read_len = 0;
    int num_users = 0;

    FILE* conf_file = fopen(filename,"r");

    while((read_len = getline(&line, &len, conf_file)) != -1) {
        // Remove endline character
        line[read_len-1] = '\0';

        sscanf(line, "%s %s", Name, Pass); 
        if (num_users < 8){
            strcpy(server_conf.users[num_users], Name);
            strcpy(server_conf.passwords[num_users++], Pass);
        }
        
    }
    
    // Debugging
    // printf("Username1: %s\n", server_conf.users[0]);
    // printf("Password1: %s\n", server_conf.passwords[0]);
    // printf("Username2: %s\n", server_conf.users[1]);
    // printf("Password2: %s\n", server_conf.passwords[1]);


    fclose(conf_file);
}

void list() {

}

int get(char *name) {

    return 0;
}

int put(char *name) {

    return 0;
}

// void process_request_server(int socket){
//     printf("heloooooooo\n");

// }


int open_port(int port){
    int socket_desc , client_sock, c, *new_sock;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (socket_desc == -1)
    {
        printf("Could not create socket\n");
    }
    printf("Socket created\n");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    printf("bind done\n");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    printf("Waiting for incoming connections...\n");
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        printf("Connection accepted\n");
        new_sock = malloc(1);
        *new_sock = client_sock;
         
        if(fork() == 0){
            printf("Connected! %d\n", port);         
            // process_request_server(client_sock);    
            exit(0);        
        }
         
        printf("Handler assigned\n");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int port;
    char *directory;

    // If there isn't 3 arguments, error out.
    if (argc != 3) {
        fprintf(stderr, "Invalid amounnt of arguments.");
        exit(0);
    }
    directory = argv[1];
    port = atoi(argv[2]);
    
    // Debugging
    // printf("Directory: %s\n", directory);
    // printf("Port Number: %d\n", port);

    // Parse through wsconf file and set variables when needed.
    parse_config("dfs.conf");
    // End parse

    open_port(port);


    return EXIT_SUCCESS;
}