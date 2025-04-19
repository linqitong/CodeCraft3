import sys
from draw import *
import os
from read_func import *
import common
from tqdm import tqdm
import math
import get_tag_rank

def main():
    original_stdin = sys.stdin
    # f = open('../Data/初赛数据/practice.in', 'r')
    f = open('../Data/sample_official_1.in', 'r')
    sys.stdin = f

    user_input = input().split()

    common.T_time_step_length = int(user_input[0])
    common.M_tag_num = int(user_input[1])
    common.N_disk_num = int(user_input[2])
    common.V_block_per_disk = int(user_input[3])
    common.G_token_per_time_step = int(user_input[4])
    common.K_garbage_recycle_size = int(user_input[5])
    common.K2 = int(user_input[6])

    common.tag_array = [common.Tag() for _ in range(common.M_tag_num + 1)]
    common.untag_write_data = [[] for _ in range(common.T_time_step_length + EXTRA_TIME + 1)]


    all_size_all = []
    fre_del_all = []
    fre_write_all = []
    fre_net_demand_all = []
    fre_read_all = []


    # 删除情况
    """
    init = True
    for n1 in range(1, common.M_tag_num + 1):
        user_input = input().split()
        all_size = 0
        for n2 in range(len(user_input)):
            if init:
                init = False
                all_size_all = [0 for _ in range(len(user_input))]
                fre_del_all = [0 for _ in range(len(user_input))]
                fre_write_all = [0 for _ in range(len(user_input))]
                fre_net_demand_all = [0 for _ in range(len(user_input))]
                fre_read_all = [0 for _ in range(len(user_input))]
                common.tag_array = [common.Tag() for _ in range(common.M_tag_num + 1)]
                common.tag_array[n1].read_data = [0 for _ in range(common.T_time_step_length + common.EXTRA_TIME + 1)]


            common.tag_array[n1].fre_del.append(int(user_input[n2]))
            common.tag_array[n1].fre_net_demand.append(-int(user_input[n2]))
            all_size -= int(user_input[n2])
            common.tag_array[n1].fre_all_size.append(all_size)
            all_size_all[n2] += int(all_size)
            fre_del_all[n2] += int(user_input[n2])
            fre_net_demand_all[n2] -= int(user_input[n2])

    # 写入情况
    for n1 in range(1, common.M_tag_num + 1):
        user_input = input().split()
        all_size = 0
        for n2 in range(len(user_input)):
            common.tag_array[n1].fre_write.append(int(user_input[n2]))
            common.tag_array[n1].fre_net_demand[n2] += int(user_input[n2])
            all_size += int(user_input[n2])
            common.tag_array[n1].fre_all_size[n2] += int(all_size)
            all_size_all[n2] += all_size
            fre_write_all[n2] += int(user_input[n2])
            fre_net_demand_all[n2] += int(user_input[n2])

    threshold = 10000
    threshold_read_all = 0
    read_all = 0
    size_array = [0 for _ in range(len(user_input))]
    # 读取情况
    for n1 in range(1, common.M_tag_num + 1):
        user_input = input().split()
        for n2 in range(len(user_input)):
            common.tag_array[n1].fre_read.append(int(user_input[n2]))
            fre_read_all[n2] += int(user_input[n2])
            read_all += int(user_input[n2])
            if int(user_input[n2]) > threshold:
                threshold_read_all += int(user_input[n2])
                size_array[n2] += common.tag_array[n1].fre_all_size[n2]
    get_tag_rank.get_tag_rank()
    """

    
    # extra_token = input()

    draw_pic = False
    if draw_pic:
        # 绘制 tag 图像
        for n1 in range(1, common.M_tag_num + 1):
            data = [common.tag_array[n1].fre_del, common.tag_array[n1].fre_write, common.tag_array[n1].fre_net_demand]
            name = ["fre_del", "fre_write", "fre_net_demand"]
            save_multi_line_plot(data, os.path.join(picture_addr, "tag_dwn"), "tag" + str(n1), name)
            data = common.tag_array[n1].fre_read
            save_line_plot(data, os.path.join(picture_addr, "tag_read"), "tag" + str(n1))
            data = common.tag_array[n1].fre_all_size
            save_line_plot(data, os.path.join(picture_addr, "tag_all_size"), "tag" + str(n1))

        data = [fre_del_all, fre_write_all, fre_net_demand_all]
        name = ["fre_del_all", "fre_write_all", "fre_net_demand_all"]
        save_multi_line_plot(data, os.path.join(picture_addr), "all_tag_dwn", name)
        data = all_size_all
        save_line_plot(data, os.path.join(picture_addr), "all_tag_size")
        data = fre_read_all
        save_line_plot(data, os.path.join(picture_addr), "all_tag_read")

    # 只考虑第一次循环
    for time in tqdm(range(1, common.T_time_step_length + common.EXTRA_TIME + 1)):
        common.t_segment = math.floor(time / 1800)
        # if (common.t % 1800) == 0:
            # print(common.t)
        common.t = time
        timestamp_action()
        delete_action()
        write_action()
        read_action()
        if (common.t % 1800) == 0:
            garbage_action()

    print("输入数据已经处理完毕")
    """
    tag_object_read_data = [[] for _ in range(17)]
    for n1 in range(1, 17):
        for o_id, value in common.tag_array[n1].object_request_dict.items():
            tag_object_read_data[n1].append(len(value))
        tag_object_read_data[n1].sort(reverse=True)
        save_line_plot(tag_object_read_data[n1], os.path.join(picture_addr, "tag_object_read"),
                       "tag" + str(n1))
    """
    no_tag_data = [[] for _ in range(17)]
    all_no_tag_data = [0 for _ in range(common.T_time_step_length + common.EXTRA_TIME + 1)]
    have_tag_data = [[] for _ in range(17)]
    for n1 in range(1, 17):
        have_tag_data[n1] = [0 for _ in range(common.T_time_step_length + common.EXTRA_TIME + 1)]
        no_tag_data[n1] = [0 for _ in range(common.T_time_step_length + common.EXTRA_TIME + 1)]
    for n1 in tqdm(range(0, common.max_object_id)):
        input_data = input().split()
        object_id = int(input_data[0])
        tag_id = int(input_data[1])
        common.objects[object_id].tag_id = tag_id
        for n2 in range(len(common.objects[object_id].read_array)):
            all_no_tag_data[common.objects[object_id].read_array[n2]] += common.objects[object_id].size
        # common.tag_array[tag_id].append((common.objects[object_id].begin_time, object_id, 0))
        if object_id not in common.tag_array[tag_id].object_dict:
            no_tag_data[tag_id][common.objects[object_id].begin_time] += common.objects[object_id].size
        else:
            have_tag_data[tag_id][common.objects[object_id].begin_time] += common.objects[object_id].size
    save_line_plot(all_no_tag_data, picture_addr,
                       "all_no_tag_write", 50)
    
    for n2 in range(1, 17):
        save_line_plot(no_tag_data[n2], os.path.join(picture_addr, "no_tag_write"),
                       "tag" + str(n2), 50)
        save_line_plot(have_tag_data[n2], os.path.join(picture_addr, "have_tag_write"),
                       "tag" + str(n2), 50)
    
    exit()
    data33 = [0 for _ in range(17)]
    # for i in range(1, 17):
        # data33[i] = common.tag_array[i].all_size / common.tag_array[i].all_num
    f = open('../Data/sample_practice_map_1.txt', 'r')
    sys.stdin = f
    have_tag_data = [[] for _ in range(17)]
    no_tag_data = [[] for _ in range(17)]
    all_no_tag_data = [0 for _ in range(common.T_time_step_length + common.EXTRA_TIME + 1)]
    objs = []
    csv_data = {"timestamp_id": [], "request_id": [], "label": [], "read_label": []}
    for n1 in range(1, 17):
        have_tag_data[n1] = [0 for _ in range(common.T_time_step_length + common.EXTRA_TIME + 1)]
        no_tag_data[n1] = [0 for _ in range(common.T_time_step_length + common.EXTRA_TIME + 1)]
    for n1 in tqdm(range(0, common.max_object_id)):
        input_data = input().split()
        object_id = int(input_data[0])
        tag_id = int(input_data[1])
        common.objects[object_id].tag_id = tag_id
        for n2 in range(len(common.objects[object_id].read_array)):
            all_no_tag_data[common.objects[object_id].read_array[n2]] += common.objects[object_id].size
        # common.tag_array[tag_id].append((common.objects[object_id].begin_time, object_id, 0))
        if object_id not in common.tag_array[tag_id].object_dict:
            common.tag_array[tag_id].objs.append((common.objects[object_id].begin_time, object_id, 0))
            no_tag_data[tag_id][common.objects[object_id].begin_time] += common.objects[object_id].size
            objs.append((common.objects[object_id].begin_time, tag_id, 0, object_id))
            csv_data["timestamp_id"].append(common.objects[object_id].begin_time)
            csv_data["request_id"].append(object_id)
            csv_data["label"].append(0)
            csv_data["read_label"].append(tag_id)
        else:
            have_tag_data[tag_id][common.objects[object_id].begin_time] += common.objects[object_id].size
            common.tag_array[tag_id].objs.append((common.objects[object_id].begin_time, object_id, 1))
            objs.append((common.objects[object_id].begin_time, tag_id, 1, object_id))
            # common.tag_array[tag_id].untag_object[object_id] = 1
            csv_data["timestamp_id"].append(common.objects[object_id].begin_time)
            csv_data["request_id"].append(object_id)
            csv_data["label"].append(tag_id)
            csv_data["read_label"].append(tag_id)
    import pandas as pd
    df = pd.DataFrame(csv_data)

    # 保存为 CSV
    df.to_csv("data.csv", index=False)  # index=False 表示不保存行索引
    exit()
    """save_line_plot(all_no_tag_data, picture_addr,
                   "all_no_tag_write", 50)"""
    """
    for n2 in range(1, 17):
        save_line_plot(no_tag_data[n2], os.path.join(picture_addr, "no_tag_write"),
                       "tag" + str(n2), 50)
        save_line_plot(have_tag_data[n2], os.path.join(picture_addr, "have_tag_write"),
                       "tag" + str(n2), 50)
    """
    objs.sort()
    for n1 in range(1, 17):
        common.tag_array[n1].objs.sort()
    data = {}
    for tag_id in range(1, common.M_tag_num + 1):
        data[tag_id] = (len(common.tag_array[tag_id].object_dict), len(common.tag_array[tag_id].untag_object))

    p_data = {}

    for index, value in data.items():
        p_data[index] = (value, value[0] / value[1])

    # exit()
    for time in tqdm(range(1, common.T_time_step_length + common.EXTRA_TIME + 1)):
        for index in range(len(common.untag_write_data[time])):
            tag_id = common.objects[common.untag_write_data[time][index]].tag_id
            common.tag_array[tag_id].untag_write_data[time] += common.objects[common.untag_write_data[time][index]].size

    step_length = 400
    for n1 in range(1, common.M_tag_num + 1):
        data = common.tag_array[n1].untag_write_data[1:]
        save_line_plot(data, os.path.join(picture_addr, "untag_write"), n1, step_length)
    exit()
    # 绘制每个标签的 read 数据
    step_length = 400
    for n1 in range(1, common.M_tag_num + 1):
        data = common.tag_array[n1].write_data[1:]
        save_line_plot(data, os.path.join(picture_addr, "tag_write"), n1, step_length)
    exit()

    # 绘制每个标签的 read 数据
    step_length = 50
    for n1 in range(1, common.M_tag_num + 1):
        data = common.tag_array[n1].read_data[1:]
        save_line_plot(data, os.path.join(picture_addr, "tag_read"), n1, step_length)

    exit()

    data1 = []
    for key, value in common.tag_array[1].object_request_dict.items():
        data1.append(value)
    draw_scatter_plot(data1, "./", "test", (0, 1000), (0, 50))

    # exit()
    data = []
    map = {}
    n1 = 1
    for key, value in sorted(common.tag_array[1].object_request_dict.items()):
        data.append(value)
        map[key] = n1
        n1 += 1

    data2 = []
    for key, value in common.tag_array[1].object_request_dict.items():
        data2.append([map[key], common.objects[key].size_index, value])
    plot_elements_3(data2, "./", "test")
    exit()
    draw_scatter_plot(data, "./", "test")
    exit()

    for n1 in range(1, common.M_tag_num + 1):
        data = []
        for value in common.tag_array[n1].object_request_dict.values():
            data.append(value)
        draw_scatter_plot(data, os.path.join(picture_addr, "object_read"), "tag" + str(n1))

    exit()
    tag_array = common.tag_array
    for tag_id in range(1, common.M_tag_num + 1):
        save_line_plot(tag_array[tag_id].read_data, os.path.join(picture_addr, "tag_read_all"), "tag" + str(tag_id))
    a = 1


if __name__ == '__main__':
    main()