#include <bits/stdc++.h>

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
#define MAX_TAG_NUM (16 + 1)
#define MAX_OBJECT_SIZE (5)
#define MAGNERIC_HEAD_NUM (2)

extern std::string default_disk_block_index_mode; 

extern int G;
extern int T_time_step_length; // 1 ~ 86400，时间片编号 1 ~ T+105
extern int M_tag_num; // 1 ~ 16，标签编号 1 ~ 16
extern int N_disk_num; // 3 ~ 10
extern int V_block_per_disk; // 1 ~ 16384，存储单元编号 1 ~ V
extern int G_token_per_time_step; // 64 ~ 1000
extern int K_max_exchange_block; //代表每次垃圾回收事件每个硬盘最多的交换存储单元的操作次数。0~100

class StorgeNode{
    public:
        int begin = 1; // 空块起始编号
        int size; // 空闲长度
        int tag_id; // 标签编号
    // 若进行sort, 根据begin的大小进行升序排列
    bool operator<(const StorgeNode& other) const {
        return begin < other.begin;
    }
};

class ExchangeBlock{
    public:
    std::vector<std::pair<int, int>> exchange_block; // 交换块的数组
};

class Object_storge{
public:
    std::vector<int> object_storge; // 从 0 开始索引
};


struct Triple{
    StorgeNode first; // 副本一
    StorgeNode second; // 副本二
    StorgeNode third; // 副本三
};
    
class VirtualSegment{
public:
    int tag_index; // 分配该 虚拟段 tag 索引
    int tag_occupy_size[17] = {0}; // 不同tag在该虚拟段的占用大小
    std::vector<int> actual_index; // 对应三个实际段编号
    std::vector<int> actual_disk; // 对应三个实际段磁盘编号
    std::vector<std::pair<int, std::vector<int>>> first_write(int object_id); // 回传写入位置，连续空间不足会报错
    std::vector<std::pair<int, std::vector<int>>> write(int object_id); // 回传写入位置，空间不足会报错
    int get_first_empty(); // 得到首次写入的剩余空间
    int get_min_empty(); // 得到占用最小的tag
    int get_empty(); // 得到总大小的剩余空间
    double get_mark(); // 得到当前时间段的计分
    void quit_all_request();
};
        
class ActualSegment{
public:
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
    int request_size = 0; // 该实际段当前的等待请求数
    long long all_request_wait_time=0;//该段所有请求的总等待时长
};

class Disk{
    public:
        std::vector<int> magnetic_head=std::vector<int>(MAGNERIC_HEAD_NUM,1); // 磁头位置 1 ~ V
        
        std::vector<int> have_read_time=std::vector<int>(MAGNERIC_HEAD_NUM);  // 已经连续读取的次数

        int get_read_time(int read_num ,int head_id); // 根据接下来的读取次数计算时间

        void head_advance(int length, std::string way,int magnetic_head_id); // 磁头前进，仅记录信息和改变 magnetic_head

        std::vector<int> current_G_token=std::vector<int>(MAGNERIC_HEAD_NUM); // 当前时间片消耗的时间数
        int disk_id; // 磁盘编号
        int empty_size; // 空闲空间大小
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

        std::vector<int> current_time_segment = std::vector<int>(MAGNERIC_HEAD_NUM); // 当前正在执行的命令的时间段，初始是第一段
        std::vector<std::vector<int>> target_actual_array = std::vector<std::vector<int>>(MAGNERIC_HEAD_NUM); // 当前时间段分配给该 disk 的 actual 序列

        // int head_move_mode = 1; // 0 特定区域巡回模式，1 全局巡回模式
        // int last_head_move_mode = 1; // 0 特定区域巡回模式，1 全局巡回模式

        bool is_target_actual(int index, int head_id); // 判断对应段索引是否在 target_actual_array 中
        std::vector<std::vector<int>> segment_read_time=std::vector<std::vector<int>>(MAGNERIC_HEAD_NUM);//记录选中实际段的跳转次数

};

class Object{
public:
    int tag; // 标签
    int size;
    std::set<int> wait_request_set; // 该物品还没有执行完毕的请求的编号
    Object_storge storge_data[REP_NUM + 1]; // 该对象三个副本存储的位置
    int virtual_segment_id = -1; // 该对象对应的虚拟段编号
    int disk_array[REP_NUM + 1]; // 该对象三个副本存储的磁盘 id 
    void check_finish(); // 检查 wait_request_set 是否部分已完成
    void quit_all_request();
};

class Request{
public:
    int request_id;
    int object_id = 0;
    int recieve_time;
    bool select = false;
    // int prev_id; // 记录同样对象的上一个请求
    bool read[MAX_OBJECT_SIZE]; // 记录该请求每个磁盘块的读取情况
    int read_num = 0; // 已经读取的磁盘块数量 
    bool read_block(int index);
    void finish();
    void clear_read_information(); // 清空记录的 disk_block_request 信息

};

class Tag{
public:
    std::vector<StorgeNode> allocate_disk; // 该标签预分配的磁盘空间
    std::vector<int> allo_begin; // 该标签预分配的磁盘空间的起始位置
    std::vector<int> fre_del; // 多个时间段内删除的对象大小之和 每个数据的范围 0 ~ 2^32 - 1
    std::vector<int> fre_write; // 多个时间段内写入的对象大小之和
    std::vector<int> fre_read; // 多个时间段内读取的对象大小之和
    std::vector<int> default_index; // 该标签默认分配的三个磁盘
    std::vector<StorgeNode> default_disk; // 该标签默认分配的磁盘的空间
    std::vector<double> pearson_tag;
    int all_write_size;
    std::vector<int> virtual_segment = std::vector<int>(); // 分配给该 tag 的虚拟段索引,从 0 开始
};

class Tag_order{
public:
    std::vector<std::pair<int, int>> tag_order = std::vector<std::pair<int, int>>(); // 某时间段的标签净需求数组, first代表净需求, second代表标签编号
};

extern int quit_num1;

extern std::vector<std::vector<int>> most_req_segment;
extern Disk disk_array[MAX_DISK_NUM]; // 所有的磁盘数据
extern Object object_array[MAX_OBJECT_NUM]; // 所有的对象数据
extern Request request_array[MAX_REQUEST_NUM]; // 所有的请求数据
extern Tag tag_array[MAX_TAG_NUM]; // 所有的标签数据

extern int time_step; // 当前时间阶段序号

extern int total_fre_net_demand; // 该时间段净需求综合
extern int time_segment;
extern std::vector<int> g;
extern int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
extern int disk_block_index[MAX_DISK_NUM][MAX_DISK_SIZE]; // 记录磁盘对应位置是对应物品的第几块，0 ~ size - 1
extern int disk_block_request[MAX_DISK_NUM][MAX_DISK_SIZE];

extern std::vector<int> finish_request; // 当前时间片完成的请求

extern void allocate_disk();
extern void allocate_disk1();

extern std::vector<int> busy_req;

extern int Data[7];

extern std::vector<int> high_read_time_vector; // 高峰时段

extern int t; // 表示全局时间步
extern int quit_request; // 放弃请求数

extern int total_write;

extern int mode; // 0 表示分块模式, 1 表示乱序模式

extern int zero_request; 

extern int occupy_size; // 占用其他标签的总大小

extern int head_idle_time; // 磁头空闲时间

extern int all_request_read_size; // 所有请求 read 的总大小

extern int max_think_num_for_empty_read; // 判断空读时，考虑的最大块数

extern bool debug_mode;
extern bool debug_mode_mark_disk_imfromation; // 判断是否记录磁盘信息，只在 debug 模式下有效
extern int calcuate_use_time, calcuate_use_time1, calcuate_use_time2, calcuate_use_time3, calcuate_use_time4; // 计算时间

extern int drop_req_num;//被丢弃的请求数量 

// 段式磁盘分配，显然在 V_block_per_disk 太小时完全失效
// 每一个虚拟段包含三个实际段，未分配到虚拟段的实际段都分配到空段集合

// 将 虚拟段 分配到 tag，并将 实际段 分配到 虚拟段
//     1、计算所有 tag 的 总写入 或者 净含量的最大值，并以此计算每个 tag 的期望 虚拟段数
//     2、计算所有 tag 在三个时间段的读取频率，方法：目标时间段的总写入 / （目标时间段结束位置的总写入 或者 目标时间段净含量的最大值)
//     3、重复选择所有 tag 中完成率最少的 tag，尝试为他分配一个虚拟段
//          3.1、为该虚拟段遍历所有可能的三个磁盘组合，选择一个使总方差值最小的三个磁盘组合
//          3.2、总方差值的计算方式：每个时间段单独计算方差并求和
//              3.2.1、每个时间段的方差：以所有磁盘的读取概率的序列计算方差
//          3.3、分配之后，将该 tag 的读取频率加到分配的磁盘的读取概率上
//          3.4、如果无法得到三个磁盘组合（无法有三个磁盘的可分配实际段数量 >= 1），就退出循环
//     4、得到每个磁盘未分配的实际段数量，和分配到这个磁盘的虚拟段编号序列

// 上一步未分配的实际段增加到空段集合中

// c++写函数实现对象向磁盘分配的功能，接收物品id, 回传写入的位置
// 分配优先级
//     1、遍历当前 tag 所有虚拟段，尝试连续分配
//     2、遍历当前 tag 所有虚拟段，尝试非连续分配
//     3、尝试分配到 空段
//          3.1、按剩余连续空间排序所有空段，尝试顺序选择三个 disk_id 不同的进行分配
//          3.2、按剩余空间排序所有空段，尝试顺序选择三个 disk_id 不同的进行分配
//     4、遍历所有 tag 所有虚拟段，尝试连续分配
//     5、遍历所有 tag 所有虚拟段，尝试非连续分配
//     6、报错
// 注意：
// 所谓“连续分配”就是 first_write，“不连续分配”就是 write
// 对于虚拟段，只需要调用 VirtualSegment 的函数，里面会自动调用 ActualSegment 的相关函数，对于空段的处理才需要使用 ActualSegment 的函数
// 对于步骤三，不涉及到虚拟段的内容，但是需要将三个ActualSegment回传的位置合并成 vector<pair<int, vector<int>>>，pair的前者是磁盘 id，后者是ActualSegment.write 的输出
// 对于 first_write 和 write，只要空间足够就能成功，不需要 try 判断
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
extern std::set<int> select_VirtualSegment;

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

extern std::vector<std::vector<int>> tag_write; // 简化成时间段的 tag 写入
extern std::vector<std::vector<int>> tag_content; // 简化成时间段的 tag 净含量
extern std::vector<std::vector<long long>> tag_read; // 简化成时间段的 tag 读取
extern std::vector<int> disk_assignable_actual_num; // 每个磁盘的可分配实际段数
extern std::vector<VirtualSegment> virtual_segment_array; // 包含所有的虚拟段
extern std::vector<std::pair<int, int>> empty_segment_array; // 包含所有空段，第一个是段在磁盘内的索引，第二个是磁盘id
extern std::vector<std::vector<int>> request_per_time;//记录每个时间片的读取请求号

void pre_process();
void time_step_action();
void read_action();
void write_action();
void write_action1();
void write_action2();
void delete_action();
void exchange_action();

// 计算磁盘上两点距离
int calculate_distance(int start, int end);
// 计算磁盘上一个点前进多少距离后的位置
int advance_position(int current, int distance);

int find_tag(int disk_id, int block_id);

int find_max_disk(int tag_id);

int find_max_disk1();

double calculate_score(int block_id, int disk_id);

int global_get_read_time(int read_num, int have_read_time);

std::vector<int> get_allocate_size_rank(int tag_id);

void analyze_read_request();

bool if_find(std::vector<int>& vec, int target);

double calculate_variance_double(const std::vector<double>& data); // 计算方差

void allocate_segments();

void write_action3();

void read_action2();

double get_mark_efficiency(int time);