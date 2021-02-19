#!/bin/bash

# deploy videoRecord to PI #2 192.168.0.8
scp ground/videoRecord pi@192.168.0.8:/home/pi

#deploy rx_raw to PI #1 192.168.0.33
scp ground/rx_raw pi@192.168.0.33:/home/pi
