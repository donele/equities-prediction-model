import numpy as np
import pdb
from keras.models import Model
from keras.layers.core import Dense, Dropout, Flatten
from keras.layers import Input, Reshape, Conv2D, add
from keras.layers.local import LocallyConnected2D
from keras.initializers import TruncatedNormal
from concfit import remove_zeros, parse_num_list, print_model

class TwoPath:
    def makeModel(self):
        '''Top branch takes a subset of total input.
        Two outputs are added in the end.
        '''
        topInputShape = self.trainData['X_top'].shape
        bottomInputShape = self.trainData['X_bottom'].shape
        targetCount = self.trainData['Y'].shape[1]

        kinit = TruncatedNormal(0, 0.001)
        nTopInputTickers = topInputShape[1]
        nTopInputVars = topInputShape[2]
        nBottomInputTickers = bottomInputShape[1]
        nBottomInputVars = bottomInputShape[2]
        nTopInputTickersDivLen = int(nTopInputTickers / self.lenconn)
    
        # Top branch, locally connected 1d.
        top_input = Input(shape=(nTopInputTickers, nTopInputVars))
        top_branch = Reshape((nTopInputTickers, nTopInputVars, 1))(top_input)
        for i, d in enumerate(self.depthTicker):
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
        for i, d in enumerate(self.depthVar):
            convAct = 'elu'
            if i == len(self.depthVar) -1 and d == 1:
                convAct = 'linear'
    
            if i == 0: # First conv
                bottom_branch = Conv2D(d, kernel_size=(1, nBottomInputVars), strides=(1, nBottomInputVars), padding='same',
                        kernel_initializer=kinit, activation=convAct)(bottom_branch)
                print('bottom_branch shape conv ', bottom_branch.shape)
            else:
                bottom_branch = Conv2D(d, kernel_size=1, kernel_initializer=kinit, activation=convAct)(bottom_branch)
        finalDepth = self.depthVar[-1]
        if finalDepth > 1:
            bottom_branch = Conv2D(1, kernel_size=1, kernel_initializer=kinit, activation='linear')(bottom_branch)
        bottom_branch = Flatten()(bottom_branch)
        print('bottom_branch shape last ', bottom_branch.shape)
    
        combined = add([top_branch, bottom_branch])
        print('combined shape', combined.shape)
    
        self.model = Model(inputs=[top_input, bottom_input], outputs=combined)
        print_model(self.model, self.log)
    
    def writeLog(self, ep, history, batch_size=512):
        corrTrain = 0.
        corrValid = 0.
        try:
            preds = self.model.predict([self.trainData['X_top'], self.trainData['X_bottom']], batch_size)
            try:
                corrTrain = np.corrcoef(self.trainData['Y'].flatten(), preds.flatten())[0,1]
            except MemoryError:
                log.write("memory error on in-sample correlation, skipping.")
            validatePreds = self.model.predict([self.validateData['X_top'], self.validateData['X_bottom']], batch_size=512)
            corrValid = np.corrcoef(self.validateData['Y'].flatten(), validatePreds.flatten())[0,1]
        except:
            pdb.set_trace()
        
        self.log.write('{} loss {:.4f} val_loss {:.4f} ctrain {:.4f} cvalid {:.4f}\n'.format(ep, history.history['loss'][0], history.history['val_loss'][0], corrTrain, corrValid))
        self.log.flush()
    
    def readData(self, volcut=0.25):
        trainData = np.load('train.npz')
        validateData = np.load('valid.npz')
    
        # Make copy
        trainDataX = np.copy(trainData['X'])
        trainDataY = np.copy(trainData['Y'])
        validateDataX = np.copy(validateData['X'])
        validateDataY = np.copy(validateData['Y'])
    
        # Select input to the top branch
        nInputTickers = trainDataX.shape[1]
        nSelectTickers = int(nInputTickers * volcut)
        selVar = [1, 4, 5, 7, 27]
        trainDataXtop = trainDataX[:, -nSelectTickers:, selVar]
        validateDataXtop = validateDataX[:, -nSelectTickers:, selVar]
    
        self.log.write("top training X shape:{}\n".format(trainDataXtop.shape))
        self.log.write("training X shape:{}\n".format(trainDataX.shape))
        self.log.write("training Y shape:{}\n".format(trainDataY.shape))
        self.log.write("top validation X shape:{}\n".format(validateDataXtop.shape))
        self.log.write("validation X shape:{}\n".format(validateDataX.shape))
        self.log.write("validation Y shape:{}\n".format(validateDataY.shape))
    
        trainDataCopy = {'X_top':trainDataXtop, 'X_bottom':trainDataX, 'Y':trainDataY}
        validateDataCopy = {'X_top':validateDataXtop, 'X_bottom':validateDataX, 'Y':validateDataY}

        self.trainData = trainDataCopy
        self.validateData = validateDataCopy

    def setLogfile(self, pre='', volcut=None):
        logfile = 'log' + pre
        if volcut is not None:
            logfile += '_vol{:.2f}'.format(float(volcut))
        for i, d in enumerate(self.depthTicker):
            if i == 0:
                logfile += '_dt_{}'.format(d)
            else:
                logfile += '_{}'.format(d)
        for i, d in enumerate(self.depthVar):
            if i == 0:
                logfile += '_dv_{}'.format(d)
            else:
                logfile += '_{}'.format(d)
        if self.lenconn > 1:
            logfile += '_len_{}'.format(self.lenconn)
        for i, n in enumerate(self.nnode):
            if i == 0:
                logfile += '_n_{}'.format(n)
            else:
                logfile += '_{}'.format(n)
        if self.tid is not None:
            logfile += '_id{}'.format(self.tid)
        logfile += '.log'
        self.log = open(logfile, 'w')
    
    def __init__(self, depthTicker=3, depthVar=3, nnode=0, tid=None):
        self.model = None
        self.lenconn = 1
        self.tid = tid
        self.nnode = parse_num_list(nnode)
        self.depthTicker = parse_num_list(depthTicker)
        self.depthVar = parse_num_list(depthVar)
        self.setLogfile()
        self.readData()
        self.makeModel()

    def writeModel(self, ep, epochs):
        if ep > 0 and (ep % 5 == 0 or ep == epochs - 1):
            filename = 'model'
            if self.tid is not None:
                filename += '_id_{}'.format(self.tid)
            filename += '_ep_{}'.format(ep)
            self.model.save_weights(filename + '.h5', overwrite=True)
    
    def train(self, epochs=3, batch_size=512):
        self.model.compile(optimizer='adam', loss='mse')
        for ep in range(epochs):
            history = self.model.fit([self.trainData['X_top'], self.trainData['X_bottom']], self.trainData['Y'],
                validation_data=([self.validateData['X_top'], self.validateData['X_bottom']], self.validateData['Y']),
                epochs=1, batch_size=batch_size)
            self.writeLog(ep, history)
            self.writeModel(ep, epochs)
    
