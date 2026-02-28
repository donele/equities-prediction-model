set(LIBNAME MSignal)
set(SOURCES
	flt.cpp
	IndexInputMaker.cpp
	MCompSignal.cpp
	MDumpSignalDL.cpp
	MFitIndexAR.cpp
	MFitIndexOLS.cpp
	MFitIndexOLSTest.cpp
	MMakeHedge.cpp
	MMakePredIndex.cpp
	MMakePredIndexOLS.cpp
	MMakePredIndexOLSTest.cpp
	MMercatorTradeTime.cpp
	MMergePred.cpp
	MPasteSample.cpp
	MReadChara.cpp
	MReadNews.cpp
	MReadOrder.cpp
	MSimComp.cpp
	MTickRead.cpp
	MTickReadIndex.cpp
	MTickReadSimple.cpp
	MTopBottomAna.cpp
	MWriteFilters.cpp
	MWriteHfuniv.cpp
	MWriteWeightsTest.cpp
	sig_at.cpp
	sig.cpp
	SigSet.cpp
	)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
