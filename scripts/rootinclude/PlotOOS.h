#ifndef __PlotOOS__
#define __PlotOOS__
#include "ftns.h"
#include <vector>
#include <string>

const bool debug = false;

class PlotOOS {
public:
	PlotOOS();
	PlotOOS(const string& basedir, const string& model);
	void add_line(int idate);
	void add_fit(std::string subdir, std::string target, std::string type, int udate, int idate1, int idate2, int maxNTrees = 0);
	void add_var(std::string var);
	void plot();

private:
	std::string basedir_;
	std::string model_;
	int nvar_;
	int nfits_;
	std::vector<int> maxNTrees_;
	std::vector<std::string> lines_;
	std::vector<int> vindx_;
	std::vector<std::pair<int, int>> sprds_;
	std::vector<std::string> vars_;
	std::vector<std::string> subdirs_;
	std::vector<std::string> targets_;
	std::vector<std::string> types_;
	std::vector<int> udates_;
	std::vector<int> idate1s_;
	std::vector<int> idate2s_;
	std::vector<int> vIdateLine_;
	std::vector<int> vXLine_;

	void read_lines(char* path, int maxNTree);
	int get_nmonths(int idate0, int idate1);
	int get_nma(int N);
	void get_ma(std::vector<double>& vMA, std::vector<double>& v, int nma);
	int get_max_ntrees(const std::string& path);
	void get_series(std::vector<int>& vd, std::vector<double>& v, int vindx, const std::pair<int, int>& sprd, double& grmax, double& grmin);
};

PlotOOS::PlotOOS()
{
}

PlotOOS::PlotOOS(const string& basedir, const string& model)
:basedir_(basedir),
	model_(model),
	nfits_(0),
	nvar_(0)
{
	sprds_.push_back(make_pair(0, 20));
	sprds_.push_back(make_pair(20, 100));
}

void PlotOOS::add_line(int idate)
{
	vIdateLine_.push_back(idate);
}

void PlotOOS::add_fit(std::string subdir, std::string target, std::string type, int udate, int idate1, int idate2, int maxNTrees)
{
	maxNTrees_.push_back(maxNTrees);
	++nfits_;
	subdirs_.push_back(subdir);
	targets_.push_back(target);
	types_.push_back(type);
	udates_.push_back(udate);
	idate1s_.push_back(idate1);
	idate2s_.push_back(idate2);
}

void PlotOOS::add_var(std::string var)
{
	++nvar_;
	vars_.push_back(var);
}

int PlotOOS::get_nmonths(int idate0, int idate1)
{
	int nyr = idate1 / 10000 - idate0 / 10000;
	int mm0 = idate0 / 100 % 100;
	int mm1 = idate1 / 100 % 100;
	int nmonths = nyr * 12 + (mm1 - mm0);
	return nmonths;
}

int PlotOOS::get_nma(int N)
{
	int nma = 1;
	for( int i = 0; i < 10; ++i )
	{
		int temp_nma = 10 * pow(2, i);
		if( temp_nma > N / 5 )
			break;
		nma = temp_nma;
	}
	return nma;
}

void PlotOOS::get_ma(std::vector<double>& vMA, std::vector<double>& v, int nma)
{
	int N = v.size();
	if( N > nma )
	{
		for( int i = nma - 1; i < N; ++i )
		{
			double sum = 0.;
			for( int j = i - nma + 1; j <= i; ++j )
				sum += v[j];
			vMA.push_back(sum / nma);
		}
	}
}

int PlotOOS::get_max_ntrees(const string& path)
{
	int max_ntrees = 0;
	ifstream ifs(path.c_str());
	if( ifs.is_open() )
	{
		string line;
		while( getline(ifs, line) )
		{
			vector<string> sl = split(line);
			if( sl[0] != "date" )
			{
				int ntrees = atoi(sl[3].c_str());
				if( max_ntrees < ntrees )
					max_ntrees = ntrees;
			}
		}
	}
	return max_ntrees;
}

void PlotOOS::plot()
{
	TFile* f = new TFile("plots.root", "recreate");
	for( int iv = 0; iv < nvar_; ++iv )
	{
		string var = vars_[iv];
		TGraph** gr = new TGraph*[6];
		TGraph** grMA = new TGraph*[6];
		int ngrMax = 0;

		for( int it = 0; it < nfits_; ++it )
		{
			char path[1600];
			sprintf(path, "%s/%s/%s/%s/stat_%d/oos_%d_%d.txt",
					basedir_.c_str(), model_.c_str(), subdirs_[it].c_str(), targets_[it].c_str(), udates_[it], idate1s_[it], idate2s_[it]);
			read_lines(path, maxNTrees_[it]);
			int vindx = vindx_[iv];

			int NS = sprds_.size();
			for( int is = 0; is < NS; ++is )
			{
				++ngrMax;
				int ngr = it * NS + is;
				vector<double> vx;
				vector<int> vd;
				vector<double> v;
				double grmax = 0.;
				double grmin = 0.;
				get_series(vd, v, vindx, sprds_[is], grmax, grmin);

				if( v.empty() )
				{
					vx.push_back(0.);
					v.push_back(0.);
				}
				int ND = v.size();
				int nma = get_nma(v.size());

				// Raw plot.
				for( int i = 0; i < ND; ++i )
					vx.push_back(i + 1);
				gr[ngr] = new TGraph(ND, &vx[0], &v[0]);
				char title[100];
				char name[100];
				char nameMA[100];
				sprintf(name, "%s_%s_%s_%d_%d", var.c_str(), model_.c_str(), types_[it].c_str(), sprds_[is].first, sprds_[is].second);
				sprintf(nameMA, "%s_%s_%s_%d_%d_%dMA", var.c_str(), model_.c_str(), types_[it].c_str(), sprds_[is].first, sprds_[is].second, nma);
				if( nma > 1 )
				{
					sprintf(title, "%s %s %s sprd(%d, %d), %d day MA", var.c_str(), model_.c_str(), types_[it].c_str(), sprds_[is].first, sprds_[is].second, nma);
				}
				else
				{
					sprintf(title, "%s %s %s sprd(%d, %d)", var.c_str(), model_.c_str(), types_[it].c_str(), sprds_[is].first, sprds_[is].second);
				}
				gr[ngr]->SetName(name);
				gr[ngr]->SetTitle(title);
				gr[ngr]->Write();

				// idate line.
				for( int idate : vIdateLine_ )
				{
					for( int i = 0; i < ND; ++i )
					{
						if( vd[i] == idate )
						{
							vXLine_.push_back(vx[i]);
							break;
						}
					}
				}

				// MA plot.
				vector<double> vxMA;
				for( int i = nma - 1; i < ND; ++i )
					vxMA.push_back(i + 1);
				vector<double> vMA;
				get_ma(vMA, v, nma);
				grMA[ngr] = new TGraph(ND - nma + 1, &vxMA[0], &vMA[0]);
				grMA[ngr]->SetName(nameMA);
				grMA[ngr]->Write();

				// hist
				char hname[20];
				sprintf(hname, "h%s%d", var.c_str(), ngr);
				TH1F* h = new TH1F(hname, hname, ND, 1, ND);
				int nmonths = get_nmonths(vd[0], vd[ND - 1]);
				int incr = nmonths / 10 + 1;
				int last_label_idate = 0;
				for( int i = 0; i < ND; ++i )
				{
					char buf[10];
					bool firstDate = (i == 0);
					bool lastDate = (i == ND - 1 && vd[i] / 100 != last_label_idate / 100);
					bool addLabel = (i > 0 && get_nmonths(last_label_idate, vd[i]) == incr);
					if( firstDate || lastDate || addLabel )
					{
						sprintf(buf, "%d", vd[i] / 100);
						h->GetXaxis()->SetBinLabel(i + 1, buf);
						last_label_idate = vd[i];
					}
				}
				h->SetMinimum(grmin*0.9);
				h->SetMaximum(grmax*1.1);
				h->SetTitle(gr[ngr]->GetTitle());
				h->GetXaxis()->SetLabelSize(0.06);
				gr[ngr]->SetHistogram(h);
			}
		}
		char cname[10];
		sprintf(cname, "%s%s", var.c_str(), model_.c_str());
		TCanvas* c1 = new TCanvas(cname, cname, 600, 270*nfits_);
		c1->Divide(2, nfits_);
		int nPads = 2 * nfits_;
		for( int i = 0; i < nPads && i < ngrMax; ++i )
		{
			c1->cd(i + 1);
			gPad->SetGridx();
			gPad->SetGridy();
			gr[i]->Draw("alp");
			grMA[i]->SetLineColor(2);
			grMA[i]->SetLineWidth(2);
			grMA[i]->SetLineStyle(2);
			grMA[i]->Draw("lp");

			for( int x : vXLine_ )
			{
				double y0 = grMA[i]->GetHistogram()->GetMinimum();
				double y1 = grMA[i]->GetHistogram()->GetMaximum();
				TLine *line = new TLine(x, y0, x, y1);
				line->Draw();
			}
		}
		c1->Print((var + "_" + model_ + ".gif").c_str());
	}
}

void PlotOOS::read_lines(char* path, int maxNTree)
{
	lines_.clear();
	vindx_.clear();
	int max_ntrees = 0;
	if( maxNTree > 0 )
		max_ntrees = maxNTree;
	else
		max_ntrees = get_max_ntrees(path);
	ifstream ifs(path);
	if( ifs.is_open() )
	{
		string line;
		while( getline(ifs, line) )
		{
			vector<string> sl = split(line);
			if( sl[0] == "date" && vindx_.empty() )
			{
				int N = sl.size();
				for( vector<string>::iterator it = vars_.begin(); it != vars_.end(); ++it )
				{
					bool var_found = false;
					for( int i = 0; i < N; ++i )
					{
						if( *it == sl[i] )
						{
							vindx_.push_back(i);
							var_found = true;
							break;
						}
					}
					if( !var_found )
					{
						cout << *it << " not found." << endl;
						exit(3);
					}
				}
			}
			else
			{
				if( max_ntrees == atoi(sl[3].c_str()) )
					lines_.push_back(line);
			}
		}
	}
	else
		cout << path  << " not open." << endl;
}

void PlotOOS::get_series(vector<int>& vd, vector<double>& v, int vindx, const pair<int, int>& sprd, double& grmax, double& grmin)
{
	double last_val = 0.;
	for( vector<string>::iterator it = lines_.begin(); it != lines_.end(); ++it)
	{
		string& line = *it;
		vector<string> sl = split(line);
		int sprd1 = atoi(sl[1].c_str());
		int sprd2 = atoi(sl[2].c_str());

		if( sprd1 == sprd.first && sprd2 == sprd.second )
		{
			double val = atof(sl[vindx].c_str());
			int idate = atoi(sl[0].c_str());
			vd.push_back(idate);
			if( val == 0. )
			{
				v.push_back(last_val);
			}
			else
			{
				if( grmax == 0. || val > grmax )
					grmax = val;

				if( grmin == 0. || val < grmin )
					grmin = val;

				v.push_back(val);
				last_val = val;
			}
		}
	}
}

#endif
