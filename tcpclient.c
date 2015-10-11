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

#define MAX_LINE 4096

// Used as a means of typecasting the sin_addr member of a sockaddr
void * get_in_addr(struct sockaddr *sa)
{
	// Always using IPv4 for Networks class, no need to consider IPv6
	return &(((struct sockaddr_in*)sa)->sin_addr);
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

	// Send the name of the requested file to the server
	if (send(sockfd, req_file, strlen(req_file)+1, 0) < 0)
	{
		fprintf(stderr, "tcpclient: ERROR!!! Call to send() failed!\n");
		fprintf(stderr, "errno: %s\n", strerror(errno));
	}

	// Prepare buffer to receive fresh new data
	memset(buf, 0, MAX_LINE);

	// Receive the file size from the server
	if ((numbytes = recv(sockfd, buf, MAX_LINE, 0)) < 0)
	{
		fprintf(stderr, "tcpclient: ERROR!!! Call to recv() failed!\n");
		fprintf(stderr, "errno: %s\n", strerror(errno));
		exit(1);
	}
	printf("tcpclient: Incoming file size: %s\n", buf);

	// Prepare buffer to receive fresh new data
	memset(buf, 0, MAX_LINE);

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

	close(sockfd);

	return 0;
}

