
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

enum DroneState_t{
	ON_GROUND_DISARMED=0,
	ON_GROUND_ARMED,
	FLYING
};
DroneState_t DroneState=ON_GROUND_DISARMED;

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
	fprintf(stderr, "Recording path: %s\n",outputPath);
	// For Mavlink input
	fprintf(stderr, "Mavlink input port: %u\n",mavlinkPort);
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
	time_t oneSecondTime = time(NULL) + 1;		

	int active = 2;
	int flyingCheckCounter = 0;
	int testFlyingAlg=0;

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
			}
		}		
		
		
		//Only run on timeout
		if(nready == 0){	
			// check if it is time to log the status:
			/*
			if((time(NULL) >= nextPrintTime) && (active > 0 )){
				active--; // count down log active counter.		
				// Log the last known posistion:
				fprintf(stderr, "Last known drone position: (%.6f %.6f) Altitude (MSL):%.0f [meters] Altitude (above ground):%.0f [meters]\n",mavlinklDataFromAir.getLatitude(),mavlinklDataFromAir.getLongitude(), mavlinklDataFromAir.getAltitudeMSL(), mavlinklDataFromAir.getAltitude());

				if(active == 0){
					fprintf(stderr, "Video Record: No Mavlink data for 20 seconds. Stop logging output.\n");	
				}

				nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
			}
			*/

			if((time(NULL) >= oneSecondTime) ){
				
				uint16_t throttle = mavlinklDataFromAir.getThrottle();
				float groundSpeed = mavlinklDataFromAir.getGroundspeed();
				
				// debug
				/*
				if(testFlyingAlg++ >= 60 ){
					groundSpeed = 5;
					if(testFlyingAlg >= 120){
						testFlyingAlg=0;
						groundSpeed = 0;
					}
				}else{
					groundSpeed = 0;
				}
				fprintf(stderr, "Video Record: Debug, testFlyingAlg(%u) throttle(%u) ground speed(%.1f)\n",testFlyingAlg, throttle, groundSpeed);
				*/
			
				switch(DroneState)
				{
					case ON_GROUND_DISARMED: 
					{
						// Are we armed?
						if(mavlinklDataFromAir.isArmed()){
							fprintf(stderr, "Video Record: Drone is now armed, droneState: ON_GROUND_DISARMED->ON_GROUND_ARMED\n");
							DroneState=ON_GROUND_ARMED;
							flyingCheckCounter=0;
							videoRecorder.start();
						}
					}
					break;

					case ON_GROUND_ARMED: 
					{
						// Are we flying?	
						if( (throttle >= 10) && (groundSpeed >= 3.0) ){
							flyingCheckCounter++;
							fprintf(stderr, "Video Record: Drone is about to fly, throttle(%u) ground speed(%.1f)\n", throttle, groundSpeed);	
						}else{
							flyingCheckCounter=0;
						}

						if(flyingCheckCounter >= 5){
							DroneState=FLYING;
							flyingCheckCounter=0;
							fprintf(stderr, "Video Record: Drone is now flying, thus starting recording, droneState: ON_GROUND_ARMED->FLYING\n");	
						}

						if(!mavlinklDataFromAir.isArmed()){
							fprintf(stderr, "Video Record: Drone is now disarmed, thus stopping recording, droneState: ON_GROUND_ARMED->ON_GROUND_DISARMED\n");
							videoRecorder.stop();
							DroneState=ON_GROUND_DISARMED;
						}
					}
					break;

					case FLYING: 
					{
						nextPrintTime++;
						if(nextPrintTime >= LOG_INTERVAL_SEC){
							fprintf(stderr, "Video Record: Flying, Last known drone position: (%.6f %.6f) Altitude (above ground):%.0f [meters]\n",mavlinklDataFromAir.getLatitude(),mavlinklDataFromAir.getLongitude(), mavlinklDataFromAir.getAltitude());
							nextPrintTime=0;
						}
						
						// Are we landed?
						if( (throttle < 10) && (groundSpeed < 3.0) ){
							flyingCheckCounter++;
							fprintf(stderr, "Video Record: Drone is about to land, throttle(%u) ground speed(%.1f)\n", throttle, groundSpeed);	
						}else{
							flyingCheckCounter=0;
						}

						if(flyingCheckCounter >= 5){
							flyingCheckCounter=0;
							if(mavlinklDataFromAir.isArmed()){
								fprintf(stderr, "Video Record: Drone is now landed but still armed, thuis stop recording, droneState: FLYING->ON_GROUND_ARMED\n");
								DroneState=ON_GROUND_ARMED;
							}else{
								fprintf(stderr, "Video Record: Drone is now landed and disarmed, thuis stop recording, droneState: FLYING->ON_GROUND_DISARMED\n");
								DroneState=ON_GROUND_DISARMED;
							}
							videoRecorder.stop();
						}
					}
					break;


					default:
					{
						fprintf(stderr, "Video Record: Unknown DroneState switch (%u).\n", DroneState);	
					}
					break;
				}				
				oneSecondTime = time(NULL) + 1;
			}
		}
	}while(1);
	
	perror("Video Record: PANIC! Exit While 1\n");
    return 1;
}

