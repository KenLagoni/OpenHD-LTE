#!/bin/bash
#Lets find out if we are on Drone or test-bech: GPIO25 (26) = 1 -> Test bench GPIO25 (26) = 0 -> Drone Set the pin for input

source $1/air-settings.sh

## keep it short since this part will only update after next reboot.


mkdir -p $MOUNTPATH
chmod 777 $MOUNTPATH 
mount -o rw,users,umask=000 /dev/mmcblk0p3 $MOUNTPATH
#mount /dev/mmcblk0p3 $MOUNTPATH 

echo -n "Checking if firmware is located on SD card..."
if [ -f $MOUNTPATH/$FIRMWARE ] ## Check for firmware on SD, this must be used if there.
then
    echo "Yes!"
	mv $MOUNTPATH/$FIRMWARE $INSTALLPATH/$FIRMWARE
    chown $USER:$USER $INSTALLPATH/$FIRMWARE
    tar -xzvf $INSTALLPATH/$FIRMWARE -C 
    chown $USER:$USER $INSTALLPATH -R 
fi

echo "No"
$AIRSTARTSCRIPT/mainPart2.sh $AIRSTARTSCRIPT