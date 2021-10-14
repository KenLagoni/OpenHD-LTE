#!/bin/bash

INSTALLPATH=$1

# Create log dir.
mkdir -p $INSTALLPATH/data/log

#Start the rtsp server
$INSTALLPATH/rtsp-simple-server 1>>$INSTALLPATH/data/log/rtsp_run.log 2>>$INSTALLPATH/data/log/rtsp_error.log &

### RTSP upload settings:
IP="127.0.0.1"
PORT="8554"
STREAM="mystream"
BITERATE="5000000"


#while true; do
	#Read the input and setup
	echo "" >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "" >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "" >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "" >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "Starting Video record... Upload to: rtsp://"$IP":"$PORT"/"$STREAM" at biterate:"$BITERATE"" | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "Get input timings:" | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log 
	v4l2-ctl --query-dv-timings | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "Set input timing" | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log
	v4l2-ctl --set-dv-bt-timings query | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "Set input format to UYVY..." | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log
	v4l2-ctl -v pixelformat=UYVY | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "Show settings:" | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log
	v4l2-ctl -V | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "Now starting Gstreamer pipe to localhost" | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log
	
	# Encode the video from CSI module (Video+OSD outputed on HDMI from RPI OpenHD) and split the output to two. 1. upload it to local RTSP server and 2. pipe video to record program.
    #gst-launch-1.0 v4l2src device=/dev/video0 ! "video/x-raw,framerate=30/1,format=UYVY" ! v4l2h264enc extra-controls="controls,h264_profile=4,h264_level=13,video_bitrate=$BITERATE;" ! video/x-h264,profile=high ! tee name=t ! h264parse ! queue ! rtspclientsink location=rtsp://$IP:$PORT/$STREAM t. ! h264parse ! fdsink | ./videoRecord -p 6000 -f record > /dev/null
	gst-launch-1.0 v4l2src device=/dev/video0 ! "video/x-raw,framerate=30/1,format=UYVY" ! v4l2h264enc extra-controls="controls,h264_profile=4,h264_level=13,video_bitrate=$BITERATE;" ! video/x-h264,profile=high ! tee name=t ! h264parse ! queue ! rtspclientsink location=rtsp://$IP:$PORT/$STREAM t. ! h264parse ! fdsink | ./videoRecord -p 6000 -f record > /dev/null 1> | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log
	echo "gst-launch-1.0 v4l2src, restarting and sleeping for 5s ..." | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log

#	sleep 5
#done
