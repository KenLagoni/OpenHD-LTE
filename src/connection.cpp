/*
	Connection.cpp

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 
#include "connection.h"

// Constructor

 Connection::Connection(){
	this->_fd=0;
	this->_port=0;
	this->_type=0;
	this->_flags=0;
	bzero(&this->_servaddr, sizeof(this->_servaddr));	
	bzero(&this->_cliaddr, sizeof(this->_cliaddr));	
 }
 
 Connection::Connection(int port, int type){
	this->_fd=0;
	this->_type=type;
	this->_port=port;
	this->_flags=0;
	bzero(&this->_servaddr, sizeof(this->_servaddr));	
	bzero(&this->_cliaddr, sizeof(this->_cliaddr));	
	this->initConnection();
 }
 
  
  Connection::Connection(int port, int type, int flags){
	  this->_fd=0;
	  this->_type=type;
	  this->_port=port;
	  this->_flags=flags;
	  bzero(&this->_servaddr, sizeof(this->_servaddr));
	  bzero(&this->_cliaddr, sizeof(this->_cliaddr));
	  this->initConnection();
  }
 
 // Only supports UDP and is set to non-blocking
  Connection::Connection(const char* hostname, int port, int type, int flags){
	this->_fd=0;
	this->_type=type;
	this->_port=port;
	this->_flags=flags;
	//this->initConnection(port, type);
	bzero(&this->_servaddr, sizeof(this->_servaddr));	
	bzero(&this->_cliaddr, sizeof(this->_cliaddr));	
	
	// send to hostname on Port.
	this->_cliaddr.sin_family = AF_INET; 
	this->_cliaddr.sin_addr.s_addr = inet_addr(hostname); 
	this->_cliaddr.sin_port = htons(this->_port); 
			
	this->initConnection();	
    
	this->isValid=true; // We can send right away because we know the receiver
 }

  // Only supports UDP and is set to non-blocking
  Connection::Connection(const char* hostname, int port, int type){
	this->_fd=0;
	this->_type=type;
	this->_port=port;
	this->_flags=0;
	//this->initConnection(port, type);
	bzero(&this->_servaddr, sizeof(this->_servaddr));
	bzero(&this->_cliaddr, sizeof(this->_cliaddr));
	
	// send to hostname on Port.
	this->_cliaddr.sin_family = AF_INET;
	this->_cliaddr.sin_addr.s_addr = inet_addr(hostname);
	this->_cliaddr.sin_port = htons(this->_port);
	
	this->initConnection();
	
	this->isValid=true; // We can send right away because we know the receiver
}
 
void Connection::initConnection(int port, int type){
	this->_type=type;
	this->_port=port;
	this->initConnection();
}
void Connection::initConnection(int port, int type, int flags){
	this->_type=type;
	this->_port=port;
	this->_flags=flags;
	this->initConnection();
}
 
 void Connection::initConnection(){ 
	// Listen on port, from hostname IP.
	this->_servaddr.sin_family = AF_INET; 
	this->_servaddr.sin_addr.s_addr = INADDR_ANY; 
	this->_servaddr.sin_port = htons(this->_port); 
	
	if(this->_type == SOCK_STREAM){
	    // create listening TCP socket 
		if ( (this->_fd  = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			perror("Connection: TCP socket creation failed");
			exit(EXIT_FAILURE);
		}
		// binding server addr structure to this->_fd 
		bind(this->_fd, (struct sockaddr*)&this->_servaddr, sizeof(this->_servaddr)); 
		listen(this->_fd, 10); 		
	}else if(this->_type == SOCK_DGRAM){
		// create UDP socket 
		if ( (this->_fd  = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
			perror("Connection: UDP socket creation failed");
			fprintf(stderr, "Connection: UDP socket creation failed");
			exit(EXIT_FAILURE);
		}
		// binding server addr structure to this->_fd  
		bind(this->_fd, (struct sockaddr*)&this->_servaddr, sizeof(this->_servaddr)); 	 
	}else{
		this->_fd=0;
		// unknown
	}
	
	// Set to nonblocking / blokcing
	if(O_NONBLOCK == this->_flags){
		//printf("NONE BLOVKING for fd=%d",this->_fd);
		int status = fcntl(this->_fd, F_SETFL, fcntl(this->_fd, F_GETFL, 0) | O_NONBLOCK);
		if (status == -1){
			perror("Connection: Error calling fcntl.");
			exit(EXIT_FAILURE);
		}
	}
 }
  
 void Connection::startConnection(int fd){	
	socklen_t len; 
	len=sizeof(this->_cliaddr);
	this->_fd = accept(fd, (struct sockaddr*)&this->_cliaddr, &len);
	this->isValid=true;
 }
   

 void Connection::setFD_SET(fd_set* fdset){
	FD_SET(this->_fd, fdset); 
 }
  
 int Connection::getFD(){
	return this->_fd;
 }

 void Connection::setFD(int fd){
	this->_fd = fd;
 }
 

 int16_t Connection::readData(void *buffer, uint16_t maxLength){ // returns number of bytes read.
	 int n;
	 socklen_t len; 
	 len=sizeof(this->_cliaddr);

	 int err;
	// this->print_ipv4((struct sockaddr*)&this->_cliaddr);
	 n = recvfrom(this->_fd, buffer, maxLength, 0, (struct sockaddr*)&this->_cliaddr, &len); 
	 err = errno; // save off errno, because because the printf statement might reset it
	 if((n < 0)){ // Error
		 if ((err == EAGAIN) || (err == EWOULDBLOCK))
		 {
			 //printf("non-blocking operation returned EAGAIN or EWOULDBLOCK\n");
			 return 0; // None blocking, no data to read, thus return 0 bytes read.
		 }else{
			 if(err == EBADF){
				 fprintf(stderr, "Connection: The argument sockfd is an invalid file descriptor.\n\r");	 
			 }else if(err == ECONNREFUSED){
				 fprintf(stderr, "Connection: A remote host refused to allow the network connection.\n\r");
			 }else if(err == EFAULT){
				 fprintf(stderr, "Connection: The receive buffer pointer(s) point outside the process's address space.\n\r");
			 }else if(err == EINTR){
				 fprintf(stderr, "Connection: The receive was interrupted by delivery of a signal before any data was available.\n\r");
			 }else if(err == EINVAL){
				 fprintf(stderr, "Connection: Invalid argument passed.\n\r");
			 }else if(err == ENOMEM){
				 fprintf(stderr, "Connection: Could not allocate memory for recvmsg().\n\r");
			 }else if(err == ENOTCONN){
				 fprintf(stderr, "Connection: The socket is associated with a connection-oriented protocol and has not been connected.\n\r");
			 }else if(err == ENOTSOCK){
				 fprintf(stderr, "Connection: The file descriptor sockfd does not refer to a socket.\n\r");
			 }else{
				 fprintf(stderr, "Connection: Unknown error - reading with result=%d, thus closing\n",(int)n);
			 }	 
			close(this->_fd);
			this->_fd=0;
			this->isValid=false;			
		 }
	 }else{
		 this->isValid=true;
	 }
 
	 return n;
 }
  
 int16_t Connection::writeData(void *buffer, uint16_t length){ // returns?
	ssize_t n = 0;
	socklen_t len; 
	len=sizeof(this->_cliaddr);	
	
	if(this->isValid){
		int err;

		//printf("Connection: Sending %d bytes to ", length);
		//this->print_ipv4((struct sockaddr*)&this->_cliaddr);
		//printf("\n");

		n = sendto(this->_fd, buffer, length, 0, (struct sockaddr *)&this->_cliaddr, len);
		err = errno; // save off errno, because because the printf statement might reset it

		if((n < 0)){ // Error
			if ((err == EAGAIN) || (err == EWOULDBLOCK))
			{
				//printf("non-blocking operation returned EAGAIN or EWOULDBLOCK\n");
				return 0;
			}else  if(err == EBADF){
				fprintf(stderr, "Connection: The argument sockfd is an invalid file descriptor.\n\r");
			}else if(err == ECONNREFUSED){
				fprintf(stderr, "Connection: A remote host refused to allow the network connection.\n\r");
			}else if(err == EFAULT){
				fprintf(stderr, "Connection: The receive buffer pointer(s) point outside the process's address space.\n\r");
			}else if(err == EINTR){
				fprintf(stderr, "Connection: The receive was interrupted by delivery of a signal before any data was available.\n\r");
			}else if(err == EINVAL){
				fprintf(stderr, "Connection: Invalid argument passed.\n\r");
			}else if(err == ENOMEM){
				fprintf(stderr, "Connection: Could not allocate memory for recvmsg().\n\r");
			}else if(err == ENOTCONN){
				fprintf(stderr, "Connection: The socket is associated with a connection-oriented protocol and has not been connected.\n\r");
			}else if(err == ENOTSOCK){
				fprintf(stderr, "Connection: The file descriptor sockfd does not refer to a socket.\n\r");
			}else{
				fprintf(stderr, "Connection: writing length=%d in FD=%d with result=%d, thus closing\n",length,this->_fd,(int)n );		 
			}
			close(this->_fd);
			this->_fd=0;
			this->isValid=false;
		}
	}
	return n;
 }
 

void Connection::print_ipv4(struct sockaddr *s)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)s;
	char ip[INET_ADDRSTRLEN];
	uint16_t port;

	inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof (ip));
	port = htons(sin->sin_port);

	printf ("%s:%d", ip, port);
}
 