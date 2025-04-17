#include "../common.h"
using namespace std;

std::vector<std::string> round2_quit_track;


void round2_quit_not_finish(){
    round2_quit_track = std::vector<std::string>(T_time_step_length + EXTRA_TIME + 1);
    for(int n1 = 1; n1 <= max_request_id; n1++){
        if(round2_finish_set.find(n1) == round2_finish_set.end()){
            int time = request_array[n1].recieve_time;
            round2_quit_track[time] += to_string(n1) + "\n";
        }
    }
}

void round2_interact_action(){

    for (t = 1, time_step = 1; t <= T_time_step_length + EXTRA_TIME; t++,time_step++) {
        int this_time_step;
        if(debug_mode && global_turn == 2){

        }else{
            scanf("%*s%d", &this_time_step);
            if(this_time_step != time_step)
            assert(this_time_step == time_step);
        }

        printf("TIMESTAMP %d\n", time_step);
        fflush(stdout);
            
        cout << round2_delete_track[t];
        fflush(stdout);
       
        cout << round2_write_track[t];
        fflush(stdout);

        cout << round2_head_track[t];
        fflush(stdout);

        cout << round2_finish_track[t];
        fflush(stdout);

        cout << round2_quit_track[t];
        fflush(stdout);
        
        if(time_step % 1800 == 0){
            cout << round2_recycle_track[t];
            fflush(stdout);
        }

    }
}