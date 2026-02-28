set(LIBNAME MFramework)
set(SOURCES
	MEnv.cpp
	MEvent.cpp
	MInitFut.cpp
	MInit.cpp
	MInitModel.cpp
	MModuleBase.cpp
	MSelectTickers.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
