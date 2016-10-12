/*******************************************************************************
Web Server implementation 
Author :Chinmay Shah 
File :server.c
Last Edit : 10/10
******************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <sys/time.h>
#include <pthread.h> // For threading , require change in Makefile -lpthread
//#include <time.h>

/* You will have to modify the program below */
#define LISTENQ 3 
#define SERV_PORT 3000

#define MAXCOLSIZE 100





#define MAXBUFSIZE 60000
#define MAXPACKSIZE 2000
#define MAXCOMMANDSIZE 100
#define PACKETNO 7
#define SIZEMESSAGE 7
#define ACKMESSAGE 4+PACKETNO
#define MAXFRAMESIZE (PACKETNO+SIZEMESSAGE+ MAXPACKSIZE)
#define MAXREPEATCOUNT 3
#define RELIABILITY
#define ERRMESSAGE 20


typedef char type2D[10][MAXCOMMANDSIZE];

typedef enum TYPEACK{NOACK,ACK,TIMEDACK}ACK_TYPE;


int server_sock,client_sock;                           //This will be our socket
struct sockaddr_in server, client;     //"Internet socket address structure"
unsigned int remote_length;         //length of the sockaddr_in structure
int nbytes;                        //number of bytes we receive in our message
char ack_message[ACKMESSAGE];

//Time set for Time out 
struct timeval timeout={0,100000};      
/*************************************************************
//Split string on basis of delimiter 
//Assumtion is string is ended by a null character
I/p : splitip - Input string to be parsed 
	  delimiter - delimiter used for parsing 
o/p : splitop - Parsed 2 D array of strings
	  return number of strings parsed 

Referred as previous code limits number of strings parsed 	  
http://stackoverflow.com/questions/20174965/split-a-string-and-store-into-an-array-of-strings
**************************************************************/
int splitString(char *splitip,char *delimiter,char (*splitop)[100],int maxattr)
{
	printf("In Split\n");

	int sizeofip=0,i=0;
	char *p=NULL;//token
	char *temp_str = NULL;
	
	if(splitip==NULL || delimiter==NULL){
		printf("Error\n");
		return -1;//return -1 on error 
	}
	
	
	p=strtok(splitip,delimiter);//first token string 
	
	//Check other token
	while(p!=NULL && p!="\n" && sizeofip<maxattr )
	{
		
		
		temp_str = realloc(*splitop,sizeof(char *)*(sizeofip +1));
		
		if(temp_str == NULL){//if reallocation failed	

			
			//as previous failed , need to free already allocated 
			if(*splitop !=NULL ){
				for (i=0;i<sizeofip;i++)
					free(splitop[i]);
				free(*splitop);	
			}

			return -1;//return -1 on error 
		}
		
		
		//Token Used
		strcat(p,"\0");
		// Set the split o/p pointer 
		splitop = temp_str;

		//allocate size of each string 
		//copy the token tp each string 
		strncpy(splitop[sizeofip],p,strlen(p));
		printf("%s\n",splitop[sizeofip]);
		sizeofip++;

		//get next token 
		p=strtok(NULL,delimiter);
		
	}
	

	return sizeofip;//Done split and return successful 
}


//Client connection for each client 

void *client_connections(void *client_sock_id)
{
	
	
	int total_attr_commands=0,i=0;

	//Obtain the socket desc
	int thread_sock = (int)client_sock_id;
	
	ssize_t read_bytes=0;
	//printf("Inside client_connections 3\n");
	char message_client[MAXPACKSIZE];//store message from client 
	
	char (*split_attr)[MAXCOLSIZE];
	//char *message ; // message to client 
	printf("passed Client connection %d\n",(int)client_sock_id);
	
	

		// Recieve the message from client  and reurn back to client 
		while((read_bytes =recv(thread_sock,message_client,MAXPACKSIZE,0))>0){

			printf("%s\n",message_client );
			//write(thread_sock,message_client,strlen(message_client));
			printf("Message length%d\n",strlen(message_client) );
			if ((split_attr=malloc(sizeof(split_attr)*MAXCOLSIZE*strlen(message_client)))){	
		
				if((total_attr_commands=splitString(message_client," ",split_attr,3))<0)
				{
					printf("Error in split\n");

					//printf("%s\n", );
					bzero(message_client,sizeof(message_client));	
					bzero(split_attr,sizeof(split_attr));	
					return NULL;
				}
				else
				{
					//printf("%s\n", );
					bzero(message_client,sizeof(message_client));	
					bzero(split_attr,sizeof(split_attr));				
				}		
			}
			else 
			{
					perror("alloacte 2d pointer");
					exit(-1);
			}		

		}
		if (read_bytes < 0){
			perror("recv from client failed ");
			return NULL;

		}
		//free alloaction of memory 
		for(i=0;i<total_attr_commands;i++){
			free((*split_attr)[i]);
		}
	free(split_attr);
	printf("Completed \n");

	//free(client_sock_id);//free the memory 
	return 1 ;
	
}


int main (int argc, char * argv[] ){
	char request[MAXPACKSIZE];             //a request to store our received message
	
	int *mult_sock=NULL;//to alloacte the client socket descriptor


	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&server,sizeof(server));                    //zero the struct
	server.sin_family = AF_INET;                   //address family
	//server.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	server.sin_port = htons(SERV_PORT);        		//htons() sets the port # to network byte order
	server.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine
	remote_length = sizeof(struct sockaddr_in);    //size of client packet 
	pthread_t client_thread;
	//Causes the system to create a generic socket of type TCP (strean)
	if ((server_sock =socket(AF_INET,SOCK_STREAM,0)) < 0){
		printf("unable to create tcp socket");
		exit(-1);
	}
	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0){
		close(server_sock);
		printf("unable to bind socket\n");
		exit(-1);
	}
	//

	if (listen(server_sock,LISTENQ)<0)
	{
		close(server_sock);
		perror("LISTEN");
		exit(-1);
	}


	printf("Server is running wait for connections \n");

	//Accept incoming connections 
	while((client_sock = accept(server_sock,(struct sockaddr *) &client, (socklen_t *)&remote_length))){
		if(client_sock<0){	
			perror("accept  request failed");
			exit(-1);
			close(server_sock);
		}
		printf("connection accepted  %d \n",(int)client_sock);	
		mult_sock = (int *)malloc(1);
		if (mult_sock== NULL)//allocate a space of 1 
		{
			perror("Malloc mult_sock unsuccessful");
			close(server_sock);
			exit(-1);
		}
		printf("Malloc successfully\n");
		//bzero(mult_sock,sizeof(mult_sock));
		*mult_sock = client_sock;

		printf("connection accepted  %d \n",*mult_sock);	
		//Create the pthread 
		if ((pthread_create(&client_thread,NULL,client_connections,(void *)(*mult_sock)))<0){
			close(server_sock);
			perror("Thread not created");
			exit(-1);

		}		

		
		
		if(pthread_join(client_thread, NULL) == 0)
		 printf("Client Thread done\n");
		else
		 perror("Client Thread");
		free(mult_sock);

	}	
	if (client_sock < 0)
	{
		perror("Accept Failure");
		close(server_sock);
		exit(-1);
	}
		


	close(server_sock);
	


}
		


	
	/*
	//printf("Socket created and binded \n");
	char msg[] = "ok";
	type2D *action;
	char temp[10][MAXCOMMANDSIZE];
	action = &temp;
	strcpy(**action,"ls ");
	char request[MAXCOMMANDSIZE];

	int total_attr_commands;
	int garbage_count=0;
	int check_bytes=2;
	

	while (1)
	{
		
		
		action = &temp;
		bzero(request,sizeof(request));
		bzero(action,sizeof(action));	
		
		//waits for an incoming message
		nbytes = recv(server_sock,request,sizeof(request),0,(struct sockaddr*)&server,&remote_length);
		if ((strlen(request)>0) && (request[strlen(request)-1]=='\n')){
				request[strlen(request)-1]='\0';
		}
		//memcpy(request,request,sizeof(request)-1);
		
		if(garbage_count)
		{
			check_bytes=0;
		}
		sendtoClient(ack_message,sizeof(ack_message),NOACK);
		//Messaged received successfully
		printf("\nRxcv request : %s bytes %d \n",request,nbytes );
		



		strcat(request,"\0");
		if (nbytes >0){
				total_attr_commands=splitString(request," ",action);	
					if(total_attr_commands>=0)
					{
							//printf("request not supported\n");
							
						int count=0;
						action = &temp;
						garbage_count++;
						//printf("Action : %s\n",**action );
						//Check type of request
						if (strcmp(**action,"ls")==0){							
									**action++;//gr
									//printf("value after ls :%s\n", **action);
									if (strlen(**action)>check_bytes){
									//	printf("request not supported\n");
										sendtoClient("request not supported",sizeof("request not supported"),NOACK);							
									}	
									else if(list()<0){
									//		printf("Error in listing!!!\n");
											sendtoClient("Error",sizeof("Error"),NOACK);
										}		
								}
						else if (strcmp(**action,"get")==0){
									//printf("in get\n");
									**action++;//gr
									if(sendFile(**action) <0){	
										//printf("Error in get!!!\n");
										sendtoClient("Error",sizeof("Error"),NOACK);
									}
								}
						else if (strcmp(**action,"put")==0){
									//printf("in put\n");
									**action++;
									//printf("File %s\n",**action);
									if(rcvFile(**action) <0){	
										//printf("Error in put!!!\n");
										//sendtoClient("Error");
									}
								}
						else if (strcmp(**action,"exit")==0){
									printf("Exiting server .........\n");
									close(server_sock);
									break;
								}
						else 	{
									sendtoClient(request,sizeof(request),NOACK);
									//printf("request not supported\n");
								}
						//clearing the split value 		
						bzero(action,sizeof(action));		
					}	
						
		}

		//sendtoClient(msg);
	}
		
	//close the socket 
	close(server_sock);

}
	*/