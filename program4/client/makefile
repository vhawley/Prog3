all: tcpclient

tcpclient: tcpclient.c
	gcc tcpclient.c -Wall -o tcpclient -lssl -lcrypto

clean:
	rm -f tcpclient
