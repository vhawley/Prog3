//vhawley
//tderanek
//nmorales
//13 October 2015

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>
#include <arpa/inet.h>

#include <openssl/md5.h>

#define MAX_LINE 4096

// Used as a means of typecasting the sin_addr member of a sockaddr
void * get_in_addr(struct sockaddr *sa)
{
    // Always using IPv4 for Networks class, no need to consider IPv6
    return &(((struct sockaddr_in*)sa)->sin_addr);
}


// Print the MD5 sum as hex-digits.
void print_md5_sum( char *out,unsigned char* md) {
    int i;
    //char out[512];
    for(i=0; i <MD5_DIGEST_LENGTH; i++) {
        sprintf(&out[i*2],"%02x",md[i]);
    }
}

/////////////////////////////  MAIN FUNCITON  //////////////////////////////////////////

int main(int argc, char *argv[])
{
    // Ensure proper usage ("./tcpclient <server> <port> <file>")
    if (argc != 4)
    {
        fprintf(stderr, "tcpclient: ERROR!!! Incorrect tcpclient takes three arguments!\n");
        fprintf(stderr, "usage: \"./tcpclient <server> <port> <file>\"\n");
        exit(1);
    }
    // Record the arguments in string objects
    char* server = argv[1];
    char* port = argv[2];
    char* req_file = argv[3];
    // Ensure that the port is actually a port number
    if (atoi(port) > 65536)
    {
        fprintf(stderr, "tcpclient: ERROR!!! Invalid port number!\n");
        fprintf(stderr, "Port number must be in range [1-65536]");
        exit(1);
    }
    
    // Initialize variables for TCP connection
    int sockfd, numbytes, servercheck;
    char buf[MAX_LINE];
    char comfile[MAX_LINE];
    struct addrinfo help, *serverinfo, *p;
    char s[INET6_ADDRSTRLEN];
    
    // Clear the helper addrinfo to ensure no unexpected behavior
    memset(&help, 0, sizeof help);
    help.ai_family = AF_INET;
    help.ai_socktype = SOCK_STREAM;
    
    // Get the host information
    if ( (servercheck = getaddrinfo(server, port, &help, &serverinfo)) != 0)
    {
        fprintf(stderr, "tcpclient: ERROR!!! Call to getaddrinfo() failed!\n");
        fprintf(stderr, "errno: %s\n", gai_strerror(servercheck));
        exit(1);
    }
    
    // Loop through all results for server and connect to first possible
    for (p=serverinfo; p!=NULL; p=p->ai_next)
    {
        // Create a socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype, 0)) < 0)
        {
            fprintf(stderr, "tcpclient: ERROR!!! Call to socket() failed!\n");
            fprintf(stderr, "errno: %s\n", strerror(errno));
            exit(1);
        }
        // Connect the socket to the host
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(sockfd);
            fprintf(stderr, "tcpclient: ERROR!!! Call to connect() failed!\n");
            fprintf(stderr, "errno: %s\n", strerror(errno));
            exit(1);
        }
        
        // Once the connection is established break from the loop
        break;
    }
    
    // Make sure the connection server info wasnt invalidated
    if (p == NULL)
    {
        fprintf(stderr, "tcpclient: Failed to connect to host\n");
        exit(1);
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("tcpclient: Connected to %s\n", s);
    // Free this, no longer needed
    freeaddrinfo(serverinfo);
    
    // Create and open the file to be copied locally
    FILE *newfp = fopen("newfile.txt", "w+");
    
    uint16_t file_name_len = strlen(req_file);
    uint16_t network_byte_order = htons(file_name_len);;
    
    // Send the lenght of the name of the requested file to the server
    if (send(sockfd, &network_byte_order, sizeof(uint16_t), 0) < 0)
    {
        fprintf(stderr, "tcpclient: ERROR!!! Call to send() failed!\n");
        fprintf(stderr, "errno: %s\n", strerror(errno));
    }
    printf("%s \n",req_file);
    // Send the name of the requested file to the server
    if (send(sockfd, req_file, strlen(req_file)+6, 0) < 0)
    {
        fprintf(stderr, "tcpclient: ERROR!!! Call to send() failed!\n");
        fprintf(stderr, "errno: %s\n", strerror(errno));
    }
    
    // Receive the file size from the server
    if ((numbytes = recv(sockfd, buf, sizeof(uint32_t), 0)) < 0)
    {
        fprintf(stderr, "tcpclient: ERROR!!! Call to recv() failed!\n");
        fprintf(stderr, "errno: %s\n", strerror(errno));
        exit(1);
    }
    int filesize_server = ntohl(*(uint32_t*)buf);
    printf("tcpclient: Incoming file size: %d\n", filesize_server);
    // Prepare buffer to receive fresh new data
    memset(buf, 0, MAX_LINE);
    
    if(filesize_server < 0){
        printf("File does not exists \n");
        exit(1);
    }
    
    // Receive the md5 hash value
    char md5server[MD5_DIGEST_LENGTH];
    char md5string[MD5_DIGEST_LENGTH];
    
    if ((numbytes = recv(sockfd, buf, MD5_DIGEST_LENGTH, 0)) < 0)
    {
        fprintf(stderr, "tcpclient: ERROR!!! Call to recv() failed!\n");
        fprintf(stderr, "errno: %s\n", strerror(errno));
        exit(1);
    }
    strcpy(md5server, buf);
    //memcpy(md5server, buf,MD5_DIGEST_LENGTH);
    printf("In buffer: %s \n", buf);
    memset(buf, 0, MAX_LINE);
    print_md5_sum(md5string,md5server);
    printf("MD5: %s \n", md5string);
    
    // Prepare buffer to receive fresh new data
    //memset(buf, 0, MAX_LINE);
    printf("In buffer: %s \n", buf);
    strcat(comfile,buf);
    printf("IN com: %s \n",comfile);
    // Wait for the file contents to be sent back
    while ((numbytes = recv(sockfd, buf, MAX_LINE, 0)) > 0)
    {
        // Ensure there was no error receiving the data from the server
        if (numbytes  < 0)
        {
            fprintf(stderr, "tcpclient: ERROR!!! Call to recv() failed!\n");
            fprintf(stderr, "errno: %s\n", strerror(errno));
            exit(1);
        }
        
        // Write to the new file
        if (fputs(buf, newfp) < 0)
        {
            fprintf(stderr, "tcpclient: ERROR!!! Call to fputs() failed!\n");
            fprintf(stderr, "errno: %s\n", strerror(errno));
            exit(1);
        }
        
        // Clear the buffer for the next round
        memset(buf, 0, MAX_LINE);
    }
    // Print confirmation message
    printf("tcpclient: File '%s' received from %s\n", req_file, server);
    
    char *message = malloc(sizeof(char));
    message[0] = 0;
    int fileSize;
    
    //attempt to read file.
    fseek(newfp, 0L, SEEK_END);
    fileSize = ftell(newfp); // get size of file
    fseek(newfp, 0L, SEEK_SET);
    message = malloc(sizeof(char)*fileSize);
    int result = fread(message, 1, fileSize, newfp);
    
    
    char md5client[MD5_DIGEST_LENGTH];
    MD5((unsigned char*) message, fileSize, md5client);
    munmap(message, fileSize); 
    
    print_md5_sum(md5string,md5client);
    printf("MD5 Client: %s \n", md5string);
    int dif;
    
    if(strcmp(md5client,md5server) != 0){
        printf("File failed md5 hash check string \n");	
    }
    
    if((dif = memcmp(md5client,md5server,MD5_DIGEST_LENGTH)) != 0){
        printf("File failed md5 hash check \n");	
    }
    printf("%d",dif);
    close(sockfd);
    
    return 0;
}