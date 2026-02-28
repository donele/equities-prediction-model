set(LIBNAME jl_lib)
set(SOURCES AccMov.cpp Arg.cpp Book.cpp CorrDaily.cpp CorrMov.cpp CrossStat.cpp
	DayLoop.cpp GapFinder.cpp GCurr.cpp GEX.cpp GFee.cpp GODBC.cpp GTH.cpp GTS.cpp
	HConfig.cpp HPreProcessor.cpp IndexBetaInfo.cpp InputCounter.cpp jlutil.cpp
	jlutil_tickdata.cpp mto.cpp cme.cpp MercatorTrade.cpp MercatorOrder.cpp MovWinLinMod.cpp OLS.cpp OLSmov.cpp
	OLSmovMM.cpp OneSecRet.cpp OneDayStat.cpp Random.cpp RICfutures.cpp
	Sessions.cpp stat_util.cpp TickerSector.cpp TickSources.cpp MSessions.cpp
	NewsCatRP.cpp NewsCatRPGlobal.cpp QuoteStat.cpp SpreadBook.cpp QueryAggregator.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
