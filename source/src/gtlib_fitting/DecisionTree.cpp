#include <gtlib_fitting/DecisionTree.h>
#include <jl_lib/sort_util.h>
#include <thread>
using namespace std;

namespace gtlib {

DecisionTree::DecisionTree()
{
}

DecisionTree::DecisionTree(int nInputFields)
	:nInputFields_(nInputFields)
{
}

DecisionTree::DecisionTree(FitData* pFitData)
	:nInputFields_(pFitData->nInputFields),
	nSamplePoints_(pFitData->nSamplePoints),
	pFitData_(pFitData),
	pRoot_(nullptr)
{
}

DecisionTree::DecisionTree(DecisionTreeParam decisionTreeParam, FitData* pFitData)
	:nInputFields_(pFitData->nInputFields),
	nSamplePoints_(pFitData->nSamplePoints),
	pFitData_(pFitData),
	pRoot_(nullptr),
	maxNodes_(decisionTreeParam.maxLeafNodes * 2 - 1),
	minPtsNode_(decisionTreeParam.minPtsNode),
	maxTreeLevels_(decisionTreeParam.maxTreeLevels),
	maxMu_(decisionTreeParam.maxMu)
{
	cout << "sorting inputs...";
	pSortedIndex_ = make_shared<vector<vector<int>>>(nInputFields_, vector<int>(nSamplePoints_, 0));
	int nThreadMax = nInputFields_;

	vector<thread> vThread;
	for( int iThread = 0; iThread < nThreadMax; ++iThread )
		vThread.push_back(thread(&DecisionTree::sortInput, this, iThread, nThreadMax));
	for( auto& t : vThread )
		t.join();
	cout << " done." << endl;
}

DecisionTree::DecisionTree(DecisionTreeParam decisionTreeParam, FitData* pFitData, shared_ptr<vector<vector<int>>> pSortedIndex)
	:nInputFields_(pFitData->nInputFields),
	nSamplePoints_(pFitData->nSamplePoints),
	pFitData_(pFitData),
	pRoot_(nullptr),
	pSortedIndex_(pSortedIndex),
	maxNodes_(decisionTreeParam.maxLeafNodes * 2 - 1),
	minPtsNode_(decisionTreeParam.minPtsNode),
	maxTreeLevels_(decisionTreeParam.maxTreeLevels),
	maxMu_(decisionTreeParam.maxMu)
{
}

DecisionTree::~DecisionTree()
{
}

void DecisionTree::sortInput(int iThread, int nThreadMax)
{
	int nJobEachThread = ceil((double)nInputFields_ / nThreadMax);
	int offset1 = iThread * nJobEachThread;
	if( offset1 < nInputFields_ )
	{
		int offset2 = (iThread + 1) * nJobEachThread;
		if( offset2 > nInputFields_ || iThread == nThreadMax - 1 )
			offset2 = nInputFields_;

		if( offset2 > offset1 )
		{
			for( int i = offset1; i < offset2; ++i )
				gsl_heapsort_index<int, float>((*pSortedIndex_)[i], pFitData_->input(i));
		}
	}
}

} // namespace gtlib
