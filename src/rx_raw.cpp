#include "rx_raw.h"

int flagHelp = 0;

void usage(void) {
	printf("\nUsage: rx_raw [options]\n"
	"\n"
	"Options:\n"
	"-p  <port>     UDP port for data input.\n"
	"-i  <IP>       IP to also relay/forward all incomming data to.\n"
	"-r  <port>     Relay port for destination.\n"
	"-n  <port>	    Relay rssi data on this port.\n"
	"Example:\n"
	"  ./rx_raw -p 7000\n"
	"  ./rx_raw -p 7000 -i 192.168.0.6 -r 14450 \n"
	"  ./rx_raw -p 7000 -i 192.168.0.67 -r 14450 -n 5154 \n"
	"\n");
	exit(1);
}

int main(int argc, char *argv[]) 
// Input arguments ./rx_raw [INPUT UDP PORT]
// argv[0] 	./main - not used
// argv[1] 	[INPUT UDP PORT] 
{
	char *p;
	int udpPort= 0; 
	char *relayIP;
	int relayPort=0;
	int rssiPort=0;

    while (1) {
	    int nOptionIndex;
	    static const struct option optiona[] = {
		    { "help", no_argument, &flagHelp, 1 },
		    {      0,           0,         0, 0 }
	    };
	    int c = getopt_long(argc, argv, "h:p:i:r:n:", optiona, &nOptionIndex);
	    if (c == -1) {
		    break;
	    }

	    switch (c) {
		    case 0: {
			    // long option
			    break;
		    }
		    case 'h': {
			    //				fprintf(stderr, "h\n");
			    usage();
			    break;
		    }
	
		    case 'p': {
			    udpPort = atoi(optarg);
			    //				fprintf(stderr, "Serial Port   :%d\n",udpSerialPort);
			    break;
		    }

	
			case 'i': {
				relayIP = optarg;
				break;
			}

	
			case 'r': {
				relayPort = atoi(optarg);
				break;
			}
			
			case 'n': {
				rssiPort = atoi(optarg);
				break;
			}

		    default: {
			    fprintf(stderr, "RX: unknown input parameter switch %c\n", c);
			    usage();

			    break;
		    }
	    }
    }

    if (optind > argc) {
	    usage();
    }
	
	fprintf(stderr, "Starting Lagoni's UDP RX program v0.20 on port %d\n", udpPort);	
	
	if (udpPort <= 0) {
		fprintf(stderr, "RX: ERROR port is 0, not valid\n");
	    usage();
	}

	// For relay
	Connection *relayConnection = NULL;
	if(relayPort != 0){
		relayConnection = new Connection(relayIP, relayPort, SOCK_DGRAM); // blocking.
	}else{
		relayConnection = new Connection(); // Empty.
	}
		
	// For UDP/TCP Sockets
	Connection myConnection(udpPort, SOCK_DGRAM); // UDP port
	
	uint8_t rxBuffer[RX_BUFFER_SIZE];
	int nready, maxfdp1; 
	fd_set rset; 
//	udpfd =  myConnection.getFD();
//	relayfd = relayConnection->getFD();

	struct timeval timeout; // select timeout.

	// For link status:
	rx_dataRates_t linkstatus;
	bzero(&linkstatus, sizeof(linkstatus));
	time_t nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
	rx_status_t rssiData;
	bzero(&rssiData, sizeof(rssiData));
	
	struct sockaddr_in si_other_rssi;
	int s_rssi, slen_rssi = sizeof(si_other_rssi);
	if(rssiPort != 0){
		si_other_rssi.sin_family = AF_INET;
		si_other_rssi.sin_port = htons(rssiPort); //PC
		//si_other_rssi.sin_port = htons(5155); //Raspberry pi
		si_other_rssi.sin_addr.s_addr = inet_addr("192.168.0.67");
		//si_other_rssi.sin_addr.s_addr = inet_addr("127.0.0.1");
		memset(si_other_rssi.sin_zero, '\0', sizeof(si_other_rssi.sin_zero));
		if ((s_rssi = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
			fprintf(stderr, "ERROR: Could not create UDP socket!");
		}		
	}
	
	  
	do{
		FD_ZERO(&rset); 
		myConnection.setFD_SET(&rset);
		
		if(relayPort != 0){
			relayConnection->setFD_SET(&rset);			
			maxfdp1 = max(myConnection.getFD(), relayConnection->getFD());
		}else{
			maxfdp1 = myConnection.getFD();
		}

		// Timeout
		timeout.tv_sec = 0;
		timeout.tv_usec = 100; // 1000ms
		
		nready = select(maxfdp1+1, &rset, NULL, NULL, &timeout); // since we are blocking, wait here for data.//
		
		if (FD_ISSET(myConnection.getFD(), &rset)) { // Data from UDP
			int result = 0;
			result = myConnection.readData(rxBuffer, RX_BUFFER_SIZE);
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > RX_BUFFER_SIZE) {
				fprintf(stderr, "RX: Error on UDP Socket, Terminate program.\n");
				exit(EXIT_FAILURE);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else {	
				write(STDOUT_FILENO, rxBuffer, result);			
				linkstatus.rx += (float)result;
				
				// UDP Relay to IP and PORT
				if(relayPort != 0){
					relayConnection->writeData(rxBuffer, result);
				}
			}
		}
		
		if( (relayPort != 0) && (FD_ISSET(relayConnection->getFD(), &rset))){// Data from UDP relay back (When we relay to Mavlink server, it will send a few messages back to drone)
			int result = 0;
			result = relayConnection->readData(rxBuffer, RX_BUFFER_SIZE);
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > RX_BUFFER_SIZE) {
				fprintf(stderr, "RX: Error on Relay UDP Socket, Terminate program.\n");
				exit(EXIT_FAILURE);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else {
				int res = 0;
				res = myConnection.writeData(rxBuffer, result); // Write incomming data to drone.
					if(res < 0){
				}else if(res == 0){
					linkstatus.dropped += result;					
				}else{							
					linkstatus.rx += result;						
				}
			}
		}
		

		// check if it is time to log the status:
		if(time(NULL) >= nextPrintTime){
			
			fprintf(stderr, "RX: Status:       UDP Packages: (tx|rx|dropped):  %*.2fKB  |  %*.2fKB  | %*.2fKB\n", 6, linkstatus.tx/1024 , 6, linkstatus.rx/1024 , 6 , linkstatus.dropped/1024);
			nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
			
			if(rssiPort != 0){
				rssiData.kbitrate = (uint32_t)((linkstatus.rx/1024)*8);
				if (sendto(s_rssi, &rssiData, 113, 0, (struct sockaddr*)&si_other_rssi, slen_rssi) == -1) {
					fprintf(stderr, "ERROR: Could not send RSSI data!");
				}
			}
			bzero(&linkstatus, sizeof(linkstatus));
		}
		
	}while(1);
	
	perror("RX: PANIC! Exit While 1\n");
    return 1;
}

