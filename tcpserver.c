/* 
Author: Jacob Everett
Description: TCP server that simulates a 3-way handshake and connection closing.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct tcp_hdr{
                unsigned short int src;
                unsigned short int des;
                unsigned int seq;
                unsigned int ack;
                unsigned short int hdr_flags;
                unsigned short int rec;
                unsigned short int cksum;
                unsigned short int ptr;
                unsigned int opt;
                char data [1024];
              };

//checksum calculation declaration
unsigned short int check(unsigned short int arr[], int n);

//print segment to console and server.out
void printSeg(struct tcp_hdr *seg);

int main(int argc, char **argv)
{
  struct tcp_hdr tSeg;               //tcp_hdr instance
  unsigned short int cksum_arr[524]; //array for calculating chksum
  unsigned int cksum;                //holds checksum
  unsigned int tmp, tmp1;            //tmp values
  char buf[32768];                   //send/recv buffer
  int listenfd, connfd, n, portNum;  //connection vars
  struct sockaddr_in servaddr;       //socket struct

  time_t tm;
  srand((unsigned) time(&tm));

  FILE *fp;

  //
  //unsigned short int hdrLen = 0x6000;       //(set bit positions 14 and 13 to 1 to represent 6 for header length/data offset field)
  unsigned short int ackFlag = 0x6010;      //set ACK flag bit position on (2^4) 16  //0x6010 (header len included)
  unsigned short int synFlag = 0x6002;      //set SYN flag bit position on (2^1) 2   //0x6002
  unsigned short int finFlag = 0x6001;      //set FIN flag bit position on (2^0) 1   //0x6001

  //check for correct usage
  if (argc != 2)
	{
		fprintf(stderr, "Usage: ./tserver <portnumber>\n");
		exit(EXIT_FAILURE);
	}

  //zero out segment
  bzero(&tSeg,sizeof(tSeg));

  //Store user-entered port number
  portNum = atoi(argv[1]);
 
  //AF_INET - IPv4 IP , Type of socket, protocol
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(portNum);

  //Allow socket reuse
	int on = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
 
  //Bind struct to socket
	if ((bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) == -1)
  {
    perror("bind\n");
  }

	//Listen for connections, zero buf
	listen(listenfd, 10);
  bzero(buf,sizeof(buf));

  //Accepts incoming connection
  connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
  bzero(buf, sizeof(buf));

  //server while loop
  while (1) 
  {
    //if server sends something...
    if (n = read(connfd, buf, sizeof(buf)) > 0)
    {
      //copy it into tSeg
      memcpy(&tSeg, buf, sizeof(buf));
    }

    //copy segment into array for checksum calculation/verification
    memcpy(cksum_arr, &tSeg, 1048);
    cksum = check(cksum_arr, 524);
    bzero(cksum_arr, sizeof(cksum_arr));

    //if no errors, checksum will be 0
    if (cksum == 0)
    {
      //check flags
      //if SYN bit set...
      if (tSeg.hdr_flags == synFlag)
      {
        //store sequence # and src port before zeroing tSeg, then set/calculate all other tSeg members
        tmp = tSeg.seq + 1;
        tmp1 = tSeg.src;
        bzero(&tSeg,sizeof(tSeg));
        tSeg.src = portNum;
        tSeg.des = tmp1;
        tSeg.seq = (rand() % 60000 + 10);
        tSeg.ack = tmp;

        //set SYN and ACK bits (and subtract extra header bits from synFlag)
        tSeg.hdr_flags = (synFlag - 0x6000) + ackFlag;

        //calculate new checksum
        memcpy(cksum_arr, &tSeg, 1048);
        tSeg.cksum = check(cksum_arr, 524);

        //write SYNACK to client and print to console/server.out
        if ((write(connfd, &tSeg, sizeof(tSeg))) == -1)
        {
            perror("write\n");
        }
        printSeg(&tSeg);
      }
      //else if ACK bit set...
      else if (tSeg.hdr_flags == ackFlag)
      {
        //print connection established msg to console/server.out
        printf("Connection established...\n\n");
        fp = fopen("server.out", "a");
        fprintf(fp, "Connection established...\n\n");
        fclose(fp);
        bzero(&tSeg, sizeof(tSeg));
      }
      //else if FIN bit set...
      else if (tSeg.hdr_flags == finFlag)
      {
        //store sequence # and src port before zeroing tSeg, then set/calculate all other tSeg members
        //set seq to 512, and ACK# to client sequence # + 1
        tmp = tSeg.seq + 1;
        tmp1 = tSeg.src;
        bzero(&tSeg, sizeof(tSeg));
        tSeg.des = tmp1;
        tSeg.src = portNum;
        tSeg.ack = tmp;
        tSeg.hdr_flags = ackFlag;
        tSeg.seq = 512;

        //create new checksum
        memcpy(cksum_arr, &tSeg, 1048);
        tSeg.cksum = check(cksum_arr, 524);
        bzero(cksum_arr, sizeof(cksum_arr));
        
        
        //send ACK to client, print to console/server.out
        if ((write(connfd, &tSeg, sizeof(tSeg))) == -1)
        {
            perror("write\n");
        }
        printSeg(&tSeg);

        //set FIN bit, recompute checksum
        tSeg.cksum = 0;
        tSeg.hdr_flags = 0;
        tSeg.hdr_flags = finFlag;
        memcpy(cksum_arr, &tSeg, 1048);
        tSeg.cksum = check(cksum_arr, 524);

        //send FIN to client, print to console/server.out
        if ((write(connfd, &tSeg, sizeof(tSeg))) == -1)
        {
            perror("write\n");
        }
        printSeg(&tSeg);

        //if server sends response, copy into tSeg
        if (n = read(connfd, buf, sizeof(buf)) > 0)
        {
          memcpy(&tSeg, buf, sizeof(buf));
        }

        //copy segment into array for checksum calculation/verification
        memcpy(cksum_arr, &tSeg, 1048);
        cksum = check(cksum_arr, 524);
        bzero(cksum_arr, sizeof(cksum_arr));
        
        //if no errors in checksum...
        if (cksum == 0)
        {
          //if ack bit is set...
          if (tSeg.hdr_flags == ackFlag)
          {
            //print connection closed msg to console/server.out, then exit(0)
            printf("The connection is now closed. Exiting...\n");
            fp = fopen("server.out", "a");
            fprintf(fp, "The connection is now closed.\n");
            fclose(fp);
            exit(0);
          }
        }
      }
      //unknown header bits detected, should not be reached without errors in sent segment
      else
      {
        printf("Error, incorrect header bits...\n");
      }
    }
    //otherwise checksum has errors, exit (should not be reached)
    else 
    {
      printf("Errors detected in checksum, exiting...\n");
      printf("cksum 0x%04X != 0\n", cksum);
      exit(-1);
    }
  }  //end server while
  close(connfd);
  return 0;
}

//given an array of unsigned short ints and the number n of elements in the array,
//check() calculates and returns the unsigned short int checksum of those elements
unsigned short int check(unsigned short int arr[], int n)
{
    unsigned int i,sum=0, cksum, wrap;

    for (i=0; i<n; i++)
    {
        sum = sum + arr[i];
    }

    wrap = sum >> 16;             // Wrap around 
    sum = sum & 0x0000FFFF; 
    sum = wrap + sum;

    wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
    sum = sum & 0x0000FFFF;
    cksum = wrap + sum;

    //return one's complement via XOR to get final checksum
    return (0xFFFF^cksum);
}

//prints the populated fields from a tcp_hdr struct to the console and to file "server.out"
void printSeg(struct tcp_hdr *seg)
{
    FILE *fp;
    char ch;

    //print segment values to console    
    printf("Source Port: %hu\n", seg->src);
    printf("Destination Port: %hu\n", seg->des);
    printf("Sequence Number: %u\n", seg->seq);
    printf("Acknowledgement Number: %u\n", seg->ack);
    printf("Header length/flags: 0x%04X\n", seg->hdr_flags);
    printf("Checksum: 0x%04X\n\n", seg->cksum);
    fp = fopen("server.out", "a");
    fprintf(fp, "Source Port: %hu\nDestination Port: %hu\nSequence Number: %u\nAcknowledgement Number: %u\nHeader length/flags: 0x%04X\nChecksum: 0x%04X\n\n",
                 seg->src, seg->des, seg->seq, seg->ack, seg->hdr_flags, seg->cksum);
    fclose(fp);
}