Author: Jacob Everett

Make file instructions 
(note: make clean will only remove executables, not server/client.out)

To compile:  make
To clean:    make clean
To run: .\tserver <port number>
        .\tclient   <port number>

	
Usage instructions

When client is executed on port x (with server running on port x), the client will create a SYN TCP segment structure and will send that segment
to the server and print the segment's source and destination ports, sequence number, ack number, header length/flags, and checksum value to 
the console and a file called "client.out." When the server receives the segment, it will verify the checksum, check the flags, and create
and send a SYNACK TCP segment to the client, as well as printing the same data to the console and "server.out." When the client receives this
segment, it will verify the checksum, check the flags, and send an ACK message to the server and again print to console/client.out. The server
will verify the new checksum, check the flags, and print a connection established message to console/server.out.

Next, the client will send a FIN TCP segment, send it to the server, and print it. The server will verify the checksum, create an ACK, send to
the client, and print it. The client will verify the checksum, check the flags, and wait for another message. The server will create a segment 
with the FIN bit set, send it to the client, and print it. The client will verify, check flags, create an ACK, send to the server, print, wait
2 seconds, and exit. The server will verify the checksum, check flags, print a connection closed message, and exit.