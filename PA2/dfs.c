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
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

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
void list(int, char *);
void get(char *, int);
void put(char * , int );

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

    // char tempresult[128];
    char result[256];

    char path[BUFFER_SIZE];
    char directory[BUFFER_SIZE];

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

/*
Push a file to client because GET requested
Reference:
http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
*/
void get( char * send_file, int sock) {
    char file_loc[128];
    int fd;
    char buf[BUFFER_SIZE];

    sprintf(file_loc, "%s%s/%s", server_dir, server_conf.current_user_name, send_file);

    if ((fd = open(file_loc, O_RDONLY)) < 0)
        printf("failed to open file\n");
        // errexit("Failed to open file at: '%s' %s\n", file_loc, strerror(errno)); 
    while (1) {
        // Read data into buffer.  We may not have enough to fill up buffer, so we
        // store how many bytes were actually read in bytes_read.
        int bytes_read = read(fd, buf, sizeof(buf));
        if (bytes_read == 0) // We're done reading from the file
            break;

        if (bytes_read < 0) {
            // handle errors
            printf("Failed to read\n");
            // errexit("Failed to read: %s\n", strerror(errno));
        }

        // You need a loop for the write, because not all of the data may be written
        // in one call; write will return how many bytes were written. p keeps
        // track of where in the buffer we are, while we decrement bytes_read
        // to keep track of how many bytes are left to write.
        void *p = buf;
        printf("Writing back into sock\n");

        while (bytes_read > 0) {
            int bytes_written = write(sock, p, bytes_read);
            if (bytes_written <= 0) {
                // handle errors
            }
            bytes_read -= bytes_written;
            p += bytes_written;
        }
    }
}


void put( char * send_file, int sock) {
    char buf[BUFFER_SIZE];
    char file_path[128];
    int file_size, remaining, len = 0;
    int len2;
    FILE *file;

    sprintf(file_path, "%s%s/", server_dir, server_conf.current_user_name);
    strncat(file_path, send_file, strlen(send_file));
    printf("%s\n", file_path);

    int read_size;
    while((read_size = recv(sock, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    {
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';
        // printf("Buf is %s\n", buf);
        // puts(buf);
        file_size = atoi(buf);
        printf("%d\n", file_size);
        if (!(file = fopen(file_path, "w")))
            printf("failed to open file\n");
            // errexit("Failed to open file at: '%s' %s\n", file_path, strerror(errno)); 

        remaining = file_size;
        while (((len2 = recv(sock, buf, BUFFER_SIZE, 0)) > 0) && (remaining > 0)) {
            printf("%s\n", buf);

            // write to file
            fprintf(file, "%s\n", buf);
            fclose(file);

            fwrite(buf, sizeof(char), len, file);
            remaining -= len;

            // fprintf(stdout, "Received %d bytes\n", len);
            return;
        }
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

    while ((read_size = recv(socket, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("Found:  %s\n", line);

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