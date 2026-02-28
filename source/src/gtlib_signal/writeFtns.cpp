#include <gtlib_signal/writeFtns.h>
#include <jl_lib/jlutil.h>
#include <jl_lib.h>
#include <cnpy.h>
#include <boost/filesystem.hpp>
using namespace std;
using namespace hff;

// ---------------------------------------------------------------
// Headers.
// ---------------------------------------------------------------

namespace gtlib {

void write_npz(const string& npPath, const string& npTxtPath, vector<SigC>* pSig, const LinearModel& linMod,
		unordered_map<string, string> mTickerUid)
{
	int nPts = (linMod.n1sec - 1) / 30 - 1; // number of 30 sec intervals
	int nTickerInuniv = 0;
	vector<string> wTickers;
	for( auto& sig : (*pSig) )
	{
		const string& ticker = sig.ticker;
		wTickers.push_back(ticker);
		if(sig.inUnivFit)
			++nTickerInuniv;
	}
	int nTicker = wTickers.size();
	vector<int> sortedIndex(nTicker);
	gsl_heapsort_index<int, string>(sortedIndex, wTickers);

	vector<double> vLogVolu(nTickerInuniv);
	vector<double> vLogPrice(nTickerInuniv);
	vector<double> vMedVolat(nTickerInuniv);

	vector<double> vvMid(nTickerInuniv * nPts);
	vector<double> vvTar1m(nTickerInuniv * nPts);
	vector<double> vvTar10m(nTickerInuniv * nPts);
	vector<double> vvTar60m(nTickerInuniv * nPts);
	vector<int> vvGptOK(nTickerInuniv * nPts);
	vector<double> vvTime(nTickerInuniv * nPts);
	vector<double> vvSprd(nTickerInuniv * nPts);
	vector<double> vvQI(nTickerInuniv * nPts);

	ofstream oft(npTxtPath);
	int cntTicker = 0;
	int cntData = 0;
	for(int i : sortedIndex)
	{
		auto& sig = (*pSig)[i];
		if(sig.inUnivFit)
		{
			int cnt = 0;
			const string& ticker = sig.ticker;
			const string uid = mTickerUid[ticker];

			if(cntData % nPts > 0)
			{
				printf("something is wrong, cntData= %d nPts= %d\n", cntData, nPts);
			}

			for(auto& sI : sig.sI)
			{
				if( sI.sampleType & regular_sample_ )
				{
					int msecs = sI.msso + linMod.openMsecs;
					double time = (double)sI.msso / (linMod.closeMsecs - linMod.openMsecs);

					int indx1m = 0;
					int indx10m = 1;
					int indx60m = 2;

					vvMid[cntData] = get_mid(sI.bid, sI.ask);
					vvTar1m[cntData] = sI.target[indx1m];
					vvTar10m[cntData] = sI.target[indx10m];
					vvTar60m[cntData] = sI.target[indx60m];
					vvGptOK[cntData] = sI.gptOK;
					vvTime[cntData] = time;
					vvSprd[cntData] = sI.sprd;
					vvQI[cntData] = sI.sig1[0];

					++cnt;
					++cntData;
				}
			}

			//printf("%f\n",sig.logVolu);

			vLogVolu[cntTicker] = sig.logVolu;
			vLogPrice[cntTicker] = sig.logPrice;
			vMedVolat[cntTicker] = sig.avgDlyVolat;

			++cntTicker;

			oft << ticker << '\t' << cnt << '\n';
		}
	}

	cnpy::npz_save(npPath, "nTicker", &nTickerInuniv, {1}, "w");
	cnpy::npz_save(npPath, "nPts", &nPts, {1}, "a");
	cnpy::npz_save(npPath, "vLogVolu", &vLogVolu[0], {nTickerInuniv}, "a");
	cnpy::npz_save(npPath, "vLogPrice", &vLogPrice[0], {nTickerInuniv}, "a");
	cnpy::npz_save(npPath, "vMedVolat", &vMedVolat[0], {nTickerInuniv}, "a");
	cnpy::npz_save(npPath, "vvMid", &vvMid[0], {nTickerInuniv, nPts}, "a");
	cnpy::npz_save(npPath, "vvTar1m", &vvTar1m[0], {nTickerInuniv, nPts}, "a");
	cnpy::npz_save(npPath, "vvTar10m", &vvTar10m[0], {nTickerInuniv, nPts}, "a");
	cnpy::npz_save(npPath, "vvTar60m", &vvTar60m[0], {nTickerInuniv, nPts}, "a");
	cnpy::npz_save(npPath, "vvGptOK", &vvGptOK[0], {nTickerInuniv, nPts}, "a");
	cnpy::npz_save(npPath, "vvTime", &vvTime[0], {nTickerInuniv, nPts}, "a");
	cnpy::npz_save(npPath, "vvSprd", &vvSprd[0], {nTickerInuniv, nPts}, "a");
	cnpy::npz_save(npPath, "vvQI", &vvQI[0], {nTickerInuniv, nPts}, "a");

	//cnpy::npz_t my_npz = cnpy::npz_load(npPath);
	//cnpy::NpyArray arr_mv1 = my_npz["vLogVolu"];
	//double* mv1 = arr_mv1.data<double>();
	//printf("%f\n", *mv1);
	//cnpy::NpyArray arr_mv2 = my_npz["vvGptOK"];
	//int* mv2 = arr_mv2.data<int>();
	//printf("%d\n", *mv2);

}

void write_bin_header(const string& sigType, ofstream& bin, ofstream& binTxt,
		const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	if( sigType == "om" )
		write_omBin_header(bin, binTxt, linMod, nonLinMod);
	else if( sigType == "tm" )
		write_tmBin_header(bin, binTxt, linMod, nonLinMod);
}

void write_omBin_header(ofstream& omBin, ofstream& omBinTxt,
		const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	if( omBin.is_open() )
	{
		// labels.
		string labels = "time," // 1
			"sprd,"
			"logVolu,"
			"logPrice,"
			"logPriceUsd,"
			"logCap,"
			"qI,"
			"TOBoff1,"
			"tarClose," // 10

			"tarCloseuh,"
			"predUniv,"
			"predErr,"
			"BsRat1,"
			"BsRat2,"
			"BqI2,"
			"Boff1,"
			"exchange,"
			"medVolat," // 20

			"hilo,"
			"Boff2,"
			"BOffmedVol.01,"
			"BOffmedVol.02,"
			"BOffmedVol.04,"
			"BOffmedVol.08,"
			"BOffmedVol.16,"
			"BOffmedVol.32,"
			"BmedSprdqI.25,"
			"fillImb," // 30

			"BmedSprdqI1,"
			"BmedSprdqI2,"
			"BmedSprdqI4,"
			"spxPspx,"
			"spfPspx,"

			"mret1,"
			"mret5,"
			"mret15,"
			"mret30,"
			"mret60,"
			"mret120,"
			"mret300,"
			"mret600," // 40
			"mret300L,"
			"mret600L," // 40
			"mret1200L," // 40
			"mret2400L," // 40

			"cwret30,"
			"cwret60,"
			"cwret120,"
			"cwret300,"
			"cwret600,"

			"swret30,"
			"swret60,"
			"swret120,"
			"swret300,"
			"swret300L,"

			"hiloQAI,"
			"hiloLag1Open,"
			"hiloLag1Rat,"
			"hiloLag1,"
			"hilo900,"
			"bollinger300,"
			"bollinger900,"

			"vm15,"
			"vm30,"
			"vm60,"
			"vm120,"
			"vm300,"
			"vm600,"
			"vmIntra,"
			"volSclBsz,"
			"volSclAsz,"
			"rtrd,"
			"rPastTrd,"
			"rCurTrd,"
			"mrtrd,"
			"dnmk,"
			"snmk,"

			"lcPlc," // 50

			"spfPlc,"
			"rusfPspx,"
			"rusfPlc,"
			"psspred,"
			"BOffmedVol.005,"
			"TOBqI2,"
			"TOBqI3,"
			"TOBoff2,"

			"predOm,"
			"predOm30s,"
			"predOm60s,"
			"predOm90s,"
			"predOm120s,"
			"predOm150s,"
			"predOm180s,"
			"predOm210s,"
			"predOm300s,"
			"predOm600s,"
			"mret30predOm,"
			"mret60predOm,"
			"mret120predOm,"
			"mret300predOm,"
			"mret600predOm,"
			"predTm,"
			"predTm30s,"
			"predTm60s,"
			"predTm90s,"
			"predTm120s,"
			"predTm300s,"
			"predTm600s,"
			"predOmIt1,"
			"predOm30sIt1,"
			"predOm60sIt1,"
			"predOm90sIt1,"
			"predOm120sIt1,"
			"predOm150sIt1,"
			"predOm180sIt1,"
			"predOm210sIt1,"
			"predOm300sIt1,"
			"predOm600sIt1,"
			"mret30predOmIt1,"
			"mret60predOmIt1,"
			"mret120predOmIt1,"
			"mret300predOmIt1,"
			"mret600predOmIt1,"

			"bidSize," // 60
			"bid,"
			"ask,"
			"askSize,"
			"costBid,"
			"costAsk,"
			"bidDepth900,"
			"askDepth900,"
			"bestPostTrade,"
			"relVolat,"
			"relSprd,"
			"ltrdImb";

		// Targets.
		int NH = linMod.nHorizon; for( int i = 0; i < NH; ++i ) // full target.
			labels += ",tar" + itos(linMod.vHorizonLag[i].first) + ";" + itos(linMod.vHorizonLag[i].second);
		for( int i = 0; i < NH; ++i ) // residual target = target - (predUniv + predErr).
			labels += ",restar" + itos(linMod.vHorizonLag[i].first) + ";" + itos(linMod.vHorizonLag[i].second);
		for( int i = 0; i < NH; ++i ) // predUniv + predErr.
			labels += ",bmpred" + itos(linMod.vHorizonLag[i].first) + ";" + itos(linMod.vHorizonLag[i].second);

		// nrows.
		int nrows = 0;
		omBin.write((char*)&(nrows), sizeof(int));

		// ncols.
		int ncols = 0; 
		for( int pos = 0; pos != string::npos; pos = labels.find(",", pos + 1) )
			++ncols;
		omBin.write((char*)&(ncols), sizeof(int));

		// labels len.
		long long int labels_len = labels.size() + 1; // 8 bytes on 32 and 64 bit OS for MSDN only; does not include terminating zero so add 1 
		omBin.write((char*)&labels_len, sizeof(labels_len));
		omBin.write(labels.c_str(), labels_len);
	}

	if( omBinTxt.is_open() )
	{
		omBinTxt.precision(6);
		omBinTxt << "uid\tticker\ttime" << endl;
	}
}

void write_tmBin_header(ofstream& tmBin, ofstream& tmBinTxt,
		const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	if( tmBin.is_open() )
	{
		// labels.
		string labels = "time,"
			"logVolu,"
			"logPrice,"
			"logPriceUsd,"
			"logMedSprd,"
			"logCap,"
			"mret60,"
			"mret120,"
			"mret300,"
			"mret300L,"
			"mret600L,"
			"mret1200L,"
			"m2400L,"
			"m4800L," // 10

			"cwret60,"
			"cwret120,"
			"cwret300,"
			"cwret300L,"

			"swret300,"
			"swret300L,"

			"mretONLag0,"
			"mretONLag1,"
			"mretIntraLag1,"
			"mretIntraLag2,"
			"cwretONLag0,"
			"cwretONLag1,"
			"cwretIntraLag1,"
			"cwretIntraLag2,"

			"vm300,"
			"vm600,"
			"vm3600,"
			"volSclBsz,"
			"volSclAsz,"
			"rtrd,"
			"rPastTrd,"
			"rCurTrd,"
			"mrtrd,"
			"dnmk,"
			"snmk,"

			"qIWt,"
			"qIMax,"
			"hilo,"
			"TOBqI2,"
			"TOBqI3,"
			"TOBoff1," // 20

			"TOBoff2,"
			"sprd,"
			"exchange,"
			"northRST,"
			"northTRD,"
			"northBP,"
			"hiloQAI,"
			"medVolat,"
			"prevDayVolu,"
			"mretOpen," // 30

			"qIMax2,"
			"vmIntra,"
			"isSecTypeF,"
			"sprdOpen,"
			"BOffmedVol.01,"
			"BOffmedVol.02,"
			"BOffmedVol.04,"
			"BOffmedVol.08," // 40

			"BOffmedVol.16,"
			"BOffmedVol.32,"
			"BqRat1,"
			"BqRat2,"
			"BsRat1,"
			"BsRat2,"
			"BqI2,"
			"BqI3,"
			"Boff1,"
			"Boff2," // 50

			"BmedSprdqI.25,"
			"BmedSprdqI.5,"
			"BmedSprdqI1,"
			"BmedSprdqI2,"
			"BmedSprdqI4,"
			"BmedSprdqI8,"
			"BmedSprdqI16,"
			"mI600,"
			"mI3600,"
			"mIIntra," // 60

			"hiloLag1Open,"
			"hiloLag2Open,"
			"hiloLag1Rat,"
			"hiloLag2Rat,"
			"hiloQAI2,"
			"hiloLag1,"
			"hiloLag2,"
			"hilo900,"
			"bollinger300,"
			"bollinger900," // 70
			"logBlockVolUsd60,"
			"logBlockVolUsd3600,"
			"logBlockVolUsdIntra,"

			"exchVol,"
			"relVolat,"
			"relSprd,"
			"ltrdImb,"

			// Tests.
			"predOm,"
			"predOm30s,"
			"predOm60s,"
			"predOm90s,"
			"predOm120s,"
			"predOm150s,"
			"predOm180s,"
			"predOm210s,"
			"predOm300s,"
			"predOm600s,"
			"mret30predOm,"
			"mret60predOm,"
			"mret120predOm,"
			"mret300predOm,"
			"mret600predOm,"
			"predTm,"
			"predTm30s,"
			"predTm60s,"
			"predTm90s,"
			"predTm120s,"
			"predTm150s,"
			"predTm180s,"
			"predTm210s,"
			"predTm300s,"
			"predTm600s,"
			"predTm1200s,"
			"predTm2400s,"
			"predTm4800s,"
			"mret120predTm,"
			"mret300predTm,"
			"mret600predTm,"
			"mret1200predTm,"
			"mret2400predTm,"
			"mret4800predTm,"
			"mret120predTm1,"
			"mret300predTm1,"
			"mret600predTm1,"
			"mret1200predTm1,"
			"mret2400predTm1,"
			"mret4800predTm1,"
			//"mretIntraLag3,"
			//"mretIntraLag4,"
			//"mretIntraLag5,"
			//"mretIntraLag6,"
			//"mretIntraLag7,"
			//"mretIntraLag8,"
			//"mretIntraLag3Sprd,"
			//"mretIntraLag4Sprd,"
			//"mretIntraLag5Sprd,"
			//"hiloQAI3,"
			//"hiloQAI4,"
			//"hiloQAI5,"
			//"hiloQAI6,"
			//"hiloQAI7,"
			//"hiloQAI8,"
			//"hiloQAI3Sprd,"
			//"hiloQAI4Sprd,"
			//"hiloQAI5Sprd,"
			//"hiloLag3Open,"
			//"hiloLag4Open,"
			//"hiloLag5Open,"
			//"hiloLag6Open,"
			//"hiloLag7Open,"
			//"hiloLag8Open,"
			//"hiloLag3OpenSprd,"
			//"hiloLag4OpenSprd,"
			//"hiloLag5OpenSprd,"
			//"hiloLag3Rat,"
			//"hiloLag4Rat,"
			//"hiloLag5Rat,"
			//"hiloLag6Rat,"
			//"hiloLag7Rat,"
			//"hiloLag8Rat,"
			//"hiloLag3RatSprd,"
			//"hiloLag4RatSprd,"
			//"hiloLag5RatSprd,"
			//"mretONLag2,"
			//"mretONLag3,"
			//"mretONLag4,"
			//"mretONLag5,"
			//"mretONLag6,"
			//"mretONLag7,"
			//"mretONLag2Sprd,"
			//"mretONLag3Sprd,"
			//"mretONLag4Sprd,"

			// Not used for fitting.
			"bidSize,"
			"bid,"
			"ask,"
			"askSize,"
			"costBid,"
			"costAsk,"

			// Targets.
			"tar11Close,"
			"tar11Closeuh,"
			"tar71Close,"
			"tar71Closeuh,"
			"tarClose,"
			"tarCloseuh,"
			"tarON,"
			"tarONuh,"
			"tarClcl,"
			"tarClcluh";

		// labels, targets.
		int N0 = linMod.nHorizon;
		int NH = nonLinMod.nHorizon;
		for( int i = N0; i < NH; ++i )
			labels += ",tar" + itos(nonLinMod.vHorizonLag[i].first) + ";" + itos(nonLinMod.vHorizonLag[i].second);
		for( int i = N0; i < NH; ++i )
			labels += ",tar" + itos(nonLinMod.vHorizonLag[i].first) + ";" + itos(nonLinMod.vHorizonLag[i].second) + "uh";
		for( int i = N0; i < NH; ++i ) // target - (predUniv + predErr).
			labels += ",restar" + itos(nonLinMod.vHorizonLag[i].first) + ";" + itos(nonLinMod.vHorizonLag[i].second);
		for( int i = N0; i < NH; ++i ) // predUniv + predErr.
			labels += ",bmpred" + itos(nonLinMod.vHorizonLag[i].first) + ";" + itos(nonLinMod.vHorizonLag[i].second);

		// nrows.
		int nrows = 0;
		tmBin.write((char*)&(nrows), sizeof(int));

		// ncols.
		int ncols = 0; 
		for( int pos = 0; pos != string::npos; pos = labels.find(",", pos + 1) )
			++ncols;
		tmBin.write((char*)&(ncols), sizeof(int));

		// labels len.
		long long int labels_len = labels.size() + 1; // 8 bytes on 32 and 64 bit OS for MSDN only; does not include terminating zero so add 1 
		tmBin.write((char*)&labels_len, sizeof(labels_len));
		tmBin.write(labels.c_str(), labels_len);
	}

	if( tmBinTxt.is_open() )
	{
		tmBinTxt.precision(6);
		tmBinTxt << "uid\tticker\ttime\tbsize\tbid\task\tasize\tbidEx\taskEx" << endl;
	}
}

void update_ncols(ofstream& ofs, int cnt)
{
	ofs.seekp(0);
	ofs.write((char*)&(cnt), sizeof(int));
}

int write_bin_evt(const string& sigType, ofstream& bin, ofstream& binTxt, const SigC& sig, const string& uid, const string& ticker,
		int minMsecs, int maxMsecs, int sampleType,
		const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	if( sigType == "om" )
		return write_omBin_evt(bin, binTxt, sig, uid, ticker, minMsecs, maxMsecs, sampleType, linMod, nonLinMod);
	else if( sigType == "tm" )
		return write_tmBin_evt(bin, binTxt, sig, uid, ticker, minMsecs, maxMsecs, sampleType, linMod, nonLinMod);
	return 0;
}

int write_omBin_evt(ofstream& omBin, ofstream& omBinTxt, const SigC& sig, const string& uid, const string& ticker,
		int minMsecs, int maxMsecs, int sampleType,
		const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	char buf[1000];
	int cntWritten = 0;
	int Npts = sig.sI.size();
	for(int j = 0; j < Npts; ++j)
	{
		const StateInfo& sI = sig.sI[j];
		if( sI.sampleType & sampleType && sI.valid )
		{
			int msecs = sI.msso + linMod.openMsecs;
			bool msecsOK = (msecs >= minMsecs && msecs <= maxMsecs);
			bool persistChina = !linMod.requirePersistChina || sI.persistentChina;
			if(msecsOK && persistChina)
			{
				++cntWritten;
				float time = sI.msso / 1000.;
				float dret = 0.;
				float notused = 0;
				float bidSize = sI.bsize * 100;
				float askSize = sI.asize * 100;

				sprintf(buf, "%s\t%s\t%d.%03d\n", uid.c_str(), ticker.c_str(), sI.msso/1000, sI.msso%1000); // float doesn't have enough precision.
				omBinTxt << buf; 

				omBin.write((char*)(&time), sizeof(float)); 
				omBin.write((char*)(&(sI.sprd)), sizeof(float)); 
				omBin.write((char*)(&(sig.logVolu)), sizeof(float)); 
				omBin.write((char*)(&(sig.logPrice)), sizeof(float));
				omBin.write((char*)(&(sig.logPriceUsd)), sizeof(float));
				omBin.write((char*)(&(sig.logCap)), sizeof(float));
				omBin.write((char*)(&(sI.sig1[0])), sizeof(float));
				omBin.write((char*)(&(sI.sig1[10])), sizeof(float));
				omBin.write((char*)(&(sI.targetClose)), sizeof(float));

				omBin.write((char*)(&(sI.targetCloseUH)), sizeof(float));
				omBin.write((char*)(&(sI.pred[0])), sizeof(float));
				omBin.write((char*)(&(sI.predErr[0])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[2])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[3])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[4])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[6])), sizeof(float));
				omBin.write((char*)(&(sig.exchange)), sizeof(float));
				omBin.write((char*)(&(sig.avgDlyVolat)), sizeof(float));

				omBin.write((char*)(&(sI.relativeHiLo)), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[7])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[8])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[14])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[15])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[16])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[17])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[18])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[19])), sizeof(float));
				omBin.write((char*)(&(sI.sig1[1])), sizeof(float));

				omBin.write((char*)(&(sI.sigBook[21])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[22])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[23])), sizeof(float));
				omBin.write((char*)(&(sI.sigIndex1[0])), sizeof(float));
				omBin.write((char*)(&(sI.sigIndex2[0])), sizeof(float));

				omBin.write((char*)(&(sI.mret1)), sizeof(float));
				omBin.write((char*)(&(sI.mret5)), sizeof(float));
				omBin.write((char*)(&(sI.mret15)), sizeof(float));
				omBin.write((char*)(&(sI.mret30)), sizeof(float));
				omBin.write((char*)(&(sI.sig1[2])), sizeof(float));
				omBin.write((char*)(&(sI.mret120)), sizeof(float));
				omBin.write((char*)(&(sI.sig1[14])), sizeof(float));
				omBin.write((char*)(&(sI.mret600)), sizeof(float));
				omBin.write((char*)(&(sI.sig1[15])), sizeof(float));
				omBin.write((char*)(&(sI.mret600L)), sizeof(float));
				omBin.write((char*)(&(sI.mret1200L)), sizeof(float));
				omBin.write((char*)(&(sI.mret2400L)), sizeof(float));

				omBin.write((char*)(&(sI.cwret30)), sizeof(float));
				omBin.write((char*)(&(sI.cwret60)), sizeof(float));
				omBin.write((char*)(&(sI.cwret120)), sizeof(float));
				omBin.write((char*)(&(sI.cwret300)), sizeof(float));
				omBin.write((char*)(&(sI.cwret600)), sizeof(float));

				omBin.write((char*)(&(sI.swret30)), sizeof(float));
				omBin.write((char*)(&(sI.swret60)), sizeof(float));
				omBin.write((char*)(&(sI.swret120)), sizeof(float));
				omBin.write((char*)(&(sI.swret300)), sizeof(float));
				omBin.write((char*)(&(sI.swret300L)), sizeof(float));

				omBin.write((char*)(&(sig.hiloQAI)), sizeof(float));
				omBin.write((char*)(&(sig.hiloLag1Open)), sizeof(float));
				omBin.write((char*)(&(sig.hiloLag1Rat)), sizeof(float));
				omBin.write((char*)(&(sI.hiloLag1)), sizeof(float));
				omBin.write((char*)(&(sI.hilo900)), sizeof(float));
				omBin.write((char*)(&(sI.bollinger300)), sizeof(float));
				omBin.write((char*)(&(sI.bollinger900)), sizeof(float));

				omBin.write((char*)(&(sI.voluMom15)), sizeof(float));
				omBin.write((char*)(&(sI.voluMom30)), sizeof(float));
				omBin.write((char*)(&(sI.voluMom60)), sizeof(float));
				omBin.write((char*)(&(sI.voluMom120)), sizeof(float));
				omBin.write((char*)(&(sI.voluMom300)), sizeof(float));
				omBin.write((char*)(&(sI.voluMom600)), sizeof(float));
				omBin.write((char*)(&(sI.intraVoluMom)), sizeof(float));
				omBin.write((char*)(&(sI.volSclBsz)), sizeof(float));
				omBin.write((char*)(&(sI.volSclAsz)), sizeof(float));
				omBin.write((char*)(&(sI.rtrd)), sizeof(float));
				omBin.write((char*)(&(sI.rPastTrd)), sizeof(float));
				omBin.write((char*)(&(sI.rCurTrd)), sizeof(float));
				omBin.write((char*)(&(sI.mrtrd)), sizeof(float));
				omBin.write((char*)(&(sI.dnmk)), sizeof(float));
				omBin.write((char*)(&(sI.snmk)), sizeof(float));

				omBin.write((char*)(&(sI.sigIndex3[0])), sizeof(float));

				omBin.write((char*)(&(sI.sigIndex4[0])), sizeof(float));
				omBin.write((char*)(&(sI.sigIndex5[0])), sizeof(float));
				omBin.write((char*)(&(sI.sigIndex6[0])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[26])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[27])), sizeof(float));
				omBin.write((char*)(&(sI.sig10[8])), sizeof(float));
				omBin.write((char*)(&(sI.sig10[9])), sizeof(float));
				omBin.write((char*)(&(sI.sig10[11])), sizeof(float));

				omBin.write((char*)(&(sI.predOm)), sizeof(float));
				omBin.write((char*)(&(sI.predOm30s)), sizeof(float));
				omBin.write((char*)(&(sI.predOm60s)), sizeof(float));
				omBin.write((char*)(&(sI.predOm90s)), sizeof(float));
				omBin.write((char*)(&(sI.predOm120s)), sizeof(float));
				omBin.write((char*)(&(sI.predOm150s)), sizeof(float));
				omBin.write((char*)(&(sI.predOm180s)), sizeof(float));
				omBin.write((char*)(&(sI.predOm210s)), sizeof(float));
				omBin.write((char*)(&(sI.predOm300s)), sizeof(float));
				omBin.write((char*)(&(sI.predOm600s)), sizeof(float));
				omBin.write((char*)(&(sI.mret30predOm)), sizeof(float));
				omBin.write((char*)(&(sI.mret60predOm)), sizeof(float));
				omBin.write((char*)(&(sI.mret120predOm)), sizeof(float));
				omBin.write((char*)(&(sI.mret300predOm)), sizeof(float));
				omBin.write((char*)(&(sI.mret600predOm)), sizeof(float));
				omBin.write((char*)(&(sI.predTm)), sizeof(float));
				omBin.write((char*)(&(sI.predTm30s)), sizeof(float));
				omBin.write((char*)(&(sI.predTm60s)), sizeof(float));
				omBin.write((char*)(&(sI.predTm90s)), sizeof(float));
				omBin.write((char*)(&(sI.predTm120s)), sizeof(float));
				omBin.write((char*)(&(sI.predTm300s)), sizeof(float));
				omBin.write((char*)(&(sI.predTm600s)), sizeof(float));
				omBin.write((char*)(&(sI.predOmIt1)), sizeof(float));
				omBin.write((char*)(&(sI.predOm30sIt1)), sizeof(float));
				omBin.write((char*)(&(sI.predOm60sIt1)), sizeof(float));
				omBin.write((char*)(&(sI.predOm90sIt1)), sizeof(float));
				omBin.write((char*)(&(sI.predOm120sIt1)), sizeof(float));
				omBin.write((char*)(&(sI.predOm150sIt1)), sizeof(float));
				omBin.write((char*)(&(sI.predOm180sIt1)), sizeof(float));
				omBin.write((char*)(&(sI.predOm210sIt1)), sizeof(float));
				omBin.write((char*)(&(sI.predOm300sIt1)), sizeof(float));
				omBin.write((char*)(&(sI.predOm600sIt1)), sizeof(float));
				omBin.write((char*)(&(sI.mret30predOmIt1)), sizeof(float));
				omBin.write((char*)(&(sI.mret60predOmIt1)), sizeof(float));
				omBin.write((char*)(&(sI.mret120predOmIt1)), sizeof(float));
				omBin.write((char*)(&(sI.mret300predOmIt1)), sizeof(float));
				omBin.write((char*)(&(sI.mret600predOmIt1)), sizeof(float));

				omBin.write((char*)(&(bidSize)), sizeof(float));
				omBin.write((char*)(&(sI.bid)), sizeof(float));
				omBin.write((char*)(&(sI.ask)), sizeof(float));
				omBin.write((char*)(&(askSize)), sizeof(float));
				omBin.write((char*)(&(sI.costBid)), sizeof(float));
				omBin.write((char*)(&(sI.costAsk)), sizeof(float));
				omBin.write((char*)(&(sI.bidDepth900)), sizeof(float));
				omBin.write((char*)(&(sI.askDepth900)), sizeof(float));
				omBin.write((char*)(&(sI.bestPostTrade)), sizeof(float));
				omBin.write((char*)(&(sI.relVolat)), sizeof(float));
				omBin.write((char*)(&(sI.relSprd)), sizeof(float));
				omBin.write((char*)(&(sI.ltrdImb)), sizeof(float));

				// Targets.
				int NH = linMod.nHorizon;
				for( int i = 0; i < NH; ++i )
				{
					float tar2 = sI.target[i];
					omBin.write((char*)(&tar2), sizeof(float));
				}
				for( int i = 0; i < NH; ++i )
				{
					float restar2 = sI.target[i] - (sI.pred[i] + sI.predErr[i]);
					omBin.write((char*)(&restar2), sizeof(float));
				}
				for( int i = 0; i < NH; ++i )
				{
					float bmpred2 = sI.pred[i] + sI.predErr[i];
					omBin.write((char*)(&bmpred2), sizeof(float));
				}
			}
		}
	}
	return cntWritten;
}

int write_tmBin_evt(ofstream& tmBin, ofstream& tmBinTxt, const SigC& sig, const string& uid, const string& ticker,
		int minMsecs, int maxMsecs, int sampleType,
		const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	char buf[1000];
	int cntWritten = 0;
	const int M = linMod.nHorizon;
	int Npts = sig.sI.size();
	for(int j = 0; j < Npts; ++j)
	{
		const StateInfo& sI = sig.sI[j];
		int msecs = sI.msso + linMod.openMsecs;
		bool msecsOK = (msecs >= minMsecs && msecs <= maxMsecs);
		bool persistChina = !linMod.requirePersistChina || sI.persistentChina;
		if( (sI.sampleType & sampleType) && sI.validTm && msecsOK && persistChina)
		{
			++cntWritten;

			float time = sI.msso / 1000.;
			float notused = 0.;

			float bidSize = sI.bsize * 100;
			float askSize = sI.asize * 100;
			sprintf(buf, "%s\t%s\t%d.%03d\t%f\t%f\t%f\t%f\t%c\t%c\n",
					uid.c_str(), ticker.c_str(), sI.msso/1000, sI.msso%1000,
					bidSize, sI.bid, sI.ask, askSize, sI.bidEx, sI.askEx);
			tmBinTxt << buf; 

			tmBin.write((char*)(&time), sizeof(float)); 
			tmBin.write((char*)(&(sig.logVolu)), sizeof(float)); 
			tmBin.write((char*)(&(sig.logPrice)), sizeof(float)); 
			tmBin.write((char*)(&(sig.logPriceUsd)), sizeof(float)); 
			tmBin.write((char*)(&(sig.logMedSprd)), sizeof(float));
			tmBin.write((char*)(&(sig.logCap)), sizeof(float));
			tmBin.write((char*)(&(sI.mret60)), sizeof(float));
			tmBin.write((char*)(&(sI.mret120)), sizeof(float));
			tmBin.write((char*)(&(sI.mret300)), sizeof(float));
			tmBin.write((char*)(&(sI.mret300L)), sizeof(float));
			tmBin.write((char*)(&(sI.mret600L)), sizeof(float));
			tmBin.write((char*)(&(sI.mret1200L)), sizeof(float));
			tmBin.write((char*)(&(sI.mret2400L)), sizeof(float));
			tmBin.write((char*)(&(sI.mret4800L)), sizeof(float));

			tmBin.write((char*)(&(sI.cwret60)), sizeof(float));
			tmBin.write((char*)(&(sI.cwret120)), sizeof(float));
			tmBin.write((char*)(&(sI.cwret300)), sizeof(float));
			tmBin.write((char*)(&(sI.cwret300L)), sizeof(float));

			tmBin.write((char*)(&(sI.swret300)), sizeof(float));
			tmBin.write((char*)(&(sI.swret300L)), sizeof(float));

			tmBin.write((char*)(&(sI.sig10[7])), sizeof(float)); // mretONLag0
			tmBin.write((char*)(&(sig.mretONLag1)), sizeof(float));
			tmBin.write((char*)(&(sig.mretIntraLag1)), sizeof(float));
			tmBin.write((char*)(&(sig.mretIntraLag2)), sizeof(float));
			tmBin.write((char*)(&(sI.cwretONLag0)), sizeof(float));
			tmBin.write((char*)(&(sig.cwretONLag1)), sizeof(float));
			tmBin.write((char*)(&(sig.cwretIntraLag1)), sizeof(float));
			tmBin.write((char*)(&(sig.cwretIntraLag2)), sizeof(float));

			tmBin.write((char*)(&(sI.voluMom300)), sizeof(float));
			tmBin.write((char*)(&(sI.voluMom600)), sizeof(float));
			tmBin.write((char*)(&(sI.voluMom3600)), sizeof(float));
			tmBin.write((char*)(&(sI.volSclBsz)), sizeof(float));
			tmBin.write((char*)(&(sI.volSclAsz)), sizeof(float));
			tmBin.write((char*)(&(sI.rtrd)), sizeof(float));
			tmBin.write((char*)(&(sI.rPastTrd)), sizeof(float));
			tmBin.write((char*)(&(sI.rCurTrd)), sizeof(float));
			tmBin.write((char*)(&(sI.mrtrd)), sizeof(float));
			tmBin.write((char*)(&(sI.dnmk)), sizeof(float));
			tmBin.write((char*)(&(sI.snmk)), sizeof(float));

			tmBin.write((char*)(&(sI.sig10[0])), sizeof(float)); // qIWt
			tmBin.write((char*)(&(sI.sig10[1])), sizeof(float)); // qIMax
			tmBin.write((char*)(&(sI.sig10[4])), sizeof(float)); // hilo
			tmBin.write((char*)(&(sI.sig10[8])), sizeof(float)); // TOBqI2
			tmBin.write((char*)(&(sI.sig10[9])), sizeof(float)); // TOBqI3
			tmBin.write((char*)(&(sI.sig10[10])), sizeof(float)); // TOBoff1

			tmBin.write((char*)(&(sI.sig10[11])), sizeof(float)); // TOBoff2
			tmBin.write((char*)(&(sI.sprd)), sizeof(float));
			tmBin.write((char*)(&(sig.exchange)), sizeof(float));
			tmBin.write((char*)(&(sig.northRST)), sizeof(float));
			tmBin.write((char*)(&(sig.northTRD)), sizeof(float));
			tmBin.write((char*)(&(sig.northBP)), sizeof(float));
			tmBin.write((char*)(&(sig.hiloQAI)), sizeof(float));
			tmBin.write((char*)(&(sig.avgDlyVolat)), sizeof(float)); // medVolat
			tmBin.write((char*)(&(sig.prevDayVolume)), sizeof(float));
			tmBin.write((char*)(&(sI.mretOpen)), sizeof(float));

			tmBin.write((char*)(&(sI.quimMax2)), sizeof(float));
			tmBin.write((char*)(&(sI.intraVoluMom)), sizeof(float)); // vmIntra
			tmBin.write((char*)(&(sig.isSecTypeF)), sizeof(float));
			tmBin.write((char*)(&(sig.sprdOpen)), sizeof(float));
			tmBin.write((char*)(&(sI.sigBook[8])), sizeof(float)); // BOffmedVol.01
			tmBin.write((char*)(&(sI.sigBook[14])), sizeof(float)); // BOffmedVol.02
			tmBin.write((char*)(&(sI.sigBook[15])), sizeof(float)); // BOffmedVol.04
			tmBin.write((char*)(&(sI.sigBook[16])), sizeof(float)); // BOffmedVol.08

			tmBin.write((char*)(&(sI.sigBook[17])), sizeof(float)); // BOffmedVol.16
			tmBin.write((char*)(&(sI.sigBook[18])), sizeof(float)); // BOffmedVol.32
			tmBin.write((char*)(&(sI.sigBook[0])), sizeof(float)); // BqRat1
			tmBin.write((char*)(&(sI.sigBook[1])), sizeof(float)); // BqRat2
			tmBin.write((char*)(&(sI.sigBook[2])), sizeof(float)); // BsRat1
			tmBin.write((char*)(&(sI.sigBook[3])), sizeof(float)); // BsRat2
			tmBin.write((char*)(&(sI.sigBook[4])), sizeof(float)); // BqI2
			tmBin.write((char*)(&(sI.sigBook[5])), sizeof(float)); // BqI3
			tmBin.write((char*)(&(sI.sigBook[6])), sizeof(float)); // Boff1
			tmBin.write((char*)(&(sI.sigBook[7])), sizeof(float)); // Boff2

			tmBin.write((char*)(&(sI.sigBook[19])), sizeof(float)); // BmedSprdqI.25
			tmBin.write((char*)(&(sI.sigBook[20])), sizeof(float)); // BmedSprdqI.5
			tmBin.write((char*)(&(sI.sigBook[21])), sizeof(float)); // BmedSprdqI1
			tmBin.write((char*)(&(sI.sigBook[22])), sizeof(float)); // BmedSprdqI2
			tmBin.write((char*)(&(sI.sigBook[23])), sizeof(float)); // BmedSprdqI4
			tmBin.write((char*)(&(sI.sigBook[24])), sizeof(float)); // BmedSprdqI8
			tmBin.write((char*)(&(sI.sigBook[25])), sizeof(float)); // BmedSprdqI16
			tmBin.write((char*)(&(sI.mI600)), sizeof(float));
			tmBin.write((char*)(&(sI.mI3600)), sizeof(float));
			tmBin.write((char*)(&(sI.mIIntra)), sizeof(float));

			tmBin.write((char*)(&(sig.hiloLag1Open)), sizeof(float));
			tmBin.write((char*)(&(sig.hiloLag2Open)), sizeof(float));
			tmBin.write((char*)(&(sig.hiloLag1Rat)), sizeof(float));
			tmBin.write((char*)(&(sig.hiloLag2Rat)), sizeof(float));
			tmBin.write((char*)(&(sig.hiloQAI2)), sizeof(float));
			tmBin.write((char*)(&(sI.hiloLag1)), sizeof(float));
			tmBin.write((char*)(&(sI.hiloLag2)), sizeof(float));
			tmBin.write((char*)(&(sI.hilo900)), sizeof(float));
			tmBin.write((char*)(&(sI.bollinger300)), sizeof(float));
			tmBin.write((char*)(&(sI.bollinger900)), sizeof(float));
			tmBin.write((char*)(&(sI.logBlockVolUsd60)), sizeof(float));
			tmBin.write((char*)(&(sI.logBlockVolUsd3600)), sizeof(float));
			tmBin.write((char*)(&(sI.logBlockVolUsdIntra)), sizeof(float));

			tmBin.write((char*)(&(sig.exchVol)), sizeof(float));
			tmBin.write((char*)(&(sI.relVolat)), sizeof(float));
			tmBin.write((char*)(&(sI.relSprd)), sizeof(float));
			tmBin.write((char*)(&(sI.ltrdImb)), sizeof(float));

			// Tests.
			tmBin.write((char*)(&(sI.predOm)), sizeof(float));
			tmBin.write((char*)(&(sI.predOm30s)), sizeof(float));
			tmBin.write((char*)(&(sI.predOm60s)), sizeof(float));
			tmBin.write((char*)(&(sI.predOm90s)), sizeof(float));
			tmBin.write((char*)(&(sI.predOm120s)), sizeof(float));
			tmBin.write((char*)(&(sI.predOm150s)), sizeof(float));
			tmBin.write((char*)(&(sI.predOm180s)), sizeof(float));
			tmBin.write((char*)(&(sI.predOm210s)), sizeof(float));
			tmBin.write((char*)(&(sI.predOm300s)), sizeof(float));
			tmBin.write((char*)(&(sI.predOm600s)), sizeof(float));
			tmBin.write((char*)(&(sI.mret30predOm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret60predOm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret120predOm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret300predOm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret600predOm)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm30s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm60s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm90s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm120s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm150s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm180s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm210s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm300s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm600s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm1200s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm2400s)), sizeof(float));
			tmBin.write((char*)(&(sI.predTm4800s)), sizeof(float));
			tmBin.write((char*)(&(sI.mret120predTm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret300predTm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret600predTm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret1200predTm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret2400predTm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret4800predTm)), sizeof(float));
			tmBin.write((char*)(&(sI.mret120predTm1)), sizeof(float));
			tmBin.write((char*)(&(sI.mret300predTm1)), sizeof(float));
			tmBin.write((char*)(&(sI.mret600predTm1)), sizeof(float));
			tmBin.write((char*)(&(sI.mret1200predTm1)), sizeof(float));
			tmBin.write((char*)(&(sI.mret2400predTm1)), sizeof(float));
			tmBin.write((char*)(&(sI.mret4800predTm1)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretIntraLag3)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretIntraLag4)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretIntraLag5)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretIntraLag6)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretIntraLag7)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretIntraLag8)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretIntraLag3Sprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretIntraLag4Sprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretIntraLag5Sprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloQAI3)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloQAI4)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloQAI5)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloQAI6)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloQAI7)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloQAI8)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloQAI3Sprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloQAI4Sprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloQAI5Sprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag3Open)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag4Open)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag5Open)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag6Open)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag7Open)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag8Open)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag3OpenSprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag4OpenSprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag5OpenSprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag3Rat)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag4Rat)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag5Rat)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag6Rat)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag7Rat)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag8Rat)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag3RatSprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag4RatSprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.hiloLag5RatSprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretONLag2)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretONLag3)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretONLag4)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretONLag5)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretONLag6)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretONLag7)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretONLag2Sprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretONLag3Sprd)), sizeof(float));
			//tmBin.write((char*)(&(sig.mretONLag4Sprd)), sizeof(float));

			// Not used for fitting.
			tmBin.write((char*)(&(bidSize)), sizeof(float));
			tmBin.write((char*)(&(sI.bid)), sizeof(float));
			tmBin.write((char*)(&(sI.ask)), sizeof(float));
			tmBin.write((char*)(&(askSize)), sizeof(float));
			tmBin.write((char*)(&(sI.costBid)), sizeof(float));
			tmBin.write((char*)(&(sI.costAsk)), sizeof(float));

			// Targets.
			tmBin.write((char*)(&(sI.target11Close)), sizeof(float));
			tmBin.write((char*)(&(sI.target11CloseUH)), sizeof(float));
			tmBin.write((char*)(&(sI.target71Close)), sizeof(float));
			tmBin.write((char*)(&(sI.target71CloseUH)), sizeof(float));
			tmBin.write((char*)(&(sI.targetClose)), sizeof(float));
			tmBin.write((char*)(&(sI.targetCloseUH)), sizeof(float));
			tmBin.write((char*)(&sig.tarON), sizeof(float));
			tmBin.write((char*)(&sig.tarONuh), sizeof(float));
			tmBin.write((char*)(&(sig.tarClcl)), sizeof(float));
			tmBin.write((char*)(&(sig.tarClcluh)), sizeof(float));

			// Targets.
			int N0 = linMod.nHorizon;
			int NH = nonLinMod.nHorizon;
			for( int i = N0; i < NH; ++i )
			{
				float tar = sI.target[i];
				tmBin.write((char*)(&tar), sizeof(float));
			}
			for( int i = N0; i < NH; ++i )
			{
				float tar = sI.targetUH[i];
				tmBin.write((char*)(&tar), sizeof(float));
			}
			for( int i = N0; i < NH; ++i )
			{
				float restar2 = sI.target[i] - (sI.pred[i] + sI.predErr[i]);
				tmBin.write((char*)(&restar2), sizeof(float));
			}
			for( int i = N0; i < NH; ++i )
			{
				float bmpred2 = sI.pred[i] + sI.predErr[i];
				tmBin.write((char*)(&bmpred2), sizeof(float));
			}
			//}
		}
	}
	return cntWritten;
}

} // namespace gtlib
