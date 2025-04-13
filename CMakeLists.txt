cmake_minimum_required(VERSION 3.8)  # 不要修改
project(CodeCraft)                   # 不要修改

set(CMAKE_CXX_STANDARD      17)      # 不要修改
set(CMAKE_C_STANDARD        11)      # 不要修改

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/) # 不要修改
option(BUILD_SHARED_LIBS    ""  OFF) # 不要修改

aux_source_directory(./ cur_src) # 不要修改
aux_source_directory(./Class/ cur_src_class)
aux_source_directory(./Action/ cur_src_action)
# 如果需要，可以使用aux_source_directory增加目录

# add_executable 在 windows 环境下会自动带有后缀名
# 因此生成 code_craft.exe
add_executable(code_craft ${cur_src} ${cur_src_class} ${cur_src_action}) # ！！！不要修改 code_craft 名称，直接影响结果；可以根据语法在 ${cur_src} 后面追加

# 以下可以根据需要增加需要链接的库
#if (NOT WIN32)
#    target_link_libraries(code_craft  pthread  rt  m)
#endif (NOT WIN32)
