#!/bin/bash

source $1/air-settings.sh

### Download Firmware and extract firmware
scp -P $GROUND_SSH_PORT $USER@$GROUND_IP:$INSTALLPATH/$FIRMWARE $INSTALLPATH/$FIRMWARE
chown $USER:$USER $INSTALLPATH/$FIRMWARE
tar -xzvf $INSTALLPATH/$FIRMWARE -C $INSTALLPATH
chown $USER:$USER $INSTALLPATH -R