all: tcpserver

tcpserver: tcpserver.c
	gcc -Wall tcpserver.c -o tcpserver -lssl -lcrypto
clean:
	rm -f tcpserver
