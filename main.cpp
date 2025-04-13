#include "common.h"

bool debug_mode = true;
bool debug_mode_mark_disk_imfromation = false;

int main()
{
    int t1 = 0;
    int time1 = 0, time2 = 0, time3 = 0, time4 = 0;
    if(debug_mode){
        freopen(".\\Data\\sample_official.in", "r", stdin);
        freopen(".\\output.txt", "w", stdout);
    }
    // if(time_step==19800) max_segment_select_size=6;
    // if(time_step==32400) max_segment_select_size=4;
    // if(time_step==48600) max_segment_select_size=6;
    // if(time_step==70200) max_segment_select_size=4;
   
    
    auto start_time = std::chrono::high_resolution_clock::now();
    pre_process();
    // allocate_disk();
    for (; t <= T_time_step_length + EXTRA_TIME; t++) {
        busy_req = std::vector<int>();
        if(t == 16462)
            int a = 1;
        if(t % FRE_PER_SLICING == 1){
            if(debug_mode){
                freopen("CON", "w", stdout);
                printf("current time step %d   %d   %d   %d\n", t, zero_request, selected_r, un_selected_r);
                std::cout << "  empty_object_read: " << empty_object_read << std::endl;
                std::cout << "  empty_request_read: " << empty_request_read << std::endl;
                std::cout << "  effective_read: " << effective_read << std::endl;
                std::cout << "  all_finish_select_size: " << all_finish_select_size << std::endl;
                std::cout << "  all_finish_request_efficiency: " << std::fixed << all_finish_request_efficiency << std::endl;
                std::cout << "  select_zero_request: " << select_zero_request << std::endl;
                std::cout << "  all_finish_select: " << all_finish_select << std::endl;
                std::cout << "  select_but_not_finish: " << select_but_not_finish << std::endl;
                std::cout << "  drop_req_num: " << drop_req_num << std::endl;
                freopen(".\\output.txt", "a+", stdout);
            }
        }
        time_step_action();
        delete_action();
        write_action3();
        read_action2();
        exchange_action();
        /*
        if(t >= 29898 && t <= 30001){
            freopen("CON", "w", stdout);
            std::cout << "disk3 head: " << disk_array[3].magnetic_head[0] << " " << disk_array[3].magnetic_head[1] << std::endl;
            freopen(".\\output.txt", "a+", stdout);
        }else if(t > 30001){
            int a = 1;
        }
        */
    }
    // clean();

    if(debug_mode){
        freopen("CON", "w", stdout);
        printf("success! ");
        int a = 1;
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        std::cout << "Analyze the total run time is : " << duration.count() / 1000 << "ms" << std::endl; 
        std::cout << "zero_request: " << zero_request << std::endl;
        std::cout << "quit_request: " << quit_request << std::endl;
        std::cout << "occupy_size: " << occupy_size << std::endl;
        std::cout << "head_idle_time: " << head_idle_time << std::endl;
        std::cout << "all_request_read_size: " << all_request_read_size << std::endl;
        std::cout << "tag_first_write_size: " << tag_first_write_size << std::endl;
        std::cout << "tag_write_size: " << tag_write_size << std::endl;
        std::cout << "first_write_size: " << first_write_size << std::endl;
        std::cout << "write_size: " << write_size << std::endl;
        std::cout << "empty_first_write_size: " << empty_first_write_size << std::endl;
        std::cout << "empty_write_size: " << empty_write_size << std::endl;
        std::cout << "all_finish_select_size: " << all_finish_select_size << std::endl;
        std::cout << "all_finish_request_efficiency: " << all_finish_request_efficiency << std::endl;
        std::cout << "all_mark: " << all_mark << std::endl;
        std::cout << "choose rate: " << (double)selected_r / ((double)un_selected_r + (double)selected_r) << std::endl;
        std::cout << "choosed mark rate: " << all_finish_request_efficiency / (((double)all_finish_select_size) / 2.0) << std::endl;
    }

    return 0;
}