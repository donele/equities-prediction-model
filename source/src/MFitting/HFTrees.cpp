//#include "HFStockParams.h"
#include <MFitting/HFTrees.h>
#include <algorithm>
#include <process.h>
#include <io.h>
#include <jl_lib.h>

using namespace std;

void *MyCalloc(long num, long size, char *callingprog) {
/*
   Alloc's num * size bytes and returns pointer to memory.
   Space is zero'd out.  Checks for failure.
*/
  char *name="MyCalloc()";
  char buf[100];
  void *ptr;
  if (num<=0 || !size)  {
    sprintf(buf, "Call to MyCalloc in %s for <=0 bytes", callingprog);
    JMErr(name,buf);
  }
  ptr = (void *) calloc((int) num, (int) size);
  if (ptr == (void *) NULL) {
    sprintf(buf, "Calloc failure for %ld x %ld bytes in %s", num, size, callingprog);
    JMErr(name,buf);
  }
/* Commented out by MCC, 20Jan95
  if (a != (ALLOC *) NULL) AddPointer(ptr);
*/
  return ptr;
}

void JMErr(char* mes1,char* mes2)
{
	
/*	char in[50];
	fprintf(stderr,"ERROR: In %s, %s\n",mes1,mes2);
	fprintf(stderr,"  EXIT    \n\a");
	fprintf(stderr,"press any key to continue\n"); gets(in); //added for Splus
		exit(-1); */
	return;
}

double Maximum(double *x, long n) {
/*
   Finds the maximum value in an array of data.
*/
  double max;
  long i;
  max = x[0];
  for (i = 1;  i < n;  i++)
    if (x[i] > max) max = x[i];
  return max;
}

double** CALL_CONVENTION dmatcalloc(int nrow, int ncol)
{
	int i,j;
	double **mat;
	mat = (double **)calloc(nrow, sizeof(double*));   
	for(i=0;i<nrow;i++)
		mat[i] = (double *)calloc(ncol, sizeof(double));
	for(i=0;i<nrow;i++)
	{
		for(j=0;j<ncol;j++)	mat[i][j]=0.;
	}
	return mat;
}

void CALL_CONVENTION dmatfree(double** mat, int nrow)
{
	for(int i=0;i<nrow;i++)
		free(mat[i]);
	free(mat);
	return;
}

vector<string> CALL_CONVENTION GetFiles(const string& pattern)
{ 
	vector<string> retval;
    struct _finddata_t fileinfo;
	int fileFindHandle=_findfirst(pattern.c_str(),&fileinfo);
	if(fileFindHandle!=-1)
	{     
		do
		{     
			retval.push_back(fileinfo.name);
		}while(_findnext(fileFindHandle,&fileinfo)!=-1);

    }
     else
		ERROR_HANDLER("no files found for pattern: "+pattern,EL_WARNING);

      _findclose(fileFindHandle);
      return retval;
}

float** CALL_CONVENTION fmatcalloc(int nrow, int ncol)
{
	int i,j;
	float **mat;
	mat = (float **)calloc(nrow, sizeof(float*));   
	for(i=0;i<nrow;i++)
		mat[i] = (float *)calloc(ncol, sizeof(float));
	for(i=0;i<nrow;i++)
	{
		for(j=0;j<ncol;j++)	mat[i][j]=0.;
	}
	return mat;
}

#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define MASK 123459876
float CALL_CONVENTION ran0(long *idum)
{
	long k;
	float ans;

	*idum ^= MASK;
	k=(*idum)/IQ;
	*idum=IA*(*idum-k*IQ)-IR*k;
	if (*idum < 0) *idum += IM;
	ans=AM*(*idum);
	*idum ^= MASK;
	return ans;
}

void CALL_CONVENTION fmatfree(float** mat, int nrow)
{
	for(int i=0;i<nrow;i++)
		free(mat[i]);
	free(mat);
	return;
}

void CALL_CONVENTION shortmatfree(short** mat, int nrow)
{
	for(int i=0;i<nrow;i++)
		free(mat[i]);
	free(mat);
	return;
}

void CALL_CONVENTION longmatfree(long** mat, int nrow)
{
	for(int i=0;i<nrow;i++)
		free(mat[i]);
	free(mat);
	return;
}
//////////// MT code /////////////////////////

void MatrixShortSortMTcore(void *args) 
{
	char *name="MatrixShortSortMTcore()";
	TreeCutArgs *tcargs = (TreeCutArgs*) args;
	for(int i=tcargs->startRowIdx;i<tcargs->endRowIdx;i++) {
//PRI(i,"%d");
		tcargs->data->rpartSorts[i]=CALLOC(long,tcargs->data->cols);


		//for(int j=0;j<tcargs->data->cols;j++) tcargs->data->rpartSorts[i][j]=j;	//initialize for indexing data	
		//if(tcargs->data->elem!=NULL) 
		//	 MccIndexArrayShort(tcargs->data->rpartSorts[i],tcargs->data->cols,tcargs->data->elem[i]); //compute ind so data->elem[cols][ind[...]] is sorted in ascending order
		//else MccIndexArrayFloat(tcargs->data->rpartSorts[i],tcargs->data->cols,tcargs->data->elemF[i]);


		// JL
		if(tcargs->data->elem!=NULL)
			gsl_heapsort_index<long, short>(tcargs->data->rpartSorts[i], tcargs->data->elem[i], tcargs->data->cols);
		else
			gsl_heapsort_index<long, float>(tcargs->data->rpartSorts[i], tcargs->data->elemF[i], tcargs->data->cols);

	}
	tcargs->finished=1;
	return;
}
// MC 20081120
long tcargsFinished(TreeCutArgs *tcargs, long nThreads) {
	long i,finished=1;
	for(i=0; i<nThreads; i++) finished *= tcargs[i].finished;
	return(finished);
}
//#define THREADS_MAX 48
#define THREADS_MAX 100

void MatrixShortSortMT(MATRIXSHORT *x, long nThreads) {

/*
	If we have a lot of RAM there is an advantage in tree fitting algorithms to pre-computing sorting indices on the fitting data x.
	Calculate x->rpartSorts for first x->rows-1 (the inputs) of x for fast indexing used by tree fitting routines
	Create    x->rpartWhich working space for indentifying which node of a tree the input vector x->elem[*][i] falls into

*/
	// MC's generalized nThread code
	char *name="MatrixShortSortMT()";
	long i,nInputsPerThread;

	ERR(x->rpartSorts!=NULL);	//This could happen if we called this function twice with same data x. Don't want to do that.
	x->rpartSorts = (long **) MyCalloc(x->rows-1, sizeof(long *), name);

//Following computes nInputsPerThread, and recompute nThreads so same nInputs/Thread except perhaps the last thread.
//This would seem to save on nThreads used without sacrificing speed.
	if(nThreads<2) nThreads=2;
	if(nThreads>THREADS_MAX) nThreads=THREADS_MAX;
PRI(nThreads,"%d");
	nInputsPerThread=(long)ceil((float)(x->rows-1)/(float)nThreads);
PRI(nInputsPerThread,"%d");
	nThreads = (long)ceil((float)(x->rows-1)/(float)nInputsPerThread);
PRI(nThreads,"%d");
//Following uses arrays and for loops instead of Jeff's individually declared objects, for generality and simplicity
	TreeCutArgs tcargs[THREADS_MAX];	 
	for(i=0;i<nThreads;i++) {
		tcargs[i].data=x;
		tcargs[i].finished=0;
		tcargs[i].startRowIdx=min((i+0)*nInputsPerThread,x->rows-1);
		tcargs[i].endRowIdx  =min((i+1)*nInputsPerThread,x->rows-1);
	}
PRI("start","%s");	//To monitor how long the multithreading for MatrixShortSort is taking
	for(i=0;i<nThreads;i++) {
		_beginthread(MatrixShortSortMTcore,0,(void*)&(tcargs[i]));
	}
	while( tcargsFinished(tcargs,nThreads)==0 ) Sleep(500);
PRI("end","%s");

	ERR(x->rpartWhich!=NULL);
	x->rpartWhich = CALLOC(long,x->cols);	
	
	
//	char *name="MatrixShortSort()";
//	ERR(x->rpartSorts!=NULL);	//This could happen if we called this function twice with same data x. Don't want to do that.
//	x->rpartSorts = (long **) MyCalloc(x->rows-1, sizeof(long *), name);

	//if(nThreads<2)
	//	nThreads=2;
	//if(nThreads>4)
	//	nThreads=4;

	//long s0=0,e0=0,s1=0,e1=0,s2=0,e2=0,s3=0,e3=0; // start and end indicies for loop over rows
	//float eqsize=float(x->rows-1)/nThreads;
	//s0=0;
	//e0=ROUND(eqsize);
	//s1=e0;
	//e1=min(x->rows-1,ROUND(2*eqsize)); 
	//s2=e1;
	//e2=min(x->rows-1,ROUND(3*eqsize));
	//s3=e2;
	//e3=x->rows-1;
	
	//TreeCutArgs tcargs0, tcargs1, tcargs2,tcargs3;
	//tcargs0.data=tcargs1.data=tcargs2.data=tcargs3.data=x;
	//tcargs0.finished=tcargs1.finished=tcargs2.finished=tcargs3.finished=0;
	//tcargs0.startRowIdx=s0;
	//tcargs1.startRowIdx=s1;
	//tcargs2.startRowIdx=s2;
	//tcargs3.startRowIdx=s3;
	//tcargs0.endRowIdx=e0;
	//tcargs1.endRowIdx=e1;
	//tcargs2.endRowIdx=e2;
	//tcargs3.endRowIdx=e3;

//  non multiThreaded version for debugging
	//MatrixShortSortMTcore(&tcargs0);	
	//MatrixShortSortMTcore(&tcargs1);	
	//MatrixShortSortMTcore(&tcargs2);
	//MatrixShortSortMTcore(&tcargs3);

	//_beginthread(MatrixShortSortMTcore,0,(void*)&tcargs0);
	//_beginthread(MatrixShortSortMTcore,0,(void*)&tcargs1);
	//_beginthread(MatrixShortSortMTcore,0,(void*)&tcargs2);
	//_beginthread(MatrixShortSortMTcore,0,(void*)&tcargs3);

	//while(!(tcargs0.finished && tcargs1.finished && tcargs2.finished && tcargs3.finished))
	//	Sleep(500); 

	//ERR(x->rpartWhich!=NULL);
	//x->rpartWhich = CALLOC(long,x->cols);
}

void TnodeCutFast2TiesMT(TNODE **tree, long start, MATRIXSHORT *data) 
{
	//////////////because of possible ties z->leftMax (population to the left of the cut) needs to be recomputed  
	//////////////For large data->rows this takes not much CPU compared to the step above
	TNODE* z;
	double tmpX;
	for(int i=0;i<data->cols;i++) {
		long which = data->rpartWhich[i];
		if(which < start) continue;
		z = tree[which];
		if(z->var < 0) ;	//never found a place to cut so z->leftMax=0

		else {
			if(data->elem!=NULL) tmpX = (double) data->elem[z->var][i];
			else                 tmpX = (double) data->elemF[z->var][i];
			if(tmpX < z->cut) z->leftMax++; 
			else ;
		}
	}
	return;
}

void TnodeCutFast2MT(void *args) {
/*
	Parallel version of TnodeCutFast() for much more speed with large number of nodes.
	This is the heart of the fast tree fitting algorithm; modified to allow for || processing. JM
*/
	TreeCutArgs *tcargs = (TreeCutArgs*) args;

	char *name="TnodeCutFast2()";
	TNODE *z;
	double tmpY,tmpX;
	double diff;
	long i,j,k;
	long *index,which;
//Initialize
	for(k=tcargs->start;k<tcargs->end;k++) {
		tcargs->tree[k]->yy0=tcargs->tree[k]->y0;	//intialize moments of the targets of each node. These are used to compute deviance efficiently
		tcargs->tree[k]->yy1=tcargs->tree[k]->y1;
		tcargs->tree[k]->yy2=tcargs->tree[k]->y2;
		tcargs->tree[k]->var=-1; 
		tcargs->tree[k]->cut=-INFINITY;
		tcargs->tree[k]->diffmax=-INFINITY;
	}

	for(j=tcargs->startRowIdx;j<tcargs->endRowIdx;j++) {	
	index = tcargs->data->rpartSorts[j];	//precomputed sorting indices used to access the inputs in order from smallest to largese
		/////////////////////////////////////////////////////////
		for(k=tcargs->start;k<tcargs->end;k++) {	//intialize quantities used to compute deviance efficiently
			tcargs->tree[k]->y0=0.; 
			tcargs->tree[k]->y1=0.; 
			tcargs->tree[k]->y2=0.;
			tcargs->tree[k]->tmpXold=-INFINITY;
		}
		for(i=0;i<tcargs->data->cols;i++) { //loop over input data in ascending order
			which = tcargs->data->rpartWhich[index[i]];
			if(which<tcargs->start) continue;			//tree[which] not a candidate for cutting because it's an old node
			//tmpY is the target corresponding to the input tmpX below
			     if(tcargs->data->target!=NULL) tmpY=(double) tcargs->data->target[index[i]];
			else if(tcargs->data->elem  !=NULL) tmpY=(double) tcargs->data->elem[tcargs->data->rows-1][index[i]]; 
			else						tmpY=(double) tcargs->data->elemF[tcargs->data->rows-1][index[i]]; 
			     if(tcargs->data->elem  !=NULL) tmpX=(double) tcargs->data->elem[j][index[i]];
			else						tmpX=(double) tcargs->data->elemF[j][index[i]];
			z = tcargs->tree[which];
			//If there is a genuine change in the ordered input data, compute TnodeCutFn used to make cutting decision
			//20060327: Added this z->tmpXold stuff to make it compatible with fast=0 and fast=1 techniques above.
			if(i==0|| tmpX > z->tmpXold) diff=TnodeCutFn(z->y0,z->y1,z->y2,z->yy0,z->yy1,z->yy2,(long)z->y0,z->indnum,tcargs->cutpts,1);
			else diff=-INFINITY;		//default
			z->tmpXold=tmpX;
			//update count y0, sum y1 and sumsqures y2 AFTER computing diff (the sum of the deviances of the 2 nodes if variable "j" is cut at < tmpX 
			z->y0+=1.0; 
			z->y1+=tmpY; 
			z->y2+=tmpY*tmpY; 
			if(diff>z->diffmax) { //TnodeCutFn has returned a new optimal value, so store tnode->cut and tnode->var
				//z->leftMax=(long) z->y0 -1 ;	//doesn't work because of ties
				z->diffmax=diff;
				z->cut=z->tmpXold;
				z->var=j;
			}
		}
	}
	tcargs->finished=1;
	return;
}

void cpyTree(TNODE *orig,TNODE *cpy, long catTar)
{
	cpy->leftMax = orig->leftMax;
	cpy->y0 = orig->y0;
	cpy->y1 = orig->y1;
	cpy->y2 = orig->y2;
	if(catTar==1)
	{
// gini
		for(int i=0;i<N_TAR_CATS;i++)
			cpy->ncat[i] = orig->ncat[i];

//		for(int i=0;i<N_TAR_CATS;i++)
//			cpy->loss[i] = orig->loss[i];
	}
	return;
}

void recombineTrees(TreeCutArgs *tcargs, long nThreads, long j) 
//void recombineTrees(TNODE *orig, TNODE *cpy1, TNODE *cpy2,TNODE *cpy3, long catTar)
{
	long i;
	TNODE* z;
	z=tcargs[0].tree[j];
	double diffmax=z->diffmax;
	for(i=1; i<nThreads-1; i++) {
		if(tcargs[i].tree[j]->diffmax>diffmax) {
			z=tcargs[i].tree[j];
			diffmax=tcargs[i].tree[j]->diffmax;
		}
	}
	if(tcargs[nThreads-1].tree[j]->diffmax>diffmax)
		z=tcargs[nThreads-1].tree[j];

	if(z==tcargs[0].tree[j]) return;

	tcargs[0].tree[j]->diffmax = z->diffmax;
	tcargs[0].tree[j]->cut = z->cut;
	tcargs[0].tree[j]->var = z->var;
	tcargs[0].tree[j]->leftMax = z->leftMax;
	tcargs[0].tree[j]->y0 = z->y0;
	tcargs[0].tree[j]->y1 = z->y1;
	tcargs[0].tree[j]->y2 = z->y2;
	
	return;

//	TNODE* z;
//	z=orig;
//	double diffmax=orig->diffmax;
//	if(cpy1->diffmax>diffmax)
//	{
//		z=cpy1;
//		diffmax=cpy1->diffmax;
//	}
//	if(cpy2->diffmax>diffmax)
//	{
//		z=cpy2;
//		diffmax=cpy2->diffmax;
//	}
//	if(cpy3->diffmax>diffmax)
//		z=cpy3;
//
//	if(z==orig)
//		return;
//
//	orig->diffmax = z->diffmax;
//	orig->cut = z->cut;
//	orig->var = z->var;
//	orig->leftMax = z->leftMax;
//	orig->y0 = z->y0;
//	orig->y1 = z->y1;
//	orig->y2 = z->y2;
//	if(catTar==1)
//	{
//		for(int i=0;i<N_TAR_CATS;i++)
////			orig->loss[i] = z->loss[i];
//			orig->ncat[i] = z->ncat[i];
//	}
//	return;
}

long TnodeCutRecurseFast2MT(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data, long maxNodes, long &nThreads, long catTar) {
/*
	Modification of MC tree fitting code to allow for parallel processing. 
*/

	char *name="TnodeCutRecurseFast2()";
	long i,which,newNodes,nInputsPerThread;
	TNODE *tnode;
	double tmpX;

//	TnodeStatsFast2(tree,start,end,data);
	if(catTar!=1)
		TnodeStatsFast2(tree,start,end,data);
	else
		TnodeStatsFast2Cat(tree,start,end,data);

//Compute nInputsPerThread, and recompute nThreads so same nInputs/Thread except perhaps the last thread.
	if(nThreads<2) nThreads=2;
	if(nThreads>THREADS_MAX) nThreads=THREADS_MAX;
	nInputsPerThread=(long)ceil((float)(data->rows-1)/(float)nThreads);
	nThreads = (long)ceil((float)(data->rows-1)/(float)nInputsPerThread);
	TreeCutArgs tcargs[THREADS_MAX];
	for(i=0;i<nThreads;i++) {
		tcargs[i].data=data;
		tcargs[i].finished=0;
		tcargs[i].startRowIdx=min((i+0)*nInputsPerThread,data->rows-1);
		tcargs[i].endRowIdx  =min((i+1)*nInputsPerThread,data->rows-1);
		tcargs[i].cutpts=cutpts;
		tcargs[i].start=start;
		tcargs[i].end=end;
		if(i==0) tcargs[i].tree=tree;
		else {
			tcargs[i].tree=(TNODE**)calloc(maxNodes+1,sizeof(TNODE*));
			for(int j=start;j<end;j++) {
				tcargs[i].tree[j]=TnodeMake(tree[j]->indnum,1);	//I'm surprised this does not lead to a memory leak?
				cpyTree(tree[j],tcargs[i].tree[j],catTar);
			}
		}
	}

//fprintf(stderr,"startMT...");	//The below seems to be the bottleneck.
	for(i=0;i<nThreads;i++) {
		_beginthread(TnodeCutFast2MT,0,(void*)&(tcargs[i]));
	}
	while( tcargsFinished(tcargs,nThreads)==0 ) Sleep(500);
//fprintf(stderr,"end\n"); // The threads finish at different times, because CPU is much less just before this fprintf?

	// Reassemble into one tree 
	for(int j=start;j<end;j++)
		recombineTrees(tcargs,nThreads,j);

	// Deal with ties 
	TnodeCutFast2TiesMT(tree,start,data);

	// Free tree memory
	for(i=1;i<nThreads;i++) {
		for(int j=start;j<end;j++) {
			//TnodeKill(tcargs[i].tree[j]);	
			//above added by MC..... I think JM forgot to do that....Actually, no...recombineTrees uses pointer info.
			//But I'm still surprised there's not a memory leak because only 1/nThreads of the tcargs[i].tree[j]->ind
			//will be used??
			free(tcargs[i].tree[j]);
		}
		free(tcargs[i].tree);
	}
	// END MultiThreading CODE 




	//char *name="TnodeCutRecurseFast2()";
	//long i,which,newNodes;
	//TNODE *tnode;
	//double tmpX;

	// START JM CODE : nThreads = 2, 3 or 4
	// I make three copies of tree even if nThread < 4 because the resulting code 
	// is simple and doesn't cost much CPU 

	//if(catTar!=1)
	//	TnodeStatsFast2(tree,start,end,data);
	//else
	//	TnodeStatsFast2Cat(tree,start,end,data);

	//if(nThreads<2)
	//	nThreads=2;
	//if(nThreads>4)
	//	nThreads=4;

	//TNODE** tree1=NULL;
	//TNODE** tree2=NULL;
	//TNODE** tree3=NULL;
	//tree1 = (TNODE**)calloc(maxNodes+1,sizeof(TNODE*));
	//tree2 = (TNODE**)calloc(maxNodes+1,sizeof(TNODE*));
	//tree3 = (TNODE**)calloc(maxNodes+1,sizeof(TNODE*));
	//for(int j=start;j<end;j++)
	//{
	//	tree1[j]=TnodeMake(tree[j]->indnum,1);
	//	tree2[j]=TnodeMake(tree[j]->indnum,1);
	//	tree3[j]=TnodeMake(tree[j]->indnum,1);
	//	cpyTree(tree[j],tree1[j],catTar);
	//	cpyTree(tree[j],tree2[j],catTar);
	//	cpyTree(tree[j],tree3[j],catTar);
	//}

	//long s0=0,e0=0,s1=0,e1=0,s2=0,e2=0,s3=0,e3=0; // start and end indicies for loop over rows
	//float eqsize=float(data->rows-1)/nThreads;
	//s0=0;
	//e0=ROUND(eqsize);
	//s1=e0;
	//e1=min(data->rows-1,ROUND(2*eqsize)); 
	//s2=e1;
	//e2=min(data->rows-1,ROUND(3*eqsize));
	//s3=e2;
	//e3=data->rows-1;
	//
	//TreeCutArgs tcargs0, tcargs1, tcargs2,tcargs3;
	//tcargs0.cutpts=tcargs1.cutpts=tcargs2.cutpts=tcargs3.cutpts=cutpts;
	//tcargs0.data=tcargs1.data=tcargs2.data=tcargs3.data=data;
	//tcargs0.end=tcargs1.end=tcargs2.end=tcargs3.end=end;
	//tcargs0.finished=tcargs1.finished=tcargs2.finished=tcargs3.finished=0;
	//tcargs0.start=tcargs1.start=tcargs2.start=tcargs3.start=start;
	//tcargs0.tree=tree;
	//tcargs1.tree=tree1;
	//tcargs2.tree=tree2;
	//tcargs3.tree=tree3;

	//tcargs0.startRowIdx=s0;
	//tcargs1.startRowIdx=s1;
	//tcargs2.startRowIdx=s2;
	//tcargs3.startRowIdx=s3;
	//tcargs0.endRowIdx=e0;
	//tcargs1.endRowIdx=e1;
	//tcargs2.endRowIdx=e2;
	//tcargs3.endRowIdx=e3;

	//if(catTar!=1)
	//{
	//	//  non multiThreaded version for debugging
	//	//TnodeCutFast2MT(&tcargs0);	//This is the main CPU bootleneck.
	//	//TnodeCutFast2MT(&tcargs1);	
	//	//TnodeCutFast2MT(&tcargs2);
	//	//TnodeCutFast2MT(&tcargs3);

	//	_beginthread(TnodeCutFast2MT,0,(void*)&tcargs0);
	//	_beginthread(TnodeCutFast2MT,0,(void*)&tcargs1);
	//	_beginthread(TnodeCutFast2MT,0,(void*)&tcargs2);
	//	_beginthread(TnodeCutFast2MT,0,(void*)&tcargs3);

	//	while(!(tcargs0.finished && tcargs1.finished && tcargs2.finished && tcargs3.finished))
	//		Sleep(500); 
	//}
	//else
	//{
	//	//  non multiThreaded version for debugging
	//	//TnodeCutFast2MTCat(&tcargs0);	//This is the main CPU bootleneck.
	//	//TnodeCutFast2MTCat(&tcargs1);	
	//	//TnodeCutFast2MTCat(&tcargs2);
	//	//TnodeCutFast2MTCat(&tcargs3);

	//	_beginthread(TnodeCutFast2MTCat,0,(void*)&tcargs0);
	//	_beginthread(TnodeCutFast2MTCat,0,(void*)&tcargs1);
	//	_beginthread(TnodeCutFast2MTCat,0,(void*)&tcargs2);
	//	_beginthread(TnodeCutFast2MTCat,0,(void*)&tcargs3);

	//	while(!(tcargs0.finished && tcargs1.finished && tcargs2.finished && tcargs3.finished))
	//		Sleep(500); 
	//}

	//// Reassemble into one tree 
	//for(int j=start;j<end;j++)
	//	recombineTrees(tree[j],tree1[j],tree2[j],tree3[j],catTar);

	//// Deal with ties 
	//TnodeCutFast2TiesMT(tree,start,data);

	//// Free tree memory
	//for(int j=start;j<end;j++)
	//{
	//	free(tree1[j]);
	//	free(tree2[j]);
	//	free(tree3[j]);
	//}
	//free(tree1);
	//free(tree2);
	//free(tree3);
	//// END JM CODE 

	newNodes=0;	//create new nodes if their population is big enough.
	for(i=start;i<end;i++) {
		if(tree[i]->leftMax < cutpts || tree[i]->leftMax > tree[i]->indnum - cutpts) { //use indum not y0 because y0 changed in TnodeCutFast2()
			continue;	//terminal node
		}
		else {
			tnode = tree[i];
			if(end+newNodes >= maxNodes) break;	//return(newNodes);	//don't need to create new nodes if we have enough already.
			tnode->left =TnodeMake(tnode->leftMax,1);
			tnode->right=TnodeMake(tnode->indnum - tnode->leftMax,1); //use indum not y0 because y0 changed in TnodeCutFast2()
			tnode->left->rpartNodeNum=end+newNodes;
			tnode->right->rpartNodeNum=tnode->left->rpartNodeNum+1;
			tree[end+newNodes]   = tnode->left;
			tree[end+newNodes+1] = tnode->right;
			tnode->left->level =tnode->level+1;  tnode->left->parent=tnode;  tnode->left->dirn=0;		
			tnode->right->level=tnode->level+1; tnode->right->parent=tnode; tnode->right->dirn=1;
			newNodes+=2;
		}
	}
	//Update data->rPartWhich used to identify which node a data point belongs to.
	for(i=0;i<data->cols;i++) {
		which = data->rpartWhich[i];
		if(tree[which]->left==NULL) continue;	//terminal node
		tnode = tree[which];

		if(data->elem!=NULL) tmpX = (double) data->elem[tnode->var][i];
		else                 tmpX = (double) data->elemF[tnode->var][i];			

		if(tmpX < tnode->cut) data->rpartWhich[i]=tnode->left->rpartNodeNum;
		else                  data->rpartWhich[i]=tnode->right->rpartNodeNum;                              
	}
	return(newNodes);
}

/////////////////// END MT code ////////////////////////////////////

//////////////////////  FUNCTIONS FOR A CATAGORICAL TARGET 


void TnodeStatsFast2Cat(TNODE **tree, long start, long end, MATRIXSHORT *data) {
/*
	Private To TnodeCutRecurseFast2()
	Determines tnode->y0,y1,y2,mu,tstat from data->rpartWhich and tnode->rpartNodeNum
	This is a version of TnodeStatsFast() that computes stats "in parallel"; 
	Modified to deal with catagorical variables; JM
	//
*/
	char *name="TnodeStatsFast2Cat()";
	double tmp; // var;
	long i,which;
	TNODE *tnode;
	for(i=0;i<data->cols;i++) {
		which = data->rpartWhich[i];	//which node does the data belong to?
		if(which< start) continue;		//only compute stats on new trees
		ERR(which >= end);
		tnode = tree[which];
		if(data->target==NULL) {
			if(data->elem!=NULL) tmp=(long) data->elem[data->rows-1][i];
			else                 tmp=(long) data->elemF[data->rows-1][i];
		}
		else                   tmp=(long) data->target[i];
		tnode->y0+=1.0;
//		tnode->y1+=tmp; 
//		tnode->y2+=(tmp*tmp);	//compute 0th, 1st and 2nd moments of the targets for the node

		// start min gini index code
		// general
		//for(int n=0;n<N_TAR_CATS;n++)
		//{
		//	if(tmp==n)
		//	{
		//		tnode->ncat[n]+=1;
		//		break;
		//	}
		//}
		// binary
		if(tmp==-1)
			tnode->ncat[0]+=1;
		else
			tnode->ncat[1]+=1;
		// end  min gini index code

		// start min loss code
		//tnode->loss[0] +=  data->ret[i] + data->cst[i]; // loss = - gain from selling; tnode->loss[1] is always 0
		//tnode->loss[2] += -data->ret[i] + data->cst[i]; // loss = -gain from buying
		// end min loss code 
	}
	for(i=start;i<end;i++) {	//update stats user is interested in (e.g.  tnode->mu are the preds)
		tnode = tree[i];
		if((long)tnode->y0 < 2) {	//This should never happen because I'm not interested in nodes with small population.
			PRI(i,"%d");
			PRI(tnode->y0,"%lf");
			PRI(tnode->indnum,"%d");
			PRI(tree[i]->parent->y0,"%lf");
			PRI(tree[i]->parent->leftMax,"%d");
			ERR(1);
		}
		tnode->indnum = (long) tnode->y0;	//Population of a node is the 0th moment of the target distribution y0
		if((long) tnode->y0 >= 2) {			
	//		tnode->mu=tnode->y1/tnode->y0;

			// start min gini code 
			// general
			//tnode->mu=0;
			//long maxcat=tnode->ncat[0];
			//for(int n=1;n<N_TAR_CATS;n++)
			//{
			//	if(tnode->ncat[n]>maxcat)
			//	{
			//		tnode->mu=n;         // if not gini, then mu is NOT the most common n; it is the n that yields the lowest loss
			//		maxcat=tnode->ncat[n];
			//	}
			//}
			//double gini=0;	
			//for(int n=0;n<N_TAR_CATS;n++)
			//	gini -= tnode->ncat[n]*tnode->ncat[n];
			//gini /= tnode->y0;
			//gini += tnode->y0;
			//tnode->deviance=gini;  //deviance = nPts*giniidx = nPts * sum( p_i * (1 - p_i) ); what we are trying to minimize 
			//tnode->tstat=0.0;
			// binary
			tnode->mu=-1;
			if(tnode->ncat[1]>tnode->ncat[0])
				tnode->mu=1;

			double gini=0;	
			for(int n=0;n<N_TAR_CATS;n++)
				gini -= tnode->ncat[n]*tnode->ncat[n];
			gini /= tnode->y0;
			gini += tnode->y0;
			tnode->deviance=gini;  //deviance = nPts*giniidx = nPts * sum( p_i * (1 - p_i) ); what we are trying to minimize 
			tnode->tstat=0.0;
			// end min gini code 

			//// start min loss code 
			//double minloss = tnode->loss[0];
			//int mincat=0;
			//for(int n=1;n<N_TAR_CATS;n++)
			//{
			//	if(tnode->loss[n] < minloss)
			//	{
			//		tnode->mu=n;         // if not gini, then mu is NOT the most common n; it is the n that yields the lowest loss
			//		minloss = tnode->loss[n];
			//	}
			// }
			//tnode->deviance=minloss;  //deviance = nPts*giniidx = nPts * sum( p_i * (1 - p_i) ); what we are trying to minimize 
			//tnode->tstat=0.0;
			// end min loss code 

//			tnode->deviance=tnode->y2 - tnode->mu*tnode->mu*tnode->y0;	//deviance = nPts*variance is function we are minimizing by cutting.
//			var=tnode->y2-(tnode->y1*tnode->mu);
//			if(var>0.0) tnode->tstat=tnode->y1/sqrt(var);
//			else tnode->tstat=0.0;
		}
		else {
			tnode->mu=0.0;
			tnode->deviance=0.0;
			tnode->tstat=0.0;
		}
	}
}

// for gini
double TnodeCutFnGini(double y0,long* ncat,double yy0,long* ncatI,
				  long i,long indnum,long cutpts,long weirdFactor){

	//if(i<cutpts/4||i>indnum-cutpts/4) return(-INFINITY);	//Dissallow cuts near the edge. TODO: replace cutpts/4 by cutpts??
	if(i< cutpts/weirdFactor || i>indnum-cutpts/weirdFactor ) return(-INFINITY);	//added 20060126

	double left_gini=0,right_gini=0;	
	for(int n=0;n<N_TAR_CATS;n++)
		left_gini -= ncat[n]*ncat[n];
	left_gini /= y0;
	left_gini += y0;

	for(int n=0;n<N_TAR_CATS;n++)
		right_gini -= (ncatI[n]-ncat[n])*(ncatI[n]-ncat[n]);
	right_gini /= (yy0-y0);
	right_gini += (yy0-y0);

	return(-left_gini-right_gini);
}

double TnodeCutFnLoss(double y0,double* loss,double yy0,double* lossI,
				  long i,long indnum,long cutpts,long weirdFactor){

	if(i< cutpts/weirdFactor || i>indnum-cutpts/weirdFactor ) return(-INFINITY);	//added 20060126

	double left_loss=loss[0];
	double right_loss=lossI[0]-loss[0];
	
	for(int n=1;n<N_TAR_CATS;n++)
	{
		if(loss[n]<left_loss)
			left_loss=loss[n];
		if(lossI[n]-loss[n]<right_loss)
			right_loss=lossI[n]-loss[n];
	}
	
	return(-left_loss-right_loss);
	return 0;
}

void TnodeCutFast2MTCat(void *args) {
/*
	Parallel version of TnodeCutFast() for much more speed with large number of nodes.
	This is the heart of the fast tree fitting algorithm; modified to allow for || processing. JM
	Adapted for catagorical variables using the gini index as the quantity to be minimized 
*/
	TreeCutArgs *tcargs = (TreeCutArgs*) args;

	char *name="TnodeCutFast2MTCat()";
	TNODE *z;
	// double tmpY,tmpX;
	double tmpX;
	long tmpY;
	double diff;
	long i,j,k;
	long *index,which;
//Initialize
	for(k=tcargs->start;k<tcargs->end;k++) {
		tcargs->tree[k]->yy0=tcargs->tree[k]->y0;	//intialize moments of the targets of each node. These are used to compute deviance efficiently
		//tcargs->tree[k]->yy1=tcargs->tree[k]->y1;
		//tcargs->tree[k]->yy2=tcargs->tree[k]->y2;

		// start gini
		for(int n=0;n<N_TAR_CATS;n++)
			tcargs->tree[k]->ncatI[n]=tcargs->tree[k]->ncat[n];
		// end gini 

		// start min loss
		//for(int n=0;n<N_TAR_CATS;n++)
		//	tcargs->tree[k]->lossI[n]=tcargs->tree[k]->loss[n];
		// end min loss

		tcargs->tree[k]->var=-1; 
		tcargs->tree[k]->cut=-INFINITY;
		tcargs->tree[k]->diffmax=-INFINITY;
	}

	for(j=tcargs->startRowIdx;j<tcargs->endRowIdx;j++) {	
	index = tcargs->data->rpartSorts[j];	//precomputed sorting indices used to access the inputs in order from smallest to largese
		/////////////////////////////////////////////////////////
		for(k=tcargs->start;k<tcargs->end;k++) {	//intialize quantities used to compute deviance efficiently
			tcargs->tree[k]->y0=0.; 
			//tcargs->tree[k]->y1=0.; 
			//tcargs->tree[k]->y2=0.;

			// start gini
			for(int n=0;n<N_TAR_CATS;n++)
				tcargs->tree[k]->ncat[n]=0;
			// end gini

			// start min loss
			//tcargs->tree[k]->loss[0]=0;		
			//tcargs->tree[k]->loss[2]=0;
			// end min loss

			tcargs->tree[k]->tmpXold=-INFINITY;
		}
		for(i=0;i<tcargs->data->cols;i++) { //loop over input data in ascending order
			which = tcargs->data->rpartWhich[index[i]];
			if(which<tcargs->start) continue;			//tree[which] not a candidate for cutting because it's an old node
			//tmpY is the target corresponding to the input tmpX below

		//	z->y1+=tmpY; 
		//	z->y2+=tmpY*tmpY; 

	//		     if(tcargs->data->target!=NULL) tmpY=(double) tcargs->data->target[index[i]];
	//		else if(tcargs->data->elem  !=NULL) tmpY=(double) tcargs->data->elem[tcargs->data->rows-1][index[i]]; 
		//	else						
				tmpY=(long) tcargs->data->elemF[tcargs->data->rows-1][index[i]]; 
		//	     if(tcargs->data->elem  !=NULL) tmpX=(double) tcargs->data->elem[j][index[i]];
		//	else		
				tmpX=(double) tcargs->data->elemF[j][index[i]];
			z = tcargs->tree[which];
			//If there is a genuine change in the ordered input data, compute TnodeCutFn used to make cutting decision
			//20060327: Added this z->tmpXold stuff to make it compatible with fast=0 and fast=1 techniques above.
	//		if(i==0|| tmpX > z->tmpXold) diff=TnodeCutFn(z->y0,z->y1,z->y2,z->yy0,z->yy1,z->yy2,(long)z->y0,z->indnum,tcargs->cutpts,1);
			
			//start gini	
			if(i==0|| tmpX > z->tmpXold) diff=TnodeCutFnGini(z->y0,z->ncat,z->yy0,z->ncatI,(long)z->y0,z->indnum,tcargs->cutpts,1);
			//end gini

			//start loss
			//if(i==0|| tmpX > z->tmpXold) diff=TnodeCutFnLoss(z->y0,z->loss,z->yy0,z->lossI,(long)z->y0,z->indnum,tcargs->cutpts,1);
			// end loss

			else diff=-INFINITY;		//default
			z->tmpXold=tmpX;
			//update count y0, sum y1 and sumsqures y2 AFTER computing diff (the sum of the deviances of the 2 nodes if variable "j" is cut at < tmpX 
			z->y0+=1.0; 

			// start gini
			for(int n=0;n<N_TAR_CATS;n++)
			{
				if(tmpY==n)
				{
					z->ncat[n]+=1;
					break;
				}
			}
			// end gini 

			//z->loss[0] +=  tcargs->data->ret[index[i]] + tcargs->data->cst[index[i]];
			//z->loss[2] += -tcargs->data->ret[index[i]] + tcargs->data->cst[index[i]];

	//		z->y1+=tmpY; 
	//		z->y2+=tmpY*tmpY; 
			if(diff>z->diffmax) { //TnodeCutFn has returned a new optimal value, so store tnode->cut and tnode->var
				//z->leftMax=(long) z->y0 -1 ;	//doesn't work because of ties
				z->diffmax=diff;
				z->cut=z->tmpXold;
				z->var=j;
			}
		}
	}
	tcargs->finished=1;
	return;
}

void TnodeCutFast2Cat(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data) {
/*
	Parallel version of TnodeCutFast() for much more speed with large number of nodes.
	This is the heart of the fast tree fitting algorithm
*/
	char *name="TnodeCutFast2CAT()";
	TNODE *z;
	double tmpX;
	long tmpY;
	double diff;
	long i,j,k;
	long *index,which;
//Initialize
	for(k=start;k<end;k++) {
		tree[k]->yy0=tree[k]->y0;	//intialize moments of the targets of each node. These are used to compute deviance efficiently
//		tree[k]->yy1=tree[k]->y1;
//		tree[k]->yy2=tree[k]->y2;

		// start min gini
		for(int n=0;n<N_TAR_CATS;n++)
			tree[k]->ncatI[n]=tree[k]->ncat[n];
		// end min gini

		// start min loss
		//for(int n=0;n<N_TAR_CATS;n++)
		//	tree[k]->lossI[n]=tree[k]->loss[n];
		// end min loss

		tree[k]->var=-1; 
		tree[k]->cut=-INFINITY;
		tree[k]->diffmax=-INFINITY;
	}

	for(j=0;j< data->rows-1 ;j++) {		//loop over the inputs
		index = data->rpartSorts[j];	//precomputed sorting indices used to access the inputs in order from smallest to largese
		/////////////////////////////////////////////////////////
		for(k=start;k<end;k++) {	//intialize quantities used to compute deviance efficiently
			tree[k]->y0=0.; 
//			tree[k]->y1=0.; 
//			tree[k]->y2=0.;

			// start min gini
			for(int n=0;n<N_TAR_CATS;n++)
				tree[k]->ncat[n]=0;
			// end min gini

			// start min loss
			//tree[k]->loss[0]=0;		
			//tree[k]->loss[2]=0;
			// end min loss

			tree[k]->tmpXold=-INFINITY;
		}
		for(i=0;i<data->cols;i++) { //loop over input data in ascending order
			which = data->rpartWhich[index[i]];
			if(which<start) continue;			//tree[which] not a candidate for cutting because it's an old node

			tmpY=(long) data->elemF[data->rows-1][index[i]];
			tmpX=(double) data->elemF[j][index[i]];
			z = tree[which];
			//If there is a genuine change in the ordered input data, compute TnodeCutFn used to make cutting decision
			//20060327: Added this z->tmpXold stuff to make it compatible with fast=0 and fast=1 techniques above.
//			if(i==0|| tmpX > z->tmpXold) diff=TnodeCutFn(z->y0,z->y1,z->y2,z->yy0,z->yy1,z->yy2,(long)z->y0,z->indnum,cutpts,1);

			// start gini
			if(i==0|| tmpX > z->tmpXold) 
				diff=TnodeCutFnGini(z->y0,z->ncat,z->yy0,z->ncatI,(long)z->y0,z->indnum,cutpts,1);
			// end gini
			// start loss
			  //if(i==0|| tmpX > z->tmpXold) diff=TnodeCutFnLoss(z->y0,z->loss,z->yy0,z->lossI,(long)z->y0,z->indnum,cutpts,1);
			// end loss
			else diff=-INFINITY;		/*default*/
			z->tmpXold=tmpX;
			//update count y0, sum y1 and sumsqures y2 AFTER computing diff (the sum of the deviances of the 2 nodes if variable "j" is cut at < tmpX 
			z->y0+=1.0; 

			// start gini
			// general 
			//for(int n=0;n<N_TAR_CATS;n++)
			//{
			//	if(tmpY==n)
			//	{
			//		z->ncat[n]+=1;
			//		break;
			//	}
			//}
			// binary
			if(tmpY==-1)
				z->ncat[0]+=1;
			else
				z->ncat[1]+=1;
			// end gini

			// start loss
			//z->loss[0] +=  data->ret[index[i]] + data->cst[index[i]];
			//z->loss[2] += -data->ret[index[i]] + data->cst[index[i]];			
			// end loss


		//	z->y1+=tmpY; 
		//	z->y2+=tmpY*tmpY; 
			if(diff>z->diffmax) { //TnodeCutFn has returned a new optimal value, so store tnode->cut and tnode->var
				//z->leftMax=(long) z->y0 -1 ;	//doesn't work because of ties
				z->diffmax=diff;
				z->cut=z->tmpXold;
				z->var=j;
			}
		}
	}
	//////////////because of possible ties z->leftMax (population to the left of the cut) needs to be recomputed  
	//////////////For large data->rows this takes not much CPU compared to the step above
	for(i=0;i<data->cols;i++) {
		which = data->rpartWhich[i];
		if(which < start) continue;
		z = tree[which];
		if(z->var < 0) ;	//never found a place to cut so z->leftMax=0

		else {
			if(data->elem!=NULL) tmpX = (double) data->elem[z->var][i];
			else                 tmpX = (double) data->elemF[z->var][i];
			if(tmpX < z->cut) z->leftMax++; 
			else ;
		}
	}
}

/////////////////// END FUNCTIONS FOR A CATAGORICAL TARGET //////////////////

TNODE *TreeModelTsFitFast2(long cutpts, MATRIXSHORT *data, long maxLevels, long maxNodes, long monitor, 
						   long nThreads, long catTar) {
/*
	My fastest algorithm for fitting trees to "data"
	Iteratively calls TnodeCutRecurseFast2() down to "maxLevels" of the tree.
	"cutpts" is minimum population of a node below which it is not cut.
	"maxNodes" is used to stop the iteration if we already have that many nodes.
	Since the total number of nodes in a tree is always odd, if maxNodes is even, this code will return a tree with maxNodes+1 nodes.
*/

	char *name="TreeModelTsFitFast2()";
	TNODE *root;
	TNODE **tree;
	long i,start,end,newNodes;

	ERR(data->rows<2||cutpts<CUTPTSMIN);	//the target is assumed to be the last row, so if rows<2 there are no inputs

	root=TnodeMake(data->cols,1);
	root->level=0;
	root->parent=(TNODE *) NULL;
	root->rpartNodeNum=0;
	//for(i=0;i<data->cols;i++) data->rpartWhich[i]=root->rpartNodeNum;		//initialization
	memset((void*)data->rpartWhich, root->rpartNodeNum, data->cols * sizeof(long));

	tree = (TNODE **) MyCalloc(maxNodes+1,sizeof(TNODE *),name);	//pointer to all the nodes of interest. Added 1 since end=maxNodes+1 possible below
	tree[0]=root;

	start=0;
	end=1;

	for(i=1;i<=maxLevels;i++) {		//iterate through the levels of the tree
		if(nThreads>1)
			newNodes=TnodeCutRecurseFast2MT(tree,start,end,cutpts,data,maxNodes,nThreads, catTar);
		else 
			newNodes=TnodeCutRecurseFast2(tree,start,end,cutpts,data,maxNodes,catTar);
		if(monitor==2) fprintf(stderr,"level= %5d newNodes= %5d %5d %5d\n",i,newNodes,start,end);
		start=end;
		end  +=newNodes;
		if(newNodes==0) break;
		//ERR(end > maxNodes);
		if(end>=maxNodes) break;	//added 20060328 since if want to limit node expansion by maxNodes need to break out of this loop!!
	}
	
//PRI(start,"%d");
//PRI(end,"%d");
	if(catTar==0)
		TnodeStatsFast2(tree,start,end,data);	//if end > start, compute stats on any nodes which would have been non-terminal
	else
		TnodeStatsFast2Cat(tree,start,end,data);	//if end > start, compute stats on any nodes which would have been non-terminal

	free(tree);
	return(root);
}
long TnodeCutRecurseFast2(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data, long maxNodes, long catTar) {
/*
	This is a much faster version of TnodeCutRecurseFast() that operates on arrays of nodes at the same "level" in a tree in parallel
	It calculates new nodes in the "tree" numbered from "start" to "end"-1 and returns the number of new nodes it created.
	Is is a wrapper around:
	TnodeStatsFast2() which does some statistics on the data in each node (some of which are used in preparation for the function:)
	TnodeCutFast2() that does the real work of figuring where to make the cuts.
*/
	char *name="TnodeCutRecurseFast2()";
	long i,which,newNodes;
	TNODE *tnode;
	double tmpX;

//	TnodeStatsFast2(tree,start,end,data);
	
	if(catTar==0)
	{
		TnodeStatsFast2(tree,start,end,data);
		TnodeCutFast2(tree,start,end,cutpts,data);	//This is the main CPU bootleneck.
	}
	else
	{
		TnodeStatsFast2Cat(tree,start,end,data);
		TnodeCutFast2Cat(tree,start,end,cutpts,data);	
	}

	newNodes=0;	//create new nodes if their population is big enough.
	for(i=start;i<end;i++) {
		if(tree[i]->leftMax < cutpts || tree[i]->leftMax > tree[i]->indnum - cutpts) { //use indum not y0 because y0 changed in TnodeCutFast2()
			continue;	//terminal node
		}
		else {
			tnode = tree[i];
			if(end+newNodes >= maxNodes) break;	//return(newNodes);	//don't need to create new nodes if we have enough already.
			tnode->left =TnodeMake(tnode->leftMax,1);
			tnode->right=TnodeMake(tnode->indnum - tnode->leftMax,1); //use indum not y0 because y0 changed in TnodeCutFast2()
			tnode->left->rpartNodeNum=end+newNodes;
			tnode->right->rpartNodeNum=tnode->left->rpartNodeNum+1;
			tree[end+newNodes]   = tnode->left;
			tree[end+newNodes+1] = tnode->right;
			tnode->left->level =tnode->level+1;  tnode->left->parent=tnode;  tnode->left->dirn=0;		
			tnode->right->level=tnode->level+1; tnode->right->parent=tnode; tnode->right->dirn=1;
			newNodes+=2;
		}
	}
	//Update data->rPartWhich used to identify which node a data point belongs to.
	for(i=0;i<data->cols;i++) {
		which = data->rpartWhich[i];
		if(tree[which]->left==NULL) continue;	//terminal node
		tnode = tree[which];

		if(data->elem!=NULL) tmpX = (double) data->elem[tnode->var][i];
		else                 tmpX = (double) data->elemF[tnode->var][i];			

		if(tmpX < tnode->cut) data->rpartWhich[i]=tnode->left->rpartNodeNum;
		else                  data->rpartWhich[i]=tnode->right->rpartNodeNum;                              
	}
	return(newNodes);
}

void TnodeStatsFast2(TNODE **tree, long start, long end, MATRIXSHORT *data) {
/*
	Private To TnodeCutRecurseFast2()
	Determines tnode->y0,y1,y2,mu,tstat from data->rpartWhich and tnode->rpartNodeNum
	This is a version of TnodeStatsFast() that computes stats "in parallel"
*/
	char *name="TnodeStatsFast2()";
	double tmp,var;
	long i,which;
	TNODE *tnode;
	for(i=0;i<data->cols;i++) {
		which = data->rpartWhich[i];	//which node does the data belong to?
		if(which< start) continue;		//only compute stats on new trees
		ERR(which >= end);
		tnode = tree[which];
		if(data->target==NULL) {
			if(data->elem!=NULL) tmp=(double) data->elem[data->rows-1][i];
			else                 tmp=(double) data->elemF[data->rows-1][i];
		}
		else                   tmp=(double) data->target[i];
		tnode->y0+=1.0; tnode->y1+=tmp; tnode->y2+=(tmp*tmp);	//compute 0th, 1st and 2nd moments of the targets for the node
	}
	for(i=start;i<end;i++) {	//update stats user is interested in (e.g.  tnode->mu are the preds)
		tnode = tree[i];
		if((long)tnode->y0 < 2) {	//This should never happen because I'm not interested in nodes with small population.
			PRI(i,"%d");
			PRI(tnode->y0,"%lf");
			PRI(tnode->indnum,"%d");
			PRI(tree[i]->parent->y0,"%lf");
			PRI(tree[i]->parent->leftMax,"%d");
			ERR(1);
		}
		tnode->indnum = (long) tnode->y0;	//Population of a node is the 0th moment of the target distribution y0
		if((long) tnode->y0 >= 2) {			
			tnode->mu=tnode->y1/tnode->y0;
			tnode->deviance=tnode->y2 - tnode->mu*tnode->mu*tnode->y0;	//deviance = nPts*variance is function we are minimizing by cutting.
			var=tnode->y2-(tnode->y1*tnode->mu);
			if(var>0.0) tnode->tstat=tnode->y1/sqrt(var);
			else tnode->tstat=0.0;
		}
		else {
			tnode->mu=0.0;
			tnode->deviance=0.0;
			tnode->tstat=0.0;
		}
	}
}

void TnodeCutFast2(TNODE **tree, long start, long end, long cutpts, MATRIXSHORT *data) {
/*
	Parallel version of TnodeCutFast() for much more speed with large number of nodes.
	This is the heart of the fast tree fitting algorithm
*/
	char *name="TnodeCutFast2()";
	TNODE *z;
	double tmpY,tmpX;
	double diff;
	long i,j,k;
	long *index,which;
//Initialize
	for(k=start;k<end;k++) {
		tree[k]->yy0=tree[k]->y0;	//intialize moments of the targets of each node. These are used to compute deviance efficiently
		tree[k]->yy1=tree[k]->y1;
		tree[k]->yy2=tree[k]->y2;
		tree[k]->var=-1; 
		tree[k]->cut=-INFINITY;
		tree[k]->diffmax=-INFINITY;
	}

	for(j=0;j< data->rows-1 ;j++) {		//loop over the inputs
		index = data->rpartSorts[j];	//precomputed sorting indices used to access the inputs in order from smallest to largese
		/////////////////////////////////////////////////////////
		for(k=start;k<end;k++) {	//intialize quantities used to compute deviance efficiently
			tree[k]->y0=0.; 
			tree[k]->y1=0.; 
			tree[k]->y2=0.;
			tree[k]->tmpXold=-INFINITY;
		}
		for(i=0;i<data->cols;i++) { //loop over input data in ascending order
			which = data->rpartWhich[index[i]];
			if(which<start) continue;			//tree[which] not a candidate for cutting because it's an old node
			//tmpY is the target corresponding to the input tmpX below

			if(data->target!=NULL)
				tmpY=(double) data->target[index[i]];
			else if(data->elem  !=NULL)
				tmpY=(double) data->elem[data->rows-1][index[i]]; 
			else
				tmpY=(double) data->elemF[data->rows-1][index[i]];

			if(data->elem  !=NULL)
				tmpX=(double) data->elem[j][index[i]];
			else
				tmpX=(double) data->elemF[j][index[i]];

			z = tree[which];
			//If there is a genuine change in the ordered input data, compute TnodeCutFn used to make cutting decision
			//20060327: Added this z->tmpXold stuff to make it compatible with fast=0 and fast=1 techniques above.
			if(i==0|| tmpX > z->tmpXold)
				diff=TnodeCutFn(z->y0,z->y1,z->y2,z->yy0,z->yy1,z->yy2,(long)z->y0,z->indnum,cutpts,1);
			else
				diff=-INFINITY;		/*default*/

			z->tmpXold=tmpX;
			//update count y0, sum y1 and sumsqures y2 AFTER computing diff (the sum of the deviances of the 2 nodes if variable "j" is cut at < tmpX 
			z->y0+=1.0; 
			z->y1+=tmpY; 
			z->y2+=tmpY*tmpY; 
			if(diff>z->diffmax) { //TnodeCutFn has returned a new optimal value, so store tnode->cut and tnode->var
				//z->leftMax=(long) z->y0 -1 ;	//doesn't work because of ties
				z->diffmax=diff;
				z->cut=z->tmpXold;
				z->var=j;
			}
		}
	}
	//////////////because of possible ties z->leftMax (population to the left of the cut) needs to be recomputed  
	//////////////For large data->rows this takes not much CPU compared to the step above
	for(i=0;i<data->cols;i++) {
		which = data->rpartWhich[i];
		if(which < start) continue;
		z = tree[which];
		if(z->var < 0) ;	//never found a place to cut so z->leftMax=0

		else {
			if(data->elem!=NULL) tmpX = (double) data->elem[z->var][i];
			else                 tmpX = (double) data->elemF[z->var][i];
			if(tmpX < z->cut) z->leftMax++; 
			else ;
		}
	}
}

TNODE *TnodeMake(long indnum, long rpartFast){
/*
	Makes and intializes a node of the tree.
	If indnum > 0 also allocates space for tnode->ind, working space used to access inputs in 
	their numerical order.
*/
	char *name="TnodeMake()";
	TNODE *tnode;
	tnode=(TNODE *) MyCalloc(1,sizeof(TNODE),name);
	tnode->left=NULL;
	tnode->right=NULL;
	tnode->indnum=indnum;
	//ERR(indnum<1);
	if(indnum>0 && rpartFast==0) tnode->ind=(long *) MyCalloc(indnum,sizeof(long),name);
	else         tnode->ind=NULL;	//added to code 20060113
	tnode->mu=tnode->tstat=0.0;
	tnode->y0=tnode->y1=tnode->y2=0.0;
	tnode->var=-1;
	tnode->cut=-INFINITY;
	tnode->prune=0;
	tnode->deviance=0.0;
	tnode->leftMax=0;
	for(int i=0;i<N_TAR_CATS;i++)
	{
		tnode->ncat[i] =0;
		tnode->ncatI[i]=0;
		tnode->loss[i]=0;
		tnode->lossI[i]=0;
	}
	return tnode;
}

void TnodeKill(TNODE *x) {
//I think this is finally working
	char *name="TnodeKill()";
	TNODE *left,*right;
	if(x!=NULL) {
		left  = x->left;	//note use of trick to kill children of x below AFTER x is killed first
		right = x->right;
		if(x->ind !=NULL) free(x->ind);	//needed to make sure that if x->ind was freed up earlier then x->ind was also set to NULL
		free(x);						
		if(left!=NULL) {
			TnodeKill(left);
			TnodeKill(right);
		}
		else ;	//nothing left to kill
	}		
}

double TnodeSignal0(TNODE *tnode, double *data, long cutPts, double mindev){
/*
	If cutPts==0, use tnode coefs to make predictions on input data by using terminal node
	If cutPts>0, stop at the deepest level of node where there number of points >= cutPts
*/
	char *name="TnodeSignal0()";
	double pred;
	if(cutPts>0)       TnodeTraverse3(tnode,data,cutPts,&pred,mindev);
	else         pred = TnodeTraverse(tnode,data);
//PRI(pred,"%lf");
//exit(1);
	return(pred);
}

// new 
double TnodeTraverse(TNODE *tnode, double *vec) {
/*
	Using the input date "vec" find the terminal node it belongs in, and return conditional mean
*/
	char *name="TnodeTraverse()";
	if(tnode->left) {	/*node has children left AND right so go deeper in the tree*/
		if(vec[tnode->var]<tnode->cut) TnodeTraverse(tnode->left,vec);
		else                           TnodeTraverse(tnode->right,vec);
		ERR(1);		//added 20060407 Guess it never reaches this?
		return(NA);	//added 20060407 Guess it never reaches this?
	}
	else return(tnode->mu);	//We have a terminal node, so 
}

void TnodeTraverse3(TNODE *tnode, double *vec, long cutPts, double *pred, double mindev) {
/*
	Like TnodeTraverse(), but stop earlier if number of points tnode->indnum <= cutPts
	The conditional mean is stored in pred
*/
	char *name="TnodeTraverse3()";
//PRI(tnode->mu,"%lf");
//PRI(tnode->var,"%d");
//PRI(tnode->cut,"%lf");
//PRI(tnode->indnum,"%d");
//PRI(tnode->prune,"%d");
//PRI(vec[tnode->var],"%lf");
//PRI(tnode->left,"%d");
//PRI(tnode->right,"%d");
	if(tnode->indnum >= cutPts || tnode->deviance >= mindev) pred[0]=tnode->mu;
	if(tnode->left && tnode->indnum>cutPts && tnode->deviance > mindev) {	/*node has children left AND right*/
		if(vec[tnode->var]<tnode->cut) TnodeTraverse3(tnode->left,vec,cutPts,pred,mindev);
		else                           TnodeTraverse3(tnode->right,vec,cutPts,pred,mindev);
	}
	else ;
}

#define DIFF_SPLUS 1	
double TnodeCutFn(double y0,double y1,double y2,double yy0,double yy1,
				  double yy2,long i,long indnum,long cutpts,long weirdFactor){
/*
	Private to TnodeCut() for evaluating goodness of fit.
	DIFF_SPLUS=1 recommended.
	With that choice, the following computed the negative of "deviance" of the cut
	Where deviance is the number of points times the variance in the left cut + npts*variance in the right cut.
	One clearly wants the deviance to be small if the cut is good.
	See Hastie et. al. "Elements of Statitical Learning p 270 and
	Chambers & Hastie. "Statistical models in S" p 414
*/
	double left_var,right_var;	/* cond variances*npts */
	double left_tstat,right_tstat;

//Next 2 lines are me playing around with trying to avoid making cuts which would lead to too few points on the 
//left or right of the cut. I think I need to take weirdFactor=1, and not 4, to allow the tree to grow properly.
//I guess that the Splus implelentation doesn't care about this, and that a node with a really small number of
//points could be pruned off. But when I first wrote this code I did not try pruning, so I did this to avoid
//terminal nodes with small numbers of points.
	//if(i<cutpts/4||i>indnum-cutpts/4) return(-INFINITY);	//Dissallow cuts near the edge. TODO: replace cutpts/4 by cutpts??
	if(i< cutpts/weirdFactor || i>indnum-cutpts/weirdFactor ) return(-INFINITY);	//added 20060126

	left_var=y2-(y1*y1/y0);
	right_var=(yy2-y2)-((yy1-y1)*(yy1-y1)/(yy0-y0));

	if(DIFF_SPLUS) return(-left_var-right_var);
	else {	//15 years ago I played around with tstats....probably didn't work well.
		left_tstat=right_tstat=0.0;
		if(left_var>0.) left_tstat=y1/sqrt(left_var); 
		if(right_var>0.) right_tstat=(yy1-y1)/sqrt(right_var);
		return(fabs(left_tstat-right_tstat));
	}
}

void MatrixShortSort(MATRIXSHORT *x) {
/*
	If we have a lot of RAM there is an advantage in tree fitting algorithms to pre-computing sorting indices on the fitting data x.
	Calculate x->rpartSorts for first x->rows-1 (the inputs) of x for fast indexing used by tree fitting routines
	Create    x->rpartWhich working space for indentifying which node of a tree the input vector x->elem[*][i] falls into

*/
	char *name="MatrixShortSort()";
	long i,j;
	ERR(x->rpartSorts!=NULL);	//This could happen if we called this function twice with same data x. Don't want to do that.
	x->rpartSorts = (long **) MyCalloc(x->rows-1, sizeof(long *), name);
	for(i=0;i<x->rows-1;i++) x->rpartSorts[i]=CALLOC(long,x->cols);
	for(i=0;i<x->rows-1;i++) {
//PRI(i,"%d");
		for(j=0;j<x->cols;j++) x->rpartSorts[i][j]=j;	//initialize for indexing data	
		if(x->elem!=NULL) 
			 MccIndexArrayShort(x->rpartSorts[i],x->cols,x->elem[i]); //compute ind so data->elem[cols][ind[...]] is sorted in ascending order
		else MccIndexArrayFloat(x->rpartSorts[i],x->cols,x->elemF[i]);
	}
	ERR(x->rpartWhich!=NULL);
	x->rpartWhich = CALLOC(long,x->cols);
}
/*======================================================================*/
/*	 END TREE NODE CODE													*/
/*   Functions below are cut and paste from numerical recipes to do		*/ 
/*   rankings of data arrays ofvarious formats 							*/
/*======================================================================*/

void MccIndexArray(long *indx, long n, double *arrin) {
	char *name="MccIndexArray()";
	ERR(n<2);	/*bugged if n=1 ??, cf Robins code*/
	indx--;		/*CONVERSION Assume indx[0,..,n-1]*/
	NrcIndexArray(indx,n,arrin);
	indx++;		/*CONVERSION Assume indx[0,..,n-1]*/
}

void MccIndexArrayShort(long *indx, long n, short *arrin) {
	char *name="MccIndexArray()";
	ERR(n<2);
	indx--;		/*CONVERSION Assume indx[0,..,n-1]*/
	NrcIndexArrayShort(indx,n,arrin);
	indx++;		/*CONVERSION Assume indx[0,..,n-1]*/
}

void MccIndexArrayFloat(long *indx, long n, REAL *arrin) {
	char *name="MccIndexArrayFloat()";
	ERR(n<2);
	indx--;		/*CONVERSION Assume indx[0,..,n-1]*/
	NrcIndexArrayFloat(indx,n,arrin);
	indx++;		/*CONVERSION Assume indx[0,..,n-1]*/
}

void NrcIndexArray(long *indx, long n, double *arrin) {
/*
	Modified version of numerical recipes "indexx.c"
	MODIFICATION has effect sorting the "indx" which could
	refer to a subset of "arrin". This is useful in 
	MbedCut().
	This really should be the lowest level implementation:-
	could have numerical recipes "indexx" and Robins "IndexArray"
	as being calls to this.........
*/
        long l,j,ir,indxt,i;
        double q;
/* MODIFICATION
        for (j=1;j<=n;j++) indx[j]=j;
*/

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

void NrcIndexArrayShort(long *indx, long n, short *arrin) {
/*
	As NrcIndexArray but for short instead of double
*/
        long l,j,ir,indxt,i;
        short q;

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

void NrcIndexArrayFloat(long *indx, long n, REAL *arrin) {
/*
	As NrcIndexArray but for REAL (almost always defined as float) instead of double
*/
        long l,j,ir,indxt,i;
        //short q;
		REAL q;					//bug fixed 20060412

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

MATRIX *MakeMatrix(long rows, long cols) {
/*
   Allocates and returns an object of type MATRIX, size of rows X cols.
   ->period cannot be modified outside this routine or deallocation
      errors will result.
*/
  char *name = "MakeMatrix";
  long i, size;
  MATRIX *x;
  if (rows <= 0  ||  cols <= 0  ||  rows > MAXVECTORLEN  ||  cols > MAXVECTORLEN) {
    fprintf(stderr,"MAXVECTORLEN=%d\n",MAXVECTORLEN);
    fprintf(stderr,"rows=%d\n",rows);
    fprintf(stderr,"cols=%d\n",cols);
//    MccErr(name,"Invalid size");  
	JMErr(name,"Invalid size");

  }
  x = CALLOC(MATRIX,1);
  x->elem = (double **) MyCalloc(rows, sizeof(double *), name);
  x->rowlabel = (char **) MyCalloc(rows, sizeof(char *), name);
  x->collabel = (char **) MyCalloc(cols, sizeof(char *), name);

    x->period = MINBLOCK / cols;    
    if (!x->period) x->period = 1;
    if (x->period > rows) x->period = rows;
    size = x->period * cols;
    for (i = 0;  i < rows;  i++)
      if (i % x->period == 0) 
	x->elem[i] = CALLOC(double,size);  
      else x->elem[i] = x->elem[i - 1] + cols;

  x->rows = rows;
  x->cols = cols;
  return x;
}

/////////////////////////////////////////////////////////////////////////////////
//Functions for making and killing and reading and writing boosted trees 
/////////////////////////////////////////////////////////////////////////////////

TREEBOOST *TreeBoostMake(long len){
	char *name="TreeBoostMake()";
	TREEBOOST *y;
	y = (TREEBOOST *) MyCalloc(1,sizeof(TREEBOOST),name);
	y->len  = len;
	y->wts  = CALLOC(double,len);
	y->elem = (TNODE **) MyCalloc(len,sizeof(TNODE *),name);
	return(y);
}

void TreeBoostKill(TREEBOOST *x) {
	char *name="TreeBoostKill()";
	long i;
	free(x->wts);
	for(i=0;i<x->len;i++) TnodeKill(x->elem[i]);
}

void TreeBoostResidual(MATRIXSHORT *y, TREEBOOST *x, long start,long end) {
/*
	Replaces last row of y ("the target") by residuals using the predictions from
	the treeboost coefs in x->elem[start,.....end]*x->wts[start,...end]
	I'm was a bit nervous that this is being done in short integer precision.
	If wts is very small (shrinkage), then the preds subtracted will often be zero!
	So if y->target is non-NULL the calculation is done in floating point precision.
*/
	char *name="TreeBoostResidual()";
	double *vec;	//working space
	long i;
	double pred;
//PRI(start,"%d");
//PRI(end,"%d");
	vec = CALLOC(double,y->rows-1);
	for(i=0;i<y->cols;i++) {
		pred = TreeBoostPred(y,x,start,end,i,vec,NULL);
		if(y->target==NULL) {
			if(y->elem!=NULL) y->elem[y->rows-1][i]-=(short)pred;
			else             y->elemF[y->rows-1][i]-=pred;
		}
		else                {
//if(end==0) printf("%lf %lf\n",pred,y->target[i]);	//checking
			y->target[i] -= (REAL)pred;		//use more precision
		}
	}
	free(vec);
}

double TreeBoostPred(MATRIXSHORT *y, TREEBOOST *x, long start,long end, long i, double *vec, double *postWts) {
/*
	Simple wrapper around TnodeSignal0() above which computes preds from individual trees
	vec is a vector of inputs
	If the signals data y is non-NULL that will be used instead of vec as inputs
*/
	char *name="TreeBoostPred()";
	long j,k;
	double pred=0.0;
	if(end < start || end < 0) return(0.0);		//sometimes do this if x has not been built yet.
	if(y!=NULL) { 
		for(k=0;k<y->rows-1;k++) {
			if(y->elem!=NULL)
				vec[k]=(double)y->elem[k][i];
			else
			{
				double x = y->elemF[k][i];
				vec[k]=(double)y->elemF[k][i];
			}
		}
	}
	else ;	//assumes vec already contains the data
	for(j=start;j<=end;j++) {
//PRI(j,"%d");
		if(postWts==NULL) pred += x->wts[j]*TnodeSignal0(x->elem[j],vec,CUTPTSMIN,0.0);		//CUTPTSMIN forces to go to terminal nodes
		else              pred +=postWts[j]*TnodeSignal0(x->elem[j],vec,CUTPTSMIN,0.0);
	}
	return(pred);
}

double TreeBoostPredCat(MATRIXSHORT *y, TREEBOOST *x, long start,long end, long i, double *vec, double *postWts) {
/*
	Simple wrapper around TnodeSignal0() above which computes preds from individual trees
	vec is a vector of inputs
	If the signals data y is non-NULL that will be used instead of vec as inputs
*/
	char *name="TreeBoostPredCat()";
	long j,k;
	double pred=0.0;
	if(end < start || end < 0) return(0.0);		//sometimes do this if x has not been built yet.
	if(y!=NULL) { 
		for(k=0;k<y->rows-1;k++) {
			if(y->elem!=NULL) vec[k]=(double)y->elem[k][i];
			else			  vec[k]=(double)y->elemF[k][i];
		}
	}
	else ;	//assumes vec already contains the data
	for(j=start;j<=end;j++) {
//PRI(j,"%d");
		if(postWts==NULL) pred += x->wts[j]*TnodeSignal0(x->elem[j],vec,CUTPTSMIN,-INFINITY);		//CUTPTSMIN forces to go to terminal nodes
		else              pred +=postWts[j]*TnodeSignal0(x->elem[j],vec,CUTPTSMIN,-INFINITY);
	}
	return(pred);
}


void TreeBoostStats(double *y, MATRIXSHORT *data, TREEBOOST *x, long start, long end, REAL *preds) {
/*
	Returns RMS error and correlation statistics for treeboost prediction
	This is a simple wrapper around TreeBoostPred()
*/
	char *name="TreeBoostStats()";
	double *vec,pred,target;
	double xx=0.0,xy=0.0,yy=0.0,x1=0.0,y1=0.0,n;	//quick look at correlation
	long i;
	vec=CALLOC(double,data->rows-1);
	for(i=0;i<data->cols;i++) {
		//pred = TreeBoostPred(data,x,start,end,i,vec,NULL);
		preds[i] += (REAL) TreeBoostPred(data,x,end,end,i,vec,NULL);
		pred = (double) preds[i];
		//if(data->target==NULL) 
		if(data->elem!=NULL) target = (double)data->elem[data->rows-1][i];
		else				 target = (double)data->elemF[data->rows-1][i];
		//else				   target = (double)data->target[i];	//don't look at that, it was altered in the tree boost.
		y[0]+= (pred-target)*(pred-target);
		xx += pred*pred;
		xy += pred*target;
		yy += target*target;
		x1 += pred;
		y1 += target;
	}
	n = (double)data->cols;
//PRI(y[0],"%lf");
	y[0]/=n;
	y[0]=sqrt(y[0]);
	xy-=((x1*y1)/n);	//20060330 added this code to subtract means. Probably a small effect
	xx-=((x1*x1)/n);	//........
	yy-=((y1*y1)/n);    //........
	if(xx*yy > 0.0000000001) y[1]=xy/sqrt(xx*yy);
	else                     y[1]=NA;	//happens if all preds are the same
//PRI(end,"%d");
//PRI(xx,"%lf");
	free(vec);
}


void TreeBoostPerformance(MATRIX *z, MATRIXSHORT *dataFit, MATRIXSHORT *dataTst, TREEBOOST *y, 
						  long start, long end, long iter, REAL *predsFit, REAL *predsTst) {
/*
	Input:  dataFit and dataTst to assess performance of TREEBOOST model y with trees from "start" to "end"
	If iter=0 need to create column labelling.
	Output: a one-row matrix "z" of various performance measures
*/
	char *name="TreeBoostPerformance()";
	double residErr=0.0,tmp,dev;
	long i,npts;
	if(iter==0) {		//need column labels for first matrix
		ERR(z->cols!=9);
		z->collabel[0]=MakeString("iterations");	//which iteration number are at?
		z->collabel[1]=MakeString("fitResidRMS");	//RMS of latest "modified" targets
		z->collabel[2]=MakeString("treeDeviance");	//note treeDeviance = fitResidRMS only if y->wts[*] = 1.0 ( no shrinkage)
		z->collabel[3]=MakeString("fitPts");		//number of points used in the fit
		z->collabel[4]=MakeString("tstRMS");		//test set RMS prediction error
		z->collabel[5]=MakeString("tstCorr");		//test set corr(predictions, target)
		z->collabel[6]=MakeString("tstPts");		//number of points in the test set
		z->collabel[7]=MakeString("fitRMS");		//test set RMS prediction error. Should be close to fitResidRMS (but fitRMS uses short int precision)
		z->collabel[8]=MakeString("fitCorr");		//test set corr(predictions, target)
	}
	else {
		for(i=0;i<z->cols;i++) {
			free(z->collabel[i]); z->collabel[i]=NULL;
		}
	}

	z->elem[0][0]=(double)iter;

	for(i=0;i<dataFit->cols;i++) {
		if(dataFit->target==NULL) {
			if(dataFit->elem!=NULL) tmp=(double) dataFit->elem[dataFit->rows-1][i];
			else                   tmp=(double) dataFit->elemF[dataFit->rows-1][i];
		}
		else					  tmp=(double) dataFit->target[i];
		residErr+=tmp*tmp;
	}
	residErr/=(double)dataFit->cols;
	residErr=sqrt(residErr);
	z->elem[0][1]=residErr;
	

	z->elem[0][3]=(double) dataFit->cols;

	if(end >= start) {	//compute deviance of most recent tree fitted  
		npts=0; dev=0.0;
		TnodeDeviance(y->elem[end],&dev,&npts); 
		z->elem[0][2]=sqrt(dev/(double)npts);	//should be equal to z->elem[0][1];
	}
	else z->elem[0][2]=NA;

	TreeBoostStats(z->elem[0]+4,dataTst,y,start,end,predsTst);

	if(dataFit->target!=NULL) {//original target dataFit->elem[dataFit->rows-1] did not get overwritten
		TreeBoostStats(z->elem[0]+7,dataFit,y,start,end,predsFit);
	}
	else {z->elem[0][7]=NA; z->elem[0][8]=NA;}	//targets get modified so calculation makes no sense

	z->elem[0][6]=(double) dataTst->cols;
}



void TnodeDeviance(TNODE *x, double *dev, long *npts) {
//sum up deviances and number of points in terminal nodes
//Deviance = npt*variance of points in each node is the objective function minimized by the greedy tree fitting algorithm
	char *name="TnodeDeviance()";
	if(x==NULL) ;
	else if(x->left==NULL || x->right==NULL) { //terminal
		*dev  +=x->deviance;	
		*npts +=x->indnum;
	}
	else {
		TnodeDeviance(x->left, dev,npts);
		TnodeDeviance(x->right,dev,npts);
	}
}



void TnodeBoostWriteDB(int udate, vector<long> &Sflags, TNODE *tnode,int treeNum, double treeWt)
{
	string market;
	if(Sflags[0] == 101)  
		market = "U";
	else if(Sflags[0] == 105)
		market = "J";
	else if(Sflags[0] == 108)
		market = "L";
	else if(Sflags[0] == 107 )
		market = "E";
	else if(Sflags[0] == 110) 
		market = "T";
	else if(Sflags[0] == 112)  
		market = "H";
	else if(Sflags[0] == 113) 
		market = "S";
	else if(Sflags[0] == 114)  
		market = "K";
	else if(Sflags[0] == 115)  
		market = "F";
	else if(Sflags[0] == 117)  
		market = "J";
	else if(Sflags[0] == 118) 
		market = "G";
	else if(Sflags[0] == 119) 
		market = "W";

	string table;
	if(Sflags[11]>=1010 && Sflags[11]<=1190)
		table = "hfTree";
	else if(Sflags[11]>=10100 && Sflags[11] <=11900)   
		table = "hfPureTreeBoost";
	else if(Sflags[11]==4) 
		table = "hfOptTreeBoost";
	else 
		return;

	string database;
	if(Sflags[6]==0)
	{
		if(Sflags[11]==4)
			database = "hfOptions";
		else if(Sflags[0]==105)
			database ="hfStocktsx";
		else if(( Sflags[0]>=106 && Sflags[0]<=109) )
			database ="hfStockEU";
		else if(Sflags[0]==110 || Sflags[0]==112 || Sflags[0]==113|| Sflags[0]==114|| Sflags[0]==118|| Sflags[0]==119)
			database ="hfStockasia";
		else if(Sflags[0]==101 || Sflags[0]==115)
			database ="hfStock";
		else if(Sflags[0]==117)
			database ="hfStockma";
		else
			return;
	}
	else
	{
		if(Sflags[11]==4)
			database = "hfOptions";
		else if(Sflags[0]==104||Sflags[0]==105)
			database ="hfStockTsx_debug";
		else if((Sflags[0]>=106 && Sflags[0]<=109))
			database ="hfStockEU_debug";
		else if(Sflags[0]==110 || Sflags[0]==112|| Sflags[0]==113|| Sflags[0]==114 || Sflags[0]==118|| Sflags[0]==119)
			database ="hfStockAsia_debug";
		else if(Sflags[0]==101 || Sflags[0]==115)
			database = "hfDebug";
		else if(Sflags[0]==117 )
			database = "hfStockma_debug";
		else
			return;
	}

	ODBCConnection odbcHFWrite;	
	odbcHFWrite.Set(database.c_str(),"mercator1","DBacc101");
	odbcHFWrite.Open();
	char sqlCmd[BUFFSIZE];
	// delete table
	if(Sflags[11] == 4) 
	{
		sprintf(sqlCmd,"delete from hfOptTreeBoost where idate=%d and treeNumber = %d",udate,treeNum);
		odbcHFWrite.ExecDirect(sqlCmd);
		market = "A"; 
	}
	else 
	{
		sprintf(sqlCmd,"delete from %s where idate=%d and market = '%s' and treeNumber = %d",table.c_str(),udate,market.c_str(),treeNum);
		odbcHFWrite.ExecDirect(sqlCmd);
	}
	
	int count=0;
	TnodeBoostWriteDBRe(tnode,odbcHFWrite,udate,market,count,treeNum,treeWt,table);
	return;
}

void TnodeBoostWriteDBRe(const TNODE *tnode,ODBCConnection &odbcHFWrite,const int &udate,const string &market,int &count,const int &treeNum,const double &treeWt, const string &table)
{
	long var=-1;
	double cut=0.0;
	int oldcount = count;
	assert( market.c_str() );
	assert( 1==market.length() );

    if(tnode!=NULL) 
	{
		cout << "treeNum = " << treeNum << "\t" << "count = " << count << "\t" << "tstat = " << tnode->tstat << endl; 
		if(tnode->left==NULL) {	var=-1; cut=0.0; }		//terminal node
		else                  { var=tnode->var; cut=tnode->cut; }
		static char sqlCmd[BUFFSIZE]; // DON'T MULTI-THREAD WITH STATIC; put this in because we were getting a static overflow because we werr recursing too deeply and there wasn't enough memory on the stack

		sprintf(sqlCmd,"insert into %s  (idate,market,cutVariable,cutValue,nPts,tstat,mu,deviance,prune,idx,treeNumber,treeWt)"
			"VALUES (%d,'%s',%d,%f,%d,%f,%f,%f,%d,%d,%d,%f)",table.c_str(),udate, market.c_str(),var,cut,
			tnode->indnum,tnode->tstat,tnode->mu,tnode->deviance,tnode->prune,count,treeNum,treeWt);

		try{odbcHFWrite.ExecDirect(sqlCmd);}
		catch(...)
		{
			cerr << "error executing SQL insertion " << sqlCmd << endl; 
		};

		(count)++; // need to keep track of the order in which we write the nodes to the db so that we can read them out in the same order
		assert(oldcount < count); 

		oldcount=count;
		TnodeBoostWriteDBRe(tnode->left ,odbcHFWrite,udate,market,count,treeNum,treeWt,table);
		assert(oldcount <= count); 
		oldcount=count;
		TnodeBoostWriteDBRe(tnode->right,odbcHFWrite,udate,market,count,treeNum,treeWt,table);
		assert(oldcount <= count); 
    }	
}

void TnodeDumpFname(TNODE *tnode,char **label,char *fname, long format, char *rw) {
/*
	Dump a tree tnode to file fname
	If format=0 use input labels to make the file easy to read by a human (don't use this much now)
	If format=1 dump in a format that is easy to read with TnodeReadFname() above
*/
	char *name="TnodeDumpFname()";
	FILE *fp;
	//fp=MccFileOpen(fname,"w",TRUE,name);
	fp=MccFileOpen(fname,rw,TRUE,name);
	if(format==0)  TnodeDumpFile(tnode,label,fp);
	else          TnodeDumpFile2(tnode,fp);			//easier to read into my code this way
	MccFileClose(fp);
}
void TnodeDumpFname2(TNODE *x, char *fname, double wt, long treeNum) {
/*
	Dump a tree tnode to file fname with data on it's wt and treeNum for use by TreeBoostReadFname()
*/
	char *name="TnodeDumpFname2()";
	FILE *fp;
	if(treeNum==0)	{
		fp=MccFileOpen(fname,"w",TRUE,name);
		fprintf(fp,"%4s %7s %3s %15s %10s %15s deviance\n","num","weight","var","varCut<","nPts","mu","deviance");
	}
	else fp=MccFileOpen(fname,"a",TRUE,name);
	TnodeDumpFile3(x,fp,wt,treeNum);
	MccFileClose(fp);
}

void TnodeDumpFile(TNODE *tnode,char **label,FILE *fp) {
//Like TnodeDumpCond but uses a filename fname
	char *name="TnodeDumpFile()";
	static int ncalls=0;  // changed from static ncalls=0; for 64
	ncalls++;
	if(ncalls==1) {
		fprintf(fp,"=============================================\n");
		fprintf(fp,"		In %s		\n",name);
		fprintf(fp,"=============================================\n");
	}
    if(tnode!=NULL) {
		long i;
		for(i=0;i<tnode->level;i++) fprintf(fp,"  ");
		if(tnode->parent==(TNODE *) NULL) fprintf(fp,"**%19s ","ROOT-------------->");
		else if(tnode->dirn) fprintf(fp,"**%10s>%8.5f ",label[tnode->parent->var],tnode->parent->cut);
		else                 fprintf(fp,"**%10s<%8.5f ",label[tnode->parent->var],tnode->parent->cut);
		fprintf(fp,"=> %d %lf %lf\n",tnode->indnum,tnode->tstat,tnode->mu);
                TnodeDumpFile(tnode->left,label,fp);
                TnodeDumpFile(tnode->right,label,fp);
    }	
}

void TnodeDumpFile2(TNODE *tnode,FILE *fp) {
//Like TnodeDumpFile, but simpler so TnodeReadFile() can work
	long var;
	double cut;
	char *name="TnodeDumpFile2()";
    if(tnode!=NULL) {
		if(tnode->left==NULL) {	var=-1; cut=0.0; }		//terminal node
		else                  { var=tnode->var; cut=tnode->cut; }
		//fprintf(fp,"%3d %15.5f %10d %15.5f %15.5f\n",var,cut,tnode->indnum,tnode->tstat,tnode->mu);
		fprintf(fp,"%3d %15.5f %10d %15.5f %15.5f %lf %6d\n",var,cut,tnode->indnum,tnode->tstat,tnode->mu,tnode->deviance,tnode->prune);
        TnodeDumpFile2(tnode->left,fp);
        TnodeDumpFile2(tnode->right,fp);
    }	
}

void TnodeDumpFile3(TNODE *tnode,FILE *fp, double wt, long treeNum) {
//Like TnodeDumpFile2, but dumps weights and treeNum and no tstat or prune info
	long var;
	double cut;
	char *name="TnodeDumpFile3()";
    if(tnode!=NULL) {
		if(tnode->left==NULL)
			{	var=-1; cut=0.0; }		//terminal node
		else
			{ var=tnode->var; cut=tnode->cut; }

//		fprintf(fp,"%4d %7.4f %3d %15.5f %10d %15.5f %lf\n",treeNum,wt,var,cut,tnode->indnum,tnode->mu,tnode->deviance);
		fprintf(fp,"%4d %7.4f %3d %15.10f %10d %15.5f %lf\n",treeNum,wt,var,cut,tnode->indnum,tnode->mu,tnode->deviance);
 		TnodeDumpFile3(tnode->left,fp,wt,treeNum);
        TnodeDumpFile3(tnode->right,fp,wt,treeNum);
    }	
}

TREEBOOST *TreeBoostReadFname(char *fname) {
//Wrapper around TnodeReadFile2() for reading in TREEBOOST coefs from a file
	char *name="TreeBoostReadFname()",buf[100];
	FILE *fp;
	TREEBOOST *y;
	TNODE *root,*temp;
	MATRIX *junk;
	long i,len;
	junk = MatrixGet(fname,0);					//First get the coefs as a matrix
	len  = 1+(long)junk->elem[junk->rows-1][0];	//Figures out the number of trees
	y = TreeBoostMake(len);
	//Read in the weights for each tree
	for(i=0;i<junk->rows;i++) y->wts[(long)junk->elem[i][0]]=junk->elem[i][1];
	fp=MccFileOpen(fname,"r",TRUE,name);
	for(i=0;i<junk->cols;i++) fscanf(fp,"%s",buf);	//flush out the header
	KillMatrix(junk);								//don't need that data anymore
	for(i=0;i<y->len;i++) {							//now read the trees one at a time
		root = TnodeMake(0,0);
		temp = TnodeReadFile2(fp,root,0);			//reads in each tree using "recursion"
		y->elem[i]=root;
	}
	MccFileClose(fp);
//TnodeDumpFname(y->elem[y->len-1],NULL,"temp.txt",1); exit(1);	//for debugging
	return(y);	
}

TNODE *TnodeReadFname(char *fname) {
//Wrapper around TnodeReadFile()
	char *name="TnodeReadFname()";
	FILE *fp;
	TNODE *root,*temp;
	root=TnodeMake(0,0);
	fp=MccFileOpen(fname,"r",TRUE,name);	
	//temp = TnodeReadFileOld(fp,root);
	temp = TnodeReadFile(fp,root,0);
	MccFileClose(fp);
	return(root);	
}

TNODE *TnodeBoostCreate(vector<long> &Sflags,int tidate,string tmarket,string model,int treeNum, double* ttreeWt) {
	// prod   = 1 if query is to hfstock; prod = 0 if query is to hfdebug 
	// idate  = the date on which params are to be used; use the most recent idate in production
	// market = "N" for NYSE, "O" for NASD
	// model  = "om" for one minute model; "tm" for ten min etc.; currently the value of model is ignored

	string dbmarket;
	if(Sflags[0] == 101)
		dbmarket = "U";
	else if(Sflags[0] == 105)
		dbmarket = "J";
	else if(Sflags[0] == 108 )
		dbmarket = "L";
	else if(Sflags[0] == 107)
		dbmarket = "E";
	else if(Sflags[0] == 110)
		dbmarket = "T";
	else if(Sflags[0] == 112)
		dbmarket = "H";
	else if(Sflags[0] == 113)
		dbmarket = "S";
	else if(Sflags[0] == 114)
		dbmarket = "K";
	else if(Sflags[0] == 115)
		dbmarket = "F";
	else if(Sflags[0] == 117)
		dbmarket = "J";
	else if(Sflags[0] == 118)
		dbmarket = "G";
	else if(Sflags[0] == 119)
		dbmarket = "W";

	string table;

	if(Sflags[11]>=1010 && Sflags[11]<=1190)
		table = "hfTree";
	else if(Sflags[11]>=10100 && Sflags[11] <=11900)   
		table = "hfPureTreeBoost";
	else if(Sflags[11]==4) 
		table = "hfOptTreeBoost";

	string database;
	if(Sflags[11]==4)
		database = "hfOptions";
	else if(Sflags[0] ==105)
		database ="hfStocktsx";
	else if(Sflags[0]>=106 && Sflags[0]<=109)
		database ="hfStockEU";
	else if(Sflags[0]==110||Sflags[0]==112||Sflags[0]==113||Sflags[0]==114||Sflags[0]==118||Sflags[0]==119)
		database ="hfStockasia";
	else if(Sflags[0]==117)
		database ="hfStockma";
	else if(Sflags[6]==1 )
		database = "hfDebug";
	else if(Sflags[6]==0 )
		database ="hfStock";

	ODBCConnection odbcHFRead;	
	odbcHFRead.Set(database.c_str(),"mercator1","DBacc101");  

	odbcHFRead.Open();
	char sqlCmd[LARGEBUFFSIZE];

	vector<vector<string> > treeData;
	enum {idate,market,cutVariable,cutValue,nPts,tstat,mu,deviance,prune,idx,treeNumber,treeWt};
	sprintf(sqlCmd,"SELECT idate,market,cutVariable,cutValue,nPts,tstat,mu,deviance,prune,idx,treeNumber,treeWt FROM %s where market = '%s' and idate = %d and treeNumber = %d order by idx",table.c_str(),dbmarket.c_str(),tidate,treeNum);
	odbcHFRead.ReadTable(sqlCmd,&treeData);

	TNODE *root=NULL,*temp;
	root=TnodeMake(0,0);
	// check that we read something
	if( treeData.size()==0 )
		return(root);

	*ttreeWt=atof(treeData[0][treeWt].c_str());
	int count=0;
	temp = TnodeCreateRe(treeData,root,&count);
	return(root);	
}

TNODE *TnodeCreateRe(const vector<vector<string> > &treeData, TNODE *root,int *count) {

//	Reads one line from file *fp
//	If flag=0, root is assumed to be the root of the tree if flag=0
//	If flag=1, root may be at a deeper level in the tree. 
//	This function is called recursively

	TNODE *y=NULL;
	long var,indnum,prune;
	double cut,tstat,mu,deviance;
	int i = *count;
	var=(long)atoi(treeData[i][2].c_str());
	indnum=(long)atoi(treeData[i][4].c_str());
	prune=(long)atoi(treeData[i][8].c_str());
	cut=(double)atof(treeData[i][3].c_str());
	tstat=(double)atof(treeData[i][5].c_str());
	mu=(double)atof(treeData[i][6].c_str());
	deviance=(double)atof(treeData[i][7].c_str());

	if(i==0)    y=root;			    //If *count=0, important to return the root node
	else        y=TnodeMake(0,0);	//If *count=1, allocate space for a new node
	y->mu=mu;
	y->deviance=deviance;
	y->tstat=tstat;
	y->indnum=indnum;
	y->var=var;
	y->cut=cut;
	y->prune=prune;
	++(*count); // Iterate the counter
	if(var> -1) {				//If the node is not terminal, read the children nodes
		y->left =TnodeCreateRe(treeData,root,count);
		y->right=TnodeCreateRe(treeData,root,count);
	}
	return(y); 
}

TNODE *TnodeReadFile(FILE *fp, TNODE *root, long flag) {

//	Reads one line from file *fp
//	If flag=0, root is assumed to be the root of the tree if flag=0
//	If flag=1, root may be at a deeper level in the tree. 
//	This function is called recursively

	char *name="TnodeReadFile()";
	TNODE *y=NULL;
	long var,indnum,prune;
	double cut,tstat,mu,deviance;
//Read one line of node coefs from FILE *fp
	fscanf(fp,"%d",&var);
	fscanf(fp,"%lf",&cut);
	fscanf(fp,"%d",&indnum);
	fscanf(fp,"%lf",&tstat);
	fscanf(fp,"%lf",&mu);
	fscanf(fp,"%lf",&deviance);
	fscanf(fp,"%d",&prune);
//PRI(var,"%d");
	if(flag==0) y=root;			//If flag=0, important to return the root node
	else        y=TnodeMake(0,0);	//If flag=1, allocate space for a new node
	y->mu=mu;
	y->deviance=deviance;
	y->tstat=tstat;
	y->indnum=indnum;
	y->var=var;
	y->cut=cut;
	y->prune=prune;
	if(var> -1) {				//If the node is not terminal, read the children nodes
		y->left =TnodeReadFile(fp,root,1);
		y->right=TnodeReadFile(fp,root,1);
	}
	return(y);
}

TNODE *TnodeReadFile2(FILE *fp, TNODE *root, long flag) {
/*
	Like TnodeReadFile() but assumes data of a different format.
	Reads one line at a time from file *fp
	If flag=0, root is assumed to be the root of the tree.
	If flag=1, root may be at a deeper level in the tree. 
	This function is called recursively
*/
	char *name="TnodeReadFile2()";
	TNODE *y=NULL;
	long var,indnum,num;
	double cut,mu,deviance,weight;
//Read one line of node coefs from FILE *fp
	fscanf(fp,"%d", &num);		//already read using call to MatrixGet() in TreeBoostReadFname()
	fscanf(fp,"%lf",&weight);	//already read using call to MatrixGet() in TreeBoostReadFname()
	fscanf(fp,"%d", &var);
	fscanf(fp,"%lf",&cut);
	fscanf(fp,"%d", &indnum);
	//fscanf(fp,"%lf",&tstat);	//not stored
	fscanf(fp,"%lf",&mu);
	fscanf(fp,"%lf",&deviance);
	//fscanf(fp,"%d",&prune);	//not stored
	if(flag==0) y=root;			//If flag=0, important to return the root node
	else        y=TnodeMake(0,0);	//If flag=1, allocate space for a new node
	y->mu=mu;
	y->deviance=deviance;
	y->tstat=0.0;				//I didn't dump this for boosted trees
	y->indnum=indnum;
	y->var=var;
	y->cut=cut;
	y->prune=0;					//I didn't dump this for boosted trees
	if(var> -1) {				//If the node is not terminal, read the children nodes
		y->left =TnodeReadFile2(fp,root,1);
		y->right=TnodeReadFile2(fp,root,1);
	}
	return(y);
}

 char *MakeString(char *buf) {
/*
	Create and return a copy of string in *buf.
*/
	char *name="MakeString()",*s;
	if(buf==(char *) NULL) return buf;
	s=CALLOC(char,(long)strlen(buf)+1);
	strcpy(s,buf);
	return s;
}

 void MccErr(char *mes1, char *mes2) {
	char in[50];
	fprintf(stderr,"ERROR: In %s, %s\n",mes1,mes2);
	fprintf(stderr,"		EXIT		  \n\a");
	fprintf(stderr,"press any key to continue\n");	gets(in);	//added for Splus
	exit(-1);
 }
void Warning(char *mes) {
  if (mes[0] == '!') fprintf(stderr, "%s\n", mes + 1);
  else fprintf(stderr, "WARNING: %s\n", mes);
}
FILE *MccFileOpen(char *fname, char *rw, long killflag, char *callprog) {
/*
	Prototype. Altered 26Dec to allow for append mode rw[0]=='a'
*/
	char *name="MccFileOpen()",buferr[200];
//	char buf[100];
	char fname2[WL];
	FILE *f;

//fprintf(stderr,"In MccFileOpen() hello world\n");

	sprintf(buferr,"callprog=%s, fname=%s, rw=%s, killflag=%d\n",callprog,fname,rw,killflag);

	ERR(global_filetype!=NOFILE);
	global_filetype=UNCOMPRESSED;

	ERR(strlen(fname)==0);
	if(!strcmp(fname,"stdout")) {
    		if(rw[0]=='r' || rw[0]=='a') 
			MccErr(name,"Can't read from stdout");
    		return stdout;
	}
	else if(!strcmp(fname, "stderr")) {
    		if(rw[0]=='r') MccErr(name,"Can't read from stderr");
    		return stderr;
	}
	else if(!strcmp(fname, "stdin")) {
		if(rw[0]=='w' || rw[0]=='a') 
			MccErr(name,"Can't write to stdin ");
    		return stdin;
	}
	else if(rw[0]=='r') {
		if(!strcmp(fname,"stdin")) f=stdin;
		else {
			strcpy(fname2,fname);
			if(!IsFile(fname2)) {
				if(!IsExtension(fname2,".Z")) strcat(fname2,".Z");
				else *strstr(fname2,".Z")='\0';
			}
			if(!IsFile(fname2)) f=(FILE *) NULL;
#if UNIX
			else if(IsExtension(fname2,".Z")){
				sprintf(buf,"zcat %s",fname2);
				f=(FILE *) popen(buf,"r");
				global_filetype=COMPRESSED;

			}
#endif
			else f=fopen(fname2,rw);
		}
	}
	else if(rw[0]== 'w' || rw[0]=='a') {
		if(IsExtension(fname,".Z")) MccErr(name,"Can't write to .Z");
		else if(IsFile(fname)) {
/*
			sprintf(buf, "File %s already exists", fname);
      			Warning(buf);
*/
			if(rw[0]=='a') ; /* appending to existing file */
			else if(killflag) {
#if UNIX 
				UnixCall("rm",fname);
#endif
				;	//DOS appears to remove the file anyway
			}
        		else MccErr(name,"file for writing exists");
    		}
    		f=(FILE *) fopen(fname, rw);
		global_filetype=UNCOMPRESSED;
	}
	else MccErr(name,"Weird error");

	if(!f) {
		global_filetype=NOFILE;
		if(killflag) MccErr(name,buferr);
	}
	return(f);
}

long MccNumberRecords(char *fname, long maxrec) {
/*
   Returns number of records in ascii file *fname.
   If more than maxrec, return NOTFOUND.
   If maxrec is zero, count all records.
*/
  char c, temp, *name = "MccNumberRecords";
  long numrec = 0;
  FILE *f;
  f = MccFileOpen(fname,"r",FALSE,name);
  if (!f) return numrec;
  while ((c = getc(f)) != EOF) {
    if (c == '\n' &&  ++numrec > maxrec  &&  maxrec) break;
    temp = c;
  }
  MccFileClose(f);
//PRI(numrec,"%d");
  if (temp != '\n') numrec++;
  if (maxrec  &&  numrec > maxrec) return NOTFOUND;
  return numrec;
}


long IsFile(char *fname) {
/*
	Used in MccFileOpen() for quick check that files exist for reading
*/
	FILE *f;
	if(!strcmp(fname,"stdin")) return(1);
	f=fopen(fname,"r");
	if(f==(FILE *)NULL) {
#if UNIX			
#######in DOS can close (FILE *) NULL
		fclose(f);
#endif
	return(0);}
	else	            {fclose(f); return(1);}
}
long IsExtension(char *name, char *ext) {
	char *p;
	p=strrchr(name,'.');
	if(p) {	/**file has an extension**/
		return (!strcmp(p,ext));
	}
	else return(0);
}

void MccFileClose(FILE *f) {
	char *name="MccFileClose()";
	ERR(global_filetype==NOFILE);
	if(f==stderr||f==stdout||f==stdin) ;
#if UNIX
	else if(global_filetype==COMPRESSED) {
			pclose(f);
	}
#endif
	else fclose(f);
	global_filetype=NOFILE;
}

long MccNumberFields(char *fname, long recnum) {
/*
   Calculates number of fields in record recnum of data file.
   Generalized by MCC to accept compressed Files
*/
  char *name="MccNumberFields()";
  long havefield = FALSE, numfield = 0;
  FILE *f;
  register char c;
  if (!(f=MccFileOpen(fname,"r",FALSE,name))) return numfield;
ERR(recnum<=0);	/*ADDED BY MCC 4Jan94, is it neccessary?? */
  while (--recnum) {
/*MCC
    fscanf(f, "%*[^\n]");
    c = fgetc(f);
*/
	while((c=fgetc(f))!='\n') ;
  }
  do {
    c = fgetc(f);
    if (!(isspace(c) || c == EOF)) havefield = TRUE;
    else if ((isspace(c) || c == EOF) && havefield) {
      havefield = FALSE;
      numfield++;
    }
  } while (c != 10 && c != EOF);

  MccFileClose(f);
  return numfield;
}

static long MccRecordIsNumeric(char *fname, long recnum) {
/*
	Private to MatrixGet()
   Returns TRUE is recnum record of fname is *all* numeric.
   Returns FALSE if it is not.
   Returns NOTFOUND if it does not exist.
   Generalized by MCC to accept compressed Files
   19Sept94, modified by MCC to allow format 6.347656E-02
   (I presume robin's stuff was format 6.347656e-02)
*/
  char *name="MccRecordIsNumeric()",c;
  FILE *f;

  if (!(f = MccFileOpen(fname, "r",FALSE,name))) return NOTFOUND;

  while (--recnum) {
    fscanf(f, "%*[^\n]");
    c = fgetc(f);
  }
  for (  ;  ;  ) {
    c = fgetc(f);
/*DEBUG PRI(c,"%c"); */
    if (c == 10  ||  c == EOF) break;
    if (isdigit(c)  ||  isspace(c)) continue;
    if (c == '+'   ||  c == '-'  ||  c == '.'  ||  c == 'e' || c == 'E') continue;
    MccFileClose(f);
    return FALSE;
  }

  MccFileClose(f);

  return TRUE;
}

MATRIX *MatrixGet(char *fname, long flag) {
/*
	-MatrixGet() does not work with fname=stdin. This means that 
	 I cannot use pipes and redirection with my code.
   Reads and returns a matrix from file *fname.
   Allowable formats are
          data only
          data and column labels
          data and row labels
          data, column and row labels
   This is GetMatrix() modified by MCC to take compresses files fname
   All these routines for getting matrices could use unifying!!
   Doesn't work with fname=stdin, since fname is accessed more than once.
   Need to get around this problem if "xx" and "xp" in mccapp,mccapp2 are
   going to work with pipes and redirection.
*/
  long rows, collabel = FALSE, rowlabel = FALSE;
  long i, cols, cols2, numeric1, numeric2;
  char *name="MatrixGet()",buf[100];
  MATRIX *m;
  FILE *f;

  ERR(!strcmp(fname,"stdin")); 
  if(!IsFile(fname)) {	/* added 30Mar93 for passing NULL matrices */
	fprintf(stderr,"WARNING, in %s, %s not found. Returning NULL\n",name,fname);
	return((MATRIX *)NULL);
  }
  if(flag==0) {
	cols = MccNumberFields(fname, 1);
	ERR(!cols);
	cols2 = MccNumberFields(fname, 2);
	numeric1 = MccRecordIsNumeric(fname, 1);
	ERR(numeric1==NOTFOUND);
	if (!cols2) {
		if (!numeric1) {
		rowlabel = TRUE;
		cols--;
		}
	} else if (cols2 == cols) {
		numeric2 = MccRecordIsNumeric(fname, 2);
		ERR(numeric2==NOTFOUND);
		if (!numeric2) {
		rowlabel = TRUE;
		cols--;
		} else if (!numeric1) collabel = TRUE;
	} else if (cols2 != cols) {
		ERR(cols2!=cols+1);
		collabel = rowlabel = TRUE;
	}
	rows = MccNumberRecords(fname, 0) - (collabel ? 1 : 0);
//PRI(rows,"%d");
  }
  else {	//Splus dumping
	  collabel=rowlabel=TRUE;
	  rows=MccNumberRecords(fname,0)-1;
	  cols=MccNumberFields(fname,1)-1;
  }
//PRI(rows,"%d");
//PRI(cols,"%d");
//PRI(rowlabel,"%d");
//PRI(collabel,"%d");
  m = MakeMatrix(rows, cols);
  m->name = MakeString(fname);
  f = MccFileOpen(fname, "r",TRUE, "MatrixGet()");
   //!!!!!!!!!!!!!!!!!!!!!!!! OK up to here 
   if(flag) {         
	ERR(fscanf(f, "%90s", buf) != 1);
	ERR(strcmp(buf,"row.names") && strcmp(buf,"uid") );
  }
   // problem after here
  if (collabel)
    for (i = 0;  i < m->cols; i++) {
       ERR(fscanf(f, "%90s", buf) != 1);
     //  buf[LABELLENGTH] = '\0';   // !!!! this causes problems 
	     buf[LABELLENGTH-1] = '\0';   // !!!! this causes problems 
//PRI(buf,"%s");
       m->collabel[i] = MakeString(buf);
    }
	// problem before
  rows = cols = 0;
  while (rows < m->rows && ((rowlabel  &&  fscanf(f, "%90s", buf) == 1)  || (!rowlabel  &&  fscanf(f, "%lf", &m->elem[rows][cols]) == 1))) {
    if (rowlabel) {
      buf[LABELLENGTH-1] = '\0';
      m->rowlabel[rows] = MakeString(buf);
//PRI(buf,"%s");
    }
//fprintf(stderr,"%d ",rows);
    for (cols =  (rowlabel ? 0 : 1);  cols < m->cols;  cols++) {
//if(rows==23399) fprintf(stderr,"%d",cols);
//PRI(m->elem[rows][cols],"%lf");
      ERR(fscanf(f, "%lf", &m->elem[rows][cols])!=1);
//if(rows==23399) fprintf(stderr," %d",rows);
	}
    cols = 0;
    rows++;
  }
//PRI("flag0","%s");
  if (rows < m->rows) {
    Warning("Possible blank lines in input file in MatrixGet");
    m->rows = rows;
  } 
//PRI("flag2","%s");
  MccFileClose(f);
//PRI("flag3","%s");   
  // !!!!!!!!!!!!!!!!! OK up to here 
  return m;
}
void MyFree(void *ptr) {free(ptr); ptr=NULL;} /*ptr=NULL doesn't see to work*/

void KillMatrix(MATRIX *x) {
/*
   Destroys *x object.
*/
	long i;
	if (x == (MATRIX *) NULL) {
		//Warning("Attempt to kill NULL matrix in KillMatrix");
		MccErr("In KillMatrix()","Attempt to kill NULL matrix");
		return;
	}
	for (i = 0;  i < x->rows;  i += x->period)
		MyFree(x->elem[i]);
	for (i = 0;  i < x->rows;  i++) 
		if (x->rowlabel[i]) MyFree(x->rowlabel[i]);
	for (i = 0;  i < x->cols;  i++)
		if (x->collabel[i]) MyFree(x->collabel[i]);
	MyFree(x->rowlabel);
	MyFree(x->collabel);
	MyFree(x->elem);
	if (x->name) MyFree(x->name);
	MyFree(x);
}


MATRIXSHORT	*MatrixShortMake(long rows, long cols, long precisionFlag){
	char *name = "MatrixShortMake()";
	long i;
	MATRIXSHORT *x;
	if (rows <= 0  ||  cols <= 0  ||  rows*cols > 2000000000) {
		fprintf(stderr,"rows=%d\n",rows);
		fprintf(stderr,"cols=%d\n",cols);
		fprintf(stderr,"rows*cols=%d > 2000000000\n",rows*cols);
		MccErr(name,"Invalid size");
	}
	x = CALLOC(MATRIXSHORT,1);
	x->rowlabel = (char **) MyCalloc(rows, sizeof(char *), name);
	if(precisionFlag==0) {
 		x->elem = (short **) MyCalloc(rows, sizeof(short *), name); 
		for (i=0;i<rows;i++) x->elem[i] = CALLOC(short,cols); 
		x->elemF = NULL;
	}
	else {
		x->elemF = (REAL **) MyCalloc(rows, sizeof(REAL *), name); 
		for (i=0;i<rows;i++) x->elemF[i] = CALLOC(REAL,cols); 
		x->elem = NULL;
	}
	x->rows = rows;
	x->cols = cols;
	x->rpartSorts = NULL;
	x->rpartWhich = NULL;
	x->target = NULL;
	return x;
}

void MatrixShortKill(MATRIXSHORT *x) {
	char *name="MatrixShortKill()";
	long i;
//PRI("shit1","%s");
	if (x == (MATRIXSHORT *) NULL) {
		//Warning("Attempt to kill NULL matrix in KillMatrix");
		MccErr("In MatrixShortKill()","Attempt to kill NULL matrixShort");
		return;
	}
	for (i=0;i<x->rows;i++) {
		if(x->elem!=NULL) MyFree(x->elem[i]);
		else              MyFree(x->elemF[i]);
		if(x->rowlabel[i]) MyFree(x->rowlabel[i]);
		if(x->rpartSorts!=NULL && i< (x->rows-1) ) MyFree(x->rpartSorts[i]);
	}
	if(x->rpartSorts!=NULL) MyFree(x->rpartSorts);
	if(x->rpartWhich!=NULL) MyFree(x->rpartWhich);
	if(x->target!=NULL) MyFree(x->target);
	MyFree(x->rowlabel);
	if(x->elem!=NULL) MyFree(x->elem);
	else              MyFree(x->elemF);
	MyFree(x);
//PRI("shit2","%s");
}


void TnodeDevianceImprovement(TNODE *x,double *y) {
 char *name = "TnodeDevianceImprovement()";
 double z;
 if(x==NULL || x->left==NULL || x->right==NULL) ;
 else {
  z = x->deviance - x->left->deviance - x->right->deviance;
  y[x->var]+=z;
//PRI(x->var,"%lf");
//PRI(z,"%lf");
  TnodeDevianceImprovement(x->left,y);
  TnodeDevianceImprovement(x->right,y);
 }
}
 
void TnodeDevianceNormalize(MATRIX *x) {
 char *name="TnodeDevianceNormalize()";
 long i,j,m,n;
 double temp;
 n = x->rows-1; //number of trees
 m = x->cols; //number of vars

	vector<vector<float> > vv(n, vector<float>(m));
	for( int i = 0; i < n; ++i )
		for( int j = 0; j < m; ++j )
			vv[i][j] = x->elem[i][j];

 for(j=0;j<m;j++) {  
  for(i=0;i<n;i++) {
   ERR(x->elem[i][j]<= 0.0);
   x->elem[n][j]+=x->elem[i][j]/(double)n;
  }
  for(i=0;i<(n+1);i++) x->elem[i][j]=sqrt(x->elem[i][j]);
 }
 for(i=0;i<(n+1);i++) {
  temp = Maximum(x->elem[i],m);
  for(j=0;j<m;j++) x->elem[i][j]=100.0*x->elem[i][j]/temp;
 }
}


void MccDumpMatrixNew2(MATRIX *m, char *fname, char *mes, long precisionArg, char *rw) {
/*
	Version of MccDumpMatrixNew that doesn't use "global_format", which gave
	bugging problems (unless EnvGet() used). Instead use precisionArg input.
	Also treats NAs
	FUCK: TODO: Use rowlabels and collabels to determine formatting too....MCC 2002
*/
	char *name="MccDumpMatrixNew2()";
//	extern char *global_format;
	char format[20],format_f[20],format_r[20],format_c[20];
	FILE *f;
	long i,j,rowlabel,collabel,*prec,precision=10;
	double meanabs;
	ERR(m==(MATRIX*)NULL);	/* cant dump null matrix */
	/* ERR(m->transpose); */	/* not installed */
	//f=MccFileOpen(fname,"w",TRUE,name);
	f=MccFileOpen(fname,rw,TRUE,name);
	rowlabel=(m->rowlabel[0]!=(char *) NULL);
	collabel=(m->collabel[0]!=(char *) NULL);

/*	if(!strcmp(global_format,"format_10")) precision=10;
	else if(!strcmp(global_format,"precise")) precision=11;
	else if(!strcmp(global_format,"imprecise")) precision=7;
	else precision=10; 	/*default*/
	precision=precisionArg;

	if(!strcmp(mes,"nolabels")) rowlabel=collabel=FALSE;
	else if(!strcmp(mes,"precise")) precision=11;
	else if(!strcmp(mes,"imprecise")) precision=6;
	/*else if(mes[0]) fprintf(f,"%s %s\n",(m->name==(char *)NULL) ? "" : m->name,mes);*/
	else if(mes[0]) fprintf(f,"%s\n",mes);	/*added June2002*/
	else ;

	sprintf(format,"%%%ds",precision);
	sprintf(format_c,"%%%ds ",precision);
	sprintf(format_r,"%%-%ds ",precision);
	sprintf(format_f,"%%%d.6f ",precision);
	/*****************determine col precision***************/
/*DEBUG*/	ERR(!m->cols);
	prec=CALLOC(long,m->cols);
	for(j=0;j<m->cols;j++) {
		meanabs=0.0;
		for(i=0;i<m->rows;i++) meanabs+=fabs(m->elem[i][j]);
		meanabs/=(double)m->rows;
		if(meanabs>100000.) prec[j]=0;
		else if(meanabs>100.) prec[j]=3;
		else prec[j]=6;
		prec[j]+=(precision-10);
		if(prec[j]<0) prec[j]=0;
	}
	/*******************************************************/
	if(collabel) {
		if(rowlabel) fprintf(f,format,"");
		for(i=0;i<m->cols;i++)
			fprintf(f,format_c,m->collabel[i]);
		fprintf(f,"\n");
	}
	for(i=0;i<m->rows;i++) {
		if(rowlabel) 
			fprintf(f,format_r,m->rowlabel[i]);
		for(j=0;j<m->cols;j++) {
			if(precision>=10) format_f[4]='0'+ (char) prec[j];
			else format_f[3]='0'+ (char) prec[j];
			//if((REAL) m->elem[i][j]!=NA) 
			if( m->elem[i][j]!=NA) 
				fprintf(f,format_f,m->elem[i][j]);
			else fprintf(f,format_c,"NA");
		}
		fprintf(f,"\n");
	}
	//fprintf(f," \n");	//added 30Mar02
	free(prec);
	MccFileClose(f);
}




