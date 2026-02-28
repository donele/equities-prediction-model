#include <MSignal/writeSig.h>
#include <jl_lib/jlutil.h>
using namespace std;
using namespace hff;

namespace writeSig {

// ---------------------------------------------------------------
// Headers.
// ---------------------------------------------------------------

void write_bin_header(const string& sigType, ofstream& bin, ofstream& binTxt)
{
	if( sigType == "om" )
		write_omBin_header(bin, binTxt);
	else if( sigType == "tm" )
		write_tmBin_header(bin, binTxt);
}

void write_omBin_header(ofstream& omBin, ofstream& omBinTxt)
{
	if( omBin.is_open() )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

		// labels.
		string labels = "time,sprd,logVolu,logPrice," // 3
			"qI,mret60,TOBoff1,mret300,mret300L," //8
			"tarClose,tarCloseuh," //10
			"predUniv,predErr,BsRat1,BsRat2,BqI2,tmp,Boff1," //17
			"exchange,medVolat,hilo," //20
			"Boff2,BOffmedVol.01,BOffmedVol.02,BOffmedVol.04,BOffmedVol.08," // 25
			"BOffmedVol.16,BOffmedVol.32,BmedSprdqI.25,fillImb,BmedSprdqI1," // 30
			"BmedSprdqI2,BmedSprdqI4,spxPspx,spfPspx,dswmds,dswmdp," // 36
			"mret5,mret15,mret30,mret120,mret600," // 41
			"hiloQAI,hiloLag1Open,hiloLag1Rat," //44
			"hiloLag1,hilo900,bollinger300,bollinger900," // 48
			"dswmdsTop,fIm1,lcPlc,spfPlc,rusfPspx,rusfPlc,fIm2," // 55
			"psspred,BOffmedVol.005," // 57
			"tmp,TOBqI2,TOBqI3,TOBoff2," // 61
			"indxPredWS1,indxPredWS2," // 63
			"indxPred1Adj,indxPred2Adj,indxPred1Sprd,indxPred2Sprd," // 67
			"eventARCA,eventNSDQ,eventNYSE,eventBATS,eventEDGX,eventEDGA," // 73
			"eventTakeRatMkt,eventTakeRatTot,bidDepth900,askDepth900,bestPostTrade,sipDiffMid,sipDiffSide"; // 80

		if( linMod.om_use_tm_input )
		{
			labels += ",logMedSprd,mret300L,mret600L,mret1200L,m2400L,m4800L,mretONLag0,mretIntraLag1,vm600,vm3600,"
				"qIWt,qIMax,TOBqI2,TOBqI3,TOBoff2,northRST,northTRD,northBP,hiloQAI,prevDayVolu,mretOpen,qIMax2,"
				"mretONLag1,mretIntraLag2,vmIntra,sprdOpen,BOffmedVol.02,BOffmedVol.04,BOffmedVol.08,BOffmedVol.16,BOffmedVol.32,"
				"BqRat1,BqRat2,BqI3,BmedSprdqI.25,BmedSprdqI.5,BmedSprdqI8,BmedSprdqI16,mI600,mI3600,mIIntra,hiloLag1Open,"
				"hiloLag2Open,hiloLag2Rat,hiloQAI2,hiloLag2,hilo900";
		}

		// Targets.
		int NH = linMod.nHorizon;
		for( int i = 0; i < NH; ++i ) // full target.
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

void write_tmBin_header(ofstream& tmBin, ofstream& tmBinTxt)
{
	if( tmBin.is_open() )
	{
		// labels.
		string labels = "time,logVolu,logPrice,logMedSprd," // 3 
			"mret300,mret300L,mret600L,mret1200L,"  // 7
			"m2400L,m4800L,mretONLag0,mretIntraLag1,"  // 11 
			"vm600,vm3600,"  //13 
			"qIWt,qIMax,hilo,TOBqI2,TOBqI3,TOBoff1,TOBoff2,sprd," // 21
			"tarClose,tarCloseuh,"   // 23 
			"mI600,mI3600,"   // 25 
			"mIIntra,tar5,"   // 27 
			"tar10,tar60,"   // 29 
			"BqRat1,BqRat2,BsRat1,BsRat2," // 33 
			"exchange,northRST,northTRD,northBP,"  // 37
			"hiloQAI,medVolat,prevDayVolu,mretOpen,"  //41
			"qIMax2,mretONLag1,mretIntraLag2,vmIntra," //45 
			"ompred,isSecTypeF," // 47
			"tmp,tmp,sprdOpen," // 50
			"BqI2,BqI3,Boff1,Boff2,BOffmedVol.01," // 55 
			"fillImb,mret60,BOffmedVol.0025,BOffmedVol.005,tmp,"  //60
			"BOffmedVol.02,BOffmedVol.04,BOffmedVol.08,BOffmedVol.16,BOffmedVol.32," // 65
			"BmedSprdqI.25,BmedSprdqI.5,BmedSprdqI1,BmedSprdqI2,BmedSprdqI4,"  //70
			"BmedSprdqI8,BmedSprdqI16," //72
			"omlinpred,totaltar,minTick,logDolVolu,logDolPrice,"// //77
			"hiloLag1Open,hiloLag2Open,hiloLag1Rat,hiloLag2Rat,hiloQAI2,"// //82
			"hiloLag1,hiloLag2,hilo900,bollinger300,bollinger900,"// 87
			"dswmds,dswmdp,"// 89
			"tmp,tarON,isExp,notused,notused," //94
			"BmedSprdqIWt1,BmedSprdqIWt2,BmedSprdqIWt4,BmedSprdqIWt8,BmedSprdqIWt16," //99
			"midCwap,midVwap,midCwap600,midVwap600,notused,timeFrac,notused"; // 106 // nffactors

		// labels, targets.
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
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
		tmBinTxt << "uid\tticker\ttime\tbsize\tbid\task\tasize" << endl;
	}
}

// ---------------------------------------------------------------
// Content for each ticker.
// ---------------------------------------------------------------

int write_omBin(ofstream& omBin, ofstream& omBinTxt, const SigC& sig, const string& uid, const string& ticker,
		int minMsecs, int maxMsecs, bool debug_sprd)
{
	int cnt = 0;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	for(int j = 0; j < linMod.gpts; j++)
	{
		const StateInfo& sI = sig.sI[j];
		int msecs = j * linMod.gridInterval * 1000 + linMod.openMsecs;
		bool sprdOK = fabs(sI.sprd) >= om_tree_min_fit_sprd_ && fabs(sI.sprd) < om_tree_max_fit_sprd_;
		bool msecsOK = (msecs >= minMsecs && msecs <= maxMsecs);
		bool sampleOK = ((j * linMod.gridInterval) % linMod.om_bin_sample_freq) == 0;
		bool persistChina = !linMod.requirePersistChina || sI.persistentChina;
		if( msecsOK && sI.valid && sampleOK && sprdOK && persistChina )
		{
			++cnt;
			int NH = linMod.nHorizon;

			float time = j * linMod.gridInterval;

			float tmp = 0;
			//float bmpred = sI.pred[0] + sI.predErr[0];
			//float target1 = sI.target[0];
			//float target1UH = 0.;

			omBinTxt << uid << "\t" << ticker << "\t" << time;
			omBinTxt << endl;

			omBin.write((char*)(&time), sizeof(float));
			omBin.write((char*)(&(sI.sprd)), sizeof(float));
			omBin.write((char*)(&(sig.logVolu)), sizeof(float));
			omBin.write((char*)(&(sig.logPrice)), sizeof(float)); //3

			omBin.write((char*)(&(sI.sig1[0])), sizeof(float));
			omBin.write((char*)(&(sI.sig1[2])), sizeof(float));
			omBin.write((char*)(&(sI.sig1[10])), sizeof(float));
			omBin.write((char*)(&(sI.sig1[14])), sizeof(float));
			omBin.write((char*)(&(sI.sig1[15])), sizeof(float)); //8

			//float bmpred = sI.pr1 + sI.pr1err;
			//float restar = target1 - bmpred;
			omBin.write((char*)(&(sI.targetClose)), sizeof(float)); //
			omBin.write((char*)(&(sI.targetCloseUH)), sizeof(float)); // 10

			omBin.write((char*)(&(sI.pred[0])), sizeof(float));
			omBin.write((char*)(&(sI.predErr[0])), sizeof(float)); 	//12
			omBin.write((char*)(&(sI.sigBook[2])), sizeof(float));
			omBin.write((char*)(&(sI.sigBook[3])), sizeof(float));
			omBin.write((char*)(&(sI.sigBook[4])), sizeof(float));
			omBin.write((char*)(&(sI.targetClose)), sizeof(float)); // 16

			omBin.write((char*)(&(sI.sigBook[6])), sizeof(float)); 						

			omBin.write((char*)(&(sig.exchange)), sizeof(float)); // exchange
			omBin.write((char*)(&(sig.avgDlyVolat)), sizeof(float)); // volat
			omBin.write((char*)(&(sI.relativeHiLo)), sizeof(float)); // hilo

			omBin.write((char*)(&(sI.sigBook[7])), sizeof(float)); //21 Boff2
			omBin.write((char*)(&(sI.sigBook[8])), sizeof(float)); // BOffmedVol.01
			omBin.write((char*)(&(sI.sigBook[14])), sizeof(float)); // BOffmedVol.02
			omBin.write((char*)(&(sI.sigBook[15])), sizeof(float)); // BOffmedVol.04
			omBin.write((char*)(&(sI.sigBook[16])), sizeof(float)); // BOffmedVol.08

			omBin.write((char*)(&(sI.sigBook[17])), sizeof(float)); // BOffmedVol.16
			omBin.write((char*)(&(sI.sigBook[18])), sizeof(float)); // BOffmedVol.32
			omBin.write((char*)(&(sI.sigBook[19])), sizeof(float)); // BmedSprdqI.25
			omBin.write((char*)(&(sI.sig1[1])), sizeof(float)); // fillImb
			omBin.write((char*)(&(sI.sigBook[21])), sizeof(float)); // 30 BmedSprdqI1
			omBin.write((char*)(&(sI.sigBook[22])), sizeof(float)); // BmedSprdqI2
			omBin.write((char*)(&(sI.sigBook[23])), sizeof(float)); // BmedSprdqI4
			omBin.write((char*)(&(sI.sigIndex1[0])), sizeof(float)); // spxPspx
			omBin.write((char*)(&(sI.sigIndex2[0])), sizeof(float)); // spfPlc
			omBin.write((char*)(&(sI.dswmds)), sizeof(float));
			omBin.write((char*)(&(sI.dswmdp)), sizeof(float));

			omBin.write((char*)(&(sI.mret5)), sizeof(float));
			omBin.write((char*)(&(sI.mret15)), sizeof(float));
			omBin.write((char*)(&(sI.mret30)), sizeof(float));
			omBin.write((char*)(&(sI.mret120)), sizeof(float)); // 40
			omBin.write((char*)(&(sI.mret600)), sizeof(float));

			omBin.write((char*)(&(sig.hiloQAI)), sizeof(float));
			omBin.write((char*)(&(sig.hiloLag1Open)), sizeof(float));
			omBin.write((char*)(&(sig.hiloLag1Rat)), sizeof(float));
			omBin.write((char*)(&(sI.hiloLag1)), sizeof(float));
			omBin.write((char*)(&(sI.hilo900)), sizeof(float));
			omBin.write((char*)(&(sI.bollinger300)), sizeof(float));
			omBin.write((char*)(&(sI.bollinger900)), sizeof(float));

			//float bmpredpss = sI.predErr[0];
			//float restarpss = target1 - bmpredpss;
			omBin.write((char*)(&(sI.dswmdsTop)), sizeof(float));
			omBin.write((char*)(&tmp), sizeof(float)); // 50
			omBin.write((char*)(&(sI.sigIndex3[0])), sizeof(float)); // lcPlc
			omBin.write((char*)(&(sI.sigIndex4[0])), sizeof(float)); // spfPlc
			omBin.write((char*)(&(sI.sigIndex5[0])), sizeof(float)); // rusfPspx
			omBin.write((char*)(&(sI.sigIndex6[0])), sizeof(float)); // rusfPlc
			omBin.write((char*)(&tmp), sizeof(float));
			omBin.write((char*)(&tmp), sizeof(float)); // 56
			omBin.write((char*)(&(sI.sigBook[27])), sizeof(float)); // BOffmedVol.005
			omBin.write((char*)(&tmp), sizeof(float));

			omBin.write((char*)(&(sI.sig10[9])), sizeof(float)); //qI3
			omBin.write((char*)(&(sI.sig10[10])), sizeof(float)); //60
			omBin.write((char*)(&(sI.sig10[11])), sizeof(float));

			omBin.write((char*)(&(sI.indxPredWS1)), sizeof(float));
			omBin.write((char*)(&(sI.indxPredWS2)), sizeof(float));

			omBin.write((char*)(&(sI.indxPred1Adj)), sizeof(float));
			omBin.write((char*)(&(sI.indxPred2Adj)), sizeof(float)); // 65
			omBin.write((char*)(&(sI.indxPred1Sprd)), sizeof(float));
			omBin.write((char*)(&(sI.indxPred2Sprd)), sizeof(float));
			omBin.write((char*)(&(sI.eventARCA)), sizeof(float));
			omBin.write((char*)(&(sI.eventNSDQ)), sizeof(float));
			omBin.write((char*)(&(sI.eventNYSE)), sizeof(float)); // 70
			omBin.write((char*)(&(sI.eventBATS)), sizeof(float));
			omBin.write((char*)(&(sI.eventEDGX)), sizeof(float));
			omBin.write((char*)(&(sI.eventEDGA)), sizeof(float));
			omBin.write((char*)(&(sI.eventTakeRatMkt)), sizeof(float));
			omBin.write((char*)(&(sI.eventTakeRatTot)), sizeof(float));
			omBin.write((char*)(&(sI.bidDepth900)), sizeof(float)); // 76
			omBin.write((char*)(&(sI.askDepth900)), sizeof(float));
			omBin.write((char*)(&(sI.bestPostTrade)), sizeof(float));
			omBin.write((char*)(&(sI.sipDiffMid)), sizeof(float));
			omBin.write((char*)(&(sI.sipDiffSide)), sizeof(float));

			// Targets and preds.
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
	return cnt;
}

int write_tmBin(ofstream& tmBin, ofstream& tmBinTxt, const SigC& sig, const string& uid, const string& ticker,
		int minMsecs, int maxMsecs, bool debug_sprd)
{
	int cnt = 0;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const int M = linMod.nHorizon;
	for(int j = 0; j < linMod.gpts; ++j)
	{
		const StateInfo& sI = sig.sI[j];

		int msecs = j * linMod.gridInterval * 1000 + linMod.openMsecs;
		//bool sprdOK = debug_sprd ? 1 : fabs(sI.sprd) >= om_tree_min_fit_sprd_ && fabs(sI.sprd) < om_tree_max_fit_sprd_;
		bool sprdOK = fabs(sI.sprd) >= om_tree_min_fit_sprd_ && fabs(sI.sprd) < om_tree_max_fit_sprd_;
		bool persistChina = !linMod.requirePersist || sI.persistentChina;
		if( (msecs >= minMsecs && msecs <= maxMsecs) && sI.valid && ((j * linMod.gridInterval) % linMod.tm_bin_sample_freq) == 0 && sprdOK && persistChina )
		{
			++cnt;

			float time = j * linMod.gridInterval;

			//float target1 = 0.;
			//float bmpred = 0.;
			//float ompred = bmpred;

			//float target10 = sI.target[M];
			//float psdtar = sI.target[M] + 0.5 * sI.target[M + 1];
			//float psdbmpred40 = 0.;
			//float ompred = sI.pr1 + sI.pr1err;
			//float tar60Intra = sI.target[M] + 0.5 * sI.target[M + 1] + 0.125 * sI.target60Intra;
			//float tar60Intra2 = sI.target[M] + 0.5 * sI.target[M + 1] + 0.0625 * sI.target60Intra;
			//float bmpred = sI.pr1 + sI.pr1err;
			//float restar = target1 - bmpred + psdtar;

			tmBinTxt << uid << "\t" << ticker << "\t" << time
				<< "\t" << sI.bsize << "\t" << sI.bid << "\t" << sI.ask << "\t" << sI.asize
				<< endl;

			tmBin.write((char*)(&time), sizeof(float)); 
			tmBin.write((char*)(&(sig.logVolu)), sizeof(float)); 
			tmBin.write((char*)(&(sig.logPrice)), sizeof(float)); 
			tmBin.write((char*)(&(sig.logMedSprd)), sizeof(float));

			tmBin.write((char*)(&(sI.sig1[14])), sizeof(float)); //mret300
			tmBin.write((char*)(&(sI.sig10[3])), sizeof(float)); //mret300L // 5
			tmBin.write((char*)(&(sI.sig10[5])), sizeof(float)); //mret600L
			tmBin.write((char*)(&(sI.sig10[6])), sizeof(float)); //mret1200L
			tmBin.write((char*)(&(sI.mret2400L)), sizeof(float)); //mret2400L
			tmBin.write((char*)(&(sI.mret4800L)), sizeof(float)); //mret4800L
			tmBin.write((char*)(&(sI.sig10[7])), sizeof(float)); //mretON // 10
			tmBin.write((char*)(&(sig.mretIntraLag1)), sizeof(float)); //prev day intra day

			tmBin.write((char*)(&(sI.voluMom600)), sizeof(float)); //volumom600
			tmBin.write((char*)(&(sI.voluMom3600)), sizeof(float)); //volumom3600

			tmBin.write((char*)(&(sI.sig10[0])), sizeof(float)); //wtQI
			tmBin.write((char*)(&(sI.sig10[1])), sizeof(float)); //maxQI // 15
			tmBin.write((char*)(&(sI.sig10[4])), sizeof(float)); //hilo
			tmBin.write((char*)(&(sI.sig10[8])), sizeof(float)); //qI2
			tmBin.write((char*)(&(sI.sig10[9])), sizeof(float)); //qI3
			tmBin.write((char*)(&(sI.sig10[10])), sizeof(float)); //of1
			tmBin.write((char*)(&(sI.sig10[11])), sizeof(float)); //of2 // 20
			tmBin.write((char*)(&(sI.sprd)), sizeof(float)); //sprd moved 

			// 40 min
			tmBin.write((char*)(&(sI.targetClose)), sizeof(float)); //
			tmBin.write((char*)(&(sI.targetCloseUH)), sizeof(float)); //

			// 10 min
			float tmp = 0.;
			tmBin.write((char*)(&(sI.mI600)), sizeof(float)); // tar
			tmBin.write((char*)(&(sI.mI3600)), sizeof(float)); // 25
			tmBin.write((char*)(&(sI.mIIntra)), sizeof(float)); // tar
			tmBin.write((char*)(&(sI.targetClose)), sizeof(float)); // tar
			tmBin.write((char*)(&tmp), sizeof(float));
			tmBin.write((char*)(&tmp), sizeof(float)); // tar

			tmBin.write((char*)(&(sI.sigBook[0])), sizeof(float)); //qutRat
			tmBin.write((char*)(&(sI.sigBook[1])), sizeof(float)); // qutRat					
			tmBin.write((char*)(&(sI.sigBook[2])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[3])), sizeof(float)); //sprdRat

			// new inputs 
			tmBin.write((char*)(&(sig.exchange)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sig.northRST)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sig.northTRD)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sig.northBP)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sig.hiloQAI)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sig.avgDlyVolat)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sig.prevDayVolume)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.mretOpen)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.quimMax2)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sig.mretONLag1)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sig.mretIntraLag2)), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.intraVoluMom)), sizeof(float)); //sprdRat

			tmBin.write((char*)(&tmp), sizeof(float));
			tmBin.write((char*)(&(sig.isSecTypeF)), sizeof(float)); //47
			tmBin.write((char*)(&(sI.targetClose)), sizeof(float)); //sprdRat

			tmBin.write((char*)(&tmp), sizeof(float)); // tarIntra60

			tmBin.write((char*)(&(sig.sprdOpen)), sizeof(float)); 

			tmBin.write((char*)(&(sI.sigBook[4])), sizeof(float)); //qutRat
			tmBin.write((char*)(&(sI.sigBook[5])), sizeof(float)); // qutRat					
			tmBin.write((char*)(&(sI.sigBook[6])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[7])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[8])), sizeof(float)); //sprdRat

			tmBin.write((char*)(&(sI.sig1[1])), sizeof(float)); // fillImb
			tmBin.write((char*)(&(sI.sig1[2])), sizeof(float)); // mret60

			tmBin.write((char*)(&(sI.sigBook[26])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[27])), sizeof(float)); //sprdRat

			tmBin.write((char*)(&tmp), sizeof(float)); // tarIntra60 // 60

			tmBin.write((char*)(&(sI.sigBook[14])), sizeof(float)); //qutRat
			tmBin.write((char*)(&(sI.sigBook[15])), sizeof(float)); // qutRat					
			tmBin.write((char*)(&(sI.sigBook[16])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[17])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[18])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[19])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[20])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[21])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[22])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[23])), sizeof(float)); //sprdRat
			tmBin.write((char*)(&(sI.sigBook[24])), sizeof(float)); //qutRat
			tmBin.write((char*)(&(sI.sigBook[25])), sizeof(float)); // qutRat					

			tmBin.write((char*)(&tmp), sizeof(float));
			tmBin.write((char*)(&tmp), sizeof(float));

			tmBin.write((char*)(&tmp), sizeof(float)); //75
			tmBin.write((char*)(&tmp), sizeof(float)); 
			tmBin.write((char*)(&tmp), sizeof(float)); 

			tmBin.write((char*)(&(sig.hiloLag1Open)), sizeof(float)); 
			tmBin.write((char*)(&(sig.hiloLag2Open)), sizeof(float)); 
			tmBin.write((char*)(&(sig.hiloLag1Rat)), sizeof(float)); //80
			tmBin.write((char*)(&(sig.hiloLag2Rat)), sizeof(float)); 
			tmBin.write((char*)(&(sig.hiloQAI2)), sizeof(float)); 

			tmBin.write((char*)(&(sI.hiloLag1)), sizeof(float)); 
			tmBin.write((char*)(&(sI.hiloLag2)), sizeof(float)); 
			tmBin.write((char*)(&(sI.hilo900)), sizeof(float)); 
			tmBin.write((char*)(&(sI.bollinger300)), sizeof(float)); 
			tmBin.write((char*)(&(sI.bollinger900)), sizeof(float)); 
			float dret = 0.;
			tmBin.write((char*)(&sI.dswmds), sizeof(float));					
			tmBin.write((char*)(&sI.dswmdp), sizeof(float));		
			tmBin.write((char*)(&(sI.targetClose)), sizeof(float)); // 90
			tmBin.write((char*)(&sig.tarON), sizeof(float)); 
			tmBin.write((char*)(&(sI.isExp)), sizeof(float)); 

			for(int w = 6 ; w < 8; ++w)
				tmBin.write((char*)(&tmp), sizeof(float)); //sprdRat

			tmBin.write((char*)(&(sI.sigBook[9])), sizeof(float)); // qutRat		//95			
			tmBin.write((char*)(&(sI.sigBook[10])), sizeof(float)); // qutRat					
			tmBin.write((char*)(&(sI.sigBook[11])), sizeof(float)); // qutRat					
			tmBin.write((char*)(&(sI.sigBook[12])), sizeof(float)); // qutRat					
			tmBin.write((char*)(&(sI.sigBook[13])), sizeof(float)); // qutRat					
			tmBin.write((char*)(&(sI.midCwap)), sizeof(float)); //100
			tmBin.write((char*)(&(sI.midVwap)), sizeof(float)); 
			tmBin.write((char*)(&(sI.midCwap600)), sizeof(float)); 
			tmBin.write((char*)(&(sI.midVwap600)), sizeof(float)); 
			tmBin.write((char*)(&tmp), sizeof(float)); 
			tmBin.write((char*)(&(sI.timeFrac)), sizeof(float)); 

			tmBin.write((char*)(&tmp), sizeof(float)); 

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
		}
	}
	return cnt;
}

void update_ncols(ofstream& ofs, int cnt)
{
	ofs.seekp(0);
	ofs.write((char*)&(cnt), sizeof(int));
}
}
