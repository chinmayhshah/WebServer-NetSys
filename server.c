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
#define LISTENQ 100 
#define SERV_PORT 3000

#define MAXCOLSIZE 100
#define HTTPREQ 	3






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



#define DEBUGLEVEL

#ifdef DEBUGLEVEL
	#define DEBUG 1
#else
	#define	DEBUG 0
#endif

#define DEBUG_PRINT(fmt, args...) \
        do { if (DEBUG) fprintf(stderr, "\n %s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __FUNCTION__, ##args); } while (0)


typedef char type2D[10][MAXCOMMANDSIZE];

typedef enum HTTPFORMAT{
							HttpExtra,//Extra Character
							HttpMethod,//Resource Method
							HttpURL,// Resource URL
							HttpVersion //Resource Version 
						}HTTP_FM;// Resource format


//typedef enum HTTPFORMAT{RM,RU,RV}HTTP_FM;


int server_sock,client_sock;                           //This will be our socket
struct sockaddr_in server, client;     //"Internet socket address structure"
unsigned int remote_length;         //length of the sockaddr_in structure
int nbytes;                        //number of bytes we receive in our message
char ack_message[ACKMESSAGE];

//Time set for Time out 
struct timeval timeout={0,100000};     

//fixed Root , take from configuration file 
char * ROOT = "/home/chinmay/Desktop/5273/PA2/www";


int responsetoClient(char (*request)[MAXCOLSIZE],int thread_sock){

		char response_message[MAXBUFSIZE];//store message from client// 
		char path[MAXPACKSIZE];
		int filedesc;
		ssize_t send_bytes;
		char sendData[MAXBUFSIZE];
		//		
		//check for Method
		if(!strcmp(request[HttpMethod],"GET")){//if first element 
			DEBUG_PRINT("Got GET");
		}
		else{
					DEBUG_PRINT("Not working");
		}	
		//check for version 
		if (!strncmp(request[HttpVersion],"HTTP/1.1",8)){//if first element 
			DEBUG_PRINT("Got HTTP");
		}
		else
		{
			DEBUG_PRINT("Not working %s",request[HttpVersion]);
		}		
		//strcpy(response_message,"HTTP/1.1 200 OK\n\n");
		//strcpy(response_message,"HTTP/1.1 404 Not Found\n<html><body>404	Not	Found	Reason	URL	does not	exist:<<requested url>></body></html>\n\n\n");
		//DEBUG_PRINT("%s socket %d",response_message,thread_sock);
		//send OK Status to client 
		//write(thread_sock,response_message,strlen(response_message));
		
		//read the defaut index file 
		memset(path,0,sizeof(path));
		//DEBUG_PRINT("Root %s  ",ROOT);
		strcpy(path,ROOT);
		//strcat(path,"/index.html");
		strcat(path,request[HttpURL]);
		memset(sendData,0,sizeof(sendData));
		memset(response_message,0,sizeof(response_message));
		//strcpy(&path[strlen(ROOT)],"/index.html");//adjust the path , pick from config  file 
		DEBUG_PRINT("Path %s   ",path);

		//Open the file ,read and send the files 
		//strcpy(response_message,"HTTP/1.1 404 Not Found\n<html><body>404	Not	Found	Reason	URL	does	not	exist	:<<requested url>></body></html>\n\n");
		//DEBUG_PRINT("%s",response_message);
		//write(thread_sock,response_message,strlen(response_message));	
		
		if ((filedesc=open(path,O_RDONLY))<1){//if File  not Found 
			perror("File not Found");
			strcpy(response_message,"HTTP/1.1 404 Not Found\n<html><body>404	Not	Found	Reason	URL	does	not	exist	:<<requested url>></body></html>\n\n");
			write(thread_sock,response_message,strlen(response_message));	
			DEBUG_PRINT("%s",response_message);
		}
		
		else
		{
			//send OK status 
			DEBUG_PRINT("Reading File");
			//send success message
			strcpy(response_message,"HTTP/1.1 200 OK\n\n");
			write(thread_sock,response_message,sizeof(response_message));		
			//send data 
			while((send_bytes=read(filedesc,sendData,MAXBUFSIZE))>0){
				send(thread_sock,sendData,send_bytes,0);		
			}
		}
		
		


		return 0;
}













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
int splitString(char *splitip,char *delimiter,char (*splitop)[MAXCOLSIZE],int maxattr)
{
	int sizeofip=1,i=1;
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
		//bzero(splitop[sizeofip],strlen(p));
		memset(splitop[sizeofip],0,sizeof(splitop[sizeofip]));
		strncpy(splitop[sizeofip],p,strlen(p));
		strcat(splitop[sizeofip],"\0");
		DEBUG_PRINT("%d %s\n",sizeofip,splitop[sizeofip]);
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
	DEBUG_PRINT("passed Client connection %d\n",(int)client_sock_id);

	

		// Recieve the message from client  and reurn back to client 
		if((read_bytes =recv(thread_sock,message_client,MAXPACKSIZE,0))>0){

			printf("%s\n",message_client );
			
			DEBUG_PRINT("Message length%d\n",strlen(message_client) );
			
			if ((split_attr=malloc(sizeof(split_attr)*MAXCOLSIZE))){	
				//bzero(split_attr,sizeof(split_attr));
				//bzero(message_client,sizeof(message_client));
		
				if((total_attr_commands=splitString(message_client," ",split_attr,4))<0)
				{
					DEBUG_PRINT("Error in split\n");

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
				//print the split value 
				
				for(i=0;i<total_attr_commands;i++){
					printf("%d %s\n",i,split_attr[i]);
				}
				printf("%s\n",split_attr[0] );
				responsetoClient(split_attr,thread_sock);
							
				//free alloaction of memory 
				for(i=0;i<total_attr_commands;i++){
					free((*split_attr)[i]);
				}
				
				free(split_attr);//clear  the request recieved 
						
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


		
	DEBUG_PRINT("Completed \n");
	//Closing SOCKET
    shutdown (thread_sock, SHUT_RDWR);         //All further send and recieve operations are DISABLED...
    close(thread_sock);
    thread_sock=-1;
	//free(thread_sock);//free the connection 

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
		DEBUG_PRINT("unable to create tcp socket");
		exit(-1);
	}
	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0){
		close(server_sock);
		DEBUG_PRINT("unable to bind socket\n");
		exit(-1);
	}
	//

	if (listen(server_sock,LISTENQ)<0)
	{
		close(server_sock);
		perror("LISTEN");
		exit(-1);
	}


	DEBUG_PRINT("Server is running wait for connections");

	//Accept incoming connections 
	while((client_sock = accept(server_sock,(struct sockaddr *) &client, (socklen_t *)&remote_length))){
		if(client_sock<0){	
			perror("accept  request failed");
			exit(-1);
			close(server_sock);
		}
		DEBUG_PRINT("connection accepted  %d \n",(int)client_sock);	
		mult_sock = (int *)malloc(1);
		if (mult_sock== NULL)//allocate a space of 1 
		{
			perror("Malloc mult_sock unsuccessful");
			close(server_sock);
			exit(-1);
		}
		DEBUG_PRINT("Malloc successfully\n");
		//bzero(mult_sock,sizeof(mult_sock));
		*mult_sock = client_sock;

		DEBUG_PRINT("connection accepted  %d \n",*mult_sock);	
		//Create the pthread 
		if ((pthread_create(&client_thread,NULL,client_connections,(void *)(*mult_sock)))<0){
			close(server_sock);
			perror("Thread not created");
			exit(-1);

		}				
		/*
		//as it does  have to wait for it to join thread ,
		//does not allow multiple connections 
		if(pthread_join(client_thread, NULL) == 0)
		 printf("Client Thread done\n");
		else
		 perror("Client Thread");
		 */
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
		

