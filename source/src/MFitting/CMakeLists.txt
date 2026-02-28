set(LIBNAME MFitting)
set(SOURCES
	MCombineTree.cpp
	MFitISLE.cpp
	MFitterTest.cpp
	MFitNonLinMod.cpp
	MModuleExample.cpp
	MPsedoTradeAdapt.cpp
	MPseudoTradeAdaptVol.cpp
	MPseudoTradeAuction.cpp
	MPseudoTrade.cpp
	MPseudoTrade_10m.cpp
	MPseudoTradeFullSimComp.cpp
	MPseudoTradeIntra.cpp
	MPseudoTradeMM.cpp
	MPseudoTradeMultPred.cpp
	MPseudoTradePred.cpp
	MPseudoTradePrep.cpp
	MPseudoTradeProfile.cpp
	MPseudoTradeRestore.cpp
	MReadSample.cpp
	MReadSignal.cpp
	MReadSignalON.cpp
	MReadTickerState.cpp
	MWriteTree.cpp
	TickerPosition.cpp
	MAnalyze.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
