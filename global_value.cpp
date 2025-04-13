#include "common.h"
using namespace std;

// 在这里定义全局变量，仍然需要在 common.h 中声明

std::string default_disk_block_index_mode = "average"; // average/difference

int T_time_step_length, M_tag_num, N_disk_num, V_block_per_disk, G_token_per_time_step,K_max_exchange_block;

Disk disk_array[MAX_DISK_NUM];
Object object_array[MAX_OBJECT_NUM];
Request request_array[MAX_REQUEST_NUM];
Tag tag_array[MAX_TAG_NUM];
int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
int disk_block_index[MAX_DISK_NUM][MAX_DISK_SIZE];
int disk_block_request[MAX_DISK_NUM][MAX_DISK_SIZE];
int time_segment = 3;

int time_step;
int quit_num1 = 0;

std::vector<int> busy_req;

// std::set<int> wait_request_set;
std::vector<int> finish_request;
std::vector<std::vector<int>> most_req_segment;

int Data[7]={64,52,42,34,28,23,19};

int zero_request = 0;

int t = 1;
int quit_request = 0;
int total_write = 0;
int occupy_size = 0;
int head_idle_time = 0;
int all_request_read_size = 0;
int mode = 0;
int drop_req_num=0;
vector<int> high_read_time_vector = {0};

int max_think_num_for_empty_read = 3;
int segment_size = 600;
int min_read_shold = 6000; 
int max_segment_select_size = 3;
vector<vector<int>> request_per_time=vector<vector<int>>(86400+1);
vector<int> disk_assignable_actual_num = vector<int>(MAX_DISK_NUM);
vector<vector<int>> tag_write;
vector<vector<int>> tag_content;
vector<vector<long long>> tag_read;
std::vector<VirtualSegment> virtual_segment_array;
std::vector<pair<int, int>> empty_segment_array;
std::vector<int> g;
int G;

long long tag_first_write_size = 0;
long long tag_write_size = 0;
long long first_write_size = 0;
long long write_size = 0;
long long empty_first_write_size = 0;
long long empty_write_size = 0;
int selected_r = 0;
int un_selected_r = 0;
set<int> select_VirtualSegment = set<int>();
double all_finish_request_efficiency = 0;
int all_finish_select_size = 0;
int all_select_efficiency = 0;
int time_segment_index = 1;

int empty_object_read = 0; // 空读数量
int empty_request_read = 0; // 空请求 read 数量
int effective_read = 0;
int select_zero_request = 0;
int all_finish_select = 0;
double all_mark = 0;
int segment_num = 36;
int calcuate_use_time = 0, calcuate_use_time1 = 0, calcuate_use_time2 = 0, calcuate_use_time3 = 0, calcuate_use_time4 = 0;

int select_but_not_finish = 0;

int calculate_distance(int start, int end){
    if (start < 1 || start > V_block_per_disk || end < 1 || end > V_block_per_disk) {
        throw std::invalid_argument("Invalid start or end position.");
    }

    if (end >= start) {
        return end - start;
    } else {
        return V_block_per_disk - start + end;
    }
}

int advance_position(int current, int distance) {
    if (current < 1 || current > V_block_per_disk) {
        throw std::invalid_argument("Invalid current position.");
    }

    // 计算新位置
    int newPosition = current + distance;

    // 处理循环
    if (newPosition > V_block_per_disk) {
        newPosition = newPosition % V_block_per_disk;
        if (newPosition == 0) {
            newPosition = V_block_per_disk;
        }
    }

    return newPosition;
}

int find_tag(int disk_id, int block_id) {
    if(mode == 0 || mode == 1){
        for(int i = 2; i <= M_tag_num; i++){
            if(block_id >= tag_array[i - 1].allocate_disk[disk_id].begin && block_id < tag_array[i].allocate_disk[disk_id].begin){
                return i - 1;
            }
        }
        return M_tag_num;
    }
    else if(mode == 2){
        if(disk_id <= 3){
            for(int i = 2; i < 6; i++){
                int tag_id = high_read_time_vector[i], tag_id_1 = high_read_time_vector[i - 1];
                if(block_id >= tag_array[tag_id_1].default_disk[0].begin && block_id < tag_array[tag_id].default_disk[0].begin)
                    return tag_id_1;
            }
            return high_read_time_vector[5];
        }
        if(disk_id > 3 && disk_id <= 7){
            for(int i = 7; i <= 11; i++){
                int tag_id = high_read_time_vector[i], tag_id_1 = high_read_time_vector[i - 1];
                if(block_id >= tag_array[tag_id_1].default_disk[0].begin && block_id < tag_array[tag_id].default_disk[0].begin)
                    return tag_id_1;
            }
            return high_read_time_vector[11];
        }
        if(disk_id > 7 && disk_id <= 10){
            for(int i = 13; i <= 16; i++){
                int tag_id = high_read_time_vector[i], tag_id_1 = high_read_time_vector[i - 1];
                if(block_id >= tag_array[tag_id_1].default_disk[0].begin && block_id < tag_array[tag_id].default_disk[0].begin)
                    return tag_id_1;
            }
            return high_read_time_vector[16];
        }
    }
}

int find_max_disk(int tag_id) {
    int max_disk = 1;
    for(int i = 2; i <= N_disk_num; i++){
        if(tag_array[tag_id].allocate_disk[i].size > tag_array[tag_id].allocate_disk[max_disk].size){
            max_disk = i;
        }
    }
    return max_disk;
}

int find_max_disk1(){
    int max_disk = 1;
    for(int i = 2; i <= N_disk_num; i++){
        if(disk_array[i].empty_size > disk_array[max_disk].empty_size){
            max_disk = i;
        }
    }
    return max_disk;
}

double calculate_score(int block_id, int disk_id){
    int obj_id = disk[disk_id][block_id], size = object_array[obj_id].size;
    double total_score = 0;
    for(int req_id:object_array[obj_id].wait_request_set){
        int wait_time = time_step - request_array[req_id].recieve_time;
        double score = 0;
        if(wait_time >= 0 && wait_time <= 10)
            score = 1 - 0.005 * wait_time;
        else if(wait_time > 10 && wait_time <= 105)
            score = 1.05 - 0.01 * wait_time;
        else if(wait_time > 105)
            score = 0;
        total_score += score;
    }

    return total_score;
}

int global_get_read_time(int read_num, int have_read_time){
    int data[7];
    data[0] = 64;
    for (int n1 = 1; n1 < 7; n1++) {
        data[n1] = std::max(16, (int)ceil(data[n1 - 1] * 0.8));
    }
    int total_read = have_read_time + read_num;
    int index = have_read_time;
    int all_time = 0;
    while (index < total_read) {
        if (index < 7) {
            all_time += data[index];
            index++;
        } else {
            // 处理所有剩余读取次数并跳出循环
            int remaining = total_read - index;
            all_time += remaining * 16;
            break;
        }
    }
    return all_time;
}

vector<int> get_allocate_size_rank(int tag_id){
    Tag& tag = tag_array[tag_id];
    vector<pair<int, int>> size_rank;
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        size_rank.push_back({tag.allocate_disk[n1].size, n1});
    }
    sort(size_rank.begin(), size_rank.end(), std::greater<pair<int, int>>());

    vector<int> result;
    for(int n1 = 0; n1 < size_rank.size(); n1++){
        result.push_back(size_rank[n1].second);
    }
    return result;
}

bool if_find(vector<int>& vec, int target){
    for(int i = 0; i < vec.size(); i++){
        if(vec[i] == target)
            return true;
    }
    return false;
}

// 根据不同时段的写入需求, 将需求量最高的标签分配到不同的磁盘中, 保证错峰读取
void analyze_read_request(){
    // 读取高峰时长
    int high_read_time = T_time_step_length - 10800 - 9000; // 10800为起始高峰, 9000为77400-86400的末尾低峰

    int start_step = 10800 / FRE_PER_SLICING + 1, end_step = 77400 / FRE_PER_SLICING + 1;

    // 将高峰时长等分为三段, 根据每段的不同标签读取需求量进行排序
    vector<pair<int, int>> high_read_time_rank_1;
    vector<pair<int, int>> high_read_time_rank_2;
    vector<pair<int, int>> high_read_time_rank_3;
    for(int t = 1; t <= M_tag_num; t++){
        int total_read_request = 0;
        for(int i = 0; i < 12; i++){
            total_read_request += tag_array[t].fre_read[start_step + i];
        }
        high_read_time_rank_1.push_back({total_read_request, t});
        for(int i = 12; i < 24; i++){
            total_read_request += tag_array[t].fre_read[start_step + i];
        }
        high_read_time_rank_2.push_back({total_read_request, t});
        for(int i = 24; i < 36; i++){
            total_read_request += tag_array[t].fre_read[start_step + i];
        }
        high_read_time_rank_3.push_back({total_read_request, t});
    }

    // 降序排列各高峰时段的写入需求表
    sort(high_read_time_rank_1.begin(), high_read_time_rank_1.end(), std::greater<pair<int, int>>());
    sort(high_read_time_rank_2.begin(), high_read_time_rank_2.end(), std::greater<pair<int, int>>());
    sort(high_read_time_rank_3.begin(), high_read_time_rank_3.end(), std::greater<pair<int, int>>());

    // 统计不同高峰时段的前五个标签, 第二个时段取前六个标签, 保证取到的16个标签不重复
    int if_re = 1;
    for(int i = 0;; i++){
        int size = high_read_time_vector.size();
        if(size == 17)
            break;
        if(size < 6 && !if_find(high_read_time_vector, high_read_time_rank_1[i].second))
            high_read_time_vector.push_back(high_read_time_rank_1[i].second);
        if(size == 6 && if_re)
            i = 0, if_re = 0;
        if(size >= 6 && size < 12 && !if_find(high_read_time_vector, high_read_time_rank_2[i].second))
            high_read_time_vector.push_back(high_read_time_rank_2[i].second), if_re = 1;
        if(size == 12 && if_re)
            i = 0, if_re = 0;
        if(size >= 12 && size < 17 && !if_find(high_read_time_vector, high_read_time_rank_3[i].second))
            high_read_time_vector.push_back(high_read_time_rank_3[i].second);
    }

    // 分配标签的默认磁盘索引
    for(int i = 1; i <= M_tag_num; i++){
        int tag_id = high_read_time_vector[i];
        if(i >= 1 && i <= 5)
            tag_array[tag_id].default_index = {1, 2, 3};
        if(i >= 6 && i <= 11)
            tag_array[tag_id].default_index = {4, 5, 6, 7};
        if(i >= 12 && i <= 16)
            tag_array[tag_id].default_index = {8, 9, 10};
    }
}

double calculate_variance_double(const std::vector<double>& data){
    assert(data.size() >= 0);

    // 计算均值
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    double mean = sum / data.size();

    // 计算方差
    double variance = 0.0;
    for (double value : data) {
        variance += std::pow(value - mean, 2);
    }
    variance /= data.size();

    return variance;
}

double get_mark_efficiency(int time){
    assert(time >= 0);
    if(time >= 0 && time <= 10){
        return 1.0 - 0.005 * (double)time;
    }else if(time > 10 && time <= 105){
        return 1.05 - 0.01 * (double)time;
    }else{
        return 0.0;
    }
}