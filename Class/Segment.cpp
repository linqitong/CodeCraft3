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

int ActualSegment::get_request_num(){
    return request_size;
}

int ActualSegment::get_score() {
    double all_mark = 0;
    for(int n1 = 1; n1 <= M_tag_num+1; n1++){
        double this_mark = (double)tag_occupy_size[n1] / (double)segment_size;
        if(global_turn == 1 ){
            if(tag_array[n1].calc_t_read!=time_step) tag_array[n1].calc_read_score();
            this_mark *= tag_array[n1].read_score;
        }else{
            int begin=this->begin_index;
            set<int> obj_set;
            for(int idx=begin_index;idx<begin_index+segment_length;idx++){
                obj_set.insert(disk[this->disk_id][idx]);
            }
            for(int i:obj_set){
                all_mark+=obj_read_data[i][time_step/pearson_sample_interval];
            }
            return all_mark;
            // this_mark *= tag_array[n1].fre_read[time_segment_index];
        }
        
        all_mark += this_mark;
    }
    return all_mark;
}