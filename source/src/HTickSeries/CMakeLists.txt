set(LIBNAME HTickSeries)
set(SOURCES
	FindReturns.cpp
	HQuoteSample.cpp
	HTickRead.cpp
	HTickReadM.cpp
	HTickSeriesHedge.cpp
	HTickSeriesQuote.cpp
	TickSeriesFtns.cpp
	HTickLoad.cpp
	HTickReadIndex.cpp
	HTickSeries.cpp
	HTickSeriesNews.cpp
	HTickSeriesTrade.cpp) 

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
