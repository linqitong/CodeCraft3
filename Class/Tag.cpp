#include "../common.h"
using namespace std;




void Tag::calc_read_score(){
    vector<double> data(3);
    int this_t=t-1;
    for(int i=0;i<3;i++){
        for(int j=0;j<stride_length_read and this_t>0;j++,this_t--){
            data[3 - i - 1]+=read_size[this_t];
        }
    }
   
    read_score=predictNextValue(data);
    calc_t_read=t;
}

double predictNextValue_2(const std::vector<double>& y) {
    if (y.size() < 3) {
        throw std::invalid_argument("输入数组长度不足，需要至少3个数据点");
    }
    for (size_t i = 0; i < y.size(); ++i) {
        if (y[i] < 0) {
            throw std::invalid_argument("all data must >= 0");
        }
    }
    
    // 将数据转为对数空间以确保预测值为正
    double log_y0 = std::log(y[0]);
    double log_y1 = std::log(y[1]);
    double log_y2 = std::log(y[2]);

    double zeroDeriv = log_y2;
    double firstDeriv = (log_y2 - log_y0) / 2;
    double secondDeriv = log_y2 - log_y0 + 2 * log_y1;

    double predicted_log;
    if (Derivatives == 0)
        predicted_log = zeroDeriv;
    else if (Derivatives == 1)
        predicted_log = zeroDeriv + firstDeriv;
    else if (Derivatives == 2)
        predicted_log = zeroDeriv + firstDeriv + secondDeriv / 2;
    else
        throw std::logic_error("Derivatives 值不在预期范围内");

    // 返回指数变换后的预测值，确保为正
    return std::exp(predicted_log);
}

double predictNextValue_3(const vector<double>& y) {
   
   
    double zeroDeriv=y[2],firstDeriv=(y[2]-y[0])/2, secondDeriv=y[2]-y[0]+2*y[1];
    return zeroDeriv;
    if(Derivatives==0) return zeroDeriv;
    if(Derivatives==1) return zeroDeriv + firstDeriv;
    if(Derivatives==2) return zeroDeriv + firstDeriv + secondDeriv / 2;

    
}

void Tag::calc_write_score(){
    vector<double> data(3);
    int this_t=t-1;
    int write_stride_length_write = 200;;
    for(int i = 0;i < 3; i++){
        for(int j = 0; j < write_stride_length_write; j++, this_t--){
            data[3 - i - 1]+=write_size[this_t];
        }
    }
    write_score = predictNextValue_3(data);
    if(write_score < 0){
        int a = 1;
    }
    calc_t_write=t;
}