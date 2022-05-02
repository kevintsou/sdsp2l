#pragma once
#include "../universal_interface/MyFlashInfo.h"


//typedef struct _FlhInfo{
//	unsigned char bMaker;
//	unsigned char Cell;
//	unsigned char Design;
//	unsigned char DieNumber;
//	unsigned char Plane;
//	unsigned char MaxBlock;
//	unsigned char PageSizeK;
//	unsigned char bIs3D;
//	unsigned char bIsBics2;
//	unsigned char bIsBics3;
//	unsigned char bIsBics4;
//	int MaxPlaneBad;
//	int MaxDieBad;
//	int MaxCEBad;
//}FlhInfo;

class CheckSSDBin
{
public:
	CheckSSDBin(FH_Paramater);
	~CheckSSDBin(void);

	FH_Paramater  _FH;

	unsigned char SSD_GetBinResultFromCSV(int MaxPlaneBad, int MaxDieBad, int MaxCEBad, int isSpectekASorAF = 0);
	int LoadBinCSV(const char* fn, int col, int startLine, int startCol);
	bool SSD_CheckBadBlock(int SSDBin,int ScanPlaneBad, int ScanDieBad, int ScanCEBad, int StdPlaneBad, int StdDieBad, int StdCEBad, int Scan2PlaneBad = 0, int Std2PlaneBad = 0);
	int GetBinCntFromSSDBinList(const char* fn, int row_index, int infoColCnt, int binCriteriaCnt);
	void GetBinRegionsFromSSDBinList(int* binReg, const char* fn, int row_index, int infoColCnt, int binCriteriaCnt);
	//FlhInfo flhInfo;

private:
	int _col;
	int _row;
	int* _data;
};
