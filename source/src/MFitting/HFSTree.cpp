#include <MFitting/HFSTree.h>
//#include "HFTrees.h"
#include <jl_lib.h>
#include <algorithm>
#include <process.h>

using namespace std;

void CALL_CONVENTION NrcwrapIndexxFloat(long n, float *arrin, long *indx, long flag) {
 // if(flag) assumes indx is uninitialized.
 long i;
        if(flag) {for (i=0;i<n;i++) indx[i]=i;}
 else ; /* there is useful indexing inromation already in indx */
 indexxFloat(indx-1,n,arrin);
}

void CALL_CONVENTION indexxFloat(long *indx, long n, float *arrin) {
	// Arrange indx[] so arrin[indx[j]] j=1,2,..,n is sorted.
        long l,j,ir,indxt,i;
        float q;
 
        l=(n >> 1) + 1;
        ir=n;
        for (;;) {
                if (l > 1)
                        q=arrin[(indxt=indx[--l])];
                else {
                        q=arrin[(indxt=indx[ir])];
                        indx[ir]=indx[1];
                        if (--ir == 1) {
                                indx[1]=indxt;
                                return;
                        }
                }
                i=l;
                j=l << 1;
                while (j <= ir) {
                        if (j < ir && arrin[indx[j]] < arrin[indx[j+1]]) j++;
                        if (q < arrin[indx[j]]) {
                                indx[i]=indx[j];
                                j += (i=j);
                        }
                        else j=ir+1;
                }
                indx[i]=indxt;
        }
}


// Sflags[15] = 1 ==> duplicate fitting data 
void TreeBoost::getData(int udate,int lasttestdate,int nfitdays,string binmodel,string bindir,vector<long> &Sflags)
{
	dI.getDataDirs(binmodel,bindir,"treeBoost",Sflags[0]);
	dI.research=Sflags[13];
	dI.dHmodel=Sflags[11];
	dI.selectDataRows(nSigs,Sflags);

	if(binmodel == "vol")
	{
		// read in the already normalized data 
//		string tfile=bindir+"pool24_msdst_0_578_fitvar.txt";
		string tfile=bindir+"pool24_hl_0_578_fitvar.txt";
		dI.totFitPts=dI.getDataTransposeAscii(dataFit,nSigs,tfile);
//		tfile=bindir+"pool24_msdst_579_888_fitvar.txt";
		tfile=bindir+"pool24_hl_579_888_fitvar.txt";
		dI.totTestPts=dI.getDataTransposeAscii(dataTest,nSigs,tfile);
		dI.selectDataRowsAux(4,Sflags);
		dI.getDataTransposeAscii(dataAux,dI.nAuxCols-1,tfile);
	}
	else if(binmodel.substr(0,3) == "opt")
	{
		dI.selectDataRowsAux(7,Sflags); // include term and delta info as well

	//const int firstDate=20060313;
	//const int lastDate=20070610;
	//const int nDaysFit=90;
	//const int nUniverse=900;
	//const double inpLimit=4.;//restrict inputs to 4 stddev
	//TickerHistory thist;
	//dI.getOLSParams("\\\\smrc-chi-lacie3\\data\\optionSample\\OLS0706\\on\\hybrid\\bil24on",
	//	nUniverse,nDaysFit,firstDate,lastDate,inpLimit,thist);

		dI.getFitDatesOpt(udate,nfitdays);
		dI.getTestDatesOpt(lasttestdate);

// sample every other day
	//vector<int> tmp;
	//tmp.resize( ( dI.fitDates.size()+1 )/2 );
	//for(int z=0;z<tmp.size();z++)
	//	tmp[z]=dI.fitDates[z*2];
	//swap(dI.fitDates,tmp);
	//dI.nFitDays=(int)dI.fitDates.size();

		// get totFitPts
		dI.totFitPts =0;
		dI.getDataTransposeOpt2(dataFit,nSigs,dI.totFitPts,dI.fitDates,dI.s);
		// get fit data 
		dI.getDataTransposeOpt2(dataFit,nSigs,dI.totFitPts,dI.fitDates,dI.s);
		dI.totTestPts=0;
		dI.getDataTransposeOpt2(dataTest,nSigs,dI.totTestPts,dI.testDates,dI.s);
		if(dI.totTestPts>0)
		{
			dI.getDataTransposeOpt2(dataTest,nSigs,dI.totTestPts,dI.testDates,dI.s);
			dI.getDataTransposeOpt2(dataAux,dI.nAuxCols-1,dI.totTestPts,dI.testDates,dI.saux);			
		}
	}
	else
	{
		if(dI.binModel=="ex" || dI.binModel=="tm" )  
			dI.selectDataRowsAux(8,Sflags);
		else if(dI.binModel=="om" ) 
			dI.selectDataRowsAux(5,Sflags);
		else
			dI.selectDataRowsAux(4,Sflags);
		dI.getFitDates(udate,nfitdays);
		dI.getTestDates(lasttestdate);

		if(Sflags[15]==0) // usual case
		{
			dI.totFitPts =dI.getNRowsData(dI.fitDates);
			dI.totTestPts=dI.getNRowsData(dI.testDates);
			dI.getDataTranspose(dataFit,nSigs,dI.totFitPts,dI.fitDates,dI.s);
			if(dI.totTestPts>0)
			{
				dI.getDataTranspose(dataTest,nSigs,dI.totTestPts,dI.testDates,dI.s);
				dI.getDataTranspose(dataAux,dI.nAuxCols-1,dI.totTestPts,dI.testDates,dI.saux);
			}
		}
		else // Europe 1 min 
		{
			dI.totFitPts =dI.getNRowsData(dI.fitDates);
			dI.totTestPts=dI.getNRowsData(dI.testDates);
			dI.getDataTranspose(dataFit,nSigs,dI.totFitPts,dI.fitDates,dI.s);
			if(dI.totTestPts>0)
			{
				dI.getDataTranspose(dataTest,nSigs,dI.totTestPts,dI.testDates,dI.s);
				dI.getDataTranspose(dataAux,dI.nAuxCols-1,dI.totTestPts,dI.testDates,dI.saux);
			}
		}
	}
}

void TreeBoost::getSignalDataByDate(int udate,string binmodel,string bindir,vector<long> &Sflags)
{
	dI.getDataDirs(binmodel,bindir,"treeBoost",Sflags[0]);
	dI.research=Sflags[13];
	dI.dHmodel=Sflags[11];

	dI.selectDataRows(nSigs,Sflags);
	dI.testDates.resize(1);
	dI.testDates[0]=udate;
	if(binmodel.substr(0,3) == "opt")
	{
		dI.selectDataRowsAux(7, Sflags);	
		dI.totTestPts=0;
		dI.getDataTransposeOpt2(dataTest,nSigs,dI.totTestPts,dI.testDates,dI.s);
		if(dI.totTestPts>0)
		{
			dI.getDataTransposeOpt2(dataTest,nSigs,dI.totTestPts,dI.testDates,dI.s);
			dI.getDataTransposeOpt2(dataAux,dI.nAuxCols-1,dI.totTestPts,dI.testDates,dI.saux);	
		}
	}
	else
	{
		if((dI.binModel=="ex" || dI.binModel == "tm") ) 
			dI.selectDataRowsAux(8, Sflags);
		else 
			dI.selectDataRowsAux(5, Sflags);
		
		dI.totTestPts=dI.getNRowsData(dI.testDates);
		if(dI.totTestPts>0)
		{
			dI.getDataTranspose(dataTest,nSigs,dI.totTestPts,dI.testDates,dI.s);
			dI.getDataTranspose(dataAux,dI.nAuxCols-1,dI.totTestPts,dI.testDates,dI.saux);
		}
	}
}

void TreeBoost::deleteData()
{
	dI.freeData(dataFit);
	if(dI.totTestPts>0)
	{
		dI.freeData(dataTest);
		dI.freeData(dataAux);
	}
}

void TreeBoost::deleteSignalDataByDate()
{
	dI.freeData(dataTest);
	dI.freeData(dataAux);
}

void TreeBoost::makeTrees()
{
	trees.len=nTrees;
	trees.wts=(double*)calloc(nTrees,sizeof(double));
	trees.elem=(TNODE **)calloc(nTrees,sizeof(TNODE *));
}

void TreeBoost::calcResiduals(int i)
{
	TreeBoostResidual(&dataFit,&trees,i-1,i-1);
}

void TreeBoost::fitTree(long monitor,int i)
{
    int maxnodes=2*maxNodes-1;
	trees.elem[i]=TreeModelTsFitFast2(minPtsNode,&dataFit,maxLevels,maxnodes,monitor,nThreads,catTar);
	trees.wts[i]=shrinkage;
}	

void TreeBoost::deleteTrees()
{
	free(trees.wts);
	for(int i=0;i<trees.len;i++) TnodeKill(trees.elem[i]);
	free(trees.elem);
}

void TreeBoost::calcStats()
{
	// Do some stats on the tree (assumes you have an array of trees "tree" (could be of length 1) )
	char buf[100]; 	
	MATRIX *stats;
 	stats=MakeMatrix(nTrees,dataFit.rows-1);  

	for(int i=0;i<stats->cols;i++)
		stats->collabel[i]=MakeString(dataFit.rowlabel[i]);

    for(int i=0;i<stats->rows-1;i++) {
     sprintf(buf,"tree.%d",i);
     stats->rowlabel[i]=MakeString(buf);
    }
	stats->rowlabel[stats->rows-1]=MakeString("treeMu");

    for(int i=0;i<nTrees;i++)
		TnodeDevianceImprovement(trees.elem[i],stats->elem[i]);
	TnodeDevianceNormalize(stats);

	sprintf(buf,"%sstats%d%s.txt",statsDir.c_str(),dI.uDate,dI.market.c_str());
	MccDumpMatrixNew2(stats,buf,"",6,"w");
	KillMatrix(stats);	
	return;
}	

void TreeBoost::writeFile()
{
	char buf[200]; 	
	sprintf(buf,"%s\\coef%d%s.txt",statsDir.c_str(),dI.uDate,dI.market.c_str());
	for(int i=0;i<nTrees;i++)
		TnodeDumpFname2(trees.elem[i],buf,trees.wts[i],i);
}

void TreeBoost::readFile()
{
	char tbuf[500]; 
	sprintf(tbuf,"%s\\coef%d%s.txt",statsDir.c_str(),dI.uDate,dI.market.c_str());
	trees = *TreeBoostReadFname(tbuf);  // TREEBOOST is a very small structure so just copy it 
}

void TreeBoost::writeDB(vector<long> &Sflags)
{
	for(int i=0;i<nTrees;i++)
		TnodeBoostWriteDB(dI.uDate,Sflags,trees.elem[i],i,trees.wts[i]); // tm tree boost
}

void TreeBoost::readDB(vector<long> &Sflags)
{
	makeTrees();
	for(int i=0;i<nTrees;i++)
		trees.elem[i] = TnodeBoostCreate(Sflags,dI.uDate,dI.market,dI.binModel,i,&(trees.wts[i]));
}

float TreeBoost::makePreds(int observ,vector<double>& vec)
{
	float pred;
	pred=(float)TreeBoostPred(&dataTest,&trees,0,nTrees-1,(long)observ,&(vec[0]),NULL);
	return pred;
}

void TreeBoost::getPreds()
{
	vector<double> vec;
	vec.resize(nSigs);
	for(int i=0;i<dataAux.cols;i++)
		dataAux.elemF[2][i]=makePreds(i,vec);   // always put this in the order tar, bm, tree
}
// the following two functions allow one to use just a subset of the trees 
float TreeBoost::makePreds(int observ,vector<double>& vec,int ntrees)
{
	float pred;
	if(catTar==0)
		pred=(float)TreeBoostPred(&dataTest,&trees,0,ntrees-1,(long)observ,&(vec[0]),NULL);
	else
		pred=(float)TreeBoostPredCat(&dataTest,&trees,0,ntrees-1,(long)observ,&(vec[0]),NULL);
	return pred;
}

float TreeBoost::makePredsCat(int observ,vector<double>& vec,int ntrees)
{
	float pred;
	pred=(float)TreeBoostPredCat(&dataTest,&trees,0,ntrees-1,(long)observ,&(vec[0]),NULL);
	return pred;
}

void TreeBoost::getPreds(int ntrees)
{
	vector<double> vec;
	vec.resize(nSigs);

	if(catTar==0)
		for(int i=0;i<dataAux.cols;i++)
			dataAux.elemF[2][i]=makePreds(i,vec,ntrees);
	else
		for(int i=0;i<dataAux.cols;i++)
			dataAux.elemF[2][i]=makePredsCat(i,vec,ntrees);
}

float TreeBoost::getPreds(int ntrees, vector<double> &vec)  // for doing just one pred using inputs
{
	float pred=TreeBoostPred(NULL,&trees,0,ntrees-1,0,&(vec[0]),NULL);
	return pred;
}

// the folowing two functions allow one to specify the data set
float TreeBoost::makePreds(MATRIXSHORT &data,int observ,vector<double>& vec,int ntrees)
{
	float pred=(float)TreeBoostPred(&data,&trees,0,ntrees-1,(long)observ,&(vec[0]),NULL);
	return pred;
}

void TreeBoost::getPreds(MATRIXSHORT &data, int ntrees, int row) // row refers to row to modify
{
	vector<double> vec;
	vec.resize(nSigs);
	for(int i=0;i<data.cols;i++)
		data.elemF[row][i]=makePreds(data,i,vec,ntrees);
}

void TreeBoost::getModelDates(vector<int> &modelDts)
{
	string allFiles = "coef*" + dI.market + ".txt";
	allFiles=statsDir+allFiles;
	vector<string> fileNames=GetFiles(allFiles);
	modelDts.resize(fileNames.size());
	for(unsigned u=0;u<fileNames.size();++u)
		modelDts[u]=atoi(fileNames[u].substr(4,8).c_str());
}

int TreeBoost::getModelDate(vector<int> &modelDts, int udate) // get greatest model date <= udate
{
	int mdate=0;
	for(int i=0;i<(int)modelDts.size();i++)
		if(udate >= modelDts[i])
			mdate=modelDts[i];
	return mdate;
}

void TreeBoost::writeSigsAndPreds(int date)
{
	char fname[BUFFSIZE];
	sprintf(fname,"%sTarPred%d%s%s.txt",outDir.c_str(),date, dI.market.c_str(), dI.binModel.c_str() );
	fstream fsWrite(fname,ios::out);

	if(dataTest.cols!=dataAux.cols)
		return; 

	for(int j=0;j<dataTest.rows;j++)	
		fsWrite << dataTest.rowlabel[j] << "\t"; 
	for(int j=0;j<dataAux.rows-1;j++)	
		fsWrite << dataAux.rowlabel[j] << "\t";
	fsWrite << dataAux.rowlabel[dataAux.rows-1] << endl;
	for(int i=0; i<dataTest.cols; i++)
	{
		for(int j=0;j<dataTest.rows;j++)	
			fsWrite << dataTest.elemF[j][i] << "\t";  

		for(int j=0;j<dataAux.rows-1;j++)	
			fsWrite << dataAux.elemF[j][i] << "\t";  	
		fsWrite << dataAux.elemF[dataAux.rows-1][i] << endl;	
	}
	return;
}




void statsObject::mEV(float* pred, float* tar, float* cost, int cattar,
		int nrow, statinfo *si)
{
	// modified to look at tradable predictions - those that 
	// beat a cost (in bps) and half sprd
	// there are two cases: cattar = 0 (standard) and cattar = 1 (new)
	
	float sumtarB=0;
	float sumpredB=0;
	int   ntrdsB=0;
	float sumtarS=0;
	float sumpredS=0;
	int   ntrdsS=0;
	
	if(cattar==0)
	{
		for(int i=0;i<nrow;i++)
		{
			if( fabs(pred[i]) < cost[i])
				continue; 
			else if(pred[i] > cost[i])
			{
				sumtarB  += (tar[i] - cost[i]);
				sumpredB += (pred[i] - cost[i]);
				++ntrdsB;
			}
			else if(-pred[i] > cost[i])
			{
				sumtarS  +=  (-tar[i] - cost[i]) ;
				sumpredS +=  (-pred[i] - cost[i]) ;
				++ntrdsS;
			}
		}
	}
//	else
//	{
//		for(int i=0;i<nrow;i++)
//		{
//// buy/do nothing/sell		
//			//if( pred[i] == 1)
//			//	continue; 
//			//else if(pred[i]==2)
//			//{
//			//	sumtar  += (tar[i] - cost[i]);
//			//	++ntrds[0];
//			//}
//			//else if(pred[i]==0)
//			//{
//			//	sumtar  +=  (-tar[i] - cost[i]) ;
//			//	++ntrds[0];
//			//}
//			// binary 
//			if( pred[i] == -1)
//				continue; 
//			// OK for buy tree but not for sell tree!
//			else if(pred[i]==1)
//			{
//				sumtar  += (tar[i] - cost[i]);
//				++ntrds[0];
//			}
//			//else if(pred[i]==0)
//			//{
//			//	sumtar  +=  (-tar[i] - cost[i]) ;
//			//	++ntrds[0];
//			//}
//		}
//	}
//
	si->ev[0]=sumtarB;
	si->of[0]=sumpredB-sumtarB;
	si->ntrds[0]=ntrdsB;

	si->ev[1]=sumtarS;
	si->of[1]=sumpredS-sumtarS;
	si->ntrds[1]=ntrdsS;

	if(si->ntrds[0]>0)
	{
		si->ev[0]/=si->ntrds[0];
		si->of[0]/=si->ntrds[0];
	}
	if(si->ntrds[1]>0)
	{
		si->ev[1]/=si->ntrds[1];
		si->of[1]/=si->ntrds[1];
	}
	return;
}


void statsObject::mEV(float* pred, float* tar, float* cost, int cattar, int nrow, double* ev, double* of, int* ntrds)
{
	// modified to look at tradable predictions - those that 
	// beat a cost (in bps) and half sprd
	// there are two cases: cattar = 0 (standard) and cattar = 1 (new)
	
	float sumtar=0;
	float sumpred=0;
	*ntrds=0;
	
	if(cattar==0)
	{
		for(int i=0;i<nrow;i++)
		{
			if( fabs(pred[i]) < cost[i])
				continue; 
			else if(pred[i] > cost[i])
			{
				sumtar  += (tar[i] - cost[i]);
				sumpred += (pred[i] - cost[i]);
				++ntrds[0];
			}
			else if(-pred[i] > cost[i])
			{
				sumtar  +=  (-tar[i] - cost[i]) ;
				sumpred +=  (-pred[i] - cost[i]) ;
				++ntrds[0];
			}
		}
	}
	else
	{
		for(int i=0;i<nrow;i++)
		{
// buy/do nothing/sell		
			//if( pred[i] == 1)
			//	continue; 
			//else if(pred[i]==2)
			//{
			//	sumtar  += (tar[i] - cost[i]);
			//	++ntrds[0];
			//}
			//else if(pred[i]==0)
			//{
			//	sumtar  +=  (-tar[i] - cost[i]) ;
			//	++ntrds[0];
			//}
			// binary 
			if( pred[i] == -1)
				continue; 
			// OK for buy tree but not for sell tree!
			else if(pred[i]==1)
			{
				sumtar  += (tar[i] - cost[i]);
				++ntrds[0];
			}
			//else if(pred[i]==0)
			//{
			//	sumtar  +=  (-tar[i] - cost[i]) ;
			//	++ntrds[0];
			//}


		}
	}

	ev[0]=sumtar;
	of[0]=sumpred-sumtar;
	if(ntrds[0]>0)
	{
		ev[0]/=ntrds[0];
		of[0]/=ntrds[0];
	}
	return;
}

// JL
void statsObject::mEV(float* predExCost, float* pred, float *tar, float *cost, int nrow, double& ev, double& of, int& ntrds, const int maxNtrade)
{
	// fixed number of trades.

	vector<int> vindx(nrow);
	gsl_heapsort_index<int, float>(&vindx[0], predExCost, nrow);

	float sumtar = 0;
	float sumpred = 0;
	ntrds = 0;
	
	int j = nrow - maxNtrade;
	if( j < 0 )
		j = 0;
	for( ; j < nrow; ++j )
	{
		int i = vindx[j];

		if( fabs(pred[i]) < cost[i])
			continue; 
		else if(pred[i] > cost[i])
		{
			sumtar += (tar[i] - cost[i]);
			sumpred += (pred[i] - cost[i]);
			++ntrds;
		}
		else if(-pred[i] > cost[i])
		{
			sumtar += (-tar[i] - cost[i]) ;
			sumpred += (-pred[i] - cost[i]) ;
			++ntrds;
		}
	}

	ev = sumtar;
	of = sumpred - sumtar;
	if(ntrds > 0)
	{
		ev /= ntrds;
		of /= ntrds;
	}
	return;
}

void statsObject::eV(float* pred, float* tar,int nrow,double* ev,double* of,double* malpred)
{
	vector<long> vIdxO(nrow); 
	// get indices in order from smallest to largest
	NrcwrapIndexxFloat((long)vIdxO.size(),pred,&(vIdxO[0]),1);

	vector<float> vpred(nrow);
	for( int i = 0; i < nrow; ++i )
		vpred[i] = pred[i];

	int binSize = (int)(EV_BIN_QUANT * nrow);
	vector<float> pB;
	vector<float> tB;
	tB.resize(2);
	pB.resize(2);
	if(binSize < nrow && binSize > 0)
	{
		for(int i=0;i<binSize;i++)
		{
			pB[0] += pred[vIdxO[i]];
			pB[1] += pred[vIdxO[vIdxO.size()-1-i]];
			tB[0] += tar[vIdxO[i]];
			tB[1] += tar[vIdxO[vIdxO.size()-1-i]];
		}	
		pB[0]/= binSize;
		pB[1]/= binSize;
		tB[0]/= binSize;
		tB[1]/= binSize;
	}
	ev[0]=.5*(tB[1]-tB[0]);
	of[0]=.5* ( (pB[1]-tB[1])- (pB[0]-tB[0]) );
	malpred[0]=.5*(pB[1]-pB[0]);
	return;
}

void statsObject::calcMean(float** dmat,int nrow,int ncol,double* mean)
{
	for(int i=0;i<nrow;i++)
		for(int j=0;j<ncol;j++)
			mean[j]+=dmat[i][j];

	for(int j=0;j<ncol;j++)
		mean[j]/=nrow;

	return;
}

void statsObject::calcMean(float** dmat,int nrow,vector<int> &cols,double* mean)
{
	int ncol=(int)cols.size();
	for(int i=0;i<nrow;i++)
		for(int j=0;j<ncol;j++)
			mean[j]+=dmat[i][cols[j]];

	for(int j=0;j<ncol;j++)
		mean[j]/=nrow;

	return;
}

void statsObject::calcMeanTrans(dataObject& dataobj)
{
	for(int j=0;j<dataobj.mean.size();j++)
		dataobj.mean[j]=0;

	for(int i=0;i<dataobj.pmat->cols;i++)
		for(int j=0;j<dataobj.mean.size();j++)
			dataobj.mean[j]+=dataobj.pmat->elemF[j][i];

	for(int j=0;j<dataobj.mean.size();j++)
		dataobj.mean[j]/=dataobj.pmat->cols;

	return;
}

void statsObject::calcSecMom(float** dmat,int nrow,int ncol,double** secMom)
{
	for(int i=0;i<nrow;i++)
		for(int j=0;j<ncol;j++)
			for(int k=0;k<=j;k++) // just doing lower left half the matrix
				secMom[j][k]+=dmat[i][j]*dmat[i][k];
		
	for(int j=0;j<ncol;j++)
		for(int k=0;k<=j;k++) 
			secMom[j][k]/=nrow;		
	
	return;
}

void statsObject::calcSecMom(float** dmat,int nrow,vector<int> &cols,double** secMom)
{
	int ncol=(int)cols.size();
	
	for(int i=0;i<nrow;i++)
		for(int j=0;j<ncol;j++)
			for(int k=0;k<=j;k++) // just doing lower left half the matrix
				secMom[j][k]+=dmat[i][cols[j]]*dmat[i][cols[k]];
		
	for(int j=0;j<ncol;j++)
		for(int k=0;k<=j;k++) 
			secMom[j][k]/=nrow;		
	
	return;
}

void statsObject::calcSecMomTrans(dataObject& dataobj)
{
	for(int i=0;i<dataobj.pmat->cols;i++)
		for(int j=0;j<dataobj.pmat->rows;j++)
			for(int k=0;k<=j;k++) // just doing lower left half the matrix
				dataobj.cov[j][k]+=dataobj.pmat->elemF[j][i]*dataobj.pmat->elemF[k][i];

	for(int j=0;j<dataobj.pmat->rows;j++)
		for(int k=0;k<=j;k++) // just doing lower left half the matrix
			dataobj.cov[j][k]/=dataobj.pmat->cols;
	
	return;
}

void statsObject::calcCov(float** dmat,int nrow,int ncol,double** cov)
{
	vector<double> mean;
	mean.resize(ncol);
	
	calcMean(dmat,nrow,ncol,&(mean[0]));
	calcSecMom(dmat,nrow,ncol,cov);
	
	for(int j=0;j<ncol;j++)
		for(int k=0;k<=j;k++) 
			cov[j][k]-=mean[j]*mean[k];

	return;
}

void statsObject::calcCov(float** dmat,int nrow,vector<int> &cols,double** cov)
{
	int ncol=(int)cols.size();
	vector<double> mean;
	mean.resize(ncol);
	
	calcMean(dmat,nrow,cols,&(mean[0]));
	calcSecMom(dmat,nrow,cols,cov);
	
	for(int j=0;j<ncol;j++)
		for(int k=0;k<=j;k++) 
			cov[j][k]-=mean[j]*mean[k];

	return;
}

void statsObject::calcCovTrans(dataObject& dataobj)
{
	calcMeanTrans(dataobj);
	calcSecMomTrans(dataobj);
	
	for(int j=0;j<dataobj.nrowsCov;j++)
		for(int k=0;k<=j;k++) 
			dataobj.cov[j][k]-=dataobj.mean[j]*dataobj.mean[k];

	return;
}

void statsObject::calcCor(float** dmat,int nrow,int ncol,double** cor)
{
	calcCov(dmat,nrow,ncol,cor);

	vector<double> stdev;
	stdev.resize(ncol);	
	
	for(int i=0;i<ncol;i++)
		stdev[i]=sqrt(cor[i][i]);
	
	for(int j=0;j<ncol;j++)
		for(int k=0;k<=j;k++) 
			if(stdev[j]>0 && stdev[k]>0)
				cor[j][k]/=stdev[j]*stdev[k];

	return;
}

// specify which columns to use
void statsObject::calcCor(float** dmat,int nrow,vector<int> &cols,double** cor)
{
	int ncol = (int)cols.size();

	calcCov(dmat,nrow,ncol,cor);

	vector<double> stdev;
	stdev.resize(ncol);	
	
	for(int i=0;i<ncol;i++)
		stdev[i]=sqrt(cor[i][i]);
	
	for(int j=0;j<ncol;j++)
		for(int k=0;k<=j;k++) 
		{
			if(stdev[j]>0 && stdev[k]>0)
				cor[j][k]/=stdev[j]*stdev[k];
			//cout << j << " " << k << " " << cor[j][k] << endl;
		}

	return;
}

void statsObject::calcCorTrans(dataObject& dataObj)
{
	calcCovTrans(dataObj);

	vector<double> stdev;
	stdev.resize(dataObj.nrowsCor);	
	
	for(int i=0;i<dataObj.nrowsCor;i++)
		stdev[i]=sqrt(dataObj.cov[i][i]);
	
	for(int j=0;j<dataObj.nrowsCor;j++)
		for(int k=0;k<=j;k++) 
			if(stdev[j]>0 && stdev[k]>0)
				dataObj.cor[j][k]=dataObj.cov[j][k]/(stdev[j]*stdev[k]);

	return;
}

void statsObject::summary(dataObject &data, double costdollars, int cattar) // assumes matrix is data by row
{
	int ncols=data.mat.cols; // number of variables; order is full target, bm pred , total pred;
	int nrows=data.mat.rows;

	vector<int> cols;
	cols.resize(3);
	cols[0]=0;cols[1]=1;cols[2]=2; // tar, bmpred, pred
	calcCor(data.mat.elemF,nrows,cols,data.cor);

	vector<float> tar;
	vector<float> bmpred;
	vector<float> pred;
	vector<float> cost;
	tar.resize(nrows);
	bmpred.resize(nrows);
	pred.resize(nrows);
	cost.resize(nrows);

	for(int j=0;j<nrows;j++)
	{
		tar[j]=data.mat.elemF[j][0];
		bmpred[j]=data.mat.elemF[j][1];
		pred[j]=data.mat.elemF[j][2];
		cost[j]=.5*data.mat.elemF[j][3];
	}
	if(costdollars !=0 && ncols > 6) // just options right now 
		for(int j=0;j<nrows;j++)
		{
			if(	data.mat.elemF[j][6] > 0)
				cost[j]+=BASIS_PTS * costdollars/data.mat.elemF[j][6]; 	
		}

	// bm preds 
	if(cattar==0)
		eV(&(bmpred[0]),&(tar[0]),nrows,&(data.ev[1]),&(data.of[1]),&(data.malpred[1]));	
	mEV(&(bmpred[0]),&(tar[0]),&(cost[0]),cattar,nrows, &(data.mev[1]), &(data.mbias[1]), &(data.ntrds[1]));			

	// new preds 
	if(cattar==0)
		eV(&(pred[0]),&(tar[0]),nrows,&(data.ev[2]),&(data.of[2]),&(data.malpred[2]));	
	mEV(&(pred[0]),&(tar[0]),&(cost[0]),cattar,nrows, &(data.mev[2]), &(data.mbias[2]), &(data.ntrds[2]));			


	return;
}

void statsObject::summary_jl(dataObject &data, double costdollars, int cattar, int& maxNtrade) // assumes matrix is data by row
{
	int ncols=data.mat.cols; // number of variables; order is full target, bm pred , total pred;
	int nrows=data.mat.rows;

	vector<int> cols;
	cols.resize(3);
	cols[0]=0;cols[1]=1;cols[2]=2; // tar, bmpred, pred
	calcCor(data.mat.elemF,nrows,cols,data.cor);

	vector<float> tar(nrows);
	vector<float> bmpred(nrows);
	vector<float> pred(nrows);
	vector<float> cost(nrows);
	vector<float> predExCost(nrows);

	for(int j=0;j<nrows;j++)
	{
		tar[j]=data.mat.elemF[j][0];
		bmpred[j]=data.mat.elemF[j][1];
		pred[j]=data.mat.elemF[j][2];
		cost[j]=.5*data.mat.elemF[j][3];
		predExCost[j] = (pred[j] > 0) ? (pred[j] - cost[j]) : (-pred[j] - cost[j]);
	}
	if(costdollars !=0 && ncols > 6) // just options right now 
		for(int j=0;j<nrows;j++)
		{
			if(	data.mat.elemF[j][6] > 0)
				cost[j]+=BASIS_PTS * costdollars/data.mat.elemF[j][6]; 	
		}

	// bm preds 
	if(cattar==0)
		eV(&(bmpred[0]),&(tar[0]),nrows,&(data.ev[1]),&(data.of[1]),&(data.malpred[1]));	
	mEV(&(bmpred[0]),&(tar[0]),&(cost[0]),cattar,nrows, &(data.mev[1]), &(data.mbias[1]), &(data.ntrds[1]));			

	// new preds 
	if(cattar==0)
		eV(&(pred[0]),&(tar[0]),nrows,&(data.ev[2]),&(data.of[2]),&(data.malpred[2]));	
	mEV(&(pred[0]),&(tar[0]),&(cost[0]),cattar,nrows, &(data.mev[2]), &(data.mbias[2]), &(data.ntrds[2]));		

	// JL sumMEV of fixed number of trades.
	if( maxNtrade == 0 )
		maxNtrade = (double)data.ntrds[2] * 2.0 / 3.0; // two thirds of nTradable.
	mEV(&predExCost[0], &pred[0], &tar[0], &cost[0], nrows, data.mev[3], data.mbias[3], data.ntrds[3], maxNtrade);

	return;
}

void statsObject::summary2(dataObject &data, double costdollars, int cattar) // assumes matrix is data by row
{
	int ncols=data.mat.cols; // number of variables; order is full target, bm pred , total pred;
	int nrows=data.mat.rows;

	vector<int> cols;
	cols.resize(3);
	cols[0]=0;cols[1]=1;cols[2]=2; // tar, bmpred, pred
	calcCor(data.mat.elemF,nrows,cols,data.cor);

	vector<float> tar;
	vector<float> bmpred;
	vector<float> pred;
	vector<float> cost;
	tar.resize(nrows);
	bmpred.resize(nrows);
	pred.resize(nrows);
	cost.resize(nrows);

	for(int j=0;j<nrows;j++)
	{
		tar[j]=data.mat.elemF[j][0];
		bmpred[j]=data.mat.elemF[j][1];
		pred[j]=data.mat.elemF[j][2];
		cost[j]=.5*data.mat.elemF[j][3];
	}
	if(costdollars !=0 && ncols > 6) // just options right now 
		for(int j=0;j<nrows;j++)
		{
			if(	data.mat.elemF[j][6] > 0)
				cost[j]+=BASIS_PTS * costdollars/data.mat.elemF[j][6]; 	
		}

	// bm preds 
	if(cattar==0)
		eV(&(bmpred[0]),&(tar[0]),nrows,&(data.ev[1]),&(data.of[1]),&(data.malpred[1]));	
	mEV(&(bmpred[0]),&(tar[0]),&(cost[0]),cattar,nrows,&(data.si[1]) );
//	mEV(&(bmpred[0]),&(tar[0]),&(cost[0]),cattar,nrows, &(data.mev[1]), &(data.mbias[1]), &(data.ntrds[1]));			

	// new preds 
	if(cattar==0)
		eV(&(pred[0]),&(tar[0]),nrows,&(data.ev[2]),&(data.of[2]),&(data.malpred[2]));	
//	mEV(&(pred[0]),&(tar[0]),&(cost[0]),cattar,nrows, &(data.mev[2]), &(data.mbias[2]), &(data.ntrds[2]));			
	mEV(&(pred[0]),&(tar[0]),&(cost[0]),cattar,nrows,&(data.si[2]) );

	return;
}









