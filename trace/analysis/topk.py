import sys
from collections import Counter

# 检查是否提供了命令行参数
if len(sys.argv) != 2:
    print("请提供文件路径作为命令行参数")
    sys.exit(1)

# 从命令行参数获取文件路径
file_path = sys.argv[1]

# 读取文件
with open(file_path, "r") as file:
    data = file.read()

# 将数据分割成行
lines = data.strip().split("\n")

# 提取第二列数据
second_column = [line.split()[1] for line in lines]

# 统计每个操作的次数
counter = Counter(second_column)

# 找出前10个最高次数的操作
top = counter.most_common(10000)

# print("Top 10000 操作:")
sum = 0
for operation, count in top:
    operation_int = int(operation)
    # print(f"{operation}")
    # print(f"{operation_int}")
    # 将整数转换为128字节的小端序字节序列
    byte_sequence = operation_int.to_bytes(16, "little")

    # 打印每个字节
    print("0x", end="")
    for byte in byte_sequence:
        # 使用'{:02x}'格式化每个字节为十六进制，并打印
        print(f"{byte:02x}", end="")
    print()
    # print(f"{operation}: {count} 次")
    # sum += count
# print(f"总计: {sum} 次")
