#include "stdafx.h"
#include "ufw_main.h"
#include "../PCIESCSI5019_ST.h"
#include "../SATASCSI3111_ST.h"
#include "../PCIESCSI5012_ST.h"
#include "ERROR_CODE.h"
#include "CommonDefine.h"
#ifdef _USB_
#include "CLoadSortingISP.h"
#endif
//#include "SortingManager.h"
//#include "SortingISPManager.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

USBSCSICmd_ST *NTCmd=NULL;
bool isPowerOff = true;
static bool is_jm583_send_i2c = true;
HANDLE DeviceHandle = INVALID_HANDLE_VALUE;
int DeviceType = IF_USB | DEVICE_USB;
DeviveInfo driveInfo;
MyFlashInfo *mflash;
FH_Paramater _FH;
BYTE AP_Parameter[PARAM_LEN] = { 0 };
BYTE Backup_AP_Parameter[PARAM_LEN] = { 0 };
BYTE bE2NAMD=0;
unsigned char cache_e13_flhid_buffer_bootcode[512] = { 0 };
int _ataCeNumberPerChannel;
int _current_flash_interface;
BYTE isPBA=0;
BYTE isPBA2=0;
int RWMode = 0;
bool isInitDrive = false;
bool isInitFlash = false;
BYTE TYPE = _D3_; 

#ifdef _WD_
BYTE isCustomerSupportID = false;
#else
BYTE isCustomerSupportID = true;
#endif

//SortingManager SrtManage;
//SortingISPManager *m_SrtIsp;

typedef enum _CTL_TYPE
{
	CTL_USB,
	CTL_S11,
	CTL_S12,
	CTL_E12,
	CTL_S13,
	CTL_E13,
	CTL_E19,
	CTL_S19,
	CTL_5017,
}CTL_TYPE;

void PrintOutDebugString(const char *IN_sDebugString, ...);
int CheckDLLFuncInit();
int CheckDLLFuncEnd();
void UpdateParameterFromFlashInfo(unsigned char * flashid_buffer);
CTL_TYPE GetControllorType();
int TestInit();
unsigned int code2size(unsigned char flashCode, unsigned char MakerCode, unsigned char* tID);
unsigned int code2sizeHynix14(unsigned char flashCode);
unsigned int code2sizeHynix3D(unsigned char flashCode);
int UpdateRWMode();
int SetInitialAPParameter();
void GetCurrentTimeString(char * timestring);
//int SetFeature(BYTE FeatureMode);
int LoadBurner_E19(unsigned char PhyDrive);
char TempLogFile[512] = { 0 };
FILE *fLog;
void PrintOutDebugString(const char *IN_sDebugString, ...);

#ifdef _DEBUG
bool isShowDebugMsg = true;
#else
bool isShowDebugMsg = false;
#endif

void PrintOutDebugString(const char *IN_sDebugString, ...)
{
	if (isShowDebugMsg) {
		char logbuf[8192];
		va_list arglist;
		va_start(arglist, IN_sDebugString);
		_vsnprintf(logbuf, sizeof(logbuf), IN_sDebugString, arglist);
		va_end(arglist);
		printf("%s", logbuf);

		if (strlen(TempLogFile) != 0) {
			fLog = fopen(TempLogFile, "a+");
			if (!fLog) {
				PrintOutDebugString("FOpen fail ");
				return;
			}
			fprintf(fLog, "%s", logbuf);
			if (logbuf[strlen(logbuf) - 1] != '\n') {
				fprintf(fLog, "\n");
			}

			fflush(fLog);
			fclose(fLog);
		}

		strcat(logbuf, "\r\n");
		OutputDebugString(logbuf);
	}

	return;
}
PHISON_TEST_PATTERN_API void SetDebugMessage(bool isEnable) {
	isShowDebugMsg = isEnable;
}

PHISON_TEST_PATTERN_API void EndDLL() {
	delete NTCmd;
	PrintOutDebugString("LOG_END");
	//delete mflash;
}

PHISON_TEST_PATTERN_API void SetTempLogFile(char* LogFileName, int FileNameLen) {
	memcpy(TempLogFile, LogFileName, FileNameLen);
}

PHISON_TEST_PATTERN_API int iGetPcieBridgeDrive(){
	unsigned char InqBuf[528] = { 0 };

	for (int idx = 0; idx < 10; idx++) {
		UFWSCSICmd::_static_Inquiry(idx, InqBuf, true);
		if (memcmp((char*)&InqBuf[0x08], "Bridge  PCIe", 12) == 0) {
			return idx;
		}
	}
	return -1;
}

PHISON_TEST_PATTERN_API int InitPhyDrive(unsigned char PhyDrive) {
	unsigned char InqBuf[528] = { 0 };
	unsigned char readBuf[512] = { 0 };
	char fwVersion[8] = { 0 };
	UFWSCSICmd::_static_Inquiry(PhyDrive, InqBuf, true);
	memset(&driveInfo, 0, sizeof(driveInfo));
	driveInfo.physical = PhyDrive;
	int ui_deviceType = IF_USB | DEVICE_USB;
	DeviceHandle = UFWInterface::_static_MyCreateHandle(PhyDrive, true, false);

	if (INVALID_HANDLE_VALUE == DeviceHandle)
	{
		PrintOutDebugString("Create PhyDrv Handle Error");
		return ERR_INVALID_HANDLE;
	}

	if ((memcmp((char *)&InqBuf[0x10], "2307 PRAM", 9) == 0) || (memcmp((char *)&InqBuf[0x10], "2307 HV TESTER", 9) == 0))
	{
		NTCmd = new USBSCSICmd_ST(&driveInfo, (FILE *)0);
		NTCmd->SetDeviceType(ui_deviceType);
		PrintOutDebugString("Detect 2307 USB device");
	}
	else if ((memcmp((char *)&InqBuf[0x10], "2309 PRAM", 9) == 0) || (memcmp((char *)&InqBuf[0x10], "2309 HV TESTER", 9) == 0))
	{
		NTCmd = new USBSCSICmd_ST(&driveInfo, (FILE *)0);
		NTCmd->SetDeviceType(ui_deviceType);
		PrintOutDebugString("Detect 2307 USB device");
	}
	else if ((memcmp((char *)&InqBuf[0x10], "5019 PRAM", 9) == 0) || (memcmp((char *)&InqBuf[0x10], "5019 SQLC", 9) == 0))
	{
		NTCmd = new PCIESCSI5019_ST(&driveInfo, (FILE *)0);
		ui_deviceType = IF_USB | DEVICE_PCIE_5019;
		NTCmd->SetDeviceType(ui_deviceType);
		NTCmd->ForceSQL(true);
		PrintOutDebugString("PCIE Device 5019");
	}
	else if (memcmp((char *)&InqBuf[0x08], "PHISON  SATA", 12) == 0)
	{
		if (isPowerOff)
		{
			if (UFWInterface::_static_C2012_PowerOnOff(PhyDrive, 1, 0x10, 0, 0, true))
			{
				PrintOutDebugString("SATA Tester Power On fail");
				return ERR_BRIDGERESET;
			}
			Sleep(2000);
			PrintOutDebugString("SATA Tester On bridge ok.");
			isPowerOff = false;
		}

		UFWInterface::_static_IdentifyATA(DeviceHandle, readBuf, 512, IF_C2012);
		int idx = 0;
		for (int i = 46; i <= 53; idx += 2, i += 2) {
			fwVersion[idx] = readBuf[i + 1];
			fwVersion[idx + 1] = readBuf[i];
		}
		fwVersion[7] = '\0';
		PrintOutDebugString("Detect SATA Tester, FW:%s", fwVersion);
		if ('B' == fwVersion[1]) {
			NTCmd = new SATASCSI3111_ST(&driveInfo, (FILE *)0);
			ui_deviceType = IF_C2012 | DEVICE_SATA_3111;
			PrintOutDebugString("S11");
			NTCmd->SetDeviceType(ui_deviceType);
		}
		else if ('C' == fwVersion[1])
		{
			unsigned char info_buf[4096] = { 0 };
			PCIESCSI5012_ST * cmd5012 = NULL;
			PrintOutDebugString("SATA S12 device");
			ui_deviceType = IF_C2012 | DEVICE_PCIE_5012;
			NTCmd = new PCIESCSI5012_ST(&driveInfo,(FILE *)0);
			cmd5012 = (PCIESCSI5012_ST *)(NTCmd);
			NTCmd->SetDeviceType(ui_deviceType);
			cmd5012->Read_System_Info_5012(info_buf, 4096);
		}
	}
	else if (memcmp((char *)&InqBuf[0x08], "ASMT", 4) == 0 || memcmp((char *)&InqBuf[0x08], "JMicron SSD", 11) == 0 || memcmp((char *)&InqBuf[0x08], "BUFF", 4) == 0
		|| memcmp((char *)&InqBuf[0x08], "Phison  SU30", 12) == 0 || memcmp((char *)&InqBuf[0x08], "PNY", 3) == 0)
	{

		UFWInterface::_static_IdentifyATA(DeviceHandle, readBuf, 512, IF_ASMedia);
		int idx = 0;
		for (int i = 46; i <= 53; idx += 2, i += 2) {
			fwVersion[idx] = readBuf[i + 1];
			fwVersion[idx + 1] = readBuf[i];
		}
		fwVersion[7] = '\0';
		PrintOutDebugString("Detect SATA bridge, FW:%s", fwVersion);
		if ('B' == fwVersion[1]) {
			NTCmd = new SATASCSI3111_ST(&driveInfo, (FILE *)0);
			ui_deviceType = IF_ASMedia | DEVICE_SATA_3111;
			PrintOutDebugString("S11");
		}
		else if ('C' == fwVersion[1]) {
			NTCmd = new PCIESCSI5012_ST(&driveInfo, (FILE *)0);
			ui_deviceType = IF_ASMedia | DEVICE_PCIE_5012;
			PrintOutDebugString("S12");
		}
		else if ('D' == fwVersion[1]) {
			NTCmd = new PCIESCSI5013_ST(&driveInfo, (FILE *)0);
			ui_deviceType = IF_ASMedia | DEVICE_PCIE_5013;
			PrintOutDebugString("S13");
		}
		NTCmd->SetDeviceType(ui_deviceType);
	}
	else if (memcmp((char *)&InqBuf[0x08], "Bridge  PCIe", 12) == 0)
	{
		ADMIN_IDENTIFY_CONTROLLER nvmeIdentify;
		if (isPowerOff)
		{
			if (-1 == UFWInterface::_static_JMS583_BridgeReset(PhyDrive, is_jm583_send_i2c))
			{
				PrintOutDebugString("power on bridge fail.");
				return ERR_BRIDGERESET;
			}
			Sleep(2000);
			PrintOutDebugString("power on bridge ok.");
			isPowerOff=false;
		}

		if (UFWSCSICmd::_static_IdentifyNVMe(PhyDrive, (unsigned char *)&nvmeIdentify, sizeof(nvmeIdentify), IF_JMS583))
		{
			PrintOutDebugString("identify NVMe failed, need force boot code?");
			return ERR_IDENTIFY_NVMe;
		}

		PrintOutDebugString("FR: %s", (char*)nvmeIdentify.FR);
		if ('D' == nvmeIdentify.FR[1])
		{
			NTCmd = new PCIESCSI5013_ST(&driveInfo, (FILE *)0);
			ui_deviceType = IF_JMS583 | DEVICE_PCIE_5013;
			NTCmd->SetDeviceType(ui_deviceType);
			NTCmd->ST_SetGlobalSCSITimeout(60 * 2);
			PrintOutDebugString("PCIE Device 5019 with Bridge");
		}
		else if ('J' == nvmeIdentify.FR[1] || '7' == nvmeIdentify.FR[0]) {
			NTCmd = new PCIESCSI5019_ST(&driveInfo, (FILE *)0);
			ui_deviceType = IF_JMS583 | DEVICE_PCIE_5019;
			NTCmd->SetDeviceType(ui_deviceType);
			NTCmd->ST_SetGlobalSCSITimeout(60 * 2);
			PrintOutDebugString("PCIE Device 5019 with Bridge");
		}
		else {
			PrintOutDebugString("Device Error");
			return ERR_DEVICE;
		}
	}
	else {
		PrintOutDebugString("Device Error");
		return ERR_DEVICE;
	}

	NTCmd->m_physicalDriveIndex = PhyDrive;
	if (0 != NTCmd) {
		NTCmd->SetIniFileName((char *)DEBUG_INI_FILE);
	}
	else {
		PrintOutDebugString("Create NTCmd Error");
		return ERR_SCSI;
	}


	if (INVALID_HANDLE_VALUE != DeviceHandle)
	{
		CloseHandle(DeviceHandle);
	}
	Sleep(200);
	DeviceType = ui_deviceType;
	isInitDrive = true;
	return 0;
}

PHISON_TEST_PATTERN_API int InitialBurner(int PhyDrv) {
	int FuncRet = 0;
	FuncRet = LoadBurner_E19(PhyDrv);
	if (FuncRet)return FuncRet;

	isInitFlash = true;

	return 0;
}

PHISON_TEST_PATTERN_API int InitialFlash(int PhyDrv) {
	int FuncRet = 0;
	FuncRet = LoadBurner(PhyDrv);
	if (FuncRet)return FuncRet;

	FuncRet = SetInitialAPParameter();
	if (FuncRet)return FuncRet;

	isInitFlash = true;

	return 0;
}

PHISON_TEST_PATTERN_API int GetCVD(int m_CE, int m_Block, int m_PageStart, bool isSLC, int start_vth, int Vth_Step)
{
	int FuncRet = 0;
	FuncRet = CheckDLLFuncInit();
	if (FuncRet)return FuncRet;


	//int m_Block = 0;
	//int m_PageStart = 0;
	//int m_CE = 0;
	int count1 = 0;
	int allpage = 0;
	int is3D = 0;
	//bool isSLC = true;
	char strType[10] = { 0 };
	if (isSLC)sprintf(strType, "SLC");
	else sprintf(strType, "TLC");

	unsigned char Count0[256] =
	{
		8, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4,
		7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
		7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
		7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
		5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
		4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0
	};

	unsigned int UserBlock;
	unsigned int WL;
	//unsigned int start_vth = 0;
	//unsigned int Vth_Step = 4;
	unsigned int Count = 256 / Vth_Step;
	unsigned int Vth_End;
	int kk;

	ULONG Count_one_value;
	unsigned char temp;
	int ipagesize = 9;
	if (_FH.PageSizeK == 16)
		ipagesize = 18;
	DWORD pagesize = ipagesize * 1024;
	FILE *Count1_data;

	unsigned char *ucBuf1 = new unsigned char[pagesize];
	memset(ucBuf1, 0, pagesize);
	UserBlock = m_Block;
	WL = m_PageStart;
	char fileName[512];
	char timestring[128];
	GetCurrentTimeString(timestring);
	memset(fileName, 0, 512);


	Vth_End = (Count * Vth_Step) + start_vth;
	//printf("Count 1 from Vth %d to %d and step is %d , page size  = %d ", start_vth, Vth_End, Vth_Step, pagesize);


	int pagecount = 1;
	int TotalLevel = 7;
	if (isSLC)TotalLevel = 1;

	if (allpage) {
		if (isSLC)
			pagecount = _FH.MaxPage;
		else
			pagecount = _FH.BigPage;
	}

	BYTE *vLevel = new BYTE[Count];
	ULONG *vData = new ULONG[Count];
	ULONG *vCount_one_value = new ULONG[Count];
	ULONG *vAllPageData = new ULONG[Count*pagecount];

	bool isFail = false;
	for (int Level = 0; Level<TotalLevel; Level++)
	{
		if (isFail) break;
		GetCurrentTimeString(timestring);
		memset(vAllPageData, 0, Count*pagecount * sizeof(ULONG));
		sprintf_s(fileName, ".//CVD//CVD_%s_Level%d_CE%d_block%d_page%d__%s.csv", strType,Level, m_CE,UserBlock, m_PageStart, timestring);
		Count1_data = fopen(fileName, "w");

		for (int page = 0; page<pagecount; page++)
		{
			if (isFail) break;
			if (!allpage) page = m_PageStart;

			int index = 0;
			for (unsigned i = 0; i< Count; i++)
			{
				vData[i] = 0;
				vCount_one_value[i] = 0;
				vLevel[i] = 0;
			}
			//	Vth_End=257;
			//	start_vth=255;
			int countPageSize = _FH.PageSize_H << 8 | _FH.PageSize_L;
			for (unsigned int vt_level = start_vth; vt_level< Vth_End - 1; vt_level += Vth_Step)
			{
				// James test
				if (isFail) break;
				int tmp_vth = vt_level; //vt_level + 0x80;
				if (tmp_vth >= 0x100)
					tmp_vth -= 0x100;
				int ret = NTCmd->ST_Send_Dist_read(0, NULL, 0, m_CE, UserBlock, page, tmp_vth, ucBuf1, ipagesize, is3D, isSLC, Level + 1);
				if (ret) {
					PrintOutDebugString("ST_Send_Dist_read Error");
					isFail = true;
					break;
				}
				Count_one_value = 0;
				for (kk = 0; kk<(countPageSize); kk++)
				{
					temp = ucBuf1[kk];
					Count_one_value += (8 - Count0[temp]);
				}
				vLevel[index] = tmp_vth;
				vCount_one_value[index++] = Count_one_value;
			}


			index = 0;
			for (unsigned i = 0; i< Count - 1; i++)
			{
				if (vCount_one_value[i + 1]>vCount_one_value[i])
				{
					vData[index] = vCount_one_value[i + 1] - vCount_one_value[i];
				}
				else
				{
					vData[index] = vCount_one_value[i] - vCount_one_value[i + 1];
				}
				index++;
			}


			index = 0;
			if (allpage) {
				for (unsigned i = 0; i< Count; i++)
				{
					vAllPageData[i + (page*Count)] = vData[i];
					index++;
				}
				PrintOutDebugString("VT page %d", page);
			}
			else {
				fprintf(Count1_data, "Page%d,Shift,Count1,Diff\n", page);
				for (unsigned i = 0; i< Count; i++)
				{
					fprintf(Count1_data, "%d,0x%X,%32d,%32d\n", i, vLevel[i],vCount_one_value[i], vData[i]);
					index++;
				}
			}
		}

		if (allpage) {
			for (int p = 0; p<pagecount; p++)
			{
				fprintf(Count1_data, "%d,", p);
			}
			fprintf(Count1_data, "\n");
			for (unsigned int i = 0; i< Count; i++)
			{
				char tempstr[4096];
				memset(tempstr, 0, 4096);
				for (int p = 0; p<pagecount; p++)
				{
					char temp[512];
					memset(temp, 0, 512);
					sprintf(temp, "%d,", vAllPageData[p*Count + i]);
					strcat(tempstr, temp);
				}
				fprintf(Count1_data, "%s\n", tempstr);
			}
		}
		fclose(Count1_data);
	}

	delete[] vLevel;
	delete[] vData;
	delete[] vCount_one_value;
	delete[] vAllPageData;
	vData = NULL;
	vCount_one_value = NULL;
	delete[] ucBuf1;

	if (isFail)return 1;

	return 0;
}

PHISON_TEST_PATTERN_API int GetFlashID(unsigned char* BufOut) {
	int FuncRet = 0;
	FuncRet = CheckDLLFuncInit();
	if (FuncRet)return FuncRet;


	unsigned char IDbuf[512];
	memset(IDbuf, 0, sizeof(IDbuf));

	NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, IDbuf);
	//Read ID : 0x90
	//int sqllen = 21;
	//int mode = 2;
	//int buflen = 512;
	//int ce = 0;
	//unsigned char sql[21] = {
	//	0xCA, 0x09, 0x01, 0xCF, 0x00, 0xC2, 0x90, 0xAB, 0x00,0xDB,0x08,0xCE, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00,
	//	0x00, 0xAE, 0xAF
	//};

	////PCIESCSI5013_ST * cmd5013 = (PCIESCSI5013_ST*)NTCmd;
	////cmd5013->NVMe_SetAPKey();
	//NTCmd->GetSQLCmd()->Dev_IssueIOCmd(sql, sqllen, mode, ce, IDbuf, buflen);
	memcpy(BufOut, IDbuf, sizeof(IDbuf));

	return 0;
}

PHISON_TEST_PATTERN_API void ReadWriteLDPCSetting(bool isWrite, int Page, int _userSelectLDPC_mode) {
	int _userSelectBlockSeed = 0x8299;
	//int _userSelectLDPC_mode = -1;
	NTCmd->Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, Page, isWrite, _userSelectLDPC_mode, 'C');
}

PHISON_TEST_PATTERN_API int SendCmd(unsigned char* CmdBuf, int CmdLen, int mode, int CE, unsigned char * Buf, int BufLen) {
	//int CmdLen = 19;
	//int mode = 2;
	//int BufLen = 512;
	//int CE = 0;
	//unsigned char CmdBuf[19] = {
	//	0xCA, 0x07, 0x01, 0xC2, 0x90, 0xAB, 0x00,0xDB,0x08,0xCE, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00,
	//	0x00, 0xAE, 0xAF
	//};

	//PCIESCSI5013_ST * cmd5013 = (PCIESCSI5013_ST*)NTCmd;
	//cmd5013->NVMe_SetAPKey();
	int FuncRet = 0;
	FuncRet = CheckDLLFuncInit();
	if (FuncRet)return FuncRet;

	PrintOutDebugString("[Step] SendCmd >> 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x Len=%d Mode=%d CE=%d\n", CmdBuf[0],CmdBuf[1], CmdBuf[2], CmdBuf[3], CmdBuf[4], CmdBuf[5], CmdLen, mode, CE);
	int ret = 0;
	ret=NTCmd->GetSQLCmd()->Dev_IssueIOCmd(CmdBuf, CmdLen, mode, CE, Buf, BufLen);
	

	return ret;
}

PHISON_TEST_PATTERN_API int PhySendCmd(int PhyDrv, unsigned char* CmdBuf, int CmdLen, int mode, int CE, unsigned char * Buf, int BufLen)
{

/*	char fwversion[128];
	unsigned char IDbuf[512];
	memset(IDbuf, 0, sizeof(IDbuf));
	isPowerOff = false;
	InitPhyDrive(PhyDrv);
	NTCmd->GetFwVersion(fwversion);
	PrintOutDebugString("load burner done, fw version=%s\n", fwversion);
	NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, IDbuf, 1);//Only buf works
	mflash = MyFlashInfo::Instance("flashInfo.csv", NULL, NULL);
	memset(&_FH, 0x00, sizeof(_FH_Paramater));
	if (0 == mflash->GetParameterMultipleCe(IDbuf, &_FH, "", 0, 0))
	{
		PrintOutDebugString("flash id not in flashinfo.csv");
		return ERR_FLASHID;
	}

	NTCmd->m_flashfeature = FEATURE_LEGACY; // default value.
	NTCmd->SetFlashParameter(&_FH);
	NTCmd->InitSQL();
	NTCmd->ForceSQL(true);
	*/

	DeviceHandle = UFWInterface::_static_MyCreateHandle(driveInfo.physical, true, false);

	PrintOutDebugString("[Step] SendCmd >> 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x Len=%d Mode=%d CE=%d\n", CmdBuf[0], CmdBuf[1], CmdBuf[2], CmdBuf[3], CmdBuf[4], CmdBuf[5], CmdLen, mode, CE);
	NTCmd->Dev_IssueIOCmd(CmdBuf, CmdLen, mode, CE, Buf, BufLen);

	/*
	HANDLE DeviceHandle = UFWInterface::_static_MyCreateHandle(PhyDrv, true, false);
	if (INVALID_HANDLE_VALUE == DeviceHandle)
	{
		PrintOutDebugString("Create PhyDrv Handle Error");
		return ERR_INVALID_HANDLE;
	}
	int direct = 0; 
	if (mode == 1)direct = 2;
	if (mode == 2|| mode == 3)direct = 1;
	int deviceType = IF_JMS583 | DEVICE_PCIE_5013;
	bool silence = false;
	unsigned long timeout = 15;

	int Ret = UFWInterface::_static_MyIssueNVMeCmdHandle(DeviceHandle, unsigned char *nvmeCmd, 0xD1, direct, unsigned char *buf, unsigned long bufLength, timeOut, silence, deviceType);
	*/
	
	if (INVALID_HANDLE_VALUE != DeviceHandle)
	{
		CloseHandle(DeviceHandle);
	}
	return 0;
}
/*
	int _TrigerDDR(int DeviceType, int CE, const unsigned char* sqlbuf, int slen, int buflen, unsigned char channel)
	{
		if (DeviceType & DEVICE_USB) {
			unsigned char cdb[16];
			int len = ((slen + 511) / 512) * 512;
			unsigned char *tempbuf;

			tempbuf = (unsigned char *)malloc(len);

			memset(cdb, 0x00, sizeof(cdb));
			memset(tempbuf, 0x00, len);

			cdb[0] = 0x06;
			cdb[1] = 0xFE;
			cdb[2] = 0;//_sqlCmd->_dataBufferAddr;	// used data mode
			cdb[3] = len / 512;
			cdb[6] = CE;
			memcpy(tempbuf, sqlbuf, slen);
			int ret = NT_CustomCmd_Phy(_hUsb, cdb, 16, SCSI_IOCTL_DATA_OUT, len, tempbuf, 60);;
			free(tempbuf);
			return ret;
		}

		if (DeviceType & DEVICE_PCIE_5013) {
			int ret = NVMe_SetAPKey();
			if (ret) {
				return ret;
			}

			unsigned char ncmd[16];
			unsigned char GetLength = 16;
			memset(ncmd, 0x00, 16);

			int newlen = _SQLMerge(sqlbuf, slen);

			ncmd[0] = 0x52;
			//ncmd[1] = 0x01;
			ncmd[2] = 0x00;

			ncmd[4] = (newlen & 0xff);
			ncmd[5] = ((newlen >> 8) & 0xff);
			ncmd[6] = ((newlen >> 16) & 0xff);
			ncmd[7] = ((newlen >> 24) & 0xff);
			ncmd[8] = buflen / 1024;

			//ncmd[9] = channel;
			ncmd[10] = channel*_channelCE + CE;
			ret = UFWInterface::_static_MyIssueNVMeCmdHandle(DeviceHandle, ncmd, 0xD1, 2, (unsigned char*)_dummyBuf, newlen, 15, false);
		}

		return ret;
	}
*/

PHISON_TEST_PATTERN_API int ReadPage(int CE, int Block, int Page, unsigned char* BufOut, bool isSLC) {
	int FuncRet = 0;
	FuncRet = CheckDLLFuncInit();
	if (FuncRet)return FuncRet;

	int mode = RWMode;
	TYPE = _D3_;
	if (isSLC) TYPE = _D1_;
	//BYTE TYPE = _D3_; //move to global

	int _userSelectBlockSeed = 0x8299;
	int _userSelectLDPC_mode = -1;
	int patternMode = 2;// user defined buffer
	int pageCnt = 1;
	int planeMode = 1;
	bool cacheEnable = false;
	bool retryRead = false;
	int datalen = _FH.PageSizeK * 1024;

	NTCmd->Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, Page, false, _userSelectLDPC_mode, 'C');

	if (TYPE == _D1_)
		FuncRet = NTCmd->GetSQLCmd()->GroupReadDataD1(CE, Block, Page, pageCnt, BufOut, datalen, planeMode, cacheEnable, retryRead);
	else
		FuncRet = NTCmd->GetSQLCmd()->GroupReadData(CE, Block, Page, pageCnt, BufOut, datalen, planeMode, cacheEnable, retryRead);

	PrintOutDebugString("[Step] Read CE%d Block%d >> %d", CE, Block, FuncRet);
	return FuncRet;
}

PHISON_TEST_PATTERN_API int WritePage(int CE, int Block, int Page, unsigned char* WriteBuf, bool isSLC) {
	int FuncRet = 0;
	FuncRet = CheckDLLFuncInit();
	if (FuncRet)return FuncRet;

	int mode = RWMode;
	TYPE = _D3_;
	if (isSLC) TYPE = _D1_;

	//normal write/read 
	int _userSelectBlockSeed = 0x8299;
	int _userSelectLDPC_mode = -1;
	int patternMode = 2;// user defined buffer
	int pageCnt = 1;
	int planeMode = 1;
	bool cacheEnable = false;
	bool retryRead = false;
	int datalen = _FH.PageSizeK * 1024;

	NTCmd->Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, Page, true, -1, 'C');
	if (TYPE == _D1_)
		FuncRet = NTCmd->GetSQLCmd()->GroupWriteD1(CE, Block, Page, pageCnt, WriteBuf, datalen, patternMode, planeMode, cacheEnable);
	else
		FuncRet = NTCmd->GetSQLCmd()->GroupWrite(CE, Block, Page, pageCnt, WriteBuf, datalen, patternMode, planeMode, cacheEnable);

	PrintOutDebugString("[Step] Write CE%d Block%d >> %d", CE, Block, FuncRet);
	return FuncRet;
}

PHISON_TEST_PATTERN_API int ReadECC(int CE, int Block, int Page, unsigned char* BufOut) {
	int FuncRet = 0;
	FuncRet = CheckDLLFuncInit();
	if (FuncRet)return FuncRet;

	int mode = RWMode;
	//BYTE TYPE = _D3_;

	int _userSelectBlockSeed = 0x8299;
	int _userSelectLDPC_mode = -1;
	int patternMode = 2;// user defined buffer
	int pageCnt = 1;
	int planeMode = 1;
	bool cacheEnable = false;
	bool retryRead = false;
	int datalen = _FH.PageSizeK;

	NTCmd->Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, Page, false, _userSelectLDPC_mode, 'C');

	if (TYPE == _D1_)
		FuncRet = NTCmd->GetSQLCmd()->GroupReadEccD1(CE, Block, Page, pageCnt, BufOut, datalen, planeMode, cacheEnable, retryRead);
	else
		FuncRet = NTCmd->GetSQLCmd()->GroupReadEcc(CE, Block, Page, pageCnt, BufOut, datalen, planeMode, cacheEnable, retryRead);

	PrintOutDebugString("[Step] ReadECC CE%d Block%d >> %d", CE, Block, FuncRet);
	return FuncRet;
}

PHISON_TEST_PATTERN_API int EraseBlock(int CE, int Block, bool isSLC) {
	int FuncRet = 0;
	FuncRet = CheckDLLFuncInit();
	if (FuncRet)return FuncRet;

	int mode = RWMode;
	//BYTE TYPE = _D3_;
	bool m_A2Check = false;
	int res = 0;
	unsigned char StatusBuf[512] = { 0 };
	TYPE = _D3_;
	if (isSLC) TYPE = _D1_;

	if (TYPE == _D1_)
	{
		if (!_FH.beD3) {//MLC flash
			mode &= ~0x01;//disable SLC mode
			if (m_A2Check)
				mode |= 0x01;//enable SLC mode
		}
		else
			mode |= 0x01;

		//mode D1:Bit1=1  D3:Bit1=0
		if (res=NTCmd->ST_DirectOneBlockErase(0, NULL, 0, mode, CE, Block, 0, _DATA_LENGTH_, NULL))
			PrintOutDebugString("[Step] Erase CE%d Block%d  >> BAD ",CE,Block);

		if (res) {
			if (NTCmd->ST_GetReadWriteStatus(0, NULL, 0, StatusBuf))
			{
				if (res != 0xfffffffe)
					return 2;
				else
					return res;
			}
			if (StatusBuf[0] && (0x01 << CE))
				PrintOutDebugString("[Step] Status CE%d Block%d  >> BAD ", CE, StatusBuf[0]);
		}
	}
	else if (TYPE == _D3_)
	{
		//mode D1:Bit1=1  D3:Bit1=0
		mode &= 0xFE;
		if (res = NTCmd->ST_DirectOneBlockErase(0, NULL, 0, mode, CE, Block, 0, _DATA_LENGTH_, NULL))
		{
			PrintOutDebugString("[Step] Erase CE%d Block%d  >> BAD ", CE, Block);
			if (NTCmd->ST_GetReadWriteStatus(0,NULL,0,StatusBuf))
			{
				if (res != 0xfffffffe)
					return 2;
				else
					return res;
			}
			if (StatusBuf[0] && (0x01 << CE))
				PrintOutDebugString("[Step] Status CE%d Block%d  >> BAD ", CE, StatusBuf[0]);
		}
	}

	return 0;
}

int CheckDLLFuncInit() {
	if (!isInitDrive || !isInitFlash)
	{
		PrintOutDebugString("Drive and Drive has not been initialized");
		return ERR_INITIAL;
	}

	if (NULL == NTCmd) {
		PrintOutDebugString("NTCmd has not been initialized");
		return ERR_SCSI;
	}

	if (!isCustomerSupportID){
		PrintOutDebugString("Flash ID Not Support");
		return ERR_FLASHID;
	}

	//int CheckRet = 0;
	//CheckRet=CheckDLLFuncEnd();
	//if (CheckRet)return CheckRet;

	NTCmd->SetDeviceType(DeviceType);
	NTCmd->ST_SetGlobalSCSITimeout(60 * 2);
	NTCmd->ForceSQL(true);

	return 0;
}

int CheckDLLFuncEnd() {
	if (INVALID_HANDLE_VALUE != DeviceHandle)
	{
		PrintOutDebugString("Closing Device Handler");
		CloseHandle(DeviceHandle);
		if (INVALID_HANDLE_VALUE != DeviceHandle)
		{
			PrintOutDebugString("Device Handler Error");
			return ERR_INVALID_HANDLE;
		}
	}
	return 0;
}

CTL_TYPE GetControllorType()
{
	if ((CONTROLLER_MASK&DeviceType) == 0)
		return CTL_TYPE::CTL_USB;

	if ((CONTROLLER_MASK&DeviceType) == DEVICE_SATA_3111)
		return CTL_TYPE::CTL_S11;

	if ((CONTROLLER_MASK&DeviceType) == DEVICE_PCIE_5013)
		return CTL_TYPE::CTL_E13;

	if ((CONTROLLER_MASK&DeviceType) == DEVICE_PCIE_5019)
		return CTL_TYPE::CTL_E13;//for now we do the same thing for 5013 & 5019

	if ((CONTROLLER_MASK&DeviceType) == DEVICE_PCIE_5016)
		return CTL_TYPE::CTL_E13;//for now we do the same thing for 5013 & 5019 & 5016

	if ((CONTROLLER_MASK&DeviceType) == DEVICE_PCIE_5012)
		return CTL_TYPE::CTL_E13;//for now we do the same thing for 5013 & 5019 & 5016

	if ((CONTROLLER_MASK&DeviceType) == DEVICE_PCIE_5017)
		return CTL_TYPE::CTL_5017;//for now we do the same thing for 5013 & 5019 & 5016

	return CTL_TYPE::CTL_USB;
}

void UpdateParameterFromFlashInfo(unsigned char * flashid_buffer)
{
	mflash = MyFlashInfo::Instance("flashInfo.csv", NULL, NULL);
	memset(AP_Parameter, 0, PARAM_LEN);
	if (!mflash->GetParameterMultipleCe(flashid_buffer, &_FH, "", 0, 0))
	{
		return;
	}
	mflash->ParamToBuffer(&_FH, AP_Parameter, "test.ini", 0, bE2NAMD, _MP_PARAM_SPEC_, 0, NULL, 0);
	if (CTL_E13 == GetControllorType()) {
		if ('T' == _FH.bToggle) {
			AP_Parameter[0x4D] = 0x03;
		}
		else {
			AP_Parameter[0x4D] = 0x02;
		}
	}

	if (_FH.bIsMicron3DTLC) {
		if (_FH.bIsMIcronB0kB)
			AP_Parameter[0x28] |= 0x01; //b0kB
		else {
			if (_FH.bIsMicronB17)
				AP_Parameter[0x28] |= 0x04; //b17
			else if (_FH.bIsMicronB16)
				AP_Parameter[0x28] |= 0x02; //b16
			else if (_FH.bIsMIcronB05)
				AP_Parameter[0x28] |= 0x08; //b05
			else if (_FH.bIsMIcronN18)
				AP_Parameter[0x28] |= 0x10; //N18
			else if (_FH.bIsMIcronN28)
				AP_Parameter[0xDD] |= 0x01; //N28
			else if (_FH.bIsMicronB27) {
				if (_FH.id[3] == 0xA2)			// B27A
					AP_Parameter[0x28] |= 0x20;
				else if (_FH.id[3] == 0xE6)	// B27B
					AP_Parameter[0x28] |= 0x40;
			}
		}
	}

	AP_Parameter[0x80] = AP_Parameter[0x81] = _FH.CEPerSlot;
	if (NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, true, AP_Parameter))
		return;
}

void UpdateMaxCeCountToFH(FH_Paramater * fh)
{
	int CeCount = 0;
	unsigned char buf[512] = { 0 };
	memset(buf, 0, sizeof(buf));
	NTCmd->NT_GetFlashExtendID(1, DeviceHandle, NULL, buf);
	//Toshiba    0x98
	//Sandisk    0x45
	//Micron    0x2C
	//Intel      0x89
	//Hynix       0xAD
	_ataCeNumberPerChannel = 0;
	for (int i = 0; i<32; i++) {
		if (0x98 == buf[i * 16] || 0x45 == buf[i * 16] || 0x2C == buf[i * 16] || 0x89 == buf[i * 16] || 0xAD == buf[i * 16])
		{
			CeCount++;
			if (i<8)
				_ataCeNumberPerChannel++;
		}
	}

	if (CeCount> fh->FlhNumber) {
		fh->FlhNumber = CeCount;
		PrintOutDebugString("CE Refine => %d", fh->FlhNumber);
	}
}

PHISON_TEST_PATTERN_API int LoadBurner(unsigned char PhyDrive) {
	int status = 0;
	int ssd_isp_status = -1;
	int bUFSCE_ = 0;
	unsigned char buf[528];
	memset(buf, 0, sizeof(buf));
	char fwversion[128];
	//char prompt[128];
	char burner_name[128] = { 0 };
	int fileNameLength = 0;
	DeviceHandle = UFWInterface::_static_MyCreateHandle(driveInfo.physical, true, false);

	if (INVALID_HANDLE_VALUE == DeviceHandle)
	{
		PrintOutDebugString("Create PhyDrv Handle Error");
		return ERR_INVALID_HANDLE;
	}

	memset(fwversion, 0, sizeof(fwversion));
	bool isPCIE = false;

	if (CTL_E13 == GetControllorType()) { // E19 = E13
		NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, buf, 1);//Only buf works
		mflash = MyFlashInfo::Instance("flashInfo.csv", NULL, NULL);
		memset(&_FH, 0x00, sizeof(_FH_Paramater));
		int over4ce_ = 0;
		if (0 == mflash->GetParameterMultipleCe(buf, &_FH, "", over4ce_, bUFSCE_))
		{
			PrintOutDebugString("flash id not in flashinfo.csv");
			return ERR_FLASHID;
		}

		NTCmd->m_flashfeature = FEATURE_LEGACY; // default value.

		ssd_isp_status = 1;
		memset(fwversion, 0, 12);

		char burner[128] = { 0 };
		char rdt[128] = { 0 };
		((PCIESCSI5013_ST *)NTCmd)->GetBurnerAndRdtFwFileName(&_FH, burner, rdt);
		//memcpy(burner, "B5019.bin", 10);
		if (!PathFileExistsA(burner))
		{
			PrintOutDebugString("auto select file %s not exist", burner);
			return ERR_LOADBURNER;
		}
		PrintOutDebugString("Burner: %s", burner);

		memset(cache_e13_flhid_buffer_bootcode, 0, sizeof(cache_e13_flhid_buffer_bootcode));
		NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, cache_e13_flhid_buffer_bootcode, 0, 0, 0);
		NTCmd->SetFlashParameter(&_FH);
		if (NTCmd->InitDrive(0,  NULL, 2)) {
			memset(fwversion, 0, sizeof(fwversion));
			NTCmd->GetFwVersion(fwversion);
			PrintOutDebugString("load burner done, fw version=%s\n", fwversion);
			UpdateParameterFromFlashInfo(cache_e13_flhid_buffer_bootcode);
			NTCmd->SetFlashParameter(&_FH);
		}
		else {
			PrintOutDebugString("load burner fail");
			return ERR_LOADBURNER;
		}

		if (0 == strlen(_FH.PartNumber))
		{
			/*E16, E12 can't get flash ID in boot code, so we need get ID in burner then update ap parameter*/
			NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, cache_e13_flhid_buffer_bootcode, 0, 0, 0);
				/*Now we can select correct file name for this flash ID, re-load  burner.*/
			if (NTCmd->InitDrive(0, (fileNameLength>4) ? burner_name : NULL, 2)) {
				memset(fwversion, 0, sizeof(fwversion));
				NTCmd->GetFwVersion(fwversion);
				PrintOutDebugString("load burner done, fw version=%s\n", fwversion);
				UpdateParameterFromFlashInfo(cache_e13_flhid_buffer_bootcode);
				NTCmd->InitSQL();
			}
			else
			{
				PrintOutDebugString("load burner fail2");
			}
		}

		NTCmd->InitSQL();

		memset(buf, 0, 512);
		NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, buf, 1);
	
		if ((buf[0] == 0x45 && buf[1] == 0x48 && buf[2] == 0x99 && buf[3] == 0xB3 && buf[4] == 0xFA && buf[5] == 0xE3) ||
			(buf[0] == 0x45 && buf[1] == 0x49 && buf[2] == 0x9A && buf[3] == 0xB3 && buf[4] == 0xFE && buf[5] == 0xE3) ||
			(buf[0] == 0x45 && buf[1] == 0x3E && buf[2] == 0x98 && buf[3] == 0xB3 && buf[4] == 0xF6 && buf[5] == 0xE3)) {
			isCustomerSupportID = true;
		}


		if (0 == buf[0] || 1 == buf[0])
		{
			PrintOutDebugString("Read ID fail after set ap parameter");
			return ERR_FLASHID;
		}

		PrintOutDebugString("CE %d", _FH.FlhNumber);
		UpdateMaxCeCountToFH(&_FH);
		PrintOutDebugString("%s : CE%d Die%d Block%d Plane%d TLC%d QLC%d PageSizeK%d PagePerBlk%d NM%X", _FH.PartNumber, _FH.FlhNumber, _FH.DieNumber, _FH.MaxBlock, _FH.Plane, _FH.beD3, _FH.Cell == 0x10, _FH.PageSizeK, _FH.PagePerBlock, _FH.Design);
		mflash->ParamToBuffer(&_FH, AP_Parameter, "test.ini", isPBA, 0, _MP_PARAM_SPEC_, 0, NULL, 0);
		_FH.CEPerSlot = _FH.FlhNumber;
		_FH._RealPlane = 1;
		_FH.bPBADie = 1;
		if (_FH.beD3 && _FH.Design > 0x15) _FH._RealPlane = 0;

		_FH.MBC_ECC = 39;
		int bFlashMixMode = 0;
		NTCmd->SetFlashParameter(&_FH);
		NTCmd->ForceSQL(true);

		return 0;
	}else if (CTL_S11 == GetControllorType()) {
		NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, buf, 1);//Only buf works
		mflash = MyFlashInfo::Instance("flashInfo.csv", NULL, NULL);
		memset(&_FH, 0x00, sizeof(_FH_Paramater));
		int over4ce_ = 0;
		if (0 == mflash->GetParameterMultipleCe(buf, &_FH, "", over4ce_, bUFSCE_))
		{
			PrintOutDebugString("flash id not in flashinfo.csv");
			return ERR_FLASHID;
		}

		NTCmd->m_flashfeature = FEATURE_LEGACY; // default value.

		if (NTCmd->Identify_ATA(fwversion)) {
			PrintOutDebugString("check fw fail!", 0, 0, 0);
			return ERR_DEVICE;
		}
		else {
			PrintOutDebugString("before load burner, fw version is %s", fwversion);
		}

		char burner_filename[256];

		if (((CNTCommandSSD*)NTCmd)->NT_ISPProgPRAMS11("B3111_TLC.bin", DeviceHandle, true, &_FH, burner_filename/*output*/)) {
			PrintOutDebugString("NT_ISPProgPRAMS11 fail!", 0, 0, 0);
			return ERR_DEVICE;
		}

		PrintOutDebugString("load bin file: %s", burner_filename);
		if (NTCmd->Identify_ATA(fwversion))
		{
			PrintOutDebugString("check fw fail!", 0, 0, 0);
		}
		else {
			PrintOutDebugString("load burner done, fw version is %s", fwversion);
			UCHAR cc[64];
			NTCmd->ST_GetCodeVersionDate(1, DeviceHandle, NULL, cc);
			PrintOutDebugString("%s", cc);
			NTCmd->Set_SCSI_Handle(DeviceHandle);
			NTCmd->SetFlashParameter(&_FH);
			NTCmd->InitSQL();
		}

		NTCmd->NT_GetFlashExtendID(1, DeviceHandle, NULL, buf, 1);
		if (0 == buf[0] || 1 == buf[0])
		{
			PrintOutDebugString("Read ID fail after set ap parameter");
			return ERR_FLASHID;
		}
		PrintOutDebugString("CE %d", _FH.FlhNumber);
		UpdateMaxCeCountToFH(&_FH);
		PrintOutDebugString("%s", _FH.PartNumber);
		PrintOutDebugString("CE %d", _FH.FlhNumber);
		PrintOutDebugString("Die %d", _FH.DieNumber);
		PrintOutDebugString("Block %d", _FH.MaxBlock);
		PrintOutDebugString("TLC %d, QLC %d", _FH.beD3, _FH.Cell == 0x10);
		PrintOutDebugString("Plane %d", _FH.Plane);
		PrintOutDebugString("PageSizeK %d", _FH.PageSizeK);
		PrintOutDebugString("PageCountPerBlk %d", _FH.PagePerBlock);
		PrintOutDebugString("NM %X", _FH.Design);
		PrintOutDebugString("%s CE%d Die%d Block%d Plane%d TLC%d QLC%d PageSizeK%d PagePerBlk%d NM%X", _FH.PartNumber, _FH.FlhNumber, _FH.DieNumber, _FH.MaxBlock, _FH.Plane, _FH.beD3, _FH.Cell == 0x10, _FH.PageSizeK, _FH.PagePerBlock, _FH.Design);
		mflash->ParamToBuffer(&_FH, AP_Parameter, "test.ini", isPBA, 0, _MP_PARAM_SPEC_, 0, NULL, 0);
		_FH.CEPerSlot = _FH.FlhNumber;
		_FH._RealPlane = 1;
		_FH.bPBADie = 1;
		if (_FH.beD3 && _FH.Design > 0x15) _FH._RealPlane = 0;
		NTCmd->SetFlashParameter(&_FH);
		NTCmd->ForceSQL(true);

	}
	else if (CTL_USB == GetControllorType()) { 
#ifdef _USB_
		//NTCmd->SetInterFaceType(NTCmd->GetInterFaceType());
		DeviceHandle = UFWInterface::_static_MyCreateHandle(PhyDrive, true, false);

		unsigned char IDBuf[512] = { 0 };
		memset(IDBuf, 0, 512);
		NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, IDBuf);
		mflash = MyFlashInfo::Instance("flashInfo.csv", NULL, NULL);
		memset(&_FH, 0x00, sizeof(_FH_Paramater));
		int over4ce_ = 0;
		mflash->GetParameterMultipleCe(IDBuf, &_FH, "", over4ce_, bUFSCE_);
		NTCmd->SetFlashParameter(&_FH);

		CLoadISP mIsp;
		memcpy(mIsp.sSortingBurner, "b2307_bank.bin", strlen("b2307_bank.bin"));
		PrintOutDebugString("FW %s", mIsp.sSortingBurner);
		mIsp.I_obType = 1;
		mIsp.I_DeviceHandle = DeviceHandle;
		mIsp.pNTCmd = NTCmd;
		mIsp.I_Design = _FH.Design;
		mIsp.I_bEd3 = _FH.beD3;
		if (int res = mIsp.DoIsp())
		{
			PrintOutDebugString("ISP FAIL");
			return ERR_FLASHID;
		}

		//NTCmd->ST_TurnFlashPower(1, DeviceHandle, 0, 1);
		NTCmd->NT_Inquiry_All(1, DeviceHandle, 0, buf);
		NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, IDBuf);
		PrintOutDebugString("Before SetParam ID: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", IDBuf[0], IDBuf[1], IDBuf[2], IDBuf[3], IDBuf[4], IDBuf[5], IDBuf[6]);
		NTCmd->m_flashfeature = FEATURE_LEGACY; // default value.
		UpdateParameterFromFlashInfo(IDBuf);
		NTCmd->InitSQL();
		NTCmd->SetFlashParameter(&_FH);
		NTCmd->ForceSQL(true);

		NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, IDBuf);
		PrintOutDebugString("After SetParam ID: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", IDBuf[0], IDBuf[1], IDBuf[2], IDBuf[3], IDBuf[4], IDBuf[5], IDBuf[6]);


#endif
	}
	else {
		PrintOutDebugString("Not Support Controller");
		return ERR_DEVICE;
	}

	return 0;
}

PHISON_TEST_PATTERN_API int SetTiming(unsigned char PhyDrive,BYTE Timing)
{
	unsigned char read_parameter[PARAM_LEN];
	DeviceHandle = UFWInterface::_static_MyCreateHandle(PhyDrive, true, false);

	if (NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, false, read_parameter)) {
		PrintOutDebugString("GetParameter Command Fail");
		return ERR_SCSI;
	}

	read_parameter[0x4D] = Timing;
	NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, true, read_parameter);
	CheckDLLFuncEnd();
	return 0;
}

PHISON_TEST_PATTERN_API int SetFeature(BYTE FeatureMode)
{
	unsigned char buf2[8256] = { 0 };
	BYTE count = 0;
	bool pass = true;
	BYTE bONFIType = 0, bONFIMode = 0, bToggle20 = 0;
	bToggle20 = 1;
	unsigned char read_parameter[PARAM_LEN];


	if (FeatureMode == __SET_LEGACY_MODE__) {
		if (NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, false, read_parameter)) {
			PrintOutDebugString("GetParameter Command Fail");
			return ERR_SCSI;
		}

		read_parameter[0x4D] = 0x02;
		NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, true, read_parameter);
		bToggle20 = 0;
		memcpy(AP_Parameter, read_parameter, PARAM_LEN);
	}


	if (NTCmd->ST_SetFeature(1, DeviceHandle, 0, FeatureMode, buf2, bONFIType, bONFIMode, bToggle20))
	{
		PrintOutDebugString("Set Feature %d FAIL.", FeatureMode);
		return ERR_SCSI;
	}

	PrintOutDebugString("Get Status: 0x%02x 0x%02x 0x%02x 0x%02x ", buf2[0], buf2[16], buf2[32], buf2[48]);
	for (int ce = 0; ce < _FH.FlhNumber; ce++)
	{
		if (((_FH.bMaker == 0x98) || (_FH.bMaker == 0x45) || (_FH.bMaker == 0xEC) || (_FH.bMaker == 0xad)) && bToggle20)
		{
			if (buf2[ce * 16] != 0x07)
			{
				pass = false;
			}
		}
	}

	if (!pass)return ERR_SETFEATURE;

	if (FeatureMode == __SET_TOGGLE_MODE__) {
		if (NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, false, read_parameter)) {
			PrintOutDebugString("GetParameter Command Fail");
			return ERR_SCSI;
		}

		if (CTL_E13 == GetControllorType()) {
			read_parameter[0x4D] = 0x0A;//1200Mhz
			NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, true, read_parameter);
		}
		memcpy(AP_Parameter, read_parameter, PARAM_LEN);
	}
	return 0;
}

PHISON_TEST_PATTERN_API int SetAPParameter(unsigned char PhyDrive,unsigned char *APParam)
{
	DeviceHandle = UFWInterface::_static_MyCreateHandle(PhyDrive, true, false);
	unsigned char read_parameter[PARAM_LEN];
	memcpy(read_parameter, APParam, PARAM_LEN);
	NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, true, read_parameter);
	CheckDLLFuncEnd();
	return 0;
}

PHISON_TEST_PATTERN_API int GetAPParameter(unsigned char PhyDrive,unsigned char *APParam)
{
	DeviceHandle = UFWInterface::_static_MyCreateHandle(PhyDrive, true, false);
	unsigned char read_parameter[PARAM_LEN];
	memcpy(read_parameter, APParam, PARAM_LEN);
	NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, true, read_parameter);
	CheckDLLFuncEnd();
	return 0;
}

PHISON_TEST_PATTERN_API int UpdateAPParameter(unsigned char PhyDrive,int idx, BYTE ParamData)
{
	DeviceHandle = UFWInterface::_static_MyCreateHandle(PhyDrive, true, false);
	unsigned char read_parameter[PARAM_LEN];

	if (NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, false, read_parameter)) {
		PrintOutDebugString("GetParameter Command Fail");
		return ERR_SCSI;
	}

	read_parameter[idx] = ParamData;
	NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, true, read_parameter);
	CheckDLLFuncEnd();
	return 0;
}

PHISON_TEST_PATTERN_API int SetFlashID(unsigned char *FlashID)
{
	unsigned char buf[512] = { 0 };
	memset(buf, 0, 512);
	NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, buf, 1);
	UpdateParameterFromFlashInfo(buf);
	NTCmd->InitSQL();
	return 0;
}

int LoadBurner_E19(unsigned char PhyDrive) {
	int status = 0;
	int ssd_isp_status = -1;
	int bUFSCE_ = 0;
	unsigned char buf[528];
	memset(buf, 0, sizeof(buf));
	char fwversion[128];
	//char prompt[128];
	//char burner_name[128] = { 0 };
	int fileNameLength = 0;

	memset(fwversion, 0, sizeof(fwversion));
	bool isPCIE = false;

	NTCmd->NT_GetFlashExtendID(0, NULL, 0, buf, 1);//Only buf works
	mflash = MyFlashInfo::Instance("flashInfo.csv", NULL, NULL);
	memset(&_FH, 0x00, sizeof(_FH_Paramater));
	int over4ce_ = 0;
	if (0 == mflash->GetParameterMultipleCe(buf, &_FH, "", over4ce_, bUFSCE_))
	{
		PrintOutDebugString("flash id not in flashinfo.csv");
		return ERR_FLASHID;
	}

	NTCmd->m_flashfeature = FEATURE_T2; // default value.
	if (CTL_E13 == GetControllorType()) { // E19 = E13
		CString ccc;
		ssd_isp_status = 1;
		memset(fwversion, 0, 12);

		char burner[128] = { 0 };
		char rdt[128] = { 0 };
		//((PCIESCSI5013_ST *)NTCmd)->GetBurnerAndRdtFwFileName(&_FH, burner, rdt);
		memcpy(burner,"FW_E19.bin",10);
		if (!PathFileExistsA(burner))
		{
			PrintOutDebugString("auto select file %s not exist", burner);
			return ERR_LOADBURNER;
		}
		PrintOutDebugString("Burner: %s", burner);

		if (NTCmd->InitDrive(0, burner, 2)) {
			memset(fwversion, 0, sizeof(fwversion));
			NTCmd->GetFwVersion(fwversion);
			PrintOutDebugString("load burner done, fw version=%s\n", fwversion);
		}
		else {
			PrintOutDebugString("load burner fail");
			return ERR_LOADBURNER;
		}

		NTCmd->InitSQL();
		NTCmd->ForceSQL(true);
		memset(buf, 0, 512);
		//NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, buf, 1);
		//Read ID : 0x90
		int sqllen = 21;
		int mode = 2;
		int buflen = 512;
		int ce = 0;
		unsigned char sql[21] = {
			0xCA, 0x09, 0x01, 0xCF, 0x00, 0xC2, 0x90, 0xAB, 0x00,0xDB,0x08,0xCE, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0xAE, 0xAF
		};

		NTCmd->GetSQLCmd()->Dev_IssueIOCmd(sql, sqllen, mode, ce, buf, buflen);
		
		if (0 == buf[0] || 1 == buf[0])
		{
			PrintOutDebugString("Read ID fail after set ap parameter");
			return ERR_FLASHID;
		}

		return 0;
	}
	else {
		PrintOutDebugString("Not Support Controller");
		return ERR_DEVICE;
	}

	return 0;
}

int TestInit()
{
	UpdateRWMode();
	if ((_FH.bMaker == 0xAD) && ((_FH.id[0] == 0x3A) || (_FH.id[0] == 0x5A)))	//hynix 14nm
	{
		_FH.FlhCapacity = code2sizeHynix14(_FH.id[1]);
		if (_FH.FlhCapacity == 0)
			_FH.FlhCapacity = code2size(_FH.id[0], _FH.bMaker, _FH.id);
	}
	else if ((_FH.bMaker == 0xAD) && ((_FH.id[0] == 0x3C) || (_FH.id[0] == 0x5C)) && ((_FH.id[4] & 0x80) == 0x80))	//hynix 3D
	{
		_FH.FlhCapacity = code2sizeHynix3D(_FH.id[1]);
		if (_FH.FlhCapacity == 0)
			_FH.FlhCapacity = code2size(_FH.id[0], _FH.bMaker, _FH.id);
	}
	else
		_FH.FlhCapacity = code2size(_FH.id[0], _FH.bMaker, _FH.id);
	_FH.FlashSlot = 1;
	_FH.DieCapacity = _FH.FlhCapacity / _FH.DieNumber;
	_FH.FlhCapacity *= _FH.FlhNumber * _FH.bPBADie;
	return 0;
}

// convert flash code to sizes .....
unsigned int code2size(unsigned char flashCode, unsigned char MakerCode, unsigned char* tID)
{
	switch (flashCode)
	{
	case 0xe3:
	case 0xe5:
	case 0x6b:
		return 4;

	case 0xe6:
		return 8;

		// 20190515: 看 flashinfo 只有新的 Bics4 QLC 才有 0x73
		// 	case 0x73:
		// 		return 16;

	case 0x75:

		return 32;
	case 0x66:
	case 0x76:
	case 0x9e:
	case 0xf0:
		return 64;

	case 0x01:
	case 0x79:
	case 0xf1:
	case 0xa1:
	case 0xc1:
	case 0xd1:
		return 128;

	case 0xda:
	case 0xca:
	case 0x71:
		return 256;

	case 0xcc:
		if ((MakerCode == 0x2C))
			return 98304;
		else
			return 512;
	case 0xdc:
	case 0x29:
		return 512;

	case 0xd3:
	case 0xb3:				// sandisk
	case 0x38:
		return 1024;

	case 0xc5:
	case 0xd5:
	case 0xb5:// sandisk
		return 2048;

	case 0x48://20161228 Annie add for TSB 
		if ((MakerCode == 0x98) || (MakerCode == 0x45))
			return 131072;
		else
			return 2048;

	case 0x68:
	case 0xd7:
	case 0xc7:
	case 0xb7:				// sandisk
	case 0xe7:
	case 0x44:
	case 0x78:

		return 4096;
	case 0x47:
	case 0x37:
		if (tID[1] == 0x99)	// pMLC
			return 32768;
		else
			return 6144;
	case 0xce:
	case 0xde:
	case 0xd9:
	case 0x88:
	case 0xee:
	case 0x64:
	case 0xae:
	case 0x2d:
	case 0x4d:
	case 0x82:
		return 8192;

	case 0x49:
		if (MakerCode == 0x98 || MakerCode == 0x45)
			return 262144;
		else
			return 8192;


	case 0x39:
		return 12288;

	case 0x3a:
	case 0x4a:
	case 0xa8:
	case 0x4C:
	case 0x84:
	case 0x1a:
		return 16384;
	case 0x3B:
		return 24576;
	case 0x33:
	case 0x35:	//pMLC
	case 0x3C:
	case 0xa4:
	case 0xc8:
		return 32768;
	case 0xb4:
		return 49152;//B0KB
	case 0x3E:
	case 0x5E:
	case 0xC3:
	case 0xC4:
		return 65536;
	case 0xd4:
	case 0x73:	// Bics4 QLC: 170GB
		return 131072;
	case 0xe4:
		return 262144;
	default:
		//ASSERT(0);
		return 0;
	}

	return 0;
}

unsigned int code2sizeHynix14(unsigned char flashCode)
{
	switch (flashCode)
	{
	case 0x18:
		return 16384;

	case 0x19:
		return 32768;

	case 0x1A:
		return 65536;

	default:
		return 0;
	}

	return 0;
}

unsigned int code2sizeHynix3D(unsigned char flashCode)
{
	switch (flashCode)
	{
	case 0x28:
		return 32768;

	case 0x29:
		return 65536;

	case 0x2A:
		return 131072;

	case 0x2B:
		return 262144;

	default:
		return 0;
	}

	return 0;
}

int UpdateRWMode()
{
	RWMode = 0;
	int TestPlane = 1;
	int m_Conversion = 1;
	int m_CacheRead = 1;
	int m_ReadErase = 0;
	int m_CloseSeed = 0;
	int m_ReadCompare = 0;


	if (m_Conversion)
		RWMode = _CONVREV_;
	else
		RWMode = 0;

	if (m_CacheRead)
		RWMode |= _CACHE_READ_;
	else
		RWMode &= (~_CACHE_READ_);

	if (TestPlane == 1) //2plane
		RWMode |= _TWO_PLANE_MODE_;

	if (TestPlane == 2) //4plane
		RWMode |= _TWO_PLANE_MODE_ | _FOUR_PLANE_MODE_;

	if (m_ReadErase)
		RWMode |= _READ_ERASE_;
	else
		RWMode &= (~_READ_ERASE_);

	if (m_CloseSeed)
		RWMode |= _CLOSE_SEED_;
	else
		RWMode &= (~_CLOSE_SEED_);

	if (m_ReadCompare)
		RWMode |= _READ_COMPARE_;
	else
		RWMode &= (~_READ_COMPARE_);

	return RWMode;
}

int SetInitialAPParameter()
{
	memset(AP_Parameter, 0, PARAM_LEN);
	memset(Backup_AP_Parameter, 0, PARAM_LEN);
	BYTE fid[528] = { 0 };
	BYTE bUFSCE = 0;
	int pbadie = 0;
	int pbace = 0;
	_FH.bPBADie = 1;
	int CloseONFI = 0;

	NTCmd->NT_GetFlashExtendID(1, DeviceHandle, 0, fid, 1, 0, 1);
	_FH.bFunctionMode = 0x00;
	mflash = MyFlashInfo::Instance("flashInfo.csv", NULL, NULL);

	// flash id is updated here.
	if (!mflash->GetParameterMultipleCe(fid, &_FH, "", 0, bUFSCE))
	{
		PrintOutDebugString("Flash ID is not supported");
		return ERR_FLASHID;
	}
	UpdateMaxCeCountToFH(&_FH);
	int MixModeindex = 0;
	_FH.bIs8T2E = false;

	//ap parameter is set here.
	mflash->ParamToBuffer(&_FH, AP_Parameter, "test.ini", 0, bE2NAMD, _MP_PARAM_SPEC_, 0, NULL, 0);

	AP_Parameter[0x26] |= 1 << 6; //parameter 0x26 BIT6 for 2307 USB reconnect link
	AP_Parameter[0x5F] |= 1 << 3; //default enable show detial ecc bit for SSD & PCIE controllers

	//(A)20200702 : temporarily marked for E19
	//if ((SrtManage.GetLogicalCECnt() >= 4) && CTL_USB == GetControllorType()) {
	//	AP_Parameter[0x4a] = 0x31;
	//	AP_Parameter[0x4b] = 0x31;
	//	AP_Parameter[0x4C] = 0x00;
	//	AP_Parameter[0x4D] = 0x82;
	//}

	if (CTL_E13 == GetControllorType()) {
		if ('T' == _FH.bToggle) 
			AP_Parameter[0x4D] = 0x03;
		else 
			AP_Parameter[0x4D] = 0x02;
	}


	int EnterpriseRule = 0;
	AP_Parameter[0x26] |= 1 << 6; //for 2307 USB reconnect link, otherwise switch 2307 to S11 will failed.
	if (_FH.bIsMicron3DTLC) {
		if (_FH.bIsMIcronB0kB)
			AP_Parameter[0x28] |= 0x01; //b0kB
		else {
			if (_FH.bIsMicronB17)
				AP_Parameter[0x28] |= 0x04; //b17
			else if (_FH.bIsMicronB16)
				AP_Parameter[0x28] |= 0x02; //b16
			else if (_FH.bIsMIcronB05)
				AP_Parameter[0x28] |= 0x08; //b05
			else if (_FH.bIsMIcronN18)
				AP_Parameter[0x28] |= 0x10; //N18
			else if (_FH.bIsMIcronN28)
				AP_Parameter[0xDD] |= 0x01; //N28
			else if (_FH.bIsMicronB27) {
				if (_FH.id[3] == 0xA2)			// B27A
					AP_Parameter[0x28] |= 0x20;
				else if (_FH.id[3] == 0xE6)	// B27B
					AP_Parameter[0x28] |= 0x40;
			}
		}
		if (EnterpriseRule) {
			// 底層在 enterprise write/read 會不一樣
			AP_Parameter[0x28] |= 0x80;
		}

		if (NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, true, AP_Parameter)) {
			PrintOutDebugString("SetParameter Command Fail");
			return ERR_SCSI;
		}
	}

	AP_Parameter[0x14] &= ~0x02; //hv3 mode close edo
	AP_Parameter[0x23] |= 0x40;  // set 拉ce delay time
	memcpy(Backup_AP_Parameter, AP_Parameter, PARAM_LEN);

	if (_FH.beD3) {
		AP_Parameter[0x2f] = 0;//copyback dummy page conversion setting, 1 close, 0 open
		AP_Parameter[0x2e] = 0xff;//copyback dummy page pattern
		AP_Parameter[0x23] |= 0x01;//erase write , 01 use d1 erase, 00 use d3 erase (d1 block), d3 block always use d3 erase
									//parameter[0x2f] |=0x80; //bit7 設定1 的話會使用新的copy back 搬法   設定0 則為原本搬法
	}

	if (CloseONFI) AP_Parameter[0x00] &= (~0x20); //close onfi
	AP_Parameter[0x80] = AP_Parameter[0x81] = _FH.CEPerSlot;

	AP_Parameter[0xDF] = GetPrivateProfileInt("E13", "GlobleRandomSeed", 0, DEBUG_INI_FILE);
	AP_Parameter[0x20] = 0x01;
	if (NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, true, AP_Parameter)) {
		PrintOutDebugString("SetParameter Command Fail");
		return ERR_SCSI;
	}

	

	unsigned char read_parameter[1024];
	if (NTCmd->ST_SetGet_Parameter(1, DeviceHandle, 0, false, read_parameter)){
		PrintOutDebugString("GetParameter Command Fail");
		return ERR_SCSI;
	}

	memcpy(AP_Parameter, read_parameter, PARAM_LEN);
	TestInit();

	_FH.CEPerSlot = _FH.FlhNumber;
	_FH.MBC_ECC = 39;
	if (_FH.beD3) {
		_FH._512ByteTriggLen = 14;
	}
	else {
		_FH._512ByteTriggLen = 15;
	}

	_FH.LegacyDriving_4D = AP_Parameter[0x4D];
	_FH.LegacyReadClock_48 = AP_Parameter[0x48];
	_FH.LegacyWriteClock_49 = AP_Parameter[0x49];

	SetFeature(__SET_TOGGLE_MODE__);
	//SetFeature(__SET_LEGACY_MODE__);



	return 0;
}

void GetCurrentTimeString(char * timestring)
{
	CTime time(CTime::GetCurrentTime());   // Initialize CTime with current time
	int yy = time.GetYear();
	int MM = time.GetMonth();
	int dd = time.GetDay();
	int hh = time.GetHour();
	int mm = time.GetMinute();
	int ss = time.GetSecond();
	sprintf(timestring, "_%04d%02d%02d_%02d%02d%02d", yy, MM, dd, hh, mm, ss);
}

