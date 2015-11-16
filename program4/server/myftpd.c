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
#include <errno.h>
#include <openssl/md5.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MAX_LINE 4096
#define MAX_PENDING 5

// Converts the MD5 sum as hex-digits string.
void md5_to_string( char *out, unsigned char* md) {
    int i;
    for(i=0; i <MD5_DIGEST_LENGTH; i++) {
        sprintf(&out[i*2],"%02x",md[i]);
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in sin;
    char *buf = malloc(sizeof(char) * MAX_LINE);
    int len;
    int s, new_s;
    int port;
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

        //receive operation message
        len = recv(new_s, buf, sizeof(buf), 0);
        if (len == -1) {
            fprintf(stderr, "error receiving message\n");
            exit(1);
        }
        if (len == 0) {
            break;
        }
        
        printf("'%s'\n", buf);
        char *operation = malloc(sizeof(char) * (strlen(buf)+1));    
        strcpy(operation, buf);
        
        if (!strcmp(operation, "REQ")) {

        }
        else if (!strcmp(operation,"UPL")) {
            /// Prepare buffer to receive fresh new data
            memset(buf, 0, MAX_LINE);
            
            //receive filename length
            len = recv(new_s, buf, sizeof(buf), 0);
            if (len == -1) {
                fprintf(stderr, "error receiving message\n");
                exit(1);
            }
            if (len == 0) {
                break;
            }

            uint16_t filename_len = ntohs(*(uint16_t*)buf);
            printf("'%d'\n", len);
            
            // Prepare buffer to receive fresh new data
            memset(buf, 0, MAX_LINE);
            
            //receive filename which we now have the size of from the previous message
            len = recv(new_s, buf, sizeof(buf), 0);
            if (len == -1) {
                fprintf(stderr, "error receiving message\n");
                exit(1);
            }
            if (len == 0) {
                printf("filename length == 0, breaking...\n");
                break;
            }
            
            char *filename = malloc(sizeof(buf));
            strcpy(filename, buf);
            
            printf("'%s' '%d' '%s'\n", operation, filename_len, filename);
        }
        else if (!strcmp(operation,"DEL")) {
            /// Prepare buffer to receive fresh new data
            memset(buf, 0, MAX_LINE);
            //receive filename length           
            len = recv(new_s, buf, sizeof(uint16_t), 0);
            if (len == -1) {
                fprintf(stderr, "error receiving message\n");
                exit(1);
            }
            if (len == 0) {
                break;
            }

            uint16_t filename_len = ntohs(*(uint16_t*)buf);
            char *filefound = malloc(sizeof(char));
            // Prepare buffer to receive fresh new data
            memset(buf, 0, MAX_LINE);
            
            //receive filename which we now have the size of from the previous message
            len = recv(new_s, buf, filename_len, 0);
            if (len == -1) {
                fprintf(stderr, "error receiving message\n");
                exit(1);
            }
            if (len == 0) {
                printf("filename length == 0, breaking...\n");
                break;
            }
           
            
            //Verify that the recieved filename has the same length as the lenght sent by the client
            if( filename_len != strlen(buf)){
                fprintf(stderr, "error filename lengths do not match\n");
               
            }
            
            char *filename = malloc(sizeof(buf));
            strcpy(filename, buf);
            
            printf("'%s' '%d' '%s'\n", operation, filename_len, filename);
            
            if( access( filename, F_OK ) != -1 ) {
                filefound = "1";
                
                // Send if file was found
                if (send(new_s, filefound, strlen(filefound), 0) < 0)
                {
                    fprintf(stderr, "myftp: ERROR!!! Third call to send() failed!\n");
                    fprintf(stderr, "errno: %s\n", strerror(errno));
                }
                
                // Prepare buffer to receive fresh new data
                memset(buf, 0, MAX_LINE);

                //Receive Confirmation from client
                len = recv(new_s, buf, sizeof(buf), 0);
                printf("%s\n",buf);
                if ((len = recv(new_s, buf, sizeof(buf), 0)) < 0) {
                    fprintf(stderr, "error receiving message\n");
                    exit(1);
                }
                if (len == 0) {
                    printf("filename length == 0, breaking...\n");
                    break;
                }
                char *confirmation = malloc(sizeof(buf));
                strcpy(confirmation, buf);
                printf("%s\n",confirmation);
                if (!strcmp(confirmation,"Yes")) {
                    filefound = "1";
                    if(remove(filename) < 0)
                    {
                        fprintf(stderr, "myftpd: ERROR!!! Delete failed!\n");
                        fprintf(stderr, "errno: %s\n", strerror(errno));
                        filefound = "-1";
                    }

                    // Send if file was deleted reusing variable
                    if (send(new_s, filefound, strlen(filefound), 0) < 0)
                    {
                        fprintf(stderr, "myftp: ERROR!!! Third call to send() failed!\n");
                        fprintf(stderr, "errno: %s\n", strerror(errno));
                    }
                }
                
            } else {
                filefound = "-1";
                
                // Send if file was found
                if (send(new_s, filefound, strlen(filefound), 0) < 0)
                {
                    fprintf(stderr, "myftp: ERROR!!! Third call to send() failed!\n");
                    fprintf(stderr, "errno: %s\n", strerror(errno));
                }
            } 
            fflush(stdout);
            
        }
        else if (!strcmp(operation,"LIS")) {
            
        }
        else if (!strcmp(operation,"XIT")) {
            
        }
        else {
            fprintf(stderr, "myftp: ERROR!!! unknown operation!\n");
        }
        
        //clean stuff
        fflush(stdout);
        bzero(buf, sizeof(buf));
        printf("Waiting for next operation\n");
    }
    
    return 0;
}

//
//uint16_t filename_len = ntohs(*(uint16_t*)buf);
//// Prepare buffer to receive fresh new data
//memset(buf, 0, MAX_LINE);
//
////receive filename which we now have the size of from the previous message
//len = recv(new_s, buf, filename_len, 0);
//if (len == -1) {
//    fprintf(stderr, "error receiving message\n");
//    exit(1);
//}
//if (len == 0) {
//    break;
//}
//
//
////Verify that the recieved filename has the same length as the lenght sent by the client
//if( filename_len != strlen(buf)){
//    fprintf(stderr, "error filename lengths do not match\n");
//    
//}
//
//FILE *f;
//char *message = malloc(sizeof(char));
//message[0] = 0;
//int fileSize = -1;
//
////attempt to read file.
//f = fopen(buf, "r");
//if (f == NULL)
//{
//    fileSize = -1;
//    printf("File does not exist\n");
//}
//else {
//    fseek(f, 0L, SEEK_END);
//    fileSize = ftell(f); // get size of file
//    fseek(f, 0L, SEEK_SET);
//    message = malloc(sizeof(char)*fileSize);
//    fread(message, 1, fileSize, f);
//}
//
//uint32_t network_byte_order = htonl(fileSize);
//
////send file size
//if (send(new_s, &network_byte_order,sizeof(uint32_t), 0) < 0) {
//    fprintf(stderr, "error sending file size back to client\n");
//    exit(1);
//}
//
////Calculate MD5
//unsigned char md5check[MD5_DIGEST_LENGTH];
//
//MD5((unsigned char*) message, fileSize, md5check);
//munmap(message, fileSize);
//
//char md5string[2*MD5_DIGEST_LENGTH];
//md5_to_string(md5string, &md5check);
//
////send file MD5 hash
//if (send(new_s, md5string,2*MD5_DIGEST_LENGTH, 0) < 0) {
//    fprintf(stderr, "error sending MD5 hash back to client\n");
//    exit(1);
//}
//memset(md5string, 0, 2*MD5_DIGEST_LENGTH);
//
////send file contents back to client if it exists, empty message with length -1 otherwise
//int numbytes = 0;
//if ((numbytes = send(new_s, message, fileSize, 0)) < 0) {
//    fprintf(stderr, "error sending message back to client\n");
//    exit(1);
//}
//
//bzero(message, sizeof(message));
//free(message);
