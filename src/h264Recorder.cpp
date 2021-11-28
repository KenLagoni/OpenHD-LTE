/*
	h264Recorder.cpp
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 
#include "h264Recorder.h" 

H264Recorder::H264Recorder(char *path){
	// clear all memmory:
	this->clearAll();
	bzero(&this->recordPath, sizeof(this->recordPath));

	sprintf(this->recordPath,"%s",path);	
	// this->startNewRecordingFile(false);
}

void H264Recorder::inputStream(uint8_t *input, uint32_t length){
	uint32_t index=0;
	
	for(; index<length; index++){

		switch(this->state)
		{
			case LOOKING_FOR_HEADER:
			{
				if(index >= 4){
					if(input[index-4] == 0x00 && input[index-3] == 0x00 && input[index-2] == 0x00 && input[index-1] == 0x01 && input[index] == 0x27){ // Header code found
						fprintf(stderr, "H264Recorder: Video header found: ");
						this->header[0]=0x00;
						this->header[1]=0x00;
						this->header[2]=0x00;
						this->header[3]=0x01;
						this->header[4]=0x27;
						this->headerIndexCounter = 5;
						this->state = SAVING_HEADER;
					}else{
						this->state = LOOKING_FOR_HEADER;
					}									
				}
			}
			break;			

			case SAVING_HEADER:
			{
				if(index < length-5){
					this->header[this->headerIndexCounter]=input[index];
					fprintf(stderr, "%02x ",this->header[this->headerIndexCounter]);
					this->headerIndexCounter++;
						
					if(input[index+1] == 0x00 && input[index+2] == 0x00 && input[index+3] == 0x00 && input[index+4] == 0x01 && input[index+5] == 0x25){ // Keyframe found, thus header done!
						if(true==this->recording){
							this->state = WAITING_FOR_KEYFRAME;						
						}else{
							this->state = STOPPED;						
						}
						fprintf(stderr, " Total header size(%u)\n",this->headerIndexCounter);		
					}									
				}
			}
			break;	

			case WAITING_FOR_KEYFRAME:
			{
				if(index >=4 ){
					if(input[index-4] == 0x00 && input[index-3] == 0x00 && input[index-2] == 0x00 && input[index-1] == 0x01 && input[index] == 0x25){ // Header code found						
						// write header to file + keyframe header:
						uint8_t tempbuf[5] = {0x00, 0x00, 0x00, 0x01, 0x25};
						this->videoRecordFile.write((char *)tempbuf,sizeof(tempbuf));	 // write keyframe start header to output file.
						this->bytesRecorded+=5;
						this->state = RECORDING;
					}else{
						this->state = WAITING_FOR_KEYFRAME;
					}									
				}					
			}
			break;		

			case RECORDING:
			{
				// Write the rest of the result buffer:
				if(true==this->recording){
					uint8_t *p;
					p=&input[index];
					int bytesToWrite=length-index;
					
					if(this->bytesRecorded > MAX_FILE_SIZE){
						// next file;
						fprintf(stderr, "H264Recorder: Bytes recorded (%u) which is larger than MAX_FILE_SIZE(%u) - thus time to change output file\n",this->bytesRecorded, MAX_FILE_SIZE);
						this->state = RECORD_UNTIL_NEXT_KEYFRAME;
						bzero(&this->buffer, sizeof(this->buffer));
						this->bufferIndexCounter=1;
						break;
					}
					
					if(bytesToWrite>0){
						this->videoRecordFile.write((char *)p,length-index);	 // write header to output file										
						this->bytesRecorded += bytesToWrite;
					}else{
						fprintf(stderr, "H264Recorder: Write error, length(%u)-index(%u) is less than 1 (%u)\n",length, index, bytesToWrite);
					}
					index=length; // this should stop the for loop;					
				}else{
					this->state = RECORD_UNTIL_NEXT_KEYFRAME;
					bzero(&this->buffer, sizeof(this->buffer));
					this->buffer[0]=input[index]; // save this loop value;
					this->bufferIndexCounter=1;
				}
				
			}
			break;		
			
			case RECORD_UNTIL_NEXT_KEYFRAME:
			{
				// continue to save data until keyframe is found:
				this->buffer[this->bufferIndexCounter]=input[index]; 
				this->bufferIndexCounter++;
			
				// look for keyframe to stop.
				if(index >=4 ){
					if(input[index-4] == 0x00 && input[index-3] == 0x00 && input[index-2] == 0x00 && input[index-1] == 0x01 && input[index] == 0x25){ // Header code found						
						// write header to file + keyframe header:
						this->videoRecordFile.write((char *)this->buffer,(this->bufferIndexCounter-5));	 // rest of buffer, but don'r write the keyframe start header (thus bufferIndexCounter-5).
						
						if(true==this->recording){
							this->startNewRecordingFile(true);	
							this->state = RECORDING; // to start record;
						}else{
							this->state = STOPPED; 
							
						}	
						break;
					}					
				}

				if(this->bufferIndexCounter >= sizeof(this->buffer)){ // write buffer to file
					this->videoRecordFile.write((char *)this->buffer,sizeof(this->buffer));
					bzero(&this->buffer, sizeof(this->buffer));
					this->bufferIndexCounter=0;
				}		
			}
			break;	
			
			case STOPPED:
			{
				if(true==this->recording){
					this->state = WAITING_FOR_KEYFRAME; // to start record;
				}else{
					this->state = STOPPED; 
					index=length; // dont loop all bytes.
				}
			}
			break;		

				
			default:
			{
				fprintf(stderr, "H264Recorder: Error - state (%u) unkown in input stream statemachine\n",this->state);
			}
			break;	
		}
	}
	
}


void H264Recorder::start(void){ // start recording to file in path with filename: YYYY-MM-DD_HH-MM-Recording0.h264
	if(this->recording == false){ // only make new file first time we go from false to true.
		startNewRecordingFile(false);
	}
	this->recording=true;
}

void H264Recorder::stop(void){  // this will also finalize H264 to mp4.
	if(this->recording == true){ // only make new file first time we go from false to true.
		fprintf(stderr, "H264Recorder: Recording stopped\n");	

		// convert to mp4:
		fprintf(stderr, "H264Recorder: Converting h264 file %s to mp4 file %s\n", this->recordFileWithPath, this->finaleFileWithPath);
		char command[500];
		sprintf(command, "ffmpeg -r 30 -i %s -c copy %s &",this->recordFileWithPath, this->finaleFileWithPath);
		system(command);
	}
	this->recording=false;
}

void H264Recorder::restart(char *filePath){
	this->clearAll();
	sprintf(this->recordPath,"%s",filePath);	
	this->startNewRecordingFile(true);
}

void H264Recorder::clearAll(void){
	this->recording=false;
	bzero(&this->buffer, sizeof(this->buffer));
	this->headerIndexCounter=0;
	this->bufferIndexCounter=0;
	this->bytesRecorded=0;
	this->state=LOOKING_FOR_HEADER;
	bzero(&this->recordFileWithPath, sizeof(this->recordFileWithPath));
}

void H264Recorder::startNewRecordingFile(bool closeOld){
	
	if(true==closeOld){
		videoRecordFile.close();	
	}
	
	this->bytesRecorded=0;
	
	time_t now = time(0);	
	// convert now to tm struct for UT
	tm *gmtm = gmtime(&now);
	
	bzero(&this->recordFileWithPath, sizeof(this->recordFileWithPath));
	int day=gmtm->tm_mday;
	int month=gmtm->tm_mon+1;
	int year=gmtm->tm_year+1900;
	int hour=gmtm->tm_hour;
	int min=gmtm->tm_min;
	int sec=gmtm->tm_sec;

	sprintf(this->recordFileWithPath,"%s/%02d-%02d-%04d_%02d-%02d-%02d_Record.h264",this->recordPath,day,month,year,hour,min,sec);	
	sprintf(this->finaleFileWithPath,"%s/%02d-%02d-%04d_%02d-%02d-%02d_Record.mp4",this->recordPath,day,month,year,hour,min,sec);	

	fprintf(stderr, "H264Recorder: New file created %s\n",this->recordFileWithPath);

	this->videoRecordFile.open(this->recordFileWithPath, std::ios::binary); // open file for write.	
	this->videoRecordFile.write((char *)this->header, this->headerIndexCounter);	 // write header to output file			
}
/*
bool checkExists(std::string file){
    std::ifstream file_to_check (file.c_str());
    if(file_to_check.is_open()){
		return true;
	}
    return false;
}

	// Record file:
	uint8_t fileNumber = 0;
	char filename[30];
	bool newFile=false;
	do{
		fileNumber++;
		sprintf(filename,"%s%d.h264",outputFile,fileNumber);
		fprintf(stderr, "tx_raw: using video output file (%s).\n",filename);
	}while(checkExists(filename));
		
	std::ofstream* videoRecordFile = new std::ofstream(filename,std::ofstream::binary);


*/

