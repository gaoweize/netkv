#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#define DEBUG

#include "util.p4"
#include "headers.p4"
#include "register.p4"

const bit<3> DPRSR_DIGEST_TYPE_A = 0;

struct metadata_t {
    resubmit_type resubmit_metadata;

    bit<32> index;
    bit<32> client_index;
    bit<32> seq;
    bit<1> cache_exist;
    bit<8> cache_valid;
    bit<32> process_pre_reuse_index_diff;
    bit<32> process_pre_w_index_diff;
    bit<32> process_index_mod;
    bit<32> process_pre_reuse_index_diff_mod;
    bit<32> process_pre_w_index_diff_mod;
    bit<32> process_index_add_a;
    bit<32> process_pre_reuse_index;
    bit<32> process_pre_w_index;
    bit<32> process_index;
    
    
    bit<8> seq_changed;

    netcache_value_slot_t value1;
    netcache_value_slot_t value2;
    netcache_value_slot_t value3;
    netcache_value_slot_t value4;
}

// ---------------------------------------------------------------------------
// Ingress parser
// ---------------------------------------------------------------------------
parser SwitchIngressParser(
        packet_in pkt,
        out header_t hdr,
        out metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    state start {
        pkt.extract(ig_intr_md);
        /* The resubmit flag indicates if the packet has been resubmitted.  When
         * set the Port Metadata is not present and those bytes instead carry
         * the resubmit data. */
        transition select(ig_intr_md.resubmit_flag) {
            0 : parse_port_metadata;
            1 : parse_resubmit;
        }
    }

    state parse_port_metadata {
        pkt.advance(64);
        transition parse_ethernet;
    }

    state parse_resubmit {
        pkt.extract(ig_md.resubmit_metadata);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4 : parse_ipv4;
            default : accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IP_PROTOCOLS_NETCACHE : parse_netcache;
            default : accept;
        }
    }

    state parse_netcache {
        pkt.extract(hdr.netcache);
        // meta init
        ig_md.seq_changed = 0x0;
        transition accept;
    }
}

// ---------------------------------------------------------------------------
// Ingress Deparser
// ---------------------------------------------------------------------------
control SwitchIngressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in metadata_t ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {

    Resubmit() resubmit;

    apply {
        if (ig_dprsr_md.resubmit_type == DPRSR_DIGEST_TYPE_A) {
            resubmit.emit(ig_md.resubmit_metadata);
        }
        pkt.emit(hdr);
    }
}

control SwitchIngress(
        inout header_t hdr,
        inout metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    // ***** forward *****
    action hit(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
    }

    action miss(bit<3> drop) {
        ig_dprsr_md.drop_ctl = drop; // Drop packet.
    }

    mac_addr_t mac_tmp;
    ipv4_addr_t ipv4_tmp; 
    action swap_src_dst() {
        mac_tmp = hdr.ethernet.dst_addr;
        hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
        hdr.ethernet.src_addr = mac_tmp;
        ipv4_tmp = hdr.ipv4.dst_addr;
        hdr.ipv4.dst_addr = hdr.ipv4.src_addr;
        hdr.ipv4.src_addr = ipv4_tmp;
    }

    table forward {
        key = {
            hdr.ethernet.dst_addr : exact;
        }

        actions = {
            hit;
            @defaultonly miss;
        }

        const default_action = miss(0x1);
        size = 1024;
    }

    // ***** client_to_index *****
    action client_to_index_action(bit<32> index) {
        ig_md.client_index = index;
    }

    table client_to_index {
        key = {
            hdr.ipv4.src_addr : exact;
        }

        actions = {
            client_to_index_action;
            @defaultonly miss;
        }

        const default_action = miss(0x1);
        size = 1024;
    }

    // ***** cache_exist *****
    action check_cache_exist_hit(bit<32> index) {
        ig_md.index = index;
        ig_md.cache_exist = 0x1;
        hdr.netcache.index = index;
        hdr.netcache.cached = 0x1;
    }

    table check_cache_exist {
        key = {
            hdr.netcache.key : exact;
        }

        actions = {
            check_cache_exist_hit;
        }

        size = 1 << 16;
    }

    // ***** cache_valid *****
    const bit<32> cache_size = 1 << 16;
    Register<bit<32>, bit<8>>(cache_size, 0) cache_valid_reg;
    RegisterAction<bit<8>, bit<32>, bit<8>>(cache_valid_reg) read_cache_valid_action = {
        void apply(inout bit<8> val, out bit<8> rv) {
            rv = val;
        }
    };
    action check_cache_valid(bit<32> index) {
        ig_md.cache_valid = read_cache_valid_action.execute(index);
    }

    RegisterAction<bit<8>, bit<32>, bit<8>>(cache_valid_reg) set_cache_valid_action = {
        void apply(inout bit<8> val) {
            val = 0x1;
        }
    };
    action set_cache_valid(bit<32> index) {
        set_cache_valid_action.execute(index);
    }

    RegisterAction<bit<8>, bit<32>, bit<8>>(cache_valid_reg) reset_cache_valid_action = {
        void apply(inout bit<8> val) {
            val = 0x0;
        }
    };
    action reset_cache_valid(bit<32> index) {
        reset_cache_valid_action.execute(index);
    }

    // ***** cache register *****
    CACHE_REGISTER(1)
    CACHE_REGISTER(2)
    CACHE_REGISTER(3)
    CACHE_REGISTER(4)


    // ***** process_valid *****
    const bit<32> process_size = 1 << 9;
    const bit<32> process_size_half = 1 << 8;
    const bit<32> process_size_total = 1 << 12;
    Register<bit<32>, bit<8>>(process_size_total, 0) process_valid;
    RegisterAction<bit<8>, bit<32>, bit<8>>(process_valid) read_process_valid_action = {
        void apply(inout bit<8> val, out bit<8> rv) {
            rv = val;
        }
    };
    action check_process_valid(inout bit<8> rv, bit<32> index) {
        rv = read_process_valid_action.execute(index);
    }

    RegisterAction<bit<8>, bit<32>, bit<8>>(process_valid) set_process_valid_action = {
        void apply(inout bit<8> val) {
            val = 0x1;
        }
    };
    action set_process_valid(bit<32> index) {
        set_process_valid_action.execute(index);
    }

    RegisterAction<bit<8>, bit<32>, bit<8>>(process_valid) reset_process_valid_action = {
        void apply(inout bit<8> val) {
            val = 0x0;
        }
    };
    action reset_process_valid(bit<32> index) {
        reset_process_valid_action.execute(index);
    }

    // ***** seq_reg *****
    Register<bit<32>, bit<32>>(cache_size, 0) seq_reg;
    RegisterAction<bit<32>, bit<32>, bit<32>>(seq_reg) inc_seq_action = {
        void apply(inout bit<32> val, out bit<32> rv) {
            rv = val;
            val = val + 1;
        }
    };
    action inc_seq(inout bit<32> seq, bit<32> index) {
        seq = inc_seq_action.execute(index);
    }

    // ***** seq_array_reg *****
    Register<bit<32>, bit<32>>(process_size_total, 0) seq_array_reg;
    RegisterAction<bit<32>, bit<32>, bit<32>>(seq_array_reg) read_seq_array_action = {
        void apply(inout bit<32> val, out bit<32> rv) {
            rv = val;
        }
    };
    action read_seq_array(bit<32> index) {
        ig_md.seq = read_seq_array_action.execute(index);
    }

    RegisterAction<bit<32>, bit<32>, bit<32>>(seq_array_reg) write_seq_array_action = {
        void apply(inout bit<32> val) {
            val = ig_md.seq;
        }
    };
    action write_seq_array(bit<32> index) {
        write_seq_array_action.execute(index);
    }

    action process_client_index_add_a() {
        ig_md.process_index_add_a = ig_md.client_index * process_size;
    }

    action process_pre_reuse_index_sub() {
        ig_md.process_pre_reuse_index_diff = hdr.netcache.psn - hdr.netcache.reuse_distance;
    }

    action process_pre_w_index_sub() {
        ig_md.process_pre_w_index_diff = hdr.netcache.psn - process_size_half;
    }

    action process_index_mod() {
        ig_md.process_index_mod = hdr.netcache.psn % process_size;
    }

    action process_pre_reuse_index_mod() {
        ig_md.process_pre_reuse_index_diff_mod = ig_md.process_pre_reuse_index_diff % process_size;
    }

    action process_pre_w_index_mod() {
        ig_md.process_pre_w_index_diff_mod = ig_md.process_pre_w_index_diff % process_size;
    }

    apply {
        if (hdr.netcache.isValid()) {
            check_cache_exist.apply();
            client_to_index.apply();

            if(hdr.netcache.is_req == 0x1) {
                // ig_md.process_index = ig_md.client_index * process_size + hdr.netcache.psn % process_size;
                if(hdr.netcache.op == 0x0 || hdr.netcache.op == 0x1){
                    process_client_index_add_a();
                    process_index_mod();
                    ig_md.process_index = ig_md.process_index_add_a + ig_md.process_index_mod;
                    process_pre_reuse_index_sub();
                    process_pre_reuse_index_mod();
                    ig_md.process_pre_reuse_index = ig_md.process_index_add_a + ig_md.process_pre_reuse_index_diff_mod;
                    process_pre_w_index_sub();
                    process_pre_w_index_mod();
                    ig_md.process_pre_w_index = ig_md.process_index_add_a + ig_md.process_pre_w_index_diff_mod;

                    if(ig_intr_md.resubmit_flag == 0) {
                        check_process_valid(ig_md.resubmit_metadata.process_pre_reuse_valid, ig_md.process_pre_reuse_index);
                        check_process_valid(ig_md.resubmit_metadata.process_valid, ig_md.process_index);
                        ig_dprsr_md.resubmit_type = DPRSR_DIGEST_TYPE_A;
                    }else{
                        if(ig_md.resubmit_metadata.process_valid == 0x0) {
                            if(hdr.netcache.reuse_distance == 0x0 || ig_md.resubmit_metadata.process_pre_reuse_valid == 0x1) {
                                set_process_valid(ig_md.process_index);
                                // reset_process_valid(ig_md.process_pre_w_index);
                                ig_md.seq_changed = 0x1;
                                if(ig_md.cache_exist == 0x1) {
                                    if(hdr.netcache.op == 0x00) {
                                        // READ
                                        check_cache_valid(ig_md.index);
                                        if(ig_md.cache_valid == 0x1) {
                                            read_cache_value1(ig_md.index);
                                            read_cache_value2(ig_md.index);
                                            read_cache_value3(ig_md.index);
                                            read_cache_value4(ig_md.index);
                                            
                                            hdr.netcache.is_req = 0x0;
                                            swap_src_dst();
                                        }
                                    }else if(hdr.netcache.op == 0x01) {
                                        // WRITE
                                        reset_cache_valid(ig_md.index);
                                    }
                                }
                            }else{
                                miss(0x1);
                            }
                        }
                        if(hdr.netcache.cached == 0x1){
                            if(ig_md.seq_changed == 0x1) {
                                inc_seq(ig_md.seq, ig_md.index);
                                write_seq_array(ig_md.process_index);
                            }else{
                                read_seq_array(ig_md.process_index);
                            }
                            hdr.netcache.gpsn = ig_md.seq;
                        }
                    }
                }
            }else{
                if(ig_md.cache_exist == 0x1) {
                    set_cache_valid(ig_md.index);
                    write_cache_value1(ig_md.index);
                    write_cache_value2(ig_md.index);
                    write_cache_value3(ig_md.index);
                    write_cache_value4(ig_md.index);
                }
            }
        }

        forward.apply();
        ig_tm_md.bypass_egress = 0x1;

#ifdef DEBUG
    hdr.netcache.process_index_mod = ig_md.process_index_mod;
    hdr.netcache.process_pre_reuse_index_diff_mod = ig_md.process_pre_reuse_index_diff_mod;
    hdr.netcache.process_pre_w_index_diff_mod = ig_md.process_pre_w_index_diff_mod;
    hdr.netcache.process_index = ig_md.process_index;
    hdr.netcache.process_pre_reuse_index = ig_md.process_pre_reuse_index;
    hdr.netcache.process_pre_w_index = ig_md.process_pre_w_index;
    hdr.netcache.process_index_add_a = ig_md.process_index_add_a;

    hdr.netcache.process_pre_reuse_valid = ig_md.resubmit_metadata.process_pre_reuse_valid;
    hdr.netcache.process_valid = ig_md.resubmit_metadata.process_valid;
    hdr.netcache.seq_changed = ig_md.seq_changed;
#endif        
    }
}

Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         EmptyEgressParser<header_t, metadata_t>(),
         EmptyEgress<header_t, metadata_t>(),
         EmptyEgressDeparser<header_t, metadata_t>()) pipe;

Switch(pipe) main;
