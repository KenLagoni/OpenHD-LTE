#!/bin/bash

# setup dir

#mkdir -p outputFiles
#chmod 755 outputFiles -R


#build tx_raw for air pi
g++ -Isrc/ -o air/tx_raw src/tx_raw.cpp src/connection.cpp

#build rx_raw for ground pi
g++ -Isrc/ -o ground/rx_raw src/rx_raw.cpp src/connection.cpp

