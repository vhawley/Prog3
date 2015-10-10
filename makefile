all: tcpclient

tcpclient: tcpclient.c
	gcc tcpclient.c -Wall -o tcpclient

clean:
	rm -f tcpclient
