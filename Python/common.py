FRE_PER_SLICING = 1800
MAX_DISK_NUM = (10 + 1)
MAX_DISK_SIZE = (16384 + 1)
MAX_REQUEST_NUM = (30000000 + 1)
MAX_OBJECT_NUM = (100000 + 1)
REP_NUM = 3
EXTRA_TIME = 105
MAX_TAG_NUM = 16 + 1

T_time_step_length = 0
M_tag_num = 0
N_disk_num = 0
V_block_per_disk = 0
G_token_per_time_step = 0
K_garbage_recycle_size = 0

t = 0
t_segment = 0

class Object:
    def __init__(self):
        self.replica = [0 for _ in range(REP_NUM + 1)]
        self.unit = [[] for _ in range(REP_NUM + 1)]
        self.size = 0
        self.lastRequestPoint = 0
        self.isDelete = False
        self.tag_id = 0
        self.begin_time = 0
        self.end_time = 89999
        self.size_index = 0
        self.read_array = []


class Tag:
    def __init__(self):
        self.fre_del = []
        self.fre_write = []
        self.fre_read = []
        self.fre_net_demand = []
        self.fre_all_size = []
        self.read_data = [0 for _ in range(T_time_step_length + EXTRA_TIME + 1)]
        self.write_data = [0 for _ in range(T_time_step_length + EXTRA_TIME + 1)]
        self.untag_write_data = [0 for _ in range(T_time_step_length + EXTRA_TIME + 1)]
        self.object_request_dict = {}
        self.all_size = 0
        self.object_dict = {}
        self.untag_object = {}
        self.all_num = 0


untag_write_data = [[] for _ in range(T_time_step_length + EXTRA_TIME + 1)]

disk = [[0 for _ in range(MAX_DISK_SIZE)] for _ in range(MAX_DISK_NUM)]
disk_point = [0 for _ in range(MAX_DISK_NUM)]
_id = [0 for _ in range(MAX_OBJECT_NUM)]

current_request = 0
current_phase = 0
req_object_ids = [0] * MAX_REQUEST_NUM
req_prev_ids = [0] * MAX_REQUEST_NUM
req_is_dones = [False] * MAX_REQUEST_NUM
objects = [Object() for _ in range(MAX_OBJECT_NUM)]
tag_array = [Tag() for _ in range(MAX_TAG_NUM)]

all_write_size = 0
all_empty_tag_write_size = 0
all_read_size = 0
all_empty_tag_read_size = 0
all_write_num = 0
all_empty_tag_write_num = 0

picture_addr = r"./Picture/决赛_训练数据1"

max_object_id = 0





