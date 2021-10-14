#!/bin/bash

# Playback video to local RTSP server.
File=$1

while true
do
	gst-launch-1.0 -e filesrc location=$File ! qtdemux ! rtspclientsink location=rtsp://127.0.0.1:8554/mystream
	sleep 20
done
