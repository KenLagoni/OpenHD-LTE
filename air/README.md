# Install air-side Raspberry pi
This setup will use a standard Raspberry Pi image, but instead of using all the space on the SD card for the Linux system, a third fat32 partition will be created to store video recordings and logs from the flight. This way the SD card can simply be extracted after flight and instarted into any CP (Windows/Linux) and the data can be read from there.
The partition table should look like this:
1. Boot (fat32) ~200MB
2. / (ext4) (Raspberry pi system, not visable from Windows) ~2.0GB
3. SDHC (fat32) ~ rest of the card (16GB card -> ~14GB). (14GB of space is ~14Hours of 5Mbit/s recording).

Select and install the latest Raspberry Pi OS lite (32-bit) on to an SD card using the "Raspberry Pi Imager" tool. (This documentation used v1.5)
- After the tool is complete it will unmount the SD card. Insert the SD back in the PC BEFORE! booting it on the raspberry.
- In the Boot partition, find and open the "cmdline.txtt" file, and remove the init=/usr/lib/init_reseize.sh part.
- In the Boot partition, make an empty file called "ssh" (no extention) to enable ssh login.
- Copy the air folder from this project to the root of Boot partition.
- Boot the SD card on a raspberry attached to your local network with DHCP and SSH to (login: pi pass: raspberry)
- Using fdisk "sudo fdisk /dev/mmcblk0" create a fat32 partition for the remaining of the SD card. Should be type "c" and partition should be nr. 3 thus creating -> /dev/mmcblk0p3.
- Add a filesystem to the new partition: "sudo mkfs.msdos /dev/mmcblk0p3"
- Run raspi-config: "sudo raspi-config" and enable camera and serial port (only serial port HW! not console/terminal on serial port).
- Run the install script: "sudo /boot/air/install.sh"