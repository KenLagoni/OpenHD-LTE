/*
	mavlinkHandler.cpp
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 
#include "mavlinkHandler.h" 

mavlinkHandler::mavlinkHandler(){
	// clear all memmory:
		std::queue<mavlink_message_t> outputFIFO;	
	this->foundMSG30=false;
	this->armed=false;

	float latitude=0;
	float longitude=0;
	float altitudeMSL=0;
	float altitude=0;
}


void mavlinkHandler::inputData(uint8_t *input, int size){ // parse input data to class.
	int index=0;
	uint8_t byte=0;
	mavlink_status_t status;
	mavlink_message_t msg;
	int chan = MAVLINK_COMM_0;

	while(size>0){
		size--;
		byte=input[index];
		index++;
		if (mavlink_parse_char(chan, byte, &msg, &status)){
			// printf("MSG ID#%d\n\r",msg.msgid);
			// MSG ID 30 (HUD 10HZ) mean transmit now!
			
			// if fifo is larger than 1024 bytes or MSG 30 har ben received, then transmit.
			this->outputFIFO.push(msg);
			
			if(msg.msgid == 30){ // MSG 30 found.
				this->foundMSG30=true;
			}

			// Keep track on ARM / DISARMED.  Status can be found in HEARTBEAT (MSG=0) from FC:
			if(msg.msgid == 0){ // Heartbeat
				mavlink_heartbeat_t newmsg;
				mavlink_msg_heartbeat_decode(&msg, &newmsg);
				this->armed = newmsg.base_mode & MAV_MODE_FLAG_SAFETY_ARMED;
			}

			// Save Last known GPS posistion and altitude
			if(msg.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT){ //#33
				mavlink_global_position_int_t newmsg;
				mavlink_msg_global_position_int_decode(&msg, &newmsg);
				this->latitude =  ((float)newmsg.lat)/10000000;
				this->longitude = ((float)newmsg.lon)/10000000;
				this->altitudeMSL = ((float)newmsg.alt)/1000;
				this->altitude = ((float)newmsg.alt)/1000;
				//fprintf(stderr, "Last known drone position: (%.6f;%.6f) Altitude (MSL):%.0f [meters] Altitude (above ground):%.0f [meters]",latitude,longitude, altitudeMSL, altitude);
			}
		}
	}
}

uint32_t mavlinkHandler::getData(uint8_t *output, int maxSize){ // copies maxSize of bytes to output.
	uint32_t bytesCopied=0;
	uint32_t msgSize=0;
	mavlink_message_t outputmsg;
	uint8_t tempout[300]; // mavlink max should be 280

	bool moreData=true;

	// prepare the right amount of data to output.
	do{
		if(this->outputFIFO.size() > 0){
			outputmsg = this->outputFIFO.front(); // get the next messages from FIFO.
			msgSize = mavlink_msg_to_send_buffer(&tempout[0], &outputmsg); // get the message size (and prepare it ):
			if( (msgSize+bytesCopied) < maxSize){ // will the new msg fit in maxSize?
				// msg will fit, thus copy it:
				memcpy(&output[bytesCopied], tempout, msgSize); // copy the messags:
				bytesCopied += msgSize;
				this->outputFIFO.pop(); // remove the msg from FIFO.
			}
		}else{
			this->foundMSG30=false; // must be since FIFO is empty.
			moreData=false;
		}
	}while(moreData);

	return bytesCopied;
} 

uint32_t mavlinkHandler::getFIFOsize(void){ // returns number of Mavlink messages ready for output.
	return (uint32_t)this->outputFIFO.size();
}

bool mavlinkHandler::outputDataHasMSG30(void){ // in order to sync output flow on msg 30 (roll/pitch 10Hz). returns true when MSG30 has been passed to output.
	return this->foundMSG30;
}

bool mavlinkHandler::isArmed(void){ 
	return this->armed;
}


float mavlinkHandler::getLatitude(void){ 
	return this->latitude;
}

float mavlinkHandler::getLongitude(void){ 
	return this->longitude;
}

float mavlinkHandler::getAltitudeMSL(void){ 
	return this->altitudeMSL;
}

float mavlinkHandler::getAltitude(void){ 
	return this->altitude;
}


/*

				uint8_t fifoSize = serialRxFIFO.size();
				//	printf("\n\r \n\r \n\rTime to transmit! - mavlink FIFO has %d elements with total size of %d\n\r", fifoSize, totalSize);
				mavlink_message_t data;
				
				if(true == serialDataToSend){
					//printf("tx_raw: Discharding %d bytes of serial data\n\r",serialBufferSize);
					linkstatus.mavlinkdropped += serialBufferSize;
				}
				
				serialBufferSize=0;
				
				for(int a=0;a<fifoSize;a++){
					if(!serialRxFIFO.isEmpty()){
						serialRxFIFO.pop(data);
						//printf("FIFO index %d has MSG ID %d\n\r",a,data.msgid);
						serialBufferSize += mavlink_msg_to_send_buffer(&serialBuffer[serialBufferSize], &data);
						//memcpy(&serialBuffer[serialBufferSize], &data., result); // copy input to buffer.
					}
				}

				//						printf("Done reading, FIFO size is now %d\n\r",serialRxFIFO.size());
				totalSize = 0; // clear the MSG FIFO
				serialDataToSend = true;
*/