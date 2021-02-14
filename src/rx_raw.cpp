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
	"Example:\n"
	"  ./rx_raw -v 7000 -m 12000 -t 5200 -i 192.168.0.67 -r 14550\n"
	"\n");
	exit(1);
}

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

	fprintf(stderr, "Starting Lagoni's UDP RX program v0.30\n");


	// For relay
	Connection *relayConnection = NULL;
	if(relayPort != 0){
		relayConnection = new Connection(relayIP, relayPort, SOCK_DGRAM); // blocking.
	}else{
		relayConnection = new Connection(); // Empty.
	}
		
	// For UDP/TCP Sockets
	Connection inputVideoConnection(videoPort, SOCK_DGRAM); // UDP port
	Connection outputVideoConnection("127.0.0.1", OUTPUT_VIDEO_PORT, SOCK_DGRAM); 

	// For UDP/TCP Sockets
	Connection inputMavlinkConnection(mavlinkPort, SOCK_DGRAM); // UDP port
	Connection outputMavlinkConnection("127.0.0.1", OUTPUT_MAVLINK_PORT, SOCK_DGRAM);
	
	// For UDP/TCP Sockets
	Connection inputTelemetryConnection(telemetryPort, SOCK_DGRAM); // UDP port
	Connection outputTelemetryConnection("127.0.0.1", OUTPUT_TELEMETRY_PORT, SOCK_DGRAM);
				
	uint8_t rxBuffer[RX_BUFFER_SIZE];
	int nready, maxfdp1; 
	fd_set rset; 
	struct timeval timeout; // select timeout.

	// For link status:
	rx_dataRates_t linkstatus;
	bzero(&linkstatus, sizeof(linkstatus));
	time_t nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;

	rx_status_t telmetryData;
	bzero(&telmetryData, sizeof(telmetryData));
	
	 telmetryData.damaged_block_cnt = 0;
	 telmetryData.lost_packet_cnt = 0;
	 telmetryData.skipped_packet_cnt = 0;
	 telmetryData.injection_fail_cnt = 0;
	 telmetryData.received_packet_cnt = 0;
	 telmetryData.kbitrate = 0; // Video rate icon.
	 telmetryData.kbitrate_measured = 0; // shown as "Measured" when clicked on video icon
	 telmetryData.kbitrate_set = 5000000; // shown as "Set" when clicked on video icon
	 telmetryData.lost_packet_cnt_telemetry_up = 0;
	 telmetryData.lost_packet_cnt_telemetry_down = 0;
	 telmetryData.lost_packet_cnt_msp_up = 0;
	 telmetryData.lost_packet_cnt_msp_down = 0;
	 telmetryData.lost_packet_cnt_rc = 0;
	 telmetryData.current_signal_joystick_uplink = 0xff;
	 telmetryData.current_signal_telemetry_uplink = 0xff;
	 telmetryData.joystick_connected = 0;
	 telmetryData.HomeLon = 55.806418;
	 telmetryData.HomeLat = 12.538012;
	 telmetryData.cpuload_gnd = 53;
	 telmetryData.temp_gnd = 69;
	 telmetryData.cpuload_air = 11;
	 telmetryData.temp_air = 11;
	 telmetryData.wifi_adapter_cnt = 1;

	 for (int j = 0; j < 6; ++j) {
		 telmetryData.adapter[j].current_signal_dbm = -100;
		 telmetryData.adapter[j].received_packet_cnt = 0;
		 telmetryData.adapter[j].type = 0;
		 telmetryData.adapter[j].signal_good = 0;
	 }
	 telmetryData.adapter[0].signal_good = 0;
	 telmetryData.adapter[0].current_signal_dbm = 0;
	 telmetryData.adapter[0].received_packet_cnt = 0;


	 telmetryData.adapter[0].signal_good = 0xDC;
	 telmetryData.adapter[1].signal_good = 0x30;
	 telmetryData.adapter[2].signal_good = 0x76;
	 telmetryData.adapter[3].signal_good = 0x00;
	 telmetryData.adapter[4].signal_good = 0xd0;
	 telmetryData.adapter[5].signal_good = 0xc4;
	
	
	
	
	
	
	
	
	
	do{
		FD_ZERO(&rset); 
		inputVideoConnection.setFD_SET(&rset);
		outputVideoConnection.setFD_SET(&rset);

		inputMavlinkConnection.setFD_SET(&rset);
		outputMavlinkConnection.setFD_SET(&rset);

		inputTelemetryConnection.setFD_SET(&rset);
		outputTelemetryConnection.setFD_SET(&rset);		
				
		if(relayPort != 0){
			relayConnection->setFD_SET(&rset);			
			maxfdp1 = max(inputVideoConnection.getFD(), relayConnection->getFD());
		}else{
			maxfdp1 = inputVideoConnection.getFD();
		}
		maxfdp1 = max(maxfdp1, outputVideoConnection.getFD());
		
		maxfdp1 = max(maxfdp1, inputMavlinkConnection.getFD());
		maxfdp1 = max(maxfdp1, outputMavlinkConnection.getFD());
		
		maxfdp1 = max(maxfdp1, inputTelemetryConnection.getFD());
		maxfdp1 = max(maxfdp1, outputTelemetryConnection.getFD());
		
		// Timeout
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000; // 10ms
		
		nready = select(maxfdp1+1, &rset, NULL, NULL, &timeout); // since we are blocking, wait here for data.//
		
		// Video DATA from Drone		
		if (FD_ISSET(inputVideoConnection.getFD(), &rset)) { 
			int result = 0;
			result = inputVideoConnection.readData(rxBuffer, RX_BUFFER_SIZE);
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > RX_BUFFER_SIZE) {
				fprintf(stderr, "RX: Error on Input video UDP Socket Port: %d, Terminate program.\n", videoPort);
				exit(EXIT_FAILURE);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else {	
				write(STDOUT_FILENO, rxBuffer, result);		// also write to STDOUT.	
			//	outputVideoConnection.writeData(rxBuffer, result);
				linkstatus.rx += (float)result;
			}
		}

		// DATA from QOpenHD (Video return) This should not happend
		if (FD_ISSET(outputVideoConnection.getFD(), &rset)) {
			int result = 0;
			result = outputVideoConnection.readData(rxBuffer, RX_BUFFER_SIZE);
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > RX_BUFFER_SIZE) {
				fprintf(stderr, "RX: Error on Output video UDP Socket Port: %d, Terminate program.\n", OUTPUT_VIDEO_PORT);
				exit(EXIT_FAILURE);
			}else if(result == 0){
				// None blocking, nothing to read.
			}else {
			
			}
		}

		// Mavlink DATA from Drone
		if (FD_ISSET(inputMavlinkConnection.getFD(), &rset)) {
			int result = 0;
			result = inputMavlinkConnection.readData(rxBuffer, RX_BUFFER_SIZE);
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > RX_BUFFER_SIZE) {
				fprintf(stderr, "RX: Error on Input Mavlink UDP Socket Port: %d, Terminate program.\n", mavlinkPort);
				exit(EXIT_FAILURE);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else {
				outputMavlinkConnection.writeData(rxBuffer, result);

				if(relayPort != 0){
					relayConnection->writeData(rxBuffer, result);
				}				
			}
		}

		// DATA from OpenHD (Mavlink return) This should not happend, or it is a keep-alive
		if (FD_ISSET(outputMavlinkConnection.getFD(), &rset)) {
			int result = 0;
			result = outputMavlinkConnection.readData(rxBuffer, RX_BUFFER_SIZE);
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > RX_BUFFER_SIZE) {
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
			result = inputTelemetryConnection.readData(rxBuffer, RX_BUFFER_SIZE);
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > RX_BUFFER_SIZE) {
				fprintf(stderr, "RX: Error on Input Telemetry UDP Socket Port: %d, Terminate program.\n", telemetryPort);
				exit(EXIT_FAILURE);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else { // We have data lets build the frame and sent it to QOpenHD
				telmetryData.cpuload_air = rxBuffer[0];
				telmetryData.temp_air = rxBuffer[1];										
			}
		}

		// DATA from OpenHD (Telemetry return) This should not happend, or it is a keep-alive
		if (FD_ISSET(outputTelemetryConnection.getFD(), &rset)) {
			int result = 0;
			result = outputTelemetryConnection.readData(rxBuffer, RX_BUFFER_SIZE);
			
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > RX_BUFFER_SIZE) {
				fprintf(stderr, "RX: Error on Output Telemetry UDP Socket Port: %d, Terminate program.\n", OUTPUT_TELEMETRY_PORT);
				exit(EXIT_FAILURE);
			}else if(result == 0){
				// None blocking, nothing to read.
			}else {
				
			}
		}

		
		
		if( (relayPort != 0) && (FD_ISSET(relayConnection->getFD(), &rset))){// Data from UDP relay back (When we relay to Mavlink server, it will send a few messages back to drone)
			int result = 0;
			result = relayConnection->readData(rxBuffer, RX_BUFFER_SIZE);
			//printf("Read result:%d\n\r", n);
			if (result < 0 || result > RX_BUFFER_SIZE) {
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
					linkstatus.dropped += result;					
				}else{							
					linkstatus.tx += result;						
				}
			}
		}
		

		// check if it is time to log the status and sent Telemtry frame to QOpenHD:
		if(time(NULL) >= nextPrintTime){
			fprintf(stderr, "RX: Status:       UDP Packages: (tx|rx|dropped):  %*.2fKB  |  %*.2fKB  | %*.2fKB", 6, linkstatus.tx/1024 , 6, linkstatus.rx/1024 , 6 , linkstatus.dropped/1024);
			nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
			
			// Sendt the Telemtry frame to QOpenHD.
			telmetryData.kbitrate = (linkstatus.rx*8)/1024; // Video kbit rate.
			telmetryData.HomeLat = 55.806341;
			telmetryData.HomeLon = 12.537463;
			int res = 0;
			res = outputTelemetryConnection.writeData(&telmetryData, 113);
			
			if(res < 0){
				fprintf(stderr, "RX: Error on write to output Telemetry UDP Socket Port: %d, Terminate program.\n", OUTPUT_TELEMETRY_PORT);		
				exit(EXIT_FAILURE);
			}else if(res == 0){

			}else{

			}			
			
			fprintf(stderr, "     Video rate: %4dkbit/s   Air CPU Load: %3d%%     CPU Temp: %3dC\n",telmetryData.kbitrate, telmetryData.cpuload_air, telmetryData.temp_air);		
			bzero(&linkstatus, sizeof(linkstatus));
			
			//Send keep alive to the Drone:
			rxBuffer[0]=0x50;
			rxBuffer[1]=0x51;
			rxBuffer[2]=0x52;
			rxBuffer[3]=0x53;
			rxBuffer[4]=0x54;
			rxBuffer[5]=0x55;
			inputVideoConnection.writeData(rxBuffer, 6);
			inputTelemetryConnection.writeData(rxBuffer, 6);
		}
		
	}while(1);
	
	perror("RX: PANIC! Exit While 1\n");
    return 1;
}

