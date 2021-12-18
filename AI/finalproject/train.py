import argparse

import torch
from torch.utils.data import DataLoader
from torch.utils.data.dataset import random_split
import matplotlib.pyplot as plt
# %matplotlib inline

from model import ModelClass
from model import EmbeddingLayer
from utils import RecommendationDataset

if __name__ == '__main__':

    # hyperparameter
    train_size, valid_size = 0.9, 0.1
    batch_size = 32
    learning_rate = 0.001
    num_epoch = 30
    weight_decay = 0

    parser = argparse.ArgumentParser(description='2021 AI Final Project')
    parser.add_argument('--save-model', default='model.pt', help="Model's state_dict")
    parser.add_argument('--dataset', default='./data', help='dataset directory')
    parser.add_argument('--batch-size', default=batch_size, help='train loader batch size')

    args = parser.parse_args()

    # load dataset in train folder
    train_data = RecommendationDataset("data/ratings.csv", train=True)
    _, _, n_ratings = train_data.get_datasize()

    # 학습 데이터와 검증 데이터 분할
    n_train = int(len(train_data) * train_size)
    n_val = len(train_data) - n_train
    train_data, valid_data = random_split(train_data, [n_train, n_val])

    # data loader
    train_loader = DataLoader(train_data, batch_size=batch_size, shuffle=True)
    valid_loader = DataLoader(valid_data, batch_size=batch_size, shuffle=True)

    # instantiate model
    model = EmbeddingLayer()
    optimizer = torch.optim.Adam(model.parameters(), lr=learning_rate, weight_decay=weight_decay)
    criterion = torch.nn.MSELoss()

    train_cost_arr = []
    valid_cost_arr = []
    for epoch in range(num_epoch):
        cost = 0
        for users, items, ratings in train_loader:
            optimizer.zero_grad()
            ratings_pred = model(users, items)
            loss = criterion(ratings_pred, ratings)
            loss.backward()
            optimizer.step()
            cost += loss.item()*len(ratings)
        cost/=(n_ratings*train_size)
        train_cost_arr.append(cost)
        print("Epoch: {} | train cost: {:.6f}".format(epoch, cost))

        cost_valid = 0
        for users, items, ratings in valid_loader:
            ratings_pred = model(users, items)
            loss = criterion(ratings_pred, ratings)
            cost_valid += loss.item()*len(ratings)
        cost_valid/=(n_ratings*valid_size)
        valid_cost_arr.append(cost_valid)
        print("Epoch: {} | valid cost: {:.6f}".format(epoch, cost_valid))
        print()

        # early stopping
        if 0 < epoch and valid_cost_arr[epoch-1] < cost_valid :
            print("------ Early Stopping : validation cost increase ------")
            break
    torch.save(model.state_dict(), args.save_model)

    # 그래프 출력
    x_train = list(range(len(train_cost_arr)))
    x_valid = list(range(len(valid_cost_arr)))
    plt.plot(x_train, train_cost_arr, x_valid, valid_cost_arr)
    plt.legend(["Train", "Val"])
    print("Minimum train cost:", min(train_cost_arr))
    print("Minimum validation cost:", min(valid_cost_arr))
    plt.show()
