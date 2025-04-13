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