#!/bin/bash
LOG=$1
VIDEOSAVEPATH=$2
SCRIPTPATH=$3
IP=$4

for (( ; ; ))
do
	echo "$(date)   -   Starting TX_RAW" >> $LOG
	cat /var/run/openhd/videofifo | $SCRIPTPATH/tx_raw -i $IP -v 7000 -s /dev/serial0 -p 12000 -t 5200 -o $VIDEOSAVEPATH/record -z 2000 >> $LOG 2>&1
      	echo "$(date)   -   TX_RAW - Crashed! - Restarting in 10 seconds..." >> $LOG
      	sleep 10
done

