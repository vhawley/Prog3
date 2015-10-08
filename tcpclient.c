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

int main(int argc, char *argv[])
{
	// Ensure proper usage ("./tcpclient <server> <port> <file>")
	if (argc != 4)
	{
		cout << "udpclient: ERROR! Incorrect udpclient takes three arguments!" << endl;
		cout << "usage: \"./udpclient <server> <port> <file>\"" << endl;
		exit(1);
	}
	// Record the arguments in string objects
	char* server = argv[1];
	char* port = argv[2];
	char* req_file = argv[3];
	

	struct hostent *hp;
	struct sockaddr_in sockin;
	char *host;
	char *buffer[MAX_LINE]
	int s;
	int len;

	// First we must translate the passed in hostname to its IP address
	hp = gethostbyname(host);

	// DONT TRUST ANYTHING BELOW HERE!!! IT IS FOR REFERENCE

	/*

	struct addrinfo hints, *serverinfo, *localinfo;

	// Prepare hints struct for getaddrinfo call
	memset(&hints, 0, sizeof hints);
	hints.ai_family =  AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;	// Fill in IP automatically
	
	// Lookup the hostname and store its info
	if ((s=getaddrinfo(server.c_str(),port,&hints,&serverinfo)) != 0)
	{
		cout << "udpclient: getaddrinfo error: " << gai_strerror(s) << endl;
		exit(1);
	}
	
	// Create a socket for the client
	if ( (s=socket(serverinfo->ai_family,serverinfo->ai_socktype,serverinfo->ai_protocol)) < 0)
	{
		cout << "udpclient: ERROR! Socket creation failed!" << endl;
		exit(1);
	}

	// Send the datagram to the server and record the time of day for RTT calc
	int buflen = strlen(buffer.c_str());
	
	int sent_bytes = sendto(s,buffer.c_str(),buflen,0,serverinfo->ai_addr,serverinfo->ai_addrlen);
	if (sent_bytes < 0)
	{
		cout << "udpclient: ERROR! Packet not sent!" << endl;
		cout << "errno: " << strerror(errno) << endl;
		exit(1);
	}
	// Ensure all of the bytes were sent
	if (sent_bytes < buflen )
	{
		cout << "Handle unsent case:" << endl;
		cout << buflen << ":" << sent_bytes << endl;
		cout << buffer.c_str() << endl;
	}


	// Step 3: receive the file from the server
	// Set up parameters for recvfrom call
	char str_rcvd[buflen];
	struct sockaddr_storage raddr;
	socklen_t slen = sizeof raddr;

	// Call recvfrom and check return value for correctness
	int recv_bytes = recvfrom(s,str_rcvd,16384,0,(sockaddr *)(&raddr),&slen);
	if (recv_bytes < 0)
	{
		cout << "udpclient: ERROR! Packet not properly received!" << endl;
		cout << "errno: " << strerror(errno) << endl;
		exit(1);
	}

	str_rcvd[recv_bytes] = 0;


	
	// Step 4: decrypt the file
	int i, max = strlen(str_rcvd);
	for (i=ceil(max/2)-1; i>=0; i--)
	{
		if (i%2 == 0)
		{
			char temp = str_rcvd[i];
			str_rcvd[i] = str_rcvd[max-i-1];
			str_rcvd[max-i-1] = temp;
		}
	}
	
	cout << str_rcvd;
//	for (int i=0; i<strlen(str_rcvd); i++)
//		cout << i%10;
	cout << endl << strlen(str_rcvd) << endl;

	// Step 5: Compute the RTT of the message (microseconds)
	int time = time2.tv_usec - time1.tv_usec;
	cout << "RTT (microseconds): " << time << endl;


	// Step 6: Display resulting response from server and RTT

	*/

	return 0;
}
