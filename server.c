/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT "41008"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
	    char buf[1024];
	    char *filename;
	    int numbytes;

	    int filename_len;

	    // Record the name of the file that the client wants
	    if ((numbytes = recv(new_fd, buf, 1023, 0)) < 0)
	    {
	    	fprintf(stderr, "server: ERROR!!! Call to recv() failed!]\n");
		fprintf(stderr, "errno: %s\n", strerror(errno));
	    }
	    else
	    {
	    	filename_len = atoi(buf);
	    	printf("Message: %d : %s\n", filename_len, buf);
	    }

	    // Record the name of the file that the client wants
	    if ((numbytes = recv(new_fd, buf, 1023, 0)) < 0)
	    {
	    	fprintf(stderr, "server: ERROR!!! Call to recv() failed!]\n");
		fprintf(stderr, "errno: %s\n", strerror(errno));
	    }
	    else
	    {
	    	filename = buf;
		filename[numbytes] = "\0";
	    	printf("Message: %s\n", filename);
	    }

	    // Send the file contents back to the client
	    // Open the file
	    int MAX_LINE = 1024, bytes_read, bytes_sent;
	    struct stat st;
	    int file_size;
	    char file_size_str[100];
	    if (stat(filename, &st) == 0)
	    	file_size = st.st_size;
	    else
	    	file_size = -1;
	    memset(file_size_str, 0, 100);
	    sprintf(file_size_str, "%d", file_size);
	    
            if ( (bytes_sent = send(new_fd, file_size_str, strlen(file_size_str), 0)) == -1)
            	perror("send");

	    int fp = open(filename, O_RDONLY);
	    if (fp < 0)
	    {
	    	fprintf(stderr, "server: Call to open() failed!\n");
		fprintf(stderr, "file '%s' could not be found!\n", filename);
		fprintf(stderr, "errno: %s\n", strerror(errno));
		exit(1);
	    }
	    char sendbuf[MAX_LINE];
	    memset(sendbuf, 0, MAX_LINE);
	    // Read part of file
	    while ( (bytes_read = read(fp, sendbuf, MAX_LINE)) > 0)
	    {
	    	// Send part of the file
            	if ( (bytes_sent = send(new_fd, sendbuf, strlen(sendbuf)-1, 0)) == -1)
            		perror("send");
	        memset(sendbuf, 0, MAX_LINE);
	    }
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}
