#!/bin/bash
LOG=$1
FPS=30
WIDTH=1920
HEIGTH=1080
BITRATE=5000000

for (( ; ; ))
do
	echo "$(date)   -   Starting Camera pipe to FIFO" >> $LOG
      	raspivid -w $WIDTH -h $HEIGTH -fps $FPS -b $BITRATE  -t 0 -o - > /var/run/openhd/videofifo 2>> $LOG
      	echo "$(date)   -   Camera pipe crashed Restarting in 10 seconds..." >> $LOG
      	sleep 10
done

