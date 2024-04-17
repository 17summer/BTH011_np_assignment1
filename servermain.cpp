#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
/* You will to add includes here */
#include <math.h>
#include <ctime>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
// Included to get the support library
#include "calcLib.h"

#define MAX_CLIENTS 5
#define TIME_OUT 5
#define PROTOCOL_STRING "TEXT TCP 1.0\n"
#define MAXSZ 1024

//record the quantities of clients that has connected
static int currClientsLen = 0;

//to mark the number of client
static int childCnt = 0;

// a mutex ensures that only one thread can access the shared resource at a time
pthread_mutex_t clientMutex;

//to define a clientInfo, which represents the socketNo of client and its childno.
typedef struct clientInfo
{
	int fd;
	int clientNo;

}clientInfo;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//used to compare right answer with result from client
bool compareResultDouble(char* op,double f1,double f2,double res)
{
	double fresult;
	
	if(strcmp(op,"fadd")==0){
		fresult=f1+f2;
	} else if (strcmp(op, "fsub")==0){
		fresult=f1-f2;
	} else if (strcmp(op, "fmul")==0){
		fresult=f1*f2;
	} else if (strcmp(op, "fdiv")==0){
		fresult=f1/f2;
	}
	
	printf("Correct Result: %8.8g\n",res);
	if(fabs(fresult-res) < 0.0001)
	{
		return true;	
	}else
	{
		return false;
	}
}

//used to compare right answer with result from client
bool compareResultInteger(char* op, int i1,int i2,int res)
{
	int iresult;
	if(strcmp(op,"add")==0){
		iresult=i1+i2;
	} else if (strcmp(op, "sub")==0){
		iresult=i1-i2;
	} else if (strcmp(op, "mul")==0){
		iresult=i1*i2;
	} else if (strcmp(op, "div")==0){
		iresult=i1/i2;
	}
	printf("Result: %d\n",iresult);
	if(iresult == res)
	{
		return true;
	}else
	{
		return false;
	}	
	
}

//function to handle each client communication.
void* handleClient(void* clientfd)
{
	//operand and results
  	double fv1,fv2,fresult;
	int iv1,iv2,iresult;

	//the char array to store the message send and receive between client and server.
	char msg[1450];

	//to get the data that passed from main
	clientInfo* info = (clientInfo*) clientfd;
	
	// Create a set of file descriptors for select
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(info->fd, &readfds);
	//set the time for timeout
	struct timeval timeout;
	timeout.tv_sec = TIME_OUT;
	timeout.tv_usec = 0;
	
	// Wait for activity on client's socket using select
	int activity = select(info->fd + 1, &readfds, NULL, NULL, &timeout);
	//select error 
	if (activity == -1) {
		perror("Select error\n");
	} 
	//if the cilent occurs timeout, do the fellow operations
	else if (activity == 0) {
		printf("Client[%d] is timeout......\n\n",info->clientNo);
		if(send(info->fd,"ERROR TO\n",9,0)==-1)
		{
			perror("send");
		}
		close(info->fd);
		pthread_mutex_lock(&clientMutex);
		currClientsLen--;
		pthread_mutex_unlock(&clientMutex);
		return NULL;
	} 
	//normal operations
	else {
		timeout.tv_sec = TIME_OUT;
		timeout.tv_usec = 0;
		if (setsockopt(info->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
				perror("setsockopt timeout");
			}
		
		//clear the message
		memset(msg,0,sizeof(msg));
		
		//receive meg from client
		recv(info->fd,&msg,MAXSZ,0);
		
		// If message is not OK, disconnect client
		if(strcmp(msg,"OK\n") != 0)
		{
			printf("Wrong message. Client[%d] disconnected.\n\n",info->clientNo);
			close(info->fd);
			pthread_mutex_lock(&clientMutex);
			currClientsLen--;
			pthread_mutex_unlock(&clientMutex);
			return NULL;
		}
		
		
		
		// Generate random operation
		char* op = randomType();
					
		memset(msg,0,sizeof(msg));
		
		// Prepare message with operands
		if(op[0] == 'f')
		{
			fv1 = randomFloat();
			fv2 = randomFloat();
			sprintf(msg,"%s %8.8g %8.8g\n",op,fv1,fv2);
		}else
		{
			iv1 = randomInt();
			iv2 = randomInt();
			sprintf(msg,"%s %d %d\n",op,iv1,iv2);
		}
		

		
		
		//send the formulation to client
		send(info->fd,&msg,strlen(msg),0);
		
		printf("Client[%d]'s formulation: %s",info->clientNo,msg);
		
		memset(msg,0,sizeof(msg));


		
		int recv_b = recv(info->fd,&msg,MAXSZ,0);
		
		
		
		//timeout
		if (recv_b == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
			    	printf("The client[%d] is timeout......\n\n",info->clientNo);
				if(send(info->fd,"ERROR TO\n",9,0)==-1)
				{
					perror("send");
				}
				close(info->fd);
				pthread_mutex_lock(&clientMutex);
				currClientsLen--;
				pthread_mutex_unlock(&clientMutex);
				return NULL;
				
			} else {
			    perror("recv error.\n");
			}
		}

		//compare the correct answer with client's answer
		if(op[0] == 'f')
		{
			sscanf(msg,"%lf",&fresult);
			printf("Client Result:%8.8g\n",fresult);
			if(compareResultDouble(op,fv1,fv2,fresult))
			{
				if(send(info->fd,"OK\n",3,0) == -1)
				{
					perror("Error in sending message");
				}
				printf("Clinet[%d] answer: Right.\n",info->clientNo);
			}else
			{
				if(send(info->fd,"ERROR\n",6,0) == -1)
				{
					perror("Error in sending message");
				}
				printf("Clinet[%d] answer: Wrong.\n",info->clientNo);
			}
		}else
		{
		
			sscanf(msg,"%d",&iresult);
			
			printf("Client Result:%d\n",iresult);
			if(compareResultInteger(op,iv1,iv2,iresult))
			{
				if(send(info->fd,"OK\n",3,0) == -1)
				{
					perror("Error in sending message");
				}
				printf("Clinet[%d] answer: Right.\n",info->clientNo);
			}else
			{
				if(send(info->fd,"ERROR\n",6,0) == -1)
				{
					perror("Error in sending message");
				}
				printf("Clinet[%d] answer: Wrong.\n",info->clientNo);
			}
		}

	}

	//close the client after finishing
	printf("client[%d] finished......\n\n",info->clientNo);
	close(info->fd);
	pthread_mutex_lock(&clientMutex);
	currClientsLen--;
	pthread_mutex_unlock(&clientMutex);

}


int main(int argc, char *argv[]) {
    	initCalcLib();

	//judge the accuracy of input	
    	if (argc != 2) {
		fprintf(stderr, "usage: %s hostname (%d)\n", argv[0], argc);
		exit(1);
	    }
	
	//split the desthost and destport
	char *str = argv[1];
	char *Desthost = "";
	char *Destport = "";
	char *colon_pos = strrchr(str, ':');
	if (colon_pos != NULL) {
	*colon_pos = '\0';
	Desthost = str;
	Destport = colon_pos + 1;
	}

	int sockfd, new_fd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
	void *addr;
	char ipstr[INET6_ADDRSTRLEN];

	// get the address from the address struct
	if (p->ai_family == AF_INET) {
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		addr = &(ipv4->sin_addr);
		} else {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
		addr = &(ipv6->sin6_addr);
		}

		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
		    perror("server: socket");
		    continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		    perror("setsockopt");
		    exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
		    close(sockfd);
		    perror("server: bind");
		    continue;
		}
		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
		printf("Server with address[%s] and Port[%s]: Waiting for connections...\n",ipstr,Destport);
		break;
	}


	freeaddrinfo(servinfo);

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, MAX_CLIENTS) == -1) {
		perror("listen");
		exit(1);
	}


	fd_set readfds;
	int maxfd, activity;
	struct timeval timeout;


	while (1) {
		// Clear the set and add the socket descriptor
		FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);
		maxfd = sockfd;

		timeout.tv_sec = TIME_OUT;
		timeout.tv_usec = 0;

		// Call select to wait for activity on the socket
		activity = select(maxfd + 1, &readfds, NULL,NULL ,&timeout);

		if(activity <= 0 && !sockfd)
		{
			printf("Something wrong happens in select......\n");
		}	

		// New connection
		if (FD_ISSET(sockfd, &readfds)) {
			sin_size = sizeof(their_addr);
			//accept the connection from client
			if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) < 0) {
				perror("Accept error\n");
			} else {
				//use a mutex to make sure only one thread can change the shared data at a time.
				pthread_mutex_lock(&clientMutex);
				//judge whether the connections is more than MAX_CLIENT.
				if (currClientsLen < MAX_CLIENTS) {
					currClientsLen++;
					childCnt++;
					inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
						struct sockaddr_in *local_sin = (struct sockaddr_in *)&their_addr;

					//friendly screen display
					printf("Client[%d] from [%s] and port [%d] connected.\n",childCnt ,s, ntohs(local_sin->sin_port));
					
					//send the protocol to client
					if(send(new_fd,PROTOCOL_STRING,13,0) == -1)
					{
						perror("send");
					}

					//client handler thread
					pthread_mutex_unlock(&clientMutex);
					pthread_t tid;
					int fd = new_fd;
					clientInfo* info = new clientInfo;
					info->fd = fd;
					info->clientNo = childCnt;
					pthread_create(&tid,NULL,handleClient,info);
					pthread_detach(tid);
				} else {
					//too much connections.
					send(new_fd, "ERROR:Server busy, you are rejecting......\n", 44, 0);
					close(new_fd);
					printf("Server now is busy, rejecting the connection......\n\n");
					pthread_mutex_unlock(&clientMutex);
				}
			}
		}
	}
	
	pthread_mutex_destroy(&clientMutex);
    return 0;
}

