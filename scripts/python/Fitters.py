import glob
import os
import sys
import math
import time
import warnings
with warnings.catch_warnings():
    warnings.filterwarnings("ignore", category=FutureWarning)
    import tensorflow as tf
import xgboost as xgb
import lightgbm as lgb
import numpy as np
import pandas as pd
import InputNormalizer
import InputReader
import MFtn
from sklearn.metrics import accuracy_score

def get_nn(x, n_input, n_hidden, drop_rate=0.):
    weights = {}
    weights['h1'] = tf.Variable(tf.random.normal([n_input, n_hidden[0]]), dtype=tf.float32)
    for i in np.arange(1, len(n_hidden)):
        weights['h{}'.format(i+1)] = tf.Variable(tf.random.normal([n_hidden[i-1], n_hidden[i]]))
    weights['out'] = tf.Variable(tf.random.normal([n_hidden[-1], 1]), dtype=tf.float32)
    print(weights)

    biases = {}
    for i in range(len(n_hidden)):
        biases['b{}'.format(i+1)] = tf.Variable(tf.random.normal([n_hidden[i]]), dtype=tf.float32)
    biases['out'] = tf.Variable(tf.random.normal([1]), dtype=tf.float32)
    print(biases)

    layers = []
    layers.append(tf.add(tf.matmul(x, weights['h1']), biases['b1']))
    layers[0] = tf.nn.relu(layers[0])
    if drop_rate > 0.:
        layers[0] = tf.nn.dropout(layers[0], rate=drop_rate)
    for i in np.arange(1, len(n_hidden)):
        layers.append(tf.add(tf.matmul(layers[i-1], weights['h{}'.format(i+1)]), biases['b{}'.format(i+1)]))
        layers[i] = tf.nn.relu(layers[i])
        if drop_rate > 0.:
            layers[i] = tf.nn.dropout(layers[i], rate=drop_rate)
    layer_out = tf.matmul(layers[-1], weights['out']) + biases['out']
    print(layers)
    sys.stdout.flush()
    return layer_out

def trainDefault(x, y, pred, fitData, testData, adam_learning_rate, n_epochs, batch_size, display_step, model, udate, modelDir):
    cost = tf.reduce_mean(tf.compat.v1.losses.mean_squared_error(labels=y, predictions=pred))
    #optimizer = tf.train.GradientDescentOptimizer(learning_rate = grad_learning_rate).minimize(cost) # rate 0.001 is too large
    optimizer = tf.compat.v1.train.AdamOptimizer(learning_rate = adam_learning_rate).minimize(cost)
    saver = tf.compat.v1.train.Saver(max_to_keep=10)
    
    start = time.time()
    with tf.compat.v1.Session() as sess:
        sess.run(tf.compat.v1.global_variables_initializer())
        for epoch in range(n_epochs):
            avg_cost = 0.
            total_batch = int(len(fitData.x) / batch_size)
            x_batches = np.array_split(fitData.x, total_batch)
            y_batches = np.array_split(fitData.y, total_batch)
            for i in range(total_batch):
                batch_x, batch_y = x_batches[i], y_batches[i]
                _, c = sess.run([optimizer, cost], feed_dict={x:batch_x, y:batch_y})
                avg_cost += c / total_batch
    
            if epoch % display_step == 0:
                end = time.time()
                modelName = '{}_{}_ep{}'.format(model, udate, epoch+1)
                saver.save(sess, './{}/model_{}.ckpt'.format(modelDir, modelName))
    
                # Test
                pred_fit, cost_fit = sess.run([pred, cost], feed_dict={x:fitData.x, y:fitData.y})
                corrFit = np.corrcoef(pred_fit[:,0], fitData.y[:,0])[0,1]

                predictions = pred_fit[:,0]
                print('pred mean {:.4f} stdev {:.4f} min {:.4f} max {:.4f} '.format(predictions.mean(), predictions.std(), predictions.min(), predictions.max()))
    
                pred_test, cost_test = sess.run([pred, cost], feed_dict={x:testData.x, y:testData.y})
                corrTest = np.corrcoef(pred_test[:,0], testData.y[:,0])[0,1]
                corrTest_0_100 = MFtn.getCorr(pred_test[:,0], testData.y[:,0], testData.sprd, 0, 100)
                print('Epoch: {:04d} cost= {:.1f} {:.1f} corr: {:.4f} Test cost: {:.1f} corr: {:.4f} (sprd 0-100): {:.4f} Elapsed: {:.1f}'.format(epoch+1, avg_cost, cost_fit, corrFit, cost_test, corrTest, corrTest_0_100, end-start))
                sys.stdout.flush()
    
        end = time.time()
        print('Elapsed: {:.2f} Batch Size: {}'.format(end - start, batch_size))

def get_conv1d_fc(x, nTimeSeriesInput, nControlInput, kernelSize, stride, nFilter, fc):
    n1 = len(np.arange(0, nTimeSeriesInput - kernelSize, stride)) + 1

    weights = {}
    weights['h1'] = tf.Variable(tf.random.normal([kernelSize + nControlInput, nFilter]), dtype=tf.float32)
    weights['h2'] = tf.Variable(tf.random.normal([nFilter * n1, fc]), dtype=tf.float32)
    weights['out'] = tf.Variable(tf.random.normal([fc, 1]), dtype=tf.float32)
    print(weights)

    biases = {}
    biases['b1'] = tf.Variable(tf.random.normal([nFilter]), dtype=tf.float32)
    biases['b2'] = tf.Variable(tf.random.normal([fc]), dtype=tf.float32)
    biases['out'] = tf.Variable(tf.random.normal([1]), dtype=tf.float32)
    print(biases)

    layers = []
    temp = []
    slice2 = x[:, nTimeSeriesInput:]
    for i in range(n1):
        lowIndex = max(0, nTimeSeriesInput - kernelSize - stride * i)
        highIndex = lowIndex + kernelSize

        slice1 = x[:, lowIndex:highIndex]
        concatenated = tf.concat([slice1, slice2], 1)
        temp.append(concatenated)
    print('temp shape', len(temp), temp[0].shape)
    stackedMat = tf.stack(temp, 1)
    print('stackedMat shape', stackedMat.shape, stackedMat[0].shape)
    product = tf.matmul(stackedMat, weights['h1']) + biases['b1']
    print('product shape', product.shape)
    flattenedProduct = tf.reshape(product, [-1, product.shape[1] * product.shape[2]])
    print('flattened product shape', flattenedProduct.shape)
    layers.append(flattenedProduct)

    layers.append(tf.add(tf.matmul(layers[-1], weights['h2']), biases['b2']))

    layer_out = tf.matmul(layers[-1], weights['out']) + biases['out']
    print(layers)
    sys.stdout.flush()
    return layer_out

def get_convThreeRow(x, nTimeSeriesInput, nControlInput, kernelSize, stride, nFilter):
    n1 = len(np.arange(0, nTimeSeriesInput - kernelSize, stride)) + 1

    weights = {}
    weights['h1'] = tf.Variable(tf.random.normal([3*kernelSize + nControlInput, nFilter]), dtype=tf.float32)
    weights['out'] = tf.Variable(tf.random.normal([nFilter * n1, 1]), dtype=tf.float32)
    print(weights)

    biases = {}
    biases['b1'] = tf.Variable(tf.random.normal([nFilter]), dtype=tf.float32)
    biases['out'] = tf.Variable(tf.random.normal([1]), dtype=tf.float32)
    print(biases)

    layers = []
    temp = []
    sliceCommon = x[:, -nControlInput:]
    for i in range(n1):
        lowIndex = max(0, nTimeSeriesInput - kernelSize - stride * i)
        highIndex = lowIndex + kernelSize

        slice1 = x[:, lowIndex:highIndex]
        slice2 = x[:, (lowIndex+nTimeSeriesInput):(highIndex+nTimeSeriesInput)]
        slice3 = x[:, (lowIndex+2*nTimeSeriesInput):(highIndex+2*nTimeSeriesInput)]
        concatenated = tf.concat([slice1, slice2, slice3, sliceCommon], 1)
        temp.append(concatenated)
        print(i, 'concatenated shape', concatenated.shape, lowIndex, highIndex, slice1.shape, slice2.shape, slice3.shape)
        print(x.shape, lowIndex, highIndex, lowIndex+nTimeSeriesInput, highIndex+nTimeSeriesInput, lowIndex+2*nTimeSeriesInput, highIndex+2*nTimeSeriesInput)
    print('temp shape', len(temp), temp[0].shape)
    stackedMat = tf.stack(temp, 1)
    print('stackedMat shape', stackedMat.shape, stackedMat[0].shape)
    product = tf.matmul(stackedMat, weights['h1']) + biases['b1']
    print('product shape', product.shape)
    flattenedProduct = tf.reshape(product, [-1, product.shape[1] * product.shape[2]])
    print('flattened product shape', flattenedProduct.shape)
    layers.append(flattenedProduct)

    layer_out = tf.matmul(layers[-1], weights['out']) + biases['out']
    print(layers)
    sys.stdout.flush()
    return layer_out

def get_conv1d(x, nTimeSeriesInput, nControlInput, kernelSize, stride, nFilter):
    n1 = len(np.arange(0, nTimeSeriesInput - kernelSize, stride)) + 1

    weights = {}
    weights['h1'] = tf.Variable(tf.random.normal([kernelSize + nControlInput, nFilter]), dtype=tf.float32)
    weights['out'] = tf.Variable(tf.random.normal([nFilter * n1, 1]), dtype=tf.float32)
    print(weights)

    biases = {}
    biases['b1'] = tf.Variable(tf.random.normal([nFilter]), dtype=tf.float32)
    biases['out'] = tf.Variable(tf.random.normal([1]), dtype=tf.float32)
    print(biases)

    layers = []
    temp = []
    slice2 = x[:, nTimeSeriesInput:]
    for i in range(n1):
        lowIndex = max(0, nTimeSeriesInput - kernelSize - stride * i)
        highIndex = lowIndex + kernelSize

        slice1 = x[:, lowIndex:highIndex]
        concatenated = tf.concat([slice1, slice2], 1)
        temp.append(concatenated)
    print('temp shape', len(temp), temp[0].shape)
    stackedMat = tf.stack(temp, 1)
    print('stackedMat shape', stackedMat.shape, stackedMat[0].shape)
    product = tf.matmul(stackedMat, weights['h1']) + biases['b1']
    print('product shape', product.shape)
    flattenedProduct = tf.reshape(product, [-1, product.shape[1] * product.shape[2]])
    print('flattened product shape', flattenedProduct.shape)
    layers.append(flattenedProduct)

    layer_out = tf.matmul(layers[-1], weights['out']) + biases['out']
    print(layers)
    sys.stdout.flush()
    return layer_out

class TestFit:
    def __init__(self, market, sigType, udate, nfitdays, testFrom, testTo):
        self.market = market
        self.sigType = sigType
        self.udate = udate
        self.testFrom = testFrom
        self.testTo = testTo
        self.nfitdays = nfitdays
        self.n_epochs = 4
        self.batch_size = 40
        self.display_step = 1
        self.debug = False
        self.drop_rate = 0
        self.grad_learning_rate = 0.000001
        self.adam_learning_rate = 0.0001
        self.n_hidden = [120]
        self.modelDir = 'models'
        if not os.path.exists('./models'):
            os.mkdir(self.modelDir)

    def fit(self):
        with InputNormalizer.InputNormalizer(self.market, self.sigType, self.udate, self.nfitdays) as inpNorm:
            inpNorm.normalize()
            sys.stdout.flush()
        print('Reading fit data...')
        sys.stdout.flush()
        fitData = InputReader.InputReader.forNNFit(self.market, self.sigType, self.udate, self.nfitdays)
        sys.stdout.flush()
        print('Reading test data...')
        sys.stdout.flush()
        testData = InputReader.InputReader.forNNOOS(self.market, self.sigType, self.udate, self.testFrom, self.testTo)
        sys.stdout.flush()
        print('Training...')
        sys.stdout.flush()
        n_input = fitData.x.shape[1]
        
        x = tf.compat.v1.placeholder(tf.float32, [None, n_input])
        y = tf.compat.v1.placeholder(tf.float32, [None, 1])
        pred = get_nn(x, n_input, self.n_hidden, self.drop_rate)
        cost = tf.reduce_mean(tf.compat.v1.losses.mean_squared_error(labels=y, predictions=pred))
        #optimizer = tf.train.GradientDescentOptimizer(learning_rate = grad_learning_rate).minimize(cost) # rate 0.001 is too large
        optimizer = tf.compat.v1.train.AdamOptimizer(learning_rate = self.adam_learning_rate).minimize(cost)
        saver = tf.compat.v1.train.Saver(max_to_keep=10)
        
        start = time.time()
        with tf.compat.v1.Session() as sess:
            sess.run(tf.compat.v1.global_variables_initializer())
            for epoch in range(self.n_epochs):
                avg_cost = 0.
                total_batch = int(len(fitData.x) / self.batch_size)
                x_batches = np.array_split(fitData.x, total_batch)
                y_batches = np.array_split(fitData.y, total_batch)
                for i in range(total_batch):
                    batch_x, batch_y = x_batches[i], y_batches[i]
                    _, c = sess.run([optimizer, cost], feed_dict={x:batch_x, y:batch_y})
                    avg_cost += c / total_batch
        
                    if self.debug:
                        print('Epoch: {:04d} Batch: {:04d} Cost: {:.8f}'.format(epoch+1, i+1, c))
                        sys.stdout.flush()
        
                if epoch % self.display_step == 0:
                    end = time.time()
                    modelName = '{}_{}_{}_ep{}'.format(self.market, self.udate, self.nfitdays, epoch+1)
                    saver.save(sess, './{}/model_{}.ckpt'.format(self.modelDir, modelName))
        
                    # Test
                    pred_fit, cost_fit = sess.run([pred, cost], feed_dict={x:fitData.x, y:fitData.y})
                    corrFit = np.corrcoef(pred_fit[:,0], fitData.y[:,0])[0,1]
        
                    pred_test, cost_test = sess.run([pred, cost], feed_dict={x:testData.x, y:testData.y})
                    corrTest = np.corrcoef(pred_test[:,0], testData.y[:,0])[0,1]
                    corrTest_0_100 = MFTn.getCorr(pred_test[:,0], testData.y[:,0], testData.sprd, 0, 100)
                    print('Epoch: {:04d} cost= {:.1f} {:.1f} corr: {:.4f} Test cost: {:.1f} corr: {:.4f} (sprd 0-100): {:.4f} Elapsed: {:.1f}'.format(epoch+1, avg_cost, cost_fit, corrFit, cost_test, corrTest, corrTest_0_100, end-start))
                    sys.stdout.flush()
        
            end = time.time()
            print('Elapsed: {:.2f} Batch Size: {}'.format(end - start, self.batch_size))

class TestDeep30sThreeRow:
    def __init__(self, model, udate):
        self.model = model
        self.udate = udate
        self.n_epochs = 4
        self.batch_size = 4
        self.display_step = 1
        self.debug = False
        self.drop_rate = 0
        self.grad_learning_rate = 0.000001
        self.adam_learning_rate = 0.0001
        self.nTimeSeriesInput = 5
        self.nControlInput = 4
        self.kernelSize = 1
        self.stride = 1
        self.nFilter = 4
        self.modelDir = 'models'
        if not os.path.exists('./models'):
            os.mkdir(self.modelDir)

    def fit(self, fitData, testData):
        n_input = fitData.x.shape[1]

        print('nTimeSeriesInput {}'.format(self.nTimeSeriesInput))
        print('nControlInput {}'.format(self.nControlInput))
        print('kernelSize {}'.format(self.kernelSize))
        print('stride {}'.format(self.stride))
        print('nFilter {}'.format(self.nFilter))

        x = tf.compat.v1.placeholder(tf.float32, [None, n_input])
        y = tf.compat.v1.placeholder(tf.float32, [None, 1])
        pred = get_convThreeRow(x, self.nTimeSeriesInput, self.nControlInput, self.kernelSize, self.stride, self.nFilter)

        trainDefault(x, y, pred, fitData, testData, self.adam_learning_rate, self.n_epochs, self.batch_size, self.display_step, self.model, self.udate, self.modelDir)

    def oos(self, modelFile, oosData, fee):
        n_input = oosData.x.shape[1]
        x = tf.placeholder(tf.float32, [None, n_input])
        y = tf.placeholder(tf.float32, [None, 1])
        pred = get_conv1d(x, self.nTimeSeriesInput, self.nControlInput, self.kernelSize, self.stride, self.nFilter)
        saver = tf.train.Saver()
        with tf.Session() as sess:
            # Restore
            print(modelFile)
            saver.restore(sess, modelFile)
    
            # OOS
            y_pred_oos = sess.run(pred, feed_dict={x:oosData.x})
            corrOos = np.corrcoef(y_pred_oos[:,0], oosData.y[:,0])[0,1]
    
            MFtn.printOOSAna(y_pred_oos[:,0], oosData.y[:,0], oosData.sprd, oosData.nDays, thres=0, fee=fee)

class TestDeep30sConv1d:
    def __init__(self, model, udate):
        self.model = model
        self.udate = udate
        self.n_epochs = 4
        self.batch_size = 4
        self.display_step = 1
        self.debug = False
        self.drop_rate = 0
        self.grad_learning_rate = 0.000001
        self.adam_learning_rate = 0.0001
        self.nTimeSeriesInput = None
        self.nControlInput = None
        self.kernelSize = None
        self.stride = None
        self.nFilter = None
        self.modelDir = 'models'
        if not os.path.exists('./models'):
            os.mkdir(self.modelDir)

    def fit(self, fitData, testData):
        n_input = fitData.x.shape[1]

        print('nTimeSeriesInput {}'.format(self.nTimeSeriesInput))
        print('nControlInput {}'.format(self.nControlInput))
        print('kernelSize {}'.format(self.kernelSize))
        print('stride {}'.format(self.stride))
        print('nFilter {}'.format(self.nFilter))

        x = tf.compat.v1.placeholder(tf.float32, [None, n_input])
        y = tf.compat.v1.placeholder(tf.float32, [None, 1])
        pred = get_conv1d(x, self.nTimeSeriesInput, self.nControlInput, self.kernelSize, self.stride, self.nFilter)

        trainDefault(x, y, pred, fitData, testData, self.adam_learning_rate, self.n_epochs, self.batch_size, self.display_step, self.model, self.udate, self.modelDir)

    def oos(self, modelFile, oosData, fee):
        n_input = oosData.x.shape[1]
        x = tf.placeholder(tf.float32, [None, n_input])
        y = tf.placeholder(tf.float32, [None, 1])
        pred = get_conv1d(x, self.nTimeSeriesInput, self.nControlInput, self.kernelSize, self.stride, self.nFilter)
        saver = tf.train.Saver()
        with tf.Session() as sess:
            # Restore
            print(modelFile)
            saver.restore(sess, modelFile)
    
            # OOS
            y_pred_oos = sess.run(pred, feed_dict={x:oosData.x})
            corrOos = np.corrcoef(y_pred_oos[:,0], oosData.y[:,0])[0,1]
    
            MFtn.printOOSAna(y_pred_oos[:,0], oosData.y[:,0], oosData.sprd, oosData.nDays, thres=0, fee=fee)

class TestDeep30s:
    def __init__(self, model, udate):
        self.model = model
        self.udate = udate
        self.n_epochs = 4
        self.batch_size = 40
        self.display_step = 1
        self.debug = False
        self.drop_rate = 0
        self.grad_learning_rate = 0.000001
        self.adam_learning_rate = 0.0001
        self.n_hidden = [10]
        self.modelDir = 'models'
        if not os.path.exists('./models'):
            os.mkdir(self.modelDir)

    def fit(self, fitData, testData):
        n_input = fitData.x.shape[1]

        x = tf.compat.v1.placeholder(tf.float32, [None, n_input])
        y = tf.compat.v1.placeholder(tf.float32, [None, 1])
        pred = get_nn(x, n_input, self.n_hidden, self.drop_rate)
        trainDefault(x, y, pred, fitData, testData, self.adam_learning_rate, self.n_epochs, self.batch_size, self.display_step, self.model, self.udate, self.modelDir)

    def oos(self, modelFile, oosData, fee):
        n_input = oosData.x.shape[1]
        x = tf.placeholder(tf.float32, [None, n_input])
        y = tf.placeholder(tf.float32, [None, 1])
        pred = get_nn(x, n_input, self.n_hidden)
        saver = tf.train.Saver()
        with tf.Session() as sess:
            # Restore
            print(modelFile)
            saver.restore(sess, modelFile)
    
            # OOS
            y_pred_oos = sess.run(pred, feed_dict={x:oosData.x})
            corrOos = np.corrcoef(y_pred_oos[:,0], oosData.y[:,0])[0,1]
    
            MFtn.printOOSAna(y_pred_oos[:,0], oosData.y[:,0], oosData.sprd, oosData.nDays, thres=0, fee=fee)

class TestXgbFit:
    def __init__(self, market, sigType, udate, nfitdays, testFrom, testTo, max_depth=1000, min_child_weight=5000,
            eta=.1, subsample=1, colsample_bytree=1, L2reg=0, tree_method=None):
        self.market = market
        self.sigType = sigType
        self.udate = udate
        self.testFrom = testFrom
        self.testTo = testTo
        self.nfitdays = nfitdays
        self.debug = False
        self.modelDir = 'model_d{}_n{}_e{:.2f}_ss{:.1f}_cst{:.1f}_L2{:.1f}'.format(max_depth, min_child_weight, eta, subsample, colsample_bytree, L2reg)
        if not os.path.exists(self.modelDir):
            os.mkdir(self.modelDir)
            os.mkdir(self.modelDir + '/auto_saved')

        self.xgbParam = {'max_depth': max_depth, 'min_child_weight': min_child_weight, 'eta': eta, 'objective': 'reg:squarederror'}
        self.xgbParam['model_dir'] = self.modelDir + '/auto_saved/'
        self.xgbParam['subsample'] = subsample
        self.xgbParam['save_period'] = 10
        self.xgbParam['colsample_bytree'] = colsample_bytree
        self.xgbParam['nthread'] = 32
        if tree_method is not None:
            self.xgbParam['tree_method'] = tree_method
        self.xgbParam['lambda'] = L2reg
        self.xgbParam['base_score'] = 0

    def read(self):
        print('Reading fit data...')
        sys.stdout.flush()
        self.fitData = InputReader.InputReader.forXgbFit(self.market, self.sigType, self.udate, self.nfitdays)
        sys.stdout.flush()
        print('Reading test data...')
        sys.stdout.flush()
        self.testData = InputReader.InputReader.forXgbOOS(self.market, self.sigType, self.udate, self.testFrom, self.testTo)
        sys.stdout.flush()
        print('Training...')
        sys.stdout.flush()

        print(self.fitData.x.shape)
        print(self.fitData.y.shape)

    def fit(self, num_round=2):
        self.ntrees = num_round

        dtrain = xgb.DMatrix(fitData.x, label=fitData.y)
        dtest = xgb.DMatrix(testData.x, label=testData.y)
        
        evallist = [(dtest, 'eval'), (dtrain, 'train')]
        bst = xgb.train(self.xgbParam, dtrain, num_round, evallist)
        bst.save_model('{}/my.model'.format(self.modelDir))
        bst.dump_model('{}/my.model.txt'.format(self.modelDir))

    def oos(self, testFrom, testTo, fee, nthread=16, ntree_list=None):
        oosData = InputReader.InputReader.forXgbOOS(self.market, self.sigType, self.udate, testFrom, testTo)
        dtest = xgb.DMatrix(oosData.x, label=oosData.y)

        model_file = '{}/my.model'.format(self.modelDir)
        print(model_file)
        bst = xgb.Booster({'nthread':nthread})
        bst.load_model(model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        for nt in ntree_list:
            y_pred_oos = bst.predict(dtest, ntree_limit=nt)
            MFtn.printOOSAna(y_pred_oos, oosData.y, oosData.sprd, oosData.nDays, thres=0, fee=fee)

class TestLGBMFit:
    def __init__(self, market, udate, ntrees, max_depth=0, num_leaves=1000, min_data=5000,
            tree_method=None):
        self.market = market
        self.udate = udate

        self.ntrees = ntrees
        self.debug = False
        self.modelDir = 'model_l{}_n{}_d{}'.format(num_leaves, min_data, max_depth)
        if not os.path.exists(self.modelDir):
            os.mkdir(self.modelDir)

        self.param = {'num_leaves': num_leaves, 'objective': 'regression', 'min_data': min_data, 'verbose': -1}
        if max_depth is not None:
            self.param['max_depth'] = max_depth


    def fit(self, fitData):
        print(fitData.x.shape)
        print(fitData.y.shape)

        train_data = lgb.Dataset(fitData.x, label=fitData.y)
        bst = lgb.train(self.param, train_data, self.ntrees)
        bst.save_model('{}/my.model.txt'.format(self.modelDir))

    def validate(self, fitData, validationData, fee, ntree_list=None):
        dFit = fitData.x
        dValid = validationData.x

        model_file = '{}/my.model.txt'.format(self.modelDir)
        print(model_file)
        bst = lgb.Booster(model_file = model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        for nt in ntree_list:
            print('### ntrees {}'.format(nt))
            print("Fit:")
            yFit = bst.predict(dFit, num_iteration=nt)
            MFtn.printOOSAna(yFit, fitData.y, fitData.sprd, fitData.nDays, thres=0, fee=fee)
            print("Validation:")
            yValid = bst.predict(dValid, num_iteration=nt)
            MFtn.printOOSAna(yValid, validationData.y, validationData.sprd, validationData.nDays, thres=0, fee=fee)

    def oos(self, oosData, fee, nthread=16, ntree_list=None):
        dtest = oosData.x

        model_file = '{}/my.model.txt'.format(self.modelDir)
        print(model_file)
        bst = lgb.Booster(model_file = model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        for nt in ntree_list:
            print('### ntrees {}'.format(nt))
            y_pred_oos = bst.predict(dtest, num_iteration=nt)
            MFtn.printOOSAna(y_pred_oos, oosData.y, oosData.sprd, oosData.nDays, thres=0, fee=fee)

class TestLGBMFitDirectionClass:
    def __init__(self, market, ntrees, max_depth=0, num_leaves=1000, min_data=5000,
            L2reg=0, tree_method=None):
        self.market = market

        self.ntrees = ntrees
        self.debug = False
        if L2reg > 0:
            self.modelDir = 'model_l{}_n{}_d{}_reg{:.1f}'.format(num_leaves, min_data, max_depth, L2reg)
        else:
            self.modelDir = 'model_l{}_n{}_d{}'.format(num_leaves, min_data, max_depth)
        if not os.path.exists(self.modelDir):
            os.mkdir(self.modelDir)

        self.param = {'num_leaves': num_leaves, 'objective': 'binary', 'min_data': min_data, 'verbose': -1}
        self.param['lambda_l2'] = L2reg
        if max_depth is not None:
            self.param['max_depth'] = max_depth


    def fit(self, fitData):
        print(fitData.x.shape)
        print(fitData.y.shape)

        train_data = lgb.Dataset(fitData.x, label=fitData.y)
        bst = lgb.train(self.param, train_data, self.ntrees)
        bst.save_model('{}/my.model.txt'.format(self.modelDir))

    def validate(self, fitData, validationData, ntree_list=None):
        dFit = fitData.x
        dValid = validationData.x

        model_file = '{}/my.model.txt'.format(self.modelDir)
        print(model_file)
        bst = lgb.Booster(model_file = model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        for nt in ntree_list:
            yFit = bst.predict(dFit, num_iteration=nt)
            yValid = bst.predict(dValid, num_iteration=nt)
            predFit = np.where(yFit > .5, 1, 0)
            predValid = np.where(yValid > .5, 1, 0)
            nPos = predValid.sum()
            accFit = accuracy_score(fitData.y, predFit)
            accValid = accuracy_score(validationData.y, predValid)
            print('nt {} pos {:0} neg {:0} accValid {:.4f} accFit {:.4f}'.format(nt, nPos, yValid.shape[0] - nPos, accValid, accFit))

    def validate_tradable(self, fitData, validationData, fitTarget, fitTargetPred, validTarget, validTargetPred, ntree_list=None):
        dFit = fitData.x
        dValid = validationData.x

        model_file = '{}/my.model.txt'.format(self.modelDir)
        print(model_file)
        bst = lgb.Booster(model_file = model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        dfFit = pd.DataFrame({'targe':fitTarget, 'targetPred':fitTargetPred})
        dfValid = pd.DataFrame({'targe':validTarget, 'targetPred':validTargetPred})

        for nt in ntree_list:
            yFit = bst.predict(dFit, num_iteration=nt)
            predFit = np.where(yFit > .5, 1, 0)
            dfFit['y'] = fitData.y
            dfFit['targetPred'] = fitTargetPred
            dfFit['pred'] = predFit
            dfFit['sprd'] = fitData.sprd
            dfFitTradable = dfFit[dfFit['targetPred'].abs() > .5*dfFit['sprd']]
            accFit = accuracy_score(dfFitTradable['y'], dfFitTradable['pred'])

            yValid = bst.predict(dValid, num_iteration=nt)
            predValid = np.where(yValid > .5, 1, 0)
            dfValid['y'] = validationData.y
            dfValid['targetPred'] = validTargetPred
            dfValid['pred'] = predValid
            dfValid['sprd'] = validationData.sprd
            dfValidTradable = dfValid[dfValid['targetPred'].abs() > .5*dfValid['sprd']]
            accValid = accuracy_score(dfValidTradable['y'], dfValidTradable['pred'])

            nPos = predValid.sum()
            print('nt {} pos {:0} neg {:0} accValid {:.4f} accFit {:.4f}'.format(nt, nPos, yValid.shape[0] - nPos, accValid, accFit))

    def oos(self, oosData, nthread=16, ntree_list=None):
        dtest = oosData.x

        model_file = '{}/my.model.txt'.format(self.modelDir)
        print(model_file)
        bst = lgb.Booster(model_file = model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        for nt in ntree_list:
            y_pred_oos = bst.predict(dtest, num_iteration=nt)
            pred = np.where(y_pred_oos > .5, 1, 0)
            nPos = pred.sum()
            acc = accuracy_score(oosData.y, pred)
            print('nt {} pos {:0} neg {:0} accuray {:.4f}'.format(nt, nPos, y_pred_oos.shape[0] - nPos, acc))

class TestLGBMFitEdgeClass:
    def __init__(self, market, ntrees, max_depth=0, num_leaves=1000, min_data=5000,
            tree_method=None):
        self.market = market
        self.ntrees = ntrees
        self.debug = False
        self.modelDir = 'model_l{}_n{}_d{}'.format(num_leaves, min_data, max_depth)
        if not os.path.exists(self.modelDir):
            os.mkdir(self.modelDir)

        self.param = {'num_leaves': num_leaves, 'objective': 'multiclass', 'min_data': min_data, 'verbose': -1}
        self.param['num_class'] = 3
        self.param['metric'] = {'multiclass'}
        if max_depth is not None:
            self.param['max_depth'] = max_depth


    def fit(self, fitData):
        print(fitData.x.shape)
        print(fitData.y.shape)

        train_data = lgb.Dataset(fitData.x, label=fitData.y)
        bst = lgb.train(self.param, train_data, self.ntrees)
        bst.save_model('{}/my.model.txt'.format(self.modelDir))

    def validate(self, fitData, validationData, ntree_list=None):
        dFit = fitData.x
        dValid = validationData.x

        model_file = '{}/my.model.txt'.format(self.modelDir)
        print(model_file)
        bst = lgb.Booster(model_file = model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        for nt in ntree_list:
            yFit = bst.predict(dFit, num_iteration=nt)
            predFit = []
            for x in yFit:
                predFit.append(np.argmax(x))
            accFit = accuracy_score(fitData.y, predFit)

            yValid = bst.predict(dValid, num_iteration=nt)
            predValid = []
            for x in yValid:
                predValid.append(np.argmax(x))
            accValid = accuracy_score(validationData.y, predValid)

            print('nt {} accValid {:.4f} accFit {:.4f}'.format(nt, accValid, accFit))

    def validate_tradable(self, fitData, validationData, fitTarget, fitTargetPred, validTarget, validTargetPred, ntree_list=None):
        dFit = fitData.x
        dValid = validationData.x

        model_file = '{}/my.model.txt'.format(self.modelDir)
        print(model_file)
        bst = lgb.Booster(model_file = model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        dfFit = pd.DataFrame({'targe':fitTarget, 'targetPred':fitTargetPred})
        dfValid = pd.DataFrame({'targe':validTarget, 'targetPred':validTargetPred})

        for nt in ntree_list:
            yFit = bst.predict(dFit, num_iteration=nt)
            predFit = []
            for x in yFit:
                predFit.append(np.argmax(x))
            dfFit['y'] = fitData.y
            dfFit['pred'] = predFit
            dfFit['sprd'] = fitData.sprd
            dfFitTradable = dfFit[dfFit['pred'].abs() > .5*dfFit['sprd']]
            accFit = accuracy_score(dfFitTradable['y'], dfFitTradable['pred'])

            yValid = bst.predict(dValid, num_iteration=nt)
            predValid = []
            for x in yValid:
                predValid.append(np.argmax(x))
            dfValid['y'] = validationData.y
            dfValid['pred'] = predValid
            dfValid['sprd'] = validationData.sprd
            dfValidTradable = dfValid[dfValid['pred'].abs() > .5*dfValid['sprd']]
            accValid = accuracy_score(dfValidTradable['y'], dfValidTradable['pred'])

            print('nt {} accValid {:.4f} accFit {:.4f}'.format(nt, accValid, accFit))

    def oos(self, oosData, nthread=16, ntree_list=None):
        dtest = oosData.x

        model_file = '{}/my.model.txt'.format(self.modelDir)
        print(model_file)
        bst = lgb.Booster(model_file = model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        for nt in ntree_list:
            yOos = bst.predict(dtest, num_iteration=nt)
            pred = []
            for x in yOos:
                pred.append(np.argmax(x))
            acc = accuracy_score(oosData.y, pred)

            print('nt {} accuray {:.2f}'.format(nt, acc))

    def oos_tradable(self, oosData, oosTarget, oosTargetPred, ntree_list=None):
        dtest = oosData.x

        model_file = '{}/my.model.txt'.format(self.modelDir)
        print(model_file)
        bst = lgb.Booster(model_file = model_file)

        if ntree_list is None:
            interval = max(1, self.ntrees // 8)
            rangeFrom = self.ntrees % interval + interval
            rangeTo = self.ntrees + interval
            ntree_list = np.arange(rangeFrom, rangeTo, interval)

        dfValid = pd.DataFrame({'targe':oosTarget, 'targetPred':oosTargetPred})

        for nt in ntree_list:
            yValid = bst.predict(dtest, num_iteration=nt)
            predValid = []
            for x in yValid:
                predValid.append(np.argmax(x))
            dfValid['y'] = oosData.y
            dfValid['targetPred'] = oosTargetPred
            dfValid['pred'] = predValid
            dfValid['sprd'] = oosData.sprd
            dfValidTradable = dfValid[dfValid['targetPred'].abs() > .5*dfValid['sprd']]
            accValid = accuracy_score(dfValidTradable['y'], dfValidTradable['pred'])

            Print('nt {} accOos {:.4f}'.format(nt, accValid))

