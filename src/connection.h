/*
	Connection.h

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <arpa/inet.h> 
#include <errno.h> 
#include <netinet/in.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <strings.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <fcntl.h>   

class Connection
{
	// Public functions to be used on all Messages
	public:
	
	Connection(); // need for array int.
    Connection(int port, int type); //constructor with port and type. This will run the startConnection(port, type)
	Connection(int port, int type, int flags); 
    Connection(const char* hostname, int port, int type, int flags); // IP for hostname and type=O_NONBLOCK or for blocking flags=0
	Connection(const char* hostname, int port, int type); // Default blocking.
	//Connection(int fd, struct sockaddr_in); //constructor with file destriptor and client address (used when greated from TCP listen). 
	
	void startConnection(int fd);
	void setFD_SET(fd_set* fdset);
	int getFD();
	void setFD(int fd);
	int16_t readData(void *buffer, uint16_t length);
	int16_t writeData(void *buffer, uint16_t length);
	int getType(void);

	void initConnection();
	
	protected:
			
	// Parameters only used on mother cl
	
	private:
	int _fd, _port, _type, _flags;
	struct sockaddr_in _servaddr;
	struct sockaddr_in _cliaddr;
	bool isValid = false;
	
	void print_ipv4(struct sockaddr *s);
	void clearAll(void);
};


#endif /* CONNECTION_H_ */
