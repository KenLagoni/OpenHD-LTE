/*
	h264RXFraming.cpp
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 
#include "h264RXFraming.h" 


H264RXFraming::H264RXFraming(){
	// clear all memmory:
}


uint16_t H264RXFraming::getPackageMaxSize(void){ // returns maximum data size.
	return this->currentBuffer->getPackageMaxSize();
}

bool H264RXFraming::setData(void *buf, uint16_t length){

/*
	fprintf(stderr, "H264_RX: setData - Package (Header 0-3) (Payload 4-10): ");					
	uint8_t *p;
	p=(uint8_t *)buf;
	for(int a=0;a<10; a++){			
		fprintf(stderr, "%02x ", *p);	
		p++;
	}		
	fprintf(stderr, "\n");	
*/
	if(length == 0){
		fprintf(stderr, "H264_RX: Error in setData, length is 0\n");					
		return true;
	}
	
	// finish the current input buffer:
	if(this->currentBuffer->setData(buf, length)){
		fprintf(stderr, "H264_RX: Input length too large\n");	
		return true;
	}	
	
	// Service the last data -> this->inputRXPackage.
	this->serviceRXPackage();
	this->addBytesInputted(length); // count bytes inputted.
	
	// jump to next 
	if(this->setNextAvailableBuffer()){
		uint8_t *p;
		
		// Inputbuffer - This is the actual data
		for(int a=0;a<INPUT_BUFFER_SIZE;a++){
			p = this->InputBuffer[a].getPackage(); // for debug
			fprintf(stderr, "H264_RX: buffer place(%u) - FrameID(%u) - PackageID(%u) - Package size(%u) package data (0-10): ", a, this->InputBuffer[a].getFrameID(), this->InputBuffer[a].getPackageID(), this->InputBuffer[a].getPackageSize());
			for(int b=0;b<10; b++){			
				fprintf(stderr, "%02x ", *p);	
				p++;
			}		
			fprintf(stderr, "\n");	
		}
		fprintf(stderr, "\n");	

		// Input data - This is the vector which cashes the pointer to packages not yet ready to temp output frame 
		uint32_t size = this->inputData.size();
		fprintf(stderr, "H264_RX: Input data size(%u)\n", size);
		for(int a=0;a<size;a++){
			p = this->inputData[a]->getPackage(); // for debug
			fprintf(stderr, "H264_RX: input data place(%u) - FrameID(%u) - PackageID(%u) - Package size(%u) package data (0-10): ", a, this->inputData[a]->getFrameID(), this->inputData[a]->getPackageID(), this->inputData[a]->getPackageSize());
			for(int b=0;b<10; b++){			
				fprintf(stderr, "%02x ", *p);	
				p++;
			}		
			fprintf(stderr, "\n");	
		}

		// Temp Output frame - This where we build the next output frame. (again pointer to data)
		fprintf(stderr, "\n");	
		H264UDPPackage *buf;
		size = this->tempOutputFrame.size();
		fprintf(stderr, "H264_RX: Temp Output frame - data size(%u)\n", size);
		for(int a=0;a<size;a++){
			buf = this->tempOutputFrame.front();
			p=buf->getPackage();
			fprintf(stderr, "H264_RX: Temp Output frame - data place(%u) - FrameID(%u) - PackageID(%u) - Package size(%u) package data (0-10): ", a, buf->getFrameID(), buf->getPackageID(), buf->getPackageSize());
			for(int b=0;b<10; b++){			
				fprintf(stderr, "%02x ", *p);	
				p++;
			}		
			fprintf(stderr, "\n");	
			this->tempOutputFrame.pop();
		}


		// This is the output frame ready for STDOUT -  (again pointer to data)
		fprintf(stderr, "\n");	
		size = this->outputPackages.size();
		fprintf(stderr, "H264_RX: Output Frame ready for STDOUT - data size(%u)\n", size);
		for(int a=0;a<size;a++){
			buf = this->outputPackages.front();
			p=buf->getPackage();
			fprintf(stderr, "H264_RX: Output Frame - data place(%u) - FrameID(%u) - PackageID(%u) - Package size(%u) package data (0-10): ", a, buf->getFrameID(), buf->getPackageID(), buf->getPackageSize());
			for(int b=0;b<10; b++){			
				fprintf(stderr, "%02x ", *p);	
				p++;
			}		
			fprintf(stderr, "\n");	
			this->outputPackages.pop();
		}

		do{	
			fprintf(stderr, "*");	
		}while(1);

		return true;
	}
	
	
	
	return false;
}


bool H264RXFraming::setData(uint16_t length){
	if(length == 0){
		fprintf(stderr, "H264_RX: Error in setData, length is 0\n");					
		return true;
	}

	// finish the current input buffer:
	if(this->currentBuffer->setData(length)){
		fprintf(stderr, "H264_RX: Input length too large\n");	
		return true;
	}	
	
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


uint32_t H264RXFraming::getOutputStream(void *buf, uint16_t maxLength){

	
	
	uint32_t res=0;
	if(this->outputPackages.size() > 0){
		int paylodSize = this->outputPackages.front()->getPayloadSize();	
		if( (paylodSize <= 0) || paylodSize>maxLength){
			fprintf(stderr, "H264_RX: getOutputStream failed, payload size(%u) must be larger than 0 and less than maxLength(%u)\n",paylodSize, maxLength);		
		}else{
			memcpy(buf, this->outputPackages.front()->getPayload(), paylodSize);
		
			this->outputPackages.pop();
			return paylodSize;
		}
	}
	
	return res;
}



void H264RXFraming::writeAllOutputStreamTo(int fd){
	bool moreData=false;
	uint32_t numberOfBytes=0;

	if(firstTime){
		firstTime=false;
		uint8_t fileNumber = 0;
		char recordename[50];
		bool retry=true;

		do{
			sprintf(recordename,"/home/pi/groundRecording%d.h264",fileNumber);
			std::ifstream file_to_check (recordename);
			if(file_to_check.is_open()){
				fileNumber++;
			}else{
				retry = false;
			}
		}while(retry);

		fprintf(stderr, "H264_RX: Open file for debug STDIN output: (%s)\n", recordename);			
		this->videoRecordFile.open(recordename, std::ios::binary); // open file for write.	
	}

	do{
		moreData=false;
		if(this->outputPackages.size() > 0){		
		    int err, n;
			int paylodSize = this->outputPackages.front()->getPayloadSize();		
			if( (paylodSize > 0) && (paylodSize <= UDP_PAYLOAD_SIZE) ){
				n = write(fd, this->outputPackages.front()->getPayload(), paylodSize);	
				err = errno; // save off errno, because because the printf statement might reset it
				if((n < 0)){ // Error
					if ((err == EAGAIN) || (err == EWOULDBLOCK)){
						 //fprintf(stderr, "H264_RX: non-blocking operation returned EAGAIN or EWOULDBLOCK\n");	
						 moreData=false;
					}else{
						 if(err == EBADF){
							 fprintf(stderr, "H264_RX:  The argument sockfd is an invalid file descriptor.\n\r");	 
						 }else if(err == ECONNREFUSED){
							 fprintf(stderr, "H264_RX:  A remote host refused to allow the network connection.\n\r");
						 }else if(err == EFAULT){
							 fprintf(stderr, "H264_RX:  The receive buffer pointer(s) point outside the process's address space.\n\r");
						 }else if(err == EINTR){
							 fprintf(stderr, "H264_RX:  The receive was interrupted by delivery of a signal before any data was available.\n\r");
						 }else if(err == EINVAL){
							 fprintf(stderr, "H264_RX:  Invalid argument passed.\n\r");
						 }else if(err == ENOMEM){
							 fprintf(stderr, "H264_RX:  Could not allocate memory for recvmsg().\n\r");
						 }else if(err == ENOTCONN){
							 fprintf(stderr, "H264_RX:  The socket is associated with a connection-oriented protocol and has not been connected.\n\r");
						 }else if(err == ENOTSOCK){
							 fprintf(stderr, "H264_RX:  The file descriptor sockfd does not refer to a socket.\n\r");
						 }else if(err == -1){
							 fprintf(stderr, "H264_RX:  Socket invalid, thus closing file descriptor.\n\r");
						 }else{
							 fprintf(stderr, "H264_RX:  Unknown error - reading with result=%d, thus closing\n",(int)n);
						 }	 
					}
				}else{
		//			this->videoRecordFile.write((char *)this->outputPackages.front()->getPayload(), paylodSize); // write to output file for debug.
					this->addBytesOutputted(this->outputPackages.front()->getPayloadSize());
					this->outputPackages.front()->clear();
					this->outputPackages.pop();
					moreData=true;
				}
			}else if(paylodSize==0){
				// empty.
				
				moreData=false;
			}else{
				fprintf(stderr, "H264_RX:  Payload size not allowed %d, must be larger than 0 and less (or equel) to UDP_PAYLOAD_SIZE (%u)\n",paylodSize, UDP_PAYLOAD_SIZE);
			}
		}
	}while(moreData);
}


uint32_t H264RXFraming::getInputQueueSize(void){

	return this->inputData.size();
}

uint32_t H264RXFraming::getTempOutputSize(void){
	
	return this->tempOutputFrame.size();
}

bool H264RXFraming::waitingForHeader(void){ // returns true of waiting for the header.
	return !(this->headerFound);
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////// Private Helper functions //////////////////////////
//////////////////////////////////////////////////////////////////////////////

bool H264RXFraming::serviceRXPackage(void){
//	fprintf(stderr, "H264_RX: Input Package with FrameID(%u) and PackageID(%u) and PayloadSize (%u) received. InputBuffer size(%u), tempOutput size(%u), OutputFIFO size(%u)... \n",this->currentBuffer->getFrameID(), this->currentBuffer->getPackageID(), this->currentBuffer->getPayloadSize(),this->inputData.size(), this->tempOutputFrame.size(), this->outputPackages.size());	

	if(this->currentBuffer->isHeaderFrame()){
		fprintf(stderr, "H264_RX: Header frame found!\n");
		this->headerFound=true;
		this->clearOutputFrame();
		this->clearInputData();
		this->buildOutputFrame(this->currentBuffer); //add data to tempOutputFrame.
		this->FrameID = this->currentBuffer->getFrameID();	// reset to new frameId. but I don't think we ever use this.?
		return false;
	}


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
			// fprintf(stderr, "H264_RX: We are Stuck! - but new frame is keyframe with packageID (%u) so lets sync on this. Input Data buffer size(%u) and TempOutputframe size(%u)\n",this->currentBuffer->getPackageID(), this->inputData.size(), this->tempOutputFrame.size());	
			this->clearOutputFrame();
			this->clearInputDataWithPackagesOlderThan(this->currentBuffer);
			fprintf(stderr, "*4*");					
			this->buildOutputFrame(this->currentBuffer); //add data to tempOutputFrame.
			this->checkInputBufferForMoreData(); // check if inputData buffer has more packages:
		}else{
			// Add data to Input FIFO.
			this->inputData.push_back(this->currentBuffer);	
		}
	}
	
	//
	
	
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
		for(uint32_t i =0; i<size ; i++){
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
		dataFound = false;
		size = this->inputData.size();
		if(size > 0){
	//		fprintf(stderr, "H264_RX: Input Data buffer has (%u) elements, searching if they can be used...",size);	
			for(uint32_t element =0; element<size ; element++){
				if(this->isNextPackage(this->inputData[element])){
					fprintf(stderr, "*1*");					
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
	uint32_t payloadSize = package->getPayloadSize();
	if(payloadSize == 0){
		fprintf(stderr, "H264_RX: Error, adding a zero package? size(%u) PackageID(%u) FrameID(%u) - Payload (0-10): ",payloadSize, package->getPackageID(), package->getFrameID());					
		uint8_t *p;
		p=package->getPayload();
		for(int a=0;a<10; a++){			
			fprintf(stderr, "%02x ", *p);	
			p++;
		}		
		fprintf(stderr, "\n");	
	}else{
		this->tempOutputFrame.push(package);
		this->PackageID = package->getPackageID();		
	}
}


bool H264RXFraming::isNextPackage(H264UDPPackage *package){
	if(package->getPackageID() == this->getNextPackagedID()){
		return true;
	}
	return false;
}


void H264RXFraming::clearInputData(void){
    // Fast first (it is highly likly all data in Inputdata is old, thus first
	std::vector<H264UDPPackage *> empty;
 	
	uint32_t size = 0;
	size = this->inputData.size();
	if(size > 0){
		uint32_t bytesDropped=0;
		for(uint32_t i =0; i<size ; i++){
			bytesDropped = bytesDropped + this->inputData[i]->getPackageSize(); // include the header size it has been transported to rx(ground).
			this->inputData[i]->clear(); // free memmory.					
		}
		this->addBytesDropped(bytesDropped); // count bytes dropped.
		//fprintf(stderr, "H264_RX: Removing (%u) old packages from Input Data buffer but keeping (%u) which is newer. A total of (%u) bytes dropped\n", old, keep, bytesDropped);			
		std::swap(this->inputData, empty); 
	}	
}

	

