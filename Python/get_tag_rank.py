import numpy as np
import random
import common
import math



def get_tag_rank():
    corr_matrix = np.random.rand(16, 16)

    for i in range(1, common.M_tag_num + 1):
        for j in range(1, common.M_tag_num + 1):
            length = (common.T_time_step_length - 1) / common.FRE_PER_SLICING + 1
            sumA = 0
            sumB = 0
            sumAB = 0.0
            sumA2 = 0.0
            sumB2 = 0.0
            for k in range(1, math.floor(length)):
                sumA += common.tag_array[i].fre_read[k]
                sumB += common.tag_array[j].fre_read[k]

            averageA = sumA / length
            averageB = sumB / length
            for k in range(1, common.M_tag_num + 1):
                sumAB += (common.tag_array[i].fre_read[k] - averageA) * (common.tag_array[j].fre_read[k] - averageB)
                sumA2 += (common.tag_array[i].fre_read[k] - averageA) * (common.tag_array[i].fre_read[k] - averageA)
                sumB2 += (common.tag_array[j].fre_read[k] - averageB) * (common.tag_array[j].fre_read[k] - averageB)

            numerator = sumAB
            denominator = sumA2**0.5 * sumB2**0.5
            corr_matrix[i - 1][j - 1] = numerator / denominator

    # 假设corr_matrix是16x16的相关系数矩阵，需替换为实际数据
    n = 16
    np.random.seed(0)

    np.fill_diagonal(corr_matrix, 1)  # 对角线为1


    def total_correlation(arrangement):
        total = 0.0
        n = len(arrangement)
        for i in range(n):
            j = (i + 1) % n
            total += corr_matrix[arrangement[i], arrangement[j]]
        return total


    def generate_2opt_neighbor(arr):
        a, b = sorted(random.sample(range(len(arr)), 2))
        return arr[:a] + arr[a:b + 1][::-1] + arr[b + 1:]


    # 模拟退火参数
    initial_temp = 1000
    cooling_rate = 0.995
    num_iterations = 5000

    current_arrangement = list(range(n))
    random.shuffle(current_arrangement)
    current_score = total_correlation(current_arrangement)

    best_arrangement = current_arrangement.copy()
    best_score = current_score

    temp = initial_temp

    for i in range(num_iterations):
        # 生成2-opt邻居
        new_arrangement = generate_2opt_neighbor(current_arrangement)
        new_score = total_correlation(new_arrangement)

        # 计算接受概率
        score_diff = new_score - current_score
        if score_diff > 0 or (np.exp(score_diff / temp) > random.random()):
            current_arrangement = new_arrangement
            current_score = new_score
            if current_score > best_score:
                best_arrangement = current_arrangement.copy()
                best_score = current_score

        # 降温
        temp *= cooling_rate

    print(f"最佳相关系数和: {best_score}")

    for i in range(len(best_arrangement)):
        best_arrangement[i] += 1
    print("最佳排列顺序:", best_arrangement)