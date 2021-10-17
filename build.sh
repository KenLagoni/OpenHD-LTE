#!/bin/bash

# setup dir

#mkdir -p outputFiles
#chmod 755 outputFiles -R


#build tx_raw for air pi
g++ -Isrc/ -o air/tx_raw src/tx_raw.cpp src/connection.cpp

#build rx_raw for ground pi OpenHD (ground-OpenHD)
g++ -Isrc/ -o ground-OpenHD/rx_raw src/rx_raw.cpp src/connection.cpp

#build videoRecord for ground pi (ground-VideoRecord)
g++ -Isrc/ -o ground-VideoRecord/videoRecord src/videoRecord.cpp src/connection.cpp


