//159.334 - Networks
// CLIENT updated 2013
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
	#include <stdlib.h>
	#include <winsock.h>

	#define WSVERS MAKEWORD(2,0)

	WSADATA wsadata;
#endif

//----- Definitions and variables
#define BUFFESIZE 200 
//remember that the BUFFESIZE has to be at least big enough to receive the answer from the server
#define SEGMENTSIZE 70
//segment size, i.e., if fgets gets more than this number of bytes it segments the message

typedef struct Key {
	int n;
	int e;
} Key;


//----- Functions prototypes

int  repeatsquare(int x, int e, int n);
void keyBuild(Key* _key, char* _receive_buffer);
void encryptRSA(Key* _key, char* _send_buffer);

//----- Main

int main(int argc, char *argv[]) {
	//*******************************************************************
	// Initialization
	//*******************************************************************
	struct sockaddr_in remoteaddr;
	struct hostent *h;

	int n;
	int bytes;

	char send_buffer[BUFFESIZE];
	char receive_buffer[BUFFESIZE];

	Key key;

	#if defined __unix__ || defined __APPLE__
	int s;
	#elif defined _WIN32 
	SOCKET s;
	#endif

	memset(&remoteaddr, 0, sizeof(remoteaddr)); //clean up 
	//*******************************************************************
	//WSASTARTUP 
	//*******************************************************************
	#if defined _WIN32 
		if (WSAStartup(WSVERS, &wsadata) != 0) {
			WSACleanup();
			printf("WSAStartup failed\n");
			exit(1);
		}
	#endif
	//*******************************************************************
	//	Dealing with user's arguments
	//*******************************************************************
	if (argc != 3) {
		printf("USAGE: client IP-address port\n");
		exit(1);
	}
	else {
		remoteaddr.sin_addr.s_addr = inet_addr(argv[1]);//IP address
		remoteaddr.sin_port = htons((u_short)atoi(argv[2]));//Port number
	}
	//*******************************************************************
	//CREATE CLIENT'S SOCKET 
	//*******************************************************************
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		printf("socket failed\n");
		exit(1);
	}
	remoteaddr.sin_family = AF_INET;
	//*******************************************************************
	//CONNECT
	//*******************************************************************
	if (connect(s, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr)) != 0) {
		printf("connect failed\n");
		exit(1);
	}

	//Requirement - Recieve public key
	n = 0;

	while(1) {
		bytes = recv(s, &receive_buffer[n], 1, 0);
		//validation
		if(bytes <= 0) {
			printf("recv failed\n");
			exit(1);
		}
		//end on a LF
		if(receive_buffer[n] == '\n') {
			receive_buffer[n] = '\0';
			break;
		}
		//ignore CRs
		if(receive_buffer[n] != '\r') {
			n++;
		}
	}

	//building public key
	keyBuild(&key, receive_buffer);
	printf("@alan: key.n = %d\n       key.e = %d\n\n", key.n, key.e);

	//*******************************************************************
	//Get input while user don't type "."
	//*******************************************************************
	memset(&send_buffer, 0, BUFFESIZE);
	fgets(send_buffer,SEGMENTSIZE,stdin);

	while (strncmp(send_buffer,".",1) != 0) {
//here
		encryptRSA(&key, send_buffer);

		//strip send_buffer from '\n' for unix
		#if defined __unix__ || defined __APPLE__
			send_buffer[strlen(send_buffer)-1]='\0';
			strcat(send_buffer,"\r\n");
		#endif


//or here ?

		printf("lenght is %d \n",(int)strlen(send_buffer));
		printf(">>> %s\n",send_buffer);//line sent  
		
		//*******************************************************************
		//SEND
		//*******************************************************************
		
		bytes = send(s, send_buffer, strlen(send_buffer),0);
		
		if (bytes < 0) {
			printf("send failed\n");
			exit(1);
		}
		
		n = 0;
		
		while (1) {
			//*******************************************************************
			//RECEIVE
			//*******************************************************************
			
			bytes = recv(s, &receive_buffer[n], 1, 0);
			//validation
			if ((bytes <= 0)) {
				printf("recv failed\n");
				exit(1);
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
		
		printf("%s \n",receive_buffer);// line received
		memset(&send_buffer, 0, BUFFESIZE);
		fgets(send_buffer,SEGMENTSIZE,stdin);
	}
	//*******************************************************************
	//CLOSESOCKET   
	//*******************************************************************
	
	#if defined __unix__ || defined __APPLE__
		close(s);
	#elif defined _WIN32 
		closesocket(s);
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
void keyBuild(Key* _key, char* _receive_buffer) {
	char buffer[80];
	char separation[2] = " ";
	char *word;
	char count;

	strcpy(buffer, _receive_buffer);

	for (word = strtok(buffer, separation); word; word = strtok(NULL, separation)) {
		count++;

		if(count == 2) {
			sscanf(word, "%d", &_key->n);
		}

		if (count == 3) {
			sscanf(word, "%d", &_key->e);
		}
	}
}

//------------------------------------------
void encryptRSA(Key* _key, char* _send_buffer) {
	int  int_temp[BUFFESIZE];
	char char_temp[BUFFESIZE];

	int i = 0;
	int j = 0;

	memset(int_temp,  0, BUFFESIZE);
	memset(char_temp, 0, BUFFESIZE);

	while (*_send_buffer != 10) {
		int_temp[i] = repeatsquare(*_send_buffer, _key->e, _key->n);

		_send_buffer++;
		i++;
	}

	while (i != 0) {
		_send_buffer--;
		i--;
	}

	while(int_temp[j] != 0) {
		char_temp[2*j   ] = 0x40 | (int_temp[j] >> 4  );
		char_temp[2*j +1] = 0x60 | (int_temp[j] & 0x0f);
	}

	strcpy(_send_buffer, char_temp);
}
