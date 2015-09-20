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
// #include <sys/wait.h>

int BUFFER_SIZE = 1024;


void listentoRequests(int);
void send_file(int, const char *);
int get_line(int, char *, int);
char * deblank(char *);
// void handle_request(int);
void handle_filetype(int , const char *);
void handle_error(int, const char *, int );
int open_port(int);

struct Config {
    int    port;
    char    DocumentRoot[256];
    char    DirectoryIndex[256];
    char    content_type[100];
    int     content_num;
} config;

struct Request {
    char    method[256];
    char    url[256];
    char    httpver[256];
    // char    host[128];
    int     keep_alive;
} request; 

/*
    Sends file requested. 
*/
void send_file(int client, const char *filename){
    char *send_buffer;
    FILE *requested_file;
    long fileLength;
    printf("Received request for file: %s on socket %d\n", filename + 1, client);
  
    // open file with rb because it isn't just a text file 
    requested_file = fopen(filename, "rb");

    // If no file could be opened, then call 404 error
    if (requested_file == NULL){
        handle_error(client, filename, 404);
    }

    // HTML has weird behavior on Linux
    // Must do the same as other binary files, but must also read and store 
    // everything from the resource.
    if(strstr(filename, ".html") != NULL){
        FILE *resource = NULL;
        int numchars = 1;
        char buf[BUFFER_SIZE];
        char buf1[BUFFER_SIZE];

        buf[0] = 'A'; buf[1] = '\0';
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));

        resource = fopen(filename, "r");
        handle_filetype(client, filename);

        // Reads the resource file and sends it to the client.
        // Can't just send entire binary file.
        fgets(buf1, sizeof(buf1), resource);
        while (!feof(resource)){
            send(client, buf1, strlen(buf1), 0);
            fgets(buf1, sizeof(buf1), resource);
        }
        fclose(resource);
    } else {
        // Find the size of requested file by streaming it through
        // then rewinds back to position in order to read again
        fseek(requested_file, 0, SEEK_END);
        fileLength = ftell(requested_file);
        rewind(requested_file);
    
        // Set the send buffer to the exact size of the filelength by malloc
        // Then read the file in with t
        send_buffer = (char*) malloc(sizeof(char)*fileLength);
        size_t result = fread(send_buffer, 1, fileLength, requested_file);
    
        // If result is readable, then handle the filetype and send the result to client
        if (result > 0) {
            handle_filetype(client, filename);
            send(client, send_buffer, result, 0);   
        }   
        else { handle_error(client, NULL, 500); }
    }
  
    fclose(requested_file);
}

/*
    Handle filetype that the client requests
*/
void handle_filetype(int client, const char *filename){
    char buf[BUFFER_SIZE];
    (void)filename;
    const char* filetype;
    long int size;

    // Find size of the file to set as content length
    FILE *f = fopen(filename, "r");
    if (f){
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);
    }

    // Parses the filename, if there is a '.' then take the +1 value
    // which will be the filetype
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) {
        filetype = "";
    } else {
        filetype = dot + 1;
    }

    // Send success codes on html and filetypes .png and .gif
    if(strcmp(filetype, "html") == 0){
        send(client, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
        send(client, "Content-Type: text/html\r\n", strlen("Content-Type: text/html\r\n"), 0);
        send(client, "\r\n", strlen("\r\n"), 0);
    }
    else if (strcmp(filetype, "png") == 0){
        printf("PNG request received.\n");
        send(client, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
        send(client, "Content-Type: image/png\r\n", strlen("Content-Type: image/png\r\n"), 0);
        sprintf(buf, "Content-Length: %ld \r\n", size);
        send(client, buf, strlen(buf), 0);
        send(client, "Content-Transfer-Encoding: binary\r\n", strlen("Content-Transfer-Encoding: binary\r\n"), 0);
        send(client, "\r\n", strlen("\r\n"), 0);
    }
    else if (strcmp(filetype, "gif") == 0) {
        printf("GIF request received.\n");
        send(client, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
        send(client, "Content-Type: image/gif\r\n", strlen("Content-Type: image/gif\r\n"), 0);
        sprintf(buf, "Content-Length: %ld \r\n", size);
        send(client, buf, strlen(buf), 0);
        send(client, "Content-Transfer-Encoding: binary\r\n", strlen("Content-Transfer-Encoding: binary\r\n"), 0);
        send(client, "\r\n", strlen("\r\n"), 0);
    }
}

/*
    Handles errors, 400,404, 500, and 501 implemented
*/
void handle_error(int client, const char *filename, int error_num){
    char buf[BUFFER_SIZE];
    // File not found is 404
    if (error_num == 404)
    {
        send(client, "HTTP/1.1 404 NOT FOUND\r\n", strlen("HTTP/1.1 404 NOT FOUND\r\n"), 0);
        send(client, "Content-Type: text/html\r\n", strlen("Content-Type: text/html\r\n"), 0);
        send(client, "\r\n", strlen("\r\n"), 0);
        send(client, "<HTML><TITLE>404 NOT FOUND</TITLE>\r\n", strlen("<HTML><TITLE>404 NOT FOUND</TITLE>\r\n"), 0);
        sprintf(buf, "<BODY><P>HTTP/1.1 404 Not Found: %s \r\n", filename);
        send(client, buf, strlen(buf), 0);
        send(client, "</BODY></HTML>\r\n", strlen("</BODY></HTML>\r\n"), 0);
    }
    // Formating error, either invalid method, http version, or uri
    else if (error_num == 400){
        send(client, "HTTP/1.1 400 BAD REQUEST\r\n", sizeof("HTTP/1.1 400 BAD REQUEST\r\n"), 0);
        send(client, "Content-type: text/html\r\n", sizeof("Content-type: text/html\r\n"), 0);
        send(client, "\r\n", sizeof("\r\n"), 0);
        if(strcmp(filename, "Invalid Method")){
            sprintf(buf, "<P>HTTP/1.1 400 Bad Request:  Invalid Method: %s \r\n", request.method);
            send(client, buf, strlen(buf), 0);
            // send(client, "<P>HTTP/1.1 400 Bad Request:  Invalid Method", sizeof("<P>HTTP/1.1 400 Bad Request:  Invalid Method"), 0);
        }
        if(strcmp(filename, "Invalid Version")){
            sprintf(buf, "<P>HTTP/1.1 400 Bad Request:  Invalid Version: %s \r\n", request.httpver);
            send(client, buf, strlen(buf), 0);
            // send(client, "<P>HTTP/1.1 400 Bad Request:  Invalid Version", sizeof("<P>HTTP/1.1 400 Bad Request:  Invalid Version"), 0);
        }
        if(strcmp(filename, "Invalid URI")){
            send(client, "<P>HTTP/1.1 400 Bad Request:  Invalid URI", sizeof("<P>HTTP/1.1 400 Bad Request:  Invalid URI"), 0);
        }
        send(client, "\r\n", sizeof("\r\n"), 0);
    }
    // unexpected server errors
    else if (error_num == 500) {
        send(client, "HTTP/1.0 500 Internal Server Error\r\n", strlen("HTTP/1.0 500 Internal Server Error\r\n"), 0);
        send(client, "Content-type: text/html\r\n", strlen("Content-type: text/html\r\n"), 0);
        send(client, "\r\n", strlen("\r\n"), 0);
        send(client, "HTTP/1.1 500  Internal  Server  Error:  cannot  allocate  memory\r\n", strlen("HTTP/1.1 500  Internal  Server  Error:  cannot  allocate  memory\r\n"), 0);
    }
    // Not implemented file format/type
    else if (error_num == 501) {
        send(client, "HTTP/1.1 501 Method Not Implemented\r\n", strlen("HTTP/1.1 501 Method Not Implemented\r\n"), 0);
        send(client, "Content-Type: text/html\r\n", strlen("Content-Type: text/html\r\n"), 0);
        send(client, "\r\n", strlen("\r\n"), 0);
        send(client, "<HTML><HEAD><TITLE>Method Not Implemented\r\n", strlen("<HTML><HEAD><TITLE>Method Not Implemented\r\n"), 0);
        send(client, "</TITLE></HEAD>\r\n", strlen("</TITLE></HEAD>\r\n"), 0);
        sprintf(buf, "<BODY><P>HTTP/1.1 501  Not Implemented:  %s\r\n", filename);
        send(client, buf, strlen(buf), 0);
        send(client, "</BODY></HTML>\r\n", strlen("</BODY></HTML>\r\n"), 0);
    }
}

/*
    Returns back a line of the header
*/
int get_line(int sock, char *buf, int size_buffer)
{
    int p = 0;
    char c = '\0';
    int n;

    while ((p < size_buffer - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        // printf("%02X\n", c);
        if (n > 0){
            if (c == '\r'){
                n = recv(sock, &c, 1, MSG_PEEK);
                // printf("%02X\n", c);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[p] = c;
            p++;
        }
        else
            c = '\n';
    }
    buf[p] = '\0';

    return(p);
}

/*
    Constantly listens to requests from client and handles it
*/
void listentoRequests(int client){
    char buf[BUFFER_SIZE];
    int numchars;
    char path[512];
    struct stat st;
    char *extension;
    char all_headers[1024];

    // gets the first line of the header
    numchars = get_line(client, buf, sizeof(buf));

    // Scan the line in buf for three things:
    //      Method, url, and HTTP version
    // and sets it as request class variables
    sscanf(buf, "%s %s %s", request.method, request.url, request.httpver);
    request.method[strlen(request.method)+1] = '\0';
    request.url[strlen(request.url)+1] = '\0';
    request.httpver[strlen(request.httpver)+1] = '\0';

    // Concatenate all remaining headers into a string
    while ((numchars > 0) && strcmp("\n", buf)){
        numchars = get_line(client, buf, sizeof(numchars));
        strcat(all_headers, buf);
    }

    // If there is a keep-alive in header, then set keep_alive to 1, else 0
    if (!strstr("Connection: keep-alive", all_headers)){
        request.keep_alive = 1;
    } else{
        request.keep_alive = 0;
    }

    // Get extension types from the url, and if the content type
    // isn't in the conf file
    extension = strrchr(request.url, '.');
    if (extension != NULL) {
        if(strstr(config.content_type, extension) == NULL){
            handle_error(client, request.url, 501);
            return;
        }
    }

    // If it is something other than GET, then it's a 400 invalid method.
    if (strcasecmp(request.method, "GET") != 0){
        handle_error(client, "Invalid Method", 400);
    }
    // If the HTTP version is something other than 1.1 or 1.0, then throw error invalid version.
    if ((strcmp(request.httpver, "HTTP/1.1") != 0) && (strcmp(request.httpver, "HTTP/1.0") != 0) ){
        handle_error(client, "Invalid Version", 400);
    }
    // Check URI, if not valid, then throw invalid URI error.
    if (strstr( request.url, " " ) || strstr( request.url, "\\" )){
        handle_error(client, "Invalid URI", 400);
    }

    // Addes the url to the path
    sprintf(path, "%s%s", config.DocumentRoot, request.url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, config.DirectoryIndex);

    // Check if DirectoryIndex is reachable, if not, send 404
    // If reachable, send whatever file is requested
    if (stat(path, &st) == -1) {
        handle_error(client, path, 404);
    } else {
        send_file(client, path);
    }
    
    // close the client
    close(client);
}

/*
    Initiates and sets up nessessary sockets, additional threads, and listens for 
    client responces.
*/
int open_port(int port) {
    int one = 1, client_request, sock;

    // Contain internet addresses for server and client 
    // using the sockaddr_in
    struct sockaddr_in server_address, client_address;
    socklen_t client_length = sizeof(client_address);

    // Create a socket, AF_INET is IPv4 Internet Protocol
    // SOCK_STREAM (Provides sequenced, reliable, two-way, connection-based byte streams)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("can't open socket");
        return -1;
    }
 
    // Makes the socket usuable by different clients
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
        perror("setsockopt - SO_REUSEADDR failed");
        return -1;
    }
 
    // Sets up information for server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
 
    // Attach the socket to the port given above
    if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        close(sock);
        perror("Bind failed");
        return -1;
    }
 
    // listen on the socket with a backlog of 10
    listen(sock, 5);

    while (1) {
        // accept a new connection/requests from client on a socket
        client_request = accept(sock, (struct sockaddr *) &client_address, &client_length);
        if (client_request == -1) {
            perror("Can't accept");
        }
 
        if(fork() == 0){
            // Perform the client’s request in the child process
            listentoRequests(client_request);
            exit(0);
        }
        close(client_request);
    }
    close(sock);

    return 0;
}


/*
    Main function that parses wsconf file for the port, then sends it to open_port.
    If open_port doesn't work, then it will exit with failure.
*/
int main() {
    // Parse through wsconf file and set variables when needed.
    // int port;
    // char line[256];
    // 
    char *line;
    char head[64], tail[256];
    size_t len = 0;
    int read_len = 0;
    int content_num = 0;
    // 
    FILE* conf_file = fopen("ws.conf","r");
    while((read_len = getline(&line, &len, conf_file)) != -1) {
        // Remove endline character
        line[read_len-1] = '\0';
        // Ignore comments
        if (line[0] == '#')
            continue;
        sscanf(line, "%s %s", head, tail);
        // Parse the port number - Must convert to int first
        if (!strcmp(head, "Listen")) {
            config.port = atoi(tail);
        } 
        // Parse the DocumentRoot
        if (!strncmp(head, "DocumentRoot", 12)) {
            sscanf(line, "%*s \"%s", config.DocumentRoot);
            // random quotation mark at end
            config.DocumentRoot[strlen(config.DocumentRoot)-1] = '\0';
        } 
        // Parse the DirectoryIndex
        if (!strcmp(head, "DirectoryIndex")) {
            sscanf(line, "%*s %s", config.DirectoryIndex);
            config.DocumentRoot[strlen(config.DirectoryIndex)-1] = '\0';
        }
        // Parse each content type that conf file has, then count how many
        // content types there are.
        if (head[0] == '.') {
            if (content_num < 10) {
                strcat(config.content_type, head);
                strcat(config.content_type, " ");
                
            }
        }
    }
    config.content_num = content_num;
    
    fclose(conf_file);
    // End parse

    int p = -1;
    
    // Open the port
    // p = open_port(port);
    p = open_port(config.port);

    if ((p < 0)) {
        perror("Port failed to open.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}



// (echo -en "GET /index.html HTTP/1.1\n Host: localhost \nConnection: keep-alive\n\nGET/index.html HTTP/1.1\nHost: localhost\n\n"; sleep 10) | telnet localhost 80