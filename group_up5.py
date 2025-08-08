import re
import json
from collections import deque

# === CONFIG ===
LOG_FILE = "train2.txt"
MATRIX_FILE = "transition_next5_syscallname.json"
OUTPUT_FILE = "grouped_syscalls5.txt"
THRESHOLD = 0.1
WINDOW_SIZE = 3

# === 正则 ===
syscall_name_re = re.compile(r'\b(\w+)\(')

def extract_syscall(line):
    match = syscall_name_re.search(line)
    return match.group(1) if match else None

# === 读取转移矩阵 ===
with open(MATRIX_FILE) as f:
    matrix = json.load(f)

# === 读取日志 ===
with open(LOG_FILE) as f:
    lines = f.readlines()

groups = []
current_group = []
sliding_window = deque(maxlen=WINDOW_SIZE)

for line in lines:
    syscall = extract_syscall(line)
    if not syscall:
        continue

    matched = False

    # 检查窗口内是否有任一 syscall → 当前 syscall 的概率 > 阈值
    for prev in sliding_window:
        if matrix.get(prev, {}).get(syscall, 0) > THRESHOLD:
            matched = True
            break

    if not current_group or matched:
        current_group.append(line.strip())
    else:
        groups.append(current_group)
        current_group = [line.strip()]

    sliding_window.append(syscall)

# 最后一组
if current_group:
    groups.append(current_group)

# === 写入输出 ===
with open(OUTPUT_FILE, "w") as f:
    for i, group in enumerate(groups):
        f.write(f"\n--- Group {i+1} ---\n")
        for line in group:
            f.write(line + "\n")

print(f"✅ 分组完成：共 {len(groups)} 组，写入 {OUTPUT_FILE}")
