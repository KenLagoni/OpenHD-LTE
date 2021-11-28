  
/*
	telemetryWrapper.h
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef TELEMETRIWRAPPER_H_
#define TELEMETRIWRAPPER_H_

#include <stdint.h> 
#include <cstdio>
#include <strings.h> // bzero
#include <queue>
#include <iostream>

class telemetryWrapper
{
	// Public functions to be used on all Messages
	public:	
	telemetryWrapper(); 
	virtual ~telemetryWrapper(){}; //destructor

	void inputDataFromTXTelemetryArray(uint8_t *input, int size); 
	
	// For Telemetry to QOpenHD
	uint8_t * getTelemtryForQOpenHDAsArray(void);
	uint32_t getTelemtryForQOpenHDSize(void);
	
	// For smaller telemetry info from TX to ground:
	uint8_t * getTXTelemetryAsArray(void);
	uint32_t getTXTelemetryArraySize(void);

	// Set TX Telemetry
	void setCPULoad(uint8_t load);
	void setCPUTemperature(int8_t temp);
	void setRSSI(int16_t rssi);

	// Get TX Telemetry
	uint8_t getCPULoad(void);
	uint8_t getCPUTemperature(void);
	int16_t getRSSI(void);


	// Set parameters
	void setQOpenHDKbitRate(uint32_t kbitss);
	void setQOpenHDAirCPULoad(uint8_t load);
	void setQOpenHDAirTemperature(uint8_t temp);

	uint8_t calculateCPULoadFromLinux(void);
	int8_t readCPUTempFromLinux(void);  
	int16_t readRSSIFromLinux(void);  


	// Parameters used by the classes using this
	protected:

		// Parameters only used on mother class.
	private:
	long double a[4], b[4]; // for Cpuload calculations


	typedef struct {
		uint8_t cpuLoad;
		int8_t cpuTemp;
		int16_t rssidBm;
	} __attribute__((packed)) telematryFrame_t;	

	telematryFrame_t txTelemetry;

	
	typedef struct {
		uint32_t received_packet_cnt;
		int8_t current_signal_dbm;
		int8_t type; // 0 = Atheros, 1 = Ralink
		int8_t signal_good;
	} __attribute__((packed)) wifi_adapter_rx_status_forward_t;

	typedef struct { // size is 113
		uint32_t damaged_block_cnt;              // number bad blocks video downstream
		uint32_t lost_packet_cnt;                // lost packets video downstream
		uint32_t skipped_packet_cnt;             // skipped packets video downstream (shownen under video icon as second number)
		uint32_t injection_fail_cnt;             // Video injection failed downstream (shownen under video icon as first number)
		uint32_t received_packet_cnt;            // packets received video downstream
		uint32_t kbitrate;                       // live video kilobitrate per second video downstream (Video rate icon).
		uint32_t kbitrate_measured;              // shown as "Measured" when clicked on video icon)
		uint32_t kbitrate_set;                   // shown as "Set" when clicked on video icon
		uint32_t lost_packet_cnt_telemetry_up;
		uint32_t lost_packet_cnt_telemetry_down;
		uint32_t lost_packet_cnt_msp_up;         // not used at the moment
		uint32_t lost_packet_cnt_msp_down;       // not used at the moment
		uint32_t lost_packet_cnt_rc;
		int8_t current_signal_joystick_uplink;   // signal strength in dbm at air pi (telemetry upstream and rc link)
		int8_t current_signal_telemetry_uplink;
		int8_t joystick_connected;               // 0 = no joystick connected, 1 = joystick connected
		float HomeLat;
		float HomeLon;
		uint8_t cpuload_gnd;
		uint8_t temp_gnd;
		uint8_t cpuload_air;
		uint8_t temp_air;
		uint32_t wifi_adapter_cnt; // 14*4+7*1+2*4=71
		wifi_adapter_rx_status_forward_t adapter[6]; //42
	} __attribute__((packed)) rx_status_t;

	rx_status_t telmetryData;
};





#endif /* MAVLINKHANDLER_H_ */