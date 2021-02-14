#!/bin/bash
#Lets find out if we are on Drone or test-bech: GPIO25 (26) = 1 -> Test bench GPIO25 (26) = 0 -> Drone Set the pin for input
VERSION=0.10
MOUNTPATH=/mnt/sd
LOGFILE=run.log
IP=81.161.156.81


gpio mode 25 in
# Set the variable through command substitution
mode=$(gpio read 25)

mkdir -p /mnt/sd
mount /dev/mmcblk0p3 /mnt/sd

if [ $mode -gt 0 ]
then
	# Test-Bech!
	echo "$(date)   -   PowerUp in test bech"
else
	# Drone!

	#Rotate log
	LOGNUMBER=0
	if [! -f $MOUNTPATH/lastlog.txt]
	then
		echo "0" > $MOUNTPATH/lastlog.txt
	else
		LOGNUMBER=`cat $MOUNTPATH/lastlog.txt`
	fi

	let LOGNUMBER++
	echo "$LOGNUMBER" > $MOUNTPATH/lastlog.txt
	FOLDER=$(printf "%06d" $LOGNUMBER)

	DATAFOLDER=$MOUNTPATH/$FOLDER
	mkdir $DATAFOLDER

	LOGFOLDER=$DATAFOLDER/log
	LOG=$LOGFOLDER/$LOGFILE
	mkdir $LOGFOLDER

        # Drone!
        echo "$(date)   -   Power Up as drone" >> $LOG
	echo "$(date)   -   Starting OpenHD-LTE $VERSION" >> $LOG
	echo "$(date)   -   Setting up LTE modem:" >> $LOG
	/boot/air/4gConnect.sh >> $LOG 2>&1
	echo "$(date)   -   LTE modem setup complete" >> $LOG

	mkdir -p /var/run/openhd
	mkfifo /var/run/openhd/videofifo

	/boot/air/startCamera.sh $LOG &
	/boot/air/startTXraw.sh $LOG $DATAFOLDER /boot/air $IP &

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

fi
