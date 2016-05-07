//Written by Bilal Jumaah    12232659
//			and Karim Abedrabbo 12226543
//				 Ashneel Kumar	  12239289

//159.334 - Networks
//FTP server prototype for assignment 1, 159334
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
	#include <windows.h>
	#include <stdio.h> 
	#include <string.h>
	#include <stdlib.h>
	#include <winsock.h>
	#include <winsock2.h>
	//#include <ws2tcpip.h>
	#define WSVERS MAKEWORD(2,0)
	WSADATA wsadata;
#endif

#define BUFFERSIZE 800
#if defined __unix__ || defined __APPLE__
#elif defined _WIN32 
#endif

void getFileName(char  *_send_buffer, char *_fileName);
void validation(char *_receive_buffer, char *_send_buffer, int _bytes, int _ns);
//*******************************************************************
//MAIN
//*******************************************************************
int main(int argc, char *argv[]) {
	//********************************************************************
	// INITIALIZATION
	//********************************************************************
	struct sockaddr_in localaddr,remoteaddr;
	struct sockaddr_in remoteaddr_act; //from part2
	#if defined __unix__ || defined __APPLE__
		int s,ns;
		int s_data_act = 0;
	#elif defined _WIN32 
		SOCKET s,ns;
		SOCKET s_data_act = 0;
	#endif
		char send_buffer[BUFFERSIZE], receive_buffer[BUFFERSIZE], temp_buffer[BUFFERSIZE];
		char fileName[40];
		//char remoteIP[INET_ADDRSTRLEN];
		//int remotePort;
		//int localPort;//no need for local IP...
		int n,bytes,addrlen;
		memset(&localaddr,0,sizeof(localaddr));//clean up the structure
		memset(&remoteaddr,0,sizeof(remoteaddr));//clean up the structure
	//*******************************************************************
	//WSASTARTUP 
	//*******************************************************************
	#if defined __unix__ || defined __APPLE__
		//nothing to do in Linux
	#elif defined _WIN32 
		if (WSAStartup(WSVERS, &wsadata) != 0) {
			WSACleanup();
			printf("WSAStartup failed\n");
			exit(1);
		}
	#endif
	//********************************************************************
	//SOCKET
	//********************************************************************

	s = socket(PF_INET, SOCK_STREAM, 0);
	if (s <0) {
		printf("socket failed\n");
	}
	localaddr.sin_family = AF_INET;
	if (argc == 2) localaddr.sin_port = htons((u_short)atoi(argv[1]));
	else localaddr.sin_port = htons(1121);//default listening port 
	localaddr.sin_addr.s_addr = INADDR_ANY;//server address should be local
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
		printf("The server is available at port %d running on ", ntohs(localaddr.sin_port)); //mine
		#if defined __unix__ || defined __APPLE__
			printf("UNIX OS.\n");
			ns = accept(s,(struct sockaddr *)(&remoteaddr),(socklen_t*)&addrlen);
		#elif defined _WIN32
			printf("WIN32 OS.\n");
			ns = accept(s,(struct sockaddr *)(&remoteaddr),&addrlen);
		#endif
		if (ns <0 ) break;
		printf("accepted a connection from client IP %s port %d \n",inet_ntoa(remoteaddr.sin_addr),ntohs(localaddr.sin_port));
		//********************************************************************
		//Respond with welcome message
		//*******************************************************************
		sprintf(send_buffer,"220 Welcome to FTP prototype server using port %d.\r\n\n", ntohs(localaddr.sin_port)); //this
		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		
		
		while (1) {
			n = 0;
			while (1) {
				//********************************************************************
				//RECEIVE
				//********************************************************************
				bytes = recv(ns, &receive_buffer[n], 1, 0);//receive byte by byte...
				//********************************************************************
				//PROCESS REQUEST
				//********************************************************************
				if ( bytes <= 0 ) break;
				if (receive_buffer[n] == '\n') { /*end on a LF*/
					receive_buffer[n] = '\0';
					break;
				}
				if (receive_buffer[n] != '\r') n++; /*ignore CRs*/
			}
			if ( bytes <= 0 ) break;

			printf("-->DEBUG: the message from client reads: '%s' \r\n", receive_buffer);
			
			validation(receive_buffer, send_buffer, bytes, ns);
			//---
			
			if (strncmp(receive_buffer,"USER",4)==0)  {
				printf("Typing password...\n");
				sprintf(send_buffer,"331 Password required! \r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
			}
			else if (strncmp(receive_buffer,"PASS",4)==0)  {
				printf("A user has logged in from %s.\n", inet_ntoa(remoteaddr.sin_addr));
				sprintf(send_buffer,"230 Public login sucessful! \r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
			}
			else if (strncmp(receive_buffer,"SYST",4)==0)  {
				#if defined __unix__ || defined __APPLE__ 
					system("system_profiler > tmp.txt");
					sprintf(send_buffer,"The server is currently running on UNIX OS (Linux / Mac OS X).\r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					if (bytes < 0) break;
				#elif defined _WIN32 
					system("systeminfo > tmp.txt");
					sprintf(send_buffer,"The server is currently running on WIN32 OS (Microsoft Windows).\r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					if (bytes < 0) break;
				#endif
				FILE *fin=fopen("tmp.txt","r");//open tmp.txt file
				sprintf(send_buffer,"150 Transfering... \r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				//char temp_buffer[80];
				while (!feof(fin)){
					fgets(temp_buffer, BUFFERSIZE-2,fin); //78
					sprintf(send_buffer,"%s",temp_buffer);
					send(s_data_act, send_buffer, strlen(send_buffer), 0);
				}
				fclose(fin);
				sprintf(send_buffer,"226 File transfer completed... \r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				#if defined __unix__ || defined __APPLE__ 
					close(s_data_act);
				#elif defined _WIN32
					closesocket(s_data_act);
				#endif
				system("del tmp.txt");
			}
			//this part was taken from part 2
			else if(strncmp(receive_buffer,"PORT",4)==0) {
				s_data_act = socket(AF_INET, SOCK_STREAM, 0);
				//local variables
				unsigned char act_port[2];
				int act_ip[4], port_dec;
				char ip_decimal[40];
				sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d",&act_ip[0],&act_ip[1],&act_ip[2],&act_ip[3],(int*)&act_port[0],(int*)&act_port[1]);
				remoteaddr_act.sin_family=AF_INET;//local_data_addr_act
				sprintf(ip_decimal, "%d.%d.%d.%d", act_ip[0], act_ip[1], act_ip[2],act_ip[3]);
				printf("IP is %s\n",ip_decimal);
				remoteaddr_act.sin_addr.s_addr=inet_addr(ip_decimal);
				port_dec=act_port[0]*256+act_port[1];
				printf("port %d\n",port_dec);
				remoteaddr_act.sin_port=htons(port_dec);
				if (connect(s_data_act, (struct sockaddr *)&remoteaddr_act, (int) sizeof(struct sockaddr)) != 0){
					printf("trying connection in %s %d\n",inet_ntoa(remoteaddr_act.sin_addr),ntohs(remoteaddr_act.sin_port));
					sprintf(send_buffer, "425 Something is wrong, can't start the active connection... \r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					#if defined __unix__ || defined __APPLE__ 
						close(s_data_act);
					#elif defined _WIN32 
						closesocket(s_data_act);
					#endif
				}
				else {
					sprintf(send_buffer, "200 Ok\r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					printf("Data connection to client created (active connection) \n");
				}
			}
			else if ((strncmp(receive_buffer,"LIST",4)==0) || (strncmp(receive_buffer,"NLST",4)==0))   {
				#if defined __unix__ || defined __APPLE__ 
					system("ls > tmp.txt");
				#elif defined _WIN32 
					system("dir > tmp.txt");
				#endif		
				FILE *fin=fopen("tmp.txt","r");//open tmp.txt file
				sprintf(send_buffer,"150 Transfering... \r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				//char temp_buffer[BUFFERSIZE]; //80
				while (!feof(fin)){
					fgets(temp_buffer, BUFFERSIZE -2,fin); //78
					sprintf(send_buffer,"%s",temp_buffer);
					send(s_data_act, send_buffer, strlen(send_buffer), 0);
				}
				fclose(fin);
				sprintf(send_buffer,"226 File transfer completed... \r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				#if defined __unix__ || defined __APPLE__ 
					close(s_data_act);
				#elif defined _WIN32
					closesocket(s_data_act);
				#endif		
				//OPTIONAL, delete the temporary file after listing
				system("del tmp.txt");
			}
			//end of part 2
			//start of part 3 and 4

			else if((strncmp(receive_buffer,"STOR",4)==0)) {
				getFileName(receive_buffer, fileName);
				if(fopen(fileName,"w")  ==  NULL) {
					printf("File doesn't exist!\n");
					sprintf(send_buffer,"450 Requested file action not taken. \r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				else {
					FILE *fout=fopen(fileName,"w");

					sprintf(send_buffer,"150 File status okay; about to open data connection. \r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					while(1) {
						n = 0;
						while(1) {
							//RECEIVE
							bytes = recv(s_data_act, &receive_buffer[n], 1, 0);//receive byte by byte...
							//PROCESS REQUEST
							if ( bytes <= 0 ) {
								break;
							}
							if (receive_buffer[n] != '\r') {
								n++; /*ignore CRs*/
							}
						}
						receive_buffer[n] = '\0';
						fprintf(fout,"%s",receive_buffer);
						if (bytes <= 0)  { 
							break; 
						}
					}

					sprintf(send_buffer,"226 File transfer completed... \r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);

					fclose(fout);
					//closesocket(s_data_act);
					#if defined __unix__ || defined __APPLE__
						close(s_data_act);
					#elif defined _WIN32 
						closesocket(s_data_act);
					#endif
				}
			}

			else if((strncmp(receive_buffer,"RETR",4)==0)) {
				getFileName(receive_buffer, fileName);
				if(fopen(fileName,"r")==NULL) {
					sprintf(send_buffer,"450 Requested file action not taken. \r\n");
					printf("File doesn't exist!\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				else {
					FILE *fin=fopen(fileName,"r");		//open tmp.txt file
					sprintf(send_buffer,"150 Transfering... \r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					int i =0;
					while (true) {
						fgets(temp_buffer, BUFFERSIZE-2,fin); //78	
						
						// sprintf(send_buffer,"\r\n");
						// send(s_data_act, send_buffer, strlen(send_buffer), 0);
						
						if (feof(fin)) {
							temp_buffer[i] = '\0';
							sprintf(send_buffer,"%s",temp_buffer);
							send(s_data_act, send_buffer, strlen(send_buffer), 0);
							break;
						}
						sprintf(send_buffer,"%s\r\n",temp_buffer);
						send(s_data_act, send_buffer, strlen(send_buffer), 0);
						i++;
					}
					fclose(fin);
					sprintf(send_buffer,"226 File transfer completed... \r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}

				#if defined __unix__ || defined __APPLE__
					close(s_data_act);
				#elif defined _WIN32 
					closesocket(s_data_act);
				#endif

			}


			//end of parts 3 and 4
			else if (strncmp(receive_buffer,"QUIT",4)==0)  {
				printf("Quit \n");
				sprintf(send_buffer,"221 Connection close by client\r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
				#if defined __unix__ || defined __APPLE__
					close(ns);
				#elif defined _WIN32 
					closesocket(ns);
				#endif
			}
		}
		//********************************************************************
		//CLOSE SOCKET
		//********************************************************************
		#if defined __unix__ || defined __APPLE__
			close(ns);
		#elif defined _WIN32 
			closesocket(ns);
		#endif
		printf("disconnected from %s\n",inet_ntoa(remoteaddr.sin_addr));
	}
	#if defined __unix__ || defined __APPLE__
		close(s);//it actually never gets to this point....use CTRL_C
	#elif defined _WIN32 
		closesocket(s);//it actually never gets to this point....use CTRL_C
	#endif
}


void getFileName(char *_send_buffer, char *_fileName) {
	int len, i;
	len = strlen(_send_buffer) - 5; //the command is 4 + space char
	for(i=0; i < len; i++) {
		_fileName[i] = _send_buffer[i+5];
	}
	_fileName[i] = '\0';
}

void validation(char *_receive_buffer, char *_send_buffer, int _bytes, int _ns) {
	if(strncmp(_receive_buffer,"USER",4) && strncmp(_receive_buffer,"PASS",4)
		&& strncmp(_receive_buffer,"SYST",4) && strncmp(_receive_buffer,"PORT",4)
		&& strncmp(_receive_buffer,"STOR",4) && strncmp(_receive_buffer,"RETR",4)
		&& strncmp(_receive_buffer,"LIST",4) && strncmp(_receive_buffer,"NLST",4)
		&& strncmp(_receive_buffer,"QUIT",4)) {
		printf("Invalid command \"%s\" recieved.\n", _receive_buffer);
		sprintf(_send_buffer,"202 Command \"%s\" not implemented, superfluous at this site. \r\n\n", _receive_buffer);
		_bytes = send(_ns, _send_buffer, strlen(_send_buffer), 0);
	}
}