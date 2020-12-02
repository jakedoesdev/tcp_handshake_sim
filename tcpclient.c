/* 
Author: Jacob Everett
Description: TCP client that simulates a 3-way handshake and connection closing.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

//tcp segment struct
struct tcp_hdr
{ 
    unsigned short int src;         //src port
    unsigned short int des;         //dest port
    unsigned int seq;               //sequence # (random, then x+1)
    unsigned int ack;               //ack #
    unsigned short int hdr_flags;   //4 bit data offset, 6 bit reserved section, 6 bit flags
    unsigned short int rec;         //16 bit receive window (0)
    unsigned short int cksum;       //checksum (calculated)
    unsigned short int ptr;         //urgent data pointer (0)
    unsigned int opt;               //options (0)
    char data [1024];               //payload data (0)
};

//checksum calculation declaration
unsigned short int check(unsigned short int arr[], int n);

//print segment declaration
void printSeg(struct tcp_hdr *seg);

int main (int argc, char **argv)
{
    struct tcp_hdr tSeg;                       //tcp struct instance
    unsigned short int cksum_arr[524];         //array for calculating checksum
    unsigned int cksum;                        //checksum vars
    unsigned int tmp, tmp1;                    //tmp vars

    char buf[32768];                           //send/recv buffer
    int sockfd, n, portNum, len = 0;           //socket vars
    struct sockaddr_in servaddr;               //socket struct

    //seed random
    time_t tm;
    srand((unsigned) time(&tm));

    //flag values (each flag bit position has 0x6000 added for 4-bit header length/data offset--subtracted from one where two flags are added together)
    unsigned short int ackFlag = 0x6010;      //set ACK flag bit position on (2^4) 16
    unsigned short int synFlag = 0x6002;      //set SYN flag bit position on (2^1) 2
    unsigned short int finFlag = 0x6001;      //set FIN flag bit position on (2^0) 1

    //check for correct usage
    if (argc != 2)
	{
		fprintf(stderr, "Usage: ./tclient <portnumber>\n");
		exit(EXIT_FAILURE);
	}

    //zero out segment
    bzero(&tSeg,sizeof(tSeg));

    //AF_INET - IPv4 IP , Type of socket, protocol
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr,sizeof(servaddr));
 
    //store port num from user-entered arg, set tSeg member
    portNum = atoi(argv[1]);
    tSeg.des = portNum;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(portNum);
 
    //Convert IPv4 and IPv6 addresses from text to binary form, connect to server on cse03
    inet_pton(AF_INET,"129.120.151.96",&(servaddr.sin_addr));
 
    //connect to server
    if ((connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))) == -1)
    {
        perror("connect\n");
    }

    //get source port via system call, set tSeg src member
    len = sizeof(servaddr);
    if ((getsockname(sockfd, (struct sockaddr *)&servaddr, &len)) == -1)
    {
        perror("getsockname\n");
    }
    len=0;
    tSeg.src = (int) ntohs(servaddr.sin_port);

    //create and store random sequence number between 10 and 60000 (values I picked arbitrarily), zero buffer
    tSeg.seq = (rand() % 60000 + 10);
    bzero(buf, sizeof(buf));

    //set 4-bit header length to 6 and SYN bit to 1 (0x6001)
    tSeg.hdr_flags = synFlag;
    
    //copy segment into checksum array, call check() to calculate checksum, set tSeg cksum member
    memcpy(cksum_arr, &tSeg, 1048);
    cksum = check(cksum_arr, 524);
    bzero(cksum_arr, sizeof(cksum_arr));
    tSeg.cksum = cksum;
    cksum = 0;

    //send SYN segment to server, then print to console/client.out
    if ((write(sockfd, &tSeg, sizeof(tSeg))) == -1)
    {
        perror("write\n");
    }
    printSeg(&tSeg);
    bzero(&tSeg,sizeof(tSeg));

    //if the server sent something...
    if (n = read(sockfd, buf, sizeof(buf)) > 0)
    {
        //copy buf into tSeg, copy tSeg into cksum_arr, call check() and store returned checksum value in cksum
        memcpy(&tSeg, buf, sizeof(buf));
        memcpy(cksum_arr, &tSeg, 1048);
        cksum = check(cksum_arr, 524);
        bzero(cksum_arr, sizeof(cksum_arr));

        //if cksum has no errors...
        if (cksum == 0)
        {
            //check flags...
            //if syn and ack flags are set...
            if (tSeg.hdr_flags == ((synFlag - 0x6000) + ackFlag))
            {

                //store sequence # and dest port before zeroing tSeg, then set/calculate all other tSeg members, set ACK# to server seq + 1
                tmp = tSeg.seq + 1;
                tmp1 = tSeg.des;
                bzero(&tSeg,sizeof(tSeg));
                tSeg.src = tmp1;
                tSeg.des = portNum;
                tSeg.seq = (rand() % 60000 + 10);
                tSeg.ack = tmp;

                //set ACK bit, calculate new checksum
                tSeg.hdr_flags = ackFlag;
                memcpy(cksum_arr, &tSeg, 1048);
                tSeg.cksum = check(cksum_arr, 524);

                //send ACK to server, print to console/client.out
                if ((write(sockfd, &tSeg, sizeof(tSeg))) == -1)
                {
                    perror("write\n");
                }
                printSeg(&tSeg);
            }
        }
        //if errors, exit
        else 
        {
            printf("Errors detected in checksum, exiting...\n");
            printf("cksum 0x%04X != 0\n", cksum);
            exit(-1);
        }
    }

    //create close request FIN segment
    bzero(&tSeg,sizeof(tSeg));
    tSeg.hdr_flags = finFlag;
    tSeg.seq = 1024;
    tSeg.ack = 512;
    tSeg.des = portNum;
    tSeg.src = (int) ntohs(servaddr.sin_port);
    
    //copy segment into checksum array, call check() to calculate checksum, set tSeg member
    memcpy(cksum_arr, &tSeg, 1048);
    cksum = check(cksum_arr, 524);
    bzero(cksum_arr, sizeof(cksum_arr));
    tSeg.cksum = cksum;
    cksum = 0;

    //send close request FIN segment to server, print to console/client.out
    if ((write(sockfd, &tSeg, sizeof(tSeg))) == -1)
    {
        perror("write\n");
    }
    printSeg(&tSeg);
    bzero(&tSeg,sizeof(tSeg));

    //if the server sends a reply...
    if (n = read(sockfd, buf, sizeof(buf)) > 0)
    {
        //copy buf into tSeg
        memcpy(&tSeg, buf, sizeof(buf));
        memcpy(cksum_arr, &tSeg, 1048);
        cksum = check(cksum_arr, 524);
        bzero(cksum_arr, sizeof(cksum_arr));

        //if cksum has no errors...
        if (cksum == 0)
        {
            //if ACK bit is set...
            if (tSeg.hdr_flags == (ackFlag))
            {
                bzero(&tSeg,sizeof(tSeg));
                if (n = read(sockfd, buf, sizeof(buf)) > 0)
                {
                    memcpy(&tSeg, buf, sizeof(buf));
                    memcpy(cksum_arr, &tSeg, 1048);
                    cksum = check(cksum_arr, 524);
                    bzero(cksum_arr, sizeof(cksum_arr));

                    //check checksum again
                    if (cksum == 0)
                    {
                        if (tSeg.hdr_flags == finFlag)
                        {
                            //store sequence # and dest port before zeroing tSeg, then set/calculate all other tSeg members
                            tmp = tSeg.seq + 1;
                            tmp1 = tSeg.des;
                            bzero(&tSeg,sizeof(tSeg));
                            tSeg.src = tmp1;
                            tSeg.des = portNum;
                            //tSeg.seq = tmp;
                            tSeg.seq = 1025;  //CLIENT sequence # + 1 as requested in instruction #7di from assignment procedure instructions
                            tSeg.ack = tmp;
                            tSeg.hdr_flags = ackFlag;

                            //copy segment into checksum array, call check() to calculate checksum, set tSeg member
                            memcpy(cksum_arr, &tSeg, 1048);
                            cksum = check(cksum_arr, 524);
                            bzero(cksum_arr, sizeof(cksum_arr));
                            tSeg.cksum = cksum;

                            //send final ACK to server, print to console/client.out, sleep 2 seconds, and exit.
                            if ((write(sockfd, &tSeg, sizeof(tSeg))) == -1)
                            {
                                perror("write\n");
                            }
                            printSeg(&tSeg); 
                            sleep(2);        
                            exit(0);
                        }
                    }
                }
            }
        }
    }

    close(sockfd);
    return 0;
}

//given an array of unsigned short ints and the number n of elements in the array,
//check() calculates and returns the unsigned short int checksum of those elements
unsigned short int check(unsigned short int arr[], int n)
{
    unsigned int i,sum=0, cksum, wrap;

    // Compute sum
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

//prints the populated fields from a tcp_hdr struct to the console and to file "client.out"
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
    fp = fopen("client.out", "a");
    fprintf(fp, "Source Port: %hu\nDestination Port: %hu\nSequence Number: %u\nAcknowledgement Number: %u\nHeader length/flags: 0x%04X\nChecksum: 0x%04X\n\n",
                 seg->src, seg->des, seg->seq, seg->ack, seg->hdr_flags, seg->cksum);
    fclose(fp);
}
        