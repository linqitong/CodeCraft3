#include "../common.h"
using namespace std;

bool block_need_read2(int block_id, int disk_id){
    int obj_id=disk[disk_id][block_id];
    if(obj_id == 0){
        return false;
    }
    int obj_block_num=0;// 表示对象块处于对象第几块
    Object &obj=object_array[obj_id];

    int copy_id=1;
    for(;copy_id<=3;copy_id++){//获得该磁盘存储的对象副本id
        if(obj.disk_array[copy_id]==disk_id) break;
    }
    
    while(1){
        if(obj.storge_data[copy_id][obj_block_num] == block_id)
            break;
        obj_block_num++;
    }

    for(int req_id:object_array[obj_id].wait_request_set){
        if(request_array[req_id].read[obj_block_num]==false) 
            return true; 
    }
    return false;
}

int pass_length(int have_read_time){
    if(have_read_time >= 7){
        return 9;
    }else if(have_read_time == 6){
        return 9;
    }else if(have_read_time == 5){
        return 8;
    }else if(have_read_time == 4){
        return 7;
    }else if(have_read_time == 3){
        return 6;
    }else if(have_read_time == 2){
        return 5;
    }else if(have_read_time == 1){
        return 2;
    }else{
        return 0;
    }
}

void read_action(){
    
    int n_read;
    int request_id, object_id;
    if(global_turn==1)
        scanf("%d", &n_read);
    else
    n_read=read_record[time_step].size();
    for (int i = 1; i <= n_read; i++) { // 总用时12s+
        if(global_turn==1){
            scanf("%d%d", &request_id, &object_id);
            max_request_id = max(max_request_id, request_id);
            request_array[request_id].object_id = object_id;
            request_array[request_id].request_id = request_id;
            request_array[request_id].recieve_time = time_step;
            read_record[time_step].push_back(request_id);
            if(object_array[object_id].true_tag){
                tag_read[object_array[object_id].tag][time_step/pearson_sample_interval]+=object_array[object_id].size;
            }else{
                obj_read_data[object_id][time_step/pearson_sample_interval]+=object_array[object_id].size;
            }
        }
        else{
            request_id=read_record[time_step][i-1];
            request_array[request_id].recieve_time = time_step;
            object_id=request_array[request_id].object_id;
        }
        
        int tag=object_array[object_id].tag;
        tag_array[tag].read_size[time_step]+=object_array[object_id].size;
        //查找是否在磁盘读取区域中
        bool if_need_read=false;

        if(object_array[object_id].quit == false){
            int disk_id = object_array[object_id].disk_array[1];
            set<int> obj_segment_id;
            for(int i=0;i<object_array[object_id].storge_data[1].size();i++){
                int actualSegment_id = (object_array[object_id].storge_data[1][i] - 1) / segment_size;
                obj_segment_id.insert(actualSegment_id);
                assert(actualSegment_id<segment_num);
            } 
            if(obj_segment_id.size()==2){
                int a=1;
            }
            for(int magnetic_head_id=0; magnetic_head_id<MAGNERIC_HEAD_NUM; magnetic_head_id++){
                bool in_need_read = true; 
                
                for(auto it=obj_segment_id.begin();it!=obj_segment_id.end();it++){
                    in_need_read = in_need_read & disk_array[disk_id].is_target_actual(*it, magnetic_head_id); 
                    
                }
            
                if_need_read = if_need_read | in_need_read;
            }
        }

            
        if(!if_need_read ) {
            un_selected_r++;
            drop_req_num++;
            busy_req.push_back(request_id);
            continue;
        }
        // 将请求添加进对象的等待请求集合
        object_array[object_id].wait_request_set.insert(request_id);
        request_per_time[time_step].push_back(request_id);
        all_request_read_size += object_array[object_id].size;

        Object& target_object = object_array[object_id];
        for(int n1 = 1; n1 <= 1; n1++){
            int disk_id = target_object.disk_array[n1];
            int actualSegment_id = (target_object.storge_data[n1][0] - 1) / segment_size;

            disk_array[disk_id].segment_array[actualSegment_id].request_size += target_object.size;

            for(int n2 = 0; n2 < target_object.size; n2++){
                int block_id = target_object.storge_data[n1][n2];
                disk_block_request[disk_id][block_id]++;
            }
        }
        selected_r++;
    }
        

    for(int disk_id = 1; disk_id < MAX_DISK_NUM; disk_id++){
        Disk& target_disk = disk_array[disk_id];

        for(int seg_id=0;seg_id<target_disk.segment_array.size();seg_id++){
            ActualSegment& segment=target_disk.segment_array[seg_id];

            segment.all_request_wait_time+=segment.get_request_num();
        }
    }

    for(int disk_id = 1; disk_id < MAX_DISK_NUM; disk_id++){
        Disk& target_disk = disk_array[disk_id];

        for(int magnetic_head_id=0;magnetic_head_id<MAGNERIC_HEAD_NUM;magnetic_head_id++){
            
            int& magnetic_head = target_disk.magnetic_head[magnetic_head_id];
            int& have_read_time = target_disk.have_read_time[magnetic_head_id];
            int& current_token = target_disk.current_G_token[magnetic_head_id];
            string& target_order = target_disk.order[magnetic_head_id];
            
            
                while(true){
                    // 计算当前实际段的 id 
                    int segment_index = (magnetic_head - 1) / segment_size;
                    ActualSegment& this_segment = target_disk.segment_array[segment_index];
                    
                    int last_segment_index = (segment_index - 1 + target_disk.segment_array.size()) % target_disk.segment_array.size();
                    ActualSegment& last_segment = target_disk.segment_array[last_segment_index];
                    int last_end_index = advance_position(last_segment.begin_index, last_segment.segment_length);
                  
                    int in_need_read = target_disk.is_target_actual(segment_index, magnetic_head_id); // 判定当前是否在需要读的段中

                    int have_r = true;
                    int target_block;
                    if(in_need_read){
                        // 计算当前所在连续时间段范围
                        int end_index = advance_position(this_segment.begin_index, this_segment.segment_length);
                        int end_segment_index = segment_index;
                        while(target_disk.is_target_actual((end_index - 1) / segment_size, magnetic_head_id)){
                            end_segment_index = (end_segment_index + 1)  % target_disk.segment_array.size();
                            ActualSegment& end_segment = target_disk.segment_array[end_segment_index];
                            end_index = advance_position(end_index, end_segment.segment_length);
                        }
                        have_r = false;

                        for(int n1 = magnetic_head; n1 < end_index; n1++){
                            if(disk_block_request[disk_id][n1] > 0){
                                have_r = true;
                                target_block = n1;
                                break;
                            }
                        }
                    }

                    if((!in_need_read)
                        || (!have_r)){
                            int jump_index;
                        
                            vector<pair<int, int>> array;
                            for(int n2 = 0; n2 < target_disk.target_actual_array[magnetic_head_id].size(); n2++){
                                int actualSegment_id = target_disk.target_actual_array[magnetic_head_id][n2];
                                ActualSegment& actualSegment = target_disk.segment_array[actualSegment_id];
                                double mark=actualSegment.get_request_num()*exp(((double)actualSegment.all_request_wait_time/actualSegment.get_request_num()/30.0));
                                //mark=actualSegment.get_request_num();
                                //mark=actualSegment.all_request_wait_time;
                                //mark=actualSegment.get_request_num();
                                array.push_back({mark, actualSegment_id});
                                
                            }
                            if(array.size()==0 ) break;
                            sort(array.begin(), array.end(), greater<pair<int, int>>());
                            
                            int compare_id = segment_index;
                            if(last_end_index == magnetic_head) compare_id = last_segment_index;
                            int actualSegment_id = -1;
                            for(int n2 = 0; n2 < target_disk.target_actual_array[magnetic_head_id].size(); n2++){
                                int actualSegment_id2 = target_disk.target_actual_array[magnetic_head_id][n2];
                                ActualSegment& actualSegment = target_disk.segment_array[actualSegment_id2];
                                if(compare_id != actualSegment_id2 && target_disk.target_actual_array[magnetic_head_id].size() == 2){
                                    actualSegment_id = actualSegment_id2;
                                }else if(target_disk.target_actual_array[magnetic_head_id].size() != 2){
                                    actualSegment_id = actualSegment_id2;
                                }
                            }
                           
                            if(actualSegment_id == -1) break;
                            int actualSegment_index=0;
                            
                            actualSegment_id=array[0].second;
                            for(int i=0;i<target_disk.target_actual_array[magnetic_head_id].size();i++){
                                if(target_disk.target_actual_array[magnetic_head_id][i]==actualSegment_id){
                                    actualSegment_index=i;
                                    break;
                                }
                            }
                          
                            ActualSegment& actualSegment = target_disk.segment_array[actualSegment_id];

                            jump_index = actualSegment.begin_index;
                            bool have_read = false;
                            for(; jump_index < actualSegment.begin_index + actualSegment.segment_length;){
                                if(block_need_read2(jump_index, disk_id)){
                                    have_read = true;
                                    break;
                                }
                                jump_index = advance_position(jump_index, 1);
                            }
                            if(!have_read){
                                jump_index = actualSegment.begin_index;
                            }
                            int length = calculate_distance(magnetic_head, jump_index);
                            if(length == 0){
                                break;
                            }
                            // 判定当前磁盘能否使用 pass 前往目标位置
                            if(length <= G_token_per_time_step - current_token){
                                // 可以 pass
                                for(int n2 = 0; n2 < length; n2++){
                                    target_order += "p";
                                }
                                target_disk.head_advance(length, "pass",magnetic_head_id);
                                have_read_time = 0;
                                current_token += length;
                                target_disk.current_time_segment[magnetic_head_id] = time_segment_index;
                                continue;
                            }else{
                                // 当前帧不能使用 pass 前往目标位置
                                // bool mast_jump;
                                if(current_token != 0){ // 当前帧不能跳转
                                    break;
                                }
                                target_order = "j " + to_string(jump_index);

                                target_disk.head_advance(calculate_distance(magnetic_head, jump_index), "jump",magnetic_head_id);
                                have_read_time = 0;
                                current_token = G_token_per_time_step;
                                target_disk.current_time_segment[magnetic_head_id] = time_segment_index;
                                
                                break; // 退出循环，让下一个磁盘行动
                            }
                    }

                    if(in_need_read){
                        // 在需要读的范围，不需要跳转，需要判定是否需要 pass
                        // 需要确保此时一定有 target_block
                        int length = calculate_distance(magnetic_head, target_block);
                        if(length > pass_length(have_read_time)){
                            if(G_token_per_time_step - current_token >= length){
                                // 可以移动到目标位置
                                for(int n1 = 0; n1 < length; n1++){
                                    target_order += "p";
                                }
                                target_disk.head_advance(length, "pass",magnetic_head_id);
                                current_token += length;
                                have_read_time = 0; // 连续读取数量+1

                            }else{
                                // 不能移动到目标位置
                                for(int n1 = 0; n1 < G_token_per_time_step - current_token; n1++){
                                    target_order += "p";
                                }

                                target_disk.head_advance(G_token_per_time_step - current_token, "pass",magnetic_head_id);
                                current_token += G_token_per_time_step - current_token;
                                have_read_time = 0; // 连续读取数量+1

                                break; // 退出当前磁盘行动
                            }
                        }
                        // 判断是否能读
                        if(G_token_per_time_step - current_token > target_disk.get_read_time(1,magnetic_head_id)){
                            // 能读就读取当前块，如果当前块有对象还要更新信息
                            target_order += "r";

                            Object& target_object = object_array[disk[disk_id][magnetic_head]];
        
                            int object_index = disk_block_index[disk_id][magnetic_head];
                            // 应该遍历该物品的所有等待请求情况
                            for(auto it = target_object.wait_request_set.begin(); it != target_object.wait_request_set.end(); ++it){
                                request_array[*it].read_block(object_index);
                            }
                            target_object.check_finish();
        
                            effective_read++;
            
            
                            target_disk.head_advance(1, "read",magnetic_head_id);
                            current_token += target_disk.get_read_time(1,magnetic_head_id);
                            have_read_time++; // 连续读取数量+1
                        }else{
                            // 不能读就退出循环
                            break;
                        }
                    }
                }

           // }

           
        }
    }
    // 报告所有磁盘磁头行动情况 
    for (int i = 1; i <= N_disk_num; i++) {
        for(int magnetic_head_id=0;magnetic_head_id<MAGNERIC_HEAD_NUM;magnetic_head_id++){
            if(disk_array[i].order[magnetic_head_id].size() >= 1){
                if(disk_array[i].order[magnetic_head_id][0] == 'j'){
                    cout << disk_array[i].order[magnetic_head_id] + "\n";
                }else{
                    cout << disk_array[i].order[magnetic_head_id] + "#\n";
                }
            }else{
                cout << disk_array[i].order[magnetic_head_id] + "#\n";
            }
        }
    }

    // 报告请求完成情况
    printf("%d\n", finish_request.size());
    for(int n1 = 0; n1 < finish_request.size(); n1++){
        if(finish_request[n1] == 416662 && global_turn == 2){
            int a = 1;
        }
        int time = (request_array[finish_request[n1]].recieve_time - 1) / FRE_PER_SLICING + 1;
        int this_time = (time_step - 1) / FRE_PER_SLICING + 1;
        if(time_step - request_array[finish_request[n1]].recieve_time > 105){
            zero_request++;
            if(request_array[finish_request[n1]].select){
                select_zero_request++;
            }
        }

        printf("%d\n", finish_request[n1]);
        if(request_array[finish_request[n1]].select){
            // all_finish_select_size 和 all_finish_request_efficiency 只记录当前时间段的
            if(time == this_time){
                all_finish_select_size += (request_array[finish_request[n1]].read_num + 1);
                all_finish_request_efficiency += 
                    get_mark_efficiency(time_step - request_array[finish_request[n1]].recieve_time) 
                    * (request_array[finish_request[n1]].read_num + 1) * 0.5; 
                all_finish_select++;    
            }
        }
        all_mark += get_mark_efficiency(time_step - request_array[finish_request[n1]].recieve_time) 
        * (request_array[finish_request[n1]].read_num + 1) * 0.5; 
    }

    // 删除当前回合的信息
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        for(int magnetic_head_id=0;magnetic_head_id<MAGNERIC_HEAD_NUM;magnetic_head_id++){
            disk_array[n1].order[magnetic_head_id] = "";
            disk_array[n1].current_G_token[magnetic_head_id] = 0;
        }
    }
    finish_request = vector<int>();
    
    //获得处于读取段，但超时未读的请求
    if(time_step>=105 ){
        for(int req_id:request_per_time[time_step-104]){
            int obj_id=request_array[req_id].object_id;
            if(object_array[obj_id].wait_request_set.find(req_id)!=object_array[obj_id].wait_request_set.end()){
                busy_req.push_back(req_id);
                Object& target_object = object_array[obj_id];
                select_but_not_finish += target_object.size;
                if(t > 30000){
                    int a = 1;
                }
                for(int n1 = 1; n1 <= 1; n1++){
                    int disk_id = target_object.disk_array[n1];
                    int actualSegment_id = (target_object.storge_data[n1][0] - 1) / segment_size;

                    disk_array[disk_id].segment_array[actualSegment_id].request_size -= target_object.size;
                    disk_array[disk_id].segment_array[actualSegment_id].all_request_wait_time-=105*target_object.size;
                    int  rate;
                    // rate=disk_array[disk_id].segment_array[actualSegment_id].all_request_wait_time/
                    // (disk_array[disk_id].segment_array[actualSegment_id].request_size+1);
                    
                    for(int n3 = 0; n3 < target_object.size; n3++){
                        if(request_array[req_id].read[n3] == false){
                            int disk_id = target_object.disk_array[n1];
                            int block_id = target_object.storge_data[n1][n3];
                            disk_block_request[disk_id][block_id]--;
                        }
                    }

                    if(disk_array[disk_id].segment_array[actualSegment_id].request_size<0){
                        int a=1;
                    }
                    if(disk_array[disk_id].segment_array[actualSegment_id].request_size>147483647){
                        int a=1;
                    }
                    if(disk_array[disk_id].segment_array[actualSegment_id].all_request_wait_time<0){
                        int a=1;
                    }
                    if(rate<0 or rate>105){
                        int a=0;
                    }
                }
                object_array[obj_id].wait_request_set.erase(req_id);
            }
        }
    }
        
    printf("%d\n",busy_req.size());
    for(int req_id:busy_req){
        printf("%d\n",req_id);
    }
    busy_req = std::vector<int>();

    fflush(stdout);
    return;
}