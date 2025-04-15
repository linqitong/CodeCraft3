#include "../common.h"

bool Request::read_block(int index){
    if(global_turn==2 and request_id==416662){
        int a=1;
    }
    assert(index >= 0); 
    if(read[index] == false){
        read_num++;
        read[index] = true;
        Object& target_object = object_array[object_id];
        for(int n1 = 1; n1 <= 3; n1++){
            int disk_id = target_object.disk_array[n1];
            int block_id = target_object.storge_data[n1][index];
            disk_block_request[disk_id][block_id]--;
        }
        return true;
    }else{
        return false;
    }
    
}

void Request::finish(){
    if(read_num != object_array[object_id].size){
        return;
    }
    if(request_id == 416662 && global_turn == 2){
        int a = 1;
    }

    Object& target_object = object_array[object_id];
    for(int n1 = 1; n1 <= 1; n1++){
        int disk_id = target_object.disk_array[n1];
        int actualSegment_id = (target_object.storge_data[n1][0] - 1) / segment_size;
       
        disk_array[disk_id].segment_array[actualSegment_id].request_size -= target_object.size;
        disk_array[disk_id].segment_array[actualSegment_id].all_request_wait_time-=(time_step-recieve_time+1)*target_object.size;
        if(disk_array[disk_id].segment_array[actualSegment_id].request_size<0){
            int a=1;
        }
        if(disk_array[disk_id].segment_array[actualSegment_id].request_size>147483647){
            int a=1;
        }
        if(disk_array[disk_id].segment_array[actualSegment_id].all_request_wait_time<0){
            int a=1;
        }
    }
    finish_request.push_back(request_id);
    

    // 全局等待集合删除，对应物品等待集合删除
    object_array[object_id].wait_request_set.erase(request_id);
}

void Request::clear_read_information(){
    Object& target_object = object_array[object_id];
    for(int n1 = 0; n1 < target_object.size; n1++){
        if(read[n1] == false){
            for(int n2 = 1; n2 <= 3; n2++){
                int disk_id = target_object.disk_array[n2];
                int block_id = target_object.storge_data[n2][n1];
                disk_block_request[disk_id][block_id]--;
            }
        }
    }

    for(int n1 = 1; n1 <= 1; n1++){
        int disk_id = target_object.disk_array[n1];
        int actualSegment_id = (target_object.storge_data[n1][0] - 1) / segment_size;
        disk_array[disk_id].segment_array[actualSegment_id].request_size -= target_object.size;
        
        disk_array[disk_id].segment_array[actualSegment_id].all_request_wait_time-=(time_step-recieve_time)*target_object.size;
        if(disk_array[disk_id].segment_array[actualSegment_id].all_request_wait_time<0){
            int a=1;
        }
    }
}