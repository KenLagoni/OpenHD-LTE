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
           "-o  <file>     Output file to local record of input stream, .h264 will be added to the name\n"
           "-z  <Mbytes>   Maximum allowed output file size, on FAT32 2000 should be used. Next file will be same filename as -o but 1..N added.\n"
           "\n"
           "Example:\n"
           "  raspvid -t 0 | ./tx_raw -i X.X.X.X -v 7000 -s /dev/serial0 -p 8000 -o record -z 2000\n"
		   "  raspvid -t 0 | ./tx_raw -i X.X.X.X -v 7000 -s /dev/serial0 -p 8000 -t 5200 -o record -z 2000\n"
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
/*
uint64_t timeMillisec() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}*/

float getCpuTemp(void){
	float systemp, millideg;
	FILE *thermal;
	int n;

	thermal = fopen("/sys/class/thermal/thermal_zone0/temp","r");
	n = fscanf(thermal,"%f",&millideg);
	fclose(thermal);
	systemp = millideg / 1000;	
	return systemp;	
}


int main(int argc, char *argv[]) {
//    setpriority(PRIO_PROCESS, 0, -10);

	//char *p;
	// Input settings.	
	char *targetIp;
	int udpVideoPort=0;
	char *serialDevice;
	int udpSerialPort=0;
	char *outputFile;
	long maxFileSize=0;
	int telemetryPort=0;
	printf("Starting tx_raw program v0.20 (c)2021 by Lagoni. Not for commercial use\n");
//	fprintf(stderr, "Inputs are:\n");

    while (1) {
        int nOptionIndex;
        static const struct option optiona[] = {
            { "help", no_argument, &flagHelp, 1 },
            {      0,           0,         0, 0 }
        };
        int c = getopt_long(argc, argv, "h:i:v:s:p:o:z:t:", optiona, &nOptionIndex);
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
	            outputFile = optarg;
//				fprintf(stderr, "Output File   :%s\n",outputFile);				
	            break;
            }

            case 'z': {
				
//				fprintf(stderr, "input:%s\n",optarg);
				int in = atoi(optarg);
	            maxFileSize = ((long)in)*1024*1024; // in bytes.
//				fprintf(stderr, "Max Size      :%d\n",maxFileSize);
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

	 if(maxFileSize == 0){
		 maxFileSize=DEFAULT_MAX_VIDEO_FILE_SIZE;
	 }

//	Connection videoToBaseConnection(targetIp,udpVideoPort, SOCK_DGRAM, O_NONBLOCK); // UDP None blocking	
	Connection videoToBaseConnection(targetIp,udpVideoPort, SOCK_STREAM, O_NONBLOCK); // TCP None blocking	
	Connection serialToBaseConnection(targetIp,udpSerialPort, SOCK_DGRAM, O_NONBLOCK); // UDP None blocking
	
	// For UDP Sockets
	uint8_t rxBuffer[MAXLINE];
	int nready;
	ssize_t n;
	
	//Mavlink parser and serial:
	mavlink_status_t status;
	mavlink_message_t msg;
	int chan = MAVLINK_COMM_0;
	RingBuf<mavlink_message_t, FIFO_SIZE> serialRxFIFO;

	// For Serial:
	char configCmd[64];
	sprintf(configCmd,"stty -F %s %d raw -echo",serialDevice,SERIAL_BAUDRATE);
	system("stty -F /dev/serial0 57600 raw -echo");
	int Serialfd = open_port(serialDevice);
	telematryFrame_t telemetryData;
	bzero(&telemetryData, sizeof(telemetryData));
		
	uint16_t totalSize = 0; // number of bytes in mavlink fifo
	uint8_t serialBuffer[MAX_SERIAL_BUFFER_SIZE];
	uint16_t serialBufferSize = 0;
	bool serialDataToSend=false; // Indicates when serial data are to be transmitted and also blocking video streaming.
	
	// For UDP mavlink from ground:
	uint8_t inputBuffer[MAX_SERIAL_BUFFER_SIZE];
	uint16_t inputBufferSize =0;	
	
	// STDIN video pipe
	char videoData[MAXLINE];
	bzero(&videoData, sizeof(videoData));
	fcntl(STDIN_FILENO, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK); // Det STDIN to nonblocking.
	uint32_t videoRecordFileSize = 0; // Don't record more than 2*1024*1024*1024 bytes = 
	char videoBuffer[MAX_VIDEO_BUFFER_SIZE]; // Only write to file when 1M has been inputted.
	uint32_t videoBufferSize=0;
	
	// Record file:
	uint8_t fileNumber = 0;
	char filename[14];
	sprintf(filename,"%s%d.h264",outputFile,fileNumber);
	std::ofstream* videoRecordFile = new std::ofstream(filename,std::ofstream::binary);
	bool armed=false; // Only recored when armed!.

	// TX video:
	RingBuf<videoFrame_t, VIDEO_TX_BUFFER_SIZE> videoBufferTx;// 2MB TX buffer
	videoFrame_t videoTxBuffer;
	videoFrame_t bufdata;
	int videoTxRetry=0;

	// For select usages.
	fd_set read_set;
	int maxfdp1;
	struct timeval timeout;

	// For link status:
	tx_dataRates_t linkstatus;
	bzero(&linkstatus, sizeof(linkstatus));
	time_t nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
	
	// For Telemetry (CPU temp / load)
	Connection telemetryToBaseConnection(targetIp,telemetryPort, SOCK_DGRAM, O_NONBLOCK); // UDP None blocking
	long double a[4], b[4]; // for Cpuload calculations
	air_status_t data;
	
	do{
		FD_ZERO(&read_set);
		
		// file dessriptors
		// 
		// Serialfd - Data from serial port "/dev/serial0" which should be sent to ground.
		// STDIN_FILENO - Input video from STDIN which should be sent to ground.
		// serialToBaseConnection.getFD() - Data from ground which should be written to Flight contontroller (Serial)
		
		// finding the max filedescriptor
		maxfdp1 = max(STDIN_FILENO, Serialfd);
		maxfdp1 = max(serialToBaseConnection.getFD(), maxfdp1);

		// Set the FD_SET on the filedesscriptors.
		FD_SET(Serialfd, &read_set);
		FD_SET(STDIN_FILENO, &read_set);
		serialToBaseConnection.setFD_SET(&read_set);

		// Set timeout (Retry to re-transmit or idle)
		if(videoTxRetry > 0){ // we have video to re-transmit, thus short timeout
			timeout.tv_sec = 0;
			timeout.tv_usec = 1000; // 1ms	
		}else{
			timeout.tv_sec = 0;
			timeout.tv_usec = 10000; // 10ms
		}
	    nready = select(maxfdp1+1, &read_set, NULL, NULL, &timeout);  // blocking

		
		if (FD_ISSET(Serialfd, &read_set)) { // Data from serial port.
//			printf("Data from Serial port!\n\r");
			int result=0;
			result = read(Serialfd, rxBuffer, sizeof(rxBuffer));  // read up to 100 characters if ready to read	
		
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > MAX_SERIAL_BUFFER_SIZE) {
				fprintf(stderr, "tx_raw: Error! on reading STD_IN (pipe input)... Terminate program.\n");
				exit(1);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else {
				uint16_t index=0;
				while(result>0){
					result--;
					uint8_t byte=rxBuffer[index];
					index++;
					if (mavlink_parse_char(chan, byte, &msg, &status)){
						// printf("MSG ID#%d\n\r",msg.msgid);
						// MSG ID 30 (HUD 10HZ) mean transmit now!
						
						// if fifo is larger than 1024 bytes or MSG 30 har ben received, then transmit.
						if(!serialRxFIFO.isFull()){
							serialRxFIFO.push(msg);
							totalSize = totalSize + msg.len;
						}else{
							uint16_t fifoSize=serialRxFIFO.size();
							printf("tx_raw: serialRxFIFO (Mavlink msg) size:%d is full!\n",fifoSize);
						}
						
						if(msg.msgid == 30 || totalSize > 512){ // time to send UDP frame if MSG 30 (HUD) or 512 bytes has been reached!
							uint8_t fifoSize = serialRxFIFO.size();
							//	printf("\n\r \n\r \n\rTime to transmit! - mavlink FIFO has %d elements with total size of %d\n\r", fifoSize, totalSize);
							mavlink_message_t data;
							
							if(true == serialDataToSend){
								//printf("tx_raw: Discharding %d bytes of serial data\n\r",serialBufferSize);
								linkstatus.mavlinkdropped += serialBufferSize;
							}
							
							serialBufferSize=0;
							
							for(int a=0;a<fifoSize;a++){
								if(!serialRxFIFO.isEmpty()){
									serialRxFIFO.pop(data);
									//printf("FIFO index %d has MSG ID %d\n\r",a,data.msgid);
									serialBufferSize += mavlink_msg_to_send_buffer(&serialBuffer[serialBufferSize], &data);
									//memcpy(&serialBuffer[serialBufferSize], &data., result); // copy input to buffer.
								}
							}

							//						printf("Done reading, FIFO size is now %d\n\r",serialRxFIFO.size());
							totalSize = 0; // clear the MSG FIFO
							serialDataToSend = true;
						}
						
						// Keep track on ARM / DISARMED for recording purporse. Status can be found in HEARTBEAT (MSG=0) from FC:
						if(msg.msgid == 0){
							mavlink_heartbeat_t newmsg;
							mavlink_msg_heartbeat_decode(&msg, &newmsg);
							armed = newmsg.base_mode & MAV_MODE_FLAG_SAFETY_ARMED;
						}
						
					}
				}
			}
		}
		
	
		// Lets see if there are any Mavlink data from ground to Flight controller:	
		if (FD_ISSET(serialToBaseConnection.getFD(), &read_set)) { // Data from serial port.
//			printf("Data from Ground (Mavlink)!\n\r");
			int result = 0;
			result = serialToBaseConnection.readData(inputBuffer, MAX_SERIAL_BUFFER_SIZE);
			if (result < 0 || result > MAX_SERIAL_BUFFER_SIZE) {
				fprintf(stderr, "tx_raw: Error! on reading STD_IN (pipe input)... Terminate program.\n");
				exit(1);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else {
				// printf("Data from ground!\n\r");
				int res = 0;
				res = write(Serialfd, inputBuffer, result);	
				if (res < 0 || res > MAXLINE) {
					fprintf(stderr, "tx_raw: Error! sending serial to flight controller (UDP from ground)... Terminate program.\n");
					exit(1);
				}
				linkstatus.mavlinkrx += res;
			}		
		}
		
		
		
		// Read from STDIN (Video pipe)
		if (FD_ISSET(STDIN_FILENO, &read_set)) { // Data from from STDIN (Video pipe)
//			printf("Data from STDIN!\n\r");
			int result = 0;
			result = read(STDIN_FILENO, videoData, MAXLINE);	

			if (result < 0 || result > MAXLINE) {
				fprintf(stderr, "tx_raw: Error! on reading STD_IN (pipe input)... Terminate program.\n");
				exit(1);
			}else  if(result == 0){
				// EOF
				//fprintf(stderr, "tx_raw: Warning! Lost connection to stdin. Please make sure that a data source is connected\n");
			}else {	
				//printf("Writing %d bytes to Videobuffer\n\r", result);
				// write to outfile
				memcpy(&videoBuffer[videoBufferSize], videoData, result); // copy input to buffer for disk write.

				if(!videoBufferTx.isFull()){
					memcpy(&videoTxBuffer.data, videoData, result); // copy input to buffer for TX write.
					videoTxBuffer.len=result;
					videoBufferTx.push(videoTxBuffer);
				}else{
					printf("tx_raw: Warning! - Video TX buffer is full, lets clear it!\n");//Buffer full, reset!
					videoBufferTx.clear();
					linkstatus.videodropped += VIDEO_TX_BUFFER_SIZE;					
				}
				
				videoBufferSize += result;
				//printf("adding %d bytes to Videofile\n\r", videoBufferSize);				
			
				if(videoBufferSize > VIDEO_BUFFER_WRITE_THRESHOLD){ // Time to write video buffer to file
	//				printf("Writing %d bytes to Videofile\n\r", videoBufferSize);	
//					if(true==armed){
//					if(false){
						videoRecordFile->write(videoBuffer,videoBufferSize);
						videoRecordFileSize += videoBufferSize;				
//					}
					videoBufferSize=0;

					if(videoRecordFileSize > maxFileSize){ // time to change file
						fileNumber++;
						printf("tx_raw: Videofile %s size is now %d MB which is larger than maxFileSize: %d MB thus switching to next file ", filename, videoRecordFileSize/(1024*1024), maxFileSize/(1024*1024));				
						sprintf(filename,"%s%d.h264",outputFile,fileNumber);
						printf("%s\n",filename);
						videoRecordFile->close();
						delete videoRecordFile;
						videoRecordFile = new std::ofstream(filename,std::ofstream::binary);
						videoRecordFileSize=0;
					}
				}					
			}
		}
		
		// All below this line is checked every time and timeout will force program to come by.

		// Here we shall handle Transmit of Serial...
		if(true == serialDataToSend){
			//			printf("Sending %d bytes back to base...",serialBufferSize);
			int result=0;
			result = serialToBaseConnection.writeData(serialBuffer,serialBufferSize);
			if(result < 0){ // Error
				fprintf(stderr, "tx_raw: Error! on UDP serial socket write... Terminate program.\n");
				exit(1);
			}else if(result == 0){
				//printf("Unable to send video\n\r", videoBufferTx.size());
			}else{	
				// We where able to send UDP data, thus clear buffer and flag
				linkstatus.mavlinktx += serialBufferSize;
				serialBufferSize=0;
				serialDataToSend=false;
			}
		}else{
			// No serial data is pending, lets try and send some video frame if needed:
			bool SendVideo = false;
			
			if(videoTxRetry > 0){ // retry to send the frame in bufdata:
				SendVideo = true;
			}else if(!(videoBufferTx.isEmpty())){ // Make new data ready for TX
				videoBufferTx.pop(bufdata);
				SendVideo = true;
			}

			if(true == SendVideo){
				int result=0;
				
//				fprintf(stderr, "tx_raw: Header: %d\n",bufdata.header );
//				fprintf(stderr, "tx_raw: Length %d\n",bufdata.len );
				result = videoToBaseConnection.writeData(bufdata.data, bufdata.len);
				
				if(result < 0){ // Error
					fprintf(stderr, "tx_raw: Error! on video socket write.. Terminate program.\n");
					exit(1);
				}else if(result == 0){ // dropping frame
					if(videoTxRetry < VIDEO_RETRY_ATTEMPTS){
						videoTxRetry++;
					}else{ // drop frame and reset retry counter:
						//printf("Unable to send video\n\r", videoBufferTx.size());
						linkstatus.videodropped += bufdata.len;
						videoTxRetry=0;
					}
				}else{
					videoTxRetry=0; // completed sending frame.
					linkstatus.videotx += result; // counting b.
					if( !(videoBufferTx.isEmpty()) ){
					//	printf("tx_raw: After successfully transmitting one video frame, there is still %d frames left in buffer.\n\r", videoBufferTx.size());
					}
				}
			}
		}
		
		//Only run on timeout
		if(nready == 0){	
			// check if it is time to log the status:
			if(time(NULL) >= nextPrintTime){		
				printf("%d tx_raw: Status:            Mavlink: (tx|rx|dropped):  %*.2fKB  |  %*.0fB  | %*.2fKB            Video: (tx|dropped)  %*.2fMB  | %*.2fMB ", time(NULL), 6, linkstatus.mavlinktx/1024 , 6, linkstatus.mavlinkrx , 6 , linkstatus.mavlinkdropped/1024, 6, linkstatus.videotx/(1024*1024), 6 ,linkstatus.videodropped/(1024*1024));
//				printf("%llu tx_raw: Status:            Mavlink: (tx|rx|dropped):  %*.2fKB  |  %*.0fB  | %*.2fKB            Video: (tx|dropped)  %*.2fMB  | %*.2fKB ", timeMillisec(), 6, linkstatus.mavlinktx/1024 , 6, linkstatus.mavlinkrx , 6 , linkstatus.mavlinkdropped/1024, 6, linkstatus.videotx/(1024*1024), 8 ,linkstatus.videodropped/1024);
				if(true==armed){			
					printf("   FC=ARMED");							
				}else{
					printf("   FC=DISARMED");							
				}																																				 
				bzero(&linkstatus, sizeof(linkstatus));
				nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
				
		        
/*								
				fp = fopen("/proc/stat", "r");
				fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &a[0], &a[1], &a[2], &a[3]);
				fclose(fp);
	*/			
		        FILE *fp;				
				fp = fopen("/proc/stat", "r");
				fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &b[0], &b[1], &b[2], &b[3]);
				fclose(fp);				
			
				telemetryData.cpuLoad = (((b[0] + b[1] + b[2]) - (a[0] + a[1] + a[2])) / ((b[0] + b[1] + b[2] + b[3]) - (a[0] + a[1] + a[2] + a[3]))) * 100;
				// move current cpu time to last cpu time.
				a[0]=b[0];
				a[1]=b[1];
				a[2]=b[2];
				a[3]=b[3];
				
				telemetryData.cpuTemp=getCpuTemp();
				printf("   CPU Load: %3d%%     CPU Temp: %3dC\n",telemetryData.cpuLoad,telemetryData.cpuTemp);			
			//	fprintf(stderr, "tx_raw: CPU load:%d CPU temperatur:%d\n",telemetryData.cpuLoad,telemetryData.cpuTemp);
				
				//telemetryData
				// Send telemetry on port:
				telemetryToBaseConnection.writeData(&telemetryData,sizeof(telemetryData));		
			}
		}
	}while(1);

	fprintf(stderr, "tx_raw: Panic!!!\n\n");
	exit(1);

}

