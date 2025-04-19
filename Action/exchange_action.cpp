#include "../common.h"
using namespace std;

// 计算该块属于哪个段
int calc_segment_id(int block_id){
    int segment_id = (block_id - 1) / segment_size;
    
    return segment_id;
}

bool judge_in_select(int block_id, int disk_id){
    // 初始化一个空的select_set
    vector<int> select_set;
    for(int i = 0; i < MAGNERIC_HEAD_NUM; i++){
        for(int j = 0; j < disk_array[disk_id].target_actual_array[i].size(); j++){
            select_set.push_back(disk_array[disk_id].target_actual_array[i][j]);
        }
    }
    if(block_id == 363)
        int a = 1;

    for(int i = 0; i < select_set.size(); i++){
        int actual_segment_id = select_set[i];
        ActualSegment& actual_segment = disk_array[disk_id].segment_array[actual_segment_id];
        // 判断该块是否在实际段中
        if(block_id >= actual_segment.begin_index && block_id < actual_segment.begin_index + actual_segment.segment_length)
            return true;
    }
    return false;
}

int find_empty_block(int disk_id, int block_id, int end_block){
    for(int i = block_id; i < end_block; i++){
        if(disk[disk_id][i] == 0)
            return i;
    }
    return -1;
}

int find_request_block(int disk_id, int block_id, int end_block, int tag_id){
    for(int i = block_id; i <= end_block; i++){
        if(disk[disk_id][i] == 0)
            continue; // 该块未被分配或者不在实际段中
        Object& obj = object_array[disk[disk_id][i]];
        if(obj.tag == tag_id)
            return i;
    }
    return -1;
}

int find_request_block1(int disk_id, int block_id, int start_block, int tag_id){
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
        if(obj.storge_data[rep_id][i] == request_id){
            int temp1 = disk_block_index[disk_id][request_id];
            disk_block_index[disk_id][request_id] = disk_block_index[disk_id][empty_id];
            disk_block_index[disk_id][empty_id] = temp1;
            int temp2 = disk_block_request[disk_id][request_id];
            disk_block_request[disk_id][request_id] = disk_block_request[disk_id][empty_id];
            disk_block_request[disk_id][empty_id] = temp2;
            obj.storge_data[rep_id][i] = empty_id;
            disk[disk_id][empty_id] = disk[disk_id][request_id];
            disk[disk_id][request_id] = 0;
            break;
        }
    }
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
    std::ostringstream oss;
    if(time_step % 1800 != 0)
        return;
    
    //写入一个字符串, 并输出该字符串
    if(global_turn == 1){
        string s1 = "", s2 = "";
        cin >> s1 >> s2;
        cout << s1 << " " << s2 << endl;
    }
    else if(global_turn == 2 && use_round2_reoutput){
        oss << "GARBAGE COLLECTION" << endl;
    }else{
        cout << "GARBAGE COLLECTION" << endl;
    }

    vector<int> exchange_disk_array; 
    ExchangeBlock temp = {};
    vector<ExchangeBlock> exchange_block_array; // 交换块的数组
    for(int i = 0; i <= N_disk_num; i++){
        if(global_turn == 1){
            exchange_disk_array.push_back(K_max_exchange_block);

        }else{
            exchange_disk_array.push_back(K2);

        }
        exchange_block_array.push_back(temp);
    }
    for(int n = 1; n <= N_disk_num; n++){
        vector<int> select_actual;
        Disk& target_disk = disk_array[n];
        for(int i = 0; i < MAGNERIC_HEAD_NUM; i++){
            for(int j = 0; j < target_disk.target_actual_array[i].size(); j++){
                int actual_segment_id = target_disk.target_actual_array[i][j];
                select_actual.push_back(actual_segment_id);
            }
        }
        for(int i = 1; i < segment_num; i++){
            if(find(select_actual.begin(), select_actual.end(), i) == select_actual.end()){
                select_actual.push_back(i);
            }
        }
        for(int i = 0; i < select_actual.size(); i++){
            int actual_segment_id = select_actual[i];
            ActualSegment& actual_segment = target_disk.segment_array[actual_segment_id];
            
            int empty_size = actual_segment.get_empty() - (segment_size - actual_segment.first_write_index);
            int block_empty = actual_segment.begin_index, block_request = advance_position(actual_segment.begin_index, actual_segment.segment_length - 1);
            block_empty = find_empty_block(n, block_empty, block_request);
            block_request = find_request_block1(n, block_request, block_empty, actual_segment.tag_index);

            if(block_empty == -1 || block_request == -1 || block_empty > block_request){
                actual_segment.first_write_index = find_first_wirte(actual_segment);
                continue; // 该块未被分配或者不在实际段中
            }
            while(true){
                Object& target_object = object_array[disk[n][block_request]];
                if(target_object.size > empty_size || target_object.size > exchange_disk_array[n] || target_object.size == 0)
                    break; // 该段空间不足，无法交换
                int judge = true;
                vector<int> empty, request;
                for(int k = 0; k < target_object.size; k++){
                    empty.push_back(block_empty);
                    request.push_back(target_object.storge_data[1][k]);
                    block_empty++;
                    block_empty = find_empty_block(n, block_empty, block_request);
                    if(block_empty == -1 || block_empty > target_object.storge_data[1][0]){
                        judge = false;
                        break; // 该块未被分配或者不在实际段中
                    }
                }
                if(judge == false)
                    break; // 该段空间不足，无法交换
                block_request = target_object.storge_data[1][0] - 1;
                for(int k = 0; k < target_object.size; k++){
                    change_object_storge(target_object, empty[k], request[k], 1, n);
                    exchange_block_array[n].exchange_block.push_back({empty[k], request[k]});
                    exchange_disk_array[n]--;
                }
                block_request = find_request_block1(n, block_request, block_empty, target_object.tag);
            }
            actual_segment.first_write_index = find_first_wirte(actual_segment);
        }
    }
    

    // // 对每个磁盘的实际段进行交换
    // for(int n = 1; n <= N_disk_num; n++){
    //     // 对该磁盘选中的实际段进行回收
    //     vector<int> select_id;
    //     Disk& target_disk = disk_array[n];
    //     for(int j = 0; j < target_disk.target_actual_array.size(); j++){
    //         for(int k = 0; k < target_disk.target_actual_array[j].size(); k++){
    //             int actual_segment_id = target_disk.target_actual_array[j][k];
    //             select_id.push_back(actual_segment_id);
    //         }
    //     }
    //     for(int n1 = 0; n1 < select_id.size(); n1++){
    //         // 优先级1: 将该实际段的空块与其他实际段中属于该tag的块进行交换
    //         ActualSegment& target_actual_segment = target_disk.segment_array[select_id[n1]];
    //         if(target_actual_segment.get_empty() == 0)
    //             continue; // 该段无需回收
    //         int segment_empty = select_id[n1], segment_request = -1;
    //         int empty_id = target_actual_segment.begin_index, request_id = 1;
    //         int end_id = advance_position(empty_id, target_actual_segment.segment_length - 1);
    //         int actual_end = segment_num * segment_size;
    //         empty_id = find_empty_block(n, empty_id, end_id);
    //         request_id = find_request_block(n, request_id, actual_end, target_actual_segment.tag_index);
    //         if(empty_id == -1 || request_id == -1){
    //             target_actual_segment.first_write_index = find_first_wirte(target_actual_segment);
    //             continue; // 该块未被分配或者不在实际段中
    //         }
    //         while(true){
    //             Object& target_object = object_array[disk[n][request_id]];
    //             if(target_object.size > target_actual_segment.get_empty() || target_object.size > exchange_disk_array[n] || target_object.size == 0){
    //                 break; // 该段空间不足，无法交换
    //             }
    //             vector<int> empty, request;
    //             int judge = true;
    //             for(int k = 0; k < target_object.size; k++){
    //                 empty.push_back(empty_id);
    //                 request.push_back(target_object.storge_data[1][k]);
    //                 empty_id++;
    //                 empty_id = find_empty_block(n, empty_id, end_id);
    //             }
    //             segment_request = calc_segment_id(target_object.storge_data[1][0]);
    //             request_id = target_object.storge_data[1][target_object.size - 1] + 1;
    //             request_id = find_request_block(n, request_id, actual_end, target_object.tag);
    //             // 交换块并更新磁盘块信息
    //             change_object_storge(target_object, empty, request, n);
    //             for(int k = 0; k < target_object.size; k++){
    //                 exchange_block_array[n].exchange_block.push_back({empty[k], request[k]});
    //                 exchange_disk_array[n]--;
    //             }
    //             if(request_id == -1 || empty_id == -1)
    //                 break; // 该块未被分配或者不在实际段中
    //         }
    //         target_actual_segment.first_write_index = find_first_wirte(target_actual_segment);
    //     }
    // }

    for(int n = 1; n <= N_disk_num; n++){
        // cout << "0" << endl;

        int size = exchange_block_array[n].exchange_block.size();
        if(global_turn == 2 && use_round2_reoutput){
            oss << size << endl;
        }else{
            cout << size << endl;
        }
        
        for(int i = 0; i < size; i++){
            if(global_turn == 2 && use_round2_reoutput){
                oss << exchange_block_array[n].exchange_block[i].first << " " << exchange_block_array[n].exchange_block[i].second << endl;
            }else{
                cout << exchange_block_array[n].exchange_block[i].first << " " << exchange_block_array[n].exchange_block[i].second << endl;
            }
        }
    }
    
    if(global_turn == 2 && use_round2_reoutput){
        round2_recycle_track[t] = oss.str();
    }
    cout.flush();
    return;
}