#!/bin/bash

DIR=$(dirname $0)
PROJ="code/netkv"
. $DIR/utils.sh

echo_r "ssh -p 2332 gaoweize@162.105.218.62 -t \"ssh -p 22 gaoweize@10.0.0.1 'scp -r /home/gaoweize/${PROJ}/trace/bfrt/ root@10.0.0.100:/root/gwz/netkv/trace/' \""