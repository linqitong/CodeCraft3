from common import *
import os
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import numpy as np


# 单组折线图
def save_line_plot(data, save_path, file_name, step=1):
    """
    生成折线图并保存到指定地址，支持按步长聚合数据。

    参数:
    data (list): 要绘制的数据数组。
    save_path (str): 保存图像的路径。
    file_name (str): 保存图像的文件名（不包括扩展名）。
    step (int): 数据聚合的步长（正整数，默认为1）。

    返回:
    None
    """
    # 参数校验
    if not isinstance(step, int) or step < 1:
        raise ValueError("Step must be a positive integer.")

    # 根据步长聚合数据
    if step > 1:
        aggregated_data = []
        for i in range(0, len(data), step):
            group = data[i:i + step]
            avg = sum(group)  # 计算平均值
            aggregated_data.append(avg)
        data_to_plot = aggregated_data
    else:
        data_to_plot = data

    # 确保保存路径存在
    if not os.path.exists(save_path):
        os.makedirs(save_path)

    # 创建折线图
    plt.plot(data_to_plot)

    # 添加标题和标签
    plt.title('Line Plot' + (f' (Step={step})' if step > 1 else ''))
    plt.xlabel('Group Index' if step > 1 else 'Index')
    plt.ylabel('Average Value' if step > 1 else 'Value')

    # 保存图像
    full_path = os.path.join(save_path, f'{file_name}.png')
    plt.savefig(full_path)

    # 关闭图像，避免内存泄漏
    plt.close()

    print(f"折线图已保存到: {full_path}")


# 多组折线图
def save_multi_line_plot(data, save_path, file_name, variable_names=None):
    """
    生成多组折线图并保存到指定地址。

    参数:
    data (list of lists): 要绘制的二重数组，每个子列表代表一个数据序列。
    save_path (str): 保存图像的路径。
    file_name (str): 保存图像的文件名（不包括扩展名）。
    variable_names (list of str, optional): 每个数据序列的变量名。如果提供，长度必须与 data 一致。

    返回:
    None
    """
    # 检查数据是否有效
    if not all(len(sub_list) == len(data[0]) for sub_list in data):
        raise ValueError("所有数据序列的长度必须一致")

    if variable_names and len(variable_names) != len(data):
        raise ValueError("变量名的数量必须与数据序列的数量一致")

    # 确保保存路径存在
    if not os.path.exists(save_path):
        os.makedirs(save_path)

    # 创建折线图
    plt.figure()
    for i, sub_data in enumerate(data):
        label = variable_names[i] if variable_names else f'Variable {i + 1}'
        plt.plot(sub_data, label=label)

    # 添加标题和标签
    plt.title('Multi-Line Plot')
    plt.xlabel('Index')
    plt.ylabel('Value')

    # 添加图例
    if variable_names:
        plt.legend()

    # 保存图像
    full_path = os.path.join(save_path, f'{file_name}.png')
    plt.savefig(full_path)

    # 关闭图像，避免内存泄漏
    plt.close()

    print(f"多组折线图已保存到: {full_path}")


# 散点图
def draw_scatter_plot(data, save_path, file_name, x_range=None, y_range=None):
    elements = 2000
    max_frames = 90000

    # 创建画布
    plt.figure(figsize=(16, 10), dpi=100)
    plt.title("Element Access Pattern Across Frames", fontsize=14, pad=20)
    plt.xlabel("Frame Number", fontsize=12)
    plt.ylabel("Element ID", fontsize=12)

    # 生成坐标数据
    x = []
    y = []
    for element_id, frames in enumerate(data):
        # Filter frames based on x_range if specified
        if x_range is not None:
            frames = [frame for frame in frames if x_range[0] <= frame <= x_range[1]]

        # Only add points if element_id is within y_range (if specified)
        if y_range is None or (y_range[0] <= element_id <= y_range[1]):
            x.extend(frames)
            y.extend([element_id] * len(frames))

        if element_id > elements:
            elements += 1000

    # 绘制散点图
    plt.scatter(x, y,
               s=5,          # 点大小
               alpha=0.8,    # 透明度
               c='royalblue',# 颜色
               edgecolors='none')

    # 优化坐标轴
    plt.xlim(x_range if x_range is not None else (0, max_frames))
    plt.ylim(y_range if y_range is not None else (0, elements))
    plt.grid(alpha=0.2)

    # 显示图表
    plt.tight_layout()
    # 保存图像
    full_path = os.path.join(save_path, f'{file_name}.png')
    plt.savefig(full_path)

    # 关闭图像，避免内存泄漏
    plt.close()

    print(f"散点图已保存到: {full_path}")


def plot_elements(data, save_path, file_name):
    elements = 2000
    max_frames = 90000

    # 准备数据
    starts = []  # 开始帧
    ends = []  # 结束帧
    indices = []  # 元素索引

    for idx, (start, end) in enumerate(data):
        indices.append(idx)
        starts.append(start)
        ends.append(end)

    # 创建图形
    plt.figure(figsize=(12, 8))

    # 绘制开始帧（用绿色表示）
    plt.scatter(starts, indices, color='green', label='Start Frame', s=10)

    # 绘制结束帧（用红色表示）
    plt.scatter(ends, indices, color='red', label='End Frame', s=10)

    # 添加连接线（浅灰色虚线）
    for idx, (start, end) in enumerate(data):
        plt.plot([start, end], [idx, idx], '--', color='lightgray', linewidth=0.5, alpha=0.5)

    # 设置坐标轴
    plt.xlabel('Frame Number', fontsize=12)
    plt.ylabel('Element Index', fontsize=12)
    plt.title('Element Start/End Frames', fontsize=14)

    # 优化坐标轴
    plt.xlim((0, max_frames))
    plt.ylim((0, elements))
    plt.grid(alpha=0.2)

    # 显示网格
    plt.grid(True, linestyle='--', alpha=0.6)

    # 调整布局
    plt.tight_layout()

    # 保存图像
    full_path = os.path.join(save_path, f'{file_name}.png')
    plt.savefig(full_path)

    # 关闭图像，避免内存泄漏
    plt.close()

    print(f"散点图已保存到: {full_path}")

def plot_elements_2(data, save_path, file_name):
    elements = 2000
    max_frames = 90000

    # 准备数据
    starts = []  # 开始帧
    ends = []  # 结束帧
    indices = []  # 元素索引
    requests = []

    for idx, (start, end, first_read) in enumerate(data):
        indices.append(idx)
        starts.append(start)
        ends.append(end)
        requests.append(first_read)


    # 创建图形
    plt.figure(figsize=(12, 8))

    # 绘制开始帧（用绿色表示）
    plt.scatter(starts, indices, color='green', label='Start Frame', s=1)

    # 绘制结束帧（用红色表示）
    plt.scatter(ends, indices, color='red', label='End Frame', s=1)

    # 绘制第一次的请求帧
    plt.scatter(requests, indices, color='blue', label='First Read Frame', s=1)

    # 添加连接线（浅灰色虚线）
    # for idx, (start, end) in enumerate(data):
        # plt.plot([start, end], [idx, idx], '--', color='lightgray', linewidth=0.5, alpha=0.5)

    # 设置坐标轴
    plt.xlabel('Frame Number', fontsize=12)
    plt.ylabel('Element Index', fontsize=12)
    plt.title('Element Start/End/FR Frames', fontsize=14)

    # 优化坐标轴
    plt.xlim((0, max_frames))
    plt.ylim((0, elements))
    plt.grid(alpha=0.2)

    # 显示网格
    plt.grid(True, linestyle='--', alpha=0.6)

    # 调整布局
    plt.tight_layout()

    # 保存图像
    full_path = os.path.join(save_path, f'{file_name}.png')
    plt.savefig(full_path)

    # 关闭图像，避免内存泄漏
    plt.close()

    print(f"散点图已保存到: {full_path}")


def plot_elements_3(data, save_path, file_name):
    elements = 2000
    max_frames = 90000

    # 准备数据
    starts = []  # 开始帧
    ends = []  # 结束帧
    indices = []  # 元素索引
    requests = []

    for idx, (start, end) in enumerate(data):
        indices.append(idx)
        starts.append(start)
        ends.append(end)


    # 创建图形
    plt.figure(figsize=(12, 8))

    # 绘制开始帧（用绿色表示）
    plt.scatter(starts, indices, color='green', label='Start Frame', s=1)

    # 绘制结束帧（用红色表示）
    # plt.scatter(ends, indices, color='red', label='End Frame', s=1)

    # 绘制第一次的请求帧
    # plt.scatter(requests, indices, color='blue', label='First Read Frame', s=1)

    # 添加连接线（浅灰色虚线）
    # for idx, (start, end) in enumerate(data):
        # plt.plot([start, end], [idx, idx], '--', color='lightgray', linewidth=0.5, alpha=0.5)

    # 设置坐标轴
    plt.xlabel('Frame Number', fontsize=12)
    plt.ylabel('Element Index', fontsize=12)
    plt.title('Element Start/End/FR Frames', fontsize=14)

    # 优化坐标轴
    plt.xlim((0, elements))
    plt.ylim((0, elements))
    plt.grid(alpha=0.2)

    # 显示网格
    plt.grid(True, linestyle='--', alpha=0.6)

    # 调整布局
    plt.tight_layout()

    # 保存图像
    full_path = os.path.join(save_path, f'{file_name}.png')
    plt.savefig(full_path)

    # 关闭图像，避免内存泄漏
    plt.close()

    print(f"散点图已保存到: {full_path}")