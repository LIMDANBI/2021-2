import torch
from torch import nn

class ModelClass(nn.Module):

    def __init__(self, num_users=610, num_items=193609, rank=10):
        """
        TODO: Write down your model
        """
        super().__init__()
        self.U = torch.nn.Parameter(torch.randn(num_users + 1, rank))
        self.V = torch.nn.Parameter(torch.randn(num_items + 1, rank))

    def forward(self, users, items):
        ratings = torch.sum(self.U[users] * self.V[items], dim=-1)
        return ratings

    # def __init__(self, num_users=610, num_items=193609, rank=10):
    #
    #     super().__init__()
    #     self.embed_user = nn.Embedding(num_users, rank)
    #     self.embed_item = nn.Embedding(num_items, rank)
    #     predict_size = rank
    #
    #     # 상수 Tensor 생성
    #     self.predict_layer = torch.ones(predict_size, 1).cuda()
    #     self._init_weight_()
    #
    # def _init_weight_(self):
    #     # weight 초기화
    #     nn.init.normal_(self.embed_user.weight, std=0.01)
    #     nn.init.normal_(self.embed_item.weight, std=0.01)
    #
    #     # bias 초기화
    #     for m in self.modules():
    #         if isinstance(m, nn.Linear) and m.bias is not None:
    #             m.bias.data.zero_()
    #
    # def forward(self, user, item):
    #     embed_user = self.embed_user(user)
    #     embed_item = self.embed_item(item)
    #     # Tensor의 원소별 곱셈
    #     output_GMF = embed_user * embed_item
    #     prediction = torch.matmul(output_GMF, self.predict_layer)
    #     return prediction.view(-1)