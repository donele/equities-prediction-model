set(LIBNAME jl_learn)
set(SOURCES Bpnn0.cpp BPTT.cpp Butil.cpp GenBPNN.cpp NN.cpp NN_bpnn.cpp NN_bptt.cpp NN_bptt_tanh.cpp NNobj.cpp
    Random.cpp RLfeeder.cpp RLmodel.cpp RLobj.cpp RmLin.cpp) 

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
