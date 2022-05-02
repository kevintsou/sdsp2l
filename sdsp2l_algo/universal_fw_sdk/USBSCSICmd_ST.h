#ifndef _USBSCSICMD_ST_H_
#define _USBSCSICMD_ST_H_
#ifdef _DUALCORE_SWITCH_
//#include <atlstr.h>  //for CString

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#endif
#include "universal_interface/USBSCSICmd.h"

#include <vector>
#include <map>

#include "common_struct.h"
#include "universal_interface/NetSCSICmd.h"
#include "universal_interface/spti.h"
#include "myQtSocket.h"

#define ST_EMODE_SLC             	 1
#define ST_EMODE_2PLANE              (1<<1)
#define ST_EMODE_4PLANE              (1<<6)
#define ST_RWMODE_CONVER             1
#define ST_RWMODE_2PLANE             (1<<1)
#define ST_RWMODE_CACHE              (1<<5)
#define ST_RWMODE_4PLANE             (1<<6)
#define ST_RWMODE_WR_PER_PAGE        (1<<7)


#define _TWO_PLANE_MODE_		0x02
#define _FOUR_PLANE_MODE_		0x40

//B////////////////////////define refer from CNTCommand.h
#define __Debug_Bank__
#define _MAX_DIE_ 16
#define _MAX_CE_ 16
#define __BOOT_MODE__ 1
#define __NORMAL_MODE__ 0 
#define PAGE_MAPPING_SIZE 1024
#define FRONT_PAGE_LOG 0
#define NTCmd_IDMS_ERR_CommandBankFF 0x1119

#ifndef DEF_BANK_BUR_BUFSIZE
#define DEF_BANK_BUR_BUFSIZE  167936+2048
#endif
//++refer MpCommon.h
#define _D1 0xD1
#define _D2 0xD2
#define _D3 0xD3
//--
#define NVME_LOG_FILE  nvme_log.txt

#define NTCmd_IDMS_ERR_CommandBankFF 0x1119

#define ICTYPE_DEF_PS2233 0x12
#define ICTYPE_DEF_PS2237 0x11
#define ICTYPE_DEF_PS2260 0x1A
#define ICTYPE_DEF_PS2261 0x1B // 2261
#define ICTYPE_DEF_PS2262 0x1C // 8 ???? 1 type
#define ICTYPE_DEF_PS2267 0x1D
#define ICTYPE_DEF_PS2268 0x1E
#define ICTYPE_DEF_PS2268_1 0x1F	//2014/04/03 cindy add
#define ICTYPE_DEF_PS2250 0x16
#define ICTYPE_DEF_PS2239 0x15
#define ICTYPE_DEF_PS2232 0x10
#define ICTYPE_DEF_PS2238 0x13
#define ICTYPE_DEF_PS2263 0x14
#define ICTYPE_DEF_PS227X 0x18		//for 2273 and 2275
#define ICTYPE_DEF_PS2275 0x75		//0x75 ???
#define ICTYPE_DEF_PS228X 0x19		//for 2283 and 2285
#define ICTYPE_DEF_PS2285 0x85		//0x85 ???
#define ICTYPE_DEF_PS2301 0x60		//0x60 USB 30 PenDrive
#define ICTYPE_DEF_PS2302 0x61		//0x61 USB 2302 30 PenDrive
#define ICTYPE_DEF_PS2303 0x62		//USB 2303 ??
#define ICTYPE_DEF_PS2303P 0x63		//USB 2303 ?? normal
#define ICTYPE_DEF_PS2307 0x67		
#define ICTYPE_DEF_PS2306 0x66		
#define ICTYPE_DEF_PS2310 0x30
#define ICTYPE_DEF_PS2280 0x80		//0x80 ???
#define ICTYPE_DEF_PS2312 0x22
#define ICTYPE_DEF_PS2313 0x23
#define ICTYPE_DEF_PS2315 0x25
#define ICTYPE_DEF_PS2316 0x26
#define ICTYPE_DEF_PS2308 0x68  
#define ICTYPE_DEF_PS2269 0x20 
#define ICTYPE_DEF_PS2270 0x21 
#define ICTYPE_DEF_PS2309 0x69
#define ICTYPE_DEF_PS2311 0x6b
#define ICTYPE_DEF_PS2319 0x6A
#define ICTYPE_DEF_S11    0xAA
#define ICTYPE_DEF_E13    0xAB
#define ICTYPE_DEF_U17    0xAC
#define ICTYPE_DEF_SD     0x46

//			2305            0x65
//			  12			0x22
//            13			0x23
//			  15			0x25
//			  16			0x26
//			  17			0x27

#define LOCK_TIMEOUT       30 		// 20 Seconds
#define LOCK_RETRIES       10		// 40

#define SCSI_MAX_TRANS			65536		// scsi max transferLength per time


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define RMODE_ECC 0x80
#define RMODE_VIRT 0x40
#define RMODE_WPAGE 0x20
#define RMODE_26SPR 0x1


//================ReadInfo=======================
#define FIRST_INFO						0x01
#define SECOND_INFO						0x02

//================BankingCommand=======================
#define		TSB			0
#define		TSB3D		1
#define		Micron		2
#define		Micron3D	3
#define		Micron3D_N18_B27_N28  4
#define		YMTC		5
#pragma pack(1)
struct TPrivateArea			// 16Bytes
{
	DWORD dwTotalSec;		// in sectors
	DWORD dwPrivateSec;		// in sectors
	DWORD dwIsInPrivate;	// 0=NO, 1=YES
	DWORD dwCurrNOA;		// Number of unsuccessful Attempts to enter the Private Area(initial=0)
	BYTE  dwReserve[16];
};
#pragma pack()
typedef struct bankingcommand
{
	BYTE Command;
	BYTE BankforTypes[10];

}BankingCommand;

struct find_command
{
	BYTE command;
	find_command(BYTE command) : command(command) {}
	bool operator () ( const bankingcommand& bk ) const
	{
		return bk.Command == command;
	}
};

typedef enum { DISK_DEVICE_TYPE_NON_UNKNOW =-1, DISK_DEVICE_TYPE_NON_NVME, DISK_DEVICE_TYPE_NVME } DISK_DEVICE_TYPE;
enum IOCONTROLTYPE { USBONLY=0,C2012,ATAONLY, NVME };
typedef enum { SCSI_NTCMD, NET_NTCMD, SSD_NTCMD } CMDTYPE;
typedef enum { FEATURE_LEGACY, FEATURE_T1, FEATURE_T2, FEATURE_T3 , FEATURE_T3_1200} FLASH_FEATURE;
namespace DUAL_STATUS{
	typedef enum { 
		IDLE=0, 
		RESUME=1,
		REQ_SWITCH_SSD=2,
		REQ_SWITCH_SCSI=4,
		SUSPEND=0x05,
		RUNNING=0x06,
		SWITCH_DONE=8,
		NOT_NEED_SWITCH=16,
		SWITCH_FAIL=32,
		TRIMMING=64
	} THREAD_STATUS;
}

typedef enum {
	I_USB,
	I_NVME,
	I_BRIDGE,//usb to pcie bridge //JMS
	I_BRIDGE_ATA,//usb to ATA Bridge //ASMedia
	I_C2012, //usb to ATA c2012 tester 
} INTERFACE_TYPE;

class BaseSorting
{

public:
	virtual void Log( const char* fmt, ... )=0;
	virtual void OutReport( unsigned int iErrStrID , int slot, unsigned int iReturnErrCode = 0)=0;
};

typedef int (* PrintPtr) (void*, char *fmt, ...);

//E/////////////////////////define refer from CNTCommand.h



class USBSCSICmd_ST : public USBSCSICmd
{
public:

    // init and create handle + create log file pointer from single app;
    USBSCSICmd_ST( DeviveInfo*, const char* logFolder );
    // init and create handle + system log pointer
    USBSCSICmd_ST( DeviveInfo*, FILE *logPtr );

	USBSCSICmd_ST();
    ~USBSCSICmd_ST();
	//int EraseBlockAll(unsigned long long fun, unsigned char start_ce, unsigned char ce_count, unsigned char RWMode, BYTE PageType
	//	, int die, int phyPBADie, int die_startblock, int die_blockcount, DWORD functionTest, unsigned char **statusbuf);


	bool SetBitInValid(unsigned char* bitmap, int idx);
	int GetLogicalCEByPhy(int phySlot, int phyCE, int phyDie, int phyPBADie);
	int GetPageShiftBits();
	bool BitValid(const unsigned char* bitmap, int idx, const unsigned char* bitmap2, const unsigned char* bitmap3);

private:
    void Init();
protected:
	DeviveInfo _default_drive_info;
	unsigned char _PARAMETERS[1024];
	bool m_isSQL;	//Enable always used SQL mode send flash command
	bool _isOutSideHandle;
	long _LogStartTime;


	
///////////////////////////////////////////////////////////////////////////////
//B:Overwrite from NetSCSICmd
///////////////////////////////////////////////////////////////////////////////
	int MyIssueCmd(  unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout=20,int useBridge=false );
	myQtTcpSocket *_socket;
	bool InitSocket( const char* addr, int port );
	void CloseSocket();
///////////////////////////////////////////////////////////////////////////////
//B:Reference from NTCmd
///////////////////////////////////////////////////////////////////////////////


public:
#ifdef _DUALCORE_SWITCH_
	CString _parentHubId;
	CString _portInfo;
#endif
	FLASH_FEATURE m_flashfeature;
	int m_physicalDriveIndex;
	int _realCommand;
	int _ForceNoSwitch;
	int _debugBankgMsg;
	int FlashIdIndex ; // for net debug use
	bool bSCSIControl;
	int currentBank;
	bool isFWBank;
	int _L3_total_fail_cnt_PE;
	int _L3_total_fail_cnt_NON_PE;
	unsigned short _PE_blocks[16][4][6];//Max store 16CE, 4Die, 6 block per Die
	int _offline_qc_report_cnt[32];

	short _win_start[_MAX_CE_*_MAX_DIE_];
	short _win_end[_MAX_CE_*_MAX_DIE_];
	void LogBank(int targetBank);
	void LogHex(const unsigned char* buf, int len );
	FILE * _fileNvme;
	//int m_LogCmdCount[RECORE_CMD_MAX_I][3];
	INTERFACE_TYPE _if_type;

	BYTE bSDCARD;
	BYTE bSTCallByDrivePath;
	BYTE m_CMD[24];
	bool m_recovered_handle;
	unsigned char **mbcBuffer;//[_MAX_DIE_][4096];
	//CNTCommand(int PortId);
	virtual int GetHardDiskSn(char *sHsn);
	std::vector<BankingCommand> CommandList;
	BYTE mCMD2BankMapping[0xff+1];
	BYTE **mCMD2BankMapping_NEW;
	int BankSize;
	int BankCommonSize;
	CMDTYPE m_cmdtype;
	virtual void Log(LogLevel level, const char* fmt, ...);
	virtual int NT_SwitchCmd(int obType, HANDLE handle, char device_letter, CMDTYPE target_type, 
		FH_Paramater * FH = NULL, BYTE * ap_parameter = NULL, bool auto_find_handle = true,  bool writeid2flh = false, BYTE bChangeMode = 0);
	unsigned char m_cacheCDB[512];  //cache for ATA use
	int _ActiveCe;
	CMDTYPE getCmdType();
	//
	//   // network functions.
	virtual BOOL NetDeviceIoControl(
		HANDLE hDevice,              // handle to device of interest
		DWORD dwIoControlCode,       // control code of operation to perform
		LPVOID lpInBuffer,           // pointer to buffer to supply input data
		DWORD nInBufferSize,         // size, in bytes, of input buffer
		LPVOID lpOutBuffer,          // pointer to buffer to receive output data
		DWORD nOutBufferSize,        // size, in bytes, of output buffer
		LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
		LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
		);

	void LogMEM ( char* op, const unsigned char* mem, int size );
	void SetInterFaceType(INTERFACE_TYPE _if);
	INTERFACE_TYPE GetInterFaceType(void);

	virtual int NetGetBin1Value(DWORD *flashCapacity,int flhTotalCapacity ,char *paramfile);
	virtual int NetCheckBin(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BinValue);
	virtual int NetSendCheckIB(BYTE obType,unsigned char *ib);
	virtual int NetStartUp(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	virtual int NetSetStatus(BYTE obType,DWORD status);
	virtual int NetStopAuto( char drive );
	virtual HANDLE NetCreateFile(char drive);
	virtual int NetCloseHandle(HANDLE *devHandle);
	virtual DWORD NetGetLastError();
	virtual DWORD NetGetLogicalDrives();
	//int Netcheck_device(char targetDisk, bool blPhy, bool *blIsNoRunFWDev);
	virtual int NetUnLockVolume(BYTE bType, HANDLE *hDeviceHandle);
	virtual int NetLockVolume(BYTE bType, BYTE bDrv, HANDLE *hDeviceHandle);
	virtual void NetCloseVolume(HANDLE *hDev);
	virtual int NetIsFloppyType(BYTE bDrv);
	virtual int NetGetFileSystem(char bDrv,char *FormatType);
	virtual int NetGetDriveType(char bDrv);
	virtual int NetTestCommnad(BYTE TcpCommand,unsigned short *usFlashid);
	// end network functions
	virtual bool Identify_ATA(char * version);
	int ST_CheckResult(int iResult);
	DWORD GetICType( BYTE* bVersion1C6, BYTE *OTPBuf, char *IcName, DWORD *dwICValue, BYTE bDrv );
	int Trim_U2_to_U3(unsigned char U2_PI_H_Reg,unsigned char U2_PI_L_Reg,unsigned char U2_DIV_Reg,
		unsigned char *U3_DIV_Reg,unsigned char *U3_PI_H_Reg,unsigned char *U3_PI_L_Reg);
	
	int Trim_U3_to_U2(unsigned char *U2_PI_H_Reg,unsigned char *U2_PI_L_Reg,unsigned char *U2_DIV_Reg,
		unsigned char U3_DIV_Reg,unsigned char U3_PI_H_Reg,unsigned char U3_PI_L_Reg);
	//RSA========================================================================================================================
	int	NTU3SS__Set_Current_Cipher_Suite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	int	NTU3SS__RSA_Get_Device_Public_Key(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf);
	//int	NTU3SS__RSA_Set_Host_Public_Key(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf);
	int	NTU3SS__RSA_Set_Host_Public_Key(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf
										,unsigned char *bHostM,unsigned char *bHostE);
	int	NTU3SS__RSA_Set_Host_Random_Challenge(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf,unsigned char *bHostR);
	int NTU3SS__RSA_Verify_Device_Public_Key(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf);
	int NTU3SS__RSA_Get_Device_Random_Challenge(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf);
	int	NTU3SS__RSA_Verify_Host_Public_Key(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf);
	int	NTU3SS__RSA_Set_Host_Secret_Number(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf,unsigned char *bHostS);
	int	NTU3SS__RSA_Get_Device_Secret_Number(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf);
	int	NTU3SS__Start_Secure_Access(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf,BYTE bDomainIndex,BYTE bSecureType);
	int	NTU3SS__Get_Secure_Session_Type(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf,BYTE bDomainIndex,BYTE bSecureType);

	//============================================================================================================================
	int NTU3_Domain_Change_Type(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE bType, WORD wProperty);
	int NTU3_Domain_Set_Flags(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, WORD wProperty);
	int	NTU__Read_Protected_Page(BYTE bType,HANDLE DeviceHandle,char drive, WORD wPageIndex, BYTE *bPW, BYTE *bBuf, WORD wBufLen);
	int	NTU__Write_Protected_Page(BYTE bType,HANDLE DeviceHandle,char drive, WORD wPageIndex, BYTE *bPW, BYTE *bBuf, WORD wBufLen);
	int	NTU3__Write_Page(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, WORD wPageIndex, BYTE *bBuf, WORD wBufLen);
	int	NTU3__Scalable_Write_Page(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, int wPageIndex, WORD wCount, BYTE *bBuf, DWORD wBufLen);
	int NTU3_U3PRA__Configure_Private_Area(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen);
	int NTU3_U3PRA__Configure_Private_Area(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen, BYTE bMode);
	int NTU3_U3PRA__Configure_2nd_Password(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen);
	int NTU3_Get_Area_Parameters(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, TPrivateArea *tPA);
	int NT_DevWriteBuf( unsigned char bDrv,	HANDLE DevcieHandle,unsigned char *pBuf,DWORD  dwStartAddr,DWORD  dwTotalSec,BOOL   blHidden );
	int NT_DevReadBuf(  unsigned char bDrv,HANDLE DevcieHandle,	unsigned char *pBuf,DWORD  dwStartAddr,DWORD  dwTotalSec,BOOL   blHidden );
	int NT_U3_WriteInfoBlock(BYTE bType,HANDLE DeviceHandle,char targetDisk, unsigned char *buf);
	int NTU3_U3PRA__Generate_Random_number(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE *bBuf, WORD wBufLen);
	int	NTU__Set_Write_Password(BYTE bType,HANDLE DeviceHandle,char drive, WORD wPageIndex, BYTE *bPW);
	int	NTU__Set_Read_Password(BYTE bType,HANDLE DeviceHandle,char drive, WORD wPageIndex, BYTE *bPW);
	int NTU3_Open(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen, BYTE bMode);
	int NTU3_Close(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex);
	int NTU3_Ready_Not_Ready_Pulse(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bLun);
	int	NTU3_Get_Device_Configuration(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE *Dev);
	int	NTU3_Query_Service(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, WORD wServiceType, BYTE *bBuf, DWORD dwBufLen);
	int NTU3_CD_Lock(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen);
	int NT_GetVoltage(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf);
	int NT_CheckIBStatus(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf);
	int NT_DetectVoltage(BYTE bType, HANDLE DeviceHandle_p, char drive,BYTE voltage,BYTE type,BYTE burnerFlow,BYTE  bInterleave);
	int NT_GetIBTable(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf);
	int GetRetryTables( unsigned char * d1_retry_table, unsigned char * retry_table);
	int GetTestPageCount(BYTE pageType);
	void SetTestPagePercentage(int);
	void UpdateDeviceHandle(HANDLE h);
	void log_nvme( const char* fmt, ... );
	void SetNvmeRecordingSize(int sizek);
	bool recomposeCDB_base(UCHAR *cdb/*i/o*/, int direct/*i*/);
	int ST_SetGetDefaultLevel(bool bSet,unsigned char ce,unsigned char *buf);
/*
int Get_Info_Page(char drive,char *p_cType, unsigned char *p_byBuf);

int NT_CustomCmd__( char bDrv, 
				  unsigned char *CDB, unsigned char bCDBLen,	
				  int direc, 
				  unsigned int iDataLen, unsigned char *buf,		
				  unsigned long timeOut);

HANDLE MyCreateFile__(char bDrv);
*/
	unsigned long BootCheckSum(unsigned char * buf, unsigned int numberOfByte);
	int ReadInfoBlock(BYTE obType,BYTE *bInfoBlock,char targetDisk,HANDLE DeviceHandle ,BYTE InfoNumber,DWORD ICType,BYTE bFC1,BYTE bFC2);

	int NT_FlashBitConfiguration(BYTE bType,HANDLE DeviceHandle,char targetDisk, int bitType);
	unsigned int NT_Unlock(char targetDisk);
	BOOL NT_Burner_ScanBAD(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk,BYTE EXPBA_CE,BYTE CE,BYTE Zone,BYTE Block_H,BYTE Block_L, ULONG dwBufLen,BYTE *bBuf);
	int	NT_Toggle2Lagcy(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	int	NT_Legacy2Toggle(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	int	NT_T2LAndL2T(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,BYTE Type, BYTE DieCount, BYTE BFlashReset ,BYTE bisSLC,int CE);
	int	NT_SortingCodePowerOn(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,BYTE BPowerOn);
	int	NT_T2LAndL2T_Rescue(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE Type);
	int	NT_Toggle2LegacyDetectBitNumber(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char*buf);
	int	NT_Toggle2LegacyReadId(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char*buf);
	int	NT_Toggle2LegacyGetSupportId(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char*buf);
	int NT_CardReader_FW_init(BYTE bType,HANDLE DeviceHandle,BYTE drive);
	int NT_CardReader_PC2RDR_Status(BYTE bType,HANDLE DeviceHandle,BYTE drive);
	int NT_CardReader_RDR2PC_Status(BYTE bType,HANDLE DeviceHandle,BYTE drive, unsigned char *buf);
	int NT_CardReader_ATR_Status(BYTE bType,HANDLE DeviceHandle,BYTE drive, unsigned char *buf);
	int NT_CardReaderSend(BYTE bType,HANDLE DeviceHandle,BYTE drive, unsigned char *buf);
	int NT_CardReaderRead(BYTE bType,HANDLE DeviceHandle,BYTE drive, unsigned char *buf);
	int NT_GetDieInformat(BYTE bType ,HANDLE DeviceHandle,char drive ,unsigned char *Buf );
	int Direct_Read_Status(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char *buf);
	int NT_SetCCIDName(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf);
	int NT_ModeSense( BYTE bType,HANDLE DeviceHandle,char targetDisk, unsigned char *buf );
	int NT_StartStopDrv( BYTE bType ,HANDLE MyDeviceHandle,char targetDisk,  bool blStop);
	int NT_GetOPT(BYTE bType,HANDLE MyDeviceHandle,BYTE drive, unsigned char *buf);
	int NT_StartLED_All(BYTE bType,HANDLE DeviceHandle,  BYTE bDrvNum ,BYTE FlashType);
	int NT_Get_FAB(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,unsigned char *bBuf);
	int NT_Get_CharInfo(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,unsigned char *bBuf);
	int NT_TuningDisk(BYTE bType,HANDLE DeviceHandel,unsigned char bDrv,unsigned char BlockSize);
	int NT_GetErrCEInfo30(BYTE bType,HANDLE DeviceHandle,char targetDisk, unsigned char *buf);
	int NT_RequestSense(BYTE bType,HANDLE DeviceHandle,char targetDisk, unsigned char *buf);
	int NT_GetDriveCapacity(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf);
	int NT_ReadCount1_2307(BYTE bType,HANDLE DeviceHandle_p,char drive, int * count1/*Output*/);
	virtual int NT_Inquiry_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf, unsigned char *DriveInfo=NULL);
	virtual int NT_CustomCmd_ATA_Phy_orig(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut);
	virtual int NT_CustomCmd_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool is512CDB=false);
	int NT_CustomCmd_in_out_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut);
	virtual int NT_CustomCmd_ATA_IO(BYTE bType,HANDLE deviceHandle, char targetDisk, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool copybuf);

	virtual int NT_CustomCmd( char targetDisk, unsigned char *CDB, unsigned char bCDBLen, int direc,unsigned int iDataLen,unsigned char *buf,unsigned long timeOut);
	//新增一個 CustomCmd_Common for common command用
	//避免呼叫 NT_CustomCmd_Phy 可能沒寫好 會造成遞迴...
	virtual int NT_CustomCmd_Common(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut);
	virtual int NT_ReadPage_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, const char *type, unsigned char *bBuf, unsigned char *DriveInfo=NULL);
	virtual int SetAPKey_ATA(HANDLE devicehandle);
	int NT_ReadGroupInfo(BYTE bType,HANDLE DeviceHandle_p,BYTE bDrvNum ,unsigned char *bBuf);
	int NT_WriteVerify_Sectors(char targetDisk,unsigned long address, int secNo, unsigned char *buf, 
						    bool fRead, bool hidden, BYTE blspec ,HANDLE hDeviceHandle,bool isover1T);	// ???
	int NT_WriteVerify_Sectors(char targetDisk,unsigned long address, int secNo, unsigned char *buf, 
							bool fRead, bool hidden, BYTE blspec ,HANDLE hDeviceHandle, int iMun,bool isover1T);
 	int NT_readwrite_sectors( char targetDisk,unsigned long address, int secNo, unsigned char *buf, 
 						  bool fRead, bool hidden, BYTE blspec ,bool isover1T);
	int NT_readwrite_sectors_l( char targetDisk, unsigned long address, int secNo, unsigned char *buf, 
		bool fRead, bool hidden, BYTE blspec ,bool isover1T);

	int NT_readwrite_sectors( char targetDisk,unsigned long long address, int secNo, unsigned char *buf, 
						  bool fRead, bool hidden, BYTE blspec, int iMun  ,bool isover1T);
 	int NT_readwrite_sectors_H( HANDLE hDeviceHandle,unsigned long address, int secNo, unsigned char *buf, 
 						  bool fRead, bool hidden, BYTE blspec ,bool isover1T,int timeout=10);
	int NT_readwrite_sectors_Hl( HANDLE hDeviceHandle,unsigned long address, int secNo, unsigned char *buf, 
		bool fRead, bool hidden, BYTE blspec ,bool isover1T);

	int NT_readwrite_sectors_H( HANDLE hDeviceHandle,unsigned long address, int secNo, unsigned char *buf, 
						  bool fRead, bool hidden, BYTE blspec, int iMun ,bool isover1T,int timeout=10);

	int NT_sRAM_readwrite_sectors(BYTE bType, HANDLE hDeviceHandle, char targetDisk,long address, int secNo, unsigned char *buf,bool fRead );

	virtual int NT_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle, char targetDisk , unsigned char *id, bool bGetUID = false,BYTE full_ce_info=0,BYTE get_ori_id=0);
	bool CheckIfValidFlashId(HANDLE h);
	int NT_Direct_Op(BYTE bType ,HANDLE MyDeviceHandle,  char targetDisk , unsigned char directFunc , unsigned char *id );
	int NT_Direct_Read_Page(BYTE bType,HANDLE DeviceHandel,char drive , unsigned char zone ,
							unsigned int block , unsigned int page,
							unsigned char para , bool bIs2154CE,
							unsigned char *buf,int length = 528 );
	int NT_Direct_Write_Page(BYTE bType,HANDLE DeviceHandel,char drive , unsigned char zone ,
											unsigned int block , unsigned int page,
											unsigned char para , bool bIs2154CE,
											unsigned char *buf );

	int NT_SetMediaChange(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	int NT_GetFW_CodeAddr(BYTE bType,HANDLE DeviceHandel,char drive,unsigned char *buf);

	int NT_Direct_Write_Page2048(BYTE bType,HANDLE DeviceHandel,char drive , unsigned char zone ,
		unsigned int block , unsigned int page,
		unsigned char para , bool bIs2154CE,
		unsigned char *buf );

	int NT_Direct_Erase(BYTE bType,HANDLE DeviceHandel,char targetDisk, BYTE bZone, WORD wBlk);
	int NT_Get_BC(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf,BYTE BCAddr, BYTE CDB5=0);

	int NT_BootRomDisable_EDO(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum);

	int NT_Disk_Op( BYTE bType ,HANDLE DeviceHandle,char drive, unsigned char directFunc, 
					unsigned int para1 , unsigned char *id);
	int NT_Password_Op(BYTE bType ,HANDLE DeviceHandle,char drive , unsigned char directFunc 
		, bool donotWait , unsigned char *id );

	int NT_write_blks(BYTE bType,HANDLE DeviceHandel, char targetDisk, long address, int secNo,
					unsigned char *buf, BYTE bBlkPreSec, BOOL hidden, bool blspec);
	//int NT_write_blks(BYTE bType,HANDLE DeviceHandel, char targetDisk, long address, int secNo,
	//				  unsigned char *buf, BYTE bBlkPreSec, BOOL hidden, bool blspec, BYTE bMun);
	int NT_TestUnitReady(BYTE bType,HANDLE MyDeviceHandle, char targetDisk , unsigned char *senseBuf );
	int NT_WriteTempInfoBlock(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf);
	int NT_WriteInfoBlock(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf);
	int NT_WriteInfoBlock_K(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf);
	int NT_WriteInfoBlock_23(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf);
	int NT_WriteInfoBlock_23_K(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf, DWORD dwICType);
	int NT_USB23Switch(BYTE bType,HANDLE DeviceHandle,char targetDisk, BYTE TYPE);
	virtual int C2012PowerOnOff(BYTE bType,HANDLE MyDeviceHandle,char targetDisk,UINT Mode,UINT Delay,UINT Voltage_SEL,UINT DDR_PWR_Flag,bool m_unit_100ms);

	unsigned int NT_EMMC_Preformat_GenH( HANDLE DeviceHandle_p,			//char drive,
		DWORD StartLba,DWORD Length,unsigned char PreformatType);


	unsigned int NT_Direct_Preformat_GenH( HANDLE DeviceHandle_p,			//char drive,
										unsigned char preFormatType,
										bool mark, bool maxblk,
										WORD wBlock,
										unsigned short *spare,
										//unsigned short spare1,
										//unsigned short* spare,
										bool onePlane, 
										bool bEnableCopyBack,
										bool bForcePreformat,
										unsigned char *bVersionPage,DWORD ICValue
										,unsigned char *firmware
										,unsigned char *falshid,BYTE flash_maker,BYTE Rel_Factory,bool bCloseInterleave);
	unsigned int NT_Direct_Preformat_GenH6(BYTE bType,BYTE bDrv, HANDLE MyDeviceHandle,			//char drive,
		unsigned char preFormatType,
		bool mark, bool maxblk,
		WORD wBlock,
		unsigned char *preformatBuf,
		//unsigned short spare1,
		//unsigned short* spare,
		
		
		bool onePlane, 
		bool bEnableCopyBack,
		bool bForcePreformat,
		unsigned char *bVersionPage);
	int NT_Set_Preformat_BlockRatio(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,unsigned char CECount,unsigned char DieCount, unsigned char *buf);

#if 0
		( HANDLE DeviceHandle_p,			//char drive,
		unsigned char preFormatType,
		bool mark, bool maxblk,
		WORD wBlock,
		unsigned char *preformatBuf,
		//unsigned short spare1,
		//unsigned short* spare,
		bool onePlane, 
		bool bEnableCopyBack,
		bool bForcePreformat,
		unsigned char *bVersionPage);
#endif
	void nt_log( const char *fmt, ... );
	void nt_log_cdb(const unsigned char * cdb,const char *fmt, ... );
	void nt_file_log( const char* fmt, ... );
	void (*logCallBack)(char * logstring);//for nt command user can log its UI or log format. 
	void SetLogCallBack(void (*logcbfun)(char * logstring));
	void SetFakeBlock( const int * ranges );
	virtual int GetTemperature(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk, FH_Paramater * fh); 
	int convertTemperature(int value_fw, FH_Paramater * fh); //return 9999 means failed
	HANDLE MyCreateFile_Phy(BYTE bPhyNum);
	int MyUnLockVolume(BYTE bType, HANDLE *hDeviceHandle);
	int MyLockVolume(BYTE bType, BYTE bDrv, HANDLE *hDeviceHandle);
	int	LockVolume(HANDLE hVolume);
	int UnlockVolume(HANDLE hVolume);
	int DismountVolume(HANDLE hVolume);
	void SetFlashParameter(FH_Paramater * fh);
	HANDLE MyOpenVolume(char drive);
	void MyCloseVolume(HANDLE *hDev);
	int MyCloseHandle(HANDLE *devHandle);
	int NT_read_sec2blks(BYTE bType,HANDLE DeviceHandel, char targetDisk, long address, int secNo,
							unsigned char *buf, BYTE bBlkPreSec, bool hidden, bool blspec);
	int	NTU3__Read_Page(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, WORD wPageIndex, DWORD dwPageTag, BYTE *bBuf, WORD wBufLen);
	int	NTU3__Scalable_Read_Page(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, unsigned int wPageIndex, unsigned int wCount, BYTE *bBuf, DWORD wBufLen);
//	int NT_RequestSense(BYTE bType,HANDLE DeviceHandel,char targetDisk, unsigned char *buf);
	//=========ISP==================================================
	virtual int NT_JumpISPMode(BYTE bType,HANDLE DeviceHandle_p,char drive);
	int NT_JumpFWISPMode(BYTE bType,HANDLE DeviceHandle_p,char drive);
	int NT_JumpISPMode63(BYTE bType,HANDLE DeviceHandle_p,char drive);
	int NT_JumpISPModeDisconnect(BYTE bType,HANDLE DeviceHandle_p,char drive);
	int NT_JumpISPModeDisconnect_Security(BYTE bType,HANDLE DeviceHandle_p,char drive);
	int NT_JumpBootCode(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char mode);
	int NT_JumpBootCodeByST(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char mode);
	int NT_VerifyBurner(BYTE bType,HANDLE DeviceHandle,char drive);
	int NT_VerifyFW(BYTE bType,HANDLE DeviceHandle,char drive,unsigned char mode);
	int NT_GetISPStatus(BYTE bType,HANDLE DeviceHandle,char drive, unsigned int *s1);
	int NT_GetCommandSequence(BYTE bType,HANDLE DeviceHandle,char drive, unsigned int *s1);
	//=======usb 3.0===================================================
	int NT_QuickWrite30(BYTE bType,HANDLE DeviceHandel,unsigned char bDrv,DWORD dwStartUnit,DWORD dwLength,WORD wValue);
	int NT_GetISPStatus30(BYTE bType,HANDLE DeviceHandle,char drive, unsigned char *s1);
	int NT_BurnerVerifyFlash30(BYTE bType,HANDLE DeviceHandle,char drive, unsigned char *buf,BYTE block,BYTE page,BYTE badOption);
	int NT_BufferReadISPData30(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int length, unsigned char mode, unsigned char verify, unsigned char *buf);
	int NT_ISP_EraseAllBlk30(BYTE bType,HANDLE DeviceHandle,char drive, BYTE bFlashIndex,BYTE bDieIndex,int bEraseUnitCount, BYTE bEraseMode, int iStartBlock = 0);
	int NT_ISP_Bics3WaferErase(BYTE bType,HANDLE DeviceHandle,char drive, BYTE bFlashIndex,BYTE bDieIndex, int iStartBlock);
	int NT_EraseSystemUnit30(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk);
	int NT_JumpISPMode30(BYTE bType,HANDLE DeviceHandle_p,char drive);
	//=============
	int NT_TransferISPData(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int length, unsigned char mode, unsigned char verify, unsigned char *buf);
	int NT_BufferReadISPData(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int length, unsigned char mode, unsigned char verify, unsigned char *buf);
	int NT_ISPSwitchMode(BYTE bType,HANDLE DeviceHandle,char drive, unsigned char mode,unsigned char *DriveInfo=NULL);
	int NT_ISPGetFlash(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf, WORD wBufSize);
	int NT_ISPGetConnectType(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf, WORD wBufSize);
	int NT_ISPTestConnect(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf, WORD wBufSize);
	int NT_ISP_EraseAllBlk(BYTE bType,HANDLE DeviceHandle,char drive, BYTE bFlashIndex, BYTE bEraseMode);
	int NT_GetIDBlk(BYTE bType,HANDLE DeviceHandle,char obDrv, BYTE *idblk);
	int NT_ISPIssueConfigFalsh(BYTE bType,HANDLE DeviceHandle,char drive,unsigned char *buf,unsigned char g_ucFCE,unsigned char FZone,unsigned char FBlock_H,unsigned char FBlock_L);
	int NT_ISPIssueReadPage(BYTE bType,HANDLE DeviceHandle,char drive ,unsigned char *buf,unsigned char pageL, unsigned char pageH,
							UCHAR mode,unsigned char g_ucFCE,unsigned char FZone,
							unsigned char FBlock_H,unsigned char FBlock_L);
	int NT_IspConfigISP_CP(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int cnt, unsigned char mode,BYTE *buf);
	int NT_IspConfigISP(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int length, unsigned char mode, unsigned char verify, unsigned char *buf);
	//finger=========================================================
	int NT_FingerSwitch_Start(BYTE bType,HANDLE DeviceHandle,char drive);
	int NT_MediaChange(BYTE bType,HANDLE DeviceHandle,char drive);
	int NT_FingerSwitch_Process(BYTE bType,HANDLE DeviceHandle,char drive);
	int NT_FingerSwitch_Result(BYTE bType,HANDLE DeviceHandle,char drive);
	int NT_GetWriteProtect(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char *p_byStatusOfDisk);
	int NT_SetWriteProtect(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char byLUN, unsigned char byType);
	int NT_BurnerTest1Block(BYTE bType ,HANDLE MyDeviceHandle,char drive);
	int NT_Get_RDYTime(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrv, unsigned char *bBuf,BYTE bMode);
	HANDLE MyCreateFile(char drive);
	int NT_Get_OTP(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf);
	int NTU3_SetLED(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk, char bMode);
	int NTU3_GetLED(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk, BYTE *bBuf);
	int NTU3_GetHint(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk, WORD wPageIndex, DWORD dwPageTag, BYTE *bBuf, WORD wBufLen);
	int NTU32SS_Command(BYTE bType, HANDLE DeviceHandle_p, BYTE targetDisk, unsigned char directFunc,
						unsigned char *cmd, unsigned char *bBuf, int length);
	int NT_FWSorting30(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf, BYTE Bmode);
	int NT_FWSorting30Result(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf);
	int NT_GetFWAuthentication(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf);
	int NT_FWAuthentication(BYTE bType,HANDLE DeviceHandle_p,char drive,DWORD ICType,BOOL BIsInit);
	int NT_SetFWAuthentication(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf);
	int NT_GetFlashUniqueID(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk , unsigned char *id, BYTE Chanelcount, BYTE CECount, BYTE DieCount);
	int NT_Flushcommand(BYTE bType,HANDLE DeviceHandle,  BYTE bDrvNum );
	virtual int GetDefaultDataRateIdxForInterFace(int interFace);
	int NT_GetNowEfuseData(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *Efuse_Data);
	int NT_GetCodeEfuseData(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *Efuse_Data);
	int NT_GetEfuseData70(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf);
	int NT_GetEfuseData(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf1);//cindy add for long scan tool
	int FlashReset(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int CE_Sel);
	int TurnFlashPower(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool ON);
	int NT_EMMC_LoadBR(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int datalen, unsigned char *bBuf);
	int NT_EMMC_LoadFW(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int datalen, unsigned char *bBuf);
	int NT_EMMC_BuildDefaultSystemArea(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,  unsigned char *buf);
	int NT_EMMC_InitializeMMCHosts(BYTE bType,HANDLE DeviceHandle,char drive);
	int NT_EMMC_AdjustCommonFrequency(BYTE bType, HANDLE DeviceHandle, char drive, unsigned char ucChannel, unsigned char ucFrequencySelect, unsigned char ucSDROrDDRModeSelect);
	int NT_EMMC_SwitchDataBusWidth(BYTE bType, HANDLE DeviceHandle, char drive, unsigned char ucChannel, unsigned char ucDataBusWidthSelect);
	int NT_EMMC_OutputClockToeMMC(BYTE bType, HANDLE DeviceHandle, char drive, unsigned char ucChannel, unsigned char ucOutputOrNot);
	int NT_EMMC_SendClocksToeMMC(BYTE bType, HANDLE DeviceHandle, char drive, unsigned char ucChannel, unsigned char ucClockCount);
	int NT_EMMC_StartLED_All(BYTE bType,HANDLE DeviceHandle,  BYTE bDrvNum);
	int NT_EMMC_GetStatus(BYTE bType, HANDLE DeviceHandle, char drive, BYTE bChannel, unsigned char *buf);
	int NT_CheckVT(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char *buff);
	//kim add
	int NT_ErsCntRead(BYTE bType ,HANDLE MyDeviceHandle,char drive, unsigned char *ucBuffer, unsigned short usVirZone, unsigned char ubMode);
	int NT_ErsCntWrite(BYTE bType ,HANDLE MyDeviceHandle,char drive, unsigned char *ucBuffer);
	int NT_Get_Bin(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf);
	int NT_GetDieInfo(BYTE bType ,HANDLE MyDeviceHandle,char drive, unsigned char *buf);
	int NT_WearLeveling_read(BYTE bType ,HANDLE MyDeviceHandle,char drive, int DieNum, unsigned char *buf, int iBufSize);
	int NT_WearLeveling_write(BYTE bType ,HANDLE MyDeviceHandle,char drive, int DieNum, unsigned char *buf, int iBufSize);
	int Get_PowerUpCount(BYTE bType ,HANDLE MyDeviceHandle,char drive, unsigned char *buf);
	//kim add for 2312
	int NT_ISP_Read_UpdateFWCount(BYTE bType,HANDLE DeviceHandle_p,char drive,unsigned char *buf);
	int NT_JumpISPMode_2312Controller(BYTE bType,HANDLE DeviceHandle_p,char drive);

	int NT_ISP_EraseAllBlk_2312Controller(BYTE bType,HANDLE DeviceHandle,char drive,BYTE *bBuf, BYTE bEraseMode);
	int U32l_SCSI_Write_CD_H(HANDLE hDeviceHandle, long address, int secNo, unsigned char *buf,
		bool fRead, BYTE bDomainIndex);
	int NT_IspConfigISP_CP30(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int cnt, unsigned char mode,BYTE *buf);
	int NT_GetFlashMode(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf);
	int	 _forceWriteCount;
	int NT_SetMediaChanged( BYTE bType ,HANDLE DeviceHandle,char drive);
	int NTU3_Open(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen);
	int NT_ScanBadBlocks30(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf, int length);
	int NT_ScanFlashID30(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf, int length);
	int NT_Erase_Syetem_Code30(BYTE bType,HANDLE DeviceHandle_p,char drive);
	int NTU3_CD_Lock(BYTE bType, HANDLE MyDeviceHandle, BYTE targetDisk, BYTE bCDKeyIndex, int datalen, unsigned char *buff);
	int	NT_Toggle2Legacy(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	int NT_BurnerState_GetIB(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf);
	int NT_Rom_rewrite_flash_ID(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char CE
		, unsigned char original_dvice_code, unsigned char new_dvice_code);
	int NT_Get_flash_ID_by_CE_Channel_Die(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char *buf);
	int NT_ISPGetStatus(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf, WORD wBufSize);
	int NAND_READ_ID_TEST(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,UCHAR* r_buf , ULONG bufsize);
	int NAND_SLC_Write(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,UCHAR* w_buf , ULONG bufsize , UINT FBlock,unsigned char WL);
	int NAND_MLC_TEST1_Write(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, UCHAR* w_buf ,BYTE bCE , ULONG bufsize , UINT FBlock);
	int NAND_BadCol_TEST(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,UCHAR* r_buf , UCHAR PLAN);
	int NAND_WP_TEST_START(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum);
	int NAND_WP_TEST(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,UCHAR* r_buf , UCHAR WP_Enable);
	int NAND_WP_TEST_END(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum);
	int PreWriteInfoBlock(BYTE bType,HANDLE DeviceHandle_p,char drive);
	int NT_SetWriteInfoBlockRand(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf);
	int NT_GetWriteInfoBlockRand(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf);
	int NT_ScanWindow(BYTE bType,HANDLE DeviceHandel,unsigned char bDrv, BYTE Channle, BYTE CE, unsigned short Length, unsigned char *bBuf);
	int NT_GetFlashExtendID_1040(BYTE bType,HANDLE DeviceHandel,unsigned char bDrv, unsigned short Length, unsigned char *bBuf);
	int NT_GetIBBuf(BYTE bType, HANDLE DeviceHandel, unsigned char bDrv, unsigned char *bBuf);
	int NT_ISP_ReadTarget(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char target, unsigned char target1,unsigned char plane,unsigned char page,unsigned char mode,unsigned char eccbit,unsigned char SecCnt,unsigned char rowRead,unsigned char *bBuf);
	int NT_ReviseFWVersion(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf);
	int NT_GetD3RRT(BYTE obType, HANDLE DeviceHandle_p, BYTE targetDisk, unsigned char *buf);
	int NT_GetRetryTablePass(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf);
	//Sorting code
	//int ST_CheckDRY_DQS(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE TYPE);
	virtual int ST_HynixCheckWritePortect(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum );
	virtual int GetEccRatio(void);
	virtual int ST_CheckDRY_DQS(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE TYPE,BYTE CE_bit_map,DWORD Block);
	virtual int ST_DumpSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf);
	virtual int ST_WriteSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf);
	int ST_2PlaneCopyBackWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int TestBlock,unsigned int SourceBlock);
	int ST_TurnFlashPower(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool ON);
	int ST_DirectWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool Plane,bool CreateTable,bool LastPage,bool FirstPage,bool MarkBad,unsigned int CE_Sel,unsigned int block,unsigned int page,unsigned int sector,unsigned char *w_buf);
	virtual int ST_FlashGetID(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool Real_Id,unsigned char *r_buf);
	int ST_GetInformation(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned int CE, unsigned int Die,unsigned char *r_buf);
	int ST_AssignFTAforD1Read(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE FTA0,BYTE FTA1,BYTE FTA2,BYTE FTA3,BYTE FTA4 );
	int ST_GetInfoBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int type,BYTE blk_addr1,BYTE blk_addr2, unsigned char *r_buf);
	virtual int ST_GetSetRetryTable(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet
				,BYTE bSetOnfi,BYTE *Buf, int length,BYTE ce,BYTE bRetryCount = 0, BYTE OPTAddress=1);
	int ST_GetB85T_OTP_Table(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet,BYTE bSetOnfi,BYTE *r_buf );
	int ST_DirectRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool DelayRead, int DReadCnt, bool SkipECC, bool Inv, bool Ecc, bool Plane, bool Cache, bool Cont, unsigned int CE_Sel,unsigned int block,unsigned int page,unsigned int sector, unsigned char ECCBit,unsigned char *r_buf);
	int ST_2A_GetBadBlockInfo(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int Sector,unsigned char *r_buf);
	int ST_ReliabilityTestingDirectWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bCE,WORD wBLOCK,int Sector ,unsigned char *w_buf);
	int ST_MarkBadBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bCE,WORD wBLOCK);
	int ST_ReliabilityTestingDirectRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bCE,WORD wBLOCK);
	int ST_EraseAll(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char PBADie, unsigned char AP_Code, unsigned int StartBlock, unsigned int EndBlock);
	int ST_DirectErase(BYTE bType,HANDLE DeviceHandle,char drive, unsigned int CE_Sel,unsigned int block);
	int ST_WriteOneBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int TestBlock,int datalen,BYTE*w_buf);
	int ST_ReadOneBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ECC_Bit,unsigned int TestBlock,BYTE bPlaneMode,bool bCopyBack);
	int ST_PollStatus(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf,bool bReturnAverage=false);
	int ST_PollData(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf);
	int ST_SetClock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE param3,BYTE param4,BYTE param7,unsigned int FlashReadDiv,unsigned int FlashWriteDiv,bool TurnOffEDO);
	int ST_SetSram(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE address,BYTE OpenBit,BYTE Open);
	virtual int ST_GetReadWriteStatus(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf);
	int ST_GetProgramCountStatus(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf);
	int ST_GetProgramCountStatus1(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf);
	virtual int ST_SetGet_Parameter(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bSetGet,unsigned char *buf);
	virtual int ST_SetQuickFindIBBuf(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf);
	virtual int ST_QuickFindIB(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int ce, unsigned int dieoffset, unsigned int BlockCount, unsigned char *buf, int buflen, int iTimeCount,
		BYTE bIBSearchPage, BYTE CheckPHSN);

	int ST_2311TranSRam(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf,int startAdd,int len,int _write);
	int ST_TranSRamThroughFlash(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf,int startAdd,int block,int page,int len,int _write);

	virtual int ST_DirectOneBlockWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode
		,BYTE bCE,WORD wBLOCK,BYTE SkipSector,int SectorCount ,unsigned char *w_buf,UINT32 bDontDirectWriteCEMask, bool bNewFormat=false);
	virtual int ST_D3PageWriteRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckEcc,BYTE bRetry_Read,int datalen ,unsigned char *w_buf,UINT32 bDontDirectWriteCEMask,BYTE DumpWriteData=0,bool bNewFormat=false);
	virtual int ST_SkipSector(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bSetSkip,unsigned char *w_buf);
	int ST_MicroSetRandomize(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE OpenClose);
	virtual int ST_MarkBad(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE,WORD wBLOCK,WORD wStartPage,WORD wPageCount);

	int ST_WriteIB(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckECC,WORD bRetry_Read,int a2command,int datalen ,int bOffset,unsigned char *w_buf,
		BYTE TYPE,DWORD Bank,BYTE S0,BYTE S1,BYTE S2,BYTE S3);

	virtual int ST_DirectPageWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckEcc,BYTE bRetry_Read,int a2commnad,int datalen,int bOffset ,unsigned char *w_buf,
		BYTE TYPE,DWORD Bank, BYTE bIBmode=0, BYTE bIBSkip=0, BYTE bRRFailSkip=0, bool bNewFormat=false);

	int ST_SetBlockSeed(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,WORD wBLOCK);

	int ST_DirectPageWriteByConv(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckECC,BYTE bRetry_Read,int a2command,int datalen ,int bOffset,unsigned char *w_buf,
		BYTE TYPE,BYTE BlockORPageSeed,BYTE Seed,DWORD Reserve);

	virtual int ST_BlockWriteByPage(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckEcc,BYTE bRetry_Read,int a2commnad,BYTE bBlockPageMode,int datalen,int bOffset ,unsigned char *w_buf,BYTE TYPE,DWORD Bank);

	int ST_DirectPageReadByConv(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD bPageCount,BYTE bECCBit,BYTE Retry_Read,BYTE bOffset,int datalen ,BYTE BlockORPageSeed,BYTE Seed,unsigned char *w_buf);
	virtual int ST_DirectPageRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD bPageCount,BYTE bECCBit,BYTE Retry_Read,UINT32 bCeMask,int datalen ,unsigned char *w_buf, bool bNewEccFormat=false);
	virtual int ST_DirectPageRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
		,WORD wStartBLOCK,WORD wPage,WORD wBlockCount,BYTE bECCBit,BYTE Retry_Read,int a2command,UINT32 bCeMask,int datalen ,unsigned char *w_buf, bool bNewEccFormat=false);
	int ST_DirectPageWriteBufferMode(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckECC,WORD bRetry_Read,int a2command,int datalen ,int bOffset,unsigned char *w_buf,BYTE TYPE,DWORD Bank);

	int ST_YMTC_Workaround(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);

	int ST_TestINOUT(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE Mode,BYTE bCE,BYTE bEraeTimes,unsigned char *w_buf);

	int ST_GetFixtureHubID(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf);

	int ST_E2NAMEGetEarlyTable(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char ce, unsigned char die,unsigned char *r_buf);
	virtual int ST_CopyBackWiteD3(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE,BYTE bEraeTimes,unsigned char *w_buf);
	virtual int ST_CopyBackWite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE Mode,BYTE bCE,unsigned short Page,BYTE bEraeTimes,unsigned char *w_buf,unsigned char bOffet,unsigned char SpecialErase);
	virtual int ST_DirectOneBlockRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE SkipSector,BYTE CheckECCBit,BYTE patternBase,int len ,unsigned char *w_buf,int lowPageEcc=0, bool bNewEccFormat=false, UINT32 ceMask=0);
	//int ST_DirectOneBlockRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE,WORD wBLOCK,WORD SkipSector,BYTE CheckECCBit,int SectorCount ,unsigned char *w_buf);
	virtual int ST_HynixTLC16ModeSwitch(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bMode);
	virtual int ST_DirectOneBlockErase(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bSLCMode,BYTE bChipEnable,WORD wBLOCK,UINT32 bCEOffset,int SectorCount ,unsigned char *w_buf,int bDf=0);
	virtual int ST_SetPatternTable(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int SectorCount ,unsigned char *w_buf);

	int ST_SectorBitMapGetSet(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet,BYTE Die,BYTE *w_buf);
	//int ST_GetUnitID(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ce,BYTE *w_buf);
	
	virtual int ST_PageBitMapGetSet(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet,BYTE Die,BYTE *w_buf);
	int ST_PBADieGetSet(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet,BYTE Die,int flashType,BYTE PBAVer,BYTE *w_buf);

	int ST_SetBadColumn(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int *currentDie, int die, int useMBC
		, int MBCSet,bool isED3,bool bSetOBCBadColumn,DWORD DieOffset,unsigned char * apParameter
		,unsigned char *badcolumndata,unsigned char *mbcBuffer);
	int ST_Set_Flash_All_Bad_Column_Collect(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE DummyReadEnable,DWORD BadColumnBlock, BYTE SinglePlane=false, BYTE BadColumnCE=0, BYTE BadColumnPlane=0,BYTE _32nmPattern=false);
	virtual int ST_GetCodeVersionDate(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char* buf);
	int ST_FlashReset(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int CE_Sel,BYTE TestBusy=0);
	int	ST_T2LAndL2T(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,BYTE Type, BYTE DieCount, BYTE bCE);
	int	ST_ModifyFlashPara(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE CeStart, BYTE CeEnd);
	int ST_CheckModifyFlashPara(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf);
	int ST_SetFlashParameter(int DelayTime);
	int ST_Set_DummyRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BadColumnCE, DWORD BadColumnBlock , BYTE BadColumnPlane);
	int ST_BadColumn_Read(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BC_CE, DWORD BC_Block , DWORD BC_Page, int len, unsigned char *buf);
	int ST_Set_Get_BadColumn(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE SinglePlane, BYTE  bGetSet, unsigned char *buf,int bSet128B=0);
	virtual int ST_SetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMode, unsigned char *w_buf, BYTE bONFIType, BYTE bONFIMode, BOOL bToggle20);
	int	ST_GetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf);
	virtual int ST_SetGetPara(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE CE, BYTE Die, BYTE bSetGet, unsigned short addr, unsigned short wdata, unsigned char *w_buf);
	int ST_SetSD_D1(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE CE);
	int ST_DisableIPR(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE CE, BYTE Die);
	int ST_2306_SetGet_Conversion(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bSetGet, unsigned char *settingBuf, unsigned char *buf);
	int ST_OpenExtentByteWrite(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE addrHigh, BYTE addrLow, BYTE _1zMLC, BYTE wdata);
	int ST_OpenExtentByteRead(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE addrHigh, BYTE addrLow, BYTE _1zMLC, int datalen, unsigned char *w_buf);
	int ST_CmdWriteFlashFuse(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE bED3, unsigned char* buf, int len );
	virtual int ST_CmdFDReset(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	int ST_CmdGetFlashFuse(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE bED3, int block, int page, unsigned char* buf, int len );
	int CmdWriteFlashFuseAD(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE);
	int ST_CmdWriteFlashFuse_GWK(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, unsigned int uiBlk, unsigned int uiPage, BYTE bED3, unsigned char* buf, int len );
	int ST_CmdGetFlashFuse_GWK(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE bED3, int block, int page, unsigned char* buf, int len );
	int ST_CmdGetFlashFuse2_GWK(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, int block, int page, unsigned char* buf, int len );
	int ST_FlashGetID_GWK(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* buf, int len );
	virtual int ST_GetSRAM_GWK(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf);
	virtual int ST_FWInit(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	int ST_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle, char targetDisk , unsigned char *id, bool bGetUID = false,BYTE full_ce_info=0,BYTE get_ori_id=0);
	int ST_CustomCmd_Path(unsigned char* bDrvPath, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut);
	HANDLE ST_CreateHandleByPath(char *MyDrivePath);
	int ST_GetE2NANDFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk , unsigned char *id, bool bGetUID,BYTE full_ce_info,BYTE get_ori_id);
	int ST_FAddrSet(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,int CE,int Die,int addr,int data );
	int ST_FAddrGet(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,int CE,int Die,int addr,unsigned char * data );
	virtual int ST_Send_Dist_read(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, int ce,WORD wBlock,WORD WL,WORD VTH, PUCHAR pucBuffer,UCHAR pagesize,int is3D,bool isSLC=true,unsigned char ShiftReadLevel=0);
	int NT_Send_Dist_read(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,int ce, WORD wBlock,WORD WL,WORD VTH,BYTE Die,PUCHAR pucBuffer,UCHAR pagesize);
	int ST_Send_Dist_readST(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,int ce, WORD wBlock,WORD WL,WORD VTH,PUCHAR pucBuffer,UCHAR pagesize);
	int ST_FAddrReset(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum );
	virtual int ST_GetUnitID(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce, BYTE m_val);
	int	NT_Toggle2LagcyReadId(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char*buf);
	int ST_GetMicronUnitID(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce);
	virtual int ST_GetTSBIdentificationTable(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce);
	virtual int ST_GetTSBRevisionCode(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce);
	int ST_GetMicroParePage(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce);
	virtual int ST_ScanBadBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int CE, unsigned char *w_buf,BOOL isCheckSpare00, unsigned int DieOffSet, unsigned int BlockPerDie);
	int ST_SetSeed(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, int Seed);
	int ST_SetToggle1020(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE toggleMode, unsigned char vcc, unsigned char* buf);
	int ST_ScanPasswindow(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, int  PhyBlock, unsigned char* buf);
	virtual int ST_CheckU3Port(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* buf);
	int ST_GetHWMDLL90Degree(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char *w_buf);
	int ST_SetWindowMidValue(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned int SDLL);
	int GetDeviceType();
	void SetDeviceType(int deviceType);
    virtual	int ST_SetDegree(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int Phase, short delayOffset);
	int ST_DetectVoltage(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE Voltage);
	int SCSIPtdBuilder(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER *sptdwb,
		unsigned int cdbLength,  unsigned char *CDB,
		unsigned char direction, unsigned int transferLength,
		unsigned long timeOut,   unsigned char *buf);
	int ST_2309CheckUECC(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, char Vcc, char ToggleMode);
	int ST_Get_Flash_status(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, unsigned char *buf);
	int ST_GetBootcodeVersion(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE *w_buf);
	/************************************************************************/
	/* Power Cycling 製具                                                                     */
	/************************************************************************/

	//Power Cycling 製具
	DWORD TPowerResetAll(BYTE obType,HANDLE DeviceHandle,char targetDisk);
	DWORD TPowerOnOff(BYTE obType,HANDLE DeviceHandle,char targetDisk,unsigned char Mode,unsigned char channel,int DelayTime);
	//==========Bank switch===================
	int BankInfoCreateFromFile();
	int BankSearchByCmd(unsigned char *CDB);
	virtual int ST_BankSwitch(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf);
	//int BankInit(_FH_Paramater *_FH,unsigned char *bankBuf,char *bankBurnerName);
	int BankInit(BYTE bType,HANDLE DeviceHandle_p,BYTE bDrvNum ,_FH_Paramater *_FH,unsigned char *bankBuf);
	int LocalBankValueInit();
	int BankFileLoad(char *fileName);

	virtual	int ST_BankBufCheck(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf);
	int ST_BankSynchronize(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	//=========================================================
	int ST_XNOR_or_Level_Indicated(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,
									BYTE mode,BYTE ce,WORD wBlock,WORD wWordLine,unsigned char* wBuf);
	virtual int ST_SetGet_RetryTableByFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BOOL isGet, int ce, int die, unsigned char * buf, int length);
	virtual int ST_APGetSetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE SetGet,BYTE Address,BYTE Param0,BYTE Param1,BYTE Param2,BYTE Param3,unsigned char *buf);
	virtual int ST_Set_pTLCMode(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk);
	virtual int ST_SetLDPCIterationCount(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk ,int count);
	void SetAlwasySync(BOOL);
	void SetDumpCDB(BOOL);
	unsigned char * GetBankBuf();
	void SetDoFlashReset(BOOL  doReset);
	void SetDoFDReset(BOOL  doReset);
	void SetForceID(unsigned char *forceid);
	int ST_SetGlobalSCSITimeout(int scsi_timeout);
	void SetIniFileName (char * ini);
	virtual void NetCommandSetAllVirtual(int);
	_FH_Paramater *_fh;
	void SetSortingBase(BaseSorting* sb);
	void ST_SetCloseTimeOutForPreformat(bool blCloseTimeOut);
	int  ST_SetPageSeed(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int pageseed);
	void SetActiveCe(int ce);
	int GetActiveCe();

	int SD_TesterGetFlashID ( BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* r_buf );
	int SD_TesterJumpBurner ( BYTE obType, HANDLE DeviceHandle, BYTE targetDisk);
	int SD_TesterISPBurner ( BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,unsigned char* buf,int secCnt,int totalSecCnt, int type );
	int SD_TesterForceToBoot(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* r_buf);
	int SD_TesterGetInfo (BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* r_buf);

	int ST_Soft_bit_dump_read(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE RWMode,BYTE SBMode,BYTE bCE
							,WORD wBLOCK,WORD Page,BYTE Shiftvalue1,BYTE Shiftvalue2,BYTE Shiftvalue3 ,BYTE Deltavalue1
							,BYTE Deltavalue2,WORD DataLen,unsigned char *w_buf);
	int SetThreadStatus(int status);
	int GetThreadStatus(void);
	int SetDumpWriteReadAPParameter(HANDLE h, unsigned char * ap_parameter);

	/************************************************************************/
	/* NES CONTROL                                                          */
	/************************************************************************/
	int	nps_set_server_info(char *ip, int port);
	int	nps_client_update_param(char *command_buf, int commandLen, char * recBuf);	
	int IsFakeBlock(int blk);

	// For PPS(NES)
	int MyLockVolume_PPS(BYTE bType, BYTE bDrv, HANDLE *hDeviceHandle);
	//int NT_SendCommand(BYTE bType, HANDLE DeviceHandel, char targetDisk, unsigned char *bCmd, int bCmdlen, int direc = 2, unsigned int iDataLen = 0, unsigned char *buf = NULL, int timeout);  // #wait#check Danson#
	int NT_SendCommand(BYTE bType, HANDLE DeviceHandel, char targetDisk, unsigned char *bCmd, int bCmdlen, int direc, unsigned int iDataLen, unsigned char *buf, int timeout);
	int NT_GetFwAutoCheckError(BYTE bType, HANDLE DeviceHandel, char targetDisk, unsigned char *buf, int len);
	int NT_GetFwErrorHandleTag(BYTE bType, HANDLE DeviceHandel, char targetDisk, BYTE Param, unsigned char *buf, int len);
	int NT_ScanMarkBad(BYTE bType, HANDLE DeviceHandel, char targetDisk, BYTE bCE, WORD wBlock, WORD wPage, BYTE bDie, BYTE *bBuf, unsigned int iSize);
	int NT_ScanMarkBad_FW(BYTE bType, HANDLE DeviceHandel, char targetDisk, BYTE bChannel, BYTE bCE, WORD wTotalBlock, BYTE *bBuf, unsigned int iSize);
	int NT_GetFlashSizeInfo(BYTE bType, HANDLE DeviceHandel, char targetDisk, FW_FlashSizeInfo *fInfo);
	//========================================================================
	
	void SetATAIOControl(IOCONTROLTYPE ata);
	IOCONTROLTYPE GetATAIOControlType();
protected:
	int  _fakeRanges[4];
	unsigned char _pageMapping[PAGE_MAPPING_SIZE];
	int _doFrontPagePercentage;
	BYTE m_apParameter[1024];
	IOCONTROLTYPE m_ATAIO;
	int _SCSI_DelayTime;
	char _ini_file_name[512];

private:
	unsigned char *_bankBufTotal;
	BOOL _doFlashReset;
	BOOL _doFDReset;
	BOOL _alwaySyncBank;
	BOOL _SetDumpCDB;

	unsigned int _max_nvme_recording_sizeK;
	bool _CloseTimeout;
	int _ThreadStatus;
	unsigned char _forceID[8];
	//	int NT_CustomCmd_Phy(BYTE	bPhyNum, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut);
	//	int NT_CustomCmd( char targetDisk, unsigned char *CDB, unsigned char bCDBLen, int direc,unsigned int iDataLen,unsigned char *buf,unsigned long timeOut);


public:

	int _creatFromNet;
	bool _retryIoControl;
	bool _requestedSuspend;
	bool _isIOing;
	BaseSorting* _pSortingBase;
	//BaseFlashSorting* _pBaseFlashSorting;
	bool _isCmdSwitching;
	int GetCtxId(void);
	int SetSuspend(bool suspend);
	bool IsSwitching(void);
	int _idxOfHub;
	int _context_id;
	UCHAR _LastCdb[5][16];
	UCHAR _LastBuf[5][32];
	int   _LastIndex;
	//for simulation
	virtual bool IsSimMode() { return false; }
	virtual bool IsLogMode() { return false; }
	virtual void DumpErrStack( const char* ) { return; }

/*B*********************NVME related************************/
protected:
	char _fwVersion[8];
private:
	int MyIssueNVMeCmd(HANDLE hUsb, UCHAR *nvmeCmd,UCHAR OPCODE,int direct,UCHAR *buf,ULONG bufLength,ULONG timeOut, int deviceType, bool silence);
public:
	HANDLE _nvmeHandle;
	HANDLE NVMeMyCreateFile( int drive );
	HANDLE FindFirstNvmeHandle(int * scsi_port/*input / output*/, char * product_name/*input / output*/);
	int IssueNVMeCmd(UCHAR *nvmeCmd,UCHAR OPCODE,int direct,UCHAR *buf,ULONG bufLength,ULONG timeOut);
	virtual bool InitDrive(unsigned char drivenum, const char* ispfile, int swithFW  );
	virtual int GetFwVersion(char * version/*I/O*/);
	virtual int GetDataRateByClockIdx(int clk_idx);
	//CNTCommand * getNVMEinstance(HANDLE h);
	virtual int Dev_SetFWSpare_WR(unsigned char Spare, bool isRead);
	virtual int Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len );
	virtual int Dev_SQL_GroupWR_Setting(int blockSeed, int pageSeed, bool isWrite, int LdpcMode, char Spare);
	virtual int SetAPKey_NVMe(void);
	int	ST_Init_SQL_Buffer(unsigned char *buf);// only usb fw need set buffer point
	virtual int _WriteDDR(UCHAR* w_buf,int Length, int ce=-1);
	virtual int _TrigerDDR(int CE, const unsigned char* sqlbuf, int slen, int buflen, unsigned char channels=0 );
	int SQL_MeasureRDY_TimeReset();
	int SQL_MeasureRDY_TimeGet(unsigned int &RDY_TimeMicrosecond);
	virtual int _ReadDDR(UCHAR* r_buf,int Length, int datatype );
	virtual int NewSort_SetPageSize ( int pagesize );
/*E*********************NVME related************************/
	void InitSQL(void);
	int LogTwo(int n);

///////////////////////////////////////////////////////////////////////////////
//E:Reference from NTCmd
///////////////////////////////////////////////////////////////////////////////

	int GetCeMapping(int input_ce);
	virtual int GetLogicalToPhysicalCE_Mapping(unsigned char * flash_id_buf, int * phsical_ce_array);
	void ForceSQL(bool enable);
	bool isForceSQL();
	HANDLE Get_SCSI_Handle(void);
	void Set_SCSI_Handle(HANDLE h);
	void SetDeviceType_NoOpen(int deviceType);

protected:
	UCHAR _flash_id_boot_cache[512];
	unsigned char _Retry_RT[2048];
	unsigned char _Retry_Order[128];
	unsigned char _PageMap[1024];
	unsigned char _Good_WL[1024];
	bool	_Init_GMP;
	
private:
	int ST_DirectOneBlockErase_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bSLCMode,BYTE bChipEnable,WORD wBLOCK,BYTE bCEOffset,int SectorCount ,unsigned char *w_buf,int bDf=0);
	int ST_DirectPageRead_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD bPageCount,BYTE bECCBit,BYTE Retry_Read,BYTE bOffset,int datalen ,unsigned char *w_buf, bool bNewEccFormat=false);
	int ST_DirectPageRead_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
		,WORD wStartBLOCK,WORD wPage,WORD wBlockCount,BYTE bECCBit,BYTE Retry_Read,int a2command,BYTE bOffset,int datalen ,unsigned char *w_buf, bool bNewEccFormat=false);
	int ST_DirectPageWrite_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckEcc,BYTE bRetry_Read,int a2commnad,int datalen,int bOffset ,unsigned char *w_buf,
		BYTE TYPE,DWORD Bank, BYTE bIBmode=0, BYTE bIBSkip=0, BYTE bRRFailSkip=0);
	int ST_DirectOneBlockRead_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE SkipSector,BYTE CheckECCBit,BYTE patternBase,int len ,unsigned char *w_buf,int lowPageEcc=0, bool bNewEccFormat=false);
	int ST_DirectOneBlockWrite_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode
		,BYTE bCE,WORD wBLOCK,BYTE SkipSector,int SectorCount ,unsigned char *w_buf,BYTE bDontDirectWriteCEMask);
	int ST_HynixTLC16ModeSwitch_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bMode);
	int ST_GetSetRetryTable_SQL(BYTE bGetSet,BYTE bSetOnfi,BYTE *r_buf, int length,BYTE ce,BYTE bRetryCount, BYTE OTPAddress);
	int ST_PageBitMapGetSet_SQL(BYTE bGetSet,BYTE Die,BYTE *w_buf);
	int ST_SetGet_Parameter_SQL(void);
	int ST_DirectGetFlashStatus_SQL(unsigned char ce,unsigned char *buf,unsigned char cmd=0x70);
	int ST_GetReadWriteStatus_SQL(unsigned char *w_buf);
	int ST_Set_pTLCMode_SQL(void);
	int ST_DirectSend_SQL(const unsigned char* sql,int sqllen, int mode, unsigned char ce, unsigned char* buf, int len );
	int ST_Send_Dist_read_SQL(int ce, WORD wBlock,WORD WL,WORD VTH,PUCHAR pucBuffer,UCHAR pagesize,int is3D,bool isSLC=true,unsigned char ShiftReadLevel=0);
	unsigned char Check_PageMapping_Bit( unsigned char ce, unsigned int page_num, unsigned int page_count );
	
	virtual int GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &WHR2);

};

class CNTCommandNet : public USBSCSICmd_ST
{
public:
	DeviveInfo _default_drive_info;
	// init and create handle + create log file pointer from single app;
	CNTCommandNet( DeviveInfo*, const char* logFolder );
	// init and create handle + system log pointer
	CNTCommandNet( DeviveInfo*, FILE *logPtr );

	CNTCommandNet();
	CNTCommandNet(int port);
	~CNTCommandNet();
};
typedef USBSCSICmd_ST CNTCommand;
#endif /*_USBSCSICMD_ST_H_*/