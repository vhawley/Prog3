//Thomas Deranek - tderanek
//Victor Hawley - vhawley
//Norman Morales - nmorales
//17 November 2015

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>

#include <openssl/md5.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h> 
#define MAX_LINE 4096
#define MAX_PENDING 5

// Converts the MD5 sum as hex-digits string.
void md5_to_string( char *out, unsigned char *md) {
    int i;
    for(i=0; i <MD5_DIGEST_LENGTH; i++) {
        sprintf(&out[i*2],"%02x",md[i]);
    }
}

// Declaration of send_file(). Definition is below
void send_file();

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

    printf("Server initiated.\n");
    
    //server loop
    while (1) 
    {
	printf("Listening for a new client ... \n");
        //accept message
        new_s = accept(s, (struct sockaddr *)&sin,(socklen_t *)&len);
        if (new_s < 0) {
            fprintf(stderr, "error accepting message from client\n");
            exit(1);
        }
        
	// Prepare buffer to receive fresh new data
	memset(buf, 0, MAX_LINE);

	printf("Client connection established.\n");
        
        while (1)
        {
            memset(buf,0,MAX_LINE);
            //receive operation message
            len = recv(new_s, buf, sizeof(buf), 0);
            if (len == -1) {
                fprintf(stderr, "error receiving message\n");
                exit(1);
            }
            else if (len == 0)
            {
                printf("The client dropped the connection.\n");
                break;
            }
            
            char *operation = malloc(sizeof(char) * (strlen(buf)+1));
            strcpy(operation, buf);
            
            if (!strcmp(operation, "REQ"))
            {
                printf("Querying client for file name ... \n");
                memset(buf, 0, sizeof(buf));
                strcpy(buf, "ACK_REQ");
                // send acknowledgment of the requestion to the client
                if (send(new_s, buf, sizeof(buf), 0) < 0)
                {
                    fprintf(stderr, "error sending REQ_ACK to client\n");
                    exit(1);
                }
                
                send_file(new_s);
            }
            else if (!strcmp(operation,"UPL")) {
                /// Prepare buffer to receive fresh new data
                memset(buf, 0, MAX_LINE);
                
                //receive filename length
                len = recv(new_s, buf, sizeof(uint16_t), 0);
                if (len == -1) {
                    fprintf(stderr, "error receiving filename length message\n");
                    exit(1);
                }
                if (len == 0) {
                    break;
                }
                
                uint16_t filename_len = ntohs(*(uint16_t*)buf);
                
                // Prepare buffer to receive fresh new data
                memset(buf, 0, MAX_LINE);
                
                //receive filename which we now have the size of from the previous message
                len = recv(new_s, buf, filename_len+1, 0);
                if (len == -1) {
                    fprintf(stderr, "error receiving filename message\n");
                    exit(1);
                }
                if (len == 0) {
                    printf("filename length == 0, breaking...\n");
                    break;
                }
                
                char *filename = malloc(sizeof(buf));
                strcpy(filename, buf);
                
                FILE *newfp = fopen(filename, "w+");
                
                //send ready message
                if (send(new_s, "READY", sizeof(char)*6, 0) < 0)
                {
                    fprintf(stderr, "myftpd: ERROR!!! Call to send() failed!\n");
                    fprintf(stderr, "errno: %s\n", strerror(errno));
                }
                
                // Prepare buffer to receive fresh new data
                memset(buf, 0, MAX_LINE);
                
                
                //receive file size
                len = recv(new_s, buf, sizeof(uint32_t), 0);
                if (len == -1) {
                    fprintf(stderr, "error receiving file size message\n");
                    exit(1);
                }
                if (len == 0) {
                    printf("fileSize == 0, breaking...\n");
                    break;
                    
                }
                
                uint32_t fileSizeClient = ntohl(*(uint32_t*)buf);
                int total = 0;
                int numbytes;
                //begin timing
                struct timeval begTimestamp;
                memset(&begTimestamp, 0, sizeof(begTimestamp));
                gettimeofday(&begTimestamp, NULL);
                long int start_time = begTimestamp.tv_sec;
                long int start_time_usec = begTimestamp.tv_usec;
                
                //receive file contents
                while((numbytes = recv(new_s, buf, MAX_LINE, 0)) > 0) {
                    // Ensure there was no error receiving the data from the server
                    if (numbytes  < 0)
                    {
                        fprintf(stderr, "myftp: ERROR!!! Third call to recv() failed!\n");
                        fprintf(stderr, "errno: %s\n", strerror(errno));
                        exit(1);
                    }
                    total += numbytes;
                    // Write to the new file
                    if (fwrite(buf,1,numbytes, newfp) < 0)
                    {
                        fprintf(stderr, "myftp: ERROR!!! Call to fputs() failed!\n");
                        fprintf(stderr, "errno: %s\n", strerror(errno));
                        exit(1);
                    }
                    if(fileSizeClient <= total){
                        break;
                    }
                    // Clear the buffer for the next round
                    memset(buf, 0, MAX_LINE);
                }
                
                struct timeval endTimestamp;
                gettimeofday(&endTimestamp, NULL);
                long int end_time = endTimestamp.tv_sec;
                long int end_time_usec = endTimestamp.tv_usec;
                
                memset(buf, 0, MAX_LINE);
                
                // Receive the md5 hash value
                if ((numbytes = recv(new_s, buf, 2*MD5_DIGEST_LENGTH, 0)) < 0)
                {
                    fprintf(stderr, "myftp: ERROR!!! 2nd call to recv() failed!\n");
                    fprintf(stderr, "errno: %s\n", strerror(errno));
                    exit(1);
                }
                
                char *md5client = strdup(buf);
                
                int fileSize;
                char *message = malloc(sizeof(char));
                message[0] = 0;
                //attempt to read file.
                fseek(newfp, 0L, SEEK_END);
                // get size of file
                fileSize = ftell(newfp);
                fseek(newfp, 0L, SEEK_SET);
                message = malloc(sizeof(char)*fileSize);
                fread(message, 1, fileSize, newfp);
                
    
                
                // Calculate the MD5of the recieved file
                unsigned char md5server[MD5_DIGEST_LENGTH];
                char md5output[2*MD5_DIGEST_LENGTH];
                MD5((unsigned char*) message, fileSize, md5server);
                munmap(message, fileSize);
                // Map the md5 value to a string
                md5_to_string(md5output,md5server);
                
                //calculate time difference
                long int timeDifInMicros = (end_time - start_time) * 1000000 + (end_time_usec - start_time_usec);
                double transtime = ((double)timeDifInMicros) / 1000000;
                double throughput = ((double)fileSize / 1000000) / transtime;
                
                char output[MAX_LINE];
                fflush(stdout);
                
                //check md5
                
                int dif = strcmp(md5output,md5client);
                if(dif != 0)
                {
                    fprintf(stderr, "myftp: ERROR!!! File hashes do not match - bad transfer\n");
                    sprintf(output, "File hashes do not match - bad transfer.\n");
                    if (remove(filename))
                        fprintf(stderr, "myftp: Corrupt file removed.");
                    else
                        fprintf(stderr, "myftp: Corrupt file could not be removed.");
                }
                else {
                    sprintf(output,"%d bytes transferred in %.3f seconds : %.3f Megabytes/sec.\nFile MD5sum: %s",fileSize,transtime, throughput, md5output);
                }
                
                uint16_t message_length = strlen(output) + 1;
                uint16_t message_byte_order = htons(message_length);
                
                // Send the length of the output message to the server
                if (send(new_s, &message_byte_order, sizeof(uint16_t), 0) < 0)
                {
                    fprintf(stderr, "myftp: ERROR!!! Call to send() failed!\n");
                    fprintf(stderr, "errno: %s\n", strerror(errno));
                }
                
                // Send the result of the transfer to the server
                if (send(new_s, output, strlen(output) + 1, 0) < 0)
                {
                    fprintf(stderr, "myftp: ERROR!!! Call to send() failed!\n");
                    fprintf(stderr, "errno: %s\n", strerror(errno));
                }
                fflush(stdout);
                fclose(newfp);
            }
            else if (!strcmp(operation,"DEL"))
            {
                /// Prepare buffer to receive fresh new data
                memset(buf, 0, MAX_LINE);
                
                //receive filename length
                len = recv(new_s, buf, sizeof(uint16_t), 0);
                if (len == -1) {
                    fprintf(stderr, "error receiving filename length message\n");
                    exit(1);
                }
                if (len == 0) {
                    break;
                }
                
                uint16_t filename_len = ntohs(*(uint16_t*)buf);
                
                // Prepare buffer to receive fresh new data
                memset(buf, 0, MAX_LINE);
                
                //receive filename which we now have the size of from the previous message
                len = recv(new_s, buf, filename_len, 0);
                if (len == -1) {
                    fprintf(stderr, "error receiving filename message\n");
                    exit(1);
                }
                if (len == 0) {
                    printf("filename length == 0, breaking...\n");
                    break;
                }
                
                char *filename = malloc(sizeof(buf));
                strcpy(filename, buf);


                //Verify that the recieved filename has the same length as the lenght sent by the client
                if( filename_len != strlen(buf)){
                    fprintf(stderr, "error filename lengths do not match\n");
                }

                char *filefound = malloc(sizeof(char));
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
            else if (!strcmp(operation,"LIS"))
            {
                char listing[MAX_LINE];
                char item[256];
                
                DIR *d;
                struct dirent *dir;
                //Open current directory and start buiding the listing
                d = opendir(".");
                if (d)
                {
                    while ((dir = readdir(d)) != NULL)
                    {
                      printf("%s\n", dir->d_name);
                      sprintf(item,"%s\n",dir->d_name);                     
                      strcat(listing, item);
                        
                    }

                closedir(d);
                }
                
                uint32_t dicrec_len = strlen(listing);
                uint32_t network_byte_order = htonl(dicrec_len);

                // Send the size of the directory
                if (send(new_s, &network_byte_order, sizeof(uint32_t), 0) < 0)
                {
                    fprintf(stderr, "myftp: ERROR!!! Second call to send() failed!\n");
                    fprintf(stderr, "errno: %s\n", strerror(errno));
                }
                
                // Send the listing of the directory
                if (send(new_s, listing, strlen(listing), 0) < 0)
                {
                    fprintf(stderr, "myftp: ERROR!!! Second call to send() failed!\n");
                    fprintf(stderr, "errno: %s\n", strerror(errno));
                }
                
                /// Prepare buffer to receive fresh new data
                memset(listing, 0, MAX_LINE);

            }
            else if (!strcmp(operation,"XIT"))
            {
                printf("Closing connection with client.\n");
                // break out of the while loop and look for a new client
                break;
            }
            else
	        {
                fprintf(stderr, "myftp: ERROR!!! unknown operation!\n");
            }
            //clean stuff
            fflush(stdout);
            bzero(buf, sizeof(buf));
        }
    }
    
    return 0;
}

void send_file(int new_s)
{
    char buf[MAX_LINE];
    int len;

    uint16_t filename_len = ntohs(*(uint16_t*)buf);
    // Prepare buffer to receive fresh new data
    memset(buf, 0, MAX_LINE);

    // receive filename length
    len = recv(new_s, buf, sizeof(buf), 0);
    if (len < 0) 
    {
        fprintf(stderr, "error receiving message\n");
        exit(1);
    }
    
    filename_len = ntohs(*(uint16_t*)buf);

    //receive filename which we now have the size of from the previous message
    len = recv(new_s, buf, filename_len, 0);
    if (len < 0) {
       fprintf(stderr, "error receiving message\n");
       exit(1);
    }
    

    //Verify that the recieved filename has the same length as the lenght sent by the client
    if( filename_len != strlen(buf)){
       fprintf(stderr, "error filename lengths do not match\n");
       
    }

    printf("Sending file '%s' to client ...\n", buf);

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
       fread(message, 1, fileSize, f);
    }

    uint32_t network_byte_order = htonl(fileSize);

    //send file size
    if (send(new_s, &network_byte_order,sizeof(uint32_t), 0) < 0) {
       fprintf(stderr, "error sending file size back to client\n");
       exit(1);
    }

    //Calculate MD5
    unsigned char md5check[MD5_DIGEST_LENGTH];

    MD5((unsigned char*) message, fileSize, md5check);
    munmap(message, fileSize);

    char md5string[2*MD5_DIGEST_LENGTH];
    md5_to_string(md5string, &md5check);

    //send file MD5 hash
    if (send(new_s, md5string,2*MD5_DIGEST_LENGTH, 0) < 0) {
       fprintf(stderr, "error sending MD5 hash back to client\n");
       exit(1);
    }
    memset(md5string, 0, 2*MD5_DIGEST_LENGTH);

    //send file contents back to client if it exists, empty message with length -1 otherwise
    int numbytes = 0;
    if ((numbytes = send(new_s, message, fileSize, 0)) < 0) {
       fprintf(stderr, "error sending message back to client\n");
       exit(1);
    }
    
    printf("Finished sending file to client.\n");
    bzero(message, sizeof(message));
    free(message);

    // TODO: Figure out why this line is needed
    // without it, the program seems to think a blank message
    // is getting sent to it after sending the file
    recv(new_s, buf, sizeof(buf), 0);

}