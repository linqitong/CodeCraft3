#include "../common.h"
using namespace std;

void Object::check_finish(){
    vector<int> delete_list;
    for (auto it = this->wait_request_set.begin(); 
        it != this->wait_request_set.end(); 
        it++) {
            delete_list.push_back(*it);
    }
    for(int n1 = 0; n1 < delete_list.size(); n1++){
        request_array[delete_list[n1]].finish();
    }
}

void Object::quit_all_request(){
    for (const auto& elem : this->wait_request_set) {
        // wait_request_set.erase(elem);
        request_array[elem].clear_read_information();
        busy_req.push_back(elem);
        quit_num1++;
    }
    this->wait_request_set = set<int>();
}

