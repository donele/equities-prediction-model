#include <MFitObj/DTMT.h>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
using namespace std;

DTMT::DTMT(const vector<string>& lines, int iTree)
:DTBase(lines, iTree)
{
}

DTMT::DTMT(int maxNodes, int minPtsNode, int treeMaxLevels, int nthreads)
:DTBase(maxNodes, minPtsNode, treeMaxLevels, nthreads),
single_thread_(false)
{
}

void DTMT::fit()
{
	DTBase::viNode.resize(ncols_);
	std::fill(DTBase::viNode.begin(), DTBase::viNode.end(), 0);

	DTBase::vAcc.resize(maxNodes_);
	DTBase::vvStat.resize(nrows_ - 1);
	for( int i = 0; i < nrows_ - 1; ++i )
		vvStat[i].resize(maxNodes_);

	pRoot_ = new DTNode();
	nodes_[0] = pRoot_;
	start_ = 0;
	end_ = 1;
	for( int il = 0; il < maxLevels_; ++il )
	{
		fit_node_stat();
		fit_node_cut();
		if( !fit_node_create() )
			break;
	}

	// if end > start, compute stats on any nodes which would have been non-terminal
	fit_node_stat();
}

void DTMT::fit_node_stat()
{
	// Initialize vAcc.
	for( int in = start_; in < end_; ++in )
		vAcc[in].clear();

	// Update vAcc.
	for( int ic = 0; ic < ncols_; ++ic )
	{
		int iNode = DTBase::viNode[ic]; // The data belong to which node.
		if( start_ <= iNode && iNode < end_ )
		{
			double tmp = (*pvTarget_)[ic];
			Acc& acc = DTBase::vAcc[iNode];
			acc.add(tmp);
		}
	}

	// Update mu, deviance, and tstat of the nodes.
	for( int in = start_; in < end_; ++in )
	{
		DTNode* node = nodes_[in];
		const Acc& acc = DTBase::vAcc[in];

		node->indnum = acc.n;
		if( acc.n >= 2 )
		{
			node->mu = acc.mean();
			double var = acc.var();
			node->deviance = var * acc.n; // deviance = nPts * variance is minimized.
			node->tstat = (var > 0.) ? acc.sum / sqrt(var) : 0.;
		}
		else
		{
			node->mu = 0.;
			node->deviance = 0.;
			node->tstat = 0.;
		}
	}
}

void DTMT::fit_node_cut()
{
	// Loop inputs. (to be multithreaded.)
	if( single_thread_ )
	{
		for( int ir = 0; ir < nrows_ - 1; ++ir )
			fit_node_cut_input(ir);
	}
	else // multi thread. (test result 35 -> 10)
	{
		list<boost::shared_ptr<boost::thread> > listThread;
		for( int ir = 0; ir < nrows_ - 1; ++ir )
		{
			listThread.push_back(
				boost::shared_ptr<boost::thread>(
					new boost::thread(boost::bind(&DTMT::fit_node_cut_input, this, ir))
				)
			);
		}

		// join ticker threads.
		for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
			(*it)->join();
	}

	// Loop nodes and find the input with max(diffmax).
	if( single_thread_ )
	{
		for( int in = start_; in < end_; ++in )
			fit_node_cut_recombine(in);
	}
	else // multi thread. (test result 10 -> 10)
	{
		list<boost::shared_ptr<boost::thread> > listThread;
		for( int in = start_; in < end_; ++in )
		{
			listThread.push_back(
				boost::shared_ptr<boost::thread>(
					new boost::thread(boost::bind(&DTMT::fit_node_cut_recombine, this, in))
				)
			);
		}

		// join ticker threads.
		for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
			(*it)->join();
	}
}

void DTMT::fit_node_cut_input(int ir)
{
	// Initialize vvStat.
	for( int in = start_; in < end_; ++in )
		vvStat[ir][in].clear();

	// Loop columns.
	vector<int>& vIndx = (*pvvInputTargetIndx_)[ir];
	for( int ic = 0; ic < ncols_; ++ic )
	{
		int cIndx = vIndx[ic];
		int iNode = DTBase::viNode[cIndx];
		if( iNode < start_ )
			continue;

		DTStat& stat = vvStat[ir][iNode];
		const Acc& tot = vAcc[iNode];

		//tmpY is the target corresponding to the input tmpX below
		double tmpX = (*pvvInputTarget_)[ir][cIndx];
		double tmpY = (*pvTarget_)[cIndx];

		//If there is a genuine change in the ordered input data, compute TnodeCutFn used to make cutting decision
		double diff = -max_double_;
		if( ic == 0 || tmpX > stat.tmpXold )
			diff = TnodeCutFn(stat.acc, tot, minPtsNode_, 1);

		//update count y0, sum y1 and sumsqures y2 AFTER computing diff (the sum of the deviances of the 2 nodes if variable "j" is cut at < tmpX 
		stat.acc.add(tmpY);

		//TnodeCutFn has returned a new optimal value, so store tnode->cut and tnode->var
		if( diff > stat.diffmax )
		{
			//z->leftMax=(long) z->y0 -1 ;	//doesn't work because of ties
			stat.leftMax = stat.acc.n - 1;
			stat.diffmax = diff;
			stat.cut = (stat.tmpXold + tmpX) / 2.;
			stat.var = ir;
		}
		stat.tmpXold = tmpX;
	}
}

void DTMT::fit_node_cut_recombine(int in)
{
	int indx = -1;
	double diffmax = -max_double_;
	for( int ir = 0; ir < nrows_ - 1; ++ir )
	{
		const DTStat& stat = vvStat[ir][in];
		if( stat.diffmax > diffmax )
		{
			indx = ir;
			diffmax = stat.diffmax;
		}
	}

	if( indx >= 0 )
	{
		const DTStat& stat = vvStat[indx][in];
		DTNode* z = nodes_[in];
		z->leftMax = stat.leftMax;
		z->diffmax = stat.diffmax;
		z->cut = stat.cut;
		z->var = stat.var;
	}
}

double DTMT::TnodeCutFn(const Acc& acc, const Acc& tot, int cutpts, int weirdFactor)
{
/*
	See Hastie et. al. "Elements of Statitical Learning p 270 and
	Chambers & Hastie. "Statistical models in S" p 414
*/

	if(acc.n < cutpts / weirdFactor || acc.n > tot.n - cutpts / weirdFactor )
		return(-max_double_);

	double left_var = acc.sum2 - (acc.sum * acc.sum / acc.n);
	double right_var = (tot.sum2 - acc.sum2) - ((tot.sum - acc.sum) * (tot.sum - acc.sum) / (tot.n - acc.n));

	return(-left_var - right_var);
}
