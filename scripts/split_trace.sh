DIR=$(dirname $0)
. $DIR/utils.sh

echo_r "python $DIR/../trace/analysis/split.py $DIR/../trace/ycsb/workloada_run.txt $DIR/../trace/ycsb/workloada_run --lines_per_file 1000000"
