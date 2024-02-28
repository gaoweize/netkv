import sys

print(
    """port = [180, 164, 148, 132]
MAC = [0x1070fd190095, 0x1070fd2fd851, 0x1070fd2fe441, 0x1070fd2fd421]
for worker in range(len(port)):
    bfrt.netkv.pipe.SwitchIngress.forward.add_with_hit(
        MAC[worker], port[worker])

for p in port:
    bfrt.port.port.add(p, "BF_SPEED_100G", "BF_FEC_TYP_RS",
                       4, True, "PM_AN_FORCE_DISABLE")

i = 1
IP = [0x00000001, 0x00000002, 0x00000003, 0x00000004]

for ip in IP:
    bfrt.netkv.pipe.SwitchIngress.client_to_index.add_with_client_to_index_action(ip, i) 
    i += 1
"""
)

args = sys.argv
i = 0

with open(args[1], "r") as file:
    for line in file:
        print(
            "bfrt.netkv.pipe.SwitchIngress.check_cache_exist.add_with_check_cache_exist_hit("
            + line.strip()
            + ", "
            + str(i)
            + ")"
        )
        i += 1
