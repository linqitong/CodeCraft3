#include "../common.h"
using namespace std;

std::vector<std::string> round2_quit_track;
// std::ofstream outFile("D:\\Project\\2025-code-craft-preliminary\\code\\CodeCraft3\\output3.txt");

void round2_quit_not_finish(){
    round2_quit_track = std::vector<std::string>(T_time_step_length + EXTRA_TIME + 1);
    vector<int> quit_num = vector<int>(T_time_step_length + EXTRA_TIME + 1);
    for(int n1 = 1; n1 <= max_request_id; n1++){
        if(round2_finish_set.find(n1) == round2_finish_set.end()){
            int time = request_array[n1].recieve_time;
            quit_num[time] += 1;
        }
    }
    for(int n1 = 1; n1 <= T_time_step_length + EXTRA_TIME; n1++){
        round2_quit_track[n1] += to_string(quit_num[n1]) + "\n";
    }
    for(int n1 = 1; n1 <= max_request_id; n1++){
        if(round2_finish_set.find(n1) == round2_finish_set.end()){
            int time = request_array[n1].recieve_time;
            round2_quit_track[time] += to_string(n1) + "\n";
        }
    }
}

void round2_interact_action(){
    round2_quit_not_finish();

    for (t = 1, time_step = 1; t <= T_time_step_length + EXTRA_TIME; t++,time_step++) {
        int this_time_step;
        if(debug_mode && global_turn == 2){

        }else{
            scanf("%*s%d", &this_time_step);
            if(this_time_step != time_step)
            assert(this_time_step == time_step);
        }

        printf("TIMESTAMP %d\n", time_step);
        // outFile << "TIMESTAMP " << time_step << "\n"; 
        fflush(stdout);
            
        cout << round2_delete_track[t] << std::flush;
        // outFile << round2_delete_track[t] << std::flush;
        fflush(stdout);
       
        cout << round2_write_track[t] << std::flush;
        // outFile << round2_write_track[t] << std::flush;
        fflush(stdout);

        cout << round2_head_track[t] << std::flush;
        // outFile << round2_head_track[t] << std::flush;
        fflush(stdout);

        cout << round2_finish_track[t] << std::flush;
        // outFile << round2_finish_track[t] << std::flush;;
        fflush(stdout);

        cout << round2_quit_track[t] << std::flush;
        // outFile << round2_quit_track[t] << std::flush;
        fflush(stdout);
        
        if(time_step % 1800 == 0){
            cout << round2_recycle_track[t] << std::flush;
            // outFile << round2_recycle_track[t] << std::flush;
            fflush(stdout);
        }

    }
}