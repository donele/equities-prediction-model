set(LIBNAME MCore)
set(SOURCES MCore.cpp ModuleFactory.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
