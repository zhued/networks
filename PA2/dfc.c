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
#include <stdarg.h>

#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
// #include <openssl/md5.h>

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
int errexit(const char *, ...);
void list();
int get(char *name);
int put(char *name);

void auth_user(int sock, char * username, char * password){
    char *result = malloc(strlen(username)+strlen(password));//+1 for the zero-terminator
    strcpy(result, "AUTH:");
    strcat(result, username);
    strcat(result, " ");
    strcat(result, password);
    if (write(sock, result, strlen(result)) < 0){
        errexit("Error in Authentication: %s\n", strerror(errno));
    }
}

int errexit(const char *format, ...) {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

void list() {
    // can list all files on one server
    // When distributed works, this will look into all server directories
}

/*
Pull/write a file from server because GET requested
Reference:
http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
*/
int get(char *line) {
    char buf[BUFFER_SIZE];
    int read_size = 0, len = 0;
    FILE *dl_file;
    char command[8], arg[64];
    char file_loc[128];
    int sock = config_dfc.dfs_fd[1];
    // int i;

    sscanf(line, "%s %s", command, arg);
    sprintf(file_loc, "./%s", arg);
    if(write(sock, line, strlen(line)) < 0) {
        errexit("Error in GET: %s\n", strerror(errno));
    }
    dl_file = fopen(file_loc, "w");
    if (dl_file == NULL){
        printf("Error opening file!\n");
        exit(1);
    }
    while ((read_size = recv(sock, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("%s\n", line);
        fprintf(dl_file, "%s\n", line);
        fclose(dl_file);
    }
    return 0;
    printf("FINISHED GET\n");
}

/*
Push a file to Server because PUT requested

This function is pretty much reverse idea of get; instead of pulling from server and writing 
to this directory, it will take a file in this directory and write it to server

Reference:
http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
*/
int put(char *line) {
    // struct timeval timeout;
    // struct timespec tim;
    // tim.tv_sec = 0;
    // tim.tv_nsec = 100000000L; /* 0.1 seconds */

    char file_loc[128];
    int fd;
    char buf[BUFFER_SIZE];
    char * files = "./premade_files";
    int sock = config_dfc.dfs_fd[1];
    char command[8], arg[64];


    sscanf(line, "%s %s", command, arg);
    sprintf(file_loc, "%s/%s", files, arg);
    printf("file location %s\n", file_loc);

    // sprintf(file_loc, "%s%s/%s", server_dir, server_conf.current_user_name, send_file);

    if(write(sock, line, strlen(line)) < 0) {
        errexit("Error in PUT: %s\n", strerror(errno));
    }

    if ((fd = open(file_loc, O_RDONLY)) < 0)
        errexit("Failed to open file at: '%s' %s\n", file_loc, strerror(errno)); 
    while (1) {
        // Read data into buffer.  We may not have enough to fill up buffer, so we
        // store how many bytes were actually read in bytes_read.
        int bytes_read = read(fd, buf, sizeof(buf));
        if (bytes_read == 0) // We're done reading from the file
            break;

        if (bytes_read < 0) {
            errexit("Failed to read: %s\n", strerror(errno));
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
    return 0; 
    printf("FINISHED PUT\n");
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
    size_t len = 0;
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
                // puts("List failed");
                errexit("Error in List: %s\n", strerror(errno));
            }
            if( recv(sock, reply, 2000, 0) < 0)
            {
                errexit("Error in recv: %s\n", strerror(errno));
                // puts("recv failed");
            } else {
                printf("%s\n", reply);
            }
        } else if (!strncasecmp(command, "GET", 3)) {
            if (strlen(line) <= 4)
                printf("GET needs an argument\n");
            else
                printf("GET CALLED!\n");
                get(line);
        } else if (!strncasecmp(command, "PUT", 3)) {
            if (strlen(line) <= 4)
                printf("PUT needs an argument\n");
            else
                printf("PUT CALLED!\n");
                put(line);
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