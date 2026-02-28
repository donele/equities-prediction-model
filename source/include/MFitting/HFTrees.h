#ifndef _HFTREES_H
#define _HFTREES_H
#include <vector>
#include <string>
#include <optionlibs/TickData.h>

#define ERR(expr) if(expr) JMErr(name,#expr)
#define CALLOC(type,size) \
	(type *) MyCalloc(size,sizeof(type),name)
void *MyCalloc(long num, long size, char *callingprog);
void JMErr(char* mes1,char* mes2);
#define BASIS_PTS             10000

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#define CALL_CONVENTION __stdcall
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#define CALL_CONVENTION
#endif

FUNC_DECLSPEC double Maximum(double *x, long n);
FUNC_DECLSPEC double** CALL_CONVENTION dmatcalloc(int nrow, int ncol);
FUNC_DECLSPEC void CALL_CONVENTION dmatfree(double** mat, int nrow);
FUNC_DECLSPEC std::vector<std::string> CALL_CONVENTION GetFiles(const std::string& pattern);
FUNC_DECLSPEC float** CALL_CONVENTION fmatcalloc(int nrow, int ncol);
FUNC_DECLSPEC float CALL_CONVENTION ran0(long *idum);
FUNC_DECLSPEC void CALL_CONVENTION fmatfree(float** mat, int nrow);
FUNC_DECLSPEC void CALL_CONVENTION shortmatfree(short** mat, int nrow);
FUNC_DECLSPEC long** CALL_CONVENTION longmatcalloc(int nrow, int ncol);
FUNC_DECLSPEC void CALL_CONVENTION longmatfree(long** mat, int nrow);


#ifdef WIN32	
#ifdef CLASS_DECLSPEC	
#undef CLASS_DECLSPEC  
#endif
#ifdef FUNC_DECLSPEC
#undef FUNC_DECLSPEC
#endif
#ifdef HFSTOCKPARAMS_EXPORTS
#define CLASS_DECLSPEC   __declspec(dllexport)
#define FUNC_DECLSPEC    __declspec(dllexport)
#else
#define CLASS_DECLSPEC   __declspec(dllimport)
#define FUNC_DECLSPEC    __declspec(dllimport)
#endif
#endif



//#include "HFStockParams.h"
//using namespace std;

//	Enumerates values that "global_filetype" can have.
enum {NOFILE,COMPRESSED,UNCOMPRESSED}; 
static long global_filetype=NOFILE;
#define NOTFOUND -1

#define WL 1000  //WORDLENGTH.. note, could use a typedef, see
#define NA (REAL) 0.0012345
#define CUTPTSMIN 5	//was 50
#define INFINITY 1.0e30
#define PRI(expr,format) \
	fprintf(stderr,"In %15s "#expr" = "format"\n",name,expr)

#define LABELLENGTH 100	//changed 20050224


typedef struct {
	char **elem;
	long len;
	long transpose;	/* used in SvecDump() */
	short *access;	/* used in SvecArg() and SvecArgCheck() */
} SVEC;

//#define N_TAR_CATS 3
#define N_TAR_CATS 2 // for trying out the ada boost 
typedef struct tnode TNODE;  /*self-referential structure for node of a tree*/
struct tnode {
	double mu,tstat;		/*conditional mean,tstat. mean mu is used as the prediction */
	long var;               /*variable to cut on. If var=-1 node is terminal*/
	double cut;             /*go left if var<cut, right otherwise.  */
	long indnum;			/* number of points in node */
	long *ind;				/*private indexes to inputs data used to locate left most data in order. NULL if using fast fitting */
	double y0,y1,y2;        /*private statistics used to compute deviance to cut on*/
	long level;				/*level of the node in the tree. Used in TnodeDumpCond() for old style printing of the tree */
	TNODE *left;			/*left child.  NULL if a terminal node*/
	TNODE *right;			/*right child. NULL if a terminal node*/
	TNODE *parent;			/*parent of the node. Useful in dumping conditions*/
	long dirn;				/*=0 or 1, useful in dumping conds. I don't think I use that anymore */
	double deviance;		/*added 20060118 for pruning */
	long prune;				/*added 20060117 for pruning */
	/////////////////fast fitting algorithm
	long rpartNodeNum;
	/////////////////very fast fitting algorithm
	double yy0,yy1,yy2,diffmax;	
	long leftMax;
	double tmpXold;	//used for resolving ties
	//////////////// catagorical targets

	long   ncat[N_TAR_CATS];  // number of points of type 1, 2, ... N_TAR_CATS in node for catagorical target variables
	long   ncatI[N_TAR_CATS]; // analogous to yy0 for a continuous variable 

	double loss[N_TAR_CATS];  // loss if cat = 0,1,2; deviance = min(cat deviance); trying to minimize loss
	double lossI[N_TAR_CATS];  // loss if cat = 0,1,2; deviance = min(cat deviance); trying to minimize loss

};

typedef struct {	//Used for treeboosting
	long len;		//number of trees
	TNODE **elem;	//coefs of individual trees
	double *wts;	//weights used to combine the trees 
} TREEBOOST;



#define REAL float				//can be double, but need to change TspparrRead()
typedef struct {
  short  **elem;       /* array of pointers to arrays of elements         */
  float **elemF;       /* for floating point inputs and targets: JM 20060412 */ 

  char   **rowlabel;   /* array of pointers to row labels                 */
  long     rows;       /* size of **elem array = number of rows           */
  long     cols;       /* size of each *elem array = number of cols       */
  //////////////////added for fast by RAM intensive recursive partitioning
  long   **rpartSorts;	/* array of pointers to rows-1 sorting indices */
  long    *rpartWhich;	/* which node does the input std::vector belong to */
  //////////////////added for more precision on target
  REAL   *target;

  // added for the catagorical tree with a PNL minimizer 
  std::vector<float> cst;  // in bps 
  std::vector<float> ret;  // in bps 
  std::vector<double> wt;  // wts for adaboost
} MATRIXSHORT;

typedef struct {
  double **elem;       /* array of pointers to arrays of elements         */
  char   **rowlabel;   /* array of pointers to row labels                 */
  char   **collabel;   /* array of pointers to col labels                 */
  char    *name;       /* optional name for matrix                        */
  long     rows;       /* size of **elem array = number of rows           */
  long     cols;       /* size of each *elem array = number of cols       */
  long     period;     /* used for intelligent allocating                 */
  long     transpose;  /* currently used only in DumpMatrix               */
} MATRIX;

struct TreeCutArgs
{	
	TNODE **tree;
	long start;
	long end;
	long cutpts;
	MATRIXSHORT *data; 
	long startRowIdx; 
	long endRowIdx;
	bool finished;
};

#define MAXVECTORLEN 40000000	
#define MINBLOCK 65536

extern "C" {
void MatrixShortSort(MATRIXSHORT *x);  
void MatrixShortSortMT(MATRIXSHORT *x, long nThreads);

void MatrixShortKill(MATRIXSHORT *x);
MATRIXSHORT	*MatrixShortMake(long rows, long cols, long precisionFlag);

void MccIndexArray(long *indx, long n, double *arrin);
void MccIndexArrayShort(long *indx, long n, short *arrin);
void FUNC_DECLSPEC MccIndexArrayFloat(long *indx, long n, REAL *arrin);
void NrcIndexArray(long *indx, long n, double *arrin);
void NrcIndexArrayShort(long *indx, long n, short *arrin); 
void NrcIndexArrayFloat(long *indx, long n, REAL *arrin);
FUNC_DECLSPEC MATRIX *MakeMatrix(long rows, long cols);
MATRIX *MatrixGet(char *fname, long flag);
FUNC_DECLSPEC void KillMatrix(MATRIX *x);
static long MccRecordIsNumeric(char *fname, long recnum);
void MyFree(void *ptr);

char	*MakeString(char *buf);
FILE *MccFileOpen(char *fname, char *rw, long killflag, char *callprog);
void MccFileClose(FILE *f);
void MccErr(char *mes1, char *mes2);
void Warning(char *mes);
long IsExtension(char *name, char *ext);
long IsFile(char *fname);
long MccNumberFields(char *fname, long recnum);
long MccNumberRecords(char *fname, long maxrec);
void MccDumpMatrixNew2(MATRIX *m, char *fname, char *mes, long precisionArg, char *rw);


TNODE	*TnodeMake(long indnum, long rpartFast);
double TnodeCutFn(double y0,double y1,double y2,double yy0,double yy1,
				  double yy2,long i,long indnum,long cutpts,long weirdFactor);
void TnodeDeviance(TNODE *x, double *dev, long *npts);
void TreeBoostStats(double *y, MATRIXSHORT *data, TREEBOOST *x, long start, long end);
void TnodeDevianceNormalize(MATRIX *x);
void TnodeDevianceImprovement(TNODE *x,double *y);

//Functions for reading and writing and killing trees 
TNODE *TnodeReadFname(char *fname);
TNODE *TnodeReadFile(FILE *fp, TNODE *root, long flag);
TNODE *TnodeReadFile2(FILE *fp, TNODE *root, long flag);
FUNC_DECLSPEC void TnodeDumpFname(TNODE *tnode,char **label,char *fname, long format, char *rw);
FUNC_DECLSPEC void TnodeKill(TNODE *x);
void TnodeDumpFile3(TNODE *tnode,FILE *fp, double wt, long treeNum);

void TnodeDumpFile(TNODE *tnode,char **label,FILE *fp);
void TnodeDumpFile2(TNODE *tnode,FILE *fp);
TNODE *TnodeReadFname(char *fname);

//void TnodeWriteDB(int udate, long* Sflags, TNODE *tnode);
//void TnodeWriteDBRe(TNODE *tnode, ODBCConnection &odbcHFWrite, int udate, std::string market, int* count);
//TNODE *TnodeCreate(long* Sflags, int idate, std::string market, std::string model);
TNODE *TnodeCreateRe(const std::vector<std::vector<std::string> > &treeData,  TNODE *root, int *count);

void TnodeBoostWriteDB(int udate, std::vector<long> &Sflags, TNODE *tnode,int treeNum, double treeWt);
void TnodeBoostWriteDBRe(const TNODE *tnode,ODBCConnection &odbcHFWrite,const int &udate,
						 const std::string &market,int &count,const int &treeNum,const double &treeWt,const std::string &table);

TNODE *TnodeBoostCreate(std::vector<long> &Sflags,int tidate,std::string tmarket,std::string model,int treeNum, double* ttreeWt);


//Functions for making predictions on data in various formats, given a tree model tnode
MATRIX *TnodeSignal2(TNODE *tnode, MATRIXSHORT *data, long numPreds);
MATRIX *TnodeSignal3(TNODE *tnode, MATRIXSHORT *data, long cutPts);
void TnodeTraverse3(TNODE *tnode, double *vec, long cutPts, double *pred, double mindev);
double TnodeTraverse(TNODE *tnode, double *vec);
double TnodeSignal0(TNODE *tnode, double *data, long cutPts, double mindev); // new 

//Functions for pruning trees
void TnodePrune(TNODE *x);
long TnodePrune2(TNODE *x, double *devArray, long devLen, long pruneInd, long pruneInd2, long ncalls);
long TnodePruneWrap(TNODE *x);

//Fast fitting
FUNC_DECLSPEC TNODE *TreeModelTsFitFast2(long cutpts, MATRIXSHORT *data, long maxLevels, long maxNodes,
										 long monitor, long nThreads, long catTar);

void MatrixShortSort(MATRIXSHORT *x);
TNODE *TreeModelTsFitFast(long cutpts, MATRIXSHORT *data);
void TreeModelMatrix(MATRIX *m, long cutpts, double fitfrac, long weirdFactor);	  //data in MATRIX format. I don't use this now
//long TnodeCutRecurseFast2(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data, long maxNodes);
long TnodeCutRecurseFast2(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data, 
						  long maxNodes, long catTar);

void TnodeStatsFast2(TNODE **tree, long start, long end, MATRIXSHORT *data);
void TnodeCutFast2(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data);

void TnodeCutFast2RF(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data, int nSigSel, long &rSeedSigSel);
long TnodeCutRecurseFast2RF(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data, long maxNodes, int nSigSel, long &rSeedSigSel);
TNODE *TreeModelTsFitFast2RF(long cutpts, MATRIXSHORT *data, long maxLevels, long maxNodes, long monitor, int nSigSel, long &rSeedSigSel);

// for catagorical targets 
//double TnodeCutFnCat(double y0,long* ncat,double yy0,long* ncatR,
//				  long i,long indnum,long cutpts,long weirdFactor);
void TnodeCutFast2Cat(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data);
void TnodeStatsFast2Cat(TNODE **tree, long start, long end, MATRIXSHORT *data);
void TnodeCutFast2MTCat(void *args);
double TreeBoostPredCat(MATRIXSHORT *y, TREEBOOST *x, long start,long end, long i, double *vec, double *postWts);

double TnodeCutFnGini(double y0,long* ncat,double yy0,long* ncatI,
					  long i,long indnum,long cutpts,long weirdFactor);
double TnodeCutFnLoss(double y0,double* loss,double yy0,double* lossI,
				  long i,long indnum,long cutpts,long weirdFactor);




//For tree boosting
FUNC_DECLSPEC TREEBOOST *TreeBoostMake(long len);
FUNC_DECLSPEC void   TreeBoostKill(TREEBOOST *x);
void   TreeBoostResidual(MATRIXSHORT *y, TREEBOOST *x, long start,long end);
//double TreeBoostPred(MATRIXSHORT *y, TREEBOOST *x, long start,long end, long i, double *vec);
double TreeBoostPred(MATRIXSHORT *y, TREEBOOST *x, long start,long end, long i, double *vec, double *postWts);
//void   TreeBoostPerformance(MATRIX *z, MATRIXSHORT *dataFit, MATRIXSHORT *dataTst, TREEBOOST *y, long start, long end, long iter);
void   TnodeDumpFname2(TNODE *x, char *fname, double wt, long treeNum);
TREEBOOST *TreeBoostReadFname(char *fname);

void TreeBoostPerformance(MATRIX *z, MATRIXSHORT *dataFit, MATRIXSHORT *dataTst, TREEBOOST *y, 
						  long start, long end, long iter, REAL *predsFit, REAL *predsTst);

}

#endif

