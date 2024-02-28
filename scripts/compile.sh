#!/bin/bash

FILE_DIR=`cd $(dirname $0); pwd`
. $FILE_DIR/utils.sh

create_dir()
{
  if [ ! \( -e $1 \) ];
  then 
    echo_r "mkdir -p $1"
  fi
}

set -e

if [ $# -ne 2 ];
then 
	echo_e "Usage: compile.sh <P4_PROGRAM_PATH> <BUILD_DIR_PATH>"
	exit 1
fi

PROGRAM_PATH=$(realpath -m $1)
BUILD_PATH=$(realpath -m $2)
BASE_NAME=$(basename $1)
TAIL=${BASE_NAME#*.}
NAME=${BASE_NAME%%.*}

if [ ! \( -e $PROGRAM_PATH \) ];
then
  echo_e "File $PROGRAM_PATH does not exist."
  exit 1
fi

if [ $TAIL != "p4" ];
then 
  echo_e "Unsupported file type."
  exit 1
fi

create_dir $BUILD_PATH
echo_r "cd $BUILD_PATH"
echo_r "cmake $SDE/p4studio/ \
  -DCMAKE_INSTALL_PREFIX=$SDE_INSTALL \
  -DCMAKE_MODULE_PATH=$SDE/cmake \
  -DP4_NAME=$NAME \
  -DP4_PATH=$PROGRAM_PATH \
"
BFA_PATH=$BUILD_PATH/$NAME/tofino/pipe/$NAME.bfa
if [ -e $BFA_PATH ];
then
  echo_r "make clean"
fi
start_time=`date +%s`
echo_r "make && make install"
end_time=`date +%s`
dt=$[ end_time - start_time ]
echo_i "Take up `cat $BFA_PATH | grep -c -E "stage.+ingress"` ingress stages"
echo_i "Take up `cat $BFA_PATH | grep -c -E "stage.+egress"` egress stages"
echo_i "Compilation time: ${dt} s" 



