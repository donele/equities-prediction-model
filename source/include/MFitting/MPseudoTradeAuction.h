#ifndef __MPseudoTradeAuction__
#define __MPseudoTradeAuction__
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <MFitting/MPseudoTradePrep.h>
#include <MFitting/MReadTickerState.h>
#include <MFitting/TickerPosition.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MPseudoTradeAuction: public MModuleBase {
public:
	MPseudoTradeAuction(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTradeAuction();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	//struct TickerPosition {
	//	TickerPosition():position(0){}
	//	int adjpos(int msecs, int hold);
	//	void add(int msecs, int shares);
	//	void beginDay();

	//	std::map<int, int> mMsecsPos;
	//	int position;
	//};

	struct AuctionTrade {
		AuctionTrade():CA_price(0.), CA_dPos(0), OA_price(0.), OA_dPos(0){}
		void beginDay();
		float CA_price;
		float CA_dPos;
		float OA_price;
		float OA_dPos;
	};

	//struct DaySum {
	//	DaySum():intra(0.), pred(0.), clcl(0.), clop(0.), dv(0.), nbuy(0), nsell(0){}
	//	double intra;
	//	double pred;
	//	double clcl;
	//	double clop;
	//	double dv;
	//	int nbuy;
	//	int nsell;
	//};

	struct AuctionSummary {
		AuctionSummary():nTicker(0), nCA(0), nOA(0), nBothAuc(0), intraShr(0), CAShr(0), OAShr(0), intraVol(0.), CAVol(0.), OAVol(0.),
			CAShrOrder(0), CAShrFill(0), OAShrOrder(0), OAShrFill(0), CAVolOrder(0.), CAVolFill(0.), OAVolOrder(0.), OAVolFill(0.),
			intraVolTot(0.), CAVolTot(0.), OAVolTot(0.), CAVolOrderTot(0.), CAVolFillTot(0.), OAVolOrderTot(0.), OAVolFillTot(0.)	
		{}
		void print(std::string moduleName);
		void endDay();
		void endJob(std::string moduleName);

		int nTicker;
		int nCA;
		int nOA;
		int nBothAuc;
		int intraShr;
		int CAShr;
		int OAShr;
		double intraVol;
		double CAVol;
		double OAVol;
		int CAShrOrder;
		int CAShrFill;
		int OAShrOrder;
		int OAShrFill;
		double CAVolOrder;
		double CAVolFill;
		double OAVolOrder;
		double OAVolFill;

		double intraVolTot;
		double CAVolTot;
		double OAVolTot;
		double CAVolOrderTot;
		double CAVolFillTot;
		double OAVolOrderTot;
		double OAVolFillTot;
	};

private:
	bool debug_;
	bool getAuctionFromBook_;
	bool requireBookComplete_;
	bool impact_;
	bool smart_;
	int verbose_;
	int maxShrChg_;
	int maxShrChgAuc_;
	bool addON_;
	int ONmult_;
	int minMsecs_;
	int maxMsecs_;
	int minMsecsON_;
	int maxMsecsON_;
	int hold_;
	double thres_;
	double thresON_;
	double fee_;
	double maxPos_;
	double maxPosTicker_;
	double restoreMax_;
	std::string dir_;
	std::string predName_;
	std::string market_;
	std::string filename_;
	std::ofstream ofs_;

	Acc accIntra_;
	Acc accClcl_;
	Acc accClop_;
	Acc accNtrd_;
	Acc accDV_;
	Acc accTotal_;

	Corr corrON_;
	int ntrdfail_;
	int nMin_;
	double previous_position_;
	std::vector<double> vPositionMin_;
	std::map<std::string, TickerPosition> mPos_;
	std::map<std::string, AuctionTrade> mAucTrd_;
	AuctionSummary aucSum_;

	bool book_complete(Book& book);
	void auction_trade(const std::map<std::string, Book>* mBookClose, const std::map<std::string, Book>* mBookOpen,
				const std::map<std::string, MPseudoTradePrep::AuctionInfo>* mAuc, double exchrat,
				const MReadTickerState::TickerState& ts, double mid, AuctionTrade& uidAuc, TickerPosition& uidPos, int closeMsecs,
				std::ofstream& ofsDebug, std::string uid);
	void find_auction(double& CA_price, int& CA_size, double& OA_price, int& OA_size, double exchrat, std::string uid,
		const std::map<std::string, Book>* mBookClose, const std::map<std::string, Book>* mBookOpen, const std::map<std::string, MPseudoTradePrep::AuctionInfo>* mAuc);
	void get_auction_book_impact(std::string oc, std::string uid, int bs, double order_price, int order_size, double exchrat, double& auction_price, int& auction_size);
	double get_smart_price(std::string uid, int bs, double order_price, int order_size, double exchrat, double auction_price);
	void market_close(DaySum& daysum, std::ofstream& ofsDebug, int idate);
	void endDayOld();
};

#endif
