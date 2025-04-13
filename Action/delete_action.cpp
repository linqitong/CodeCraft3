#include "../common.h"
using namespace std;

void delete_action()
{
    // 记录要删除的对象
    int n_delete;
    int abort_num = 0; // 还没有完成的请求总数

    static int _id[MAX_OBJECT_NUM]; // 要删除的对象 id 序列

    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);
        abort_num += object_array[_id[i]].wait_request_set.size();
    }

    printf("%d\n", abort_num);

    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        for (const auto& elem : object_array[_id[i]].wait_request_set) {
            printf("%d\n", elem);
            // wait_request_set.erase(elem);
            request_array[elem].clear_read_information();
        }

        // 删除请求
        quit_request += object_array[id].wait_request_set.size();
        object_array[id].wait_request_set = set<int>();
        int virtual_id = object_array[id].virtual_segment_id;
        if(virtual_id != -1){
            VirtualSegment& target_virtual_segment = virtual_segment_array[virtual_id];
            target_virtual_segment.tag_occupy_size[object_array[id].tag] -= object_array[id].size;
        }
        
        // 清除对象的磁盘块信息并修改 allocate_disk
        for (int j = 1; j <= REP_NUM; j++) {
            int disk_id = object_array[id].disk_array[j];
            int first_block = object_array[id].storge_data[j].object_storge[0];
            ActualSegment& target_actual_segment = disk_array[disk_id].segment_array[(first_block - 1) / segment_size];

            for(int n1 = object_array[id].size - 1; n1 >= 0; n1--){
                disk[disk_id] // 清除磁盘块信息
                    [object_array[id].storge_data[j].object_storge[n1]] = 0;
                target_actual_segment.all_size--;
                while(target_actual_segment.first_write_index - 1>= 0 
                    && disk[disk_id][target_actual_segment.begin_index + target_actual_segment.first_write_index - 1] == 0){
                        target_actual_segment.first_write_index--;
                    }
            }
        }
    }

    fflush(stdout);
}


