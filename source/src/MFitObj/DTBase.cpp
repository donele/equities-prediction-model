#include <MFitObj/DTBase.h>
#include <jl_lib/GODBC.h>
#include <math.h>
using namespace std;
vector<int> DTBase::viNode;
vector<Acc> DTBase::vAcc;
vector<vector<DTStat> > DTBase::vvStat;

DTBase::DTBase()
:pRoot_(0),
nrows_(0),
ncols_(0),
start_(0),
end_(1)
{}

DTBase::DTBase(const vector<string>& lines, int iTree)
:pRoot_(0)
{
	for( auto it = lines.begin(); it != lines.end(); ++it )
	{
		vector<string> sl = split(*it);
		if( sl.size() < 2 )
			sl = split(*it, '\t');
		if( !sl.empty() )
		{
			int line_iTree = atoi(sl[0].c_str());
			if( line_iTree == iTree && sl[0].find("num") == string::npos )
				lines_.push_back(sl);
			else if( line_iTree > iTree )
				break;
		}
	}
	iLine_ = 0;
	pRoot_ = make_node();
}

DTBase::DTBase(int maxNodes, int minPtsNode, int maxLevels, int nthreads)
:pRoot_(0),
nrows_(0),
ncols_(0),
start_(0),
end_(1)
{
	maxNodes_ = maxNodes;
	minPtsNode_ = minPtsNode;
	maxLevels_ = maxLevels;
	nodes_.resize(maxNodes);
}

void DTBase::setData(vector<vector<float> >* pvvInputTarget, vector<vector<float> >* pvvInputTargetOOS,
		vector<vector<int> >* pvvInputTargetIndx, vector<float>& vTarget)
{
	pvvInputTarget_ = pvvInputTarget;
	pvvInputTargetOOS_ = pvvInputTargetOOS;
	pvvInputTargetIndx_ = pvvInputTargetIndx;
	pvTarget_ = &vTarget;

	nrows_ = pvvInputTarget->size();
	ncols_ = (*pvvInputTarget)[0].size();
	vInput_.resize(nrows_ - 1);
}

void DTBase::setDataIns(vector<vector<float> >* pvvInputTarget)
{
	pvvInputTarget_ = pvvInputTarget;
	nrows_ = pvvInputTarget->size();
	ncols_ = (*pvvInputTarget)[0].size();
	vInput_.resize(nrows_ - 1);
}

void DTBase::setDataOOS(vector<vector<float> >* pvvInputTarget)
{
	pvvInputTargetOOS_ = pvvInputTarget;
	nrows_ = pvvInputTarget->size();
	ncols_ = (*pvvInputTarget)[0].size();
	vInput_.resize(nrows_ - 1);
}

bool DTBase::fit_node_create()
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
		int which = DTBase::viNode[ic];
		DTNode* z = nodes_[which];
		if( z->left == NULL ) // terminal node.
			continue;

		double tmpX = (*pvvInputTarget_)[z->var][ic];
		if( tmpX < z->cut )
			DTBase::viNode[ic] = z->left->rpartNodeNum;
		else
			DTBase::viNode[ic] = z->right->rpartNodeNum;
	}

	start_ = end_;
	end_ += newNodes;
	if( newNodes == 0 || end_ >= maxNodes_ )
		return false;
	return true;
}

DTNode* DTBase::make_node()
{
	vector<string>& thisLine = lines_[iLine_++];

	DTNode* pnode = new DTNode();
	pnode->mu = atof(thisLine[5].c_str());
	pnode->deviance = atof(thisLine[6].c_str());
	pnode->indnum = atoi(thisLine[4].c_str());
	pnode->var = atof(thisLine[2].c_str());
	pnode->cut = atof(thisLine[3].c_str());

	if( pnode->var > -1 ) // not terminal.
	{
		pnode->left = make_node();
		pnode->right = make_node();
	}

	return pnode;
}

double DTBase::TnodeCutFn(double y0,double y1,double y2,double yy0,double yy1,
				  double yy2,int indnum,int cutpts,int weirdFactor)
{
/*
	See Hastie et. al. "Elements of Statitical Learning p 270 and
	Chambers & Hastie. "Statistical models in S" p 414
*/
	double left_var,right_var;	/* cond variances*npts */
	double left_tstat,right_tstat;

	if(y0< cutpts/weirdFactor || y0>indnum-cutpts/weirdFactor ) return(-max_double_);	//added 20060126

	left_var=y2-(y1*y1/y0);
	right_var=(yy2-y2)-((yy1-y1)*(yy1-y1)/(yy0-y0));

	return(-left_var-right_var);
}

// -------------------------------------------------------------------------------
// Dump
// -------------------------------------------------------------------------------

void DTBase::dump(ofstream& ofs, int iTree, double weight)
{
	dump_node(ofs, pRoot_, iTree, weight);
}

void DTBase::dump_node(ofstream& ofs, DTNode* z, int iTree, double weight)
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
		sprintf(buf, "%4d %7.4f %3d %12.6f %10d %10.5f %15.4f\n", iTree, weight, var, cut, z->indnum, z->mu, z->deviance);
		ofs << buf;
		dump_node(ofs, z->left, iTree, weight);
		dump_node(ofs, z->right, iTree, weight);
	}
}

// -------------------------------------------------------------------------------
// get_pred()
// -------------------------------------------------------------------------------

double DTBase::get_pred(int ic)
{
	// Input vector for column ic.
	for( int ir = 0; ir < nrows_ - 1; ++ir )
		vInput_[ir] = (*pvvInputTarget_)[ir][ic];

	return traverse_mu(pRoot_);
}

double DTBase::get_pred_oos(int ic)
{
	// Input vector for column ic.
	for( int ir = 0; ir < nrows_ - 1; ++ir )
		vInput_[ir] = (*pvvInputTargetOOS_)[ir][ic];

	return traverse_mu(pRoot_);
}

double DTBase::traverse_mu(DTNode* pnode)
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

double DTBase::get_deviance()
{
	double sumDeviance = 0.;
	int npts = 0;
	traverse_deviance(pRoot_, sumDeviance, npts);

	double avgDeviance = 0.;
	if( npts > 0 )
		avgDeviance = sqrt(sumDeviance / npts);
	return avgDeviance;
}

void DTBase::traverse_deviance(DTNode* pnode, double& sumDeviance, int& npts)
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

void DTBase::deviance_improvement(vector<double>& stats)
{
	traverse_improvement(pRoot_, stats);
}

void DTBase::traverse_improvement(DTNode* pnode, vector<double>& stats)
{
	if( pnode != 0 && pnode->left != 0 && pnode->right != 0 )
	{
		double z = pnode->deviance - pnode->left->deviance - pnode->right->deviance;
		stats[pnode->var] += z;
		traverse_improvement(pnode->left, stats);
		traverse_improvement(pnode->right, stats);
	}
}

// -------------------------------------------------------------------------------
// write_tree()
// -------------------------------------------------------------------------------

void DTBase::write_tree(const string& dbname, const string& tablename, const string& mCode,
		int dbDate, int iTree, float wgt)
{
	dbname_ = dbname;
	tablename_ = tablename;
	mCode_ = mCode;
	dbDate_ = dbDate;
	iTree_ = iTree;
	wgt_ = wgt;

	// delete the table.
	char chk[400];
	char cmd[400];
	sprintf(chk, "select count(*) from %s where idate = %d and market = '%s' and treeNumber = %d ", tablename_.c_str(), dbDate_, mCode_.c_str(), iTree_);
	check_and_exit(dbname_, chk);

	// Write recursively.
	int count = 0;
	write_node(pRoot_, count);
}

void DTBase::write_node(DTNode* pnode, int& count)
{
	if( pnode != 0 )
	{
		int var = -1;
		double cut = 0.;
		if( pnode->left != 0 ) // not a terminal node.
		{
			var = pnode->var;
			cut = pnode->cut;
		}

		char cmd[1000];
		sprintf(cmd, "insert into %s (idate, market, cutVariable, cutValue, nPts, tstat, mu, deviance, prune, idx, treeNumber, treeWt) "
			" values (%d, '%s', %d, %f, %d, %f, %f, %f, %f, %d, %d, %f) ",
			tablename_.c_str(), dbDate_, mCode_.c_str(), var, cut,
			pnode->indnum, pnode->tstat, pnode->mu, pnode->deviance, 0.,
			count, iTree_, wgt_);
		//GODBC::Instance()->get(dbname_)->ExecDirect(cmd);
		GODBC::Instance()->exec(dbname_, cmd);

		++count;
		write_node(pnode->left, count);
		write_node(pnode->right, count);
	}
}

vector<string> DTBase::get_query(const string& mCode, int dbDate, int iTree, float wgt)
{
	mCode_ = mCode;
	dbDate_ = dbDate;
	iTree_ = iTree;
	wgt_ = wgt;

	// Append recursively.
	vector<string> v;
	int count = 0;
	append_query(v, pRoot_, count);

	return v;
}

void DTBase::append_query(vector<string>& v, DTNode* pnode, int& count)
{
	if( pnode != 0 )
	{
		int var = -1;
		double cut = 0.;
		if( pnode->left != 0 ) // not a terminal node.
		{
			var = pnode->var;
			cut = pnode->cut;
		}

		char cmd[1000];
		sprintf(cmd,
			"%d,'%s',%d,%f,%d,%f,%f,%f,%f,%d,%d,%f",
			dbDate_, mCode_.c_str(), var, cut,
			pnode->indnum, pnode->tstat, pnode->mu, pnode->deviance, 0.,
			count, iTree_, wgt_);
		v.push_back(cmd);

		++count;
		append_query(v, pnode->left, count);
		append_query(v, pnode->right, count);
	}
}
