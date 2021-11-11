/*
	h264RXFraming.cpp
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 
#include "h264RXFraming.h" 


H264RXFraming::H264RXFraming(){
	// clear all memmory:
}


uint8_t * H264RXFraming::getInputBuffer(void){ // returns pointer to the an available input buffer.
	return this->currentBuffer->getPackage();
}


uint16_t H264RXFraming::getPackageMaxSize(void){ // returns maximum data size.
	return this->currentBuffer->getPackageMaxSize();
}


bool H264RXFraming::setData(uint16_t length){

	// finish the current input buffer:
	this->currentBuffer->setData(length);
	
	// Service the last data -> this->inputRXPackage.
	this->serviceRXPackage();
	
	this->addBytesInputted(length); // count bytes inputted.
		
	// jump to next 
	if(this->setNextAvailableBuffer()){
		fprintf(stderr, "H264_RX: Input buffer full\n");	
		return true;
	}
	return false;
}


uint32_t H264RXFraming::getOutputStreamFIFOSize(void){
	return (uint32_t)this->outputPackages.size();
}


void H264RXFraming::writeAllOutputStreamTo(int fd){
	bool moreData=false;
	uint32_t numberOfBytes=0;
	do{
		if(this->outputPackages.size() > 0){		
			write(fd, this->outputPackages.front()->getPayload(), this->outputPackages.front()->getPayloadSize());	
			this->addBytesOutputted(this->outputPackages.front()->getPayloadSize());
			this->outputPackages.front()->clear();
			this->outputPackages.pop();
			moreData=true;
		}else{
			moreData=false;
		}
	}while(moreData);
}


//////////////////////////////////////////////////////////////////////////////
////////////////////////// Private Helper functions //////////////////////////
//////////////////////////////////////////////////////////////////////////////

bool H264RXFraming::serviceRXPackage(void){
//	fprintf(stderr, "H264_RX: Input Package with FrameID(%u) and PackageID(%u) and size (%u) received. InputBuffer size(%u), tempOutput size(%u), OutputFIFO size(%u)... ",this->currentBuffer->getFrameID(), this->currentBuffer->getPackageID(), this->currentBuffer->getSize(),this->inputData.size(), this->tempOutputFrame.size(), this->outputPackages.size());	

	// Is this the next package we are expecting?
	if(this->isNextPackage(this->currentBuffer)){	
//		fprintf(stderr, "Match!\n");	
		// If this frame has a key- or I-frame start header, then all the data in the output fifo is complete and it can be sent to outputstream, thus:
		if(this->currentBuffer->isNewFrame()){
			// move tempOutputFrame to OutputFIFO.
			this->finishOutputFrame();
		}

		//add data to tempOutputFrame.
		this->buildOutputFrame(this->currentBuffer);
		
		// check if inputData buffer has more packages:
		this->checkInputBufferForMoreData();
	}else{
//		fprintf(stderr, "Was expecting PackageID(%u), so not next package.\n", this->getNextPackagedID());	
		// If this frame has a keyframe start header, then we should resync to this:	
		if(this->currentBuffer->isNewKeyFrame()){
			// fprintf(stderr, "H264_RX: We are Stuck! - but new frame is keyframe with pacakgeID (%u) so lets sync on this. Input Data buffer size(%u) and TempOutputframe size(%u)\n",this->currentBuffer->getPackageID(), this->inputData.size(), this->tempOutputFrame.size());	
			this->clearOutputFrame();
			this->clearInputDataWithPackagesOlderThan(this->currentBuffer);
			this->buildOutputFrame(this->currentBuffer); //add data to tempOutputFrame.
			this->checkInputBufferForMoreData(); // check if inputData buffer has more packages:
		}else{
			// Add data to Input FIFO.
			this->inputData.push_back(this->currentBuffer);	
		}
	}
	return false;
}

void H264RXFraming::clearInputDataWithPackagesOlderThan(H264UDPPackage *input){
	// Fast first (it is highly likly all data in Inputdata is old, thus first
	std::vector<H264UDPPackage *> onlyNewerPackages;
 	
	uint32_t size = 0;
	size = this->inputData.size();
	if(size > 0){
		uint16_t old=0;
		uint16_t keep=0;
		uint32_t bytesDropped=0;
		for(uint16_t i =0; i<size ; i++){
			if(this->inputData[i]->isNewerThan(input->getFrameID(), input->getPackageID())){ //
				keep++;
				onlyNewerPackages.push_back(this->inputData[i]);
			}else{
				old++;
				bytesDropped = bytesDropped + this->inputData[i]->getPackageSize(); // include the header size it has been transported to rx(ground).
				this->inputData[i]->clear(); // free memmory.
			}					
		}
		this->addBytesDropped(bytesDropped); // count bytes dropped.
		//fprintf(stderr, "H264_RX: Removing (%u) old packages from Input Data buffer but keeping (%u) which is newer. A total of (%u) bytes dropped\n", old, keep, bytesDropped);			
		std::swap(this->inputData, onlyNewerPackages); 
	}
}


void H264RXFraming::checkInputBufferForMoreData(void){
	
	bool dataFound = false;
	uint32_t size = 0;
	do{
		size = this->inputData.size();
		if(size > 0){
	//		fprintf(stderr, "H264_RX: Input Data buffer has (%u) elements, searching if they can be used...",size);	
			for(uint16_t element =0; element<size ; element++){
				if(this->isNextPackage(this->inputData[element])){
	//				fprintf(stderr, "OK(%u)\n",element);					
					this->buildOutputFrame(this->inputData[element]); // add the data from inputbuffer to output buffer.					
					this->inputData.erase(this->inputData.begin() + element); // erease data.
					dataFound=true; // will must search again.
				}					
			}
		}
	}while(true==dataFound);

	//if( (false==dataFound) && (size!=0)){
	//	fprintf(stderr, "\n");						
	//}
}


void H264RXFraming::clearOutputFrame(void){
	
	// we hace to run throught all of them to clear the data, else it will fill the buffer:
	uint32_t size = this->tempOutputFrame.size();
	if(size > 0){
		uint32_t bytesDropped=0;
		//fprintf(stderr, "H264_RX: flushing tempOutputFrame buffer with (%u) packages ",size);					
		for(uint32_t i=0;i<size;i++){
			bytesDropped = bytesDropped +this->tempOutputFrame.front()->getPackageSize(); // include the header size it has been transported to rx(ground).
			this->tempOutputFrame.front()->clear(); // free the data in the buffer		 
			this->tempOutputFrame.pop();
		}		
		this->addBytesDropped(bytesDropped); // count bytes dropped.
		//fprintf(stderr, "(a total of %u bytes dropped)\n",bytesDropped);	
	}
}


void H264RXFraming::finishOutputFrame(void){
	uint32_t size = this->tempOutputFrame.size();
//	fprintf(stderr, "H264_RX: Temp Output Frame with (%u) packages is complete, moving it to output FIFO\n",size);					
	for(uint32_t i=0;i<size;i++){
		this->outputPackages.push(this->tempOutputFrame.front());
		this->tempOutputFrame.pop();
	}		
}



void H264RXFraming::buildOutputFrame(H264UDPPackage *package){
	this->tempOutputFrame.push(package);
	this->PackageID = package->getPackageID();	
}


bool H264RXFraming::isNextPackage(H264UDPPackage *package){
	if(package->getPackageID() == this->getNextPackagedID()){
		return true;
	}
	return false;
}

