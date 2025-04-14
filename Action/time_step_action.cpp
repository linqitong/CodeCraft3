#include "../common.h"
using namespace std;

void time_step_action()
{
    scanf("%*s%d", &time_step);
    printf("TIMESTAMP %d\n", time_step);

    time_segment_index = ((time_step) / FRE_PER_SLICING) + 1;
    G_token_per_time_step = G + g[((time_step - 1) / FRE_PER_SLICING) + 1];
    if(time_step == 23401){
        int a = 1;
    }

    if((time_step) % FRE_PER_SLICING == 0 || time_step == 1){
        select_VirtualSegment = set<int>();
        set<int> last_select_VirtualSegment = set<int>();
        // 清空所有磁盘上一次分配的段
        for(int n1 = 1; n1 <= N_disk_num; n1++){
            disk_array[n1].target_actual_array = vector<vector<int>>(MAGNERIC_HEAD_NUM);
        }

        while(true){
            // 所有磁盘循环，依次选择实际段
            // 1、选择实际段为可选实际段 read 最多的
            // 2、该实际段 read >= min_read_shold
            // 3、该磁盘已经选择的实际段数 <= max_segment_select_size
            // 4、该实际段对应的虚拟段不能被选择过
            // 5、不考虑空段
            for(int n1 = 1; n1 <= N_disk_num; n1++){
                for(int magnetic_head_id=0;magnetic_head_id<MAGNERIC_HEAD_NUM;magnetic_head_id++){
                    Disk& target_disk = disk_array[n1];
                    vector<ActualSegment>& segment_array = target_disk.segment_array;
    
                    if(target_disk.target_actual_array[magnetic_head_id].size() >= max_segment_select_size){
                        continue; // 当前磁盘选择的段已经达到上限，让下一个磁盘选
                    }
    
                    vector<pair<int, int>> array;
                    for(int n2 = 0; n2 < segment_array.size(); n2++){
                        int virtual_id = segment_array[n2].virtual_id;
                        if(virtual_id != -1 // 不为空段
                        && select_VirtualSegment.find(virtual_id) == select_VirtualSegment.end() // 未被选择
                        ){ 
                            VirtualSegment& virtualSegment = virtual_segment_array[virtual_id];
                            double virtualSegment_mark = virtualSegment.get_mark();
                            if(virtualSegment_mark
                                >= min_read_shold){ // 该段 read >= min_read_shold
                                // 添加进候选名单
                                array.push_back({virtualSegment_mark, n2});
                            }
                        } 
                    }
    
                    sort(array.begin(), array.end(), greater<pair<int, int>>());
    
                    if(array.size() == 0){ // 如果没有符合要求的段，就跳过
                        continue;
                    }
    
                    target_disk.target_actual_array[magnetic_head_id].push_back(array[0].second);
    
                    int virtual_id = segment_array[array[0].second].virtual_id;
                    select_VirtualSegment.insert(virtual_id);
                    if(time_segment_index == 20 && virtual_id == 67){
                        int a = 1;
                    }
                }
            }
            // 判定当前选择的虚拟段和上一次的虚拟段是否相同
            // 如果相同，就代表该次循环没有有效分配，就退出
            if(select_VirtualSegment == last_select_VirtualSegment){
                break;
            }
            last_select_VirtualSegment = select_VirtualSegment;
        }

        for(int n1 = 1; n1 <= N_disk_num; n1++){
            Disk& target_disk = disk_array[n1];
            // sort()
            
            sort(target_disk.target_actual_array[0].begin(), target_disk.target_actual_array[0].end());
            sort(target_disk.target_actual_array[1].begin(), target_disk.target_actual_array[1].end());
        }

        for(int n1 = 0; n1 < virtual_segment_array.size(); n1++){
            if(select_VirtualSegment.find(n1) != select_VirtualSegment.end()){
                continue;
            }
            // virtual_segment_array[n1].quit_all_request();
        }

        if(debug_mode){
            freopen("CON", "w", stdout);
            for(int n1 = 1; n1 <= N_disk_num; n1++){
                for(int magnetic_head_id=0; magnetic_head_id< MAGNERIC_HEAD_NUM; magnetic_head_id++){
                    std::cout << "  disk" << n1 << " head" << magnetic_head_id << " allocate" ;
                    for(int n2 = 0; n2 < disk_array[n1].target_actual_array[magnetic_head_id].size(); n2++){
                        std::cout << " " << disk_array[n1].target_actual_array[magnetic_head_id][n2] ;
                    }
                    std::cout << std::endl;
                    std::cout << "quit_num1:" << quit_num1 << std::endl;
                }
                
            }
            freopen(".\\output.txt", "a+", stdout);
        }
    }

    fflush(stdout);
}