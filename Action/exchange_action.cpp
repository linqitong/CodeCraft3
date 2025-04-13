#include "../common.h"
using namespace std;

int find_empty_block(int disk_id, int block_id, int end_block){
    for(int i = block_id; i < end_block; i++){
        if(disk[disk_id][i] == 0)
            return i;
    }

    return -1;
}

int find_request_block(int disk_id, int block_id, int start_block, int tag_id){
    for(int i = block_id; i > start_block; i--){
        if(disk[disk_id][i] == 0)
            continue; // 该块未被分配或者不在实际段中
        Object& obj = object_array[disk[disk_id][i]];
        if(obj.tag == tag_id)
            return i;
    }

    return -1;
}

int change_object_storge(Object& obj, int empty_id, int request_id, int rep_id, int disk_id){
    for(int i = 0; i < obj.size; i++){
        if(obj.storge_data[rep_id].object_storge[i] == request_id){
            int temp1 = disk_block_index[disk_id][request_id];
            disk_block_index[disk_id][request_id] = disk_block_index[disk_id][empty_id];
            disk_block_index[disk_id][empty_id] = temp1;
            int temp2 = disk_block_request[disk_id][request_id];
            disk_block_request[disk_id][request_id] = disk_block_request[disk_id][empty_id];
            disk_block_request[disk_id][empty_id] = temp2;
            obj.storge_data[rep_id].object_storge[i] = empty_id;
            disk[disk_id][empty_id] = disk[disk_id][request_id];
            disk[disk_id][request_id] = 0;
            break;
        }
    }
}

int calc_exchange_K(VirtualSegment& target_virtual_segment, vector<int>& exchange_disk_array){
    int size = K_max_exchange_block;
    for(int i = 0; i < 3; i++){
        int disk_id = target_virtual_segment.actual_disk[i];
        if(exchange_disk_array[disk_id] < size)
            size = exchange_disk_array[disk_id];
    }

    return size;
}

int find_first_wirte(ActualSegment& target_actual_segment){
    int seg = target_actual_segment.segment_length, begin = target_actual_segment.begin_index, disk_id = target_actual_segment.disk_id;
    for(int i = seg + begin - 1; i >= begin; i--){
        if(disk[disk_id][i] == 0)
            continue;
        if(i + 1 == seg + begin){
            return seg; // 该段未被分配
        }
        return i + 1 - begin;    
    }
    return 0;
}

void exchange_action()
{
    if(time_step % 1800 != 0)
        return;
    
    //写入一个字符串, 并输出该字符串
    string s1 = "", s2 = "";
    cin >> s1 >> s2;
    cout << s1 << " " << s2 << endl;

    vector<int> exchange_disk_array; 
    ExchangeBlock temp = {};
    vector<ExchangeBlock> exchange_block_array; // 交换块的数组
    for(int i = 0; i <= N_disk_num; i++){
        exchange_disk_array.push_back(K_max_exchange_block);
        exchange_block_array.push_back(temp);
    }
    
    // 从已选择的虚拟段中进行交换
    for(auto i = select_VirtualSegment.begin(); i != select_VirtualSegment.end(); i++){
        int virtual_segment_id = *i;
        VirtualSegment& target_virtual_segment = virtual_segment_array[virtual_segment_id];
        
        int exchange_size = calc_exchange_K(target_virtual_segment, exchange_disk_array);

        // 对虚拟段的三个实际段进行回收
        for(int j = 0; j < 3; j++){
            int have_change_size = 0;
            int disk_id = target_virtual_segment.actual_disk[j];
            ActualSegment& target_actual_segment = disk_array[disk_id].segment_array[target_virtual_segment.actual_index[j]];
            int empty_size = target_actual_segment.get_empty() - (segment_size -target_actual_segment.first_write_index);
            int block_empty_id = target_actual_segment.begin_index, block_request_id = advance_position(target_actual_segment.begin_index, target_actual_segment.segment_length - 1);
            int tag_id = target_virtual_segment.tag_index;
            block_empty_id = find_empty_block(disk_id, block_empty_id, block_request_id), block_request_id = find_request_block(disk_id, block_request_id, block_empty_id, tag_id);
            if(block_empty_id == -1 || block_request_id == -1 || block_empty_id > block_request_id){
                target_actual_segment.first_write_index = find_first_wirte(target_actual_segment);
                continue; // 该块未被分配或者不在实际段中
            }
            while(true){
                Object& target_object = object_array[disk[disk_id][block_request_id]];
                if(target_object.size >= empty_size || target_object.size > exchange_size || target_object.size == 0){
                    break; // 该段空间不足，无法交换
                }
                if(exchange_disk_array[disk_id] == 0 || have_change_size + target_object.size > exchange_size)
                    break; // 该磁盘交换块数目已达到上限
                vector<int> empty, request;
                int judge = true;
                for(int k = 0; k < target_object.size; k++){
                    empty.push_back(block_empty_id);
                    request.push_back(target_object.storge_data[j + 1].object_storge[k]);
                    block_empty_id++;
                    block_empty_id = find_empty_block(disk_id, block_empty_id, block_request_id);
                    if(block_empty_id == -1 || block_empty_id > target_object.storge_data[j + 1].object_storge[0]){
                        judge = false;
                        break; // 该块未被分配或者不在实际段中
                    }
                }
                if(judge == false)
                    break; // 该段空间不足，无法交换
                block_request_id = target_object.storge_data[j + 1].object_storge[0] - 1;

                // 交换块并更新磁盘块信息
                for(int k = 0; k < target_object.size; k++){
                    change_object_storge(target_object, empty[k], request[k], j + 1, disk_id);
                    exchange_block_array[disk_id].exchange_block.push_back({empty[k], request[k]});
                    exchange_disk_array[disk_id]--;
                }
                have_change_size += target_object.size;
                empty_size -= target_object.size;
            }
            target_actual_segment.first_write_index = find_first_wirte(target_actual_segment);
        }
    }

    for(int n = 1; n <= N_disk_num; n++){
        // cout << "0" << endl;

        int size = exchange_block_array[n].exchange_block.size();
        cout << size << endl;
        for(int i = 0; i < size; i++){
            cout << exchange_block_array[n].exchange_block[i].first << " " << exchange_block_array[n].exchange_block[i].second << endl;
        }
    }
    
    cout.flush();
    return;
}