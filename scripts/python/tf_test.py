import numpy as np
import tensorflow as tf
#tf.InteractiveSession()

import tensorflow as tf
tf.enable_eager_execution()
matrix1 = tf.constant([[3., 3.]])
matrix2 = tf.constant([[2.],[2.]])
product = tf.matmul(matrix1, matrix2)

#print the product
print(product)         # tf.Tensor([[12.]], shape=(1, 1), dtype=float32)
print(product.numpy()) # [[12.]]

#Variable
normal_rv = tf.Variable( tf.truncated_normal([2,3],stddev = 0.1))
print(normal_rv)

#
nTimeSeriesInput = 10
kernelSize = 2
nControlInput = 3
nFilter = 1
w = tf.Variable( tf.truncated_normal([kernelSize + nControlInput, nFilter],stddev = 0.1))
print(w)
n1 = 9
layer = tf.zeros([n1, nFilter], tf.dtypes.float32)
print(layer)
values = [1., 2., 3., 4., 5., 6., 7., 8., 9., 10.]
layer2 = tf.convert_to_tensor(values, dtype=tf.float32)
print(layer2)





nTimeSeriesInput = 10
kernelSize = 2
nControlInput = 3
nFilter = 2
stride = 2
x = np.random.randint(-1, 3, [nTimeSeriesInput + nControlInput, 1])
w = np.random.randint(-1, 3, [kernelSize + nControlInput, nFilter])
temp = np.zeros([nTimeSeriesInput + nControlInput, nFilter])
lowIndex = 0
highIndex = 2
x
w
temp
temp[i] = np.matmul(np.append(x[lowIndex:highIndex], x[nTimeSeriesInput:]), w)
temp



nTimeSeriesInput = 10
kernelSize = 2
nControlInput = 3
nFilter = 2
stride = 2
x = np.random.randint(-1, 3, [nTimeSeriesInput + nControlInput, 1])
w = tf.Variable( tf.truncated_normal([kernelSize + nControlInput, nFilter],stddev = 0.1))
temp = np.zeros([nTimeSeriesInput + nControlInput, nFilter])
lowIndex = 0
highIndex = 2
x
w
temp
temp[i] = np.matmul(np.append(x[lowIndex:highIndex], x[nTimeSeriesInput:]), w.eval()
temp



inputMatrix = np.zeros([n1, kernelSize + nControlInput])
for i, offset in enumerate(np.arange(0, nTimeSeriesInput, stride)):
    lowIndex = nTimeSeriesInput - stride * (offset + 1)
    highIndex = nTimeSeriesInput - stride * offset
    inputMatrix[i, 0:kernelSize] = x[lowIndex:highIndex, 0]
    inputMatrix[i, kernelSize:] = x[nTimeSeriesInput:, 0]



for i in np.arange(0, nTimeSeriesInput, stride):
    lowIndex = nTimeSeriesInput - stride * (i + 1)
    highIndex = nTimeSeriesInput - stride * i
    #temp[i] = tf.matmul(np.append(x[lowIndex:highIndex], x[nTimeSeriesInput:]), weights['h1'])
    temp[i] = np.matmul(np.append(x[lowIndex:highIndex], x[nTimeSeriesInput:]), w)
layers.append(tf.convert_to_tensor(temp))
