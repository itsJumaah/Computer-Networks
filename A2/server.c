//************************************************************************/
//159.334 - Networks - 2016
//************************************************************************/
//SERVER: prototype written by: 
//-------------------------------------------------------
//	Bilal Jumaah     12232659
//	Karim Abedrabbo  12226543
//	Ashneel Kumar    12239289
//************************************************************************/
//gcc server.c -o server.exe -lws2_32 -std=gnu11
//-------------------------------------------------------
//Otherwise will not function properly or compile errors
//************************************************************************/
//2 Functions are from the textbook 6th edition Page 221.
//************************************************************************/

// ---------------- INCLUDES FOR UNIX -----------------
#if defined __unix__ || defined __APPLE__
	#include <unistd.h>
	#include <errno.h>
	#include <stdlib.h>
	#include <stdio.h> 
	#include <string.h>
	#include <sys/types.h> 
	#include <sys/socket.h> 
	#include <arpa/inet.h>
	#include <netinet/in.h> 
	#include <netdb.h>
	#include "UDP_supporting_functions_2016.c"
// ---------------- INCLUDES FOR WIN32 -----------------
#elif defined _WIN32 
	#include <stdio.h> 
	#include <string.h>
	#include <stdlib.h>
	#include <winsock2.h>
	#include "UDP_supporting_functions_2016.c"
	
	#define WSVERS MAKEWORD(2,0)
	WSADATA wsadata;
#endif

// ---------------- MACROS -----------------
#define BUFFERSIZE 80   //remember, the BUFFERSIZE has to be at least big enough to receive the answer from the serv1
#define SEGMENTSIZE 78 //segment size, i.e., if fgets gets more than this number of bytes is segments the message in smaller parts.
#define NUMBER_OF_WORDS_IN_THE_HEADER 3 //2

#define GENERATOR 0x8005 //0x8005, generator for polynomial division

// ---------------- FUNCTION PROTOTYPES -----------------
void save_line_without_header(char *receive_buffer, FILE *fout);

unsigned int CRC_poly(char *_buffer);

unsigned int  SN_check(int _SN_expected, char *_recieve_buffer);
unsigned int  CRC_check(char *_recieve_buffer);
		 void ACK_build(char *_send_buffer, int _SN_checked);

//*******************************************************************
//MAIN
//*******************************************************************

int main(int argc, char *argv[]) {
	
	//********************************************************************
	// INITIALIZATION
	//********************************************************************
	
	#if defined __unix__ || defined __APPLE__
		int s;
	#elif defined _WIN32 
		SOCKET s;
	#endif
	
	struct sockaddr_in localaddr, remoteaddr;
	char   send_buffer[BUFFERSIZE], receive_buffer[BUFFERSIZE];
	int    n, bytes, addrlen;
	
	int SN_expected =  0; //Added
	int SN_checked  = -1; //Added
	
	addrlen = sizeof(remoteaddr);
	memset(&localaddr,0,sizeof(localaddr));//clean up the structure
	memset(&remoteaddr,0,sizeof(remoteaddr));//clean up the structure
	randominit();
	
	//********************************************************************
	// WSSTARTUP for win 32
	//********************************************************************
	
	#if defined _WIN32 
		if (WSAStartup(WSVERS, &wsadata) != 0) {
			WSACleanup();
			printf("WSAStartup failed\n");
		}
	#endif
	
	//********************************************************************
	//SOCKET
	//********************************************************************
	
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		printf("socket failed\n");
	}
	
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = INADDR_ANY;//server address should be local
	
	if (argc != 4) {
		printf("2016 code USAGE: server port  lossesbit(0 or 1) damagebit(0 or 1)\n");
		exit(1);
	}
	
	localaddr.sin_port = htons((u_short)atoi(argv[1]));
	int remotePort=1234;
	packets_damagedbit=atoi(argv[3]);
	packets_lostbit=atoi(argv[2]);
	
	if (packets_damagedbit<0 || packets_damagedbit>1 || packets_lostbit<0 || packets_lostbit>1){
		printf("2016 code USAGE: server port  lossesbit(0 or 1) damagebit(0 or 1)\n");
		exit(0);
	}
	
	//********************************************************************
	//REMOTE HOST IP AND PORT
	//********************************************************************
	
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	remoteaddr.sin_port = htons(remotePort);
	int counter=0;
	
	//********************************************************************
	//BIND
	//********************************************************************
	
	if (bind(s,(struct sockaddr *)(&localaddr),sizeof(localaddr)) != 0) {
		printf("Bind failed!\n");
		exit(0);
	}
	
	//********************************************************************
	// Open file to save the incoming packets
	//********************************************************************
	
	FILE *fout=fopen("file1_saved.txt","w");
	
	//********************************************************************
	//INFINITE LOOP
	//********************************************************************
	
	while (1) {
	
		//********************************************************************
		//RECEIVE
		//********************************************************************
		
		printf("Waiting... \n");
		
		#if defined __unix__ || defined __APPLE__
			bytes = recvfrom(s, receive_buffer, SEGMENTSIZE, 0,(struct sockaddr *)(&remoteaddr),(socklen_t*)&addrlen);
		#elif defined _WIN32
			bytes = recvfrom(s, receive_buffer, SEGMENTSIZE, 0,(struct sockaddr *)(&remoteaddr),&addrlen);
		#endif
		
		printf("Received %d bytes\n",bytes);
		
		//********************************************************************
		//PROCESS REQUEST
		//********************************************************************
		
		n = 0;
		
		while (n < bytes) {
			n++;
			if ((bytes < 0) || (bytes == 0)) {
				break;
			}
			/*end on a LF*/
			if (receive_buffer[n] == '\n') { 
				receive_buffer[n] = '\0';
				break;
			}
			/*ignore CRs*/
			if (receive_buffer[n] == '\r') {
				receive_buffer[n] = '\0';
			}
		}
		
		if ((bytes < 0) || (bytes == 0)) { 
			break;
		}
		
		printf("\n================================================\n");	
		printf("RECEIVED --> %s \n",receive_buffer);
		
		//commands start
		
		if (strncmp(receive_buffer,"PACKET",6)==0)  {
			sscanf(receive_buffer, "PACKET %d",&counter);
			//********************************************************************
			//SEND ACK
			//********************************************************************
			sprintf(send_buffer,"ACKNOW %d \r\n",counter);
			send_unreliably(s,send_buffer,remoteaddr);
			save_line_without_header(receive_buffer,fout);
		}
		
		if (strncmp(receive_buffer,"CLOSE",5)==0)  {
			//if client says "CLOSE", the last packet for the file was sent. Close the file
			//Remember that the packet carrying "CLOSE" may be lost or damaged!
			
			fclose(fout);
			
			#if defined __unix__ || defined __APPLE__
				close(s);
			#elif defined _WIN32
				closesocket(s);
			#endif	
			
			printf("Server saved file1_saved.txt \n");//you have to manually check to see if this file is identical to file1.txt
			exit(0);
		}

		else {

		//it is not PACKET nor CLOSE, therefore there might be a damaged packet
		//in this assignment, CLOSE always arrive (read UDP_supporting_functions_2016.c to see why...)
		//do nothing, ignoring the damaged packet? Or send a negative ACK? It is up to you to decide.
			
			if (SN_check(SN_expected, receive_buffer) && CRC_check(receive_buffer)) {
				//if SN and CRC checks pass, save the line
				save_line_without_header(receive_buffer, fout); //save without header
				
				SN_expected++;
				SN_checked++;			
			}
			
			if (SN_checked >= 0) {
				//default: build and send ACK using SN Checked
				ACK_build(send_buffer, SN_checked);
				send_unreliably(s, send_buffer, remoteaddr);
				
			}
			
		}
		
	} 

	//End of infinite loop
	
	#if defined __unix__ || defined __APPLE__
		close(s);	
	#elif defined _WIN32 
		closesocket(s);
	#endif
	
	exit(0);
}

//*******************************************************************
// FUNCTIONS
//*******************************************************************

//Function to save lines and discarding the header
void save_line_without_header(char *receive_buffer, FILE *fout) {
	char sep[2] = " "; //separation is space
	char *word;
	int wcount=0;
	char lineoffile[BUFFERSIZE]="\0";
	for (word = strtok(receive_buffer, sep);word;word = strtok(NULL, sep))
	{
		wcount++;
		if(wcount > NUMBER_OF_WORDS_IN_THE_HEADER){
			strcat(lineoffile,word);
			strcat(lineoffile," ");
		}	
	}
	printf("DATA: %s \n",lineoffile);
	lineoffile[strlen(lineoffile)-1]=(char)'\0';//get rid of last space
	if (fout!=NULL) fprintf(fout,"%s\n",lineoffile);
	else {
		printf("error when trying to write...\n");
		exit(0);
	}
}
//-----------------------------------------------
//Function to build ACK packet using SN_chcked
void ACK_build(char *_send_buffer, int _SN_checked) {
	char CRC_buffer[10];
	char temp_buffer[BUFFERSIZE];
	
	sprintf(temp_buffer, "ACKNOW %d", _SN_checked);
	
	int tempCRC = CRC_poly(temp_buffer);
	sprintf(CRC_buffer,  "%d", tempCRC);
	
	strcat(CRC_buffer, " ");
	strcat(CRC_buffer, temp_buffer);
	
	strcpy(temp_buffer, CRC_buffer);
	
	strcat(temp_buffer, " \r\n");
	
	strcpy(_send_buffer, temp_buffer);
}
//-----------------------------------------------
//Function to checks if the CRC value passes or fail
unsigned int  CRC_check(char *_recieve_buffer) {
	char buffer[BUFFERSIZE];
	char temp_buffer[BUFFERSIZE] = "\0";
	char separation[2] = " "; //separation is space char
	char *word;
	
	int count = 0;
	
	unsigned int CRC_client = 0;
	
	strcpy(buffer, _recieve_buffer);
	
	sscanf(buffer, "%d", &CRC_client);
	
	for(word = strtok(buffer, separation); word; word = strtok(NULL, separation)) {
		count++;
		
		if(count > 1) {
			//skip the first space
			strcat(temp_buffer, word);
			strcat(temp_buffer, " ");
		}
	}
	
	int temp_size = strlen(temp_buffer);
	temp_buffer[temp_size - 1] = 0; // delete the last space
	
	return CRC_client == CRC_poly(temp_buffer); 
}
//-----------------------------------------------
//Function that check if the recieved buffer is equal to the expected
unsigned int  SN_check(int _SN_expected, char *_recieve_buffer) {
	char buffer[BUFFERSIZE];
	char separation[2] = " "; //separation is space char
	char *word;
	
	int count = 0;
	int SN_ext;
	
	strcpy(buffer, _recieve_buffer);
	
	for(word = strtok(buffer, separation); word; word = strtok(NULL, separation)) {
		count++;
		
		if(count == 3) {
			//skip the first 2 spaces
			sscanf(word, "%d", &SN_ext);
		}
	}
	
	return _SN_expected == SN_ext;
}
//-----------------------------------------------
//return the generated CRC value
unsigned int CRC_poly(char *_buffer) {

   unsigned char c;
   unsigned int  rem = 0x0000;

   int buffer_size = strlen((char*)_buffer);

   while(buffer_size-- != 0) {

      for(c = 0x80; c != 0; c /= 2) {

         if((rem & 0x8000) != 0) {
            rem  = rem <<  1;
            rem ^= GENERATOR;
         }
         else {
            rem = rem << 1;
         }


         if((*_buffer & c) != 0) {
            rem ^= GENERATOR;
         }

      }

      _buffer++;
   }

   rem = rem & 0xffff;
   return rem;
}
