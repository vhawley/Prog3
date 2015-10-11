//vhawley
//tderanek
//nmorales
//13 October 2015

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define MAX_LINE 4096
#define MAX_PENDING 5

int main(int argc, char *argv[]) {
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int len;
    int s, new_s;
    int port;
    
    if (argc < 2) {
        fprintf(stderr, "error: must specify port to run server on\n");
        exit(1);
    }
    else {
        port = atoi(argv[1]);
        if (port < 1) {
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
    
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "error binding tcp socket\n");
        exit(1);
    }
    
    if (listen(s, MAX_PENDING) < 0) {
        fprintf(stderr, "error listening on tcp socket\n");
        exit(1);
    }
    
    printf("Welcome to TCP SERVER\n");
    
    while (1) {
        new_s = accept(s, (struct sockaddr *)&sin, &len);
        if (new_s < 0) {
            fprintf(stderr, "error accepting message from client");
            exit(1);
        }
        
        while (1) {
            len = recv(new_s, buf, sizeof(buf), 0);
            if (len == -1) {
                fprintf(stderr, "error receiving message");
                exit(1);
            }
            if (len == 0) {
                break;
            }
            printf("TCP Server Received: %s", buf);
            close(new_s);
        }
        fflush(stdout);
    }
}