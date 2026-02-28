set(LIBNAME gtlib)
set(SOURCES
	EstTime.cpp
	util.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
