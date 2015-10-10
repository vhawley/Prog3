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

#define MAX_LINE 4096

void * get_in_addr(struct sockaddr *sa)
{
	// Always using IPv4 for Networks class, no need to consider IPv6
	return &(((struct sockaddr_in*)sa)->sin_addr);
}


/////////////////////////////  MAIN FUNCITON  ///////////////////////////////////////////////

int main(int argc, char *argv[])
{
	// Ensure proper usage ("./tcpclient <server> <port> <file>")
	if (argc != 4)
	{
		fprintf(stderr, "tcpclient: ERROR!!! Incorrect udpclient takes three arguments!\n");
		fprintf(stderr, "usage: \"./udpclient <server> <port> <file>\"\n");
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
		fprintf(stderr, "Port number must be in range [1-65536]")
		exit(1);
	}
	
	// Initialize variables for TCP connection
	int sockfd, numbytes, servercheck;
	char buf[MAX_LINE];
	struct addrinfo help, *serverinfo, *p;
	char s[INET6_ADDRSTRLEN];

	// Clear the helper addrinfo to ensure no unexpected behavior
	memset($hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	// Get the host information
	if ( (servercheck = getaddrinfo(server, port, &help, &serverinfo)) != 0 )
	{
		fprintf(stderr, "tcpclient: ERROR!!! Call to getaddrinfo() failed!\n");
		fpritnf(stderr, "errno: %s\n", gai_strerror(servercheck));
		exit(1);
	}

	// Loop through all results for server and connect to first possible
	for (p=serverinfo; p!=NULL; p=p->ai_next)
	{
		// Create a socket 
		if ((sockfd = socket(p->ai_family, p->ai_socktype, 0) == -1)
		{
			fprintf(stderr, "tcpclient: ERROR!!! Call to socket() failed!\n");
			fprintf(stderr, "errno: %s\n", strerror(errno);
			exit(1);
		}
		// Connect the socket to the host
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
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
	printf("tcpclient: Connect to %s\n", s);

	freeaddrinfo(servinfo);

	if ((numbytes = recv(sockfd, buf, MAX_LINE-1, 0) < 0)
	{
		fprintf(stderr, "tcpclient: ERROR!!! Call to recv() failed!\n");
		fprintf(stderr, "errno: %s\n", strerror(errno));
		exit(1);
	}

	buf[numbytes] = "tcpclient: received '%s'\n", buf);

	close(sockfd);

	return 0;
}
