#ifndef __DTNode__
#define __DTNode__

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

struct CLASS_DECLSPEC DTNode {
	DTNode();

	int indnum;
	int level;
	int var;
	int leftMax;
	int rpartNodeNum;
	double cut;
	double mu;
	double deviance;
	double tstat;
	DTNode* left;
	DTNode* right;
	DTNode* parent;

	// not essential:
	double y0;
	double y1;
	double y2;
	double yy0;
	double yy1;
	double yy2;
	double diffmax;
	double tmpXold;
};

#endif
