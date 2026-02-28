set(SOURCES flt.cpp HFitIndexAR.cpp HMakePredIndex.cpp HMakeSampleTarget.cpp HReadSample.cpp
    writeSig.cpp HBiLinModTest.cpp
    HMakeSample.cpp HReadChara.cpp sig.cpp writeSig.cpp)

add_library(HFitting ${SOURCES})
link_directories(${PROJECT_SOURCE_DIR})
