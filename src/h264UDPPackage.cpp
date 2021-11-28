/*
	h264UDPPackage.cpp
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 
#include "h264UDPPackage.h" 


H264UDPPackage::H264UDPPackage(){
	// clear all memmory:
	this->clear();
}
 

void H264UDPPackage::clear(void){ // clear all data.
	this->index=0;
	this->FrameID=0;            
    this->PackageID=0; 				    
    bzero(&this->data, sizeof(this->data));
}


bool H264UDPPackage::isFree(void){ 
	if( (this->index==0) && (this->FrameID==0) && (this->PackageID==0) ){
		return true; // data is free.
	}else{
		return false; // not free.
	}
}

bool H264UDPPackage::isFull(void){ // clear all data.
	if(this->index < UDP_DATA_SIZE){
		return false;
	}	
	return true;
}

bool H264UDPPackage::setData(uint16_t length){
	if(length > sizeof(this->data)){  // too large
		return true;
	}
	this->FrameID = (uint16_t)((uint16_t)this->data[0] +  (uint16_t)(this->data[1] << 8));	
	this->PackageID = (uint16_t)((uint16_t)this->data[2] +  (uint16_t)(this->data[3] << 8));	
	this->index=length-UDP_HEADER_SIZE;		
	return false;
}

// input data with pointer to array and length of bytes to copy.
bool H264UDPPackage::setData(void *input, uint16_t length){ // return true if error.
	if(length > sizeof(this->data)){  // too large
		return true;
	}
	
	// too small
	if(length<4){
		fprintf(stderr, "H264_UDPPackage: Input is too small(%u)\n",length);	
		return true;
	}
	
	memcpy(&this->data, input, length);
	this->FrameID = (uint16_t)((uint16_t)this->data[0] +  (uint16_t)(this->data[1] << 8));	
	this->PackageID = (uint16_t)((uint16_t)this->data[2] +  (uint16_t)(this->data[3] << 8));	
	this->index=length-UDP_HEADER_SIZE;
	
	if(this->index == 0){ // this is a zero data?
		fprintf(stderr, "H264_UDPPackage: Error, adding a zero package? PackageID(%u) FrameID(%u) - Data (0-10): ", this->PackageID,this->FrameID);					
		for(int a=0;a<10; a++){			
			fprintf(stderr, "%02x ", this->data[a]);	
		}		
		fprintf(stderr, "\n");	
	}

	return false;
}
bool H264UDPPackage::addData(uint8_t input){ // return true when full.

	if(this->isFull()){
		return true;
	}else{
		int place= (int)(this->index+UDP_HEADER_SIZE);
		if(place >= sizeof(this->data)){
			fprintf(stderr, "H264_UDPPackage: place is to large(%d)\n",place);	
		}else if(place < 0){
			fprintf(stderr, "H264_UDPPackage: place is negative (%d)\n",place);	
		}else{
			//fprintf(stderr, "%u ",place);	
			this->data[place]=input;
		}

		if(this->index < UDP_DATA_SIZE){
			this->index++;
		}

		if(this->index >= UDP_DATA_SIZE){ //Data is now full
			return true;
		}
	}
	return false; // room for more data.
}

uint16_t H264UDPPackage::getIndex(void){
	return this->index;
}

uint8_t * H264UDPPackage::getPackage(void){
	// set FrameID and Pacakge ID to data array and return the pointer to it.
	data[0]= (uint8_t)(this->FrameID & 0x00FF);
	data[1]= (uint8_t)((this->FrameID >> 8) & 0x00FF);
	data[2]= (uint8_t)(this->PackageID & 0x00FF);
	data[3]= (uint8_t)((this->PackageID >> 8) & 0x00FF);
		
	uint8_t *p;
	p=&this->data[0];
	return p;
} 

uint16_t H264UDPPackage::getPackageSize(void){ 
	uint16_t size=0;
	size = this->index+UDP_HEADER_SIZE;
	if(size > sizeof(this->data)){
		fprintf(stderr, "H264_UDPPackage: Package size is to large(%u) - max size is (%u)\n",size, sizeof(this->data));	
	}
	return size; // including the Header;

}

uint8_t * H264UDPPackage::getPayload(void){
	uint8_t *p;
	p=&this->data[UDP_HEADER_SIZE];
	return p;
} 


uint16_t H264UDPPackage::getPayloadSize(void){ // returns the size of the data.
	return this->index;
}

uint16_t H264UDPPackage::getPackageMaxSize(void){
	return UDP_PACKET_SIZE;
}

uint16_t H264UDPPackage::getPackageID(void){
	return this->PackageID;
}

uint16_t H264UDPPackage::getFrameID(void){
	return this->FrameID;
}

void H264UDPPackage::setPackageID(uint16_t packageID){
	this->PackageID = packageID;
}

void H264UDPPackage::setFrameID(uint16_t frameID){
	this->FrameID = frameID;
}

bool H264UDPPackage::isNewFrame(void){
	// Is this a I-frame start?
	if( this->data[0+UDP_HEADER_SIZE]==0x00 && this->data[1+UDP_HEADER_SIZE]==0x00 && this->data[2+UDP_HEADER_SIZE]==0x00 && this->data[3+UDP_HEADER_SIZE]==0x01){
		// this could be new frame:
	 	if(this->data[4+UDP_HEADER_SIZE]==0x21){ // I-frame, yes new frame!
			return true;
		}
	 	if(this->data[4+UDP_HEADER_SIZE]==0x25){ // K-frame, yes new frame!
			return true;
		}
	}
	return false;
}


bool H264UDPPackage::isNewKeyFrame(void){
	// Is this a keyframe start?
	if( this->data[0+UDP_HEADER_SIZE]==0x00 && this->data[1+UDP_HEADER_SIZE]==0x00 && this->data[2+UDP_HEADER_SIZE]==0x00 && this->data[3+UDP_HEADER_SIZE]==0x01 &&this->data[4+UDP_HEADER_SIZE]==0x25){
		return true;
	}
	return false;	
}

bool H264UDPPackage::isHeaderFrame(void){
	if( this->data[0+UDP_HEADER_SIZE]==0x00 && this->data[1+UDP_HEADER_SIZE]==0x00 && this->data[2+UDP_HEADER_SIZE]==0x00 && this->data[3+UDP_HEADER_SIZE]==0x01 &&this->data[4+UDP_HEADER_SIZE]==0x27){
		return true;
	}
	return false;		
} // returns true if this packages is the header.


bool H264UDPPackage::isNewerThan(uint16_t FrameID, uint16_t PackageID){
	if(this->FrameID > FrameID){
		return true;
	}else if(this->FrameID == FrameID){
		if(this->PackageID > PackageID){ // so what happens if packageID overflows?
			return true;
		}
	}	
	return false;
}
