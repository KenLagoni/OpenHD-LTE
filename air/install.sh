#!/bin/bash

# install wirinpi for GPIO read.
apt-get install wiringpi -y

#install driver/libarry for EC25 LTE modem.
apt install libqmi-utils udhcpc -y

# override rc.local
cp rc.local /etc/rc.local

# override mount
cp fstab /etc/fstab

#make ssh file for enable ssh.
touch /boot/ssh

# TODO
echo "TODO:"
echo "Ensure log/record fat32 (type=c) partion /dev/mmcblk0p3 is available on SD card (fdisk+mkfs.msdos)"
echo "Enable camera with raspi-config"
echo "Enable serial hardware but without terminal/console  raspi-config"

