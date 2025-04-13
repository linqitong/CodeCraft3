import os


def clear_txt_files(folder_path):
    # 遍历文件夹中的所有文件
    for filename in os.listdir(folder_path):
        # 检查文件是否以 .txt 结尾
        if filename.endswith(".txt"):
            file_path = os.path.join(folder_path, filename)
            # 打开文件并清空内容
            with open(file_path, "w") as file:
                file.truncate(0)
            print(f"已清空文件: {filename}")


# 示例用法
folder_path = "../disk"  # 替换为你的文件夹路径
clear_txt_files(folder_path)