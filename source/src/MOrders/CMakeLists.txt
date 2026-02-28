set(LIBNAME MOrders)
set(SOURCES
	MMercatorTrades.cpp
	MOrderAna.cpp
	MOrderSummRead.cpp
	MSampleCaptureRat.cpp
	MSelfDealingAna.cpp
	MTradeAna.cpp
	MTradeEventAna.cpp
	TradeQuantileInfo.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
