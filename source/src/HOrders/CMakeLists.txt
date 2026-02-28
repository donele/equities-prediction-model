set(LIBNAME HOrders)
set(SOURCES
	HaltCondPnl.cpp
	HaltCondRet.cpp
	HaltCondRetWgt.cpp
	HaltSched.cpp
	HaltSim.cpp
	HaltSimCumPred.cpp
	HaltSimCumPredRat.cpp
	HaltSimMult.cpp
	HaltSimPnl.cpp
	HaltSimRet.cpp
	HaltSimRetWgt.cpp
	HCloseToCloseAna.cpp
	HEurexRead.cpp
	HFillTime.cpp
	HFillTimeHist.cpp
	HMarketMakingSpeed.cpp
	HMercatorTrades.cpp
	HOrderAna.cpp
	HOrderSumm.cpp
	HOrderSummRead.cpp
	HOrderSummRead_test.cpp
	HReturnAna.cpp
	HTradeAna.cpp
	HTradeAnaGainSig.cpp
	HTradeAnaGainSigMult.cpp
	HTradeAnaProfile.cpp
	HTradeAnaVolume.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
