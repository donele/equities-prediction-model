set(LIBNAME MFitMod)
set(SOURCES
	MFastSim.cpp
	MFastSimDataDump.cpp
	MFitAna.cpp
	MFitNtrees.cpp
	MFitRead.cpp
	MPrintTrades.cpp
	MFitter.cpp
	MVolSurpAna.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
