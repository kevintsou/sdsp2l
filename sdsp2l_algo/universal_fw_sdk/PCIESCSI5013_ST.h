#ifndef _PCIESCSI5013_ST_H_
#define _PCIESCSI5013_ST_H_

#include "USBSCSICmd_ST.h"
#include "convertion_libs/ldpcecc.h"

/**current for U17***/
#define   MAX_CHANNEL           (2)
#define   MAX_CE_PER_CHANNEL    (8)
#define   MAX_DIE               (4)

typedef struct SDLL_offset {
	unsigned short SDLL_read_offset_die[MAX_CE_PER_CHANNEL][MAX_DIE];
	unsigned short SDLL_write_offset_die[MAX_CE_PER_CHANNEL][MAX_DIE];
}SDLL_offset_t;

typedef struct DQS_delay_TXRX
{
	union {
		unsigned char u8All;
		struct {
			unsigned char tx_delay  :4;
			unsigned char rx_delay  :4;
		};
	};
} DQS_delay_TXRX_t;

typedef struct DQ_delay_Die
{
	union{
		unsigned int u32_DQ_0to3_Delay;
		struct {
			unsigned int DQ0RX    :4;
			unsigned int DQ0TX    :4;
			unsigned int DQ1RX    :4;
			unsigned int DQ1TX    :4;
			unsigned int DQ2RX    :4;
			unsigned int DQ2TX    :4;
			unsigned int DQ3RX    :4;
			unsigned int DQ3TX    :4;
		} DQ_0to3;
	};

	union{
		unsigned int u32_DQ_4to7_Delay;
		struct {
			unsigned int DQ4RX    :4;
			unsigned int DQ4TX    :4;
			unsigned int DQ5RX    :4;
			unsigned int DQ5TX    :4;
			unsigned int DQ6RX    :4;
			unsigned int DQ6TX    :4;
			unsigned int DQ7RX    :4;
			unsigned int DQ7TX    :4;
		} DQ_4to7;
	};
}DQ_delay_Die_t;

/**current for U17***/

//=====================  ID Page  ======================================
/* ID Page */
#define IP_CP_IDX					(0x24 >> 1)
#define IP_FLASH_ID                 (0x100)
#define IP_VENDOR_TYPE              (0x108)
#define IP_FLASH_TYPE               (0x109)
//#define IP_CELL_TYPE                (0x10A)
#define IP_SPECIFY_FUNTION          (0x10A)
#define SPECIFY_BIT_DISABLE_TSB_RELEX_READ   (1)
#define SPECIFY_BIT_ENABLE_6_ADD             (1<<1)
#define SPECIFY_GLOBAL_RANDOM_SEED           (1<<2)
#define SPECIFY_ADJUST_AC_TIMING             (1<<3)
#define SPECIFY_ACRR_ENABLE                  (1<<4) //address cycle retry read(ACRR)

//#define IP_IS_6_ADDRESS             (0x10B)
#define IP_MICRON_TIMING            (0x10B)
#define IP_DEFAULT_INTERFACE        (0x10C)
#define IP_TARGET_INTERFACE         (0x10D)
#define IP_FLASH_CLOCK              (0x10E)
#define IP_FLASH_DRIVE              (0x10F) //0:UNDER 1:NORMAL 2:OVER
#define IP_DIE_PER_CE               (0x110)
#define IP_PLANE_PER_DIE            (0x111)
#define IP_BLOCK_PER_PLANE          (0x112 >> 1)
#define IP_BLOCK_NUM_FOR_DIE_BIT    (0x114 >> 1)
#define IP_D1_PAGE_PER_BLOCK        (0x116 >> 1)
#define IP_PAGE_PER_BLOCK           (0x118 >> 1)
#define IP_DATA_BYTE_PER_PAGE       (0x11A >> 1)
#define IP_FLASH_BYTE_PER_PAGE      (0x11C >> 1)
#define IP_FRAME_PER_PAGE           (0x11E)
#define IP_PAGE_NUM_PER_WL          (0x11F)
#define IP_PAGE_BIT_FOR_FSA         (0x120)
#define IP_D1_PAGE_BIT_FOR_FSA      (0x121)
#define IP_D1_PAGE_BIT_GAP          (0x122)
#define IP_FLASH_ODT                (0x123) //0:off 1:150 2:100 3:75 4:50 (ohm)
#define IP_CONTROLLER_DRIVING       (0x124) //0:50 1:37.5 2:35 3:25 4:21.43 (ohm)
#define IP_CONTROLLER_ODT           (0x125) //0:150 1:100 2:75 3:50 4:0ff (ohm)
#define IP_ALU_RULE                 (0x130 >> 2)
#define IP_D1_ALU_RULE              (0x134 >> 2)
#define IP_TIMING_ADL               (0x140)
#define IP_TIMING_WHR               (0x141)
#define IP_TIMING_WHR2              (0x142)
//=====================  ID Page  ======================================

//=====================  VendorType  ======================================
enum {UNIV_TOSHIBASANDISK = 0, UNIV_INTELMICRON = 1, UNIV_SKHYNIX = 2, UNIV_YMTC = 3, UNIV_ALL_SUPPORT_VENDOR_NUM = 4};
//=====================  VendorType  ======================================

//=====================  FlashType  ======================================
#define TOSHIBA_BICS2					(0)
#define TOSHIBA_BICS3					(1)
#define TOSHIBA_BICS4					(2)
#define TOSHIBA_QLC_BICS4				(3)
#define MICRON_B16A						(4)
#define MICRON_B17A						(5)
#define SANDISK_BICS3					(6)
#define YMTC_GEN2						(7)
#define MICRON_N18A						(8)
#define MICRON_B27A						(9)
#define MICRON_N28A						(10)
#define TOSHIBA_1Z_MLC					(11)
#define SANDISK_1Z_MLC					(12)
#define HYNIX_3DV4						(13)
#define MICRON_B27B						(14)
#define MICRON_B05A						(15)
#define HYNIX_3DV5						(16)
#define HYNIX_3DV6						(17)
#define SANDISK_BICS4                   (18)
#define MICRON_B37R                     (19)
#define MICRON_B27C                     (20)
#define MICRON_B36R                     (21)
#define MICRON_B47R                     (22)
#define MICRON_Q1428A                   (23)

//=====================  FlashType  ======================================

#define  TASK_ID_WAIT_TEMPERATURE       (0x25)
#define  TASK_ID_LOOP_START             (0x3F)
#define  TASK_ID_LED_MODE               (0x54)
#define  TASK_MP_CACHE_WRITE            (0x1C)
#define  TASK_MP_WRITE                  (0x1B)

//=====================  CellType  ======================================
enum {UNIV_FLH_SLC = 0, UNIV_FLH_MLC = 1, UNIV_FLH_TLC = 2, UNIV_FLH_QLC = 3, UNIV_ALL_CELLTYPE_NUM = 4};
//=====================  CellType  ======================================

//=====================  TSB RR Process  ======================================
typedef enum RetryToshibaProcessEnum
{
	RETRY_TOSHIBA_FLASH_PROCESS_1X,
	RETRY_TOSHIBA_FLASH_PROCESS_1Y,
	RETRY_TOSHIBA_FLASH_PROCESS_1Z,
	RETRY_TOSHIBA_FLASH_PROCESS_BICS2,
	RETRY_TOSHIBA_FLASH_PROCESS_BICS3_128G,
	RETRY_TOSHIBA_FLASH_PROCESS_BICS3_256G,
	RETRY_TOSHIBA_FLASH_PROCESS_BICS3_512G,
	RETRY_TOSHIBA_FLASH_PROCESS_BICS3_1024G,
	RETRY_TOSHIBA_FLASH_PROCESS_BICS4_256G,
	RETRY_TOSHIBA_FLASH_PROCESS_BICS4_512G,
	RETRY_TOSHIBA_FLASH_PROCESS_BICS4_1024G
} RetryToshibaProcessEnum_t;
//=====================  TSB RR Process  ======================================

//=====================  Interface  ======================================
typedef enum
{
	FLH_INTERFACE_LEGACY = 0,
	FLH_INTERFACE_T1,
	FLH_INTERFACE_T2,		//equal to Micron NVDDR2
	FLH_INTERFACE_T3,		//equal to Micron NVDDR3
} FlashInterface_t;
//=====================  Interface  ======================================

//=====================  Clock  ======================================
typedef enum UNIV_FLASH_CLK
{
	UNIV_FLH_CLOCK_10MHZ  = 0, //legacy10 or toggle-20
	UNIV_FLH_CLOCK_33MHZ  = 1, //legacy33 or toggle-66
	UNIV_FLH_CLOCK_40MHZ  = 2, //legacy40 or toggle-80
	UNIV_FLH_CLOCK_100MHZ = 3, //toggle-200
	UNIV_FLH_CLOCK_166MHZ = 4, //toggle-332
	UNIV_FLH_CLOCK_200MHZ = 5, //toggle-400
	UNIV_FLH_CLOCK_266MHZ = 6, //toggle-533
	UNIV_FLH_CLOCK_333MHZ = 7, //toggle-667
	UNIV_FLH_CLOCK_400MHZ = 8, //toggle-800
	UNIV_FLH_CLOCK_MAX
} UNIV_FLH_CLK_t;
//=====================  Clock  ======================================


typedef enum _ERR_
{
	SCHEDULER_DISPATCHING = 0,
	SCHEDULER_DISPATCH_DONE,
	SCHEDULER_SUSPEND_SCANWINDOW_FAIL,
	SCHEDULER_SUSPEND_INIT_BT_FAIL,
	SCHEDULER_SUSPEND_PTO_FAIL,
	SCHEDULER_SUSPEND_FPU_FAIL,
	SCHEDULER_SUSPEND_TIMEOUT_FAIL,
	SCHEDULER_SUSPEND_INIT_SCHEDULER_FAIL,
	SCHEDULER_SUSPEND_ZQ_FAIL,
	SCHEDULER_STATE_MAX,
} ERR_;//ubFailReason


/*QC page definition*/
#define QC_RESERVED_SIZE				1
#define QC_CHIP_ID_SIZE					2
#define QC_BOOT_CODE_SIZE				8
#define QC_SORTING_VERSION_SIZE			10
#define QC_SORTING_AP_VERSION_SZIE		12
#define QC_RESERVED2_SIZE               12
#define QC_RESERVED3_SIZE               2
#define SORTING_MAX_DIE                 4
#define UNIV_MAX_DIE                    4
#define UNIV_FOUR_PLANE                 4
typedef unsigned char  u8_t;
typedef unsigned short u16_t;
typedef unsigned int   u32_t;
typedef unsigned long long u64_t;

#define QC_PAGE_FUNC_FAIL_DF2D            (1)
#define QC_PAGE_FUNC_FAIL_CACHE_READ      (1<<2)


#define PACKED
#pragma pack(push,1)

typedef struct UNIV_CMD_PACKED
{
	u8_t ubInterface;
	u16_t uwBadBlockCount[UNIV_MAX_DIE];
	u8_t ubFuncFail;
	u8_t ubCurRecordNumber;
	u8_t ubTotalRecordNumber;
	u8_t ubIC_Channel_Number;
	u16_t uwMarkBadFailCount;
	u8_t ubReserved[QC_RESERVED_SIZE];
	u32_t ulChipID[QC_CHIP_ID_SIZE];
	u8_t ubBootCode_Ver[QC_BOOT_CODE_SIZE];
	u8_t ubSortingFW_ver[QC_SORTING_VERSION_SIZE];
	u8_t ubSortingAP_ver[QC_SORTING_AP_VERSION_SZIE];
	u16_t uwMDLL_Center;
	u16_t uwMDLL_W_Offset;
	u16_t uwMDLL_R_Offset;
	u16_t uwFlaTemperature;
	u64_t ullTime;
	u8_t ubSchedulerState;
	u8_t ubPatternFailReason;
	u8_t ubClock;
	u8_t ubEnableEntry;
	u8_t ubECCThreshold_SLC;
	u8_t ubECCThreshold_Normal;
	u8_t ubRR_ECCThreshold_SLC;
	u8_t ubRR_ECCThreshold_Normal;
	u16_t uwSLCEraseMaxBusyTime;
	u16_t uwSLCWriteMaxBusyTime;
	u16_t uwSLCReadMaxBusyTime;
	u16_t uwNormalEraseMaxBusyTime;
	u16_t uwNormalWriteMaxBusyTime;
	u16_t uwNormalReadMaxBusyTime;
	u16_t uwCtrlTemperature;
	u16_t uwRR_PassCnt[UNIV_FOUR_PLANE];
	u16_t uwFailBlkCntPerDie[UNIV_MAX_DIE][UNIV_FOUR_PLANE];
	u16_t uwW_Count0_SDLLMin;
	u16_t uwW_Count0_SDLLMax;
	u16_t uwR_Count0_SDLLMin;
	u16_t uwR_Count0_SDLLMax;
	u8_t ubReserved3[QC_RESERVED3_SIZE];
	u16_t uwCheckSum;
} QC_Page_t;




#define MAX_PE_BLOCK_CNT_INFO_PAGE  (255)
#define PE_BLOCK_LIST_START_ADDR    (0x1C00)


#pragma pack(pop)
#undef PACKED

#define E13_L3_QC_PAGE_SEGEMENT_LENGT  (0x300)



#define INFORMATION_PAGE_ADDR            100
#define E13_REPORT_PAGE_ADDR             101
#define INFOBLOCK_TAG					 "INFOBLOCK"




#define INFOPAGE_FLASHTYPE               0x200 //0: MLC 1: TLC 2: QLC
#define INFOPAGE_DIE_PER_CE              0x201
#define INFOPAGE_BLK_PER_CE_H            0x202
#define INFOPAGE_BLK_PER_CE_L            0x203
#define INFOPAGE_PAGE_CNT_H              0x204//D3 cnt
#define INFOPAGE_PAGE_CNT_L              0x205//D3 cnt
#define INFOPAGE_PAGE_SIZE_H             0x206
#define INFOPAGE_PAGE_SIZE_L             0x207
#define INFOPAGE_PLANE_CNT               0x208
#define INFOPAGE_FAST_PAGE_CNT_H         0x209//D1 cnt
#define INFOPAGE_FAST_PAGE_CNT_L         0x20A//D1 cnt
#define INFOPAGE_FLASH_CLOCK             0x20B
#define INFOPAGE_FLASH_INTERFACE         0x20C

#define INFOPAGE_FLASH_INTERFACE         0x20C
/*
0: legacy
1: toggle1
2: toggle2
3: toggle3
*/
#define INFOPAGE_FLASH_RETRY_READ        0x20B
#define INFOPAGE_CODEBODY_LDPC_MODE      0x20E

#define INFOBLOCK_ECC_THRESHOLD_D1       0x210
#define INFOBLOCK_ECC_THRESHOLD          0x211
#define INFOBLOCK_ENABLE_RETRY           0x216


#define INFOBLOCK_RETRY_FROM             0x217
#define INFOBLOCK_RETRY_GROUPS_CNT_D1    0x218
#define INFOBLOCK_RETRY_LEN_PER_GROUP_D1 0x219
#define INFOBLOCK_RETRY_GROUPS_CNT       0x21A
#define INFOBLOCK_RETRY_LEN_PER_GROUP    0x21B
#define INFOBLOCK_RETRY_KEEP_LEVEL       0x21C

#define INFOBLOCK_ECC_THRESHOLD_RR_D1    0x21D
#define INFOBLOCK_ECC_THRESHOLD_RR       0x21E
#define INFOBLOCK_SCAN_WIN_THRESHOLD     0x21F

#define INFOPAGE_RECORD_NUM              0x220
#define INFOPAGE_PAGE_CNT_PER_RECORD     0x221 //will be Die Num(block map) + 1 (QC page) + 1(Other page) 
#define INFOBLOCK_TAG_ADDR				 0x2D6 //will be "INFOBLOCK"
#define INFOBLOCK_AP_VERSION		     0x2E0 //start from this length can be 16 bytes
#define INFOBLOCK_RETRYTABLE_D1		     0x400 
#define INFOBLOCK_RETRYTABLE		     0x800 

#define INFOBLOCK_L3_BLK_START_ADDR      0xC00

#define RETRYTABLE_MAX_LENGTH_D1         1024
#define RETRYTABLE_MAX_LENGTH            2048


#define QC_PAGE_INTERFACE				 0
typedef enum _PAGE_WIDTH{
	PAGE_BIT_WIDTH_7,
	PAGE_BIT_WIDTH_8,
	PAGE_BIT_WIDTH_9,
	PAGE_BIT_WIDTH_10,
	PAGE_BIT_WIDTH_11,
	PAGE_BIT_WIDTH_12,
	PAGE_BIT_WIDTH_13,
}PAGE_WIDTH;

#define L3_MAX_SUPPORT_DIE       (16)
#define L3_MAX_SUPPORT_PLANE     (4)
#define PICK_BLOCK_PER_PLANE     (6)
typedef struct L3_BLOCKS_PER_CE {
	unsigned short die[L3_MAX_SUPPORT_DIE][L3_MAX_SUPPORT_PLANE*PICK_BLOCK_PER_PLANE]; //(head's flash_plane_cnt + middles's flash_plane_cnt + tail's flash_plane_cnt) * 2
}_L3_BLOCKS_PER_CE;


typedef struct L3_BLOCKS_ALL {
	L3_BLOCKS_PER_CE ce[32];
}_L3_BLOCKS_ALL;

class PCIESCSI5013_ST : public USBSCSICmd_ST
{
public:
    // init and create handle + create log file pointer from single app;
    PCIESCSI5013_ST( DeviveInfo*, const char* logFolder );
    // init and create handle + system log pointer
    PCIESCSI5013_ST( DeviveInfo*, FILE *logPtr );
    ~PCIESCSI5013_ST();
    

private:
    void Init();

protected:
	unsigned char _RDTCodeBodyLDPCMode;



/////////////////////////BASE/////////////////////
public:
	virtual int MyIssueNVMeCmd( unsigned char *nvmeCmd,unsigned char OPCODE,int direct /*NVME_DIRECTION_TYPE*/,unsigned char *buf,ULONG bufLength,ULONG timeOut, bool silence );	

	virtual int Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len );	
	virtual int Dev_InitDrive( const char* ispfile );		
	virtual int Dev_GetFlashID(unsigned char* r_buf);
	virtual int Dev_CheckJumpBackISP ( bool needJump, bool bSilence );
	virtual int Dev_GetSRAMData ( unsigned char offset, unsigned char* buf, int len );
	virtual int Dev_SetSRAMData ( unsigned char offset, const unsigned char* buf, int len );
	//////////////////////////////////////
	virtual int Dev_SetFlashClock ( int timing_idx );
	virtual int Dev_GetRdyTime ( int action, unsigned int* busytime );
	virtual int Dev_SetFlashDriving ( int driving_idx );
	virtual int Dev_SetEccMode ( unsigned char ldpc_idx );
	virtual int Dev_SetFWSpare ( unsigned char Spare );
	virtual int Dev_SetFWSeed ( int page,int block=0x8299 );
	virtual int Dev_SetFlashPageSize ( int pagesize );
	virtual	int Dev_SetP4K( unsigned char * buf, int length, bool isWrite);
	virtual int Dev_SQL_GroupWR_Setting(int blockSeed, int pageSeed, bool isWrite, int LdpcMode=-1/*-1 means auto select*/, char Spare='C');
	virtual int Dev_SetFWSpare_WR(unsigned char Spare, bool isRead);
	virtual void InitLdpc();
	int Dev_SetCtrlODT(int odt_idx);
	////////////////////////////////
	int ATA_SetAPKey();
	int NVMe_SetAPKey();
	int NVMe_ISPProgPRAM(const char* m_MPBINCode);
	////////////////////////////////

	////////////////////////////////
	virtual int _WriteDDR(UCHAR* w_buf,int Length, int ce=-1);
	virtual int _TrigerDDR(int CE, const unsigned char* sqlbuf, int slen, int buflen, unsigned char channels=0 );
	virtual int _ReadDDR(UCHAR* r_buf,int Length, int datatype );
	virtual int GetFwVersion(char * version/*I/O*/);
	virtual bool InitDrive(unsigned char drivenum, const char* ispfile, int swithFW  );
	void ParsingBinFile(unsigned char *buf);
	bool Identify_ATA(char * version);

protected:
	int _flash_driving;
	int _flash_ODT;
	int _controller_driving;
	int _controller_ODT;

	unsigned short _last_offline_time;
	unsigned char _cacheFlashIDBuffer[512];
	unsigned short m_Section_Sec_Cnt[6];
	int m_Section_CRC32[5];
	LdpcEcc * _ldpc;
	unsigned char _dummyBuf[8*1024];
	bool _burnerMode;
	unsigned char *pbinbuf;	// 2M
	char _fwVersion[8];
	void PrepareIDpage_E13(unsigned char *buf);
	void InitialCodePointer_E13(unsigned char *ibuf,unsigned char *obuf,unsigned short *blk);
	void GenSpare_E13(unsigned char *buf,unsigned char mode,unsigned char FrameNo,unsigned char Section);
	void GenE13Spare(unsigned char *buf,unsigned char mode,unsigned char FrameNo,unsigned char Section);
	virtual void Dev_GenSpare_32(unsigned char *buf,unsigned char mode,unsigned char FrameNo,unsigned char Section, int buf_len);
	int OverrideAPParameterFromIni( unsigned char * ap_parameter);
	virtual int GetDefaultDataRateIdxForInterFace(int interFace);
	int _SQLMerge( const unsigned char* sql, int len );
	int NVMe_ForceToBootRom ( int type );
	int NVMe_ISPProgramPRAM(unsigned char* buf,unsigned int BuferSize,bool mScrambled,bool mLast,unsigned int m_Segment);
	void Fill_L3BlocksToInfoPage(unsigned char * input_4k_buf, L3_BLOCKS_ALL * L3_blocks);
	void GetODT_DriveSettingFromIni();

///////////////////////////////////////////////ST//////////////////////////////////////////////////////////
public:
	int NVMe_Identify(unsigned char* buf, int len );
	int GetEccRatio(void);
	virtual int WriteRDTCode(char * rdt_fileName, char * output_infopage_filename,  CNTCommand * inputCmd,
		unsigned char * input_ap_parameter, unsigned char * input_4k_buf, char * ap_version, int userSpecifyBlkCnt, int clk,
		int flh_interface, BOOL isEnableRetry, unsigned char * d1_retry_buffer, unsigned char * retry_buffer,
		int normal_read_ecc_threshold_d1, int retry_read_ecc_threshold_d1,
		int normal_read_ecc_threshold   , int retry_read_ecc_threshold,
		bool isSQL_groupRW
		);
	virtual int GetE13ClkMapping(int clk_mhz);
	virtual int GetDataRateByClockIdx(int clk_idx);
	virtual int GetSDLL_Overhead();
	static int GetScanWinWorstPattern(unsigned char * buf, int len);
	static void * GetBootCodeRule(void);
	static int SelectLdpcModeByPageSize( int pageSize);
	static float CaculateWindowsWidthPicoSecond( unsigned short win_start, unsigned short win_end, unsigned short DLL_Lock_Center, unsigned short data_rate);

	void InitialCodePointer(unsigned char *buf,unsigned short *blk);
	void PrepareIDpage(unsigned char *buf, int target_interface, int target_clock, int userSpecifyBlkCnt);
	int ST_Set_Parameter(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf);
	virtual int  Issue512CdbCmd(unsigned char * cdb512, unsigned char * iobuf, int lenght, int direction, int timeout);
	////////override///////////////
	int ST_ScanBadBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int CE, unsigned char *w_buf,BOOL isCheckSpare00, unsigned int DieOffSet, unsigned int BlockPerDie);
	int ST_SetLDPCIterationCount(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk ,int count);
	virtual int ST_SetGet_Parameter(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bSetGet,unsigned char *buf);
	virtual int NT_ReadPage_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, const char *type, unsigned char *bBuf, unsigned char *DriveInfo=NULL);
	virtual int NT_Inquiry_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf, unsigned char *DriveInfo=NULL);
	int NT_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle, char targetDisk , unsigned char *id, bool bGetUID = false,BYTE full_ce_info=0,BYTE get_ori_id=0);
	void SetAlwasySync(BOOL sync);
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
	static int List_PE_Block(USBSCSICmd_ST *cmd, HANDLE handle, unsigned char * ap_parameter, FH_Paramater *fh, int ce_idx, unsigned short * pe_phy_blocks_list, int max_pe_cnt);
	static int	Mark_PE_Blocks(USBSCSICmd_ST * cmd,  HANDLE handle, unsigned char * ap_parameter ,FH_Paramater * fh ,int ce_idx, int phy_block);
	static bool CheckIs_PE_Block(USBSCSICmd_ST *cmd, HANDLE handle, unsigned char * ap_parameter, FH_Paramater *fh,int ce_idx, int phy_block);
	int NT_SwitchCmd(void);
	int ST_GetCodeVersionDate(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char* buf);
	int ScanWindows(unsigned char * output_buf, bool isGetDll_center);
	int NT_CustomCmd(char targetDisk, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut);
	virtual int NT_CustomCmd_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool is512CDB=false);
	void PrepareIDpageForRDTBurner(int target_interface, int target_clock, int userSpecifyBlkCnt);
	virtual int GetBurnerAndRdtFwFileName(FH_Paramater * fh,  char * burnerFileName,  char * RdtFileName);
	virtual int OverrideOfflineInfoPage(USBSCSICmd_ST *cmd, unsigned char * info_buf);
	int PrepareInFormationPage(USBSCSICmd_ST  * inputCmd, FH_Paramater * fh, unsigned char * task_buf, char * app_version, unsigned char * retry_buf_d1, unsigned char * retry_buf);


	int HandleL3_QC_page( USBSCSICmd_ST * cmd, unsigned char * input_parameter);
	int CalculatePageCntForFWBin(char * fw_bin_file, int pageSizeK);
	int FindTotalValidRecordCnt(USBSCSICmd_ST * inputCmd, unsigned char * input_ap_parameter, int ce_idx, int blk_idx, int data_segment_length, int fw_reserved_cnt, int max_find_cnt, bool isCntL3Fail);
	int LogOut_QC_report(USBSCSICmd_ST * cmd, QC_Page_t * qc_data, int ce_idx);
	int LogOut_L3_QC_report(USBSCSICmd_ST * cmd, unsigned char *L3_qc_buffer, int ce_idx , bool isCntL3Fail);
	virtual int GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &tWHR2);
	static void GetPossiblePEblockList(int * pe_list, int array_len, FH_Paramater * fh);

	SDLL_offset  _SDLL_offset[MAX_CHANNEL];
	DQS_delay_TXRX _DQS_delay[MAX_CE_PER_CHANNEL];
	DQ_delay_Die  _DQ_delay[MAX_CHANNEL][MAX_CE_PER_CHANNEL][MAX_DIE];

	


};
typedef PCIESCSI5013_ST MyPCIEScsiUtil_5013;
#endif /*_PCIESCSI5013_ST_H_*/