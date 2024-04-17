#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#define SA struct sockaddr
/* You will to add includes here */

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
//#define DEBUG


// Included to get the support library
#include <calcLib.h>
//Function to recieve a message from the server
void recieveMessage(int &socket_desc, char* server_message, unsigned int msg_size){

  //Clears the message array before recieving the server msg.
  //Because for some reason the function doesnt do it by itself
  memset(server_message, 0, msg_size);
  if(recv(socket_desc, server_message, msg_size, 0) < 0){
  	#ifdef DEBUG
  	printf("Error receiving message\n");
  	#endif
  	exit(-1);
  }
  else 
    printf(server_message);
} 
void sendMessage(int &socket_desc, char* client_message, unsigned int msg_size){

  //Sends message to client, clearing the array at the end to not interfere with
  //the messages we put in before calling the function
  if(send(socket_desc, client_message, msg_size, 0) < 0){
	#ifdef DEBUG
  	printf("Unable to send message\n");
  	#endif DEBUG
  	exit(-1);
  }
  else printf(client_message);
 
}
void calculateMessage(char* server_message, int &socket_desc){
	int i1, i2, iresult;
	double f1, f2, fresult;
	char* operation, *grab1, *grab2, *temp1, *temp2;
	
	if(server_message[0] == 'f'){
		operation = strtok(server_message, " ");
		grab1 = strtok(NULL, " ");
		grab2 = strtok(NULL, "\\");
		f1 = atof(grab1); 
		f2 = atof(grab2);
		
		if(strcmp(operation, "fadd") == 0){
			fresult = f1+f2;
		} else if(strcmp(operation, "fsub") == 0){
			fresult = f1-f2;
		} else if(strcmp(operation, "fmul") == 0){
			fresult = f1*f2;
		} else if(strcmp(operation, "fdiv") == 0){
			fresult = f1/f2;
		}
		
		char* str = (char*)malloc(1450);
		sprintf(str, "%8.8g\n", fresult);
		//TODO UNCOMMENT THIS
		sendMessage(socket_desc, str, strlen(str));
		return;
		
	} else {
	
	  operation = strtok(server_message, " ");
		grab1 = strtok(NULL, " ");
		grab2 = strtok(NULL, "\\");
		i1 = atoi(grab1); 
		i2 = atoi(grab2);
		
		if(strcmp(operation, "add") == 0){
			iresult = i1+i2;
		} else if(strcmp(operation, "sub") == 0){
			iresult = i1-i2;
		} else if(strcmp(operation, "mul") == 0){
			iresult = i1*i2;
		} else if(strcmp(operation, "div") == 0){
		//Make result into string then add \n
			iresult = i1/i2;
		}
		char* strResult = (char*)malloc(1450);
		sprintf(strResult, "%d\n", iresult);
		//TODO UNCOMMENT THIS
		sendMessage(socket_desc, strResult, strlen(strResult));
		return;
	}


}

int CAP = 2000;
int main(int argc, char *argv[]){


	//Variables
	char* splits[CAP];
  char* p = strtok(argv[1], ":");
  int delimCounter = 0;
  char *Desthost;
  char *Destport;
  int port;
	int serverfd;
	struct sockaddr_in client;
	char server_message[CAP];
  //Get argv
  while(p != NULL){
  	//Look for the amount of ":" in argv to determine if ipv4 or ipv6
  	splits[delimCounter++] = p;
  	p = strtok(NULL, ":");
  }
  Destport = splits[--delimCounter];
  Desthost = splits[0];
  for(int i = 1;i<delimCounter;i++){
  	
  	sprintf(Desthost, "%s:%s",Desthost, splits[i]);
  }
  port=atoi(Destport);
  printf("Host %s and port %d.\n",Desthost,port);
	
	//Getaddrinfo
	struct addrinfo hints, *serverinfo = 0;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if(getaddrinfo(Desthost, Destport, &hints, &serverinfo) < 0){
		printf("Getaddrinfo error: %s\n", strerror(errno)); 
		exit(0);
	} //else printf("Getaddrinfo success\n");
  
  //Create TCP socket
  int socket_desc;
  struct sockaddr_in server_addr;
  socket_desc = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
  
  if(socket_desc < 0){
  	#ifdef DEBUG
  	printf("Unable to create socket\n");
  	#endif
  	return -1;
  }
  #ifdef DEBUG
  else printf("Socket Created\n");
  #endif
  
  
#ifdef DEBUG 
  printf("Host %s, and Port %d.\n",Desthost,port);
#endif

  //Create Socket Structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(Desthost);
  int error;
  
  //Establish Connection
  error = connect(socket_desc, serverinfo->ai_addr, serverinfo->ai_addrlen);
  if(error < 0){
  	#ifdef DEBUG
  	printf("Unable to connect\n");
  	printf("Error: %d \n", errno);
  	#endif
  	return -1;
  }
  #ifdef DEBUG
  else printf("Connected\n");
  #endif
  
  //Recieve message from server
  recieveMessage(socket_desc, server_message, sizeof(server_message));
  
  //Compare strings to verify version

  if(strcmp(server_message,"TEXT TCP 1.0\n") == 0){
  	#ifdef DEBUG
  	printf("Same\n");
  	#endif DEBUG
  	char* str = "OK\n";
  	//strcpy(client_message, str);
  	//Send back the OK
  	sendMessage(socket_desc, str, strlen(str));
  }
  else{
	  printf("Closing connection\n");
	  close(socket_desc);
	  return -1;
  }

	//sleep(6);
  //Recieve the problem
  printf("We are here\n");
  recieveMessage(socket_desc, server_message, sizeof(server_message));
  
  if(strcmp(server_message, "ERROR TO\n") == 0){
	  printf("We got TO'ed. Closing connection\n");
	  close(socket_desc);
	  return -1;
  }
  
  //Translate Message
  calculateMessage(server_message, socket_desc);
  
  //Send answer to server
  //sendMessage(socket_desc, client_message, sizeof(client_message));
  
  //Recieve the final Message
  recieveMessage(socket_desc, server_message, sizeof(server_message));
  if(strcmp(server_message, "ERROR TO\n") == 0){
	  printf("We got TO'ed. Closing connection\n");
	  close(socket_desc);
	  return -1;
  }
  //Close socket and quit program
  //TODO
  close(socket_desc);
  return 0;
}
