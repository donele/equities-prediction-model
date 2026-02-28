#include <MSignal/writeSig_at.h>
#include <boost/filesystem.hpp>
using namespace std;
using namespace hff;

const bool debug_sprd_write = true;

namespace writeSig {

// ---------------------------------------------------------------
// Content for each ticker.
// ---------------------------------------------------------------

int write_bin_evt(const string& sigType, ofstream& bin, ofstream& binTxt, const SigC& sig, const string& uid, const string& ticker,
		int minMsecs, int maxMsecs, int sampleType)
{
	if( sigType == "om" )
		return write_omBin_evt(bin, binTxt, sig, uid, ticker, minMsecs, maxMsecs, sampleType);
	else if( sigType == "tm" )
		return write_tmBin_evt(bin, binTxt, sig, uid, ticker, minMsecs, maxMsecs, sampleType);
	return 0;
}

int write_omBin_evt(ofstream& omBin, ofstream& omBinTxt, const SigC& sig, const string& uid, const string& ticker,
		int minMsecs, int maxMsecs, int sampleType)
{
	char buf[1000];
	int cntWritten = 0;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	int Npts = sig.sI.size();
	for(int j = 0; j < Npts; ++j)
	{
		const StateInfo& sI = sig.sI[j];
		if( sI.sampleType & sampleType && sI.valid )
		{
			int msecs = sI.msso + linMod.openMsecs;
			bool msecsOK = (msecs >= minMsecs && msecs <= maxMsecs);
			bool persist = !linMod.requirePersist || sI.persistent;
			if(msecsOK && persist)
			{
				++cntWritten;
				float time = sI.msso / 1000.;

				sprintf(buf, "%s\t%s\t%d.%03d\n", uid.c_str(), ticker.c_str(), sI.msso/1000, sI.msso%1000); // float doesn't have enough precision.
				omBinTxt << buf; 

				omBin.write((char*)(&time), sizeof(float)); 
				omBin.write((char*)(&(sI.sprd)), sizeof(float)); 
				omBin.write((char*)(&(sig.logVolu)), sizeof(float)); 
				omBin.write((char*)(&(sig.logPrice)), sizeof(float)); //3

				omBin.write((char*)(&(sI.sig1[0])), sizeof(float));
				omBin.write((char*)(&(sI.sig1[2])), sizeof(float));
				omBin.write((char*)(&(sI.sig1[10])), sizeof(float));
				omBin.write((char*)(&(sI.sig1[14])), sizeof(float));
				omBin.write((char*)(&(sI.sig1[15])), sizeof(float)); //8

				omBin.write((char*)(&(sI.targetClose)), sizeof(float)); //
				omBin.write((char*)(&(sI.targetCloseUH)), sizeof(float)); // 10

				omBin.write((char*)(&(sI.pred[0])), sizeof(float)); 	
				float notused = 0;
				omBin.write((char*)(&(sI.predErr[0])), sizeof(float)); 	//12
				omBin.write((char*)(&(sI.sigBook[2])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[3])), sizeof(float));
				omBin.write((char*)(&(sI.sigBook[4])), sizeof(float));
				omBin.write((char*)(&(sI.targetClose)), sizeof(float)); // 16

				omBin.write((char*)(&(sI.sigBook[6])), sizeof(float));

				omBin.write((char*)(&(sig.exchange)), sizeof(float)); // exchange
				omBin.write((char*)(&(sig.avgDlyVolat)), sizeof(float)); // volat
				omBin.write((char*)(&(sI.relativeHiLo)), sizeof(float)); // 20 hilo

				omBin.write((char*)(&(sI.sigBook[7])), sizeof(float)); //
				omBin.write((char*)(&(sI.sigBook[8])), sizeof(float)); //
				omBin.write((char*)(&(sI.sigBook[14])), sizeof(float)); //
				omBin.write((char*)(&(sI.sigBook[15])), sizeof(float)); //
				omBin.write((char*)(&(sI.sigBook[16])), sizeof(float)); // 25

				omBin.write((char*)(&(sI.sigBook[17])), sizeof(float)); //
				omBin.write((char*)(&(sI.sigBook[18])), sizeof(float)); //
				omBin.write((char*)(&(sI.sigBook[19])), sizeof(float)); //
				omBin.write((char*)(&(sI.sig1[1])), sizeof(float)); // fillImb
				omBin.write((char*)(&(sI.sigBook[21])), sizeof(float)); // 30
				omBin.write((char*)(&(sI.sigBook[22])), sizeof(float)); //
				omBin.write((char*)(&(sI.sigBook[23])), sizeof(float)); //
				omBin.write((char*)(&(sI.sigIndex1[0])), sizeof(float)); // spxPspx
				omBin.write((char*)(&(sI.sigIndex2[0])), sizeof(float)); // spfPlc
				omBin.write((char*)(&(sI.dswmds)), sizeof(float)); // 35
				omBin.write((char*)(&(sI.dswmdp)), sizeof(float));

				omBin.write((char*)(&(sI.mret5)), sizeof(float));
				omBin.write((char*)(&(sI.mret15)), sizeof(float));
				omBin.write((char*)(&(sI.mret30)), sizeof(float));
				omBin.write((char*)(&(sI.mret120)), sizeof(float)); // 40
				omBin.write((char*)(&(sI.mret600)), sizeof(float));

				omBin.write((char*)(&(sig.hiloQAI)), sizeof(float)); //
				omBin.write((char*)(&(sig.hiloLag1Open)), sizeof(float));
				omBin.write((char*)(&(sig.hiloLag1Rat)), sizeof(float));
				omBin.write((char*)(&(sI.hiloLag1)), sizeof(float)); // 45
				omBin.write((char*)(&(sI.hilo900)), sizeof(float));
				omBin.write((char*)(&(sI.bollinger300)), sizeof(float));
				omBin.write((char*)(&(sI.bollinger900)), sizeof(float));

				float dret = 0.;
				omBin.write((char*)(&sI.dswmdsTop), sizeof(float));
				omBin.write((char*)(&(sI.fIm1)), sizeof(float)); // 50
				omBin.write((char*)(&(sI.sigIndex3[0])), sizeof(float)); // lcPlc
				omBin.write((char*)(&(sI.sigIndex4[0])), sizeof(float)); // spfPlc
				omBin.write((char*)(&(sI.sigIndex5[0])), sizeof(float)); // rusfPspx
				omBin.write((char*)(&(sI.sigIndex6[0])), sizeof(float)); // rusfPlc
				omBin.write((char*)(&(sI.fIm2)), sizeof(float)); // 55
				omBin.write((char*)(&(sI.sigBook[26])), sizeof(float)); //
				omBin.write((char*)(&(sI.sigBook[27])), sizeof(float)); //
				omBin.write((char*)(&notused), sizeof(float)); //

				omBin.write((char*)(&(sI.sig10[9])), sizeof(float)); // qI3
				omBin.write((char*)(&(sI.sig10[10])), sizeof(float)); //60 of1
				omBin.write((char*)(&(sI.sig10[11])), sizeof(float)); // of2

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

				if( linMod.om_use_tm_input )
				{
					omBin.write((char*)(&(sig.logMedSprd)), sizeof(float));
					omBin.write((char*)(&(sI.sig10[3])), sizeof(float)); // mret300L
					omBin.write((char*)(&(sI.sig10[5])), sizeof(float)); // mret600L
					omBin.write((char*)(&(sI.sig10[6])), sizeof(float)); // mret1200L
					omBin.write((char*)(&(sI.mret2400L)), sizeof(float)); // m2400L
					omBin.write((char*)(&(sI.mret4800L)), sizeof(float)); // m4800L
					omBin.write((char*)(&(sI.sig10[7])), sizeof(float)); // mretONLag0
					omBin.write((char*)(&(sig.mretIntraLag1)), sizeof(float)); // mretIntraLag1
					omBin.write((char*)(&(sI.voluMom600)), sizeof(float)); // vm600
					omBin.write((char*)(&(sI.voluMom3600)), sizeof(float)); // vm3600
					omBin.write((char*)(&(sI.sig10[0])), sizeof(float)); // qIWt
					omBin.write((char*)(&(sI.sig10[1])), sizeof(float)); // qIMax
					omBin.write((char*)(&(sI.sig10[8])), sizeof(float)); // TOBqI2
					omBin.write((char*)(&(sI.sig10[9])), sizeof(float)); // TOBqI3
					omBin.write((char*)(&(sI.sig10[11])), sizeof(float)); // TOBoff2
					omBin.write((char*)(&(sig.northRST)), sizeof(float)); // northRST
					omBin.write((char*)(&(sig.northTRD)), sizeof(float)); // northTRD
					omBin.write((char*)(&(sig.northBP)), sizeof(float)); // northBP
					omBin.write((char*)(&(sig.hiloQAI)), sizeof(float)); // hiloQAI
					omBin.write((char*)(&(sig.prevDayVolume)), sizeof(float)); // prevDayVolu
					omBin.write((char*)(&(sI.mretOpen)), sizeof(float)); // mretOpen
					omBin.write((char*)(&(sI.quimMax2)), sizeof(float)); // qIMax2
					omBin.write((char*)(&(sig.mretONLag1)), sizeof(float)); // mretONLag1
					omBin.write((char*)(&(sig.mretIntraLag2)), sizeof(float)); // mretIntraLag2
					omBin.write((char*)(&(sI.intraVoluMom)), sizeof(float)); // vmIntra
					omBin.write((char*)(&(sig.sprdOpen)), sizeof(float)); // sprdOpen
					omBin.write((char*)(&(sI.sigBook[14])), sizeof(float)); // BOffmedVol.02
					omBin.write((char*)(&(sI.sigBook[15])), sizeof(float)); // BOffmedVol.04
					omBin.write((char*)(&(sI.sigBook[16])), sizeof(float)); // BOffmedVol.08
					omBin.write((char*)(&(sI.sigBook[17])), sizeof(float)); // BOffmedVol.16
					omBin.write((char*)(&(sI.sigBook[18])), sizeof(float)); // BOffmedVol.32
					omBin.write((char*)(&(sI.sigBook[0])), sizeof(float)); // BqRat1
					omBin.write((char*)(&(sI.sigBook[1])), sizeof(float)); // BqRat2
					omBin.write((char*)(&(sI.sigBook[5])), sizeof(float)); // BqI3					
					omBin.write((char*)(&(sI.sigBook[19])), sizeof(float)); // BmedSprdqI.25
					omBin.write((char*)(&(sI.sigBook[20])), sizeof(float)); // BmedSprdqI.5
					omBin.write((char*)(&(sI.sigBook[24])), sizeof(float)); // BmedSprdqI8
					omBin.write((char*)(&(sI.sigBook[25])), sizeof(float)); // BmedSprdqI16
					omBin.write((char*)(&(sI.mI600)), sizeof(float)); // mI600
					omBin.write((char*)(&(sI.mI3600)), sizeof(float)); // mI3600
					omBin.write((char*)(&(sI.mIIntra)), sizeof(float)); // mIIntra
					omBin.write((char*)(&(sig.hiloLag1Open)), sizeof(float)); // hiloLag1Open
					omBin.write((char*)(&(sig.hiloLag2Open)), sizeof(float)); // hiloLag2Open
					omBin.write((char*)(&(sig.hiloLag2Rat)), sizeof(float)); // hiloLag2Rat
					omBin.write((char*)(&(sig.hiloQAI2)), sizeof(float)); // hiloQAI2
					omBin.write((char*)(&(sI.hiloLag2)), sizeof(float)); // hiloLag2
					omBin.write((char*)(&(sI.hilo900)), sizeof(float)); // hilo900
				}

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
		int minMsecs, int maxMsecs, int sampleType)
{
	char buf[1000];
	int cntWritten = 0;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const int M = linMod.nHorizon;
	int Npts = sig.sI.size();
	for(int j = 0; j < Npts; ++j)
	{
		const StateInfo& sI = sig.sI[j];
		if( sI.sampleType & sampleType && sI.validTm )
		{
			int msecs = sI.msso + linMod.openMsecs;
			bool msecsOK = (msecs >= minMsecs && msecs <= maxMsecs);
			bool persist = !linMod.requirePersist || sI.persistent;
			if(msecsOK && persist)
			{
				++cntWritten;

				float time = sI.msso / 1000.;
				float notused = 0.;

				sprintf(buf, "%s\t%s\t%d.%03d\t%.2f\t%f\t%f\t%.2f\n", uid.c_str(), ticker.c_str(), sI.msso/1000, sI.msso%1000, sI.bsize, sI.bid, sI.ask, sI.asize);
				tmBinTxt << buf; 

				tmBin.write((char*)(&time), sizeof(float)); 
				tmBin.write((char*)(&(sig.logVolu)), sizeof(float)); 
				tmBin.write((char*)(&(sig.logPrice)), sizeof(float)); 
				tmBin.write((char*)(&(sig.logMedSprd)), sizeof(float));

				tmBin.write((char*)(&(sI.sig1[14])), sizeof(float)); // mret300
				tmBin.write((char*)(&(sI.sig10[3])), sizeof(float)); // mret300L // 5
				tmBin.write((char*)(&(sI.sig10[5])), sizeof(float)); // mret600L
				tmBin.write((char*)(&(sI.sig10[6])), sizeof(float)); // mret1200L
				tmBin.write((char*)(&(sI.mret2400L)), sizeof(float)); // m2400L
				tmBin.write((char*)(&(sI.mret4800L)), sizeof(float)); // m4800L
				tmBin.write((char*)(&(sI.sig10[7])), sizeof(float)); // mretONLag0 // 10
				tmBin.write((char*)(&(sig.mretIntraLag1)), sizeof(float)); // mretIntraLag1

				tmBin.write((char*)(&(sI.voluMom600)), sizeof(float)); // vm600
				tmBin.write((char*)(&(sI.voluMom3600)), sizeof(float)); // vm3600

				tmBin.write((char*)(&(sI.sig10[0])), sizeof(float)); // qIWt
				tmBin.write((char*)(&(sI.sig10[1])), sizeof(float)); // qIMax // 15
				tmBin.write((char*)(&(sI.sig10[4])), sizeof(float)); // hilo
				tmBin.write((char*)(&(sI.sig10[8])), sizeof(float)); // TOBqI2
				tmBin.write((char*)(&(sI.sig10[9])), sizeof(float)); // TOBqI3
				tmBin.write((char*)(&(sI.sig10[10])), sizeof(float)); // TOBoff1
				tmBin.write((char*)(&(sI.sig10[11])), sizeof(float)); // TOBoff2 // 20
				tmBin.write((char*)(&(sI.sprd)), sizeof(float)); // sprd

				// 40 min
				tmBin.write((char*)(&(sI.targetClose)), sizeof(float)); //
				tmBin.write((char*)(&(sI.targetCloseUH)), sizeof(float)); //

				// 10 min
				tmBin.write((char*)(&(sI.mI600)), sizeof(float)); // mI600
				tmBin.write((char*)(&(sI.mI3600)), sizeof(float)); // mI3600 // 25
				tmBin.write((char*)(&(sI.mIIntra)), sizeof(float)); // mIIntra
				tmBin.write((char*)(&notused), sizeof(float)); // tar5
				tmBin.write((char*)(&notused), sizeof(float)); // tar10
				tmBin.write((char*)(&notused), sizeof(float)); // tar60

				tmBin.write((char*)(&(sI.sigBook[0])), sizeof(float)); // BqRat1
				tmBin.write((char*)(&(sI.sigBook[1])), sizeof(float)); // BqRat2
				tmBin.write((char*)(&(sI.sigBook[2])), sizeof(float)); // BsRat1
				tmBin.write((char*)(&(sI.sigBook[3])), sizeof(float)); // BsRat2

				tmBin.write((char*)(&(sig.exchange)), sizeof(float)); // exchange
				tmBin.write((char*)(&(sig.northRST)), sizeof(float)); // northRST
				tmBin.write((char*)(&(sig.northTRD)), sizeof(float)); // northTRD
				tmBin.write((char*)(&(sig.northBP)), sizeof(float)); // northBP
				tmBin.write((char*)(&(sig.hiloQAI)), sizeof(float)); // hiloQAI
				tmBin.write((char*)(&(sig.avgDlyVolat)), sizeof(float)); // medVolat
				tmBin.write((char*)(&(sig.prevDayVolume)), sizeof(float)); // prevDayVolu
				tmBin.write((char*)(&(sI.mretOpen)), sizeof(float)); // mretOpen
				tmBin.write((char*)(&(sI.quimMax2)), sizeof(float)); // qIMax2
				tmBin.write((char*)(&(sig.mretONLag1)), sizeof(float)); // mretONLag1
				tmBin.write((char*)(&(sig.mretIntraLag2)), sizeof(float)); // mretIntraLag2
				tmBin.write((char*)(&(sI.intraVoluMom)), sizeof(float)); // vmIntra

				tmBin.write((char*)(&notused), sizeof(float)); // ompred
				tmBin.write((char*)(&(sig.isSecTypeF)), sizeof(float)); // isSecTypeF 47
				tmBin.write((char*)(&notused), sizeof(float)); // intraTar

				tmBin.write((char*)(&notused), sizeof(float)); // tar1060Intra

				tmBin.write((char*)(&(sig.sprdOpen)), sizeof(float)); // sprdOpen

				tmBin.write((char*)(&(sI.sigBook[4])), sizeof(float)); // BqI2
				tmBin.write((char*)(&(sI.sigBook[5])), sizeof(float)); // BqI3					
				tmBin.write((char*)(&(sI.sigBook[6])), sizeof(float)); // Boff1
				tmBin.write((char*)(&(sI.sigBook[7])), sizeof(float)); // Boff2
				tmBin.write((char*)(&(sI.sigBook[8])), sizeof(float)); // BOffmedVol.01

				tmBin.write((char*)(&(sI.sig1[1])), sizeof(float)); // fillImb
				tmBin.write((char*)(&(sI.sig1[2])), sizeof(float)); // mret60

				tmBin.write((char*)(&(sI.sigBook[26])), sizeof(float)); // BOffmedVol.0025
				tmBin.write((char*)(&(sI.sigBook[27])), sizeof(float)); // BOffmedVol.005

				tmBin.write((char*)(&notused), sizeof(float)); // tar1060Intra2 // 60

				tmBin.write((char*)(&(sI.sigBook[14])), sizeof(float)); // BOffmedVol.02
				tmBin.write((char*)(&(sI.sigBook[15])), sizeof(float)); // BOffmedVol.04
				tmBin.write((char*)(&(sI.sigBook[16])), sizeof(float)); // BOffmedVol.08
				tmBin.write((char*)(&(sI.sigBook[17])), sizeof(float)); // BOffmedVol.16
				tmBin.write((char*)(&(sI.sigBook[18])), sizeof(float)); // BOffmedVol.32
				tmBin.write((char*)(&(sI.sigBook[19])), sizeof(float)); // BmedSprdqI.25
				tmBin.write((char*)(&(sI.sigBook[20])), sizeof(float)); // BmedSprdqI.5
				tmBin.write((char*)(&(sI.sigBook[21])), sizeof(float)); // BmedSprdqI1
				tmBin.write((char*)(&(sI.sigBook[22])), sizeof(float)); // BmedSprdqI2
				tmBin.write((char*)(&(sI.sigBook[23])), sizeof(float)); // BmedSprdqI4
				tmBin.write((char*)(&(sI.sigBook[24])), sizeof(float)); // BmedSprdqI8
				tmBin.write((char*)(&(sI.sigBook[25])), sizeof(float)); // BmedSprdqI16

				tmBin.write((char*)(&notused), sizeof(float));
				tmBin.write((char*)(&notused), sizeof(float));

				tmBin.write((char*)(&notused), sizeof(float)); // minTick 75
				tmBin.write((char*)(&notused), sizeof(float)); // logDolVolu
				tmBin.write((char*)(&notused), sizeof(float)); // logDolPrice

				tmBin.write((char*)(&(sig.hiloLag1Open)), sizeof(float)); // hiloLag1Open
				tmBin.write((char*)(&(sig.hiloLag2Open)), sizeof(float)); // hiloLag2Open
				tmBin.write((char*)(&(sig.hiloLag1Rat)), sizeof(float)); // hiloLag1Rat 80
				tmBin.write((char*)(&(sig.hiloLag2Rat)), sizeof(float)); // hiloLag2Rat
				tmBin.write((char*)(&(sig.hiloQAI2)), sizeof(float)); // hiloQAI2

				tmBin.write((char*)(&(sI.hiloLag1)), sizeof(float)); // hiloLag1
				tmBin.write((char*)(&(sI.hiloLag2)), sizeof(float)); // hiloLag2
				tmBin.write((char*)(&(sI.hilo900)), sizeof(float)); // hilo900
				tmBin.write((char*)(&(sI.bollinger300)), sizeof(float)); // bollinger300
				tmBin.write((char*)(&(sI.bollinger900)), sizeof(float)); // bollinger900
				tmBin.write((char*)(&sI.dswmds), sizeof(float));
				tmBin.write((char*)(&sI.dswmdp), sizeof(float));	// notused
				tmBin.write((char*)(&notused), sizeof(float)); // 90
				tmBin.write((char*)(&sig.tarON), sizeof(float)); // tarON
				tmBin.write((char*)(&(sI.isExp)), sizeof(float)); // notused
				tmBin.write((char*)(&notused), sizeof(float)); //notused
				tmBin.write((char*)(&notused), sizeof(float)); //notused

				tmBin.write((char*)(&(sI.sigBook[9])), sizeof(float)); // BmedSprdqIWt1	95			
				tmBin.write((char*)(&(sI.sigBook[10])), sizeof(float)); // BmedSprdqIWt2					
				tmBin.write((char*)(&(sI.sigBook[11])), sizeof(float)); // BmedSprdqIWt4					
				tmBin.write((char*)(&(sI.sigBook[12])), sizeof(float)); // BmedSprdqIWt8					
				tmBin.write((char*)(&(sI.sigBook[13])), sizeof(float)); // BmedSprdqIWt16					
				tmBin.write((char*)(&notused), sizeof(float)); //100
				tmBin.write((char*)(&notused), sizeof(float)); 
				tmBin.write((char*)(&notused), sizeof(float)); 
				tmBin.write((char*)(&notused), sizeof(float)); 
				tmBin.write((char*)(&notused), sizeof(float)); 
				tmBin.write((char*)(&(sI.timeFrac)), sizeof(float)); // timeFrac

				tmBin.write((char*)(&notused), sizeof(float)); 

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
	}
	return cntWritten;
}
}
