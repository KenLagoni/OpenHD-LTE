/*
	rx_raw.h

	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef RX_RAW_H_
#define RX_RAW_H_

#include <fcntl.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <chrono> // Crone time measure
#include <sys/mman.h>
#include "connection.h"

//Video record to file
#include <fstream>

// for Mavlink
#include "c_library_v1-master/common/mavlink.h"
#include "c_library_v1-master/ardupilotmega/mavlink.h"

//FIFO
#include "RingBuf.h"
#define FIFO_SIZE 256 // Mavlink messages
//#include "h264.h"
#include "h264TXFraming.h"

// Serial:
#define MAXLINE 1400

#define DEFAULT_MAX_VIDEO_FILE_SIZE 2000000000 // (FAT32 max 2Gb (2147483648)) if not set with -z option.
#define MAX_VIDEO_BUFFER_SIZE 1024*1024*2 // 2MB
#define VIDEO_BUFFER_WRITE_THRESHOLD 1024*1024*1 // 1MB
#define VIDEO_TX_BUFFER_SIZE 1024 // 1024 of the struct with 1024 in each, so 1024*1024=1MB

#define SERIAL_BAUDRATE 57600 //**** TODO also implement it in open_port
#define LOG_INTERVAL_SEC 1 // log every minute

#define VIDEO_RETRY_ATTEMPTS 3


int max(int x, int y)
{
	if (x > y)
	return x;
	else
	return y;
}

#define MAX_SERIAL_BUFFER_SIZE 1024 // fit inside one UDP, this could perhaps be 1024 or 1508, but if we transmitt everytime MSG30 (HUD) is received will will never get more than ~500bytes.
#define TELEMETRY_HEADER 0xFC
#define TELEMETRY_HEADER_SIZE 5

typedef struct {
	uint8_t cpuLoad;
	int8_t cpuTemp;
	int16_t rssidBm;
} telematryFrame_t;

typedef struct {
	uint16_t len;
	uint8_t data[1024];
} videoFrame_t;


typedef struct {
    uint32_t received_packet_cnt;
    int8_t current_signal_dbm;
    int8_t type; // 0 = Atheros, 1 = Ralink
    int8_t signal_good;
} __attribute__((packed)) wifi_adapter_rx_status_forward_t;


typedef struct {
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
    uint32_t wifi_adapter_cnt;
	wifi_adapter_rx_status_forward_t adapter[6];
} __attribute__((packed)) air_status_t;



#endif /* RX_RAW_H_ */
