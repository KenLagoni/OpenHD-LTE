#!/bin/bash

source $1/air-settings.sh

LOG=$2
VIDEOSAVEPATH=$3

for (( ; ; ))
do
	echo "Starting TX_RAW to IP=$IP VIDEO_PORT=$VIDEOPORT MAVLINK_PORT=$MAVLINKPORT TELEMETRY_PORT=$TELEMETRIPORT with max recording size of "$FILEMAX"MB." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	raspivid -w $WIDTH -h $HEIGTH -fps $FPS -b $BITRATE  -t 0 -o - | $AIRSTARTSCRIPT/tx_raw -i $GROUND_IP -v $VIDEOPORT -s /dev/serial0 -p $MAVLINKPORT -t $TELEMETRIPORT -o $VIDEOSAVEPATH/record -z $FILEMAX 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG 
	#cat /var/run/openhd/videofifo | $AIRSTARTSCRIPT/tx_raw -i $GROUND_IP -v $VIDEOPORT -s /dev/serial0 -p $MAVLINKPORT -t $TELEMETRIPORT -o $VIDEOSAVEPATH/record -z $FILEMAX 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG 
	echo "TX_RAW - Crashed! - Restarting in 10 seconds..." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
    sleep 10
done

