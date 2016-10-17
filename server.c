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
#define LISTENQ 1000 
#define SERV_PORT 3000

#define MAXCOLSIZE 100
#define HTTPREQ 	30


#define MAXBUFSIZE 600
#define MAXPACKSIZE 200
#define ERRORMSGSIZE 1000
#define MAXCOMMANDSIZE 100




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


//Time set for Time out 
struct timeval timeout={0,100000};     

//fixed Root , take from configuration file 
char * ROOT = "/home/chinmay/Desktop/5273/PA2/www";



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
		//splitop[0] = temp_str;

		//allocate size of each string 
		//copy the token tp each string 
		//bzero(splitop[sizeofip],strlen(p));
		memset(splitop[sizeofip],0,sizeof(splitop[sizeofip]));
		strncpy(splitop[sizeofip],p,strlen(p));
		strcat(splitop[sizeofip],"\0");
		//DEBUG_PRINT("%d %s\n",sizeofip,splitop[sizeofip]);
		sizeofip++;

		//get next token 
		p=strtok(NULL,delimiter);
		
	}

	if (sizeofip<maxattr)
	{
		DEBUG_PRINT("unsuccessful split");
		return -1;
	}

	

	return sizeofip;//Done split and return successful 
}

int error_response(char *err_message,char Http_URL[MAXCOLSIZE],int sock,char Http_Version[MAXCOLSIZE],char reason[MAXCOLSIZE]){

	char error_message[ERRORMSGSIZE];
	
	//write(sock,"Check data",strlen("Check data\n"));
	bzero(error_message,strlen(error_message));
	
	DEBUG_PRINT("%s",err_message);
	
	sprintf(error_message,"%s %s\r\n\n<head>\r\n<title>%s</title>\n\r</head>\n\r<body>\n\r<h1>%s</h1>\n\r<b>Reason :</b>	<font color=\"red\"> %s :<font color=\"blue\">%s\n\r</body>\n\r</html>\n\r",Http_Version, err_message,err_message,err_message,reason,Http_URL);
	
	DEBUG_PRINT("%s",error_message);
	
	write(sock,error_message,strlen(error_message));
	
	return 0;
}

int responsetoClient(char (*request)[MAXCOLSIZE],int thread_sock){

		char response_message[MAXBUFSIZE];//store message from client// 
		char path[MAXPACKSIZE],copypath[MAXPACKSIZE];
		int filedesc=0,filesize=0;
		ssize_t send_bytes=0,total_size=0;
		char sendData[MAXBUFSIZE];
		struct stat *file_stats=NULL;
		char file_type[MAXCOLSIZE];
		int total_attr_commands=0,i=0;
		char *p=NULL;
		char *lastptr=NULL;
		//		
		//check for Method
		if(!strcmp(request[HttpMethod],"GET")){//if first element 
			DEBUG_PRINT("GET Method implemented");
		}
		else{
			error_response("400 Bad Request",request[HttpURL],thread_sock,request[HttpVersion],"Invalid Method");
			DEBUG_PRINT("Method isn't implemented");
			return -1;
		}	
		//check for version 
		if (!strncmp(request[HttpVersion],"HTTP/1.1",8)){//if first element 
			DEBUG_PRINT("Got HTTP 1.1");
			
		}
		else if(!strncmp(request[HttpVersion],"HTTP/1.0",8)){
			DEBUG_PRINT("Got HTTP 1.0");
			
		}
		else
		{
			error_response("400 Bad Request",request[HttpURL],thread_sock,request[HttpVersion],"Invalid HTTP-Version");
			DEBUG_PRINT("Method isn't implemented");
			return -1;
		}		
		
		//read the defaut index file 
		memset(path,0,sizeof(path));
		
		//**acquire root path ** left 
		strcpy(path,ROOT);
		DEBUG_PRINT("Path before request:%s",path);
		DEBUG_PRINT("request URL:%s",request[HttpURL]);
		
		
		//Concate root path with requested URL 
		
		//check if no path/default location 		
		if(!(strncmp(request[HttpURL], "/\0", 2)))  {
			strcat(path,"/index.html");			
		}
		else{
			//Concate root path with requested URL 
			strcat(path,request[HttpURL]);			
		}
		DEBUG_PRINT("Path after request%s   ",path);


		memset(sendData,0,sizeof(sendData));
		memset(response_message,0,strlen(response_message));
		//size of file 
		if ((filedesc=open(path,O_RDONLY))<1){//if File  not found 
			
			error_response("404 Not Found",request[HttpURL],thread_sock,request[HttpVersion],"URL Does not Exist");
			perror("File not Found");
			return -1;
		}
		
		else
		{
			//send OK status 
			memset(response_message,0,strlen(response_message));
			
			//send success message
			memset(response_message,0,strlen(response_message));
			strcpy(response_message,"HTTP/1.1 200 OK\r\n");
			printf("%s",response_message);
			write(thread_sock,response_message,strlen(response_message));		
			//DEBUG_PRINT("%s",response_message);

			//for content type 
			memset(response_message,0,strlen(response_message));
			strcpy(copypath,path);
			p=strtok(copypath,".");
			if(p!=NULL)
			{
				//strncpy(file_type[0],p,strlen(p));
				//DEBUG_PRINT("1st %s",file_type[0]);
				//p=strtok(NULL,".");
				//strncpy(file_type[1],p,strlen(p));
				//DEBUG_PRINT("type %s",file_type[1]);
				DEBUG_PRINT("path seached%s",path);
				lastptr = strrchr(path,'.');
				if(lastptr){
					DEBUG_PRINT("last occurence %s",lastptr);
					*lastptr++;
					
					strcpy(file_type,lastptr);
					DEBUG_PRINT	("format %s \n",file_type);
				}
				else
				{
					DEBUG_PRINT("COuld not find '.' ");
				}

				if(!strcmp(file_type,"html")){
					//strcpy(response_message,"Content-Type: ");
					//strcat(response_message,file_type[1]); 
					sprintf(response_message,"Content-Type: text/html\r\n");
				}
				else if (!strcmp(file_type,"png")){
					sprintf(response_message,"Content-Type: image/png\r\n");

				}
				else if (!strcmp(file_type,"jpg")){
					sprintf(response_message,"Content-Type: image/jpeg\r\n");

				}
				else if (!strcmp(file_type,"gif")){
					sprintf(response_message,"Content-Type: image/gif\r\n");

				}
				else if (!strcmp(file_type,"jpeg")){
					sprintf(response_message,"Content-Type: image/jpeg\r\n");

				}
				else if (!strcmp(file_type,"htm")){
					sprintf(response_message,"Content-Type: text/html\r\n");

				}
				else if (!strcmp(file_type,"txt")){
					sprintf(response_message,"Content-Type: text/plain\r\n");

				}
				else if (!strcmp(file_type,"css")){
					sprintf(response_message,"Content-Type: text/css\r\n");

				}
				else if (!strcmp(file_type,"js")){
					sprintf(response_message,"Content-Type: text/javascript\r\n");

				}
				else
				{
					//sprintf(response_message,"Content-Type:	application/octet-stream\r\n");
					//need to add type 
					error_response("501 Not Implemented",file_type,thread_sock,request[HttpVersion],"File type Not Implemented");
					DEBUG_PRINT("File type is not implemented");
					return -1;
				}


				printf("%s",response_message);
				write(thread_sock,response_message,strlen(response_message));			

			}
			else
			{
				DEBUG_PRINT("Unable to split...exit ");
				error_response("500 Internal Server Error",request[HttpURL],thread_sock,request[HttpVersion],"Invalid File Name");
				return -1;
			}
			
			
			//for content lenght 
			file_stats = malloc(sizeof(struct stat));
			memset(file_stats,0,sizeof(file_stats));
			stat(path, file_stats);
			filesize = file_stats->st_size;
			//DEBUG_PRINT("File size %d",filesize);
			memset(response_message,0,strlen(response_message));
			sprintf(response_message,"Content-Length: %d\r\n\n",(int)filesize);					
			printf("%s",response_message);
			write(thread_sock,response_message,strlen(response_message));		

			DEBUG_PRINT("%s",response_message);

			bzero(file_stats,sizeof(bzero));
			free(file_stats);
						
			DEBUG_PRINT("Reading and send File data");
			
			memset(sendData,0,sizeof(sendData));
			//send data 

			while((send_bytes=read(filedesc,sendData,MAXBUFSIZE))>0){
				DEBUG_PRINT("%s\n",sendData); 
				total_size += send_bytes;
				send(thread_sock,sendData,send_bytes,0);		
				memset(sendData,0,sizeof(sendData));
			}
			DEBUG_PRINT("Total size read %d",(int)total_size);
			memset(response_message,0,strlen(response_message));
			strcpy(response_message,"\nCompleted\r\n");
			printf("%s",response_message);
			//write(thread_sock,response_message,strlen(response_message));		

			close(filedesc);//close the file opened 
		}
	


		return 0;
}

//Client connection for each client 

void *client_connections(void *client_sock_id)
{
	
	
	int total_attr_commands=0,i=0;
	int thread_sock = (int*)(client_sock_id);
	ssize_t read_bytes=0;
	char message_client[MAXPACKSIZE];//store message from client 
	char (*split_attr)[MAXCOLSIZE];
	DEBUG_PRINT("passed Client connection %d\n",(int)thread_sock);

	

	

		// Recieve the message from client  and reurn back to client 
		if((read_bytes =recv(thread_sock,message_client,MAXPACKSIZE,0))>0){

			DEBUG_PRINT("request from client %s\n",message_client );
			
			DEBUG_PRINT("Message length%d\n",(int)strlen(message_client) );
			
			if ((split_attr=malloc(sizeof(split_attr)*MAXCOLSIZE))){	
				strcpy(split_attr[HttpVersion],"HTTP/1.1");//Default
				strcpy(split_attr[HttpMethod],"GET");//Default
				strcpy(split_attr[HttpURL],"index.html");//No data 
	
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
					
					bzero(message_client,sizeof(message_client));	
					bzero(split_attr,sizeof(split_attr));				
					DEBUG_PRINT("Cannot split input request");
				}
				//print the split value 
				
				for(i=0;i<total_attr_commands;i++){
					DEBUG_PRINT("%d %s\n",i,split_attr[i]);
				}
				
				
				responsetoClient(split_attr,thread_sock);
							
				//free alloaction of memory 
				for(i=0;i<total_attr_commands;i++){
					free((*split_attr)[i]);
				}
				
				free(split_attr);//clear  the request recieved 
						
			}
			else 
			{
					error_response("500 Internal Server Error",split_attr[HttpURL],thread_sock,split_attr[HttpVersion],"Invalid File Name");
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
		DEBUG_PRINT("Freed");

	}	
	if (client_sock < 0)
	{
		perror("Accept Failure");
		close(server_sock);
		exit(-1);
	}
		


	close(server_sock);
	


}
		

