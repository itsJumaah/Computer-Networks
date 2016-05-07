//159.334 - Networks - 2016
//************************************************************************/
//CLIENT: prototype written by: 
//-------------------------------------------------------
// Bilal Jumaah     12232659
// Karim Abedrabbo  12226543
// Ashneel Kumar    12239289
//************************************************************************/
//gcc client.c -o client.exe -lws2_32 -std=gnu11
//-------------------------------------------------------
//Otherwise will not function properly or compile errors
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
   #include <stdlib.h>
   #include <winsock2.h>
   #include "UDP_supporting_functions_2016.c"

   #define WSVERS MAKEWORD(2,0)
   WSADATA wsadata;
#endif
// ---------------- MACROS -----------------
#define GENERATOR 0x8005 //0x8005, generator for polynomial division
#define N_GBN       2
#define TIMER_LIMIT 2

#define BUFFERSIZE   80  //the BUFFERSIZE has to be at least big enough to receive the answer from the server
#define SEGMENTSIZE 78 //segment size, i.e., if fgets gets more than this number of bytes is segments the message in smaller parts.

// ---------------- VARIABLES -----------------
struct Timer {
   int count;
   int enabled;
};

struct sockaddr_in localaddr,remoteaddr;
struct Timer timer;

// ---------------- FUNCTIONS PROTOTYPES -----------------

unsigned int CRC_poly(char *_buffer);
unsigned int CRC_check(char *_buffer);
int ACK_SN(char *_buffer);
void build_packet(char *_send_buffer, int _next, char *_sendpacket);

// ---------------- MAIN PROGRAM -----------------
// Simple usage: client IP port, or client IP (use default port) 
int main(int argc, char *argv[]) {

   //********************************************************************
   // WSSTARTUP
   //********************************************************************
   
   #if defined _WIN32 
      if (WSAStartup(WSVERS, &wsadata) != 0) {
         WSACleanup();
         printf("WSAStartup failed\n");
      }
   #endif

   //*******************************************************************
   // Initialization
   //*******************************************************************

   //My own
   int SN_Base   = 0;
   int SN_Next   = 0;
   timer.count   = 0;
   timer.enabled = 1;
   char sendpacket[12] [BUFFERSIZE];
   
   memset(&localaddr, 0, sizeof(localaddr));//clean up
   int localPort=1234;
   localaddr.sin_family = AF_INET;
   localaddr.sin_addr.s_addr = INADDR_ANY;//server address should be local
   localaddr.sin_port = htons(localPort);
   memset(&remoteaddr, 0, sizeof(remoteaddr));//clean up

   //localaddr.sin_addr.s_addr = inet_addr(localIP);
   //char localIP[INET_ADDRSTRLEN]="127.0.0.1";
  
   randominit();
  
   //********************************************************************
   
   #if defined __unix__ || defined __APPLE__
      int s;
   #elif defined _WIN32
      SOCKET s;
   #endif
   
   char send_buffer[BUFFERSIZE],receive_buffer[BUFFERSIZE];
   remoteaddr.sin_family = AF_INET;
   
   //*******************************************************************
   //	Dealing with user's arguments
   //*******************************************************************
   
   if (argc != 5) {
      printf("2011 code USAGE: client remote_IP-address port  lossesbit(0 or 1) damagebit(0 or 1)\n");
      exit(1);
   }
   
   remoteaddr.sin_addr.s_addr = inet_addr(argv[1]);//IP address
   remoteaddr.sin_port = htons((u_short)atoi(argv[2]));//always get the port number
   packets_lostbit=atoi(argv[3]);
   packets_damagedbit=atoi(argv[4]);
  
   if (packets_damagedbit<0 || packets_damagedbit>1 || packets_lostbit<0 || packets_lostbit>1){
      printf("2011 code USAGE: client remote_IP-address port  lossesbit(0 or 1) damagebit(0 or 1)\n");
      exit(0);
   }
  
   //*******************************************************************
   //CREATE CLIENT'S SOCKET 
   //*******************************************************************
   
   s = socket(AF_INET, SOCK_DGRAM, 0);//this is a UDP socket
   
   if (s < 0) {
      printf("socket failed\n");
      exit(1);
   }

   //***************************************************************
   //NONBLOCKING OPTION for Windows
   //***************************************************************

   #if defined _WIN32
      u_long iMode=1;
      ioctlsocket(s,FIONBIO,&iMode);
   #endif

   //*******************************************************************
   //SEND A TEXT FILE 
   //*******************************************************************
  
   #if defined __unix__ || defined __APPLE__
      FILE *fin=fopen("file1.txt","r");
   #elif defined _WIN32
      FILE *fin=fopen("file1_Windows.txt","r");
   #endif
  
   if(fin==NULL) {
      printf("cannot open file\n");
      exit(0);
   }

   while (1) {

      //Run the timer if enabled
      if(timer.enabled) {
         timer.count++;
      }
      //--------
      memset(send_buffer, 0, sizeof(send_buffer));//clean up the send_buffer before reading the next line
      
      if(SN_Next < (SN_Base + N_GBN)) { //Error checking
         fgets(send_buffer,SEGMENTSIZE,fin);
      }
     
      if (!feof(fin)) {
         //Send normally
			
         if(SN_Next < (SN_Base + N_GBN)) {
            build_packet(send_buffer, SN_Next, sendpacket[SN_Next]); //Build it
            send_unreliably(s, sendpacket[SN_Next], remoteaddr);     //Send it

            if(SN_Base == SN_Next) {
               timer.enabled = 1; //enable the timer
            }
            SN_Next++;
         }

         //if timedout, send all packets as not acknowledged

         if(timer.count > TIMER_LIMIT) {
            timer.enabled = 1; // enable the timer
				int i;
            for(i = SN_Base; i < SN_Next; i++) {
               send_unreliably(s, sendpacket[i], remoteaddr);
               #if defined __unix__ || defined __APPLE__
                  sleep(1);
               #elif defined _WIN32
                  Sleep(1);
               #endif
            }
         }
         
         #if defined __unix__ || defined __APPLE__
            sleep(1);
         #elif defined _WIN32
            Sleep(1000);
         #endif
        
         //********************************************************************
         //RECEIVE
         //********************************************************************
         
         memset(receive_buffer, 0, sizeof(receive_buffer));
         recv_nonblocking(s,receive_buffer, remoteaddr);//you can replace this, but use MSG_DONTWAIT to get non-blocking recv
         printf("RECEIVE --> %s \n",receive_buffer);
         
         //when CRC check is passed

         if(CRC_check(receive_buffer)) {
            

            if(ACK_SN(receive_buffer) > -1) {
               SN_Base = ACK_SN(receive_buffer) + 1;
            }
            //Stop the timer
            if(SN_Base == SN_Next) {
               timer.enabled = 0;
               timer.count   = 0;
            }
            //Start the timer;
            else {
               timer.enabled = 1;
            }
         }

         #if defined __unix__ || defined __APPLE__
            sleep(1);//wait for a bit before trying the next packet
         #elif defined _WIN32
            Sleep(1000);
         #endif

      }
      else {
         printf("end of the file \n"); 
         memset(send_buffer, 0, sizeof(send_buffer)); 
         sprintf(send_buffer,"CLOSE \r\n");
         send_unreliably(s,send_buffer,remoteaddr);//we actually send this reliably, read UDP_supporting_functions_2016.c

         break;
      }
   }
   
   //*******************************************************************
   //CLOSESOCKET   
   //*******************************************************************
   
   printf("closing everything on the client's side ... \n");
   fclose(fin);
   
   #if defined __unix__ || defined __APPLE__
      close(s);
   #elif defined _WIN32
      closesocket(s);
   #endif

   exit(0);
}

// ---------------- MAIN ENDS - FUNCTIONS STARTS -----------------


//Build the packets and save it in _sendpacket
void build_packet(char *_send_buffer, int _next, char *_sendpacket) {
   //buffers definition
   char send_buffer[BUFFERSIZE];
   char temp_buffer[BUFFERSIZE];
   char CRC_buffer[BUFFERSIZE];

   //memory set
   memset(send_buffer, 0, sizeof(send_buffer));
   memset(temp_buffer, 0, sizeof(temp_buffer));
   memset(CRC_buffer , 0, sizeof(CRC_buffer ));

   strcpy(send_buffer, _send_buffer);
   //SN DATA
   sprintf(temp_buffer, "PACKET %d ", _next);
   strcat(temp_buffer, _send_buffer);

   int temp_buffer_size = strlen(temp_buffer);
   temp_buffer[temp_buffer_size-1] = 0;

   sprintf(CRC_buffer, "%d ", CRC_poly(temp_buffer));
   //CRC SN DATA
   strcat(CRC_buffer, temp_buffer);

   strcat(CRC_buffer, "\n");
   strcpy(_sendpacket, CRC_buffer);
}
//--------------------------------------------------

//Return the ACK SN from the buffer specified
int ACK_SN(char *_buffer) {

   char temp_buffer[BUFFERSIZE];
   char separation[2] = " ";
   char *word;
   int count = 0;
   int SN    = -10;


   strcpy(temp_buffer, _buffer);

   for (word = strtok(temp_buffer, separation); word; word = strtok(NULL, separation)) {
      
      count++;

      if(count == 3) {
         //Skip the first space
         sscanf(word, "%d", &SN);
      }

   }

   return SN;
}
//--------------------------------------------------

//Check the CRC value, return 1 if pass, otherwise 0
unsigned int CRC_check(char *_buffer) {
   
   char temp_buffer [BUFFERSIZE];
   char temp_buffer2[BUFFERSIZE] = "\0";

   char separation[2] = " ";
   char *word;
   
   unsigned int CRC_Client = 0;
            int count      = 0;
   
   strcpy(temp_buffer, _buffer);
   sscanf(temp_buffer, "%d", &CRC_Client);

   for(word = strtok(temp_buffer, separation); word; word = strtok(NULL, separation)) {

      count++;

      if(count > 1) {
         strcat(temp_buffer2, word);
         strcat(temp_buffer2, " ");
      }

   }

   int temp_size = strlen(temp_buffer2);
   temp_buffer2[temp_size - 1] = 0;

   return CRC_Client == CRC_poly(temp_buffer2);

}
//--------------------------------------------------

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
