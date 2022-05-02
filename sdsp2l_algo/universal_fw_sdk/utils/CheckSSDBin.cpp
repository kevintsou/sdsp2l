#include "../ufw/StdAfx.h"
#include "CheckSSDBin.h"
#include <fstream> 
#include <iostream>
#include <string>
#include<sstream>
#include "../universal_interface/MyFlashInfo.h"
using namespace std;


CheckSSDBin::CheckSSDBin(FH_Paramater FH)
{
	_row = 0;
	_col = 0;
	_FH = FH;
}

CheckSSDBin::~CheckSSDBin(void)
{
	if ( _data )  {
		free ( _data );
		_data = 0;
	}
}

unsigned char CheckSSDBin::SSD_GetBinResultFromCSV(int MaxPlaneBad, int MaxDieBad, int MaxCEBad, int isSpectekASorAF){
	int bin_result = 0xFF;
	enum eFlash_Info { eAlias, eMaker, eCell_Type, e3D_Type, eDesign, eDie_PerCE, ePlane_PerDie, eBlock_PerCE };
	const int cn_Flash_Info = 8;
	//int Bin_Regions[] = { 0x1A, 0x01, 0x02, 0x2F, 0x03, 0x3F, 0x04, 0x4A };
	//const int cn_Bin_Region = sizeof(Bin_Regions)/sizeof(int);
	enum eBin_Criteria{ ePerPlane, ePerDie, ePerCE };
	const int cn_Bin_Criteria = 3;
	const int start_row = 7;
	const int start_col = 1;
	
	//Bin result depend on SSDBinList.csv.
	int cn_Bin_Region = GetBinCntFromSSDBinList("SSDBinList.csv", start_row, cn_Flash_Info, cn_Bin_Criteria);
	int* Bin_Regions;
	Bin_Regions = (int*)malloc(cn_Bin_Region * sizeof(int));
	GetBinRegionsFromSSDBinList(Bin_Regions, "SSDBinList.csv", start_row, cn_Flash_Info, cn_Bin_Criteria);
	
	_col = cn_Flash_Info + cn_Bin_Region * cn_Bin_Criteria;
	//int all_criteria[cn_Bin_Region * cn_Bin_Criteria] = {0};
	int* all_criteria;
	all_criteria = (int*)malloc(cn_Bin_Region * cn_Bin_Criteria * sizeof(int));
	int rowNo = LoadBinCSV("SSDBinList.csv", _col, 0, start_col);
	if(!rowNo){

		return bin_result;
	}
	int ver_SSDBinList = _data[1];
	int maker = -1;
	int cell_type = -1;
	int fl_3d_type = 0;

	switch(_FH.bMaker){
		//Toshiba(0x98)/SanDisk(0x45)
		case 0x98:
		case 0x45:
			maker = 0;
			break;
			//Intel(0x89)/Micron(0x2C)
		case 0x89:
		case 0x2C:
			maker = 1;
			break;
			//Hynix(0xAD)
		case 0xAD:
			maker = 2;
			break;
			//YMTC
		case 0x9b:
			maker = 3;
			break;
	}
	switch(_FH.Cell){
		//SLC
		case 2:
			cell_type = 0;
			break;
			//MLC
		case 4:
			cell_type = 1;
			break;
			//TLC
		case 8:
			cell_type = 2;
			break;
			//QLC
		case 0x10:
			cell_type = 3;
			break;
	}
	if(_FH.bIs3D){
		//Toshiba(0x98)/SanDisk(0x45)
		if(0 == maker){
			if(_FH.bIsBics2) fl_3d_type=1;
			if(_FH.bIsBics3) fl_3d_type=2;
			if(_FH.bIsBics4) fl_3d_type=3;
			//Hynix(0xAD)
		}else if(2 == maker){
			//if(_FH.bIsHynix3DTLC) 
			fl_3d_type=4;
			//Intel(0x89)/Micron(0x2C)
		}else if(1 == maker){
			//if(_FH.bIsMicron3DMLC || _FH.bIsMicron3DTLC) 
			fl_3d_type=5;
		}else if(3 == maker){
			fl_3d_type=6;
		}
	}
	if (0 == maker && 0 == cell_type){
		
		//20170524	For Chamber
		if((_FH.PageSizeK==4) && (_FH.Design==0x24))
		{
			if(SSD_CheckBadBlock(0x1,MaxPlaneBad,MaxDieBad,MaxCEBad,22,0,0)){
				bin_result = 0x1;
			}else{
				bin_result = 0x2;
			}
		}
		//20171025	For Chamber SLC 8K 32nm
		else if((_FH.PageSizeK==8) && (_FH.Design==0x32))
		{
			if(SSD_CheckBadBlock(0x1,MaxPlaneBad,MaxDieBad,MaxCEBad,192,0,0)){
				bin_result = 0x1;
			}else{
				bin_result = 0x2;
			}
		}

		return bin_result;
	}

	bool existed = false;

	for ( int row=start_row; row < _row; row++ ) {
		if ((maker == _data[row * _col + eMaker]) &&
			(cell_type == _data[row * _col + eCell_Type]) &&
			(fl_3d_type == _data[row * _col + e3D_Type]) &&
			(_FH.Design == _data[row * _col + eDesign]) &&
			(_FH.DieNumber == _data[row * _col + eDie_PerCE]) &&
			(_FH.Plane == _data[row * _col + ePlane_PerDie]) &&
			(_FH.MaxBlock == _data[row * _col + eBlock_PerCE])){
				existed = true;
				for(int col=0 ; col < (cn_Bin_Region * cn_Bin_Criteria); col++){
					all_criteria[col] = _data[row * _col + (col + cn_Flash_Info)];
					int Bin_Region = Bin_Regions[col/cn_Bin_Criteria];
					if(ver_SSDBinList >= 15)
					{
						if(maker == 0 && cell_type == 2 && fl_3d_type == 3 && (_FH.MaxBlock == 1980 || _FH.MaxBlock == 3960 || _FH.MaxBlock == 3916 || _FH.MaxBlock == 7832))
						{
							if((Bin_Region == 0x04 && _FH.bMaker == 0x45) || (Bin_Region == 0x4B && _FH.bMaker == 0x98))
								all_criteria[col] = 0;
						}
						if(ver_SSDBinList >= 24)
						{
							if(isSpectekASorAF == 0)
							{
								if(Bin_Region == 0x4F)
									all_criteria[col] = 0;
							}
						}
					}
				}
				break;
		}
	}

	if (existed){

		//check bin region one by one
		for(int iBin_Region=0; iBin_Region < cn_Bin_Region; iBin_Region++){
			bool passed = true;
			bool skipToCheck = true;
			int bin_criteria[cn_Bin_Criteria] = {0};

			for(int iCriteria=0; iCriteria < cn_Bin_Criteria; iCriteria++){
				bin_criteria[iCriteria] = all_criteria[iBin_Region*cn_Bin_Criteria + iCriteria];
				if(0 != bin_criteria[iCriteria]){
					skipToCheck = false;
					switch(iCriteria){
						case ePerPlane:
							if(MaxPlaneBad > bin_criteria[iCriteria]) passed=false;
							break;
						case ePerDie:
							if(MaxDieBad > bin_criteria[iCriteria]) passed=false;
							break;
						case ePerCE:
							if(MaxCEBad > bin_criteria[iCriteria]) passed=false;
							break;
					}
				}

			}

			//if all criteria is 0, it means that don't need to check this bin region
			if(skipToCheck) continue;

			if(passed){
				bin_result = Bin_Regions[iBin_Region];
				break;
			}
		}

		//if no any bin region was assigned, it means that it's not matched all bin criteria, please assign Bin5
		if(0xFF == bin_result) bin_result=0x05;
	}

	if(Bin_Regions)
	{
		free(Bin_Regions);
		Bin_Regions = 0;
	}
	if(all_criteria)
	{
		free(all_criteria);
		all_criteria = 0;
	}

	return bin_result;
}

int CheckSSDBin::LoadBinCSV(const char* fn, int col, int startLine, int startCol) 
{ 
	ifstream ifs ( fn , ifstream::in );
	if(!ifs){
		return 0;
	}
	string str;
	while(getline(ifs, str)){
		_row++;
	}
	_data = (int*) malloc( _col*_row*sizeof(int) );

	int i = 0;
	//int countno = 0;

	ifstream ifs2 ( fn , ifstream::in );
	while(getline(ifs2, str) && i < _row){
		int j = 0;
		string field;
		istringstream sin(str); //將整行字串line讀入到字串流sin中
		//if(countno == startLine){
			while(getline(sin, field, ',') && j < _col){
				istringstream iss (field);
				int t;
				iss >> t;
				if(!iss.fail()){
					_data[i * _col + j] = t;
				}
				j++;
			}
		//}
			i++;
	}

	ifs.close();
	ifs2.close();


	return _row;
} 
bool CheckSSDBin::SSD_CheckBadBlock(int SSDBin,int ScanPlaneBad, int ScanDieBad, int ScanCEBad, int StdPlaneBad, int StdDieBad, int StdCEBad, int Scan2PlaneBad, int Std2PlaneBad)
{
	bool isCheckPass=true;
	//20161110 Annie : Fixed from ">="  to  ">"
	if((ScanPlaneBad > StdPlaneBad) && (StdPlaneBad > 0 )) 	
		isCheckPass=false;
	if((ScanDieBad > StdDieBad) && (StdDieBad > 0 )) 	
		isCheckPass=false;
	if((ScanCEBad > StdCEBad) && (StdCEBad > 0 )) 	
		isCheckPass=false;
	if((Scan2PlaneBad > Std2PlaneBad) && (Std2PlaneBad > 0 )) 	
		isCheckPass=false;
	//Log("[***SSD***]Bin:%x   >>>  StdPlane %d StdDie %d  StdCE %d Std2P %d",SSDBin,StdPlaneBad,StdDieBad,StdCEBad,Std2PlaneBad);
	return isCheckPass;
}

int CheckSSDBin::GetBinCntFromSSDBinList(const char* fn, int row_index, int infoColCnt, int binCriteriaCnt)
{ 
	ifstream ifs ( fn , ifstream::in );
	if(!ifs){
		return 0;
	}
	string str;
	int count_row = 0;
	int count_col = 0;
	while(getline(ifs, str)){
		count_row++;
		if(count_row < row_index) continue;
		if(count_row > row_index) break;
		string field;
		istringstream sin(str);
		while(getline(sin, field, ',')){
			count_col++;			
		}
	}

	ifs.close();

	return ((count_col - infoColCnt) / binCriteriaCnt);
} 

void CheckSSDBin::GetBinRegionsFromSSDBinList(int* binReg, const char* fn, int row_index, int infoColCnt, int binCriteriaCnt)
{ 
	ifstream ifs ( fn , ifstream::in );
	if(!ifs){
		return;
	}
	string str;
	int count_row = 0;
	int count_col = 0;
	while(getline(ifs, str)){
		count_row++;
		if(count_row < row_index) continue;
		if(count_row > row_index) break;
		string field;
		istringstream sin(str);
		while(getline(sin, field, ',')){
			count_col++;
			if(count_col <= infoColCnt) continue;
			if(((count_col - infoColCnt)%binCriteriaCnt)!=1) continue;
			istringstream iss (field);
			string t;
			iss >> t;
			int endpos = t.find("_");
			binReg[(count_col - infoColCnt)/binCriteriaCnt] = strtol((("0x" + t.substr(1, endpos - 1)).c_str()), NULL, 16);
			
		}
	}

	ifs.close();
} 

