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
int drop_req_num=0;
vector<int> high_read_time_vector = {0};

int max_think_num_for_empty_read = 3;
int segment_size; // 端长度在初始化时动态生成，不预先设置
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