set(LIBNAME MFutModel)
set(SOURCES MTickReadFut.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
