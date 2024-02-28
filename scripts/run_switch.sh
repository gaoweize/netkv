#!/bin/bash

. $(dirname $0)/utils.sh

set -e 

if [ $# -ne 2 ] || [ $1 == "-h" ] || [ $1 == "--help" ];
then 
	echo_e "Usage: run_switch.sh <P4_PROGRAM_NAME> <BFRT_PRELOAD_FILE>"
	exit 1
fi


DIR=`cd $(dirname $0); pwd`
PROGRAM=$1

if [ $# -eq 2 ];
then
BFRT_PRELOAD_FILE=`cd $(dirname $2); pwd`/$(basename $2)
fi

echo_i "Find and kill previous process."

$DIR/kill_switch.sh $PROGRAM

sleep 0.1

if [ -n "`pgrep bf_switchd`" ];
then 
    echo_e "Switch is being used by another program."
    exit 1
fi

echo_i "Boot switch in the background."

echo_r "$SDE/run_switchd.sh -p $PROGRAM > /dev/null 2>&1 &"


echo_i "Boot controller... "

echo_r "$SDE/run_bfshell.sh -b $BFRT_PRELOAD_FILE > /dev/null 2>&1"

echo_i "Done."
