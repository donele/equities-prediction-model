#include <HFitting/writeSig.h>
using namespace std;
using namespace hff;

namespace writeSig {

	// ---------------------------------------------------------------
	// Paths.
	// ---------------------------------------------------------------

	string get_bmTxt_path(string model, int idate)
	{
		char outdir[400];
		sprintf( outdir, "%s\\%s\\txtSig\\bm", HEnv::Instance()->baseDir().c_str(), model.c_str() );
		ForceDirectory(outdir);

		string filename = "sig" + itos(idate) + "Ebm.txt";
		string path = (string)outdir + "\\" + filename;
		return path;
	}

	string get_omTxt_path(string model, int idate)
	{
		char outdir[400];
		sprintf( outdir, "%s\\%s\\txtSig\\om", HEnv::Instance()->baseDir().c_str(), model.c_str() );
		ForceDirectory(outdir);

		string filename = "sigTB" + itos(idate) + "Eom.txt";
		string path = (string)outdir + "\\" + filename;
		return path;
	}

	string get_omBin_path(string model, int idate)
	{
		char outdir[400];
		sprintf( outdir, "%s\\%s\\binSig\\om", HEnv::Instance()->baseDir().c_str(), model.c_str() );
		ForceDirectory(outdir);

		string filename = "sig" + itos(idate) + "Eom.bin";
		string path = (string)outdir + "\\" + filename;
		return path;
	}

	string get_omBinTxt_path(string model, int idate)
	{
		char outdir[400];
		sprintf( outdir, "%s\\%s\\binSig\\om", HEnv::Instance()->baseDir().c_str(), model.c_str() );
		ForceDirectory(outdir);

		string filename = "sig" + itos(idate) + "Eom.txt";
		string path = (string)outdir + "\\" + filename;
		return path;
	}

	string get_tmTxt_path(string model, int idate)
	{
		char outdir[400];
		sprintf( outdir, "%s\\%s\\txtSig\\tm", HEnv::Instance()->baseDir().c_str(), model.c_str() );
		ForceDirectory(outdir);

		string filename = "sigTB" + itos(idate) + "Etm.txt";
		string path = (string)outdir + "\\" + filename;
		return path;
	}

	string get_tmBin_path(string model, int idate)
	{
		char outdir[400];
		sprintf( outdir, "%s\\%s\\binSig\\tm", HEnv::Instance()->baseDir().c_str(), model.c_str() );
		ForceDirectory(outdir);

		string filename = "sig" + itos(idate) + "Etm.bin";
		string path = (string)outdir + "\\" + filename;
		return path;
	}

	string get_tmBinTxt_path(string model, int idate)
	{
		char outdir[400];
		sprintf( outdir, "%s\\%s\\binSig\\tm", HEnv::Instance()->baseDir().c_str(), model.c_str() );
		ForceDirectory(outdir);

		string filename = "sig" + itos(idate) + "Etm.txt";
		string path = (string)outdir + "\\" + filename;
		return path;
	}

	// ---------------------------------------------------------------
	// Headers.
	// ---------------------------------------------------------------

	void write_bmTxt_header(ofstream& bmTxt, string model, int idate)
	{
		bmTxt.open( get_bmTxt_path(model, idate).c_str(), ios::out );
		if( bmTxt.is_open() )
		{
			bmTxt.precision(6);
			bmTxt << "uid" << "\t" << "ticker" << "\t" << "time";

			const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
			int NH = linMod.nHorizon;

			for( int i = 0; i < NH; ++i )
				bmTxt << "\t" << "pred" + itos(linMod.vHorizonLag[i].first) + ";" + itos(linMod.vHorizonLag[i].second);
			for( int i = 0; i < NH; ++i )
				bmTxt << "\t" << "predErr" + itos(linMod.vHorizonLag[i].first) + ";" + itos(linMod.vHorizonLag[i].second);
			for( int i = 0; i < NH; ++i )
				bmTxt << "\t" << "target" + itos(linMod.vHorizonLag[i].first) + ";" + itos(linMod.vHorizonLag[i].second);

			bmTxt << endl;
		}
	}

	void write_omTxt_header(ofstream& omTxt, string model, int idate)
	{
		omTxt.open( get_omTxt_path(model, idate).c_str(), ios::out );
		if( omTxt.is_open() )
		{
			omTxt.precision(6);
			omTxt << "uid" << "\t" << "ticker" << "\t" << "time"<<"\t"<<"sprd"<<"\t"<<"logVolu"<<  //4
				"\t" << "logPrice" << "\t"<<"qI"<<"\t"<<"mret60"<<     //7
				"\t" << "TOBoff1" << "\t"<<"mret300"<<"\t"<<"mret300L"<<"\t"<<"TOBqI2" <<  //11
				"\t" << "TOBqI3" << "\t"<<"TOBoff2"<<"\t"<<"BqRat1"<<"\t"<<"BqRat2"<<  //15
				"\t" << "BsRat1" << "\t"<<"BsRat2" <<"\t" <<  "target1" << "\t" << "bmpred" << //19
				"\t" << "exchange" << "\t" << "medVolat" << "\t" << "hilo" <<  //22
				"\t" << "BqI2" << "\t" << "Boff1" << "\t" << "Boff2" <<			//25
				"\t" << "BOffmedVol.01" << "\t" << "BmedSprdqI.5" << "\t" << "BmedSprdqI1" <<		//28	
				"\t" << "BmedSprdqI2" << "\t" << "BmedSprdqI4" <<   //30
				"\t" << "fillImb" << "\t" << "spxPspx" << "\t"  << "spfPspx" << "\t" << "lcPlc" << //34
				"\t" << "spfPlc"  << "\t" << "rusfPspx" << "\t" << "rusfPlc" << //37
				"\t" << "logDolVolu" << "\t" <<"logDolPrice" << 	//39
				"\t" << "mret5" << "\t" << "mret15" << "\t" << "mret30" << //42
				"\t" << "mret120" << "\t"  << "mret600" <<  //44
				"\t" <<"hiloQAI" << "\t" << "hiloLag1Open" << "\t" <<"hiloLag1Rat" << //47
				"\t" <<"hiloLag1" << "\t" << "hilo900" << "\t" <<"bollinger300" <<  "\t" <<"bollinger900" << //51
				"\t" << "dmret5" << "\t" << "dmret15" << "\t" << "dmret30" << "\t" << "dmret60" <<  //55
				"\t" << "dmret120" << "\t" << "dmret300" << "\t" << "dmret600" <<  
				"\t" << "ssECpred" << endl;
		}
	}

	void write_omBin_header(ofstream& omBin, ofstream& omBinTxt, string model, int idate)
	{
		omBin.open( get_omBin_path(model, idate).c_str(), ios::out | ios::binary );
		if( omBin.is_open() )
		{
			int nrows = 0;
			omBin.write((char*)&(nrows), sizeof(int));

			int ncols = 69; 
			omBin.write((char*)&(ncols), sizeof(int));

			//static const char* labels;
			//labels = "time,sprd,logVolu,logPrice," // 3
			//	"qI,mret60,TOBoff1,mret300,mret300L," //8
			//	"restar,bmpred,"  //10
			//	"minTick,arDown,BsRat1,BsRat2,BqI2,arOscil,Boff1,"  //17
			//	"exchange,medVolat,hilo,"//20
			//	"Boff2,BOffmedVol.01,BOffmedVol.02,BOffmedVol.04,BOffmedVol.08,"  // 25
			//	"BOffmedVol.16,BOffmedVol.32,BmedSprdqI.25,BmedSprdqI.5,BmedSprdqI1,"  // 30
			//	"BmedSprdqI2,BmedSprdqI4,BmedSprdqI8,BmedSprdqI16,logDolVolu,logDolPrice," //minTick";  // 36
			//	"mret5,mret15,mret30,mret120,mret600," // 41
			//	"hiloQAI,hiloLag1Open,hiloLag1Rat,"//,minTick";  //44
			//	"hiloLag1,hilo900,bollinger300,bollinger900," // 48
			//	"dmret5,dmret15,dmret30,dmret60,dmret120,dmret300,dmret600," // 55 <- these are for ETFs only
			//	"BOffmedVol.0025,BOffmedVol.005," // 57
			//	"target1UH,TOBqI2,TOBqI3,TOBoff2," // 61
			//	"dark1,dark5," // 63
			//	"dark20,mI15,mI30,mI60,mI120"; // 68

			//long long int labels_len = strlen(labels) + 1; // 8 bytes on 32 and 64 bit OS for MSDN only; does not include terminating zero so add 1 
			//omBin.write((char*)&labels_len, sizeof(labels_len));
			//omBin.write(labels, labels_len);

			string label1;
			label1 = "time,sprd,logVolu,logPrice," // 3
				"qI,mret60,TOBoff1,mret300,mret300L," //8
				"restar,bmpred,"  //10
				"minTick,arDown,BsRat1,BsRat2,BqI2,arOscil,Boff1,"  //17
				"exchange,medVolat,hilo,"//20
				"Boff2,BOffmedVol.01,BOffmedVol.02,BOffmedVol.04,BOffmedVol.08,"  // 25
				"BOffmedVol.16,BOffmedVol.32,BmedSprdqI.25,BmedSprdqI.5,BmedSprdqI1,"  // 30
				"BmedSprdqI2,BmedSprdqI4,BmedSprdqI8,BmedSprdqI16,logDolVolu,logDolPrice," //minTick";  // 36
				"mret5,mret15,mret30,mret120,mret600," // 41
				"hiloQAI,hiloLag1Open,hiloLag1Rat,"//,minTick";  //44
				"hiloLag1,hilo900,bollinger300,bollinger900," // 48
				"dmret5,dmret15,dmret30,dmret60,dmret120,dmret300,dmret600," // 55 <- these are for ETFs only
				"BOffmedVol.0025,BOffmedVol.005," // 57
				"target1UH,TOBqI2,TOBqI3,TOBoff2," // 61
				"dark1,dark5," // 63
				"dark20,mI15,mI30,mI60,mI120"; // 68

			string labels = label1;
			long long int labels_len = labels.size() + 1; // 8 bytes on 32 and 64 bit OS for MSDN only; does not include terminating zero so add 1 
			omBin.write((char*)&labels_len, sizeof(labels_len));
			omBin.write(labels.c_str(), labels_len);
		}

		omBinTxt.open( get_omBinTxt_path(model, idate).c_str(), ios::out );
		if( omBinTxt.is_open() )
		{
			omBinTxt.precision(6);
			omBinTxt << "uid" << "\t" << "time" << endl;
		}
	}

	void write_tmTxt_header(ofstream& tmTxt, string model, int idate)
	{
		tmTxt.open( get_tmTxt_path(model, idate).c_str(), ios::out );
		if( tmTxt.is_open() )
		{
			tmTxt.precision(6);
			tmTxt <<"uid"<<"\t"   //  0    
				<<"ticker"<<"\t"
				<<"time"<<"\t"   //2
				<<"logVolu"<<"\t"
				<<"logPrice"<<"\t"
				<<"logMedSprd"<<"\t"
				<<"mret300"<<"\t"
				<<"mret300L"<<"\t"
				<<"mret600L"<<"\t"
				<<"mret1200L"<<"\t"  
				<<"mret2400L"<<"\t"  //10
				<<"mret4800L"<<"\t"
				<<"mretONLag0"<<"\t"
				<<"mretIntraLag1"<<"\t"
				<<"vm600"<<"\t"
				<<"vm3600"<<"\t"
				<<"qIWt"<<"\t"
				<<"qIMax"<<"\t"
				<<"hilo"<<"\t"
				<<"TOBqI2"<<"\t"   
				<<"TOBqI3"<<"\t"   //20 
				<<"TOBoff1"<<"\t"
				<<"TOBoff2"<<"\t" 
				<<"sprd"<<"\t"   //23 
				<<"target"<<"\t"    // 24 
				<<"uhtarget"<<"\t"
				<<"exchange"<<"\t"    
				<<"northRST"<<"\t"
				<<"northTRD"<<"\t"
				<<"northBP"<<"\t"
				<<"hiloLag1"<<"\t"  //30
				<<"medVolat"<<"\t"
				<<"prevDayVolu"<<"\t"	
				<<"mretOpen"<<"\t"	
				<<"qIMax2"<<"\t"
				<<"mretONLag1" <<"\t"
				<<"mretIntraLag2" <<"\t" 
     				<<"vmIntra" << "\t"
				<<"isSecTypeF" << "\t" 
				<<"sprdOpen" << "\t" 

				<<"BOffmedVol.01"<<"\t"   //40
				<<"BOffmedVol.02"<<"\t"  
				<<"BOffmedVol.04"<<"\t"  
				<<"BOffmedVol.08"<<"\t"  
				<<"BOffmedVol.16"<<"\t"  
				<<"BOffmedVol.32"<<"\t"  
				<<"BqRat1"<<"\t" 
				<<"BqRat2"<<"\t" 
				<<"BsRat1"<<"\t" 
				<<"BsRat2"<<"\t" 

				<<"BqI2"<<"\t"    // 50 
				<<"BqI3"<<"\t"
				<<"Boff1"<<"\t"
				<<"Boff2"<<"\t"
				
				<<"BmedSprdqI.25"<<"\t"   
				<<"BmedSprdqI.5"<<"\t"   
				<<"BmedSprdqI1"<<"\t"   
				<<"BmedSprdqI2"<<"\t"   
				<<"BmedSprdqI4"<<"\t"   
				<<"BmedSprdqI8"<<"\t"   
				<<"BmedSprdqI16"<<"\t"   // 60

				<<"mI600" <<"\t"
				<<"mI3600" <<"\t"						
				<<"mIIntra" << "\t"

				<<"qI" << "\t"
				<<"fillImb" <<"\t"						
				<<"mret60" << "\t"

				<<"target41" << "\t"			
				<<"linpred1" << "\t"
				<<"logDolVolu" << "\t"	
				<<"logDolPrice" << "\t" // 70

				<<"hiloLag1Open" << "\t" // 71 
				<<"hiloLag2Open" << "\t"
				<<"hiloLag1Rat" << "\t"
				<<"hiloLag2Rat" << "\t"
				<<"hiloQAI2" << "\t"
				<<"hiloLag1" << "\t"
				<<"hiloLag2" << "\t"
				<<"hilo900" << "\t"
				<<"bollinger300" <<  "\t"
				<<"bollinger900" <<  "\t" //  endl;  // 80
				<< "dmret300" << "\t" 
				<< "dmret600" << "\t"
				<<"BOffmedVol.0025"<<"\t" 
				<<"BOffmedVol.005" << "\t"
				<<"minTick" << "\t"
				<<"target1";
			tmTxt << "\t" << "mocSig";
			tmTxt << "\t" << "BmedSprdqIWt1";
			tmTxt << "\t" << "BmedSprdqIWt2";
			tmTxt << "\t" << "BmedSprdqIWt4";
			tmTxt << "\t" << "BmedSprdqIWt8";
			tmTxt << "\t" << "BmedSprdqIWt16";
			tmTxt << "\t" << "bollinger1800";
			tmTxt << "\t" << "bollinger3600";
			tmTxt << "\t" << "vwap300";
			tmTxt << "\t" << "vwap900";
			tmTxt << endl; 
		}
	}

	void write_tmBin_header(ofstream& tmBin, ofstream& tmBinTxt, string model, int idate)
	{
		tmBin.open( get_tmBin_path(model, idate).c_str(), ios::out | ios::binary );
		if( tmBin.is_open() )
		{
			int nrows = 0;
			tmBin.write((char*)&(nrows), sizeof(int));

			int ncols = 107; 
			tmBin.write((char*)&(ncols), sizeof(int));

			static const char* labels;
			labels = "time,logVolu,logPrice,logMedSprd," // 3 
					"mret300,mret300L,mret600L,mret1200L,"  // 7
					"m2400L,m4800L,mretONLag0,mretIntraLag1,"  // 11 
					"vm600,vm3600,"  //13 
					"qIWt,qIMax,hilo,TOBqI2,TOBqI3,TOBoff1,TOBoff2,sprd," // 21
					"tar,bmpred,"   // 23 
					"mI600,mI3600,"   // 25 
					"mIIntra,tar10,"   // 27 
					"tar60,moc,"   // 29 
					"BqRat1,BqRat2,BsRat1,BsRat2," // 33 
	 				"exchange,northRST,northTRD,northBP,"  // 37
					"hiloQAI,medVolat,prevDayVolu,mretOpen,"  //41
					"qIMax2,mretONLag1,mretIntraLag2,vmIntra," //45 
					"ompred,isSecTypeF," // 47
					"intraTar,tar1060Intra,sprdOpen," // 50
					"BqI2,BqI3,Boff1,Boff2,BOffmedVol.01," // 55 
					"fillImb,mret60,BOffmedVol.0025,BOffmedVol.005,tar1060Intra2,"  //60
					"BOffmedVol.02,BOffmedVol.04,BOffmedVol.08,BOffmedVol.16,BOffmedVol.32," // 65
					"BmedSprdqI.25,BmedSprdqI.5,BmedSprdqI1,BmedSprdqI2,BmedSprdqI4,"  //70
					"BmedSprdqI8,BmedSprdqI16," //72
					"omlinpred,totaltar,minTick,logDolVolu,logDolPrice,"// //77
					"hiloLag1Open,hiloLag2Open,hiloLag1Rat,hiloLag2Rat,hiloQAI2,"// //82
					"hiloLag1,hiloLag2,hilo900,bollinger300,bollinger900,"// 87
					"dmret300,dmret600,"// 89
					"isENews,isNews,newsIdx,news6,news7," //94
					"BmedSprdqIWt1,BmedSprdqIWt2,BmedSprdqIWt4,BmedSprdqIWt8,BmedSprdqIWt16," //99
					"dark1,dark5,dark20,vwap900,vwap60,timeFrac,g11"; // 106 // nffactors
			long long int labels_len = strlen(labels) + 1; // 8 bytes on 32 and 64 bit OS for MSDN only; does not include terminating zero so add 1 
			tmBin.write((char*)&(labels_len), sizeof(labels_len));
			tmBin.write(labels,labels_len);
		}

		tmBinTxt.open( get_tmBinTxt_path(model, idate).c_str(), ios::out );
		if( tmBinTxt.is_open() )
		{
			tmBinTxt.precision(6);
			tmBinTxt << "uid" << "\t" << "time" << endl;
		}
	}

	// ---------------------------------------------------------------
	// Content for each ticker.
	// ---------------------------------------------------------------

	void write_bmTxt(ofstream& bmTxt, SigC& sig, string uid, string ticker)
	{
		const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
		for(int j = 0; j < linMod.gpts; j++)
		{
			if( sig.sI[j].valid  && ((j * linMod.gridInterval) % linMod.om_txt_sample_freq) == 0) 
			{
				int time = j * linMod.gridInterval;
				int NH = linMod.nHorizon;

				bmTxt << uid << "\t"
						<< base_name(ticker) << "\t" 
						<< time;

				for( int i = 0; i < NH; ++i )
					bmTxt << "\t" << sig.sI[j].pred[i];
				for( int i = 0; i < NH; ++i )
					bmTxt << "\t" << sig.sI[j].predErr[i];
				for( int i = 0; i < NH; ++i )
					bmTxt << "\t" << sig.sI[j].target[i];
				bmTxt << endl;
			}
		}
	}

	void write_omTxt(ofstream& omTxt, SigC& sig, string uid, string ticker)
	{
		int exchange = 0;
		const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
		for(int j = 0; j < linMod.gpts; j++)
		{
			if( sig.sI[j].valid  && ((j * linMod.gridInterval) % linMod.om_txt_sample_freq) == 0) 
			{
				int time = j * linMod.gridInterval;

				double target1 = 0.;
				double bmpred = sig.sI[j].bmpred;
				for( int iT = 0; iT < linMod.nHorizon; ++iT )
					target1 += sig.sI[j].target[iT];

				omTxt << uid << "\t"
						<< base_name(ticker) << "\t" 
						<< time << "\t" 
						<< sig.sI[j].sprd << "\t" 
						<< sig.logVolu  << "\t" 
						<< sig.logPrice << "\t";
				omTxt << sig.sI[j].sig1[0] << "\t"
						<< sig.sI[j].sig1[2] << "\t"
						<< sig.sI[j].sig1[10] << "\t"
						<< sig.sI[j].sig1[14] << "\t"
						<< sig.sI[j].sig1[15] << "\t" // 10
						<< sig.sI[j].sig1[8] << "\t"
						<< sig.sI[j].sig1[9] << "\t"
						<< sig.sI[j].sig1[11] << "\t";
				omTxt << sig.sI[j].sigBook[0] << "\t"  // 14
						<< sig.sI[j].sigBook[1] << "\t"
						<< sig.sI[j].sigBook[2] << "\t"
						<< sig.sI[j].sigBook[3] << "\t";   //17
				omTxt << target1 << "\t"
						//<< sig.sI[j].pr1 + sig.sI[j].pr1err << "\t"
						<< bmpred << "\t"
						<< exchange << "\t"   // 20
						<< sig.avgDlyVolat << "\t"
						<< sig.sI[j].relativeHiLo << "\t"  //22
						
						<< sig.sI[j].sigBook[4] << "\t"  // 23
						<< sig.sI[j].sigBook[6] << "\t"
						<< sig.sI[j].sigBook[7] << "\t"
						<< sig.sI[j].sigBook[8] << "\t"
						<< sig.sI[j].sigBook[20] << "\t"
						<< sig.sI[j].sigBook[21] << "\t"
						<< sig.sI[j].sigBook[22] << "\t"
						<< sig.sI[j].sigBook[23] << 	"\t" // endl; //30

						<< sig.sI[j].sig1[1] << "\t"
						<< sig.sI[j].sig1[4] << "\t"
						<< sig.sI[j].sig1[5] << "\t"
						<< sig.sI[j].sig1[6] << "\t" // 34
						<< sig.sI[j].sig1[7] << "\t"
						<< sig.sI[j].sig1[12] << "\t"
						<< sig.sI[j].sig1[13] << "\t" 
						<< 0.	 << "\t" 
						<< 0.	 <<	"\t"

						<< sig.sI[j].mret5	 <<	"\t"  //40
						<< sig.sI[j].mret15	 <<	"\t"
						<< sig.sI[j].mret30	 <<	"\t"
						<< sig.sI[j].mret120	 <<	"\t"
						<< sig.sI[j].mret600	 <<	"\t"
						<< sig.hiloQAI	 <<	"\t"
						<< sig.hiloLag1Open	 <<	"\t"
						<< sig.hiloLag1Rat	 <<	"\t"
						<< sig.sI[j].hiloLag1	 <<	"\t"
						<< sig.sI[j].hilo900	 <<	"\t"
						<< sig.sI[j].bollinger300	 <<	"\t" // 50
						<< sig.sI[j].bollinger900	 <<	"\t"
						<< 0.	 <<	"\t"
						<< 0. 	 <<	"\t"
						<< 0. 	 <<	"\t" //54
						<< 0. 	 <<	"\t"
						<< 0. 	 <<	"\t"
						<< 0. 	 <<	"\t"
						<< 0. 	 <<	 "\t" // endl;
						<< sig.sI[j].pr1err <<   "\n"; 
			}
		}
	}

	void write_omBin(ofstream& omBin, ofstream& omBinTxt, SigC& sig, string uid, string ticker, int& cnt)
	{
		int exchange = 0;
		const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
		for(int j = 0; j < linMod.gpts; j++)
		{
			bool sprdOK = fabs(sig.sI[j].sprd) >= om_tree_min_fit_sprd_ && fabs(sig.sI[j].sprd) < om_tree_max_fit_sprd_;
			if( sig.sI[j].valid && ((j * linMod.gridInterval) % linMod.om_bin_sample_freq) == 0 && sprdOK)
			{
				++cnt;

				float time = j * linMod.gridInterval;

				float bmpred = sig.sI[j].bmpred;
				float target1 = 0.;
				float target1UH = 0.;
				for( int iT = 0; iT < linMod.nHorizon; ++iT )
				{
					target1 += sig.sI[j].target[iT];
					target1UH += sig.sI[j].targetUH[iT];
					//bmpred += sig.sI[j].bmpred[iT];
				}

				omBinTxt << uid << "\t" << time << endl; 
				
				omBin.write((char*)(&(time)), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].sprd)), sizeof(float)); 
				omBin.write((char*)(&(sig.logVolu)), sizeof(float)); 
				omBin.write((char*)(&(sig.logPrice)), sizeof(float)); //3

				omBin.write((char*)(&(sig.sI[j].sig1[0])), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].sig1[2])), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].sig1[10])), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].sig1[14])), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].sig1[15])), sizeof(float)); //8

				//float bmpred = sig.sI[j].pr1 + sig.sI[j].pr1err;
				float restar = target1 - bmpred; 
				omBin.write((char*)(&restar), sizeof(float)); // restar
				omBin.write((char*)(&bmpred), sizeof(float)); // bmpred //10

				float tmp = 0;
				omBin.write((char*)(&tmp), sizeof(float)); 	
				omBin.write((char*)(&tmp), sizeof(float)); 	//12
				omBin.write((char*)(&(sig.sI[j].sigBook[2])), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].sigBook[3])), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].sigBook[4])), sizeof(float)); 					
				omBin.write((char*)(&tmp), sizeof(float)); // 16

				omBin.write((char*)(&(sig.sI[j].sigBook[6])), sizeof(float)); 						

				omBin.write((char*)(&(exchange)), sizeof(float)); // exchange
				omBin.write((char*)(&(sig.avgDlyVolat)), sizeof(float)); // volat
				omBin.write((char*)(&(sig.sI[j].relativeHiLo)), sizeof(float)); //hilo

				omBin.write((char*)(&(sig.sI[j].sigBook[7])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[8])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[14])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[15])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[16])), sizeof(float)); //sprdRat

				omBin.write((char*)(&(sig.sI[j].sigBook[17])), sizeof(float)); //sprdRat			
				omBin.write((char*)(&(sig.sI[j].sigBook[18])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[19])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[20])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[21])), sizeof(float)); //sprdRat			
				omBin.write((char*)(&(sig.sI[j].sigBook[22])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[23])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[24])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[25])), sizeof(float)); //sprdRat
				omBin.write((char*)(&tmp), sizeof(float)); 
				omBin.write((char*)(&tmp), sizeof(float)); 

				omBin.write((char*)(&(sig.sI[j].mret5)), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].mret15)), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].mret30)), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].mret120)), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].mret600)), sizeof(float)); 

				omBin.write((char*)(&(sig.hiloQAI)), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.hiloLag1Open)), sizeof(float)); 
				omBin.write((char*)(&(sig.hiloLag1Rat)), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].hiloLag1)), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].hilo900)), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].bollinger300)), sizeof(float)); 
				omBin.write((char*)(&(sig.sI[j].bollinger900)), sizeof(float)); 

				float dret= 0.;
				omBin.write((char*)(&(dret)), sizeof(float)); 
				omBin.write((char*)(&(dret)), sizeof(float));					
				omBin.write((char*)(&(dret)), sizeof(float));					
				omBin.write((char*)(&(dret)), sizeof(float));					
				omBin.write((char*)(&(dret)), sizeof(float));					
				omBin.write((char*)(&(dret)), sizeof(float));					
				omBin.write((char*)(&(dret)), sizeof(float));					
				omBin.write((char*)(&(sig.sI[j].sigBook[26])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(sig.sI[j].sigBook[27])), sizeof(float)); //sprdRat
				omBin.write((char*)(&(target1UH)), sizeof(float)); //sprdRat

				omBin.write((char*)(&(sig.sI[j].sig10[9])), sizeof(float)); //qI3
				omBin.write((char*)(&(sig.sI[j].sig10[10])), sizeof(float)); //of1
				omBin.write((char*)(&(sig.sI[j].sig10[11])), sizeof(float)); //of2

				omBin.write((char*)(&dret), sizeof(float)); //of2
				omBin.write((char*)(&dret), sizeof(float)); //of2

				omBin.write((char*)(&dret), sizeof(float)); // tar
				omBin.write((char*)(&(sig.sI[j].mI15)), sizeof(float)); // tar
				omBin.write((char*)(&(sig.sI[j].mI30)), sizeof(float)); // tar
				omBin.write((char*)(&(sig.sI[j].mI60)), sizeof(float)); // tar
				omBin.write((char*)(&(sig.sI[j].mI120)), sizeof(float)); // tar
			}
		}
	}

	void write_tmTxt(ofstream& tmTxt, SigC& sig, string uid, string ticker)
	{
		int exchange = 0;
		const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
		const hff::NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();
		const int M = nonLinMod.nHorizon;
		for(int j = 0; j < linMod.gpts; j++)
		{
			bool sprdOK = fabs(sig.sI[j].sprd) >= om_tree_min_fit_sprd_ && fabs(sig.sI[j].sprd) < om_tree_max_fit_sprd_;
			if( sig.sI[j].valid  && ((j * linMod.gridInterval) % linMod.tm_txt_sample_freq) == 0 && sprdOK ) 
			{
				int time = j * linMod.gridInterval;

				double target1 = 0.;
				double bmpred = sig.sI[j].bmpred;
				for( int iT = 0; iT < linMod.nHorizon; ++iT )
				{
					target1 += sig.sI[j].target[iT];
					//bmpred += sig.sI[j].bmpred[iT];
				}

				float psdtarget = sig.sI[j].target[M-2] + .5 * sig.sI[j].target[M-1];
				float psdtargetuh = sig.sI[j].targetUH[M-2] + .5 * sig.sI[j].targetUH[M-1];

				tmTxt	 << uid << "\t"
					<< ticker << "\t" 
					<< time	<< "\t" 
					<< sig.logVolu			<< "\t" 
					<< sig.logPrice			<< "\t"
					<< sig.logMedSprd		<< "\t" 
					<< sig.sI[j].sig1[14]    << "\t"
					<< sig.sI[j].sig10[3]    << "\t"
					<< sig.sI[j].sig10[5]	<< "\t"
					<< sig.sI[j].sig10[6]	<< "\t"
					<< sig.sI[j].mret2400L	<< "\t"
					<< sig.sI[j].mret4800L	<< "\t"
					<< sig.sI[j].sig10[7]    << "\t"
					<< sig.mretIntraLag1  << "\t"
					<< sig.sI[j].voluMom600  << "\t"
					<< sig.sI[j].voluMom3600 << "\t"  
					<< sig.sI[j].sig10[0] << "\t"
					<< sig.sI[j].sig10[1] << "\t"
			//		<< sig.sI[j].relativeHiLo << "\t" // hilo
					<< sig.sI[j].sig10[4] << "\t" // hilo
					<< sig.sI[j].sig10[8] << "\t"
					<< sig.sI[j].sig10[9] << "\t"
					<< sig.sI[j].sig10[10] << "\t"
					<< sig.sI[j].sig10[11] << "\t"
					<< sig.sI[j].sprd      << "\t";

					//float psdtarget = sig.sI[j].target10 + .5 * sig.sI[j].target60;
					tmTxt	<< psdtarget  << "\t";
					//float psdtargetuh = sig.sI[j].target10UH + .5 * sig.sI[j].target60UH;
					tmTxt	<< psdtargetuh  << "\t";
					tmTxt	<< exchange << "\t" 
						<<sig.northRST <<"\t"
						<<sig.northTRD <<"\t"
						<<sig.northBP <<"\t"
						<<sig.hiloQAI <<"\t"
						<<sig.avgDlyVolat <<"\t"
						<<sig.prevDayVolume <<"\t"

						<<sig.sI[j].mretOpen << "\t"
						<<sig.sI[j].quimMax2 << "\t"

						<<sig.mretONLag1 <<"\t"
						<<sig.mretIntraLag2 <<"\t" 
						<<sig.sI[j].intraVoluMom << "\t"
					//	<<sig.sI[j].pr1 + sig.sI[j].pr1err + sig.sI[j].pr1tree << "\t"

						<<sig.isSecTypeF << "\t" 
						<<sig.sprdOpen <<  "\t" //endl;


					<< sig.sI[j].sigBook[8]<< "\t"
					<< sig.sI[j].sigBook[14]<< "\t"
					<< sig.sI[j].sigBook[15]<< "\t"
					<< sig.sI[j].sigBook[16]<< "\t"
					<< sig.sI[j].sigBook[17]<< "\t"
					<< sig.sI[j].sigBook[18]<< "\t"

					<< sig.sI[j].sigBook[0]<< "\t"
					<< sig.sI[j].sigBook[1]<< "\t"
					<< sig.sI[j].sigBook[2]<< "\t"
					<< sig.sI[j].sigBook[3]<< "\t"
					<< sig.sI[j].sigBook[4]<< "\t"
					<< sig.sI[j].sigBook[5]<< "\t"
					<< sig.sI[j].sigBook[6]<< "\t"
					<< sig.sI[j].sigBook[7]<< "\t"

					<< sig.sI[j].sigBook[19]<< "\t"
					<< sig.sI[j].sigBook[20]<< "\t"
					<< sig.sI[j].sigBook[21]<< "\t"
					<< sig.sI[j].sigBook[22]<< "\t"
					<< sig.sI[j].sigBook[23]<< "\t"
					<< sig.sI[j].sigBook[24]<< "\t"
					<< sig.sI[j].sigBook[25]<< "\t"
					<< sig.sI[j].mI600 << "\t"						
					<< sig.sI[j].mI3600 << "\t"					
					<< sig.sI[j].mIIntra << "\t"

					<< sig.sI[j].sig1[0] << "\t"	
					<< sig.sI[j].sig1[1] << "\t"						
					<< sig.sI[j].sig1[2] << "\t";

					float fulltar = target1 + psdtarget; 
					//float bmpred = sig.sI[j].pr1 + sig.sI[j].pr1err;
			
			tmTxt	<<  fulltar << "\t"						
					<<  bmpred  << "\t" 

					<< 0. << "\t" 
					<< 0.	<< "\t" 

					<< sig.hiloLag1Open	<< "\t" 
					<< sig.hiloLag2Open	<< "\t" 
					<< sig.hiloLag1Rat	<< "\t" 
					<< sig.hiloLag2Rat	<< "\t" 
					<< sig.hiloQAI2	<< "\t" 

					<< sig.sI[j].hiloLag1 << "\t"
					<< sig.sI[j].hiloLag2 << "\t"
					<< sig.sI[j].hilo900 << "\t"
					<< sig.sI[j].bollinger300 <<  "\t"
					<< sig.sI[j].bollinger900 <<  "\t"

					<< 0.	 <<	"\t"
//						<< -sig.sI[j].mret600 + sig.sI[j].etfret600	 <<	 endl;
					<< 0. <<	"\t"
					<< sig.sI[j].sigBook[26]<< "\t"
					<< sig.sI[j].sigBook[27] << "\t"
					<< 0.	<< "\t" 

					<< target1;

					tmTxt << "\t" << 0.;
					tmTxt << "\t" << sig.sI[j].sigBook[9];
					tmTxt << "\t" << sig.sI[j].sigBook[10];
					tmTxt << "\t" << sig.sI[j].sigBook[11];
					tmTxt << "\t" << sig.sI[j].sigBook[12];
					tmTxt << "\t" << sig.sI[j].sigBook[13];
					tmTxt << "\t" << sig.sI[j].bollinger1800;
					tmTxt << "\t" << sig.sI[j].bollinger3600;
					tmTxt << "\t" << 0.;
					tmTxt << "\t" << 0.;

					tmTxt << endl; 
			}
		}
	}

	void write_tmBin(ofstream& tmBin, ofstream& tmBinTxt, SigC& sig, string uid, string ticker, int& cnt)
	{
		int exchange = 0;
		const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
		const hff::NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();
		const int M = nonLinMod.nHorizon;
		for(int j = 0; j < linMod.gpts; j++)
		{
			bool sprdOK = fabs(sig.sI[j].sprd) >= om_tree_min_fit_sprd_ && fabs(sig.sI[j].sprd) < om_tree_max_fit_sprd_;
			if( sig.sI[j].valid && ((j * linMod.gridInterval) % linMod.tm_bin_sample_freq) == 0 && sprdOK)
			{
				++cnt;

				int time = j * linMod.gridInterval;

				float target1 = 0.;
				float bmpred = sig.sI[j].bmpred;
				for( int iT = 0; iT < linMod.nHorizon; ++iT )
				{
					target1 += sig.sI[j].target[iT];
					//bmpred += sig.sI[j].bmpred[iT];
				}
				float ompred = bmpred;

				float target10 = sig.sI[j].target[M-2];
				float psdtar = sig.sI[j].target[M-2] * 0.5 * sig.sI[j].target[M-1];
				float psdbmpred40 = 0.;
				float psdbmpred10 = 0.5 * sig.sI[j].target[M-1];
				//float ompred = sig.sI[j].pr1 + sig.sI[j].pr1err;
				float tar60Intra = sig.sI[j].target[M-2] + 0.5 * sig.sI[j].target[M-1] + 0.125 * sig.sI[j].target60Intra;
				float tar60Intra2 = sig.sI[j].target[M-2] + 0.5 * sig.sI[j].target[M-1] + 0.0625 * sig.sI[j].target60Intra;
				//float bmpred = sig.sI[j].pr1 + sig.sI[j].pr1err;
				float restar = target1 - bmpred + psdtar;
				float newsI = 0.;

				tmBinTxt << uid << "\t" << time << endl;

				float tmp=0;
				tmBin.write((char*)(&(time)), sizeof(float)); 
				tmBin.write((char*)(&(sig.logVolu)), sizeof(float)); 
				tmBin.write((char*)(&(sig.logPrice)), sizeof(float)); 
				tmBin.write((char*)(&(sig.logMedSprd)), sizeof(float)); 

				tmBin.write((char*)(&(sig.sI[j].sig1[14])), sizeof(float)); //mret300
				tmBin.write((char*)(&(sig.sI[j].sig10[3])), sizeof(float)); //mret300L
				tmBin.write((char*)(&(sig.sI[j].sig10[5])), sizeof(float)); //mret600L
				tmBin.write((char*)(&(sig.sI[j].sig10[6])), sizeof(float)); //mret1200L
				tmBin.write((char*)(&(sig.sI[j].mret2400L)), sizeof(float)); //mret2400L
				tmBin.write((char*)(&(sig.sI[j].mret4800L)), sizeof(float)); //mret4800L
				tmBin.write((char*)(&(sig.sI[j].sig10[7])), sizeof(float)); //mretON
				tmBin.write((char*)(&(sig.mretIntraLag1)), sizeof(float)); //prev day intra day

				tmBin.write((char*)(&(sig.sI[j].voluMom600)), sizeof(float)); //volumom600
				tmBin.write((char*)(&(sig.sI[j].voluMom3600)), sizeof(float)); //volumom3600

				tmBin.write((char*)(&(sig.sI[j].sig10[0])), sizeof(float)); //wtQI
				tmBin.write((char*)(&(sig.sI[j].sig10[1])), sizeof(float)); //maxQI
				tmBin.write((char*)(&(sig.sI[j].sig10[4])), sizeof(float)); //hilo
				tmBin.write((char*)(&(sig.sI[j].sig10[8])), sizeof(float)); //qI2
				tmBin.write((char*)(&(sig.sI[j].sig10[9])), sizeof(float)); //qI3
				tmBin.write((char*)(&(sig.sI[j].sig10[10])), sizeof(float)); //of1
				tmBin.write((char*)(&(sig.sI[j].sig10[11])), sizeof(float)); //of2
				tmBin.write((char*)(&(sig.sI[j].sprd)), sizeof(float)); //sprd moved 

				// 40 min
				tmBin.write((char*)(&(psdtar)), sizeof(float)); // tar
				tmBin.write((char*)(&(psdbmpred40)), sizeof(float)); // bmpred

				// 10 min
				tmBin.write((char*)(&(sig.sI[j].mI600)), sizeof(float)); // tar
				tmBin.write((char*)(&(sig.sI[j].mI3600)), sizeof(float)); // bmpred
				tmBin.write((char*)(&(sig.sI[j].mIIntra)), sizeof(float)); // tar
				tmBin.write((char*)(&(target10)), sizeof(float)); // tar
				tmBin.write((char*)(&(psdbmpred10)), sizeof(float)); // bmpred
				tmBin.write((char*)(&tmp), sizeof(float)); // tar

				tmBin.write((char*)(&(sig.sI[j].sigBook[0])), sizeof(float)); //qutRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[1])), sizeof(float)); // qutRat					
				tmBin.write((char*)(&(sig.sI[j].sigBook[2])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[3])), sizeof(float)); //sprdRat

				// new inputs 
				tmBin.write((char*)(&(exchange)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.northRST)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.northTRD)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.northBP)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.hiloQAI)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.avgDlyVolat)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.prevDayVolume)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].mretOpen)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].quimMax2)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.mretONLag1)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.mretIntraLag2)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].intraVoluMom)), sizeof(float)); //sprdRat

				tmBin.write((char*)(&(ompred)), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.isSecTypeF)), sizeof(float)); //47
				tmBin.write((char*)(&(sig.sI[j].targetClose)), sizeof(float)); //sprdRat

				tmBin.write((char*)(&(tar60Intra)), sizeof(float)); // tarIntra60

				tmBin.write((char*)(&(sig.sprdOpen)), sizeof(float)); 

				tmBin.write((char*)(&(sig.sI[j].sigBook[4])), sizeof(float)); //qutRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[5])), sizeof(float)); // qutRat					
				tmBin.write((char*)(&(sig.sI[j].sigBook[6])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[7])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[8])), sizeof(float)); //sprdRat

				tmBin.write((char*)(&(sig.sI[j].sig1[1])), sizeof(float)); // fillImb
				tmBin.write((char*)(&(sig.sI[j].sig1[2])), sizeof(float)); // mret60
				
				tmBin.write((char*)(&(sig.sI[j].sigBook[26])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[27])), sizeof(float)); //sprdRat
			
				tmBin.write((char*)(&(tar60Intra2)), sizeof(float)); // tarIntra60 // 60
				
				tmBin.write((char*)(&(sig.sI[j].sigBook[14])), sizeof(float)); //qutRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[15])), sizeof(float)); // qutRat					
				tmBin.write((char*)(&(sig.sI[j].sigBook[16])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[17])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[18])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[19])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[20])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[21])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[22])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[23])), sizeof(float)); //sprdRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[24])), sizeof(float)); //qutRat
				tmBin.write((char*)(&(sig.sI[j].sigBook[25])), sizeof(float)); // qutRat					

				tmBin.write((char*)(&bmpred), sizeof(float)); // bmpred
				tmBin.write((char*)(&restar), sizeof(float)); // restar

				tmBin.write((char*)(&tmp), sizeof(float)); //75
				tmBin.write((char*)(&tmp), sizeof(float)); 
				tmBin.write((char*)(&tmp), sizeof(float)); 

				tmBin.write((char*)(&(sig.hiloLag1Open)), sizeof(float)); 
				tmBin.write((char*)(&(sig.hiloLag2Open)), sizeof(float)); 
				tmBin.write((char*)(&(sig.hiloLag1Rat)), sizeof(float)); 
				tmBin.write((char*)(&(sig.hiloLag2Rat)), sizeof(float)); 
				tmBin.write((char*)(&(sig.hiloQAI2)), sizeof(float)); 

				tmBin.write((char*)(&(sig.sI[j].hiloLag1)), sizeof(float)); 
				tmBin.write((char*)(&(sig.sI[j].hiloLag2)), sizeof(float)); 
				tmBin.write((char*)(&(sig.sI[j].hilo900)), sizeof(float)); 
				tmBin.write((char*)(&(sig.sI[j].bollinger300)), sizeof(float)); 
				tmBin.write((char*)(&(sig.sI[j].bollinger900)), sizeof(float)); 
				tmBin.write((char*)(&tmp), sizeof(float));					
				tmBin.write((char*)(&tmp), sizeof(float));		
				tmBin.write((char*)(&tmp), sizeof(float)); 
				tmBin.write((char*)(&tmp), sizeof(float)); 
				tmBin.write((char*)(&tmp), sizeof(float)); 

				for(int w = 6; w < 8; ++w)
					tmBin.write((char*)(&tmp), sizeof(float)); //sprdRat

				tmBin.write((char*)(&(sig.sI[j].sigBook[9])), sizeof(float)); // qutRat					
				tmBin.write((char*)(&(sig.sI[j].sigBook[10])), sizeof(float)); // qutRat					
				tmBin.write((char*)(&(sig.sI[j].sigBook[11])), sizeof(float)); // qutRat					
				tmBin.write((char*)(&(sig.sI[j].sigBook[12])), sizeof(float)); // qutRat					
				tmBin.write((char*)(&(sig.sI[j].sigBook[13])), sizeof(float)); // qutRat					
				tmBin.write((char*)(&(tmp)), sizeof(float)); 
				tmBin.write((char*)(&(tmp)), sizeof(float)); 
				tmBin.write((char*)(&(tmp)), sizeof(float)); 
				tmBin.write((char*)(&(tmp)), sizeof(float)); 
				tmBin.write((char*)(&(tmp)), sizeof(float)); 
				tmBin.write((char*)(&(sig.sI[j].timeFrac)), sizeof(float)); 

				tmBin.write((char*)(&(tmp)), sizeof(float)); 
			}
		}
	}

	void update_ncols(ofstream& ofs, int cnt)
	{
		ofs.seekp(0);
		ofs.write((char*)&(cnt), sizeof(int));
	}
}
