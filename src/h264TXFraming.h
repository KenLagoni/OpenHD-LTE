  
/*
	h264TXFraming.h
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef H264TXFRAMING_H_
#define H264TXFRAMING_H_

#include "h264.h"

class H264TXFraming : public H264
{
	// Public functions
	public:	
	H264TXFraming(); 
	virtual ~H264TXFraming(){}; //destructor

	void inputStream(uint8_t *data, uint32_t maxlength); // input data with pointer to array and length of bytes to copy.	
	uint16_t getTXPackage(uint8_t * &data); // Sets the pointer to the data array and returnt number of bytes in package.
	void nextTXPackage(void); // Informs H264 that package was transmitted so it can move to next package.
	void reTransmitHeader(void);
	
	//uint16_t getStartHeader(uint8_t *data, uint32_t maxlength); // copy start header to data and returns number of bytes copied.
	// getStatus...
	
	
	private:
	
	// for Header search:
	struct H264Header{
		uint8_t counter;
		uint8_t data[40]; // Header is 29
	};
	H264Header startHeader;
	bool savingStream=false;
			
	enum FrameState_t{
	  LOOK_FOR_HEADER_00=0,		
	  LOOK_FOR_HEADER_00_2,
	  LOOK_FOR_HEADER_00_3,
	  LOOK_FOR_HEADER_01,
	  ANALYSE_HEADER,
  	  GET_SPS_HEADER,
	  GET_PPS_HEADER
	};
	FrameState_t state=LOOK_FOR_HEADER_00;
	
	// private functions to manage the Inputbuffer array:
	void addData(uint8_t data);
	void startNewPackage(bool keyframe);		
	void trimOutputFIFO(void);	
	bool headerRequested = false;
};

#endif /* H264TXFRAMING_H_ */