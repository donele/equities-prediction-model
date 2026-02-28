import math

def mytest():
    print("myStat")

class Acc():
    def __init__(self):
        self.sum = 0.
        self.sum2 = 0.
        self.n = 0.
    def add(self, x):
        self.sum += x
        self.sum2 += x*x
        self.n += 1
    def mean(self):
        if self.n == 0:
            return 0
        return self.sum / self.n
    def std(self):
        return math.sqrt(self.sum2 / self.n - self.mean() * self.mean())

