#!/usr/bin/env python3

import re
import time

from scapy.all import *
from scapy.layers.inet import *


class netcache(Packet):
    name = "netcache"
    fields_desc = [
        IntField("psn", 0),
        IntField("gpsn", 0),
        IntField("reuse_distance", 0),
        BitField("is_req", 0, 1),
        BitField("op", 0, 2),
        BitField("reserved", 0, 5),
        IntField("key1", 0),
        IntField("key2", 0),
        IntField("key3", 0),
        IntField("key4", 0),
        IntField("value1", 0),
        IntField("value2", 0),
        IntField("value3", 0),
        IntField("value4", 0),
    ]


bind_layers(IP, netcache, proto=0x54)
iface = "ens10f1np1"


def print_resp(resp):
    print("==== Response packet ====")
    hexdump(resp)
    if resp:
        p4calc = resp[netcache]
        if p4calc:
            print(p4calc)
        else:
            print("cannot find netcache header in the packet")
    else:
        print("Didn't receive response")


def main():
    pkt = (
        Ether(dst="10:70:fd:2f:d8:51", src="10:70:fd:19:00:95")
        / IP()
        / netcache(psn=3, reuse_distance=0, is_req=0x1, op=0x0, key4=1, value4=0)
    )
    print("==== Request packet ====")
    hexdump(pkt)
    resp = srp1(pkt, iface=iface, timeout=1, verbose=False)
    print_resp(resp)

    pkt = (
        Ether(dst="10:70:fd:2f:d8:51", src="10:70:fd:19:00:95")
        / IP()
        / netcache(psn=4, reuse_distance=0, is_req=0x1, op=0x0, key4=2, value4=0)
    )
    print("==== Request packet ====")
    hexdump(pkt)
    resp = srp1(pkt, iface=iface, timeout=1, verbose=False)
    print_resp(resp)

    pkt = (
        Ether(dst="10:70:fd:2f:d8:51", src="10:70:fd:19:00:95")
        / IP()
        / netcache(psn=5, reuse_distance=0, is_req=0x1, op=0x0, key4=3, value4=0)
    )
    print("==== Request packet ====")
    hexdump(pkt)
    resp = srp1(pkt, iface=iface, timeout=1, verbose=False)
    print_resp(resp)
    


if __name__ == "__main__":
    main()
