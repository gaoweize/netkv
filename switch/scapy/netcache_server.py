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


def process_packet(packet):
    if packet.haslayer(netcache) and packet[Ether].dst == "10:70:fd:2f:d8:51":
        print("==== Request packet ====")
        hexdump(packet)
        # print(packet.haslayer(IP))
        # print(packet[IP].proto)
        # Assuming you have defined the netcache class
        # Swap source and destination MAC addresses
        packet[Ether].src, packet[Ether].dst = packet[Ether].dst, packet[Ether].src
        packet[netcache].is_req = 0
        packet[netcache].value4 = 2
        # Send the response packet
        print("==== Response packet ====")
        hexdump(packet)
        sendp(packet, iface=iface)
        # exit(0)


def main():
    sniff(iface=iface, prn=process_packet)


if __name__ == "__main__":
    main()
