# Install air-side Raspberry pi

Before starting ensure the [ground pi](https://github.com/KenLagoni/OpenHD-LTE/tree/main/ground-OpenHD) is installed and ready on 192.186.0.8.

This setup will use a standard Raspberry Pi image, but instead of using all the space on the SD card for the Linux system, a third fat32 partition will be created to store video recordings and logs from the flight. This way the SD card can simply be extracted after flight and instarted into any CP (Windows/Linux) and the data can be read from there.
The partition table should look like this:
1. Boot (fat32) ~200MB
2. / (ext4) (Raspberry pi system, not visable from Windows) ~1.6GB
3. SDHC (fat32) ~ rest of the card (16GB card -> ~14GB). (14GB of space is ~14Hours of 5Mbit/s recording).

Install the [2020-02-13-raspbian-buster-lite.img](https://downloads.raspberrypi.org/raspbian_lite/images/raspbian_lite-2020-02-14/2020-02-13-raspbian-buster-lite.zip) image on to an SD card using the "Raspberry Pi Imager" tool (use custom). It must be this version since the later versions have raspivid command replaced with libcamera-vid and it doesn't work very well with the Arducam camera at the moment. 
- After the tool is complete it will unmount the SD card. Insert the SD back in the PC BEFORE! booting it on the raspberry.
- In the Boot partition, find and open the "cmdline.txt" file, and replace "init=/usr/lib/raspi-config/init_resize.sh" with "ip=192.168.0.11" (ensure ip is available on the network).
- In the Boot partition, make an empty file called "ssh" (no extention) to enable ssh login.
- Copy the air folder to to /boot/air/ 
- Boot the SD card on a Raspberry Pi attached to your local network with internet access - This could be the Raspberry later intended to be as ground pi OpenHD.
- SSH to 192.168.0.11 (login: pi pass: raspberry) 
- Run the init_resize script to create the correct partitions "sudo /boot/air/init_resize.sh", the script will automatticly reboot when done.
- Login again and edit the /boot/air/air-settings.sh with:
	1. GROUND_IP (WAN IP for Air pi to stream data to).
	2. APN (the apn settings for the 4G modem, this can be found from the SIM Card provider).
- Run the install script: "sudo /boot/air/install.sh /boot/air" This will ask for:
	1. Enter passphrase (empty for no passphrase): - Leave empty/blank!.
	2. login password for Ground pi (default is raspberry if not changed).
	3. Samba server and utilities (Modify smb.conf to use WINS settings from DHCP?) -> NO.
	4. Enter New SMB password: (same as login password).
	5. run "sudo raspi-config" and enable the camera after the script is done.
- The script will end with the list of ports needed to be forwarded in the router. the default is:
Installation complete - Please setup your local router to forward:\
SCP (Firmware update)     : \*.\*\.\*.\*:33    Forward to 192.168.0.10:22    Protocol: TCP/UDP\
Video stream             : \*.\*.\*.\*:12000  Forward to 192.168.0.10:12000 Protocol: TCP/UDP\
Telemetri stream (OSD)    : \*.\*.\*.\*:12001  Forward to 192.168.0.10:12001  Protocol: TCP/UDP\
Mavlink stream (OSD)      : \*.\*.\*.\*:12002 Forward to 192.168.0.10:12002 Protocol: TCP/UDP

The sd card can now be used in the air side pi.
