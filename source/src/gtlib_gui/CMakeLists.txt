set(LIBNAME gtlib_gui)

include_directories(${GT_INCLUDE_DIR}/gtlib_gui) # Needed for dict generation.

set(SOURCES MyMainFrame.cpp BidAskFrame.cpp)
ROOT_GENERATE_DICTIONARY(${LIBNAME}_dict
	MyMainFrame.h
	BidAskFrame.h
	LINKDEF ${LIBNAME}_linkdef.h)
set(DICTS ${LIBNAME}_dict.cxx)

add_library(${LIBNAME} ${SOURCES} ${DICTS})

install(TARGETS ${LIBNAME} DESTINATION lib64)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}_dict_rdict.pcm DESTINATION lib64)
