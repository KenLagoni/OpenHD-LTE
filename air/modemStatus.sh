#!/bin/bash

function remove_quotes {
 echo $1 | sed -e "s/\ .*//" |  sed -e "s/'//" | sed -e "s/'//"
}

LOGFOLDER=$1

echo "Opperation Mode,Cell ID,MCC,MNC,Tracking Area Code [TAC],Radio Interface,Active Band Class,Active Channel,RSSI [dBm],RSRQ [dB],SNR [dB],Selected Network" >> $LOGFOLDER/modemStatus.log

for (( ; ; ))
do
	MODE=$(sudo qmicli -d /dev/cdc-wdm0 --dms-get-operating-mode | sed -n 's/.*Mode: //p' | sed -e "s/'//" | sed -e "s/'//")

	SYSTEM_INFO=$(sudo qmicli -d /dev/cdc-wdm0 --nas-get-system-info)
	#CELL_ID=$(echo $SYSTEM_INFO | sed -n 's/.*Cell ID: //p' | sed -e "s/\ .*//" |  sed -e "s/'//" | sed -e "s/'//")
	CELL_ID=$(remove_quotes $(echo $SYSTEM_INFO | sed -n 's/.*Cell ID: //p'))
	MCC=$(remove_quotes $(echo $SYSTEM_INFO | sed -n 's/.*MCC: //p'))
	MNC=$(remove_quotes $(echo $SYSTEM_INFO | sed -n 's/.*MNC: //p'))
	TAC=$(remove_quotes $(echo $SYSTEM_INFO | sed -n 's/.*Tracking Area Code: //p'))

	RF_BAND_INFO=$(sudo qmicli -d /dev/cdc-wdm0 --nas-get-rf-band-info)
	RADIO_INTERFACE=$(remove_quotes $(echo $RF_BAND_INFO | sed -n 's/.*Radio Interface: //p'))
	BAND=$(remove_quotes $(echo $RF_BAND_INFO | sed -n 's/.*Active Band Class: //p'))
	CHANNEL=$(remove_quotes $(echo $RF_BAND_INFO | sed -n 's/.*Active Channel: //p'))

	SIGNAL_INFO=$(sudo qmicli -d /dev/cdc-wdm0 --nas-get-signal-info)
	RSSI=$(remove_quotes $(echo $SIGNAL_INFO | sed -n 's/.*RSSI: //p'))
	RSRQ=$(remove_quotes $(echo $SIGNAL_INFO | sed -n 's/.*RSRQ: //p'))
	RSRP=$(remove_quotes $(echo $SIGNAL_INFO | sed -n 's/.*RSRP: //p'))
	SNR=$(remove_quotes $(echo $SIGNAL_INFO | sed -n 's/.*SNR: //p'))


	SERVICE_SYSTEM=$(sudo qmicli -d /dev/cdc-wdm0 --nas-get-serving-system)
	SELECTED_NETWORK=$(remove_quotes $(echo $SERVICE_SYSTEM | sed -n 's/.*Selected network: //p'))

	## Make file for tx_raw to read:
	echo $RSSI > /var/run/openhd/signalStatus

	## Log all output:
	echo $MODE","$CELL_ID","$MCC","$MNC","$TAC","$RADIO_INTERFACE","$BAND","$CHANNEL","$RSSI","$RSRQ","$RSRP","$SNR","$SELECTED_NETWORK >> $LOGFOLDER/modemStatus.log
	
	sleep 5
done



#echo "Opperation Mode="$MODE
#echo "System Info="$SYSTEM_INFO
#echo "System Info:"
#echo "Cell ID="$CELL_ID
#echo "MCC="$MCC
#echo "MNC="$MNC
#echo "Tracking Area Code="$TAC
#echo " "
#echo "RF Band Info:"
#echo "Radio Interface="$RADIO_INTERFACE
#echo "Active Band Class="$BAND
#echo "Active Channel="$CHANNEL
#echo " "
#echo "Signal Info:"
#echo "RSSI [dBm]="$RSSI
#echo "RSRQ  [dB]="$RSRQ
#echo "RSRP [dBm]="$RSRP
#echo "SNR   [dB]="$SNR
#echo ""
#echo "Service System:"
#echo "Selected Network="$SELECTED_NETWORK




