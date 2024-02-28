#ifndef _REGISTER_
#define _REGISTER_

#define CACHE_REGISTER(i) \
    Register<bit<32>, bit<32>>(cache_size, 0) cache_value##i; \
    RegisterAction<bit<32>, bit<32>, bit<32>>(cache_value##i) read_cache_value##i##_action = { \
        void apply(inout bit<32> val, out bit<32> rv) { \
            rv = val; \
        } \
    }; \
    action read_cache_value##i (bit<32> index) { \
        hdr.netcache.value##i = read_cache_value##i##_action.execute(index); \
    } \
    RegisterAction<bit<32>, bit<32>, bit<32>>(cache_value##i) write_cache_value##i##_action = { \
        void apply(inout bit<32> val) { \
            val = hdr.netcache.value##i; \
        } \
    }; \
    action write_cache_value##i (bit<32> index) { \
        write_cache_value##i##_action.execute(index); \
    }

#endif /* _HEADERS_ */