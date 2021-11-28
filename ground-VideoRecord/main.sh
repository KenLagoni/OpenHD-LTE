#!/bin/bash

INSTALLPATH=$1

### RTSP upload settings:
IP="127.0.0.1"
PORT="8554"
STREAM="mystream"
BITERATE="5000000"
RECORDPATH="$INSTALLPATH/data"
LOGPATH="$INSTALLPATH/data/logs"
LOGFILE="$LOGPATH/videoRecord.log"

# Create log dir.
mkdir -p $LOGPATH

#Start the rtsp server if not running:
RTSP_PID=$(pgrep -f $INSTALLPATH/rtsp-simple-server)

echo "Checking rtsp-simple-server status..." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
if [ -z "$RTSP_PID" ]
then
      echo "rtsp-simple-server not running, thus starting it now." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
      $INSTALLPATH/rtsp-simple-server >> $LOGFILE 2>&1 &
else
      echo "rtsp-simple-server is allready running with PID: $RTSP_PID" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
fi




#while true; do
	#Read the input and setup
#	echo "" >> $INSTALLPATH/data/log/videoRecord-main.log 
#	echo "" >> $INSTALLPATH/data/log/videoRecord-main.log 
#	echo "" >> $INSTALLPATH/data/log/videoRecord-main.log 
#	echo "" >> $INSTALLPATH/data/log/videoRecord-main.log 
	echo "Starting Video record... Upload to: rtsp://"$IP":"$PORT"/"$STREAM" at biterate:"$BITERATE"" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE 
	echo "Get input timings:" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
	v4l2-ctl --query-dv-timings | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE 
	echo "Set input timing" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
	v4l2-ctl --set-dv-bt-timings query | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
	echo "Set input format to UYVY..." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
	v4l2-ctl -v pixelformat=UYVY | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
	echo "Show settings:" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
	v4l2-ctl -V | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
	echo "Now starting Gstreamer pipe to localhost" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE
	
	# Encode the video from CSI module (Video+OSD outputed on HDMI from RPI OpenHD) and split the output to two. 1. upload it to local RTSP server and 2. pipe video to record program.
    #gst-launch-1.0 v4l2src device=/dev/video0 ! "video/x-raw,framerate=30/1,format=UYVY" ! v4l2h264enc extra-controls="controls,h264_profile=4,h264_level=13,video_bitrate=$BITERATE;" ! video/x-h264,profile=high ! tee name=t ! h264parse ! queue ! rtspclientsink location=rtsp://$IP:$PORT/$STREAM t. ! h264parse ! fdsink | ./videoRecord -p 6000 -f record > /dev/null
#	gst-launch-1.0 v4l2src 2> >( ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log) device=/dev/video0 ! "video/x-raw,framerate=30/1,format=UYVY" ! v4l2h264enc extra-controls="controls,h264_profile=4,h264_level=13,video_bitrate=$BITERATE;" ! video/x-h264,profile=high ! tee name=t ! h264parse ! queue ! rtspclientsink location=rtsp://$IP:$PORT/$STREAM protocols=tcp t. ! h264parse ! fdsink | $INSTALLPATH/ground-VideoRecord/videoRecord -p 6000 -f record 1> /dev/null 2> >( ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log)
	gst-launch-1.0 v4l2src 2> >( ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE) device=/dev/video0 ! "video/x-raw,framerate=30/1,format=UYVY" ! v4l2h264enc extra-controls="controls,h264_profile=4,h264_level=13,video_bitrate=$BITERATE;" ! video/x-h264,profile=high ! tee name=t ! h264parse ! rtspclientsink location=rtsp://$IP:$PORT/$STREAM protocols=tcp t. ! h264parse ! fdsink | $INSTALLPATH/ground-VideoRecord/videoRecord -p  6000 -f $RECORDPATH 1> /dev/null 2> >( ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE)

	
	#Also show stream on RPI HDMI for local display, not working yet :-(
	#gst-launch-1.0 v4l2src device=/dev/video0 ! "video/x-raw,framerate=30/1,format=UYVY" ! v4l2h264enc extra-controls="controls,h264_profile=4,h264_level=13,video_bitrate=$BITERATE;" ! video/x-h264,profile=high ! tee name=t ! h264parse ! queue ! rtspclientsink location=rtsp://$IP:$PORT/$STREAM t. ! tee name=t ! h264parse ! fdsink | $INSTALLPATH/ground-VideoRecord/videoRecord -p 6000 -f record > /dev/null | ts '[%Y-%m-%d %H:%M:%S]' >> $INSTALLPATH/data/log/videoRecord-main.log t. ! h264parse ! avdec_h264 ! xvimagesink
	
	echo "gst-launch-1.0 v4l2src, restarting and sleeping for 5s ..." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOGFILE

#	sleep 5
#done
