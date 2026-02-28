set(LIBNAME gtlib_fitana)
set(SOURCES
	AnaAllDaysStat.cpp
	AnaDayStatMaker.cpp
	AnaObject.cpp
	AnaObjectBySprd.cpp
	EvStatMaker.cpp
	fitanaObjs.cpp
	MevStatMaker.cpp
	PredAna.cpp
	PredAnaBySprd.cpp
	PredLine.cpp
	TargetPred.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
