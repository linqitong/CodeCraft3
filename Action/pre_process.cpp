#include "../common.h"
using namespace std;

vector<vector<double>> compute_corr_matrix() {
    vector<vector<double>> corr_matrix(M_tag_num, vector<double>(M_tag_num, 0.0));
    int length = (T_time_step_length - 1) / FRE_PER_SLICING + 1;

    for (int i = 1; i <= M_tag_num; ++i) {
        for (int j = 1; j <= M_tag_num; ++j) {
            double sumA = 0, sumB = 0, sumAB = 0, sumA2 = 0, sumB2 = 0;

            for (int k = 1; k < length; ++k) {
                sumA += tag_array[i].fre_read[k];
                sumB += tag_array[j].fre_read[k];
            }

            double avgA = sumA / length;
            double avgB = sumB / length;

            for (int k = 1; k < length; ++k) {
                double a = tag_array[i].fre_read[k] - avgA;
                double b = tag_array[j].fre_read[k] - avgB;
                sumAB += a * b;
                sumA2 += a * a;
                sumB2 += b * b;
            }

            double denominator = sqrt(sumA2) * sqrt(sumB2);
            if (denominator == 0) {
                corr_matrix[i - 1][j - 1] = 0;
            } else {
                corr_matrix[i - 1][j - 1] = sumAB / denominator;
            }
        }
    }

    for (int i = 0; i < M_tag_num; ++i) {
        corr_matrix[i][i] = 1.0;
    }

    return corr_matrix;
}

double total_correlation(const vector<int>& arrangement, const vector<vector<double>>& corr_matrix) {
    double total = 0.0;
    int n = arrangement.size();
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        total += corr_matrix[arrangement[i]][arrangement[j]];
    }
    return total;
}

vector<int> generate_2opt_neighbor(const vector<int>& arr) {
    int a = rand() % arr.size();
    int b = rand() % arr.size();
    if (a > b) swap(a, b);

    vector<int> new_arr = arr;
    reverse(new_arr.begin() + a, new_arr.begin() + b + 1);
    return new_arr;
}

vector<int> get_tag_rank() {
    vector<vector<double>> corr_matrix = compute_corr_matrix();

    // 模拟退火参数
    const double initial_temp = 1000;
    const double cooling_rate = 0.995;
    const int num_iterations = 5000;

    vector<int> current_arrangement(M_tag_num);
    for (int i = 0; i < M_tag_num; ++i)
        current_arrangement[i] = i;

    random_device rd;
    mt19937 g(rd());
    shuffle(current_arrangement.begin(), current_arrangement.end(), g);

    double current_score = total_correlation(current_arrangement, corr_matrix);
    vector<int> best_arrangement = current_arrangement;
    double best_score = current_score;
    double temp = initial_temp;

    for (int i = 0; i < num_iterations; ++i) {
        vector<int> new_arrangement = generate_2opt_neighbor(current_arrangement);
        double new_score = total_correlation(new_arrangement, corr_matrix);
        double diff = new_score - current_score;

        if (diff > 0 || exp(diff / temp) > ((double)rand() / RAND_MAX)) {
            current_arrangement = new_arrangement;
            current_score = new_score;
            if (current_score > best_score) {
                best_arrangement = current_arrangement;
                best_score = current_score;
            }
        }
        temp *= cooling_rate;
    }

    // cout << "最佳相关系数和: " << best_score << endl;
    // cout << "最佳排列顺序: ";
    vector<int> result;
    for (int i = 0; i < M_tag_num; ++i) {
        result.push_back(best_arrangement[i] + 1);
    }
    return result;
}

void allocate_segments() {
    string expect_num_mode = "tag_content"; // tag_write/tag_content
    string read_probability_mode = "tag_content"; // tag_write/tag_content

    // Step 1: 计算每个tag的期望虚拟段数目
    vector<int> expected_as(M_tag_num + 2, 0);
    vector<vector<int>> content_size(M_tag_num + 2,vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2));
    for (int i = 1; i <= M_tag_num+1; ++i) {
        int max_val = 0;
       
        for (int t = 1; t <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; ++t) {
            content_size[i][t]=content_size[i][t-1]+tag_array[i].fre_write[t]-tag_array[i].fre_del[t];
            int current = content_size[i][t];
            max_val = max(max_val, current);
        }
        
        expected_as[i] = max_val / segment_size; // 忽略多余的请求
    }

    // Step 2: 计算每个tag在每个时间段的读取频率
    vector<vector<double>> tag_read_freq(M_tag_num + 2, vector<double>(((T_time_step_length - 1) / FRE_PER_SLICING + 2), 0.0));
    for (int i = 1; i <= M_tag_num+1; ++i) {
        for (int t = 1; t <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; ++t) {
            tag_read_freq[i][t]=static_cast<double>(tag_array[i].fre_read[t])/content_size[i][t];
        }
    }

    // 初始化磁盘的读取概率和虚拟段列表
    vector<vector<double>> disk_read_prob(((T_time_step_length - 1) / FRE_PER_SLICING + 2), vector<double>(N_disk_num + 1, 0.0));
    vector<int> allocated(M_tag_num + 2, 0);
    vector<vector<int>> tag_allocated_space(M_tag_num+2,vector<int>(N_disk_num+1,0));//记录每个tag在每个磁盘中分配的空间；
    while (true) {
        // 选择完成率最低的tag
        int selected_tag = -1;
        double min_ratio = 10000000000000000;
        for (int i = 1; i <= M_tag_num+1; ++i) {
            if (expected_as[i] == 0) continue;
            double ratio = static_cast<double>(allocated[i]) / (double)expected_as[i];
            if (ratio < min_ratio) {
                min_ratio = ratio;
                selected_tag = i;
            }
        }
        assert(min_ratio < 10000000000000000);

        // 收集可用的磁盘
        vector<int> available_disks;
        for (int d = 1; d <= N_disk_num; ++d) {
            if (disk_assignable_actual_num[d] >= 1) available_disks.push_back(d);
        }
        if (available_disks.size() == 0) break;
        //计算方差，选择磁盘
        double min_var=100000000000;
        int disk_selected;
        for(int i=0;i<available_disks.size();i++){
            int disk_id=available_disks[i];
            double var=0;
            for(int j=1;j<=(T_time_step_length - 1) / FRE_PER_SLICING + 1;j++){
                vector<double> v=disk_read_prob[j];
                v[disk_id]+=tag_read_freq[selected_tag][j];
                //[disk_id]+=tag_array[selected_tag].fre_read[j];
                v=vector<double>(v.begin()+1,v.end());
                var+=calculate_variance_double(v);
            }
            if(var<min_var){
                min_var=var;
                disk_selected=disk_id;
            }
        }
        allocated[selected_tag]++;
        tag_allocated_space[selected_tag][disk_selected]++;
        disk_assignable_actual_num[disk_selected]--;
        //available_disks[disk_selected]--;
        //更新disk_read_prob
        for(int j=1;j<=(T_time_step_length - 1) / FRE_PER_SLICING + 1;j++){
            disk_read_prob[j][disk_selected]+=tag_read_freq[selected_tag][j];
            //disk_read_prob[j][disk_selected]+=tag_array[selected_tag].fre_read[j];
        }
        
    }
    // 到现在为止的结果
    // {disk_assignable_actual_num, disk_virtual_segments};

    /*
    // 解决 TSP 制定 tag 在段内的优先级
    // 准备数据
    vector<vector<double>> tag_read_data;
    for(int n1 = 1; n1 <= M_tag_num; n1++){
        tag_read_data.push_back(vector<double>());
        for(int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++){
            tag_read_data[n1 - 1].push_back(tag_array[n1].fre_read[j]);
        }
    }
    vector<int> TSP_result = solveTSP(tag_read_data);
    map<int, int> priority;
    for(int n1 = 0; n1 < TSP_result.size(); n1++){
        priority[TSP_result[n1] + 1] = n1;
    }
    */

    // 在 磁盘 内部分配虚拟段
    // 暂定按照第二时间段的消耗时间分配
    //vector<int> tag_rank = {14, 12, 9, 4, 16, 6, 5, 11, 15, 13, 3, 2, 7, 8, 10, 1};
    // vector<int> tag_rank = get_tag_rank();
    vector<int> tag_rank = get_tag_rank();;
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        int now_segment=0;
        Disk &target_disk= disk_array[n1];
        for(int n2=0;n2<tag_rank.size();n2++){
            int tag=tag_rank[n2];
            for(int n3=0;n3<tag_allocated_space[tag][n1];n3++){
                target_disk.segment_array[now_segment].tag_index=tag;
                now_segment++;
            }
            
        }
        assert(now_segment<=segment_num);
    }
}


void calc_pearson()
{
    for(int i = 1; i <= M_tag_num+1; i++){
        for(int j = 0; j <= M_tag_num+1; j++){
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

void load_history(){
    ifstream fin(history_name);
    if(!fin){
        cerr << "Error: cannot open history file: " << history_name << endl;
        exit(EXIT_FAILURE);
    }
    
    // 读取基本参数
    fin >> G >> T_time_step_length >> M_tag_num >> N_disk_num 
        >> V_block_per_disk >> K_max_exchange_block >> max_object_id >> max_request_id;
    
    // 读取 object_array 数据（假设下标从1开始）
    for(int n1 = 1; n1 <= max_object_id; n1++){
        Object& target_object = object_array[n1];
        fin >> target_object.tag >> target_object.true_tag >> target_object.size >> target_object.read_times;
    }
    
    // 读取 request_array 数据（假设下标从1开始）
    for(int n1 = 1; n1 <= max_request_id; n1++){
        Request& target_request = request_array[n1];
        fin >> target_request.request_id >> target_request.object_id;
    }
    
    // 读取 tag_read 数据（vector<vector<int>>），需要先清空并重新初始化
    int vecSize;
    fin >> vecSize;
    tag_read.clear();
    tag_read.resize(vecSize);
    for(int i = 0; i < vecSize; i++){
        int innerSize;
        fin >> innerSize;
        tag_read[i].resize(innerSize);
        for (int j = 0; j < innerSize; j++){
            fin >> tag_read[i][j];
        }
    }
    
    // 读取 read_record 数据
    fin >> vecSize;
    read_record.clear();
    read_record.resize(vecSize);
    for(int i = 0; i < vecSize; i++){
        int innerSize;
        fin >> innerSize;
        read_record[i].resize(innerSize);
        for (int j = 0; j < innerSize; j++){
            fin >> read_record[i][j];
        }
    }
    
    // 读取 write_record 数据
    fin >> vecSize;
    write_record.clear();
    write_record.resize(vecSize);
    for(int i = 0; i < vecSize; i++){
        int innerSize;
        fin >> innerSize;
        write_record[i].resize(innerSize);
        for (int j = 0; j < innerSize; j++){
            fin >> write_record[i][j];
        }
    }
    
    // 读取 del_record 数据
    fin >> vecSize;
    del_record.clear();
    del_record.resize(vecSize);
    for(int i = 0; i < vecSize; i++){
        int innerSize;
        fin >> innerSize;
        del_record[i].resize(innerSize);
        for (int j = 0; j < innerSize; j++){
            fin >> del_record[i][j];
        }
    }
    
    // 读取 obj_read_data 数据
    fin >> vecSize;
    obj_read_data.clear();
    obj_read_data.resize(vecSize);
    for(int i = 0; i < vecSize; i++){
        int innerSize;
        fin >> innerSize;
        obj_read_data[i].resize(innerSize);
        for (int j = 0; j < innerSize; j++){
            fin >> obj_read_data[i][j];
        }
    }
    
    fin.close();
}


void save_history(){
    freopen(history_name.c_str(), "w", stdout);
    cout << G << " " << T_time_step_length << " " << M_tag_num << " " << N_disk_num;
    cout << " " << V_block_per_disk << " " << K_max_exchange_block << " " << max_object_id;
    cout << " " << max_request_id << endl;
    for(int n1 = 1; n1 <= max_object_id; n1++){
        Object& target_object = object_array[n1];
        cout << target_object.tag << " " << target_object.true_tag << " " << target_object.size << " " << target_object.read_times << endl;
    }
    for(int n1 = 1; n1 <= max_request_id; n1++){
        Request& target_request = request_array[n1];
        cout << target_request.request_id << " " << target_request.object_id << endl;
    }
    cout << tag_read.size() << endl;
    for(int n1 = 0; n1 < tag_read.size(); n1++){
        cout << tag_read[n1].size() << endl;
        for(int n2 = 0; n2 < tag_read[n1].size(); n2++){
            cout << tag_read[n1][n2] << " ";
        }
        cout << endl;
    }
    cout << read_record.size() << endl;
    for(int n1 = 0; n1 < read_record.size(); n1++){
        cout << read_record[n1].size() << endl;
        for(int n2 = 0; n2 < read_record[n1].size(); n2++){
            cout << read_record[n1][n2] << " ";
        }
        cout << endl;
    }
    cout << write_record.size() << endl;
    for(int n1 = 0; n1 < write_record.size(); n1++){
        cout << write_record[n1].size() << endl;
        for(int n2 = 0; n2 < write_record[n1].size(); n2++){
            cout << write_record[n1][n2] << " ";
        }
        cout << endl;
    }
    cout << del_record.size() << endl;
    for(int n1 = 0; n1 < del_record.size(); n1++){
        cout << del_record[n1].size() << endl;
        for(int n2 = 0; n2 < del_record[n1].size(); n2++){
            cout << del_record[n1][n2] << " ";
        }
        cout << endl;
    }
    cout << obj_read_data.size() << endl;
    for(int n1 = 0; n1 < obj_read_data.size(); n1++){
        cout << obj_read_data[n1].size() << endl;
        for(int n2 = 0; n2 < obj_read_data[n1].size(); n2++){
            cout << obj_read_data[n1][n2] << " ";
        }
        cout << endl;
    }
    freopen(".\\output.txt", "a+", stdout);
}

void pre_process_2(){
    round2_head_track = std::vector<std::string>(T_time_step_length + EXTRA_TIME + 1);
    round2_recycle_track = std::vector<std::string>(T_time_step_length + EXTRA_TIME + 1);
    round2_write_track = std::vector<std::string>(T_time_step_length + EXTRA_TIME + 1);
    round2_finish_track = std::vector<std::string>(T_time_step_length + EXTRA_TIME + 1);
    round2_delete_track = std::vector<std::string>(T_time_step_length + EXTRA_TIME + 1);

    // 第二轮初始化相关的代码
    for(int n1 = 0; n1 < MAX_DISK_NUM; n1++){
        disk_array[n1] = Disk();
        for(int n2 = 0; n2 < MAX_DISK_SIZE; n2++){
            disk[n1][n2] = 0;
            disk_block_index[n1][n2] = 0;
            disk_block_request[n1][n2] = 0;
        }
    }
    for(int n1 = 0; n1 < MAX_REQUEST_NUM; n1++){
        for(int n2= 0; n2 < MAX_OBJECT_SIZE; n2++){
            request_array[n1].read[n2] = false;
        }
        request_array[n1].read_num = 0;
        request_array[n1].select = false;
    }
   
    
    for(int n1 = 0; n1 < M_tag_num+1; n1++){
        tag_array[n1] = Tag();
    }

    // 所有磁盘垃圾栈初始化
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        disk_array[n1].rubbish_stack = stack<int>();
        for(int n2 = V_block_per_disk; n2 > efficient_disk_end; n2--){
            disk_array[n1].rubbish_stack.push(n2);
        }
    }
    // std::ifstream fin(".\\Data\\sample_practice_map_1.txt"); // 创建文件输入流并打开文件
    // int a,b;
    // while(fin>>a>>b){
    //     object_array[a].tag=b;
    //     object_array[a].true_tag=true;
    // }
    // fin.close(); // 关闭文件(析构函数会自动调用)
    //根据pearson相似计算每个物品的tag
    //freopen(".\\predict_result.txt", "w", stdout);

    int num=0;
    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> distrib(1, M_tag_num);
    tag_read = vector<vector<long long>>(M_tag_num + 1, vector<long long>((T_time_step_length+EXTRA_TIME)/pearson_sample_interval+1, 0));
    for(int time=1;time<=T_time_step_length + EXTRA_TIME;time++){
        for(int i=0;i<read_record[time].size();i++){
            int req=read_record[time][i];
            int obj_id=request_array[req].object_id;
            if(object_array[obj_id].read_times>=100 and object_array[obj_id].true_tag){
                tag_read[object_array[obj_id].tag][time/pearson_sample_interval]+=object_array[obj_id].size;
            }
        }
    }

    vector<vector<pair<int, int>>> tag_obj_read = vector<vector<pair<int, int>>>(M_tag_num + 1);
    
    for(int i=1;i <= max_object_id;i++){
        Object& target_object = object_array[i];
        if(!object_array[i].true_tag){
            continue;
        }
        tag_obj_read[target_object.tag].push_back(
            {target_object.read_times * target_object.size, i}
        );
    }
    for(int n1 = 1; n1 <= M_tag_num; n1++){
        sort(tag_obj_read[n1].begin(), tag_obj_read[n1].end(), greater<pair<int, int>>());
        double all_size = 0;
        for(int n2 = 0; n2 < tag_obj_read[n1].size(); n2++){
            all_size += tag_obj_read[n1][n2].first;
        }
        int high_index = 0;
        int low_index = tag_obj_read[n1].size() - 1;
        double current_high_size = 0;
        double current_low_size = all_size;
        for(int n2 = 0; n2 < tag_obj_read[n1].size(); n2++){
            current_high_size += tag_obj_read[n1][n2].first;
            if(current_high_size > all_size * round2_high_threshold){
                break;
            }
            high_index = n2;
        }
        for(int n2 = tag_obj_read[n1].size() - 1; n2 >= 0; n2--){
            current_low_size -= tag_obj_read[n1][n2].first;
            if(current_low_size < all_size * round2_low_threshold){
                break;
            }
            low_index = n2;
        }
        tag_array[n1].round2_high_request_num = tag_obj_read[n1][high_index].first;
        tag_array[n1].round2_low_request_num = tag_obj_read[n1][low_index].first;
    }


    for(int i=1;i <= max_object_id;i++){
        Object& target_object = object_array[i];
        if(!object_array[i].true_tag){
            num++;
            int tag=distrib(gen);
            double similarity=0;
            for(int j=1;j<=M_tag_num;j++){
                double sim = pearsonCorrelation(tag_read[j], obj_read_data[i]);
                if(sim>similarity){
                    similarity=sim;
                    tag=j;
                }
            }
            if(similarity<=0.5){
                
                // if(accumulate(obj_read_data[i].begin(),obj_read_data[i].end(),0.0)<100)
                // predict_num++;
            } 
            
            //cout<<i<<' '<<tag<<' '<<similarity<<endl;
            //assert(similarity>0);
            object_array[i].true_tag=true;
            
            object_array[i].tag=tag;
            // if(similarity<=0){
            //     object_array[i].tag=17;
            // }
            for(int j=0;j<obj_read_data[i].size();j++){
                tag_read[tag][j] += obj_read_data[i][j];
            }
            if( accumulate(obj_read_data[i].begin(),obj_read_data[i].end(),0.0)< 100){
                predict_num++;
                object_array[i].quit = true;
            }
        }else{
         if( object_array[i].read_times<100){
                predict_num++;
                object_array[i].quit = true;
            }   
        }
    }
    




    //cout<<"total:"<<num<<endl;
    //freopen(".\\output.txt", "a+", stdout);
   
    for (int i = 1; i <= M_tag_num+1; i++) {
        tag_array[i].fre_del = vector<int>((T_time_step_length + EXTRA_TIME - 1) / FRE_PER_SLICING + 2);
        tag_array[i].fre_write = vector<int>((T_time_step_length + EXTRA_TIME - 1) / FRE_PER_SLICING + 2);
        tag_array[i].fre_read = vector<int>((T_time_step_length + EXTRA_TIME - 1) / FRE_PER_SLICING + 2);
    }
    for(int time=1;time<=T_time_step_length + EXTRA_TIME;time++){
        for(int i=0;i<read_record[time].size();i++){
            int req=read_record[time][i];
            int obj_id=request_array[req].object_id;
            Object obj=object_array[obj_id];
            if(obj.quit!=true)
                tag_array[obj.tag].fre_read[(time - 1) / FRE_PER_SLICING + 1]+=obj.size;
        }
        for(int i=0;i<write_record[time].size();i++){
            Object obj=object_array[write_record[time][i]];
            tag_array[obj.tag].fre_write[(time - 1) / FRE_PER_SLICING + 1]+=obj.size;
        }
        for(int i=0;i<del_record[time].size();i++){
            Object obj=object_array[del_record[time][i]];
            tag_array[obj.tag].fre_del[(time - 1) / FRE_PER_SLICING + 1]+=obj.size;
        }
    }
    
     
    
    
    //  for (int i = 1; i <= M_tag_num; i++) {
    //      int time_segment = ((T_time_step_length - 1) / FRE_PER_SLICING + 2) / 3;
    //      int content = 0;
    //      for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
    //          content += tag_array[i].fre_write[j];
    //          double a = tag_array[i].fre_del[j] * 2.5;
    //          content -= a;
    //          if(j / time_segment > 2){
    //              continue;
    //          }
    //          tag_write[i][j / time_segment] += tag_array[i].fre_write[j];
    //          tag_write[i][j / time_segment] -= tag_array[i].fre_del[j];
    //          tag_read[i][j / time_segment] += tag_array[i].fre_read[j];
    //          tag_content[i][j / time_segment] = max(tag_content[i][j / time_segment], content);
    //      }
    //  }
 
     // 初始化 Disk 信息 和 disk_assignable_actual_num
     for(int n1 = 1; n1 <= N_disk_num; n1++){
         Disk& target_disk = disk_array[n1];
         target_disk.disk_id = n1;
         disk_assignable_actual_num[n1] = segment_num;
         for(int n2 = 0; n2 < disk_assignable_actual_num[n1]; n2++){
             ActualSegment actualSegment = ActualSegment();
             actualSegment.disk_id = n1;
             actualSegment.begin_index = n2 * segment_size + 1;
             actualSegment.segment_length = segment_size;
             target_disk.segment_array.push_back(actualSegment);
         }
     }
     calc_pearson();
     allocate_segments();
}


// 预处理
void pre_process(){

    // 读取全局参数
    scanf("%d%d%d%d%d%d", &T_time_step_length, &M_tag_num, &N_disk_num, &V_block_per_disk, &G,&K_max_exchange_block);
    int efficient_size = ceil((double)V_block_per_disk * efficient_disk_rate);
    segment_size = ceil((double)efficient_size / (double)segment_num);
    efficient_disk_end = segment_size * segment_num;

    // 所有磁盘垃圾栈初始化
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        disk_array[n1].rubbish_stack = stack<int>();
        for(int n2 = V_block_per_disk; n2 > efficient_disk_end; n2--){
            disk_array[n1].rubbish_stack.push(n2);
        }
    }

    // // 初始化时间片信息
    // for (int i = 1; i <= M_tag_num; i++) {
    //     tag_array[i].fre_del = vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2,1);
    //     tag_array[i].fre_write = vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2,1);
    //     tag_array[i].fre_read = vector<int>((T_time_step_length - 1) / FRE_PER_SLICING + 2,1);
    //     g = vector<int>((T_time_step_length - 1 + 105) / FRE_PER_SLICING + 2);
    // }
    read_record=std::vector<std::vector<int>>(T_time_step_length+106);
    write_record=std::vector<std::vector<int>>(T_time_step_length+106);
    del_record=std::vector<std::vector<int>>(T_time_step_length+106);
    for (int i = 1; i <= M_tag_num; i++){
        possibility.emplace_back(1.0,i);
    }
    // // 删除时间片信息
    // for (int i = 1; i <= M_tag_num; i++) {
    //     for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
    //         scanf("%d", &tag_array[i].fre_del[j]);
    //     }
    // }
    // // 写入时间片信息
    // for (int i = 1; i <= M_tag_num; i++) {
    //     for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
    //         scanf("%d", &tag_array[i].fre_write[j]);
    //         tag_array[i].all_write_size += tag_array[i].fre_write[j];
    //         total_write += tag_array[i].fre_write[j];
    //     }
    // }

    // // 读取时间片信息
    // for (int i = 1; i <= M_tag_num; i++) {
    //     for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
    //         scanf("%d", &tag_array[i].fre_read[j]);
    //     }
    // }

    
    // for (int j = 1; j <= (T_time_step_length - 1 + 105) / FRE_PER_SLICING + 1; j++) {
    //     scanf("%d", &g[j]);
    // }

    // 计算 tag_write，tag_content，tag_read
    
    tag_read = vector<vector<long long>>(M_tag_num + 1, vector<long long>((T_time_step_length+EXTRA_TIME)/pearson_sample_interval+1, 0));
    obj_read_data = std::vector<std::vector<int>> (MAX_OBJECT_NUM, vector<int>((T_time_step_length+EXTRA_TIME)/pearson_sample_interval+1, 0));
    // for (int i = 1; i <= M_tag_num; i++) {
    //     int time_segment = ((T_time_step_length - 1) / FRE_PER_SLICING + 2) / 3;
    //     int content = 0;
    //     for (int j = 1; j <= (T_time_step_length - 1) / FRE_PER_SLICING + 1; j++) {
    //         content += tag_array[i].fre_write[j];
    //         double a = tag_array[i].fre_del[j] * 2.5;
    //         content -= a;
    //         if(j / time_segment > 2){
    //             continue;
    //         }
    //         tag_write[i][j / time_segment] += tag_array[i].fre_write[j];
    //         tag_write[i][j / time_segment] -= tag_array[i].fre_del[j];
    //         tag_read[i][j / time_segment] += tag_array[i].fre_read[j];
    //         tag_content[i][j / time_segment] = max(tag_content[i][j / time_segment], content);
    //     }
    // }
   
    // 初始化 Disk 信息 和 disk_assignable_actual_num
    for(int n1 = 1; n1 <= N_disk_num; n1++){
        Disk& target_disk = disk_array[n1];
        target_disk.disk_id = n1;
        disk_assignable_actual_num[n1] = segment_num;
        for(int n2 = 0; n2 < disk_assignable_actual_num[n1]; n2++){
            ActualSegment actualSegment = ActualSegment();
            actualSegment.disk_id = n1;
            actualSegment.begin_index = n2 * segment_size + 1;
            actualSegment.segment_length = segment_size;
            target_disk.segment_array.push_back(actualSegment);
        }
    }
    // allocate_segments();
    // calc_pearson();
    printf("OK\n");
    fflush(stdout);
}

