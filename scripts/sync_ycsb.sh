#!/bin/bash

DIR=$(dirname $0)
PROJ="code/netkv/trace/YCSB-cpp/"
. $DIR/utils.sh

for i in {1..2}
do
	echo_r "rsync -avzhcqPe 'ssh -p 2332 gaoweize@162.105.218.62 -t ssh -p 22' ${DIR}/../trace/YCSB-cpp/  gaoweize@10.0.0.${i}:/home/gaoweize/${PROJ}/" 
done