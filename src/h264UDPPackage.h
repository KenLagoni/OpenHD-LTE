  
/*
	h264UDPPackage.h
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef H264UDPPAGE_H_
#define H264UDPPAGE_H_

#include <stdint.h> 
#include <cstdio>
#include <strings.h> // bzero
#include <cstring> // memcpy

#define UDP_PACKET_SIZE 1400 // MAX MTU size for ethernet is ~1456, so keep below this for none framing.
#define UDP_HEADER 4
#define UDP_DATA_SIZE UDP_PACKET_SIZE-UDP_HEADER

class H264UDPPackage
{
	// Public functions
	public:	
	H264UDPPackage(); 
	virtual ~H264UDPPackage(){}; //destructor
		
	void clear(void); // clear all data.
	bool isFree(void); // return true if free. Else false.
	bool isFull(void); // return true if Full. Else false.
	bool isNewFrame(void); // return true if this is the start of a key- or I-frameFull. Else false.
	bool isNewKeyFrame(void); // return true if this is the start of a keyframeFull. Else false.

	bool setData(void *input, uint16_t length); // return true if ok.
	bool setData(uint16_t size); // this is used if data is inputted directly via getPayload pointer. )for faster performance.
	bool addData(uint8_t data); // return true when full.
	
	// Used for RX:
	uint8_t * getPayload(void); // returns a pointer to the payload. (for read or write)
	uint16_t getPayloadSize(void); // returns the size of the data.
		
	// Used for TX:
	uint8_t * getPackage(void); // returns a pointer to the complete data array includein.
	uint16_t getPackageSize(void); // returns the size of the data.
	
	uint16_t getPackageMaxSize(void); // returns the maxsize for the package.
	
	uint16_t getFrameID(void);
	uint16_t getPackageID(void);

	void setFrameID(uint16_t frameID);
	void setPackageID(uint16_t packageID);
	
	bool isNewerThan(uint16_t FrameID, uint16_t PackageID); // compare it self to frameID and PackageID input, and return true if package is newer than input.
		
	private:
	uint16_t index;
	uint16_t FrameID;            
    uint16_t PackageID; 				    
    uint8_t data[UDP_PACKET_SIZE]; 
};

#endif /* H264UDPPAGE_H_ */