set(LIBNAME gtlib_linmod)
set(SOURCES
	ARMulti.cpp
	BiLinearModel.cpp
	BiLinearModelN.cpp
	BiLinearModelWeights.cpp
	CovMatSet.cpp
	CovMatSetCollUniv.cpp
	CovMatSetCollErr.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
