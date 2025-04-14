#include "../common.h"
using namespace std;


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


void calc_pearson()
{
    for(int i = 1; i <= M_tag_num; i++){
        for(int j = 0; j <= M_tag_num; j++){
            if(j == 0){
                tag_array[i].pearson_tag.push_back(0.0);
                continue;
            }
            int length = (T_time_step_length - 1) / FRE_PER_SLICING + 1;
            double sumA = 0, sumB = 0, sumAB = 0, sumA2 = 0, sumB2 = 0;
            for(int k = 1; k <= length; k++){
                sumA += tag_array[i].fre_read[k];
                sumB += tag_array[j].fre_read[k];
            }
            double averageA = sumA / length, averageB = sumB / length;
            for(int k = 1; k <= length; k++){
                sumAB += (tag_array[i].fre_read[k] - averageA) * (tag_array[j].fre_read[k] - averageB);
                sumA2 += (tag_array[i].fre_read[k] - averageA) * (tag_array[i].fre_read[k] - averageA);
                sumB2 += (tag_array[j].fre_read[k] - averageB) * (tag_array[j].fre_read[k] - averageB);
            }
            double numerator = sumAB;
            double denominator = std::sqrt(sumA2) * std::sqrt(sumB2);
            tag_array[i].pearson_tag.push_back(numerator / denominator);
        }
    }
}


// 预处理
void pre_process(){

    // 读取全局参数
    scanf("%d%d%d%d%d%d", &T_time_step_length, &M_tag_num, &N_disk_num, &V_block_per_disk, &G,&K_max_exchange_block);

    int efficient_size = ceil((double)V_block_per_disk * efficient_disk_rate);
    segment_size = ceil((double)efficient_size / (double)segment_num);
    efficient_disk_end = segment_size * segment_num;

    // 所有磁盘垃圾栈初始化
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        disk_array[n1].rubbish_stack = stack<int>();
        for(int n2 = V_block_per_disk; n2 > efficient_disk_end; n2--){
            disk_array[n1].rubbish_stack.push(n2);
        }
    }

    // 初始化时间片信息
    for (int i = 1; i <= M_tag_num; i++) {
        tag_array[i].fre_del = vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2);
        tag_array[i].fre_write = vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2);
        tag_array[i].fre_read = vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2);
        g = vector<int>((T_time_step_length - 1 + 105) / FRE_PER_SLICING + 2);
    }

    // 删除时间片信息
    for (int i = 1; i <= M_tag_num; i++) {
        for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%d", &tag_array[i].fre_del[j]);
        }
    }
    // 写入时间片信息
    for (int i = 1; i <= M_tag_num; i++) {
        for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%d", &tag_array[i].fre_write[j]);
            tag_array[i].all_write_size += tag_array[i].fre_write[j];
            total_write += tag_array[i].fre_write[j];
        }
    }

    // 读取时间片信息
    for (int i = 1; i <= M_tag_num; i++) {
        for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%d", &tag_array[i].fre_read[j]);
        }
    }

    
    for (int j = 1; j <= (T_time_step_length - 1 + 105) / FRE_PER_SLICING + 1; j++) {
        scanf("%d", &g[j]);
    }

    // 计算 tag_write，tag_content，tag_read
    vector<int> b = vector<int>(5);
    tag_write = vector<vector<int>>(M_tag_num + 1, vector<int>(3, 0));
    tag_content = vector<vector<int>>(M_tag_num + 1, vector<int>(3, 0));
    tag_read = vector<vector<long long>>(M_tag_num + 1, vector<long long>(3, 0));
    for (int i = 1; i <= M_tag_num; i++) {
        int time_segment = ((T_time_step_length - 1) / FRE_PER_SLICING + 2) / 3;
        int content = 0;
        for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
            content += tag_array[i].fre_write[j];
            double a = tag_array[i].fre_del[j] * 2.5;
            content -= a;
            if(j / time_segment > 2){
                continue;
            }
            tag_write[i][j / time_segment] += tag_array[i].fre_write[j];
            tag_write[i][j / time_segment] -= tag_array[i].fre_del[j];
            tag_read[i][j / time_segment] += tag_array[i].fre_read[j];
            tag_content[i][j / time_segment] = max(tag_content[i][j / time_segment], content);
        }
    }
    // vector<int> b = vector<int>(5);
    // 初始化 Disk 信息 和 disk_assignable_actual_num
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        Disk& target_disk = disk_array[n1];
        target_disk.disk_id = n1;
        disk_assignable_actual_num[n1] = V_block_per_disk / segment_size;
        for(int n2 = 0; n2 < disk_assignable_actual_num[n1]; n2++){
            ActualSegment actualSegment = ActualSegment();
            actualSegment.disk_id = n1;
            actualSegment.begin_index = n2 * segment_size + 1;
            actualSegment.segment_length = segment_size;
            target_disk.segment_array.push_back(actualSegment);
        }
        if(V_block_per_disk % segment_size != 0){
            ActualSegment actualSegment = ActualSegment();
            actualSegment.disk_id = n1;
            actualSegment.begin_index = disk_assignable_actual_num[n1] * segment_size + 1;
            actualSegment.segment_length = V_block_per_disk % segment_size;
            target_disk.segment_array.push_back(actualSegment);
            empty_segment_array.push_back({target_disk.segment_array.size() - 1, n1});
        }
    }
    allocate_segments();
    calc_pearson();
    printf("OK\n");
    fflush(stdout);
}

