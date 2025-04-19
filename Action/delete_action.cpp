#include "../common.h"
using namespace std;

void delete_action()
{
    std::ostringstream oss;
    // 记录要删除的对象
    int n_delete;
    int abort_num = 0; // 还没有完成的请求总数

    static int _id[MAX_OBJECT_NUM]; // 要删除的对象 id 序列

    if(global_turn==1){
        scanf("%d", &n_delete);
        for (int i = 1; i <= n_delete; i++) {
            scanf("%d", &_id[i]);
            del_record[time_step].push_back(_id[i]);
            abort_num += object_array[_id[i]].wait_request_set.size();
            object_array[_id[i]].life_time+=time_step -object_array[_id[i]].load_time;
        }
    }else{
        n_delete=del_record[time_step].size();
        for (int i = 0; i < del_record[time_step].size(); i++){
            
            Object obj=object_array[del_record[time_step][i]];
            _id[i+1]=del_record[time_step][i];
            abort_num += obj.wait_request_set.size();
        }
    }

    if(global_turn == 2 && use_round2_reoutput){
        oss << abort_num << "\n";
    }else{
        printf("%d\n", abort_num);
    }

    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        for (const auto& elem : object_array[_id[i]].wait_request_set) {
            if(global_turn == 2 && use_round2_reoutput){
                oss << elem << "\n";
            }else{
                printf("%d\n", elem);
            }
            
            // wait_request_set.erase(elem);
            request_array[elem].clear_read_information();
        }

        // 删除请求
        quit_request += object_array[id].wait_request_set.size();
        object_array[id].wait_request_set = set<int>();
        
        Object& target_object = object_array[id];

        // 清除对象的磁盘块信息并修改 allocate_disk
        if(target_object.quit == false){
            int j = 1;
            int disk_id = object_array[id].disk_array[j];
            int first_block = object_array[id].storge_data[j][0];
            ActualSegment& target_actual_segment = disk_array[disk_id].segment_array[(first_block - 1) / segment_size];
            target_actual_segment.object_set.erase(id);
            for(int n1 = object_array[id].size - 1; n1 >= 0; n1--){
                disk[disk_id] // 清除磁盘块信息
                    [object_array[id].storge_data[j][n1]] = 0;
                target_actual_segment.all_size--;
                while(target_actual_segment.first_write_index - 1>= 0 
                    && disk[disk_id][target_actual_segment.begin_index + target_actual_segment.first_write_index - 1] == 0){
                        target_actual_segment.first_write_index--;
                    }
            }
        }else{
            int disk_0 = target_object.disk_array[1];
            for(int n1 = 0; n1 < target_object.size; n1++){
                disk[disk_0][target_object.storge_data[1][n1]] = 0;
                Disk& target_disk1 = disk_array[disk_0];
                target_disk1.rubbish_stack.push(target_object.storge_data[1][n1]);
            }
        }

        // 删除垃圾栈的信息
        int disk_1 = target_object.disk_array[2];
        int disk_2 = target_object.disk_array[3];
        for(int n1 = 0; n1 < target_object.size; n1++){
            disk[disk_1][target_object.storge_data[2][n1]] = 0;
            disk[disk_2][target_object.storge_data[3][n1]] = 0;
            Disk& target_disk1 = disk_array[disk_1];
            Disk& target_disk2 = disk_array[disk_2];
            target_disk1.rubbish_stack.push(target_object.storge_data[2][n1]);
            target_disk2.rubbish_stack.push(target_object.storge_data[3][n1]);
        }
    }

    if(global_turn == 2 && use_round2_reoutput){
        round2_delete_track[t] = "0\n";
    }
    fflush(stdout);
}


