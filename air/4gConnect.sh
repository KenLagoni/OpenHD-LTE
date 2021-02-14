#!/bin/bash

ip link set wwan0 down
echo 'Y' | sudo tee /sys/class/net/wwan0/qmi/raw_ip
sudo ip link set wwan0 up
sudo qmicli -d /dev/cdc-wdm0 --wda-get-data-format
sudo qmicli -p -d /dev/cdc-wdm0 --device-open-net='net-raw-ip|net-no-qos-header' --wds-start-network="apn='data.tre.dk',ip-type=4" --client-no-release-cid
sudo udhcpc -q -f -i wwan0
