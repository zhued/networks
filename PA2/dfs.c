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
    char    users[8][128];
    char    passwords[8][128];
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
    printf("Username1: %s\n", server_conf.users[0]);
    printf("Password1: %s\n", server_conf.passwords[0]);
    printf("Username2: %s\n", server_conf.users[1]);
    printf("Password2: %s\n", server_conf.passwords[1]);


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

int main(int argc, char *argv[]) {
    // Parse through wsconf file and set variables when needed.
    parse_config("dfs.conf");
    // End parse

    return EXIT_SUCCESS;
}