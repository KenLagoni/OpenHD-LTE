#!/bin/bash

### Install script for OpenHD-LTE ground-VideoRecord
## https://forums.raspberrypi.com/viewtopic.php?f=38&t=281972&sid=187fc6a42380e1b1d9c88a4eee2579e5

INSTALLPATH="/home/pi/OpenHD-LTE"
GROUNDSTARTSCRIPT="$INSTALLPATH/ground-VideoRecord"

echo "OpenHD-LTE Ground side installation"
echo "Install Path             : $INSTALLPATH"
echo "Ground Start Script Path : $GROUNDSTARTSCRIPT"

sudo apt-get update

#install ts (for timestamp) in moreutils packages
sudo apt install moreutils -y

### Add cma=256M to the end of /boot/cmdline.txt, but first ensure to remove all spaces after last charactor, to ensure we don't insert two spaces if user added spaces after last char.
sudo bash -c "sed 's/ *$//' /boot/cmdline.txt > tmp" && sudo mv tmp /boot/cmdline.txt && sudo truncate -s-1 /boot/cmdline.txt && sudo bash -c 'echo -n " cma=256M" >> /boot/cmdline.txt'

### Add "dtoverlay=tc358743" to the end of /boot/config.txt 
sudo bash -c 'echo "dtoverlay=tc358743" >> /boot/config.txt'

### Download RTSP-simple-server:
wget https://github.com/aler9/rtsp-simple-server/releases/download/v0.17.6/rtsp-simple-server_v0.17.6_linux_armv7.tar.gz -P $INSTALLPATH
tar -xvzf $INSTALLPATH/rtsp-simple-server_v0.17.6_linux_armv7.tar.gz -C $INSTALLPATH/
rm $INSTALLPATH/rtsp-simple-server_v0.17.6_linux_armv7.tar.gz

### Install gstreamer 1.14
sudo apt-get install gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-rtsp -y


### Install SAMBA server, This ask for WINS netbios addd to smb.conf -> NO
sudo apt-get install samba samba-common-bin -y 

### Create SMB config:
sudo mv /etc/samba/smb.conf /etc/samba/smb.conf_backup
sudo bash -c 'echo "[OpenHD-LTE]" > /etc/samba/smb.conf'
sudo bash -c 'echo "comment = Shared folder" >> /etc/samba/smb.conf'
sudo bash -c 'echo "path = '$INSTALLPATH'" >> /etc/samba/smb.conf'
sudo bash -c 'echo "browseable = yes" >> /etc/samba/smb.conf'
sudo bash -c 'echo "writeable = yes" >> /etc/samba/smb.conf'
sudo bash -c 'echo "only guest = no" >> /etc/samba/smb.conf'
sudo bash -c 'echo "create mask = 0777" >> /etc/samba/smb.conf'
sudo bash -c 'echo "directory mask = 0777" >> /etc/samba/smb.conf'
sudo bash -c 'echo "public = yes" >> /etc/samba/smb.conf'
sudo bash -c 'echo "guest ok = yes" >> /etc/samba/smb.conf'

### Create Data dir:
sudo mkdir data
sudo chmod 0777 data
sudo chown pi:pi data

### Add pi user as SMB user, this will ask for SMB password
sudo smbpasswd -a pi  
sudo service smbd restart && sudo service nmbd restart

### Create rc.local script to start OpenHD-LTE on boot.
sudo mv /etc/rc.local /etc/rc.local_backup
sudo bash -c 'echo "#!/bin/sh -e" > /etc/rc.local'
sudo bash -c 'echo "'$GROUNDSTARTSCRIPT'/main.sh '$INSTALLPATH'" >> /etc/rc.local'
sudo bash -c 'echo "exit 0" >> /etc/rc.local'
sudo chmod 755 /etc/rc.local

### Build OpenHD-LTE source
./build.sh


echo ""
echo "Installation complete - Please reboot"
