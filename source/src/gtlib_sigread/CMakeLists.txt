set(LIBNAME gtlib_sigread)
set(SOURCES
	SignalBuffer.cpp
	SignalBufferBin.cpp
	SignalBufferText.cpp
	SignalBufferDecorator.cpp
	SignalBufferDecoratorHeader.cpp
	SignalBufferDecoratorPred.cpp
	SignalBufferDecoratorPredFromSig.cpp
	SignalHeader.cpp
	SignalBufferPred.cpp
	SignalBufferPredMulti.cpp
	SignalWholeDay.cpp
	SignalWholeDayFriends.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
