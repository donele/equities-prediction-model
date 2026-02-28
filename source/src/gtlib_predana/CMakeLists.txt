set(LIBNAME gtlib_predana)
set(SOURCES
	GsprFtns.cpp
	GsprObjectMultTar.cpp
	GsprObjectSimple.cpp
	PAnaSimple.cpp
	PObjMultTar.cpp
	PredAnaByNTree.cpp
	PredAnaByNTreeMultTar.cpp
	PredAnaByNTreeMultTarPrintTrades.cpp
	PredAnaByQuantile.cpp
	PredAnaByQuantileTrade.cpp
	PredAnaByQuantileTradeLastBin.cpp
	PredAnaByQuantileUnhedgedTarget.cpp
	PredWholeDay.cpp
	PredWholeDayMultTar.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
