/*
	telemetryWrapper.cpp
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 
#include "telemetryWrapper.h" 

telemetryWrapper::telemetryWrapper(){
	// clear all memmory:

	bzero(&txTelemetry, sizeof(txTelemetry));
	bzero(&a, sizeof(a));
	bzero(&b, sizeof(b));
	bzero(&telmetryData, sizeof(telmetryData));
	
	 telmetryData.damaged_block_cnt = 0;
	 telmetryData.lost_packet_cnt = 0;
	 telmetryData.skipped_packet_cnt = 0;
	 telmetryData.injection_fail_cnt = 0;
	 telmetryData.received_packet_cnt = 0;
	 telmetryData.kbitrate = 0; // Video rate icon.
	 telmetryData.kbitrate_measured = 0; // shown as "Measured" when clicked on video icon
	 telmetryData.kbitrate_set = 5000000; // shown as "Set" when clicked on video icon
	 telmetryData.lost_packet_cnt_telemetry_up = 0;
	 telmetryData.lost_packet_cnt_telemetry_down = 0;
	 telmetryData.lost_packet_cnt_msp_up = 0;
	 telmetryData.lost_packet_cnt_msp_down = 0;
	 telmetryData.lost_packet_cnt_rc = 0;
	 telmetryData.current_signal_joystick_uplink = -10;
	 telmetryData.current_signal_telemetry_uplink = 0;
	 telmetryData.joystick_connected = 0;
	 telmetryData.HomeLon = 0;
	 telmetryData.HomeLat = 0;
	 telmetryData.cpuload_gnd = 53;
	 telmetryData.temp_gnd = 69;
	 telmetryData.cpuload_air = 11;
	 telmetryData.temp_air = 11;
	 telmetryData.wifi_adapter_cnt = 1;

	 for (int j = 0; j < 6; ++j) {
		 telmetryData.adapter[j].current_signal_dbm = 50;
		 telmetryData.adapter[j].received_packet_cnt = 10;
		 telmetryData.adapter[j].type = 1;
		 telmetryData.adapter[j].signal_good = 1;
	 }

	 telmetryData.adapter[0].signal_good = 1;
	 telmetryData.adapter[0].current_signal_dbm = 50;
	 telmetryData.adapter[0].received_packet_cnt = 10;

/*
	 telmetryData.adapter[0].signal_good = 0xDC;
	 telmetryData.adapter[1].signal_good = 0x30;
	 telmetryData.adapter[2].signal_good = 0x76;
	 telmetryData.adapter[3].signal_good = 0x00;
	 telmetryData.adapter[4].signal_good = 0xd0;
	 telmetryData.adapter[5].signal_good = 0xc4;
	 */
}

void telemetryWrapper::inputDataFromTXTelemetryArray(uint8_t *input, int size){

	if(size == sizeof(this->txTelemetry)){
		this->setCPULoad(input[0]);
		this->setCPUTemperature((int8_t)input[1]);
		this->setRSSI( (int16_t)((int16_t)input[2] +  (int16_t)(input[3] << 8)) );	
	}	
} 

uint8_t * telemetryWrapper::getTelemtryForQOpenHDAsArray(void){
	return (uint8_t *)&this->telmetryData;
}

uint32_t  telemetryWrapper::getTelemtryForQOpenHDSize(void){
	return sizeof(this->telmetryData); // should be 113
//	return 113; // this should be the same as sizeOf(this->rx_status_t)
}

uint8_t * telemetryWrapper::getTXTelemetryAsArray(void){
	return (uint8_t *)&this->txTelemetry;
}

uint32_t  telemetryWrapper::getTXTelemetryArraySize(void){
	return sizeof(this->txTelemetry); 
}



void  telemetryWrapper::setCPULoad(uint8_t load){
	this->txTelemetry.cpuLoad = load;
}

void  telemetryWrapper::setCPUTemperature(int8_t temp){
	this->txTelemetry.cpuTemp = temp;
}


void  telemetryWrapper::setRSSI(int16_t rssi){
	this->txTelemetry.rssidBm = rssi;
}


uint8_t  telemetryWrapper::getCPULoad(void){
	return this->txTelemetry.cpuLoad;
}

uint8_t  telemetryWrapper::getCPUTemperature(void){
	return this->txTelemetry.cpuTemp;
}

int16_t  telemetryWrapper::getRSSI(void){
	return this->txTelemetry.rssidBm;
}



void  telemetryWrapper::setQOpenHDKbitRate(uint32_t kbitss){
	this->telmetryData.kbitrate = kbitss;
}


void  telemetryWrapper::setQOpenHDAirCPULoad(uint8_t load){
	this->telmetryData.cpuload_air = load;
}

void  telemetryWrapper::setQOpenHDAirTemperature(uint8_t temp){
	this->telmetryData.temp_air = temp;
}

void  telemetryWrapper::setQOpenHDRSSI(int16_t rssi){
	if( (rssi >= -128) &&  (rssi <= 127) ){
		this->telmetryData.current_signal_joystick_uplink=(int8_t)rssi;
	}
}




int8_t  telemetryWrapper::readCPUTempFromLinux(void){
	float systemp, millideg;
	FILE *thermal;
	int n;

	thermal = fopen("/sys/class/thermal/thermal_zone0/temp","r");
	n = fscanf(thermal,"%f",&millideg);
	fclose(thermal);
	systemp = millideg / 1000;	
	return (int8_t)systemp;	
}

uint8_t telemetryWrapper::calculateCPULoadFromLinux(void){
	uint8_t res=0;
	FILE *fp;				

	/*								
		fp = fopen("/proc/stat", "r");
		fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &a[0], &a[1], &a[2], &a[3]);
		fclose(fp);
	*/			

	fp = fopen("/proc/stat", "r");
	fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &b[0], &b[1], &b[2], &b[3]);
	fclose(fp);				
				
	res = (((b[0] + b[1] + b[2]) - (a[0] + a[1] + a[2])) / ((b[0] + b[1] + b[2] + b[3]) - (a[0] + a[1] + a[2] + a[3]))) * 100;
	
	// move current cpu time to last cpu time.
	a[0]=b[0];
	a[1]=b[1];
	a[2]=b[2];
	a[3]=b[3];

	return res;
}

int16_t telemetryWrapper::readRSSIFromLinux(void){
	int16_t res=0;
	long double input=0;
	FILE *fp;				

	/*								
		fp = fopen("/proc/stat", "r");
		fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &a[0], &a[1], &a[2], &a[3]);
		fclose(fp);
	*/			

	fp = fopen("/var/run/openhd/signalStatus", "r");
	if(fp){
		fscanf(fp, "%Lf", &input);
		fclose(fp);				
	}
				
	res = (int16_t)input;

	return res;
}
				