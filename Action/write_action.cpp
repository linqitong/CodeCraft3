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
            if(actualSegment.tag_index != current_tag){
                continue; // 跳过不属于该 tag 的段
            }
            if(actualSegment.get_empty() >= size){
                tag_first_write_size += size;
                actualSegment.tag_occupy_size[current_tag] += size;
                object_array[object_id].segment_id = n2;
                return {n1, actualSegment.write(object_id)};
            }
        }
    }

    // 优先级3：对于其他 tag 的段且有当前 tag 的占用，按照当前 tag 占用大小顺序连续分配
    // vector<pair<>>

    // 优先级4：对于其他 tag 的段且没有当前 tag 的占用，按与当前 tag 的相关系数顺序连续分配

    // 优先级5：对于其他 tag 的段且有当前 tag 的占用，按照当前 tag 占用大小顺序非连续分配

    // 优先级6：对于其他 tag 的段且没有当前 tag 的占用，按与当前 tag 的相关系数顺序非连续分配

    throw runtime_error("efficient allocation failed for object " + to_string(object_id));
}

vector<pair<int, vector<int>>> allocate_object(int object_id) {
    Object& obj = object_array[object_id];
    int current_tag = obj.tag;
    int size = obj.size;

    // 分配有效区的空间
    pair<int, vector<int>> efficient_allocate = efficient_allocate_object(object_id);

    vector<pair<int, vector<int>>> rubbish_allocate;
    // 分配垃圾区的空间
    vector<pair<int, int>> disk_array_for_rubbish;
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        if(n1 == efficient_allocate.first){
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
        if(rubbish_allocate.size() >= 2){
            break;
        }
    }

    if(rubbish_allocate.size() < 2){
        throw runtime_error("rubbish allocation failed for object " + to_string(object_id));
    }
    
    return {efficient_allocate, rubbish_allocate[0], rubbish_allocate[1]};

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

    // Priority 6: 都失败，报错
    throw runtime_error("Allocation failed for object " + to_string(object_id));
}

void write_action(){
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
            object_array[write_array[i]].storge_data[n1 + 1] = allocate_result[n1].second;
            assert(object_array[write_array[i]].storge_data[n1 + 1].size() == object_array[write_array[i]].size);
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
                cout << " " << object_array[write_array[i]].storge_data[j][k]; 
            }
            printf("\n");
        }
    }

    fflush(stdout);
    return;
}