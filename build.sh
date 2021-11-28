#!/bin/bash

# setup dir

#mkdir -p outputFiles
#chmod 755 outputFiles -R


#build tx_raw for air pi
g++ -Isrc/ -g -o air/tx_raw src/tx_raw.cpp src/connection.cpp src/h264.cpp src/h264TXFraming.cpp src/h264UDPPackage.cpp src/h264Recorder.cpp src/mavlinkHandler.cpp src/telemetryWrapper.cpp

#build rx_raw for ground pi OpenHD (ground-OpenHD)
g++ -Isrc/ -g -o ground-OpenHD/rx_raw src/rx_raw.cpp src/connection.cpp src/h264.cpp src/h264RXFraming.cpp src/h264UDPPackage.cpp src/h264Recorder.cpp src/mavlinkHandler.cpp src/telemetryWrapper.cpp

#build videoRecord for ground pi (ground-VideoRecord)
g++ -Isrc/ -g -o ground-VideoRecord/videoRecord src/groundRecording.cpp src/connection.cpp src/h264Recorder.cpp src/mavlinkHandler.cpp


