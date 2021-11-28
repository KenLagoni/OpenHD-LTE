/*
	h264.cpp
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 
#include "h264.h" 

H264::H264(){
	// clear all memmory:
	
	for(int a=0;a<INPUT_BUFFER_SIZE;a++){
		this->InputBuffer[a].clear();
	}
	

	this->currentBuffer=&this->InputBuffer[0]; // start with index 0
}


bool H264::setNextAvailableBuffer(void){
	
	// search for next empty buffer:
	for(uint32_t count=this->bufferIndex; count<INPUT_BUFFER_SIZE; count++){
//		fprintf(stderr, "H264: 1-New buffer: count(%u) bufferIndex(%u) SizeOfInputbuffer(%u)...", count,this->bufferIndex,INPUT_BUFFER_SIZE);	
		if(this->InputBuffer[count].isFree()){
			this->bufferIndex=count;
			this->currentBuffer=&this->InputBuffer[count];
//			fprintf(stderr, "found free!\n");	
			return false;
		}
	}
	
	// if we come to here, then try searching from 0 to bufferIndex.:
	for(uint32_t count=0; count<this->bufferIndex; count++){
		if(this->InputBuffer[count].isFree()){
			this->bufferIndex=count;
			this->currentBuffer=&this->InputBuffer[count];
//			fprintf(stderr, "H264: 2-New buffer index(%u) count(%u)\n",this->currentBuffer->getIndex(), count);	
			return false;
		}
	}
	fprintf(stderr, "H264: Input buffer full - bufferindex(%u)\n", this->bufferIndex);
	return true;
}

uint16_t H264::getNextFrameID(void){
	uint32_t next=this->FrameID;
	next++;
	if(next>=MAX_FRAMEID){ // should be 16 bit to the max, (65535).
		next=1; // Frame start at 1.
	}
	return next;
}
 
uint16_t H264::getNextPackagedID(void){
	uint32_t next=this->PackageID;
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

uint32_t H264::getBufferSize(void){
	return this->outputPackages.size();
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
