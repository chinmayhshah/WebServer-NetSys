ReadMe.txt
Instructions 

For Execution File 


1. Run "make" with .c files to generate executbles for "server" 
	a. For Individual Files run make server or make client
	b. For removal of object files Run "make clean"
2. To run 
	a. ./server to run the webserver 
	

Assumptions 
1.ws.conf File does not have bankspaces



Functionality
1. Implementation of Basic Features 
	Basic Implementation of webserver accessing files include .txt,.html,.jss,.css,.png,.jpeg,.jpg with error messages implemented 

2. Implementation of Configuration file 
	--Checks for Listen port availability and less than 1024 
	-- Checks for alteast availability of one content type
	-- Check for directory root
	-- Check for Default directory , iganores if not avialable 
3.Implentation of POST 
	-- Access posttest.html , The data posted on this page is replied to client 
4. Implementation of pipeline 
	-- I had implemented it but has many erros thus reverted back (with Keep alive file :server_keepalive.c)
	-- enable #define KeepAlive

5.# To enable Debugging 
	#define DEBUGLEVEL

	
Test 

Listen port :8000

1.Test for Error 501 
http://localhost:8000/files/test.tx

2.Test for Error 404
http://localhost:8000/files/test.txt

3. Test invalid HTTP version (1.1)(400)
GET /index.html HTTP/1.2\nHost: localhost\n

4. Test invalid HTTP (400)
HEAD /index.html HTTP/1.1\nHost: localhost\n

5. Test invalid HTTP (400)
GET \index\as.html HTTP/1.1\nHost: localhost\n

6. Test Post implementation 
http://localhost:8000/posttest.html
