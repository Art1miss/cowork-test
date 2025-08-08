import re
import json
from collections import defaultdict

# 配置
INPUT_LOG = "train4.txt"
OUTPUT_JSON = "transition_next5_syscallname.json"

# 正则
pid_re = re.compile(r'pid=(\d+)')
syscall_name_re = re.compile(r'\b(\w+)\(')  # 匹配 syscall 名（如 open、mmap）

# 提取调用
calls_by_pid = defaultdict(list)

with open(INPUT_LOG) as f:
    for line in f:
        pid_match = pid_re.search(line)
        syscall_match = syscall_name_re.search(line)
        if pid_match and syscall_match:
            pid = int(pid_match.group(1))
            syscall = syscall_match.group(1)
            calls_by_pid[pid].append(syscall)

# 统计每个调用后 3 条调用的出现频率
transition_counts = defaultdict(lambda: defaultdict(int))

for call_seq in calls_by_pid.values():
    for i in range(len(call_seq)):
        current = call_seq[i]
        for j in range(1, 6):  # 后三步
            if i + j < len(call_seq):
                next_call = call_seq[i + j]
                transition_counts[current][next_call] += 1


# 转为概率矩阵
transition_matrix = {}
for src, dsts in transition_counts.items():
    total = sum(dsts.values())
    transition_matrix[src] = {dst: count / total for dst, count in dsts.items()}

# 保存
with open(OUTPUT_JSON, "w") as f:
    json.dump(transition_matrix, f, indent=2)

print("✅ 已生成 transition_next3_syscallname.json （系统调用名版本）")
