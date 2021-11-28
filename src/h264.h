  
/*
	h264.h
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef H264_H_
#define H264_H_

#include <stdint.h> 
#include <cstdio>
#include <strings.h> // bzero
#include <queue>
#include "h264UDPPackage.h"

#define UDP_PACKET_SIZE 1400
#define UDP_PAYLOAD_SIZE UDP_PACKET_SIZE-4
#define MAX_PACKAGEID 65535
#define MAX_FRAMEID 65535
#define INPUT_BUFFER_SIZE 16384 // total RAM size = UDP_PACKET_LENGTH * FRAME_BUFFER_SIZE = 1024 * 8192 = 8MB
//#define INPUT_BUFFER_SIZE 1200 // total RAM size = UDP_PACKET_LENGTH * FRAME_BUFFER_SIZE = 1024 * 8192 = 8MB


class H264
{
	// Public functions to be used on all Messages
	public:	
	H264(); 
	virtual ~H264(){}; //destructor
	void clearIOstatus(void);
	uint32_t getBytesInputted(void);
	uint32_t getBytesOutputted(void);
	uint32_t getBytesDropped(void);
	uint32_t getBufferSize(void);

	// Parameters used by the classes using this
	protected:
	bool setNextAvailableBuffer(void); // returns true if buffer full (error)
	uint16_t getNextFrameID(void);     // returns next Frame ID number
	uint16_t getNextPackagedID(void);  // returns next packaged ID number
//	uint16_t getFrameID(void);         // returns current Frame ID number
//	uint16_t getPackageID(void);       // returns current Package ID number
	uint16_t FrameID=0;
	uint16_t PackageID=0;
	H264UDPPackage *currentBuffer; 		
	std::queue<H264UDPPackage *> outputPackages;

	void addBytesInputted(uint32_t bytes);
	void addBytesOutputted(uint32_t bytes);
	void addBytesDropped(uint32_t bytes);

	H264UDPPackage InputBuffer[INPUT_BUFFER_SIZE]; // for debug
	// Parameters only used on mother class.
	private:

	//H264UDPPackage InputBuffer[INPUT_BUFFER_SIZE];
	uint32_t bufferIndex=0;
	
	uint32_t bytesInputted=0;
	uint32_t bytesOutputted=0;
	uint32_t bytesDropped=0;
};

#endif /* H264_H_ */