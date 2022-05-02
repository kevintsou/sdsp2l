#ifndef _SATASCSI3111_ST_H_
#define _SATASCSI3111_ST_H_

#ifdef _DUALCORE_SWITCH_
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxtempl.h>
#include "dconfig.h"
#endif
#include "USBSCSICmd_ST.h"


#define SCSI_CRITICAL_FAIL -9 //FPU timeout, DQS MISMATCH
#define FORCE_SCSI 0 //The flag determine all vendor commands will pass via SCSI rather then SSD. for debugging

#define SCSI_IOCTL_DATA_IN_DIRECT_GET SCSI_IOCTL_DATA_IN+99

#define PAD_PARAMETER_ADDR_FLASH_ODT 2
#define PAD_PARAMETER_ADDR_FLASH_DRIVE 3
#define PAD_PARAMETER_ADDR_PAD_ODT_ENABLE 4
#define PAD_PARAMETER_ADDR_PAD_ODT_VALUE 5
#define PAD_PARAMETER_ADDR_PAD_DRIVE_ENABLE 6

typedef struct _ST_HUBINFO2
{
	BYTE xIsSubHub;
	WORD wPortIndex;
	WORD wHubIndex;
	WORD wHubPortOffset;
	WORD wParentHubPortOffset;
	WORD wNumberOfPorts;
	TCHAR tcDevPath[MAX_PATH];
} ST_HUBINFO2;



typedef struct _S11_SCAN_WINDOW_OPTIONS
{
	int FlashODT;
	int FlashDrive;
	int PadODTEnable;
	int PadODT;
	int PadDriveEnable;
} S11_SCAN_WINDOW_OPTIONS;

typedef struct  _MyString
{
	char _string[256];
}MyString;

class SATASCSI3111_ST : public USBSCSICmd_ST
{
public:
    // init and create handle + create log file pointer from single app;
    SATASCSI3111_ST( DeviveInfo*, const char* logFolder );
    // init and create handle + system log pointer
    SATASCSI3111_ST( DeviveInfo*, FILE *logPtr );
    ~SATASCSI3111_ST();
    

private:
    void Init();

///////////////////////////////BASE//////////////////////////////////////////
public:
	virtual int Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len );	
	virtual int Dev_InitDrive( const char* ispfile );		
	virtual int Dev_GetFlashID(unsigned char* r_buf);
	virtual int Dev_CheckJumpBackISP ( bool needJump, bool bSilence );
	virtual int Dev_GetSRAMData ( unsigned char offset, unsigned char* buf, int len );
	virtual int Dev_SetSRAMData ( unsigned char offset, const unsigned char* buf, int len );
	//////////////////////////////////////
	virtual int Dev_SetFlashClock ( int timing );
	virtual int Dev_GetRdyTime ( int action, unsigned int* busytime );
	virtual int Dev_SetFlashDriving ( int driving );
	virtual int Dev_SetEccMode ( unsigned char Ecc );
	virtual int Dev_SetFWSpare ( unsigned char Spare );
	virtual int Dev_SetFlashPageSize ( int pagesize );
	virtual int GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &WHR2);
	int  ATA_SetAPKey();
	virtual bool Identify_ATA(char * version);

protected:
	int _flash_driving;
	int _flash_ODT;
	int _controller_driving;
	int _controller_ODT;
	bool _burnerMode;
	char _fwVersion[8];
	int _TrigerDDR(int CE, const unsigned char* sqlbuf, int slen, int buflen, unsigned char channel );
	virtual int _WriteDDR(UCHAR* w_buf,int Length, int ce=-1);
	int _ReadDDR(unsigned char* r_buf,int Length, int datatype );
	unsigned char _SATABuf[128*512];
	UCHAR _dummyBuf[512];
	UCHAR _CurOutRegister[8],_CurATARegister[8],_PreATARegister[8];
	int _MakeSATABuf( const UCHAR* sql,int sqllen, const UCHAR* buf, int len );
	int  ATA_GetFlashConfig();

	int  ATA_Identify( UCHAR* buf, int len );
	int  ATA_ISPProgPRAM(const char* m_MPBINCode);
	bool ATA_ISPProgramPRAM_ICode(UCHAR* w_buf,UINT SectorNum,UINT gOffset,UINT glasted);
	bool ATA_ISPVerifyPRAM_ICode(UCHAR* w_buf,UINT SectorNum,UINT gOffset);
	int  ATA_Vender_Check_CRC(UCHAR* buf,UINT SectorNum);
	int  ATA_ISPProgramPRAM(UCHAR* buf,UINT SectorNum,bool pDecodeEnable);
	int  ATA_DirectWriteSRAM(UCHAR* buf,UCHAR SectorCount,UINT pCL,UINT pCH);
	int  ATA_DirectReadSRAM(UCHAR* buf,UCHAR SectorCount,UCHAR CL,UCHAR CH);
	int  ATA_Vender_Send_Seed(UCHAR* buf,UINT SectorNum);
	int  ATA_Vendor_Write4ByteReg(UCHAR* buf,UINT Byte_No);
	int  ATA_Vendor_Read4ByteReg(UCHAR* buf,UINT Byte_No);
	int  ATA_ForceToBootRom ( int type );
	int  ATA_ISPJumpMode();
	void GetODT_DriveSettingFromIni();

///////////////////////////////////////Sorting API///////////////////////////////////////////////
public:
	int NT_ScanWindow_ATA(FH_Paramater * FH, int bank_number, S11_SCAN_WINDOW_OPTIONS * options);
	bool Vender_InitialPadAndDrive_3111(UCHAR* buf,UINT sMode);
	bool WriteRead_4Byte_Reg_Vendor(UINT mode,UINT Address,UINT Value,UINT ByteNo);
	bool Vendor_Scan_SDLL(UCHAR* buf,UINT Channel_number,UINT Bank_number,UINT Die_Number,UCHAR SectorCount);
	//CRITICAL_SECTION * SATASCSI3111_ST::GetHubCS(unsigned char hubId, int portIndex);
	HANDLE FindSwitchedHandle(int max_times, UCHAR *inqBuf, bool enableLog = true);
	virtual int ST_GetReadWriteStatus(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf);
	int SetLegacyParameter_and_Command(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, bool bCallResetWhenFail, FH_Paramater * FH, BYTE * ap_parameter);
	int SwitchToggleLegacy(BYTE obType, HANDLE DeviceHandle, char bMyDrv, FLASH_FEATURE TYPE,BYTE bONFIType,BYTE bONFIMode,BYTE bToggle20, FH_Paramater * pPH);
	virtual int GetDataRateByClockIdx(int clk_idx);
	virtual int GetLogicalToPhysicalCE_Mapping(unsigned char * flash_id_buf, int * phsical_ce_array);
#ifdef _DUALCORE_SWITCH_
	virtual int NT_SwitchCmd(int obType, HANDLE handle, char device_letter, CMDTYPE target_type, 
		FH_Paramater * FH = NULL, BYTE * ap_parameter = NULL, bool auto_find_handle = true,  bool writeid2flh = false, BYTE bChangeMode = 0);
	CRITICAL_SECTION * GetHubCS(CString hubId, int portIndex);
	CMap <CString, LPCTSTR, ST_HUBINFO2, ST_HUBINFO2&> m_HubInfoMap;
	UINT GetIndex(DEVINST devInst, DEVINST devInstHub);
	UINT HubPolling();
	void DualCoreObjectsInit(void);
	void DualCoreObjectsFree(void);
#endif
	//////////override////////////
public:
	virtual int BankInit(BYTE bType,HANDLE DeviceHandle_p,BYTE bDrvNum ,_FH_Paramater *_FH,unsigned char *bankBuf);
	virtual int NT_Inquiry_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf, unsigned char *DriveInfo=NULL);
	int GetEccRatio(void);
	// not support on s11 now.
	int NT_ReadPage_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, const char *type, unsigned char *bBuf, unsigned char *DriveInfo);
	int ST_CheckDRY_DQS(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE TYPE,BYTE CE_bit_map,DWORD Block);
	int ST_SetDegree(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int Phase, short delayOffset);
	int ST_FWInit(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	int ST_GetSRAM_GWK(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf);
	int ST_DumpSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf);
	int ST_WriteSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf);
	int ST_SetQuickFindIBBuf(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf);
	int ST_QuickFindIB(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int ce, unsigned int dieoffset, unsigned int BlockCount, unsigned char *buf, int buflen, int iTimeCount,
		BYTE bIBSearchPage, BYTE CheckPHSN);
	int ST_CheckU3Port(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* buf);
	int ST_APGetSetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE SetGet,BYTE Address,BYTE Param0,BYTE Param1,BYTE Param2,BYTE Param3,unsigned char *buf);
	int NT_JumpISPMode(BYTE obType,HANDLE DeviceHandle,char drive);
	int ST_HynixCheckWritePortect(BYTE bType,HANDLE DeviceHandle, BYTE bDrvNum);
	int ST_HynixTLC16ModeSwitch(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bMode);
	int NT_SwitchCmd(void);
	int NT_CustomCmd(char targetDisk, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut);
	virtual int NT_CustomCmd_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool is512CDB=false);
	int NT_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle, char targetDisk , unsigned char *id, bool bGetUID = false,BYTE full_ce_info=0,BYTE get_ori_id=0);
	int IssueVenderFlow(char opType/*0: disk, 1:handler*/, char targetDisk, HANDLE iHandle, unsigned char *CDB, int direct, int dataLength, unsigned char * buf, int timeOut);
	int NT_ISPRAMS11(char * userSpecifyBurnerName);
	int NT_ISPProgPRAMS11(const char * filename, HANDLE handle, bool isForceProgram, FH_Paramater * fh = NULL, char * auto_got_filename = NULL);
	virtual int ST_GetCodeVersionDate(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char* buf);
	int CheckIfLoadCorrectBurner(HANDLE devicehandle, FH_Paramater * fh);
	int ST_ScanBadBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int CE, unsigned char *w_buf,BOOL isCheckSpare00, unsigned int DieOffSet, unsigned int BlockPerDie);
	int ST_SetLDPCIterationCount(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk ,int count);
	virtual int ST_SetGet_Parameter(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bSetGet,unsigned char *buf);
	virtual int ST_SetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMode, unsigned char *w_buf, BYTE bONFIType, BYTE bONFIMode, BOOL bToggle20);
	void EnableDualMode();
private:
	UCHAR TempBuf[512];
	UCHAR CommonBuffer[512];

	//bool _USBMode;
};
typedef SATASCSI3111_ST CNTCommandSSD;
#endif /*_SATASCSI3111_ST_H_*/