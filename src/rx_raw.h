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
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <chrono> // Crone time measure and sleep debug
#include <thread> // for sleep debug
#include <sys/mman.h>

#include "connection.h"
#include "telemetryWrapper.h"
#include "h264RXFraming.h"
#include "timer.h"
// For H264 recording
#include "h264Recorder.h"

// For Mavlink
#include "mavlinkHandler.h"


#define RX_BUFFER_SIZE 1400
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


#endif /* RX_RAW_H_ */
