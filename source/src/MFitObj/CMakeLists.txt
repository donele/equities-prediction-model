set(LIBNAME MFitObj)
set(SOURCES
	CorrInput.cpp
	DTBase.cpp
	DTBoost.cpp
	DT.cpp
	DTMT.cpp
	DTMTpre.cpp
	DTNode.cpp
	FitISLE.cpp
	Perf.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
