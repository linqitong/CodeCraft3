import numpy as np
import pandas as pd
from sklearn.preprocessing import LabelEncoder
from sklearn.model_selection import TimeSeriesSplit
from sklearn.metrics import accuracy_score
import lightgbm as lgb
from tqdm import tqdm


def prepare_features(df, window_size=5):
    """生成时间序列特征（排除label=0的数据）"""
    # 过滤有效标签数据生成特征
    valid_labels = df[df['label'] > 0]

    # 统计每个时间步各标签的数量
    label_counts = valid_labels.groupby(['timestamp_id', 'label']).size().unstack(fill_value=0)

    # 填充缺失的时间步
    full_range = np.arange(df['timestamp_id'].min(), df['timestamp_id'].max() + 1)
    label_counts = label_counts.reindex(full_range, fill_value=0)

    # 计算滑动窗口特征
    features = []
    for i in tqdm(range(len(label_counts))):
        window = label_counts.iloc[max(0, i - window_size):i + 1]
        features.append({
            'timestamp_id': label_counts.index[i],
            'current_total': window.sum(axis=1)[-1],
            **{f'label_{l}_mean': window[l].mean() for l in label_counts.columns},
            **{f'label_{l}_trend': window[l].values[-1] - window[l].values[0]
            if len(window) > 1 else 0 for l in label_counts.columns}
        })

    features_df = pd.DataFrame(features)
    return features_df


def train_predict(df, features_df):
    """训练预测流程"""
    # 合并特征
    merged = df.merge(features_df, on='timestamp_id')

    # 分割有标签数据（label != 0）
    labeled_data = merged[merged['label'] > 0]
    X = labeled_data.drop(['label', 'read_label', 'timestamp_id', 'request_id'], axis=1)
    y = LabelEncoder().fit_transform(labeled_data['label'])  # 将1-16编码为0-15

    # 时间序列交叉验证
    tscv = TimeSeriesSplit(n_splits=3)
    models = []

    for train_index, val_index in tscv.split(X):
        X_train, X_val = X.iloc[train_index], X.iloc[val_index]
        y_train, y_val = y[train_index], y[val_index]

        params = {
            'objective': 'multiclass',
            'num_class': 16,
            'metric': 'multi_logloss',
            'num_leaves': 63,
            'learning_rate': 0.05,
            'feature_fraction': 0.8,
            'verbose': -1
        }

        model = lgb.train(
            params,
            lgb.Dataset(X_train, y_train),
            num_boost_round=1000,
            valid_sets=[lgb.Dataset(X_val, y_val)],
            callbacks=[lgb.early_stopping(50)]
        )
        models.append(model)

    # 预测无标签数据（label == 0）
    unlabeled = merged[merged['label'] == 0]
    X_pred = unlabeled.drop(['label', 'read_label', 'timestamp_id', 'request_id'], axis=1)

    # 集成预测
    pred_probs = np.zeros((len(X_pred), 16))
    for model in models:
        pred_probs += model.predict(X_pred)
    predicted_classes = np.argmax(pred_probs, axis=1)

    # 转换回原始标签（1-16）
    label_encoder = LabelEncoder()
    label_encoder.fit(labeled_data['label'])  # 确保编码器包含所有1-16标签
    predicted_labels = label_encoder.inverse_transform(predicted_classes)

    # 计算准确率
    true_labels = unlabeled['read_label'].values
    acc = accuracy_score(true_labels, predicted_labels)
    print(f"\n无标签数据预测准确率: {acc:.4f}")

    # 构建结果
    result = pd.DataFrame({
        'request_id': unlabeled['request_id'],
        'true_label': true_labels,
        'predicted_label': predicted_labels
    })

    return result


# 数据预处理示例
def load_data(path):
    df = pd.read_csv(path)
    # 确保数据格式正确
    assert {'timestamp_id', 'request_id', 'label', 'read_label'}.issubset(df.columns)
    assert df['read_label'].between(1, 16).all()
    return df


if __name__ == "__main__":
    # 加载数据（示例）
    df = load_data('data.csv')

    # 特征工程
    print("Generating features...")
    features_df = prepare_features(df)

    # 训练预测
    print("Training and predicting...")
    predictions = train_predict(df, features_df)

    # 保存结果
    predictions.to_csv('predictions_with_accuracy.csv', index=False)