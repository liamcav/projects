//Liam Caveney V00721857
//361 p2
//sender that implements reliable data transfer over UDP
//use of socket API based on lab2 udp_client.c and udp_server.c 
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

int sock;
struct sockaddr_in sa;
int bytes_sent;
ssize_t recsize;
socklen_t fromlen;

char* sIP;
int sPort;
char* rIP;
int rPort;

int iSEQ;
int lastAck;
int windowSize;
char* filePath;

int total_bytes_sent=0;
int unique_bytes_sent=0;
int data_packets_sent=0;
int unique_data_packets_sent=0;
int SYN_sent=0;
int FIN_sent=0;
int RST_sent=0;
int ACK_rec=0;
int RST_rec=0;

struct header{
	char magic[6];
	char type;
	int seqno;
	int ackno;
	int length;
	int size; 
	char l;
};

void printTime(){
	time_t rawtime;
	struct tm* info;
	time(&rawtime);
	info = localtime(&rawtime);
	char timeString[9];
	strftime(timeString, sizeof(timeString), "%T", info);
	struct timeval now;
	int rc = gettimeofday(&now, NULL);
	printf("%s:%06u", timeString, (unsigned int)now.tv_usec);
}


int sendPacket(struct header* h, char* d){  
	char buffer[28 + h->length];
	memcpy(buffer, h, sizeof(struct header));
	memcpy(&buffer[28], d, h->length*sizeof(char));
	bytes_sent = sendto(sock, buffer, sizeof(struct header) + sizeof(char)*(h->length), 0,(struct sockaddr*)&sa, sizeof sa);
	if (bytes_sent < 0) {
		printf("Error sending packet: %s\n", strerror(errno));
		return 0;
	}
	return 1;
}

int receivePacket(struct header* buffer){
	recsize = recvfrom(sock, (void*)buffer, sizeof(struct header), 0, (struct sockaddr*)&sa, &fromlen);
	if (recsize < 0) {
		struct header sh;
		strcpy(sh.magic, "CSC361");
		sh.type = 'R';
		sh.seqno = 0;
		sh.ackno = 0;
		sh.length = 0;
		sh.size = 0;
		sh.l = '\n';
		char* sd;
		sendPacket(&sh, sd);
		fprintf(stderr, "%s\n", strerror(errno));
		return 0;
	}
	return 1;
}



int rdpConnect(){			
	//send SYN 
	srand(time(NULL)); 
	iSEQ = rand()%1000;
	struct header sh;
	strcpy(sh.magic, "CSC361");
	sh.type = 'S';
	sh.seqno = iSEQ;
	sh.ackno = 0;
	sh.length = 0;
	sh.size = 0;
	sh.l = '\n';
	char* sd;
	fd_set read_fds;

	int i=0;
	while(1==1){
		sendPacket(&sh, sd);
		SYN_sent++;
		printTime();
		char s = 's';
		if(i>0) s='S';
		printf(" %c %s:%i %s:%i SYN %i 0\n", s, sIP, sPort, rIP, rPort, iSEQ);
		i++;
		//wait for ACK
		FD_ZERO(&read_fds);
		FD_SET(sock, &read_fds); 
		struct timeval tv;
		tv.tv_sec = 4;		//wait up to 1 sec for ACK
	    tv.tv_usec = 0;
		select(FD_SETSIZE, &read_fds, 0, 0, &tv);				
		if(FD_ISSET(sock, &read_fds)){
			struct header rh;
			receivePacket(&rh);
			//read ACK and window size
			if(rh.type = 'A'){
				ACK_rec++;
				printTime();
				printf(" r %s:%i %s:%i ACK %i %i\n", rIP, rPort, sIP, sPort, rh.ackno, rh.size);
				lastAck = rh.ackno;
				//int nextByte = ack - iSEQ;
				windowSize = rh.size;	
				return 1;			
			}
			if(rh.type = 'R'){
				RST_rec++;		
				return -1;
			}
		}
	}
}


int rdpSend(){
	char* fileBuffer = NULL;
	size_t fileSize = 0;
	FILE* fp = fopen(filePath, "r");
	fseek(fp, 0, SEEK_END);
	fileSize=ftell(fp);				//finds size of file
	rewind(fp);	
	fileBuffer = malloc(fileSize); 
	fread(fileBuffer, fileSize, 1, fp); //write file to fileBuffer
	
	int SEQ = iSEQ;
	int sent=0;
	int bytesRemaining = fileSize;
	int bytesToWrite;
	int nextByte;
	fd_set read_fds;
	while(1==1){
		int i;
		for(i=0; i<2; i++){		
			if(bytesRemaining>0){
				bytesToWrite=1000;
				nextByte = SEQ-iSEQ;
				if (nextByte+1000 > fileSize) bytesToWrite = fileSize-nextByte;

				if(windowSize>=bytesToWrite){	
					char d[bytesToWrite];
					memcpy(d, &fileBuffer[nextByte], bytesToWrite);
					struct header sh;
					strcpy(sh.magic, "CSC361");
					sh.type = 'D';
					sh.seqno = SEQ;
					sh.ackno = 0;
					sh.length = bytesToWrite;
					sh.size = 0;
					sh.l = '\n';
					sendPacket(&sh, d);
					data_packets_sent++;
					printTime();
					char s = 's';
					if(sh.seqno<=sent){
						s='S';
					} else{
						unique_data_packets_sent++;
						unique_bytes_sent+=sh.length;
					}	
					total_bytes_sent+=sh.length;
					printf(" %c %s:%i %s:%i DAT %i %i\n", s, sIP, sPort, rIP, rPort, sh.seqno, sh.length);
					bytesRemaining -= bytesToWrite;
					if(sh.seqno>sent) sent=sh.seqno;
					SEQ+=bytesToWrite;
				}
			}
		}
		FD_ZERO(&read_fds);
		FD_SET(sock, &read_fds); 
		struct timeval tv;
		tv.tv_sec = 1;		//wait up to 1 sec for ACK
       	tv.tv_usec = 0;
		select(FD_SETSIZE, &read_fds, 0, 0, &tv);				
		if(FD_ISSET(sock, &read_fds)){			
			struct header rh;
			receivePacket(&rh);
			ACK_rec++;
			printTime();
			char r = 'r';
			if(lastAck>rh.ackno) r='R';
			printf(" r %s:%i %s:%i ACK %i %i\n", rIP, rPort, sIP, sPort, rh.ackno, rh.size);
			if(lastAck<rh.ackno) lastAck = rh.ackno;
			//read ACK and window size
			if(rh.type = 'A'){
				int ack = rh.ackno;
				nextByte = ack - iSEQ;
				windowSize = rh.size;					
			}
			if(rh.type == 'R'){
				RST_rec++;		
				return -1;
			}
		}
		if(bytesRemaining<=0) return 1;
	}
}

int rdpClose(){
	//send FIN
	struct header sh;
	strcpy(sh.magic, "CSC361");
	sh.type = 'F';
	sh.seqno = 0;
	sh.ackno = 0;
	sh.length = 0;
	sh.size = 0;
	sh.l = '\n';
	char* sd;
	fd_set read_fds;

	int i;
	for(i=0; i<3; i++){			//sends three FIN, then assumes other end will close
		sendPacket(&sh, sd);
		FIN_sent++;
		printTime();
		char s = 's';
		if(i>0) s='S';
		printf(" %c %s:%i %s:%i FIN %i 0\n", s, sIP, sPort, rIP, rPort, iSEQ);
		i++;

		//wait for ACK
		FD_ZERO(&read_fds);
		FD_SET(sock, &read_fds); 
		struct timeval tv;
		tv.tv_sec = 1;		//wait up to 1 sec for ACK
	    tv.tv_usec = 0;
		select(FD_SETSIZE, &read_fds, 0, 0, &tv);				
		if(FD_ISSET(sock, &read_fds)){
			struct header rh;
			receivePacket(&rh);
			if(rh.type = 'A'){	
				ACK_rec++;
				printTime();
				printf(" r %s:%i %s:%i ACK %i %i\n", rIP, rPort, sIP, sPort, rh.ackno, rh.size);	
				return 1;			
			}
			if(rh.type == 'R'){
				RST_rec++;		
				return -1;
			}
		}
	}
}

int main(int argc, char **argv){
	time_t start_t, end_t;	
	time(&start_t);	
	if (argc < 6){
		printf("Please include c100000orrect arguments.\n");
		return 0;	
	}

	sIP = argv[1];
	sPort = atoi(argv[2]);
	rIP = argv[3];
	rPort = atoi(argv[4]);
	filePath = argv[5];

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (-1 == sock) {
      /* if socket failed to initialize, exit */
		printf("Error Creating Socket");
		exit(EXIT_FAILURE);
	}
 

	memset(&sa, 0, sizeof sa); 
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("10.10.1.100");
	sa.sin_port = htons(8080);
 	
	rst:
		if(rdpConnect()!=1) goto rst;
		if(rdpSend()!=1)  goto rst;
		if(rdpClose()!=1) goto rst;

	close(sock); /* close the socket */

	printf("\ntotal data bytes sent: %i\n", total_bytes_sent);
	printf("unique data bytes sent: %i\n", unique_bytes_sent);
	printf("total data packets sent: %i\n", data_packets_sent);
	printf("unique data packets sent: %i\n", unique_data_packets_sent);
	printf("SYN packets sent: %i\n", SYN_sent);
	printf("FIN packest sent: %i\n", FIN_sent);
	printf("RST packets sent: %i\n", RST_sent);
	printf("ACK packets received: %i\n", ACK_rec);
	printf("RST packets received: %i\n", RST_rec);
	
	time(&end_t);
	printf("total time duration (seconds): %.0f\n", difftime(end_t, start_t));
	
	return 0;
}

