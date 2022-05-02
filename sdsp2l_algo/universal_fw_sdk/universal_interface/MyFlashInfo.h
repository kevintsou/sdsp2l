
#ifndef _MYFLASHINFO_H_
#define _MYFLASHINFO_H_
//#include "GlobalDefine.h"
#include <stdio.h>
//#include "SortingSetting.h"
#define MAX_SLOT 4
#define _APPARAM_SIZE 1024

#define PAGE_21  0x21
#define PAGE_22  0x22
#define PAGE_35  0x35
#define PAGE_42  0x42
#define PAGE_43  0x43
#define PAGE_44  0x44
#define PAGE_45  0x45
#define PAGE_46  0x46
#define PAGE_48  0x48
#define PAGE_49  0x49
#define PAGE_4A  0x4A
#define PAGE_61  0x61
#define PAGE_62  0x62

#define CTRLUSB  0
#define CTRLS11  1
#define CTRLE13  2

#define SSD_ECC_RATIO 4

typedef struct _FLH_ABILITY
{
	bool isLegacy;
	bool isT1;
	bool isT2;
	bool isT3;
	unsigned int MaxClk_Legacy;
	unsigned int MaxClk_T1;
	unsigned int MaxClk_T2;
	unsigned int MaxClk_T3;
}FLH_ABILITY;

#ifndef _USE_OLD_PARAMETER
#pragma pack(1)

enum {_SCAN_WINDOWS_PASS, _SCAN_WINDOWS_FAIL};
typedef struct _FH_Paramater { //refer Param.ini setting
	//cp & cz
	char PartNumber[32];
	unsigned char bMaker;
	unsigned char id[6];
	unsigned char Configed;

	unsigned char Type;
	unsigned char Cell; //2 SLC, 4 MLC , 8 TLC, 0x10 QLC
	unsigned char Mode;
	unsigned char Plane; //幾個plane
	unsigned char Plane_Mode;
	unsigned char Vcc;
	//10
	unsigned char IO_Bus;
	unsigned char PageDefine;//0x08=512	0x0C=2k		0x0D=4K		0x0E=8K		0x0F=16K,
	unsigned char Row_Shift;
	unsigned char Page;
	unsigned char DIE_Shift;//1die 0, 2 die 1, 4 die 2
	unsigned char DIE_Sel;
	unsigned char Extend;
	unsigned char Ext_Mode;
	unsigned char Ext_H;
	unsigned char Ext_L;
	//20
	unsigned char Page_L; //非ed3 Sorting要用的MaxPage 要 (Page_H << 8 | page_L) * (?K page*2) 
	unsigned char Page_H;
	unsigned char SecondCol_Shift;
	unsigned char ECCByte;  //flash 最大ECC
	unsigned char Spare0Len;
	unsigned char Spare1Len;
	unsigned char Conversion;
	unsigned char Reverse;
	unsigned char FWSpareCount;		//20160420 Sam: 原_KeepFWSpareCount, 改放到下面_KeepFWSpareCount[Max_slot]
	//unsigned char EraseCount;
	unsigned char Design; //多少nm
	//30
	unsigned char PageAMap;
	unsigned char PageA;
	// set sorting parameter
	unsigned char bToggle;
	unsigned char beD3;
	unsigned short MaxBlock;
    unsigned char PageSize_L;
    unsigned char PageSize_H;
    unsigned char PageSize_Ext;
	unsigned char Read_Timming;
	unsigned char Write_timing;
	//40
	unsigned char Read_PBATiming;
	unsigned char Write_PBATiming;
	unsigned char bIs3D;
	//fw data setting
	unsigned char _OC_ECC; //old controller ecc-> 61 67
	unsigned char _OC_SPARE;//old controller spare-> 61 67
	unsigned char _NC_ECC; //new Controller ecc -> 70 09 07
	unsigned char _NC_SPARE; //new Controller ecc -> 70 09 07
	unsigned char FlashCommandCycle;
#if 0
	unsigned char SLC_RDY_L;
	unsigned char SLC_RDY_H;
	unsigned char First_RDY_L;
	unsigned char First_RDY_H;
	unsigned char Foggy_RDY_L;
	unsigned char Foggy_RDY_H;
	//50
	unsigned char Fine_RDY_L;
	unsigned char Fine_RDY_H;
#endif

    //====這邊以上不可以插入新的定義===========

	//setting by program==============================================
	unsigned char reserve[8];
	unsigned char bHV_TYPE[MAX_SLOT];
	unsigned char bFunctionMode;
	unsigned char bPBADie;
	unsigned char bSetBadColumn;
	unsigned char bSetOBCBadColumn;
	unsigned char FlhNumber;
	unsigned char DieNumber;
    unsigned int BigPage;
    unsigned int PagePerBlock;
    //unsigned char SectorPerPage;
    unsigned char PageSizeK;
    unsigned int PageSize;
    unsigned char PageFrameCnt;
    // sector count >=PageSize;
    unsigned int PageSizeSector;
    // K size >=PageSizeK;
    unsigned int PageSizeSectorK;
    unsigned char IODriving;
    unsigned char CMDTiming;
    unsigned char DMATiming;
    unsigned char SpareMode;
	unsigned int  UnitSize ;// sector  512byte
    //setting by MPSorting
    unsigned char inverseCE[9];
	unsigned int MaxCapacity;
	unsigned int FlhCapacity;
	unsigned int DieCapacity;
	unsigned int MaxPage;
	unsigned int FlashSlot ;
	unsigned int Block_Per_Die;
	unsigned char MaxEccBit;//controll or flash 最大ECC
	unsigned int TGoodPage;
	unsigned int TFinalCapacity[MAX_SLOT];
	unsigned int TFinalByPassStatusCapacity[MAX_SLOT];
	//unsigned int TGoodBlockPerPlane[8][8];
	unsigned int _512ByteTriggLen;
//#ifdef _USEMGR
	unsigned int TGoodBlockPerDiePlane[32][8];//logical ce + plane
//#else
//	unsigned int TGoodBlockPerDiePlane[16][8];//ce die plane
//#endif
	unsigned char SortingRdTiming;
	unsigned char SortingWtTiming;
	unsigned char SortingEdo;
	//================================================================
	unsigned int EraseBadCount	;
	unsigned int WriteBadCount	;
	unsigned int ReadBadCount	;
	unsigned int OriginalBadCount;
	unsigned int APCapacity;
	unsigned int TBadPage;
	unsigned char HalfMode;
	unsigned char LowMode;
	unsigned int CMode;
	unsigned int CapacityMode;
	unsigned int AllBadBlock;
	//unsigned short NormalReadPassBlock[MAX_SLOT];
	unsigned int TBadBlock;
	unsigned int TGoodBlock[MAX_SLOT];
	unsigned int FR_Clock;
	unsigned char FW_Clock;
	unsigned int  DieOffset;
	unsigned char isDefect;
	unsigned char bOnFiEnable;
	unsigned char bToggleEnable;
	/************************************************************************/
	/* for onfi 要切回legacy時 需要記錄clock and driving                                                                     */
	/************************************************************************/
	unsigned char LegacyEDO_14;
	unsigned char LegacyDriving_4D;
	unsigned char LegacyReadClock_48;
	unsigned char LegacyWriteClock_49;
	unsigned int ECODE[MAX_SLOT];
	unsigned int ERROR_RESULT[MAX_SLOT];
	unsigned char SD_Rule;
	int MBC_ECC;
	int ECC;
	char Bin_Region[MAX_SLOT][2][512];
	int FirstBinCheck[MAX_SLOT];
	int BIN_MODE_TYPE[MAX_SLOT];
	int SHOW_PD_MODE[MAX_SLOT];
	char HY14MLC_DA[MAX_SLOT];

	unsigned char CEPerSlot;
	unsigned int fast_page_list[2048];
	unsigned char fast_page_bit[2048];
	unsigned short PairPageAddress[1024];
	short d2_fastMap[1024];
	unsigned char CsvYear;
	unsigned char CsvMonth;
	unsigned char CsvDay;
	unsigned char CsvCount;
	unsigned char CEMap[16];
	unsigned char LMUMode[MAX_SLOT];
	unsigned char EraseCount[MAX_SLOT];
	unsigned char ProgramCount[MAX_SLOT];
	unsigned char D3ProgramCount[MAX_SLOT];
	unsigned char Pretest1P2P[MAX_SLOT];
	unsigned char bIs8T2B;
	unsigned char bIs8T2E;
	unsigned char bIsBics2;
	unsigned char bIsBics3;
	unsigned char bIsBics3pMLC;
	unsigned char bIsBics4;
	unsigned char bIsHynix16TLC;
	unsigned char bIsHynix3DTLC;
	unsigned char bIsHynixD1OTPFail[MAX_SLOT];
	unsigned char bIsHynixD3OTPFail[MAX_SLOT];
	unsigned char bIsHynix3DV3;
	unsigned char bIsHynix3DV4;
	unsigned char bIsHynix3DV5;
	unsigned char bIsHynix3DV6;
	unsigned char bIsMicron3DMLC;
	unsigned char bIsMicron3DTLC;
	unsigned char bIsMicronB16;
	unsigned char bIsMicronB17;
	unsigned char bIsMicronB27;
	unsigned char bIsMicronB36R;
	unsigned char bIsMicronB37R;
	unsigned char bIsMicronB47R;
	unsigned char bIsMicronQ1428A;
	unsigned char bIsMIcronB0kB;
	unsigned char bIsMIcronB05;
	unsigned char bIsMIcronL06;
	unsigned char bIsMIcronL04;
	unsigned char bIsMIcronN18;
	unsigned char bIsMIcronN28;
	unsigned char bIsYMTC3D;
	unsigned char bIsYMTCMLC3D;
	unsigned char bIsSamsungMLC14nm;
	unsigned char bIsBiCs4QLC;
	unsigned char bIsBiCs3QLC;
	unsigned char bIsBiCs3eTLC;
	unsigned char bIsSD_INand_A19_MLC; //for 冠元科技
	unsigned char bBiscType;//[BICS3] 1=Normal; 2=Enterprise; 3=Industrial   [BICS4]1=Early  2=Final
	//unsigned char bIsTsb3DTLC;
	unsigned char b8t2eHalfMode;
	unsigned char bUseA2Command;
	int DMAMode[MAX_SLOT][4];  //20151109 sam: 1k/512byte mode, DMAMODE[slot][Group plane]
	unsigned int b1KEccBit[MAX_SLOT];
	unsigned int b512ByteEccBit[MAX_SLOT];
	int OBCDMAMode[MAX_SLOT][4];
	unsigned int OBC1KEccBit[MAX_SLOT];
	unsigned int OBC512ByteEccBit[MAX_SLOT];
	unsigned int FlashPageFun[MAX_SLOT];  //20171024 danson: keep is full page size or cut 1k mode
	unsigned char ePage[MAX_SLOT];
	unsigned char _KeepFWSpareCount[MAX_SLOT];
	unsigned char _KeepD3FWSpareCount[MAX_SLOT];
	unsigned char PretestFillFull[MAX_SLOT];
	unsigned char _ForceD1SoureCnt;
	unsigned char SelectFWPlaneMode[MAX_SLOT];
	unsigned char MLCCopybackMode;	// MLC FW 走 Copy back, 不用走 pretest page mapping.
									// 進 efastrn 去測, 當 D1 Block 留給 FW. IB Bad table 變跟 TLC 一樣. 
	int iSLCCountforMLC;	// For 2 condition: 1. MLC efastrn block, 2. MLC Copyback d1 block.

	unsigned char U2_PI_L_Reg;
	unsigned char U2_PI_H_Reg;
	unsigned char U2_DIV_Reg;

	unsigned char U3_PI_L_Reg;
	unsigned char U3_PI_H_Reg;
	unsigned char U3_DIV_Reg;

	unsigned char _USB_TYPE_;
	unsigned char _RealPlane;
	//unsigned char ERR[512];
	unsigned char ToolType;
	unsigned long F2_dwCapacity[MAX_SLOT];
	unsigned char F2_SN[MAX_SLOT][64];
	bool bisUFS;
	int bBinModeBySlot[MAX_SLOT];//20160225 Annie : HV Sorting Mode8 的 HV2 跟 ModeC 的 IB 要一致 
	int bSetMBCBySlot[MAX_SLOT];
	unsigned short usVerifyFunction[MAX_SLOT];
	int HV3IBMode[MAX_SLOT];//20160905 Annie added
	unsigned char bCheckLowPage; //use retry read to check / danson
	unsigned char IspCkPage0[MAX_SLOT];
	char Bin_Region_SSD[MAX_SLOT][64];
	unsigned char bXLGA;
	//save rdy , polling status 
	unsigned char FlashFunction[32];
	unsigned int  alu_PageCount;
	unsigned char _doWriteFuse[32];
	unsigned char _analyzeTrim[MAX_SLOT];
	unsigned char GetHVIB[MAX_SLOT];
	unsigned char bIsTrimmed[MAX_SLOT];
	unsigned char bDieSortTag[MAX_SLOT];
	unsigned char inkDieToPG[MAX_SLOT];
	unsigned int TrimEarlyBlockPerDiePlane[32][8];//logical ce + plane
	unsigned char RetryFunctionClose[MAX_SLOT];
	unsigned int IB2Tag[MAX_SLOT];
	unsigned int IB3Tag[MAX_SLOT];
	unsigned int IB4Tag[MAX_SLOT];
	unsigned int NEWMBCTag[MAX_SLOT];
	char Bin_Region_Retention[MAX_SLOT][64];

	unsigned char CzPBADie;
	unsigned char CzFlhNumber;
	int checkSpecialCall[MAX_SLOT];
	unsigned char GetFuseBuf[16]; //logical ce
	bool bisExPBA;
	unsigned char bisInterLeave;
	unsigned char b512ByteChangeColumnECC[MAX_SLOT];
	unsigned char SIDType;	//Special ID Type

	unsigned char FlashInterface;
	unsigned char bL06_TO_B0k;
	unsigned char bBics4_QLC_to_TLC;
	int WindowPhase;
	int WindowBackUpPhase; //for mode9: legacy to toggle need to reset degree
	short WindowDelayOffset;
	short WindowBackUpDelayOffset;
	unsigned char b09Reverse;
	unsigned char bNeedTrimRdyBusy;
	unsigned char bUFSMap;
	unsigned char bSetMicronDDR;
	unsigned char bIsHynixE3NAMD;
	int china_customer_ecc;
	int PageMappingBytes;
	unsigned char GetIDTiming;
	unsigned int  SortingOriMaxCap[MAX_SLOT];
	unsigned char bHynixSwitchMode;
	unsigned char bNoR8BlockAllBad[MAX_SLOT];
	//20180608
	unsigned char bBootCodeVersion[2];
	unsigned char bCheckT2TOL;
	int			  bS11FlhClk[3];//0 legacy;1 T1; 2 T2
	unsigned char bUID[16][16];
	unsigned char SDK9T23;
	unsigned char _AlignExtendBlock;
	unsigned char UcReserved[512];
	unsigned char FunctionSkipSorting;
	//unsigned char _2307T2ScanWindowReslut; /*0 for OK 1 for fail*/
	unsigned char BypassStatus;
	unsigned char _1k1kStatus;
	int HynixOTPD1Result[32]; // 1: SSD Pass, 2: SSD Fail+USB Pass, 3: SSD Fail+USB Fail, 4: MDie-LAD Pass
	int HynixOTPD3Result[32]; // 1: SSD Pass, 2: SSD Fail+USB Pass, 3: SSD Fail+USB Fail, 4: MDie-LAD Pass
	bool HV_D1Mode;
	unsigned char RRT_Index[6];
	unsigned char PageClass;
	unsigned short WindowsStartR[16];
	unsigned short WindowsEndR[16];
	unsigned short WindowsStartW[16];
	unsigned short WindowsEndW[16];
	unsigned short WindowsDLL_Center[16];
	int isCheckSSDBinPass;
	unsigned char ID_Exist[8];
	bool PageMode_HighClass[64];

}FH_Paramater;

#pragma pack()


class MyFlashInfo
{
public:
	wchar_t filename[256];
	
	static MyFlashInfo* Instance(const wchar_t*, const unsigned char* buf, long lSize);
	static MyFlashInfo* Instance(const char*, const unsigned char* buf, long lSize);
	static MyFlashInfo* Instance();
	static int UpdateFlashTypeFromID( FH_Paramater * param);
	void LoadBuf(const unsigned char* buf, long lSize);
	bool IsValid();
	static void Free();
    bool GetParameterUPTool(const unsigned char*, FH_Paramater*);  //for up
    bool ParamToBufferUPTool(FH_Paramater* , unsigned char*  );
	int	 GetFastPage(int , unsigned char);
	int GetPairPage(int FPage,unsigned char MLC_PageA,FH_Paramater* param);
	bool GetFlashParamFromCSV(const unsigned char* flashID, FH_Paramater* param);
	bool GetParameter(const unsigned char*, FH_Paramater*, const char * ini,int over4ce, unsigned char bUFSCE, bool bIsCheckExtendBuf=false);
	bool GetParameterMultipleCe(const unsigned char* flashID, FH_Paramater* param, const char *ini,int over4ce, unsigned char bUFSCE);
	static bool IsSixAdressFlash(FH_Paramater *fh);
	static bool IsAddressCycleRetryReadFlash(FH_Paramater *fh);
    //for up
//	bool ParamToBuffer(FH_Paramater* , unsigned char* , const char*,bool FlashMixMode ,FH_Paramater* );
    //for st
	unsigned char ParamToBuffer(FH_Paramater* , unsigned char* , const char* ,unsigned char,unsigned char,unsigned char ,bool FlashMixMode,FH_Paramater*,int closeOnfi);
	void SetFlashCmd_Buffer(FH_Paramater* param, unsigned char* buffer, unsigned char bE2NAMD);

    int  getFlashInverseED3 ( FH_Paramater*, unsigned char, const unsigned char* ,bool mixmode, int over4ce, unsigned char bUFSCE, bool bIsCheckExtendBuf);
    int  getFlashInverse ( FH_Paramater*, unsigned char, const unsigned char* ,bool mixmode,int over4ce,  unsigned char bUFSCE);
	int	 CheckFileDateTime();
	bool FindAppendile(char *sSouce, char *sTag, int stagLen, unsigned char *buf, unsigned long &filesize);
	static int GetFlashDefaultInterFace(int voltage, FH_Paramater * fh);
	static FLH_ABILITY GetFlashSupportInterFaceAndMaxDataRate(int voltage, const FH_Paramater *fh);
	static unsigned char GetMicroClockTable(int data_rate, int flh_interface, const FH_Paramater *fh);
	static  int GetAddressGapBit(FH_Paramater * fh);
	unsigned char CsvYear;
	unsigned char CsvMonth;
	unsigned char CsvDay;
	unsigned char CsvCount;
	short	versionYear;
	int	versionMonth;
	int	versionDay;
	int ForceCeCount;
	int ForceSortingCE;
	bool _SupportWrongID;
	unsigned char b_page_outliers[32];

	// Statistics processing for IB records
	unsigned char MarkPatternResults[64];
	unsigned char Curve_Fitting_Theta[64][2];

	//int FlashMixMode;
private:
	MyFlashInfo(const wchar_t*, const unsigned char* ftable, long lSize);
	MyFlashInfo();
	~MyFlashInfo();
	static MyFlashInfo* _inst;
	unsigned char** _infos;
	unsigned char** _keys;
	unsigned char** _partNums;
	int _flashCnt;
	bool _valid;
	int _colCnt;
};

#endif

extern const char* _FLASH_INFO;

unsigned char _IsMicronB95AGen4(const FH_Paramater*);
unsigned char _IsTSB3D(const FH_Paramater*);
unsigned char _IsHynix3D(const FH_Paramater* fh);
unsigned char _IsMicron3D(const FH_Paramater* fh);
unsigned char _IsBiCS3QLC(const FH_Paramater* fh);
unsigned char _IsBiCS3pMLC(const FH_Paramater* fh);
unsigned char _IsBiCS4QLC(const FH_Paramater* fh);
unsigned char _IsTestSamsungMLC(const FH_Paramater* fh);
unsigned char _IsYMTC3D(const FH_Paramater* fh);
int _GetParameterIdxCnt(const FH_Paramater*);

#endif /*_MYFLASHINFO_H_*/
#pragma pack()
