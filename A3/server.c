//159.334 - Networks
// SERVER updated 2013
//This prototype can be compiled with gcc (or g++) in both Linux and Windows
//To see the differences, just follow the ifdefs

#if defined __unix__ || defined __APPLE__
   #include <unistd.h>
   #include <errno.h>
   #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <arpa/inet.h>

#elif defined _WIN32
   # include <windows.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <winsock.h>

   #define WSVERS MAKEWORD(2,0)

   WSADATA wsadata;

#endif

//----- Definitions and variables
#define BUFFESIZE 200 //remember that the BUFFESIZE has to be at least big enough to receive the answer
#define SEGMENTSIZE 198 //segment size, i.e., BUFFESIZE - 2 bytes (for \r\n)

//----- Functions prototypes

int  repeatsquare(int x, int e, int n);
void decryptRSA(char* _receive_buffer, int n, int d);

//*******************************************************************
//MAIN
//*******************************************************************
main(int argc, char *argv[]) {

   //@alan {n, e, d}

   int countKey = 0;

   int Keys[3][3] = {
      {143, 7, 103},
      {187, 27, 83},
      {209, 17, 53}
   };

   //********************************************************************
   // INITIALIZATION
   //********************************************************************
   struct sockaddr_in localaddr,remoteaddr;

   #if defined __unix__ || defined __APPLE__
      int s, ns;
   #elif defined _WIN32
      SOCKET s, ns;
   #endif
   
   char send_buffer[BUFFESIZE];
   char receive_buffer[BUFFESIZE];

   memset(&send_buffer,0,BUFFESIZE);
   memset(&receive_buffer,0,BUFFESIZE);

   int n, bytes, addrlen;

   memset(&localaddr,0,sizeof(localaddr));//clean up the structure
   memset(&remoteaddr,0,sizeof(remoteaddr));//clean up the structure
   
   //********************************************************************
   // WSSTARTUP
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

   s = socket(PF_INET, SOCK_STREAM, 0);
   
   if (s < 0) {
      printf("socket failed\n");
   }

   localaddr.sin_family = AF_INET;
   
   if (argc == 2) { 
      localaddr.sin_port = htons((u_short)atoi(argv[1]));
   }
   else {
      localaddr.sin_port = htons(1234);//default listening port
   }
   
   localaddr.sin_addr.s_addr = INADDR_ANY;//server IP address should be local
   //********************************************************************
   //BIND
   //********************************************************************
   if (bind(s,(struct sockaddr *)(&localaddr),sizeof(localaddr)) != 0) {
      printf("Bind failed!\n");
      exit(0);
   }
   //********************************************************************
   //LISTEN
   //********************************************************************
   
   listen(s,5);

   //********************************************************************
   //INFINITE LOOP
   //********************************************************************
   while (1) {
      addrlen = sizeof(remoteaddr);
      
      //********************************************************************
      //NEW SOCKET newsocket = accept
      //********************************************************************
      
      #if defined __unix__ || defined __APPLE__
         ns = accept(s,(struct sockaddr *)(&remoteaddr),(socklen_t*)&addrlen);
      #elif defined _WIN32
         ns = accept(s,(struct sockaddr *)(&remoteaddr),&addrlen);
      #endif
      
      if (ns < 0) { 
         break;
      }
      
      printf("accepted a connection from client IP %s port %d \n",inet_ntoa(remoteaddr.sin_addr),ntohs(localaddr.sin_port));
     

      //Sending public key
      if (countKey > 2) {
         countKey = 0;
      }

      sprintf(send_buffer, "Public_Key %d %d\r\n", Keys[countKey][0], Keys[countKey][1]);

      bytes = send(ns, send_buffer, strlen(send_buffer), 0);
      printf("@alan: %s", send_buffer);


      while (1) {
         
         n = 0;
         
         while (1) {

            //********************************************************************
            //RECEIVE
            //********************************************************************
            
            bytes = recv(ns, &receive_buffer[n], 1, 0);
            
            //********************************************************************
            //PROCESS REQUEST
            //********************************************************************
            
            if (bytes <=0) {
               break;
            }
            //end on a LF
            if (receive_buffer[n] == '\n') {
               receive_buffer[n] = '\0';
               break;
            }
            //ignore CRs
            if (receive_buffer[n] != '\r') {
               n++;
            }
         }

         if (bytes <= 0) { 
            break;
         }

         printf("The client is sending: %s\n",receive_buffer);
         memset(&send_buffer, 0, BUFFESIZE);
         sprintf(send_buffer, "<<< SERVER SAYS:The client typed '%s' - There are %d bytes of information\r\n", receive_buffer, n);


         //decryption + saving it into receive_buffer
         decryptRSA(receive_buffer, Keys[countKey][0], Keys[countKey][2]);
         printf("@alan the decrypted message is: \"%s\"\n", receive_buffer);
         printf("---------------------------------\n");


         //********************************************************************
         //SEND
         //********************************************************************
         
         bytes = send(ns, send_buffer, strlen(send_buffer), 0);
         
         if (bytes < 0) {
            break;
         }
      }
      //********************************************************************
      //CLOSE SOCKET
      //********************************************************************
      
      #if defined __unix__ || defined __APPLE__
         close(ns); //close connecting socket
      #elif defined _WIN32
         closesocket(ns); //close connecting socket
      #endif
      
      printf("disconnected from %s\n", inet_ntoa(remoteaddr.sin_addr));


      //next key
      countKey++;
   }

   #if defined __unix__ || defined __APPLE__
      close(s); //close listening socket
   #elif defined _WIN32
      closesocket(s); //close listening socket
   #endif
   
   return 0;
}

//------------------------------------------
//Functions definitions
//------------------------------------------
int repeatsquare(int x, int e, int n) {
   
   int y = 1;
   
   while(e > 0){

      if( (e%2) == 0) {
         x = (x*x) % n;
         e = e / 2;
      }

      else {
         y = (x*y) % n;
         e = e - 1;
      }
   }

   return y;
}

//------------------------------------------
void decryptRSA(char* _receive_buffer, int n, int d) {

   unsigned int i = 0;
   unsigned int j = 0;

   char temp_buffer[BUFFESIZE];
   char temp_decryp[BUFFESIZE];

   int  temp_build[BUFFESIZE];

   memset(temp_buffer, 0, BUFFESIZE);
   memset(temp_decryp, 0, BUFFESIZE);
   memset(temp_build,  0, BUFFESIZE);

   strcpy(temp_buffer, _receive_buffer);

   while((2*i) < strlen(temp_buffer)) {

      temp_build[i] = ((temp_buffer[2*i] & 0x0f) << 4) | (temp_buffer[2*i + 1] & 0x0f);

      i++;
   }

   while(temp_build[j] != 0) {
      
      temp_decryp[j] = repeatsquare(temp_build[j], d, n);

      j++;
   }

   strcpy(_receive_buffer, temp_decryp);
}
