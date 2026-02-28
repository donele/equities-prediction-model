set(LIBNAME gtlib_fastsim)
set(SOURCES
	FastSim.cpp
	FastSimDataDump.cpp
	FastSimNet.cpp
	fastsimObjs.cpp
	FastSimSimple.cpp
	)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
