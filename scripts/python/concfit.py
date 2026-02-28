import numpy as np
import pdb
import sys
import glob
import os.path
import os
import collections.abc
from pandas import read_csv
from keras.models import Sequential, Model, load_model
from keras.layers.core import Dense, Activation, Dropout, Flatten
from keras.layers import Input, GRU, LSTM, TimeDistributed, Reshape, Conv2D, Merge, Lambda
from keras.layers import concatenate, add
from keras.layers.local import LocallyConnected1D, LocallyConnected2D
from keras.layers.normalization import BatchNormalization
from keras.optimizers import Adam
from keras.initializers import TruncatedNormal
from tensorflow.python.client import device_lib

def remove_zeros(x):
    assert isinstance(x, collections.abc.Sequence)
    seqType = type(x)
    return seqType(filter(lambda a: a > 0, x))

def parse_num_list(x):
    if not isinstance(x, collections.abc.Sequence):
        x = [x]
    return remove_zeros(x)

def print_model(model, log):
    #model.summary(print_fn=lambda x: log.write(x + '\n')) # 2.0.6
    orig_stdout = sys.stdout
    sys.stdout = log
    print(model.summary())
    sys.stdout = orig_stdout

################################################################################
# Create models. 
#
# getModelDense(): Dense
# getModelConn(): LocallyConnected2D Dense
# getModelConnGRU(): LocallyConnected2D GRU Dense
# getModelDenseCGRU(): Dense GRU Dense
# getModelLstm(): LSTM Dense
#                 Restriction on the input size.
# getModelUniv(): Universal model.
# getModelTwoPath(): path1: LocallyConnected2D (GRU)
#                    path2: Conv2d (GRU)
#                    path1 + path2 -> Dense
################################################################################

def getModelFC(log, inputShape, targetCount, nnode):
    '''Fully connected neural network.
    '''
    model = Sequential()
    model.add(Flatten(input_shape=inputShape[1:]))

    kinit = TruncatedNormal(0, 0.001)
    for n in nnode:
        model.add(Dense(n, kernel_initializer=kinit, activation='elu'))
        model.add(Dropout(0.5) )
    model.add(Dense(targetCount, kernel_initializer=kinit, activation='linear'))

    print_model(model, log)
    return model

def getModelConn(log, inputShape, targetCount, depth, lenconn, nnode=[]):
    '''LocallyConnected2D followed by fully connected layers.
    '''
    nInputTickers = inputShape[1]
    nInputVars = inputShape[2]
    nInputTickersDivLen = int(nInputTickers / lenconn)
    model = Sequential()
    model.add(Reshape((nInputTickers, nInputVars, 1), input_shape=(nInputTickers, nInputVars)))
    print('local connect size {}'.format(nInputTickers))
    for i, d in enumerate(depth):
        if i == 0:
            model.add(LocallyConnected2D(d, kernel_size=(nInputTickersDivLen, 1), strides=(nInputTickersDivLen, 1),
                activation='elu'))
        else:
            model.add(LocallyConnected2D(d, 1, activation='elu'))

    model.add(Flatten())
    kinit = TruncatedNormal(0, 0.001)
    for n in nnode:
        model.add(Dense(n, kernel_initializer=kinit, activation='elu'))
        model.add(Dropout(0.5))
    model.add(Dense(targetCount, kernel_initializer=kinit, activation='linear'))

    print_model(model, log)
    return model

def getModelLstm(log, inputShape, targetCount, nrecurr, nnode=[]):
    '''LSTM followed by fully connected layers.
    The input size is (batch_size, n_time_steps, n_inputs).
    Number of input variable must be one in this implementation.
    '''
    nTimeSlice = inputShape[1]
    nInputTickers = inputShape[2]
    kinit = TruncatedNormal(0, 0.001)
    model = Sequential()
    for i, n in enumerate(nrecurr):
        if i == 0:
            model.add(LSTM(n, input_shape=(nTimeSlice, nInputTickers),
                return_sequences=True, stateful=False))
        else:
            model.add(LSTM(n, return_sequences=True, stateful=False))
    for n in nnode:
        model.add(TimeDistributed(Dense(n, kernel_initializer=kinit, activation='elu')))
        model.add(Dropout(0.5))
    odel.add(TimeDistributed(Dense(targetCount, kernel_initializer=kinit)))

    print_model(model, log)
    return model

def getModelConnGRU(log, inputShape, targetCount, depth, lenconn, nrecurr=[], nnode=[]):
    '''LocallyConnected2D, GRU, and Dense.
    Can take many input variables.
    '''
    nTimeSlice = inputShape[1]
    nInputTickers = inputShape[2]
    nInputVars = inputShape[3]
    nInputTickersDivLen = int(nInputTickers / lenconn)

    kinit = TruncatedNormal(0, 0.001)
    model = Sequential()
    model.add(Reshape((nTimeSlice, nInputTickers, nInputVars, 1), input_shape=(nTimeSlice, nInputTickers, nInputVars)))
    print('local connect size {}'.format(nInputTickers))
    for i, d in enumerate(depth):
        if i == 0:
            model.add(TimeDistributed(LocallyConnected2D(d, kernel_size=(nInputTickersDivLen, 1), strides=(nInputTickersDivLen, 1), activation='elu')))
        else:
            model.add(TimeDistributed(LocallyConnected2D(d, 1, activation='elu')))

    model.add(Reshape((nTimeSlice, np.prod(model.output_shape[-2:]))))
    for i, n in enumerate(nrecurr):
        model.add(GRU(n, return_sequences=True))

    for n in nnode:
        model.add(TimeDistributed(Dense(n, kernel_initializer=kinit, activation='elu')))
        model.add(Dropout(0.5))
    model.add(TimeDistributed(Dense(targetCount, kernel_initializer=kinit, activation='linear')))
    model.add(Reshape((nTimeSlice, targetCount)))

    print_model(model, log)
    return model

def getModelDenseGRU(log, inputShape, targetCount, dense, nrecurr=[], nnode=[]):
    '''Dense, GRU, and Dense.
    Can take many input variables.
    '''
    nTimeSlice = inputShape[1]
    nInputTickers = inputShape[2]
    nInputVars = inputShape[3]

    kinit = TruncatedNormal(0, 0.001)
    model = Sequential()
    print('local connect size {}'.format(nInputTickers))
    for i, d in enumerate(dense):
        if i == 0:
            model.add(TimeDistributed(Dense(d, activation='elu'), input_shape=(nTimeSlice, nInputTickers, nInputVars)))
        else:
            model.add(TimeDistributed(Dense(d, activation='elu')))

    model.add(Reshape((nTimeSlice, np.prod(model.output_shape[-2:]))))
    for i, n in enumerate(nrecurr):
        model.add(GRU(n, return_sequences=True))

    for n in nnode:
        model.add(TimeDistributed(Dense(n, kernel_initializer=kinit, activation='elu')))
        model.add(Dropout(0.5))
    model.add(TimeDistributed(Dense(targetCount, kernel_initializer=kinit, activation='linear')))

    print_model(model, log)
    return model

def getModelUniv(log, inputShape, targetCount, depth, nnode):
    kinit = TruncatedNormal(0, 0.001)
    nInputTickers = inputShape[1]
    nInputVars = inputShape[2]

    # Ticker-universal convolution.
    model = Sequential()
    for i, d in enumerate(depth):
        if i == 0:
            model.add(Reshape((nInputTickers, nInputVars, 1), input_shape=(nInputTickers, nInputVars)))
            model.add(Conv2D(d, kernel_size=(1, nInputVars), strides=(1, nInputVars), padding='same',
                kernel_initializer=kinit, activation='elu'))
        else:
            model.add(Conv2D(d, kernel_size=(1, 1), kernel_initializer=kinit, activation='elu'))

    # Fully connected.
    for i, n in enumerate(nnode):
        if i == 0:
            model.add(Flatten())
            model.add(Dense(n, kernel_initializer=kinit, activation='elu'))
            model.add(Dropout(0.5))
        else:
            model.add(Dense(n, kernel_initializer=kinit, activation='elu'))
            model.add(Dropout(0.5))

    # Last layer.
    if nnode == []:
        model.add(Conv2D(1, kernel_size=(1, 1), kernel_initializer=kinit, activation='linear'))
        model.add(Flatten())
    else:
        model.add(Dense(targetCount, kernel_initializer=kinit, activation='linear'))

    print_model(model, log)
    return model

def getModelTwoPath(log, inputShapes, targetCount, depthTicker, depthVar, lenconn, nnode=[]):
    '''Top branch takes a subset of total input.
    '''
    kinit = TruncatedNormal(0, 0.001)
    nTopInputTickers = inputShapes[0][1]
    nTopInputVars = inputShapes[0][2]
    nBottomInputTickers = inputShapes[1][1]
    nBottomInputVars = inputShapes[1][2]
    nTopInputTickersDivLen = int(nTopInputTickers / lenconn)

    # Top branch, locally connected 1d.
    top_input = Input(shape=(nTopInputTickers, nTopInputVars))
    top_branch = Reshape((nTopInputTickers, nTopInputVars, 1))(top_input)
    for i, d in enumerate(depthTicker):
        if i == 0:
            top_branch = LocallyConnected2D(d, kernel_size=(nTopInputTickersDivLen, 1), strides=(nTopInputTickersDivLen, 1),
                kernel_initializer=kinit, activation='elu')(top_branch)
        else:
            top_branch = LocallyConnected2D(d, 1, activation='elu')(top_branch)

    finalDepthTicker = depthTicker[-1]
    top_branch = Reshape((finalDepthTicker * nTopInputVars,))(top_branch)
    print('top_branch shape ', top_branch.shape)

    # Bottom branch, ticker-universal convolution.
    bottom_input = Input(shape=(nBottomInputTickers, nBottomInputVars))
    bottom_branch = Reshape((nBottomInputTickers, nBottomInputVars, 1))(bottom_input)
    print('bottom_branch shape begin ', bottom_branch.shape)
    for i, d in enumerate(depthVar):
        if i == 0:
            bottom_branch = Conv2D(d, kernel_size=(1, nBottomInputVars), strides=(1, nBottomInputVars), padding='same', kernel_initializer=kinit, activation='elu')(bottom_branch)
            print('bottom_branch shape conv ', bottom_branch.shape)
        else:
            bottom_branch = Conv2D(d, kernel_size=1, kernel_initializer=kinit, activation='elu')(bottom_branch)

    finalDepthVar = depthVar[-1]
    bottom_branch = Reshape((nBottomInputTickers, finalDepthVar))(bottom_branch)
    print('bottom_branch shape last ', bottom_branch.shape)


    # Split bottom branch by ticker
    byticker = [Lambda(lambda x: x[:,i,:])(bottom_branch) for i in range(nBottomInputTickers)]
    print('byticker[0] shape', byticker[0].shape)

    # Concatenate each split with the top branch
    byticker = [concatenate([top_branch, bottom_piece]) for bottom_piece in byticker]
    print('byticker concatenated len', len(byticker))
    print('byticker[0] concatenated shape', byticker[0].shape)
    byticker = [Dense(1, kernel_initializer=kinit, activation='linear')(x) for x in byticker]
    print('byticker densed len', len(byticker))
    print('byticker[0] densed shape', byticker[0].shape)

    # Combine the splits
    combined = concatenate(byticker, axis=1)
    print('combined shape', combined.shape)

    model = Model(inputs=[top_input, bottom_input], outputs=combined)
    print_model(model, log)
    return model

def getModelTwoPathAdd(log, inputShapes, targetCount, depthTicker, depthVar, lenconn, nnode=[]):
    '''Top branch takes a subset of total input.
    Two outputs are added in the end.
    '''
    kinit = TruncatedNormal(0, 0.001)
    nTopInputTickers = inputShapes[0][1]
    nTopInputVars = inputShapes[0][2]
    nBottomInputTickers = inputShapes[1][1]
    nBottomInputVars = inputShapes[1][2]
    nTopInputTickersDivLen = int(nTopInputTickers / lenconn)

    # Top branch, locally connected 1d.
    top_input = Input(shape=(nTopInputTickers, nTopInputVars))
    top_branch = Reshape((nTopInputTickers, nTopInputVars, 1))(top_input)
    for i, d in enumerate(depthTicker):
        if i == 0:
            top_branch = LocallyConnected2D(d, kernel_size=(nTopInputTickersDivLen, 1), strides=(nTopInputTickersDivLen, 1),
                kernel_initializer=kinit, activation='elu')(top_branch)
        else:
            top_branch = LocallyConnected2D(d, 1, activation='elu')(top_branch)
    top_branch = Flatten()(top_branch)
    top_branch = Dense(targetCount, kernel_initializer=kinit, activation='linear')(top_branch)
    print('top_branch shape ', top_branch.shape)

    # Bottom branch, ticker-universal convolution.
    bottom_input = Input(shape=(nBottomInputTickers, nBottomInputVars))
    bottom_branch = Reshape((nBottomInputTickers, nBottomInputVars, 1))(bottom_input)
    print('bottom_branch shape begin ', bottom_branch.shape)
    for i, d in enumerate(depthVar):
        convAct = 'elu'
        if i == len(depthVar) -1 and d == 1:
            convAct = 'linear'

        if i == 0: # First conv
            bottom_branch = Conv2D(d, kernel_size=(1, nBottomInputVars), strides=(1, nBottomInputVars), padding='same',
                    kernel_initializer=kinit, activation=convAct)(bottom_branch)
            print('bottom_branch shape conv ', bottom_branch.shape)
        else:
            bottom_branch = Conv2D(d, kernel_size=1, kernel_initializer=kinit, activation=convAct)(bottom_branch)
    finalDepth = depthVar[-1]
    if finalDepth > 1:
        bottom_branch = Conv2D(1, kernel_size=1, kernel_initializer=kinit, activation='linear')(bottom_branch)
    bottom_branch = Flatten()(bottom_branch)
    print('bottom_branch shape last ', bottom_branch.shape)

    combined = add([top_branch, bottom_branch])
    print('combined shape', combined.shape)

    model = Model(inputs=[top_input, bottom_input], outputs=combined)
    print_model(model, log)
    return model

################################################################################
# Functions.
################################################################################

def write_log_chunk(model, log, trainData, validateData, ep, history, batch_size=512):
    corrTrain = 0.
    corrValid = 0.
    try:
        preds = model.predict(trainData['X'], batch_size)
        try:
            corrTrain = np.corrcoef(trainData['Y'].flatten(), preds.flatten())[0,1]
        except MemoryError:
            log.write("memory error on in-sample correlation, skipping.")
        validatePreds = model.predict(validateData['X'], batch_size=512)
        corrValid = np.corrcoef(validateData['Y'].flatten(), validatePreds.flatten())[0,1]
    except:
        pdb.set_trace()
    
    losses = history.history['loss']
    val_losses = history.history['val_loss']
    N = len(losses)
    for i in range(N):
        if i < N - 1:
            log.write('{} loss {:.4f} val_loss {:.4f}\n'.format(i + ep, losses[i], val_losses[i]))
        else:
            log.write('{} loss {:.4f} val_loss {:.4f} ctrain {:7.4f} cvalid {:7.4f}\n'.format(i + ep, losses[i], val_losses[i], corrTrain, corrValid))
    log.flush()

def write_log(model, log, trainData, validateData, i, history, batch_size=512):
    corrTrain = 0.
    corrValid = 0.
    try:
        preds = model.predict(trainData['X'], batch_size)
        try:
            corrTrain = np.corrcoef(trainData['Y'].flatten(), preds.flatten())[0,1]
        except MemoryError:
            log.write("memory error on in-sample correlation, skipping.")
        validatePreds = model.predict(validateData['X'], batch_size=512)
        corrValid = np.corrcoef(validateData['Y'].flatten(), validatePreds.flatten())[0,1]
    except:
        pdb.set_trace()
    
    log.write('{} loss {:.4f} val_loss {:.4f} ctrain {:.4f} cvalid {:.4f}\n'.format(i, history.history['loss'][0], history.history['val_loss'][0], corrTrain, corrValid))
    log.flush()

def write_log_two_path_two_input(model, log, trainData, validateData, i, history, batch_size=512):
    corrTrain = 0.
    corrValid = 0.
    try:
        preds = model.predict([trainData['X_top'], trainData['X_bottom']], batch_size)
        try:
            corrTrain = np.corrcoef(trainData['Y'].flatten(), preds.flatten())[0,1]
        except MemoryError:
            log.write("memory error on in-sample correlation, skipping.")
        validatePreds = model.predict([validateData['X_top'], validateData['X_bottom']], batch_size=512)
        corrValid = np.corrcoef(validateData['Y'].flatten(), validatePreds.flatten())[0,1]
    except:
        pdb.set_trace()
    
    log.write('{} loss {:.4f} val_loss {:.4f} ctrain {:.4f} cvalid {:.4f}\n'.format(i, history.history['loss'][0], history.history['val_loss'][0], corrTrain, corrValid))
    log.flush()

def getDataFromFile(log, filename, volcut=None, nTimeSlice=None):
    data = np.load(filename)
    dataX = np.copy(data['X'])
    dataY = np.copy(data['Y'])
    dataW = np.copy(data['W'])
    dataIdates = []
    dataTickers = []
    dataTimestamps = []
    if 'idates' in data:
        dataIdates = np.copy(data['idates'])
    if 'tickers' in data:
        dataTickers = np.copy(data['tickers'])
    if 'timestamps' in data:
        dataTimestamps = np.copy(data['timestamps'])
    log.write("{} X shape:{}\n".format(filename, dataX.shape))
    log.write("{} Y shape:{}\n".format(filename, dataY.shape))

    if volcut is not None:
        nInputTickers = dataX.shape[1]
        nSelectTickers = int(nInputTickers * volcut)
        dataX = dataX[:,-nSelectTickers:,:]
        log.write("  volcut={}, X shape changed:{}\n".format(volcut, dataX.shape))

    if nTimeSlice is not None:
        nInputTickers = dataX.shape[1]
        nInputVars = dataX.shape[2]
        nOutputTickers = dataY.shape[1]
        dataX = dataX.reshape(-1, nTimeSlice, nInputTickers, nInputVars)
        dataY = dataY.reshape(-1, nTimeSlice, nOutputTickers)
        log.write("  X shape changed:{}\n".format(dataX.shape))
        log.write("  Y shape changed:{}\n".format(dataY.shape))

    #dataRet = {'X':dataX, 'Y':dataY, 'W':dataW}
    dataRet = {'X':dataX, 'Y':dataY, 'W':dataW, 'idates':dataIdates, 'tickers':dataTickers, 'timestamps':dataTimestamps}
    return dataRet

def getTrainingData(log, volcut=None):
    trainData = getDataFromFile(log, 'train.npz', volcut)
    validateData = getDataFromFile(log, 'valid.npz', volcut)
    return trainData, validateData

def getTrainingDataRecurrent(log, nTimeSlice, volcut=None):
    trainData = getDataFromFile(log, 'train.npz', volcut=volcut, nTimeSlice=nTimeSlice)
    validateData = getDataFromFile(log, 'valid.npz', volcut=volcut, nTimeSlice=nTimeSlice)
    return trainData, validateData

def getDataTwoPath(log, filename, topcut=0.25):
    data = getDataFromFile(log, filename)

    # Select input to the top branch
    nInputTickers = data['X'].shape[1]
    nSelectTickers = int(nInputTickers * topcut)
    selVar = [1, 4, 5, 7, 27]
    dataXtop = data['X'][:, -nSelectTickers:, selVar]

    dataRet = {'X_top':dataXtop, 'X_bottom':data['X'], 'Y':data['Y']}
    return dataRet

def getTrainingDataTwoPath(log):
    trainData = getDataTwoPath(log, 'train.npz')
    validateData = getDataTwoPath(log, 'valid.npz')
    return trainData, validateData

def getTestData(log, filename='test.npz'):
    data = getDataFromFile(log, filename)
    dataRet = {'X':data['X'], 'Y':data['Y'], 'W':data['W'], 'idates':data['idates'], 'tickers':data['tickers'], 'timestamps':data['timestamps']}
    return dataRet

def getTestDataRecurrent(log, nTimeSlice, filename='test.npz'):
    data = getDataFromFile(log, filename, nTimeSlice=nTimeSlice)
    dataRet = {'X':data['X'], 'Y':data['Y'], 'W':data['W'], 'idates':data['idates'], 'tickers':data['tickers'], 'timestamps':data['timestamps']}
    return dataRet

def getTestDataTwoPath(log, filename='test.npz', topcut=0.25):
    data = getDataFromFile(log, filename)

    # Select input to the top branch
    nInputTickers = data['X'].shape[1]
    nSelectTickers = int(nInputTickers * topcut)
    selVar = [1, 4, 5, 7, 27]
    dataXtop = data['X'][:, -nSelectTickers:, selVar]

    dataRet = {'X_top':dataXtop, 'X_bottom':data['X'], 'Y':data['Y'], 'W':data['W'], 'idates':data['idates'], 'tickers':data['tickers'], 'timestamps':data['timestamps']}
    return dataRet

def train_model_chunk(model, trainData, validateData, epochs, epoch_chunk, batch_size, log):
    model.compile(optimizer='adam', loss='mse')
    ep = 0
    while ep < epochs:
        history = model.fit(trainData['X'], trainData['Y'],
            validation_data=(validateData['X'], validateData['Y']),
            epochs=epoch_chunk, batch_size=batch_size)
        ep += epoch_chunk
        write_log_chunk(model, log, trainData, validateData, ep, history)

def train_model(model, trainData, validateData, epochs, batch_size, log, desc):
    model.compile(optimizer='adam', loss='mse')
    for i in range(epochs):
        history = model.fit(trainData['X'], trainData['Y'],
            validation_data=(validateData['X'], validateData['Y']),
            epochs=1, batch_size=batch_size)
        write_log(model, log, trainData, validateData, i, history)
        if i > 0 and (i % 2 == 0 or i == epochs - 1):
            model.save('model_{}_ep_{}.h5'.format(desc, i), overwrite=True)

def train_model_wgt(model, trainData, validateData, epochs, batch_size, log):
    model.compile(optimizer='adam', loss='mse', sample_weight_mode='temporal')
    for i in range(epochs):
        history = model.fit(trainData['X'], trainData['Y'],
            sample_weight=trainData['W'],
            validation_data=(validateData['X'], validateData['Y']),
            epochs=1, batch_size=batch_size)
        write_log(model, log, trainData, validateData, i, history)

def train_model_two_path(model, trainData, validateData, epochs, batch_size, log, desc):
    model.compile(optimizer='adam', loss='mse')
    for i in range(epochs):
        history = model.fit([trainData['X_top'], trainData['X_bottom']], trainData['Y'],
            validation_data=([validateData['X_top'], validateData['X_bottom']], validateData['Y']),
            epochs=1, batch_size=batch_size)
        write_log_two_path_two_input(model, log, trainData, validateData, i, history)
        if i > 0 and (i % 5 == 0 or i == epochs - 1):
            model.save('model_{}_ep_{}.h5'.format(desc, i), overwrite=True)

def getLogfile(dense=[], depth=[], depthTicker=[], depthVar=[], lenconn=1, nrecurr=[], nnode=[], pre=None, tid=None, volcut=None):
    #logfile = 'log' + pre
    desc = ''
    if pre is not None:
        desc = pre + '_'
    if volcut is not None:
        desc += '_vol{:.2f}'.format(float(volcut))
    for i, d in enumerate(dense):
        if i == 0:
            desc += '_dense_{}'.format(d)
        else:
            desc += '_{}'.format(d)
    for i, d in enumerate(depthTicker):
        if i == 0:
            desc += '_dt_{}'.format(d)
        else:
            desc += '_{}'.format(d)
    for i, d in enumerate(depthVar):
        if i == 0:
            desc += '_dv_{}'.format(d)
        else:
            desc += '_{}'.format(d)
    for i, d in enumerate(depth):
        if i == 0:
            desc += '_d_{}'.format(d)
        else:
            desc += '_{}'.format(d)
    if not len(depth) == 0 and lenconn > 1:
        desc += '_len_{}'.format(lenconn)
    for i, n in enumerate(nrecurr):
        if i == 0:
            desc += '_r_{}'.format(n)
        else:
            desc += '_{}'.format(n)
    for i, n in enumerate(nnode):
        if i == 0:
            desc += '_n_{}'.format(n)
        else:
            desc += '_{}'.format(n)
    if tid is not None:
        desc += '_id{}'.format(tid)
    logfile = 'log' + desc + '.log'
    return open(logfile, 'w'), desc

################################################################################
# Train with fully connected layers.
################################################################################

def trainFC(epochs=5, batch_size=128, nnode=10, tid=None):
    nnode = parse_num_list(nnode)
    log, _ = getLogfile(nnode=nnode, tid=tid)

    trainData, validateData = getTrainingData(log)
    inputShape = trainData['X'].shape
    targetCount = trainData['Y'].shape[1]
    model = getModelFC(log, inputShape, targetCount, nnode)

    train_model(model, trainData, validateData, epochs, batch_size, log)

################################################################################
# Train with locally connected layers followed by fully connected layers.
################################################################################

def trainConn(epochs=5, batch_size=128, depth=10, lenconn=1, nnode=0, tid=None, volcut=None):
    nnode = parse_num_list(nnode)
    depth = parse_num_list(depth)
    log, _ = getLogfile(depth=depth, lenconn=lenconn, nnode=nnode, volcut=volcut)

    trainData, validateData = getTrainingData(log, volcut)
    inputShape = trainData['X'].shape
    targetCount = trainData['Y'].shape[1]
    model = getModelConn(log, inputShape, targetCount, depth, lenconn, nnode)

    train_model(model, trainData, validateData, epochs, batch_size, log)

################################################################################
# Train with locally connected layers followed by GRU.
################################################################################

def trainConnGRU(epochs=5, depth=10, lenconn=1, nrecurr=10, nnode=0, nTimeSlice=779, tid=None, volcut=None):
    depth = parse_num_list(depth)
    nrecurr = parse_num_list(nrecurr)
    nnode = parse_num_list(nnode)
    log, desc = getLogfile(depth=depth, lenconn=lenconn, nrecurr=nrecurr, nnode=nnode, volcut=volcut, tid=tid)

    trainData, validateData = getTrainingDataRecurrent(log, nTimeSlice, volcut)

    inputShape = trainData['X'].shape
    targetCount = trainData['Y'].shape[2]
    model = getModelConnGRU(log, inputShape, targetCount, depth, lenconn, nrecurr, nnode)

    batch_size = 1
    train_model(model, trainData, validateData, epochs, batch_size, log, desc)

def trainDenseGRU(epochs=5, dense=10, nrecurr=10, nnode=0, nTimeSlice=779, tid=None, volcut=None):
    dense = parse_num_list(dense)
    nrecurr = parse_num_list(nrecurr)
    nnode = parse_num_list(nnode)
    log, _ = getLogfile(dense=dense, nrecurr=nrecurr, nnode=nnode, volcut=volcut)

    trainData, validateData = getTrainingDataRecurrent(log, nTimeSlice, volcut)

    inputShape = trainData['X'].shape
    targetCount = trainData['Y'].shape[2]
    model = getModelDenseGRU(log, inputShape, targetCount, dense, nrecurr, nnode)

    batch_size = 1
    train_model(model, trainData, validateData, epochs, batch_size, log)

################################################################################
# Train with LSTM
################################################################################

def trainLstm(epochs=5, nrecurr=10, nnode=0, nTimeSlice=779, tid=None):
    # Number of input variable must be 1.

    nrecurr = parse_num_list(nrecurr)
    nnode = parse_num_list(nnode)
    log, _ = getLogfile(nrecurr=nrecurr, nnode=nnode)

    trainData, validateData = getTrainingData(log)

    nInputTickers = trainData['X'].shape[1]
    nInputVars = trainData['X'].shape[2]
    nOutputTickers = trainData['Y'].shape[1]

    trainData['X'] = trainData['X'].reshape(-1, nTimeSlice, nInputTickers)
    validateData['X'] = validateData['X'].reshape(-1, nTimeSlice, nInputTickers)
    trainData['Y'] = trainData['Y'].reshape(-1, nTimeSlice, nOutputTickers)
    validateData['Y'] = validateData['Y'].reshape(-1, nTimeSlice, nOutputTickers)

    inputShape = trainData['X'].shape
    targetCount = trainData['Y'].shape[2]
    model = getModelLstm(log, inputShape, targetCount, nrecurr, nnode)

    batch_size = 1
    train_model(model, trainData, validateData, epochs, batch_size, log)

################################################################################
# Ticker-universal model.
################################################################################

def trainUniv(depth=3, epochs=5, batch_size=128, nnode=0, tid=None):
    depth= parse_num_list(depth)
    nnode= parse_num_list(nnode)
    log, _ = getLogfile(depth=depth, nnode=nnode, pre='_univ', volcut=volcut)

    trainData, validateData = getTrainingData(log)
    inputShape = trainData['X'].shape
    targetCount = trainData['Y'].shape[1]
    model = getModelUniv(log, inputShape, targetCount, depth, nnode)

    train_model(model, trainData, validateData, epochs, batch_size, log)

################################################################################
# Two path training.
################################################################################

def trainTwoPath(depthTicker=3, depthVar=3, lenconn=1, epochs=5, batch_size=128, nnode=0, tid=None):
    '''Input for top and bottom branches are returned from getTrainingDataTwoPath().
    '''
    nnode = parse_num_list(nnode)
    depthTicker = parse_num_list(depthTicker)
    depthVar = parse_num_list(depthVar)
    log, desc = getLogfile(depthTicker=depthTicker, depthVar=depthVar, nnode=nnode, tid=tid)

    trainData, validateData = getTrainingDataTwoPath(log)
    topInputShape = trainData['X_top'].shape
    bottomInputShape = trainData['X_bottom'].shape
    targetCount = trainData['Y'].shape[1]
    model = getModelTwoPathAdd(log, (topInputShape, bottomInputShape), targetCount, depthTicker, depthVar, lenconn, nnode)

    train_model_two_path(model, trainData, validateData, epochs, batch_size, log, desc)

################################################################################
# Load and predict
################################################################################

def getModelDate(baseDir, idate):
    fitDir = baseDir + '/fit/tar60;0_xbmpred60;0'
    coefDir = fitDir + '/coef';
    pat = '{}/coef*.txt'.format(coefDir)
    coefFiles = glob.glob(pat)
    maxPastIdate = 0
    for cfile in coefFiles:
        dloc = cfile.find('201')
        cdate = int(cfile[dloc:(dloc + 8)])
        if cdate <= idate and cdate > maxPastIdate:
            maxPastIdate = cdate
    return maxPastIdate

def getHeader(baseDir, idate):
    sigDir = baseDir + '/binSig/om'
    sigHeaderFile = sigDir + '/sig{}.txt'.format(idate)
    dfHeader = read_csv(sigHeaderFile, delimiter='\t', dtype=str, keep_default_na=False, na_values=None)
    dfHeader.drop('uid', axis=1)
    return dfHeader

def getSamplePreds(dfHeader, dayPreds, tickerMap, timeMap):
    samplePreds = []
    for indx, row in dfHeader.iterrows():
        ticker = row['ticker']
        time = row['time']
        pred = dayPreds[timeMap[time], tickerMap[ticker]]
        samplePreds.append(pred)
    return samplePreds

def getDfPred(baseDir, idate):
    modelDate = getModelDate(baseDir, idate)
    fitDir = baseDir + '/fit/tar60;0_xbmpred60;0'
    predDir = fitDir + '/stat_{}/preds'.format(modelDate)
    predFilename = predDir + '/pred{}.txt'.format(idate)
    dfPred = read_csv(predFilename, delimiter='\t')
    return dfPred

def updatePred(dfPred, samplePreds):
    dfPred['bmpred'] = dfPred['pred']
    dfPred['pred'] = dfPred['pred'] + samplePreds
    dfPred.iloc[:,-1] = dfPred['pred']
    return dfPred

def writePredTwoPath(idate, dayPreds, tickerMap, timeMap):
    baseDir = '/home/jelee/hffit/CA'

    dfHeader = getHeader(baseDir, idate)
    samplePreds = getSamplePreds(dfHeader, dayPreds, tickerMap, timeMap)
    dfPred = getDfPred(baseDir, idate)
    dfPred = updatePred(dfPred, samplePreds)

    concFitDir = baseDir + '/fit/conctar60;0_xbmpred60;0'
    concPredDir = concFitDir + '/stat_20010101/preds'
    if not os.path.isdir(concPredDir):
        os.mkdir(concPredDir)
    concPredFilename = concPredDir + '/pred{}.txt'.format(idate)
    dfPred.to_csv(concPredFilename, sep='\t', index=False, float_format='%.4f')

def printCorr(log, y, preds, weights):
    corr = np.corrcoef(y, preds)[0,1]
    log.write('all y shape {} test corr {:.4f}\n'.format(y.shape, corr))

    #y_valid = y[weights==1]
    #corr = np.corrcoef(y_valid, preds[weights==1])[0,1]
    #log.write('valid y shape {} test corr {:.4f}\n'.format(y_valid.shape, corr))

def getIndexMaps(data):
    nTimeSlice = len(data['timestamps'])
    timeMap = {}
    for i, time in enumerate(data['timestamps']):
        timeMap[time] = i

    tickerMap = {}
    for i, ticker in enumerate(data['tickers']):
        tickerMap[ticker] = i

    idateMap = {}
    for i, idate in enumerate(data['idates']):
        idateMap[idate] = i

    return timeMap, tickerMap, idateMap

def testTwoPath(modelFile, batch_size=512):
    log = open('test_{}.log'.format(modelFile), 'w')
    model = load_model(modelFile)
    data = getTestDataTwoPath(log)
    preds = model.predict([data['X_top'], data['X_bottom']], batch_size)
    weights = data['W']
    printCorr(log, data['Y'].flatten(), preds.flatten(), weights)

    corr = np.corrcoef(data['Y'].flatten(), preds.flatten())[0,1]
    log.write('test corr {:.4f}'.format(corr))

    timeMap, tickerMap, idateMap = getIndexMaps(data)
    nTimeSlice = len(data['timestamps'])
    for idate in data['idates']:
        ifrom = idateMap[idate] * nTimeSlice
        ito = ifrom + nTimeSlice
        writePredTwoPath(idate, preds[ifrom:ito], tickerMap, timeMap)

def testConnGRU(modelFile, nTimeSlice=779, batch_size=512):
    log = open('test_{}.log'.format(modelFile), 'w')
    model = load_model(modelFile)
    data = getTestDataRecurrent(log, nTimeSlice)
    preds = model.predict(data['X'], batch_size)
    weights = data['W']
    printCorr(log, data['Y'].flatten(), preds.flatten(), weights)

    corr = np.corrcoef(data['Y'].flatten(), preds.flatten())[0,1]
    log.write('test corr {:.4f}'.format(corr))

    timeMap, tickerMap, idateMap = getIndexMaps(data)
    nTimeSlice = len(data['timestamps'])
    for i, idate in enumerate(data['idates']):
        ifrom = idateMap[idate] * nTimeSlice
        ito = ifrom + nTimeSlice
        writePredTwoPath(idate, preds[i], tickerMap, timeMap)

def corrTwoPath(modelFile, batch_size=512):
    log = open('test_{}.log'.format(modelFile), 'w')
    model = load_model(modelFile)

    data = getTestDataTwoPath(log)
    data_target = getTestDataTwoPath(log, 'test_target.npz')
    preds = model.predict([data['X_top'], data['X_bottom']], batch_size)

    restar = data['Y']
    target = data_target['Y']
    bmpred = target - restar
    totpred = bmpred + preds

    weights = data['W']
    log.write('preds-restar\n')
    printCorr(log, restar.flatten(), preds.flatten(), weights.flatten())
    log.write('preds-target\n')
    printCorr(log, target.flatten(), preds.flatten(), weights.flatten())
    log.write('preds-bmpred\n')
    printCorr(log, bmpred.flatten(), preds.flatten(), weights.flatten())
    log.write('bmpred-target\n')
    printCorr(log, target.flatten(), bmpred.flatten(), weights.flatten())
    log.write('totpred-target\n')
    printCorr(log, target.flatten(), totpred.flatten(), weights.flatten())

    timeMap, tickerMap, idateMap = getIndexMaps(data)
    nTimeSlice = len(data['timestamps'])
    for idate in data['idates']:
        ifrom = idateMap[idate] * nTimeSlice
        ito = ifrom + nTimeSlice

        vValid = weights[ifrom:ito] == 1
        vRestar = restar[ifrom:ito][vValid].flatten()
        vTarget = target[ifrom:ito][vValid].flatten()
        vPred = preds[ifrom:ito][vValid].flatten()
        vBmpred = bmpred[ifrom:ito][vValid].flatten()
        vTotPred = totpred[ifrom:ito][vValid].flatten()

        rescorr = np.corrcoef(vRestar, vPred)[0,1]
        bmcorr = np.corrcoef(vTarget, vBmpred)[0,1]
        totcorr = np.corrcoef(vTarget, vTotPred)[0,1]

        log.write('{} test rescorr {:.4f} bmcorr {:.4f} totcorr {:.4f} diff {:.4f}\n'.format(idate, rescorr, bmcorr, totcorr, totcorr-bmcorr))

        eyyhat = np.average(vTarget * vBmpred)
        epdy = np.average(vRestar * vPred)
        etot = np.average(vTarget * vTotPred)
        log.write('    tar-prod {:.4f} restar-pred {:.4f} tar-tot {:.4f} diff {:.4f}\n'.format(eyyhat, epdy, etot, etot-eyyhat))

        cyyhat = np.average(vTarget * vBmpred) / np.std(vTarget) / np.std(vBmpred)
        cpdy = np.average(vRestar * vPred) / np.std(vRestar) / np.std(vPred)
        ctot = np.average(vTarget * vTotPred) / np.std(vTarget) / np.std(vTotPred)
        log.write('    tar-prod {:.4f} restar-pred {:.4f} tar-tot {:.4f} diff {:.4f}\n'.format(cyyhat, cpdy, ctot, ctot-cyyhat))

        c1yyhat = np.average(vTarget * vBmpred) / np.std(vTarget)
        c1pdy = np.average(vRestar * vPred) / np.std(vRestar)
        c1tot = np.average(vTarget * vTotPred) / np.std(vTarget)
        log.write('    tar-prod {:.4f} restar-pred {:.4f} tar-tot {:.4f} diff {:.4f}\n'.format(c1yyhat, c1pdy, c1tot, c1tot-c1yyhat))

        c2yyhat = np.average(vTarget * vBmpred) / np.std(vBmpred)
        c2pdy = np.average(vRestar * vPred) / np.std(vPred)
        c2tot = np.average(vTarget * vTotPred) / np.std(vTotPred)
        log.write('    tar-prod {:.3f} restar-pred {:.4f} tar-tot {:.4f} diff {:.4f}\n'.format(c2yyhat, c2pdy, c2tot, c2tot-c2yyhat))
