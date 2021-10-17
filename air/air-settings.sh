#!/bin/bash
VERSION=0.20

### 4G settings
APN=""

### Connection parameters
GROUND_IP=""
GROUND_SSH_PORT=33
USER="pi"
FIRMWARE="airfirmware.tar.gz"
VIDEOPORT=7000
TELEMETRIPORT=7001
MAVLINKPORT=12000

## Air Camera settings:
#WIDTH=1440
WIDTH=1920
HEIGTH=1080
FPS=30
BITRATE=5000000

#Maximum allowed output file size for local recording on air pt. (on FAT32 2000 should be used)
FILEMAX=2000


### /home/pi/OpenHD-LTE
INSTALLPATH="/home/pi/OpenHD-LTE"
AIRSTARTSCRIPT="$INSTALLPATH/air"

## SD log path:
MOUNTPATH=/mnt/sd
LOGFILE=run.log
