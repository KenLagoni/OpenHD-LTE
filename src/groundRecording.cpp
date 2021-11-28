
#include <fcntl.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <chrono> // Crone time measure
#include <ctime>

#include "connection.h"
#include "h264Recorder.h"
#include "mavlinkHandler.h"


#define LOG_INTERVAL_SEC 30

int max(int x, int y)
{
	if (x > y)
	return x;
	else
	return y;
}

int flagHelp = 0;

void usage(void) {
	printf("\nUsage: videoRecord [options]\n"
	"\n"
	"Options:\n"
	"-p  <port>       Port for input Mavlink data (only record when armed).\n"
	"-f  <outputPath> Path to output recordings.\n"
	"\n"
	"Example:\n"
	"[video pipe] | ./videoRecord -p 6000 -f /home/pi \n"
	"\n");
	exit(1);
}

int main(int argc, char *argv[]) 
{
	char *p;
	int mavlinkPort= 0; 
	char *outputPath;
		
    while (1) {
	    int nOptionIndex;
	    static const struct option optiona[] = {
		    { "help", no_argument, &flagHelp, 1 },
		    {      0,           0,         0, 0 }
	    };
	    int c = getopt_long(argc, argv, "h:p:f:", optiona, &nOptionIndex);
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
			    mavlinkPort = atoi(optarg);
			    //				fprintf(stderr, "Serial Port   :%d\n",udpSerialPort);
			    break;
		    }

	
			case 'f': {
				outputPath = optarg;
				break;
			}

		    default: {
			    fprintf(stderr, "Video Record: unknown input parameter switch %c\n", c);
			    usage();

			    break;
		    }
	    }
    }

    if (optind > argc) {
	    usage();
    }
	
	
	fprintf(stderr, "Starting Video Record program \n");

	// For Mavlink input
	Connection mavlinkConnection(mavlinkPort, SOCK_DGRAM); // UDP blocking
	uint8_t inputBuffer[1500];

	// For select usages.
	fd_set read_set;
	int maxfdp1;
	struct timeval timeout;
	int nready;

	mavlinkHandler mavlinklDataFromAir;
	H264Recorder videoRecorder(outputPath);

	time_t nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;		
	
	int active = 10; // print log for 10 seconds.

	do{
		FD_ZERO(&read_set);	
		
		// finding the max filedescriptor
		maxfdp1 = max(STDIN_FILENO, mavlinkConnection.getFD());

		// Set the FD_SET on the filedesscriptors.
		FD_SET(STDIN_FILENO, &read_set);
		mavlinkConnection.setFD_SET(&read_set);

		timeout.tv_sec = 0;
		timeout.tv_usec = 10000; // 10ms

		nready = select(maxfdp1+1, &read_set, NULL, NULL, &timeout);  // blocking	

		// DATA from Mavlink UDP
		if (FD_ISSET(mavlinkConnection.getFD(), &read_set)) {  // Data from Mavlink UDP.
			int result=0;
			result = mavlinkConnection.readData(inputBuffer, sizeof(inputBuffer));
			
			// Parse buffer to mavlink class.
			if(result > 0){
				mavlinklDataFromAir.inputData(inputBuffer, result);
				
				if(mavlinklDataFromAir.isArmed()){
					videoRecorder.start();
				}else{
					videoRecorder.stop();
				}
				active = 2; 
			}
		}

		
		// Read from STDIN (Video pipe)
		if (FD_ISSET(STDIN_FILENO, &read_set)) { // Data from from STDIN (Video pipe)
			int result = 0;
			result = read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer));

			if (result < 0 || result > sizeof(inputBuffer)) {
				fprintf(stderr, "Video Record: Error! on reading STD_IN (pipe input)... Terminate program.\n");
				exit(1);
			}else  if(result == 0){
				// NOP - no data.
			}else { // Data from video pipe.
				videoRecorder.inputStream(inputBuffer, (uint16_t)result); // parse data for video recording.				
				active = 2; 
			}
		}		
		
		
		//Only run on timeout
		if(nready == 0){	
			// check if it is time to log the status:
			if((time(NULL) >= nextPrintTime) && (active > 0 )){
				active--; // count down log active counter.		
				// Log the last known posistion:
				fprintf(stderr, "Last known drone position: (%.6f;%.6f) Altitude (MSL):%.0f [meters] Altitude (above ground):%.0f [meters]\n",mavlinklDataFromAir.getLatitude(),mavlinklDataFromAir.getLongitude(), mavlinklDataFromAir.getAltitudeMSL(), mavlinklDataFromAir.getAltitude());

				if(active == 0){
					fprintf(stderr, "RX: Status: No data for 10 seconds. Stop logging output.\n");	
				}

				nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
			}
		}
	}while(1);
	
	perror("VideoRecord: PANIC! Exit While 1\n");
    return 1;
}

