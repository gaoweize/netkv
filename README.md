### How To Run

#### Switch
netcache

``` shell
cd ~/gwz/code/netcacheplus
./scripts/run_switch.sh netcache switch/netcache/bfrt.py
```

netcacheplus
``` shell
./scripts/compile.sh switch/p4src/netcacheplus.p4 switch/build

cd ~/gwz/code/netcacheplus
./scripts/run_switch.sh netcacheplus switch/bfrt.py
```

#### Host
netcache

``` shell
cd ~/code/netcacheplus/host
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=1 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace1.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace1" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=1 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace2.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace2" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=1 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace3.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace3" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=1 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace4.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace4" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=1 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace5.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace5" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=1 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace6.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace6" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=1 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace7.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace7" --loss=0.1

sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace1.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace1_2" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace2.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace2_2" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace3.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace3_2" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace4.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace4_2" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace5.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace5_2" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace6.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace6_2" --loss=0.1
sudo ./build/netcache --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace7.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace7_2" --loss=0.1
```

netcacheplus
``` shell
cd ~/code/netcacheplus/host
sudo ./build/netcacheplus --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/test_trace.txt" --loss=0.1

sudo ./build/netcacheplus --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace1.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace1_3" --loss=0.1
sudo ./build/netcacheplus --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace2.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace2_3" --loss=0.1
sudo ./build/netcacheplus --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace3.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace3_3" --loss=0.1
sudo ./build/netcacheplus --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace4.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace4_3" --loss=0.1
sudo ./build/netcacheplus --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace5.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace5_3" --loss=0.1
sudo ./build/netcacheplus --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace6.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace6_3" --loss=0.1
sudo ./build/netcacheplus --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=256 --trace="/home/gaoweize/code/netcacheplus/trace/exp1/trace7.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp1/trace7_3" --loss=0.1

sudo ./build/netcacheplus --client=192.168.0.1 --server=192.168.0.2 --thread=1 --window=1024 --trace="/home/gaoweize/code/netcacheplus/trace/exp2/trace.txt" --result="/home/gaoweize/code/netcacheplus/trace/exp2/trace1_3" --loss=0


sudo ./build/netcache --client=192.168.1.1 --server=192.168.1.2 --thread=1 --window=1 --trace="/home/gaoweize/code/dpdk2_test/trace/exp1/trace1.txt" --loss=0
```


``` shell
sudo ./host/build/netkv --client=192.168.1.1 --server=192.168.1.2 --thread=4 --window=256 --trace=./trace/ycsb/workloada_run --loss=0
sudo ./host/build/netkv --client=192.168.1.1 --server=192.168.1.2 --thread=1 --window=256 --trace=./trace/ycsb/workloada_run --loss=0
sudo ./host/build/netkv --client=192.168.1.1 --server=192.168.1.2 --thread=1 --window=128 --trace=./trace/ycsb/workloada_run --loss=0
sudo ./host/build/netkv --client=192.168.1.1 --server=192.168.1.2 --thread=1 --window=1 --trace=./trace/ycsb/workloada_load.txt --loss=0 --load
```
