#include <MCore/ModuleFactory.h>
#include <MFramework.h>
#include <MSignal.h>
#include <MSigMod.h>
#include <MFitting.h>
#include <MFitMod.h>
#include <MOrders.h>
#include <string>
using namespace std;

MModuleBase* ModuleFactory::getModule(const string& className,
		const string& moduleName, const multimap<string, string>& conf)
{
	if( "MInit" == className )
		return new MInit(moduleName, conf);
	else if( "MInitModel" == className )
		return new MInitModel(moduleName, conf);
	else if( "MSelectTickers" == className )
		return new MSelectTickers(moduleName, conf);
	else if( "MTickRead" == className )
		return new MTickRead(moduleName, conf);
	else if( "MTickReadSimple" == className )
		return new MTickReadSimple(moduleName, conf);
	else if( "MTickReadIndex" == className )
		return new MTickReadIndex(moduleName, conf);
	else if( "MMakeSampleTP" == className )
		return new MMakeSampleTP(moduleName, conf);
	else if( "MMakeSampleMicro" == className )
		return new MMakeSampleMicro(moduleName, conf);
	else if( "MTradeQuoteLatency" == className )
		return new MTradeQuoteLatency(moduleName, conf);
	else if( "MMergePred" == className )
		return new MMergePred(moduleName, conf);
	else if( "MPasteSample" == className )
		return new MPasteSample(moduleName, conf);
	else if( "MSimComp" == className )
		return new MSimComp(moduleName, conf);
	else if( "MCompSignal" == className )
		return new MCompSignal(moduleName, conf);
	else if( "MDumpSignalDL" == className )
		return new MDumpSignalDL(moduleName, conf);
	else if( "MMakeHedge" == className )
		return new MMakeHedge(moduleName, conf);
	else if( "MTopBottomAna" == className )
		return new MTopBottomAna(moduleName, conf);
	else if( "MFitIndexAR" == className )
		return new MFitIndexAR(moduleName, conf);
	else if( "MFitIndexOLS" == className )
		return new MFitIndexOLS(moduleName, conf);
	else if( "MFitIndexOLSTest" == className )
		return new MFitIndexOLSTest(moduleName, conf);
	else if( "MMakePredIndex" == className )
		return new MMakePredIndex(moduleName, conf);
	else if( "MMakePredIndexOLS" == className )
		return new MMakePredIndexOLS(moduleName, conf);
	else if( "MMakePredIndexOLSTest" == className )
		return new MMakePredIndexOLSTest(moduleName, conf);
	else if( "MMercatorTradeTime" == className )
		return new MMercatorTradeTime(moduleName, conf);
	else if( "MReadChara" == className )
		return new MReadChara(moduleName, conf);
	else if( "MReadOrder" == className )
		return new MReadOrder(moduleName, conf);
	else if( "MReadNews" == className )
		return new MReadNews(moduleName, conf);
	else if( "MReadSignal" == className )
		return new MReadSignal(moduleName, conf);
	else if( "MReadSignalON" == className )
		return new MReadSignalON(moduleName, conf);
	else if( "MReadSample" == className )
		return new MReadSample(moduleName, conf);
	else if( "MReadTickerState" == className )
		return new MReadTickerState(moduleName, conf);
	else if( "MFastSim" == className )
		return new MFastSim(moduleName, conf);
	else if( "MFastSimDataDump" == className )
		return new MFastSimDataDump(moduleName, conf);
	else if( "MFitAna" == className )
		return new MFitAna(moduleName, conf);
	else if( "MFitNtrees" == className )
		return new MFitNtrees(moduleName, conf);
	else if( "MFitRead" == className )
		return new MFitRead(moduleName, conf);
	else if( "MFitter" == className )
		return new MFitter(moduleName, conf);
	else if( "MPrintTrades" == className )
		return new MPrintTrades(moduleName, conf);
	else if( "MVolSurpAna" == className )
		return new MVolSurpAna(moduleName, conf);
	else if( "MFitterTest" == className )
		return new MFitterTest(moduleName, conf);
	else if( "MFitISLE" == className )
		return new MFitISLE(moduleName, conf);
	else if( "MCombineTree" == className )
		return new MCombineTree(moduleName, conf);
	else if( "MAnalyze" == className )
		return new MAnalyze(moduleName, conf);
	else if( "MPseudoTradePrep" == className )
		return new MPseudoTradePrep(moduleName, conf);
	else if( "MPseudoTrade" == className )
		return new MPseudoTrade(moduleName, conf);
	else if( "MPseudoTrade_10m" == className )
		return new MPseudoTrade_10m(moduleName, conf);
	else if( "MPseudoTradeFullSimComp" == className )
		return new MPseudoTradeFullSimComp(moduleName, conf);
	else if( "MPseudoTradeIntra" == className )
		return new MPseudoTradeIntra(moduleName, conf);
	else if( "MPseudoTradeRestore" == className )
		return new MPseudoTradeRestore(moduleName, conf);
	else if( "MPseudoTradePred" == className )
		return new MPseudoTradePred(moduleName, conf);
	else if( "MPseudoTradeProfile" == className )
		return new MPseudoTradeProfile(moduleName, conf);
	else if( "MPseudoTradeAdapt" == className )
		return new MPseudoTradeAdapt(moduleName, conf);
	else if( "MPseudoTradeAdaptVol" == className )
		return new MPseudoTradeAdaptVol(moduleName, conf);
	else if( "MPseudoTradeAuction" == className )
		return new MPseudoTradeAuction(moduleName, conf);
	else if( "MPseudoTradeMM" == className )
		return new MPseudoTradeMM(moduleName, conf);
	else if( "MPseudoTradeMultPred" == className )
		return new MPseudoTradeMultPred(moduleName, conf);
	else if( "MWriteWeightsTest" == className )
		return new MWriteWeightsTest(moduleName, conf);
	else if( "MWriteHfuniv" == className )
		return new MWriteHfuniv(moduleName, conf);
	else if( "MWriteFilters" == className )
		return new MWriteFilters(moduleName, conf);
	else if( "MWriteTree" == className )
		return new MWriteTree(moduleName, conf);
	else if( "MMercatorTrades" == className )
		return new MMercatorTrades(moduleName, conf);
	else if( "MOrderAna" == className )
		return new MOrderAna(moduleName, conf);
	else if( "MOrderSummRead" == className )
		return new MOrderSummRead(moduleName, conf);
	else if( "MSampleCaptureRat" == className )
		return new MSampleCaptureRat(moduleName, conf);
	else if( "MSelfDealingAna" == className )
		return new MSelfDealingAna(moduleName, conf);
	else if( "MTradeAna" == className )
		return new MTradeAna(moduleName, conf);
	else if( "MTradeEventAna" == className )
		return new MTradeEventAna(moduleName, conf);

	cerr << " module " << className << " is unknown to MCore." << endl;
	exit(3);
	return nullptr;
}
