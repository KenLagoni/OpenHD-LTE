  
/*
	mavlinkHandler.h
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef MAVLINKHANDLER_H_
#define MAVLINKHANDLER_H_

#include <stdint.h> 
#include <cstdio>
#include <strings.h> // bzero
#include <queue>

// for Mavlink
#include "c_library_v1-master/common/mavlink.h"
#include "c_library_v1-master/ardupilotmega/mavlink.h"

class mavlinkHandler
{
	// Public functions to be used on all Messages
	public:	
	mavlinkHandler(); 
	virtual ~mavlinkHandler(){}; //destructor

	void inputData(uint8_t *input, int size); // parse input data to class.
	uint32_t getData(uint8_t *output, int maxSize); // copies data to output.

	uint32_t getFIFOsize(void); // returns number of Mavlink messages ready for output.
	bool outputDataHasMSG30(void); // in order to sync output flow on msg 30 (roll/pitch 10Hz). returns true when MSG30 has been passed to output.
	bool isArmed(void); // return true if FC is armed.

	float getLatitude(void);
	float getLongitude(void);
	float getAltitudeMSL(void);
	float getAltitude(void);

	float getAirspeed(void);
	float getGroundspeed(void);
	uint16_t getThrottle(void);
	

	// Parameters used by the classes using this
	protected:

	
	// Parameters only used on mother class.
	private:
	std::queue<mavlink_message_t> outputFIFO;	
	bool foundMSG30=false;
	bool armed=false;

	float latitude=0;
	float longitude=0;
	float altitudeMSL=0;
	float altitude=0;

	float airspeed=0;
	float groundspeed=0;
	uint16_t throttle=0;
};

#endif /* MAVLINKHANDLER_H_ */