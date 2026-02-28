#include <MFitObj/DT.h>
#include <MFitObj.h>
#include <math.h>
using namespace std;

vector<int> DT::rpartWhich;

DT::DT()
:pRoot_(0),
nrows_(0),
ncols_(0),
start_(0),
end_(1)
{}

DT::DT(int maxNodes, int minPtsNode, int maxLevels, int nthreads)
:pRoot_(0),
nrows_(0),
ncols_(0),
start_(0),
end_(1)
{
	maxNodes_ = maxNodes;
	minPtsNode_ = minPtsNode;
	maxLevels_ = maxLevels;
	nthreads_ = nthreads;
	nodes_.resize(maxNodes);
}

void DT::setData(vector<vector<float> >* pvvInputTarget, vector<vector<float> >* pvvInputTargetOOS,
		vector<vector<int> >* pvvInputTargetIndx, vector<float>& vTarget)
{
	pvvInputTarget_ = pvvInputTarget;
	pvvInputTargetOOS_ = pvvInputTargetOOS;
	pvvInputTargetIndx_ = pvvInputTargetIndx;
	//pvvInputTargetOOSIndx_ = pvvInputTargetOOSIndx;
	pvTarget_ = &vTarget;

	nrows_ = pvvInputTarget->size();
	ncols_ = (*pvvInputTarget)[0].size();
	vInput_.resize(nrows_ - 1);
}

// -------------------------------------------------------------------------------
// Functions for fitting:
//  fit()
//  fit_node_stat()
//  fit_node_cut()
//  fit_node_create()
//  TnodeCutFn()
// -------------------------------------------------------------------------------

void DT::fit()
{
	DT::rpartWhich.resize(ncols_);
	//memset((void*)&DT::rpartWhich[0], 0, ncols_ * sizeof(int));
	//memset(&v[0], 0, rpartWhich.size() * sizeof rpartWhich[0]);
	std::fill(DT::rpartWhich.begin(), DT::rpartWhich.end(), 0);

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

void DT::fit_node_stat()
{
	for( int ic = 0; ic < ncols_; ++ic )
	{
		int which = DT::rpartWhich[ic]; // The data belong to which node.
		if( which < start_ ) // compute stats on new trees only.
			continue;
		if( which >= end_ )
			exit(7);
		DTNode* node = nodes_[which];

		//double tmp = (*pvvInputTarget_)[nrows_ - 1][ic];
		double tmp = (*pvTarget_)[ic];
		++node->y0;
		node->y1 += tmp;
		node->y2 += tmp * tmp;
	}

	for( int in = start_; in < end_; ++in )
	{
		DTNode* node = nodes_[in];
		node->indnum = node->y0;
		if( node->y0 >= 2 )
		{
			node->mu = node->y1 / node->y0;
			node->deviance = node->y2 - node->mu * node->mu * node->y0; // deviance = nPts * variance is minimized.
			double var = node->y2 - (node->y1 * node->mu);
			if( var > 0. )
				node->tstat = node->y1 / sqrt(var);
			else
				node->tstat = 0.;
		}
		else
		{
			node->mu = 0.;
			node->deviance = 0.;
			node->tstat = 0.;
		}
	}
}

void DT::fit_node_cut()
{
	// Initialize.
	for( int in = start_; in < end_; ++in )
	{
		DTNode* z = nodes_[in];
		z->yy0 = z->y0;
		z->yy1 = z->y1;
		z->yy2 = z->y2;
		z->var = -1.;
		z->cut = -max_double_;
		z->diffmax = -max_double_;
	}

	// Loop inputs.
	for( int ir = 0; ir < nrows_ - 1; ++ir )
	{
		vector<int>& vIndx = (*pvvInputTargetIndx_)[ir];
		for( int in = start_; in < end_; ++in )
		{
			DTNode* z = nodes_[in];
			z->y0 = 0.;
			z->y1 = 0.;
			z->y2 = 0.;
			z->tmpXold = -max_double_;
		}
		for( int ic = 0; ic < ncols_; ++ic )
		{
			int cIndx = vIndx[ic];
			int which = DT::rpartWhich[cIndx];
			if( which < start_ )
				continue;

			//tmpY is the target corresponding to the input tmpX below
			double tmpX = (*pvvInputTarget_)[ir][cIndx];
			//double tmpY = (*pvvInputTarget_)[nrows_ - 1][cIndx];
			double tmpY = (*pvTarget_)[cIndx];

			DTNode* z = nodes_[which];
			//If there is a genuine change in the ordered input data, compute TnodeCutFn used to make cutting decision
			double diff = -max_double_;
			if( ic == 0 || tmpX > z->tmpXold )
				diff = TnodeCutFn(z->y0, z->y1, z->y2, z->yy0, z->yy1, z->yy2, (long)z->y0, z->indnum, minPtsNode_, 1);
			z->tmpXold = tmpX;

			//update count y0, sum y1 and sumsqures y2 AFTER computing diff (the sum of the deviances of the 2 nodes if variable "j" is cut at < tmpX 
			++z->y0;
			z->y1 += tmpY;
			z->y2 += tmpY * tmpY;

			//TnodeCutFn has returned a new optimal value, so store tnode->cut and tnode->var
			if( diff > z->diffmax )
			{
				//z->leftMax=(long) z->y0 -1 ;	//doesn't work because of ties
				z->diffmax = diff;
				z->cut = z->tmpXold;
				z->var = ir;
			}
		}
	}

	// leftMax.
	for( int ic = 0; ic < ncols_; ++ic )
	{
		int which = DT::rpartWhich[ic];
		if( which < start_ )
			continue;
		DTNode* z = nodes_[which];
		if( z->var >= 0 )
		{
			double tmpX = (*pvvInputTarget_)[z->var][ic];
			if( tmpX < z->cut )
				++z->leftMax;
		}
	}
}

bool DT::fit_node_create()
{
	int newNodes = 0;
	for( int in = start_; in < end_; ++in )
	{
		DTNode* z = nodes_[in];
		if( z->leftMax >= minPtsNode_ && z->leftMax <= z->indnum - minPtsNode_ && end_ + newNodes < maxNodes_ )
		{
			z->left = new DTNode();
			z->right = new DTNode();
			z->left->rpartNodeNum = end_ + newNodes;
			z->right->rpartNodeNum = z->left->rpartNodeNum + 1;
			nodes_[end_ + newNodes] = z->left;
			nodes_[end_ + newNodes + 1] = z->right;
			z->left->level = z->level + 1;
			z->left->parent = z;
			z->right->level = z->level + 1;
			z->right->parent = z;
			newNodes += 2;
		}
	}

	//Update data->rPartWhich used to identify which node a data point belongs to.
	for( int ic = 0; ic < ncols_; ++ic )
	{
		int which = DT::rpartWhich[ic];
		DTNode* z = nodes_[which];
		if( z->left == NULL ) // terminal node.
			continue;

		double tmpX = (*pvvInputTarget_)[z->var][ic];
		if( tmpX < z->cut )
			DT::rpartWhich[ic] = z->left->rpartNodeNum;
		else
			DT::rpartWhich[ic] = z->right->rpartNodeNum;
	}

	start_ = end_;
	end_ += newNodes;
	if( newNodes == 0 || end_ >= maxNodes_ )
		return false;
	return true;
}

#define DIFF_SPLUS 1	
double DT::TnodeCutFn(double y0,double y1,double y2,double yy0,double yy1,
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
	if(i< cutpts/weirdFactor || i>indnum-cutpts/weirdFactor ) return(-max_double_);	//added 20060126

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

// -------------------------------------------------------------------------------
// Dump
// -------------------------------------------------------------------------------

void DT::dump(string filename, double weight)
{
	ofstream ofs(filename.c_str());
	char buf[1000];
	sprintf(buf, "%7s %3s %12s %10s %10s %15s\n", "weight", "var", "varCut<", "nPts", "mu", "deviance");
	ofs << buf;
	dump_node(ofs, pRoot_, weight);
}

void DT::dump_node(ofstream& ofs, DTNode* z, double weight)
{
	int var = -1;
	double cut = 0.;
	if( z != 0 )
	{
		if( z->left != 0 ) // not a terminal node.
		{
			var = z->var;
			cut = z->cut;
		}
		char buf[1000];
		sprintf(buf, "%7.4f %3d %12.6f %10d %10.5f %15.4f\n", weight, var, cut, z->indnum, z->mu, z->deviance);
		ofs << buf;
		dump_node(ofs, z->left, weight);
		dump_node(ofs, z->right, weight);
	}
}

// -------------------------------------------------------------------------------
// get_pred()
// -------------------------------------------------------------------------------

double DT::get_pred(int ic)
{
	// Input vector for column ic.
	for( int ir = 0; ir < nrows_ - 1; ++ir )
		vInput_[ir] = (*pvvInputTarget_)[ir][ic];

	return traverse_mu(pRoot_);
}

double DT::get_pred_oos(int ic)
{
	// Input vector for column ic.
	for( int ir = 0; ir < nrows_ - 1; ++ir )
		vInput_[ir] = (*pvvInputTargetOOS_)[ir][ic];

	return traverse_mu(pRoot_);
}

double DT::traverse_mu(DTNode* pnode)
{
	if( pnode->left != 0 )
	{
		if( vInput_[pnode->var] < pnode->cut )
			return traverse_mu(pnode->left);
		else
			return traverse_mu(pnode->right);
	}
	return pnode->mu;
}

// -------------------------------------------------------------------------------
// get_deviance()
// -------------------------------------------------------------------------------

double DT::get_deviance()
{
	double sumDeviance = 0.;
	int npts = 0;
	traverse_deviance(pRoot_, sumDeviance, npts);

	double avgDeviance = 0.;
	if( npts > 0 )
		avgDeviance = sqrt(sumDeviance / npts);
	return avgDeviance;
}

void DT::traverse_deviance(DTNode* pnode, double& sumDeviance, int& npts)
{
	if( pnode->left == 0 || pnode->right == 0 ) // terminal.
	{
		sumDeviance += pnode->deviance;
		npts += pnode->indnum;
	}
	else
	{
		traverse_deviance(pnode->left, sumDeviance, npts);
		traverse_deviance(pnode->right, sumDeviance, npts);
	}
}

// -------------------------------------------------------------------------------
// deviance_improvement()
// -------------------------------------------------------------------------------

void DT::deviance_improvement(vector<double>& stats)
{
	traverse_improvement(pRoot_, stats);
}

void DT::traverse_improvement(DTNode* pnode, vector<double>& stats)
{
	if( pnode != 0 && pnode->left != 0 && pnode->right != 0 )
	{
		double z = pnode->deviance - pnode->left->deviance - pnode->right->deviance;
		stats[pnode->var] += z;
		traverse_improvement(pnode->left, stats);
		traverse_improvement(pnode->right, stats);
	}
}
