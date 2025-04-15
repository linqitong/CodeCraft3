# -*- coding: utf-8 -*-

import numpy as np
import os
import csv
import logging
import matplotlib.pyplot as plt
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)
from tqdm import tqdm

MAX_TAG = 16
MAX_SIZE = 5
MAX_TIME = 10000 

def load_infile(infile):
    logger.info("Loading file: %s" % infile)
    time_stramps = []
    call_writes = []
    call_reads = []
    call_deletes = []

    with open(infile, 'r') as f:
        lines = f.readlines()

    time_stamp = -1
    delmod = 0
    readmod = 0
    writemod = 0

    for line in lines:
        if line.startswith("TIMESTAMP"):
            time_stamp = int(line.split()[1])
            data = [time_stamp, call_writes, call_reads, call_deletes]
            call_writes,call_reads,call_deletes = [],[],[]
            time_stramps.append(data)
            delmod = -1
            readmod = -1
            writemod = -1
            continue
        elif line.startswith("GARBAGE COLLECTION"):
            continue

        if delmod != 0:
            # read 1 integers:
            data = line.split()
            if delmod == -1:
                delmod = int(data[0])
                continue
            else:
                call_deletes.append([int(data[0])])
                delmod -= 1
                continue
        elif writemod != 0:
            # read 3 integers:
            data = line.split()
            if writemod == -1:
                writemod = int(data[0])
                continue
            else:
                call_writes.append([int(data[0]), int(data[1]), int(data[2])])
                writemod -= 1
                continue
        elif readmod != 0:
            # read 2 integers:
            data = line.split()
            if readmod == -1:
                readmod = int(data[0])
                continue
            else:
                call_reads.append([int(data[0]), int(data[1])])
                readmod -= 1
                continue
    logger.info("File loaded: %s" % infile)
    global MAX_TIME
    MAX_TIME = time_stamp
    return time_stramps

def clean_data(data):
    data_len = len(data)
    writes = []
    reads = []
    deletes = []
    objects = []


    for i in range(data_len):
        time_stamp = data[i][0]
        call_writes = data[i][1]
        for j in range(len(call_writes)):
            write_id = call_writes[j][0]
            wirte_size = call_writes[j][1]
            wirte_tag = call_writes[j][2]
            wirte_info = {"time_stamp": time_stamp,"id":write_id, "size": wirte_size, "tag": wirte_tag}
            writes.append(wirte_info)
            objects.append([write_id, wirte_size, wirte_tag, time_stamp, 0 , 0 ,[]])

    for i in range(data_len):
        time_stamp = data[i][0]
        call_reads = data[i][2]
        call_dels = data[i][3]
        for j in range(len(call_reads)):
            read_id = call_reads[j][0]
            read_target = call_reads[j][1] - 1
            read_obj = objects[read_target]
            assert read_target == read_obj[0] - 1
            read_size = read_obj[1]
            read_tag = read_obj[2]
            read_info = {"time_stamp": time_stamp, "id": read_obj[0] - 1, "size": read_size, "tag": read_tag}
            objects[read_target][5] += 1
            objects[read_target][6].append(time_stamp)
            reads.append(read_info)
        for j in range(len(call_dels)):
            del_id = call_dels[j][0] - 1
            del_target = objects[del_id][0]
            del_size = objects[del_id][1]
            del_tag = objects[del_id][2]
            objects[del_id][4] = time_stamp
            del_info = {"time_stamp": time_stamp, "id": del_target, "size": del_size, "tag": del_tag}
            deletes.append(del_info)

    return_objs = []
    for obj in objects:
        obj_id = obj[0]
        obj_size = obj[1]
        obj_tag = obj[2]
        obj_write_time = obj[3]
        obj_delete_time = obj[4]
        obj_read_count = obj[5]
        obj_read_at_time = obj[6]
        if obj_delete_time == 0:
            obj_live_time = MAX_TIME - obj_write_time
        else:
            obj_live_time = obj_delete_time - obj_write_time
        obj_info = {"id": obj_id, "size": obj_size, "tag": obj_tag, "write_time": obj_write_time, "delete_time": obj_delete_time, "live_time": obj_live_time, "read_count": obj_read_count, "read_at_time": obj_read_at_time}
        return_objs.append(obj_info)

    logger.info("Data cleaned")
    return writes, reads, deletes, return_objs

def analyse_data(writes, reads, deletes, objects):
    N = len(objects)
    obj_lives = np.zeros([MAX_TAG,N])
    obj_sizes = np.zeros([MAX_TAG,N])
    obj_start_times = np.zeros([MAX_TAG,N])
    obj_delete_times = np.zeros([MAX_TAG,N])
    obj_read_counts = np.zeros([MAX_TAG,N])
    tag_read_counts = np.zeros(MAX_TAG)
    tag_counts = np.zeros(MAX_TAG)
    for i in range(N):
        obj = objects[i]
        tag = obj["tag"] - 1
        size = obj["size"]
        live_time = obj["live_time"]
        start_time = obj["write_time"]
        obj_start_times[tag][i] = start_time
        tag_read_counts[tag] += obj["read_count"]
        obj_read_counts[tag][i] = obj["read_count"]
        obj_delete_times[tag][i] = obj["delete_time"] if obj["delete_time"] > 0 else MAX_TIME
        obj_lives[tag][i] = live_time if live_time > 0 else MAX_TIME - obj["write_time"]
        obj_sizes[tag][i] = size
        tag_counts[tag] += 1
    
    # 计算每个tag的统计信息
    results = []
    for tag in range(MAX_TAG):
        count = tag_counts[tag]
        if count == 0:
            avg_live = 0.0
            avg_size = 0.0
            avg_read_count = 0.0
        else:
            # 计算平均值（自动忽略未被写入的零值）
            avg_live = obj_lives[tag].sum() / count
            avg_size = obj_sizes[tag].sum() / count
            avg_read_count = tag_read_counts[tag] / count
            all_read_counts = tag_read_counts[tag]

            avg_start_time = obj_start_times[tag].sum() / count
            avg_deead_time = obj_delete_times[tag].sum() / count
        
        # 将结果存入字典
        results.append({
            'tag': tag + 1,  # 还原为原始tag编号
            'average_live_time': avg_live,
            'average_size': avg_size,
            'count': count,
            'average_read_count': avg_read_count,
            'all_read_counts': all_read_counts,
            'average_start_time': avg_start_time,
            'average_delete_time': avg_deead_time,
            'obj_lives': obj_lives,
            'obj_sizes': obj_sizes,
            'obj_read_counts': obj_read_counts,
            'obj_start_times': obj_start_times,
            'obj_delete_times': obj_delete_times
        })

    # 打印展示结果
    print(f"{'Tag':<5} \t| {'Count':<6} \t| {'Avg Size':<10} \t| {'Avg Live':<12} \t|{'Avg Read times'}\t|{'All Read times'}\t|{'AvgStart Time'}\t|{'Avg Dead Time'}")
    print("-" * 200)
    for res in results:
        if res['count'] == 0:
            continue  # 跳过没有对象的tag
        print(
            f"{res['tag']:<5} \t| "
            f"{res['count']:6} \t|"
            f"{res['average_live_time']:10.2f} \t| "
            f"{res['average_size']:10.2f} \t| "
            f"{res['average_read_count']:10.2f} \t| "
            f"{res['all_read_counts']:10.2f} \t| "
            f"{res['average_start_time']:10.2f} \t| "
            f"{res['average_delete_time']:10.2f} \t| "
        )
    # save as .csv:
    with open('tools/results.csv', 'w', newline='') as csvfile:
        fieldnames = ['tag标记','出现次数', '平均生存时间', '平均大小', '平均读取次数', '总读取次数', '平均出生时间', '平均死亡时间']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        write_results = []
        for res in results:
            write_results.append({'tag标记': res['tag'], 
                                '出现次数': res['count'], 
                                '平均生存时间': res['average_live_time'], 
                                '平均大小': res['average_size'], 
                                '平均读取次数': res['average_read_count'],
                                '总读取次数': res['all_read_counts'], 
                                '平均出生时间': res['average_start_time'], 
                                '平均死亡时间': res['average_delete_time']})
            writer.writerows(write_results)

    return results

tag2color = {
    1: 'b',
    2: 'g',
    3: 'r',
    4: 'c',
    5: 'm',
    6: 'y',
    7: 'k',
    8: 'indigo',
    9: 'orange',
    10: 'purple',
    11: 'brown',
    12: 'pink',
    13: 'gray',
    14: 'olive',
    15: 'cyan',
    16: 'lime'
}
    
def show_obj_read_at_time(objs):
    read_at_times = []
    obj_start_times = []
    del_at_times = []
    colors = []
    tags = []
    for obj in objs:
        tag = obj['tag']
        read_at_time = obj['read_at_time']
        del_at_time = obj['delete_time'] if obj["delete_time"] > 0 else MAX_TIME
        if len(read_at_time) == 0:
            continue
        obj_start_time = obj['write_time']
        read_at_time = np.array(read_at_time)
        del_at_time = np.zeros_like(read_at_time) + del_at_time
        obj_start_time = np.zeros_like(read_at_time) + obj_start_time
        read_at_times.extend(read_at_time)
        del_at_times.extend(del_at_time)
        obj_start_times.extend(obj_start_time)
        colors.extend([tag2color[tag]] * len(read_at_time))
        tags.extend([tag] * len(read_at_time))
    read_at_times = np.array(read_at_times)
    del_at_times = np.array(del_at_times)
    obj_start_times = np.array(obj_start_times)
    colors = np.array(colors)
    tags = np.array(tags)
    tbar = tqdm(range(1, MAX_TAG + 1))
    for tag in tbar:
    # for tag in range(1, MAX_TAG + 1):
        mask = tags == tag
        fig, ax = plt.subplots(figsize=(24, 24))
        ax.scatter(obj_start_times[mask], read_at_times[mask], c=colors[mask], s=20, alpha=0.4, edgecolor='k', linewidths=0)
        ax.scatter(obj_start_times[mask], del_at_times[mask], c='r', s=30, alpha=0.8, edgecolor='k', linewidths=0)
        ax.set_title(f"Tag {tag} Object Read Time Distribution")
        ax.set_xlabel('Object Start Time')
        ax.set_ylabel('Time')
        ax.grid(True, linestyle='--', alpha=0.6)
        plt.tight_layout()
        # plt.show()
        plt.savefig(f"tools/tag_{tag}_read_at_time_distribution.png")
        plt.close()

def view_results(results):
    # 绘制每个tag的散点图
    tbar = tqdm(range(1, MAX_TAG + 1))
    for res in results:
        tbar.set_description(f"Processing tag {res['tag']}")
        tbar.update(1)
        tag = res['tag']
        obj_lives = res['obj_lives'][tag - 1]
        obj_sizes = res['obj_sizes'][tag - 1]
        obj_read_counts = res['obj_read_counts'][tag - 1]
        obj_start_times = res['obj_start_times'][tag - 1]
        obj_delete_times = res['obj_delete_times'][tag - 1]
        
        N = len(obj_lives)
        fig, ax = plt.subplots(4, 1, figsize=(16, 12))  # 适当增大画布

        mask = obj_lives > 0  # 过滤掉未被写入的对象
        range_N = np.arange(N)[mask]
        obj_lives = obj_lives[mask]
        obj_sizes = obj_sizes[mask]
        obj_read_counts = obj_read_counts[mask]
        obj_start_times = obj_start_times[mask]
        obj_delete_times = obj_delete_times[mask]
        
        # 散点图参数配置
        scatter_kwargs = {
            's': 20,          # 点的大小
            'alpha': 0.4,      # 透明度
            'edgecolor': 'k', # 边缘颜色
            'linewidths': 0 # 边缘线宽
        }

        ax[0].hist(obj_start_times, bins=100, color='skyblue', alpha=0.7)
        ax[0].set_title(f"Tag {tag} Object Start Time Distribution")
        ax[0].set_xlabel('Object Write Time')
        ax[0].set_ylabel('Count')
        ax[0].grid(True, linestyle='--', alpha=0.6)
        
        # 绘制存活时间散点图
        ax[1].scatter(obj_start_times, obj_lives, **scatter_kwargs)
        ax[1].set_title(f"Tag {tag} Object Live Time Distribution")
        ax[1].set_xlabel('Object Start Time')
        ax[1].set_ylabel('Live Time')
        ax[1].grid(True, linestyle='--', alpha=0.6)  # 添加辅助网格线
        
        # 绘制对象大小散点图
        ax[2].scatter(obj_start_times, obj_sizes, color='orange', **scatter_kwargs)
        ax[2].set_title(f"Tag {tag} Object Size Distribution")
        ax[2].set_xlabel('Object Start Time')
        ax[2].set_ylabel('Size')
        ax[2].grid(True, linestyle='--', alpha=0.6)
        
        # 绘制读取次数散点图
        ax[3].scatter(obj_start_times, obj_read_counts, color='green', **scatter_kwargs)
        ax[3].set_title(f"Tag {tag} Object Read Count Distribution")
        ax[3].set_xlabel('Object Start Time')
        ax[3].set_ylabel('Read Count')
        ax[3].grid(True, linestyle='--', alpha=0.6)
        
        plt.tight_layout()  # 自动调整子图间距
        # plt.show()
        plt.savefig(f"tools/tag_{tag}_distribution.png")
        plt.close()

def read_prediction(read_info):
    Nread = len(read_info)
    all_reads = []
    eread_info = enumerate(read_info)
    reads = {}
    read_id = []
    read_size = []
    read_tag = []
    all_Nread = enumerate(range(MAX_TIME))
    try:
        j, read = next(eread_info)
        i, _  = next(all_Nread)
        while True:
            if read['time_stamp'] == i + 1:
                read_id.append(read["id"])
                read_size.append(read["size"])
                read_tag.append(read["tag"])
                j, read = next(eread_info)
            elif read['time_stamp'] < i + 1:
                j, read = next(eread_info)
            elif read['time_stamp'] > i + 1: # 说明有时间戳没有读取, 保存后清空, 下一个时间戳开始
                reads = {"id": read_id, "size": read_size, "tag": read_tag}
                all_reads.append(reads)
                read_id = []
                read_size = []
                read_tag = []
                i, _  = next(all_Nread)
    except StopIteration:
        logger.info("Read prediction finished")
        return all_reads
    logger.error("Read prediction failed")
                
def main():
    infile = "p1\data\sample_practice.in"
    data = load_infile(infile)
    writes, reads, deletes, objects = clean_data(data)
    all_reads = read_prediction(reads)
    results = analyse_data(writes, reads, deletes, objects)
    show_obj_read_at_time(objects)
    view_results(results)


if __name__ == "__main__":
    main()