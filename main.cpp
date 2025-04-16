#include "common.h"

bool debug_mode = false;
bool debug_mode_mark_disk_imfromation = false;

int main()
{
    setGlobalRandomSeed(42);
    if(debug_mode){
        freopen(".\\Data\\sample_practice_1.in", "r", stdin);
        freopen(".\\output.txt", "w", stdout);
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    pre_process();
    global_turn = 1; // 第一轮
    for (t = 1, time_step = 1; t <= T_time_step_length + EXTRA_TIME; t++,time_step++) {
        
        if(t % FRE_PER_SLICING == 1){
            if(debug_mode){
                freopen("CON", "w", stdout);
                printf("current time step %d   %d   %d\n", t, zero_request, selected_r);
                std::cout << "  empty_object_read: " << empty_object_read << std::endl;
                std::cout << "  empty_request_read: " << empty_request_read << std::endl;
                std::cout << "  effective_read: " << effective_read << std::endl;
                std::cout << "  all_finish_select_size: " << all_finish_select_size << std::endl;
                std::cout << "  all_finish_request_efficiency: " << std::fixed << all_finish_request_efficiency << std::endl;
                std::cout << "  select_zero_request: " << select_zero_request << std::endl;
                std::cout << "  all_finish_select: " << all_finish_select << std::endl;
                std::cout << "  select_but_not_finish: " << select_but_not_finish << std::endl;
                std::cout << "  drop_req_num: " << drop_req_num << std::endl;
                std::cout << "  all_mark: " << all_mark << std::endl;
                freopen(".\\output.txt", "a+", stdout);
            }
        }
        time_step_action();
        delete_action();
        write_action();
        read_action();
        exchange_action();
    }

    // 第二轮
    global_turn = 2; 
    all_mark=0;
    select_but_not_finish=0;
    drop_req_num=0;
    max_segment_select_size=4;
    segment_num=15;
    int efficient_size = ceil((double)V_block_per_disk * efficient_disk_rate);
    segment_size = ceil((double)efficient_size / (double)segment_num);
    efficient_disk_end = segment_size * segment_num;
    pre_process_2();

    // 根据上一轮的全局信息进行统计

    for (t = 1, time_step = 1; t <= T_time_step_length + EXTRA_TIME; t++,time_step++) {
        if(t % FRE_PER_SLICING == 1){
            if(debug_mode){
                freopen("CON", "w", stdout);
                printf("current time step %d   %d   %d\n", t, zero_request, selected_r);
                std::cout << "  empty_object_read: " << empty_object_read << std::endl;
                std::cout << "  empty_request_read: " << empty_request_read << std::endl;
                std::cout << "  effective_read: " << effective_read << std::endl;
                std::cout << "  all_finish_select_size: " << all_finish_select_size << std::endl;
                std::cout << "  all_finish_request_efficiency: " << std::fixed << all_finish_request_efficiency << std::endl;
                std::cout << "  select_zero_request: " << select_zero_request << std::endl;
                std::cout << "  all_finish_select: " << all_finish_select << std::endl;
                std::cout << "  select_but_not_finish: " << select_but_not_finish << std::endl;
                std::cout << "  drop_req_num: " << drop_req_num << std::endl;
                std::cout << "  all_mark: " << all_mark << std::endl;
                std::cout << "  predict_num: " << predict_num << std::endl;
                freopen(".\\output.txt", "a+", stdout);
            }
        }
        time_step_action();
        delete_action();
        write_action();
        read_action();
        exchange_action();
    }

    if(debug_mode){
        freopen("CON", "w", stdout);
        printf("success! ");
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        std::cout << "Analyze the total run time is : " << duration.count() / 1000 << "ms" << std::endl; 
        std::cout << "zero_request: " << zero_request << std::endl;
        std::cout << "quit_request: " << quit_request << std::endl;
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