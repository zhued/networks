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
#include <pthread.h>

#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
 
// Get's rid of the buffer so the file is actually servable.
#define ISspace(x) isspace((int)(x))


void listentoRequests(int);
void send_file(int, const char *);
int get_line(int, char *, int);
void handle_filetype(int client, const char *filename);

/*
    Sends file requested. Specifically the picture formats. 
*/
void send_file(int client, const char *filename){
    char *sendbuf;
    FILE *requested_file;
    long fileLength;
    printf("Received request for file: %s on socket %d\n\n", filename + 1, client);
  
    requested_file = fopen(filename, "rb");
  
    if (requested_file == NULL){
        // error404(client, filename);
        printf("Received request for file");
    }
    else {
        fseek (requested_file, 0, SEEK_END);
        fileLength = ftell(requested_file);
        rewind(requested_file);
    
    sendbuf = (char*) malloc(sizeof(char)*fileLength);
    size_t result = fread(sendbuf, 1, fileLength, requested_file);
    
    if (result > 0) {
        handle_filetype(client, filename);
        send(client, sendbuf, result, 0);   
    }   
    else { printf("Send error."); exit(1); }
    }
  
  fclose(requested_file);
}

/*
    Handle filetype that the client requests
*/
void handle_filetype(int client, const char *filename)
{
    char buf[1024];
    (void)filename;
    const char* filetype;
    struct stat st; 
    off_t size;

    if (stat(filename, &st) == 0)
        size = st.st_size;

    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) {
        filetype = "";
    } else {
        filetype = dot + 1;
    }

    printf("%s", filetype);
    printf("\n");

    if(strcmp(filetype, "html") == 0){
        strcpy(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        strcpy(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
    }

    else if (strcmp(filetype, "png") == 0){
        printf("PNG request received.");
        strcpy(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
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
        printf("GIF request received.");
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






int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0){
            if (c == '\r'){
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
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
    char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;

    char *query_string = NULL;

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

    if (strcasecmp(method, "GET") == 0){
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if (*query_string == '?'){
           *query_string = '\0';
           query_string++;
        }
    }

    // Addes the url to the path
    sprintf(path, "www%s", url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");
    if (stat(path, &st) == -1) {
        while ((numchars > 0) && strcmp("\n", buf))   //read & discard header 
            numchars = get_line(client, buf, sizeof(buf));
        not_found(client);
    } else {
        if ((st.st_mode & S_IFMT) == S_IFDIR){
            strcat(path, "/index.html");
        }
        send_file(client, path);
    }
    close(client);
}

/*
    Main function that parses wsconf file for the port, then initiates and sets up
    nessessary sockets, additional threads, and listens for client responces.
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
    
    int one = 1, client_request, sock;
    pthread_t newthread;

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
 
        // Create pthread that listens for more requests
        if (pthread_create(&newthread , NULL, listentoRequests, client_request) != 0){
            perror("pthread_create");
        }
    }
    close(client_request);

}



// (echo -en "GET /index.html HTTP/1.1\n Host: localhost \nConnection: keep-alive\n\nGET/index.html HTTP/1.1\nHost: localhost\n\n"; sleep&10) | telnet localhost 80