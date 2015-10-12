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
// #include <sys/types.h> 
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <err.h>
// #include <fcntl.h>

// #include <ctype.h>
// #include <strings.h>
#include <string.h>
// #include <sys/stat.h>

int BUFFER_SIZE = 1024;

struct Config {
    char    Username[50];
    char    Password[50];
} config_dfs;

void parse_config(const char *);

void parse_config(const char *filename) {
    char *line;
    char head[BUFFER_SIZE], second[BUFFER_SIZE], host_port[BUFFER_SIZE];
    size_t len = 0;
    int read_len = 0;

    FILE* conf_file = fopen(filename,"r");
    while((read_len = getline(&line, &len, conf_file)) != -1) {
        // Remove endline character
        line[read_len-1] = '\0';
        sscanf(line, "%s %s %s", head, second, host_port); 

        // Parce/Set Username
        if (!strcmp(head, "Username:")) {
            strcat(config_dfs.Username, second);
        } 

        // Parce/Set Password
        if (!strcmp(head, "Password:")) {
            strcat(config_dfs.Password, second);
        } 
    }
    
    fclose(conf_file);
}

int main() {
    // Parse through wsconf file and set variables when needed.
    parse_config("dfs.conf");
    // End parse

    return EXIT_SUCCESS;
}