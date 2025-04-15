#include "../common.h"
using namespace std;




void Tag::calc_read_score(){
    vector<double> data(3);
    int this_t=t-1;
    for(int i=0;i<3;i++){
        for(int j=0;j<stride_length_read;j++,this_t--){
            data[3 - i - 1]+=read_size[this_t];
        }
    }
    read_score=predictNextValue(data);
    calc_t_read=t;
}

void Tag::calc_write_score(){
    vector<double> data(3);
    int this_t=t-1;
    for(int i=0;i<3;i++){
        for(int j=0;j<stride_length_write;j++,this_t--){
            data[3 - i - 1]+=write_size[this_t];
        }
    }
    write_score=predictNextValue(data);
    calc_t_write=t;
}