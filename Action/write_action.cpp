#include "../common.h"
using namespace std;

// 分配有效区的空间
pair<int, vector<int>> efficient_allocate_object(int object_id){
    Object& obj = object_array[object_id];
    int current_tag = obj.tag;
    int size = obj.size;

    // 优先级1：对该 tag 的段尝试连续分配
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        Disk& target_disk = disk_array[n1];
        for(int n2 = 0; n2 < target_disk.segment_array.size(); n2++){
            ActualSegment& actualSegment = target_disk.segment_array[n2];
            if(actualSegment.tag_index == 0){
                continue;
            }
            if(actualSegment.tag_index != current_tag){
                continue; // 跳过不属于该 tag 的段
            }
            if(actualSegment.get_first_empty() >= size){
                tag_first_write_size += size;
                actualSegment.tag_occupy_size[current_tag] += size;
                object_array[object_id].segment_id = n2;
                return {n1, actualSegment.first_write(object_id)};
            }
        }
    }

    // 优先级2：对该 tag 的段尝试非连续分配
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        Disk& target_disk = disk_array[n1];
        for(int n2 = 0; n2 < target_disk.segment_array.size(); n2++){
            ActualSegment& actualSegment = target_disk.segment_array[n2];
            if(actualSegment.tag_index == 0){
                continue;
            }
            if(actualSegment.tag_index != current_tag){
                continue; // 跳过不属于该 tag 的段
            }
            if(actualSegment.get_empty() >= size){
                tag_write_size += size;
                actualSegment.tag_occupy_size[current_tag] += size;
                object_array[object_id].segment_id = n2;
                return {n1, actualSegment.write(object_id)};
            }
        }
    }

    if(global_turn == 1){
        // 优先级3：尝试分配一个空段给当前标签,然后连续写入
        vector<pair<int, pair<int, int>>> disk_array_for_allocate;
        for(int n1 = 1; n1 <= N_disk_num; n1++){
            int empty_segment = 0;
            int act_id;
            Disk& target_disk = disk_array[n1];
            for(int n2 = 0 ; n2 < target_disk.segment_array.size(); n2++){
                ActualSegment& actualSegment = target_disk.segment_array[n2];
                if(actualSegment.tag_index != 0){
                    continue; // 排除非空段
                }
                empty_segment++;
                if(empty_segment == 1){
                    act_id = n2;
                }
            }
            if(empty_segment > 0){
                disk_array_for_allocate.push_back({empty_segment, {n1, act_id}});
            }
        }

        sort(disk_array_for_allocate.begin(), disk_array_for_allocate.end(), greater<pair<int, pair<int, int>>>());

        for(int n1 = 0; n1 < disk_array_for_allocate.size(); n1++){
            int disk_id = disk_array_for_allocate[n1].second.first;
            int actualSegment_id = disk_array_for_allocate[n1].second.second;
            Disk& target_disk = disk_array[disk_id];
            ActualSegment& actualSegment = target_disk.segment_array[actualSegment_id];

            actualSegment.tag_occupy_size[current_tag] += size;
            object_array[object_id].segment_id = actualSegment_id;
            actualSegment.tag_index = current_tag;
            return {disk_id, actualSegment.first_write(object_id)};
        }
    }
    
    // 优先级4：对于其他 tag 的段且有当前 tag 的占用，按照当前 tag 占用大小顺序连续分配
    vector<pair<int, pair<int, int>>> actSet_array_for_occupy;
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        Disk& target_disk = disk_array[n1];
        for(int n2 = 0; n2 < target_disk.segment_array.size(); n2++){
            ActualSegment& actualSegment = target_disk.segment_array[n2];
            if(actualSegment.tag_index == 0){
                continue;
            }
            if(actualSegment.tag_index == current_tag){
                continue; // 跳过该 tag 的段
            }
            if(actualSegment.get_first_empty() >= size){
                int this_tag_size = actualSegment.tag_occupy_size[current_tag];
                if(this_tag_size == 0 && global_turn == 2){
                    continue; // 跳过没有当前 tag 占用的段
                }
                actSet_array_for_occupy.push_back({this_tag_size, {n1, n2}});
            }
        }
    }

    sort(actSet_array_for_occupy.begin(), actSet_array_for_occupy.end(), greater<pair<int, pair<int, int>>>());

    for(int n1 = 0; n1 < actSet_array_for_occupy.size(); n1++){
        int disk_id = actSet_array_for_occupy[n1].second.first;
        int actualSegment_id = actSet_array_for_occupy[n1].second.second;
        Disk& target_disk = disk_array[disk_id];
        ActualSegment& actualSegment = target_disk.segment_array[actualSegment_id];

        actualSegment.tag_occupy_size[current_tag] += size;
        object_array[object_id].segment_id = actualSegment_id;
        return {disk_id, actualSegment.first_write(object_id)};
    }

    // 优先级5：对于其他 tag 的段且没有当前 tag 的占用，按与当前 tag 的相关系数顺序连续分配
    vector<pair<double, pair<int, int>>> actSet_array_for_pearson;
    if(global_turn == 2){
        for(int n1 = 1; n1 <= N_disk_num; n1++){
            Disk& target_disk = disk_array[n1];
            for(int n2 = 0; n2 < target_disk.segment_array.size(); n2++){
                ActualSegment& actualSegment = target_disk.segment_array[n2];
                if(actualSegment.tag_index == current_tag){
                    continue; // 跳过该 tag 的段
                }
                if(actualSegment.get_first_empty() >= size){
                    int this_tag_size = actualSegment.tag_occupy_size[current_tag];
                    if(actualSegment.tag_index == 0){
                        continue;
                    }
                    if(this_tag_size != 0){
                        continue; // 跳过有当前 tag 占用的段
                    }
                    actSet_array_for_pearson.push_back(
                        {tag_array[current_tag].pearson_tag[actualSegment.tag_index], {n1, n2}}
                    );
                }
            }
        }

        sort(actSet_array_for_pearson.begin(), actSet_array_for_pearson.end(), greater<pair<double, pair<int, int>>>());

        for(int n1 = 0; n1 < actSet_array_for_pearson.size(); n1++){
            int disk_id = actSet_array_for_pearson[n1].second.first;
            int actualSegment_id = actSet_array_for_pearson[n1].second.second;
            Disk& target_disk = disk_array[disk_id];
            ActualSegment& actualSegment = target_disk.segment_array[actualSegment_id];
            
            actualSegment.tag_occupy_size[current_tag] += size;
            object_array[object_id].segment_id = actualSegment_id;
            return {disk_id, actualSegment.first_write(object_id)};
        }
    }
    
    // 优先级6：对于其他 tag 的段且有当前 tag 的占用，按照当前 tag 占用大小顺序非连续分配
    actSet_array_for_occupy = vector<pair<int, pair<int, int>>>();
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        Disk& target_disk = disk_array[n1];
        for(int n2 = 0; n2 < target_disk.segment_array.size(); n2++){
            ActualSegment& actualSegment = target_disk.segment_array[n2];
            if(actualSegment.tag_index == 0){
                continue;
            }
            if(actualSegment.tag_index == current_tag){
                continue; // 跳过该 tag 的段
            }
            if(actualSegment.get_empty() >= size){
                int this_tag_size = actualSegment.tag_occupy_size[current_tag];
                if(this_tag_size == 0 && global_turn == 2){
                    continue; // 跳过没有当前 tag 占用的段
                }
                actSet_array_for_occupy.push_back({this_tag_size, {n1, n2}});
            }
        }
    }

    sort(actSet_array_for_occupy.begin(), actSet_array_for_occupy.end(), greater<pair<int, pair<int, int>>>());

    for(int n1 = 0; n1 < actSet_array_for_occupy.size(); n1++){
        int disk_id = actSet_array_for_occupy[n1].second.first;
        int actualSegment_id = actSet_array_for_occupy[n1].second.second;
        Disk& target_disk = disk_array[disk_id];
        ActualSegment& actualSegment = target_disk.segment_array[actualSegment_id];

        actualSegment.tag_occupy_size[current_tag] += size;
        object_array[object_id].segment_id = actualSegment_id;
        return {disk_id, actualSegment.write(object_id)};
    }

    // 优先级7：对于其他 tag 的段且没有当前 tag 的占用，按与当前 tag 的相关系数顺序非连续分配
    actSet_array_for_pearson = vector<pair<double, pair<int, int>>>();
    if(global_turn == 2){
        for(int n1 = 1; n1 <= N_disk_num; n1++){
            Disk& target_disk = disk_array[n1];
            for(int n2 = 0; n2 < target_disk.segment_array.size(); n2++){
                ActualSegment& actualSegment = target_disk.segment_array[n2];
                if(actualSegment.tag_index == 0){
                    continue;
                }
                if(actualSegment.tag_index == current_tag){
                    continue; // 跳过该 tag 的段
                }
                if(actualSegment.get_empty() >= size){
                    int this_tag_size = actualSegment.tag_occupy_size[current_tag];
                    if(this_tag_size != 0){
                        continue; // 跳过有当前 tag 占用的段
                    }
                    actSet_array_for_pearson.push_back(
                        {tag_array[current_tag].pearson_tag[actualSegment.tag_index], {n1, n2}}
                    );
                }
            }
        }
    
        sort(actSet_array_for_pearson.begin(), actSet_array_for_pearson.end(), greater<pair<double, pair<int, int>>>());
    
        for(int n1 = 0; n1 < actSet_array_for_pearson.size(); n1++){
            int disk_id = actSet_array_for_pearson[n1].second.first;
            int actualSegment_id = actSet_array_for_pearson[n1].second.second;
            Disk& target_disk = disk_array[disk_id];
            ActualSegment& actualSegment = target_disk.segment_array[actualSegment_id];
            
            actualSegment.tag_occupy_size[current_tag] += size;
            object_array[object_id].segment_id = actualSegment_id;
            return {disk_id, actualSegment.write(object_id)};
        }
    }


    throw runtime_error("efficient allocation failed for object " + to_string(object_id));
}

vector<pair<int, vector<int>>> allocate_object(int object_id) {
    Object& obj = object_array[object_id];
    int current_tag = obj.tag;
    int size = obj.size;

    pair<int, vector<int>> efficient_allocate;
    // 分配有效区的空间
    if(obj.quit == false){
        efficient_allocate = efficient_allocate_object(object_id);
    }

    vector<pair<int, vector<int>>> rubbish_allocate;
    // 分配垃圾区的空间
    vector<pair<int, int>> disk_array_for_rubbish;
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        if(n1 == efficient_allocate.first && obj.quit == false){
            continue;
        }
        Disk& target_disk = disk_array[n1];
        disk_array_for_rubbish.push_back({target_disk.rubbish_stack.size(), n1});
    }

    sort(disk_array_for_rubbish.begin(), disk_array_for_rubbish.end(), greater<pair<int, int>>());

    for(int n1 = 0; n1 < disk_array_for_rubbish.size(); n1++){
        Disk& target_disk = disk_array[disk_array_for_rubbish[n1].second];
        if(target_disk.rubbish_stack.size() < size){
            continue; // 跳过剩余空间不足的情况 
        }
        pair<int, vector<int>> allocate_item = {disk_array_for_rubbish[n1].second, {}};
        for(int n2 = 0; n2 < size; n2++){
            int block_index = target_disk.rubbish_stack.top();

            disk[disk_array_for_rubbish[n1].second][block_index] = object_id;

            allocate_item.second.push_back(block_index);

            target_disk.rubbish_stack.pop();
        }
        rubbish_allocate.push_back(allocate_item);
        if(rubbish_allocate.size() >= 2 && obj.quit == false){
            break;
        }else if(rubbish_allocate.size() >= 3 && obj.quit == true){
            break;
        }
    }


    if(rubbish_allocate.size() < 2 && obj.quit == false){
        throw runtime_error("rubbish allocation failed for object " + to_string(object_id));
    }else if(rubbish_allocate.size() < 3 && obj.quit == true){
        throw runtime_error("rubbish allocation failed for object " + to_string(object_id));
    }
    
    if(obj.quit == false){
        return {efficient_allocate, rubbish_allocate[0], rubbish_allocate[1]};
    }else{
        return {rubbish_allocate[0], rubbish_allocate[1], rubbish_allocate[2]};
    }
}

void write_action(){
    std::ostringstream oss;

    int n_write;
    if(global_turn==1){
        scanf("%d", &n_write);
    }else{
        n_write=write_record[time_step].size();
    }
    
    vector<int> write_array(n_write + 1); // 要写入的对象 id 序列
    for (int i = 1; i <= n_write; i++) {
        int id, size, tag;
        if(global_turn==1){
            scanf("%d%d%d", &id, &size, &tag);
        }else{
            id=write_record[time_step][i-1];
            size=object_array[id].size;
            tag=object_array[id].tag;
        }
        max_object_id = max(max_object_id, id);
        object_array[id].size = size;
        if(global_turn==2) assert(tag!=0);
        if(tag==0) {
            /*
            if(t>=7000){
                vector<pair<double,int>> p;
                for(int i=1;i<=M_tag_num;i++){
                    if(time_step != tag_array[i].calc_t_write){
                        tag_array[i].calc_write_score();
                    }
                    p.emplace_back(tag_array[i].write_score, i);
                }
                possibility = p;
            }else{
                vector<pair<double,int>> p;
                for(int i=1;i<=M_tag_num;i++){
                    p.emplace_back(tag_array[i].all_wirte_size, i);
                }
                possibility = p;
            }
            */
            object_array[id].true_tag=false;
            tag=predictObject(possibility);
        }    
        else{
            tag_array[tag].write_size[time_step]+=object_array[id].size;
            tag_array[tag].all_wirte_size += object_array[id].size;
        }
        object_array[id].tag = tag;
        write_record[time_step].push_back(id);
        write_array[i] = id;
        auto allocate_result = allocate_object(id);
        

        assert(allocate_result.size() == 3);
        
        for(int n1 = 0; n1 < allocate_result.size(); n1++){
            object_array[write_array[i]].disk_array[n1 + 1] = allocate_result[n1].first;
            object_array[write_array[i]].storge_data[n1 + 1] = allocate_result[n1].second;
            assert(object_array[write_array[i]].storge_data[n1 + 1].size() == object_array[write_array[i]].size);
            assert(allocate_result[n1].first >= 1 && allocate_result[n1].first <= M_tag_num);
            for(int n2 = 0; n2 < allocate_result[n1].second.size(); n2++){
                assert(allocate_result[n1].second[n2] >= 1 && allocate_result[n1].second[n2] <= V_block_per_disk);
            }
        }
    }
    for (int i = 1; i <= n_write; i++){
        if(global_turn == 1){
            cout << write_array[i] << endl;
            for (int j = 1; j <= REP_NUM; j++){
                cout << object_array[write_array[i]].disk_array[j];
                for(int k = 0; k < object_array[write_array[i]].size; k++){
                    cout << " " << object_array[write_array[i]].storge_data[j][k]; 
    
                }
                cout << "\n";
            }
        }else{
            oss << write_array[i] << endl;
            for (int j = 1; j <= REP_NUM; j++){
                oss << object_array[write_array[i]].disk_array[j];
                for(int k = 0; k < object_array[write_array[i]].size; k++){
                    oss << " " << object_array[write_array[i]].storge_data[j][k]; 
    
                }
                oss << "\n";
            }
        }

    }

    if(global_turn == 2){
        round2_write_track[t] = oss.str();
    }
    fflush(stdout);
    return;
}