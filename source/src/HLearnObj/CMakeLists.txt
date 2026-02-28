set(SOURCES BPNNbatch.cpp BPTT2.cpp BPTT.cpp BPTTpred2in.cpp BPTTpred2inD.cpp BPTTpred.cpp InputInfo.cpp NNBase.cpp
    Pred2Sig_2in.cpp Pred2Sig.cpp RRL.cpp Sample.cpp Signal.cpp)

add_library(HLearnObj ${SOURCES})
link_directories(${PROJECT_SOURCE_DIR})
