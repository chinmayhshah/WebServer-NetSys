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
	  --Does not allow more than two spaces 
**************************************************************/
int splitString(char *splitip,char *delimiter,type2D *splitop)
{

	int sizeofip=strlen(splitip);
	int totalstr=0;
	char *p;//token
	
	p=strtok(splitip,delimiter);//first token string 
	
	//Check other token
	while(p!=NULL&&totalstr <=3)
	{
		strcpy(**splitop++,p);
		p=strtok(NULL,delimiter);
		//Token Used
		totalstr++;
		if(totalstr >=3)
		{
			return -1;
		}
	}
	
	
	return totalstr;
}

/*************************************************************************************
Calculate MD5 and return value as string 
Assumtion : File is in same directory 
i/p - Filename
o/p - MD5 Hash value 
Ref for coding :http://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c?newreg=957f1b2b2132420fb1b4783484823624
Library :http://stackoverflow.com/questions/14295980/md5-reference-error
		gcc server.c -o server -lcrypto -lssl
***************************************************************************************/
int MD5Cal(char *filename, char *MD5_result)
{
	//unsigned char *MD5_gen;
	unsigned char MD5_gen[MD5_DIGEST_LENGTH];
	MD5_CTX mdCont;//
	int file_bytes;//bytes read
	unsigned char tempdata[10];//store temp data from file 
	bzero(tempdata,sizeof(tempdata));
	unsigned char temp;
	int i=0;

	FILE *fd = fopen(filename,"rb");
	if (fd ==NULL)
	{
		perror(filename);//if can't open
		return -1;
	}

	MD5_Init(&mdCont);//Initialize 
	
	while((file_bytes = fread(tempdata,1,10,fd)) != 0){//Read data from files
		MD5_Update(&mdCont,tempdata,file_bytes);//Convert and update context 		
	}	
	MD5_Final(MD5_gen,&mdCont);
	fclose(fd);	

	for ( i=0;i<MD5_DIGEST_LENGTH;i++) {
		temp = MD5_gen[i];
		sprintf(MD5_result,"%x",((MD5_gen[i]&0xF0)>>4) );
		*MD5_result++;
		sprintf(MD5_result,"%x",((MD5_gen[i]&0x0F)) );
		*MD5_result++;
		
	   }
	return 0;
}

/*************************************************************
Send message recieved to Client 
*************************************************************/
int sendtoClient(char *msg,ssize_t bytes,ACK_TYPE ack_type)
{
	ssize_t send_bytes,recv_bytes;
	int rcvd_packno=0;
	char ack[ACKMESSAGE];
	//printf("sendtoClient %s , %d\n",msg,(int)bytes );
	if ((send_bytes=sendto(server_sock,msg,bytes,0,(struct sockaddr*)&server,remote_length)) < bytes)
	{
		fprintf(stderr,"Error in sending to clinet in sendtoClient %s %d \n",strerror(errno),(int)send_bytes );
	}
	//check for ACK
	if(ack_type==ACK){	

		//Wait for Acknowledege 	   
		if (recv_bytes = recvfrom(server_sock,ack,sizeof(ack),0,(struct sockaddr*)&server,&remote_length)<0){
			//printf("ACK not recieved \n");//Not ACk Packet 
		}
		else{
				//printf("%s\n",ack );
			
			if (!strstr(ack_message,ack))
			{
				rcvd_packno=strtol(&ack[3],NULL,10);	//Parse the ack no rcvd 
			}
			else
			{
				//printf("ACK not recieved %s\n",ack);		//ACK not recieved 
				return -1;
			}		
		}
	}	
	return rcvd_packno	;

}
// To close socket and exit server
void exitServer(){
	printf("in exit Server\n");
	//close the socket 
	sendtoClient("ok",sizeof("ok"),NOACK);
	close(server_sock);
}

/*************************************************************
List the files on local directory and send file details to server 
o/p : list of files in present directory posted to client 
Ref:http://www.lemoda.net/c/list-directory/
**************************************************************/
int list(){
	DIR * d;
	char * directory = "."; // by default present directory
	char * listtosend ;
	//open directory 
	d = opendir (directory);
	if (!d){
		fprintf(stderr,"Error in opening directory %s\n",strerror(errno));
		sendtoClient(strerror(errno),(ssize_t)sizeof(strerror(errno)),NOACK);
		return -1;
	}

	listtosend = (char *)calloc(MAXBUFSIZE,sizeof(char));
	//memset(listtosend,NULL,MAXBUFSIZE);
	if (listtosend <0 ){
		fprintf(stderr,"Error in allocating memory %s\n",strerror(errno));
		return -1;
	}
	while(1){
		struct  dirent *listfiles;
		//ref: http://nxmnpg.lemoda.net/3/readdir
		listfiles= readdir(d);
		if (!listfiles){
			break;
		}
		
		if(!(strcmp(listfiles->d_name,".") == 0 || strcmp(listfiles->d_name,"..") ==0)){//excluding present and previous dir
			strcat(listtosend,listfiles->d_name);
			strcat(listtosend,"\t");
		}
	}
	//close the directory 
	if(closedir(d)){
		fprintf(stderr,"Error in closing directory %s\n",strerror(errno));
		sendtoClient(strerror(errno),(ssize_t)sizeof(strerror(errno)),NOACK);
		return -1;
	}
	//printf("%s\t",listtosend);
	ssize_t send_bytes;
	//if ((send_bytes=sendto(server_sock,msg,bytes,0,(struct sockaddr*)&server,remote_length)) < bytes)
	if ((send_bytes=sendto(server_sock,listtosend,strlen(listtosend),0,(struct sockaddr*)&server,remote_length)) < sizeof(listtosend))
	{
		fprintf(stderr,"Error in sending to clinet in lsit send %s\n",strerror(errno));
	}
	//sendtoClient(listtosend,(ssize_t)sizeof(listtosend));
	free(listtosend);
	return 0;//sucess 	
}

/*************************************************************
// Get a file from Client to Server

I/p : filename - File name to be store data in  to server 

O/p :file is rcvd from client
	 return number of status

Reference for  basic understanding :
https://lms.ksu.edu.sa/bbcswebdav/users/mdahshan/Courses/CEN463/Course-Notes/07-file_transfer_ex.pdf	  
**************************************************************/
int rcvFile (char *filename){
	
	int fd; // File decsriptor 
	ssize_t file_bytes=0;
	int file_size=0;
	int rcount=0;//count for send count 

	
	char *err_indication = "Error"; 
	char *comp_indication = "Comp"; 
	char recv_pack[MAXPACKSIZE+1];
	char packet_no[PACKETNO];
	char prev_pack_no[PACKETNO];
	char rcvd_packet_no[PACKETNO];
	char rcvd_size[SIZEMESSAGE];
	char recv_frame[MAXFRAMESIZE];
	int  rcvd_bytes;
	int  pos_data=0;
	int  pos_size=0;
	char *MD5_Client,*MD5_Server;
	MD5_Client =(char *)malloc(MD5_DIGEST_LENGTH*2*sizeof(char));
	if (MD5_Client == NULL)
	{
		perror(MD5_Client);
		return -1;

	}
	MD5_Server =(char*)malloc(MD5_DIGEST_LENGTH*2*sizeof(char));
	if (MD5_Server == NULL)
	{
		perror(MD5_Server);
		return -1;
	}
	//clear Md5
	bzero(MD5_Client,sizeof(MD5_Client));
	bzero(MD5_Server,sizeof(MD5_Server));


	file_bytes =recvfrom(server_sock,recv_frame,sizeof(recv_frame),0,(struct sockaddr*)&server,&remote_length);	
	//copy the data of file 
	
	if (strchr(recv_frame,'|')){
		//printf("file to write  %s\n",recv_pack );
	}
	else
	{
		//printf("Cant find | before loop \n");
		strcpy(recv_pack,recv_frame);
	}
	//open the file
	if (strcmp(recv_pack,err_indication)==0 && file_bytes >0 ){//check for error Message from server
				file_bytes =recvfrom(server_sock,recv_frame,sizeof(recv_frame),0,(struct sockaddr*)&server,&remote_length);	
				printf("\n%s\n",recv_pack );
				rcount =-1;
				//return -1;
			}
	else if((fd= open(filename,O_CREAT|O_RDWR|O_TRUNC,0666)) <0){//open a file to store the data in file
		perror(filename);//if can't open
		return -1;
	}
	else // open file successfully ,read file 
	{
		//debug 
		//check if any bytes read 
		do
		{
			//split packet no and data 
			if (strchr(recv_frame,'|')){
				//extract packet No
				pos_size =(int)(strchr(recv_frame,'|') - recv_frame)+1;//posintion of '|'
				
				memcpy(rcvd_packet_no,recv_frame,pos_size-1);//packet of data 
				rcvd_packet_no[pos_size+1]='\0';
				
				//extract Size of Packet 
				pos_data =(int)(strchr(&recv_frame[pos_size],'|') - recv_frame)+1;
				memcpy(rcvd_size,&recv_frame[pos_size],pos_data-(pos_size));
				rcvd_size[pos_data+1]='\0';
				
				rcvd_bytes=strtol(rcvd_size,NULL,10);

				//extract File Data
				memcpy(recv_pack,&recv_frame[pos_data],rcvd_bytes);//packet of data 

				recv_pack[rcvd_bytes]='\0';

				//printf("pos size,pos data %d %d %d\n",pos_size,pos_data,(int)sizeof(recv_frame));
				//printf("From Client Packet:Size  %s %s\n",rcvd_packet_no,rcvd_size);	
				//printf("%s\n",recv_pack );	
			}
			else
			{
				//printf("Cant find | \n");
				strcpy(recv_pack,recv_frame);
			}
			
			if (strcmp(recv_pack,comp_indication)==0)//check for File Completion
			{
				//printf("\n%s\n",recv_pack );
				break;
			}
		#ifndef RELIABILITY
			else if(write(fd,recv_pack,rcvd_bytes)<0){//write to file , with strlen as sizeof has larger value 
						perror("Error for writing to file");
						rcount =-1;	
				}
			else// when write has be done
			{	
					//printf("packetno:bytes:written bytes %d:%d:%d\n",rcount,(int)file_bytes,rcvd_bytes );
					file_size = file_size + rcvd_bytes;
					strcmp(prev_pack_no,packet_no);		
		        	rcount++;//recvd packet counter
		        	//FOmrating ACK|PacketNO
					bzero(ack_message,sizeof(ack_message));
					//sprintf(packet_no,"%d",(int)strtol(rcvd_packet_no,NULL,10));
					strcpy(ack_message,"ACK");
					strcat(ack_message,rcvd_packet_no);
					sendtoClient(ack_message,sizeof(ack_message),NOACK);

			}
	    #else
	      
			else if (!strcmp(prev_pack_no,rcvd_packet_no)){//check if packet was already recieved before and ACK was missed 
					//strcat(ack_message,sprintf(packet_no,"%d",);
					strcpy(ack_message,"ACK");
					strcat(ack_message,prev_pack_no);
					//printf("Resending ACK %s\n",ack_message );
					sendtoClient(ack_message,sizeof(ack_message),NOACK);	
			}	
			else{//if previous packet no is not same as present 
			
				if(write(fd,recv_pack,rcvd_bytes)<0){//write to file , with strlen as sizeof has larger value 
					perror("Error for writing to file");
					rcount =-1;	
				}
				else// when write has be done
				{
				
					//printf("packetno:bytes:written bytes %d:%d:%d\n",rcount,(int)file_bytes,rcvd_bytes );
					file_size = file_size + rcvd_bytes;//Calculate the complete file Size 
					
		        
		        	rcount++;//recvd packet counter
		        	
		        	//Fomrating ACK|PacketNO
					bzero(ack_message,sizeof(ack_message));
					strcpy(ack_message,"ACK");
					sprintf(packet_no,"%d",(int)strtol(rcvd_packet_no,NULL,10));
					strcat(ack_message,packet_no);
					sendtoClient(ack_message,sizeof(ack_message),NOACK);
					strcpy(prev_pack_no,rcvd_packet_no);

		        }
	        
			}
		#endif							
			
			file_bytes =0 ;
			pos_data =0;
			bzero(recv_pack,sizeof(recv_pack));
			bzero(recv_frame,sizeof(recv_frame));
		}while(((file_bytes =recvfrom(server_sock,recv_frame,sizeof(recv_frame),0,(struct sockaddr*)&server,&remote_length))) >0);//read file 
		//while (file_bytes=recv_packet(recv_frame,sizeof(recv_frame))>0);


		if (rcount >0){				
				printf("Rcvxd packet: total Bytes %d:%d\n",rcount,(int )file_size);	
		}	
		// Close file	
		if(close(fd) <0)//check if file closed 
		{
			perror(filename);
			rcount =-1;
		}					
		
		//Receive MD5 value from Server,//Wait for MD5 hash value 
		if(file_bytes =recvfrom(server_sock,MD5_Client,MD5_DIGEST_LENGTH*2,0,(struct sockaddr*)&server,&remote_length)>0){
			MD5Cal(filename,MD5_Server);//Calculate for file at Client end 
			//printf("Client: %s\n Server: %s\n",MD5_Client,MD5_Server);
			//Compare 
			if(!strcmp(MD5_Client,MD5_Server))
			{
				printf("Recieved File match with Client \n" );	
			}
			else
			{
				printf("Retry ,Files dont match!!\n");		
			}
		}
		//Acknowledge  
	}
	//printf("Completed Rcvxd Routine\n");
	free(MD5_Client);
	free(MD5_Server);


	bzero(recv_pack,sizeof(recv_pack));
	bzero(packet_no,sizeof(packet_no));
	//bzero(prev_pack_no,sizeof(prev_pack_no));
	bzero(rcvd_packet_no,sizeof(rcvd_packet_no));
	bzero(rcvd_size,sizeof(rcvd_size));
	bzero(recv_frame,sizeof(recv_frame));
	rcvd_bytes=0;
	pos_data=0;
	pos_size=0;

	return rcount;
}




/*************************************************************
// Send a file from Server to Client
I/p : filename - File name to be pused to client 
o/p : find and file is pushed to client 
	  return number of status
Reference for understanding :
https://lms.ksu.edu.sa/bbcswebdav/users/mdahshan/Courses/CEN463/Course-Notes/07-file_transfer_ex.pdf	  
**************************************************************/
int sendFile (char *filename){
	
	int fd; // File decsriptor 
	ssize_t file_bytes,file_size;
	int scount=0;//count for send count 

	char *err_indication = "Error";
	char *comp_indication = "Comp";

	char err_message[ERRMESSAGE] = "File not Present"; 
	char *MD5_message;
	char MD5_temp[MD5_DIGEST_LENGTH*2];
	MD5_message = MD5_temp;
	char send_pack[MAXPACKSIZE];
	char send_frame[MAXFRAMESIZE];
	char send_size[SIZEMESSAGE];
	int temp_displace=PACKETNO+SIZEMESSAGE;
	int recvd_ack_no=0;
	int repeat_count=MAXREPEATCOUNT;
	int flags = fcntl(server_sock, F_GETFL);
	//open the file
	if((fd= open(filename,O_RDONLY)) <0 ){//open read only file 
		perror(filename);//if can't open
		//send to client the error 
		sendtoClient(err_indication,sizeof(err_indication),NOACK);
		sendtoClient(err_message,sizeof(err_message),NOACK);
		return -1;
	}
	else // open file successfully ,read file 
	{
		//debug 
		//printf("Sending %s\n",filename);
		//read bytes and send 
		while((file_bytes = read(fd,send_pack,MAXPACKSIZE)) != 0 ){//read data from file 			
			//Append packet no to packet --Frame -- PACKETNO|PACKETSIZE|PACKETDATA
			sprintf(send_frame,"%06d|%06d|",scount,(int)file_bytes);
			memcpy(send_frame+temp_displace,send_pack,file_bytes);//copy the send pack to send frame (** use memset as image files an issue)
			//printf("frame :%s\n",send_frame );

			#ifndef RELIABILITY
				recvd_ack_no=sendtoClient(send_frame,sizeof(send_frame),ACK);	//send frame
			#else	
				//RELIABILITY 
				//Reset to UnBlocking 
				timeout.tv_sec = 0;
				timeout.tv_usec = 100000;

				if (setsockopt (server_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0){
	        		perror("setsockopt failed\n");
	    		}
	    		else
	    		{	//repeat for MAXREPEAT COUNT till acknowledge recieved 
	    			do
	    			{
		    			recvd_ack_no=sendtoClient(send_frame,sizeof(send_frame),ACK);	//send frame
		    			//printf("ack no%d\n",recvd_ack_no );
		    		}while (recvd_ack_no!=scount && repeat_count-- >=0);	
		    		if (recvd_ack_no != scount)//if even multiple tries failed 
		    		{
		    			//printf("try Again !!Files failing %d times\n",MAXREPEATCOUNT);
		    			repeat_count=MAXREPEATCOUNT;
		    		}

		    		else
		    		{	
			    		//printf("Send the packet in %d\n",repeat_count );
			    		repeat_count=MAXREPEATCOUNT;
			    		//recvd_ack_no=0;
			    	}	
	    		}
	    		//Reset to Blocking 
	    		timeout.tv_sec = 0;
				timeout.tv_usec = 0;
				if (setsockopt (server_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0){
	        		perror("setsockopt failed\n");
	    		}
				//Set it  blocking
				//fcntl(server_sock, F_SETFL, flags);

			#endif
			//update count 
			scount++;
			//clearing for new data
			file_bytes =0;
			bzero(send_pack,sizeof(send_pack));
			bzero(send_frame,sizeof(send_frame));
		}		
		close(fd);//close file if opened 
	}
	sendtoClient(comp_indication,(ssize_t)sizeof(comp_indication),NOACK);	//send Completion of File
	MD5Cal(filename,MD5_message);
	//printf("MD5 Message%s\n",MD5_message );
	//Send MD5 Hash value 
	sendtoClient(MD5_message,2*MD5_DIGEST_LENGTH,NOACK);

	
	//printf("No of packets send %d\n",scount );
	return scount;
}

//Client connection for each client 

void *client_connections(void *client_sock_id)
{
	//Obtain the socket desc
	printf("Pthread created \n");
	int thread_sock = (int)client_sock_id;
	
	ssize_t read_bytes=0;
	//printf("Inside client_connections 3\n");
	char message_client[MAXPACKSIZE];//store message from client 
	
	//char *message ; // message to client 
	printf("passed Client connection %d\n",(int)client_sock_id);
	

	// Recieve the message from client  and reurn back to client 
	while((read_bytes =recv(thread_sock,message_client,MAXPACKSIZE,0))>0){

		printf("%s\n",message_client );
		//write(thread_sock,message_client,strlen(message_client));
		bzero(message_client,sizeof(message_client));
	}
	if (read_bytes < 0){
		perror("recv from client failed ");
		return NULL;

	}
	printf("Completed \n");
	//free(client_sock_id);//free the memory 
	return 1 ;
	
}


int main (int argc, char * argv[] ){
	char request[MAXPACKSIZE];             //a request to store our received message
	
	int *mult_sock;//to alloacte the client socket descriptor


	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&server,sizeof(server));                    //zero the struct
	server.sin_family = AF_INET;                   //address family
	//server.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	server.sin_port = htons(SERV_PORT);        //htons() sets the port # to network byte order
	server.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine
	remote_length = sizeof(struct sockaddr_in);           //size of client packet 
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