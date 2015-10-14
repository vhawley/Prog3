//vhawley
//tderanek
//nmorales
//13 October 2015

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <openssl/md5.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MAX_LINE 4096
#define MAX_PENDING 5

int main(int argc, char *argv[]) {
    struct sockaddr_in sin;
    char *buf = malloc(sizeof(char) * MAX_LINE);
    int len;
    int s, new_s;
    int port;
    unsigned char md5check[MD5_DIGEST_LENGTH];
    if (argc < 2) {
        fprintf(stderr, "error: must specify port to run server on\n");
        exit(1);
    }
    else {
        port = atoi(argv[1]);
        if (port < 1 || port > 65536) {
            fprintf(stderr, "error: port must be positive integer\n");
            exit(1);
        }
    }
    
    //build data structure
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    
    //setup socket
    s = socket(PF_INET,SOCK_STREAM,0);
    if (s < 0) {
        fprintf(stderr, "error creating socket\n");
        exit(1);
    }
    
    //set socket option
    int optval;
    if (setsockopt(s, SOL_SOCKET,SO_REUSEADDR, &optval, sizeof(int)) < 0) {
        fprintf(stderr, "error setting socket option\n");
        exit(1);
    }
    
    //bind socket
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "error binding tcp socket\n");
        exit(1);
    }
    
    //listen on tcp port
    if (listen(s, MAX_PENDING) < 0) {
        fprintf(stderr, "error listening on tcp socket\n");
        exit(1);
    }
    
    //server loop
    while (1) {
        //accept message
        new_s = accept(s, (struct sockaddr *)&sin,(socklen_t *)&len);
        if (new_s < 0) {
            fprintf(stderr, "error accepting message from client\n");
            exit(1);
        }
        
		// Prepare buffer to receive fresh new data
		memset(buf, 0, MAX_LINE);
		
        //receive filename lenght
        len = recv(new_s, buf, sizeof(buf), 0);
        if (len == -1) {
            fprintf(stderr, "error receiving message\n");
            exit(1);
        }
        if (len == 0) {
            break;
        }
		uint16_t filename_len = ntohs(*(uint16_t*)buf);
		printf("%d ok \n", filename_len);
		// Prepare buffer to receive fresh new data
		memset(buf, 0, MAX_LINE);
		
		//receive filename which we now have the size of from the previous message
        len = recv(new_s, buf, filename_len, 0);
        if (len == -1) {
            fprintf(stderr, "error receiving message\n");
            exit(1);
        }
        if (len == 0) {
            break;
        }
		
		//Verify that the recieved filename has the same length as the lenght sent by the client
		if( filename_len != strlen(buf)){
			fprintf(stderr, "error filename lenght do not match\n");
			
		}
        
        FILE *f;
        char *message = malloc(sizeof(char));
        message[0] = 0;
        int fileSize = -1;
        
        //attempt to read file.
        f = fopen(buf, "r");
        if (f == NULL)
        {
            fileSize = -1;
			printf("File does not exist\n");
        }
        else {
            fseek(f, 0L, SEEK_END);
            fileSize = ftell(f); // get size of file
            fseek(f, 0L, SEEK_SET);
            message = malloc(sizeof(char)*fileSize);
            int result = fread(message, 1, fileSize, f);
        }
		
		uint32_t network_byte_order = htonl(fileSize);

		//send file size
        if (send(new_s, &network_byte_order,sizeof(uint32_t), 0) < 0) {
            fprintf(stderr, "error sending file size back to client\n");
            exit(1);
        }
		
        //Calculate MD5
		MD5((unsigned char*) message, fileSize, md5check);
		munmap(message, fileSize); 
		
		//send file MD% hash
        if (send(new_s, md5check,MD5_DIGEST_LENGTH, 0) < 0) {
            fprintf(stderr, "error sending MD5 hash back to client\n");
            exit(1);
        }
		
		printf("%s",message);
		
        //send file contents back to client if it exists, empty message with length -1 otherwise
        if (send(new_s, message, fileSize, 0) < 0) {
            fprintf(stderr, "error sending message back to client\n");
            exit(1);
        }
        
        //clean stuff
        close(new_s);
        fflush(stdout);
        bzero(buf, sizeof(buf));
        bzero(message, sizeof(message));
        free(message);
    }
    
    return 0;
}