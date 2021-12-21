#!/bin/bash

# After flash
#console=serial0,115200 console=tty1 root=PARTUUID=9730496b-02 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait quiet init=/usr/lib/raspi-config/init_resize.sh
#console=serial0,115200 console=tty1 root=PARTUUID=4850bb8e-02 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait


SCRIPTPATH=$1
source $SCRIPTPATH/air-settings.sh

echo "OpenHD-LTE Air side installation"
echo "Install Path             : $INSTALLPATH"
echo "Air Start Script Path    : $AIRSTARTSCRIPT"
echo "Script is executed from  : $SCRIPTPATH"

mkdir -p $INSTALLPATH

sudo apt-get update


#install ts (for timestamp) in moreutils packages
sudo apt install moreutils -y

#### install wirinpi for GPIO read. (not used any more)
#sudo apt-get install wiringpi -y 

####install driver/libarry for EC25 LTE modem.
sudo apt install libqmi-utils udhcpc -y

####Install sshpass
####sudo apt-get install sshpass -y

### Add Ground pi to known_hosts (so sshpass wont get thrown out due to askint this).
mkdir -p /home/$USER/.ssh
mkdir -p /root/.ssh
ssh-keyscan -p $GROUND_SSH_PORT $GROUND_IP >> /home/$USER/.ssh/known_hosts
ssh-keyscan -p $GROUND_SSH_PORT $GROUND_IP >> /root/.ssh/known_hosts


### Generate SSH key
ssh-keygen -t rsa -f /home/$USER/.ssh/id_rsa
cp /home/$USER/.ssh/id_rsa.pub /root/.ssh/id_rsa.pub
cp /home/$USER/.ssh/id_rsa /root/.ssh/id_rsa

# Ensure USER hass access to keys:
chown $USER:$USER /home/$USER/.ssh -R

### Install SSH key on ground pi so no password  is needed for SCP commands.
cat /home/$USER/.ssh/id_rsa.pub | ssh -p $GROUND_SSH_PORT $USER@$GROUND_IP "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"

### Create rc.local script to start OpenHD-LTE air on boot.
sudo mv /etc/rc.local /etc/rc.local_backup
sudo bash -c 'echo "#!/bin/sh -e" > /etc/rc.local'
#sudo bash -c 'echo "'$AIRSTARTSCRIPT'/firmwareUpdate.sh '$AIRSTARTSCRIPT'" >> /etc/rc.local'
sudo bash -c 'echo "'$AIRSTARTSCRIPT'/main.sh '$AIRSTARTSCRIPT'" >> /etc/rc.local'
sudo bash -c 'echo "exit 0" >> /etc/rc.local'
sudo chmod 755 /etc/rc.local

### Install SAMBA server, This ask for WINS netbios addd to smb.conf -> NO
sudo apt-get install samba samba-common-bin -y 
sudo mkdir -p $MOUNTPATH

### Create SMB config:
sudo mv /etc/samba/smb.conf /etc/samba/smb.conf_backup
sudo bash -c 'echo "[OpenHD-LTE-air]" > /etc/samba/smb.conf'
sudo bash -c 'echo "comment = Shared folder" >> /etc/samba/smb.conf'
sudo bash -c 'echo "path = '$MOUNTPATH'" >> /etc/samba/smb.conf'
sudo bash -c 'echo "browseable = yes" >> /etc/samba/smb.conf'
sudo bash -c 'echo "writeable = yes" >> /etc/samba/smb.conf'
sudo bash -c 'echo "only guest = no" >> /etc/samba/smb.conf'
sudo bash -c 'echo "create mask = 0777" >> /etc/samba/smb.conf'
sudo bash -c 'echo "directory mask = 0777" >> /etc/samba/smb.conf'
sudo bash -c 'echo "public = yes" >> /etc/samba/smb.conf'
sudo bash -c 'echo "guest ok = yes" >> /etc/samba/smb.conf'

### Setup SAMBA dir:
sudo chmod 0777 $MOUNTPATH

### Add pi user as SMB user, this will ask for SMB password
sudo smbpasswd -a pi  
sudo service smbd restart && sudo service nmbd restart

#### Firmware update
$SCRIPTPATH/firmwareUpdate.sh $SCRIPTPATH

## Remove the /boot/air folder
sudo rm /boot/air -R

### Enable UART and disdable login on uart
sudo raspi-config nonint do_serial 2

#sudo bash -c 'echo "enable_uart=1" >> /boot/config.txt'
### Ensure no login on UART 
#sed -i 's|console=serial0,115200 ||' /boot/cmdline.txt

### Enable Camera (set_camera 0) unlogical but it works.
#sudo raspi-config nonint set_camera 0 (incorrect command?)
sudo raspi-config nonint set camera 0


## uncomment for composite PAL
#sdtv_mode=2
#sdtv_aspect=1

# TODO
echo "Installation complete - Please setup your local router to forward:"
echo "SCP (Firmware update)  :  $GROUND_IP:$GROUND_SSH_PORT    Forward to 192.168.0.10:22    Protocol: TCP/UDP"
echo "Video stream           :  $GROUND_IP:$VIDEOPORT  Forward to 192.168.0.10:$VIDEOPORT  Protocol: TCP/UDP"
echo "Telemetri stream (OSD) :  $GROUND_IP:$TELEMETRIPORT  Forward to 192.168.0.10:$TELEMETRIPORT  Protocol: TCP/UDP"
echo "Mavlink stream (OSD)   :  $GROUND_IP:$MAVLINKPORT Forward to 192.168.0.10:$MAVLINKPORT Protocol: TCP/UDP"