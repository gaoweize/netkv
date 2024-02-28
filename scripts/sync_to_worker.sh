#!/bin/bash

DIR=$(dirname $0)
PROJ="code/netkv"
. $DIR/utils.sh

for i in {1..2}
do
	echo_r "rsync -avzhcqPe 'ssh -p 2332 gaoweize@162.105.218.62 -t ssh -p 22' --exclude=trace/ycsb --exclude=trace/YCSB-cpp ${DIR}/../  gaoweize@10.0.0.${i}:/home/gaoweize/${PROJ}" 
done

echo_r "ssh -p 2332 gaoweize@162.105.218.62 -t \"ssh -p 22 gaoweize@10.0.0.1 'cd /home/gaoweize/${PROJ}/host/ && make clean && make' \""
echo_r "ssh -p 2332 gaoweize@162.105.218.62 -t \"ssh -p 22 gaoweize@10.0.0.1 'scp -r /home/gaoweize/${PROJ}/host/build/ gaoweize@10.0.0.2:/home/gaoweize/${PROJ}/host/' \""