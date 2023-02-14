#include<stdio.h>	//For standard things
#include<stdlib.h>	//malloc
#include<string.h>	//memset
#include<netinet/ip_icmp.h>	//Provides declarations for icmp header
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/tcp.h>	//Provides declarations for tcp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include<netinet/ip6.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <fcntl.h>
#include<sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/types.h>
#include <net/ethernet.h>

void ProcessPacket(unsigned char* , int);
void print_ip6_header(unsigned char * , int);
void print_tcp_packet(unsigned char* , int);
void PrintData (unsigned char* , int);

int sock_raw;
FILE *logfile;
int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,i,j;
struct sockaddr_in6 source,dest;

int main()
{
	int data_size;
	struct sockaddr_in6 saddr;
	socklen_t saddr_size;
	struct in_addr in;


	//unsigned char *buffer = (unsigned char *)malloc(65536); //Its Big!
	
	logfile=fopen("log.txt","w");
	if(logfile==NULL) 
	{
		printf("Unable to create file.");
	}

	printf("Starting...\n");
	//Create a raw socket that shall sniff
	sock_raw = socket(AF_PACKET , SOCK_RAW, htons(ETH_P_ALL));
	if(sock_raw < 0)
	{
		printf("Socket Error\n");
		return 1;
	}
	// int flags = fcntl(sock_raw, F_GETFL, 0);

	// if (flags < 0) {
	// 	// handle error
	// 	printf("Flags error\n");
	// 	return 1;
	// }

	// flags |= O_NONBLOCK;

	// if (fcntl(sock_raw, F_SETFL, flags) < 0) {
	// 	// handle error
	// 	printf("non blocking error\n");
	// 	return 1;
	// }


	// memset(&saddr, 0, sizeof(saddr));
	// saddr.sin6_family = AF_INET6;
	// saddr.sin6_port = htons(0);
	// saddr.sin6_addr = in6addr_any;
	// if (bind(sock_raw, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
    // 	perror("bind() failed");
    // 	exit(EXIT_FAILURE);
	// }


	// struct msghdr buffer;  //uncomment line 31
	// char   msgbuffer[80];
	// struct iovec   iov[1];
	// int pass_sd;
	// memset(&buffer,   0, sizeof(buffer));
   	// memset(iov,    0, sizeof(iov));

	// iov[0].iov_base = msgbuffer;
   	// iov[0].iov_len  = sizeof(msgbuffer);
   	// buffer.msg_iov     = iov;
   	// buffer.msg_iovlen  = 1;

	
	char buffer[65536];
	while(1)
	{
		saddr_size = sizeof(saddr);
		//Receive a packet
		data_size = recvfrom(sock_raw , buffer , 65536, 0, (struct sockaddr *) &saddr, &saddr_size);
		//printf("%d\n", data_size);
		if(data_size <0 )
		{
			printf("Recvfrom error , failed to get packets\n");
			return 1;
		}
		//Now process the packet
		ProcessPacket(buffer , data_size);

		//NEED A WAY TO TERMINATE THE LOOP
	}
	close(sock_raw);
	printf("Finished");
	return 0;
}



void ProcessPacket(unsigned char* buffer, int size)
{
	//Get the IP Header part of this packet
	struct ip6_hdr * iph = (struct ip6_hdr *) buffer;
	++total;
	switch (iph->ip6_nxt) //Check the Protocol and do accordingly...
	{
		
		case IPPROTO_TCP:  //TCP Protocol
			++tcp;
			print_ip6_header(buffer, size);
			break;
		
		default: //Some Other Protocol like ARP etc.
			++others;
			print_ip6_header(buffer, size);
			break;
	}
	//printf("TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\r",tcp,udp,icmp,igmp,others,total);
}

void print_ip6_header(unsigned char * buffer, int size)
{
	//unsigned short iphdrlen;
		
	struct ip6_hdr *iph = (struct ip6_hdr *) buffer;
	//iphdrlen = sizeof(struct ip6_hdr);
	
	
	//memset(&source, 0, sizeof(source));
	//source.sin6_addr = iph->ip6_src;
	
	//memset(&dest, 0, sizeof(dest));
	//dest.sin6_addr = iph->ip6_dst;
	
	char src_addr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(iph->ip6_src), src_addr, INET6_ADDRSTRLEN);


	fprintf(logfile,"\n");
	fprintf(logfile,"IP Header\n");
	//fprintf(logfile,"   |-IP Version        : %d\n",(unsigned int)iph->ip6_ctlun.ip6_un1.ip6_un1_flow);
	//fprintf(logfile,"   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
	//fprintf(logfile,"   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
	//fprintf(logfile,"   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
	//fprintf(logfile,"   |-Identification    : %d\n",ntohs(iph->id));
	//fprintf(logfile,"   |-Reserved ZERO Field   : %d\n",(unsigned int)iphdr->ip_reserved_zero);
	//fprintf(logfile,"   |-Dont Fragment Field   : %d\n",(unsigned int)iphdr->ip_dont_fragment);
	//fprintf(logfile,"   |-More Fragment Field   : %d\n",(unsigned int)iphdr->ip_more_fragment);
	//printf("   |- Hop Limit      : %d\n",(unsigned int)iph->ip6_ctlun.ip6_un1.ip6_un1_hlim);
	fprintf(logfile,"   |- Hop Limit      : %d\n",(unsigned int)iph->ip6_ctlun.ip6_un1.ip6_un1_hlim);
	//fprintf(logfile,"   |-Protocol : %d\n",(unsigned int)iph->protocol);
	//fprintf(logfile,"   |-Checksum : %d\n",ntohs(iph->check));
	//printf("   |-Source IP        : %s\n", src_addr);
	fprintf(logfile,"   |-Source IP        : %s\n", src_addr);
	//fprintf(logfile,"   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
}


void print_tcp_packet(unsigned char* Buffer, int Size)
{
	unsigned short iphdrlen;
	
	struct iphdr *iph = (struct iphdr *)Buffer;
	iphdrlen = iph->ihl*4;
	
	struct tcphdr *tcph=(struct tcphdr*)(Buffer + iphdrlen);
			
	fprintf(logfile,"\n\n***********************TCP Packet*************************\n");	
		
	print_ip6_header(Buffer,Size);
		
	fprintf(logfile,"\n");
	fprintf(logfile,"TCP Header\n");
	fprintf(logfile,"   |-Source Port      : %u\n",ntohs(tcph->source));
	fprintf(logfile,"   |-Destination Port : %u\n",ntohs(tcph->dest));
	fprintf(logfile,"   |-Sequence Number    : %u\n",ntohl(tcph->seq));
	fprintf(logfile,"   |-Acknowledge Number : %u\n",ntohl(tcph->ack_seq));
	fprintf(logfile,"   |-Header Length      : %d DWORDS or %d BYTES\n" ,(unsigned int)tcph->doff,(unsigned int)tcph->doff*4);
	//fprintf(logfile,"   |-CWR Flag : %d\n",(unsigned int)tcph->cwr);
	//fprintf(logfile,"   |-ECN Flag : %d\n",(unsigned int)tcph->ece);
	fprintf(logfile,"   |-Urgent Flag          : %d\n",(unsigned int)tcph->urg);
	fprintf(logfile,"   |-Acknowledgement Flag : %d\n",(unsigned int)tcph->ack);
	fprintf(logfile,"   |-Push Flag            : %d\n",(unsigned int)tcph->psh);
	fprintf(logfile,"   |-Reset Flag           : %d\n",(unsigned int)tcph->rst);
	fprintf(logfile,"   |-Synchronise Flag     : %d\n",(unsigned int)tcph->syn);
	fprintf(logfile,"   |-Finish Flag          : %d\n",(unsigned int)tcph->fin);
	fprintf(logfile,"   |-Window         : %d\n",ntohs(tcph->window));
	fprintf(logfile,"   |-Checksum       : %d\n",ntohs(tcph->check));
	fprintf(logfile,"   |-Urgent Pointer : %d\n",tcph->urg_ptr);
	fprintf(logfile,"\n");
	fprintf(logfile,"                        DATA Dump                         ");
	fprintf(logfile,"\n");
		
	fprintf(logfile,"IP Header\n");
	PrintData(Buffer,iphdrlen);
		
	fprintf(logfile,"TCP Header\n");
	PrintData(Buffer+iphdrlen,tcph->doff*4);
		
	fprintf(logfile,"Data Payload\n");	
	PrintData(Buffer + iphdrlen + tcph->doff*4 , (Size - tcph->doff*4-iph->ihl*4) );
						
	fprintf(logfile,"\n###########################################################");
}


void PrintData (unsigned char* data , int Size)
{
	
	for(i=0 ; i < Size ; i++)
	{
		if( i!=0 && i%16==0)   //if one line of hex printing is complete...
		{
			fprintf(logfile,"         ");
			for(j=i-16 ; j<i ; j++)
			{
				if(data[j]>=32 && data[j]<=128)
					fprintf(logfile,"%c",(unsigned char)data[j]); //if its a number or alphabet
				
				else fprintf(logfile,"."); //otherwise print a dot
			}
			fprintf(logfile,"\n");
		} 
		
		if(i%16==0) fprintf(logfile,"   ");
			fprintf(logfile," %02X",(unsigned int)data[i]);
				
		if( i==Size-1)  //print the last spaces
		{
			for(j=0;j<15-i%16;j++) fprintf(logfile,"   "); //extra spaces
			
			fprintf(logfile,"         ");
			
			for(j=i-i%16 ; j<=i ; j++)
			{
				if(data[j]>=32 && data[j]<=128) fprintf(logfile,"%c",(unsigned char)data[j]);
				else fprintf(logfile,".");
			}
			fprintf(logfile,"\n");
		}
	}
}