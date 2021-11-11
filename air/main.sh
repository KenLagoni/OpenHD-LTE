#!/bin/bash
#Lets find out if we are on Drone or test-bech: GPIO25 (26) = 1 -> Test bench GPIO25 (26) = 0 -> Drone Set the pin for input

source $1/air-settings.sh


#LOGPATH="/home/pi/OpenHD-LTE"
#LOGFILE="test.log"


mkdir -p $MOUNTPATH
mount /dev/mmcblk0p3 $MOUNTPATH

mkdir -p /var/run/openhd
mkfifo /var/run/openhd/videofifo

#mkdir -p $LOGPATH


LOGNUMBER=0
if [ -f $MOUNTPATH/lastlog.txt ]
then
	LOGNUMBER=`cat $MOUNTPATH/lastlog.txt`
else
	echo "0" > $MOUNTPATH/lastlog.txt
fi

let LOGNUMBER++
echo "$LOGNUMBER" > $MOUNTPATH/lastlog.txt
FOLDER=$(printf "%06d" $LOGNUMBER)

DATAFOLDER=$MOUNTPATH/$FOLDER
mkdir $DATAFOLDER

LOGFOLDER=$DATAFOLDER/log
LOG=$LOGFOLDER/$LOGFILE
mkdir $LOGFOLDER




#gpio mode 25 in
# Set the variable through command substitution
#mode=$(gpio read 25)


#if [ $mode -gt 0 ]
#then
	# Test-Bech!
#	echo "$(date)   -   PowerUp in test bech"
	
#	start_camera
	
#else
	# Drone!
		#Rotate log


    # Drone!
   # echo "$(date)   -   Power Up as drone" >> $LOG
#	echo "$(date)   -   Starting OpenHD-LTE $VERSION" >> $LOG
#	echo "$(date)   -   Setting up LTE modem:" >> $LOG
	
	echo "Setting up LTE modem..." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	ip link set wwan0 down 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	echo 'Y' | sudo tee /sys/class/net/wwan0/qmi/raw_ip 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	sudo ip link set wwan0 up 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	sudo qmicli -d /dev/cdc-wdm0 --wda-get-data-format 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	sudo qmicli -p -d /dev/cdc-wdm0 --device-open-net='net-raw-ip|net-no-qos-header' --wds-start-network="apn='$APN',ip-type=4" --client-no-release-cid 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	sudo udhcpc -q -f -i wwan0 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG	
#	$AIRSTARTSCRIPT/4gConnect.sh $AIRSTARTSCRIPT 2>&1 >> $LOG 
	echo "LTE modem setup complete." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	# echo "$(date)   -   LTE modem setup complete" >> $LOG

	sleep 2
	
	echo "Starting OpenHD-LTE Air side version $VERSION" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	echo "Updating Firmware...." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	$AIRSTARTSCRIPT/firmwareUpdate.sh $AIRSTARTSCRIPT 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG

	sleep 1

	## Test Camera:
	echo "Testing camera:" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
	raspistill -o test.jpg 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG

	#$AIRSTARTSCRIPT/startCamera.sh $AIRSTARTSCRIPT $LOG &
	#sleep 1
	$AIRSTARTSCRIPT/startTXraw.sh $AIRSTARTSCRIPT $LOG $DATAFOLDER &
	
	cp /var/log/messages $LOGFOLDER/messages
	dd if=/dev/null of=/var/log/messages
	tail -f -n 0 /var/log/messages >> $LOGFOLDER/messages &

	cp /var/log/syslog $LOGFOLDER/syslog
	dd if=/dev/null of=/var/log/syslog
	tail -f -n 0 /var/log/syslog >> $LOGFOLDER/syslog &

	cp /var/log/daemon.log $LOGFOLDER/daemon.log
	dd if=/dev/null of=/var/log/daemon.log
    tail -f -n 0 /var/log/daemon.log >> $LOGFOLDER/daemon.log &

	cp /var/log/kern.log $LOGFOLDER/kern.log
	dd if=/dev/null of=/var/log/kern.log
    tail -f -n 0 /var/log/kern.log >> $LOGFOLDER/kern.log &

    cp /var/log/auth.log $LOGFOLDER/auth.log
    dd if=/dev/null of=/var/log/auth.log
    tail -f -n 0 /var/log/auth.log >> $LOGFOLDER/auth.log &

    cp /var/log/user.log $LOGFOLDER/user.log
    dd if=/dev/null of=/var/log/user.log
    tail -f -n 0 /var/log/user.log >> $LOGFOLDER/user.log &

    cp /var/log/debug $LOGFOLDER/debug
    dd if=/dev/null of=/var/log/debug
    tail -f -n 0 /var/log/debug >> $LOGFOLDER/debug &

	sync
#fi
