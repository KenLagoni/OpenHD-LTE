#include "rx_raw.h"

int flagHelp = 0;


void usage(void) {
	printf("\nUsage: rx_raw [options]\n"
	"\n"
	"Options:\n"
	"-v  <port>     Port for input video data.\n"
	"-m  <port>     Port for input Mavlink UDP data.\n"
	"-t  <port>     Port for input Telemetri UDP data.\n"

	"-i  <IP>       IP to forward all Mavlink data to MavlinkServer.\n"
	"-r  <port>     Port for relay Mavlink data.\n"
	"Program will automatically sent:\n"
	"Video->localhost:5600\n"
	"Mavlink->localhost:14450\n"
	"Telemetry(reframed for QOpenHD)->localhost:5155\n"
	"Optional:\n"
	"Mavlink<->IP:PORT\n"
	"Mavlink->192.168.0.8:6000\n" // For video record on/off.
	"Example:\n"
	"  ./rx_raw -v 7000 -m 12000 -t 5200 -i 192.168.0.67 -r 14550\n"
	"\n");
	exit(1);
}


Timer totalReadTime,totalSetDataTime,totalWriteSTDOUTTime,totalWaitTime,totalBusyTime;

#define OUTPUT_VIDEO_PORT 5600
#define OUTPUT_MAVLINK_PORT 14550
#define OUTPUT_TELEMETRY_PORT 5155

int main(int argc, char *argv[]) 
// Input arguments ./rx_raw [INPUT UDP PORT]
// argv[0] 	./main - not used
// argv[1] 	[INPUT UDP PORT] 
{
	char *p;
	int videoPort= 0; 
	int mavlinkPort= 0; 
	int telemetryPort= 0; 
	char *relayIP;
	int relayPort=0;
		
    while (1) {
	    int nOptionIndex;
	    static const struct option optiona[] = {
		    { "help", no_argument, &flagHelp, 1 },
		    {      0,           0,         0, 0 }
	    };
	    int c = getopt_long(argc, argv, "h:v:m:t:i:r:", optiona, &nOptionIndex);
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
	
		    case 'v': {
			    videoPort = atoi(optarg);
			    //				fprintf(stderr, "Serial Port   :%d\n",udpSerialPort);
			    break;
		    }

	
			case 'm': {
				mavlinkPort = atoi(optarg);
				break;
			}

	
			case 't': {
				telemetryPort = atoi(optarg);
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
	
	
	if (videoPort <= 0) {
		fprintf(stderr, "RX: ERROR video port is 0, not valid\n");
	    usage();
	}
	
	if (mavlinkPort <= 0) {
		fprintf(stderr, "RX: ERROR mavlink port is 0, not valid\n");
		usage();
	}
	
	if (telemetryPort <= 0) {
		fprintf(stderr, "RX: ERROR telemetry port is 0, not valid\n");
		usage();
	}

	fprintf(stderr, "Starting UDP RX program\n");


	// For relay
	Connection *relayConnection = NULL;
	if(relayPort != 0){
		relayConnection = new Connection(relayIP, relayPort, SOCK_DGRAM); // blocking.
	}else{
		relayConnection = new Connection(); // Empty.
	}
		
	// For UDP Sockets
	Connection inputVideoConnection(videoPort, SOCK_DGRAM,O_NONBLOCK);

	// For UDP Sockets
	Connection inputMavlinkConnection(mavlinkPort, SOCK_DGRAM); // UDP port
	Connection outputMavlinkConnection("127.0.0.1", OUTPUT_MAVLINK_PORT, SOCK_DGRAM);
	Connection extraRelayMavlinkConnection("192.168.0.8", 6000, SOCK_DGRAM);
		
	// For UDP Sockets
	Connection inputTelemetryConnection(telemetryPort, SOCK_DGRAM); // UDP port
	Connection outputTelemetryConnection("127.0.0.1", OUTPUT_TELEMETRY_PORT, SOCK_DGRAM);
				
	static H264RXFraming RXpackageManager; // Needs to be static so it is not allocated on the stack, because it uses 8MB.
	uint8_t rxBuffer[UDP_PACKET_SIZE];
	
	telemetryWrapper RXtelemetry;
	
//	std::string str = "/home/pi";
//    char* file = &*str.begin();
//	H264Recorder videoRecorder(file); // debug
//	videoRecorder.start();

	mavlinkHandler mavlinkDataFromAir;

	int nready, maxfdp1; 
	fd_set rset; 
	struct timeval timeout; // select timeout.

	// For link status:
	rx_dataRates_t linkstatus;
	bzero(&linkstatus, sizeof(linkstatus));
	time_t nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;


	int status = fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL, 0) | O_NONBLOCK);
	if (status == -1){
		perror("Error calling fcntl on STDOUT_FILENO.");
		exit(EXIT_FAILURE);
	}
	
	int active = 2; // print log for 2 seconds.

	do{
		FD_ZERO(&rset); 
		inputMavlinkConnection.setFD_SET(&rset);
		outputMavlinkConnection.setFD_SET(&rset);
		
		//inputVideoConnectionListener.setFD_SET(&rset); // For TCP
		if(inputVideoConnection.getFD() != 0){ // only include id connection is valid
			inputVideoConnection.setFD_SET(&rset);			
		}
		inputTelemetryConnection.setFD_SET(&rset);
		outputTelemetryConnection.setFD_SET(&rset);		
				
						
				
		if(relayPort != 0){
			relayConnection->setFD_SET(&rset);			
			maxfdp1 = max(inputMavlinkConnection.getFD(), relayConnection->getFD());
		}else{
			maxfdp1 = inputMavlinkConnection.getFD();
		}
		maxfdp1 = max(maxfdp1, outputMavlinkConnection.getFD());
		
		//maxfdp1 = max(maxfdp1, inputVideoConnectionListener.getFD());		
		if(inputVideoConnection.getFD() != 0){ // only include id connection is valid
			maxfdp1 = max(maxfdp1, inputVideoConnection.getFD());
		}
		maxfdp1 = max(maxfdp1, inputTelemetryConnection.getFD());
		maxfdp1 = max(maxfdp1, outputTelemetryConnection.getFD());
		
		// Timeout
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000; // 10ms
		
		
		totalBusyTime.stop();
		totalWaitTime.start();
		nready = select(maxfdp1+1, &rset, NULL, NULL, &timeout); // since we are blocking, wait here for data.//
		totalWaitTime.stop();
		totalBusyTime.start();
		
		// Video DATA from Drone		
		if (FD_ISSET(inputVideoConnection.getFD(), &rset)) { 

			int result = 0;
			do{
				totalReadTime.start();
				result = inputVideoConnection.readData(rxBuffer, sizeof(rxBuffer));
				totalReadTime.stop();
				
				if (result < 0 || result > sizeof(rxBuffer)){
					// If TCP that means server connection is lost:					
					// Establish connection again!
								
					// IF UDP
					fprintf(stderr, "RX: Error on Input video UDP Socket Port: %d, Terminate program.\n", videoPort);
					exit(EXIT_FAILURE);
				}else if(result == 0){
					// None blocking, nothing to read.
					// Blocking will never end up here because it will wait in readData... :-(
				}else{	
					totalSetDataTime.start();
					RXpackageManager.setData(rxBuffer, (uint16_t)result); // handles the 
					totalSetDataTime.stop();
					active = 10; // ensure log will run for 10 seconds.
				}				
			}while(result>0);			
		}


		// Mavlink DATA from Drone
		if (FD_ISSET(inputMavlinkConnection.getFD(), &rset)) {
			int result = 0;
			result = inputMavlinkConnection.readData(rxBuffer, sizeof(rxBuffer));
			
			if (result < 0 || result > sizeof(rxBuffer)) {
				fprintf(stderr, "RX: Error on Input Mavlink UDP Socket Port: %d, Terminate program.\n", mavlinkPort);
				exit(EXIT_FAILURE);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else {
				active = 10; // ensure log will run for 10 seconds.
				outputMavlinkConnection.writeData(rxBuffer, result);

				mavlinkDataFromAir.inputData(rxBuffer, result); // keep track of air status.

				if(relayPort != 0){
					relayConnection->writeData(rxBuffer, result);
				}			
				extraRelayMavlinkConnection.writeData(rxBuffer, result); // extra relay for video record when armed.	
			}
		}


		// DATA from OpenHD (Mavlink return) This should not happend, or it is a keep-alive
		if (FD_ISSET(outputMavlinkConnection.getFD(), &rset)) {
			int result = 0;
			result = outputMavlinkConnection.readData(rxBuffer, sizeof(rxBuffer));
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > sizeof(rxBuffer)) {
				fprintf(stderr, "RX: Error on read from Output Mavlink UDP Socket Port: %d, Terminate program.\n", OUTPUT_MAVLINK_PORT);
				exit(EXIT_FAILURE);
			}else if(result == 0){
				// None blocking, nothing to read.
			}else {
					
			}
		}


		// Telemtry DATA from Drone
		if (FD_ISSET(inputTelemetryConnection.getFD(), &rset)) {
			int result = 0;
			result = inputTelemetryConnection.readData(rxBuffer, sizeof(rxBuffer));
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > sizeof(rxBuffer)) {
				fprintf(stderr, "RX: Error on Input Telemetry UDP Socket Port: %d, Terminate program.\n", telemetryPort);
				exit(EXIT_FAILURE);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else { // We have data lets parse it to the telemetry wrapper class.
				RXtelemetry.inputDataFromTXTelemetryArray(rxBuffer, result);
				RXtelemetry.setQOpenHDAirCPULoad(RXtelemetry.getCPULoad());
				RXtelemetry.setQOpenHDAirTemperature(RXtelemetry.getCPUTemperature());
				active = 10; // ensure log will run for 10 seconds.
			}
		}


		// DATA from OpenHD (Telemetry return) This should not happend, or it is a keep-alive
		if (FD_ISSET(outputTelemetryConnection.getFD(), &rset)) {
			int result = 0;
			result = outputTelemetryConnection.readData(rxBuffer, sizeof(rxBuffer));
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > sizeof(rxBuffer)) {
				fprintf(stderr, "RX: Error on Output Telemetry UDP Socket Port: %d, Terminate program.\n", OUTPUT_TELEMETRY_PORT);
				exit(EXIT_FAILURE);
			}else if(result == 0){
				// None blocking, nothing to read.
			}else {
				
			}
		}

		
		if( (relayPort != 0) && (FD_ISSET(relayConnection->getFD(), &rset))){// Data from UDP relay back (When we relay to Mavlink server, it will send a few messages back to drone)
			int result = 0;
			result = relayConnection->readData(rxBuffer, sizeof(rxBuffer));
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > sizeof(rxBuffer)) {
				fprintf(stderr, "RX: Error on Input Relay UDP Socket Port: %d, Terminate program.\n", relayPort);
				exit(EXIT_FAILURE);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else {
				int res = 0;
				res = inputMavlinkConnection.writeData(rxBuffer, result); // Write incoming data to drone.
				if(res < 0){
					fprintf(stderr, "RX: Error on write to Output Mavlink UDP Socket Port: %d, Terminate program.\n", OUTPUT_MAVLINK_PORT);
					exit(EXIT_FAILURE);
				}else if(res == 0){
				 // NOP
				}else{							
				 // NOP
				}
			}
		}

		// Try to write every time.
		totalWriteSTDOUTTime.start();
		RXpackageManager.writeAllOutputStreamTo(STDOUT_FILENO);
		totalWriteSTDOUTTime.stop();

		// check if it is time to log the status and sent Telemtry frame to QOpenHD:
		if((time(NULL) >= nextPrintTime) && (active > 0 )){
			active--; // count down log active counter.

			linkstatus.rx = RXpackageManager.getBytesInputted();
			linkstatus.dropped = RXpackageManager.getBytesDropped();
			linkstatus.tx = RXpackageManager.getBytesOutputted(); 
						
			fprintf(stderr, "RX: Status: ");
			fprintf(stderr, "Timing[ms] (Read|Set|Write)  %4.0lf | %4.0lf | %4.0lf   ", totalReadTime.getTotalDuration(), totalSetDataTime.getTotalDuration() ,totalWriteSTDOUTTime.getTotalDuration());
			fprintf(stderr, "Program Time[ms] (Waiting|Busy):  %4.0lf | %4.0lf   ", totalWaitTime.getTotalDuration(),totalBusyTime.getTotalDuration());		
			fprintf(stderr, "Buffers(Input|tempOut|Out):  %5u |%5u|%5u   ", RXpackageManager.getInputQueueSize(), RXpackageManager.getTempOutputSize(), RXpackageManager.getBufferSize());	
			fprintf(stderr, "UDP: (rx|drop|out):  %*.2fKB  |  %*.2fKB  |  %*.2fKB  ", 7, linkstatus.rx/1024 , 7, linkstatus.dropped/1024 , 7 , linkstatus.tx/1024);
			fprintf(stderr, " (%u)", RXpackageManager.waitingForHeader());
			
			if(mavlinkDataFromAir.isArmed()){			
					fprintf(stderr,"  FC=ARMED");							
				}else{
					fprintf(stderr, "  FC=DISARMED");							
				}		
			fprintf(stderr, "\n");

			// parse telemetry to QOpenHD on UDP Port 5155
			RXtelemetry.setQOpenHDKbitRate((linkstatus.rx*8)/1024);
			if(outputTelemetryConnection.writeData(RXtelemetry.getTelemtryForQOpenHDAsArray(), RXtelemetry.getTelemtryForQOpenHDSize()) < 0){
				fprintf(stderr, "RX: Error on write to output Telemetry UDP Socket Port: %d, Terminate program.\n", OUTPUT_TELEMETRY_PORT);		
				exit(EXIT_FAILURE);
			}		
			
						
			
			// request header from drone?
			if( RXpackageManager.waitingForHeader() == true ){ // we are recieveing data and no header is found. since the first packages from TX is header, this should always return false. however if RX__RAW is rebooted, it will receive data but have no header, thus true here:
				// Make header request from TX package:
				fprintf(stderr, "RX: Requisting header from Drone\n");
				rxBuffer[0]=0x10; rxBuffer[1]=0x11; rxBuffer[2]=0x12; rxBuffer[3]=0x13; rxBuffer[4]=0x14; rxBuffer[5]=0x15;
			}else{
				// Make keep alive package:
				rxBuffer[0]=0x50; rxBuffer[1]=0x51; rxBuffer[2]=0x52; rxBuffer[3]=0x53; rxBuffer[4]=0x54; rxBuffer[5]=0x55;
			}
			//Send Package header request or keep alive:
			inputVideoConnection.writeData(rxBuffer, 6); // this is for header request and keep alive
			// inputTelemetryConnection.writeData(rxBuffer, 6); // no need to keep this alive?

			if(active == 0){
				fprintf(stderr, "RX: Status: No data for 10 seconds. Stop logging output.\n");	
			}

			// reset counters:
			bzero(&linkstatus, sizeof(linkstatus));
			RXpackageManager.clearIOstatus();
			totalReadTime.reset();
			totalSetDataTime.reset();
			totalWriteSTDOUTTime.reset();
			totalWaitTime.reset();
			totalBusyTime.reset();

			// set next time for service:
			nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
		}
	}while(1);
	
	perror("RX: PANIC! Exit While 1\n");
    return 1;
}