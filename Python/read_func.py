import sys
import common

def timestamp_action():
    timestamp = input().split()[1]
    # print(f"TIMESTAMP {timestamp}")
    sys.stdout.flush()


def delete_action():

    n_delete = int(input())
    abortNum = 0
    for i in range(1, n_delete + 1):
        common._id[i] = int(input())
    for i in range(1, n_delete + 1):
        delete_id = common._id[i]
        currentId = common.objects[delete_id].lastRequestPoint
        common.objects[delete_id].end_time = common.t
        while currentId != 0:
            if not common.req_is_dones[currentId]:
                abortNum += 1
            currentId = common.req_prev_ids[currentId]

    # 忽略输出
    """ 
    print(f"{abortNum}")
    for i in range(n_delete + 1):
        delete_id = _id[i]
        currentId = objects[delete_id].lastRequestPoint
        while currentId != 0:
            if not req_is_dones[currentId]:
                print(f"{currentId}")
            currentId = req_prev_ids[currentId]
        for j in range(1, REP_NUM + 1):
            do_object_delete(objects[delete_id].unit[j], disk[objects[delete_id].replica[j]], objects[delete_id].size)
        objects[delete_id].isDelete = True
    sys.stdout.flush()
    """


def write_action():
    n_write = int(input())
    for i in range(1, n_write + 1):
        write_input = input().split()
        write_id = int(write_input[0])
        size = int(write_input[1])
        tag_id = int(write_input[2])
        common.objects[write_id].lastRequestPoint = 0
        common.tag_array[tag_id].write_data[common.t] += size
        common.max_object_id = max(common.max_object_id, write_id)
        for j in range(1, common.REP_NUM + 1):
            common.objects[write_id].replica[j] = (write_id + j) % common.N_disk_num + 1
            common.objects[write_id].unit[j] = [0 for _ in range(size + 1)]
        common.objects[write_id].size = size
        common.objects[write_id].isDelete = False
        common.objects[write_id].tag_id = tag_id
        common.objects[write_id].begin_time = common.t
        common.all_write_size += size
        common.all_write_num += 1
        if tag_id != 0:
            common.objects[write_id].size_index = common.tag_array[tag_id].all_size
            if common.t >= 1 and common.t <= 1800 :
                common.tag_array[tag_id].all_size += size
                common.tag_array[tag_id].all_num += 1
            common.tag_array[tag_id].object_dict[write_id] = 1
        else:
            common.untag_write_data[common.t].append(write_id)
            if common.t > 1800:
                common.all_empty_tag_write_size += size
                common.all_empty_tag_write_num += 1
            # do_object_write(common.objects[write_id].unit[j], common.disk[common.objects[write_id].replica[j]], size, write_id)
        # 忽略输出
        """
        print(f"{write_id}")
        for j in range(1, REP_NUM + 1):
            print_next(f"{objects[write_id].replica[j]}")
            for k in range(1, size + 1):
                print_next(f" {objects[write_id].unit[j][k]}")
            print()
        """
    sys.stdout.flush()


def read_action():
    request_id = 0
    nRead = int(input())
    for i in range(1, nRead + 1):
        read_input = input().split()
        request_id = int(read_input[0])
        objectId = int(read_input[1])
        common.req_object_ids[request_id] = objectId
        common.req_prev_ids[request_id] = common.objects[objectId].lastRequestPoint
        common.objects[objectId].lastRequestPoint = request_id
        common.req_is_dones[request_id] = False
        common.all_read_size += common.objects[objectId].size
        common.objects[objectId].read_array.append(common.t)
        tag_id = common.objects[objectId].tag_id
        if tag_id != 0:
            common.tag_array[tag_id].read_data[common.t] += common.objects[objectId].size

            if objectId not in common.tag_array[tag_id].object_request_dict:
                common.tag_array[tag_id].object_request_dict[objectId] = []

            common.tag_array[tag_id].object_request_dict[objectId].append(common.t)
        else:
            common.all_empty_tag_read_size += common.objects[objectId].size

    # 忽略输出
    """
    global current_request
    global current_phase
    if current_request == 0 and nRead > 0:
        current_request = request_id
    if current_request == 0:
        for i in range(1, N + 1):
            print("#")
        print("0")
    else:
        current_phase += 1
        objectId = req_object_ids[current_request]
        for i in range(1, N + 1):
            if i == objects[objectId].replica[1]:
                if current_phase % 2 == 1:
                    print(f"j {objects[objectId].unit[1][int(current_phase / 2 + 1)]}")
                else:
                    print("r#")
            else:
                print("#")
        if current_phase == objects[objectId].size * 2:
            if objects[objectId].isDelete:
                print("0")
            else:
                print(f"1\n{current_request}")
                req_is_dones[current_request] = True
            current_request = 0
            current_phase = 0
        else:
            print("0")
    sys.stdout.flush()
    """


def garbage_action():
    garbage_message = input()
    assert garbage_message == "GARBAGE COLLECTION"


def print_next(message):
    print(f"{message}", end="")


import math



