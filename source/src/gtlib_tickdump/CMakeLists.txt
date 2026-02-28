set(LIBNAME gtlib_tickdump)
set(SOURCES
	DumpCountConsumer.cpp
	DumpFtns.cpp
	DumpNbboConsumer.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
