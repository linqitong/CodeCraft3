#include "../common.h"
using namespace std;

int Disk::get_read_time(int read_num,int head_id) {
    int total_read = have_read_time[head_id] + read_num;
    int index = have_read_time[head_id];
    int all_time = 0;
    while (index < total_read) {
        if (index < 7) {
            all_time += Data[index];
            index++;
        } else {
            // 处理所有剩余读取次数并跳出循环
            int remaining = total_read - index;
            all_time += remaining * 16;
            break;
        }
    }
    return all_time;
}


void Disk::head_advance(int length, std::string way,int head_id){

    
    if (magnetic_head[head_id] < 1 || magnetic_head[head_id] > V_block_per_disk || length >= V_block_per_disk || length <= 0) {
        throw std::invalid_argument("Invalid arg for head_advance.");
    }

    // 计算新位置
    int newPosition = magnetic_head[head_id] + length;
    int current_turn_length = min(length, V_block_per_disk - magnetic_head[head_id] + 1);

    if(way == "jump"){
        jump_block += current_turn_length;
        jump_num++;
    }else if(way == "read"){
        read_block += current_turn_length;
    }else if(way == "pass"){
        pass_block += current_turn_length;
    }

    // 处理循环
    if (newPosition > V_block_per_disk) {
        newPosition = newPosition - V_block_per_disk;

        if(debug_mode && debug_mode_mark_disk_imfromation){
            string addr = ".\\disk\\" + to_string(disk_id) + ".txt";
            freopen(addr.c_str(), "a+", stdout);
            cout << "time" << t << " turn:" << turn_num + 1
                << " j:" << jump_block << " r:" << read_block << " p:" << pass_block
                << " j_num:" << jump_num << endl;
            freopen(".\\output.txt", "a+", stdout);
        }
        jump_block = 0;
        read_block = 0;
        pass_block = 0;
        jump_num = 0;
        turn_num += 1;

        if(way == "jump"){
            jump_block += newPosition - 1;
        }else if(way == "read"){
            read_block += newPosition - 1;
        }else if(way == "pass"){
            pass_block += newPosition - 1;
        }
    }

    magnetic_head[head_id] = newPosition;

    return;
}

bool Disk::is_target_actual(int index, int head_id){
    for(int n2 = 0; n2 < target_actual_array[head_id].size(); n2++){
        if(index == target_actual_array[head_id][n2]){
            return true;
        }
    }
    return false;
}