#include "../common.h"
using namespace std;

std::vector<int> ActualSegment::first_write(int object_id) {
    int object_size = object_array[object_id].size;
    int available_continuous = segment_length - first_write_index;
    
    if (object_size > available_continuous) {
        assert(false);
        return {}; // 连续空间不足，返回空vector表示错误
    }
    
    int start = begin_index + first_write_index;
    for (int i = 0; i < object_size; ++i) {
        if (disk[disk_id][start + i] != 0) {
            assert(false);
            return {}; // 发现已被占用，无法连续写入
        }
    }
    
    int index = 0;
    std::vector<int> positions;
    for (int i = 0; i < object_size; ++i) {
        disk[disk_id][start + i] = object_id;
        disk_block_index[disk_id][start + i] = index;
        positions.push_back(start + i);
        index++;
    }
    
    first_write_index += object_size;
    all_size += object_size;
    return positions;
}

std::vector<int> ActualSegment::write(int object_id) {
    int object_size = object_array[object_id].size;
    std::vector<int> positions;
    
    for (int i = 0; i < segment_length; ++i) {
        int current_pos = begin_index + i;
        if (disk[disk_id][current_pos] == 0) {
            positions.push_back(current_pos);
            if (positions.size() == object_size) {
                break;
            }
        }
    }
    
    if (positions.size() < object_size) {
        assert(false);
        return {}; // 空间不足，返回空vector
    }
    
    int index = 0;
    for (int pos : positions) {
        if(pos == first_write_index + begin_index){
            first_write_index++;
        }
        disk[disk_id][pos] = object_id;
        disk_block_index[disk_id][pos] = index;
        index++;
    }
    
    all_size += object_size;
    return positions;
}

int ActualSegment::get_first_empty() {
    return segment_length - first_write_index;
}

int ActualSegment::get_empty() {
    return segment_length - all_size;
}

vector<pair<int, vector<int>>> VirtualSegment::first_write(int object_id){
    
    vector<pair<int, vector<int>>> result;
    for(int n1 = 0; n1 < 3; n1++){
        Disk& target_disk = disk_array[actual_disk[n1]];
        result.push_back({actual_disk[n1], target_disk.segment_array[actual_index[n1]].first_write(object_id)});
    }
    return result;
}

vector<pair<int, vector<int>>> VirtualSegment::write(int object_id){
    Disk& target_disk = disk_array[actual_disk[0]];
    vector<pair<int, vector<int>>> result;
    for(int n1 = 0; n1 < 3; n1++){
        Disk& target_disk = disk_array[actual_disk[n1]];
        result.push_back({actual_disk[n1], target_disk.segment_array[actual_index[n1]].write(object_id)});
    }
    return result;
}


int VirtualSegment::get_first_empty(){
    Disk& target_disk = disk_array[actual_disk[0]];
    return target_disk.segment_array[actual_index[0]].get_first_empty();
}

int VirtualSegment::get_empty(){
    Disk& target_disk = disk_array[actual_disk[0]];
    return target_disk.segment_array[actual_index[0]].get_empty();
}

int VirtualSegment::get_min_empty(){
    int min_empty = segment_size, min_tag = -1;
    for(int M = 1; M <= M_tag_num; M++){
        if(tag_occupy_size[M] < min_empty && tag_occupy_size[M] != 0){
            min_empty = tag_occupy_size[M];
            min_tag = M;
        }
    }
    
    return min_tag;
}

int ActualSegment::get_request_num(){
    return request_size;
}

double VirtualSegment::get_mark(){
    double all_mark = 0;
    for(int n1 = 1; n1 <= M_tag_num; n1++){
        double this_mark = (double)tag_occupy_size[n1] / (double)segment_size;
        this_mark *= tag_array[n1].fre_read[time_segment_index];
        all_mark += this_mark;
    }
    return all_mark;
}

void VirtualSegment::quit_all_request(){
    int disk_id = actual_disk[0];
    ActualSegment& actualSegment = disk_array[disk_id].segment_array[actual_index[0]];
    for(int n1 = 0; n1 < actualSegment.segment_length; n1++){
        int index = actualSegment.begin_index + n1;
        if(disk_block_request[disk_id][index] > 0){
            Object& target_object = object_array[disk[disk_id][n1]];
            target_object.quit_all_request();
        }
    }
}