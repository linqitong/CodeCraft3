#include "../common.h"
using namespace std;

void calc_pearson()
{
    for(int i = 1; i <= M_tag_num; i++){
        for(int j = 0; j <= M_tag_num; j++){
            if(j == 0){
                tag_array[i].pearson_tag.push_back(0.0);
                continue;
            }
            int length = (T_time_step_length - 1) / FRE_PER_SLICING + 1;
            double sumA = 0, sumB = 0, sumAB = 0, sumA2 = 0, sumB2 = 0;
            for(int k = 1; k <= length; k++){
                sumA += tag_array[i].fre_read[k];
                sumB += tag_array[j].fre_read[k];
            }
            double averageA = sumA / length, averageB = sumB / length;
            for(int k = 1; k <= length; k++){
                sumAB += (tag_array[i].fre_read[k] - averageA) * (tag_array[j].fre_read[k] - averageB);
                sumA2 += (tag_array[i].fre_read[k] - averageA) * (tag_array[i].fre_read[k] - averageA);
                sumB2 += (tag_array[j].fre_read[k] - averageB) * (tag_array[j].fre_read[k] - averageB);
            }
            double numerator = sumAB;
            double denominator = std::sqrt(sumA2) * std::sqrt(sumB2);
            tag_array[i].pearson_tag.push_back(numerator / denominator);
        }
    }
}

// 预处理
void pre_process(){

    // 读取全局参数
    scanf("%d%d%d%d%d%d", &T_time_step_length, &M_tag_num, &N_disk_num, &V_block_per_disk, &G,&K_max_exchange_block);

    segment_size = V_block_per_disk / segment_num;

    most_req_segment=vector<vector<int>>(N_disk_num+1,vector<int>(MAGNERIC_HEAD_NUM,-1));

    // 初始化时间片信息
    for (int i = 1; i <= M_tag_num; i++) {
        tag_array[i].fre_del = vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2);
        tag_array[i].fre_write = vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2);
        tag_array[i].fre_read = vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2);
        g = vector<int>((T_time_step_length - 1 + 105) / FRE_PER_SLICING + 2);
    }

    // 删除时间片信息
    for (int i = 1; i <= M_tag_num; i++) {
        for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%d", &tag_array[i].fre_del[j]);
        }
    }
    // 写入时间片信息
    for (int i = 1; i <= M_tag_num; i++) {
        for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%d", &tag_array[i].fre_write[j]);
            tag_array[i].all_write_size += tag_array[i].fre_write[j];
            total_write += tag_array[i].fre_write[j];
        }
    }

    // 读取时间片信息
    for (int i = 1; i <= M_tag_num; i++) {
        for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%d", &tag_array[i].fre_read[j]);
        }
    }

    
    for (int j = 1; j <= (T_time_step_length - 1 + 105) / FRE_PER_SLICING + 1; j++) {
        scanf("%d", &g[j]);
    }

    // 重置磁盘空闲内存块表
    for (int i = 1; i <= N_disk_num; i++){
        disk_array[i].empty_size = V_block_per_disk;
    }
    // analyze_read_request();

    // 计算 tag_write，tag_content，tag_read
    vector<int> b = vector<int>(5);
    tag_write = vector<vector<int>>(M_tag_num + 1, vector<int>(3, 0));
    tag_content = vector<vector<int>>(M_tag_num + 1, vector<int>(3, 0));
    tag_read = vector<vector<long long>>(M_tag_num + 1, vector<long long>(3, 0));
    for (int i = 1; i <= M_tag_num; i++) {
        int time_segment = ((T_time_step_length - 1) / FRE_PER_SLICING + 2) / 3;
        int content = 0;
        for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
            content += tag_array[i].fre_write[j];
            double a = tag_array[i].fre_del[j] * 2.5;
            content -= a;
            if(j / time_segment > 2){
                continue;
            }
            tag_write[i][j / time_segment] += tag_array[i].fre_write[j];
            tag_write[i][j / time_segment] -= tag_array[i].fre_del[j];
            tag_read[i][j / time_segment] += tag_array[i].fre_read[j];
            tag_content[i][j / time_segment] = max(tag_content[i][j / time_segment], content);
        }
    }
    // vector<int> b = vector<int>(5);
    // 初始化 Disk 信息 和 disk_assignable_actual_num
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        Disk& target_disk = disk_array[n1];
        target_disk.disk_id = n1;
        disk_assignable_actual_num[n1] = V_block_per_disk / segment_size;
        for(int n2 = 0; n2 < disk_assignable_actual_num[n1]; n2++){
            ActualSegment actualSegment = ActualSegment();
            actualSegment.disk_id = n1;
            actualSegment.begin_index = n2 * segment_size + 1;
            actualSegment.segment_length = segment_size;
            target_disk.segment_array.push_back(actualSegment);
        }
        if(V_block_per_disk % segment_size != 0){
            ActualSegment actualSegment = ActualSegment();
            actualSegment.disk_id = n1;
            actualSegment.begin_index = disk_assignable_actual_num[n1] * segment_size + 1;
            actualSegment.segment_length = V_block_per_disk % segment_size;
            target_disk.segment_array.push_back(actualSegment);
            empty_segment_array.push_back({target_disk.segment_array.size() - 1, n1});
        }
    }
    allocate_segments();
    calc_pearson();
    printf("OK\n");
    fflush(stdout);
}

