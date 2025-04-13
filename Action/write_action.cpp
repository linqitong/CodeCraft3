#include "../common.h"
using namespace std;

/*// 对磁盘进行空闲内存更新
int update_disk_free_storge(vector<pair<int, int>>& disk_order, int disk_id, int tag_id, int demand_size)
{
    for(int i = 0; i < disk_order.size(); i++){
        if(disk_order[i].second == disk_id){
            disk_order[i].first -= demand_size; // 更新磁盘的剩余空间大小
            break;
        }
    }
    disk_array[disk_id].allocate_free_size -= demand_size; // 更新全局变量disk的预分配空余磁盘块大小

    tag_array[tag_id].occupy_index.push_back(disk_id);

    int begin = disk_array[disk_id].allocate_free_storge[0].begin;

    for(int i = 0; i < disk_array[disk_id].allocate_free_storge.size(); i++){
        
        int left = disk_array[disk_id].allocate_free_storge[i].begin; // begin为当前空闲磁盘块的左边界 
        if(disk_array[disk_id].allocate_free_storge[i].size > demand_size){
            disk_array[disk_id].free_storge.push_back({begin, demand_size}); // 将该空闲表插入到实际空闲内存块表中

            disk_array[disk_id].allocate_free_storge[i].begin += demand_size; // 将该空闲表的左边界向右移动
            disk_array[disk_id].allocate_free_storge[i].size -= demand_size; // 更新该空闲表的大小

            break;
        }
        else{
            demand_size -= disk_array[disk_id].allocate_free_storge[i].size;
            disk_array[disk_id].allocate_free_storge.erase(disk_array[disk_id].allocate_free_storge.begin() + i); // 删除该空闲表
            
            disk_array[disk_id].free_storge.push_back({left, disk_array[disk_id].allocate_free_storge[i].size}); // 将该空闲表插入到实际空闲内存块表中
            i--;
        }
    }

    // 更新磁盘的空闲内存块表
    
    sort(disk_array[disk_id].free_storge.begin(), disk_array[disk_id].free_storge.end());
    sort(disk_array[disk_id].allocate_free_storge.begin(), disk_array[disk_id].allocate_free_storge.end());

    return begin;
}

// 根据时段净需求分配对应标签的默认磁盘索引
void allocate_disk(int time_step)
{
    int step = time_step / FRE_PER_SLICING + 1;
    
    // 根据磁盘剩余空间大小降序排列
    vector<pair<int, int>> disk_order; // first 代表磁盘剩余空间大小, second 代表磁盘编号
    for(int i = 1; i <= N_disk_num; i++){
        disk_order.push_back({disk_array[i].allocate_free_size, i});
    }
    sort(disk_order.begin(), disk_order.end(), greater<pair<int, int>>());

    // 根据磁盘剩余空间大小与标签净需求进行磁盘分配
    for(int i = 1; i <= M_tag_num; i++){
        int tag_id = tag_order1[step].tag_order[i].second;
        int net_demand = tag_order1[step].tag_order[i].first * 1.1; // 该标签当前时间段净需求, 1.1为调整系数
        for(int j = 0; j < 3; j++){

            // 分别更新同一个标签的三个副本的磁盘空间, 同时说明该标签占用磁盘号
            int left = update_disk_free_storge(disk_order, disk_order[j].second, tag_id, net_demand);
            
            
            
            // 对某一标签下的默认索引进行赋值, begin为占用磁盘块号的左边界, size为净需求大小
            if(j == 0)
                tag_array[tag_id].default_index.first = {left, net_demand, disk_order[j].second};
            else if(j == 1)
                tag_array[tag_id].default_index.second = {left, net_demand, disk_order[j].second};
            else
                tag_array[tag_id].default_index.third = {left, net_demand, disk_order[j].second};
        }

        // 对磁盘剩余空间大小进行重新排序, 使得下一次分配时优先选择最大的磁盘
        sort(disk_order.begin(), disk_order.end(), greater<pair<int, int>>());
    }
}*/

//v2
void allocate_disk()
{
    int step = time_step / FRE_PER_SLICING + 1;

    for(int i = 1; i <= M_tag_num; i++){
        tag_array[i].allocate_disk.clear();
        tag_array[i].allocate_disk.push_back({0, 0, 0});
    }

    // 根据当前时间段净需求对一个磁盘进行分区, 分区比例为各标签的净需求比例
    int left = 1;
    for(int id = 1; id <= M_tag_num; id++){
        // 计算该标签的分配磁盘块数
        if(tag_array[id].all_write_size <= 0){
            for(int j = 1; j <= N_disk_num; j++){
                StorgeNode storge = {left, 1, id};
                tag_array[id].allocate_disk.push_back(storge);
                tag_array[id].allo_begin.push_back(left);
            }
            continue;
        }
        double temp1 = tag_array[id].all_write_size, temp2 = total_write;
        double rate = temp1 / temp2;
        int size = V_block_per_disk * rate;

        for(int j = 1; j <= N_disk_num; j++){
            StorgeNode storge = {left, size, id};
            if(id == 16 && left + size > V_block_per_disk){
                storge.size = V_block_per_disk - left;
            }
            else if(id == 16 && left + size <= V_block_per_disk){
                storge.size = V_block_per_disk - left;
            }
            tag_array[id].allocate_disk.push_back(storge); 
            tag_array[id].allo_begin.push_back(left); 
        }
        left += size;
    }
}

// 如果分配磁盘空间不够, 征用相邻标签的磁盘空间
void occupy_others(int tag_id, int rep_num, Object& obj, int obj_id)
{
    vector<pair<int, int>> disk_size_rank;
    for(int i = 1; i <= N_disk_num; i++){
        disk_size_rank.push_back({disk_array[i].empty_size, i});
    }

    sort(disk_size_rank.begin(), disk_size_rank.end(), greater<pair<int, int>>());

    for(int i = 1; i <= N_disk_num; i++){
        int disk_id = disk_size_rank[i].second;
        if(obj.disk_array[1] == disk_id || obj.disk_array[2] == disk_id)
            continue;
        int if_empty = 16;
        for(int j = tag_id % 16 + 1; j <= M_tag_num && j != tag_id; j++){
            if_empty--;
            if(tag_array[j].allocate_disk[disk_id].size > obj.size){
                Object_storge storge;
                int left = tag_array[j].allocate_disk[disk_id].begin;
                for(int k = 0; k < obj.size; k++){
                    if(disk[disk_id][left] == 0){
                        storge.object_storge.push_back(left);
                        disk[disk_id][left] = obj_id;
                        tag_array[j].allocate_disk[disk_id].size--;
                        left++;
                    }
                    else
                        k--, left++;
                }
                obj.storge_data[rep_num] = storge;
                obj.disk_array[rep_num] = disk_id;
                return;
            }
            if(j == 16)
                j = 1;
            if(if_empty == 0)
                break;
        }
    }
}

void write_action()
{
    // 读取信息
    mode = 0;
    int n_write;
    scanf("%d", &n_write);
    vector<int> write_array(n_write + 1); // 要写入的对象 id 序列

    for (int i = 1; i <= n_write; i++) {
        int id, size, tag;
        scanf("%d%d%d", &id, &size, &tag);
        object_array[id].size = size;
        object_array[id].tag = tag;
        write_array[i] = id;

        // 实现写入逻辑
        for(int j = 1; j <= REP_NUM; j++){
            // int k = find_max_disk(tag), times = 16;
            int k = 1, times = 16;
            for(k; k <= N_disk_num; k++){ 
                if(times == 0)
                    break;
                times--;
                int disk_index = k;
                if(object_array[id].disk_array[1] == disk_index || object_array[id].disk_array[2] == disk_index)
                    continue;
                if(tag_array[tag].allocate_disk[disk_index].size >= size){
                    Object_storge storge;
                    int obj_size = size, left = tag_array[tag].allocate_disk[disk_index].begin;
                    for(; obj_size > 0; obj_size--){
                        if(disk[disk_index][left] == 0){
                            storge.object_storge.push_back(left);
                            disk[disk_index][left] = id,
                            tag_array[tag].allocate_disk[disk_index].size--;
                            left++;
                        }
                        else
                            obj_size++, left++;
                    }
                    object_array[id].disk_array[j] = disk_index;
                    object_array[id].storge_data[j] = storge;
                    break;
                }
            }
        }

        for(int j = 1; j <= REP_NUM; j++){
            if(object_array[id].disk_array[j] == 0){
                occupy_others(tag, j, object_array[id], id);
                occupy_size += object_array[id].size;
            }
        }
    }
    // 输出信息
    for (int i = 1; i <= n_write; i++){
        cout << write_array[i] << endl;
        for (int j = 1; j <= REP_NUM; j++){
            cout << object_array[write_array[i]].disk_array[j];
            for(int k = 0; k < object_array[write_array[i]].size; k++){
                cout << " " << object_array[write_array[i]].storge_data[j].object_storge[k]; 
            }
            printf("\n");
        }
    }

    fflush(stdout);
    return;
}

// 乱序写入策略
void write_action1()
{
    // 读取信息
    mode = 1;
    int n_write;
    scanf("%d", &n_write);
    vector<int> write_array(n_write + 1); // 要写入的对象 id 序列

    if(time_step == 37227)
        int a = 1;

    for (int i = 1; i <= n_write; i++) {
        int id, size, tag;
        scanf("%d%d%d", &id, &size, &tag);
        object_array[id].size = size;
        object_array[id].tag = tag;
        write_array[i] = id;

        // 实现写入逻辑, 乱序存储
        for(int j = 1; j <= REP_NUM; j++){
            int k = find_max_disk1();
            for(; k <= N_disk_num; k = (k % N_disk_num) + 1){
                if(object_array[id].disk_array[1] == k || object_array[id].disk_array[2] == k)
                    continue;
                if(disk_array[k].empty_size >= size){
                    Object_storge storge;
                    int left = 1;
                    for(int l = 0; l < size; l++){
                        if(disk[k][left] == 0){
                            storge.object_storge.push_back(left);
                            disk[k][left] = id;
                            left++;
                            disk_array[k].empty_size--;
                        }
                        else
                            l--, left++;
                    }
                    object_array[id].disk_array[j] = k;
                    object_array[id].storge_data[j] = storge;
                    break;
                }
            }
        }
    }
    // 输出信息
    for (int i = 1; i <= n_write; i++){
        cout << write_array[i] << endl;
        for (int j = 1; j <= REP_NUM; j++){
            cout << object_array[write_array[i]].disk_array[j];
            for(int k = 0; k < object_array[write_array[i]].size; k++){
                cout << " " << object_array[write_array[i]].storge_data[j].object_storge[k]; 
            }
            printf("\n");
        }
    }

    fflush(stdout);
    return;
}

// 根据不同时段写入进行错峰分盘, 依据high_read_time_vector进行分盘
void allocate_disk1()
{   
    vector<double> total = {0, 0, 0};
    for(int i = 1; i <= 16; i++){
        int j = 0;
        if(i <= 5) j = 0;
        else if(i <= 11) j = 1;
        else j = 2;
        int tag_id = high_read_time_vector[i];
        total[j] += tag_array[tag_id].all_write_size;
    }
    int left = 1;
    for(int i = 1; i <= 5; i++){
        int tag_id = high_read_time_vector[i];
        double rate = tag_array[tag_id].all_write_size / total[0];
        int size = V_block_per_disk * rate;
        StorgeNode storge = {left, size, tag_id};
        tag_array[tag_id].default_disk = {storge, storge, storge};
        left += size;
    }

    left = 1;
    for(int i = 6; i <= 11; i++){
        int tag_id = high_read_time_vector[i];
        double rate = tag_array[tag_id].all_write_size / total[1];
        int size = V_block_per_disk * rate;
        StorgeNode storge = {left, size, tag_id};
        tag_array[tag_id].default_disk = {storge, storge, storge, storge};
        left += size;
    }

    left = 1;
    for(int i = 12; i <= 16; i++){
        int tag_id = high_read_time_vector[i];
        double rate = tag_array[tag_id].all_write_size / total[2];
        int size = V_block_per_disk * rate;
        StorgeNode storge = {left, size, tag_id};
        tag_array[tag_id].default_disk = {storge, storge, storge};
        left += size;
    }
}

void occupy_others1(int tag_id, int rep_num, Object& obj, int obj_id){
    int disk_id = 1;
    for(int i = 1; i <= 16; i++){
        if(i == tag_id)
            continue;
        for(int j = 0; j < tag_array[i].default_index.size(); j++){
            int disk_id = tag_array[i].default_index[j];
            if(obj.disk_array[1] == disk_id || obj.disk_array[2] == disk_id)
                continue;
            if(tag_array[i].default_disk[j].size >= obj.size){
                Object_storge storge;
                int left = tag_array[i].default_disk[j].begin;
                for(int k = 0; k < obj.size; k++){
                    if(disk[disk_id][left] == 0){
                        storge.object_storge.push_back(left);
                        disk[disk_id][left] = obj_id;
                        tag_array[i].default_disk[j].size--;
                        left++;
                    }
                    else
                        k--, left++;
                }
                obj.disk_array[rep_num] = disk_id;
                obj.storge_data[rep_num] = storge;
                return;
            }
        }
    }
}

pair<int, int> find_max_disk2(int tag_id){
    int max = tag_array[tag_id].default_disk[0].size, disk_id = tag_array[tag_id].default_index[0], temp = 0;
    for(int i = 1; i < 3; i++){
        if(tag_array[tag_id].default_disk[i].size > max)
            max = tag_array[tag_id].default_disk[i].size, disk_id = tag_array[tag_id].default_index[i], temp = i;
    }

    return {disk_id, temp};
}

// 暂定缩小每个磁盘的分区数目的新写入逻辑, 按照读请求的局部性进行分盘
void write_action2()
{
    // 读取信息
    mode = 2;
    int n_write;
    scanf("%d", &n_write);
    vector<int> write_array(n_write + 1); // 要写入的对象 id 序列

    if(time_step == 37227)
        int a = 1;

    for (int i = 1; i <= n_write; i++) {
        int id, size, tag;
        scanf("%d%d%d", &id, &size, &tag);
        object_array[id].size = size;
        object_array[id].tag = tag;
        write_array[i] = id;

        // 实现写入逻辑, 错峰写入
        for(int j = 1; j <= REP_NUM; j++){
            // int k = find_max_disk1();
            int k = 1;

            vector<pair<int, int>> max_disk;

            int d_size = tag_array[tag].default_disk.size();
            pair<int, int> max;
            if(d_size == 4){
                max = find_max_disk2(tag);
                int disk_index = max.first;
                if(object_array[id].disk_array[1] == disk_index || object_array[id].disk_array[2] == disk_index)
                    continue;
                if(tag_array[tag].default_disk[max.second].size >= size){
                    Object_storge storge;
                    int left = tag_array[tag].default_disk[max.second].begin;
                    for(int l = 0; l < size; l++){
                        if(disk[disk_index][left] == 0){
                            storge.object_storge.push_back(left);
                            disk[disk_index][left] = id;
                            tag_array[tag].default_disk[max.second].size--;
                            left++;
                        }
                        else
                            l--, left++;
                    }
                    object_array[id].disk_array[j] = disk_index;
                    object_array[id].storge_data[j] = storge;
                }
            }
            else {
                for(int k = 0; k < d_size; k++){
                    int disk_index = tag_array[tag].default_index[k];
                    if(object_array[id].disk_array[1] == disk_index || object_array[id].disk_array[2] ==disk_index)
                        continue;
                    if(tag_array[tag].default_disk[k].size >= size){
                        Object_storge storge;
                        int left = tag_array[tag].default_disk[k].begin;
                        for(int l = 0; l < size; l++){
                            if(disk[disk_index][left] == 0){
                                storge.object_storge.push_back(left);
                                disk[disk_index][left] = id;
                                tag_array[tag].default_disk[k].size--;
                                left++;
                            }
                            else
                                l--, left++;
                        }
                        object_array[id].disk_array[j] = disk_index;
                        object_array[id].storge_data[j] = storge;
                        break;
                    }
                }
            }
        }

        for(int j = 1; j <= REP_NUM; j++){
            if(object_array[id].disk_array[j] == 0){
                occupy_others1(tag, j, object_array[id], id);
                occupy_size += object_array[id].size;
            }
        }
    }
    // 输出信息
    for (int i = 1; i <= n_write; i++){
        cout << write_array[i] << endl;
        for (int j = 1; j <= REP_NUM; j++){
            cout << object_array[write_array[i]].disk_array[j];
            for(int k = 0; k < object_array[write_array[i]].size; k++){
                cout << " " << object_array[write_array[i]].storge_data[j].object_storge[k]; 
            }
            printf("\n");
        }
    }

    fflush(stdout);
    return;
}


const double INF = numeric_limits<double>::max();
// 计算两个数列的欧氏距离
double calculateDistance(const vector<double>& a, const vector<double>& b) {
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

// 解决TSP问题，返回最优路径的索引顺序
vector<int> solveTSP(const vector<vector<double>>& sequences) {
    const int n = sequences.size();
    if (n == 0) return {};

    // 预计算距离矩阵
    vector<vector<double>> dist(n, vector<double>(n, 0));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            dist[i][j] = calculateDistance(sequences[i], sequences[j]);
        }
    }

    // DP数组：dp[mask][i] 表示经过mask集合中的节点，最后位于i的最小路径
    vector<vector<double>> dp(1 << n, vector<double>(n, INF));
    vector<vector<int>> prev(1 << n, vector<int>(n, -1));

    // 初始化：单节点路径
    for (int i = 0; i < n; ++i) {
        dp[1 << i][i] = 0;
    }

    // 动态规划填表
    for (int mask = 0; mask < (1 << n); ++mask) {
        for (int i = 0; i < n; ++i) {
            if (!(mask & (1 << i)) || dp[mask][i] == INF) continue;
            
            for (int j = 0; j < n; ++j) {
                if (mask & (1 << j)) continue;
                
                const int new_mask = mask | (1 << j);
                const double new_cost = dp[mask][i] + dist[i][j];
                
                if (new_cost < dp[new_mask][j]) {
                    dp[new_mask][j] = new_cost;
                    prev[new_mask][j] = i;
                }
            }
        }
    }

    // 寻找闭合环路的最优解
    int full_mask = (1 << n) - 1;
    double min_cost = INF;
    int last = -1;
    
    for (int j = 0; j < n; ++j) {
        if (dp[full_mask][j] != INF) {
            const double total_cost = dp[full_mask][j] + dist[j][0]; // 闭合到起点0
            if (total_cost < min_cost) {
                min_cost = total_cost;
                last = j;
            }
        }
    }

    // 回溯路径
    vector<int> path;
    int current_mask = full_mask;
    int current = last;
    
    while (current != -1) {
        path.push_back(current);
        int next = prev[current_mask][current];
        if (next == -1) break;
        current_mask ^= (1 << current);
        current = next;
    }
    
    reverse(path.begin(), path.end());
    return path;
}

void allocate_segments() {
    string expect_num_mode = "tag_content"; // tag_write/tag_content
    string read_probability_mode = "tag_content"; // tag_write/tag_content

    // Step 1: 计算每个tag的期望虚拟段数目
    vector<int> expected_vs(M_tag_num + 1, 0);

    for (int i = 1; i <= M_tag_num; ++i) {
        int max_val = 0;
        if(expect_num_mode == "tag_write"){
            for (int t = 0; t < 3; ++t) {
                int current = tag_write[i][t];
                max_val = max_val + current;
            }
        }else{ // tag_content
            for (int t = 0; t < 3; ++t) {
                int current = tag_content[i][t];
                max_val = max(max_val, current);
            }
        }
        expected_vs[i] = max_val / segment_size; // 忽略多余的请求
    }

    // Step 2: 计算每个tag在三个时间段的读取频率
    vector<vector<double>> tag_read_freq(M_tag_num + 1, vector<double>(3, 0.0));
    for (int i = 1; i <= M_tag_num; ++i) {
        for (int t = 0; t < 3; ++t) {
            int denominator = 0;
            if(read_probability_mode == "tag_write"){
                for (int t = 0; t < 3; ++t) {
                    int current = tag_write[i][t];
                    denominator = denominator + current;
                }
            }else{ // tag_content
                for (int t = 0; t < 3; ++t) {
                    int current = tag_content[i][t];
                    denominator = max(denominator, current);
                }
            }
            tag_read_freq[i][t] = static_cast<double>(tag_read[i][t]) / denominator;
        }
    }

    // 初始化磁盘的读取概率和虚拟段列表
    vector<vector<double>> disk_read_prob(N_disk_num + 1, vector<double>(3, 0.0));
    vector<vector<int>> disk_virtual_segments(N_disk_num + 1);
    vector<int> allocated(M_tag_num + 1, 0);

    while (true) {
        // 选择完成率最低的tag
        int selected_tag = -1;
        double min_ratio = 10000000000000000;
        for (int i = 1; i <= M_tag_num; ++i) {
            if (expected_vs[i] == 0) continue;
            double ratio = static_cast<double>(allocated[i]) / (double)expected_vs[i];
            if (ratio < min_ratio) {
                min_ratio = ratio;
                selected_tag = i;
            }
        }
        assert(min_ratio < 10000000000000000);

        // 收集可用的磁盘
        vector<int> available_disks;
        for (int d = 1; d <= N_disk_num; ++d) {
            if (disk_assignable_actual_num[d] >= 1) available_disks.push_back(d);
        }
        if (available_disks.size() < 3) break;

        // 生成所有可能的三个磁盘组合
        vector<tuple<int, int, int>> candidates;
        for (size_t i = 0; i < available_disks.size(); ++i) {
            for (size_t j = i + 1; j < available_disks.size(); ++j) {
                for (size_t k = j + 1; k < available_disks.size(); ++k) {
                    candidates.emplace_back(
                        available_disks[i], available_disks[j], available_disks[k]
                    );
                }
            }
        }
        if (candidates.empty()) break;

        // 选择最优组合
        tuple<int, int, int> best_candidate;
        double min_total_variance = INT_MAX;
        for (const auto& cand : candidates) {
            int d1 = get<0>(cand), d2 = get<1>(cand), d3 = get<2>(cand);
            vector<vector<double>> temp_prob = disk_read_prob;
            for (int t = 0; t < 3; ++t) {
                double freq = tag_read_freq[selected_tag][t];
                temp_prob[d1][t] += freq;
                temp_prob[d2][t] += freq;
                temp_prob[d3][t] += freq;
            }

            double total_var = 0;
            for (int t = 0; t < 3; ++t) {
                vector<double> probs;
                for (int d = 1; d <= N_disk_num; ++d) probs.push_back(temp_prob[d][t]);
                total_var += calculate_variance_double(probs);
            }

            if (total_var < min_total_variance || 
               (total_var == min_total_variance && cand < best_candidate)) {
                min_total_variance = total_var;
                best_candidate = cand;
            }
        }

        // 分配虚拟段
        int d1 = get<0>(best_candidate), d2 = get<1>(best_candidate), d3 = get<2>(best_candidate);
        disk_assignable_actual_num[d1]--;
        disk_assignable_actual_num[d2]--;
        disk_assignable_actual_num[d3]--;

        disk_array[d1].assigned_segment++;
        disk_array[d2].assigned_segment++;
        disk_array[d3].assigned_segment++;

        VirtualSegment vs;
        // vs.actual_disk = {d1, d2, d3};
        vs.tag_index = selected_tag;
        virtual_segment_array.push_back(vs);
        int vs_index = virtual_segment_array.size() - 1;

        disk_virtual_segments[d1].push_back(vs_index);
        disk_virtual_segments[d2].push_back(vs_index);
        disk_virtual_segments[d3].push_back(vs_index);

        
        allocated[selected_tag]++;
        tag_array[selected_tag].virtual_segment.push_back(vs_index);
        for (int t = 0; t < 3; ++t) {
            double freq = tag_read_freq[selected_tag][t];
            disk_read_prob[d1][t] += freq;
            disk_read_prob[d2][t] += freq;
            disk_read_prob[d3][t] += freq;
        }
    }
    // 到现在为止的结果
    // {disk_assignable_actual_num, disk_virtual_segments};

    // 向 empty_segment 中增加空段
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        int disk_actual_num = disk_array[n1].segment_array.size();
        int index = disk_virtual_segments[n1].size(); 
        for(int n2 = 0; n2 < disk_assignable_actual_num[n1]; index++, n2++){
            empty_segment_array.push_back({index, n1});
        }
    }


    /*
    // 解决 TSP 制定 tag 在段内的优先级
    // 准备数据
    vector<vector<double>> tag_read_data;
    for(int n1 = 1; n1 <= M_tag_num; n1++){
        tag_read_data.push_back(vector<double>());
        for(int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++){
            tag_read_data[n1 - 1].push_back(tag_array[n1].fre_read[j]);
        }
    }
    vector<int> TSP_result = solveTSP(tag_read_data);
    map<int, int> priority;
    for(int n1 = 0; n1 < TSP_result.size(); n1++){
        priority[TSP_result[n1] + 1] = n1;
    }
    */

    // 在 磁盘 内部分配虚拟段
    // 暂定按照第二时间段的消耗时间分配
    vector<int> tag_rank = {14, 12, 9, 4, 16, 6, 5, 11, 15, 13, 3, 2, 7, 8, 10, 1};
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        vector<pair<double, int>> virtual_segment_order;
        Disk& target_disk = disk_array[n1];
        for(int n2 = 0; n2 < disk_virtual_segments[n1].size(); n2++){
            // tag_read_freq
            int virtual_segment_id = disk_virtual_segments[n1][n2];
            int tag_id = virtual_segment_array[virtual_segment_id].tag_index;
            double tag_read_second = tag_read_freq[tag_id][1];
            for(int n3 = 0; n3 < tag_rank.size(); n3++){
                if(tag_id == tag_rank[n3]){
                    virtual_segment_order.push_back({n3, virtual_segment_id});
                    break;
                }
            }
        }
        sort(virtual_segment_order.begin(), virtual_segment_order.end());
        int actual_segment_index = 0;
        for(int n2 = 0; n2 < virtual_segment_order.size(); n2++){
            VirtualSegment& target_virtual = virtual_segment_array[virtual_segment_order[n2].second];
            target_virtual.actual_index.push_back(actual_segment_index);
            target_virtual.actual_disk.push_back(n1);
            target_disk.segment_array[actual_segment_index].virtual_id = virtual_segment_order[n2].second;
            actual_segment_index++;
        }
    }

}

vector<pair<int, vector<int>>> allocate_object(int object_id) {
    Object& obj = object_array[object_id];
    int current_tag = obj.tag;
    int size = obj.size;

    if(time_step == 20856)
        int a = 1;
    
    // 优先级1：对该 tag 的虚拟段尝试连续分配
    for (int vs_index : tag_array[current_tag].virtual_segment) {
        VirtualSegment& vs = virtual_segment_array[vs_index];
        if (vs.get_first_empty() >= size) {
            tag_first_write_size += size;
            vs.tag_occupy_size[current_tag] += size;
            object_array[object_id].virtual_segment_id = vs_index;
            return vs.first_write(object_id);
        }
    }


    // 优先级2：对该 tag 的虚拟段尝试非连续分配
    for (int vs_index : tag_array[current_tag].virtual_segment) {
        VirtualSegment& vs = virtual_segment_array[vs_index];
        if (vs.get_empty() >= size) {
            tag_write_size += size;
            vs.tag_occupy_size[current_tag] += size;
            object_array[object_id].virtual_segment_id = vs_index;
            return vs.write(object_id);
        }
    }



    // 优先级3：遍历所有虚拟段，尝试连续分配
    vector<pair<int, int>> virtual_rank, tag_occupy_rank;
    vector<pair<double, int>> tag_pearson_rank;
    for(int n1 = 0; n1 < virtual_segment_array.size(); n1++){
        VirtualSegment& virtualSegment = virtual_segment_array[n1];
        virtual_rank.push_back({virtualSegment.get_first_empty(), n1}); 
    }
    sort(virtual_rank.begin(), virtual_rank.end(), greater<pair<int, int>>()); // 所有虚拟段可连续分配的集合, first为可连续分配的size大小, second为段序号
    
    // 可注释代码开头
    if(virtual_rank[0].first >= size){
        for(int n1 = 0; n1 < virtual_segment_array.size(); n1++){
            VirtualSegment& virtualSegment = virtual_segment_array[n1];
            tag_occupy_rank.push_back({virtualSegment.tag_occupy_size[current_tag], n1}); // 当前tag的所有段占用情况, first为对段的占用size大小, second为该段序号
        }
        for(int n2 = 0; n2 <= M_tag_num; n2++){
            if(n2 == 0)
                tag_pearson_rank.push_back({0, 0});
            tag_pearson_rank.push_back({tag_array[obj.tag].pearson_tag[n2], n2});
        }
        sort(tag_pearson_rank.begin(), tag_pearson_rank.end(), greater<pair<double, int>>());
        sort(tag_occupy_rank.begin(), tag_occupy_rank.end(), greater<pair<int, int>>());
        
        for(int n1 = 0; n1 < tag_occupy_rank.size(); n1++){
            if(tag_occupy_rank[n1].first == 0){
                for(int k = 1; k <= M_tag_num; k++){
                    if(tag_pearson_rank[k].first == 0)
                        continue;
                    for(int n2 = 0; n2 < virtual_rank.size(); n2++){
                        VirtualSegment& vs = virtual_segment_array[virtual_rank[n2].second];
                        if(vs.tag_index != tag_pearson_rank[k].second)
                            continue;
                        if(virtual_rank[n2].first >= size){
                            first_write_size += size;
                            vs.tag_occupy_size[current_tag] += size;
                            object_array[object_id].virtual_segment_id = virtual_rank[n2].second;
                            return vs.first_write(object_id);
                        }else{
                            break;
                        }
                    }
                }
            }else{
                for(int n2 = 0; n2 < virtual_rank.size(); n2++){
                    if(virtual_rank[n2].first >= size){
                        if(virtual_rank[n2].second != tag_occupy_rank[n1].second)
                            continue;
                        VirtualSegment& vs = virtual_segment_array[virtual_rank[n2].second];
                        first_write_size += size;
                        vs.tag_occupy_size[current_tag] += size;
                        object_array[object_id].virtual_segment_id = virtual_rank[n2].second;
                        return vs.first_write(object_id);
                    }
                    else{
                        break;
                    }
                }
            }   
        }
    }
    // 可注释代码结尾
    

    // for(int n1 = 0; n1 < virtual_rank.size(); n1++){
    //     if(virtual_rank[n1].first >= size){
    //         VirtualSegment& vs = virtual_segment_array[virtual_rank[n1].second];
    //         first_write_size += size;
    //         vs.tag_occupy_size[current_tag] += size;
    //         object_array[object_id].virtual_segment_id = virtual_rank[n1].second;
    //         return vs.first_write(object_id);
    //     }else{
    //         break;
    //     }
    // }

    // 优先级4：遍历所有虚拟段，尝试非连续分配
    virtual_rank = vector<pair<int, int>>(), tag_occupy_rank = vector<pair<int, int>>();
    for(int n1 = 0; n1 < virtual_segment_array.size(); n1++){
        VirtualSegment& virtualSegment = virtual_segment_array[n1];
        virtual_rank.push_back({virtualSegment.get_empty(), n1});
    }
    sort(virtual_rank.begin(), virtual_rank.end(), greater<pair<int, int>>());

    // 可注释代码开头
    if(virtual_rank[0].first >= size){
        for(int n1 = 0; n1 < virtual_segment_array.size(); n1++){
            VirtualSegment& virtualSegment = virtual_segment_array[n1];
            tag_occupy_rank.push_back({virtualSegment.tag_occupy_size[current_tag], n1});
        }
        for(int n2 = 0; n2 <= M_tag_num; n2++){
            if(n2 == 0)
                tag_pearson_rank.push_back({0, 0});
            tag_pearson_rank.push_back({tag_array[obj.tag].pearson_tag[n2], n2});
        }
        sort(tag_pearson_rank.begin(), tag_pearson_rank.end());
        sort(tag_occupy_rank.begin(), tag_occupy_rank.end(), greater<pair<int, int>>());
        
        for(int n1 = 0; n1 < tag_occupy_rank.size(); n1++){
            if(tag_occupy_rank[n1].first == 0){
                for(int k = 1; k <= M_tag_num; k++){
                    if(tag_pearson_rank[k].first == 0)
                        continue;
                    for(int n2 = 0; n2 < virtual_rank.size(); n2++){
                        VirtualSegment& vs = virtual_segment_array[virtual_rank[n2].second];
                        if(vs.tag_index != tag_pearson_rank[k].second)
                            continue;
                        if(virtual_rank[n2].first >= size){
                            write_size += size;
                            vs.tag_occupy_size[current_tag] += size;
                            object_array[object_id].virtual_segment_id = virtual_rank[n2].second;
                            return vs.write(object_id);
                        }else{
                            break;
                        }
                    }
                }
            }else{
                for(int n2 = 0; n2 < virtual_rank.size(); n2++){
                    if(virtual_rank[n2].first >= size){
                        if(virtual_rank[n2].second != tag_occupy_rank[n1].second)
                            continue;
                        VirtualSegment& vs = virtual_segment_array[virtual_rank[n2].second];
                        write_size += size;
                        vs.tag_occupy_size[current_tag] += size;
                        object_array[object_id].virtual_segment_id = virtual_rank[n2].second;
                        return vs.write(object_id);
                    }
                    else{
                        break;
                    }
                }
            }   
        }
    }
    // 可注释代码结尾

    // for(int n1 = 0; n1 < virtual_rank.size(); n1++){
    //     if(virtual_rank[n1].first >= size){
    //         VirtualSegment& vs = virtual_segment_array[virtual_rank[n1].second];
    //         write_size += size;
    //         vs.tag_occupy_size[current_tag] += size;
    //         object_array[object_id].virtual_segment_id = virtual_rank[n1].second;
    //         return vs.write(object_id);
    //     }else{
    //         break;
    //     }
    // }

    // 优先级5：尝试分配到空段
    // 分配到空段的辅助函数
    auto find_valid_segments = [&](bool check_continuous) -> vector<pair<int, vector<int>>> {
        vector<pair<ActualSegment*, int>> candidates;
        
        for (auto& entry : empty_segment_array) {
            int seg_idx = entry.first;
            int disk_id = entry.second;
            ActualSegment& as = disk_array[disk_id].segment_array[seg_idx];
            
            int available = check_continuous ? as.get_first_empty() : as.get_empty();
            if (available >= size) {
                candidates.emplace_back(&as, available);
            }
        }

        // 排序所有空段的剩余空间
        sort(candidates.begin(), candidates.end(), 
            [](const auto& a, const auto& b) { return a.second > b.second; });

        // 选择三个副本的实际段
        vector<ActualSegment*> selected;
        unordered_set<int> used_disks;
        for (auto& [as, _] : candidates) {
            if (used_disks.find(as->disk_id) == used_disks.end()) {
                selected.push_back(as);
                used_disks.insert(as->disk_id);
                if (selected.size() == 3) break;
            }
        }
        // 进行连续分配或者非连续分配
        if (selected.size() == 3) {
            vector<pair<int, vector<int>>> result;
            for (auto as : selected) {
                vector<int> positions = check_continuous ? 
                    as->first_write(object_id) : as->write(object_id);
                result.emplace_back(as->disk_id, positions);
            }
            return result;
        }
        return {};
    };


    // 优先级5.1: 尝试连续分配到空段
    if (auto res = find_valid_segments(true); !res.empty()) {
        empty_first_write_size += size;
        return res;
    }

    // 优先级5.2: 尝试非连续分配到空段
    if (auto res = find_valid_segments(false); !res.empty()) {
        empty_write_size += size;
        return res;
    }
    

    // Priority 6: 都失败，报错
    throw runtime_error("Allocation failed for object " + to_string(object_id));
}

void write_action3(){
    int n_write;
    scanf("%d", &n_write);
    vector<int> write_array(n_write + 1); // 要写入的对象 id 序列
    for (int i = 1; i <= n_write; i++) {
        int id, size, tag;
        scanf("%d%d%d", &id, &size, &tag);
        object_array[id].size = size;
        object_array[id].tag = tag;
        write_array[i] = id;

        
        auto allocate_result = allocate_object(id);

        assert(allocate_result.size() == 3);
        
        for(int n1 = 0; n1 < allocate_result.size(); n1++){
            object_array[write_array[i]].disk_array[n1 + 1] = allocate_result[n1].first;
            object_array[write_array[i]].storge_data[n1 + 1].object_storge = allocate_result[n1].second;
            assert(object_array[write_array[i]].storge_data[n1 + 1].object_storge.size() == object_array[write_array[i]].size);
            assert(allocate_result[n1].first >= 1 && allocate_result[n1].first <= M_tag_num);
            for(int n2 = 0; n2 < allocate_result[n1].second.size(); n2++){
                assert(allocate_result[n1].second[n2] >= 1 && allocate_result[n1].second[n2] <= V_block_per_disk);
            }
        }
    }
    for (int i = 1; i <= n_write; i++){
        cout << write_array[i] << endl;
        for (int j = 1; j <= REP_NUM; j++){
            cout << object_array[write_array[i]].disk_array[j];
            for(int k = 0; k < object_array[write_array[i]].size; k++){
                cout << " " << object_array[write_array[i]].storge_data[j].object_storge[k]; 
            }
            printf("\n");
        }
    }

    fflush(stdout);
    return;
}