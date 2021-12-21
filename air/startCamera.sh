#!/bin/bash


### Load air-settings
source $1/air-settings.sh

LOG=$2


## Test Camera:
echo "Testing camera:" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
raspistill -o test.jpg 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
#libcamera-still -o test.jpg 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG

for (( ; ; ))
do
	echo "Starting Camera pipe to FIFO using WIDTH=$WIDTH HEIGHT=$HEIGTH FPS=$FPS BITERATE=$BITRATE." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
    raspivid -w $WIDTH -h $HEIGTH -fps $FPS -b $BITRATE  -t 0 -o - > /var/run/openhd/videofifo 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	#libcamera-vid --width $WIDTH --height $HEIGTH --framerate $FPS -b $BITRATE -t 0 -o - > /var/run/openhd/videofifo 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	echo "Camera pipe crashed Restarting in 10 seconds..." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
    sleep 10
done

