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
void handle_filetype(int , const char *);
void handle_error(int, const char *, int );
void send_error(int , char *);

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

    if(strcmp(filetype, "html") == 0){
        send(client, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
        send(client, "Content-Type: text/html\r\n", strlen("Content-Type: text/html\r\n"), 0);
        send(client, "\r\n", strlen("\r\n"), 0);
    }

    else if (strcmp(filetype, "png") == 0){
        printf("PNG request received.\n");
        send(client, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
        sprintf(buf, "Content-Type: image/png\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Length: %lld \r\n", size);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Transfer-Encoding: binary\r\n");
        send(client, buf, strlen(buf), 0);
        strcpy(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
    }

    else if (strcmp(filetype, "gif") == 0) {
        printf("GIF request received.\n");
        strcpy(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: image/gif\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Length: %lld \r\n", size);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Transfer-Encoding: binary\r\n");
        send(client, buf, strlen(buf), 0);
        strcpy(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
    }
}

/*
    Handles errors
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
        sprintf(buf, "HTTP/1.1 400 BAD REQUEST\r\n");
        send(client, buf, sizeof(buf), 0);
        sprintf(buf, "Content-type: text/html\r\n");
        send(client, buf, sizeof(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, sizeof(buf), 0);
        if(strcmp(filename, "Invalid Method")){
            sprintf(buf, "<P>HTTP/1.1 400 Bad Request:  Invalid Method");
            send(client, buf, sizeof(buf), 0);
        }
        sprintf(buf, "\r\n");
        send(client, buf, sizeof(buf), 0);

    }
    else if (error_num == 500) {
        sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "HTTP/1.1 500  Internal  Server  Error:  cannot  allocate  memory\r\n");
        send(client, buf, strlen(buf), 0);
    }
    else if (error_num == 501) {
        sprintf(buf, "HTTP/1.1 501 Method Not Implemented\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</TITLE></HEAD>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<BODY><P>HTTP/1.1 501  Not Implemented:  %s\r\n", filename);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</BODY></HTML>\r\n");
        send(client, buf, strlen(buf), 0);
    }
}


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

char * deblank(char *str)
{
    char *out = str, *put = str;

    for(; *str != '\0'; ++str){
        if(*str != ' ')
            *put++ = *str;
    }
    *put = '\0';

    return out;
}

void listentoRequests(int client){
    char buf[BUFFER_SIZE];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;
    char *extension;
    char contentType[100];
    char directoryIndex[100]; 
    char document_root[100];
    char line[256];

    char *query_string = NULL;

    FILE* file = fopen("ws.conf", "r");
    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        if(line[0] != '#'){
            char* parse = strtok(line, " " );
            while (parse){
                if(strcmp(parse, "DirectoryIndex") == 0){
                    parse = strtok(NULL, "");
                    // strcpy(test, parse);
                    strcat(directoryIndex, strtok(deblank(parse),"\n"));
                }
                else if(strcmp(parse, "DocumentRoot") == 0){
                    parse = strtok(NULL, " ");
                    strcat(document_root, strtok(deblank(parse), "\n"));
                }
                else if(parse[0] == '.'){
                    strcat(contentType, parse);
                    parse = strtok(NULL, ",");
                }
                else{
                    parse = strtok(NULL, " "); 
                }
            }
        }
    } 
    fclose(file);

    numchars = get_line(client, buf, sizeof(buf));
    
    i = 0; j = 0;
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1)){
        method[i] = buf[j];
        i++; j++;
    }
    
    method[i] = '\0';

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

    extension = strrchr(url, '.');
    if (extension != NULL) {
        if(strstr(contentType, extension) == NULL){
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
    sprintf(path, "www%s", url);
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
    int port;
    char line[256];
    FILE* conf_file = fopen("ws.conf","r");
    while(fgets(line, sizeof(line), conf_file)){
        if(line[0] != '#'){
            char* parse = strtok(line, " ");
            while (parse){
                if (strcmp(parse, "Listen") == 0)
                {
                    parse = strtok(NULL, " ");
                    port = atoi(parse);
                }
                else{
                    parse = strtok(NULL, " ");
                }
            }
        }
    }
    fclose(conf_file);
    // End parse

    int p = -1;
    
    p = open_port(port);

    if ((p < 0)) {
        perror("Port failed to open.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}



// (echo -en "GET /index.html HTTP/1.1\n Host: localhost \nConnection: keep-alive\n\nGET/index.html HTTP/1.1\nHost: localhost\n\n"; sleep 10) | telnet localhost 80