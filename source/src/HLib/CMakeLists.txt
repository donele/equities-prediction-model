set(LIBNAME HLib)
set(SOURCES
	AR.cpp
	HEnv.cpp
	HEvent.cpp
	HInit.cpp
	HInitModel.cpp
	HModule.cpp
	pooledar.cpp
	QProfile.cpp
	QTH1F.cpp
	TickReader.cpp
	EurexOrder.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
