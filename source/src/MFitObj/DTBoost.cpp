#include <MFitObj/DTBoost.h>
#include <MFitObj.h>
#include <gtlib_model/pathFtns.h>
#include <boost/filesystem.hpp>
#include <string>
using namespace std;
using namespace gtlib;

DTBoost::DTBoost()
:pvvInputTarget_(0),
pvvInputTargetOOS_(0),
udate_(0),
dbDate_(0),
evQuantile_(0)
{}

DTBoost::DTBoost(int ntrees)
:ntrees_(ntrees),
pvvInputTarget_(0),
pvvInputTargetOOS_(0),
udate_(0),
dbDate_(0),
evQuantile_(0)
{}

DTBoost::DTBoost(int ntrees, bool isResTar, double evQuantile)
:ntrees_(ntrees),
interval_ana_(1),
isResTar_(isResTar),
evQuantile_(evQuantile)
{
	if( ntrees_ > 20 )
		interval_ana_ = 5;
	else
		interval_ana_ = 2;
	n_tree_points_ = ntrees_ / interval_ana_;
}

DTBoost::DTBoost(int ntrees, double shrinkage, int maxNodes, int minPtsNode, int maxLevels, int nthreads)
:ntrees_(ntrees),
interval_ana_(1),
shrinkage_(shrinkage),
pvvInputTarget_(0),
pvvInputTargetOOS_(0),
udate_(0),
dbDate_(0),
evQuantile_(0.01)
{
	if( ntrees_ > 20 )
		interval_ana_ = 5;
	else
		interval_ana_ = 2;
	n_tree_points_ = ntrees_ / interval_ana_;

	wts_.resize(ntrees);

	for( int i = 0; i < ntrees; ++i )
		dts_.push_back(new DTMT(2 * maxNodes - 1, minPtsNode, maxLevels, nthreads));
}

void DTBoost::setData(vector<vector<float> >* pvvInputTarget, vector<vector<float> >* pvvInputTargetOOS,
		vector<vector<int> >* pvvInputTargetIndx)
{
	pvvInputTarget_ = pvvInputTarget;
	pvvInputTargetOOS_ = pvvInputTargetOOS;

	for( vector<DTBase*>::iterator it = dts_.begin(); it != dts_.end(); ++it )
		(*it)->setData(pvvInputTarget, pvvInputTargetOOS, pvvInputTargetIndx, vTarget_);
}

void DTBoost::setDir(const string& fitDir, int udate)
{
	string coefDir = fitDir + "/coef";
	setDir(fitDir, coefDir, udate);
}

void DTBoost::setDir(const string& fitDir, const string& coefDir, int udate)
{
	udate_ = udate;
	coefDir_ = coefDir;
	statDir_ = fitDir + xpf("\\stat_") + itos(udate);
	predDir_ = fitDir + xpf("\\preds");
	predInsDir_ = fitDir + xpf("\\predsIns");
	mkd(coefDir_);
	mkd(statDir_);
}

void DTBoost::fit()
{
	// Make a copy the targets.
	int nrows = pvvInputTarget_->size();
	int ncols = (*pvvInputTarget_)[0].size();
	int ncolsOOS = (*pvvInputTargetOOS_)[0].size();
	vTarget_.resize(ncols);
	for( int ic = 0; ic < ncols; ++ic )
		vTarget_[ic] = (*pvvInputTarget_)[nrows - 1][ic];

	Perf perf(statDir_, ncols, ncolsOOS);
	bool oosEmpty = pvvInputTargetOOS_ == 0 || pvvInputTargetOOS_->empty() || (*pvvInputTargetOOS_)[0].empty();
	for( int iTree = 0; iTree < ntrees_; ++iTree )
	{
		cout << "iTree " << iTree << endl;

		if( iTree > 0 )
			calcResiduals(iTree);

		// oos performance.
		if( !oosEmpty && (iTree % 5 == 0 || iTree == ntrees_ - 1) )
			perf.run(dts_, wts_, iTree, vTarget_, pvvInputTarget_, pvvInputTargetOOS_);

		// Fit.
		dts_[iTree]->fit();
		wts_[iTree] = shrinkage_;
	}

	// Dump.
	char filename[1000];
	sprintf(filename, "%s\\coef%d.txt", coefDir_.c_str(), udate_);
	ofstream ofs(xpf(filename).c_str());

	char buf[1000];
	sprintf(buf, "%4s %7s %3s %12s %10s %10s %15s\n", "num", "weight", "var", "varCut<", "nPts", "mu", "deviance");
	ofs << buf;

	for( int iTree = 0; iTree < ntrees_; ++iTree )
		dts_[iTree]->dump(ofs, iTree, wts_[iTree]);
}

void DTBoost::calcResiduals(int iTree)
{
	// Subtract prediction of dts[iTree - 1] from the target.
	int ncols = (*pvvInputTarget_)[0].size();
	for( int ic = 0; ic < ncols; ++ic )
	{
		double pred = dts_[iTree - 1]->get_pred(ic);
		double wgt = wts_[iTree - 1];
		vTarget_[ic] -= pred * wgt;
	}
}

void DTBoost::stat(const vector<string>& inputNames)
{
	int nInput = pvvInputTarget_->size() - 1;
	vector<vector<double> > stats(ntrees_ + 1, vector<double>(nInput));

	// Calculate.
	for( int it = 0; it < ntrees_; ++it )
		dts_[it]->deviance_improvement(stats[it]);

	// Normalize.
	for( int it = 0; it < ntrees_; ++it )
		for( int ic = 0; ic < nInput; ++ic )
			stats[ntrees_][ic] += stats[it][ic] / ntrees_;

	for( int it = 0; it < ntrees_ + 1; ++it )
		for( int ic = 0; ic < nInput; ++ic )
			stats[it][ic] = sqrt(stats[it][ic]);

	for( int it = 0; it < ntrees_ + 1; ++it )
	{
		double maxElem = *max_element(stats[it].begin(), stats[it].end());
		for( int ic = 0; ic < nInput; ++ic )
			stats[it][ic] = 100. * stats[it][ic] / maxElem;
	}

	// Write.
	char filename[1000];
	sprintf(filename, "%s\\stat.txt", statDir_.c_str());
	ofstream ofs(xpf(filename).c_str());

	// first row.
	ofs << "    ";
	for( auto it = inputNames.begin(); it != inputNames.end(); ++it )
		ofs << " " << *it;
	ofs << "\n";

	// other rows.
	for( int it = 0; it < ntrees_ + 1; ++it )
	{
		char rowLabel[10];
		if( it == ntrees_ )
			sprintf(rowLabel, " mu");
		else
			sprintf(rowLabel, "%3d", it);
		ofs << rowLabel;

		for( int ic = 0; ic < nInput; ++ic )
		{
			char buf[10];
			sprintf(buf, " %6.2f", stats[it][ic]);
			ofs << buf;
		}
		ofs << "\n";
	}
}

void DTBoost::analyze(vector<int>& testDates, vector<vector<float> >& vvInputTarget, vector<vector<float> >& vvSpectator,
					  map<int, pair<int, int> >& mOffset, const string& model, const string& sample, bool rollingModelDate)
{
	//{
	//	//AnaObject tempAna5("5", 0., 5., evQuantile_);
	//	//AnaObject tempAna10("10", 5., 10., evQuantile_);
	//	//AnaObject tempAna20("20", 10., 20., evQuantile_);
	//	AnaObject tempAnaT("t", 0., 20., evQuantile_);
	//	AnaObject tempAnaW("w", 20., 100., evQuantile_);
	//	//AnaObject tempAnaE("e", 100., 1000., evQuantile_);
	//	//AnaObject tempAnaL("L", -1e-8, 1e-8, evQuantile_);
	//	//AnaObject tempAnaC("C", -100, -1e-8, evQuantile_);

	//	tempAnaT.setMarket(model);
	//	tempAnaW.setMarket(model);

	//	//vector<AnaObject> vAnaObj5(n_tree_points_, tempAna5);
	//	//vector<AnaObject> vAnaObj10(n_tree_points_, tempAna10);
	//	//vector<AnaObject> vAnaObj20(n_tree_points_, tempAna20);
	//	vector<AnaObject> vAnaObjT(n_tree_points_, tempAnaT);
	//	vector<AnaObject> vAnaObjW(n_tree_points_, tempAnaW);
	//	//vector<AnaObject> vAnaObjE(n_tree_points_, tempAnaE);
	//	//vector<AnaObject> vAnaObjL(n_tree_points_, tempAnaL);
	//	//vector<AnaObject> vAnaObjC(n_tree_points_, tempAnaC);

	//	for( int i = 0; i < n_tree_points_; ++i )
	//	{
	//		for( double th = 0.; th < 1.; th += 0.1 )
	//		{
	//			//vAnaObj5[i].addThres(th);
	//			//vAnaObj10[i].addThres(th);
	//			//vAnaObj20[i].addThres(th);
	//			vAnaObjT[i].addThres(th);
	//			vAnaObjW[i].addThres(th);
	//			//vAnaObjE[i].addThres(th);
	//			//vAnaObjL[i].addThres(th);
	//			//vAnaObjC[i].addThres(th);
	//		}
	//	}

	//	// Write in the old format.
	//	char fout1[200];
	//	sprintf(fout1, "%s\\%s_%d_%d.txt", statDir_.c_str(), sample.c_str(), testDates[0], testDates[testDates.size() - 1]);
	//	ofstream ofs1(xpf(fout1).c_str());
	//	vAnaObjT[0].write_header(ofs1);

	//	// Loop test dates.
	//	int prev_modelDate = 0;
	//	std::vector<double> wts;
	//	std::vector<DTBase*> dts;
	//	for( vector<int>::iterator it = testDates.begin(); it != testDates.end(); ++it )
	//	{
	//		// udate.
	//		int testDate = *it;
	//		int modelDate = udate_;
	//		if( "oos" == sample && rollingModelDate )
	//			modelDate = get_modelDate(coefDir_, testDate);

	//		if( modelDate != prev_modelDate )
	//		{
	//			prev_modelDate = modelDate;
	//			wts.clear();
	//			dts.clear();

	//			// read model.
	//			string filename = coefDir_ + "\\coef" + itos(modelDate) + ".txt";
	//			ifstream ifs(xpf(filename).c_str());
	//			vector<string> lines;
	//			string line;
	//			while( getline(ifs, line) )
	//				lines.push_back(line);

	//			for( int iTree = 0; iTree < ntrees_; ++iTree )
	//			{
	//				dts.push_back(new DTMT(lines, iTree));

	//				for( vector<string>::iterator it = lines.begin(); it != lines.end(); ++it )
	//				{
	//					vector<string> sl = split(*it);
	//					if( sl.size() < 2 )
	//						sl = split(*it, '\t');
	//					if( sl[0] == itos(iTree) )
	//					{
	//						wts.push_back( atof(sl[1].c_str()) );
	//						break;
	//					}
	//				}
	//			}
	//		}

	//		// Set data.
	//		for( vector<DTBase*>::iterator it = dts.begin(); it != dts.end(); ++it )
	//		{
	//			if( "ins" == sample )
	//				(*it)->setDataIns(&vvInputTarget);
	//			else if( "oos" == sample )
	//				(*it)->setDataOOS(&vvInputTarget);
	//		}

	//		// find offset.
	//		int offset1 = mOffset[testDate].first;
	//		int offset2 = mOffset[testDate].second;

	//		// Add bm to target and pred if appropriate.
	//		if( isResTar_ )
	//		{
	//			for( int ic = offset1; ic < offset2; ++ic )
	//			{
	//				vvSpectator[0][ic] += vvSpectator[1][ic]; // target is already read. add to it.
	//				vvSpectator[2][ic] = vvSpectator[1][ic]; // pred will be calculated later and added to this.
	//			}
	//		}

	//		// get preds.
	//		for( int iTree = 0; iTree < ntrees_; ++iTree )
	//		{
	//			// Calculate pred.
	//			for( int ic = offset1; ic < offset2; ++ic )
	//			{
	//				if( "ins" == sample )
	//					vvSpectator[2][ic] += dts[iTree]->get_pred(ic) * wts[iTree];
	//				if( "oos" == sample )
	//					vvSpectator[2][ic] += dts[iTree]->get_pred_oos(ic) * wts[iTree];
	//			}

	//			int anaIndx = get_ana_indx(iTree);
	//			if( anaIndx >= 0 )
	//			{
	//				// Analyze.
	//				//vAnaObj5[anaIndx].beginDay(testDate, vvSpectator, offset1, offset2);
	//				//vAnaObj10[anaIndx].beginDay(testDate, vvSpectator, offset1, offset2);
	//				//vAnaObj20[anaIndx].beginDay(testDate, vvSpectator, offset1, offset2);
	//				vAnaObjT[anaIndx].beginDay(testDate, vvSpectator, offset1, offset2);
	//				vAnaObjW[anaIndx].beginDay(testDate, vvSpectator, offset1, offset2);
	//				//vAnaObjE[anaIndx].beginDay(testDate, vvSpectator, offset1, offset2);
	//				//vAnaObjL[anaIndx].beginDay(testDate, vvSpectator, offset1, offset2);
	//				//vAnaObjC[anaIndx].beginDay(testDate, vvSpectator, offset1, offset2);

	//				// Write old format.
	//				int idate = *it;
	//				vAnaObjT[anaIndx].write_by_date(ofs1, idate, 0., iTree + 1);
	//				vAnaObjW[anaIndx].write_by_date(ofs1, idate, 0., iTree + 1);
	//				//vAnaObjE[anaIndx].write_by_date(ofs1, idate, 0., iTree + 1);
	//			}
	//		}

	//		// Wirte pred.
	//		if( 0 )
	//		{
	//			char fout[100];
	//			if( "ins" == sample )
	//				sprintf(fout, "%s\\pred%d.txt", predInsDir_.c_str(), testDate);
	//			else if( "oos" == sample )
	//				sprintf(fout, "%s\\pred%d.txt", predDir_.c_str(), testDate);
	//			ofstream fpreds(xpf(fout).c_str());
	//			if( fpreds.is_open() )
	//			{
	//				fpreds << "target" << "\t" << "bmpred" << "\t" << "tbpred" << "\t"<< "sprd" <<  endl;
	//				char buf[400];
	//				for( int ic = offset1; ic < offset2; ++ic )
	//				{
	//					sprintf(buf, "%.4f\t%.4f\t%.4f\t%.4f\n", vvSpectator[0][ic], vvSpectator[1][ic], vvSpectator[2][ic], vvSpectator[3][ic]);
	//					fpreds << buf;
	//				}
	//			}
	//		}
	//	}

	//	// Write in another format.
	//	{
	//		char fout2[1000];
	//		sprintf(fout2, "%s\\ana%s_%d_%d.txt", statDir_.c_str(), sample.c_str(), testDates[0], testDates[testDates.size() - 1]);
	//		ofstream ofs2(xpf(fout2).c_str());

	//		vAnaObjT[0].write_by_tree_header(ofs2);
	//		//for( int indx = 0; indx < n_tree_points_; ++indx )
	//			//vAnaObj5[indx].write_by_tree(ofs2, get_iTree(indx));
	//		//for( int indx = 0; indx < n_tree_points_; ++indx )
	//			//vAnaObj10[indx].write_by_tree(ofs2, get_iTree(indx));
	//		//for( int indx = 0; indx < n_tree_points_; ++indx )
	//			//vAnaObj20[indx].write_by_tree(ofs2, get_iTree(indx));
	//		for( int indx = 0; indx < n_tree_points_; ++indx )
	//			vAnaObjT[indx].write_by_tree(ofs2, get_iTree(indx));
	//		for( int indx = 0; indx < n_tree_points_; ++indx )
	//			vAnaObjW[indx].write_by_tree(ofs2, get_iTree(indx));
	//		//for( int indx = 0; indx < n_tree_points_; ++indx )
	//			//vAnaObjE[indx].write_by_tree(ofs2, get_iTree(indx));
	//		//for( int indx = 0; indx < n_tree_points_; ++indx )
	//			//vAnaObjL[indx].write_by_tree(ofs2, get_iTree(indx));
	//		//for( int indx = 0; indx < n_tree_points_; ++indx )
	//			//vAnaObjC[indx].write_by_tree(ofs2, get_iTree(indx));
	//	}

	//	// Write by threshold.
	//	{
	//		char fout3[1000];
	//		sprintf(fout3, "%s\\thres_%d_%d.txt", statDir_.c_str(), testDates[0], testDates[testDates.size() - 1]);
	//		ofstream ofs3(xpf(fout3).c_str());

	//		for( int indx = 0; indx < n_tree_points_; ++indx )
	//			vAnaObjT[indx].write_by_thres(ofs3, get_iTree(indx));
	//		for( int indx = 0; indx < n_tree_points_; ++indx )
	//			vAnaObjW[indx].write_by_thres(ofs3, get_iTree(indx));
	//	}
	//}
}

int DTBoost::get_ana_indx(int iTree)
{
	int ret = -1;
	int rIndx = ntrees_ - iTree - 1;
	if( rIndx % interval_ana_ == 0 )
		ret = n_tree_points_ - rIndx / interval_ana_ - 1;
	return ret;
}

int DTBoost::get_iTree(int indx)
{
	int ret = -1;
	if( indx >= 0 )
		ret = ntrees_ - (n_tree_points_ - indx - 1) * interval_ana_;
	return ret;
}

void DTBoost::write_trees(const string& dbname, const string& tablename, const string& mCode,
		int udate, int dbDate, bool one_query)
{
	vector<int> modelDates = get_modelDates(coefDir_);
	if( !modelDates.empty() )
	{
		int modelDate = 0;
		for( auto it = modelDates.begin(); it != modelDates.end(); ++it )
		{
			if( *it <= udate )
				modelDate = *it;
		}

		if( modelDate > 0 )
		{
			std::vector<double> wts;
			std::vector<DTBase*> dts;

			// read model.
			string filename = coefDir_ + "\\coef" + itos(modelDate) + ".txt";
			ifstream ifs(xpf(filename).c_str());
			vector<string> lines;
			string line;
			int maxItree = 0;
			while( getline(ifs, line) )
			{
				lines.push_back(line);
				vector<string> sl = split(line);
				if(sl.size() > 2)
				{
					int iTree = atoi(sl[0].c_str());
					if(iTree > maxItree)
						maxItree = iTree;
				}
			}

			for( int iTree = 0; iTree < ntrees_ && iTree <= maxItree ; ++iTree )
			{
				dts.push_back(new DTMT(lines, iTree));

				for( auto it = lines.begin(); it != lines.end(); ++it )
				{
					vector<string> sl = split(*it);
					if( sl.size() < 2 )
						sl = split(*it, '\t');
					if( sl[0] == itos(iTree) )
					{
						wts.push_back( atof(sl[1].c_str()) );
						break;
					}
				}
			}

			// write trees.
			if( !one_query )
			{
				for( int iTree = 0; iTree < ntrees_ && iTree <= maxItree ; ++iTree )
					dts[iTree]->write_tree(dbname, tablename, mCode, dbDate, iTree, wts[iTree]);
			}
			else
			{
				// delete the table.
				char chk[400];
				char cmd[400];
				sprintf(chk, "select count(*) from %s where idate = %d and market = '%s' ", tablename.c_str(), dbDate, mCode.c_str());
				sprintf(cmd, "delete from %s where idate = %d and market = '%s' ", tablename.c_str(), dbDate, mCode.c_str());
				check_and_delete(dbname, chk, cmd);

				// Compose query.
				vector<string> v;
				if( 1 ) // Works with postgres and MSSQL 2008.
				{
					for( int iTree = 0; iTree < ntrees_ && iTree <= maxItree ; ++iTree )
					{
						vector<string> vnew = dts[iTree]->get_query(mCode, dbDate, iTree, wts[iTree]);
						v.insert(v.end(), vnew.begin(), vnew.end());
					}
					string cmdW = "insert into " + tablename
						+ " (idate, market, cutVariable, cutValue, nPts, tstat, mu, deviance, prune, idx, treeNumber, treeWt) values ";
					for( auto it = v.begin(); it != v.end(); ++it )
					{
						if( it != v.begin() )
							cmdW += ",";
						cmdW += (string)"(" + *it + ")";
					}
					GODBC::Instance()->exec(dbname, cmdW);
				}
				else if ( 0 ) // Should work with MSSQL 2005, but seems to cause a problem.
				{
					for( int iTree = 0; iTree < ntrees_ && iTree <= maxItree ; ++iTree )
					{
						vector<string> vnew = dts[iTree]->get_query(mCode, dbDate, iTree, wts[iTree]);
						v.insert(v.end(), vnew.begin(), vnew.end());
					}
					string cmdW = "insert into " + tablename
						+ " (idate, market, cutVariable, cutValue, nPts, tstat, mu, deviance, prune, idx, treeNumber, treeWt) ";
					for( auto it = v.begin(); it != v.end(); ++it )
					{
						if( it != v.begin() )
							cmdW += " union all ";
						cmdW += " select ";
						cmdW += *it;
					}
					GODBC::Instance()->exec(dbname, cmdW);
				}
			}
		}
	}
}
