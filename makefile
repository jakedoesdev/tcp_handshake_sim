all: tcpserver.c tcpclient.c
	gcc tcpserver.c -o tserver
	gcc tcpclient.c -o tclient

clean: 
	rm tserver tclient
