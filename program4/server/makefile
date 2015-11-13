all: myftpd

myftpd: myftpd.c
	gcc -Wall myftpd.c -o myftpd -lssl -lcrypto
clean:
	rm -f myftpd
