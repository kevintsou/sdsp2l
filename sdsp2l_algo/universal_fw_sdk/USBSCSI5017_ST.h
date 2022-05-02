
#pragma once
#include "PCIESCSI5013_ST.h"

enum running_mode
{
	get_running_mode_fail = 0x0,
	fw_not_support = 0x1,
	boot_mode = 0x2,
	burner_mode = 0x3,
	fw_mode = 0x4,
	rdt_mode = 0x5,
	/*ex_rom_boot_mode = 0x6,*/
};
enum FORMFACTOR
{
	FORMFACTOR_EVB = 24,
	FORMFACTOR_U32C = 32,
	FORMFACTOR_QSI = 34
};

#define _STATION_PRE_F1_ 1
#define _STATION_MP_F1_ 2
#define _STATION_MP_F2_ 3
#define _STATION_ST_START_ 4

#define  MAX_BANK_CNT_5017     (8)

class USBSCSI5017_ST :
	public PCIESCSI5013_ST
{
public:
	void Init();
	USBSCSI5017_ST(DeviveInfo* info, const char* logFolder);
	USBSCSI5017_ST(DeviveInfo* info, FILE *logPtr);
	~USBSCSI5017_ST(void);
	unsigned char * Dev_GetApParameter();
	int _user_select_clk_idx;
	int _user_fw_performance;
	int _user_fw_advance;
	int _user_bus_switch;
	int _user_form_factor;
	int _user_nes_switch;
	int _user_tag_switch;
	int _user_free_run;
	int _user_perFrame_dummy;
	int success_times;
	int Dev_ParameterInit(const char *inifile, unsigned char bE2NAND);
	virtual int Dev_GetFlashID(unsigned char* r_buf);
	virtual int GetBurnerAndRdtFwFileName(FH_Paramater * fh, char * burnerFileName, char * RdtFileName);
	virtual int GetSDLL_Overhead();
	virtual int Dev_SetEccMode(unsigned char ldpc_idx);
	virtual int Dev_SetFWSeed(int page, int block);
	virtual void InitLdpc();
	virtual int Dev_InitDrive(const char* ispfile);
	int USB_ISPProgPRAM(const char* m_MPBINCode);
	
	bool InitDrive(unsigned char drivenum, const char* ispfile, int swithFW);
	int NT_GetFlashExtendID(BYTE bType, HANDLE MyDeviceHandle, char targetDisk, unsigned char *buf, bool bGetUID, BYTE full_ce_info, BYTE get_ori_id);
	int USB_Identify(unsigned char *buf, int timeout);
	int USB_SetAPKey();
	virtual void Log(LogLevel level, const char* fmt, ...);
	virtual int NT_GetDriveCapacity(BYTE bType, HANDLE MyDeviceHandle, char targetDisk, unsigned char *buf);
	virtual int NT_GetDriveCapacity_4T(BYTE bType, HANDLE MyDeviceHandle, char targetDisk, unsigned char *buf);
	int NT_trim(BYTE bType, HANDLE MyDeviceHandle, char targetDisk, int len, unsigned char *buf);
	virtual int GetDefaultDataRateIdxForInterFace(int interFace);
	virtual int ST_SetGet_Parameter(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BOOL bSetGet, unsigned char *buf);
	int USB_ISPProgPRAM(unsigned char* buf,unsigned int BuferSize,bool mScrambled,bool mLast,unsigned int m_Segment);
	int USB_ISPJumpCode(int subFeature);
	int USB_Set_Parameter(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char *buf);
	int USB_ISPEraseAll();
	virtual int Dev_SetP4K(unsigned char * buf, int Length, bool isWrite);
	int ST_u17ScanSpare(CNTCommand * inputCmd, int ce, int codeBlock, unsigned char *buf);
	int WriteRDTCode(char * rdt_fileName, char * output_infopage_filename, int ce, int codeBlock, CNTCommand * inputCmd, int writeMethod,
		unsigned char * input_ap_parameter, unsigned char * input_4k_buf, char * ap_version, int userSpecifyBlkCnt, int clk,
		int flh_interface, BOOL isEnableRetry, unsigned char * d1_retry_buffer, unsigned char * retry_buffer,
		int normal_read_ecc_threshold_d1, int retry_read_ecc_threshold_d1,
		int normal_read_ecc_threshold, int retry_read_ecc_threshold, L3_BLOCKS_ALL * L3_blocks, int times, bool isSQL_groupRW, unsigned char *scanWindowOutbuf, 
		unsigned char * HTRefbuf, int HTRefSize, bool isWrittingCP, unsigned char * odtSettingbuf);
	int WriteCPCode(unsigned char* buf, int ce, int codeBlock, CNTCommand * inputCmd, int writeMethod, bool isSQL_groupRW);
	int ReadCPCode(unsigned char* buf, int ce, int codeBlock, CNTCommand * inputCmd, int writeMethod, bool isSQL_groupRW);
	void BuildCPBuffer(unsigned char* buf, int ce, int success_cut, int codeBlock);
	unsigned char* GetCPBuffer();
	void ParsingBinFile(unsigned char *buf);
	int USB_ISPPreformat(unsigned char* buf, ULONG filesize, int subFeature=0);
	int NT_TestPreformat(unsigned char* buf, unsigned int BuferSize);
	int USB_ReadInfoBlock(unsigned char* buf, unsigned int BuferSize);
	int USB_WriteInfoBlock(unsigned char* buf, unsigned int BuferSize);
	int USB_Send_apDBT(unsigned char* buf, unsigned int BuferSize, int mLast);
	int USB_Read_apDBT(unsigned char* buf, unsigned int BuferSize, int channel, int ce, int die);
	int USB_ReadFCE(unsigned char* buf, unsigned int BuferSize);
	int USB_ReadSystemInfo(unsigned char* buf, unsigned int BuferSize);
	int USB_GetBadBlockCriteria(unsigned char* buf, unsigned int BuferSize);
	virtual void Dev_GenSpare_32(unsigned char *buf, unsigned char mode, unsigned char FrameNo, unsigned char Section, int buf_len);
	void GenU17Spare(unsigned char *buf, unsigned char mode, unsigned char FrameNo, unsigned char Section);
	void GetBootCodeRule(std::vector<int>* buf, int pageLogBit);
	void PrepareIDpage(unsigned char *buf, int target_interface, int target_clock, int userSpecifyBlkCnt);
	void InitialCodePointer(unsigned char *buf, unsigned short *blk);
	void CalculatePerFrameDummy(int ldpcMode);
	int USB_SetSnapWR(int ldpcMode);
	void PrepareIDpageForRDTBurner(int target_interface, int target_clock, int userSpecifyBlkCnt);
	int Trim_Efuse(bool isGet, unsigned char * buf, int bank);
	int PreparePadpage(unsigned char *buf, unsigned char * odt_setting);
	int USB_ISPProgFlash(unsigned char* buf, unsigned int BuferSize, bool mScrambled, bool mLast, unsigned int m_Segment);
	int USB_ISPReadFlash(unsigned char* buf, unsigned int BuferSize, bool mScrambled, bool mLast, unsigned int m_Segment);
	int Nvme3PassVenderCmd(unsigned char * cdb512, unsigned char * iobuf, int length, int direction, int timeout);
	int BuildNvmeVendor(unsigned char * nvme64, int dataLenBytes, int directions);
	int USB_Get_Rdt_Log(unsigned char * buf, unsigned int BuferSize, int table, int page);
	int USB_Scan_Rdt_Log();
	virtual int GetFwVersion(char * version/*I/O*/);
	bool Check5017CDBisSupport(unsigned char *CDB);
	virtual int  Issue512CdbCmd(unsigned char * cdb512, unsigned char * iobuf, int lenght, int direction, int timeout);
	int SCSISendSQ(unsigned char *buf, int timeout);
	int SCSIWriteReadData(bool read, unsigned char *buf, unsigned int bufLength, int timeout);
	int SCSIReadCQ(unsigned char *buf, int timeout);
	int Dev_IssueIOCmd(const unsigned char* sql, int sqllen, int mode, int ce, unsigned char* buf, int len);
	int _TrigerDDR(int CE, const unsigned char* sqlbuf, int slen, int buflen, unsigned char channel);
	int _WriteDDR(unsigned char* w_buf, int Length, int ce);
	int _ReadDDR(unsigned char* r_buf, int Length, int datatype);
	int FFReset();
	void saveToFile(unsigned char *buf, int Length, int fileName, bool read);
	virtual int GetDataRateByClockIdx(int clk_idx);
	virtual int NT_CustomCmd_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool is512CDB=false);
	int checkRunningMode();
	int ST_Relink(unsigned char type);
	virtual int GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &tWHR2);
	int USB_ISPProgPRAM_ST(const char* m_MPBINCode, int type, int portinfo);
	int USB_ISPProgPRAM_ST(unsigned char* buf, unsigned int BuferSize, bool mScrambled, bool mLast, unsigned int m_Segment, int type);
	int NT_ReadPage_All(BYTE bType, HANDLE DeviceHandle_p, BYTE bDrvNum, const char *type, unsigned char *bBuf, unsigned char *DriveInfo = 0);

private:
	DeviveInfo* _info;
	unsigned char pIdPageC[4096];
	unsigned short *pIdPageS;
	unsigned int *pIdPageI;
	unsigned char pCPDataC[4096];
	unsigned short *pCPDataS;
	unsigned int *pCPDataI;
	int running_mode;
	unsigned char *gCPBuf;
	int writePageCount;
};
typedef USBSCSI5017_ST MyUSBScsiUtil_5017;

class SSD_ODTInfo
{
public:
	static SSD_ODTInfo* Instance(const char*);
	static void Free();
	const unsigned char* GetParamByLine(int index);
	const unsigned char* GetParam(const char* ctrl, const char* flash, const char* model, unsigned char die, unsigned char readrate, unsigned char writerate );
private:
	SSD_ODTInfo();
	~SSD_ODTInfo();
	static SSD_ODTInfo* _inst;
	char** _keys;
	unsigned char** _datas;
	int _colCnt;
	int _rowCnt;
};
