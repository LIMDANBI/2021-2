import argparse

import torch
from torch.utils.data import DataLoader
from torch.utils.data.dataset import random_split

from model import ModelClass
from utils import RecommendationDataset

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='2021 AI Final Project')
    parser.add_argument('--save-model', default='model.pt', help="Model's state_dict")
    parser.add_argument('--dataset', default='./data', help='dataset directory')
    parser.add_argument('--batch-size', default=16, help='train loader batch size')

    args = parser.parse_args()

    # instantiate model
    model = ModelClass()

    # load dataset in train folder
    train_data = RecommendationDataset("data/ratings.csv", train=True)
    _, _, n_ratings = train_data.get_datasize()

    # 정규화에 사용
    lambda1 = 0.0001
    lambda2 = 0.0001

    # 학습 데이터와 검증 데이터 분할
    n_train = int(len(train_data) * 0.8)
    n_val = len(train_data) - n_train
    train_data, valid_data = random_split(train_data, [n_train, n_val])

    # data loader
    train_loader = DataLoader(train_data, batch_size=args.batch_size, shuffle=True)
    valid_loader = DataLoader(valid_data, batch_size=args.batch_size, shuffle=True)

    # Implement code for training the recommendation model
    optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
    criterion = torch.nn.MSELoss()

    for epoch in range(11):
        cost = 0
        for users, items, ratings in train_loader:
            optimizer.zero_grad()
            ratings_pred = model(users, items)
            loss = criterion(ratings_pred, ratings)
            loss.backward()
            optimizer.step()
            cost += loss.item()*len(ratings)
        cost = (cost + lambda1*torch.sum(model.U**2) + lambda2*torch.sum(model.V**2))/(n_ratings*0.8)
        print("Epoch: {} | train cost: {:.6f}".format(epoch, cost))

        cost_valid = 0
        for users, items, ratings in valid_loader:
            ratings_pred = model(users, items)
            loss = criterion(ratings_pred, ratings)
            cost_valid += loss.item()*len(ratings)
        cost_valid/=(n_ratings*0.2)
        print("Epoch: {} | valid cost: {:.6f}".format(epoch, cost_valid))
        print()
    torch.save(model.state_dict(), args.save_model)
