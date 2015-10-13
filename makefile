all: tcpclient tcpserver

tcpclient: tcpclient.c
	gcc tcpclient.c -Wall -o tcpclient -lssl -lcrypto

tcpserver: tcpserver.c
	gcc tcpserver.c -Wall -o tcpserver -lssl -lcrypto

clean:
	rm -f tcpclient tcpserver
