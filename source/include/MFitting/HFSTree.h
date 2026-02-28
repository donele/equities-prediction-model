#ifndef __TreeBoost__
#define __TreeBoost__
#include <MFitting/HFTrees.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#define CALL_CONVENTION __stdcall
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#define CALL_CONVENTION
#endif

FUNC_DECLSPEC void CALL_CONVENTION NrcwrapIndexxFloat(long n, float *arrin, long *indx, long flag);
FUNC_DECLSPEC void CALL_CONVENTION indexxFloat(long *indx, long n, float *arrin);
#define EV_BIN_QUANT          .01  // ie top 1%; bottom 1%

class dataObject;

typedef struct
{
	double ev[2];
	double of[2];
	int ntrds[2]; // buys and sells
} statinfo;

class dataHandler
{
	public:

	int research; // research or production flag
	int dHmodel; // should be set to Sflags[11] 
//	int duplicateData; // duplicate the fit data setting the book inputs to zero in half

	std::string market;
	std::string market2;
	int fileCols;
	std::string wModel;
	std::string binModel,binDir;
	std::vector<std::string> colLabels;

	std::vector<int> s;
	std::vector<int> saux; // for additional columns in the test data set
	int nAuxCols;

	int uDate; 
	int nFitDays;
	std::vector<int> fitDates;
	int totFitPts;

	int nTestDays;
	std::vector<int> testDates;
	int lastTestDate;
	int totTestPts;

	int maxFileRows;

	int nNormDays;
	std::vector<int> normDates;
	int totNormPts;

	// for HF options 
	//HfoForecParams<hfoLinFrc<BilinInput41Trd> > olsParams;
	int nUniverse;
	int lastDate;
	int nDaysFit; 
	double inpLimit;
	TickerHistory th;
	int nGroups;

	dataHandler()
	{
		maxFileRows=fileCols=totFitPts=totTestPts=0;
		nFitDays=nTestDays=0;
		uDate=lastTestDate=0;
		fileColCount=0;
		nAuxCols=0;
		wModel=="";
		nUniverse=0;
		research=0;
		dHmodel=0;
		return;
	};

	// initialization
	void getDataDirs(std::string binmodel,std::string bindir,std::string wmodel,int imarket);
	void initMatrixShort(MATRIXSHORT &data);

	// get dates
	void getFitDates(int udate, int nfitdays);
	void getNormDates(int nnormdays);
	void getTestDates(int lasttestdate);
	void getFitDatesOpt(int udate, int nfitdays);
	void getTestDatesOpt(int lasttestdate);
	void getNormDatesOpt(int nnormdays);

	// get sizes
	int getNRowsData(std::vector<int> &dates);
//	int getNRowsDataOpt(std::vector<int> &dates); // for options data 

	// select relevant rows
	void selectDataRows(int nsigs, std::vector<long> &Sflags);
	void selectDataRowsAux(int nsigs,std::vector<long> &Sflags);
//	void selectDataRowsAux(int nsigs);

	// sort data
	void sortDataRows(MATRIXSHORT &data, long nThreads);
	
	// free data
	void freeData(MATRIXSHORT &data); 

	// read in data
	void getData(MATRIXSHORT &data, int nsigs,int npts,std::vector<int> &dates);            // rows are observations; the usual case except for trees 
	void getDataTranspose(MATRIXSHORT &data,int nsigs,int npts,std::vector<int> &dates,std::vector<int> &ws);

	void getDataTransposeCond(MATRIXSHORT &data,int nsigs,int npts,std::vector<int> &dates,std::vector<int> &ws,
			int conditionRow, float conditionValue);

	void getDataTranspose(MATRIXSHORT &data, bool allocMem, int nsigs, int npts, std::vector<int> &dates, std::vector<int> &ws, int &countRows);
//	void getDataTransposeOpt(MATRIXSHORT &data,int nsigs,int npts,std::vector<int> &dates, std::vector<int> &whichIdx); // for options data
	void getDataTransposeOpt2(MATRIXSHORT &data, int nsigs, int &npts, std::vector<int> &dates, std::vector<int> &whichIdx);
	void getDataTransposeOpt3(MATRIXSHORT &data, int nsigs, int &npts, std::vector<int> &dates, std::vector<int> &whichIdx);

	void getDataOpt(MATRIXSHORT &data, int nsigs,int npts,std::vector<int> &dates);            // rows are observations; the usual case except for trees 
	int getDataAscii(MATRIXSHORT &data, int nsigs, std::string filename);// read from an ascii file 
	int getDataTransposeAscii(MATRIXSHORT &data, int nsigs, std::string filename);// read from an ascii file 

	// subset the data; selects a subset of data with column = colname and value = colvalue
//	getDataSubsetTranspose(MATRIXSHORT &data, MATRIXSHORT &dataSub, std::string subCol, int subColValue);
	void getDataSubsetTranspose(MATRIXSHORT &dataFit, MATRIXSHORT &dataIdx, MATRIXSHORT &dataSub, int dataIdxValue);

	// read a boostrap sample 
	void getDataBootStrap(MATRIXSHORT &data, long seed);	

	// transform the target for catagorical variables 
	void makeCatTarget(MATRIXSHORT &data, int taridx, int sprdidx, int midpriceidx, double thresh);
	void makeBinaryCatTarget(MATRIXSHORT &data, int taridx, int sprdidx, int midpriceidx, double thresh, int buySell);

	// select a subset by sprd 
	void selectBySprd(const MATRIXSHORT &rdata,bool isTranspose,const int sprdRow, const int minSprd, const int maxSprd,MATRIXSHORT &data);
	void selectBySprdNQ(const MATRIXSHORT &rdata,bool isTranspose,const int sprdRow, 
		const int minSprd, const int maxSprd,MATRIXSHORT &data, int nqrow, float nynasd);

	// bin data
	void bin(const MATRIXSHORT &rdata,bool isTranspose, int col,std::vector<double> &bins,std::vector<dataObject> &data);

	// write MATRIXSHORT to txt file
	void writeMatrixShort(const MATRIXSHORT &data, std::string file);
	
	// options 
//	void getOLSParams(std::string olsdir, int nuniverse,int ndaysfit,int firstdate,
//		int lastdate, double inplimit, TickerHistory &thist);

	void getOLSParams(std::string olsdir);
//	void getOLSParams(std::string olsdir, const int firstdate, const int lastdate, const int ndaysfit, 
//							   const int nuniverse, const int ngroups, const int inplimit);
	~dataHandler()
	{
		return;
	}

	private:
		int fileColCount;
};

class statsObject
{
public:

	void calcMean(float** dmat,int nrow,int ncol,double* mean);
	void calcSecMom(float** dmat,int nrow,int ncol,double** cor);
	void calcCov(float** dmat,int nrow,int ncol,double** cor);
	void calcCor(float** dmat,int nrow,int ncol,double** cor);

	void calcMean(float** dmat,int nrow,std::vector<int> &cols,double* mean);
	void calcSecMom(float** dmat,int nrow,std::vector<int> &cols,double** cor);
	void calcCov(float** dmat,int nrow,std::vector<int> &cols,double** cor);
	void calcCor(float** dmat,int nrow,std::vector<int> &cols,double** cor);

	void calcMeanTrans(dataObject& dataobj);
	void calcSecMomTrans(dataObject& dataobj);
	void calcCovTrans(dataObject& dataobj);
	void calcCorTrans(dataObject& dataobj);

	void eV(float* pred, float* tar,int nrow,double* ev, double* of, double* malpred);
	void mEV(float* pred, float* tar, float* cost, int cattar,
		int nrow, double* ev, double* of, int* ntrds);
	void mEV(float* pred, float* tar, float* cost, int cattar,
		int nrow, statinfo *si);
	// JL
	void mEV(float* predExCost, float* pred, float *tar, float *cost, int nrow, double& ev, double& of, int& ntrds, const int maxNtrade);

//	void summary(dataObject &data);
	void summary(dataObject &data, double costdollars, int cattar); // assumes matrix is data by row
	void summary_jl(dataObject &data, double costdollars, int cattar, int& maxNtrade); // assumes matrix is data by row
	void summary2(dataObject &data, double costdollars, int cattar);
};

class dataObject
{
	public:

	dataHandler dH;
	MATRIXSHORT mat;
	MATRIXSHORT* pmat;

	int isTranspose;

	double **cor;
	double **cov;
	int nrowsCor;
	int nrowsCov;

	std::vector<double> ev;
	std::vector<double> of;
	std::vector<double> malpred; // mean absolute large pred
	std::vector<double> mev;     // unnormalized modified ev: target - cst when pred > cst
	std::vector<double> mbias;   // goes with 
	std::vector<int> ntrds;

	std::vector<statinfo> si;
	std::vector<double> mean;

	dataObject()
	{
		mat.cols=0;
		mat.elem=NULL;
		mat.elemF=NULL;
		mat.rowlabel=NULL;
		mat.rows=0;
		mat.rpartSorts=NULL;
		mat.rpartWhich=NULL;
		mat.target=NULL;
		cor=NULL;
		cov=NULL;
		isTranspose=0;
		nrowsCov=0;
		return;
	};   
	// when we are using this to do stats, format of mat is tar, bmpred, pred1,...
	// if there is just one pred other than the bm, nvars = 3
	dataObject(int ncols) 
	{
		mat.cols=ncols;
		mat.elem=NULL;
		mat.elemF=NULL;
		mat.rowlabel=NULL;
		mat.rows=0;
		mat.rpartSorts=NULL;
		mat.rpartWhich=NULL;
		mat.target=NULL;
		cor=dmatcalloc(ncols,ncols);
		nrowsCor=ncols;
		cov=NULL;
		nrowsCov=0;

		ev.resize(ncols);
		of.resize(ncols);
		malpred.resize(ncols);

		mev.resize(ncols + 1);
		mbias.resize(ncols + 1);
		ntrds.resize(ncols + 1);
		si.resize(ncols);	

		return;
	};

	dataObject(MATRIXSHORT* ms, int isTrans) 
	{
		// mat is not used
		mat.cols=0;
		mat.elem=NULL;
		mat.elemF=NULL;
		mat.rowlabel=NULL;
		mat.rows=0;
		mat.rpartSorts=NULL;
		mat.rpartWhich=NULL;
		mat.target=NULL;

		pmat=ms;
		isTranspose=isTrans;
		pmat->rows;
		cor=dmatcalloc(pmat->rows,pmat->rows);
		nrowsCor=pmat->rows;
		cov=dmatcalloc(pmat->rows,pmat->rows);
		nrowsCov=pmat->rows;
		mean.resize(pmat->rows);

		ev.resize(pmat->rows);
		of.resize(pmat->rows);
		malpred.resize(pmat->rows);

		mev.resize(pmat->rows);
		mbias.resize(pmat->rows);
		ntrds.resize(pmat->rows);
		si.resize(pmat->rows);

		return;
	};

	void init(int ncols)
	{
		mat.cols=ncols;
		mat.elem=NULL;
		mat.elemF=NULL;
		mat.rowlabel=NULL;
		mat.rows=0;
		mat.rpartSorts=NULL;
		mat.rpartWhich=NULL;
		mat.target=NULL;
		cor=dmatcalloc(ncols,ncols);
		nrowsCor=ncols;
		cov=dmatcalloc(ncols,ncols);
		nrowsCov=ncols;

		ev.resize(ncols);
		of.resize(ncols);
		malpred.resize(ncols);
		mev.resize(ncols);
		mbias.resize(ncols);
		ntrds.resize(ncols);
		si.resize(ncols);

		return;	
	}

	~dataObject()
	{
		if(mat.cols>0)
			dH.freeData(mat);
		if(cor!=NULL)
		 dmatfree(cor,nrowsCor);
		if(cov!=NULL)
		 dmatfree(cov,nrowsCov);
		return;
	}
};

class TreeBoost
{
	public:
		std::string statsDir,outDir;	
		int nSigs;
		int nTrees;
		float shrinkage;
		int maxNodes; // max terminal nodes
		int minPtsNode;
		int maxLevels;
		int modelNum;  // 0 = bm 40; 1 = bm1; 5 = 40min uh; 6 = 40 min ptb etc

		int nThreads;
		int catTar; 

		TREEBOOST trees;
//		LIMMOD lin;

		dataHandler  dI;
		MATRIXSHORT dataFit;
		MATRIXSHORT dataTest;
		MATRIXSHORT dataAux; // contains target, bm pred, rbf pred, tree pred

		float treeErr;
		float linErr;

		TreeBoost(std::string statsdir,std::string outdir,int nsigs,int ntrees,float shrink,int maxnodes,
				int minptsnode, int maxlevels, int model, int nthreads, int cattar)
		{
			statsDir=statsdir;
			outDir=outdir;	
			modelNum = model;
			nSigs=nsigs;
			nTrees = ntrees;
			shrinkage = shrink;
			trees.elem=NULL;
			trees.len=0;
			trees.wts=NULL;

			maxNodes=maxnodes;
			minPtsNode=minptsnode;
			maxLevels=maxlevels;
			
			nThreads=nthreads;
			catTar=cattar;

			treeErr=0.;
			linErr=0.;
			return;
		}
		
//		void getData(int udate,int lasttestdate,int nfitdays,std::string binmodel,std::string bindir,int imarket);
		void getData(int udate,int lasttestdate,int nfitdays,std::string binmodel,std::string bindir,std::vector<long> &Sflags);
		void deleteData();
		void getSignalDataByDate(int udate,std::string binmodel,std::string bindir,std::vector<long> &Sflags);
		void deleteSignalDataByDate();

		void calcResiduals(int i);
		void fitTree(long monitor,int i);
		void fitLin(int i);

		void getPerformance(MATRIX *z, long start, long end, long iter);

		void getModelDates(std::vector<int>   &modelDts);  // udates on which model was fit 
		int getModelDate(std::vector<int> &modelDts, int udate); // closest FitDate

		void writeFile();
		void readFile();
		void writeDB(std::vector<long> &Sflags);
		void readDB(std::vector<long> &Sflags);

		void getPreds();
		void getPreds(int ntrees);  // if we don't want to use all the trees 
		void getPreds(MATRIXSHORT &data, int ntrees, int row); // if we want to use another data set
		float getPreds(int ntrees, std::vector<double> &vec); // use vec as the inputs rather than a MATRIXSHORT obj
		float makePreds(int observ,std::vector<double>& vec);
		float makePreds(int observ,std::vector<double>& vec,int ntrees);
		float makePreds(MATRIXSHORT &data,int observ,std::vector<double>& vec,int ntrees);
		float makePredsCat(int observ,std::vector<double>& vec,int ntrees);
		void calcStats();

		void makeTrees();
		void deleteTrees();

		void writeSigsAndPreds(int date);

		// destructor 
		~TreeBoost(){return;}
};

#endif