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
 
// Get's rid of the buffer so the file is actually servable.
// Checks whitespace
#define ISspace(x) isspace((int)(x))

int BUFFER_SIZE = 1024;


void listentoRequests(int);
void send_file(int, const char *);
int get_line(int, char *, int);
char * deblank(char *);
void handle_request();
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
    char    command[8];
    char    request[256];
    char    version[16];
    char    host[128];
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
    off_t size;

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
        sprintf(buf, "Content-Length: %lld \r\n", size);
        send(client, buf, strlen(buf), 0);
        send(client, "Content-Transfer-Encoding: binary\r\n", strlen("Content-Transfer-Encoding: binary\r\n"), 0);
        send(client, "\r\n", strlen("\r\n"), 0);
    }
    else if (strcmp(filetype, "gif") == 0) {
        printf("GIF request received.\n");
        send(client, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
        send(client, "Content-Type: image/gif\r\n", strlen("Content-Type: image/gif\r\n"), 0);
        sprintf(buf, "Content-Length: %lld \r\n", size);
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
    else if (error_num == 400){
        send(client, "HTTP/1.1 400 BAD REQUEST\r\n", sizeof("HTTP/1.1 400 BAD REQUEST\r\n"), 0);
        send(client, "Content-type: text/html\r\n", sizeof("Content-type: text/html\r\n"), 0);
        send(client, "\r\n", sizeof("\r\n"), 0);
        if(strcmp(filename, "Invalid Method")){
            send(client, "<P>HTTP/1.1 400 Bad Request:  Invalid Method", sizeof("<P>HTTP/1.1 400 Bad Request:  Invalid Method"), 0);
        }
        send(client, "\r\n", sizeof("\r\n"), 0);

    }
    else if (error_num == 500) {
        send(client, "HTTP/1.0 500 Internal Server Error\r\n", strlen("HTTP/1.0 500 Internal Server Error\r\n"), 0);
        send(client, "Content-type: text/html\r\n", strlen("Content-type: text/html\r\n"), 0);
        send(client, "\r\n", strlen("\r\n"), 0);
        send(client, "HTTP/1.1 500  Internal  Server  Error:  cannot  allocate  memory\r\n", strlen("HTTP/1.1 500  Internal  Server  Error:  cannot  allocate  memory\r\n"), 0);
    }
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

// void handle_request(int fd){
//     char   *req_400_1 =    "HTTP/1.1 400 Bad Request: Invalid Method: ";
//     char    *req_400_2 =    "HTTP/1.1 400 Bad Request: Invalid URI: ";
//     char    *req_400_3 =    "HTTP/1.1 400 Bad Request: Invalid HTTP-Version: ";
//     char    *req_500 =  "HTTP/1.1 500 Internal Server Error: ";

//     struct  Request request;
//     char    buf[BUFFER_SIZE];
//     char    current[BUFFER_SIZE];
//     char    resp[2048];

//     int read_len = 0;
//     int     len = 0;

//     /* Holds the state of a request 
//      * 0 means no request received
//      * 1 means request initiated, waiting on paramaters */
//     int     req_state = 0; 
//     /* Maintains whether a request has been parsed for a single token
//      * 0 means no completed request
//      * 1 means a complete request has been parsed and will be sent */
//     int     req_complete = 0;
//     /* Maintains whether a request has been sent. Used for building segmented requests.
//      * 0 means no request has been sent
//      * 1 means a request was sent during the last batch */
//     int     req_sent = 0;
    
//     fd_set set;
//     FD_ZERO(&set);
//     FD_SET(fd, &set);

//     struct timeval timeout;
//     timeout.tv_sec = 10;
//     int rv;

//     while ((rv = select(fd + 1, &set, NULL, NULL, &timeout)) > 0) {
//         // Read input from user
//         read_len = recv(fd, &buf[len], (BUFSIZ-len), 0);
//         char    current[read_len];

//         // Copy last received line
//         strncpy(current, &buf[len], sizeof(current));

//         len += read_len;

//         // Zero end of array
//         buf[len] = '\0';
//         current[read_len-1] = '\0';

//         // Split current buffer line endings
//         char *current_ptr = &current[0];
//         char *token = strsep(&current_ptr, "\n");
//         printf("%s\n", token);
//         for(token; token != '\0'; token = strsep(&current_ptr, "\n")) {
//             // Remove trailing '\r'
//             if (token[strlen(token)-1] == '\r')
//                 token[strlen(token) - 1] = '\0';

//             //printf("'%s'\n", token);
//             // Empty line is the end of a request if one has been started
//             if (strlen(token) == 0) {
//                 if (req_state == 1)
//                     req_complete = 1;
//                 else {
//                     // Exit loop when no longer receiving input
//                     break;
//                 }
//             } else {
//                 if(req_state == 0) {    // Not currently receiving headers
//                     // Valid HTTP header received
//                     if (!strncmp(token, "GET", 3)) {
//                         sscanf(token, "%s %s %s %*s", 
//                             req.command, req.request, req.version);
//                         req_state = 1;
//                         // Unsuported version
//                         if (strncmp(req.version, "HTTP/1.0", 8) &&
//                             strncmp(req.version, "HTTP/1.1", 8)) {
//                             int rlen = sprintf(resp, "%s", req_400_3);
//                             rlen += sprintf(resp + rlen, "%s\r\n", req.version); 
//                         }

//                     } else { // Invalid request
//                         len = 0;
//                         strcpy(resp, req_400_1);
//                         strcat(resp, token);
//                         if (send(fd, resp, strlen(resp), 0) < 0)
//                             errexit("echo write: %s\n", strerror(errno));
//                     }
//                 }  else if (req_state == 1) {   // Receiving request headers
//                     // Read in host
//                     if (!strncmp(token, "Host:", 5)) 
//                         sscanf(token, "%*s %s %*s", req.host);
//                     // Read in keep-alive
//                     if (!strcmp(token, "Connection: keep-alive"))
//                         req.keep_alive = 1;

//                 }
//             }

//             /* HTTP request was captured
//             */
//             if (req_complete == 1) {
//                 if (process_request(fd, req) < 0) {
//                     sprintf(resp, "%s", req_500);
//                     // Send respnse
//                     if (write(fd, resp, strlen(resp)) < 0)
//                             errexit("echo write: %s\n", strerror(errno));
//                 }

//                 // Restart request
//                 req = EmptyRequest;
//                 req_state = 0;
//                 req_complete = 0;
//                 req_sent = 1;
//                 len = 0;
//             }
//         }
//         Kill connection
//         if (req.keep_alive == 0 && req_sent == 1)
//             return 0;
//         req_complete = 0;
//         req_sent = 0;

//     }
//     // if (rv < 0)
//     //     errexit("echo in select: %s\n", strerror(errno));
//     // // Timeout
//     // return 0;
// }


int get_line(int sock, char *buf, int size_buffer)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size_buffer - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        // printf("%02X\n", c);
        if (n > 0){
            if (c == '\r'){
                n = recv(sock, &c, 1, MSG_PEEK);
                //printf("%02X\n", c);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';
    return(i);
}

void listentoRequests(int client){
    char buf[BUFFER_SIZE];
    int numchars;
    char method[255];
    // char request_head[255];
    char url[255];
    char httpver[255];
    char path[512];
    size_t i, j;
    struct stat st;
    char *extension;

    char *query_string = NULL;

    numchars = get_line(client, buf, sizeof(buf));
    
    i = 0; j = 0;
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1)){
        method[i] = buf[j];
        i++; j++;
    }
    method[i] = '\0';
    
    // request_head[i] = '\0';
    // sscanf(request_head, "%s %s %s", method, url, httpver);
    // method[strlen(method)+1] = '\0';
    // url[strlen(method)+1] = '\0';


    // if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    // {
    //  unimplemented(client);
    //  return;
    // }

    i = 0;
    while (ISspace(buf[j]) && (j < sizeof(buf)))
        j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))){
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

    i = 0;
    while (ISspace(buf[j]) && (j < sizeof(buf)))
        j++;
    while (!ISspace(buf[j]) && (i < sizeof(httpver) - 1) && (j < sizeof(buf))){
        httpver[i] = buf[j];
        i++; j++;
    }
    httpver[i] = '\0';

    printf("%s\n", method);
    printf("%s\n", url);
    printf("%s\n", httpver);



    extension = strrchr(url, '.');
    if (extension != NULL) {
        if(strstr(config.content_type, extension) == NULL){
            handle_error(client, url, 501);
            return;
        }
    }

    if (strcasecmp(method, "GET") == 0){
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if (*query_string == '?'){
           *query_string = '\0';
           query_string++;
        }
    }

    if (strcasecmp(method, "POST") == 0)
    {
        handle_error(client, "Invalid Method", 400);
    }

    // Addes the url to the path
    sprintf(path, "%s%s", config.DocumentRoot, url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");
    if (stat(path, &st) == -1) {
        while ((numchars > 0) && strcmp("\n", buf))   //read & discard header 
            numchars = get_line(client, buf, sizeof(buf));
        handle_error(client, path, 404);
    } else {
        if ((st.st_mode & S_IFMT) == S_IFDIR){
            strcat(path, "/index.html");
        }
        send_file(client, path);
    }
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
        /* Perform the client’s request in the child process. */
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