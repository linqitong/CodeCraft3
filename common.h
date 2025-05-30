#include <bits/stdc++.h>

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (600)
#define EXTRA_TIME (105)
#define MAX_TAG_NUM (16 + 1)
#define MAX_OBJECT_SIZE (5)
#define MAGNERIC_HEAD_NUM (2)
#define MAX_TIME (86400)

extern int G;
extern int T_time_step_length; // 1 ~ 86400，时间片编号 1 ~ T+105
extern int M_tag_num; // 1 ~ 16，标签编号 1 ~ 16
extern int N_disk_num; // 3 ~ 10
extern int V_block_per_disk; // 1 ~ 16384，存储单元编号 1 ~ V
extern int G_token_per_time_step; // 64 ~ 1000
extern int K_max_exchange_block; //代表每次垃圾回收事件每个硬盘最多的交换存储单元的操作次数。0~100
extern int max_object_id;
extern int max_request_id;
extern int predict_window; // 预测窗口大小
extern int K2;

extern std::set<int> size_of_tag[6];

class ExchangeBlock{
    public:
    std::vector<std::pair<int, int>> exchange_block; // 交换块的数组
};
        
class ActualSegment{
    public:
    int tag_index = 0; // 分配该段的 tag 索引
    int tag_occupy_size[18] = {0}; // 不同tag在该段的占用大小
    int disk_id; // 所在磁盘 id
    int begin_index; // 该段起始位置，该段在磁盘上的索引 = begin_index % segment_size
    int segment_length; // 段大小
    int first_write_index = 0; // 连续写入位置
    int all_size = 0; // 存储信息总大小
    int virtual_id = -1; // 对应虚拟段，如果有的话
    std::vector<int> first_write(int object_id); // 回传写入位置，连续空间不足会报错
    std::vector<int> write(int object_id); // 回传写入位置，空间不足会报错
    int get_first_empty(); // 得到首次写入的剩余空间
    int get_empty(); // 得到总大小的剩余空间
    int get_request_num();
    int get_score(); // 计算该段的分数
    int request_size = 0; // 该实际段当前的等待请求数
    long long all_request_wait_time=0;//该段所有请求的总等待时长
    std::set<int> object_set;
    std::vector<std::pair<int, int>> get_read_rank();
};

class Disk{
    public:
        std::vector<int> magnetic_head=std::vector<int>(MAGNERIC_HEAD_NUM,1); // 磁头位置 1 ~ V
        
        std::vector<int> have_read_time=std::vector<int>(MAGNERIC_HEAD_NUM);  // 已经连续读取的次数

        int get_read_time(int read_num ,int head_id); // 根据接下来的读取次数计算时间

        void head_advance(int length, std::string way,int magnetic_head_id); // 磁头前进，仅记录信息和改变 magnetic_head

        std::vector<int> current_G_token=std::vector<int>(MAGNERIC_HEAD_NUM); // 当前时间片消耗的时间数

        int disk_id; // 磁盘编号

        std::vector<std::string> order=std::vector<std::string>(MAGNERIC_HEAD_NUM); // 当前时间片的指令序列

        std::vector<ActualSegment> segment_array; // 实际段序列;

        // 以下仅作为记录信息而用
        int turn_num = 0; // 已旋转次数
        int jump_block = 0; // jump 的磁盘块数
        int read_block = 0; // read 的磁盘块数
        int pass_block = 0; // pass 的磁盘块数
        int jump_num = 0; // jump 的次数

        int default_jump = 0; // 该磁头默认 jump 位置
        int assigned_segment = 0; // 该磁盘分配的虚拟段数

        std::stack<int> rubbish_stack = std::stack<int>(); // 垃圾栈

        std::vector<int> current_time_segment = std::vector<int>(MAGNERIC_HEAD_NUM); // 当前正在执行的命令的时间段，初始是第一段
        std::vector<std::vector<int>> target_actual_array = std::vector<std::vector<int>>(MAGNERIC_HEAD_NUM); // 当前时间段分配给该 disk 的 actual 序列

        bool is_target_actual(int index, int head_id); // 判断对应段索引是否在 target_actual_array 中

};

class Object{
public:
    int tag; // 标签
    bool true_tag=true;//标记标签是否为真值
    int right_tag = 0;
    int size;
    int load_time;
    int life_time=0;
    int  read_times=0;// 读取次数 
    std::set<int> wait_request_set; // 该物品还没有执行完毕的请求的编号
    std::vector<int> storge_data[REP_NUM + 1]; // 该对象三个副本存储的位置
    int virtual_segment_id = -1; // 【已被弃用】该对象对应的虚拟段编号
    int segment_id = -1; // 该对象对应的段编号
    int disk_array[REP_NUM + 1]; // 该对象三个副本存储的磁盘 id 
    int quit = false;
    void check_finish(); // 检查 wait_request_set 是否部分已完成
    void quit_all_request();
    
};

class Request{
public:
    int request_id;
    int object_id = 0;
    int recieve_time;
    bool select = false;
    bool read[MAX_OBJECT_SIZE]; // 记录该请求每个磁盘块的读取情况
    int read_num = 0; // 已经读取的磁盘块数量 
    bool read_block(int index);
    void finish();
    void clear_read_information(); // 清空记录的 disk_block_request 信息

};

class Tag{
public:
    std::vector<int> fre_del; // 多个时间段内删除的对象大小之和 每个数据的范围 0 ~ 2^32 - 1
    std::vector<int> fre_write; // 多个时间段内写入的对象大小之和
    std::vector<int> fre_read; // 多个时间段内读取的对象大小之和
    std::vector<double> pearson_tag;
    std::vector<int> read_size=std::vector<int>(MAX_TIME+106);//记录每帧读取数量
    std::vector<int> write_size=std::vector<int>(MAX_TIME+106);//记录每帧写入数量
    std::pair<int, double> average_size;
    int all_write_size;
    int calc_t_read=0;
    int calc_t_write=0;
    double read_score=0,write_score=0;
    void calc_read_score();
    void calc_write_score();
    int round2_high_request_num;
    int round2_low_request_num;
    // std::vector<int> virtual_segment = std::vector<int>(); // 分配给该 tag 的虚拟段索引,从 0 开始
};
extern int stride_length_read;
extern int stride_length_write;
extern int stride_num;
extern int global_turn;
extern double round2_low_threshold;
extern double round2_high_threshold;

extern std::set<int> has_been_predicted;//已经预测出真值的obj_id 
extern int quit_num1;

extern Disk disk_array[MAX_DISK_NUM]; // 所有的磁盘数据
extern Object object_array[MAX_OBJECT_NUM]; // 所有的对象数据
extern Request request_array[MAX_REQUEST_NUM]; // 所有的请求数据
extern Tag tag_array[MAX_TAG_NUM+1]; // 所有的标签数据
extern std::vector<std::pair<double,int>> possibility;

extern int pearson_sample_interval;//对象与各tag计算相似度时采样间隔
extern int time_step; // 当前时间阶段序号

extern int total_fre_net_demand; // 该时间段净需求综合
extern int time_segment;
extern std::vector<int> g;
extern int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
extern int disk_block_index[MAX_DISK_NUM][MAX_DISK_SIZE]; // 记录磁盘对应位置是对应物品的第几块，0 ~ size - 1
extern int disk_block_request[MAX_DISK_NUM][MAX_DISK_SIZE];

extern std::vector<int> finish_request; // 当前时间片完成的请求
extern std::vector<int> busy_req;       // 当前时间片放弃的请求

extern int Data[7];

extern int t; // 表示全局时间步

extern int quit_request; // 放弃请求数

extern int total_write;

extern int zero_request; 

extern int head_idle_time; // 磁头空闲时间

extern int all_request_read_size; // 所有请求 read 的总大小

extern int max_think_num_for_empty_read; // 判断空读时，考虑的最大块数

extern bool debug_mode;
extern bool debug_mode_mark_disk_imfromation; // 判断是否记录磁盘信息，只在 debug 模式下有效

extern int drop_req_num;//被丢弃的请求数量 
extern int right_predict;
extern int need_predict;

extern long long tag_first_write_size;
extern long long tag_write_size;
extern long long first_write_size;
extern long long write_size;
extern long long empty_first_write_size;
extern long long empty_write_size;
extern int min_read_shold; 
extern int max_segment_select_size;
extern int segment_size; // 默认段大小
extern int segment_num; // 段数目 
extern int selected_r; // 得到分配的请求
extern int un_selected_r; // 未得到分配的请求
extern int empty_object_read; // 空读数量
extern int empty_request_read; // 空请求 read 数量
extern int effective_read; // 有效读数量
extern double all_finish_request_efficiency; // 总体 request 有效性
extern int all_finish_select_size; // 被完成的选中请求大小
extern int all_select_efficiency; // 
extern int select_zero_request; // 被选中的零请求
extern int all_finish_select; // 被完成的选中请求数
extern double all_mark; // 估计分数
extern int time_segment_index; // 当前所在的时间片编号
extern int select_but_not_finish; // 被选中但是没有被完成的请求
extern std::set<int> select_ActualSegment;
extern int predict_num;


extern std::vector<std::vector<long long>> tag_write; // 简化成时间段的 tag 写入
extern std::vector<std::vector<long long>> tag_del; // 简化成时间段的 tag 净含量
extern std::vector<std::vector<long long>> tag_read; // 简化成时间段的 tag 读取
extern std::vector<int> disk_assignable_actual_num; // 每个磁盘的可分配实际段数
extern std::vector<std::vector<int>> request_per_time;//记录每个时间片的读取请求号

extern std::vector<std::vector<int>> read_record;//记录每帧读取请求
extern std::vector<std::vector<int>> write_record;//记录每帧写入对象
extern std::vector<std::vector<int>> del_record;//记录每帧删除对象
extern std::vector<std::vector<int>> obj_read_data;//记录每个对象的读取数据

extern double efficient_disk_rate;
extern int efficient_disk_end; // 有效磁盘段的结尾
extern int Derivatives;//0 1 2代表求导阶数

extern std::string history_name;
extern bool jump_1_round;
extern bool save_round_1;

extern std::vector<std::string> round2_head_track;
extern std::vector<std::string> round2_recycle_track;
extern std::vector<std::string> round2_write_track;
extern std::vector<std::string> round2_finish_track;
extern std::vector<std::string> round2_delete_track;
extern std::set<int> round2_finish_set;

extern bool use_round2_reoutput;

void save_history();
void load_history();

void pre_process();
void time_step_action();
void delete_action();
void exchange_action();
void pre_process_2();

std::pair<bool,int> predict_tag(int id);
// 计算磁盘上两点距离
int calculate_distance(int start, int end);
// 计算磁盘上一个点前进多少距离后的位置
int advance_position(int current, int distance);

double calculate_score(int block_id, int disk_id);

int global_get_read_time(int read_num, int have_read_time);

double calculate_variance_double(const std::vector<double>& data); // 计算方差



void allocate_segments();

void write_action();

void read_action();

double get_mark_efficiency(int time);

int predictObject(const std::vector<std::pair<double, int>>& probabilities);

struct RandomState {
    std::mt19937 engine;
    unsigned int seed;
    unsigned int call_count = 0;
};
static RandomState global_random_state{std::mt19937(42), 42};
void setGlobalRandomSeed(unsigned int seed);

double predictNextValue(const std::vector<double>& y) ;

double pearsonCorrelation(const std::vector<long long>& x, const std::vector<int>& y, int n = 0);

void check();

void round2_interact_action();