  
/*
	h264RXFraming.h
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef H264RXFRAMING_H_
#define H264RXFRAMING_H_

#include <stdlib.h>
#include <cerrno>   // for errno
#include <unistd.h> // for write
#include <fstream> //Video record to file debug
#include "h264.h"

class H264RXFraming : public H264
{
	// Public functions
	public:	
	H264RXFraming(); 
	virtual ~H264RXFraming(){}; //destructor

	uint8_t * getInputBuffer(void); // returns pointer to the an available input buffer.
	uint16_t getPackageMaxSize(void); // returns maximum data size.
	
	bool setData(void *buf, uint16_t length);
	bool setData(uint16_t size); // this is used after data is inputted directly via getInputBuffer pointer with maxSize.
	uint32_t getOutputStreamFIFOSize(void); // returns the number of packages ready in output FIFO
	uint32_t getOutputStream(void *buf, uint16_t maxLength); 
	void writeAllOutputStreamTo(int fd);
	uint32_t getInputQueueSize(void);
	uint32_t getTempOutputSize(void);
	bool waitingForHeader(void); // returns true of waiting for the header.
	
	private:
	bool serviceRXPackage(void);
	std::vector<H264UDPPackage *> inputData; // Place data here if it is not the next package inline for output.
	std::queue<H264UDPPackage *> tempOutputFrame;	// build output frame here, only transfer to output when a complete frame is ready.
	
	bool isNextPackage(H264UDPPackage *package);
	void buildOutputFrame(H264UDPPackage *package);
	void checkInputBufferForMoreData(void);
	void clearOutputFrame(void);
	void finishOutputFrame(void);
	void clearInputDataWithPackagesOlderThan(H264UDPPackage *input);
	void clearInputData(void);
	bool headerFound=false;


	std::ofstream videoRecordFile;// new std::ofstream(recordename,std::ofstream::binary);
	//bool firstTime=true; //record
	bool firstTime=false; // don't record
};

#endif /* H264RXFRAMING_H_ */