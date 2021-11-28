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

// for UDP connection
#include "connection.h"

// For H264 framing
#include "h264TXFraming.h"

// For H264 recording
#include "h264Recorder.h"

// For Mavlink
#include "mavlinkHandler.h"

// Serial:
#define SERIAL_BAUDRATE 57600 //**** TODO also implement it in open_port

// For Telemtry (CPU load and temp):
#include "telemetryWrapper.h"

// Debug output
#include "timer.h"
#define LOG_INTERVAL_SEC 1 // log every minute

int max(int x, int y)
{
	if (x > y)
	return x;
	else
	return y;
}

//#define TELEMETRY_HEADER 0xFC
//#define TELEMETRY_HEADER_SIZE 5


#endif /* RX_RAW_H_ */
