/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2018-2019 Barefoot Networks, Inc.

 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of
 * Barefoot Networks, Inc. and its suppliers, if any. The intellectual and
 * technical concepts contained herein are proprietary to Barefoot Networks,
 * Inc.
 * and its suppliers and may be covered by U.S. and Foreign Patents, patents in
 * process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is
 * strictly forbidden unless prior written permission is obtained from
 * Barefoot Networks, Inc.
 *
 * No warranty, explicit or implicit is provided, unless granted under a
 * written agreement with Barefoot Networks, Inc.
 *
 *
 ******************************************************************************/

#ifndef _HEADERS_
#define _HEADERS_

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;

typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;

typedef bit<8> ip_protocol_t;
const ip_protocol_t IP_PROTOCOLS_NETCACHE = 8w0x54;

typedef bit<128> netcache_key_t;
typedef bit<32> netcache_value_slot_t;

header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16> ether_type;
}

header ipv4_h {
    bit<4> version;
    bit<4> ihl;
    bit<8> diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3> flags;
    bit<13> frag_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header netcache_h {
    bit<32> psn;
    bit<32> gpsn;
    bit<32> index;
    bit<32> reuse_distance;
    bit<1> is_req;
    bit<2> op;
    bit<1> cached;
    bit<4> reserved;
    netcache_key_t key;
    netcache_value_slot_t value1;
    netcache_value_slot_t value2;
    netcache_value_slot_t value3;
    netcache_value_slot_t value4;

#ifdef DEBUG
    bit<32> process_index_mod;
    bit<32> process_pre_reuse_index_diff_mod;
    bit<32> process_pre_w_index_diff_mod;

    bit<32> process_index_add_a;
    bit<32> process_pre_reuse_index;
    bit<32> process_pre_w_index;
    bit<32> process_index;

    bit<8> process_pre_reuse_valid;
    bit<8> process_valid;
    bit<8> seq_changed;
#endif
}

struct header_t {
    ethernet_h ethernet;
    ipv4_h ipv4;
    netcache_h netcache;
}

header resubmit_type {
    bit<8> process_pre_reuse_valid;
    bit<8> process_valid;
    bit<48> unused;
}
#endif /* _HEADERS_ */
