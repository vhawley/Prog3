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
#include <sys/mman.h>
#define MAX_LINE 4096

// Used by the REQ command.
// Declaration only!!! Definition is below
void request_file(char *filename, int sockfd);


// Used as a means of typecasting the sin_addr member of a sockaddr
void * get_in_addr(struct sockaddr *sa)
{
    // Always using IPv4 for Networks class, no need to consider IPv6
    return &(((struct sockaddr_in*)sa)->sin_addr);
}


// Converts the MD5 sum as hex-digits string.
void md5_to_string( char *out, unsigned char* md) {
    int i;
    for(i=0; i <MD5_DIGEST_LENGTH; i++) {
        sprintf(&out[i*2],"%02x",md[i]);
    }
}

/////////////////////////////  MAIN FUNCITON  //////////////////////////////////////////

int main(int argc, char *argv[]) {
    // Ensure proper usage ("./myftp <server> <port> <file>")
    if (argc < 3)
    {
        fprintf(stderr, "myftp: ERROR!!! Incorrect myftp takes at least 2 arguments!\n");
        fprintf(stderr, "usage: \"myftp <server> <port>\"\n");
        exit(1);
    }
    // Record the arguments in string objects
    char* server = argv[1];
    char* port = argv[2];
    
    // Ensure that the port is actually a port number
    if (atoi(port) > 65536)
    {
        fprintf(stderr, "myftp: ERROR!!! Invalid port number!\n");
        fprintf(stderr, "Port number must be in range [1-65536]");
        exit(1);
    }
    
    // Initialize variables for TCP connection
    int sockfd, servercheck;
    struct addrinfo help, *serverinfo, *p;
    char s[INET6_ADDRSTRLEN];
    
    // Clear the helper addrinfo to ensure no unexpected behavior
    memset(&help, 0, sizeof help);
    help.ai_family = AF_INET;
    help.ai_socktype = SOCK_STREAM;
    
    // Get the host information
    if ( (servercheck = getaddrinfo(server, port, &help, &serverinfo)) != 0)
    {
        fprintf(stderr, "myftp: ERROR!!! Call to getaddrinfo() failed!\n");
        fprintf(stderr, "errno: %s\n", gai_strerror(servercheck));
        exit(1);
    }
    
    // Loop through all results for server and connect to first possible
    for (p=serverinfo; p!=NULL; p=p->ai_next)
    {
        // Create a socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype, 0)) < 0)
        {
            fprintf(stderr, "myftp: ERROR!!! Call to socket() failed!\n");
            fprintf(stderr, "errno: %s\n", strerror(errno));
            exit(1);
        }
        // Connect the socket to the host
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(sockfd);
            fprintf(stderr, "myftp: ERROR!!! Call to connect() failed!\n");
            fprintf(stderr, "errno: %s\n", strerror(errno));
            exit(1);
        }
        
        // Once the connection is established break from the loop
        break;
    }
    
    // Make sure the connection server info wasnt invalidated
    if (p == NULL)
    {
        fprintf(stderr, "myftp: Failed to connect to host\n");
        exit(1);
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    
    
    // Free this, no longer needed; needed later for prog4
    //freeaddrinfo(serverinfo);
    
    
    printf("Connection to %s:%s established.\n", server, port);
    
    while (1) 
    {
        char input[MAX_LINE];
        char *command;
    	char *arg;

    	printf("Please enter a command (REQ, UPL, LIS, DEL, or XIT): ");

    	// This was changed to a single input using strtok so that invalid inputs
    	// without a second arg don't have an awkward hang while it waits 
    	// for the second input word
    	scanf("%s", input);

    	command = strtok(input, " ");
    	arg = strtok(NULL, " ");
           
    	//
    	///////////////// REQ /////////////////////////////////
    	//
    	
        if (!strcmp(command, "REQ")) 
    	{	
		int numbytes;
		char buf[MAX_LINE];
    		
		strcpy(buf, "REQ");
		// Send the name of the request to the server
		if (send(sockfd, "REQ", 4, 0) < 0)
		{
			fprintf(stderr, "myftp: ERROR!!! 1st call to send() failed!\n");
			fprintf(stderr, "errno: %s\n", strerror(errno));
			exit(1);
		}		
		
		// Wait for acknowledgement from server
		if ((numbytes = recv(sockfd, buf, sizeof(buf), 0)) < 0)
		{
			fprintf(stderr, "myftp: ERROR!!! 1st call to recv() failed!\n");
			fprintf(stderr, "errno: %s\n", strerror(errno));
			exit(1);
		}
		
		printf("Please enter a file name: ");
    		memset(input, 0, sizeof(input));
    		scanf("%s", input);
            	char *filename = input;
    		
    		request_file(filename, sockfd);

    	}
           
    	//
    	///////////////// UPL /////////////////////////////////
    	//
        else if (!strcmp(command,"UPL")) {
            // Send the name of the operation to the server
            if (send(sockfd, command, sizeof(command), 0) < 0)
            {
                fprintf(stderr, "myftp: ERROR!!! First call to send() failed!\n");
                fprintf(stderr, "errno: %s\n", strerror(errno));
            }
            
            
            printf("Enter filename: ");
	    memset(input, 0, sizeof(input));
            scanf("%s", input);
	    char *filename = input;
            
            // Send the length of the name of the requested file to the server
            uint16_t file_name_len = strlen(filename);
            uint16_t network_byte_order = htons(file_name_len);
            
            // Send the length of the name of the requested file to the server
            if (send(sockfd, &network_byte_order, sizeof(uint16_t), 0) < 0)
            {
                fprintf(stderr, "myftp: ERROR!!! Second call to send() failed!\n");
                fprintf(stderr, "errno: %s\n", strerror(errno));
            }
            
            // Send the name of the requested file to the server
            if (send(sockfd, filename, sizeof(arg), 0) < 0)
            {
                fprintf(stderr, "myftp: ERROR!!! Third call to send() failed!\n");
                fprintf(stderr, "errno: %s\n", strerror(errno));
            }
        }
       
	//
	///////////////// DEL /////////////////////////////////
	//
	
        else if (!strcmp(command,"DEL")) {
            
        }
       
	//
	///////////////// LIS /////////////////////////////////
	//
	
        else if (!strcmp(command,"LIS")) {
            
        }
       
	//
	///////////////// XIT /////////////////////////////////
	//
	
        else if (!strcmp(command,"XIT"))
	{
		if (send(sockfd, "XIT", 4, 0) < 0)
		{
			fprintf(stderr, "myftp: ERROR!!! 1st call to send() failed!\n");
			fprintf(stderr, "errno: %s\n", strerror(errno));
			exit(1);
		}		
		close(sockfd);
		break;
        }
        else 
	{
            fprintf(stderr, "myftp: ERROR!!! unknown command!\n");
        }
    }
    
    printf("Connection with server closed.\n");
    
    return 0;
}

void request_file(char *filename, int sockfd)
{
	uint16_t file_name_len = strlen(filename);
	uint16_t file_size = htons(file_name_len);

	// Send the length of the name of the requested file to the server
	if (send(sockfd, &file_size, sizeof(uint16_t), 0) < 0)
	{
        fprintf(stderr, "myftp: ERROR!!! First call to send() failed!\n");
        fprintf(stderr, "errno: %s\n", strerror(errno));
	}
	// Send the name of the requested file to the server
	if (send(sockfd, filename, strlen(filename)+6, 0) < 0)
	{
		fprintf(stderr, "myftp: ERROR!!! Second call to send() failed!\n");
		fprintf(stderr, "errno: %s\n", strerror(errno));
	}

	struct timeval begTimestamp;
	memset(&begTimestamp, 0, sizeof begTimestamp);
	gettimeofday(&begTimestamp, NULL);
	long int start_time = begTimestamp.tv_sec;
	long int start_time_usec = begTimestamp.tv_usec;
	int numbytes;
	char buf[MAX_LINE];

	// Receive the file size from the server
	if ((numbytes = recv(sockfd, buf, sizeof(uint32_t), 0)) < 0)
	{
		fprintf(stderr, "myftp: ERROR!!! 1st call to recv() failed!\n");
		fprintf(stderr, "errno: %s\n", strerror(errno));
		exit(1);
	}
	int filesize_server = ntohl(*(uint32_t*)buf);

	// Prepare buffer to receive fresh new data
	memset(buf, 0, MAX_LINE);

	if(filesize_server < 0)
	{
		printf("myftpd: File does not exist!\n");
	}
	else
	{
		// Create and open the file to be copied locally
		FILE *newfp = fopen(filename, "w+");

		// Receive the md5 hash value
		if ((numbytes = recv(sockfd, buf, 2*MD5_DIGEST_LENGTH, 0)) < 0)
		{
			fprintf(stderr, "myftp: ERROR!!! 2nd call to recv() failed!\n");
			fprintf(stderr, "errno: %s\n", strerror(errno));
			exit(1);
		}
	
		//Copy recieved buffer into the MD5 Server variable
		char *md5server;
		md5server = strdup(buf);
		memset(buf, 0, MAX_LINE);
		int total = 0;
	
		// Wait for the file contents to be sent back
		while((numbytes = recv(sockfd, buf, MAX_LINE, 0)) > 0)
		{
	       
			// Ensure there was no error receiving the data from the server
			if (numbytes  < 0)
			{
				fprintf(stderr, "myftp: ERROR!!! 3rd call to recv() failed!\n");
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
			if(filesize_server <= total)
			{
				break;
			}
			// Clear the buffer for the next round
			memset(buf, 0, MAX_LINE);
		}
	
		struct timeval endTimestamp;
		gettimeofday(&endTimestamp, NULL);
	
		char *message = malloc(sizeof(char));
		message[0] = 0;
		int fileSize;
		//attempt to read file.
		fseek(newfp, 0L, SEEK_END);
		// get size of file
		fileSize = ftell(newfp);
		fseek(newfp, 0L, SEEK_SET);
		message = malloc(sizeof(char)*fileSize);
		fread(message, 1, fileSize, newfp);
	
		// Close the file pointed to
		fclose (newfp);
	
		// Calculate the MD5of the recieved file
		unsigned char md5client[MD5_DIGEST_LENGTH];
		char md5output[2*MD5_DIGEST_LENGTH];
		MD5((unsigned char*) message, fileSize, md5client);
		munmap(message, fileSize);
		// Map the md5 value to a string
		md5_to_string(md5output,md5client);
	
		// Calculate time difference
		long int timeDifInMicros = (endTimestamp.tv_sec - start_time) * 1000000 + (endTimestamp.tv_usec - start_time_usec);
		double transtime = ((double)timeDifInMicros) / 1000000;
		double throughput = ((double)filesize_server / 1000000) / transtime;
	
		char output[512];
	
		// Check md5
		if(memcmp(md5output,md5server,MD5_DIGEST_LENGTH) != 0)
		{
			fprintf(stderr, "ERROR!!! File hashes do not match - bad transfer\n");   
			if (remove(filename) == 0)
				fprintf(stderr, "Corrupt file removed.\n");
			else
				fprintf(stderr, "Corrupt file couldn't be removed.\n");
		}

		// Print results
		sprintf(output,"\n%d bytes transferred in %.3f seconds : %.3f Megabytes/sec.\nFile MD5sum: %s\n\n",filesize_server,transtime, throughput, md5output);
		printf("%s",output);
	}
}


