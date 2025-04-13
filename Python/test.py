import matplotlib.pyplot as plt
import numpy as np
import math
import common
import draw
import os

common.G_token_per_time_step = 1000


def global_get_read_time(read_num, have_read_time):
    data = [0] * 7
    data[0] = 64
    for n1 in range(1, 7):
        data[n1] = max(16, math.ceil(data[n1 - 1] * 0.8))

    total_read = have_read_time + read_num
    index = have_read_time
    all_time = 0

    while index < total_read:
        if index < 7:
            all_time += data[index]
            index += 1
        else:
            # Process all remaining reads and break the loop
            remaining = total_read - index
            all_time += remaining * 16
            break

    return all_time


# 计算完成读取指定块数的任务需要多少帧
def calculate_frames_needed(blocks_to_read, consecutive_reads):
    frames = 0
    remaining_blocks = blocks_to_read

    while remaining_blocks > 0:
        frames += 1
        tokens_available = common.G_token_per_time_step
        blocks_read_this_frame = 0

        # 在当前帧中尽可能多地读取块
        while tokens_available > 0 and remaining_blocks > 0:
            # 计算读取下一个块需要的token数
            tokens_needed = global_get_read_time(1, consecutive_reads + blocks_read_this_frame)

            if tokens_needed <= tokens_available:
                tokens_available -= tokens_needed
                blocks_read_this_frame += 1
                remaining_blocks -= 1
            else:
                break  # 当前帧token不足，跳到下一帧

        # 更新连续读取次数（不会因为帧切换而重置）
        consecutive_reads += blocks_read_this_frame

    return frames


def get_mark_efficiency(time):
    if 0 <= time <= 10:
        return 1.0 - 0.005 * time
    elif 10 < time <= 105:
        return 1.05 - 0.01 * time
    else:
        return 0.0


# time = global_get_read_time(100, 0)

read_efficiency = []
times = []

for i in range(50):
    read_efficiency.append((i + 1) * 10 / calculate_frames_needed((i + 1) * 10, 0))
    times.append(calculate_frames_needed((i + 1) * 10, 0))

nums = []
for i in range(7):
    pass_time = global_get_read_time(7, 0)
    read_time = global_get_read_time(7, i)
    nums.append((pass_time - read_time) / 16)

# draw.save_line_plot(result, "./Picture", "read_efficiency")

# 200 的段大小是一个分配与效率都很优的策略

time = calculate_frames_needed(600, 0)

a = 1