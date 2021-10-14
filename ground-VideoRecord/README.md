# Install gorund-side Raspberry pi (ground-VideoRecord ip:192.168.0.8)
This setup will use a Raspberry Pi 4 hardware with the Raspberry pi OS Lite (32-bit).

Select and install the latest Raspberry Pi OS lite (32-bit) on to an SD card using the "Raspberry Pi Imager" tool. (This documentation used v1.6.2).
- After the tool is complete it will unmount the SD card. 
- Insert the SD card to the Raspberry Pi and let it boot and resize the SD card.
- When the Raspberry has booted, power it off and remove the SD card and insert it into a PC.
- In the Boot partition, find and open the "cmdline.txt" file, and add ip=192.168.0.8 at the end of the line.
- In the Boot partition, make an empty file called "ssh" (no extention) to enable ssh login.
- Connect the Pi to a network with internet access and power it on and login via ssh (IP:192.168.0.8 User: pi Password: raspberry).
- Consider changing the password with "passwd", but do it now before generating ssh keys.
- Install Git: "sudo apt-get install git -y"
- Clone this project to home folder: "git clone https://github.com/KenLagoni/OpenHD-LTE.git ~/OpenHD-LTE"
- Run the install script: "sudo ~/OpenHD-LTE/ground-VideoRecord/install.sh"
- 

