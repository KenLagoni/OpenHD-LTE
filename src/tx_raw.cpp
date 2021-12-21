#include "tx_raw.h"

int flagHelp = 0;

typedef struct {
	float mavlinktx;
	float mavlinkrx;
	float mavlinkdropped;
	float videotx;
	float videodropped;
} tx_dataRates_t;

void usage(void) {
    printf("\nUsage: [Video stream] | tx_raw [options]\n"
           "\n"
           "Options:\n"
           "-i  <ip>       Ip to which the stream is sent. This is where the rx_raw is listening\n"
           "-v  <port>     UDP port for video.\n"
           "-s  <serial>   Serial device to listen for Mavlink packages from Flight Computer\n"
		   "-p  <port>     UDP port for serial data output.\n"
		   "-t  <port>     Port for Telemetry data.\n"
           "-o  <path>     Output path for recorded video stream\n"
		   "-d  <debug>    1=always record\n"
           "\n"
           "Example:\n"
           "  raspvid -t 0 | ./tx_raw -i X.X.X.X -v 12000 -s /dev/serial0 -p 12001 -o /home/pi \n"
		   "  raspvid -t 0 | ./tx_raw -i X.X.X.X -v 12000 -s /dev/serial0 -p 12001 -t 12002 -o /home/pi \n"
           "\n");
    exit(1);
}

int open_port(const char* serialDevice){
	int fd;		//File descriptor for the port
	struct termios options;

	fd = open(serialDevice, O_RDWR | O_NOCTTY | O_NDELAY);
//	fd = open(serialDevice, O_RDWR );

	if (fd == -1){
		//Could not open the port.
		fprintf(stderr, "tx_raw: Serial Port Failed to Open, exit");
		exit(EXIT_FAILURE);
	}
	else{
		fcntl(fd, F_SETFL, FNDELAY); // Sets the read() function to return NOW and not wait for data to enter buffer if there isn't anything there.

		//Configure port for 8N1 transmission
		tcgetattr(fd, &options);					//Gets the current options for the port
		cfsetispeed(&options, B57600);				//Sets the Input Baud Rate
		cfsetospeed(&options, B57600);				//Sets the Output Baud Rate
		options.c_cflag |= (CLOCAL | CREAD);		// Enable reciever
		options.c_cflag &= ~PARENB;					// Set parry bit
		options.c_cflag &= ~CSTOPB;					// 1 Stop Bit
		options.c_cflag &= ~CSIZE;					// 8 Data bits
		options.c_cflag |= CS8;						// --||---
	    options.c_iflag &= ~(IXON | IXOFF | IXANY); // disable flow control (software)

		tcsetattr(fd, TCSANOW, &options);			//Set the new options for the port "NOW"

		//std::cout << "seems like everything is ok, keep going\n";
	};

	return (fd);
};

// Global used function to read STDIN or serial and only returns number of bytes to handle. On error exit here with error message.
int readData(int fd, void *input, int size){
	int result=0, err=0;
	result = read(fd, input, size); 
	err = errno; 			
	//printf("Read result:%d\n\r", n);
	if (result < 0 || result > size) {
		if ((err == EAGAIN) || (err == EWOULDBLOCK))
		{
			fprintf(stderr, "tx_raw: None blocking  - nothing to read?.\n"); // ?
		}else{
			if(err == EBADF){
				fprintf(stderr, "tx_raw: The argument sockfd is an invalid file descriptor.\n\r");	 
			}else if(err == ECONNREFUSED){
				fprintf(stderr, "tx_raw: A remote host refused to allow the network connection.\n\r");
			}else if(err == EFAULT){
				fprintf(stderr, "tx_raw: The receive buffer pointer(s) point outside the process's address space.\n\r");
			}else if(err == EINTR){
				fprintf(stderr, "tx_raw: The receive was interrupted by delivery of a signal before any data was available.\n\r");
			}else if(err == EINVAL){
				fprintf(stderr, "tx_raw: Invalid argument passed.\n\r");
			}else if(err == ENOMEM){
				fprintf(stderr, "tx_raw: Could not allocate memory for recvmsg().\n\r");
			}else if(err == ENOTCONN){
				fprintf(stderr, "tx_raw: The socket is associated with a connection-oriented protocol and has not been connected.\n\r");
			}else if(err == ENOTSOCK){
				fprintf(stderr, "tx_raw: The file descriptor sockfd does not refer to a socket.\n\r");
			}else if(err == -1){
				fprintf(stderr, "tx_raw: Socket invalid, thus closing file descriptor.\n\r");
			}else{
				fprintf(stderr, "tx_raw: Unknown error - reading with result=%d and errno=%d, thus closing\n",result,err);
			}	 			
		}
		fprintf(stderr,"tx_raw: failed in file %s at line # %d - ", __FILE__,__LINE__);
		exit(EXIT_FAILURE);
	}else if(result == 0){
		// None blocking, nothing to read.
	}
	return result;
}

// Global used function to write STDIN or serial and only returns number of bytes to handle. On error exit here with error message.
int writeData(int fd, void *input, int size){
	int result=0, err=0;
	result = write(fd, input, size); 
	err = errno; 

	if (result < 0){
		fprintf(stderr,"tx_raw: Error writing to Serial: %s \n", strerror(err));
		return 0;
	} 

	return result;
}


// Timers for perpormance analysis.
Timer totalVideoReadTime,totalVideoParseTime, totalVideoSendTime;
Timer totalSerialReadTime,totalSerialParseTime, totalSerialSendTime;
Timer totalRecorderParseTime;
Timer totalWaitTime,totalBusyTime;
double logTimer;
Timer totalLogServiceTime;

Timer oneTimer;
Timer twoTimer;
Timer threeTimer;


int main(int argc, char *argv[]) {
//    setpriority(PRIO_PROCESS, 0, -10);

	//char *p;
	// Input settings.	
	char *targetIp;
	int udpVideoPort=0;
	char *serialDevice;
	int udpSerialPort=0;
	char *outputPath;
	int telemetryPort=0;
	int debug=0;
	printf("Starting tx_raw program (c)2021 by Lagoni.\n");

    while (1) {
        int nOptionIndex;
        static const struct option optiona[] = {
            { "help", no_argument, &flagHelp, 1 },
            {      0,           0,         0, 0 }
        };
        int c = getopt_long(argc, argv, "h:i:v:s:p:t:o:d:", optiona, &nOptionIndex);
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
            case 'i': {
//				sprintf(targetIp,"%s",optarg);
//				targetIp = optarg;
				targetIp=optarg;
//				fprintf(stderr, "ip            :%s\n",targetIp);
                break;
            }
			
            case 'v': {
	            udpVideoPort = atoi(optarg);
//				fprintf(stderr, "Video port    :%d\n",udpVideoPort);
	            break;
            }

            case 's': {
				serialDevice = optarg;
//				fprintf(stderr, "Serial device :%s\n",serialDevice);
	            break;
            }


            case 'p': {
	            udpSerialPort = atoi(optarg);
//				fprintf(stderr, "Serial Port   :%d\n",udpSerialPort);
	            break;
            }

			case 't': {
				telemetryPort = atoi(optarg);
				break;
			}			

            case 'o': {
	            outputPath = optarg;
//				fprintf(stderr, "Output File   :%s\n",outputPath);				
	            break;
            }

            case 'd': {
	            debug = atoi(optarg);
//				fprintf(stderr, "Output File   :%s\n",outputPath);				
	            break;
            }

            default: {
                fprintf(stderr, "tx_raw: Unknown input switch %c\n", c);
                usage();

                break;
            }
        }
    }

    if (optind > argc) {
        usage();
    }

	Connection videoToBaseConnection(targetIp,udpVideoPort, SOCK_DGRAM, O_NONBLOCK); // UDP None blocking	
	Connection serialToBaseConnection(targetIp,udpSerialPort, SOCK_DGRAM, O_NONBLOCK); // UDP None blocking
	
	// For UDP Sockets
	int nready;
	ssize_t n;
	
	// For Serial:
	char configCmd[64];
	sprintf(configCmd,"stty -F %s %d raw -echo",serialDevice,SERIAL_BAUDRATE);
	//system("stty -F /dev/serial0 57600 raw -echo");
	system(configCmd);

	int Serialfd = open_port(serialDevice);
	mavlinkHandler mavlinkDataToGround;
	mavlinkHandler mavlinkDataToFC;

	// For UDP mavlink from ground:
	//uint8_t buffer[BUFFER_SIZE]; // BUFFER_SIZE defined in h264 UDP size (1400)
	 uint8_t buffer[65535];
	
	// STDIN video pipe
	fcntl(STDIN_FILENO, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK); // Det STDIN to nonblocking.
	H264Recorder videoRecorder(outputPath);

	// TX video:
	static H264TXFraming TXpackageManager; // Needs to be static so it is not allocated on the stack, because it uses 8MB.

	// For select usages.
	fd_set read_set;
	int maxfdp1;
	struct timeval timeout;

	// For link status:
	tx_dataRates_t linkstatus;
	bzero(&linkstatus, sizeof(linkstatus));
	time_t nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
	
	// For Telemetry (CPU temp / load)
	telemetryWrapper TXtelemetry;
	Connection telemetryToBaseConnection(targetIp,telemetryPort, SOCK_DGRAM, O_NONBLOCK); // UDP None blocking
	//air_status_t data;
	
	do{
		FD_ZERO(&read_set);
		
		// file dessriptors
		// 
		// Serialfd - Data from serial port "/dev/serial0" which should be sent to ground.
		// STDIN_FILENO - Input video from STDIN which should be sent to ground.
		// serialToBaseConnection.getFD() - Data from ground which should be written to Flight contontroller (Serial)
		
		// finding the max filedescriptor
		maxfdp1 = max(STDIN_FILENO, Serialfd); // Video Input on STDIN and Serial Input on /dev/serial0
		maxfdp1 = max(serialToBaseConnection.getFD(), maxfdp1); // Data on serial UDP connection.
		maxfdp1 = max(videoToBaseConnection.getFD(), maxfdp1);  // Data on video UDP connection.

		// Set the FD_SET on the filedesscriptors.
		FD_SET(Serialfd, &read_set);
		FD_SET(STDIN_FILENO, &read_set);
		serialToBaseConnection.setFD_SET(&read_set);
		videoToBaseConnection.setFD_SET(&read_set); // for header request.

		timeout.tv_sec = 0;
		timeout.tv_usec = 1000; // 1ms	
		

		totalBusyTime.stop();
		totalWaitTime.start();
	    nready = select(maxfdp1+1, &read_set, NULL, NULL, &timeout);  // blocking
		totalWaitTime.stop();
		totalBusyTime.start();

		
		if (FD_ISSET(Serialfd, &read_set)) { // Data from serial port.
//			printf("Data from Serial port!\n\r");
			int result=0;
			
			//number of byets to handle = readData(int fd, void *input, int size);
			totalSerialReadTime.start();
			result = readData(Serialfd, buffer, sizeof(buffer));
			totalSerialReadTime.stop();
			
			// Parse buffer to mavlink class.
			if(result > 0){
				totalSerialParseTime.start();
				mavlinkDataToGround.inputData(buffer, result);
				totalSerialParseTime.stop();
			}

		}
		


		// Lets see if there are any Mavlink data from ground to Flight controller:	
		if (FD_ISSET(serialToBaseConnection.getFD(), &read_set)){ // Mavlink data from UDP(ground) to serial port.
			oneTimer.start();
//			printf("Data from Serial port!\n\r");
			int result=0;
			
			//number of byets to handle = readData(int fd, void *input, int size);
			result = serialToBaseConnection.readData(buffer, sizeof(buffer));
			
			// Parse buffer to mavlink class.
			if(result > 0){
				mavlinkDataToFC.inputData(buffer, result);
			}
			oneTimer.stop();
		}
		
		
		// Read from STDIN (Video pipe)
		if (FD_ISSET(STDIN_FILENO, &read_set)) { // Data from from STDIN (Video pipe)
//			printf("Data from STDIN!\n\r");
			int result = 0;
			totalVideoReadTime.start();
			result = readData(STDIN_FILENO, buffer, sizeof(buffer));
			totalVideoReadTime.stop();

			// Parse buffer to video class.
			if(result > 0){
				totalVideoParseTime.start();
				TXpackageManager.inputStream(buffer, (uint16_t)result); // parse data for UDP transmission
				totalVideoParseTime.stop();

				totalRecorderParseTime.start();
//				videoRecorder.inputStream(buffer, (uint16_t)result); // parse data for video recording.
				totalRecorderParseTime.stop();
			}
		}
		

		
		// Read from videoToBaseConnection (Video UDP return) - is keep alive or header request.
		if (FD_ISSET(videoToBaseConnection.getFD(), &read_set)) { // Data from video UDP, this is keep alive or header request.
			twoTimer.start();
			int result = 0;
			result = videoToBaseConnection.readData(buffer, sizeof(buffer));

			if(result >= 6){
				if( (buffer[0]==0x10) && (buffer[1]==0x11) && (buffer[2]==0x12) && (buffer[3]==0x13) && (buffer[4]==0x14) && (buffer[5]==0x15) ){
					// header requist:
					printf("Ground has requested the header - thus sending it\n");
					TXpackageManager.reTransmitHeader();
				}	
			}
			twoTimer.stop();
		}	
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////// All below this line is checked every time and timeout will force program to come by. ////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Here we shall handle Transmit of Serial...
		bool sendVideo=false; // by default video is not allowed to be transmitted, only when no serial data is available can video be transmitted.

		if( mavlinkDataToGround.outputDataHasMSG30() ){ // lets sendt all data
			bool moreData=false;
			bool transmisionFailed=false;
			totalSerialSendTime.start();
			do{
				int bytesReady=0;
				int result=0;
				bytesReady = mavlinkDataToGround.getData(buffer, sizeof(buffer));
				if(bytesReady > 0 ){
					result = serialToBaseConnection.writeData(buffer,bytesReady);
					if(result!=bytesReady){ // Failed to transmitted all data.
						transmisionFailed=true;
					}else{
						moreData=true;
					}
				}else{
					moreData=false;
					sendVideo=true; // send video because we don't have any more serial data
				}
			}while(moreData && !transmisionFailed );
			totalSerialSendTime.stop();
		}else{
			sendVideo=true; // no serial data ready for TX, thus sendVideo.
		}

		// Can we sent video?
		if(sendVideo){
			uint8_t *data; // pointer for output data.
			uint16_t size=0;
			int result = 0;			
			double sendttime=0;						
			totalVideoSendTime.start();

			do{				
				size = TXpackageManager.getTXPackage(data);						
				
				if(size>0){

					result = videoToBaseConnection.writeData(data, size);
					
					if(result == size){
						//fprintf(stderr, "Ok!\n");
						TXpackageManager.nextTXPackage();	
					}else{
						//fprintf(stderr, "Error! result(%u) != size(%u)\n", result, size);
						sendVideo=false;
						if(result > 0){ // not all was transmitted, this is not good for UDP
							fprintf(stderr, "tx_raw: Error! on video tx. Bytes to be sent(%u) is lower than bytes transmitted(%u).\n",size, result);
						}
					}
				}else{
					sendVideo=false;
				}
			}while(sendVideo);
			totalVideoSendTime.stop();
		}
		
		//Mavlink to FC from ground?
		if(mavlinkDataToFC.getFIFOsize() > 0){ // If there are any Mavlink msg then sent them to FC
			bool moreData=false;
			bool transmisionFailed=false;
			do{
				int bytesReady=0;
				int result=0;
				bytesReady = mavlinkDataToFC.getData(buffer, sizeof(buffer));
				if(bytesReady > 0 ){
					result = writeData(Serialfd, buffer, sizeof(buffer));
					if(result!=bytesReady){ // Failed to transmitted all data.
						transmisionFailed=true;
					}else{
						moreData=true;
					}
				}else{
					moreData=false;
				}
			}while(moreData && !transmisionFailed );
		}
		

		//Only run on timeout
	//	if(nready == 0){	
			// check if it is time to log the status:
			if(time(NULL) >= nextPrintTime){		
				totalLogServiceTime.start();
				linkstatus.videodropped=TXpackageManager.getBytesDropped();
				linkstatus.videotx=TXpackageManager.getBytesOutputted();
				TXpackageManager.clearIOstatus();

				fprintf(stderr, "TX: Status: ");
				fprintf(stderr, "Timing[ms] Serial:(Read|Parse|Send)  %4.0lf | %4.0lf | %4.0lf  ", totalSerialReadTime.getTotalDuration(), totalSerialParseTime.getTotalDuration(),totalSerialSendTime.getTotalDuration() );
				fprintf(stderr, "Video:(Read|Parse|Send)  %4.0lf | %4.0lf | %4.0lf  ", totalVideoReadTime.getTotalDuration(), totalVideoParseTime.getTotalDuration(), totalVideoSendTime.getTotalDuration() );
				fprintf(stderr, "Recorder:(Parse)  %4.0lf  ", totalRecorderParseTime.getTotalDuration() );
				fprintf(stderr, "LOG:(service)  %4.0lf  ", logTimer);
				fprintf(stderr, "Loop: (Waiting|Busy):  %4.0lf | %4.0lf  ", totalWaitTime.getTotalDuration(),totalBusyTime.getTotalDuration());		
//				fprintf(stderr, "Buffers(Input|tempOutput|Output):  %5u|%5u|%5u     ", RXpackageManager.getInputQueueSize(), RXpackageManager.getTempOutputSize(), RXpackageManager.getBufferSize());	
//				fprintf(stderr, "UDP Packages: (rx|dropped|outputted):  %*.2fKB  |  %*.2fKB  |  %*.2fKB     ", 7, linkstatus.rx/1024 , 7, linkstatus.dropped/1024 , 7 , linkstatus.tx/1024);
//				fprintf(stderr, " (%u)", RXpackageManager.waitingForHeader());
				fprintf(stderr, "(1|2): %4.0lf | %4.0lf  ", oneTimer.getTotalDuration(),twoTimer.getTotalDuration());	
				if(mavlinkDataToGround.isArmed()){			
					fprintf(stderr,"  FC=ARMED");							
				}else{
					fprintf(stderr, "  FC=DISARMED");							
				}		
				fprintf(stderr, "\n");

				// reset counters.
				totalSerialReadTime.reset(); totalSerialParseTime.reset(); totalSerialSendTime.reset();
				totalVideoReadTime.reset(); totalVideoParseTime.reset(); totalVideoSendTime.reset();
				totalRecorderParseTime.reset();
				totalWaitTime.reset(); totalBusyTime.reset();
				oneTimer.reset(); twoTimer.reset();
				

/*
				printf("%d tx_raw: Status:            Mavlink: (tx|rx|dropped):  %*.2fKB  |  %*.0fB  | %*.2fKB            Video: (tx|dropped)  %*.2fMB  | %*.2fMB ", time(NULL), 6, linkstatus.mavlinktx/1024 , 6, linkstatus.mavlinkrx , 6 , linkstatus.mavlinkdropped/1024, 6, linkstatus.videotx/(1024*1024), 6 ,linkstatus.videodropped/(1024*1024));
				if(mavlinkDataToGround.isArmed()){			
					printf("   FC=ARMED");							
				}else{
					printf("   FC=DISARMED");							
				}	
*/																																			 
				bzero(&linkstatus, sizeof(linkstatus));
				
				
				
		        TXtelemetry.setCPULoad(TXtelemetry.calculateCPULoadFromLinux());
				TXtelemetry.setCPUTemperature(TXtelemetry.readCPUTempFromLinux());
				TXtelemetry.setRSSI(TXtelemetry.readRSSIFromLinux());

//				printf("   CPU Load: %3d%%     CPU Temp: %3dC     LTE-RSSI: %3d[dBm]\n",TXtelemetry.getCPULoad(),TXtelemetry.getCPUTemperature(), TXtelemetry.getRSSI());			
				
				//telemetryData
				// Send telemetry on port:
				telemetryToBaseConnection.writeData(TXtelemetry.getTXTelemetryAsArray(),TXtelemetry.getTXTelemetryArraySize());		
				
				if( (mavlinkDataToGround.isArmed()) || (debug==1) ){
					videoRecorder.start();
				}else{
					videoRecorder.stop();
				}

				nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
				totalLogServiceTime.stop();
				logTimer=totalLogServiceTime.getTotalDuration();
				totalLogServiceTime.reset();
			}
	//	}

	}while(1);

	fprintf(stderr, "tx_raw: Panic!!!\n\n");
	exit(EXIT_FAILURE);	
}

