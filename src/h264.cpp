/*
	h264.cpp
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 
#include "h264.h" 

H264::H264(){
	// clear all memmory:
	bzero(&this->InputBuffer, sizeof(this->InputBuffer));
	this->currentBuffer=&this->InputBuffer[0]; // start with index 0
}


bool H264::setNextAvailableBuffer(void){
	
	// search for next empty buffer:
	for(uint32_t count=bufferIndex;count<INPUT_BUFFER_SIZE;count++){
		if(this->InputBuffer[count].isFree()){
			bufferIndex=count;
			this->currentBuffer=&this->InputBuffer[count];
			return false;
		}
	}
	
	// if we come to here, then try searching from 0 to bufferIndex.:
	for(uint32_t count=0;count<bufferIndex;count++){
		if(this->InputBuffer[count].isFree()){
			bufferIndex=count;
			this->currentBuffer=&this->InputBuffer[count];
			return false;
		}
	}
	
	return true;
}

/*
uint16_t H264::getFrameID(void){
	return this->frameID;
}


uint16_t H264::getPackageID(void){
	return this->packageID;
}
*/

uint16_t H264::getNextFrameID(void){
	uint16_t next=this->FrameID;
	next++;
	if(next>=MAX_FRAMEID){ // should be 16 bit to the max, (65535).
		next=1; // Frame start at 1.
	}
	return next;
}
 
uint16_t H264::getNextPackagedID(void){
	uint16_t next=this->PackageID;
	next++;
	if(next>=MAX_PACKAGEID){ // should be 16 bit to the max, (65535).
		next=0;
	}
	return next;
}

void H264::addBytesInputted(uint32_t bytes){
	this->bytesInputted+=bytes;
}

void H264::addBytesOutputted(uint32_t bytes){
	this->bytesOutputted+=bytes;
}

void H264::addBytesDropped(uint32_t bytes){
	this->bytesDropped+=bytes;
}

void H264::clearIOstatus(void){
	 this->bytesInputted=0;
	 this->bytesOutputted=0;
	 this->bytesDropped=0;
}

uint32_t H264::getBytesInputted(void){
	return this->bytesInputted;
}

uint32_t H264::getBytesOutputted(void){
	return this->bytesOutputted;
}

uint32_t H264::getBytesDropped(void){
	return this->bytesDropped;
}
