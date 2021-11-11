/*
	h264TXFraming.cpp
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 
#include "h264TXFraming.h" 


H264TXFraming::H264TXFraming(){

}
 
// input data with pointer to array and length of bytes to copy.
void H264TXFraming::inputStream(uint8_t *data, uint32_t length){

	// Scan througth input and find H264 headers:
	// Start with SPS + PPS header.
	// 0x00 0x00 0x00 0x01 0x27 (SPS header, size is 15)
	// 0x00 0x00 0x00 0x01 0x28 (PPS header, size is 4)
	// 0x00 0x00 0x00 0x01 0x25 (P-header (keyframe))
	// 0x00 0x00 0x00 0x01 0x21 (I-Frame)
	
	// Save the (SPS + PPS) data (23 bytes) in startHeader[].
	
	for(uint16_t index=0; index<length; index++){
		//fprintf(stderr, "Handling input (%u) with length(%u)\n", data[index], length);	
		switch(this->state)
		{
			case LOOK_FOR_HEADER_00:
				if(data[index] == 0x00){
					this->state=LOOK_FOR_HEADER_00_2;
				}else if(this->savingStream == true){ // We are running, save the data.								
					this->addData(data[index]);
				}
			break;			
			
			case LOOK_FOR_HEADER_00_2:
				if(data[index] == 0x00){
					this->state=LOOK_FOR_HEADER_00_3;
				}else{
					this->state=LOOK_FOR_HEADER_00;
					// Save the prev. 0x00 as it is data, not header.
					if(this->savingStream == true){ // We are running, save the data.
						this->addData(0x00);
						this->addData(data[index]);
					}
				}
			break;			
			
			case LOOK_FOR_HEADER_00_3:
				if(data[index] == 0x00){
					this->state=LOOK_FOR_HEADER_01;
				}else{
					this->state=LOOK_FOR_HEADER_00;
					// Save the prev. 0x00 0x00 as it is data, not header.
					if(this->savingStream == true){ // We are running, save the data.
						this->addData(0x00);
						this->addData(0x00);
						this->addData(data[index]);
					}
				}
			break;		

			case LOOK_FOR_HEADER_01:
				if(data[index] == 0x01){
					this->state=ANALYSE_HEADER;
				}else{
					this->state=LOOK_FOR_HEADER_00;
					// Save the prev. 0x00 0x00 0x00 as it is data, not header.
					if(this->savingStream == true){ // We are running, save the data.
						this->addData(0x00);
						this->addData(0x00);
						this->addData(0x00);
						this->addData(data[index]);
					}
				}
			break;					
			
			case ANALYSE_HEADER: 
				// start with I frame since this is the most occuring header:
				if(data[index] == 0x21){ // I frame:
//					fprintf(stderr, "H264_TX: I-Frame Header found.\n");
					this->startNewPackage(false); // split on I-frame.
					this->addData(0x00);
					this->addData(0x00);
					this->addData(0x00);
					this->addData(0x01);
					this->addData(0x21);	
					this->state=LOOK_FOR_HEADER_00;					
				}else if(data[index] == 0x25){ // Keyframe
			//		fprintf(stderr, "H264_TX: Keyframe found in input stream, placed at (%u).\n", this->FifoState.InputPackageID);
					this->startNewPackage(true); // split on keyframe.
					this->savingStream=true;
					this->addData(0x00);
					this->addData(0x00);
					this->addData(0x00);
					this->addData(0x01);
					this->addData(0x25);		
					this->state=LOOK_FOR_HEADER_00;					
				}else if(data[index] == 0x27){ // SPS Header
					fprintf(stderr, "H264_TX: SPS Header found.\n");
					this->startHeader.data[0]=0x00;
					this->startHeader.data[1]=0x00;
					this->startHeader.data[2]=0x00;
					this->startHeader.data[3]=0x01;
					this->startHeader.data[4]=0x27;
					this->startHeader.counter=5;
					this->state=GET_SPS_HEADER;
				}else if(data[index] == 0x28){ // PPS Header
					fprintf(stderr, "H264_TX: PPS Header found.\n");
					this->startHeader.data[20]=0x00;
					this->startHeader.data[21]=0x00;
					this->startHeader.data[22]=0x00;
					this->startHeader.data[23]=0x01;
					this->startHeader.data[24]=0x28;	
					this->startHeader.counter=25;					
					this->state=GET_PPS_HEADER;
				}else if(data[index] == 0x09){ // Unknown, but lets keep it.
					this->addData(0x00);
					this->addData(0x00);
					this->addData(0x00);
					this->addData(0x01);
					this->addData(0x09);		
					this->state=LOOK_FOR_HEADER_00;
				}else{
					// Error Header not reconiced.
					this->state=LOOK_FOR_HEADER_00;
					fprintf(stderr, "H264_TX: Error - header (%u) not reconized in inputstream\n",data[index]);
				}
			break;
			
			case GET_SPS_HEADER: 
			{
				
				this->startHeader.data[this->startHeader.counter] = data[index]; // Save header.
				
				this->startHeader.counter++;
				if(this->startHeader.counter >= 20){
					this->state=LOOK_FOR_HEADER_00;
				}
			}
			break;
			
			
			
			case GET_PPS_HEADER: 
			{
				this->startHeader.data[this->startHeader.counter] = data[index]; // Save header.
				
				this->startHeader.counter++;
				if(this->startHeader.counter >= 29){
					this->state=LOOK_FOR_HEADER_00; // start saving data.
					fprintf(stderr, "H264_TX: header found:");
					for(int a=0;a<29; a++){
						fprintf(stderr, "%02x ", this->startHeader.data[a]);	
						this->addData(this->startHeader.data[a]);
					}
					fprintf(stderr, "\n");	
				}
			}
			break;
				
			default:
				fprintf(stderr, "H264_TX: Error - state (%u) unkown in input stream statemachine\n",this->state);
			break;	
		}
	}
	this->addBytesInputted(length); // count bytes inputted.
}


void H264TXFraming::addData(uint8_t data){
	// add data to buffer and if full transfer buffer to Output FIFO and start a new buffer:
	
	if(this->currentBuffer->addData(data)){ // buffer is now full
		this->startNewPackage(false);
	}
}

void H264TXFraming::startNewPackage(bool keyframe){
//	fprintf(stderr, "H264_TX: Start new package...");
	
	this->PackageID=getNextPackagedID();

	if(keyframe){
		this->FrameID=getNextFrameID();
				
		// do we need to trim the output FIFO?	
		this->trimOutputFIFO();
	}
	this->currentBuffer->setFrameID(this->FrameID);
	this->currentBuffer->setPackageID(this->PackageID);

/*	
	fprintf(stderr, "H264_TX: TX Package complete - FrameID(%u) PackageID(%u) and size (%u) - is Keyframe(%u) : ",this->currentBuffer->getFrameID(),this->currentBuffer->getPackageID(),this->currentBuffer->getSize(),this->currentBuffer->isNewKeyFrame() );	
	
	uint8_t *p = this->currentBuffer->getData();
	for(int a=0;a<10;a++){
		fprintf(stderr, " %02x", p[a+4]);
	}
	fprintf(stderr, "\n");
	
	*/
	
	this->outputPackages.push(this->currentBuffer); // add current buffer pointer to FIFO.
//	fprintf(stderr, "Package saved with FrameID(%u) PacakgeID(%u). outputPackages size(%u) - local FrameID(%u) and PackageID(%u)\n",this->currentBuffer->getFrameID(), this->currentBuffer->getPackageID(), this->outputPackages.size(),  this->FrameID, this->PackageID);
	if(this->setNextAvailableBuffer()){
		// no buffer availble:
		fprintf(stderr, "H264_TX: Error - Input buffer full\n");	
			// clear all input and resync on next keyframe?
	}
}

void H264TXFraming::trimOutputFIFO(void){
	uint32_t size = this->outputPackages.size();
	uint32_t bytesDropped=0;
	
	if(size > 0){
		uint16_t frameIDforTX=this->outputPackages.front()->getFrameID();
		
		for(int a=0; a<size; a++){
			if(this->outputPackages.front()->getFrameID() < (this->FrameID-1) ){ // sunc on next keyframe
				fprintf(stderr, "H264_TX: Dropping package in txOutputFIFO - FrameID(%u) PackageID(%u)\n",this->outputPackages.front()->getFrameID(),this->outputPackages.front()->getPackageID());	
				// remove data because it is too old.
				bytesDropped = bytesDropped + this->outputPackages.front()->getPayloadSize(); // Only count the actual payload data as dropped, not the header we have made :-)
				this->addBytesOutputted(this->outputPackages.front()->getPackageSize()); // count all bytes sent, therefore Package not just Payload.
				this->outputPackages.front()->clear();
				this->outputPackages.pop();
			}else{
				break;
			}		
		}
		
		if(bytesDropped>0){
			fprintf(stderr, "H264_TX: Output FIFO trimmed. FrameID for TX was(%u) and latest input is (%u) - thus (%u) bytes was dropped.\n",frameIDforTX,this->FrameID,bytesDropped);	
			this->addBytesDropped(bytesDropped); // count bytes dropped.
		}
	}
}
	
uint16_t H264TXFraming::getTXPackage(uint8_t * &data){
	if(this->outputPackages.empty()){
		return 0;
	}
	
	uint16_t size = this->outputPackages.front()->getPackageSize();
	
	if(size == 0 ){
		return 0;
	}
//	fprintf(stderr, "H264_TX: Outputting TXPacakge with FrameID(%u) PacakgeID(%u). outputPackages size(%u)\n", this->outputPackages.front()->getFrameID(),this->outputPackages.front()->getPackageID(), this->outputPackages.size());
	data = this->outputPackages.front()->getPackage();
	return size;
}

void H264TXFraming::nextTXPackage(void){
	if( !(this->outputPackages.empty()) ){
//		fprintf(stderr, "H264_TX: TX Package successfully extracted, remove Package from txoutput. Before size(%u) ", this->outputPackages.size());
		this->addBytesOutputted(this->outputPackages.front()->getPackageSize()); // count bytes sent.
		this->outputPackages.front()->clear(); // free the data		
		this->outputPackages.pop();

//		fprintf(stderr, "After Size(%u)\n", this->outputPackages.size());
	}
}