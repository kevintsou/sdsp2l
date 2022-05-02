#include "USBSCSI5017_ST.h"
#include "IDPageAndCP_Hex_SSD.h"
#include "convertion_libs/ldpcecc.h"
#include <math.h>
#include "time.h"

#define B0517_BURNER_TSB        "B5017_TSB.bin"
#define B0517_BURNER_MICRON     "B5017_MICRON.bin"
#define B0517_BURNER_YMTC       "B5017_YMTC.bin"
#define B0517_BURNER_HYNIX      "B5017_HYNIX.bin"
#define B0517_BURNER_DEFAULT    "B5017.bin"

#define B5017_RDT_TSB           "B5017_FW_TSB.bin"
#define B5017_RDT_MICRON        "B5017_FW_MICRON.bin"
#define B5017_RDT_YMTC          "B5017_FW_YMTC.bin"
#define B5017_RDT_HYNIX         "B5017_FW_HYNIX.bin"
#define B5017_RDT_DEFAULT       "B5017_FW.bin"

#define USB_DATA_OUT          0
#define USB_DATA_IN           1
#define USB_DATA_UNSPECIFIED  2

#define USB_DIRCT_NON        0
#define USB_DIRCT_READ       1
#define USB_DIRCT_WRITE      2

#define TRANSFER_SIZE 65536
#define FUNTIONRECORD 0

#define OPCMD_COUNT_5017 8

static const unsigned char OpCmd5017[OPCMD_COUNT_5017] = 
{0x01, 0x14, 0x81, 0xAA, 0xAF, 0xBB, 0xBC, 0xCE};

int clock_idx_to_Mhz_5017[] = {20, 66, 80  /*<--legacy supported*/  , 200, 266, 333/*5*/, 400, 444, 533, 600, 677/*10*/, 700, 800, 933, 1200, 1400/*15*/}; //it is data rate base TOGGLE
USBSCSI5017_ST::USBSCSI5017_ST(DeviveInfo* info, const char* logFolder)
	:PCIESCSI5013_ST(info, logFolder)
{
	Init();
	_info = info;
}

USBSCSI5017_ST::USBSCSI5017_ST(DeviveInfo* info, FILE *logPtr)
	: PCIESCSI5013_ST(info, logPtr)
{
	Init();
	_info = info;
}

void USBSCSI5017_ST::Log(LogLevel level, const char* fmt, ...)
{
	char logbuf[1024];
	va_list arglist;
	va_start ( arglist, fmt );
	_vsnprintf ( logbuf, sizeof(logbuf), fmt, arglist );
	va_end ( arglist );
	UFWInterface::Log(level, "%s", logbuf);
}

void USBSCSI5017_ST::Init(void)
{
	pIdPageS = (unsigned short*)pIdPageC;
	pIdPageI = (unsigned int*)pIdPageC;
	pCPDataS = (unsigned short*)pCPDataC;
	pCPDataI = (unsigned int*)pCPDataC;
	gCPBuf = NULL;

	_user_select_clk_idx = 0;
	_user_fw_performance = 0;
	_user_bus_switch = 0;
	_user_form_factor = 0;
	_user_fw_advance = 0;
	_user_nes_switch = 0;
	_user_tag_switch = 0;
	_user_free_run = 0;
	_user_perFrame_dummy = 0;
	success_times = 0;
	memset(pIdPageC, 0, sizeof(pIdPageC));

}

USBSCSI5017_ST::~USBSCSI5017_ST(void)
{
	if (NULL != gCPBuf)
		delete[] gCPBuf;
	gCPBuf = NULL;
}

int USBSCSI5017_ST::GetDataRateByClockIdx(int clk_idx)
{
	return clock_idx_to_Mhz_5017[clk_idx];
}

int USBSCSI5017_ST::GetDefaultDataRateIdxForInterFace(int interFace)
{
	int user_specify_clk_idx = -1;
	if (0 == interFace) {
		user_specify_clk_idx = GetPrivateProfileInt("U17","LegacyClk",-1,_ini_file_name);
		if (-1 != user_specify_clk_idx) {
			return user_specify_clk_idx;
		} else { 
			return 2;//40Mhz
		}
	} else if (1 == interFace) {
		user_specify_clk_idx = GetPrivateProfileInt("U17","T1Clk",-1,_ini_file_name);
		if (-1 != user_specify_clk_idx) {
			return user_specify_clk_idx;
		} else {
			return 3;//200Mhz
		}
	} else if (2 == interFace) {
		user_specify_clk_idx = GetPrivateProfileInt("U17","T2Clk",-1,_ini_file_name);
		if (-1 != user_specify_clk_idx) {
			return user_specify_clk_idx;
		} else {
			return 5;//400Mhz
		}
	} else if (3 == interFace) {
		user_specify_clk_idx = GetPrivateProfileInt("U17","T3Clk",-1,_ini_file_name);
		if (-1 != user_specify_clk_idx) {
			return user_specify_clk_idx;
		} else {
			return 8;//800Mhz
		}
	} else if (4 == interFace) {
		return 14;//1200Mhz
	}
	else {
		assert(0);
	}
	return 0;
}

int USBSCSI5017_ST::ST_SetGet_Parameter(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BOOL bSetGet, unsigned char *buf)
{
	unsigned char cdb512[512] = { 0 };
	cdb512[0] = 0x06;
	cdb512[1] = 0xF6;
	cdb512[2] = 0xBB;

	if (bSetGet)
	{
		memcpy(m_apParameter, buf, 1024);

		if (GetPrivateProfileInt("Configuration","1v2",0,_ini_file_name))
		{
			int default_interface_1v2 = MyFlashInfo::GetFlashDefaultInterFace(12, _fh);
			if (default_interface_1v2 != 0)
			{
				m_apParameter[0] |= 1<<4;
			}
		}

		switch (m_flashfeature)
		{
		case FLASH_FEATURE::FEATURE_T3_1200:
			m_apParameter[0x4D] = GetDefaultDataRateIdxForInterFace(FEATURE_T3_1200);
			break;
		case FLASH_FEATURE::FEATURE_T3:
			m_apParameter[0x4D] = GetDefaultDataRateIdxForInterFace(FEATURE_T3);
			break;
		case FLASH_FEATURE::FEATURE_T2:
			m_apParameter[0x4D] = GetDefaultDataRateIdxForInterFace(FEATURE_T2);
			break;
		case FLASH_FEATURE::FEATURE_T1:
			m_apParameter[0x4D] = GetDefaultDataRateIdxForInterFace(FEATURE_T1);
			break;
		case FLASH_FEATURE::FEATURE_LEGACY:
			m_apParameter[0x4D] = GetDefaultDataRateIdxForInterFace(FEATURE_LEGACY);
			break;
		}
		int current_clk_idx = m_apParameter[0x4D];

		int tADL = -1, tWHR = -1, tWHR2 = -1;
		if (-1 == tADL && -1 == tWHR && -1 == tWHR2)
		{
			if (FEATURE_LEGACY == m_flashfeature) { //legacy 
				GetACTimingTable(GetDataRateByClockIdx(current_clk_idx), tADL, tWHR, tWHR2);
			}
			else { //toggle
				GetACTimingTable(GetDataRateByClockIdx(current_clk_idx) / 2, tADL, tWHR, tWHR2);
			}
		}
		m_apParameter[0xE4] = tADL;
		m_apParameter[0xE5] = tWHR;
		m_apParameter[0xE6] = tWHR2;

		if (GetPrivateProfileIntA ("Sorting","ChangeFlashMode", 0, _ini_file_name))
		{
			m_apParameter[0x5F] |= 0x10;
		}
		if (GetPrivateProfileIntA("Sorting","DumpErrorBits", 0, _ini_file_name)) 
		{
			m_apParameter[0x5F] |= 0x08;
		}

		GetODT_DriveSettingFromIni();
		/*m_apParameter[0xE0] = _flash_driving;
		m_apParameter[0xE1] = _flash_ODT;
		m_apParameter[0xE2] = _controller_driving;
		m_apParameter[0xE3] = _controller_ODT;*/

		Issue512CdbCmd(cdb512, m_apParameter, 1024, SCSI_IOCTL_DATA_OUT, _SCSI_DelayTime);
	}
	else
	{
		cdb512[3] = 1;
		Issue512CdbCmd(cdb512, buf, 1024, SCSI_IOCTL_DATA_IN, _SCSI_DelayTime);
	}

	return 0;//always return 0 for win10 issue
}

int USBSCSI5017_ST::Dev_GetFlashID(unsigned char* r_buf)
{
	return this->NT_GetFlashExtendID(1, this->Get_SCSI_Handle(), 0, r_buf, 0, 0, 0);
}

int USBSCSI5017_ST::GetBurnerAndRdtFwFileName(FH_Paramater * fh, char * burnerFileName, char * RdtFileName)
{
	if (fh) {
		if (fh->bIsYMTC3D) {
			sprintf_s(burnerFileName,64, "%s", B0517_BURNER_YMTC);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_YMTC);
		}
		else if (fh->bIsBiCs4QLC) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_TSB);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_TSB);
		}
		else if ((fh->bMaker == 0x98 || fh->bMaker == 0x45) && (fh->bIsBics4 || fh->bIsBics2 || fh->bIsBics3) && (fh->beD3)) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_TSB);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_TSB);
		}
		else if ((fh->bMaker == 0x98) && (fh->Design == 0x15) && (!fh->beD3)) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_DEFAULT);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_DEFAULT);
		}
		else if ((fh->bMaker == 0x45) && (fh->Design == 0x15) && (!fh->beD3)) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_DEFAULT);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_DEFAULT);
		}
		else if (fh->bIsMicronB16 || fh->bIsMicronB17) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_MICRON);
		}
		else if (fh->bIsMIcronN18) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_MICRON);
		}
		else if (fh->bIsMIcronN28) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_MICRON);
		}
		else if (fh->bIsHynix3DV4 || fh->bIsHynix3DV5 || fh->bIsHynix3DV6) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_HYNIX);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_HYNIX);
		}
		else if (fh->bIsMIcronB05) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_MICRON);
		}
		else if (fh->bIsMicronB27) {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_MICRON);
		}
		else {
			sprintf_s(burnerFileName, 64, "%s", B0517_BURNER_DEFAULT);
			sprintf_s(RdtFileName, 64, "%s", B5017_RDT_DEFAULT);
		}
	}
	return 0;
}

int USBSCSI5017_ST::Dev_SetEccMode(unsigned char ldpc_mode)
{
	//return 0;
	unsigned char ncmd[512] = { 0 };
	unsigned char buf[512] = { 0 };
	/*Confirm if LDPC bit will not to big*/
	/*The LDPC mode for loading FW code */
	//  Mode 0: 2048bit 256byte
	//	Mode 1: 1920bit 240byte
	//	Mode 2: 1792bit 224byte
	//	Mode 3: 1664bit 208byte
	//	Mode 4: 1408bit 176byte
	//	Mode 5: 1152bit 144byte
	//	Mode 6: 1024bit 128byte
	//	Mode 7: 2432bit 304byte
	//	Mode 8: 2304bit 228byte

	int u17_ldpc_bit[] = { 2048, 1920, 1792, 1664, 1408, 1152, 1024, 2432, 2304 };


	int frame_count = _fh->PageSize / 4096;

	int frame_size_with_parity_crc_ldpc = _fh->PageSize / frame_count;
	int remains_size = frame_size_with_parity_crc_ldpc - 4112;
	int max_ldpc_bytes = remains_size / 2;

	/*if (-1 == ldpc_mode) {
		Log(LOGDEBUG, "e13 auto select ldpc");
		return 0;
	}
	else if (e13_ldpc_bit[ldpc_mode] > max_ldpc_bytes * 8)
	{
		Log(LOGDEBUG, "the selected ldpc bit too big");
		//return -1;
	}*/

	ncmd[0] = 0xD1;  //Byte0
	ncmd[0x28] = 0x80;//B40
	ncmd[0x30] = 0x50;//B48
	ncmd[0x31] = ldpc_mode;//B47
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, 512, false, 20);
	return result;
}

int USBSCSI5017_ST::Dev_SetFWSeed(int page, int block)
{
	unsigned char ncmd[512] = { 0 };
	unsigned char buf[512] = { 0 };
	
	ncmd[0] = 0xD1;  //Byte0
	ncmd[0x28] = 0x80;  //B40
	ncmd[0x30] = 0x56;  //B48
	ncmd[0x31] = page & 0xFF;//B49
	ncmd[0x32] = (page >> 8) & 0xFF;//B50
	ncmd[0x33] = block & 0xFF;//B51
	ncmd[0x34] = (block >> 8) & 0xFF;//B52
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, 512, false, 20);
	return result;
}

void USBSCSI5017_ST::InitLdpc()
{
	if (_ldpc)
	{
		//assert(0);
	}
	else
	{
		_ldpc = new LdpcEcc(0x5017);
	}
}

bool USBSCSI5017_ST::InitDrive(unsigned char drivenum, const char* ispfile, int swithFW)
{
	char auto_select_rdt_file[256] = { 0 };
	char auto_select_burner_file[256] = { 0 };

	if (!ispfile)
	{
		GetBurnerAndRdtFwFileName(_fh, auto_select_burner_file, auto_select_rdt_file);
	}
	USB_SetAPKey();
	return Dev_InitDrive(ispfile ? ispfile : auto_select_burner_file)?true:false;
}
int USBSCSI5017_ST::Dev_ParameterInit(const char *inifile,unsigned char bE2NAND)
{
	_FH.bPBADie = 1;
	MyFlashInfo* inst = 0;
	inst = MyFlashInfo::Instance();
	inst->ParamToBuffer(&_FH, m_apParameter, "test.inifile", 0, bE2NAND, 0, 0, NULL, 0);
	/*if (-1 == _current_flash_interface) {
	if (CTL_E13 == GetControllorType()) {
	if ('T' == _FH.bToggle) {
	parameter[0x4D] = 0x03;
	}
	else {
	parameter[0x4D] = 0x02;
	}
	}
	}*/
	if (_FH.bIsMicron3DTLC) {
		if (_FH.bIsMIcronB0kB)
			m_apParameter[0x28] |= 0x01; //b0kB
		else {
			if (_FH.bIsMicronB17)
				m_apParameter[0x28] |= 0x04; //b17
			else if (_FH.bIsMicronB16)
				m_apParameter[0x28] |= 0x02; //b16
			else if (_FH.bIsMIcronB05)
				m_apParameter[0x28] |= 0x08; //b05
			else if (_FH.bIsMIcronN18)
				m_apParameter[0x28] |= 0x10; //N18
			else if (_FH.bIsMIcronN28)
				m_apParameter[0xDD] |= 0x01; //N28
			else if (_FH.bIsMicronB27) {
				if (_FH.id[3] == 0xA2)			// B27A
					m_apParameter[0x28] |= 0x20;
				else if (_FH.id[3] == 0xE6)	// B27B
					m_apParameter[0x28] |= 0x40;
			}
			else {
				//Log("No Define Micron3D");
			}
		}
	}
	m_apParameter[0x80] = m_apParameter[0x81] = _FH.CEPerSlot;
	/*if (NTCmd->ST_SetGet_Parameter(1, DeviceHandle, bMyDrv, true, parameter))
	return;*/
	return 0;
}

unsigned char * USBSCSI5017_ST::Dev_GetApParameter()
{
	return &m_apParameter[0];
}

#define _NO_BURNER_MODE_ 1
int USBSCSI5017_ST::Dev_InitDrive(const char* ispfile)
{
	//_iFF->BridgeReset();
	InitFlashInfoBuf(0,0, 0, 1);

//	char* fw;
//	unsigned char bufinq[4096];
	/*if (NVMe_Identify(bufinq, sizeof(bufinq)) != 0) {
		Log(LOGDEBUG, "Identify fail\n");
		return false;
	}
	fw = _fwVersion;
	Log(LOGDEBUG, "FW Version %s\n", fw);
	if (fw[0] == 'E' && fw[1] == 'D' && fw[2] == 'B') {
		_burnerMode = 1;
	}*/

	if (burner_mode== checkRunningMode())
	{
		return 1;
	}
	USB_SetAPKey();

	unsigned char buf[4096];
	memset(buf, 0, sizeof(buf));
	unsigned char *pBuf = &buf[0];
	if (USB_ISPProgPRAM(ispfile) != 0) {
		Log(LOGDEBUG, "### Err: ISP program fail\n");
		//cmd->MyCloseHandle(&_hUsb);
		return false;
	}
#if 1
	if (USB_ISPJumpCode(2))
	{
		Log(LOGDEBUG, "Load burner pass but jump ISP fail..");
		_burnerMode = _NO_BURNER_MODE_;
		return 0;
	}
	running_mode = checkRunningMode();
	if (running_mode != burner_mode)
	{
		Log(LOGDEBUG, "Load burner pass but jump ISP fail..");
		return 0;
	}
#endif
	return true;
}

int USBSCSI5017_ST::USB_ISPProgPRAM(const char* m_MPBINCode)
{
	//Log(LOGDEBUG, "%s %s\n",__FUNCTION__, m_MPBINCode );

	//bool Scrambled=true;
	bool Scrambled = false;
	bool with_jump = true;

	//QString message,FWName;
	ULONG filesize;
	unsigned char *m_srcbuf;
	unsigned int package_num, remainder,  ubCount, loop;
	bool status, is_lasted;
	//double timeout;
	//clock_t begin;

	//begin = clock();
	errno_t err;
	FILE * fp;
	err = fopen_s(&fp,m_MPBINCode, "rb");
	if (!fp)
	{
		//Err_str = "S01";
		Log(LOGDEBUG, "Can not open MP BIN File: %s successfully \n!", m_MPBINCode);
		return 1;
	}

	fseek(fp, 0L, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);


	if (filesize == -1L)
	{
		//Err_str = "S01";  //Bin file have problem
		Log(LOGDEBUG, "Failed ! Check FW BIN File size(%s) = 0 \n", m_MPBINCode);
		fclose(fp);
		return 1;
	}

	m_srcbuf = (unsigned char *)malloc(filesize);
	if (m_srcbuf) {
		memset(m_srcbuf, 0x00, filesize);

		if (fread(m_srcbuf, 1, filesize, fp) <= 0)
			//if ( _read(fh,m_srcbuf,filesize) <= 0 )  //Bin file have problem
		{
			//Err_str = "S01";
			Log(LOGDEBUG, "Reading MP BIN File Failed !!!\n");
			fclose(fp);
			free(m_srcbuf);
			return 1;
		}
	}
	fclose(fp);

	ubCount = 128;  //Nvme Cmd. Max Data Length: 512K
	int ISPFW_SingleBin_NoHeader = true;
	if (ISPFW_SingleBin_NoHeader)
	{
		/*if ((filesize % TRANSFER_SIZE))
		{
			Log(LOGDEBUG, "BIN File size must be multiple of 512 bytes !!!\n");
			free(m_srcbuf);
			return 1;
		}*/

		package_num = filesize / TRANSFER_SIZE;
		remainder = filesize % TRANSFER_SIZE;
		/*if (remainder)
		{
			package_num +=1;
		}*/ 

		//quotient = sector_no / ubCount;
		//quotient = sector_no + 1;
		Log(LogLevel::LOGDEBUG, "ISP Program Burner:  (Bin No Header)\n");
		Log(LogLevel::LOGDEBUG, "Burner Code Size= %d bytes. loop=%d, remin.=%d\n", filesize, package_num, remainder);
	}
	else
	{

	}

	for (loop = 0; loop<package_num; loop++)
	{
		is_lasted = false;
		if (remainder == 0)
		{
			if (loop == (package_num - 1))
				is_lasted = true;
		}

		if (ISPFW_SingleBin_NoHeader)
			//status = USB_ISPProgPRAM(m_srcbuf + (loop*ubCount * 512), ubCount * 512, Scrambled, is_lasted, loop) == 0;
		{
			status = USB_ISPProgPRAM(m_srcbuf + (loop * TRANSFER_SIZE), TRANSFER_SIZE, Scrambled, is_lasted, loop)?true:false;
		}
		else {
			//status=NVMe_ISPProgramPRAM(m_srcbuf+m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+loop*ubCount)*512,ubCount*512,Scrambled,is_lasted,loop)==0;
		}
		if (status != 0)
		{
			//Err_str = "I_x03";
			Log(LOGDEBUG, "ISP Program Burner Code Failed_loop:%d !!\n", loop);
			free(m_srcbuf);
			return 1;
		}
		Log(LOGDEBUG, "%d done\n", loop);
	}

	if (remainder)
	{
		if (ISPFW_SingleBin_NoHeader)
			//status = USB_ISPProgPRAM(m_srcbuf + (quotient*ubCount * 512), remainder * 512, Scrambled, true, loop) == 0;
			status = USB_ISPProgPRAM(m_srcbuf + (loop * TRANSFER_SIZE), remainder, Scrambled, true, package_num)?true:false;
		else
		{
			//status=NVMe_ISPProgramPRAM(m_srcbuf+(m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+quotient*ubCount)*512),remainder*512,Scrambled,true,loop)==0;
		}


		if (status != 0)
		{
			//Err_str = "I_x03";
			Log(LOGDEBUG, "ISP Program Burner Code Failed_lasted !!\n");
			free(m_srcbuf);
			return 1;
		}
		Log(LOGDEBUG, "last part done\n");
	}
	Log(LOGDEBUG, "ISP Prog Burner Code OK.\n");
	free(m_srcbuf);

	Sleep(500);
	return 0;
}

int USBSCSI5017_ST::NT_GetFlashExtendID(BYTE bType, HANDLE MyDeviceHandle, char targetDisk, unsigned char *buf, bool bGetUID, BYTE full_ce_info, BYTE get_ori_id)
{
	errno_t err;
	FILE * fp;
	err = fopen_s(&fp,"FAKE_FLASH_ID.bin", "rb");
	if (fp)
	{
		fread(buf, 1, 512, fp);
		fclose(fp);
		return 0;
	}
	USB_SetAPKey();
	int ret = 0;
	checkRunningMode();
	if (running_mode == boot_mode || running_mode == fw_mode || running_mode == rdt_mode) {

		unsigned char ncmd[512] = { 0 };
		ncmd[0] = 0xD2;  //Byte48 //Feature

		ncmd[0x28] = 0x80;  //B40
		ncmd[0x30] = 0x90;  //B48
		ncmd[0x31] = 0x00;  //B49 //00
		ncmd[0x3B] = 0x08;  //B59
#if FUNTIONRECORD
		Log(LOGDEBUG, "*****************");
		Log(LOGDEBUG, __func__);
		UFWLoger::LogMEM(ncmd, 512);
		Log(LOGDEBUG, "*****************");
#endif
		ret = Nvme3PassVenderCmd(ncmd, buf, 512, true, 20);
		unsigned char buf2[512] = { 0 };
		for (int ce = 0; ce < 32; ce++) {
			memcpy(buf2 + (16 * ce), buf + (8 * ce), 8);
		}
		memcpy(buf, buf2, 512);
		if (0 == buf[0] && 0 == buf[1] && 0 ==buf[2]) {
			memcpy(&buf[0], &buf[16], 16);
			memset(&buf[16], 0, 16);
		}
		if (0 == buf[0] && 0 == buf[1] && 0 == buf[2]) {
			if (0 != buf[0x20] && 0 != buf[0x21] && 0 != buf[0x22]) {
				memcpy(&buf[0], &buf[0x20], 16);
			}
		}
	}
	else {
		//In Burner
		unsigned char cdb512[512] = { 0 };
		cdb512[0] = 0x06;
		cdb512[1] = 0x56;
		ret = Issue512CdbCmd(cdb512, buf, 512, SCSI_IOCTL_DATA_IN, _SCSI_DelayTime);
	}
	return ret;
}

int USBSCSI5017_ST::USB_Identify(unsigned char *buf, int timeout)
{
	unsigned int bufLength = 512;
	UCHAR    cdb[16] = { 0 };
	cdb[0] = 0x06;                 // SECURITY PROTOCOL Out
	cdb[1] = 0xFE;                 // security_protocol
	cdb[2] = 0x05;                 // security_protocol_speciric & 0xFF;
	//cdb[3] = (((bufLength >> 9) >> 8) & 0xFF);
	//cdb[4] = ((bufLength >> 9) & 0xFF);
	cdb[4] = 0x08;
	//cdb[5] = ((bufLength >> 24) & 0xFF);
	//cdb[6] = ((bufLength >> 16) & 0xFF);
	//cdb[7] = ((bufLength >> 8) & 0xFF);
	//cdb[8] = ((bufLength) & 0xFF);
	cdb[7] = 0x20;
	cdb[8] = 0x00;
	int result = MyIssueCmd(cdb, true, buf, bufLength, timeout, false);
	return result;
}

int USBSCSI5017_ST::Trim_Efuse(bool isGet, unsigned char * buf, int bank)
{
	unsigned char cdb512[512] = {0};
	cdb512[0] = 0x06;
	cdb512[1] = 0xF6;
	cdb512[2] = 0x41;
	cdb512[3] = (isGet?1:0);
	cdb512[3] |= 0x02;

	cdb512[4] = bank;

	if (isGet)
	{
		Issue512CdbCmd(cdb512, buf, 512, SCSI_IOCTL_DATA_IN, 15);
	}
	else
	{
		Issue512CdbCmd(cdb512, buf, 512, SCSI_IOCTL_DATA_OUT, 15);
	}
	return 0;
}

int USBSCSI5017_ST::USB_SetAPKey()
{
	unsigned char ncmd[512] = {0};

	ncmd[0] = 0xd0; 

	ncmd[0x34] = 0x6F;  //B52    //SC
	ncmd[0x35] = 0xFE;  //B53    //SN
	ncmd[0x36] = 0xEF;  //B54    //CL
	ncmd[0x37] = 0xfa;  //B55    //CH
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, NULL, 0, false, 20);
	return result;
}

int USBSCSI5017_ST::NT_GetDriveCapacity(BYTE bType, HANDLE MyDeviceHandle, char targetDisk, unsigned char *buf)
{
	nt_log("%s", __FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	if (buf == NULL)	return 1;

	memset(buf, 0, 12);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x25;
	iResult = MyIssueCmd(CDB, true, buf, 512, 20);
	return iResult;
}

int USBSCSI5017_ST::NT_GetDriveCapacity_4T(BYTE bType, HANDLE MyDeviceHandle, char targetDisk, unsigned char *buf)
{
	nt_log("%s", __FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	if (buf == NULL)	return 1;

	memset(buf, 0, 12);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x9e;
	CDB[1] = 0x10;
	CDB[13] = 0x0c;
	iResult = MyIssueCmd(CDB, true, buf, 512, 20);
	return iResult;
}

int USBSCSI5017_ST::NT_trim(BYTE bType, HANDLE MyDeviceHandle, char targetDisk,int len, unsigned char *buf)
{
	nt_log("%s", __FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	if (buf == NULL)	return 1;

	memset(buf, 0, 12);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x42;
	//CDB[1] = 0x00;
	CDB[8] = len;
	
	iResult = MyIssueCmd(CDB, true, buf, len, 20);
	return iResult;
}

int USBSCSI5017_ST::USB_ISPProgPRAM(unsigned char* buf, unsigned int BuferSize, bool mScrambled, bool mLast, unsigned int m_Segment)
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD1;  //Byte0
	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43
	ncmd[0x30] = 0x12;  //B48
	ncmd[0x31] = mLast << 2;//B49
	ncmd[0x36] = m_Segment;//B54
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, false, 20);
	return result;
}

int USBSCSI5017_ST::USB_ISPJumpCode(int subFeature)
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xd0;
	ncmd[0x30] = 0x02;  //B48    
	ncmd[0x31] = subFeature;  //B49 
	//0: jump to bootcode and enable load code, 1: jump to bootcode and disable load code, 2: jump to PRAM
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, NULL, 0, false, 20);
	Sleep(100);
	//Inquiry get running mode
	return result;
}

int USBSCSI5017_ST::USB_ISPProgPRAM_ST(const char* m_MPBINCode, int type, int portinfo)
{

	//Log(LOGDEBUG, "%s %s\n",__FUNCTION__, m_MPBINCode );

	//bool Scrambled=true;
	bool Scrambled = false;
	bool with_jump = true;

	//QString message,FWName;
	ULONG filesize;
	unsigned char *m_srcbuf, *m_srcbuf2;
	unsigned int package_num, remainder, ubCount, loop;
	bool status, is_lasted;
	unsigned char IDPBuf[4096];
	int total_size_len = 0;
	//double timeout;
	//clock_t begin;

	//begin = clock();
	char file_name[128] = { 0 };

	if (type == 1) {
		PrepareIDpage(IDPBuf, 2, 5, 0);
	}

	errno_t err;
	FILE * fp;
	err = fopen_s(&fp, m_MPBINCode, "rb");
	if (!fp)
	{
		//Err_str = "S01";
		Log(LOGDEBUG, "Can not open MP BIN File: %s successfully \n!", m_MPBINCode);
		return 1;
	}

	fseek(fp, 0L, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);


	if (filesize == -1L)
	{
		//Err_str = "S01";  //Bin file have problem
		Log(LOGDEBUG, "Failed ! Check FW BIN File size(%s) = 0 \n", m_MPBINCode);
		fclose(fp);
		return 1;
	}

	//////////////////
	total_size_len = sizeof(IDPage) + filesize;
	m_srcbuf = (unsigned char *)malloc(total_size_len);
	memset(m_srcbuf, 0x00, total_size_len);
	IDPage[352] = portinfo;	//for device U17 mark mux no
	memcpy(m_srcbuf, IDPage, sizeof(IDPage));
	m_srcbuf2 = (unsigned char *)malloc(filesize);
	memset(m_srcbuf2, 0x00, filesize);

	if (fread(m_srcbuf2, 1, filesize, fp) <= 0)
		//if ( _read(fh,m_srcbuf,filesize) <= 0 )  //Bin file have problem
	{
		//Err_str = "S01";
		Log(LOGDEBUG, "Reading MP BIN File Failed !!!\n");
		fclose(fp);
		free(m_srcbuf2);
		return 1;
	}
	fclose(fp);
	memcpy(&m_srcbuf[4096], m_srcbuf2, filesize);


	sprintf(file_name, "%s", "U17_ID_PAGE_DUMP.bin");//IDPAGE
	err = fopen_s(&fp, file_name, "wb");
	fwrite(m_srcbuf, 1, total_size_len, fp);
	fclose(fp);
	////////////////////////


	ubCount = 128;  //Nvme Cmd. Max Data Length: 512K
	int ISPFW_SingleBin_NoHeader = true;
	if (ISPFW_SingleBin_NoHeader)
	{
		/*if ((filesize % TRANSFER_SIZE))
		{
		Log(LOGDEBUG, "BIN File size must be multiple of 512 bytes !!!\n");
		free(m_srcbuf);
		return 1;
		}*/

		package_num = filesize / TRANSFER_SIZE;
		remainder = filesize % TRANSFER_SIZE;
		/*if (remainder)
		{
		package_num +=1;
		}*/

		//quotient = sector_no / ubCount;
		//quotient = sector_no + 1;
		Log(LOGDEBUG, "ISP Program Burner:  (Bin No Header)\n");
		Log(LOGDEBUG, "Burner Code Size= %d bytes. loop=%d, remin.=%d\n", filesize, package_num, remainder);
	}
	else
	{

	}

	for (loop = 0; loop<package_num; loop++)
	{
		is_lasted = false;
		if (remainder == 0)
		{
			if (loop == (package_num - 1))
				is_lasted = true;
		}

		if (ISPFW_SingleBin_NoHeader)
			//status = USB_ISPProgPRAM(m_srcbuf + (loop*ubCount * 512), ubCount * 512, Scrambled, is_lasted, loop) == 0;
		{
			status = USB_ISPProgPRAM_ST(m_srcbuf + (loop * TRANSFER_SIZE), TRANSFER_SIZE, Scrambled, is_lasted, loop, type) ? true : false, type;
		}
		else {
			//status=NVMe_ISPProgramPRAM(m_srcbuf+m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+loop*ubCount)*512,ubCount*512,Scrambled,is_lasted,loop)==0;
		}
		if (status != 0)
		{
			//Err_str = "I_x03";
			Log(LOGDEBUG, "ISP Program Burner Code Failed_loop:%d !!\n", loop);
			free(m_srcbuf);
			return 1;
		}
		Log(LOGDEBUG, "%d done\n", loop);
	}

	if (remainder)
	{
		if (ISPFW_SingleBin_NoHeader)
			//status = USB_ISPProgPRAM(m_srcbuf + (quotient*ubCount * 512), remainder * 512, Scrambled, true, loop) == 0;
			status = USB_ISPProgPRAM_ST(m_srcbuf + (loop * TRANSFER_SIZE), remainder, Scrambled, true, package_num, type) ? true : false;
		else
		{
			//status=NVMe_ISPProgramPRAM(m_srcbuf+(m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+quotient*ubCount)*512),remainder*512,Scrambled,true,loop)==0;
		}


		if (status != 0)
		{
			//Err_str = "I_x03";
			Log(LOGDEBUG, "ISP Program Burner Code Failed_lasted !!\n");
			free(m_srcbuf);
			return 1;
		}
		Log(LOGDEBUG, "last part done\n");
	}
	Log(LOGDEBUG, "ISP Prog Burner Code OK.\n");
	free(m_srcbuf);

	//Sleep(500);
	return 0;
}

int USBSCSI5017_ST::USB_ISPProgPRAM_ST(unsigned char* buf, unsigned int BuferSize, bool mScrambled, bool mLast, unsigned int m_Segment, int type)
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD1;  //Byte0
	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43
	ncmd[0x30] = 0x12;  //B48
	ncmd[0x31] = mLast << 2;//B49
	ncmd[0x32] = type;		//B50
	ncmd[0x36] = m_Segment;//B54
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, false, 20);
	return result;
}

int USBSCSI5017_ST::BuildNvmeVendor(unsigned char * nvme64, int dataLenBytes, int directions)
{
	memset(nvme64, 0, 512);
	nvme64[0x30] = 0x66;  //B48, vendor key

	if (SCSI_IOCTL_DATA_UNSPECIFIED == directions)
	{
		nvme64[0] = 0xD0;
	}
	else if (SCSI_IOCTL_DATA_OUT == directions)
	{
		nvme64[0] = 0xD1;
	}
	else if (SCSI_IOCTL_DATA_IN == directions)
	{
		nvme64[0] = 0xD2;
	}
	else
	{
		assert(0);
	}

	int dataLenDwords = dataLenBytes/4;

	nvme64[0x28] = dataLenDwords & 0xFF; //B40
	nvme64[0x29] = (dataLenDwords >> 8) & 0xFF; //B41
	nvme64[0x2A] = (dataLenDwords >> 16) & 0xFF;  //B42	 
	nvme64[0x2B] = (dataLenDwords >> 24) & 0xFF;  //B43

	nvme64[0x34] = (dataLenBytes & 0xff);//B52
	nvme64[0x35] = ((dataLenBytes >> 8) & 0xff);
	nvme64[0x36] = ((dataLenBytes >> 16) & 0xff);
	nvme64[0x37] = ((dataLenBytes >> 24) & 0xff);

	return 0;

}

int USBSCSI5017_ST::Issue512CdbCmd(unsigned char * cdb512, unsigned char * iobuf, int length, int direction, int timeout)
{
	unsigned char nvme64[512] = {0};
	int total_data_len_send_bytes = 512 + length;

	//USB_SetAPKey();
	int rtn = 0;
	
	nvme64[0x30] = 0x66;  //B48, vendor key

	if (SCSI_IOCTL_DATA_IN == direction && 0 ==length )
	{
		direction = SCSI_IOCTL_DATA_UNSPECIFIED;
	}

	if (SCSI_IOCTL_DATA_UNSPECIFIED == direction)
	{
		total_data_len_send_bytes = 512;
		BuildNvmeVendor(nvme64, total_data_len_send_bytes, SCSI_IOCTL_DATA_OUT);
		rtn += Nvme3PassVenderCmd(nvme64, cdb512, 512, SCSI_IOCTL_DATA_OUT,  timeout); 
	}
	else if (SCSI_IOCTL_DATA_OUT == direction)
	{
		unsigned char * outbuf = new unsigned char [total_data_len_send_bytes];
		memset(outbuf, 0, total_data_len_send_bytes);
		memcpy(outbuf, cdb512, 512);
		memcpy(outbuf+512, iobuf, total_data_len_send_bytes-512);

		BuildNvmeVendor(nvme64, total_data_len_send_bytes, SCSI_IOCTL_DATA_OUT);
		rtn += Nvme3PassVenderCmd(nvme64, outbuf, total_data_len_send_bytes, SCSI_IOCTL_DATA_OUT,  timeout); 
		if (outbuf)
		{
			delete [] outbuf;
		}
		
	}
	else if (SCSI_IOCTL_DATA_IN == direction)
	{
		//send CDB buffer first
		total_data_len_send_bytes = 512;
		BuildNvmeVendor(nvme64, total_data_len_send_bytes, SCSI_IOCTL_DATA_OUT);
		rtn += Nvme3PassVenderCmd(nvme64, cdb512, 512, SCSI_IOCTL_DATA_OUT,  timeout); 

		//read result
		BuildNvmeVendor(nvme64, length, SCSI_IOCTL_DATA_IN);
		rtn += Nvme3PassVenderCmd(nvme64, iobuf, length, SCSI_IOCTL_DATA_IN,  timeout);
	}
	else
	{
		assert(0);
	}
	if (rtn)
		rtn = -2;
	return rtn;
}

int USBSCSI5017_ST::USB_Set_Parameter(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char *buf)
{
	const int Length = 1024;

	unsigned char cdb512[512] = { 0 };
	cdb512[0] = 0x06;
	cdb512[1] = 0xF6;
	cdb512[2] = 0xBB;

	unsigned char finalBuf[Length] = { 0 };
	memcpy(finalBuf, cdb512, 512);
	memcpy(&finalBuf[512], buf, 512);
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xd1;
	ncmd[0x30] = 0x66;  //B48

	unsigned int temp = Length / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43

	ncmd[0x34] = (Length & 0xff);//B52
	ncmd[0x35] = ((Length >> 8) & 0xff);
	ncmd[0x36] = ((Length >> 16) & 0xff);
	ncmd[0x37] = ((Length >> 24) & 0xff);
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, finalBuf, 1024, false, 20);
	return result;
}

int USBSCSI5017_ST::USB_ISPEraseAll()
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xd0;
	ncmd[0x30] = 0x08;  //B48
	ncmd[0x31] = 0x01;  //B49
	  

	int result = Nvme3PassVenderCmd(ncmd, NULL, 0, false, 20);
	return result;
}

int USBSCSI5017_ST::Dev_SetP4K(unsigned char * buf, int Length, bool isWrite)
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD1;  //Byte0
	unsigned int temp = Length / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43
	ncmd[0x30] = 0x58;  //B48
	if (isWrite)
		ncmd[0x31] = 0x00;//B49
	else
		ncmd[0x31] = 0x01;//B49

	ncmd[0x34] = (Length & 0xff);//B52
	ncmd[0x35] = ((Length >> 8) & 0xff);
	ncmd[0x36] = ((Length >> 16) & 0xff);
	ncmd[0x37] = ((Length >> 24) & 0xff);
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, Length, false, 20);
	return result;
}

int USBSCSI5017_ST::ST_u17ScanSpare(CNTCommand * inputCmd, int ce, int codeBlock,unsigned char *buf)
{
	int pageIdx = 0;
	const int MAX_FRAME_CNT = 4;
	const int L4K_SIZE_PER_FRAME = 32;
	const int L4K_BUF_SIZE_WR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT * 2;
	int L4K_WRITE_START_ADDR = 0;
	int L4K_READ_START_ADDR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT;
	unsigned char P4K_WR[L4K_BUF_SIZE_WR] = { 0 };
	int page_size_alignK = ((_fh->PageSize + 1023) / 1024) * 1024;
	unsigned char tmpbuf[512] = { 0 }, buf512[512] = {0};
	CharMemMgr pbv(page_size_alignK);
	unsigned char * page_buf_verify = pbv.Data();// new unsigned char[page_size_alignK];

	SQLCmd * sqlCmd = inputCmd->GetSQLCmd();

	int res = 0;

	res |= Dev_SetFWSeed(pageIdx % 256, 0x8299);
	res |= Dev_SetEccMode(7);
	res |= USB_SetSnapWR(7);
	Dev_GenSpare_32(P4K_WR, 'I', 1, 0, L4K_BUF_SIZE_WR);
	res |= Dev_SetP4K(&P4K_WR[L4K_READ_START_ADDR], 32 * 4, false);

	//sqlCmd->GroupReadEccD1(ce, codeBlock, pageIdx, 1, buf512, 4096, 1, 0, 0);
	res |= sqlCmd->GroupReadDataD1(ce, codeBlock, pageIdx, 1, page_buf_verify, 4096, 1, false, false);

	res |= _ReadDDR(tmpbuf, 64, 5);
	memcpy(buf, tmpbuf, 512);
	//if (tmpbuf[0x04] == 0x12 && tmpbuf[0x05] == 0xCF) //IDPage
	//{
	//	return 1;
	//}
	//else if (tmpbuf[0x00] == 0xB0 && tmpbuf[0x01] == 0xAA && tmpbuf[0x02] == 0xAA && tmpbuf[0x03] == 0xDF) //DBT
	//{
	//	return 2;
	//}
	return res;
}


int USBSCSI5017_ST::WriteRDTCode(char * fw_file_name, char * output_infopage_filename, int ce, int codeBlock, CNTCommand * inputCmd, int writeMethod,
	unsigned char * input_ap_parameter, unsigned char * input_4k_buf, char * ap_version, int userSpecifyBlkCnt, int clk,
	int flh_interface, BOOL isEnableRetry, unsigned char * d1_retry_buffer, unsigned char * retry_buffer,
	int normal_read_ecc_threshold_d1, int retry_read_ecc_threshold_d1,
	int normal_read_ecc_threshold, int retry_read_ecc_threshold, L3_BLOCKS_ALL * L3_blocks, int times, bool isSQL_groupRW, unsigned char * scanwindowOutbuf, 
	unsigned char * HTRefbuf, int HTRefLength, bool isWrittingCP, unsigned char * odtSettingbuf)
{
	int rtn = 0;
	int file_size = 0, offset = 0;

	char rdtFileNameRuled[128] = { 0 };
	char burnerFileNameRuled[128] = { 0 };

	unsigned short blk[8], TPgCnt, SectionMaxSecCnt = 0;
	unsigned char IDPBuf[4096], CPBuf[4096], section = 0, FrameNo, WriteSec;//test
	unsigned int TmpCRC;
	inputCmd->InitSQL();
	inputCmd->ForceSQL(true);
	SQLCmd * sqlCmd = inputCmd->GetSQLCmd();

	const int MAX_FRAME_CNT = 4;
	const int L4K_SIZE_PER_FRAME = 32;
	const int L4K_BUF_SIZE_WR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT * 2;
	const int L4K_WRITE_START_ADDR = 0;
	const int L4K_READ_START_ADDR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT;

	unsigned char P4K_WR[L4K_BUF_SIZE_WR] = { 0 };
	bool isOnly4Kdata = false;

	memset(blk, 0xff, sizeof(blk));
	errno_t err;
	FILE * fp = NULL;
	if (!fw_file_name)
	{
		GetBurnerAndRdtFwFileName(_fh, burnerFileNameRuled, rdtFileNameRuled);
		err =fopen_s(&fp,rdtFileNameRuled, "rb");
		Log(LOGDEBUG, "U17 FW file name=%s\n", rdtFileNameRuled);
	}
	else
	{
		err = fopen_s(&fp, fw_file_name, "rb");
		Log(LOGDEBUG, "U17 FW file name=%s\n", fw_file_name);
	}

	if (!fp)
		return 1;

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char * file_buf = new unsigned char[file_size];
	fread(file_buf, 1, file_size, fp);
	fclose(fp);

	ParsingBinFile(file_buf);
	blk[0] = codeBlock;
	blk[1] = codeBlock + 1;
	InitialCodePointer(CPBuf, blk);
	if (!isWrittingCP) {
		if (gCPBuf == NULL) {
			gCPBuf = new unsigned char[4096];
			memcpy(gCPBuf, CPBuf, 4096);
		}
	}
	PrepareIDpage(IDPBuf, flh_interface, clk, userSpecifyBlkCnt);

	if (times != 0) {
		IDPBuf[0x02] = 0x13;
	}

	//if (0 != memcmp(&input_4k_buf[0x2D6], "INFOBLOCK", 9))
	//{
	//	nt_file_log("%s: info block mark check error!", __FUNCTION__);
	//	return 1;
	//}

	int page_size_alignK = ((_fh->PageSize + 1023) / 1024) * 1024;
	unsigned char * page_buf = new unsigned char[page_size_alignK];
	unsigned char * page_buf_verify = new unsigned char[page_size_alignK];
	CharMemMgr Pgf(page_size_alignK);
	unsigned char * pageToFile = Pgf.Data();// new unsigned char[page_size_alignK];

#if  1
	int HTRefPageCount = 0;
	if (HTRefLength != 0) {
		if (HTRefLength % (_fh->PageSizeK * 1024))
			HTRefPageCount = HTRefLength / (_fh->PageSizeK * 1024) + 1;
		else
			HTRefPageCount = HTRefLength % (_fh->PageSizeK * 1024);
		IDPBuf[0xD8] = IDPBuf[0x70] + 1;
		IDPBuf[0xD9] = 0x00;
		IDPBuf[0xDA] = HTRefPageCount;
		IDPBuf[0x24] += HTRefPageCount;
	}
#endif

	input_4k_buf[INFOPAGE_PAGE_CNT_PER_RECORD] = _fh->DieNumber + 2;
	input_4k_buf[INFOBLOCK_ENABLE_RETRY] = isEnableRetry;
	input_4k_buf[INFOBLOCK_RETRY_FROM] = 1;//retry table from app
	input_4k_buf[INFOBLOCK_ECC_THRESHOLD_D1] = normal_read_ecc_threshold_d1;
	input_4k_buf[INFOBLOCK_ECC_THRESHOLD_RR_D1] = retry_read_ecc_threshold_d1;
	input_4k_buf[INFOBLOCK_ECC_THRESHOLD] = normal_read_ecc_threshold;
	input_4k_buf[INFOBLOCK_ECC_THRESHOLD_RR] = retry_read_ecc_threshold;

	input_4k_buf[INFOBLOCK_RETRY_GROUPS_CNT_D1] = d1_retry_buffer[0];
	input_4k_buf[INFOBLOCK_RETRY_LEN_PER_GROUP_D1] = d1_retry_buffer[1];
	input_4k_buf[INFOBLOCK_RETRY_GROUPS_CNT] = retry_buffer[0];
	input_4k_buf[INFOBLOCK_RETRY_LEN_PER_GROUP] = retry_buffer[1];
	int retry_table_size_bytes_d1 = d1_retry_buffer[0] * d1_retry_buffer[1];
	if (retry_table_size_bytes_d1>RETRYTABLE_MAX_LENGTH_D1)
	{
		//assert(0);
		retry_table_size_bytes_d1 = RETRYTABLE_MAX_LENGTH_D1;
	}

	int retry_table_size_bytes = retry_buffer[0] * retry_buffer[1];
	if (retry_table_size_bytes>RETRYTABLE_MAX_LENGTH)
	{
		//assert(0);
		retry_table_size_bytes = RETRYTABLE_MAX_LENGTH;
	}

	memcpy(&input_4k_buf[INFOBLOCK_RETRYTABLE_D1], d1_retry_buffer + 2, retry_table_size_bytes_d1);
	memcpy(&input_4k_buf[INFOBLOCK_RETRYTABLE], retry_buffer + 2, retry_table_size_bytes);

	input_4k_buf[INFOPAGE_CODEBODY_LDPC_MODE] = _RDTCodeBodyLDPCMode;
	memcpy(&input_4k_buf[INFOBLOCK_AP_VERSION], ap_version, 16);
	if (L3_blocks)
	{
		//Fill_L3BlocksToInfoPage(input_4k_buf, L3_blocks);
	}
//	errno_t err;
	if (output_infopage_filename)
	{
		err = fopen_s(&fp,output_infopage_filename, "wb");
		if (fp)
		{
			fwrite(input_4k_buf, 1, 4096, fp);
			fflush(fp);
			fclose(fp);
		}
	}

	InitLdpc();

	TmpCRC = _ldpc->E13_FW_CRC32((unsigned int*)IDPBuf, 4096);
	IDPBuf[0x14] = (unsigned char)TmpCRC;
	IDPBuf[0x15] = (unsigned char)(TmpCRC >> 8);
	IDPBuf[0x16] = (unsigned char)(TmpCRC >> 16);
	IDPBuf[0x17] = (unsigned char)(TmpCRC >> 24);

	if (isSQL_groupRW)
	{
		Dev_SetFWSeed(0, 0x8299);
	}
	else
	{
		_ldpc->SetBlockSeed(0x8299);
	}
	TPgCnt = ((IDPBuf[0x25] << 8) + IDPBuf[0x24]) + 1;//will write page cnt
	writePageCount = TPgCnt;
	SectionMaxSecCnt = (file_buf[0x81] << 8) + file_buf[0x80];

	isOnly4Kdata = false;
	for (int pageIdx = 0; pageIdx< TPgCnt; pageIdx++) {
		//char pageFileName[128] = {0};
		//sprintf(pageFileName, "Ce%dBlk%dPage%d", ce, codeBlock, pageIdx);
		//FILE * fp2 = fopen(pageFileName, "wb");
		memset(page_buf, 0x00, page_size_alignK);
		memset(page_buf_verify, 0x00, page_size_alignK);

		if (isSQL_groupRW)
		{
			Dev_SetFWSeed(pageIdx % 256, 0x8299);
			Dev_SetEccMode(7);
			USB_SetSnapWR(7);
		}
		else
		{
			_ldpc->SetLDPCMode(7);
			_ldpc->SetPageSeed(pageIdx % 256);// mod 256
		}

		if (0 == pageIdx) {// page 0 ==> IDPage
			isOnly4Kdata = true;
			memcpy(page_buf, IDPBuf, 4096);

			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'I', 1, 0, L4K_BUF_SIZE_WR);
				Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32 * 4, true);
			}
			else
			{
				GenU17Spare(&page_buf[_fh->PageSizeK * 1024], 'I', 1, 0);
				_ldpc->GenRAWData(&page_buf[0], 4112, _fh->PageSizeK);
			}
		}
		else if (1 == pageIdx) {// Page 1 ==> Code sign header
			isOnly4Kdata = true;
			memcpy(page_buf, file_buf, 1024);	// for same with burner do
			offset = 512;
			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'H', 1, 0, L4K_BUF_SIZE_WR);
				Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32 * 4, true);
			}
			else
			{
				GenU17Spare(&page_buf[_fh->PageSizeK * 1024], 'H', 1, 0);
				_ldpc->GenRAWData(&page_buf[0], 4112, _fh->PageSizeK);
			}
		}
		else if ((TPgCnt - 3 - HTRefPageCount) == pageIdx) {//// Page last-2 Signature
			isOnly4Kdata = true;
			memcpy(page_buf, &file_buf[offset], 2048 + 512);
			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'S', 1, 0, L4K_BUF_SIZE_WR);
				Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32 * 4, true);
			}
			else
			{
				GenU17Spare(&page_buf[_fh->PageSizeK * 1024], 'S', 1, 0);
				_ldpc->GenRAWData(&page_buf[0], 4112, _fh->PageSizeK);
			}
		}
		else if ((TPgCnt - 2 - HTRefPageCount) == pageIdx) {//// Page last -1 Pad Page

			unsigned char* DQS_delay[2] = { 0 };
			unsigned char* DQ_delay[512] = { 0 };
			unsigned char* SDLL_offset[256] = { 0 };
			memcpy(DQS_delay, scanwindowOutbuf + 256, 2);
			memcpy(DQ_delay, scanwindowOutbuf + 512, 512);
			memcpy(SDLL_offset, scanwindowOutbuf + 1024, 256); //8CE*4DIE*4Byte*2CH = 128Byte * 2CH
			CharMemMgr a(8192);//8k
			a.MemSet(0x55);
			memcpy(page_buf, a.Data(), 8192);
			memcpy(page_buf + 150, SDLL_offset, 256);//SDLL Delay Offset 
			memcpy(page_buf + 1430, DQS_delay, 2);//DQS Delay
			memcpy(page_buf + 1434, DQ_delay, 512);//PAD DQ delay Offset 
			memcpy(page_buf + 2046, scanwindowOutbuf + 2046, 1);//Bus switch
			//if (_fh->bMaker == 0xad) {
			if (PreparePadpage(page_buf, odtSettingbuf))
			{
				Log(LOGDEBUG, "Prepare Pad page Fail! \n");
				return 3;
			}
			Dev_SetEccMode(IDPBuf[0x20]);
			USB_SetSnapWR(IDPBuf[0x20]);
			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'A', 1, 0, L4K_BUF_SIZE_WR);
				Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32 * 4, true);
			}
			else
			{
				GenU17Spare(&page_buf[_fh->PageSizeK * 1024], 'A', 1, 0);
				_ldpc->GenRAWData(&page_buf[0], 4112, _fh->PageSizeK);
			}
		}
		else if ((TPgCnt - 2) >= pageIdx && (TPgCnt - 2 - HTRefPageCount) <= pageIdx && HTRefPageCount) {//// Page HT ReferenceTable Page

			int tempIdx = pageIdx - TPgCnt + 2 + HTRefPageCount - 1;
			if (HTRefLength > (_fh->PageSizeK * 1024)) {
				memcpy(page_buf, &HTRefbuf[tempIdx*(_fh->PageSizeK * 1024)], (_fh->PageSizeK * 1024));
				HTRefLength -= (_fh->PageSizeK * 1024);
			}
			else {
				memcpy(page_buf, &HTRefbuf[tempIdx*(_fh->PageSizeK * 1024)], HTRefLength);
			}			

			Dev_SetEccMode(IDPBuf[0x20]);
			USB_SetSnapWR(IDPBuf[0x20]);
			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'T', 1, 0, L4K_BUF_SIZE_WR);
				Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32 * 4, true);
			}
			else
			{
				GenU17Spare(&page_buf[_fh->PageSizeK * 1024], 'T', 1, 0);
				_ldpc->GenRAWData(&page_buf[0], 4112, _fh->PageSizeK);
			}
		}
		else if (((TPgCnt - 1) == pageIdx)) {//// Page last code pointer
			isOnly4Kdata = true;
			memcpy(page_buf, CPBuf, 4096);
			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'P', 1, 0, L4K_BUF_SIZE_WR);
				Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32 * 4, true);
			}
			else
			{
				GenU17Spare(&page_buf[_fh->PageSizeK * 1024], 'P', 1, 0);
				_ldpc->GenRAWData(&page_buf[0], 4112, _fh->PageSizeK);
			}
		}
#if 0
		else if (TPgCnt - 1 == pageIdx) {//Write Information Page
			isOnly4Kdata = true;
			memcpy(page_buf, input_4k_buf, 4096);

			if (isSQL_groupRW)
			{
				USB_SetEccMode(7);
				USB_SetFWSeed(pageIdx, 0x8299);
			}
			else
			{
				_ldpc->SetLDPCMode(7);
				_ldpc->SetBlockSeed(0x8299);
				_ldpc->SetPageSeed(pageIdx); //YC teams setting
											 //GenE13Spare(&page_buf[_fh->PageSizeK*1024],'I',1,0);
				_ldpc->GenRAWData(page_buf, 4112, _fh->PageSizeK);
			}
		}
#endif
		else if (pageIdx >= TPgCnt) {
			/*write dummp data for page 0~100's data stable*/
			//srand((unsigned int)time(NULL));
			//for (int i = 0; i < page_size_alignK; i++)
			//{
			//	srand((unsigned int)rand() + i);
			//	page_buf[i] = (BYTE)(rand() % 256);
			//}
			//page_buf[0]='D';
			//page_buf[1]='U';
			//page_buf[2]='M';
			//page_buf[3]='M';
			//page_buf[4]='Y';
		}
		else {
			isOnly4Kdata = false;
			if (section<5)
			{
				if (SectionMaxSecCnt>(_fh->PageSizeK * 2))
				{
					SectionMaxSecCnt -= (_fh->PageSizeK * 2);
					WriteSec = (_fh->PageSizeK * 2);
				}
				else
				{
					WriteSec = SectionMaxSecCnt & 0xff;
					SectionMaxSecCnt = 0;
				}
				memcpy(page_buf, &file_buf[offset], WriteSec * 512);
				FrameNo = (WriteSec * 512) / 4096;
				if ((WriteSec * 512) % 4096)
					FrameNo++;

				if (isSQL_groupRW)
				{
					Dev_SetEccMode(IDPBuf[0x20]);
					USB_SetSnapWR(IDPBuf[0x20]);
					Dev_GenSpare_32(P4K_WR, 'C', FrameNo, section, L4K_BUF_SIZE_WR);
					Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32 * 4, true);
				}
				else
				{
					_ldpc->SetLDPCMode(IDPBuf[0x20]);
					GenU17Spare(&page_buf[_fh->PageSizeK * 1024], 'C', FrameNo, section);
					_ldpc->GenRAWData(&page_buf[0], FrameNo * 4112, _fh->PageSizeK);
				}
				offset += WriteSec * 512;
				if (!SectionMaxSecCnt)
				{
					do
					{
						section++;
						SectionMaxSecCnt = (file_buf[0x81 + (section * 2)] << 8) + file_buf[0x80 + (section * 2)];
					} while ((SectionMaxSecCnt == 0) && (section <= 5));
				}
			}

		}

		Utilities u;
		int total_diff = 0;
		int verify_fail = 0;
		//User need to set ap parameter to dump write read first!
		HANDLE handle = inputCmd->Get_SCSI_Handle();

		/*if (0 == pageIdx) {
			CharMemMgr paramBuf(1024);
			unsigned char* pParamBuf = paramBuf.Data();
			memcpy(pParamBuf, input_ap_parameter, paramBuf.Size());
			pParamBuf[0x00] = pParamBuf[0x00] & (~(0x20)); //close onfi
			pParamBuf[0x10] = pParamBuf[0x10] & 0x7F; //disable conversion -- bit8 = 0;
			pParamBuf[0x15] &= ~0x01;//close ecc	
			pParamBuf[0x0f] = 0x00; //close spare
			pParamBuf[0x0d] = 0x00; //close ecc
			pParamBuf[0x14] |= 0x20;//close inverse
			pParamBuf[0x27] |= 0x02;
			pParamBuf[0x20] = 0;// clear erase cnt
			//inputCmd->ST_SetGet_Parameter(1, handle, 0, true, pParamBuf);
		}*/

		if (0 == pageIdx) {
			//inputCmd->SetDumpWriteReadAPParameter(handle, input_ap_parameter);
		}
		if (!isWrittingCP && ((TPgCnt - 1) == pageIdx)) {
			goto END;
		}
		else {
			
			if (isSQL_groupRW)
			{
				unsigned char eccbuf[512] = { 0 };
				sqlCmd->GroupWriteD1(ce, codeBlock, pageIdx, 1, page_buf, _FH.PageSize, 2, 1, false);
				Dev_SetP4K(&P4K_WR[L4K_READ_START_ADDR], 32 * 4, false);
				sqlCmd->GroupReadDataD1(ce, codeBlock, pageIdx, 1, page_buf_verify, _FH.PageSize, 1, false, false);
			}
			else
			{
				inputCmd->ST_DirectPageWrite(1, handle, 0, 0/*RWMode*/, ce, codeBlock, pageIdx, 1/*page cnt*/, 0, 0, 0, page_size_alignK, 0, page_buf, 0, 0);
				inputCmd->ST_DirectPageRead(1, handle, 0, 0/*RWMode*/, ce, codeBlock, pageIdx, 1, 0, 0, 0, 0, page_size_alignK, page_buf_verify, 0);
			}
		}


		//saveToFile(page_buf, page_size_alignK, pageIdx, 1);
		//saveToFile(page_buf_verify, page_size_alignK, pageIdx, 0);

		verify_fail = u.DiffBitCheckPerK(page_buf, page_buf_verify, isOnly4Kdata ? 4096 : _fh->PageSizeK * 1024, 10, NULL, &total_diff);
		Log(LOGDEBUG, "ce %2d ,blk %5d ,page %5d ,writeCodePageDiff=%d \n", ce, codeBlock, pageIdx, total_diff);
		if (verify_fail) {
			rtn = 2;
			goto END;
			//rtn = 0;
		}


	}//end of page loop


END:
	if (NULL != file_buf)
		delete[] file_buf;
	if (NULL != page_buf)
		delete[] page_buf;
	if (NULL != page_buf_verify)
		delete[] page_buf_verify;
	if (NULL != fp)
		fclose(fp);

	//inputCmd->ST_SetGet_Parameter(1, inputCmd->Get_SCSI_Handle(), 0, true, input_ap_parameter);
	//if (NULL != fp2)
	//	fclose(fp2);
	return rtn;
}

#if 1
int USBSCSI5017_ST::WriteCPCode(unsigned char* buf, int ce, int codeBlock, CNTCommand * inputCmd, int writeMethod, bool isSQL_groupRW)
{
	int result = 0, pageIdx = writePageCount - 1;
	bool isOnly4Kdata = true;
	unsigned int *CPbufI = (unsigned int*)buf;
	LdpcEcc ldpcecc(0x5017);
	CPbufI[0x04 >> 2] = 0x00000000;
	CPbufI[0x04 >> 2] = ldpcecc.E13_FW_CRC32((unsigned int*)buf, 4096);
	Utilities u;
	int total_diff = 0;
	int verify_fail = 0;
	HANDLE handle = inputCmd->Get_SCSI_Handle();
	SQLCmd * sqlCmd = inputCmd->GetSQLCmd();
	const int MAX_FRAME_CNT = 4;
	const int L4K_SIZE_PER_FRAME = 32;
	const int L4K_BUF_SIZE_WR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT * 2;
	const int L4K_WRITE_START_ADDR = 0;
	const int L4K_READ_START_ADDR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT;

	unsigned char P4K_WR[L4K_BUF_SIZE_WR] = { 0 };
	int page_size_alignK = ((_fh->PageSize + 1023) / 1024) * 1024;
	unsigned char * page_buf = new unsigned char[page_size_alignK];
	unsigned char * page_buf_verify = new unsigned char[page_size_alignK];
	memset(page_buf, 0x00, page_size_alignK);
	memset(page_buf_verify, 0x00, page_size_alignK);

	if (isSQL_groupRW)
	{
		Dev_SetFWSeed(0, 0x8299);
		Dev_SetFWSeed(pageIdx % 256, 0x8299);
		Dev_SetEccMode(7);
		USB_SetSnapWR(7);
		Dev_GenSpare_32(P4K_WR, 'P', 1, 0, L4K_BUF_SIZE_WR);
		Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32 * 4, true);
	}
	else
	{
		_ldpc->SetBlockSeed(0x8299);
		_ldpc->SetPageSeed(pageIdx % 256);// mod 256
		_ldpc->SetLDPCMode(7);
		GenU17Spare(&page_buf[_fh->PageSizeK * 1024], 'P', 1, 0);
		_ldpc->GenRAWData(&page_buf[0], 4112, _fh->PageSizeK);
	}
	memcpy(page_buf, buf, 4096);
	isOnly4Kdata = true;
	if (isSQL_groupRW)
	{
		unsigned char eccbuf[512] = { 0 };
		sqlCmd->GroupWriteD1(ce, codeBlock, pageIdx, 1, page_buf, _FH.PageSize, 2, 1, false);
		Dev_SetP4K(&P4K_WR[L4K_READ_START_ADDR], 32 * 4, false);
		sqlCmd->GroupReadDataD1(ce, codeBlock, pageIdx, 1, page_buf_verify, _FH.PageSize, 1, false, false);
	}
	else
	{
		inputCmd->ST_DirectPageWrite(1, handle, 0, 0/*RWMode*/, ce, codeBlock, pageIdx, 1/*page cnt*/, 0, 0, 0, page_size_alignK, 0, page_buf, 0, 0);
		inputCmd->ST_DirectPageRead(1, handle, 0, 0/*RWMode*/, ce, codeBlock, pageIdx, 1, 0, 0, 0, 0, page_size_alignK, page_buf_verify, 0);
	}
	verify_fail = u.DiffBitCheckPerK(page_buf, page_buf_verify, isOnly4Kdata ? 4096 : _fh->PageSizeK * 1024, 10, NULL, &total_diff);
	Log(LOGDEBUG, "ce %2d ,blk %5d ,page %5d ,writeCodePageDiff=%d \n", ce, codeBlock, pageIdx, total_diff);
	if (verify_fail) {
		result = 2;
	}
	if (NULL != page_buf)
		delete[] page_buf;
	if (NULL != page_buf_verify)
		delete[] page_buf_verify;
	return result;
}
#endif

int USBSCSI5017_ST::ReadCPCode(unsigned char* buf, int ce, int codeBlock, CNTCommand * inputCmd, int writeMethod, bool isSQL_groupRW)
{
	int result = 0, pageIdx = writePageCount - 1;
	bool isOnly4Kdata = true;
	Utilities u;
	int total_diff = 0;
	int verify_fail = 0;
	HANDLE handle = inputCmd->Get_SCSI_Handle();
	SQLCmd * sqlCmd = inputCmd->GetSQLCmd();
	const int MAX_FRAME_CNT = 4;
	const int L4K_SIZE_PER_FRAME = 32;
	const int L4K_BUF_SIZE_WR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT * 2;
	const int L4K_WRITE_START_ADDR = 0;
	const int L4K_READ_START_ADDR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT;

	unsigned char P4K_WR[L4K_BUF_SIZE_WR] = { 0 };
	int page_size_alignK = ((_fh->PageSize + 1023) / 1024) * 1024;
	unsigned char * page_buf = new unsigned char[page_size_alignK];
	unsigned char * page_buf_verify = new unsigned char[page_size_alignK];
	memset(page_buf_verify, 0x00, page_size_alignK);

	if (isSQL_groupRW)
	{
		Dev_SetFWSeed(0, 0x8299);
		Dev_SetFWSeed(pageIdx % 256, 0x8299);
		Dev_SetEccMode(7);
		USB_SetSnapWR(7);
		Dev_GenSpare_32(P4K_WR, 'P', 1, 0, L4K_BUF_SIZE_WR);
		Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32 * 4, true);
	}
	else
	{
		_ldpc->SetBlockSeed(0x8299);
		_ldpc->SetPageSeed(pageIdx % 256);// mod 256
		_ldpc->SetLDPCMode(7);
		GenU17Spare(&page_buf[_fh->PageSizeK * 1024], 'P', 1, 0);
		_ldpc->GenRAWData(&page_buf[0], 4112, _fh->PageSizeK);
	}
	isOnly4Kdata = true;
	if (isSQL_groupRW)
	{
		unsigned char eccbuf[512] = { 0 };
		Dev_SetP4K(&P4K_WR[L4K_READ_START_ADDR], 32 * 4, false);
		sqlCmd->GroupReadDataD1(ce, codeBlock, pageIdx, 1, page_buf_verify, _FH.PageSize, 1, false, false);
	}
	else
	{
		inputCmd->ST_DirectPageRead(1, handle, 0, 0/*RWMode*/, ce, codeBlock, pageIdx, 1, 0, 0, 0, 0, page_size_alignK, page_buf_verify, 0);
	}
	if (NULL != page_buf_verify)
		delete[] page_buf_verify;
	return result;
}

void USBSCSI5017_ST::BuildCPBuffer(unsigned char* buf, int ce, int success_cnt, int codeBlock)
{
	if (_fh->bMaker == 0x2c || _fh->bMaker == 0x89)
		codeBlock *= 4;
	buf[0x110 + ce * 4 + success_cnt * 2] = codeBlock;
	buf[0x110 + ce * 4 + success_cnt * 2 + 1] = (codeBlock >> 8);
}

unsigned char* USBSCSI5017_ST::GetCPBuffer() {
	return gCPBuf;
}

unsigned char CodeBlockMark_U17[16] = {
	0x43, 0x4F, 0x44, 0x45, 0x20, 0x42, 0x4C, 0x4F, 0x43, 0x4B, 0x55, 0xAA, 0xAA, 0x55, 0x00, 0xFF
};

int USBSCSI5017_ST::GetSDLL_Overhead()
{
	return 12;
}

void USBSCSI5017_ST::ParsingBinFile(unsigned char *buf)
{
	unsigned char i;
	//unsigned int j;
	LdpcEcc ldpcecc(0x5017);
	unsigned int *offset;
	if (memcmp(buf, CodeBlockMark_U17, sizeof(CodeBlockMark_U17)) != 0)
		return;

	m_Section_Sec_Cnt[0] = ((unsigned short *)buf)[0x80 >> 1];	// ATCM
	m_Section_Sec_Cnt[1] = ((unsigned short *)buf)[0x82 >> 1];	// Code Bank
	m_Section_Sec_Cnt[2] = ((unsigned short *)buf)[0x84 >> 1];	// BTCM
	m_Section_Sec_Cnt[3] = ((unsigned short *)buf)[0x86 >> 1];	// Andes
	m_Section_Sec_Cnt[4] = ((unsigned short *)buf)[0x88 >> 1];	// FIP
	m_Section_Sec_Cnt[5] = ((unsigned short *)buf)[0x8A >> 1];	// Total sec count
	memset(m_Section_CRC32, 0x00, sizeof(m_Section_CRC32));
	offset = (unsigned int*)&buf[512];
	for (i = 0; i<5; i++)
	{
		if (m_Section_Sec_Cnt[i] != 0)
		{
			m_Section_CRC32[i] = ldpcecc.E13_FW_CRC32(offset, m_Section_Sec_Cnt[i] << 9);
			offset += (m_Section_Sec_Cnt[i] << 7);// x512 /4
		}
	}

}

int USBSCSI5017_ST::USB_ISPPreformat(unsigned char* buf, ULONG filesize, int subFeature)
{
	//ULONG filesize;
	//unsigned char *m_srcbuf;
	bool status;
	//errno_t err;
	//FILE * fp;
	//err = fopen_s(&fp,m_MPBINCode, "rb");
	//if (err)
	//{
	//	//Err_str = "S01";
	//	Log(LOGDEBUG, "Can not open MP BIN File: %s successfully \n!", m_MPBINCode);
	//	return 1;
	//}

	//fseek(fp, 0L, SEEK_END);
	//filesize = ftell(fp);
	//fseek(fp, 0, SEEK_SET);


	//if (filesize == -1L)
	//{
	//	//Err_str = "S01";  //Bin file have problem
	//	Log(LOGDEBUG, "Failed ! Check FW BIN File size(%s) = 0 \n", m_MPBINCode);
	//	fclose(fp);
	//	return 1;
	//}

	//m_srcbuf = (unsigned char *)malloc(filesize);
	//memset(m_srcbuf, 0x00, filesize);

	//if (fread(m_srcbuf, 1, filesize, fp) <= 0)
	//{
	//	//Err_str = "S01";
	//	Log(LOGDEBUG, "Reading MP BIN File Failed !!!\n");
	//	fclose(fp);
	//	free(m_srcbuf);
	//	return 1;
	//}
	//fclose(fp);

	//status = USB_ISPPreformat(m_srcbuf, filesize)?true:false;
	if(subFeature == 1) {
		//Send System Info for RDT
		status = USB_WriteInfoBlock(buf, filesize) ? true : false;
	}
	else {
		status = NT_TestPreformat(buf, filesize) ? true : false;
	}
	
	if (status != 0)
	{
		//Err_str = "I_x03";
		Log(LOGDEBUG, "ISP Program Burner Code Failed_loop!!\n");
		//free(buf);
		return 1;
	}

	//free(buf);
	Sleep(500);
	return status;
}


int USBSCSI5017_ST::NT_TestPreformat(unsigned char* buf, unsigned int BuferSize)
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xd1;

	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43

	ncmd[0x30] = 0x10;  //B48
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, false, 20);
	return result;
}

int USBSCSI5017_ST::USB_ReadInfoBlock(unsigned char* buf, unsigned int BuferSize)
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xd2;

	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43

	ncmd[0x30] = 0xB1;  //B48
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, true, 20);
	return result;
}

int USBSCSI5017_ST::USB_WriteInfoBlock(unsigned char* buf, unsigned int BuferSize)
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xd1;

	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43

	ncmd[0x30] = 0x32;  //B48
	ncmd[0x31] = 0x01;  //B49
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, false, 20);
	return result;
}

int USBSCSI5017_ST::USB_Send_apDBT(unsigned char* buf, unsigned int BuferSize, int mLast)
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xd1;

	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43

	ncmd[0x30] = 0x36;  //B48
	if(mLast)
		ncmd[0x31] = 0x02;//B49
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, false, 20);
	return result;
}

int USBSCSI5017_ST::USB_Read_apDBT(unsigned char* buf, unsigned int BuferSize, int channel, int ce, int die)
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xd2;

	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43

	ncmd[0x30] = 0xF6;  //B48
	ncmd[0x31] = channel;  //B49
	ncmd[0x32] = ce;  //B50
	ncmd[0x33] = die;  //B51
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, true, 20);
	return result;
}

int USBSCSI5017_ST::USB_ReadFCE(unsigned char* buf, unsigned int BuferSize)
{
	unsigned char ncmd[512] = { 0 };
	int result = 0;
	if (running_mode == rdt_mode) {
		ncmd[0] = 0xd2;

		unsigned int temp = BuferSize / 4;
		ncmd[0x28] = temp & 0xFF; //B40
		ncmd[0x29] = (temp >> 8) & 0xFF; //B41
		ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
		ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43

		ncmd[0x30] = 0x8E;  //B48

		result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, true, 20);
	}
	else if (running_mode == burner_mode) {
		unsigned char cdb512[512] = { 0 };
		cdb512[0] = 0x06;
		cdb512[1] = 0xF6;
		cdb512[2] = 0x99;
		result = Issue512CdbCmd(cdb512, buf, 512, SCSI_IOCTL_DATA_IN, _SCSI_DelayTime);
	}
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	return result;
}

int USBSCSI5017_ST::USB_ReadSystemInfo(unsigned char* buf, unsigned int BuferSize)
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xd2;

	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43

	ncmd[0x30] = 0x80;  //B48
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, true, 20);
	return result;
}

int USBSCSI5017_ST::USB_GetBadBlockCriteria(unsigned char* buf, unsigned int BuferSize)
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xd2;

	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43

	ncmd[0x30] = 0x90;  //B48
	ncmd[0x31] = 0x08;  //B49
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, true, 20);
	return result;
}

void USBSCSI5017_ST::Dev_GenSpare_32(unsigned char *buf, unsigned char mode, unsigned char FrameNo, unsigned char Section, int buf_len)
{
//

	const int L4KBADR_READ = 1;
	const int L4KBADR_WRITE = 0;
	const int MAX_FRAME_CNT = 4;
	const int L4K_SIZE_PER_FRAME = 32;

	/*1st 128 byte for write L4K, 2nd 128 bytes for read L4K*/
	if (((L4K_SIZE_PER_FRAME*MAX_FRAME_CNT * 2) != buf_len) || !buf)
	{
		assert(0);
	}



	/*follows cfg is depends on CTL, need to check with fw team!*/
	unsigned char L4k_default[2][16] = {
		{
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0xFF, 0xFF, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x11
		},
		{
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0xFF, 0xFF, 0x07, 0x00,
			0x00, 0x00, 0x00, 0x11
		},
	};


#if 1
	/*follows cfg is depends on CTL, for override the byte L4K[13], need to check with fw team!*/
	unsigned char L4KBADR[2][MAX_FRAME_CNT] = { { 0x14, 0x1C, 0x24, 0x2C },   //W
												{ 0x54, 0x5C, 0x64, 0x6C } };  //R
#else 
	unsigned char L4KBADR[2][MAX_FRAME_CNT] = { { 0x81, 0x89, 0x91, 0x99 },   //W
												{ 0xC1, 0xC9, 0xD1, 0xD9 } };  //R
#endif

	for (int wr_idx = 0; wr_idx<2; wr_idx++)//0 for write, 1 for read
	{
		for (int frame_idx = 0; frame_idx <FrameNo; frame_idx++)
		{
			int start_addr = (wr_idx*MAX_FRAME_CNT*L4K_SIZE_PER_FRAME) + frame_idx*L4K_SIZE_PER_FRAME;
			memset(&buf[start_addr], 0x00, L4K_SIZE_PER_FRAME);
			memcpy(&buf[start_addr], &L4k_default[wr_idx][0], 16);
			buf[start_addr] = 0xCB;
			buf[start_addr + 1] = 0xFF;
			buf[start_addr + 2] = 0xFF;
			buf[start_addr + 3] = 0xFF;
			buf[start_addr + 6] = mode;
			buf[start_addr + 7] = 0xFF;	// buffer vaild(all vaild)
			buf[start_addr + 13] = 0x90;
			if (mode == 'C')
			{
				buf[start_addr + 4] = 0x00;
				buf[start_addr + 5] = Section;
			}
			else if (mode == 'I')
			{
				buf[start_addr + 4] = 0x12;
				buf[start_addr + 5] = 0xCF;
			}
			else if (mode == 'P')
			{
				if(success_times % 2 == 0)
					buf[start_addr + 4] = 0x12;
				else
					buf[start_addr + 4] = 0x13;
				buf[start_addr + 5] = 0xF3;
				//buf[i*16+5] = 0x15;
			}
			/*
			else if(mode=='S')
			{
			buf[i*16+4] = 0xDF;
			//buf[i*16+5] = 0x19;
			buf[i*16+5] = 0x14;
			}
			*/
			buf[start_addr + 13] = L4KBADR[wr_idx][frame_idx];

		}
	}//end of w/r loop

}

void USBSCSI5017_ST::GenU17Spare(unsigned char *buf, unsigned char mode, unsigned char FrameNo, unsigned char Section)
{

	unsigned char i;

	for (i = 0; i<FrameNo; i++)
	{
		memset(&buf[i * 16], 0x00, 16);
		buf[i * 16] = 0xCB;
		buf[i * 16 + 1] = 0xFF;
		buf[i * 16 + 2] = 0xFF;
		buf[i * 16 + 3] = 0xFF;
		buf[i * 16 + 6] = mode;
		buf[i * 16 + 7] = 0xFF;	// buffer vaild(all vaild)
		buf[i * 16 + 13] = 0x90;
		if (mode == 'C')
		{
			buf[i * 16 + 4] = 0x00;
			buf[i * 16 + 5] = Section;
		}
		else if (mode == 'I')
		{
			/*buf[i * 16 + 4] = 0x01;
			buf[i * 16 + 5] = 0x48;*/
			buf[i * 16 + 4] = 0x12;
			buf[i * 16 + 5] = 0xCF;
		}
		else if (mode == 'P')
		{
			buf[i*16+4] = 0x12;
			buf[i*16+5] = 0xF3;
		}
		/*
		else if(mode=='S')
		{
		buf[i*16+4] = 0xDF;
		//buf[i*16+5] = 0x19;
		buf[i*16+5] = 0x14;
		}
		*/
		if (i == 1)
			buf[i * 16 + 13] = 0xA0;
		else if (i == 2)
			buf[i * 16 + 13] = 0xF0;
		else if (i == 3)
			buf[i * 16 + 13] = 0xC0;
	}
}


#define PageLogBit 9
#define eFuse 0
#define scanIndexNum 8*pow((double)2,eFuse)

void USBSCSI5017_ST::GetBootCodeRule(std::vector<int> * buf, int pageLogBit)
{
	for (int i = 0; i <= scanIndexNum; i++)
	{
		if (i % 2 == 0)
		{
			buf->push_back((int)(floor((i * pow((double)2, 6) * pow((double)2, pageLogBit) / pow((double)2, pageLogBit)))));
		}
		else
		{
			for (int j = 0; j <= 6; j++)
			{
				buf->push_back((int)(floor((i * pow((double)2, j) * pow((double)2, pageLogBit)) / pow((double)2, pageLogBit))));
			}
		}
	}
}


void USBSCSI5017_ST::PrepareIDpage(unsigned char *buf, int target_interface, int target_clock, int userSpecifyBlkCnt)
{


	UCHAR ubEN_SpeedEnhance = 0;
	UINT uli;
#if 0	
	if ((_fh->bMaker == 0x98) || (_fh->bMaker == 0x45)) {	//No Legacy: T32 SLC, T24 SLC
		if ((_fh->id[5] == 0xD6) || (_fh->id[5] == 0xD5)) {
			ubEN_SpeedEnhance = 0;
		}
		else {
			ubEN_SpeedEnhance = 1;
		}
	}
	else {
		ubEN_SpeedEnhance = 1;
	}
#else
	ubEN_SpeedEnhance = 0;
#endif


	/*0x00 -- XModemBuf[0] = "I", [1] = "D" */
	//pIdPageC[IDB_HEADER_0] = 'I';
	//pIdPageC[IDB_HEADER_1] = 'D';
	pIdPageC[0x00] = 'I';
	pIdPageC[0x01] = 'D';

	/*0x02 -- Set Burner Number */
	/*
	#if FAIL_PATTERN_2
	//pIdPageC[IDB_FW_REVISION_ID] = 0x21;
	pIdPageC[0x02] = 0x21;
	#else
	//pIdPageC[IDB_FW_REVISION_ID] = 0x12;
	pIdPageC[0x02] = 0x12;
	#endif
	*/
	pIdPageC[0x02] = 0x01;
	/*0x03 -- If this byte is equal to 0x33, then bootcode will check CRC of 4KB ID Table and code ptr */
	//pIdPageC[IDB_CRC32_CHECK_IDTABLE] = 0x33;
	pIdPageC[0x03] = 0x33;
	/*0x04 -- If this byte is equal to 0x33, then bootcode will check CRC32 of each FW section */
	//pIdPageC[IDB_CRC32_CHECK_FWSECTION] = 0x33;
	pIdPageC[0x04] = 0x33;
	/*0x05*/
	//**這邊要交由誰決定?
	//pIdPageC[IDB_LOAD_CODE_SPEED_ENHANCE] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;			//If 0x33, bootcode will issue 0x28~0x2D to do set feature command
	pIdPageC[0x05] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;			//If 0x33, bootcode will issue 0x28~0x2D to do set feature command
																/*0x06*/
																//pIdPageC[IDB_LOAD_CODE_SPEED_ENHANCE_CHECK] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;	//If 0x33, bootcode will issue get feature command to check feature
	pIdPageC[0x06] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;	//If 0x33, bootcode will issue get feature command to check feature
														/*0x07 -- Flash Interface after set feature 0:Legacy mode 2:toggle mode */
														/*
														if (ubEN_SpeedEnhance) {
														if (UFS_ONLY) {
														pIdPageC[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_TOGGLE;
														}
														else if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == LEGACY_INTERFACE ) {	// Default lefacy force to toggle	. Reference CH0 setting
														pIdPageC[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_TOGGLE;
														}
														else if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == TOGGLE_INTERFACE ) {	// Default toggle force to legacy
														pIdPageC[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_LEGACY;
														}
														}
														else {
														pIdPageC[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_LEGACY;
														}
														*/
	pIdPageC[0x07] = 0x00;

	/*0x08 -- FW version */
	//pIdPageC[IDB_FW_VERSION_CHAR + 0] = 'E';
	//pIdPageC[IDB_FW_VERSION_CHAR + 1] = 'D';
	//pIdPageC[IDB_FW_VERSION_CHAR + 2] = 'R';
	//pIdPageC[0x08] = 'E';
	//pIdPageC[0x09] = 'D';
	//pIdPageC[0x0A] = 'R';

	/*0x10 -- Vendor ID and Device ID (PCIE usage) */
	//IDPGL[IDL_PCIE_VENDOR_DEVICE_ID] = 0xEDBB1987;	//0x00;
	//IDPGL[0x10>>2] = 0xEDBB1987;	//0x00;
	/*0x14 -- The CRC32 value of the 1st 4KB of ID table */
	// Before CRC calculate, set this region as 0x00
	//IDPGL[IDL_IDPAGE_CRC] = 0x00000000;
	pIdPageI[0x14 >> 2] = 0x00000000;

	/*0x18 -- The number of sectors per page represented in powers of 2 (sector per Page == 1 << sector per Page bit) */
	//pIdPageC[IDB_SECTOR_PER_PAGE_LOGBIT] = gpFlhEnv->ubSectorsPerPageLog;	//log value for sectors per page
	//pIdPageC[0x18] = 0x05;	// 32 sector (16K)	//log value for sectors per page
	int sector_cnt = (_fh->PageSizeK * 1024) / 512;
	pIdPageC[0x18] = LogTwo(sector_cnt);
	/*0x19 -- The number of page per block represented in powers of 2 (page per block == 1 << page per block bit) */
	//pIdPageC[IDB_PAGE_ADR_BIT_WIDTH] = gpFlhEnv->ubPageADRBitWidth;
	//pIdPageC[0x19] = gpFlhEnv->ubPageADRBitWidth;
	pIdPageC[0x19] = LogTwo(_fh->PagePerBlock);
	//_fh.PagePerBlock
	uli = (_fh->PageSize_H << 8) + _fh->PageSize_L;
	/*0x1A -- The number of DMA frames are involved in one page */
	//pIdPageC[IDB_FRAME_PER_PAGE] = (U8)((gpFlhEnv->uwPageByteCnt) >> 12);	// Page size / 4KB
	pIdPageC[0x1A] = (unsigned char)(uli >> 12);	// Page size / 4KB

												/*0x1B -- */
												//pIdPageC[IDB_MDLL_DIRVAL_EN] = 0x00;
	pIdPageC[0x1B] = 0x00;
	/*0x1C -- Enable FPU_PTR_CXX_AX_C00_A5_C30 */
	//pIdPageC[IDB_NEW_FPU_EN] = 0x00;
	pIdPageC[0x1C] = 0x00;
	/*0x1D -- */
	//pIdPageC[IDB_NEW_FPU_CMD] = 0x00;
	pIdPageC[0x1D] = 0x00;
	/*0x1E -- */
	//pIdPageC[IDB_NEW_FPU_ADR_NUM] = 0x00;
	pIdPageC[0x1E] = 0x00;

	/*0x20 -- The LDPC mode for loading FW code */
	//pIdPageC[IDB_LDPC_MODE] = (U8)(r32_FCON[FCONL_LDPC_CFG] & CHK_ECC_MODE_MASK);		//前面已經算好了
	//  Mode 0: 2048bit 
	//	Mode 1: 1920bit 
	//	Mode 2: 1792bit
	//	Mode 3: 1664bit
	//	Mode 4: 1408bit
	//	Mode 5: 1152bit
	//	Mode 6: 1024bit
	//	Mode 7: 2432bit
	//	Mode 8: 2304bit
	uli /= pIdPageC[0x1A];
	uli -= 4112;
	uli /= 2;
	if (uli>304)
		pIdPageC[0x20] = 7;
	else if (uli>288)
		pIdPageC[0x20] = 8;
	else if (uli>256)
		pIdPageC[0x20] = 0;
	else if (uli>240)
		pIdPageC[0x20] = 1;
	else if (uli>224)
		pIdPageC[0x20] = 2;
	else if (uli>208)
		pIdPageC[0x20] = 3;
	else if (uli>176)
		pIdPageC[0x20] = 4;
	else if (uli>144)
		pIdPageC[0x20] = 5;
	else
		pIdPageC[0x20] = 6;

	_RDTCodeBodyLDPCMode = pIdPageC[0x20];
	/*0x21 -- If this byte is equal to 0x33, then we will not set PCIe link in boot code, only in FW */
	//pIdPageC[IDB_HOST_DELAY_LINK] = 0x33;
	pIdPageC[0x21] = 0x33;
	//pIdPageC[IDB_HOST_DELAY_LINK] = 0x00;		//Force let PCIe link at IDPG
	/*0x22 -- If this byte is equal to 0x33, Set FPU to 6 Address else Set FPU to 5 Address */
	//pIdPageC[IDB_FLH_6ADR_NUM] = 0x00;
	pIdPageC[0x22] = 0x00;
	/*0x23 -- Micron 0xDA 0x91 0x40 Hynix 0xDA */
	//pIdPageC[IDB_SLC_METHOD] = gpFlhEnv->ubSLCMethod;
	if (_fh->bIsHynix3DTLC)
		pIdPageC[0x23] = 0xDA;
	else
		pIdPageC[0x23] = 0xA2;
	/*0x24 -- The logical page index for code pointer */
	//IDPGW[IDW_CODEPTR_PAGEIDX] = FlaDealCodeSectionData();		//在知道slot如何擺後才去算

	/*0x26 -- If this byte is equal to 0x33, do IP ZQ calibration on when speed enhance */
	/*
	#ifdef VERIFY_PIO_CODE_BLOCK
	pIdPageC[IDB_ZQ_NAND_ENHANCE] = 0x00;
	#else
	pIdPageC[IDB_ZQ_NAND_ENHANCE] = 0x00;
	#endif
	*/
	pIdPageC[0x26] = 0x00;
	/*0x27 -- If this byte is equal to 0x33, do NAND ZQ calibration on when speed enhance */
	/*
	#ifdef VERIFY_PIO_CODE_BLOCK
	pIdPageC[IDB_ZQ_IP_ENHANCE] = 0x00;
	#else
	pIdPageC[IDB_ZQ_IP_ENHANCE] = 0x00;
	#endif
	*/
	pIdPageC[0x27] = 0x00;
	/*0x28 -- The 1st Set Feature Command that will be issued to Flash */
	/*
	pIdPageC[IDB_SETFEATURE_CMD] = 0x00;
	pIdPageC[IDB_SETFEATURE_ADR] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT0] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT1] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT2] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT3] = 0x00;
	if (ubEN_SpeedEnhance) {
	if (UFS_ONLY) {		// Toshiba
	pIdPageC[IDB_SETFEATURE_CMD] = 0xEF;
	pIdPageC[IDB_SETFEATURE_ADR] = 0x80;
	pIdPageC[IDB_SETFEATURE_DAT0] = 0x00;	// Toggle: 0x00,  SDR: 0x01
	pIdPageC[IDB_SETFEATURE_DAT1] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT2] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT3] = 0x00;
	}
	else if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == LEGACY_INTERFACE ) {		// Default lefacy force to toggle
	Dump_Debug(uart_printf("Default legacy\n"));
	if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == TOSHIBASANDISK ) {		// Toshiba (Bisc3)
	pIdPageC[IDB_SETFEATURE_CMD] = 0xEF;
	pIdPageC[IDB_SETFEATURE_ADR] = 0x80;
	pIdPageC[IDB_SETFEATURE_DAT0] = 0x00;	// Toggle: 0x00,  SDR: 0x01
	pIdPageC[IDB_SETFEATURE_DAT1] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT2] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT3] = 0x00;
	}
	else if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == INTELMICRON ) {		// Micron (B16A)
	pIdPageC[IDB_SETFEATURE_CMD] = 0xEF;
	pIdPageC[IDB_SETFEATURE_ADR] = 0x01;
	pIdPageC[IDB_SETFEATURE_DAT0] = 0x20;	// [4:5] NVDDR2(toggle): 0x2, Asychronous(Legacy): 0x0
	#ifdef VERIFY_PIO_CODE_BLOCK
	//pIdPageC[IDB_SETFEATURE_DAT0] = 0x00;
	//pIdPageC[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_LEGACY;

	//pIdPageC[IDB_SETFEATURE_DAT0] |= 0x0;
	//pIdPageC[IDB_SETFEATURE_DAT0] |= 0x4; // Toggle 100Mhz or Legacy 40Mhz
	//pIdPageC[IDB_SETFEATURE_DAT0] |= 0x7; // 200Mhz
	pIdPageC[IDB_SETFEATURE_DAT0] |= 0xA; // 400Mhz
	#endif
	pIdPageC[IDB_SETFEATURE_DAT1] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT2] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT3] = 0x00;
	}
	else if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == SKHYNIX ) {			// Hynix (H14)
	pIdPageC[IDB_SETFEATURE_CMD] = 0xEF;
	pIdPageC[IDB_SETFEATURE_ADR] = 0x01;
	pIdPageC[IDB_SETFEATURE_DAT0] = 0x20;	// [4:5] DDR(toggle): 0x2, SDR(Legacy): 0x0
	pIdPageC[IDB_SETFEATURE_DAT1] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT2] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT3] = 0x00;
	}
	else if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == SAMSUNG ) {			// Samsung (MLC)

	}
	}
	else if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == TOGGLE_INTERFACE ) {		// Default toggle force to legacy
	Dump_Debug(uart_printf("Default toggle\n"));
	if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == TOSHIBASANDISK ) {		//Toshiba(1Z TLC, 1Z MLC)
	pIdPageC[IDB_SETFEATURE_CMD] = 0xEF;
	pIdPageC[IDB_SETFEATURE_ADR] = 0x80;
	pIdPageC[IDB_SETFEATURE_DAT0] = 0x01;	// Toggle: 0x00,  SDR: 0x01
	pIdPageC[IDB_SETFEATURE_DAT1] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT2] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT3] = 0x00;
	}
	else if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == INTELMICRON ) {		// Micron (B16A)
	pIdPageC[IDB_SETFEATURE_CMD] = 0xEF;
	pIdPageC[IDB_SETFEATURE_ADR] = 0x01;
	pIdPageC[IDB_SETFEATURE_DAT0] = 0x00;	// [4:5] NVDDR2(toggle): 0x2, Asychronous(Legacy): 0x0
	pIdPageC[IDB_SETFEATURE_DAT1] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT2] = 0x00;
	pIdPageC[IDB_SETFEATURE_DAT3] = 0x00;
	}
	}
	}
	*/
	pIdPageC[0x28] = 0x00;
	pIdPageC[0x29] = 0x00;
	pIdPageC[0x2A] = 0x00;
	pIdPageC[0x2B] = 0x00;
	pIdPageC[0x2C] = 0x00;
	pIdPageC[0x2D] = 0x00;
	/*0x2E -- Set feature busy time*/
	//IDPGW[IDW_SETFEATURE_BUSYTIME] = (ubEN_SpeedEnhance) ? 0x10 : 0x00;
	pIdPageS[0x2E >> 1] = (ubEN_SpeedEnhance) ? 0x10 : 0x00;
	pIdPageS[0x31] = 0;
	// Back door used IDPage default
	/*0x30 -- The first PCIe use back door entry number, these back door will process before host interface init */
	//pIdPageC[IDB_PCIE_BACKDOOR0_START_ENTRY] = 0x00;
	/*0x31 -- The total number of PCIe back door entry count,  these back door will process before host interface init */
	//pIdPageC[IDB_PCIE_BACKDOOR0_ENTRY_COUNT] = 0x00;
	/*0x32 -- The first PCIe use back door entry number, these back door will process after host interface init */
	//pIdPageC[IDB_PCIE_BACKDOOR1_START_ENTRY] = 0x00;
	/*0x33 -- The total number of PCIe back door entry count,  these back door will process after host interface init */
	//pIdPageC[IDB_PCIE_BACKDOOR1_ENTRY_COUNT] = 0x00;
	/*0x34 -- The first PCIe use back door entry number, These back door will process after ID Page load */
	//pIdPageC[IDB_NORMAL_BACKDOOR_START_ENTRY] = 0x00;
	/*0x35 -- The total number of normal back door entry count. These back door will process after ID Page load */
	//pIdPageC[IDB_NORMAL_BACKDOOR_ENTRY_COUNT] = 0x00;
	/*0x36 -- The Last back door entry number,  these back door will process before PRAM enable jump to FW */
	//pIdPageC[IDB_LAST_BACKDOOR_START_ENTRY] = 0x00;
	/*0x37 -- The Last back door entry count, these back door will process before PRAM enable jump to FW */
	//pIdPageC[IDB_LAST_BACKDOOR_ENTRY_COUNT] = 0x00;

	/*0x38 -- 0 : According to Efuse BIT0: Code sign enable  BIT1: HMAC Enable  BIT2: FIPS-140 Enable */
	/*
	#if 1
	pIdPageC[IDB_SECURITY_OPTION] = 0;	//BIT0 | BIT2;
	uart_printf("===========================\n");
	uart_printf("This is no Code Sign Burner\n");
	uart_printf("This is no Code Sign Burner\n");
	uart_printf("This is no Code Sign Burner\n");
	uart_printf("===========================\n");
	#else
	pIdPageC[IDB_SECURITY_OPTION] = BIT0 | BIT2;
	#endif
	*/
	pIdPageC[0x3D] = 0x33;
	pIdPageC[0x38] = 0;	//BIT0 | BIT2;

						/*0x39 -- If this byte is equal to 0x33, set VREF on when speed enhance */
						// toggle need take care
						/*
						#ifdef VERIFY_PIO_CODE_BLOCK
						if(pIdPageC[IDB_FLASH_INTERFACE_AFTER_ENHANCE] == INTERFACE_TOGGLE){
						pIdPageC[IDB_VREF_ENHANCE] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;
						}
						else{
						pIdPageC[IDB_VREF_ENHANCE] = 0x00;
						}
						#else
						pIdPageC[IDB_VREF_ENHANCE] = 0x00;
						#endif
						*/
	pIdPageC[0x39] = 0x00;

	/*0x3A -- Cell Type.  0 : According to ID Detection 1: SLC  2: MLC 3: TLC 4: QLC  */
	//pIdPageC[IDB_CELL_TYPE] = gpFlhEnv->FlashDefaultType.BitMap.ubCellType + 1;
	pIdPageC[0x3A] = 3; // TLC test only
					 /*0x3B -- Remapping Info.  //0 : 4CH(Default) 1: 2CH(CH0+CH1)  2: 2CH(CH2+CH3)  */
					 //gsIPL_OptionInfo.dw4.ubRemappingCEInfo = 0;
					 //pIdPageC[IDB_REMAPPING_INFO] = gsIPL_OptionInfo.dw4.ubRemappingCEInfo;
	pIdPageC[0x3B] = 0x00;
	/*0x3C -- 0x33 Enable DQS High when set feature */
	//pIdPageC[IDB_SET_FEATURE_DQSH] = 0x00;
	pIdPageC[0x3C] = 0x00;
	/*0x3D -- 0x33 EDO On */
	/*
	if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == TOGGLE_INTERFACE ) {
	pIdPageC[IDB_EDO_ENABLE] = 0x00;
	}
	else {
	pIdPageC[IDB_EDO_ENABLE] = 0x33;
	}
	*/
	pIdPageC[0x3D] = 0x00;
	/*0x3E -- 0x33 Uart Disable */
	//pIdPageC[IDB_UART_DISABLE] = 0x00;
	pIdPageC[0x3E] = 0x00;
	/*0x3F -- 0x33 CE Decoder Enable */
	//pIdPageC[IDB_CE_DECODER_ENABLE] = gsIPL_OptionInfo.dw0.btCeDecoder ? 0x33 : 0x00;
	pIdPageC[0x3F] = 0x00;

	/*0x40 -- */
	//pIdPageC[IDB_SYSx2_CLK_SOURCE] = OSC1_KOUT2_SEL;
	/*0x41 -- */
	//pIdPageC[IDB_SYSx2_CLK_DIVIDER] = CLK_DIV_1;
	/*0x42 -- */
	//pIdPageC[IDB_ECC_CLK_SOURCE] = OSC2_KOUT1_SEL;
	/*0x43 -- */
	//pIdPageC[IDB_ECC_CLK_DIVIDER] = CLK_DIV_2;
	/*0x44 -- FLH_IF_CLK */
	//0: 10Mhz (Toggle/Legacy)
	//1: 33Mhz (Toggle/Legacy)
	//2: 40Mhz (Toggle/Legacy)
	//3: 100Mhz (Toggle)
	//4: 200Mhz (Toggle2)(Uper this speed one more set feature need)
	//5: 267Mhz (Toggle2)(Upper this ZQ Calibration need)
	//6: 333Mhz (Toggle2)
	//7: 400Mhz (Toggle2)

	//pIdPageC[IDB_FLH_IF_CLK] = DEF_FLASH_10MHZ;
	//pIdPageC[0x44] = 0x00;
	/*0x45 -- 0x45 Do Clock setting */
	/*
	#ifdef VERIFY_PIO_CODE_BLOCK
	pIdPageC[IDB_CLK_CHANGE_EN] = 0x00;	//0x33;
	if(pIdPageC[IDB_SETFEATURE_DAT0] == 0x24) {
	pIdPageC[IDB_FLH_IF_CLK] = FLH_IF_100MHz;
	}
	if(pIdPageC[IDB_SETFEATURE_DAT0] == 0x27) {
	pIdPageC[IDB_FLH_IF_CLK] = FLH_IF_200MHz;
	}
	if(pIdPageC[IDB_SETFEATURE_DAT0] == 0x2A) {
	pIdPageC[IDB_FLH_IF_CLK] = FLH_IF_400MHz;
	}
	if(pIdPageC[IDB_SETFEATURE_DAT0] == 0x04) {
	pIdPageC[IDB_FLH_IF_CLK] = FLH_IF_40MHz;
	}
	#else
	pIdPageC[IDB_CLK_CHANGE_EN] = 0x00;
	#endif
	*/
	pIdPageC[0x45] = 0x00;
	/*0x46 -- */
	//pIdPageC[IDB_SHA_CLK_SOURCE] = OSC1_KOUT2_SEL;
	pIdPageC[0x46] = 0x00;
	/*0x47 -- */
	//pIdPageC[IDB_SHA_CLK_DIVIDER] = CLK_DIV_2;
	pIdPageC[0x47] = 0x00;

	/*0x48 -- */
	//IDPGW[IDW_MDLL_CH0_READVAL] = 0x00;
	/*0x4A -- */
	//IDPGW[IDW_MDLL_CH1_READVAL] = 0x00;
	/*0x4C -- */
	//IDPGW[IDW_MDLL_CH2_READVAL] = 0x00;
	/*0x4E -- */
	//IDPGW[IDW_MDLL_CH3_READVAL] = 0x00;
	/*0x50 -- */
	//IDPGW[IDW_MDLL_CH0_WRITEVAL] = 0x00;
	/*0x52 -- */
	//IDPGW[IDW_MDLL_CH1_WRITEVAL] = 0x00;
	/*0x54 -- */
	//IDPGW[IDW_MDLL_CH2_WRITEVAL] = 0x00;
	/*0x56 -- */
	//IDPGW[IDW_MDLL_CH3_WRITEVAL] = 0x00;

	/*0x5A -- SATA GEN */
	//pIdPageC[IDB_SATA_GEN] = 0x0;		// 0: Gen3 (default), 1: Gen2, 2: Gen1
	/*0x5B -- PCIE GEN*/
	//pIdPageC[IDB_PCIE_GEN] = 0x3;	//0x1;		// 2: Support Gen2. 3: Support Gen3
	//pIdPageC[0x5B] = 0x03;		//0x1;		// 2: Support Gen2. 3: Support Gen3

	/*0x5C -- DCC Setting */	// 彥廷說不用設沒關係
								//pIdPageC[IDB_DCC_SETTING] = 0x00;
								//pIdPageC[IDB_DCC_SETTING] |= (0x2 & DCC_TRAIN_SIZE_MASK) << DCC_TRAIN_SIZE_SHIFT;		// Frame count.    0: 4k, 1:8k, 2: 16k
								//pIdPageC[IDB_DCC_SETTING] |= (0x1 & DCC_TSB_MODE_MASK) << DCC_TSB_MODE_SHIFT;			// 1: Toshiba mode

								/*0x5D -- CRS delay off */
								//pIdPageC[IDB_CRS_DELAY_OFF] = 0x00; //0x00;
								/*0x5E -- PCIE Lane */
								//pIdPageC[IDB_PCIE_LANE] = PCIE_LANEx1;		// PCIE_LANEx1 (0x0), PCIE_LANEx2 (0x1), PCIE_LANEx4 (0x3), PCIE_LANEx8 (0x7), PCIE_LANEx16 (0xF)
								//pIdPageC[IDB_PCIE_LANE] = PCIE_LANEx4;		// PCIE_LANEx1 (0x0), PCIE_LANEx2 (0x1), PCIE_LANEx4 (0x3), PCIE_LANEx8 (0x7), PCIE_LANEx16 (0xF)
								//pIdPageC[0x5E] = 0x03;		// PCIE_LANEx1 (0x0), PCIE_LANEx2 (0x1), PCIE_LANEx4 (0x3), PCIE_LANEx8 (0x7), PCIE_LANEx16 (0xF)


								/*0x400 -- Converts logical page to the coresponding physical page, there are 256 page rules*/
								//for (uli = 0; uli < MAX_FAST_PG; uli++) {
								//	IDPGW[IDW_FAST_PAGE(uli)] = flaFastPageRule(uli);
								//}

	/*pIdPageC[0x08] = 'E';
	pIdPageC[0x09] = 'D';
	pIdPageC[0x0A] = 'R';

	pIdPageC[0x10] = 0x87;
	pIdPageC[0x11] = 0x19;
	pIdPageC[0x12] = 0xBB;
	pIdPageC[0x13] = 0xED;*/

	pIdPageC[0x02] = 0x12;
	pIdPageC[0x3C] = 0x05;
	pIdPageC[0x3E] = 0x03;
	pIdPageC[0x3F] = 0x01;

	pIdPageC[0x41] = 0x33;
	pIdPageC[0x44] = 0x01;
	pIdPageC[0x45] = 0x01;
	pIdPageC[0x46] = 0x03;
	pIdPageC[0x47] = 0x03;
	pIdPageC[0x48] = 0x05;
	pIdPageC[0x49] = 0x01;
	pIdPageC[0x4A] = 0x01;
	pIdPageC[0x4C] = 0x02;
	pIdPageC[0x4D] = 0x01;
	pIdPageC[0x4E] = 0x00;

	pIdPageC[0x50] = 0x00;
	pIdPageC[0x52] = 0x00;
	pIdPageC[0x54] = 0x00;
	pIdPageC[0x56] = 0x00;

	pIdPageC[0x9F] = 0x08;
	pIdPageC[0xA0] = 0x03;
	pIdPageC[0xA2] = 0x07;
	pIdPageC[0xA4] = 0x99;
	pIdPageC[0xA5] = 0x82;
	pIdPageC[0xA8] = 0x01;
	pIdPageC[0xAC] = 0x0B;
	pIdPageC[0xAE] = 0x0F;
	pIdPageC[0xB0] = 0x02;
	pIdPageC[0xB4] = 0x03;
	pIdPageC[0xB8] = 0x14;
	pIdPageC[0xBA] = 0x18;
	pIdPageC[0xBC] = 0x04;
	pIdPageC[0xC0] = 0x05;
	pIdPageC[0xC4] = 0x20;
	pIdPageC[0xC6] = 0x60;
	pIdPageC[0xC8] = 0x06;
	pIdPageC[0xCC] = 0x07;
	pIdPageC[0xDB] = _user_fw_performance;
	pIdPageC[0xDC] = _user_fw_advance;
	pIdPageC[0xDD] = _user_nes_switch;
	pIdPageC[0xDE] = _user_tag_switch;
	if (_user_perFrame_dummy != 0) {
		pIdPageC[0x67] = _user_perFrame_dummy;
		CalculatePerFrameDummy(pIdPageC[0x20]);
		CalculatePerFrameDummy(7);
	}
	PrepareIDpageForRDTBurner(target_interface, target_clock, userSpecifyBlkCnt);
	memcpy(buf, pIdPageC, sizeof(IDPage));
	return;
}


void USBSCSI5017_ST::InitialCodePointer(unsigned char *buf, unsigned short *blk)
{
	memcpy(pCPDataC, CODEPTRB, 4096);
	memcpy(pIdPageC, IDPGB, 4096);

	UCHAR ubSectorIdx, ubMaxSlotValue;
	UCHAR ubSectorNum = 5;
	USHORT uwPageIdx;
	UINT ulFWSlot_Base, ulFWSector_Base;
	LdpcEcc ldpcecc(0x5017);
	// Initial CP Base Zero value
	ubMaxSlotValue = 1;

	/*0x00 -- Code Pointer Mark. [0] = 'C', [1] = 'P' */
	pCPDataC[0] = 'C';
	pCPDataC[1] = 'P';
	/*0x02 -- ID Page FW Revision*/
	//pCPDataC[CPB_IDPG_REVISION] = 0x01;
	pCPDataC[0x02] = 0x01;
	/*0x04 -- CRC32 value of 4KB Code Pointer*/
	//晚一點才有辦法算
	/*0x08 -- Record which slot has valid, and FW BIT15 is reserved*/
	//CODEPTRW[CPW_VALIDSLOT_BITMAP] = 0x0001;	//先開FW slot 0
	pCPDataS[0x08 >> 1] = 0x0001;	//先開FW slot 0

									/*0x0B -- Record the number of valid slot FW*/
									//pCPDataC[CPB_VALID_SLOTNUM] = 0x01;			//先開FW slot 0
	pCPDataC[0x0B] = 0x01;			//先開FW slot 0
									/*0x10 -- Record which FW slot to active*/
									//pCPDataC[CPB_ACTIVE_SLOTINDEX] = 0x00;		//先開FW slot 0
	pCPDataC[0x10] = 0x00;		//先開FW slot 0
								/*
								#if MULTI_FW_SLOT
								CODEPTRW[CPW_VALIDSLOT_BITMAP] = 0x0003;	//開FW slot 0, 1
								pCPDataC[CPB_VALID_SLOTNUM] = 0x02;			//開FW slot 0, 1
								#if	DEBUG_MAKE_FAIL_PATTERN
								pCPDataC[CPB_ACTIVE_SLOTINDEX] = 0x00;		//Active FW slot 0, and will do retry list
								#else
								pCPDataC[CPB_ACTIVE_SLOTINDEX] = 0x00;		//Active FW slot 1, Normal but not fail pattern, setting to read slot 1
								#endif

								#endif
								*/

#if 0	
								/*0x11 -- RetryList Slot Index 0*/
	pCPDataC[CPB_RETRYLIST_0] = 0x01;
	/*0x12 -- RetryList Slot Index 1*/
	pCPDataC[CPB_RETRYLIST_1] = 0x00;
	/*0x13 -- RetryList Slot Index 2*/
	pCPDataC[CPB_RETRYLIST_2] = 0x00;
	/*0x14 -- RetryList Slot Index 3*/
	pCPDataC[CPB_RETRYLIST_3] = 0x00;
	/*0x15 -- RetryList Slot Index 4*/
	pCPDataC[CPB_RETRYLIST_4] = 0x00;
	/*0x16 -- RetryList Slot Index 5*/
	pCPDataC[CPB_RETRYLIST_5] = 0x00;
	/*0x17 -- RetryList Slot Index 6*/
	pCPDataC[CPB_RETRYLIST_6] = 0x00;
	/*0x18 -- RetryList Slot Index 7*/
	pCPDataC[CPB_RETRYLIST_7] = 0x00;
	/*0x19 -- RetryList Slot Index 8*/
	pCPDataC[CPB_RETRYLIST_8] = 0x00;
	/*0x1A -- RetryList Slot Index 9*/
	pCPDataC[CPB_RETRYLIST_9] = 0x00;
	/*0x1B -- RetryList Slot Index 10*/
	pCPDataC[CPB_RETRYLIST_10] = 0x00;
	/*0x1C -- RetryList Slot Index 11*/
	pCPDataC[CPB_RETRYLIST_11] = 0x00;
	/*0x1D -- RetryList Slot Index 12*/
	pCPDataC[CPB_RETRYLIST_12] = 0x00;
	/*0x1E -- RetryList Slot Index 13*/
	pCPDataC[CPB_RETRYLIST_13] = 0x00;
	/*0x1F -- Record the number of Slot in List should retry, 0 means no retry other slot*/
	pCPDataC[CPB_RETRY_LIST_NUM] = 0x00;
#if MULTI_FW_SLOT
	pCPDataC[CPB_RETRY_LIST_NUM] = 0x01;
#endif
#endif


	//=================================================================
	// Initial FW Slot ID: 0. Start Section: 0, Section Number: 2.
	//=================================================================
	//pCPDataC[(CP_FWSLOT0_OFFSET + CP_FWSLOT_ID)] = 0x00;
	//pCPDataC[(CP_FWSLOT0_OFFSET + CP_FWTYPE)] 	 = 0x00;	// 0x00: Normal FW, 0x01: Loader
	//pCPDataC[(CP_FWSLOT0_OFFSET + CP_CODEBANK_NUM)] = gCODE_SECTION[1].size / DEF_KB(32);	//暫定每個 Code bank 都是32K
	//CODEPTRW[(CP_FWSLOT0_OFFSET + CP_CSHPAGE_IDX) >> 1]  = 0x01;	// Zero page for IDPG, 1st Page for Code Sign Header
	//pCPDataC[(CP_FWSLOT0_OFFSET + CP_CODEBANK_SECTORCNT)] = gCODE_SECTION[1].size / 512;;
	//CODEPTRW[(CP_FWSLOT0_OFFSET + CP_SIGNPAGE_IDX) >> 1] = 在後面算出數值後才給值;
	//pCPDataC[(CP_FWSLOT0_OFFSET + CP_FWSTART_SECTION)] = 0x00;
	pCPDataC[(CP_FWSLOT0_OFFSET + 0x00)] = 0x00;
	pCPDataC[(CP_FWSLOT0_OFFSET + 0x01)] = 0x00;	// 0x00: Normal FW, 0x01: Loader
	pCPDataC[(CP_FWSLOT0_OFFSET + 0x02)] = (unsigned char)m_Section_Sec_Cnt[1] / 64;	//暫定每個 Code bank 都是32K
	pCPDataC[(CP_FWSLOT0_OFFSET + 0x03)] = (unsigned char)m_Section_Sec_Cnt[1];		// need 2 byte but Spec only allocate one byte 
	pCPDataS[(CP_FWSLOT0_OFFSET + 0x04) >> 1] = 0x01;	// Zero page for IDPG, 1st Page for Code Sign Header

														//CODEPTRW[(CP_FWSLOT0_OFFSET + CP_SIGNPAGE_IDX) >> 1] = 在後面算出數值後才給值;
	pCPDataC[(CP_FWSLOT0_OFFSET + 0x08)] = 0x00;
	/*
	for (ubSectorIdx = 0; ubSectorIdx < CP_MAX_SECTOR_NUM; ubSectorIdx++) {
	if (gCODE_SECTION[CP_MAX_SECTOR_NUM - 1 - ubSectorIdx].size != 0) {
	break;
	}
	ubSectorNum = ubSectorNum - 1;
	}
	*/
	pCPDataC[(CP_FWSLOT0_OFFSET + 0x09)] = ubSectorNum;
	//pCPDataC[(CP_FWSLOT0_OFFSET + CP_CBANKINATCM_SECCNT)] = (gCODE_SECTION[1].size - (32 << 10) ) / 512;
	pCPDataC[(CP_FWSLOT0_OFFSET + 0x0A)] = 64; //CodeBank In ATCM SectorCnt, one bank(bank0)->32KByte->64 sectors

	ulFWSlot_Base = CP_FWSLOT0_OFFSET;
	/* 0x10 ~ 0x1E -- Record each channel Block Index */
	/*pCPDataS[(ulFWSlot_Base + 0x10) >> 1] = blk[0];		// default value	
	pCPDataS[(ulFWSlot_Base + 0x12) >> 1] = blk[1];		// default value	
	pCPDataS[(ulFWSlot_Base + 0x14) >> 1] = blk[0];		// default value
	pCPDataS[(ulFWSlot_Base + 0x16) >> 1] = blk[1];		// default value
	pCPDataS[(ulFWSlot_Base + 0x18) >> 1] = blk[0];		// default value
	pCPDataS[(ulFWSlot_Base + 0x1A) >> 1] = blk[1];		// default value
	pCPDataS[(ulFWSlot_Base + 0x1C) >> 1] = blk[0];		// default value
	pCPDataS[(ulFWSlot_Base + 0x1E) >> 1] = blk[1];		// default value*/

	uwPageIdx = 0x00002;	// First page is for IDPG; Second Page is for Code Sign Header

	ulFWSlot_Base = CP_FWSLOT0_OFFSET;		//FW slot 0
	for (ubSectorIdx = 0; ubSectorIdx < 5; ubSectorIdx++) {
		ulFWSector_Base = ulFWSlot_Base + 0x80 + 16 * ubSectorIdx;

		pCPDataS[(ulFWSector_Base) >> 1] = uwPageIdx;
		pCPDataS[(ulFWSector_Base + 0x02) >> 1] = (m_Section_Sec_Cnt[ubSectorIdx] >> 1) / _fh->PageSizeK;
		if (((m_Section_Sec_Cnt[ubSectorIdx] >> 1) % _fh->PageSizeK) > 0) {
			pCPDataS[(ulFWSector_Base + 0x02) >> 1] += 1;
		}
		pCPDataS[(ulFWSector_Base + 0x04) >> 1] = (m_Section_Sec_Cnt[ubSectorIdx]);
		uwPageIdx = uwPageIdx + pCPDataS[(ulFWSector_Base + 0x02) >> 1];
		// Every section CRC 32
		pCPDataI[(ulFWSector_Base + 0x08) >> 2] = m_Section_CRC32[ubSectorIdx];
		if (m_Section_Sec_Cnt[ubSectorIdx] == 0)
		{
			memset(&pCPDataS[(ulFWSector_Base) >> 1], 0x00, 16);
		}
	}
#if 0	// 開不開code都起的來
	// Set Target Base
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0x80 + 0x0C) >> 2] = 0x00000000;		//ATCM at CPU 0x00000000
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0x90 + 0x0C) >> 2] = 0x22180000;		//CODEBANK at CPU 0x22180000
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0xA0 + 0x0C) >> 2] = 0x000C0000;		//BTCM at CPU 0x000C0000
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0xB0 + 0x0C) >> 2] = 0x00281000;		//Andes at CPU 0x00280000 
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0xC0 + 0x0C) >> 2] = 0x22180000 + 48 * 1024;	//FIP at CPU 0x000C0000
#endif
																				/*FW Slot0 Signature(Digest) Page Index*/
	pCPDataS[(CP_FWSLOT0_OFFSET + 0x06) >> 1] = uwPageIdx;
	uwPageIdx = uwPageIdx + 0x01;	//已寫了Signature, Idx+1

	//Pad page 
	pIdPageC[0x70] = pCPDataS[(CP_FWSLOT0_OFFSET + 0x06) >> 1] + 1;
	pIdPageC[0x71] = 0x00;

	//Code Pointer Info
	pIdPageS[0x24 >> 1] = pCPDataS[(CP_FWSLOT0_OFFSET + 0x06) >> 1] + 1 + 1;//For another Pad Page

	/*0x04 -- CRC32 value of 4KB Code Pointer*/
	pCPDataI[0x04 >> 2] = 0x00000000;
	pCPDataI[0x04 >> 2] = ldpcecc.E13_FW_CRC32((unsigned int*)pCPDataC, 4096);
	memcpy(buf, pCPDataC, sizeof(CPdata));
}

void USBSCSI5017_ST::CalculatePerFrameDummy(int ldpcMode)
{
	int paritySize = 0;
	switch (ldpcMode)
	{
	case 0: 
		paritySize = 256;
		break;
	case 1:
		paritySize = 240;
		break;
	case 2:
		paritySize = 224;
		break;
	case 3:
		paritySize = 208;
		break;
	case 4:
		paritySize = 176;
		break;
	case 5:
		paritySize = 144;
		break;
	case 6:
		paritySize = 128;
		break;
	case 7:
		paritySize = 304;
		break;
	case 8:
		paritySize = 288;
		break;
	}
	int frameSize = 4096 + 32 + (paritySize * 2);
	int frameCounts = _fh->PageSize / frameSize;
	int totalDummy = _fh->PageSize - (frameSize*frameCounts);
	int averageDummy = (totalDummy / frameCounts) / 2 * 2;
	int index = 0;
	if (ldpcMode == 7)
		index = 0xD0;
	else
		index = 0x68;
	for (int frameNum = 0; frameNum < frameCounts; frameNum++) {
		pIdPageC[index + frameNum * 2] = averageDummy;
		pIdPageC[index + frameNum * 2 + 1] = averageDummy >> 8;
		if (frameNum == (frameCounts - 1)) {
			pIdPageC[index + frameNum * 2] = (totalDummy - (averageDummy * (frameCounts - 1)));
			pIdPageC[index + frameNum * 2 + 1] = (totalDummy - (averageDummy * (frameCounts - 1))) >> 8;
		}
	}
}

int USBSCSI5017_ST::USB_SetSnapWR(int ldpcMode)
{
	if (!_user_perFrame_dummy)
		return 0;

	unsigned char ncmd[512] = { 0 };
	unsigned char buf[512] = { 0 };
	ncmd[0] = 0xD1;  //Byte0
	unsigned int temp = 512 / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43
	ncmd[0x30] = 0x57;  //B48

	int index = 0;
	if (ldpcMode == 7)
		index = 0xD0;
	else
		index = 0x68;

	for (int i = 0x32; i <= 0x3D; i++) {
		if (i > 0x33 && i < 0x38)
			continue;
		ncmd[i] = pIdPageC[index];
		index++;
	}

	/*if (ldpcMode == 7) {
		ncmd[0x32] = pIdPageC[0x68];
		ncmd[0x33] = pIdPageC[0x69];
		ncmd[0x34] = pIdPageC[0x6A];
		ncmd[0x35] = pIdPageC[0x6B];
		ncmd[0x36] = pIdPageC[0x6C];
		ncmd[0x37] = pIdPageC[0x6D];
		ncmd[0x38] = pIdPageC[0x6E];
		ncmd[0x39] = pIdPageC[0x6F];
	}*/
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, 512, false, 20);
	return result;
}

void USBSCSI5017_ST::PrepareIDpageForRDTBurner(int target_interface, int target_clock, int userSpecifyBlkCnt)
{
	pIdPageC[IP_FLASH_ID + 0] = _fh->bMaker;
	pIdPageC[IP_FLASH_ID + 1] = _fh->id[0];
	pIdPageC[IP_FLASH_ID + 2] = _fh->id[1];
	pIdPageC[IP_FLASH_ID + 3] = _fh->id[2];
	pIdPageC[IP_FLASH_ID + 4] = _fh->id[3];
	pIdPageC[IP_FLASH_ID + 5] = _fh->id[4];

	//if (0x9B == _fh->bMaker) {
	//	pIdPageC[IP_VENDOR_TYPE] = UNIV_YMTC;
	//}
	//else if(0xAD == _fh->bMaker) {
	//	pIdPageC[IP_VENDOR_TYPE] = UNIV_SKHYNIX;
	//}
	//else if(0x89 == _fh->bMaker || 0x2C == _fh->bMaker) {
	//	pIdPageC[IP_VENDOR_TYPE] = UNIV_INTELMICRON;
	//}
	//else if(0x98 == _fh->bMaker || 0x45 == _fh->bMaker) {
	//	pIdPageC[IP_VENDOR_TYPE] = UNIV_TOSHIBASANDISK;
	//}
	//else {
	//	pIdPageC[IP_VENDOR_TYPE] = UNIV_ALL_SUPPORT_VENDOR_NUM;
	//}


	if (_fh->bIsBics2) {
		pIdPageC[IP_FLASH_TYPE] = TOSHIBA_BICS2;
	}
	else if (_fh->bIsBics3 || _fh->bIsBics3pMLC || _fh->bIsBiCs3QLC) {
		pIdPageC[IP_FLASH_TYPE] = TOSHIBA_BICS3;
	}
	else if (_fh->bIsBics4) {
		pIdPageC[IP_FLASH_TYPE] = TOSHIBA_BICS4;
	}
	else if (_fh->bIsBiCs4QLC) {
		pIdPageC[IP_FLASH_TYPE] = TOSHIBA_QLC_BICS4;
	}
	else if (_fh->bIsMicronB16) {
		pIdPageC[IP_FLASH_TYPE] = MICRON_B16A;
	}
	else if (_fh->bIsMicronB17) {
		pIdPageC[IP_FLASH_TYPE] = MICRON_B17A;
	}
	else if (_fh->bIsBics3 && 0x45 == _fh->bMaker) {
		pIdPageC[IP_FLASH_TYPE] = SANDISK_BICS3;
	}
	//else if (_fh->bIsYMTCMLC3D || _fh->bIsYMTC3D ) {
	//	pIdPageC[IP_FLASH_TYPE] = YMTC_GEN2;
	//}
	else if (_fh->bIsMIcronN18) {
		pIdPageC[IP_FLASH_TYPE] = MICRON_N18A;
	}
	else if (_fh->bIsMicronB27 && 0xA2 == _fh->id[3]) {
		pIdPageC[IP_FLASH_TYPE] = MICRON_B27A;
	}
	else if (_fh->bIsMIcronN28) {
		pIdPageC[IP_FLASH_TYPE] = MICRON_N28A;
	}
	else if (0x98 == _fh->bMaker && 0x15 == _fh->Design && 0x04 == _fh->Cell) {
		pIdPageC[IP_FLASH_TYPE] = TOSHIBA_1Z_MLC;
	}
	else if (0x45 == _fh->bMaker && 0x15 == _fh->Design && 0x04 == _fh->Cell) {
		pIdPageC[IP_FLASH_TYPE] = SANDISK_1Z_MLC;
	}
	else if (_fh->bIsHynix3DV4) {
		pIdPageC[IP_FLASH_TYPE] = HYNIX_3DV4;
	}
	else if (_fh->bIsMicronB27 && 0xE6 == _fh->id[3]) {
		pIdPageC[IP_FLASH_TYPE] = MICRON_B27B;
	}
	else if (_fh->bIsMIcronB05) {
		pIdPageC[IP_FLASH_TYPE] = MICRON_B05A;
	}
	else if (_fh->bIsHynix3DV5) {
		pIdPageC[IP_FLASH_TYPE] = HYNIX_3DV5;
	}
	else if (_fh->bIsHynix3DV6) {
		pIdPageC[IP_FLASH_TYPE] = HYNIX_3DV6;
	}

	int page_number_ratio = 1;
	if (0x02 == _fh->Cell) {
		//pIdPageC[IP_CELL_TYPE] = UNIV_FLH_SLC;
		page_number_ratio = 1;
	}
	else if (0x04 == _fh->Cell) {
		//pIdPageC[IP_CELL_TYPE] = UNIV_FLH_MLC;
		page_number_ratio = 2;
	}
	else if (0x08 == _fh->Cell) {
		//pIdPageC[IP_CELL_TYPE] = UNIV_FLH_TLC;
		page_number_ratio = 3;
	}
	else if (0x010 == _fh->Cell) {
		//pIdPageC[IP_CELL_TYPE] = UNIV_FLH_QLC;
		page_number_ratio = 4;
	}

	if (_fh->bIsYMTC3D) {
		pIdPageC[IP_SPECIFY_FUNTION] = pIdPageC[IP_SPECIFY_FUNTION] | SPECIFY_BIT_ENABLE_6_ADD;
	}

	if ('T' == _fh->bToggle) {
		pIdPageC[IP_DEFAULT_INTERFACE] = FLH_INTERFACE_T1;
	}
	else {
		pIdPageC[IP_DEFAULT_INTERFACE] = FLH_INTERFACE_LEGACY;
	}

	pIdPageC[IP_TARGET_INTERFACE] = target_interface;

	pIdPageC[IP_FLASH_CLOCK] = target_clock;
	pIdPageC[IP_FLASH_DRIVE] = GetPrivateProfileIntA("E13", "FlashDriving", 2, _ini_file_name); //0:UNDER 1:NORMAL 2:OVER
	pIdPageC[IP_FLASH_ODT] = GetPrivateProfileIntA("E13", "FlashODT", 4, _ini_file_name); //0:off 1:150 2:100 3:75 4:50 (ohm)
	pIdPageC[IP_CONTROLLER_DRIVING] = GetPrivateProfileIntA("E13", "ControlDriving", 4, _ini_file_name);//0:50 1:37.5 2:35 3:25 4:21.43 (ohm)
	pIdPageC[IP_CONTROLLER_ODT] = GetPrivateProfileIntA("E13", "ControlODT", 3, _ini_file_name);//0:150 1:100 2:75 3:50 4:0ff (ohm)

	pIdPageC[IP_DIE_PER_CE] = _fh->DieNumber;
	pIdPageC[IP_PLANE_PER_DIE] = _fh->Plane;

	int BlockCntPerPlane = _fh->Block_Per_Die / _fh->Plane;
	if (-1 != userSpecifyBlkCnt) {
		BlockCntPerPlane = userSpecifyBlkCnt / _fh->Plane;
	}
	pIdPageC[IP_BLOCK_PER_PLANE << 1] = BlockCntPerPlane & 0xFF;
	pIdPageC[(IP_BLOCK_PER_PLANE << 1) + 1] = (BlockCntPerPlane >> 8) & 0xFF;
	pIdPageC[IP_BLOCK_NUM_FOR_DIE_BIT << 1] = (_fh->DieOffset) & 0xFF;
	pIdPageC[(IP_BLOCK_NUM_FOR_DIE_BIT << 1) + 1] = (_fh->DieOffset >> 8) & 0xFF;
	pIdPageC[IP_D1_PAGE_PER_BLOCK << 1] = _fh->PagePerBlock & 0xFF;
	pIdPageC[(IP_D1_PAGE_PER_BLOCK << 1) + 1] = (_fh->PagePerBlock >> 8) & 0xFF;
	pIdPageC[IP_PAGE_PER_BLOCK << 1] = (_fh->PagePerBlock*page_number_ratio) & 0xFF;
	pIdPageC[(IP_PAGE_PER_BLOCK << 1) + 1] = ((_fh->PagePerBlock*page_number_ratio) >> 8) & 0xFF;
	pIdPageC[IP_DATA_BYTE_PER_PAGE << 1] = (_fh->PageSizeK * 1024) & 0xFF;
	pIdPageC[(IP_DATA_BYTE_PER_PAGE << 1) + 1] = ((_fh->PageSizeK * 1024) >> 8) & 0xFF;
	pIdPageC[IP_FLASH_BYTE_PER_PAGE << 1] = (_fh->PageSize) & 0xFF;
	pIdPageC[(IP_FLASH_BYTE_PER_PAGE << 1) + 1] = (_fh->PageSize >> 8) & 0xFF;
	pIdPageC[IP_FRAME_PER_PAGE] = _fh->PageSizeK / 4;
	pIdPageC[IP_PAGE_NUM_PER_WL] = page_number_ratio;
}

int USBSCSI5017_ST::PreparePadpage(unsigned char * buf, unsigned char * odt_setting)
{
	//PAD_WE
	buf[1946] = odt_setting[15];
	buf[1947] = 0;
	buf[1948] = odt_setting[15];
	buf[1949] = 0;

	//PAD_RE
	buf[1954] = odt_setting[16];
	buf[1955] = 0;
	buf[1956] = odt_setting[16];
	buf[1957] = 0;

	//PAD_DQS
	buf[1962] = ((odt_setting[11] & 0x7) << 5) | ((odt_setting[10] & 0x1) << 4) | (odt_setting[17] & 0xf);
	buf[1963] = 0;
	buf[1964] = ((odt_setting[11] & 0x7) << 5) | ((odt_setting[10] & 0x1) << 4) | (odt_setting[17] & 0xf);
	buf[1965] = 0;

	//PAD_DAT
	buf[1970] = ((odt_setting[11] & 0x7) << 5) | ((odt_setting[10] & 0x1) << 4) | (odt_setting[18] & 0xf);
	buf[1971] = 0;
	buf[1972] = ((odt_setting[11] & 0x7) << 5) | ((odt_setting[10] & 0x1) << 4) | (odt_setting[18] & 0xf);
	buf[1973] = 0;

	//PAD_ALE
	buf[2010] = odt_setting[13] << 2;
	buf[2011] = odt_setting[13] << 2;

	//PAD_CLE
	buf[2014] = odt_setting[14] << 2;
	buf[2015] = odt_setting[14] << 2;

	//PAD_FCE
	buf[2018] = odt_setting[21] << 2;
	buf[2019] = odt_setting[21] << 2;

	//PAD_FWP
	buf[2022] = odt_setting[20] << 2;
	buf[2023] = odt_setting[20] << 2;

	//PAD_FRDY
	buf[2026] = odt_setting[19] << 2;
	buf[2027] = odt_setting[19] << 2;

	//ODT_VALUE
	buf[2030] =  ((odt_setting[6] & 0x7) << 5) | ((odt_setting[5] & 0x1) << 4)
		       | ((odt_setting[4] & 0x7) << 1) | (odt_setting[3] & 0x1);

	//DV_VALUE
	if(_fh->bMaker == 0xad)
		buf[2031] = ((((odt_setting[9]+1)*2) & 0x1F) << 1) | (odt_setting[8] & 0x1);
	else
		buf[2031] = ((odt_setting[9] & 0x1F) << 1) | (odt_setting[8] & 0x1);

	//CLK_Rate_READ
	buf[2032] = odt_setting[1];
	buf[2033] = 0;

	//CLK_Rate_WRITE
	buf[2034] = odt_setting[2];
	buf[2035] = 0;

	//WarmUP_Cycle_READ
	buf[2036] = odt_setting[35];
	//WarmUP_Cycle_WRITE
	buf[2037] = odt_setting[36];

	return 0;
}

int USBSCSI5017_ST::USB_ISPProgFlash(unsigned char * buf, unsigned int BuferSize, bool mScrambled, bool mLast, unsigned int m_Segment)
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD1;  //Byte0
	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43
	ncmd[0x30] = 0x30;  //B48
	ncmd[0x31] = mLast << 2;//B49
	ncmd[0x36] = m_Segment;//B54
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, false, 20);
	return result;
}

int USBSCSI5017_ST::USB_ISPReadFlash(unsigned char * buf, unsigned int BuferSize, bool mScrambled, bool mLast, unsigned int m_Segment)
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD2;  //Byte0
	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43
	ncmd[0x30] = 0xC0;  //B48
	//ncmd[0x31] = mLast << 2;//B49
	//ncmd[0x36] = m_Segment;//B54
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, true, 20);
	return result;
}

int USBSCSI5017_ST::USB_Get_Rdt_Log(unsigned char * buf, unsigned int BuferSize, int table, int page)
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD2;  //Byte0
	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43
	ncmd[0x30] = 0xF1;  //B48
	ncmd[0x31] = table;//B49
	ncmd[0x34] = page & 0xFF;//B52
	ncmd[0x35] = (page >> 8) & 0xFF;//B53
	ncmd[0x36] = (page >> 16) & 0xFF;//B54
	ncmd[0x37] = (page >> 24) & 0xFF;//B55

#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, true, 20);
	return result;
}

int USBSCSI5017_ST::USB_Scan_Rdt_Log()
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD0;  //Byte0
	ncmd[0x30] = 0xF3;  //B48

#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, NULL, 0, false, 20);
	return result;
}

int USBSCSI5017_ST::GetFwVersion(char * version/*I/O*/)
{
	return 0;
}

bool USBSCSI5017_ST::Check5017CDBisSupport(unsigned char *CDB)
{
	if (running_mode == fw_mode)
		return TRUE;
	if((CDB[0] != 0x06) || (CDB[1] != 0xF6))
		return FALSE;
	for(int i=0; i<OPCMD_COUNT_5017; i++) {
		if(CDB[2] == OpCmd5017[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

int USBSCSI5017_ST::NT_CustomCmd_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool is512CDB)
{
	int res = 0;
	int cdbDebug = GetPrivateProfileInt("Sorting","CDBDebug",0,_ini_file_name);
	int checkSupportCmd5017 = GetPrivateProfileInt("Sorting","CheckSupportCmd5017",1,_ini_file_name);

	unsigned char cdb512[512] = {0};
	if(!Check5017CDBisSupport(CDB)  && checkSupportCmd5017) {
		nt_file_log("CDB [0x%x 0x%x 0x%x 0x%x] not support.", CDB[0], CDB[1], CDB[2], CDB[3]);
		return 0;
	}

	if (is512CDB)
		memcpy(cdb512, CDB, 512);
	else
		memcpy(cdb512, CDB, 16);
	
	if(running_mode == fw_mode)
		res = MyIssueCmd(cdb512, direc, buf, iDataLen, timeOut);
	else
		res = Issue512CdbCmd(cdb512, buf,  iDataLen, direc, timeOut);

	if(cdbDebug) {
		if(res) {
			_pSortingBase->Log("[CDB]%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X, rtn=%d",
				CDB[0], CDB[1], CDB[2], CDB[3], CDB[4], CDB[5],
				CDB[6], CDB[7], CDB[8], CDB[9], CDB[10], CDB[11],
				CDB[12], CDB[13], CDB[14], CDB[15], res);
		}
	}
	return res;
}

#define PREFORMAT_ERROR_INFO_BLOCK_ERROR	(0xF0)
#define PREFORMAT_ERROR_NOT_ENOUGH_D3UNIT	(0xF1)
#define PREFORMAT_ERROR_NOT_ENOUGH_D1UNIT	(0xF2)
#define PREFORMAT_ERROR_NOT_ENOUGH_TOTALUNIT	(0xF3)
#define PREFORMAT_ERROR_NOT_ENOUGH_FREEBLK	(0xF4)

int USBSCSI5017_ST::Nvme3PassVenderCmd(unsigned char * cdb512, unsigned char * iobuf, int length, int direction, int timeout) 
{
	int result = 0;
	unsigned char usbCmd[512] = { 0 };

	result = SCSISendSQ(cdb512, timeout);
	if (SCSI_IOCTL_DATA_UNSPECIFIED == direction)
	{
		result = SCSIWriteReadData(false, NULL, 0, timeout);
	}
	else if (SCSI_IOCTL_DATA_IN == direction)
	{
		result = SCSIWriteReadData(true, iobuf, length, timeout);
	}
	else
	{
		result = SCSIWriteReadData(false, iobuf, length, timeout);
	}
	if (cdb512[0x30] == 0x10) { // Polling FW preformat status
		bool isPreformating = true;
		while (isPreformating)
		{
			result = SCSIReadCQ(usbCmd, 2000);
			if (usbCmd[0x03] == 0)
			{
				isPreformating = false;
			}
			char stime[9];
			_strtime(stime);
			Log(LOGDEBUG, "[%s] preformat status: %d ,error code: %d \n",stime, usbCmd[0x03], usbCmd[0x04]);
			if (usbCmd[0x04]) {
				if (usbCmd[0x04] == PREFORMAT_ERROR_INFO_BLOCK_ERROR)
					Log(LOGDEBUG, "!!!!!!!!!!!!!!!!!!!!!PREFORMAT_ERROR_INFO_BLOCK_ERROR\n");
				else if (usbCmd[0x04] == PREFORMAT_ERROR_NOT_ENOUGH_D3UNIT)
					Log(LOGDEBUG, "!!!!!!!!!!!!!!!!!!!!!PREFORMAT_ERROR_NOT_ENOUGH_D3UNIT\n");
				else if (usbCmd[0x04] == PREFORMAT_ERROR_NOT_ENOUGH_D1UNIT)
					Log(LOGDEBUG, "!!!!!!!!!!!!!!!!!!!!!PREFORMAT_ERROR_NOT_ENOUGH_D1UNIT\n");
				else if (usbCmd[0x04] == PREFORMAT_ERROR_NOT_ENOUGH_TOTALUNIT)
					Log(LOGDEBUG, "!!!!!!!!!!!!!!!!!!!!!PREFORMAT_ERROR_NOT_ENOUGH_TOTALUNIT\n");
				else if (usbCmd[0x04] == PREFORMAT_ERROR_NOT_ENOUGH_FREEBLK)
					Log(LOGDEBUG, "!!!!!!!!!!!!!!!!!!!!!PREFORMAT_ERROR_NOT_ENOUGH_FREEBLK\n");
			}
			Sleep(1000);
		}
		result = usbCmd[0x04];
	}
	else {
		result = SCSIReadCQ(usbCmd, timeout);
	}

	return result;
}

int USBSCSI5017_ST::SCSISendSQ(unsigned char *buf, int timeout)
{
	unsigned int bufLength = 512;
	UCHAR    cdb[16] = { 0 };
	cdb[0] = 0x06;                 // SECURITY PROTOCOL Out
	cdb[1] = 0xFE;                 // security_protocol
	cdb[2] = 0xC0;                 // security_protocol_speciric & 0xFF;
	cdb[3] = (((bufLength >> 9) >> 8) & 0xFF);
	cdb[4] = ((bufLength >> 9) & 0xFF);
	cdb[5] = ((bufLength >> 24) & 0xFF);
	cdb[6] = ((bufLength >> 16) & 0xFF);
	cdb[7] = ((bufLength >> 8) & 0xFF);
	cdb[8] = ((bufLength) & 0xFF);
	
	int result = MyIssueCmd(cdb, false, buf, bufLength, timeout, false);
	return result;
}
int USBSCSI5017_ST::SCSIWriteReadData(bool read, unsigned char *buf, unsigned int bufLength, int timeout)
{
	UCHAR    cdb[16] = { 0 };
	cdb[0] = 0x06;                 // SECURITY PROTOCOL Out
	cdb[1] = 0xFE;                 // security_protocol
	if (read)
		cdb[2] = 0xC2;                 // security_protocol_speciric & 0xFF;
	else
		cdb[2] = 0xC1;
	cdb[3] = (((bufLength >> 9) >> 8) & 0xFF);
	cdb[4] = ((bufLength >> 9) & 0xFF);
	cdb[5] = ((bufLength >> 24) & 0xFF);
	cdb[6] = ((bufLength >> 16) & 0xFF);
	cdb[7] = ((bufLength >> 8) & 0xFF);
	cdb[8] = ((bufLength) & 0xFF);

	int result = MyIssueCmd(cdb, read, buf, bufLength, timeout, false);
	return result;
}
int USBSCSI5017_ST::SCSIReadCQ(unsigned char *buf, int timeout)
{
	UCHAR    cdb[16] = { 0 };
	cdb[0] = 0x06;                 // SECURITY PROTOCOL Out
	cdb[1] = 0xFE;                 // security_protocol
	cdb[2] = 0xC3;                 // security_protocol_speciric & 0xFF;
	cdb[3] = 0;
	cdb[4] = 1;
	int result = MyIssueCmd(cdb, true, buf, 512, timeout, false);
	if (buf[15] != 0)
		return 1;
	return result;
}

int USBSCSI5017_ST::Dev_IssueIOCmd(const unsigned char* sql, int sqllen, int mode, int ce, unsigned char* buf, int len)
{
	int hmode = mode & 0xF0;
	mode = mode & 0x0F;

	if ((mode & 0x03) == 1) {
		/*if (_SetPattern(buf, len, ce)) {
			Log(LOGDEBUG, "Set Pattern fail \n");
			return 1;
		}*/
		if (_WriteDDR(buf, len, ce)) {
			Log(LOGDEBUG, "Write DDR fail \n");
			return 1;
		}
		//_TrigerDDR(ce, sql, sqllen, len, _workingChannels);
	}

	//#ifdef DEBUGSATABIN
	//	if ( _debugCmdPause ) {
	//		cmd->Log ( LOGDEBUG, "triger DDR command pattern:\n" );
	//		cmd->LogMEM(sql,sqllen);
	//		cmd->Log ( LOGDEBUG, "press any key to triger DDR\n" );
	//		getch();
	//	}
	//#endif

	if (!hmode) {
		if (_TrigerDDR(ce, sql, sqllen, len, _workingChannels)) {
			Log(LOGDEBUG, "Trigger DDR fail \n");
			//FFReset();
			//Log(LOGDEBUG, "!!!!!!!!!!!FFReset \n");
			return 2;
		}
		//#ifdef DEBUGSATABIN
		//Log ( LOGDEBUG, "Trigger ce %d DDR time consume: %ul \n", ce, std::clock()-cstart );
		//#endif

		if (mode == 2) {
			//#ifdef DEBUGSATABIN
			//			if ( _debugCmdPause ) {
			//				cmd->Log ( LOGDEBUG, "read DDR command pattern:\n" );
			//				cmd->LogMEM(sql,sqllen);
			//				cmd->Log ( LOGDEBUG, "press any key to read DDR\n" );
			//				getch();
			//			}
			//#endif
			if (_ReadDDR(buf, len, 0)) {
				Log(LOGDEBUG, "Read DDR fail \n");
				return 3;
			}
		}
		else if (mode == 3) {
			//#ifdef DEBUGSATABIN
			//			if ( _debugCmdPause ) {
			//				cmd->Log ( LOGDEBUG, "read DDR command pattern:\n" );
			//				cmd->LogMEM(sql,sqllen);
			//				cmd->Log ( LOGDEBUG, "press any key to read DDR\n" );
			//				getch();
			//			}
			//#endif
			unsigned char tmpbuf[512];
			if (_ReadDDR(tmpbuf, 512, 2)) {
				Log(LOGDEBUG, "Read DDR fail \n");
				return 3;
			}
			memcpy(buf, tmpbuf, len);
		}
	}
	return 0;
}

int USBSCSI5017_ST::_TrigerDDR(int CE, const unsigned char* sqlbuf, int slen, int buflen, unsigned char channel)
{
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	/*int ret = USB_SetAPKey();
	if (ret) {
		return ret;
	}*/
	
	unsigned char ncmd[512] = { 0 };
	//unsigned char GetLength = 16;
	//memset(ncmd, 0x00, 16);

	int newlen = _SQLMerge(sqlbuf, slen);

	ncmd[0] = 0xD1;
	ncmd[0x28] = 0x80;	//B40
	ncmd[0x30] = 0x52;	//B48
	ncmd[0x34] = newlen & 0xFF;  //B52
	ncmd[0x35] = (newlen >> 8) & 0xFF;  //B53
	ncmd[0x36] = (newlen >> 16) & 0xFF;;  //B54
	ncmd[0x37] = (newlen >> 24) & 0xFF;;  //B55	
	ncmd[0x3A] = CE;  //B58
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, (unsigned char*)sqlbuf, newlen, false, 20);
	return result;
}

int USBSCSI5017_ST::_WriteDDR(unsigned char* w_buf, int Length, int ce)
{
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	/*int ret = USB_SetAPKey();
	if (ret) {
		return ret;
	}*/
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD1;  //Byte0
	int tempLength = Length / 4;
	ncmd[0x28] = tempLength & 0xFF;	//B40
	ncmd[0x29] = (tempLength >> 8) & 0xFF;	//B41
	ncmd[0x2A] = (tempLength >> 16) & 0xFF;	//B42
	ncmd[0x2B] = (tempLength >> 24) & 0xFF;	//B43
	ncmd[0x30] = 0x51;	//B48
	ncmd[0x34] = Length & 0xFF;  //B52
	ncmd[0x35] = (Length >> 8) & 0xFF;  //B53
	ncmd[0x36] = (Length >> 16) & 0xFF;  //B54
	ncmd[0x37] = (Length >> 24) & 0xFF;  //B55	
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, w_buf, Length, false, 20);
	return result;
}



//datatype: 0=data, 1=vender, 2=ecc 3=time, 4=time+reset
int USBSCSI5017_ST::_ReadDDR(unsigned char* r_buf, int Length, int datatype)
{
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	/*int ret = USB_SetAPKey();
	if (ret) {
		return ret;
	}*/

	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD2;  //Byte0
	int tempLength = Length / 4;
	ncmd[0x28] = tempLength & 0xFF;	//B40
	ncmd[0x29] = (tempLength >> 8) & 0xFF;	//B41
	ncmd[0x2A] = (tempLength >> 16) & 0xFF;	//B42
	ncmd[0x2B] = (tempLength >> 24) & 0xFF;	//B43
	ncmd[0x30] = 0xD1;	//B48
	ncmd[0x31] = datatype;	//B48
	ncmd[0x34] = Length & 0xFF;  //B52
	ncmd[0x35] = (Length >> 8) & 0xFF;  //B53
	ncmd[0x36] = (Length >> 16) & 0xFF;;  //B54
	ncmd[0x37] = (Length >> 24) & 0xFF;;  //B55	
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, r_buf, Length, true, 20);
	return result;
}

int USBSCSI5017_ST::FFReset()
{
	unsigned char cdb[512] = { 0 };
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x09;

#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	unsigned char buf[512] = { 0 };
	return Issue512CdbCmd(cdb, buf, 512, SCSI_IOCTL_DATA_OUT, _SCSI_DelayTime);
	//int result = Nvme3PassVenderCmd(ncmd, (unsigned char*)sqlbuf, newlen, false, 20);
	//return result;
}



void USBSCSI5017_ST::saveToFile(unsigned char * buf, int Length, int fileName, bool write)
{
	char file_name[128] = {0};
	if (write) {
		sprintf_s(file_name, "write/PAGE_%d.bin", fileName);
	}
	else {
		sprintf_s(file_name, "verify/PAGE_%d.bin", fileName);
	}
	errno_t err;
	FILE *fp;
	err = fopen_s(&fp,file_name, "wb");
	fwrite(buf, 1, Length, fp);
	fclose(fp);
}

int USBSCSI5017_ST::checkRunningMode()
{
	unsigned char InqBuf[528] = { 0 };
	char fwVersion[8] = { 0 };
	HANDLE handle = this->Get_SCSI_Handle();
	if (handle == INVALID_HANDLE_VALUE) return -1;
	int result = 0;
	int retryCount = 0;
	//while (true) 
		result = this->Inquiry(handle, InqBuf, true, 0xff);

		DWORD Error = GetLastError();
		//result = UFWSCSICmd::Inquiry(_info->physical, InqBuf, true);
		//if (result == 0)
		//	break;
		//Sleep(200);

		//this->MyCloseHandle(&handle);
		//this->MyCreateFile_Phy(this->_info->physical);
		//handle = this->Get_SCSI_Handle();
		//retryCount++;
		//if (10 <= retryCount)
		//	break;
		//printf("~~~~~~~~~~~~~ %d \n", retryCount);
	//}
	//result = UFWInterface::Inquiry(handle, InqBuf, true);
	//int result = UFWSCSICmd::Inquiry(_info->physical, InqBuf, true);
	if (result)
		return get_running_mode_fail;
	if ((memcmp((char *)&InqBuf[0x10], "5017 PRAM", 9) == 0)) {
		running_mode = boot_mode;
	} else if((memcmp((char *)&InqBuf[0x10], "5017 SQLC", 9) == 0)) {
		running_mode = burner_mode;
	}
	else if ((memcmp((char *)&InqBuf[0x14], "RDT", 3) == 0)) {
		running_mode = rdt_mode;
	}
	else if (InqBuf[0x7f] == 0xba) {
		running_mode = fw_mode;
	} else {
		running_mode = fw_not_support;
	}
	return running_mode;
}

int USBSCSI5017_ST::ST_Relink(unsigned char type)
{
	unsigned char r_buf[512] = { 0 };
	unsigned char cdb[16];
	memset(cdb, 0, 16);
	cdb[0] = 0x06;
	cdb[1] = 0x00;
	cdb[2] = 0xcd;
	cdb[3] = type;

	int err = 0;
	err = MyIssueCmdHandle(Get_SCSI_Handle(), cdb, true, r_buf, cdb[4], 10, IF_USB);
	return err;
}

int USBSCSI5017_ST::GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &tWHR2)
{
	if (40 == flh_clk_mhz)
	{
		tADL = 6;  tWHR = 0;  tWHR2 = 0;
	}
	else if (  (100 >= flh_clk_mhz) || (133 == flh_clk_mhz) )
	{
		tADL = 11;  tWHR = 0;  tWHR2 = 11;
	}
	else if ( 166 == flh_clk_mhz )
	{
		tADL = 15;  tWHR = 0;  tWHR2 = 15;
	}
	else if ( (200 == flh_clk_mhz) || (266 == flh_clk_mhz) )
	{
		tADL = 24;  tWHR = 4;  tWHR2 = 24;
	}
	else if ( 266 == flh_clk_mhz )
	{
		tADL = 30;  tWHR = 6;  tWHR2 = 30;
	}
	else if ((300 == flh_clk_mhz) || (333 == flh_clk_mhz) )
	{
		tADL = 41;  tWHR = 11;  tWHR2 = 41;
	}
	else if ( 350 == flh_clk_mhz )
	{
		tADL = 43;  tWHR = 11;  tWHR2 = 43;
	}
	else if ( 400 == flh_clk_mhz )
	{
		tADL = 51;  tWHR = 14;  tWHR2 = 50;
	}
	else if ( 466 == flh_clk_mhz )
	{
		tADL = 60;  tWHR = 18;  tWHR2 = 60;
	}
	else if ( 600 == flh_clk_mhz )
	{
		tADL = 81;  tWHR = 27;  tWHR2 = 81;
	}
	else if ( 700 == flh_clk_mhz )
	{
		tADL = 131;  tWHR = 40;  tWHR2 = 130;
	}
	else {
		nt_file_log("GetACTimingTable:can't get table for flash clk %d Mhz", flh_clk_mhz);
		assert(0);
	}
	return 0;
}

int USBSCSI5017_ST::NT_ReadPage_All(BYTE bType, HANDLE DeviceHandle_p, BYTE bDrvNum, const char *type, unsigned char *bBuf, unsigned char *DriveInfo)
{
	nt_log("%s", __FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[16] = { 0 };
	DWORD dwByte = 528;

	if (bBuf == NULL) return 1;

	memset(bBuf, 0, 512);	// ? 528
	memset(bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0x06;
	bCBD[1] = 0x05;
	bCBD[8] = 0x80;
	_realCommand = 1;
	if (type)
		memcpy(&bCBD[2], type, 6);

	_ForceNoSwitch = 1;
	if (bType == 0)
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 16, SCSI_IOCTL_DATA_IN, dwByte, bBuf, 30);
	}
	else if (bType == 1) {
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 16, SCSI_IOCTL_DATA_IN, dwByte, bBuf, 30);
	}
	else if (bType == 2) {
		iResult = ST_CustomCmd_Path(DriveInfo, bCBD, 16, SCSI_IOCTL_DATA_IN, dwByte, bBuf, 30);
	}
	else return 0xFF;

	return iResult;
}

SSD_ODTInfo* SSD_ODTInfo::_inst = 0;

inline bool _IsODTTag( char* ftable )
{
	return ('@'== ftable[0] && 'T'== ftable[1] && 'T'== ftable[2] && '@'== ftable[3]);
}
inline bool _IsENDTag( char* ftable )
{
	return ('@'== ftable[0] && 'E'== ftable[1] && 'N'== ftable[2] && 'D'== ftable[3] && '@'== ftable[4] );
}

inline unsigned char _HexToByte(const char *s)
{
	return
		(((s[0] >= 'A' && s[0] <= 'Z') ? (10 + s[0] - 'A') :
		(s[0] >= 'a' && s[0] <= 'z') ? (10 + s[0] - 'a') :
		(s[0] >= '0' && s[0] <= '9') ? (s[0] - '0') : 0) << 4) |
		((s[1] >= 'A' && s[1] <= 'Z') ? (10 + s[1] - 'A') :
		(s[1] >= 'a' && s[1] <= 'z') ? (10 + s[1] - 'a') :
		(s[1] >= '0' && s[1] <= '9') ? (s[1] - '0') : 0);
}
const unsigned char* SSD_ODTInfo::GetParamByLine(int index)
{
	return _datas[index];
}
const unsigned char* SSD_ODTInfo::GetParam(const char* ctrl, const char* flash, const char* model, unsigned char die, unsigned char readrate, unsigned char writerate )
{
	int clen = strlen(ctrl);
	int flen = strlen(flash);
	int mlen = strlen(model);
	for( int r=0; r<_rowCnt; r++ ) {
		char* key = _keys[r];
		if ( strncmp(key,ctrl,clen) ) {
			continue;
		}

		key=key+clen+1;
		if ( strncmp(key,flash,flen) ) {
			continue;
		}

		key=key+flen+1;
		if ( strncmp(key,model,mlen) ) {
			continue;
		}

		if ( die!=_datas[r][0] ) {
			continue;
		}

		if ( 0xFF!=readrate ) {
			if ( readrate!=_datas[r][1] ) {
				continue;
			}
		}

		if ( 0xFF!=writerate ) {
			if ( writerate!=_datas[r][2] ) {
				continue;
			}
		}

		return _datas[r];
	}
	return 0;
}

SSD_ODTInfo* SSD_ODTInfo::Instance(const char* fn)
{
	if ( _inst ) return _inst;

	FILE* op = fopen( fn, "rb" );
	if ( !op ) {
		//_DEBUGINFO("file not exist: %ls\n",fn);
		return 0;
	}
	fseek ( op, 0, SEEK_END );
	int lSize = (int)ftell (op);
	rewind (op);
	char* ftable = (char*)malloc(lSize);
	int len = (int)fread(ftable,1,lSize,op);
	fclose(op);

	int startIdx=-1;
	int endIdx=-1;
	int recCnt=0;
	for ( int i=0; i<lSize; i++ ) {
		if ( -1== startIdx) {
			if ( _IsODTTag(&ftable[i]) ) {
				startIdx = i;
				recCnt++;
			}
			continue;
		}
		else {
			if ( _IsENDTag(&ftable[i]) ) {
				endIdx = i;
				break;
			}

			if ( _IsODTTag(&ftable[i]) ) {
				recCnt++;
			}
		}
	}

	//_DEBUGINFO("SSD_ODTInfo StartIdx=%08X, EndIdx=%08X recCnt=%d\n",startIdx,endIdx,recCnt);

	_inst = new SSD_ODTInfo();
	_inst->_rowCnt = recCnt;
	_inst->_keys = new char*[_inst->_rowCnt];
	_inst->_datas = new unsigned char*[_inst->_rowCnt];
	for ( int i=0; i<_inst->_rowCnt; i++ ) {
		_inst->_keys[i] = 0;
		_inst->_datas[i] = 0;
	}

	recCnt=0;
	for ( int i=startIdx; i<endIdx; i++ ) {
		if ( !_IsODTTag(&ftable[i]) ) continue;

		int colCnt = 0;
		int starCol = 0;
		for ( int r=i; i<endIdx; r++ ) {
			if ( ','==ftable[r] ) {
				colCnt++;
				if ( 1==colCnt ) {
					starCol=r+1;
				}

				if ( 4!=colCnt )  {
					continue;
				}
				int endCol = r;
				int len = endCol-starCol;

				char* keydata = new char[len+1];
				memcpy(keydata,&ftable[starCol],len);
				keydata[len] = '\0';
				//_DEBUGINFO("keydata=%s\n",keydata);
				_inst->_keys[recCnt]=keydata;

				starCol = r+1;
				for( int r2=r+1; r2<=endIdx; r2++) {
					if ( '\n'==ftable[r2] ) {
						i=r; //faster;
						endCol = r2-2;
						break;
					}
				}

				////for delete use
				len = endCol-starCol;
				assert( (len%2)==0 );

				// char* sdata = new char[len+1];
				// memcpy(sdata,&ftable[starCol],len);
				// sdata[len] = '\0';
				// _DEBUGINFO("sdata=%s\n",sdata);

				//_DEBUGINFO("idata[%d]=",recCnt);
				unsigned char* data = new unsigned char[len];
				memset(data,0,len);
				for ( int l=0; l<len/2; l++ ) {
					data[l]=_HexToByte(&ftable[starCol+l*2]);
					//_DEBUGINFO("%02X",data[l]);
				}
				//_DEBUGINFO("\n");
				_inst->_datas[recCnt]=data;
				recCnt++;
				assert( recCnt<=_inst->_rowCnt);
			}
			else if ( '\n'==ftable[r] ) {
				i=r; //faster
				break;
			}
		}
	}

	free (ftable);
	return _inst;
}

void SSD_ODTInfo::Free()
{
	if ( _inst ) {
		delete _inst;
		_inst = 0;
	}
}

SSD_ODTInfo::SSD_ODTInfo()
: _keys(0)
, _datas(0)
{
	return;
}

SSD_ODTInfo::~SSD_ODTInfo()
{
	if ( !_keys ) {
		return;
	}

	for( int i=0; i<_rowCnt; i++ ) {
		if ( _keys[i] ) {
			delete[] _keys[i];
		}
		if ( _datas[i] ) {
			delete[] _datas[i];
		}
	}
	delete[] _keys;
	delete[] _datas;
}


