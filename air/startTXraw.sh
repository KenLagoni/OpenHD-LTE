#!/bin/bash

source $1/air-settings.sh

LOG=$2
VIDEOSAVEPATH=$3

## Test Camera:
echo "Testing camera:" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
raspistill -o test.jpg 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG


for (( ; ; ))
do
	echo "Starting TX_RAW to IP=$IP VIDEO_PORT=$VIDEOPORT MAVLINK_PORT=$MAVLINKPORT TELEMETRY_PORT=$TELEMETRIPORT" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	#raspivid -w $WIDTH -h $HEIGTH -fps $FPS -b $BITRATE  -t 0 -o - | $AIRSTARTSCRIPT/tx_raw -i $GROUND_IP -v $VIDEOPORT -s /dev/serial0 -p $MAVLINKPORT -t $TELEMETRIPORT -o $VIDEOSAVEPATH 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG 
	cat /var/run/openhd/videofifo | $AIRSTARTSCRIPT/tx_raw -i $GROUND_IP -v $VIDEOPORT -s /dev/serial0 -p $MAVLINKPORT -t $TELEMETRIPORT -o $VIDEOSAVEPATH 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG 
	echo "TX_RAW - Crashed! - Restarting in 10 seconds..." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
    sleep 10
done

