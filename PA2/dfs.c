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
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/md5.h>

int BUFFER_SIZE = 2048;
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
int errexit(const char *, ...);
void listserver(int, char *);
void get(char *, int);
void put(char * , int );
void process_request_server(int);
int open_port(int);

/*
check if a user and their password is in the conf file.
If it isn't then return that User is not authorized
If it is, then keep on going with commands.
*/
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
Similar parser like the client and also my webserver
*/
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

/*
go through directory and print out the files

Reference:
http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
*/
void list_server(int socket, char * username) {
    printf("Getting Files for: %s\n", username);
    DIR * d;
    struct dirent *dir;
    struct stat filedets;
    int status;
    int i = 0;

    char result[256];

    char path[BUFFER_SIZE];
    char directory[BUFFER_SIZE];

    strcpy(directory, server_dir);
    strcat(directory, username);
    printf("%s\n", directory);
    d = opendir(directory);
    int length = 0;

    // Skip the '..' in a folder and get all the files
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(".", dir->d_name) == 0){
            } else if (strcmp("..", dir->d_name) == 0){
            } else {
                sprintf(path, "%s/%s", directory, dir->d_name);
                status = lstat(path, &filedets);
                length += sprintf(result + length, "%d. %s\n", i, dir->d_name);
                // This is where I would check other servers for
                // other files
                i++;

            }
        }
        printf("FILES FOUND: \n %s\n", result);
        write(socket, result, strlen(result));
        closedir(d);
    }
}

/*
Push a file to client because GET requested

Reference:
http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
*/
void get( char * send_file, int sock) {
    char file_path[128];
    int fd;
    char buf[BUFFER_SIZE];

    sprintf(file_path, "%s%s/%s", server_dir, server_conf.current_user_name, send_file);

    if ((fd = open(file_path, O_RDONLY)) < 0)
        errexit("Failed to open file at: '%s' %s\n", file_path, strerror(errno)); 
    while (1) {
        // Read data into buffer
        int bytes_read = read(fd, buf, sizeof(buf));
        if (bytes_read == 0) // We're done reading from the file
            break;
        if (bytes_read < 0) {
            errexit("Failed to read: %s\n", strerror(errno));
        }

        // Write to the socket what is in the file
        void *p = buf;
        printf("Writing back into sock\n");
        while (bytes_read > 0) {
            int bytes_written = write(sock, p, bytes_read);
            bytes_read -= bytes_written;
            p += bytes_written;
        }
    }
}

/*
Push a file to Server because PUT requested

This function is pretty much reverse idea of get; instead of pulling from server and writing 
to this directory, it will take a file in this directory and write it to server

Reference:
http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
*/
void put( char * send_file, int sock) {
    char buf[BUFFER_SIZE];
    char file_path[128];
    int len = 0;
    int read_size;
    FILE *put_file;

    // Set the file path to the server_dir/username/file
    // like - DFS1/Alice/put.txt
    sprintf(file_path, "%s%s/", server_dir, server_conf.current_user_name);
    strncat(file_path, send_file, strlen(send_file));
    printf("file path is %s\n", file_path);

    // open file with write permission
    put_file = fopen(file_path, "w");
    if (put_file == NULL){
        printf("Error opening file\n");
        exit(1);
    }
    // fprintf all concents in the buffer into the file on server
    while ((read_size = recv(sock, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("%s\n", line);
        fprintf(put_file, "%s\n", line);
        fclose(put_file);
    }
}

/*
 Listens to what the client sends to the server.
 First authenticates the user, then next ones are listening for LIST, GET, POST requests
*/
void process_request_server(int socket){
    char buf[BUFFER_SIZE];
    char arg[64];
    char *token;
    char username[32], password[32];
    int read_size    = 0;
    int len = 0;
    char command[256];

    // Keep reading in what client sends
    while ((read_size = recv(socket, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("Found:  %s\n", line);

        // Authentication finds username and password
        // and checks them with all the accounts in the conf file
        if (strncmp(line, "AUTH:", 5) == 0)
        {
            token = strtok(line, ": ");
            token = strtok(NULL, " ");
            strcpy(username, token);
            token = strtok(NULL, " ");
            strcpy(password, token);
        }
        sscanf(line, "%s %s", command, arg);
        if(strncmp(command, "AUTH", 4) == 0) {
            if (check_user(socket, username, password) < 0){
                char *message = "Invalid Username/Password. Please try again.\n";
                write(socket, message, strlen(message));
                close(socket);
                printf("socket closed\n");
                return;
            }
        // Authentication done. Now onto LIST, GET, and PUT
        } else if(strncmp(command, "LIST", 4) == 0) {
            printf("LIST CALLED:\n");
            list_server(socket, username);
        } else if (strncmp(command, "GET", 3) == 0) {
            printf("GET CALLED!\n");
            get(arg, socket);
        } else if(strncmp(command, "PUT", 3) == 0){
            printf("PUT Called!\n");
            put(arg, socket); 
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
    listen(sockfd , 3);
     
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
            process_request_server(client_sock);    
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