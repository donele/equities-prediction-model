set(LIBNAME gtlib_signal)
set(SOURCES
	#haltedTickers.cpp
	Hedge.cpp
	HedgeFullLen.cpp
	HedgeFullLenNF.cpp
	HedgeMarket.cpp
	HedgeNF.cpp
	HedgeNFadj.cpp
	MercatorVolume.cpp
	QuoteSample.cpp
	QuoteView.cpp
	RestrictedUS.cpp
	StatusChangeUS.cpp
	sigFtns.cpp
	signalObjs.cpp
	TargetHedger.cpp
	TargetHedgerFullLen.cpp
	TargetHedgerFullLenNF.cpp
	TargetHedgerMarket.cpp
	TargetHedgerNF.cpp
	TargetHedgerNFadj.cpp
	writeFtns.cpp
	)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
