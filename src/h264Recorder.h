  
/*
	h264.h
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef H264RECORDER_H_
#define H264RECORDER_H_

#include <stdint.h> 
#include <cstdio>
#include <strings.h> // bzero
#include <fstream> //Video record to file

#define MAX_FILE_SIZE 2136997888 //(2GB-10MB)


class H264Recorder
{
	// Public functions to be used on all Messages
	public:	
	H264Recorder(char *filePath); 
	virtual ~H264Recorder(){}; //destructor
	void inputStream(uint8_t *data, uint32_t length);
	void start(void); // start recording to file in path with filename: YYYY-MM-DD_HH-MM-Recording0.h264
	void stop(void);  // this will also finalize H264 to mp4.
	void restart(char *filePath); // start recording to file a new file.

	// Parameters used by the classes using this
	protected:
	
	// Parameters only used on mother class.
	private:
	
	void startNewRecordingFile(bool closeFile);
	void clearAll(void);
	
	enum RecorderState_t{
	  LOOKING_FOR_HEADER=0,		
	  SAVING_HEADER,
	  WAITING_FOR_KEYFRAME,
  	  RECORDING,
	  RECORD_UNTIL_NEXT_KEYFRAME,
	  STOPPED
	};
	RecorderState_t state=LOOKING_FOR_HEADER;
	
	std::ofstream videoRecordFile;// new std::ofstream(recordename,std::ofstream::binary);
	char recordPath[100];
	char recordFileWithPath[100]; // h264 filename
	char finaleFileWithPath[100]; // mp4 filename

	bool recording=false;
	uint8_t buffer[65565]; // needs to include the keyframe start header.	
	uint8_t header[60];
	uint32_t headerIndexCounter=0;
	uint32_t bufferIndexCounter=0;
	
//	bool headerFound=false;
	uint32_t bytesRecorded=0;

};

#endif /* H264RECORDER_H_ */