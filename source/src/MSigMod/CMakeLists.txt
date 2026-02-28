set(LIBNAME MSigMod)
set(SOURCES
	MMakeSampleMicro.cpp
	MMakeSampleTP.cpp
	MTradeQuoteLatency.cpp
	SignalCalculatorMicro.cpp
	SignalCalculatorMicroTest.cpp
	SignalCalculatorTP.cpp
	TradeQuoteLatencyCalculator.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
