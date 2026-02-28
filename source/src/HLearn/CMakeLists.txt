set(SOURCES HBinSigRead.cpp HNNReadRoot.cpp HNNSampleCommodity.cpp HNNSample.cpp HNNSampleNews.cpp HNNSampleNewsFile.cpp
    HNNSampleRead.cpp HNNTester.cpp HNNTrader.cpp HNNTrainer.cpp HSignalNewsBin.cpp HSignalNewsFile.cpp HSignalRead.cpp
    HSignalSubset.cpp mcpred.cpp NewsSignal.cpp)

add_library(HLearn ${SOURCES})
link_directories(${PROJECT_SOURCE_DIR})
