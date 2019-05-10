#!/bin/bash

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BUILDDIR="$SCRIPTDIR/../build/src/examples"
LOGDIR="$SCRIPTDIR/../log"

APPLICATION="traffic_test"
APP_PARAMS="-n 10 -p 30 -s 1000000 -o $LOGDIR"
APP_STDOUT="$LOGDIR/eventlog.log"

LOCAL_HOSTNAME=$(hostname)

STORAGE_SERVER_ADDR="129.217.211.43"
STORAGE_SERVER_USER="ng40"
STORAGE_SERVER_BASEDIR="c-mna-log"

source $SCRIPTDIR/config.sh

echo  $SCRIPTDIR
echo  $BUILDDIR
echo  $LOGDIR
echo  $APPLICATION
echo  $APP_PARAMS

echo  $LOCAL_HOSTNAME

# Start reversetunnel
echo "Starting reversetunnel:"
echo "screen -d -m $SCRIPTDIR/./ssh-reversetunnel-ng40.sh"
screen -d -m $SCRIPTDIR/./ssh-reversetunnel-ng40.sh

while true;
do
	$BUILDDIR/./$APPLICATION $APP_PARAMS >> $APP_STDOUT
	echo "running rsync"
	rsync -e ssh $LOGDIR/* $STORAGE_SERVER_USER@$STORAGE_SERVER_ADDR:$STORAGE_SERVER_BASEDIR/$LOCAL_HOSTNAME
	echo "pause for 30s"
	sleep 30
done;
