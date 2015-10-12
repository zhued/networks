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
    char    dps1_port[20];
    char    dps2_port[20];
    char    dps3_port[20];
    char    dps4_port[20];
    char    Username[50];
    char    Password[50];
} config_dfc;

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

        // If the head is Server, then parse each server created
        // and set it in the config_dfc object
        if (!strcmp(head, "Server")) {
            if (!strcmp(second, "DFS1"))
                strcat(config_dfc.dps1_port, host_port);
            if (!strcmp(second, "DFS2"))
                strcat(config_dfc.dps2_port, host_port);
            if (!strcmp(second, "DFS3"))
                strcat(config_dfc.dps3_port, host_port);
            if (!strcmp(second, "DFS4"))
                strcat(config_dfc.dps4_port, host_port);
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
    
    fclose(conf_file);
}

int main() {
    // Parse through wsconf file and set variables when needed.
    parse_config("dfc.conf");
    // End parse

    return EXIT_SUCCESS;
}