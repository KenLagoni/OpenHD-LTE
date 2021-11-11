  
/*
	h264RXFraming.h
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef H264RXFRAMING_H_
#define H264RXFRAMING_H_

#include <unistd.h> // for write
#include "h264.h"

class H264RXFraming : public H264
{
	// Public functions
	public:	
	H264RXFraming(); 
	virtual ~H264RXFraming(){}; //destructor

	uint8_t * getInputBuffer(void); // returns pointer to the an available input buffer.
	uint16_t getPackageMaxSize(void); // returns maximum data size.
	bool setData(uint16_t size); // this is used after data is inputted directly via getInputBuffer pointer with maxSize.
	uint32_t getOutputStreamFIFOSize(void); // returns the number of packages ready in output FIFO
	void writeAllOutputStreamTo(int fd);
	
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
};

#endif /* H264RXFRAMING_H_ */