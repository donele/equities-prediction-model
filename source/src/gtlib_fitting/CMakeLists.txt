set(LIBNAME gtlib_fitting)
set(SOURCES
	BDT_costWgtTar.cpp
	BDT_wgtTradable.cpp
	BoostedDecisionTree.cpp
	CorrInput.cpp
	DailyDataCount.cpp
	DecisionTree.cpp
	DecisionTreeRegression.cpp
	DecisionTreeRegressionForce.cpp
	DecisionTreeRegressionPrune.cpp
	DecisionTreeRegressionTimeWgt.cpp
	DecisionTreeRegressionWgt.cpp
	DTFtns.cpp
	FitData.cpp
	Fitter.cpp
	fittingFtns.cpp
	fittingObjs.cpp
	PerfBoost.cpp
	PredFromBoostedTree.cpp
	VariablesInfo.cpp)

add_library(${LIBNAME} ${SOURCES})
install(TARGETS ${LIBNAME} DESTINATION lib64)
