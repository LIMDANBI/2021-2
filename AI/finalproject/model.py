import torch
from torch import nn

# 기본 모델
class ModelClass(nn.Module):

    def __init__(self, num_users=610, num_items=193609, rank=10):
        super().__init__()
        self.U = torch.nn.Parameter(torch.randn(num_users + 1, rank))
        self.V = torch.nn.Parameter(torch.randn(num_items + 1, rank))

    def forward(self, users, items):
        ratings = torch.sum(self.U[users] * self.V[items], dim=-1)
        return ratings

# 임베딩 모델
class EmbeddingLayer(nn.Module):
    def __init__(self, num_users=610, num_items=193609, rank=10):
        super().__init__()
        self.U = nn.Embedding(num_users + 1, rank)
        self.V = nn.Embedding(num_items + 1, rank)

        self.U.weight.data.uniform_(0, 0.05)
        self.V.weight.data.uniform_(0, 0.05)

    def forward(self, users, items):
        ratings = torch.sum(self.U(users) * self.V(items), dim=-1)
        return ratings