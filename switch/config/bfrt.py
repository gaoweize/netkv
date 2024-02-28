port = [180, 164, 148, 132]
MAC = [0x1070fd190095, 0x1070fd2fd851, 0x1070fd2fe441, 0x1070fd2fd421]
for worker in range(len(port)):
    bfrt.netkv.pipe.SwitchIngress.forward.add_with_hit(
        MAC[worker], port[worker])

for p in port:
    bfrt.port.port.add(p, "BF_SPEED_100G", "BF_FEC_TYP_RS",
                       4, True, "PM_AN_FORCE_DISABLE")

bfrt.netkv.pipe.SwitchIngress.check_cache_exist.add_with_check_cache_exist_hit(0x00000000000000000000000000000001, 0)
bfrt.netkv.pipe.SwitchIngress.check_cache_exist.add_with_check_cache_exist_hit(0x00000000000000000000000000000002, 1)
bfrt.netkv.pipe.SwitchIngress.check_cache_exist.add_with_check_cache_exist_hit(0x00000000000000000000000000000003, 2)
bfrt.netkv.pipe.SwitchIngress.check_cache_exist.add_with_check_cache_exist_hit(0x00000000000000000000000000000004, 3)