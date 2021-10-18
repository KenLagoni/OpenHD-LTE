
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

//Video record to file
#include <fstream>


// for Mavlink
#include "c_library_v1-master/common/mavlink.h"
#include "c_library_v1-master/ardupilotmega/mavlink.h"

#define LOG_INTERVAL_SEC 30

#define BUFFER_SIZE 1024

#define MAX_FILE_SIZE 2136997888 //(2GB-10MB)
//#define MAX_FILE_SIZE 1024*1024 // (1MB) for testing.

#define SPS_HEADER_SIZE 15
#define SPS_HEADER_CODE 0x27
#define PPS_HEADER_SIZE 4
#define PPS_HEADER_CODE 0x28
#define P_HEADER_CODE 0x25 // Keyframe

typedef struct { // 27, 28, 25 (keyframe) 21 (I-frame)
	char SPSHeader[SPS_HEADER_SIZE];
	bool SPSHeaderFound;
	char PPSHeader[PPS_HEADER_SIZE];
	bool PPSHeaderFound;
	uint32_t bytesRecorded;
	bool fileCreated;
} videoStream_t;


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
	"-p  <port>     Port for input Mavlink data (only record when armed).\n"
	"-f  <filename> Path+Filename to record.\n"
	"\n"
	"Example:\n"
	"[video pipe] | ./videoRecord -p 6000 -f demo.h264 (record when armed) \n"
	"[video pipe] | ./videoRecord -f demo.h264 (record all)\n"
	"\n");
	exit(1);
}

int main(int argc, char *argv[]) 
{
	char *p;
	int mavlinkPort= 0; 
	char *filename;
		
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
				filename = optarg;
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
	
	
	fprintf(stderr, "Starting Lagoni's Video Record program v0.30\n");


	// For Mavlink input
	Connection mavlinkConnection(mavlinkPort, SOCK_DGRAM); // UDP blocking
	char inputBuffer[BUFFER_SIZE];

	// For select usages.
	fd_set read_set;
	int maxfdp1;
	struct timeval timeout;
	int nready;

	//Mavlink parser and serial:
	mavlink_status_t status;
	mavlink_message_t msg;
	int chan = MAVLINK_COMM_0;

	bool armed=false;

	if(mavlinkPort==0){
		armed=true;
	}

	videoStream_t recordStream; // 0x27 First header in h.264 stream
	bzero(&recordStream, sizeof(recordStream));

	// Record file:
	uint8_t fileNumber = 0;
	char recordename[50];
	char mp4name[50];

//	sprintf(recordename,"%s%d.h264",filename,fileNumber);
	std::ofstream* videoRecordFile = NULL;// new std::ofstream(recordename,std::ofstream::binary);

	// Make FIFO for video
	// Creating the named file(FIFO)
	// mkfifo(<pathname>,<permission>)

/*
	char videofifo[25];
	sprintf(videofifo,"%s","/run/videofifo");
	int res = mkfifo(videofifo, 0666);
	if(res < 0){
		fprintf(stderr, "Video Record: Unable to create video FIFO %s Reason: ",videofifo);
		if(errno == EACCES){
			fprintf(stderr, "One of the directories in pathname did not allow search (execute) permission.\n");
			exit(1);
		}else if(errno == EDQUOT){
			fprintf(stderr, "The user's quota of disk blocks or inodes on the file system has been exhausted.\n");
			exit(1);
		}else if(errno == EEXIST){
			fprintf(stderr, "pathname already exists. This includes the case where pathname is a symbolic link, dangling or not.\n");
		}else if(errno == ENAMETOOLONG){
			fprintf(stderr, "Either the total length of pathname is greater than PATH_MAX, or an individual filename component has a length greater than NAME_MAX. In the GNU system, there is no imposed limit on overall filename length, but some file systems may place limits on the length of a component.\n");
			exit(1);
		}else if(errno == ENOENT){
			fprintf(stderr, "A directory component in pathname does not exist or is a dangling symbolic link.\n");
			exit(1);
		}else if(errno == ENOSPC){
			fprintf(stderr, "The directory or file system has no room for the new file.\n");
			exit(1);
		}else if(errno == ENOTDIR){
			fprintf(stderr, "A component used as a directory in pathname is not, in fact, a directory.\n");
			exit(1);
		}else if(errno == EROFS){
			fprintf(stderr, "pathname refers to a read-only file system.\n");
			exit(1);
		}else{
			fprintf(stderr, "unknown.\n");
			exit(1);
		}
	}
	*/
	// start G-streamer record from /dev/video0 (CSI-HDMI) to FIFO
	// gst-launch-1.0 v4l2src ! "video/x-raw,framerate=30/1,format=UYVY" ! v4l2h264enc extra-controls="controls,h264_profile=4,h264_level=13,video_bitrate=5000000;" ! video/x-h264,profile=high ! h264parse ! filesink location=/run/videofifo
	char command[500];
//	snprintf(command, 300, "gst-launch-1.0 v4l2src device=/dev/video0 ! \"video/x-raw,framerate=30/1,format=UYVY\" ! v4l2h264enc extra-controls=\"controls,h264_profile=4,h264_level=13,video_bitrate=5000000;\" ! video/x-h264,profile=high ! h264parse ! filesink location=%s &", videofifo);
//	snprintf(command, 500, "gst-launch-1.0 v4l2src device=/dev/video0 ! \"video/x-raw,framerate=30/1,format=UYVY\" ! v4l2h264enc extra-controls=\"controls,h264_profile=4,h264_level=13,video_bitrate=5000000;\" ! video/x-h264,profile=high ! h264parse ! tee name=t ! queue ! rtspclientsink location=rtsp://192.168.0.200:8554/mystream t. ! queue ! filesink location=%s &", videofifo);

	// Works
//	snprintf(command, 500, "gst-launch-1.0 v4l2src device=/dev/video0 ! \"video/x-raw,framerate=30/1,format=UYVY\" ! v4l2h264enc extra-controls=\"controls,h264_profile=4,h264_level=13,video_bitrate=5000000;\" ! video/x-h264,profile=high ! tee name=t ! h264parse ! queue ! rtspclientsink location=rtsp://192.168.0.200:8554/mystream t. ! h264parse ! filesink location=%s &", videofifo);
//	fprintf(stderr, "Executing gstreamer command: %s\n", command);
//	int commmandres = system(command);
//	fprintf(stderr, "Command Result: %d\n", commmandres);

	// After write side is open, now open read side. (else will block).	
	// First open in read only and read
//	fprintf(stderr, "Open Video FIFO %s for reading\n", videofifo);
//	int videofd = open(videofifo,O_RDONLY);


	time_t nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;		

	fprintf(stderr, "Starting record loop\n");
	do{
		FD_ZERO(&read_set);	
		
		// finding the max filedescriptor
		maxfdp1 = max(STDIN_FILENO, mavlinkConnection.getFD());
		//maxfdp1 = max(videofd, mavlinkConnection.getFD()); //read input video from FIFO not STDIN

		// Set the FD_SET on the filedesscriptors.
		FD_SET(STDIN_FILENO, &read_set);
		//FD_SET(videofd, &read_set);
		mavlinkConnection.setFD_SET(&read_set);

		timeout.tv_sec = 0;
		timeout.tv_usec = 10000; // 10ms

		nready = select(maxfdp1+1, &read_set, NULL, NULL, &timeout);  // blocking	
		
		
			
		if (FD_ISSET(mavlinkConnection.getFD(), &read_set)) { // Data from Mavlink UDP.
			//			printf("Data from Ground (Mavlink)!\n\r");
			int result = 0;
			result = mavlinkConnection.readData(inputBuffer, BUFFER_SIZE);
			if (result < 0 || result > BUFFER_SIZE) {
				fprintf(stderr, "Video Record: Error! on reading STD_IN (pipe input)... Terminate program.\n");
				exit(1);
			}else  if(result == 0){
				// None blocking, nothing to read.
			}else {
				// Parse mavlink and get armed state.
				uint16_t index=0;
				while(result>0){
					result--;
					uint8_t byte=inputBuffer[index];
					index++;
					if (mavlink_parse_char(chan, byte, &msg, &status)){
						// Keep track on ARM / DISARMED for recording purporse. Status can be found in HEARTBEAT (MSG=0) from FC:
						if(msg.msgid == 0){
							mavlink_heartbeat_t newmsg;
							mavlink_msg_heartbeat_decode(&msg, &newmsg);

							if(true==armed && !(newmsg.base_mode & MAV_MODE_FLAG_SAFETY_ARMED)){
								fprintf(stderr, "Video Record: Drone no longer armed, stop recording on next key frame.\n");
							}else if(false==armed && (newmsg.base_mode & MAV_MODE_FLAG_SAFETY_ARMED)){
								fprintf(stderr, "Video Record: Drone is now armed!\n");
							}
							armed = newmsg.base_mode & MAV_MODE_FLAG_SAFETY_ARMED;
						}
					}				
				}
			}
		}
		
		
		
		
		// Read from STDIN (Video pipe)
		if (FD_ISSET(STDIN_FILENO, &read_set)) { // Data from from STDIN (Video pipe)
		//if (FD_ISSET(videofd, &read_set)) { // Data from from Video FIFO
			//printf("Data from STDIN!\n\r");
			int result = 0;
			result = read(STDIN_FILENO, inputBuffer, BUFFER_SIZE);
			//result = read(videofd, inputBuffer, BUFFER_SIZE);

			if (result < 0 || result > BUFFER_SIZE) {
				fprintf(stderr, "Video Record: Error! on reading STD_IN (pipe input)... Terminate program.\n");
				exit(1);
			}else  if(result == 0){
				// EOF
				fprintf(stderr, "Video Record: Warning! Lost connection to stdin. Please make sure that a data source is connected\n");
			}else { // Data from video pipe.
				// Write data to STDOUT.
				write(STDOUT_FILENO, inputBuffer, result);

				// Record: Scan all data while file is not created, (looking for header and P frame)
				if( (false==recordStream.SPSHeaderFound) || (false==recordStream.PPSHeaderFound) || (false==recordStream.fileCreated) ){
					int index=0;
					for(index=0;index<result-5;index++){
						if(inputBuffer[index] == 0x00 && inputBuffer[index+1] == 0x00 && inputBuffer[index+2] == 0x00 && inputBuffer[index+3] == 0x01){ // Header code found
							if(inputBuffer[index+4] == SPS_HEADER_CODE){
								fprintf(stderr, "Video Record: Video SPS 0x27 header found!");
								for(int b=0; b<SPS_HEADER_SIZE; b++){
									if(index+SPS_HEADER_SIZE < result){
										recordStream.SPSHeader[b]=inputBuffer[index+5+b];
										fprintf(stderr, "%02x ",recordStream.SPSHeader[b]);
									}
								}
								recordStream.SPSHeaderFound = true;
								fprintf(stderr, "\n");
							}else if(inputBuffer[index+4] == PPS_HEADER_CODE){
								fprintf(stderr, "Video Record: Video PPS 0x28 header found!: ");
								for(int b=0; b<PPS_HEADER_SIZE; b++){
									if(index+PPS_HEADER_SIZE < result){
										recordStream.PPSHeader[b]=inputBuffer[index+5+b];
										fprintf(stderr, "%02x ",recordStream.PPSHeader[b]);
									}
								}
								recordStream.PPSHeaderFound = true;
								fprintf(stderr, "\n");
							}else if( (inputBuffer[index+4] == P_HEADER_CODE) && (true==armed)){
								fprintf(stderr, "Video Record: Video P   0x25 header (keyframe) found! - SPS=%d PPS=%d armed=%d\n",recordStream.SPSHeaderFound, recordStream.PPSHeaderFound, armed);
								
								if((true==recordStream.SPSHeaderFound) && (true==recordStream.PPSHeaderFound)){ // If we have found headers, then start the file.
									if(false==recordStream.fileCreated){ // create the file
										char buf[34]; // size of header: 34
										buf[0]=0x00;
										buf[1]=0x00;
										buf[2]=0x00;
										buf[3]=0x01;										
										buf[4]=SPS_HEADER_CODE;										
										for(int a=0;a<SPS_HEADER_SIZE;a++){
											buf[5+a]=recordStream.SPSHeader[a];											
										}	
																
										buf[SPS_HEADER_SIZE+5]=0x00;
										buf[SPS_HEADER_SIZE+6]=0x00;
										buf[SPS_HEADER_SIZE+7]=0x00;
										buf[SPS_HEADER_SIZE+8]=0x01;
										buf[SPS_HEADER_SIZE+9]=PPS_HEADER_CODE;		
										for(int a=0;a<PPS_HEADER_SIZE;a++){
											buf[SPS_HEADER_SIZE+10+a]=recordStream.PPSHeader[a];
										}
										
										buf[SPS_HEADER_SIZE+PPS_HEADER_SIZE+10]=0x00;
										buf[SPS_HEADER_SIZE+PPS_HEADER_SIZE+11]=0x00;
										buf[SPS_HEADER_SIZE+PPS_HEADER_SIZE+12]=0x00;
										buf[SPS_HEADER_SIZE+PPS_HEADER_SIZE+13]=0x01;
										buf[SPS_HEADER_SIZE+PPS_HEADER_SIZE+14]=P_HEADER_CODE;
						
										
										fprintf(stderr, "Writing header: ");
										for(int a=0;a<sizeof(buf);a++){
											fprintf(stderr, "%02x ",(uint8_t)buf[a]);
										}
										fprintf(stderr, "\n");
											
										delete videoRecordFile; // lets make a new one.
										// current date/time based on current system
										time_t now = time(0);	
									    // convert now to tm struct for UTC
									    tm *gmtm = gmtime(&now);
										
										sprintf(recordename,"%02d-%02d-%04d_%02d-%02d-%02d_%s.h264",gmtm->tm_mday,gmtm->tm_mon,gmtm->tm_year+1900,gmtm->tm_hour,gmtm->tm_min,gmtm->tm_sec,filename);
										sprintf(mp4name,"%02d-%02d-%04d_%02d-%02d-%02d_%s.mp4",gmtm->tm_mday,gmtm->tm_mon,gmtm->tm_year+1900,gmtm->tm_hour,gmtm->tm_min,gmtm->tm_sec,filename);																				
										videoRecordFile = new std::ofstream(recordename,std::ofstream::binary);
																														
										videoRecordFile->write(buf,sizeof(buf));	 // write header to output file									
										/*
										fprintf(stderr, "Writing rest: ");
										for(int a=0;a<=(sizeof(inputBuffer)-index-5);a++){
											fprintf(stderr, "%02x ",(uint8_t)inputBuffer[index+5+a]);
										}
										fprintf(stderr, "\n");
										*/
										
										// Write the rest of the result buffer:
										videoRecordFile->write(&inputBuffer[index+5],sizeof(inputBuffer)-index-5);	 // write header to output file									

										recordStream.fileCreated=true;
										
										fprintf(stderr, "Video Record: Output file %s created and ready for recording\n",recordename);
									
										recordStream.bytesRecorded=34+sizeof(inputBuffer)-index-5;

										// debug stop here.
										//write(STDOUT_FILENO, inputBuffer, result);	
										//videoRecordFile->close();
										//exit(0);
									}
								}
							}else{
								// fprintf(stderr, "Unknown header: %02x found with first data: %02x\n", inputBuffer[index+4],inputBuffer[index+5]);
							}
						}
					}
				}else{
					if( (true==armed) && (recordStream.bytesRecorded < MAX_FILE_SIZE) ){
						videoRecordFile->write(inputBuffer,result);
						recordStream.bytesRecorded+=result; // count how much we have recoreded. if to large (2GB on fat 32) we must then change file.				
					}else{ // not armed or file to large
						if(true==recordStream.fileCreated){ // file has been created, thus we are active. 
							// record until next Key frame:
							int index=0;
							bool keyFrameFound = false;
							for(index=0;index<result-5;index++){
								if(inputBuffer[index] == 0x00 && inputBuffer[index+1] == 0x00 && inputBuffer[index+2] == 0x00 && inputBuffer[index+3] == 0x01 && inputBuffer[index+4] == P_HEADER_CODE){ // look for key frame
									// key frame found 
									fprintf(stderr, "Video Record: Keyframe found on index %d\n", index);
									keyFrameFound=true;
									break;
								}
							}
							
							if(keyFrameFound){ // write all data upto keyframe
								fprintf(stderr, "Video Record: writing %d bytes up to keyframe, then closing file\n", index);
								videoRecordFile->write(inputBuffer,index);							
								videoRecordFile->close();
								fprintf(stderr, "Video Record: Record file: %s closed, total bytes written: %u\n", recordename, recordStream.bytesRecorded+index);
								recordStream.fileCreated=false;	
								
								// convert to mp4:
								fprintf(stderr, "Video Record: Converting h264 file %s to mp4 file %s\n", recordename, mp4name);
								sprintf(command, "ffmpeg -r 30 -i %s -c copy %s &",recordename, mp4name);
								system(command);

//								fileNumber++; // increase file for next recording.
//								sprintf(recordename,"%s%d.h264",filename,fileNumber);
//								delete videoRecordFile;
//								videoRecordFile = new std::ofstream(recordename,std::ofstream::binary);
							}else{ // keep writing:
								videoRecordFile->write(inputBuffer,result);
								recordStream.bytesRecorded+=result;			
							}						
						}						
					}
				}								
			}
		}		
		
		
		//Only run on timeout
		if(nready == 0){	
			// check if it is time to log the status:
			if(time(NULL) >= nextPrintTime){		
				//simulate arm/disarm
//				fprintf(stderr, "Simulate arm/disarm\n");
//				armed=!armed;
/*
				printf("%d tx_raw: Status:            Mavlink: (tx|rx|dropped):  %*.2fKB  |  %*.0fB  | %*.2fKB            Video: (tx|dropped)  %*.2fMB  | %*.2fMB ", time(NULL), 6, linkstatus.mavlinktx/1024 , 6, linkstatus.mavlinkrx , 6 , linkstatus.mavlinkdropped/1024, 6, linkstatus.videotx/(1024*1024), 6 ,linkstatus.videodropped/(1024*1024));
//				printf("%llu tx_raw: Status:            Mavlink: (tx|rx|dropped):  %*.2fKB  |  %*.0fB  | %*.2fKB            Video: (tx|dropped)  %*.2fMB  | %*.2fKB ", timeMillisec(), 6, linkstatus.mavlinktx/1024 , 6, linkstatus.mavlinkrx , 6 , linkstatus.mavlinkdropped/1024, 6, linkstatus.videotx/(1024*1024), 8 ,linkstatus.videodropped/1024);
				if(true==armed){			
					printf("   FC=ARMED");							
				}else{
					printf("   FC=DISARMED");							
				}																																				 
				bzero(&linkstatus, sizeof(linkstatus));
				*/
				nextPrintTime = time(NULL) + LOG_INTERVAL_SEC;
			}
		}
	
		
		
		
	}while(1);
	
	perror("VideoRecord: PANIC! Exit While 1\n");
    return 1;
}

