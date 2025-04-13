import os
import re
import argparse

folder_path = "../disk"


def process_file(file_path):
    last_turn = 0
    sum_j = 0
    sum_r = 0
    sum_p = 0
    sum_jnum = 0
    count = 0

    with open(file_path, 'r') as f:
        for line in f:
            # 使用正则表达式提取各个字段的数值
            try:
                turn = int(re.search(r'turn:(\d+)', line).group(1))
                j = int(re.search(r'j:(\d+)', line).group(1))
                r = int(re.search(r'r:(\d+)', line).group(1))
                p = int(re.search(r'p:(\d+)', line).group(1))
                jnum = int(re.search(r'j_num:(\d+)', line).group(1))
            except AttributeError:
                print(f"文件 {file_path} 中存在格式错误的行：{line.strip()}")
                continue

            # 更新总和和计数
            sum_j += j
            sum_r += r
            sum_p += p
            sum_jnum += jnum
            count += 1
            last_turn = turn

    if count == 0:
        print(f"文件 {file_path} 中没有有效数据。")
        return

    # 计算平均值
    avg_j = sum_j / count
    avg_r = sum_r / count
    avg_p = sum_p / count
    avg_jnum = sum_jnum / count

    # 输出结果
    print(f"文件：{file_path}")
    print(f"最后 turn 值：{last_turn}")
    print(f"j 总和：{sum_j}，平均值：{avg_j:.2f}")
    print(f"r 总和：{sum_r}，平均值：{avg_r:.2f}")
    print(f"p 总和：{sum_p}，平均值：{avg_p:.2f}")
    print(f"j_num 总和：{sum_jnum}，平均值：{avg_jnum:.2f}")
    print("-" * 50)


def main(folder_path):
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            if file.endswith('.txt'):
                file_path = os.path.join(root, file)
                process_file(file_path)


if __name__ == "__main__":
    main(folder_path)