/*
    Edward Zhu
    Programming Assignment 2
    Distributive Filesystem - in C

    dfc.c
    Distributive Filesystem Server


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
#include <dirent.h>
// #include <err.h>
// #include <fcntl.h>

// #include <ctype.h>
// #include <strings.h>
#include <string.h>
#include <sys/stat.h>

int BUFFER_SIZE = 1024;
char server_dir[256] = ".";
int user_num = 0;

struct Config {
    // char    dir[128];
    char    users[8][128];
    char    passwords[8][128];
    char    current_user_name[128];
    char    current_user_pass[128];
    // int     port;
} server_conf;

void parse_config(const char *);
int check_user(int , char * , char * );
void list(int, char *);
requestFileCheck(char * );
void checkFileCurrServ(int , char * );
int get(char *);
int put(char *name);

int check_user(int socket, char * username, char * password) 
{
    int i;
    char directory[4096];

    strcpy(directory, server_dir);
    strcat(directory, username);
    for (i = 0; i < user_num; i++){
        if (strncmp(server_conf.users[i], username, strlen(username)) == 0){
            if(strncmp(server_conf.passwords[i], password, strlen(password)) == 0)
            {
                strcpy(server_conf.current_user_name, server_conf.users[i]);
                strcpy(server_conf.current_user_pass, server_conf.passwords[i]);
                if(!opendir(directory)) {
                    write(socket, "Directory Doesn't Exist. Creating!\n", 35);
                    mkdir(directory, 0770);
                }
                printf("User Authenticated.\n");
                return 1;
            }
        }
    }
    printf("User NOT Authenticated.\n");
    return -1;
}

void parse_config(const char *filename) {
    char *line;
    char Name[BUFFER_SIZE], Pass[BUFFER_SIZE];
    size_t len = 0;
    int read_len = 0;

    FILE* conf_file = fopen(filename,"r");

    while((read_len = getline(&line, &len, conf_file)) != -1) {
        // Remove endline character
        line[read_len-1] = '\0';

        sscanf(line, "%s %s", Name, Pass); 
        if (user_num < 8){
            strcpy(server_conf.users[user_num], Name);
            strcpy(server_conf.passwords[user_num++], Pass);
        }
        
    }
    
    // Debugging
    // printf("Username1: %s\n", server_conf.users[0]);
    // printf("Password1: %s\n", server_conf.passwords[0]);
    // printf("Username2: %s\n", server_conf.users[1]);
    // printf("Password2: %s\n", server_conf.passwords[1]);


    fclose(conf_file);
}

void list_server(int socket, char * username) {
    printf("Getting Files for: %s\n", username);
    DIR * d;
    struct dirent *dir;
    struct stat filedets;
    int status;
    int i = 0;

    // char tempresult[128];
    char result[256];

    char path[BUFFER_SIZE];
    char directory[4096];

    strcpy(directory, server_dir);
    strcat(directory, username);
    printf("%s\n", directory);
    d = opendir(directory);
    int length = 0;

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(".", dir->d_name) == 0){
            } else if (strcmp("..", dir->d_name) == 0){
            } else {
                sprintf(path, "%s/%s", directory, dir->d_name);
                status = lstat(path, &filedets);
                if(S_ISDIR(filedets.st_mode)) {
                    // Skip directories
                } else {
                    if(strncmp(dir->d_name, ".DS_Store", 9) != 0)
                    {
                        length += sprintf(result + length, "%d. %s\n", i, dir->d_name);
                        //TODO: Check for Pieces on Other Servers
                        i++;
                        // continue;
                    }
                }
            }
        }
        printf("FILES FOUND: \n %s\n", result);
        write(socket, result, strlen(result));
        closedir(d);
    }
}


int get(char *line)
{
    
}


int put(char *name) {

    return 0;
}

void process_request_server(int socket){
    char buf[BUFFER_SIZE];
    char arg[64];
    char *token;
    char username[32], password[32];
    int read_size    = 0;
    int len = 0;
    char command[256];

    while ((read_size = recv(socket, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("Found:  %s\n", line);

        // printf("Line: %s\n", command);
        if (strncmp(line, "AUTH:", 5) == 0)
        {
            token = strtok(line, ": ");
            token = strtok(NULL, " ");
            strcpy(username, token);
            token = strtok(NULL, " ");
            strcpy(password, token);
        }
        sscanf(line, "%s %s", command, arg);

        if(strncmp(command, "LIST", 4) == 0) {
            printf("LIST CALLED:\n");
            list_server(socket, username);
        } else if (strncmp(command, "GET", 3) == 0) {
            printf("GET CALLED!\n");
            // serverGet(socket, arg);
        } else if(strncmp(command, "PUT", 3) == 0){
            printf("PUT Called!\n");
            // serverPut(socket, arg);
        } else if(strncmp(command, "AUTH", 4) == 0) {
            if (check_user(socket, username, password) < 0){
                char *message = "Invalid Username/Password. Please try again.\n";
                write(socket, message, strlen(message));
                close(socket);
                printf("socket closed\n");
                return;
            }
        } else {
            printf("Unsupported Command: %s\n", command);
        }
    }
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
     
    return;
}

/*
 Connect to a certain socket and listen to it
 Modified from my webserver code
 Reference from:
 http://www.binarytides.com/server-client-example-c-sockets-linux/
*/
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
            process_request_server(client_sock);    
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

/*
    Modified logic from my webserver assignment
*/
int main(int argc, char *argv[]) {
    int port;

    // If there isn't 3 arguments, error out.
    if (argc != 3) {
        fprintf(stderr, "Invalid amount of arguments.");
        exit(0);
    }
    strcat(server_dir, argv[1]);
    strcat(server_dir, "/");
    if(!opendir(server_dir)){
        mkdir(server_dir, 0770);      
    }
    port = atoi(argv[2]);
    
    // Debugging
    // printf("Server Directory: %s\n", server_dir);
    // printf("Port Number: %d\n", port);

    // Parse through wsconf file and set variables when needed.
    parse_config("dfs.conf");
    // End parse

    open_port(port);


    return EXIT_SUCCESS;
}























