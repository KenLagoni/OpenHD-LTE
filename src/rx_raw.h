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
#include "connection.h"


#include <sys/mman.h>

//Video record to file
#include <fstream>

//FIFO
#include "RingBuf.h"

#define RX_BUFFER_SIZE 1024
#define LOG_INTERVAL_SEC 1 // log every minute

int max(int x, int y)
{
	if (x > y)
	return x;
	else
	return y;
}

typedef struct {
	float tx;
	float rx;
	float dropped;
} rx_dataRates_t;

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
} __attribute__((packed)) rx_status_t;


#endif /* RX_RAW_H_ */
