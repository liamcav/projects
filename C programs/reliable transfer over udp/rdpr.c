//Liam Caveney
//361 p2
//receiver that implements reliable data transfer over UDP
//use of socket API based on lab2 udp_client.c and udp_server.c 
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h> /* for close() for socket */ 
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

struct sockaddr_in sa; 
char buffer1[1024];
ssize_t recsize;
socklen_t fromlen;
int bytes_sent;
int sock;

int sPort;
int rPort;
char* rIP;
char* sIP;
char* filePath;

int iSEQ;
int windowSize;
int FINrec;
char fileBuffer[100000];
int curr;
FILE* fp;
int allRec; //set when receives FIN

int total_bytes_rec=0;
int unique_bytes_rec=0;
int data_packets_rec=0;
int unique_data_packets_rec=0;
int SYN_rec=0;
int FIN_rec=0;
int RST_rec=0;
int ACK_sent=0;
int RST_sent=0;									///send reset when hardware failure		

int sentack=0;


struct header{
	char magic[6];
	char type;
	int seqno;//char seqno[2];
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

int sendPacket(struct header* buffer){
	bytes_sent = sendto(sock, buffer, 4*sizeof(buffer), 0,(struct sockaddr*)&sa, sizeof sa);
	  if (bytes_sent < 0) {
		printf("Error sending packet: %s\n", strerror(errno));
		return 0;
	  }
	return 1;
}

int receivePacket(struct header* h, char* data){
	char buffer[1028];
	recsize = recvfrom(sock, (void*)buffer, 1028, 0, (struct sockaddr*)&sa, &fromlen);
	if (recsize < 0) {
		return 0;
	}
	memcpy(h, buffer, sizeof(struct header));
	memcpy(data, &buffer[28], h->length);
	return 1;
}

void fileWriter(int n){
	fwrite(fileBuffer, 1, n, fp);
	windowSize+=n;	
	curr=0;
	if(allRec) fclose(fp); 
}

int rdpConnect(){ 
	//wait for SYN
	struct header rh;
	char d[1000];
	receivePacket(&rh, d);
	sIP = "192.168.1.100";
	sPort = 8080;
	printTime();
	printf(" r %s:%i %s:%i SYN %i 0\n", sIP, sPort, rIP, rPort, rh.seqno);
	windowSize=100000;
	//read SYN and window size
	if(rh.type=='S'){
		SYN_rec++;
		iSEQ = rh.seqno;
		//reply ACK
		struct header sh;
		strcpy(sh.magic, "CSC361");
		sh.type = 'A';
		sh.seqno = 0;
		sh.ackno = iSEQ;
		sh.length = 0;
		sh.size = windowSize;
		sh.l = '\n';
		sendPacket(&sh);
		printTime();
		printf(" s %s:%i %s:%i ACK %i %i\n", rIP, rPort, sIP, sPort, sh.ackno, sh.size);
		sentack=sh.ackno;
		ACK_sent++;
		return 1;		
	}
	if(rh.type == 'R'){
		RST_rec++;		
		return -1;
	}
	return 0;
}

int rdpReceive(){			
	int curr=0;
	int expected=iSEQ;
	fd_set read_fds;
	while(1==1){
		int i;	
		for(i=0; i<2; i++){
			FD_ZERO(&read_fds);
			FD_SET(sock, &read_fds); 
			struct timeval tv;
			tv.tv_sec = 1;		//wait up to 1 sec for ACK
		   	tv.tv_usec = 0;
			select(FD_SETSIZE, &read_fds, 0, 0, &tv);				
			if(FD_ISSET(sock, &read_fds)){
				struct header rh;
				char d[1000];
				receivePacket(&rh, d);
				if(rh.type=='F'){
					printTime();					
					printf(" r %s:%i %s:%i FIN %i %i\n", sIP, sPort, rIP, rPort, rh.seqno, rh.length);
					FIN_rec=1;
					rdpClose();
					return 1;		
				}
				if(rh.type == 'D'){
					if(rh.seqno == expected){
						printTime();
						total_bytes_rec+=rh.length;
						data_packets_rec++;
						char r = 'r';
						if(rh.seqno<expected){
							r='R';						
						} else{
							unique_bytes_rec+=rh.length;
							unique_data_packets_rec++;						
						}
						printf(" %c %s:%i %s:%i DAT %i %i\n", r, sIP, sPort, rIP, rPort, rh.seqno, rh.length);						
						if(rh.seqno==expected){						
							memcpy(&fileBuffer[curr], d, rh.length);		
							curr+=rh.length;
							expected+=rh.length; 
							windowSize-=rh.length;
						}
					}					
				}
				if(rh.type == 'R'){
					RST_rec++;		
					return -1;
				}
			}
			else {break;}
		}

		fileWriter(100000-windowSize);			
		curr=0;
		struct header sh;
		strcpy(sh.magic, "CSC361");
		sh.type = 'A';
		sh.seqno = 0;
		sh.ackno = expected;
		sh.length = 0;
		sh.size = windowSize;
		sh.l = '\n';
		sendPacket(&sh);
		printTime();
		char s='s';
		if(sh.ackno==sentack) s='S';
		printf(" %c %s:%i %s:%i ACK %i %i\n", s, rIP, rPort, sIP, sPort, sh.ackno, sh.size);  
		sentack=sh.ackno;
		ACK_sent++;
	}

}

int rdpClose(){			
	allRec=1;
	struct header sh;
	strcpy(sh.magic, "CSC361");
	sh.type = 'A';
	sh.seqno = 0;
	sh.ackno = 0;
	sh.length = 0;
	sh.size = 0;
	sh.l = '\n';
	sendPacket(&sh);
	printTime();
	printf(" s %s:%i %s:%i ACK %i %i\n", rIP, rPort, sIP, sPort, sh.ackno, sh.size); 
	fileWriter(100000-windowSize);
	return 1;
}



int main(int argc, char **argv){
	time_t start_t, end_t;	
	time(&start_t);
	if (argc < 4){
		printf("Please include correct arguments.\n");
		return 0;	
	}

	rIP = argv[1];
	rPort = atoi(argv[2]);
	filePath = argv[3];

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(8080);
	fromlen = sizeof(sa);

	if (-1 == bind(sock, (struct sockaddr *)&sa, sizeof sa)) {
		perror("error bind failed");
		close(sock);
		exit(EXIT_FAILURE);
	}

	fp=fopen(argv[3], "w");	

	rdpConnect();
	rdpReceive();

	printf("\ntotal data bytes received: %i\n", total_bytes_rec);
	printf("unique data bytes received: %i\n", unique_bytes_rec);
	printf("total data packets received: %i\n", data_packets_rec);
	printf("unique data packets received: %i\n", unique_data_packets_rec);
	printf("SYN packets received: %i\n", SYN_rec);
	printf("FIN packest received: %i\n", FIN_rec);
	printf("RST packets received: %i\n", RST_rec);
	printf("ACK packets sent: %i\n", ACK_sent);
	printf("RST packets sent: %i\n", RST_sent);
	
	time(&end_t);
	printf("total time duration (seconds): %.0f\n", difftime(end_t, start_t));

}

