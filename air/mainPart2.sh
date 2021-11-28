#!/bin/bash
#Lets find out if we are on Drone or test-bech: GPIO25 (26) = 1 -> Test bench GPIO25 (26) = 0 -> Drone Set the pin for input

source $1/air-settings.sh

mkdir -p /var/run/openhd
mkfifo /var/run/openhd/videofifo

mkdir -p $MOUNTPATH/log
LOGNUMBER=0

if [ -f $MOUNTPATH/log/lastlog.txt ]
then
	LOGNUMBER=`cat $MOUNTPATH/log/lastlog.txt`
else
	echo "0" > $MOUNTPATH/log/lastlog.txt
fi

let LOGNUMBER++
echo "$LOGNUMBER" > $MOUNTPATH/log/lastlog.txt
FOLDER=$(printf "%06d" $LOGNUMBER)

DATAFOLDER=$MOUNTPATH/log/$FOLDER

# MOUNTPATH=/mnt/sd
# FOLDER=NNNNNN
# DATAFOLDER=/mnt/sd/log/NNNNNN
# LOGFOLDER=/mnt/sd/log/NNNNNN
# LOG=/mnt/sd/log/NNNNNN/run.log
mkdir -p $DATAFOLDER
LOGFOLDER=$DATAFOLDER
LOG=$LOGFOLDER/$LOGFILE

	
echo "Setting up LTE modem using APN="$APN | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
ip link set wwan0 down 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
echo 'Y' | sudo tee /sys/class/net/wwan0/qmi/raw_ip 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
sudo ip link set wwan0 up 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
sudo qmicli -d /dev/cdc-wdm0 --wda-get-data-format 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
sleep 2
sudo qmicli -p -d /dev/cdc-wdm0 --device-open-net='net-raw-ip|net-no-qos-header' --wds-start-network="apn='$APN',ip-type=4" --client-no-release-cid 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
sudo udhcpc -q -f -i wwan0 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG	
#	$AIRSTARTSCRIPT/4gConnect.sh $AIRSTARTSCRIPT 2>&1 >> $LOG 
echo "LTE modem setup complete." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
# echo "$(date)   -   LTE modem setup complete" >> $LOG

sleep 2

# Pack old folders and upload it to ground before starting:
TARFILE=`date '+%Y-%m-%d_%H_%M_%S-Airlog.tar.gz'`
echo "Creating tar with old logs ($TARFILE)" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
tar -zcvf $INSTALLPATH/$TARFILE $MOUNTPATH/log 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
# Upload old all logs to Ground for debug:
echo "Uploading old logs ($INSTALLPATH/$TARFILE) from "$INSTALLPATH/$TARFILE" to ground ip=$GROUND_IP on SSH port=$GROUND_SSH_PORT as user=$USER to $INSTALLPATH/data/logs/air" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
scp -P $GROUND_SSH_PORT $INSTALLPATH/$TARFILE $USER@$GROUND_IP:$INSTALLPATH/data/logs/air 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG

sleep 2

echo "Starting OpenHD-LTE Air side version $VERSION" | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
echo "Updating Firmware...." | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG
$AIRSTARTSCRIPT/firmwareUpdate.sh $AIRSTARTSCRIPT 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' >> $LOG

sleep 1

$AIRSTARTSCRIPT/startCamera.sh $AIRSTARTSCRIPT $LOG &

sleep 1

# Start TX_raw and camera:
$AIRSTARTSCRIPT/startTXraw.sh $AIRSTARTSCRIPT $LOG $MOUNTPATH &

# Start LTE modem status script:
$AIRSTARTSCRIPT/modemStatus.sh $LOGFOLDER &

# Ensure all relevant linux logs are saved on SD.
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
