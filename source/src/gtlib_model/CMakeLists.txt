set(LIBNAME gtlib_model)
set(SOURCES
	FutSigPar.cpp
	hff.cpp
	mdl.cpp
	mFtns.cpp
	pathFtns.cpp
	)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
