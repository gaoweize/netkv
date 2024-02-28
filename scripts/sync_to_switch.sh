#!/bin/bash

DIR=$(dirname $0)
PROJ="netkv"
. $DIR/utils.sh


# echo_r "rsync -avzhcqPe 'ssh -p 2332 gaoweize@162.105.218.62 -t ssh -p 22' ${DIR}/../ root@10.0.0.100:/root/gwz/${PROJ}"
echo_r "rsync -avzhcqPe 'ssh -p 2332 gaoweize@162.105.218.62 -t ssh -p 22' ${DIR}/../ root@10.0.0.100:/root/gwz/${PROJ}"
