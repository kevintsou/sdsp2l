
#include "USBSCSICmd_ST.h"
#include "U3Def.h"
//#include <ctime> 
#include <winioctl.h>
#include <time.h>

#include "utils/RetryTable.h"
#ifdef OutputDebugString
#undef OutputDebugString
#define OutputDebugString(x) 
#endif

#pragma warning(disable : 4996)


static int g_context_id = 0;

USBSCSICmd_ST::USBSCSICmd_ST( DeviveInfo* info, const char* logFolder )
:USBSCSICmd(info,logFolder)
{
    Init();
}

USBSCSICmd_ST::USBSCSICmd_ST( DeviveInfo* info, FILE *logPtr )
:USBSCSICmd(info,logPtr)
{
    Init();
}

USBSCSICmd_ST::USBSCSICmd_ST()
:USBSCSICmd(&_default_drive_info,(FILE *)NULL)
{
//	memset(_default_drive_info,0, sizeof(DeviveInfo));
	Init();
}

void USBSCSICmd_ST::CloseSocket()
{
	if ( _socket ) {
		delete _socket;
		_socket = 0;
	}
}

bool USBSCSICmd_ST::InitSocket( const char* addr, int port )
{
	try {
		_socket = new myQtTcpSocket(port,0);
		_socket->connectToServer(addr);
		return true;
	}catch(myQtSocketException* excp)  {
		Log (LOGDEBUG,"%s %s\n",__FUNCTION__,excp->getErrMsg());
		delete excp;
	}
	return false;
}

int USBSCSICmd_ST::MyIssueCmd(  unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout,int useBridge )
{
	int res = 0;
	DWORD lasterr = 0;
	for (int i = 0; i < 2; i++) {
		if (_useNet) {
			//socket 
			res = MyIssueCmdHandle(_hUsb, cdb, read, buf, bufLength, timeout, _deviceType);
		}
		else {
			res = MyIssueCmdHandle(_hUsb, cdb, read, buf, bufLength, timeout, _deviceType);
		}
		if (!res) break;
		Sleep(1000);
		if (cdb[0x02] == 0xc2 || cdb[0x02] == 0xc1) {
			Log(LOGDEBUG, "!!!!!!!!!!!!!!!!!!!!!!!!!Received C1/C2 command fail res:%d  cdb[0x03]:%x !!!!!!!!!!!!!!!!!!\n", res, cdb[0x02]);
		}
		Log(LOGDEBUG, "!!!!!!!!!!!!!!!!!!!!!!!!!Received return command fail res:%d  cdb[0x03]:%x !!!!!!!!!!!!!!!!!!\n", res, cdb[0x02]);
		lasterr = ::GetLastError();
		::SetLastError(0);
	}
	return res;
}


USBSCSICmd_ST::~USBSCSICmd_ST()
{
	CloseSocket();

	if(_bankBufTotal){
		delete []_bankBufTotal;
		_bankBufTotal = 0;
	}
	if(mCMD2BankMapping_NEW)
	{
		for (int i = 0;i < 0xff+1;i++)
		{
			if(mCMD2BankMapping_NEW[i])
			{
				delete[] mCMD2BankMapping_NEW[i];
				mCMD2BankMapping_NEW[i] = 0;
			}

		}
		delete[] mCMD2BankMapping_NEW;
		mCMD2BankMapping_NEW = 0;
	}
	/*this handle will be close in outside*/
	if (_isOutSideHandle)
		_hUsb = INVALID_HANDLE_VALUE;

	_pSortingBase = NULL;
}

void USBSCSICmd_ST::Init()
{
	_socket = 0;
	memset(&_default_drive_info, 0, sizeof(_default_drive_info));
	_LogStartTime = 0;

	bSDCARD=0;
	_creatFromNet=0;
	_context_id = g_context_id++;
	memset(m_CMD,0,24);
	m_physicalDriveIndex = -1;
	_alwaySyncBank = FALSE;
	_SetDumpCDB = FALSE;
	_forceWriteCount = 0;
	bSCSIControl = 1;
	_retryIoControl = true;
	FlashIdIndex = 0;
	_SCSI_DelayTime = 100;
	currentBank = 0;
	BankSize = 0;
	BankCommonSize = 0;
	_fh = NULL;
	_bankBufTotal = NULL;
	isFWBank = false;
	_realCommand = 0;
	_doFlashReset = TRUE;
	_doFDReset = TRUE;
	memset(_forceID, 0x00, 8);
	_debugBankgMsg = 0;
	_ForceNoSwitch = 0;
	m_recovered_handle = false;
	_isIOing = false;
	_isCmdSwitching = false;
	_requestedSuspend = false;
	_ThreadStatus = 0;
	_bankBufTotal= new unsigned char[DEF_BANK_BUR_BUFSIZE];
	memset(_bankBufTotal,0, DEF_BANK_BUR_BUFSIZE);
	_CloseTimeout = false;
	m_cmdtype = SCSI_NTCMD;
	_hUsb = INVALID_HANDLE_VALUE;
	//m_cmdtype = SSD_NTCMD;
	//memset(m_LogCmdCount,0,sizeof(m_LogCmdCount));
	_idxOfHub = -1;
	_ActiveCe = -1;
	_LastIndex=-1;
	memset(_win_start, 0xFF, sizeof(_win_start));
	memset(_win_end, 0xFF, sizeof(_win_end));
	m_flashfeature = FEATURE_LEGACY;
	_deviceType = 0;
	mCMD2BankMapping_NEW = new BYTE*[0xff+1];
	for(int i = 0; i < 0xff+1; ++i)
	{
		if(mCMD2BankMapping_NEW){
			mCMD2BankMapping_NEW[i] = new BYTE[10];		
		}
		else{
			assert(0);
		}
		memset(mCMD2BankMapping_NEW[i],0xff,10);		//預設為-1
	}
	memset(_offline_qc_report_cnt, 0, sizeof(_offline_qc_report_cnt));
	memset(_fakeRanges, -1, sizeof(_fakeRanges));
	memset(_pageMapping, 0, sizeof(_pageMapping));
	logCallBack = NULL;
	_nvmeHandle = INVALID_HANDLE_VALUE;
	memset(_fwVersion, 0, sizeof(_fwVersion));
	m_ATAIO = USBONLY;
	m_isSQL=false;
	_fileNvme = NULL;
	_pSortingBase=NULL;
	_max_nvme_recording_sizeK = 4;
	SetInterFaceType(I_NVME);
	memset(_flash_id_boot_cache, 0, sizeof(_flash_id_boot_cache));
	_doFrontPagePercentage=0;
	_isOutSideHandle = false;
	memset(&_PE_blocks[0][0][0], 0xFF, sizeof(_PE_blocks));
	_LogStartTime = clock();
}

#define _BIT_MAP_VALID 0
#define _BIT_MAP_INVALID 1

bool USBSCSICmd_ST::BitValid(const unsigned char* bitmap, int idx, const unsigned char* bitmap2, const unsigned char* bitmap3)
{
	bool v = (bitmap[idx / 8] >> (idx % 8) & 0x01) == _BIT_MAP_VALID;
	if (!v) {
		if (bitmap2) {
			v = (bitmap2[idx / 8] >> (idx % 8) & 0x01) == _BIT_MAP_VALID;
			if (!v) {
				if (bitmap3) {
					v = (bitmap3[idx / 8] >> (idx % 8) & 0x01) == _BIT_MAP_VALID;
				}
			}
		}
	}
	return v;
}

bool USBSCSICmd_ST::SetBitInValid(unsigned char* bitmap, int idx)
{
	bitmap[idx / 8] |= (1 << (idx % 8));
	return 0;
}

int USBSCSICmd_ST::GetPageShiftBits()
{
	//int pages = _fh->MaxPage;	//WL
	_FH_Paramater *_fh = &_FH;
	int pages = _fh->PagePerBlock;
	int PageBits = 0;
	bool bB95A = false;
	if ((_fh->bMaker == 0x2c || _fh->bMaker == 0x89) && _fh->Design == 0x16 && _fh->beD3)
		bB95A = true;
	else if (_fh->beD3 && !bB95A && ((_fh->bMaker == 0x2C) || (_fh->bMaker == 0x89) || (_fh->bMaker == 0x9B))) {	//20180717 B95A不乘3
		if (_fh->Cell == 0x10)	//QLC
			pages = pages * 4;
		else
			pages = pages * 3;
	}
	int TotalPage = pages;
	if (_fh->bL06_TO_B0k) {
		TotalPage = pages = 1024;
	}
	pages -= 1; //ex 512 page 最大為511
	while (pages > 0) {
		pages = pages >> 1;
		PageBits++;
	}
	if (_fh->beD3 && (PageBits<8)) {
		PageBits = 8;
	}

	return PageBits;
}

int USBSCSICmd_ST::Dev_SQL_GroupWR_Setting(int blockSeed, int pageSeed, bool isWrite, int LdpcMode, char Spare)
{
	return 0;
}
int USBSCSICmd_ST::GetLogicalCEByPhy(int phySlot, int phyCE, int phyDie, int phyPBADie)
{
	int _slotCnt = 1;
	phyCE = phyCE / _slotCnt;
	phySlot = 0;
	//return ((phySlot*_ceCnt+phyCE)*_dieCnt+phyDie)*_pbaDieCnt+phyPBADie;
	return ((phySlot*_FH.bPBADie + phyPBADie)*_FH.DieNumber + phyDie)*_FH.FlhNumber + phyCE;
}

int USBSCSICmd_ST::GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &tWHR2)
{
	return 0;
}


//
//int USBSCSICmd_ST::EraseBlockAll(unsigned long long fun, unsigned char start_ce, unsigned char ce_count, unsigned char RWMode, BYTE PageType
//	, int die, int phyPBADie, int die_startblock, int die_blockcount, DWORD functionTest, unsigned char **statusbuf)
//{
//
//	assert(start_ce >= 0);
//	assert((die_startblock + die_blockcount) <= _FH.Block_Per_Die);
//
//	_FH.AllBadBlock;
//	int planemode = 1;
//	if ((RWMode & _TWO_PLANE_MODE_) == _TWO_PLANE_MODE_)
//		planemode = 2;
//	if ((RWMode & _TWO_PLANE_MODE_) == _FOUR_PLANE_MODE_)
//		planemode = 4;
//
//	BYTE sql_buf[512] = { 0 };
//
//	for (int RealCe = start_ce; RealCe < (start_ce + ce_count); RealCe++) {
//		//if (SrtManager.GetIsMultiCE()) 
//		//{
//		//	for (unsigned int ce = 0; ce < _FH.FlhNumber; ce++) {
//		//		bCEMask &= ~(1 << ce); // bit -- 0要測 1 不測
//		//	}
//		//	RealCe = _FH.FlhNumber;
//		//}
//		//else {
//		//	bCEMask &= ~(1 << RealCe);
//		//}
//		for (int die = 0; die < _FH.DieNumber; die++) {
//
//			for (int pbadie = 0; pbadie < _FH.bPBADie; pbadie++) {
//				//this->FL_SwitchPBADie(pbadie);
//				//int lce = SrtManager.GetLogicalCEByPhy(0, RealCe, die, pbadie);	//not real lce, because not count block yet
//				if (_FH.bisExPBA) {
//					BYTE w_buf[512] = { 0 };
//					this->ST_PBADieGetSet(1, this->Get_SCSI_Handle(), 0, 0, pbadie, _FH.beD3 ? 1 : 0, _FH.bisExPBA == 2 ? 1 : 0, w_buf);
//				}
//
//				int lce = GetLogicalCEByPhy(0, RealCe, die, pbadie);
//				for (int block = die_startblock; block < (die_startblock + die_blockcount); block += planemode) {
//
//					this->ST_DirectOneBlockErase(1, this->Get_SCSI_Handle(), 0, 0, RealCe, block, 0, 512, sql_buf);
//					//check status
//					//tempLog.sprintf("Erase CE: %d   Block: %d", ce, codeBlock);
//					//ProgressRecord(tempLog, true);
//					if (statusbuf != nullptr) {
//						SetBitInValid(&statusbuf[lce][0], block);
//					}
//				}
//			}
//		}
//	}
//	return 0;
//}



bool FORCE_SCSI = false;
#define  NES_BASECMD_TIMEOUT  400
void CNTCommand::log_nvme( const char* fmt, ... )
{
	if (!_fileNvme)
		return;

	char buf[4096];

	va_list arglist;
	//CTime time( CTime::GetCurrentTime() );   // Initialize CTime with current time
	//int hh = time.GetHour();
	//int mm = time.GetMinute();
	//int ss = time.GetSecond();

	va_start( arglist, fmt );
	_vsnprintf( buf, 4096, fmt,arglist );
	//sprintf(buf2, "%02d:%02d:%02d:%s\n", hh, mm, ss , buf);
	fprintf(_fileNvme, "%s", buf);
	va_end( arglist );
	fflush(_fileNvme);
}

void CNTCommand::LogMEM ( char* op, const unsigned char* mem, int size )
{
	sprintf ( op, "\n%s-----------------------%d------------------------\n",op ,size );
	sprintf ( op, "%s----- 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n",op );
	sprintf ( op, "%s-----------------------------------------------------\n",op );
	int row = 0;
	char line[8192],buf[20];
	sprintf ( line,"" );
	for ( int i=0; i<size; i++ )  {
		if ( i==0 || ( i%16 ) ==0 ) {
			sprintf ( buf,"%04x0 ", row );
			strcat ( line,buf );
			row++;
		}
		sprintf ( buf,"%02x ", mem[i] );
		strcat ( line,buf );
		if ( ( i%16 ) ==15 ) {

			for (int j=15; j>=0; j--) {
				unsigned char ascii = mem[i-j];
				if ((ascii>='0'&& ascii<='9') || (ascii>='a'&& ascii<='z') || (ascii>='A'&& ascii<='Z')) {
					//if (ascii>=2 && ascii<=127) {
					sprintf(buf,"%c", ascii);
					strcat ( line,buf );
				} else {
					sprintf(buf,".");
					strcat ( line,buf );
				}
			}
			strcat ( line,"\n" );
			sprintf ( op,"%s%s",op, line );
			sprintf ( line,"" );
		}
	}
	strcat ( line,"\n" );
	sprintf ( op,"%s%s",op, line );
}

void CNTCommand::nt_log(const char *fmt, ... )
{
#ifdef LOG_NT_FUN
	char buf[256];
	char buf2[512];
	va_list arglist;

	va_start( arglist, fmt );
	_vsnprintf( buf, 255, fmt,arglist );
	if (SSD_NTCMD == m_cmdtype)
		sprintf(buf2,"[%2d](%s) %s\n",_context_id-1,"SSD ",buf);
	else if(SCSI_NTCMD == m_cmdtype)
		sprintf(buf2,"[%2d](%s) %s\n",_context_id-1,"SCSI",buf);
	va_end( arglist );
#ifdef TAKE_OUT_NT_LOG
	if (strstr(buf2,"NTCommand"))
		return;
#endif
	OutputDebugString( buf2 );
#endif
}



bool CNTCommand::CheckIfValidFlashId(HANDLE h)
{
	unsigned char idbuf[512] = {0};
	if (h == NULL || h == INVALID_HANDLE_VALUE)
		return false;
	if (NT_GetFlashExtendID(1, h, NULL, idbuf))
		return false;
	unsigned int id_fail = 0;
	for (int i = 0; i<5; i++){
		if (idbuf[i] == 0)
			id_fail++;
	}
	if (id_fail)
		nt_file_log("check id:%x %x %x %x %x", idbuf[0], idbuf[1], idbuf[2], idbuf[3], idbuf[4]);
	if(5==id_fail)
		return false;
	return true;
}


void CNTCommand::SetIniFileName (char * ini)
{
	sprintf(_ini_file_name, "%s", ini);
}


void CNTCommand::nt_log_cdb(const unsigned char * cdb,const char *fmt, ... )
{
#ifdef LOG_NT_FUN
	char buf[256];
	char buf2[512];
	va_list arglist;

	va_start( arglist, fmt );
	_vsnprintf( buf, 255, fmt,arglist );
	if (SSD_NTCMD == m_cmdtype)
		sprintf(buf2,"[%2d](%s) %s, %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",_context_id,"SSD ",buf,cdb[0],cdb[1],cdb[2],cdb[3],cdb[4],cdb[5],cdb[6],cdb[7],cdb[8],cdb[9],cdb[10],cdb[11],cdb[12],cdb[13],cdb[14],cdb[15]);
	else if(SCSI_NTCMD == m_cmdtype)
		sprintf(buf2,"[%2d](%s) %s, %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",_context_id,"SCSI",buf,cdb[0],cdb[1],cdb[2],cdb[3],cdb[4],cdb[5],cdb[6],cdb[7],cdb[8],cdb[9],cdb[10],cdb[11],cdb[12],cdb[13],cdb[14],cdb[15]);
	va_end( arglist );
#ifdef TAKE_OUT_NT_LOG
	if (strstr(buf2,"NTCommand"))
		return;
#endif
	OutputDebugString( buf2 );
#endif
}

bool CNTCommand::IsSwitching(void) {
	return _isCmdSwitching;
}
int CNTCommand::GetCtxId(){
	return _context_id;
}
int CNTCommand::SetSuspend(bool suspend){
	if (_requestedSuspend == suspend)
		return 0;
	_requestedSuspend = suspend;
	while (_isIOing) {Sleep (100);};
	//if (_pSortingBase)
	//_pSortingBase->Log("SetSuspend %d", suspend);
	//suspend();
	return 0;
}

INTERFACE_TYPE CNTCommand::GetInterFaceType(void){
	return _if_type;
}
void CNTCommand::SetInterFaceType(INTERFACE_TYPE _if) {
	_if_type = _if;
}

int CNTCommand::GetFwVersion(char * version/*I/O*/)
{//no implement
	return 0;
}
void CNTCommand::SetATAIOControl(IOCONTROLTYPE ata){
	m_ATAIO = ata;
}
IOCONTROLTYPE CNTCommand::GetATAIOControlType()
{
	return m_ATAIO;
}
void CNTCommand::SetLogCallBack(void (*logcbfun)(char * logstring))
{
	logCallBack = logcbfun;
}

void CNTCommand::SetNvmeRecordingSize(int sizeK)
{
	_max_nvme_recording_sizeK = sizeK;
}
void CNTCommand::SetFakeBlock( const int * ranges )
{
	_fakeRanges[0] = ranges[0];
	_fakeRanges[1] = ranges[1];
	_fakeRanges[2] = ranges[2];
	_fakeRanges[3] = ranges[3];
}


int CNTCommand::GetActiveCe()
{
	//if (_creatFromNet)//ATA Sorting for NTCommandNET should always return 0 otherwise DebugServer will crash. (array index out of range.)
	//return 0;
	return _ActiveCe;
}

void CNTCommand::SetActiveCe(int ce)
{
	_ActiveCe = ce;
}

void CNTCommand::SetAlwasySync(BOOL sync)
{
	_alwaySyncBank = sync;
}

void CNTCommand::SetDumpCDB(BOOL Dump)
{
	_SetDumpCDB = Dump;
}

void CNTCommand::SetSortingBase(BaseSorting* sb)
{
	_pSortingBase = sb;
}

int CNTCommand::NT_SwitchCmd(int obType, HANDLE handle, char device_letter,CMDTYPE target_type,FH_Paramater * FH , BYTE * ap_parameter, bool auto_find_handle, bool isWriteflh, BYTE bChangeMode)
{
	if (target_type == m_cmdtype){
		OutputDebugString("Warning: Cmd type the same!");
		return 1;
	}
	nt_log("#########  Cmd Switch to %s  ########",target_type == SCSI_NTCMD?"SCSI":"SSD"); 
	//if (SCSI_NTCMD == target_type)
	m_cmdtype = target_type;
	return 0;
}
CMDTYPE CNTCommand::getCmdType(){
	return m_cmdtype;
}

int CNTCommand::SetAPKey_ATA(HANDLE devicehandle)
{
	return 1;
}

int CNTCommand::NT_CustomCmd_ATA_Phy_orig(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut)
{
	return 0;
}

//
//CNTCommand::CNTCommand(int PortId)
//{
//	memset(m_CMD,0,24);
//	_forceWriteCount = 0;
//	bSCSIControl = 1;
//	FlashIdIndex = 0;
//	_SCSI_DelayTime = 100;
//	currentBank = 0;
//	BankSize = 0;
//	_fh = NULL;
//	_bankBufTotal = NULL;
//	isFWBank = false;
//	_realCommand = 0;
//	_doFlashReset = TRUE;
//	_debugBankgMsg = 0;
//	_ForceNoSwitch = 0;
//	_bankBufTotal= new unsigned char[DEF_BUR_BUFSIZE];
//	memset(_bankBufTotal,0, DEF_BUR_BUFSIZE);
//
//}
//Assign bankBuf directly or burner name
void CNTCommand::NetCommandSetAllVirtual(int){
	return ;
}

unsigned char * CNTCommand::GetBankBuf(){
	return _bankBufTotal;
	/*if(_bankBufTotal!=NULL){
		memcpy(buf,_bankBufTotal,size);
	}
	else{
		buf[0] = 'N';
		buf[1] = 'B';
	}*/
}
void  CNTCommand::SetFlashParameter(FH_Paramater * fh)
{
	_fh = fh;
	memcpy((void *)&_FH, fh, sizeof(FH_Paramater));
	nt_file_log("Flash id=%x %x %x %x %x",_fh->id[0],_fh->id[1],_fh->id[2],_fh->id[3],_fh->id[4]);
}
int CNTCommand::BankInit(BYTE bType,HANDLE DeviceHandle_p,BYTE bDrvNum ,_FH_Paramater *_FH,unsigned char *bankBuf)
{
	BYTE temp[528] = {0};
	this->isFWBank = false;
	this->_fh = _FH;

	NT_Inquiry_All(bType,DeviceHandle_p,bDrvNum, temp);
	
	if (memcmp((char *)&temp[0x15], "HV Tester", 4)==0)		// In Sorting Code
	{
	//	this->isFWBank = false;
		//This Command must be in Common Bank
		ST_GetCodeVersionDate(bType,DeviceHandle_p,bDrvNum, temp);
		BYTE CodeMark=temp[12];	
		if(CodeMark == 'z')			//有code，且為Bank code
		{
			if (bankBuf == NULL)
				return 1;
			this->isFWBank = true;
			memcpy(_bankBufTotal,bankBuf,DEF_BANK_BUR_BUFSIZE);
			BankSize = (int)_bankBufTotal[0x20];
			BankCommonSize = (int)_bankBufTotal[0x10] - BankSize;
			BankInfoCreateFromFile();
			ST_BankSynchronize(bType,DeviceHandle_p,bDrvNum);
			return 0;
		}
		else
		{
			this->isFWBank = false;
			this->_fh = NULL;
			return 0;
		}
	}
	else{
		this->isFWBank = false;
		this->_fh = NULL;
		return 0;
	}
	
	return 0;
}
int CNTCommand::BankFileLoad(char *fileName)
{
	FILE * fp = NULL;
	unsigned long filesize;
	char *mpMark = (char *)"this is mp mark";
	BOOL result=0;
	//_bankBufTotal = new unsigned char[DEF_BUR_BUFSIZE];

	unsigned long codefilesize=DEF_BANK_BUR_BUFSIZE-2048;
	
	memset( _bankBufTotal, 0, DEF_BANK_BUR_BUFSIZE);
	fp = fopen(fileName, "rb");

	if (!fp)
		return -1;

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (filesize >codefilesize)
	{
		fclose(fp);
		return -1;
	}

	if (fread(_bankBufTotal, 1, filesize, fp) <= 0)
	{
		fclose(fp);
		return -1;
	}
	if (fp)
		fclose(fp);

	if( (_bankBufTotal[0]!='B')||(_bankBufTotal[1]!='t') )
	{
		memset(_bankBufTotal, 0, sizeof(_bankBufTotal));
		return 1;
	}

	if(memcmp(_bankBufTotal+(filesize-512),mpMark,15)==0)
		filesize -= 512;//this burner marked by may , need reduce 512 bytes


	if(filesize==0)
		return 1;

	return 0;
}

int CNTCommand::NT_GetFW_CodeAddr(BYTE bType,HANDLE DeviceHandle_p,char bDrvNum,unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12]; 
	DWORD dwByte = 8;    

	if( CDB==NULL ) return 1;

	memset( bBuf, 0, 8);
	memset( CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xDD;
	
	if(bType == 0)
		iResult = NT_CustomCmd(bDrvNum, CDB, 16 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf,10);
	Sleep(80);

	return iResult;
}


int CNTCommand::NT_Get_CharInfo(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12]; 
	DWORD dwByte = 1024;    

	if( CDB==NULL ) return 1;

	memset( bBuf, 0, 1024);
	memset( CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 0x52;//'R';
	CDB[3] = 0x41;//'A';
	CDB[4] = 0x5A;
	CDB[5] = 0x00;

	if(bType == 0)
		iResult = NT_CustomCmd(bDrvNum, CDB, 16 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf,10);
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_Get_FAB(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12]; 
	DWORD dwByte = 1024;    

	if( CDB==NULL ) return 1;

	memset( bBuf, 0, 1024);
	memset( CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 0x52;//'R';
	CDB[3] = 0x41;//'A';
	CDB[4] = 0xF1;
	CDB[5] = 0x50;

	if(bType == 0)
		iResult = NT_CustomCmd(bDrvNum, CDB, 16 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf,10);
	Sleep(80);

	return iResult;
}


unsigned long CNTCommand::BootCheckSum(unsigned char * buf, unsigned int numberOfByte)
{
	unsigned long checkSum = 0;
	unsigned long index=0;

	for(index=0; index<numberOfByte; index++)
	{
		if((index == 106) || (index == 107) || (index == 112))
		{
			continue;
		}
		checkSum = ((checkSum&1) ?0x80000000 : 0) + (checkSum>>1) + (unsigned int)buf[index];
	}
	return checkSum;
}


int CNTCommand::GetHardDiskSn(char *sHsn) 
{

	//IDSECTOR Is;
	//GethardDiskInfo(&Is);
	//memcpy(sHsn , Is.sSerialNumber,20);
	return 0;
}

int CNTCommand::ReadInfoBlock(BYTE obType,BYTE *bInfoBlock,char targetDisk,HANDLE DeviceHandle ,BYTE InfoNumber,DWORD ICType,BYTE bFC1,BYTE bFC2)
{
	if(InfoNumber == FIRST_INFO)//讀第1份
		return NT_ReadPage_All(obType,DeviceHandle,targetDisk,"INFO",bInfoBlock);
	if(InfoNumber == SECOND_INFO)//讀第2份;
	{
		if((ICType == ICTYPE_DEF_PS2280 && bFC1 == 0xff && bFC2 == 0x0a) ||
		   (ICType == ICTYPE_DEF_PS228X && bFC1 == 0xff && bFC2 == 0x0C) ||
		   (ICType == ICTYPE_DEF_PS2285 && bFC1 == 0xff && bFC2 == 0x0E) ||
		   (ICType == ICTYPE_DEF_PS2313 && bFC1 == 0xff && bFC2 == 0x0C) ||
		   (ICType == ICTYPE_DEF_PS2316 && bFC1 == 0xff && bFC2 == 0x0C) ||
		   (ICType == ICTYPE_DEF_PS2301) || (ICType == ICTYPE_DEF_PS2307 ) ||  (ICType == ICTYPE_DEF_PS2308 ))
			return NT_ReadPage_All(obType,DeviceHandle,targetDisk,"IF23",bInfoBlock);
		else
			return NT_ReadPage_All(obType,DeviceHandle,targetDisk,"IF2F",bInfoBlock);
	}
	else
		return 0x01;
}


int CNTCommand::NT_BootRomDisable_EDO(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	BYTE temp[16] = {0};
	memset(CDB, 0, sizeof(CDB));
//06 23 00 50 68 49 F2 0C 04
	CDB[0] = 0x06;
	CDB[1] = 0x23;
	CDB[2] = 0x00;
	CDB[3] = 0x50;
	CDB[4] = 0x68;
	CDB[5] = 0x49;
	CDB[6] = 0xF2;
	CDB[7] = 0x0C;
	CDB[8] = 0x04;

	


	if(bType == 0)
		iResult = NT_CustomCmd(bDrvNum, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL,10);
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_FlashBitConfiguration(BYTE bType,HANDLE DeviceHandle,char targetDisk, int bitType)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	BYTE temp[16] = {0};
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x57;
	CDB[2] = 0x43;
	CDB[3] = 0x4E;
	CDB[4] = 0x46;

	CDB[5] = bitType;	//0x85;


	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_IN, 16, temp, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 16, temp, 10);
	Sleep(80);

	return iResult;

}

int CNTCommand::NT_ModeSense( BYTE bType,HANDLE DeviceHandle,char targetDisk, unsigned char *buf )
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )	return 1;

	memset( buf, 0, 18);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x5A;
	CDB[2] = 0xBF;	//0x85;
	CDB[7] = 1;
	CDB[8] = 40;

	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_IN, 18, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 18, buf, 10);
	Sleep(80);

	return iResult;
}
int CNTCommand::NT_WriteVerify_Sectors(char targetDisk,unsigned long address, int secNo, unsigned char *buf, 
						    bool fRead, bool hidden, BYTE blspec ,HANDLE hDeviceHandle,bool isover1T)	// ???
{

	BYTE bRdBuf[128*512];
	if(fRead==true) return 1;
	if( secNo>128 ) return 2;
	//BYTE bHdSpec = 0;
	//if(blspec)
	//	bHdSpec = 1;
	if (hDeviceHandle != NULL)
	{

		memset( bRdBuf, 0, sizeof(bRdBuf));
		if( NT_readwrite_sectors_H(hDeviceHandle, address, secNo, buf, fRead, hidden, blspec,isover1T) )
			return 0x10;
		if( NT_readwrite_sectors_H(hDeviceHandle, address, secNo, bRdBuf, true, hidden, blspec,isover1T) )
			return 0x11;


		//test for debug
		//for(int i=0;i<secNo*512;i++)
		//{
		//	if( buf[i] != bRdBuf[i] )
		//	{
		//		return 0x12;
		//	}
		//}
		if( memcmp(buf, bRdBuf, secNo*512)!=0 )
		{
			return 0x12;
		}

	}
	else
	{

		if( NT_readwrite_sectors(targetDisk, address, secNo, buf, fRead, hidden, blspec,isover1T) )
			return 0x10;
		if( NT_readwrite_sectors(targetDisk, address, secNo, bRdBuf, true, hidden, blspec,isover1T) )
			return 0x11;
		if( memcmp(buf, bRdBuf, secNo*512)!=0 )
		{
			return 0x12;
		}

	}
	return 0;
}

int CNTCommand::NT_WriteVerify_Sectors(char targetDisk,unsigned long address, int secNo, unsigned char *buf, 
									   bool fRead, bool hidden, BYTE blspec ,HANDLE hDeviceHandle, int iMun,bool isover1T)	// ???
{
	BYTE bRdBuf[128*512];
	if(fRead==true) return 1;
	if( secNo>128 ) return 2;
	//BYTE bHdSpec = 0;
	//if(blspec)
	//	bHdSpec = 1;
	if (hDeviceHandle != NULL)
	{

		memset( bRdBuf, 0, sizeof(bRdBuf));
		if( NT_readwrite_sectors_H(hDeviceHandle, address, secNo, buf, fRead, hidden, blspec, iMun,isover1T) )
			return 0x10;
		if( NT_readwrite_sectors_H(hDeviceHandle, address, secNo, bRdBuf, true, hidden, blspec, iMun,isover1T) )
			return 0x11;
		if( memcmp(buf, bRdBuf, secNo*512)!=0 )
		{
			return 0x12;
		}

	}
	else
	{

		if( NT_readwrite_sectors(targetDisk, address, secNo, buf, fRead, hidden, blspec, iMun,isover1T) )
			return 0x10;
		if( NT_readwrite_sectors(targetDisk, address, secNo, bRdBuf, true, hidden, blspec, iMun,isover1T) )
			return 0x11;
		if( memcmp(buf, bRdBuf, secNo*512)!=0 )
		{
			return 0x12;
		}

	}
	return 0;
}

HANDLE CNTCommand::MyCreateFile_Phy(BYTE bPhyNum)		
{
#if 1
	DWORD  accessMode1, accessMode2, accessMode3, accessMode4;
	DWORD  shareMode = 0;
	char   devicename[128];
	HANDLE DeviceHandle_p = INVALID_HANDLE_VALUE; // NULL;  <S>

	memset( devicename, 0, sizeof(devicename));
	sprintf (devicename, "\\\\.\\PHYSICALDRIVE%d", bPhyNum);

	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;		// default
	accessMode1 = GENERIC_READ | GENERIC_WRITE;			// default
	accessMode2 = accessMode1 | GENERIC_EXECUTE;		// default
	accessMode3 = accessMode2 | GENERIC_ALL;			// default
	accessMode4 = accessMode3 | MAXIMUM_ALLOWED;		// default

	DeviceHandle_p = CreateFile( devicename , 
		accessMode1 , 
		shareMode , 
		NULL ,
		OPEN_EXISTING , 
		FILE_ATTRIBUTE_NORMAL,		// ? why 
		0 );

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
	{
		DeviceHandle_p = CreateFile( devicename, accessMode2, shareMode, NULL ,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL );
	}
	else return DeviceHandle_p;

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
	{
		DeviceHandle_p = CreateFile( devicename, accessMode3, shareMode, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	}
	else
		return DeviceHandle_p;

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
	{
		DeviceHandle_p = CreateFile( devicename, accessMode4, shareMode, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	}
	else
		return DeviceHandle_p;
#endif

	return INVALID_HANDLE_VALUE;			// <S>
}


int CNTCommand::NT_CardReaderSend(BYTE bType,HANDLE DeviceHandle,BYTE drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	//unsigned char CDB[12];

	if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
//	memset(CDB, 0, sizeof(CDB));
	unsigned char CDB[12]={0x06,0x36,0x70,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0xFF};
	
	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 10);
	Sleep(80);
	return iResult;
}

//Set FW init
int CNTCommand::NT_CardReader_FW_init(BYTE bType,HANDLE DeviceHandle,BYTE drive)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char buf[512]={0};

	if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	//memset(CDB, 0, sizeof(CDB));
	unsigned char CDB[12]={0x06,0x36,0x5D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	Sleep(80);
	return iResult;
}

//Send status command to smart card
int CNTCommand::NT_CardReader_PC2RDR_Status(BYTE bType,HANDLE DeviceHandle,BYTE drive)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char buf[512]={0};
	buf[0] = 0x65;

	if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	//memset(CDB, 0, sizeof(CDB));
	unsigned char CDB[12]={0x06,0x36,0x70,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0xFF};

	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 10);
	Sleep(80);
	return iResult;
}

//get status command to smart card , must send status command first (PC2RDR);
int CNTCommand::NT_CardReader_RDR2PC_Status(BYTE bType,HANDLE DeviceHandle,BYTE drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
		if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	//memset(CDB, 0, sizeof(CDB));
	unsigned char CDB[12]={0x06,0x36,0x71,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0xFF};

	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	Sleep(80);
	return iResult;
}

int CNTCommand::NT_CardReader_ATR_Status(BYTE bType,HANDLE DeviceHandle,BYTE drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	//memset(CDB, 0, sizeof(CDB));
	unsigned char CDB[12]={0x06,0x36,0x5E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	Sleep(80);
	return iResult;
}

int CNTCommand::NT_CardReaderRead(BYTE bType,HANDLE DeviceHandle,BYTE drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	if( buf==NULL )	return 1;
	
	//memset( buf, 0, 528);
	//memset(CDB, 0, sizeof(CDB));
	unsigned char CDB[12]={0x06,0x36,0x71,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0xFF};

	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	Sleep(80);
	return iResult;
}

int CNTCommand::NT_GetOPT(BYTE bType,HANDLE DeviceHandle,BYTE drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )	return 1;

	memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0x5;
	CDB[2] = 'P';
	CDB[3] = 'Y';
	CDB[4] = 0;
	CDB[5] = 0xF6; /* Phy register. */
	CDB[6] = 0;
	CDB[7] = 0;
	CDB[8] = 0;
	CDB[9] = 0;

	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	Sleep(80);
	return iResult;
}

int CNTCommand::NT_TestUnitReady(BYTE bType,HANDLE MyDeviceHandle, char targetDisk , unsigned char *senseBuf )
{
	BOOL status = 0;
	ULONG length = 0, returned = 0;
	unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p = INVALID_HANDLE_VALUE;
	if(bType ==0)
		DeviceHandle_p = MyCreateFile(targetDisk);
	else if(bType == 1)
		DeviceHandle_p = MyDeviceHandle;
	if( senseBuf==NULL ) return 1;
	memset( senseBuf, 0, 18);

//	DeviceHandle_p = MyCreateFile(targetDisk);

	DWORD err=GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE)
		return 0x02;	//err;

	memset(CDB, 0, sizeof(CDB));

	SCSIPtdBuilder(&sptdwb, 6, CDB, SCSI_IOCTL_DATA_IN, 0, 10, NULL);
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);
	err=GetLastError();
	if(bType == 0)
		MyCloseHandle(&DeviceHandle_p);

	memcpy(senseBuf , sptdwb.ucSenseBuf , 18 );
	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return err;
}

int CNTCommand::NT_IspConfigISP_CP(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int cnt, unsigned char mode,BYTE *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xEE;
	CDB[2] = mode;
	CDB[3] = cnt;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_OUT:SCSI_IOCTL_DATA_IN, 72, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_OUT:SCSI_IOCTL_DATA_IN, 72, buf, 10);
	Sleep(80);

	return iResult;
}
int CNTCommand::NT_IspConfigISP_CP30(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int cnt, unsigned char mode,BYTE *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xEE;
	CDB[2] = mode;
	CDB[3] = cnt;
	CDB[4] = 0x48;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_OUT:SCSI_IOCTL_DATA_IN, 72, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_OUT:SCSI_IOCTL_DATA_IN, 72, buf, 10);
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_IspConfigISP(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int length, unsigned char mode, unsigned char verify, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB1;
	CDB[2] = mode;
	CDB[3] = 0;
	CDB[4] = 0;
	CDB[7] = (unsigned char)((length>>8)&0xff);
	CDB[8] = (unsigned char)(length&0xff);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, length<<9, buf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, length<<9, buf, 30);
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_readwrite_sectors_l( char targetDisk, unsigned long address, int secNo, unsigned char *buf, 
						  bool fRead, bool hidden, BYTE blspec ,bool isover1T)
{
	nt_log("%s",__FUNCTION__);
    BOOL status = 0;
    ULONG length = 0 ,  returned = 0;
    unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p = NULL; 
	int count = 0;			
        
	if( buf==NULL ) return 1;
	
	DeviceHandle_p = MyCreateFile(targetDisk);

	DWORD err=GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE)
		return 2;	//err;
        
OnceAgain:
    memset( CDB , 0 , sizeof(CDB));

	if (hidden)
	{
		CDB[0] = 6;
		CDB[1] = fRead ? 0xA : 0xB;	// 0xA:read, 0xB:write
		CDB[9] = 'V';

		if( fRead ) CDB[10] = 'R';
		else CDB[10] = 'W';

		CDB[11] =  blspec;

	} else {
		CDB[0] = fRead ? 0x28 : 0x2A;
		// Magic char for PHISON
		CDB[6] = 'P';
		CDB[9] = 'H';
	}
	
	if (isover1T)
	{
		INT64 aa = address;
		OutputDebugString("NT_readwrite_sectors_l");
		CDB[0] = fRead ? 0x88 : 0x8A;
		CDB[2] = (unsigned char)(( aa >> 56 ) & 0xff );
		CDB[3] = (unsigned char)(( aa >> 48 ) & 0xff );
		CDB[4] = (unsigned char)(( aa >> 40 ) & 0xff );
		CDB[5] = (unsigned char)(( aa >> 32 ) & 0xff );
		CDB[6] = (unsigned char)(( aa >> 24 ) & 0xff );
		CDB[7] = (unsigned char)(( aa >> 16 ) & 0xff );
		CDB[8] = (unsigned char)(( aa >> 8 ) & 0xff );
		CDB[9] = (unsigned char)(( aa ) & 0xff );		
		CDB[10] = (char)( secNo >> 24 );
		CDB[11] = (char)( secNo >> 16 );
		CDB[12] = (char)( secNo >> 8 );
		CDB[13] = secNo;

	}
	else
	{
		//CDB[0] = fRead ? 0x28 : 0x2A;
		CDB[2] = (char)(( address >> 24 ) & 0xff );
		CDB[3] = (char)(( address >> 16 ) & 0xff );
		CDB[4] = (char)(( address >> 8 ) & 0xff );
		CDB[5] = (char)(( address ) & 0xff );
		CDB[7] = (char)( secNo >> 8 );
		CDB[8] = secNo;
	}
    

	SCSIPtdBuilder(&sptdwb, 16, CDB, fRead ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT, (secNo )<<9, 60, buf);
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(DeviceHandle_p,
							IOCTL_SCSI_PASS_THROUGH_DIRECT,
							&sptdwb,
							length,
							&sptdwb,
							length,
							&returned,
							FALSE);

    err = GetLastError();
     
	if (sptdwb.sptd.ScsiStatus && 10 > count)
	{
//		TRACE("Read/Write ScsiStatus: %d\n", sptdwb.sptd.ScsiStatus);
		count++;
		Sleep(100);
		goto OnceAgain;
	}

	memcpy(m_CMD,CDB,16);
	err=GetLastError();

	MyCloseHandle(&DeviceHandle_p);

	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return err;
}
int CNTCommand::NT_readwrite_sectors( char targetDisk,unsigned long address, int secNo, unsigned char *buf, 
						  bool fRead, bool hidden, BYTE blspec ,bool isover1T)
{
	int count = 1;
	unsigned char *readbuf;
	int size = (secNo )<<9;
	readbuf = new unsigned char [size];
	if(_forceWriteCount > 0) count = _forceWriteCount;
	int res = 0;
	for(int i=0;i<count;i++){
		res= NT_readwrite_sectors_l(targetDisk,address,secNo,buf,fRead,hidden,blspec,isover1T);
#if 1
		if(!fRead && _forceWriteCount>1){
	
			res = NT_readwrite_sectors_l(targetDisk,address,secNo,readbuf,true,hidden,blspec,isover1T);
			if(memcmp(readbuf,buf,size)){
				res =1 ;
				continue;
			}
			else break;
		}
	}
#endif
	delete [] readbuf;
	return res;
}

int CNTCommand::NT_readwrite_sectors( char targetDisk,unsigned long long address, int secNo, unsigned char *buf, 
									 bool fRead, bool hidden, BYTE blspec, int iMun ,bool isover1T)
{
	nt_log("%s",__FUNCTION__);
	BOOL status = 0;
	ULONG length = 0 ,  returned = 0;
	unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p = NULL; 
	int count = 0;			

	if( buf==NULL ) return 1;

	DeviceHandle_p = MyCreateFile(targetDisk);

	DWORD err=GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE)
		return 2;	//err;

OnceAgain:
	memset( CDB , 0 , sizeof(CDB));
	if (iMun == 0)
	{
		if (hidden)
		{
			CDB[0] = 6;
			CDB[1] = fRead ? 0xA : 0xB;	// 0xA:read, 0xB:write
			CDB[9] = 'V';

			if( fRead ) CDB[10] = 'R';
			else CDB[10] = 'W';

			CDB[11] =  blspec;

		} else {
			CDB[0] = fRead ? 0x28 : 0x2A;
			// Magic char for PHISON
			CDB[6] = 'P';
			CDB[9] = 'H';
		}
	}
	else
	{
		CDB[0] = 6;
		CDB[1] = fRead ? 0xA : 0xB;	// 0xA:read, 0xB:write
		CDB[6] = iMun;
		CDB[9] = 'V';

		if( fRead ) CDB[10] = 'R';
		else CDB[10] = 'W';

		CDB[11] =  blspec;
	}
	
	if (isover1T)
	{
		INT64 aa = address;
		OutputDebugString("NT_readwrite_sectors ---");
		CDB[0] = fRead ? 0x88 : 0x8A;
		CDB[2] = (unsigned char)(( aa >> 56 ) & 0xff );
		CDB[3] = (unsigned char)(( aa >> 48 ) & 0xff );
		CDB[4] = (unsigned char)(( aa >> 40 ) & 0xff );
		CDB[5] = (unsigned char)(( aa >> 32 ) & 0xff );
		CDB[6] = (unsigned char)(( aa >> 24 ) & 0xff );
		CDB[7] = (unsigned char)(( aa >> 16 ) & 0xff );
		CDB[8] = (unsigned char)(( aa >> 8 ) & 0xff );
		CDB[9] = (unsigned char)(( aa ) & 0xff );		
		CDB[10] = (char)( secNo >> 24 );
		CDB[11] = (char)( secNo >> 16 );
		CDB[12] = (char)( secNo >> 8 );
		CDB[13] = secNo;

	}
	else
	{
		//CDB[0] = fRead ? 0x28 : 0x2A;
		CDB[2] = (char)(( address >> 24 ) & 0xff );
		CDB[3] = (char)(( address >> 16 ) & 0xff );
		CDB[4] = (char)(( address >> 8 ) & 0xff );
		CDB[5] = (char)(( address ) & 0xff );
		CDB[7] = (char)( secNo >> 8 );
		CDB[8] = secNo;
	}
	

	SCSIPtdBuilder(&sptdwb, 12, CDB, fRead ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT, (secNo )<<9, 10, buf);
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);

	err = GetLastError();

	if (sptdwb.sptd.ScsiStatus && 10 > count)
	{
		//		TRACE("Read/Write ScsiStatus: %d\n", sptdwb.sptd.ScsiStatus);
		count++;
		Sleep(100);
		goto OnceAgain;
	}

	err=GetLastError();

	MyCloseHandle(&DeviceHandle_p);

	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return err;
}

//---------------------------------------------------------------------------
int CNTCommand::NT_Password_Op(BYTE bType ,HANDLE DeviceHandle,char drive , unsigned char directFunc 
							   , bool donotWait , unsigned char *id )
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( id==NULL ) return 1;

	memset( CDB , 0 , sizeof(CDB));
	CDB[0] = 0x0E;
	CDB[2] = directFunc;
	if( donotWait )
	{
		CDB[3] = 0x55;
		CDB[4] = 0xAA;
	}

	if(bType ==0)
		iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, id, 40);	// #wait#Lock#
	else
		iResult = NT_CustomCmd_Phy( DeviceHandle, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, id, 40);	// #wait#Lock#


	return iResult;	
}

//---------------------------------------------------------------------------
int CNTCommand::NT_GetDieInformat(BYTE bType ,HANDLE DeviceHandle,char drive ,unsigned char *Buf )
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];



	memset( CDB , 0 , sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x05;
	//if( donotWait )
	
	CDB[2] = 0x44; //"D"
	CDB[3] = 0x49; //"I"
	

	if(bType ==0)
		iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_IN, 528, Buf, 3);	// #wait#Lock#
	else
		iResult = NT_CustomCmd_Phy( DeviceHandle, CDB, 12, SCSI_IOCTL_DATA_IN, 528, Buf, 3);	// #wait#Lock#


	return iResult;	
}

//===========================================================================
int CNTCommand::NT_Disk_Op( BYTE bType ,HANDLE DeviceHandle,char drive, unsigned char directFunc, 
							unsigned int para1 , unsigned char *id)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( id==NULL ) return 1;

	memset( CDB, 0, sizeof(CDB));
	CDB[0] = 0x0D;
	CDB[4] = 8;
	CDB[5] = directFunc;
	CDB[6] = (unsigned char )(para1);
	CDB[7] = (unsigned char )(para1 >> 8 );
	CDB[8] = (unsigned char )(para1 >> 16 );
	CDB[9] = (unsigned char )(para1 >> 24 );

	if(bType==0)
		iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_IN, 8, id, 10);
	else
		iResult = NT_CustomCmd_Phy( DeviceHandle, CDB, 12, SCSI_IOCTL_DATA_IN, 8, id, 10);

	return iResult;	
}



//---------------------------------------------------------------------------
int CNTCommand::NT_DevWriteBuf( unsigned char bDrv,HANDLE DeviceHandle,		
				   unsigned char *pBuf,	
				   DWORD  dwStartAddr, 
				   DWORD  dwTotalSec,
				   BOOL   blHidden   )
{
	DWORD dwCount;
	DWORD dwSec, dwSize, i;
	DWORD dwSizePreTime = 0;

	//dwSizePreTime = 64*512;		//  32K, 64sectors
	dwSizePreTime = 128*512;		//  32K, 64sectors
	dwSize = dwTotalSec*512;

	dwCount = dwSize/dwSizePreTime;
	if( (dwSize%dwSizePreTime)!=0 ) dwCount++;

	for(i=0; i<dwCount; i++)
	{
		if( ((dwSize%dwSizePreTime)!=0)&&(i==(dwCount-1)) )
		{
			dwSec = (dwSize%dwSizePreTime)/512;
			if( dwSize%512 ) dwSec++;
		} else {
			dwSec = dwSizePreTime/512;
		}

	//	if( NT_write_sectors( (char)bDrv, dwStartAddr, dwSec, 
	//		(unsigned char *)(pBuf+(i*dwSizePreTime) ), // 64*512
	//		blHidden) )
	//		return 1;

		if(DeviceHandle == NULL)
		{
			if(NT_readwrite_sectors((char)bDrv,dwStartAddr,dwSec,(unsigned char *)(pBuf+(i*dwSizePreTime) ),false,true,0,false) )
			{
				OutputDebugString("Read Write fail1");
				//iResult = 0x10;		
				//	OutReport(IDMS_ERR_TREAD);
				//	goto rw_end;
				return 2;
			}
		}
		else
		{
			if(NT_readwrite_sectors_H(DeviceHandle, dwStartAddr,dwSec,(unsigned char *)(pBuf+(i*dwSizePreTime) ),false,true,0,false) )
			{
				OutputDebugString("Read Write fail1");
				//iResult = 0x10;		
				//	OutReport( IDMS_ERR_TREAD);
				//	goto rw_end;
				return 2;
			}
		}
//		if(NT_readwrite_sectors((char)bDrv,dwStartAddr,dwSec,(unsigned char *)(pBuf+(i*dwSizePreTime) ),false,true,0))
//			return 1;
		dwStartAddr = dwStartAddr+dwSec;
	}

	return 0;
}

//-----------------------------------------------------------------
int CNTCommand::NT_DevReadBuf(  unsigned char bDrv,HANDLE DeviceHandle,
							  unsigned char *pBuf,	
							  DWORD  dwStartAddr, 
							  DWORD  dwTotalSec,
							  BOOL   blHidden  )
{
	DWORD dwCount;
	DWORD dwSec, dwSize, i;
	DWORD dwSizePreTime = 0;

	//dwSizePreTime = 64*512;		//  32K, 64sectors
	dwSizePreTime = 128*512;		//  32K, 64sectors
	dwSize = dwTotalSec*512;

	dwCount = dwSize/dwSizePreTime;
	if( (dwSize%dwSizePreTime)!=0 ) dwCount++;

	for(i=0; i<dwCount; i++)
	{
		if( ((dwSize%dwSizePreTime)!=0)&&(i==(dwCount-1)) )
		{
			dwSec = (dwSize%dwSizePreTime)/512;
			if( dwSize%512 ) dwSec++;
		} else {
			dwSec = dwSizePreTime/512;
		}
		//		if( NT_read_sectors( (char)bDrv, dwStartAddr, dwSec, 
		//			(unsigned char *)(pBuf+(i*dwSizePreTime) ), // 64*512
		//			blHidden) )
		//			return 1;

		if(DeviceHandle == NULL)
		{
			if(NT_readwrite_sectors((char)bDrv,dwStartAddr,dwSec,(unsigned char *)(pBuf+(i*dwSizePreTime) ),true,true,0,false) )
			{
				OutputDebugString("Read Write fail1");
				//iResult = 0x10;		
				//	OutReport(IDMS_ERR_TREAD);
				//	goto rw_end;
				return 2;
			}
		}
		else
		{
			if(NT_readwrite_sectors_H(DeviceHandle, dwStartAddr,dwSec,(unsigned char *)(pBuf+(i*dwSizePreTime) ),true,true,0,false) )
			{
				OutputDebugString("Read Write fail1");
				//iResult = 0x10;		
				//	OutReport( IDMS_ERR_TREAD);
				//	goto rw_end;
				return 2;
			}
		}		
		//		if(NT_readwrite_sectors((char)bDrv,dwStartAddr,dwSec,(unsigned char *)(pBuf+(i*dwSizePreTime) ),true,true,0))
		//			return 1;
		dwStartAddr = dwStartAddr+dwSec;
	}

	return 0;
}


//---------------------------------------------------------------------------
/*
#define	U3CE_Generate_Random_number_Op				0xE2	// U3CE_Generate_Random_number_OpCode
This command generates a 64-byte true random number and return to the host

output 64Bytes Random number
*/
int CNTCommand::NTU3_U3PRA__Generate_Random_number(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 64;					// 64Bytes, 64Byte random number

	if(bBuf==NULL) return 1;
	if(wBufLen<dwByte) return 1;

	memset( bCBD, 0, sizeof(bCBD));
	memset( bBuf, 0, wBufLen);

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3CE_Generate_Random_number_Op;			// 0xE2 (0x00E2 )
	bCBD[2] = (BYTE)(U3CE_Generate_Random_number_Op>>8);	// 0x00;

	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);

	//	iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	//	if( iResult==0 )	
	//	{				
	//	} 	
	return iResult;
}
//---------------------------------------------------------------------------
/*
#define	U3HA__Read_Page_Op							0x61	// U3HA__Read_Page_OpCode
This command reads Tag-protected page. The Tag is the first word(4Bytes) on the page,
and this is the authentication to read this hidden page(protection Type 1)

Input  : wPageIndex, 0 indexed
output : 512Bytes
The first 4 bytes of the hidden page content the Tag value

FAIL :
1. The page is password-protected for write access
2. The page is a secure page

note : 

*/
int	CNTCommand::NTU3__Read_Page(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, WORD wPageIndex, DWORD dwPageTag, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 512;					// 512Byte: Tag(4):data(508)

	if(bBuf==NULL) return 1;
	if(wBufLen<dwByte) return 1;		

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3HA__Read_Page_Op;			// 0x61  (0x0061 )
	bCBD[2] = (BYTE)(U3HA__Read_Page_Op>>8);	// 0x00;

	bCBD[3] = (BYTE)(wPageIndex);
	bCBD[4] = (BYTE)(wPageIndex>>8);

	bCBD[5] = (BYTE)(dwPageTag);
	bCBD[6] = (BYTE)(dwPageTag>>8);
	bCBD[7] = (BYTE)(dwPageTag>>16);
	bCBD[8] = (BYTE)(dwPageTag>>24);
	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);

	if( iResult==0 )	
	{	
	} 	
	return iResult;
}

int	CNTCommand::NTU3__Scalable_Read_Page(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, unsigned int wPageIndex, unsigned int wCount, BYTE *bBuf, DWORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 512;					// 512Byte: Tag(4):data(508)

	if(bBuf==NULL) return 1;	

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = 0x06;
	bCBD[2] = 0x01;	
	bCBD[3] = 0xff;

	bCBD[7] = (unsigned char)((wPageIndex>>24)&0xff);
	bCBD[6] = (unsigned char)((wPageIndex>>16)&0xff);
	bCBD[5] = (unsigned char)((wPageIndex>>8)&0xff);
	bCBD[4] = (unsigned char)(wPageIndex&0xff);

	bCBD[11] = (unsigned char)((wCount>>24)&0xff);
	bCBD[10] = (unsigned char)((wCount>>16)&0xff);
	bCBD[9] = (unsigned char)((wCount>>8)&0xff);
	bCBD[8] = (unsigned char)(wCount&0xff);

	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, wBufLen, bBuf, 100);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, wBufLen, bBuf, 100);

	if( iResult==0 )	
	{	
	} 	
	return iResult;
}

//---------------------------------------------------------------------------
int CNTCommand::NT_read_sec2blks(BYTE bType,HANDLE DeviceHandel, char targetDisk, long address, int secNo,
								 unsigned char *buf, BYTE bBlkPreSec, bool hidden, bool blspec)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )					return 1;
	//memset( buf, 0, (secNo*bBlkPreSec)<<9 );		// buffer overflow
	memset( buf, 0, (secNo)<<9 );

	if( bBlkPreSec==0 )				return 1;
	if( (address%bBlkPreSec)!=0 )	return 1;
	if( (secNo%bBlkPreSec)!=0 )		return 1;
	address = address/bBlkPreSec;
	secNo   = secNo/bBlkPreSec;

	memset(CDB, 0, sizeof(CDB));
	if (hidden)
	{
		CDB[0] = 6;
		CDB[1] = 0xA;
		CDB[9] = 'V';
		CDB[10] = 'R';

		CDB[11] =  blspec;
	}
	else {
		CDB[0] = 0x28;

		// Magic char for PHISON
		CDB[6] = 'P';
		CDB[9] = 'H';
	}

	CDB[2] = (char)((address>>24)&0xff);
	CDB[3] = (char)((address>>16)&0xff);
	CDB[4] = (char)((address>>8)&0xff);
	CDB[5] = (char)(address&0xff);
	CDB[7] = (char)(secNo>>8);
	CDB[8] = (char)secNo;
	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, (secNo*bBlkPreSec)<<9, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandel, CDB, 12 ,SCSI_IOCTL_DATA_IN, (secNo*bBlkPreSec)<<9, buf, 10);

	return iResult;
}

//---------------------------------------------------------------------------

int CNTCommand::NT_SetWriteProtect(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char byLUN, unsigned char byType)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[12];
	int iResult = 0;
	int m_Size  = 0;
	int iTOut   = 10;
	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x31;
	CDB[2] = byLUN;
	CDB[3] = byType;
	//iResult = NT_CustomCmd(drive, CDB, 12, SCSI_IOCTL_DATA_OUT, m_Size, NULL, iTOut);
	if(bType ==0 )
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, m_Size, NULL, iTOut);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB, 12 ,SCSI_IOCTL_DATA_OUT, m_Size, NULL, iTOut);
	return iResult;
}

int CNTCommand::NT_BurnerTest1Block(BYTE bType ,HANDLE MyDeviceHandle,char drive)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[12];
	int iResult = 0;
	int m_Size  = 0;
	int iTOut   = 180;
	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x31;

	if(bType ==0 )
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, m_Size, NULL, iTOut);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB, 12 ,SCSI_IOCTL_DATA_OUT, m_Size, NULL, iTOut);
	return iResult;
}

int CNTCommand::NT_GetWriteProtect(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char *p_byStatusOfDisk)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12]; 
	unsigned char bBuf[8]; 
	DWORD dwByte = 8;

	memset( bCBD, 0, sizeof(bCBD));
	memset( bBuf, 0, sizeof(bBuf));

	bCBD[0] = 0x5A;
	if(bType ==0 )
		iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);

	if( iResult!=0 ) 
	{ 
		return iResult;
	}  



	if ((bBuf[3] & 0x80) == 0x80)
		*p_byStatusOfDisk = 1;
	else
		*p_byStatusOfDisk = ((bBuf[3])&128); 

	return 0;
}

// note : id, min 8 Bytes
int CNTCommand::NT_Direct_Op(BYTE bType ,HANDLE MyDeviceHandle, char targetDisk , unsigned char directFunc , unsigned char *id )
{
	nt_log("%s",__FUNCTION__);
	BOOL status = 0;
	ULONG length = 0 ,  returned = 0;
	unsigned char CDB[16];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p=NULL; 
	int count = 0;			

	if( id==NULL ) return 1;
	if(bType ==0)
		DeviceHandle_p = MyCreateFile(targetDisk);
	else if(bType == 1)
		DeviceHandle_p = MyDeviceHandle;

	DWORD err=GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE)
		return err;

OnceAgain:	
	memset( id, 0, 8);
	memset( CDB , 0 , sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xA0;
	CDB[4] = 0x08;
	CDB[5] = directFunc;

	SCSIPtdBuilder(&sptdwb, 16, CDB, SCSI_IOCTL_DATA_IN, 8, 30, id);	
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);

	err = GetLastError();

	if (sptdwb.sptd.ScsiStatus && 5 > count)
	{
		//TRACE("NT_GetFlashExtendID ScsiStatus: %d\n", sptdwb.sptd.ScsiStatus);
		count++;
		Sleep(100);
		goto OnceAgain;
	}

	err=GetLastError();
	if(bType == 0)
		MyCloseHandle(&DeviceHandle_p);

	if (status)
	{
		/*
		memcpy(sptdwb->sptd.Cdb, CDB, cdbLength);
		......
		*/
		return -sptdwb.sptd.ScsiStatus;
	} else
		return err;
}

//---------------------------------------------------------------------------
// return : 0 = OK
//          others = fail
// note : buf, min 8 Bytes
int CNTCommand::NT_SetCCIDName(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )	return 1;

	//memset( buf, 0, 8);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x83;
	CDB[2] = 0x00;
	CDB[3] = 0x50;
	CDB[4] = 0x68;
	CDB[5] = 0x49;
	//name  4 bytes
	CDB[6] = buf[3];  
	CDB[7] = buf[2];
	CDB[8] = buf[1];
	CDB[9] = buf[0];

	if(bType ==0 )
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB,12,SCSI_IOCTL_DATA_OUT,0,NULL,10);

	return iResult;
}

//---------------------------------------------------------------------------
// return : 0 = OK
//          others = fail
// note : buf, min 8 Bytes
int CNTCommand::NT_GetDriveCapacity(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )	return 1;

	memset( buf, 0, 8);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x25;
	if(bType ==0 )
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 8, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB,12,SCSI_IOCTL_DATA_IN,8,buf,10);

	return iResult;
}

int CNTCommand::NT_sRAM_readwrite_sectors(BYTE bType, HANDLE hDeviceHandle, char targetDisk,long address, int secNo, unsigned char *buf,bool fRead )
{
	nt_log("%s",__FUNCTION__);
	BOOL iResult = 0;
	ULONG length = 0 ,  returned = 0;
	unsigned char CDB[12];
	//SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	//HANDLE DeviceHandle_p = NULL; 
	int count = 0;			

	if( buf==NULL ) return 1;

	//DeviceHandle_p = MyCreateFile(targetDisk);

	DWORD err=0;	//GetLastError();
	if (hDeviceHandle == INVALID_HANDLE_VALUE)
		return 2;	//err;

	memset( CDB , 0 , sizeof(CDB));


	CDB[0] = 6;
	CDB[1] = fRead ? 0x21 : 0x20;	// 0x21:read, 0x20:write
	//CDB[4] = address;

	//if (address >= 100)
	//{
	CDB[4] = (char)(( address ) & 0xff );
	CDB[5] = (char)(( address >> 8 ) & 0xff );
	//}
	//else
	//{
	//	CDB[4] = address;
	//}
	CDB[8] = secNo;	

	if(bType ==0 )
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,fRead ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT, secNo << 9, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(hDeviceHandle,CDB,12,fRead ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT,secNo << 9,buf,10);

	return iResult;
}

int CNTCommand::NT_readwrite_sectors_Hl( HANDLE hDeviceHandle,unsigned long address, int secNo, unsigned char *buf, 
										bool fRead, bool hidden, BYTE blspec ,bool isover1T){

											nt_log("%s",__FUNCTION__);
											BOOL status = 0;
											ULONG length = 0 ,  returned = 0;
											unsigned char CDB[16];
											SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
											//HANDLE DeviceHandle_p = NULL; 
											int count = 0;			

											if( buf==NULL ) return 1;

											//DeviceHandle_p = MyCreateFile(targetDisk

											DWORD err=0;	//GetLastError();
											if (hDeviceHandle == INVALID_HANDLE_VALUE)
												return 2;	//err;

OnceAgain:
											memset( CDB , 0 , sizeof(CDB));

											if (hidden)
											{
												CDB[0] = 6;
												CDB[1] = fRead ? 0xA : 0xB;	// 0xA:read, 0xB:write
												CDB[9] = 'V';

												if( fRead ) CDB[10] = 'R';
												else CDB[10] = 'W';

												CDB[11] =  blspec;

											} else {
												CDB[0] = fRead ? 0x28 : 0x2A;
												// Magic char for PHISON
												CDB[6] = 'P';
												CDB[9] = 'H';
											}


											if (isover1T)
											{
												INT64 aa = address;
												OutputDebugString("NT_readwrite_sectors_Hl ---");
												CDB[0] = fRead ? 0x88 : 0x8A;
												CDB[2] = (unsigned char)(( aa >> 56 ) & 0xff );
												CDB[3] = (unsigned char)(( aa >> 48 ) & 0xff );
												CDB[4] = (unsigned char)(( aa >> 40 ) & 0xff );
												CDB[5] = (unsigned char)(( aa >> 32 ) & 0xff );
												CDB[6] = (unsigned char)(( aa >> 24 ) & 0xff );
												CDB[7] = (unsigned char)(( aa >> 16 ) & 0xff );
												CDB[8] = (unsigned char)(( aa >> 8 ) & 0xff );
												CDB[9] = (unsigned char)(( aa ) & 0xff );		
												CDB[10] = (char)( secNo >> 24 );
												CDB[11] = (char)( secNo >> 16 );
												CDB[12] = (char)( secNo >> 8 );
												CDB[13] = secNo;

											}
											else
											{
												//CDB[0] = fRead ? 0x28 : 0x2A;
												CDB[2] = (char)(( address >> 24 ) & 0xff );
												CDB[3] = (char)(( address >> 16 ) & 0xff );
												CDB[4] = (char)(( address >> 8 ) & 0xff );
												CDB[5] = (char)(( address ) & 0xff );
												CDB[7] = (char)( secNo >> 8 );
												CDB[8] = secNo;
											}


											SCSIPtdBuilder(&sptdwb, 16, CDB, fRead ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT, secNo<<9, 180, buf);
											length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
											status = DeviceIoControl(hDeviceHandle, //DeviceHandle_p,
												IOCTL_SCSI_PASS_THROUGH_DIRECT,
												&sptdwb,
												length,
												&sptdwb,
												length,
												&returned,
												FALSE);

											err = GetLastError();

											if (sptdwb.sptd.ScsiStatus && 10 > count)
											{
												//		TRACE("Read/Write ScsiStatus: %d\n", sptdwb.sptd.ScsiStatus);
												count++;
												Sleep(100);
												goto OnceAgain;
											}

											err=GetLastError();

											//MyCloseHandle(&DeviceHandle_p);

											if (status)
												return -sptdwb.sptd.ScsiStatus;
											else
												return err;
}

int CNTCommand::NT_readwrite_sectors_H( HANDLE hDeviceHandle,unsigned long address, int secNo, unsigned char *buf, 
									   bool fRead, bool hidden, BYTE blspec,bool isover1T ,int timeout)
{
	int count = 1;
	int MinSplitSec = 128;//強迫ㄧ次只能寫 16個sector 2014.03.28 16->128 
	unsigned char *readbuf;
	int size = (secNo )<<9;
	int splitSecCnt = MinSplitSec;
	if(secNo < MinSplitSec || fRead)
		splitSecCnt = secNo;
	readbuf = new unsigned char [size];
	int remain = secNo;
	if(_forceWriteCount > 0) count = _forceWriteCount;
	int res = 0;
	int DataOffset =0;
	int LBAOffset =0;
	while(remain){
		for(int i=0;i<count;i++){//for 設定 write 到pass 條件
			int compareSize =  (splitSecCnt << 9);
			res= NT_readwrite_sectors_Hl(hDeviceHandle,address+LBAOffset,splitSecCnt,buf+DataOffset,fRead,hidden,blspec,isover1T);

			if(res !=0) break;
			if(fRead) break;
			if(!fRead && _forceWriteCount>1){

				res = NT_readwrite_sectors_Hl(hDeviceHandle,address+LBAOffset,splitSecCnt,readbuf,true,hidden,blspec,isover1T);
				if(memcmp(readbuf,buf+DataOffset,compareSize)){
					res =1 ;
					OutputDebugString("try to Write lba ---");
					continue;
				}
				else break;
			}
		}
		if(res) break;
		remain -= splitSecCnt;
		DataOffset+=(splitSecCnt << 9);
		LBAOffset += splitSecCnt;
		if(remain < splitSecCnt && remain >0) splitSecCnt = remain;

	}

	delete [] readbuf;
	return res;

}

#if 0
int CNTCommand::NT_readwrite_sectors_H( HANDLE hDeviceHandle,unsigned long address, int secNo, unsigned char *buf, 
									   bool fRead, bool hidden, BYTE blspec, int iMun ,bool isover1T,int timeout)
{
	nt_log("%s",__FUNCTION__);
	BOOL status = 0;
	ULONG length = 0 ,  returned = 0;
	unsigned char CDB[16];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	//HANDLE DeviceHandle_p = NULL; 
	int count = 0;			

	if( buf==NULL ) return 1;

	//DeviceHandle_p = MyCreateFile(targetDisk);

	DWORD err=0;	//GetLastError();
	if (hDeviceHandle == INVALID_HANDLE_VALUE)
		return 2;	//err;

OnceAgain:
	memset( CDB , 0 , sizeof(CDB));
	if (iMun == 0)
	{
		if (hidden)
		{
			CDB[0] = 6;
			CDB[1] = fRead ? 0xA : 0xB;	// 0xA:read, 0xB:write
			CDB[9] = 'V';

			if( fRead ) CDB[10] = 'R';
			else CDB[10] = 'W';

			CDB[11] =  blspec;

		} else {
			CDB[0] = fRead ? 0x28 : 0x2A;
			// Magic char for PHISON
			CDB[6] = 'P';
			CDB[9] = 'H';
		}
	}
	else
	{
		CDB[0] = 6;
		CDB[1] = fRead ? 0xA : 0xB;	// 0xA:read, 0xB:write
		CDB[6] = iMun;
		CDB[9] = 'V';

		if( fRead ) CDB[10] = 'R';
		else CDB[10] = 'W';

		CDB[11] =  blspec;
	}

	if(isover1T)
	{
		INT64 aa = address;
		OutputDebugString("NT_readwrite_sectors_H  ---");
		CDB[0] = fRead ? 0x88 : 0x8A;
		CDB[2] = (unsigned char)(( aa >> 56 ) & 0xff );
		CDB[3] = (unsigned char)(( aa >> 48 ) & 0xff );
		CDB[4] = (unsigned char)(( aa >> 40 ) & 0xff );
		CDB[5] = (unsigned char)(( aa >> 32 ) & 0xff );
		CDB[6] = (unsigned char)(( aa >> 24 ) & 0xff );
		CDB[7] = (unsigned char)(( aa >> 16 ) & 0xff );
		CDB[8] = (unsigned char)(( aa >> 8 ) & 0xff );
		CDB[9] = (unsigned char)(( aa ) & 0xff );		
		CDB[10] = (char)( secNo >> 24 );
		CDB[11] = (char)( secNo >> 16 );
		CDB[12] = (char)( secNo >> 8 );
		CDB[13] = secNo;
	}
	else
	{
		//CDB[0] = fRead ? 0x28 : 0x2A;
		CDB[2] = (char)(( address >> 24 ) & 0xff );
		CDB[3] = (char)(( address >> 16 ) & 0xff );
		CDB[4] = (char)(( address >> 8 ) & 0xff );
		CDB[5] = (char)(( address ) & 0xff );
		CDB[7] = (char)( secNo >> 8 );
		CDB[8] = secNo;
	}

	SCSIPtdBuilder(&sptdwb, 16, CDB, fRead ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT, secNo<<9, timeout, buf);
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(hDeviceHandle, //DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);

	err = GetLastError();

	if (sptdwb.sptd.ScsiStatus && 10 > count)
	{
		//		TRACE("Read/Write ScsiStatus: %d\n", sptdwb.sptd.ScsiStatus);
		count++;
		Sleep(100);
		goto OnceAgain;
	}

	err=GetLastError();

	//MyCloseHandle(&DeviceHandle_p);

	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return err;
}
#endif


#if 1  /// #wait#check#read/write 12 or 16#PH#
// NES Using it
int CNTCommand::NT_readwrite_sectors_H(HANDLE hDeviceHandle, unsigned long address, int secNo, unsigned char *buf,
	bool fRead, bool hidden, BYTE blspec, int iMun, bool isover1T, int timeOUt)
{
	BOOL status = 0;
	ULONG length = 0, returned = 0;
	unsigned char CDB[16];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	int count = 0;
	DWORD err = 0;

	if (buf == NULL) return 1;
	if (hDeviceHandle == INVALID_HANDLE_VALUE) return 2;

OnceAgain:
	memset(CDB, 0, sizeof(CDB));
	if (iMun == 0)
	{
		if (hidden)
		{
			CDB[0] = 6;
			CDB[1] = fRead ? 0xA : 0xB;	// 0xA:read, 0xB:write
			CDB[9] = 'V';

			if (fRead) CDB[10] = 'R';
			else CDB[10] = 'W';

			CDB[11] = blspec;

		}
		else {
			CDB[0] = fRead ? 0x28 : 0x2A;
			// Magic char for PHISON
			CDB[6] = 'P';
			CDB[9] = 'H';
		}
	}
	else
	{
		CDB[0] = 6;
		CDB[1] = fRead ? 0xA : 0xB;	// 0xA:read, 0xB:write
		CDB[6] = iMun;
		CDB[9] = 'V';

		if (fRead) CDB[10] = 'R';
		else CDB[10] = 'W';

		CDB[11] = blspec;
	}

	if (isover1T)
	{
		INT64 aa = address;
		OutputDebugString("NT_readwrite_sectors_H  ---");
		//CDB[0] = fRead ? 0x88 : 0x8A;		// read/write 16
		CDB[0] = fRead ? 0xA8 : 0xAA;		// read/write 12
#if 0
		CDB[2] = (unsigned char)((aa >> 56) & 0xff);
		CDB[3] = (unsigned char)((aa >> 48) & 0xff);
		CDB[4] = (unsigned char)((aa >> 40) & 0xff);
		CDB[5] = (unsigned char)((aa >> 32) & 0xff);
		CDB[6] = (unsigned char)((aa >> 24) & 0xff);
		CDB[7] = (unsigned char)((aa >> 16) & 0xff);
		CDB[8] = (unsigned char)((aa >> 8) & 0xff);
		CDB[9] = (unsigned char)((aa) & 0xff);
		CDB[10] = (char)(secNo >> 24);
		CDB[11] = (char)(secNo >> 16);
		CDB[12] = (char)(secNo >> 8);
		CDB[13] = secNo;
#endif
		CDB[2] = (char)((address >> 24) & 0xff);
		CDB[3] = (char)((address >> 16) & 0xff);
		CDB[4] = (char)((address >> 8) & 0xff);
		CDB[5] = (char)((address) & 0xff);
		CDB[6] = (char)((secNo >> 24) & 0xff);
		CDB[7] = (char)((secNo >> 16) & 0xff);
		CDB[8] = (char)((secNo >> 8) & 0xff);
		CDB[9] = (char)((secNo) & 0xff);;
		CDB[10] = 'P';
		CDB[11] = 'H';

	}
	else
	{
		//CDB[0] = fRead ? 0x28 : 0x2A;
		CDB[2] = (char)((address >> 24) & 0xff);
		CDB[3] = (char)((address >> 16) & 0xff);
		CDB[4] = (char)((address >> 8) & 0xff);
		CDB[5] = (char)((address) & 0xff);
		CDB[7] = (char)(secNo >> 8);
		CDB[8] = secNo;
	}
	memcpy(m_CMD, CDB, 16);

	SCSIPtdBuilder(&sptdwb, 16, CDB, fRead ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT, secNo << 9, timeOUt, buf);
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	SetLastError(0);
	double startT = clock();
	status = DeviceIoControl(hDeviceHandle,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);
	double endT = clock();
	double diff = (endT - startT) / CLOCKS_PER_SEC;
	err = GetLastError();

	if (err == 121) { //time out //but 有可能是CSW Fail
		if (diff < (double)(timeOUt / 2)) {
			SetLastError(0);
			Sleep(100);
			goto OnceAgain;
		}
	}

	if ((err && count <2) || sptdwb.sptd.ScsiStatus) {
		SetLastError(0);
		if (count < 2) {
			count++;
			Sleep(100);
			goto OnceAgain;
		}
	}

	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return err;
}
#endif

int CNTCommand::NT_write_blks(BYTE bType,HANDLE DeviceHandel, char targetDisk, long address, int secNo,
							  unsigned char *buf, BYTE bBlkPreSec, BOOL hidden, bool blspec)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL ) return 1;

	memset(CDB, 0, sizeof(CDB));

	if (hidden)
	{
		CDB[0] = 6;
		CDB[1] = 0xB;
		CDB[9] = 'V';
		CDB[10] = 'W';

		CDB[11] =  blspec;
	}
	else {
		CDB[0] = 0x2A;

		// Magic char for PHISON	// <S> fix
		CDB[6] = 'P';
		CDB[9] = 'H';
	}
	CDB[2] = (char)((address>>24)&0xff);
	CDB[3] = (char)((address>>16)&0xff);
	CDB[4] = (char)((address>>8)&0xff);
	CDB[5] = (char)(address&0xff);
	CDB[7] = (char)(secNo>>8);
	CDB[8] = (char)secNo;


	if( bType==0 )
	{
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, (secNo*bBlkPreSec)<<9, buf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandel, CDB, 12 ,SCSI_IOCTL_DATA_OUT,  (secNo*bBlkPreSec)<<9, buf, 10);
	}

	return iResult;
}

//int CNTCommand::NT_write_blks(BYTE bType,HANDLE DeviceHandel, char targetDisk, long address, int secNo,
//							  unsigned char *buf, BYTE bBlkPreSec, BOOL hidden, bool blspec, BYTE bMun)
//{
//	int iResult = 0;
//	unsigned char CDB[12];
//
//	if( buf==NULL ) return 1;
//
//	memset(CDB, 0, sizeof(CDB));
//
//	if (bMun)
//	{
//		CDB[0] = 6;
//		CDB[1] = 0xB;
//		CDB[6] = bMun;
//		CDB[9] = 'V';
//		CDB[10] = 'W';
//		CDB[11] =  blspec;
//	}
//	else if (hidden)
//	{
//		CDB[0] = 6;
//		CDB[1] = 0xB;
//		CDB[9] = 'V';
//		CDB[10] = 'W';
//
//		CDB[11] =  blspec;
//	}
//	else {
//		CDB[0] = 0x2A;
//
//		// Magic char for PHISON	// <S> fix
//		CDB[6] = 'P';
//		CDB[9] = 'H';
//	}
//	CDB[2] = (char)((address>>24)&0xff);
//	CDB[3] = (char)((address>>16)&0xff);
//	CDB[4] = (char)((address>>8)&0xff);
//	CDB[5] = (char)(address&0xff);
//	CDB[7] = (char)(secNo>>8);
//	CDB[8] = (char)secNo;
//
//
//	if( bType==0 )
//	{
//		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, (secNo*bBlkPreSec)<<9, buf, 10);
//	} else if( bType==1 )	{
//		iResult = NT_CustomCmd_Phy(DeviceHandel, CDB, 12 ,SCSI_IOCTL_DATA_OUT,  (secNo*bBlkPreSec)<<9, buf, 10);
//	}
//
//	return iResult;
//}

int CNTCommand::NT_USB23Switch(BYTE bType,HANDLE DeviceHandle,char targetDisk, BYTE TYPE)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];


	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xcd;
	CDB[2] = TYPE;


	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);
	Sleep(80);

	return iResult;
}


unsigned int CNTCommand::NT_EMMC_Preformat_GenH( HANDLE DeviceHandle_p,			//char drive,
												DWORD StartLba,DWORD Length,unsigned char PreformatType)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0x35;
	CDB[2] = PreformatType;
	CDB[4] = (unsigned char)((StartLba>>24)&0xff);
	CDB[5] = (unsigned char)((StartLba>>16)&0xff);
	CDB[6] = (unsigned char)((StartLba>>8)&0xff);
	CDB[7] = (unsigned char)(StartLba&0xff);

	CDB[8] = (unsigned char)((Length>>24)&0xff);
	CDB[9] = (unsigned char)((Length>>16)&0xff);
	CDB[10] = (unsigned char)((Length>>8)&0xff);
	CDB[11] = (unsigned char)(Length&0xff);


	//memcpy(m_CMD,CDB,16);
	return NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16, SCSI_IOCTL_DATA_OUT, 0, NULL, 1800);
}


unsigned int CNTCommand::NT_Direct_Preformat_GenH( HANDLE DeviceHandle_p,			//char drive,
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
												  ,unsigned char *falshid,BYTE flash_maker,
												  BYTE Rel_Factory,bool bCloseInterleave)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0x35;
	CDB[2] = preFormatType;
	if(bCloseInterleave)
		CDB[11] = 0x96;
	// only for 2153 and 2151
	if (mark)
	{
		if(bVersionPage[0x18E] == 0x54)//check if 18E = 0x54('T') then use different preformat command 
			strncpy((char *)&CDB[3], "TSB", 3);
		else
			strncpy((char *)&CDB[3], "PhIsOn", 6);
	}

	if (bForcePreformat) // 2136CF2,CG force.
		CDB[9] = 0x70;

	if( wBlock )					// KW 06.01.17 valid if non-zero // <S> 05.06.03  for Max Logic Block setting
	{
		CDB[6] = 0xC6;	//H		// V32
		CDB[7] = (wBlock>>8);//V		// 980: [7:8] 03:D4, 512: 01EA
		CDB[8] = (wBlock&0xFF); 
	}
	if (spare[0])
	{
		CDB[6] = 0xc6;
		CDB[7] = (unsigned char)((spare[0]>>8)&0xff);
		CDB[8] = (unsigned char)(spare[0]&0xff);
	}
	if (spare[1])
	{
		CDB[11] = 0xc6;
		CDB[12] = (unsigned char)((spare[1]>>8)&0xff);
		CDB[13] = (unsigned char)(spare[1]&0xff);
	}
	if (spare[2])
	{
		CDB[4] = (unsigned char)((spare[2]>>8)&0xff);
		CDB[5] = (unsigned char)(spare[2]&0xff);
	}
	if (spare[3])
	{
		CDB[9] = (unsigned char)((spare[3]>>8)&0xff);
		CDB[10] = (unsigned char)(spare[3]&0xff);
	}
	if (spare[4])
	{
		CDB[14] = (unsigned char)((spare[4]>>8)&0xff);
		CDB[15] = (unsigned char)(spare[4]&0xff);
	}
	if((ICValue  & 0xff) == 0x21)
	{
		if((ICValue >> 8) > 0x54)
		{
			if (onePlane)
			{
				CDB[9] = 0xc6;
				CDB[10] = 0xAD;
			}
		}
	}

	if (Rel_Factory == 0x6E && ICValue == 0x5621 && firmware[0]/*major*/ == 0x04)
		if(falshid[0] == 0xd3 && falshid[1] == 0x55 
			&& falshid[2] == 0x25 && falshid[3] == 0x58 && flash_maker == 0xec)
			CDB[9] = 0x70;
	//============================

	if (bEnableCopyBack)
		CDB[14] = 0xc2;

	memcpy(m_CMD,CDB,16);
	return NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16, SCSI_IOCTL_DATA_OUT, 0, NULL, 1800);
}
unsigned int CNTCommand::NT_Unlock(char targetDisk)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x1D;
	CDB[4] = 0x41;
	CDB[5] = 0x57;

	iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);

	return iResult;
}
unsigned int CNTCommand::NT_Direct_Preformat_GenH6(BYTE bType,
												   BYTE bDrv,
												   HANDLE DeviceHandle_p,			//char drive,
												   unsigned char preFormatType,
												   bool mark, 
												   bool maxblk,
												   WORD wBlock,
												   unsigned char *preformatBuf,
												   //unsigned short spare1,
												   //unsigned short* spare,
												   bool onePlane, 
												   bool bEnableCopyBack,
												   bool bForcePreformat,
												   unsigned char *bVersionPage)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	//#ifndef __TEST_TIME_TUNING__	
	//	CDB[1] = 0x35;
	//#else
	CDB[1] = 0x09;
	//#endif
	CDB[2] = preFormatType;

	// only for 2153 and 2151

	if (mark)
	{
		if(bVersionPage[0x18E] == 0x54)//check if 18E = 0x54('T') then use different preformat command 
			strncpy((char *)&CDB[3], "TSB", 3);
		else
			strncpy((char *)&CDB[3], "PhIsOn", 6);
	}

	if (bForcePreformat) // 2136CF2,CG force.
		CDB[9] = 0x70;

	//if( wBlock )					// KW 06.01.17 valid if non-zero // <S> 05.06.03  for Max Logic Block setting
	{
		CDB[6] = 'H';		// V32
		CDB[7] = 'V';		// 980: [7:8] 03:D4, 512: 01EA
		CDB[8] =  0; 
	}
#ifdef __NEWFW_TESTING__	
	CDB[9] = 0x01;
	CDB[10] = 0x01;
#endif

	int iResult = 0;
	isFWBank = 0;
	memcpy(m_CMD,CDB,16);

	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrv, CDB, 16 ,SCSI_IOCTL_DATA_OUT, 512, preformatBuf, 1800);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_OUT, 512, preformatBuf, 1800);
	}
	return iResult;
}



int CNTCommand::NT_MediaChange(BYTE bType,HANDLE DeviceHandle,char drive)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof( CDB ));

	CDB[0] = 0x06;
	CDB[1] = 0x30;
	if(bType == 0)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
}

int CNTCommand::NT_FingerSwitch_Start(BYTE bType,HANDLE DeviceHandle,char drive)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof( CDB ));

	CDB[0] = 0x06;
	CDB[1] = 0xF2;
	CDB[2] = 0xD0;
	CDB[3] = 0xff*rand()/RAND_MAX;	//0;		// don't care
	CDB[4] = 0xff*rand()/RAND_MAX;	//0;		// don't care
	CDB[5] = 0x86;
	CDB[6] = 0xA7;
	if(bType == 0)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
}


int CNTCommand::NT_FingerSwitch_Process(BYTE bType,HANDLE DeviceHandle,char drive)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof( CDB ));

	CDB[0] = 0x06;
	CDB[1] = 0xF2;
	CDB[2] = 0xA1;
	CDB[3] = 0x01;		
	CDB[4] = 0xff*rand()/RAND_MAX;	//0;		// don't care
	CDB[5] = 0xff*rand()/RAND_MAX;	//0;		// don't care

	if(bType == 0)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
}

int CNTCommand::NT_FingerSwitch_Result(BYTE bType,HANDLE DeviceHandle,char drive)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof( CDB ));

	CDB[0] = 0x06;
	CDB[1] = 0xF2;
	CDB[2] = 0xA3;

	CDB[3] = 0x84;		
	CDB[4] = 0x28;	
	CDB[5] = 0x11;	

	CDB[6] = 0x56;		
	CDB[7] = 0xFB;	
	CDB[8] = 0xE3;	

	CDB[9]  = 0x0E;		
	CDB[10] = 0x46;	
	CDB[11] = 0x39;	

	if(bType == 0)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
}

int CNTCommand::GetDeviceType()
{
	return _deviceType;
}

void CNTCommand::SetDeviceType(int deviceType)
{
	_deviceType = deviceType;
	if ((_deviceType&CONTROLLER_MASK) == DEVICE_SATA_3111)
		m_cmdtype = SSD_NTCMD;
	else
		m_cmdtype = SCSI_NTCMD;

	return UFWInterface::SetDeviceType(deviceType);
}

void CNTCommand::SetDeviceType_NoOpen(int deviceType)
{
	_deviceType = deviceType;
	if ((_deviceType&CONTROLLER_MASK) == DEVICE_SATA_3111)
		m_cmdtype = SSD_NTCMD;
	else
		m_cmdtype = SCSI_NTCMD;

	_isOutSideHandle = true;
}
//---------------------------------------------------------------------------
int CNTCommand::NT_StartStopDrv( BYTE bType ,HANDLE MyDeviceHandle,char targetDisk,  bool blStop)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x1B;
	//	CDB[4] = blStop?0x02:0x00;
	CDB[4] = blStop?0x02:0x03;


	if(bType == 0)
		return NT_CustomCmd(targetDisk, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(MyDeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);


	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);

	return iResult;
}

int CNTCommand::NT_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk , unsigned char *id, bool bGetUID,BYTE full_ce_info,BYTE get_ori_id)
{
	//	BOOL status = 0;
	//	ULONG length = 0 ,  returned = 0;
	//	unsigned char CDB[12];
	//	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	//	HANDLE DeviceHandle_p=NULL; 
	//	int count = 0;			
	//
	//	if( id==NULL ) return 1;
	//	if(bType == 0)
	//		DeviceHandle_p = MyCreateFile(targetDisk);
	//	if(bType == 1)
	//		DeviceHandle_p = MyDeviceHandle;
	//
	//	DWORD err=GetLastError();
	//	if (DeviceHandle_p == INVALID_HANDLE_VALUE && bSCSIControl)
	//		return err;
	//
	//OnceAgain:

	if(bSDCARD==1)
		return SD_TesterGetFlashID (bType, MyDeviceHandle, targetDisk,id);

	int iResult = 0;
	unsigned char CDB[16];
	memset( id, 0, 512);
	memset( CDB , 0 , sizeof(CDB));


	if( _forceID[0]!=0x00 && _forceID[1]!=0x00 && _forceID[2]!=0x00 ){
		memcpy(id, _forceID, 8);
		return 0;
	}

	CDB[0] = 0x06;
	CDB[1] = 0x56;
	CDB[2] = full_ce_info;
	CDB[3] = get_ori_id; //sorting code下 get 原始ce mapping id
	if (bGetUID)
	{
		CDB[4] = 0x5A;
	}
	nt_log_cdb(CDB,"%s",__FUNCTION__);
	_ForceNoSwitch = 1;
	for(int i=0;i<2;i++)
	{
		if(bType ==0)
			iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, id, 5);
		else
			iResult = NT_CustomCmd_Phy(MyDeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, id, 5);
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(3000);
	}

	return iResult;

	//SCSIPtdBuilder(&sptdwb, 12, CDB, SCSI_IOCTL_DATA_IN, 512, 10, id);	
	//length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	//status = NetDeviceIoControl(DeviceHandle_p,
	//	IOCTL_SCSI_PASS_THROUGH_DIRECT,
	//	&sptdwb,
	//	length,
	//	&sptdwb,
	//	length,
	//	&returned,
	//	FALSE);

	//err = GetLastError();

	//if (sptdwb.sptd.ScsiStatus && 5 > count)
	//{
	//	count++;
	//	Sleep(100);
	//	goto OnceAgain;
	//}

	//err=GetLastError();
	//if(bType == 0)
	//	MyCloseHandle(&DeviceHandle_p);

	//if (status)
	//{
	//	return -sptdwb.sptd.ScsiStatus;
	//} else
	//	return err;
}

int CNTCommand::NT_ReadGroupInfo(BYTE bType,HANDLE DeviceHandle_p,BYTE bDrvNum ,unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 16384;				

	if( bBuf==NULL ) return 1;

	memset( bBuf, 0, 16384);	// ? 528
	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0x06;
	bCBD[1] = 0x95;
	bCBD[2] = 0x02;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 300);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 300);
	} else return 0xFF;

	return iResult;
}

int CNTCommand::NT_ReadPage_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, const char *type, unsigned char *bBuf, unsigned char *DriveInfo)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[16] = {0};	
	DWORD dwByte = 528;				

	if( bBuf==NULL ) return 1;

	memset( bBuf, 0, 512);	// ? 528
	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0x06;
	bCBD[1] = 0x05;
	bCBD[8] = 0x80;
	_realCommand = 1;
	if (type)
		memcpy(&bCBD[2], type, 6);

	_ForceNoSwitch = 1;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 16 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 30);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 16 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 30);
	} else if( bType==2 )	{
		iResult = ST_CustomCmd_Path(DriveInfo, bCBD, 16 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 30);
	} else return 0xFF;

	return iResult;
}

int CNTCommand::NT_WriteInfoBlock(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL ) return 1;

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x06;
	CDB[2] = 1;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 512, buf, 30);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 512, buf, 30);
	} else return 0xFF;
	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);

	return iResult;	
}

int CNTCommand::NT_WriteInfoBlock_K(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if(buf==NULL ) return 1;

	memset(CDB, 0, sizeof(CDB));

	if (PreWriteInfoBlock(bType, DeviceHandle_p, drive))
		return 1;

	CDB[0] = 0x06;
	CDB[1] = 0x06;
	CDB[2] = 1;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 512, buf, 30);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 512, buf, 30);
	} else return 0xFF;
	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);

	return iResult;	
}

int CNTCommand::NT_WriteInfoBlock_23(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL ) return 1;
	if ( (buf[9] )== 0x09 && buf[8]==0x51) 
	{
		if (PreWriteInfoBlock(bType, DeviceHandle_p, drive))
			return 1;
	}


	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x06;
	CDB[2] = 0x04;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 10);
	} else return 0xFF;
	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);

	return iResult;	
}

int CNTCommand::NT_WriteInfoBlock_23_K(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf, DWORD dwICType)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL ) return 1;

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x06;
	CDB[2] = 0x04;

	if (dwICType != 0x8022)
	{
		if (PreWriteInfoBlock(bType, DeviceHandle_p, drive))
			return 1;
	}

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 10);
	} else return 0xFF;
	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);

	return iResult;	
}

int CNTCommand::NT_WriteTempInfoBlock(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL ) return 1;

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x06;
	CDB[2] = 3;

	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);
	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 512, buf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 512, buf, 10);
	} else return 0xFF;
	return iResult;	
}


int CNTCommand::NT_Inquiry_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf, unsigned char *DriveInfo)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 44;				

	if( bBuf==NULL ) return 1;

	memset( bBuf, 0, 44);
	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = SCSIOP_INQUIRY;
	bCBD[4] = 44;
	_realCommand =1;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else if( bType==2 )	{
		iResult = ST_CustomCmd_Path(DriveInfo, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else return 0xFF;

	return iResult;
}

int CNTCommand::NT_CustomCmd(char targetDisk, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut)
{
	HANDLE hdl = MyCreateHandle(targetDisk,false,false);
	int rtn = 1;
	if (INVALID_HANDLE_VALUE != hdl)
	{
		rtn = NT_CustomCmd_Phy(hdl, CDB, bCDBLen, direc, iDataLen, buf, timeOut);
	}
	MyCloseHandle(&hdl);
	return rtn;

	//if ( (_deviceType&INTERFACE_MASK)==IF_USB) {
	//	switch bank;
	//}
	//_ntOpType = 0;
	//_ntTargetDisk = targetDisk;
	//MyIssueCmd(  unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout=20 );
	//int ret = NetMyIssueCmdHandle(hdl, CDB, SCSI_IOCTL_DATA_IN==direc?true:false, buf, iDataLen, timeOut,_deviceType);
	//MyCloseHandle(&hdl);
	//return ret;
#if 0
	BOOL status  = 0;
	ULONG length = 0, returned = 0;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p = NULL; 
	DWORD dwErr = 0;

	if (0 > bCDBLen || 16 < bCDBLen)	return 1;
	if( CDB==NULL )						return 1;
	if( (iDataLen!=0)&&(buf==NULL) )	return 1;
	if( iDataLen>SCSI_MAX_TRANS )		return 1;
	// 	wchar_t ttmp[512];
	// 	wsprintfW(ttmp, L"[CDB] %02X %02X %02X %02X", CDB[0],CDB[1],CDB[2],CDB[3]);
	// 	OutputDebugStringW(ttmp);
	//Find Command in Bank database
	char direcStr[128];
	if(direc==SCSI_IOCTL_DATA_IN)
		sprintf(direcStr,"DATA IN");
	else if(direc==SCSI_IOCTL_DATA_OUT)
		sprintf(direcStr,"DATA OUT");
	else
		sprintf(direcStr," ");
	if ( _SetDumpCDB ) {
		_pSortingBase->Log("CDB=[%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X], %s, size=%d",
			CDB[0],CDB[1],CDB[2],CDB[3],CDB[4],CDB[5],CDB[6],CDB[7],CDB[8],CDB[9],CDB[10],CDB[11],CDB[12],CDB[13],CDB[14],CDB[15],direcStr,iDataLen);
	}
	if(isFWBank && (CDB[0] == 0x06) && (_bankBufTotal != NULL) && (_ForceNoSwitch==0))	//if _bankBufTotal == NULL -> MP Calling,don't check bank
	{
		int Bank = BankSearchByCmd(CDB);
		if(Bank == 0xff) //Error
		{
			assert(0);
			if(_pSortingBase!=NULL){
				_pSortingBase->Log("[Command Bank 0xff] CDB=[%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X], %s, size=%d",
					CDB[0],CDB[1],CDB[2],CDB[3],CDB[4],CDB[5],CDB[6],CDB[7],CDB[8],CDB[9],CDB[10],CDB[11],CDB[12],CDB[13],CDB[14],CDB[15],direcStr,iDataLen);
				_pSortingBase->OutReport(NTCmd_IDMS_ERR_CommandBankFF, -1);
			}
			return 1;
		}
		//if(_debugBankgMsg){
		//	if(CDB[2] != 0x53)
		//	{

		//		LogBank(Bank);
		//		//unsigned char bankBuf_cmp[BankSize*1024] = {0};
		//		unsigned char *bankBuf_cmp = new unsigned char[BankSize*1024];
		//		memset(bankBuf_cmp,0,BankSize*1024);
		//		ST_BankBufCheck(0,NULL,targetDisk,bankBuf_cmp);
		//		delete[] bankBuf_cmp;
		//	}
		//}
		//

		if(_alwaySyncBank)
			this->ST_BankSynchronize(0,NULL,targetDisk);

		if(Bank != currentBank && (Bank!=0x99))//if command not in current bank & not in common,do switch
		{
			//check bank
			unsigned char *bankBuf = new unsigned char[BankSize*1024];

			/*512 Tag + CommonSize + #Bank*Banksize*/
			int bankOffset = 512 + BankCommonSize*1024 + Bank*BankSize*1024;	
			memset(bankBuf,0,BankSize*1024);
			memcpy(bankBuf,_bankBufTotal+bankOffset,BankSize*1024);
			if(ST_BankSwitch(0,0,targetDisk,bankBuf)){
				delete[] bankBuf;
				_ForceNoSwitch = 0;
				return 1;
			}
			currentBank = Bank;
			delete[] bankBuf;
		}

	}
	_ForceNoSwitch = 0;


	SCSIPtdBuilder(&sptdwb, bCDBLen, CDB, SCSI_IOCTL_DATA_IN, iDataLen, timeOut, buf);

	if (SCSI_IOCTL_DATA_IN==direc)				sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	else if (SCSI_IOCTL_DATA_OUT==direc)		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	else if(SCSI_IOCTL_DATA_UNSPECIFIED==direc)	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	else return 3;	

	if(SCSI_IOCTL_DATA_IN==direc) memset( buf, 0,  iDataLen);


	DeviceHandle_p = MyCreateFile(targetDisk);
	dwErr = GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE && bSCSIControl) return 2;	//dwErr;

	double startT=clock();
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = NetDeviceIoControl( DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);
	dwErr=GetLastError();
	MyCloseHandle(&DeviceHandle_p);
	double endT=clock();
	double duration=endT-startT;
	if(((duration/CLOCKS_PER_SEC)>180) && !_CloseTimeout){
		if(_pSortingBase!=NULL){
			_pSortingBase->Log("TO CDB=[%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X], %s, size=%d",
				CDB[0],CDB[1],CDB[2],CDB[3],CDB[4],CDB[5],CDB[6],CDB[7],CDB[8],CDB[9],CDB[10],CDB[11],CDB[12],CDB[13],CDB[14],CDB[15],direcStr,iDataLen);
			return 2;
		}
	}

	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
	{
		//assert(0);
		return dwErr;
	}
#endif
	return 0;
}


int CNTCommand::NT_GetErrCEInfo30(BYTE bType,HANDLE DeviceHandle,char targetDisk, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )	return 1;

	memset( buf, 0, 18);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xA4;
	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	return iResult;


}

// note : buf, min 18 Bytes
int CNTCommand::NT_RequestSense(BYTE bType,HANDLE DeviceHandle,char targetDisk, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )	return 1;

	memset( buf, 0, 18);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 3;
	CDB[4] = 18;
	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 18, buf, 60);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 18, buf, 60);
	return iResult;

}
int CNTCommand::NT_JumpISPModeDisconnect(BYTE bType,HANDLE DeviceHandle_p,char drive)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xBF;
	CDB[4] = 8;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	

	Sleep(80);

	return iResult;
}

int CNTCommand::NT_JumpISPModeDisconnect_Security(BYTE bType,HANDLE DeviceHandle_p,char drive)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xBF;
	CDB[2] = 0x01;
	CDB[4] = 0x08;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	

	Sleep(80);

	return iResult;
}

int CNTCommand::NT_JumpISPMode63(BYTE bType,HANDLE DeviceHandle_p,char drive)
{

	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53JumpISPMode(drive);
	//#endif
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xBF;
	CDB[2] = 1;
	CDB[4] = 8;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	

	Sleep(80);

	return iResult;
}
int CNTCommand::NT_JumpFWISPMode(BYTE bType,HANDLE DeviceHandle_p,char drive)
{

	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53JumpISPMode(drive);
	//#endif
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xBF;
	CDB[4] = 8;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	

	Sleep(80);

	return iResult;
}

int CNTCommand::NT_JumpISPMode30(BYTE bType,HANDLE DeviceHandle_p,char drive)
{

	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53JumpISPMode(drive);
	//#endif
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB3;
	CDB[4] = 0x00;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 100);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 1000);	

	Sleep(80);

	return iResult;
}



int CNTCommand::NT_JumpISPMode(BYTE bType,HANDLE DeviceHandle_p,char drive)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53JumpISPMode(drive);
	//#endif

	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB3;
	CDB[4] = 8;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 60);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 60);	

	Sleep(80);

	return iResult;
}
int CNTCommand::NT_JumpBootCode(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char mode)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53JumpISPMode(drive);
	//#endif

	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB3;
	CDB[2] = mode;
	CDB[4] = 8;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 60);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 60);	

	Sleep(80);

	return iResult;
}
int CNTCommand::NT_JumpBootCodeByST(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char mode)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53JumpISPMode(drive);
	//#endif

	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB3;
	CDB[2] = mode;
	CDB[4] = 0;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 60);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 60);	

	Sleep(80);

	return iResult;
}
//add by kim
int CNTCommand::NT_JumpISPMode_2312Controller(BYTE bType,HANDLE DeviceHandle_p,char drive)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB3;
	//CDB[4] = 8;

	CDB[2] = 0x09;//bit 0:Return CSW 1:USB Disconnect 3:Jump to Boot Code
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 30);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 30);	

	Sleep(80);

	return iResult;
}


int CNTCommand::NT_VerifyBurner(BYTE bType,HANDLE DeviceHandle,char drive)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof( CDB ));

	CDB[0] = 0x06;
	CDB[1] = 0xB2;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,CDB, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 30);
	Sleep(80);

	return iResult;
}



int CNTCommand::NT_VerifyFW(BYTE bType,HANDLE DeviceHandle,char drive,unsigned char mode)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof( CDB ));

	CDB[0] = 0x06;
	CDB[1] = 0xB2;
	CDB[2] = mode;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,CDB, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 30);
	Sleep(80);

	return iResult;
}
/*
----------------------------------------------------------------------------- 
2301 Burner V1.12
11th.Oct.2011
-----------------------------------------------------------------------------
1. Add TSB 24nm D2: TC58NVG5D2HTA00 98 d7 94 32 76 56 09
2. Add command to verify block. 
CDB[0] 0x06
CDB[1] 0x09 (Data-in 512 byte)
CDB[6] Number of block to verify. 0~n.
CDB[7] Number of pages to verify.
CDB[9] Bad option.
Result:
CH0         CH1         ...       CH7
CE0  [bad][err]  [bad][err]  ...      [bad][err]
CE1  [bad][err]  [bad][err]  ...      [bad][err]
CE31 [bad][err]  [bad][err]  ...      [bad][err]*/
int CNTCommand::NT_BurnerVerifyFlash30(BYTE bType,HANDLE DeviceHandle,char drive, unsigned char *buf,BYTE block,BYTE page,BYTE badOption)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53GetISPStatus(drive,s1);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];
	//unsigned char buf[64];

	//if( s1==NULL ) return 1;
	//*buf = 0;				

	memset(buf,0 ,512);
	memset( buf, 0, sizeof(buf));
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0x09;
	CDB[6] = block;
	CDB[7] = page;
	CDB[9] = badOption;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 500);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 500);
	//memcpy(s1,buf,64);
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_GetISPStatus30(BYTE bType,HANDLE DeviceHandle,char drive, unsigned char *s1)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53GetISPStatus(drive,s1);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];
	unsigned char buf[64];

	if( s1==NULL ) return 1;
	*s1 = 0;				

	memset( buf, 0, sizeof(buf));
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB0;
	CDB[4] = 64;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 64, buf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,CDB, 16 ,SCSI_IOCTL_DATA_IN, 64, buf, 30);
	memcpy(s1,buf,64);
	Sleep(80);

	return iResult;
}
int CNTCommand::NT_GetISPStatus(BYTE bType,HANDLE DeviceHandle,char drive, unsigned int *s1)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53GetISPStatus(drive,s1);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];
	unsigned char buf[8];

	if( s1==NULL ) return 1;
	*s1 = 0;				

	memset( buf, 0, sizeof(buf));
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB0;
	CDB[4] = 0x08;
	_realCommand = 1;
	_ForceNoSwitch = 1;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 8, buf, 2);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,CDB, 16 ,SCSI_IOCTL_DATA_IN, 8, buf, 2);
	*s1 = buf[0];
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_GetCommandSequence(BYTE bType,HANDLE DeviceHandle,char drive, unsigned int *s1)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53GetISPStatus(drive,s1);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];
	unsigned char buf[8];

	if( s1==NULL ) return 1;
	*s1 = 0;				

	memset( buf, 0, sizeof(buf));
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0x2F;
	_realCommand = 1;
	_ForceNoSwitch = 1;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 8, buf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,CDB, 16 ,SCSI_IOCTL_DATA_IN, 8, buf, 30);
	*s1 = buf[0];
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_TransferISPData(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int length, unsigned char mode, unsigned char verify, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53TransferISPData(drive,address,length,mode,verify,buf);

	//#endif
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = verify? 0xB2:0xB1;
	CDB[2] = mode;
	CDB[3] = (unsigned char)((address>>8)&0xff);
	CDB[4] = (unsigned char)(address &0xff);
	CDB[7] = (unsigned char)((length>>8)&0xff);
	CDB[8] = (unsigned char)(length&0xff);
	_realCommand =1;
	_ForceNoSwitch = 1;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, length<<9, buf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, length<<9, buf, 30);
	Sleep(80);

	return iResult;
}




int CNTCommand::NT_BufferReadISPData(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int length, unsigned char mode, unsigned char verify, unsigned char *buf)
{

	nt_log("%s",__FUNCTION__);
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53BufferReadISPData(drive,address,length,mode,verify,buf);

	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0x21;
	CDB[2] = mode;
	CDB[3] = (unsigned char)((address>>8)&0xff);
	CDB[4] = (unsigned char)(address &0xff);
	CDB[7] = (unsigned char)((length>>8)&0xff);
	CDB[8] = (unsigned char)(length&0xff);
	_realCommand =1;
	_ForceNoSwitch = 1;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, length<<9, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, length<<9, buf, 10);
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_BufferReadISPData30(BYTE bType,HANDLE DeviceHandle,char drive, unsigned long address, int length, unsigned char mode, unsigned char verify, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);

	//if(gTC.sLoadBurner != "1")
	//	return NT_M53BufferReadISPData(drive,address,length,mode,verify,buf);

	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB2;
	CDB[2] = mode;
	CDB[3] = (unsigned char)((address>>8)&0xff);
	CDB[4] = (unsigned char)(address &0xff);
	CDB[7] = (unsigned char)((length>>8)&0xff);
	CDB[8] = (unsigned char)(length&0xff);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, length<<9, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,(mode&0x10)?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, length<<9, buf, 10);
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_ISPSwitchMode(BYTE bType,HANDLE DeviceHandle,char drive, unsigned char mode,unsigned char *DriveInfo)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53ISPSwitchMode(drive,mode);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB3;

	CDB[2] = mode ;	//BootROM --ISP burner--> burner = 0, Burner --ISP firmware--> BootROM = 1.
	//CDB[3] = 0xFF;
	_realCommand =1;
	_ForceNoSwitch = 1; 
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 200);
	else if(bType == 1){
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 200);
	}
	else if(bType == 2){
		iResult = ST_CustomCmd_Path(DriveInfo, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 200);
	}
	else{
		//_assert(0);
	}
	Sleep(80);

	return iResult;
}



//---------------------------------------------------------------------------
// for Burner erase all
// ID_Block_Data[0x1F0]). 
int CNTCommand::NT_ISPGetFlash(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf, WORD wBufSize)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53ISPGetFlash(drive,bBuf,wBufSize);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];
	int iLen = 1088;

	if( bBuf==NULL )	return 1;
	if( wBufSize<iLen ) return 1;

	memset( bBuf, 0, wBufSize );
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB5;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, iLen, bBuf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, iLen, bBuf, 5);

	return iResult;
}

int CNTCommand::NT_ISPGetConnectType(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf, WORD wBufSize)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	if( bBuf==NULL )	return 1;

	memset( bBuf, 0, wBufSize );
	memset( CDB , 0 , sizeof(CDB) );

	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 0x52;
	CDB[3] = 0x41;
	CDB[4] = 0xf0;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, wBufSize, bBuf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, wBufSize, bBuf, 5);

	return iResult;
}

int CNTCommand::NT_ISPTestConnect(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf, WORD wBufSize)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	if( bBuf==NULL )	return 1;

	memset( bBuf, 0, wBufSize );
	memset( CDB , 0 , sizeof(CDB) );

	CDB[0] = 0x06;
	CDB[1] = 0x06;
	CDB[2] = 0x55;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, wBufSize, bBuf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, wBufSize, bBuf, 5);

	return iResult;
}

int CNTCommand::NT_ISP_EraseAllBlk30(BYTE bType,HANDLE DeviceHandle,char drive, BYTE bFlashIndex,BYTE bDieIndex,int bEraseUnitCount, BYTE bEraseMode, int iStartBlock)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53ISP_EraseAllBlk(drive,bFlashIndex,bEraseMode);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];
	int iLen = 0;

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB7;
	CDB[2] = 0x00;
	CDB[3] = bFlashIndex;
	CDB[4] = bDieIndex;
	CDB[5] = iStartBlock >> 8;		//BEGIN_BLOCK_H
	CDB[6] = iStartBlock & 0xFF;	//END_BLOCK_L

	CDB[7] = bEraseUnitCount >> 8;		//END_BLOCK_H
	CDB[8] = bEraseUnitCount & 0xFF;	//END_BLOCK_L

	CDB[9] = bEraseMode; 
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, iLen, NULL, 30000);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, iLen, NULL, 30000);


	return iResult;
}
int CNTCommand::NT_ISP_Bics3WaferErase(BYTE bType,HANDLE DeviceHandle,char drive, BYTE bFlashIndex,BYTE bDieIndex, int iStartBlock)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53ISP_EraseAllBlk(drive,bFlashIndex,bEraseMode);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];
	int iLen = 0;

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0x01;
	CDB[2] = 0x00;
	CDB[3] = bFlashIndex;
	CDB[4] = bDieIndex;
	CDB[5] = iStartBlock >> 8;	
	CDB[6] = iStartBlock & 0xFF;
	//CDB[9] = bEraseMode; 
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, iLen, NULL, 1200);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, iLen, NULL, 1200);


	return iResult;
}
int CNTCommand::NT_ISP_EraseAllBlk(BYTE bType,HANDLE DeviceHandle,char drive, BYTE bFlashIndex, BYTE bEraseMode)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53ISP_EraseAllBlk(drive,bFlashIndex,bEraseMode);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];
	int iLen = 0;

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB7;
	CDB[2] = 0x00;
	CDB[3] = bFlashIndex;

	CDB[9] = bEraseMode; 
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, iLen, NULL, 1200);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, iLen, NULL, 1200);


	return iResult;
}

int CNTCommand::NT_ISP_EraseAllBlk_2312Controller(BYTE bType,HANDLE DeviceHandle,char drive,BYTE *bBuf, BYTE bEraseMode)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53ISP_EraseAllBlk(drive,bFlashIndex,bEraseMode);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];
	int direc;
	//int iLen = 0;

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xBF;
	if (bEraseMode==0)//Random data read
	{
		CDB[10]= 0xB7;
		direc=SCSI_IOCTL_DATA_IN;
	}
	else if (bEraseMode==1)//Verify & Erase
	{
		CDB[10]= 0xB8;
		direc=SCSI_IOCTL_DATA_OUT;
	}
	else
		return 1;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,direc, 512, bBuf, 300);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,direc, 512, bBuf, 300);


	return iResult;
}
int CNTCommand::NT_GetIDBlk(BYTE bType,HANDLE DeviceHandle,char obDrv, BYTE *idblk)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset(CDB,0,16);
	CDB[0] = 0x06;
	CDB[1] = 0x03;
	CDB[9] = 0x81;
	if(bType == 0)
		return NT_CustomCmd(obDrv, CDB, 16, 1, 1088, idblk, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, 1, 1088, idblk, 10);
}


int CNTCommand::NT_ISPIssueReadPage(BYTE bType,HANDLE DeviceHandle,char drive ,unsigned char *buf,unsigned char pageL, unsigned char pageH,
									UCHAR mode,unsigned char g_ucFCE,unsigned char FZone,
									unsigned char FBlock_H,unsigned char FBlock_L)
{
	nt_log("%s",__FUNCTION__);
	UINT wBufSize;
	BOOL result=FALSE;
	if (mode & RMODE_26SPR) /* force 26 byte read */
	{
		wBufSize = 0x200 + 26;
	}
	else
	{
		wBufSize = 0x200;
	}

	memset((void *)(buf), 0x0, wBufSize);
	unsigned char CDB[16];
	memset((void *)&CDB, 0x0, sizeof(CDB));
	CDB[0] = 0x06;//SCSIOP_PS_CMD;
	CDB[1] = 0x03;//SCSIOP_PS_SUBCMD_DIRECT_READ;
	CDB[3] = g_ucFCE;
	CDB[4] = FZone;
	CDB[5] = FBlock_H;
	CDB[6] = FBlock_L;

	CDB[7] = pageL;
	CDB[8] = pageH;
	CDB[9] = mode;

	BYTE iResult =0;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN,wBufSize, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN,wBufSize, buf, 10);
	return iResult;
}



int CNTCommand::NT_ISPIssueConfigFalsh(BYTE bType,HANDLE DeviceHandle,char drive,unsigned char *buf,unsigned char g_ucFCE,unsigned char FZone,unsigned char FBlock_H,unsigned char FBlock_L)
{
	nt_log("%s",__FUNCTION__);
	unsigned char iResult;
	unsigned char CDB[16];
	//unsigned int Status;
	memset(CDB,0,16);
	CDB[0] = 0x06 ;//SCSIOP_PS_CMD;
	CDB[1] = 0xB4 ;//SCSIOP_PS_SUBCMD_DIRECT_ERASE;
	CDB[3] = g_ucFCE;
	CDB[4] = FZone;
	CDB[5] = FBlock_H;
	CDB[6] = FBlock_L;

	CDB[7] = 0x00;
	CDB[8] = 0x00;
	CDB[9] = 0x00;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_OUT,512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_OUT,512, buf, 10);
	//Sleep(80);
	return iResult;

}

int CNTCommand::NT_CustomCmd_in_out_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut)
{
	nt_log("%s",__FUNCTION__);
	BOOL status  = 0;
	ULONG length = 0, returned = 0;
	BYTE data1[512] = {0};
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb_out;
	//HANDLE DeviceHandle_p = NULL; 

	if (0 > bCDBLen || 16 < bCDBLen)	return 1;
	if( CDB==NULL )						return 1;
	if( (iDataLen!=0)&&(buf==NULL) )	return 1;
	if( iDataLen>SCSI_MAX_TRANS )		return 1;

	//	DeviceHandle_p = MyCreateFile_Phy(bPhyNum);
	//dwErr = GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE) return 2;	//0xff;	//dwErr;

	SCSIPtdBuilder(&sptdwb, bCDBLen, CDB, SCSI_IOCTL_DATA_IN, iDataLen, timeOut, buf);
	SCSIPtdBuilder(&sptdwb_out, bCDBLen, CDB, SCSI_IOCTL_DATA_IN, 512, timeOut, data1);

	if (SCSI_IOCTL_DATA_IN==direc)				sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	else if (SCSI_IOCTL_DATA_OUT==direc)		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	else if(SCSI_IOCTL_DATA_UNSPECIFIED==direc)	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	else return 0x3;	

	sptdwb_out.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	memset( data1, 0,  512);
	if(SCSI_IOCTL_DATA_IN==direc) memset( buf, 0,  iDataLen);

	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl( DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb_out,
		length,
		&returned,
		FALSE);
	//dwErr=NetGetLastError();
	//MyCloseHandle(&DeviceHandle_p);
	memcpy(m_CMD,CDB,16);
	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return GetLastError();		
	return 0;
}

int CNTCommand::NT_CustomCmd_Common(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut)
{
	bool isRead = (direc == SCSI_IOCTL_DATA_IN)?true:false;
	return MyIssueCmdHandle(DeviceHandle_p, CDB, isRead, buf, iDataLen, timeOut, IF_USB);
#if 0
	BOOL status  = 0;
	ULONG length = 0, returned = 0;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	//HANDLE DeviceHandle_p = NULL; 
	DWORD dwErr = 0;
	unsigned char *bankBuf = NULL;

	if (0 > bCDBLen || 16 < bCDBLen)	return 1;
	if( CDB==NULL )						return 1;
	if( (iDataLen!=0)&&(buf==NULL) )	return 1;
	if( iDataLen>SCSI_MAX_TRANS )		return 1;
	// 	wchar_t ttmp[512];
	// 	wsprintfW(ttmp, L"[CDB] %02X %02X %02X %02X", CDB[0],CDB[1],CDB[2],CDB[3]);
	// 	OutputDebugStringW(ttmp);
	//	DeviceHandle_p = MyCreateFile_Phy(bPhyNum);
	dwErr = GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE && bSCSIControl) return 2;	//0xff;	//dwErr;

	_ForceNoSwitch = 0;

	SCSIPtdBuilder(&sptdwb, bCDBLen, CDB, SCSI_IOCTL_DATA_IN, iDataLen, timeOut, buf);

	if (SCSI_IOCTL_DATA_IN==direc)				sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	else if (SCSI_IOCTL_DATA_OUT==direc)		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	else if(SCSI_IOCTL_DATA_UNSPECIFIED==direc)	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	else return 0x3;	

	if(SCSI_IOCTL_DATA_IN==direc) memset( buf, 0,  iDataLen);

	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl( DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);
	if(status ==0 )
		dwErr=GetLastError();
	//MyCloseHandle(&DeviceHandle_p);
	/*if (dwErr != 0)
	{
	char Buf[256] ;
	sprintf(Buf, "<IO Error = %4d, SCSI Command Error Code = %4d>\r\n",status, dwErr);
	OutputDebugString(Buf);
	}*/


	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
	{
		//assert(0);
		return dwErr;
	}

	return 0;
#endif
}

int CNTCommand::GetEccRatio(void)
{
	return 1;
}
int CNTCommand::NT_CustomCmd_ATA_IO(BYTE bType, HANDLE deviceHandle, char targetDisk, unsigned char *CDB, unsigned char bCDBLen, int direct, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool copyBuf)
{
	return 0;
}
int CNTCommand::NT_CustomCmd_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool is512CDB)
{
#if 1
	BOOL status  = 0;
	ULONG length = 0, returned = 0;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	memset(&sptdwb, 0, sizeof(sptdwb));
	//HANDLE DeviceHandle_p = NULL; 
	DWORD dwErr = 0;
	unsigned char *bankBuf = NULL;

	if (0 > bCDBLen || 16 < bCDBLen)	return 1;
	if( CDB==NULL )						return 1;
	if( (iDataLen!=0)&&(buf==NULL) )	return 1;
	if( iDataLen>SCSI_MAX_TRANS )		return 1;
	// 	wchar_t ttmp[512];
	// 	wsprintfW(ttmp, L"[CDB] %02X %02X %02X %02X", CDB[0],CDB[1],CDB[2],CDB[3]);
	// 	OutputDebugStringW(ttmp);
	//	DeviceHandle_p = MyCreateFile_Phy(bPhyNum);
	//dwErr = GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE && bSCSIControl) return 2;	//0xff;	//dwErr;

	char direcStr[128];
	if(direc==SCSI_IOCTL_DATA_IN)
		sprintf(direcStr,"DATA IN");
	else if(direc==SCSI_IOCTL_DATA_OUT)
		sprintf(direcStr,"DATA OUT");
	else
		sprintf(direcStr," ");
	if ( _SetDumpCDB ) {
		_pSortingBase->Log("CDB=[%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X], %s, size=%d",
			CDB[0],CDB[1],CDB[2],CDB[3],CDB[4],CDB[5],CDB[6],CDB[7],CDB[8],CDB[9],CDB[10],CDB[11],CDB[12],CDB[13],CDB[14],CDB[15],direcStr,iDataLen);
	}
	//Find Command in Bank database
	if(isFWBank && (CDB[0] == 0x06) && (_bankBufTotal != NULL) && (_ForceNoSwitch == 0) )//if _bankBufTotal == NULL -> MP Calling,don't check bank
	{
		//OutputDebugString("ST BANK1");
		int Bank = BankSearchByCmd(CDB);
		if(Bank == -1)	//Error
		{
			assert(0);
			return 1;
		}
		if(Bank == 0xff) //Error
		{
			assert(0);
			if(_pSortingBase!=NULL){
				_pSortingBase->Log("[Command Bank 0xff] CDB=[%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X], %s, size=%d",
					CDB[0],CDB[1],CDB[2],CDB[3],CDB[4],CDB[5],CDB[6],CDB[7],CDB[8],CDB[9],CDB[10],CDB[11],CDB[12],CDB[13],CDB[14],CDB[15],direcStr,iDataLen);
				_pSortingBase->OutReport(NTCmd_IDMS_ERR_CommandBankFF, -1);
			}
			return 1;
		}
		/*if(_debugBankgMsg){
		if(CDB[2]!= 0x53)
		{
		LogBank(Bank);
		unsigned char *bankBuf_cmp = new unsigned char[BankSize*1024];
		memset(bankBuf_cmp,0,BankSize*1024);
		ST_BankBufCheck(1,DeviceHandle_p,NULL,bankBuf_cmp);
		delete[] bankBuf_cmp;

		}
		}*/
		if(_alwaySyncBank&&((Bank != currentBank)&&(Bank!=8)&&(Bank!=0x99))){	// James test , for SQL don't send 53 cmd everytime
			this->ST_BankSynchronize(1,DeviceHandle_p,0);
		}

		//unsigned char *bankBuf_cmp1 = new unsigned char[BankSize*1024];
		//ST_BankBufCheck(1,DeviceHandle_p,NULL,bankBuf_cmp1);
		//int a=bankBuf_cmp1[4];
		//delete bankBuf_cmp1;

		if(Bank != currentBank && (Bank!=0x99))//if command not in current bank & not in common,do switch
		{
			//get bank buf
			bankBuf = new unsigned char[BankSize*1024];
			int bankOffset = 512 + BankCommonSize*1024 + Bank*BankSize*1024;//Tag 512,Common 20K
			memset(bankBuf,0,BankSize*1024);
			memcpy(bankBuf,_bankBufTotal+bankOffset,BankSize*1024);
			if(ST_BankSwitch(1,DeviceHandle_p,0,bankBuf)){
				delete[] bankBuf;
				return 1;
			}
			currentBank = Bank;
			delete[] bankBuf;
		}
	}

	_ForceNoSwitch = 0;

	//SCSIPtdBuilder(&sptdwb, bCDBLen, CDB, SCSI_IOCTL_DATA_IN, iDataLen, timeOut, buf);

	if (SCSI_IOCTL_DATA_IN==direc)				sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	else if (SCSI_IOCTL_DATA_OUT==direc)		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	else if(SCSI_IOCTL_DATA_UNSPECIFIED==direc)	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	else return 0x3;	


	_LastIndex++;
	_LastIndex = _LastIndex%5;

	//CDB
	if (SCSI_NTCMD == m_cmdtype){
		memcpy(&_LastCdb[_LastIndex][0], CDB, 16);

	} else {
		if (buf && iDataLen>=16)
			memcpy(&_LastCdb[_LastIndex][0], buf, 16);
	}

	//buffer
	if(SCSI_IOCTL_DATA_IN==direc){
		memset( buf, 0,  iDataLen);
		if(buf){
			memset(&_LastBuf[_LastIndex], 0,  sizeof(_LastBuf[_LastIndex]));
			memcpy(&_LastBuf[_LastIndex][0], buf, iDataLen>=32 ? 32: iDataLen );
		}

	} else if (SCSI_IOCTL_DATA_OUT==direc){

		if (SCSI_NTCMD == m_cmdtype){
			if (buf)
				memcpy(&_LastBuf[_LastIndex][0], buf, iDataLen>=32 ? 32: iDataLen );
		} else {
			if (buf || iDataLen>=512)
				memcpy(&_LastBuf[_LastIndex][0], buf+512, (iDataLen - 512) >=32 ? 32: (iDataLen - 512) );
		}
	}

	double startT=clock();
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	int isResumed = 0;

	if (INVALID_HANDLE_VALUE == _hUsb && m_recovered_handle) {
		nt_file_log("skip device I/O CTL!");
		return 2;
	}
	for(int ii=0;ii<2;ii++)
	{
		//status = DeviceIoControl( m_recovered_handle== true?m_deviceHandle:DeviceHandle_p,
		//	IOCTL_SCSI_PASS_THROUGH_DIRECT,
		//	&sptdwb,
		//	length,
		//	&sptdwb,
		//	length,
		//	&returned,
		//	FALSE);
		//if(status != 0) 
		//	break;
		//else{
		//	dwErr=GetLastError();
		//	if(dwErr == 0x79) break; //time out
		//}
		status = MyIssueCmdHandle(DeviceHandle_p, CDB, (SCSI_IOCTL_DATA_IN==direc)?true:false, buf, iDataLen, timeOut, IF_USB, bCDBLen);
		if (0 == status || -2 == status)
			break;
		if (!_retryIoControl)
			break;
		Sleep(100);
	}
	

	//MyCloseHandle(&DeviceHandle_p);
	/*if (dwErr != 0)
	{
	char Buf[256] ;
	sprintf(Buf, "<IO Error = %4d, SCSI Command Error Code = %4d>\r\n",status, dwErr);
	OutputDebugString(Buf);
	}*/
	double endT=clock();
	double duration=endT-startT;
	//if(((duration/CLOCKS_PER_SEC)>180) && !_CloseTimeout){
	//	if(_pSortingBase!=NULL){
	//		_pSortingBase->Log("TO CDB=[%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X], %s, size=%d",
	//			CDB[0],CDB[1],CDB[2],CDB[3],CDB[4],CDB[5],CDB[6],CDB[7],CDB[8],CDB[9],CDB[10],CDB[11],CDB[12],CDB[13],CDB[14],CDB[15],direcStr,iDataLen);
	//		return 2;
	//	}
	//}
	//	

	if(status && -2!=status)
	{
		dwErr=GetLastError();
		SetLastError(0);
	}
	if ((((duration/CLOCKS_PER_SEC)>180) && !_CloseTimeout) ||
		(0 != dwErr)
		)
	{
		char ttt[512];
		memset(ttt,0,512);
		sprintf(ttt,">>> %f,%d,%d,%d,%d",duration,_CloseTimeout,sptdwb.sptd.ScsiStatus,dwErr,status);
		OutputDebugString(ttt);
		if (_pSortingBase != NULL)
		{
			_pSortingBase->Log(ttt);
			_pSortingBase->Log("NT_CustomCmd_Phy:Print Last %d command", _LastIndex + 1);
		}

		for (int i = _LastIndex; i>=0; i--) {
			if (_pSortingBase != NULL) {
				_pSortingBase->Log("TO CDB=[%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]",
					_LastCdb[i][0],_LastCdb[i][1],_LastCdb[i][2],_LastCdb[i][3],_LastCdb[i][4],_LastCdb[i][5],_LastCdb[i][6],_LastCdb[i][7],_LastCdb[i][8],_LastCdb[i][9],_LastCdb[i][10],_LastCdb[i][11],_LastCdb[i][12],_LastCdb[i][13],_LastCdb[i][14],_LastCdb[i][15]);
				_pSortingBase->Log("TO BUF=[%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]",
					_LastBuf[i][0],_LastBuf[i][1],_LastBuf[i][2],_LastBuf[i][3],_LastBuf[i][4],_LastBuf[i][5],_LastBuf[i][6],_LastBuf[i][7],_LastBuf[i][8],_LastBuf[i][9],_LastBuf[i][10],_LastBuf[i][11],_LastBuf[i][12],_LastBuf[i][13],_LastBuf[i][14],_LastBuf[i][15]);
			}
		}

		if ((((duration/CLOCKS_PER_SEC)>180) && !_CloseTimeout)){
			OutputDebugString("ST BANK4");
			return 2;
		}
	}
    _isIOing = false;
	return status;
#endif
	return 0;
}
/*
HANDLE CNTCommand::MyCreateFile__(char bDrv)
{
DWORD  accessMode = 0, shareMode = 0;   
char   devicename[8];
HANDLE DeviceHandle_p = INVALID_HANDLE_VALUE;

memset(devicename, 0, sizeof(devicename));
sprintf (devicename, "\\\\.\\%1C:", bDrv);
shareMode  = FILE_SHARE_READ | FILE_SHARE_WRITE;  
accessMode = GENERIC_WRITE | GENERIC_READ;       

DeviceHandle_p = CreateFile( devicename, 
accessMode, shareMode, NULL,
OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

//if( DeviceHandle_p==INVALID_HANDLE_VALUE )

return DeviceHandle_p;
}
*/


void CNTCommand::MyCloseVolume(HANDLE *hDev)
{
	CloseHandle(*hDev);
	*hDev = INVALID_HANDLE_VALUE;
}

int CNTCommand::MyUnLockVolume(BYTE bType, HANDLE *hDeviceHandle)	
{
	//return 0;
	OutputDebugString("My UnLockVolume Function !!\r\n");
	if( bType== 0)
	{
		if (UnlockVolume( *hDeviceHandle))
		{
			MyCloseVolume( hDeviceHandle );
			//OutReport(IDMS_ERR_UnLockVolume);
			return 4;
		}
		MyCloseVolume(hDeviceHandle);
	}
	else if(bType == 1)
	{
		if (UnlockVolume( *hDeviceHandle))
		{
			MyCloseVolume( hDeviceHandle );
			//OutReport(IDMS_ERR_UnLockVolume);
			return 4;
		}
		//MyCloseVolume(hDeviceHandle);
	}

	Sleep(10);
	return 0;
}

int CNTCommand::SetDumpWriteReadAPParameter(HANDLE h, unsigned char * ap_parameter)
{
	CharMemMgr paramBuf( 1024 );
	unsigned char* pParamBuf = paramBuf.Data();
	memcpy(pParamBuf,ap_parameter,paramBuf.Size());
	pParamBuf[0x00] = pParamBuf[0x00] & (~(0x20)); //close onfi
	pParamBuf[0x10] = pParamBuf[0x10] & 0x7F; //disable conversion -- bit8 = 0;
	pParamBuf[0x15] &= ~0x01;//close ecc	
	pParamBuf[0x0f] = 0x00; //close spare
	pParamBuf[0x0d] = 0x00; //close ecc
	pParamBuf[0x14] |= 0x20;//close inverse
	pParamBuf[0x27] |= 0x02;
	pParamBuf[0x20] = 0;// clear erase cnt
	ST_SetGet_Parameter(1, h, 0,true, pParamBuf);
	return 0;
}

int CNTCommand::MyLockVolume(BYTE bType, BYTE bDrv, HANDLE *hDeviceHandle)
{
	//*hDeviceHandle = INVALID_HANDLE_VALUE;
	//return 0;
	OutputDebugString("MyLockVolume ");
	if (bType == 0)
	{
		*hDeviceHandle = MyOpenVolume(bDrv);
	}
	else if (bType == 2) {
		*hDeviceHandle = MyOpenVolume(bDrv);
	}

	if (*hDeviceHandle == INVALID_HANDLE_VALUE) return 2;

	FlushFileBuffers(*hDeviceHandle);

	if (bType == 0)
	{
		if (LockVolume(*hDeviceHandle))
		{
			MyCloseVolume(hDeviceHandle);
			//OutReport(IDMS_ERR_LockVolume);
			*hDeviceHandle = NULL;

			return 3;
		}
		if (DismountVolume(*hDeviceHandle))
		{
			MyCloseVolume(hDeviceHandle);
			*hDeviceHandle = NULL;
			//OutReport(IDMS_ERR_LockVolume);

			return 3;
		}

	}
	if (bType == 1)
	{
		OutputDebugString("bType == 1\r\n");
		if (LockVolume(*hDeviceHandle)) {
			Sleep(200);
			if (LockVolume(*hDeviceHandle) != 5)
			{
				OutputDebugString("LockVolume1 != 5");
				Sleep(1000);//kim modify
				*hDeviceHandle = MyOpenVolume(bDrv);
				LockVolume(*hDeviceHandle);
				if (LockVolume(*hDeviceHandle) != 5)
				{
					OutputDebugString("LockVolume2 != 5");
					Sleep(1000);//kim modify
					*hDeviceHandle = MyOpenVolume(bDrv);
					LockVolume(*hDeviceHandle);

				}
			}
		}
		if (DismountVolume(*hDeviceHandle))

		{
			//MyCloseVolume( hDeviceHandle );
			//*hDeviceHandle = NULL;
			//OutReport(IDMS_ERR_LockVolume);
			//return 3;
		}
		//if (UnlockVolume(*hDeviceHandle))
		//{
		//	//MyCloseVolume( hDeviceHandle );
		//	//OutReport(IDMS_ERR_LockVolume);
		//	//*hDeviceHandle = NULL;
		//	//return 3;
		//}
		//Sleep(100);
		//if (LockVolume(*hDeviceHandle))
		//{
		//	//MyCloseVolume( hDeviceHandle );
		//	//OutReport(IDMS_ERR_LockVolume);
		//	//*hDeviceHandle = NULL;
		//	//return 3;
		//}
		//LockVolume(*hDeviceHandle);
	}
	if (bType == 2)
	{
		if (LockVolume(*hDeviceHandle))
		{
			MyCloseVolume(hDeviceHandle);
			*hDeviceHandle = NULL;
			//*hDeviceHandle = MyOpenVolume(bDrv);
		}

	}
	Sleep(10);
	return 0;
}

int CNTCommand::NT_QuickWrite30(BYTE bType,HANDLE DeviceHandel,unsigned char bDrv,DWORD dwStartUnit,DWORD dwLength,WORD wValue)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[12];
	unsigned char buf[8];
	int iResult;
	memset(buf,0,8);
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0]  = 0x06;
	CDB[1]  = 0x86;
	CDB[2]  = (unsigned char)(dwStartUnit >> 24);
	CDB[3]  = (unsigned char)(dwStartUnit >> 16);
	CDB[4]  = (unsigned char)(dwStartUnit >> 8);
	CDB[5]  = (unsigned char)(dwStartUnit & 0xff);

	CDB[6]  = (unsigned char)(dwLength >> 8);
	CDB[7]  = (unsigned char)(dwLength & 0xff);

	CDB[8]  = (wValue >> 8)&0xff;
	CDB[9] = wValue & 0xff;

	if(!bType)
		iResult =  NT_CustomCmd(bDrv, CDB, 12 ,SCSI_IOCTL_DATA_IN, 0, NULL, 300);
	else
		iResult =  NT_CustomCmd_Phy(DeviceHandel, CDB, 12 ,SCSI_IOCTL_DATA_IN, 0, NULL, 300);

	return iResult;
}

int CNTCommand::NT_TuningDisk(BYTE bType,HANDLE DeviceHandel,unsigned char bDrv,unsigned char BlockSize)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[12];
	unsigned char buf[8];
	memset(buf,0,8);
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x0D;
	CDB[5] = 0x01;
	CDB[10] = 0xA5;
	CDB[11] = BlockSize;
	int iResult = 0;
	if(!bType)
		iResult =  NT_CustomCmd(bDrv, CDB, 12 ,SCSI_IOCTL_DATA_IN, 8, buf, 60);
	else
		iResult =  NT_CustomCmd_Phy(DeviceHandel, CDB, 12 ,SCSI_IOCTL_DATA_IN, 8, buf, 60);

	return iResult;
}

int	CNTCommand::NT_Toggle2LegacyReadId(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char*buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	//if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0xf6;
	CDB[2] = 0x01;
	CDB[3] = 0x01;
	//	int iResult = NT_CustomCmd(bDrv, cmd, 12 ,SCSI_IOCTL_DATA_OUT m_Size, NULL, 10);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);

	return iResult;
}

int	CNTCommand::NT_Toggle2LegacyGetSupportId(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char*buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )	return 1;

	memset( buf, 0, 512);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xF6;
	CDB[2] = 0xB0;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);

	return iResult;
}

int	CNTCommand::NT_Toggle2LegacyDetectBitNumber(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char*buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	//if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0xf6;
	CDB[2] = 0x01;
	CDB[3] = 0x00;
	//	int iResult = NT_CustomCmd(bDrv, cmd, 12 ,SCSI_IOCTL_DATA_OUT m_Size, NULL, 10);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);

	return iResult;
}

int	CNTCommand::NT_Toggle2Lagcy(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	//if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0xf6;
	CDB[2] = 0x45;

	//	int iResult = NT_CustomCmd(bDrv, cmd, 12 ,SCSI_IOCTL_DATA_OUT m_Size, NULL, 10);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);

	return iResult;
}

int CNTCommand::GetRetryTables(unsigned char * d1_retry_table, unsigned char * retry_table)
{
#if 1
	BYTE RetryTable[4096] = {0};
	BYTE bSetRetryTable = 0;
	//int bBics3Retry0x26=0;

	UCHAR RRMode = ((_FH.bMaker == 0x01) || (_FH.bMaker == 0x45)) ? 5 : ((_FH.Design <= 0x24) ? 1 : 2);


	if (!_FH.beD3) 
	{
		if (_FH.bMaker == 0xAD ) 
		{			
			//assert(0);
			return -1;

		}
		else if ((_FH.bMaker == 0x2C) || (_FH.bMaker == 0x89)) 
		{
			if (_FH.bMaker == 0x2C && _FH.id[0] == 0x84 && _FH.id[1] == 0x64 && _FH.id[2] == 0x54 && _FH.id[3] == 0xa9)
			{
				nt_file_log("Set Retry Table (IM L95B).\r\n" );
				memcpy(RetryTable, IM_MLC_L95B_Retry, sizeof(IM_MLC_L95B_Retry));
			}
			else
			{
				nt_file_log("Set Retry Table (IM D2).\r\n" );
				memcpy(RetryTable, IM_MLC_Retry, sizeof(IM_MLC_Retry));
			}
			bSetRetryTable = 1;
		} 
		else if ((_FH.bMaker == 0x45) && (_FH.Design <= 0x19)) 
		{
			if(_FH.Design != 0x15)
			{
				nt_file_log( "+ Set Retry Table (Sandisk 19 D2)." );
				memcpy(RetryTable, SD_19nm_MLC_Retry, sizeof(SD_19nm_MLC_Retry));
				bSetRetryTable = 1;
			}
			if ((_FH.bMaker == 0x45) && (_FH.Design == 0x15)) 
			{
				nt_file_log( "+ Set Retry Table (Sandisk 1z D2)." );
				memcpy(RetryTable, SD_15nm_MLC_Retry, sizeof(SD_15nm_MLC_Retry));
				bSetRetryTable = 1;
			}
		} 
		else if ((_FH.bMaker == 0x98) && (_FH.Design == 0x16) && (!_FH.beD3))  //toshiba mlc a19 ed2
		{
			nt_file_log( "+ Set Retry Table (Sandisk 19 D2)." );
			memcpy(RetryTable, TSB_A19_MLC_Retry, sizeof(TSB_A19_MLC_Retry));
			bSetRetryTable = 1;
		}
		else if ((_FH.bMaker == 0x98) && (_FH.Design == 0x15) && (!_FH.beD3)) 
		{
			nt_file_log( "+ Set Retry Table (1z D2)." );
			memcpy(RetryTable, TSB_1Znm_MLC_Retry, sizeof(TSB_1Znm_MLC_Retry));
			bSetRetryTable = 1;
		} 
		else if (_FH.bMaker == 0xEC){
			nt_file_log( "+ Set Retry Table (SAMSUNG D2)." );
			memcpy(RetryTable, SAMSUNG_14nm_MLC_Retry, sizeof(SAMSUNG_14nm_MLC_Retry));
			bSetRetryTable = 1;
		}
		else
		{
			nt_file_log( "+ Set Retry Table (D2)." );
			memcpy(RetryTable, TSB_24nm_MLC_Retry, sizeof(TSB_24nm_MLC_Retry));
			bSetRetryTable = 1;
		}

		memcpy(d1_retry_table, RetryTable, 4096);

	}
	else
	{
		if (1)//get D1 table
		{
			if(_FH.bMaker==0xad)
			{
				bSetRetryTable = 1;
				//assert(0);
				//if(_FH.bIsHynix16TLC || _FH.bIsHynix3DTLC)
				//{
				//	if(_FH.bIsHynix16TLC)
				//		Log("**Set Retry Table (HY D3 16NM)");
				//	if(_FH.bIsHynix3DTLC)
				//		Log("**Set Retry Table (HY 3D TLC)");
				//	FL_CheckHynixRetryTable16nm(PageType, RetryTable);
				//	bSetRetryTable = 1;
				//}
				//else
				//{
				//	Log("**Set Retry Table (HY D3).\r\n");
				//	if(FL_CheckHynixRetryTable(RetryTable, _FH, PageType))
				//	{	//20150819 Annie : if fail ,填 default 值
				//		memcpy(RetryTable, HY_16nm_128Gb_TLC_D1_Retry, sizeof(HY_16nm_128Gb_TLC_D1_Retry));
				//	}
				//	bSetRetryTable = 1;	
				//}
			}
			else if ((_FH.bMaker == 0x01) || (_FH.bMaker == 0x45)) {

				int CapacityPerDie = 0;
				switch(_FH.id[0])
				{
				case 0x3A://16GB
					CapacityPerDie = (16 / (1 << (_FH.id[1] & 0x03)));
					break;

				case 0x35://pMLC
				case 0x37://pMLC
				case 0x3C://32GB
					CapacityPerDie = (32 / (1 << (_FH.id[1] & 0x03)));
					break;

				case 0x3E://64GB
					CapacityPerDie = (64 / (1 << (_FH.id[1] & 0x03)));
					break;

				case 0x48://128GB
					CapacityPerDie = (128 / (1 << (_FH.id[1] & 0x03)));  
					break;

				default:
					break;

				}
				if (_FH.bIsBics2) {
					nt_file_log( "+ Set Retry Table (SD Bics2 D1)." );
					memcpy(RetryTable, SD_3D_BiCS2_D1_Retry, sizeof(SD_3D_BiCS2_D1_Retry));
					bSetRetryTable = 1;
				}
				else if(_FH.bIsBics3) {

					if(CapacityPerDie == 16){	//128Gb
						nt_file_log( "+ Set Retry Table (SD Bics3 eD3 D1 128Gb)." );
						memcpy(RetryTable, SanDisk_D3_BiCS3_128Gb_D1_Retry, sizeof(SanDisk_D3_BiCS3_128Gb_D1_Retry));
						bSetRetryTable = 1;
					}
					if(CapacityPerDie == 32){	//256Gb
						// 						if(blMPCall || gTC.bDoMPALL){	//20191025 : Add ST MPALL
						// 							Log( "+ Set Retry Table (SD Bics3 eD3 D1 256Gb Normal + Enterprise)." );
						// 							memcpy(RetryTable, SanDisk_D3_BiCS3_256Gb_D1_Retry_Merge, sizeof(SanDisk_D3_BiCS3_256Gb_D1_Retry_Merge));
						// 							bSetRetryTable = 1;
						// 						}
						// 						else
						{
							nt_file_log( "+ Set Retry Table (SD Bics3 eD3 D1 256Gb Normal)." );
							memcpy(RetryTable, SanDisk_D3_BiCS3_256Gb_D1_Retry, sizeof(SanDisk_D3_BiCS3_256Gb_D1_Retry));
							bSetRetryTable = 1;
						}
					}
					if(CapacityPerDie == 64){	//512Gb	
						nt_file_log( "+ Set Retry Table (TSB Bics3 eD3 D1 512Gb)." );
						memcpy(RetryTable, SanDisk_D3_BiCS3_512Gb_D1_Retry, sizeof(SanDisk_D3_BiCS3_512Gb_D1_Retry));
						bSetRetryTable = 1;						
					}
					bSetRetryTable = 1;
				}
				else if(_FH.bIsBics4){
					if(CapacityPerDie == 32){	//256Gb	
						nt_file_log( "+ Set Retry Table (SD Bics4 eD3 D1 256Gb)." );
						memcpy(RetryTable, SD_3D_BiCS4_256Gb_D1_Retry, sizeof(SD_3D_BiCS4_256Gb_D1_Retry));
						bSetRetryTable = 1;	
					}
					else if(CapacityPerDie == 64){	//512Gb	
						nt_file_log( "+ Set Retry Table (SD Bics4 eD3 D1 512Gb)." );
						memcpy(RetryTable, SD_3D_BiCS4_512Gb_D1_Retry, sizeof(SD_3D_BiCS4_512Gb_D1_Retry));
						bSetRetryTable = 1;						
					}
					else{
						//Not Support yet
						bSetRetryTable = 0;	
						assert(0);
					}
				}
				else{
					nt_file_log( "+ Set Retry Table (24 eD3 D1)." );
					memcpy(RetryTable, TSB_19nm_ed3_D1_Retry, sizeof(TSB_19nm_ed3_D1_Retry));
					bSetRetryTable = 1;
				}

			} else if (_FH.Design == 0x24) {
				nt_file_log( "+ Set Retry Table (24 eD3 D1)." );
				memcpy(RetryTable, TSB_24nm_ed3_D1_Retry, sizeof(TSB_24nm_ed3_D1_Retry));
				bSetRetryTable = 1;
			} else if (_FH.Design > 0x24) {
				nt_file_log( "+ Set Retry Table (32 eD3 D1)." );
				memcpy(RetryTable, TSB_32nm_ed3_D1_Retry, sizeof(TSB_32nm_ed3_D1_Retry));
				bSetRetryTable = 1;
			} else if (_FH.Design == 0x16) {
				nt_file_log( "+ Set Retry Table (A19 eD3 D1)." );
				memcpy(RetryTable, TSB_A19nm_ed3_D1_Retry, sizeof(TSB_A19nm_ed3_D1_Retry));
				bSetRetryTable = 1;
			} else if (_FH.Design == 0x15 && _FH.bMaker == 0x98) {
				nt_file_log( "+ Set Retry Table (1z eD3 D1)." );
				memcpy(RetryTable, TSB_1Z_ed3_D1_Retry, sizeof(TSB_1Z_ed3_D1_Retry));
				bSetRetryTable = 1;
			}
			else if (_FH.Design == 0x14 && _FH.bMaker == 0x98) {
				nt_file_log( "+ Set Retry Table (TSB Bics2 D1)." );
				memcpy(RetryTable, TSB_3D_ed3_D1_Retry, sizeof(TSB_3D_ed3_D1_Retry));
				bSetRetryTable = 1;
			}
			else if (_FH.bIsBiCs4QLC) {
				nt_file_log( "+ Set Retry Table (TSB Bics4 QLC D1)." );
				memcpy(RetryTable, TSB_BiCS4_QLC_D1_Retry, sizeof(TSB_BiCS4_QLC_D1_Retry));
				bSetRetryTable = 1;
			}
			else if (_FH.bIsBics4) {
				nt_file_log( "+ Set Retry Table (Bics4 D1)." );
				memcpy(RetryTable, TSB_3D_BiCS4_D1_Retry, sizeof(TSB_3D_BiCS4_D1_Retry));
				bSetRetryTable = 1;
			}
			else if (_FH.bIsYMTC3D) {
				nt_file_log( "+ Set Retry Table (YMTC 3D D1)." );
				memcpy(RetryTable, YMTC_3D_D1_Retry, sizeof(YMTC_3D_D1_Retry));
				bSetRetryTable = 1;
			}
			else if (_FH.bIsYMTCMLC3D) {
				nt_file_log( "+ Set Retry Table (YMTC DBS MLC D1)." );
				memcpy(RetryTable, YMTC_3D_DBS_D1_Retry, sizeof(YMTC_3D_DBS_D1_Retry));
				bSetRetryTable = 1;
			}
			else if ((_FH.bMaker == 0x2C) || (_FH.bMaker == 0x89))
			{
				nt_file_log("Set Retry Table (IM D1).");
				memcpy(RetryTable, IM_MLC_Retry, sizeof(IM_MLC_Retry));
				bSetRetryTable = 1;
			}
			else {
				nt_file_log( "+ Set Retry Table (19 eD3 D1)." );
				memcpy(RetryTable, TSB_19nm_ed3_D1_Retry, sizeof(TSB_19nm_ed3_D1_Retry));
				bSetRetryTable = 1;
			}
			memcpy(d1_retry_table, RetryTable, 4096);
		}
		if (1) { //Get D3/D4 table

			if(_FH.bMaker==0xad){
				//assert(0);
				//if(_FH.bIsHynix16TLC || _FH.bIsHynix3DTLC)
				//{
				//	if(_FH.bIsHynix16TLC)
				//		Log("**Set Retry Table (HY D3 16NM)");
				//	if(_FH.bIsHynix3DTLC)
				//		Log("**Set Retry Table (HY 3D TLC)");
				//	FL_CheckHynixRetryTable16nm(PageType, RetryTable);
				//	bSetRetryTable = 1;
				//}
				//else{
				//	Log("**Set Retry Table (HY D3).\r\n");
				//	if(FL_CheckHynixRetryTable(RetryTable, _FH, PageType))
				//	{
				//		//20150819 Annie : if fail ,填 default 值
				//		memcpy(RetryTable, HY_16nm_128Gb_TLC_D3_Retry, sizeof(HY_16nm_128Gb_TLC_D3_Retry));
				//	}
				//	bSetRetryTable = 1;	
				//}
			}
			else if ((_FH.bMaker == 0x2C) || (_FH.bMaker == 0x89)) 
			{
				nt_file_log("Set Retry Table (IM D2).\r\n" );//B0k
				memcpy(RetryTable, IM_MLC_Retry, sizeof(IM_MLC_Retry));
				bSetRetryTable = 1;
			}
			else if ((_FH.bMaker == 0x01) || (_FH.bMaker == 0x45)) {
				int CapacityPerDie = 0;
				switch(_FH.id[0])
				{
				case 0x3A://16GB
					CapacityPerDie = (16 / (1 << (_FH.id[1] & 0x03)));
					break;

				case 0x3C://32GB
					CapacityPerDie = (32 / (1 << (_FH.id[1] & 0x03)));
					break;

				case 0x3E://64GB
					CapacityPerDie = (64 / (1 << (_FH.id[1] & 0x03)));
					break;

				case 0x48://128GB
					CapacityPerDie = (128 / (1 << (_FH.id[1] & 0x03)));  
					break;

				default:
					CapacityPerDie = _FH.DieCapacity/1024;
					break;
				}
				if (_FH.Design == 0x24) {
					nt_file_log( "+ Set Retry Table (24 eD3 D1)." );
					memcpy(RetryTable, TSB_19nm_ed3_D1_Retry, sizeof(TSB_19nm_ed3_D1_Retry));
					bSetRetryTable = 1;
				} else if (_FH.Design == 0x16) {
					nt_file_log( "+ Set Retry Table (Sandisk A19 D3)." );
					memcpy(RetryTable, SD_A19nm_eD3_Retry, sizeof(SD_A19nm_eD3_Retry));
					bSetRetryTable = 1;
				}
				else if (_FH.Design == 0x15 ) {
					nt_file_log( "+ Set Retry Table (1z SD eD3 D3)." );
					memcpy(RetryTable, SD_1Z_ed3_D3_Retry, sizeof(SD_1Z_ed3_D3_Retry));
					bSetRetryTable = 1;
				}
				else if(_FH.bIsBics2) {
					nt_file_log( "+ Set Retry Table (SD Bics2 D3)." );
					memcpy(RetryTable, SD_3D_BiCS2_D3_Retry, sizeof(SD_3D_BiCS2_D3_Retry));
					bSetRetryTable = 1;
				}
				else if(_FH.bIsBics3) {
					if(_FH.bMaker == 0x45){

						if(CapacityPerDie == 16){	//128Gb
							nt_file_log( "+ Set Retry Table (SD Bics3 eD3 D3 128Gb)." );
							memcpy(RetryTable, SanDisk_D3_BiCS3_128Gb_D3_Retry, sizeof(SanDisk_D3_BiCS3_128Gb_D3_Retry));
							bSetRetryTable = 1;

						}
						if(CapacityPerDie == 32){	//256Gb	
							nt_file_log( "+ Set Retry Table (TSB Bics3 eD3 D3 256Gb)." );
							memcpy(RetryTable, SanDisk_D3_BiCS3_256Gb_D3_Retry_Merge, sizeof(SanDisk_D3_BiCS3_256Gb_D3_Retry_Merge));
							bSetRetryTable = 1;	
						}
						if(CapacityPerDie == 64){	//512Gb	
							nt_file_log( "+ Set Retry Table (TSB Bics3 eD3 D3 512Gb)." );
							memcpy(RetryTable, SanDisk_D3_BiCS3_512Gb_D3_Retry, sizeof(SanDisk_D3_BiCS3_512Gb_D3_Retry));
							bSetRetryTable = 1;						
						}
					}
					else{

						nt_file_log( "+ Set Retry Table (TSB Bics3 eD3 D3)." );
						memcpy(RetryTable, TSB_3D_BiCS3_D3_Retry, sizeof(TSB_3D_BiCS3_D3_Retry));
						bSetRetryTable = 1;
					}

				}
				else if(_FH.bIsBics4){
					if(CapacityPerDie == 32){	//256Gb
						nt_file_log( "+ Set Retry Table (SD Bics4 eD3 D3 256Gb)." );
						memcpy(RetryTable, SD_3D_BiCS4_256Gb_D3_Retry, sizeof(SD_3D_BiCS4_256Gb_D3_Retry));
						bSetRetryTable = 1;
					}
					else if(CapacityPerDie == 64){	//512Gb	
						nt_file_log( "+ Set Retry Table (SD Bics4 eD3 D3 512Gb)." );
						memcpy(RetryTable, SD_3D_BiCS4_512Gb_D3_Retry, sizeof(SD_3D_BiCS4_512Gb_D3_Retry));
						bSetRetryTable = 1;
					}
					else{
						nt_file_log( "+ Set Retry Table (TSB Bics4 D3)." );
						memcpy(RetryTable, TSB_3D_BiCS4_D3_Retry, sizeof(TSB_3D_BiCS4_D3_Retry));
						bSetRetryTable = 1;
					}
				}
				else {
					nt_file_log( "+ Set Retry Table (24 eD3 D1)." );
					memcpy(RetryTable, SD_19nm_eD3_Retry, sizeof(SD_19nm_eD3_Retry));
					bSetRetryTable = 1;
				}

			} else if (_FH.Design == 0x24) {
				nt_file_log( "+ Set Retry Table (24 eD3)." );
				memcpy(RetryTable, TSB_24nm_eD3_Retry, sizeof(TSB_24nm_eD3_Retry));
				bSetRetryTable = 1;
			} else if (_FH.Design > 0x24) {
				// 32 nm Retry table
				nt_file_log( "+ Set Retry Table (32 eD3)." );
				memcpy(RetryTable, eD3_Rrtry, sizeof(eD3_Rrtry));
				bSetRetryTable = 1;
			} else if (_FH.Design == 0x16) {
				// A19 nm Retry table
				nt_file_log( "+ Set Retry Table (A19 eD3)." );
				memcpy(RetryTable, TSB_A19nm_eD3_Retry, sizeof(TSB_A19nm_eD3_Retry));
				bSetRetryTable = 1;
			}
			else if (_FH.Design == 0x15 && (_FH.bMaker == 0x98 || _FH.bMaker == 0x45))
			{
				//if (gDVar.bAPType != 0x1A)
				//{
				if(_FH.bMaker == 0x98){
					if(1){ //default not enable HV3 table
						nt_file_log( "+ Set Retry Table (1z TSB eD3 D3 )." );
						memcpy(RetryTable, TSB_1Z_ed3_D3_Retry, sizeof(TSB_1Z_ed3_D3_Retry)); //1z gen 7 retry table
					}
					else{
						nt_file_log( "+ Set Retry Table (1z TSB eD3 hv3 D3)." );
						memcpy(RetryTable, TSB_1Z_ed3_HV3_D3_Retry, sizeof(TSB_1Z_ed3_HV3_D3_Retry)); //1z gen 7 retry table

					}
				}
				else
				{
					nt_file_log( "+ Set Retry Table (1z SD eD3 D3)." );
					memcpy(RetryTable, SD_1Z_ed3_D3_Retry, sizeof(SD_1Z_ed3_D3_Retry));
				}
				//}
				//else
				//{
				//	Log( "+ Set Retry Table (1z SD eD3 D3)." );
				//	memcpy(RetryTable, SD_1Z_ed3_D3_Retry, sizeof(SD_1Z_ed3_D3_Retry));
				//}


				bSetRetryTable = 1;
			} 
			else if (_FH.Design == 0x14 && _FH.bMaker == 0x98) {
				nt_file_log( "+ Set Retry Table (TSB Bics2 eD3 D3)." );
				memcpy(RetryTable, TSB_Bics2_eD3_Retry_HV3, sizeof(TSB_Bics2_eD3_Retry_HV3));
				bSetRetryTable = 1;
			}
			else if (_FH.bIsBics2) {//tsb
				nt_file_log( "+ Set Retry Table (TSB Bics2 eD3 D3)." );
				memcpy(RetryTable, TSB_Bics2_eD3_Retry_HV3, sizeof(TSB_Bics2_eD3_Retry_HV3));
				bSetRetryTable = 1;
			}
			else if (_FH.bIsBics3) {
				if(1){ //default using HV retry
					int CapacityPerDie = 0;
					switch(_FH.id[0])
					{
					case 0x3A://16GB
						CapacityPerDie = (16 / (1 << (_FH.id[1] & 0x03)));
						break;

					case 0x3C://32GB
						CapacityPerDie = (32 / (1 << (_FH.id[1] & 0x03)));
						break;

					case 0x3E://64GB
						CapacityPerDie = (64 / (1 << (_FH.id[1] & 0x03)));
						break;

					case 0x48://128GB
						CapacityPerDie = (128 / (1 << (_FH.id[1] & 0x03)));  
						break;

					default:
						break;

					}
					if(CapacityPerDie == 16){	//128Gb
						nt_file_log( "+ Set Retry Table (TSB Bics3 eD3 D3 128Gb)." );
						memcpy(RetryTable, TSB_3D_BiCS3_128Gb_D3_Retry, sizeof(TSB_3D_BiCS3_128Gb_D3_Retry));
						bSetRetryTable = 1;
					}
					if(CapacityPerDie == 32){	//256Gb
						nt_file_log( "+ Set Retry Table (TSB Bics3 eD3 D3 256Gb)." );
						memcpy(RetryTable, TSB_3D_BiCS3_256Gb_D3_Retry, sizeof(TSB_3D_BiCS3_256Gb_D3_Retry));
						bSetRetryTable = 1;		
					}
					if(CapacityPerDie == 64){	//512Gb
						nt_file_log( "+ Set Retry Table (TSB Bics3 eD3 D3 512Gb)." );
						memcpy(RetryTable, TSB_3D_BiCS3_512Gb_D3_Retry, sizeof(TSB_3D_BiCS3_512Gb_D3_Retry));
						bSetRetryTable = 1;
					}
				}
				else{
					nt_file_log( "+ Set Retry Table (TSB Bics3 eD3 D3)." );
					memcpy(RetryTable, TSB_3D_BiCS3_D3_Retry, sizeof(TSB_3D_BiCS3_D3_Retry));
					bSetRetryTable = 1;
				}

			}

			else if (_FH.bIsBiCs4QLC) {
				nt_file_log( " + Set Retry Table (TSB Bics4 QLC D3)." );
				memcpy(RetryTable, TSB_BiCS4_QLC_D3_Retry, sizeof(TSB_BiCS4_QLC_D3_Retry));
				bSetRetryTable = 1;
			}
			else if(_FH.bIsBics4){
				int CapacityPerDie = 0;
				switch(_FH.id[0])
				{
				case 0x3A://16GB
					CapacityPerDie = (16 / (1 << (_FH.id[1] & 0x03)));
					break;

				case 0x3C://32GB
					CapacityPerDie = (32 / (1 << (_FH.id[1] & 0x03)));
					break;

				case 0x3E://64GB
					CapacityPerDie = (64 / (1 << (_FH.id[1] & 0x03)));
					break;

				case 0x48://128GB
					CapacityPerDie = (128 / (1 << (_FH.id[1] & 0x03)));  
					break;

				default:
					break;

				}
				if(CapacityPerDie == 64){	//512Gb	
					nt_file_log( "+ Set Retry Table (TSB Bics4 eD3 D3 512Gb)." );
					memcpy(RetryTable, Toshiba_D3_BiCS4_512Gb_D3_Retry, sizeof(Toshiba_D3_BiCS4_512Gb_D3_Retry));
					bSetRetryTable = 1;						
				}
				else{
					nt_file_log( "+ Set Retry Table (TSB Bics4 eD3 D3)." );
					memcpy(RetryTable, TSB_3D_BiCS4_D3_Retry, sizeof(TSB_3D_BiCS4_D3_Retry));
					bSetRetryTable = 1;
					//gTC.bBicsRetry0x26 = 1;	//20180903: Bics4 Retry下0x26
				}
			}
			else if (_FH.bIsYMTC3D) {
				nt_file_log( "+ Set Retry Table (YMTC 3D D3)." );
				memcpy(RetryTable, YMTC_3D_D3_Retry, sizeof(YMTC_3D_D3_Retry));
				bSetRetryTable = 1;
			}
			else if (_FH.bIsYMTCMLC3D==1) {
				nt_file_log( "+ Set Retry Table (YMTC DBS MLC D2)." );
				memcpy(RetryTable, YMTC_3D_DBS_D2_Retry, sizeof(YMTC_3D_DBS_D2_Retry));
				bSetRetryTable = 1;
			}
			else {
				nt_file_log( "+ Set Retry Table (19 TSB eD3 D3)." );
				memcpy(RetryTable, TSB_19nm_eD3_Retry, sizeof(TSB_19nm_eD3_Retry));
				bSetRetryTable = 1;

			}
			memcpy(retry_table, RetryTable, 4096);
		}
	}

	assert(bSetRetryTable);
	if (bSetRetryTable) {
		nt_file_log("Retry Table***************");
		char buf[2048] = {0};
		char buf1[2048] = {0};
		int group = RetryTable[0];
		int len = RetryTable[1];
		for (int g = 0; g < group; g++) {
			for (int l = 0; l < len - 1; l++) {

				sprintf(buf, "%s%02x,", buf, RetryTable[2 + (g * len) + l]);
				//StrCat(buf1,buf);
			}
			sprintf(buf, "%s%02x", buf, RetryTable[2 + (g * len) + len - 1]);

			nt_file_log(buf);
			memset(buf, 0, 2048);
		}
		return 0;
	}
#endif
	return 0;
}

int	CNTCommand::NT_Legacy2Toggle(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	//if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0xf6;
	CDB[2] = 0x46;
	CDB[4] = 0x01;


	//	int iResult = NT_CustomCmd(bDrv, cmd, 12 ,SCSI_IOCTL_DATA_OUT m_Size, NULL, 10);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);

	return iResult;
}


int	CNTCommand::NT_T2LAndL2T_Rescue(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE Type)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	//if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0xF6;
	CDB[2] = 0xAE;
	//CDB[4] = 0x0;
	//CDB[5] = Type;


	//	int iResult = NT_CustomCmd(bDrv, cmd, 12 ,SCSI_IOCTL_DATA_OUT m_Size, NULL, 10);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);

	return iResult;
}

int	CNTCommand::NT_T2LAndL2T(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,BYTE Type, BYTE DieCount, BYTE BFlashReset,BYTE bisSLC,int flashnumber)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	//BFlashReset = 0;
	//if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0xF6;
	if (bisSLC)
		CDB[2] = 0x45;
	else
		CDB[2] = 0xAD;
	CDB[3] = 0x08;
	//0x08
	CDB[4] = DieCount;
	CDB[5] = Type;
	CDB[6] = BFlashReset;

	if (flashnumber == 0)
		flashnumber = 4;
	for (BYTE i = 0; i < flashnumber; i++)
	{
		CDB[3] = i;

		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 60);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 60);

		if (iResult)
			break;
	}

	return iResult;
}
int	CNTCommand::NT_GetRetryTablePass(BYTE obType,HANDLE DeviceHandle, BYTE targetDisk, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];


	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0x87;
	
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 8192, bBuf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 8192, bBuf, 30);
	return iResult;


}
int	CNTCommand::NT_SortingCodePowerOn(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,BYTE BPowerOn)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];


	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0xF6;
	CDB[2] = 0x08;
	CDB[3] = BPowerOn;    //00=Off;01=On

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 0, NULL, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 0, NULL, 30);

	return iResult;


}

int CNTCommand::LockVolume(HANDLE hVolume)
{
	DWORD dwBytesReturned;
	DWORD dwSleepAmount;
	int nTryCount = 0;
	DWORD err;
	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;		// 6/2 = 3msec=> 300msec, 

	// Do this in a loop until a timeout period has expired
	for (nTryCount=0; nTryCount<LOCK_RETRIES; nTryCount++)
	{
		SetLastError(0);
		if ( DeviceIoControl( hVolume,
			FSCTL_LOCK_VOLUME,
			NULL, 0,
			NULL, 0,
			&dwBytesReturned,
			NULL))
		{
			Sleep(200);
			//err = GetLastError();
			return 0;	// lock

		} else{
			err = GetLastError();
		}		
		Sleep(dwSleepAmount*100);
	}

	return err;
}

int CNTCommand::UnlockVolume(HANDLE hVolume)
{
	DWORD dwBytesReturned;
	DWORD dwSleepAmount;
	int nTryCount;
	BOOL result;
	DWORD err;
	char str[256] = {0};
	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;		// 6/2 = 3, 300mSec

	// Do this in a loop until a timeout period has expired
	for (nTryCount=0; nTryCount<LOCK_RETRIES; nTryCount++)
	{
		if (result = DeviceIoControl(hVolume,
			FSCTL_UNLOCK_VOLUME,
			NULL, 0,
			NULL, 0,
			&dwBytesReturned,
			NULL))
		{
			return 0;
		}
		else
		{
			err = GetLastError();
		}

		Sleep(dwSleepAmount*100);
	}

	return err;
}

//---------------------------------------------------------------------------
int CNTCommand::NT_Direct_Erase(BYTE bType,HANDLE DeviceHandel,char targetDisk, BYTE bZone, WORD wBlk)
{	
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x1;

	CDB[4] = bZone;
	CDB[5] = (wBlk>>8);		// Hi
	CDB[6] = (BYTE)wBlk;	// Lo

	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 40);
	if(!bType)
		return NT_CustomCmd(targetDisk, CDB, 12, SCSI_IOCTL_DATA_OUT, 0, NULL, 40);
	else
		return NT_CustomCmd_Phy(DeviceHandel,CDB,12,SCSI_IOCTL_DATA_OUT,0,NULL,40);

	return iResult;
}


//---------------------------------------------------------------------------
int CNTCommand::NT_Direct_Write_Page2048(BYTE bType,HANDLE DeviceHandel,char drive , unsigned char zone ,
										 unsigned int block , unsigned int page,
										 unsigned char para , bool bIs2154CE,
										 unsigned char *buf )
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof( CDB ));
	/*
	Cmd4=FZone, Cmd5=High Byte of FBlock, Cmd6=Low Byte of FBlock
	Cmd8=High Byte of FPage, Cmd7=Low Byte of FPage,
	Cmd9 : bit 7 : ECC enable, bit 6~0 : reserved

	CDB[4] = (char)(( address >> 8 ) & 0xff );
	CDB[5] = (char)(( address ) & 0xff );
	*/
	CDB[0] = 0x06;
	CDB[1] = 0x02;
	CDB[3] = 0x04;
	CDB[4] = zone;
	CDB[5] = (unsigned char)(( block >> 8 ) & 0xff );
	CDB[6] = (unsigned char)(( block ) & 0xff );

	if (bIs2154CE)
	{
		CDB[7] = (unsigned char)(( page >> 8 ) & 0xff );
		CDB[8] = (unsigned char)(( page ) & 0xff );
	}
	else
	{
		CDB[7] = (unsigned char)(( page ) & 0xff );
		CDB[8] = (unsigned char)(( page >> 8 ) & 0xff );
	}
	CDB[9] = para;

	//return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_OUT, 528, buf, 10);
	if(!bType)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_OUT, 2112, buf, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandel,CDB,16,SCSI_IOCTL_DATA_OUT,2112,buf,10);

}
//---------------------------------------------------------------------------
int CNTCommand::NT_Direct_Write_Page(BYTE bType,HANDLE DeviceHandel,char drive , unsigned char zone ,
									 unsigned int block , unsigned int page,
									 unsigned char para , bool bIs2154CE,
									 unsigned char *buf )
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof( CDB ));
	/*
	Cmd4=FZone, Cmd5=High Byte of FBlock, Cmd6=Low Byte of FBlock
	Cmd8=High Byte of FPage, Cmd7=Low Byte of FPage,
	Cmd9 : bit 7 : ECC enable, bit 6~0 : reserved

	CDB[4] = (char)(( address >> 8 ) & 0xff );
	CDB[5] = (char)(( address ) & 0xff );
	*/
	CDB[0] = 0x06;
	CDB[1] = 0x02;
	CDB[4] = zone;
	CDB[5] = (unsigned char)(( block >> 8 ) & 0xff );
	CDB[6] = (unsigned char)(( block ) & 0xff );

	if (bIs2154CE)
	{
		CDB[7] = (unsigned char)(( page >> 8 ) & 0xff );
		CDB[8] = (unsigned char)(( page ) & 0xff );
	}
	else
	{
		CDB[7] = (unsigned char)(( page ) & 0xff );
		CDB[8] = (unsigned char)(( page >> 8 ) & 0xff );
	}
	CDB[9] = para;

	//return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_OUT, 528, buf, 10);
	if(!bType)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_OUT, 528, buf, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandel,CDB,16,SCSI_IOCTL_DATA_OUT,528,buf,10);

}

int CNTCommand::NT_Direct_Read_Page(BYTE bType,HANDLE DeviceHandel,char drive , unsigned char zone ,
									unsigned int block , unsigned int page,
									unsigned char para , bool bIs2154CE,
									unsigned char *buf ,int length)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof( CDB ));

	CDB[0] = 0x06;
	CDB[1] = 0x03;
	CDB[4] = zone;
	CDB[5] = (unsigned char)(( block >> 8 ) & 0xff );
	CDB[6] = (unsigned char)(( block ) & 0xff );

	if (bIs2154CE)
	{
		CDB[7] = (unsigned char)(( page >> 8 ) & 0xff );
		CDB[8] = (unsigned char)(( page ) & 0xff );
	}
	else
	{
		CDB[7] = (unsigned char)(( page ) & 0xff );
		CDB[8] = (unsigned char)(( page >> 8 ) & 0xff );
	}
	CDB[9] = para;



	if(!bType)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_IN, length, buf, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandel,CDB,16,SCSI_IOCTL_DATA_IN,length,buf,10);
}

//===========================================================================
// Global Function
//===========================================================================
HANDLE CNTCommand::MyOpenVolume(char drive)
{
	HANDLE MyHandle = INVALID_HANDLE_VALUE;
	DWORD  accessMode1, accessMode2, accessMode3, accessMode4;
	DWORD  shareMode = 0;
	char   devicename[8];

	memset( devicename, 0, sizeof(devicename));
	sprintf( devicename , "\\\\.\\%1C:" , drive );
	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;		// default
	accessMode1 = GENERIC_READ | GENERIC_WRITE;			// default
	accessMode2 = accessMode1 | GENERIC_EXECUTE;		// default
	accessMode3 = accessMode2 | GENERIC_ALL;			// default
	accessMode4 = accessMode3 | MAXIMUM_ALLOWED;		// default

	MyHandle = CreateFileA( devicename , 
		accessMode1 , 
		shareMode , 
		NULL ,
		OPEN_EXISTING , 
		FILE_ATTRIBUTE_NORMAL , 
		0 );

	if ( INVALID_HANDLE_VALUE == MyHandle )
	{
		MyHandle = CreateFileA( devicename, accessMode2, shareMode, NULL ,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL );
	}
	else return MyHandle;

	if ( INVALID_HANDLE_VALUE == MyHandle )
	{
		MyHandle = CreateFileA( devicename, accessMode3, shareMode, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	}
	else
		return MyHandle;

	if ( INVALID_HANDLE_VALUE == MyHandle )
	{
		MyHandle = CreateFileA( devicename, accessMode4, shareMode, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	}
	else
		return MyHandle;

	return MyHandle;
}

int CNTCommand::DismountVolume(HANDLE hVolume)
{
	DWORD dwBytesReturned;
	DWORD err = 0;
	int count = 0;

	do
	{
		if (DeviceIoControl(hVolume,
			FSCTL_DISMOUNT_VOLUME,
			NULL, 0,
			NULL, 0,
			&dwBytesReturned,
			NULL))
		{
			Sleep(200);
			return 0;	// OK
		}
		if (count <4) {
			err = GetLastError();
			SetLastError(0);
			count++;
		}
		else {
			break;
		}
	} while (true);

	return err;
}

HANDLE CNTCommand::MyCreateFile(char drive)
{
	DWORD  accessMode1, accessMode2, accessMode3, accessMode4;
	DWORD  shareMode = 0;
	char   devicename[8];
	HANDLE DeviceHandle_p = NULL;
	//	OutputDebugString("MyCreateFile");
	memset( devicename, 0, sizeof(devicename));
	sprintf( devicename , "\\\\.\\%1C:" , drive );

	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;		// default
	accessMode1 = GENERIC_READ | GENERIC_WRITE;			// default
	accessMode2 = accessMode1 | GENERIC_EXECUTE;		// default
	accessMode3 = accessMode2 | GENERIC_ALL;			// default
	accessMode4 = accessMode3 | MAXIMUM_ALLOWED;		// default

	DeviceHandle_p = CreateFileA( devicename , 
		accessMode1 , 
		shareMode , 
		NULL ,
		OPEN_EXISTING , 
		FILE_FLAG_NO_BUFFERING, //FILE_ATTRIBUTE_NORMAL,		// ? why 
		0 );

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
	{
		DeviceHandle_p = CreateFileA( devicename, accessMode2, shareMode, NULL ,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL );
	}
	else return DeviceHandle_p;

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
	{
		DeviceHandle_p = CreateFileA( devicename, accessMode3, shareMode, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	}
	else
		return DeviceHandle_p;

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
	{
		DeviceHandle_p = CreateFileA( devicename, accessMode4, shareMode, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	}
	else
		return DeviceHandle_p;
	return NULL;
}

int CNTCommand::MyCloseHandle(HANDLE *devHandle)
{
	int iRes = 0;
	if(*devHandle != INVALID_HANDLE_VALUE)
		iRes = CloseHandle(*devHandle);
	//*devHandle = NULL;					// <S>
	*devHandle = INVALID_HANDLE_VALUE;
	return iRes;
}

int CNTCommand::SCSIPtdBuilder(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER *sptdwb,
							   unsigned int cdbLength,  unsigned char *CDB,
							   unsigned char direction, unsigned int transferLength,
							   unsigned long timeOut,   unsigned char *buf)
{
	if( sptdwb==NULL )	return 1;
	if( CDB==NULL )		return 1;
	if( 0>cdbLength || 16 < cdbLength)	return 1;

	ZeroMemory(sptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
	sptdwb->sptd.Length				= sizeof(SCSI_PASS_THROUGH);
	sptdwb->sptd.PathId				= 0;
	sptdwb->sptd.TargetId			= 1;
	sptdwb->sptd.Lun				= 0;
	sptdwb->sptd.CdbLength			= cdbLength;	
	sptdwb->sptd.SenseInfoLength	= 24;
	sptdwb->sptd.TimeOutValue		= timeOut;
	sptdwb->sptd.DataIn				= direction;
	sptdwb->sptd.DataTransferLength = transferLength;
	sptdwb->sptd.DataBuffer			= buf;
	sptdwb->sptd.SenseInfoOffset = 
		offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
	memcpy(sptdwb->sptd.Cdb, CDB, cdbLength);

	return 0;
}
int CNTCommand::NT_U3_WriteInfoBlock(BYTE bType,HANDLE DeviceHandle,char targetDisk, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	BOOL status = 0;
	ULONG length = 0, returned = 0;
	unsigned char CDB[12];
	//SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	//HANDLE DeviceHandle_p=NULL; 

	if( buf==NULL ) return 1;

	//DeviceHandle_p = MyCreateFile(targetDisk);
	//	if (DeviceHandle_p == INVALID_HANDLE_VALUE)
	//		return 2;

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x06;
	CDB[2] = 2;		//1;

	//iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 10);

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 10);

	Sleep(10);	
	return iResult;
}

int CNTCommand::NTU3_Domain_Set_Flags(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, WORD wProperty)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	unsigned char bBuf[512];				
	DWORD dwByte = 3;					// 3Bytes, bMdIndex:bType:wProperty

	memset( bCBD, 0, sizeof(bCBD));
	memset( bBuf, 0, sizeof(bBuf));

	bBuf[0] = bMdIndex;
	bBuf[1] = (BYTE)wProperty;		
	bBuf[2] = (BYTE)(wProperty>>8);	

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3CFG__Domain_Set_Flags_Op;			// 0x26 (0x0026 )
	bCBD[2] = (BYTE)(U3CFG__Domain_Set_Flags_Op>>8);	// 0x00;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	//	iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	if( iResult==0 )	
	{	
	} 	
	return iResult;
}


int CNTCommand::NTU3_Domain_Change_Type(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE bType, WORD wProperty)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	unsigned char bBuf[512];				
	DWORD dwByte = 4;					// 4Bytes, bMdIndex:bType:wProperty

	memset( bCBD, 0, sizeof(bCBD));
	memset( bBuf, 0, sizeof(bBuf));

	bBuf[0] = bMdIndex;
	bBuf[1] = bType;
	bBuf[2] = (BYTE)wProperty;		
	bBuf[3] = (BYTE)(wProperty>>8);	

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3CFG__Domain_Change_Type_Op;		// 0x25 (0x0025 )
	bCBD[2] = (BYTE)(U3CFG__Domain_Change_Type_Op>>8);	// 0x00;

	//iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	if( iResult==0 )	
	{	
	} 	
	return iResult;
}
// 0x80: Set Current Cipher Suite
/************************************************************************/
/* 	
const byte CMD83[12] = { 0xFF, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const byte CMD84[12] = { 0xFF, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const byte CMD85[12] = { 0xFF, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const byte CMD86[12] = { 0xFF, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

const byte CMD87[12] = { 0xFF, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const byte CMD88[12] = { 0xFF, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const byte CMD89[12] = { 0xFF, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// test secure session : Hidden_Page
const byte CMD8B[12] = { 0xFF, 0x8b, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const byte CMD8A[12] = { 0xFF, 0x8a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
*/
/************************************************************************/
int	CNTCommand::NTU3SS__Set_Current_Cipher_Suite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	nt_log("%s",__FUNCTION__);
	BYTE CMD80[12] = { 0xFF, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];	
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD80, 12);
	int direc  = 2,iResult=0;		// 0:IN, 1:OUT, 2:NONE	
	DWORD m_Size = 0;
	//	int iResult = NT_CustomCmd(bDrv, cmd, 12 ,SCSI_IOCTL_DATA_OUT m_Size, NULL, 10);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, m_Size, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, m_Size, NULL, 10);

	return iResult;
}
// 0x81: get device public key 
// set : bDevM, bDevE
int	CNTCommand::NTU3SS__RSA_Get_Device_Public_Key(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf)
{	
	nt_log("%s",__FUNCTION__);
	BYTE CMD81[12] = { 0xFF, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];	
	memset( buf, 0 , sizeof(buf));
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD81, 12);
	int direc  = 1,iResult=0;		// 0:IN, 1:OUT, 2:NONE	
	DWORD m_Size = 128;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_IN, m_Size, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_IN, m_Size, buf, 10);
	return iResult;
}


int	CNTCommand::NTU3SS__RSA_Set_Host_Public_Key(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf
												,unsigned char *bHostM,unsigned char *bHostE)
{
	// 0x82 : Set Host Public Key
	nt_log("%s",__FUNCTION__);
	BYTE CMD82[12] = { 0xFF, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];
	memset( buf, 0 , sizeof(buf));
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD82, 12);
	//direc  = 0;		// 0:OUT, 1:IN, 2:NONE
	//m_Size = 128;

	memcpy( buf, bHostM, 64);
	memcpy( buf+64, bHostE, 64);
	int iResult = 0;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_OUT, 128, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_OUT, 128, buf, 10);

	return 0;
}

int	CNTCommand::NTU3SS__RSA_Set_Host_Random_Challenge(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk
													  ,unsigned char *buf,unsigned char *bHostR)
{
	nt_log("%s",__FUNCTION__);
	// 0x83 : Set Host Random Challenge
	BYTE CMD83[12] = { 0xFF, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];
	memset( buf, 0 , sizeof(buf));
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD83, 12);
	//	direc  = 0;		// 0:OUT, 1:IN, 2:NONE
	int iResult = 0;
	memcpy( buf, bHostR, 64);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_OUT, 64, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_OUT, 64, buf, 10);

	return iResult;
}


int	CNTCommand::NTU3SS__RSA_Verify_Device_Public_Key(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk
													 ,unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	// 0x84: Rsa verify device public key 
	// In : 64Byte, Host Ramdom endcrypted with device's private key
	BYTE CMD84[12] = { 0xFF, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];
	memset( buf, 0 , sizeof(buf));
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD84, 12);
	int iResult = 0;
	//direc  = 1;		// 0:OUT, 1:IN, 2:NONE
	//m_Size = 64;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);

	//if ( NT_CustomCmd(bDrv, cmd, 12, direc, m_Size, buf, iTOut)!=0 )
	return iResult;
}

int	CNTCommand::NTU3SS__RSA_Get_Device_Random_Challenge(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk
														,unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	// 0x85 : get device random challenge
	BYTE CMD85[12] = { 0xFF, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];
	memset( buf, 0 , sizeof(buf));
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD85, 12);
	int iResult=0;
	//direc  = 1;		// 0:OUT, 1:IN, 2:NONE
	//m_Size = 64;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);

	return iResult;
}

int	CNTCommand::NTU3SS__RSA_Verify_Host_Public_Key(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk
												   ,unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	BYTE CMD86[12] = { 0xFF, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD86, 12);
	int iResult = 0;
	//	direc  = 0;		// 0:OUT, 1:IN, 2:NONE
	//	m_Size = 64;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_OUT, 64, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_OUT, 64, buf, 10);

	//if ( NT_CustomCmd(bDrv, cmd, 12, direc, m_Size, buf, iTOut)!=0 )
	//	return 0x86;

	return 0;
}

int	CNTCommand::NTU3SS__RSA_Set_Host_Secret_Number(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk
												   ,unsigned char *buf,unsigned char *bHostS)
{
	nt_log("%s",__FUNCTION__);
	BYTE CMD87[12] = { 0xFF, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];
	memset( buf, 0 , sizeof(buf));
	memcpy( buf, bHostS, 64);
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD87, 12);
	int iResult = 0;
	//direc  = 0;		// 0:OUT, 1:IN, 2:NONE
	//m_Size = 64;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_OUT, 64, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_OUT, 64, buf, 10);

	//	if ( NT_CustomCmd(bDrv, cmd, 12, direc, m_Size, buf, iTOut)!=0 )
	//		return 0x87;
	return iResult;
}

int	CNTCommand::NTU3SS__RSA_Get_Device_Secret_Number(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	// 0x88 :Get Device secret number
	// In : encrypt with host's RSA Public Key
	BYTE CMD88[12] = { 0xFF, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];

	memset( buf, 0 , sizeof(buf));
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD88, 12);
	//direc  = 1;		// 0:OUT, 1:IN, 2:NONE
	//m_Size = 64;	
	int iResult;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);

	//if ( NT_CustomCmd(bDrv, cmd, 12, direc, m_Size, buf, iTOut)!=0 )
	//	return 0x88;
	return iResult;
}

int	CNTCommand::NTU3SS__Start_Secure_Access(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf,BYTE bDomainIndex,BYTE bSecureType)
{
	nt_log("%s",__FUNCTION__);
	// 0x8B:
	//	const byte CMD8B[12] = { 0xFF, 0x8b, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	//	const byte CMD8A[12] = { 0xFF, 0x8a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE CMD8B[12] = { 0xFF, 0x8b, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD8B, 12);
	//direc  = 2;		// 0:OUT, 1:IN, 2:NONE
	//m_Size = 0;
	cmd[3] = bDomainIndex;
	cmd[4] = bSecureType;
	int iResult;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, buf, 10);

	//	if ( NT_CustomCmd(bDrv, cmd, 12, direc, m_Size, buf, iTOut)!=0 )
	//		return 0x8B;
	return iResult;
}

int	CNTCommand::NTU3SS__Get_Secure_Session_Type(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf,BYTE bDomainIndex,BYTE bSecureType)
{
	nt_log("%s",__FUNCTION__);
	// check secure session type
	BYTE CMD8A[12] = { 0xFF, 0x8a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char cmd[12];
	memset( cmd, 0, sizeof(cmd));
	memcpy( cmd, CMD8A, 12);
	//direc  = 1;		// 0:OUT, 1:IN, 2:NONE
	//m_Size = 8;

	cmd[3] = bDomainIndex;
	cmd[4] = bSecureType;
	memset( buf, 0, sizeof(buf));
	int iResult = 0;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cmd, 12 ,SCSI_IOCTL_DATA_IN, 8, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cmd, 12 ,SCSI_IOCTL_DATA_IN, 8, buf, 10);

	//	if ( NT_CustomCmd(bDrv, cmd, 12, direc, m_Size, buf, iTOut)!=0 )
	//		return 0x8A;
	return iResult;
}

int	CNTCommand::NTU__Read_Protected_Page(BYTE bType,HANDLE DeviceHandle,char drive, WORD wPageIndex, BYTE *bPW, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 512;					// 512Byte: Tag(4):data(508)
	BYTE  bPWLen = 7;
	int i;

	if(bBuf==NULL) return 1;
	if(wBufLen<dwByte) return 1;		
	if( bPW==NULL ) return 1;

	//bPW[6] = 0;		// only for buffalo

	memset( bCBD, 0, sizeof(bCBD));
	memset( bBuf, 0, wBufLen);

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3HA__Read_Protected_Page_Op;		// 0x63  (0x0063 )
	bCBD[2] = (BYTE)(U3HA__Read_Protected_Page_Op>>8);	// 0x00;

	bCBD[3] = (BYTE)(wPageIndex);
	bCBD[4] = (BYTE)(wPageIndex>>8);

	for(i=0; i<bPWLen; i++)
		bCBD[5+i] = bPW[i];

	//iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	if(bType ==0)
		iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);

	return iResult;
}


//===========================================================================
// Hidden Area Read/write
int	CNTCommand::NTU__Write_Protected_Page(BYTE bType,HANDLE DeviceHandle,char drive, WORD wPageIndex, BYTE *bPW, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 512;					// 512Byte: Tag(4):data(508)
	BYTE  bPWLen = 7;
	int i;

	if(bBuf==NULL) return 1;
	if(wBufLen<dwByte) return 1;	
	if( bPW==NULL ) return 1;

	//bPW[6] = 0;		// only for buffalo

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3HA__Write_Protected_Page_Op;		// 0x62  (0x0062 )
	bCBD[2] = (BYTE)(U3HA__Write_Protected_Page_Op>>8);	// 0x00;

	bCBD[3] = (BYTE)(wPageIndex);
	bCBD[4] = (BYTE)(wPageIndex>>8);

	for(i=0; i<bPWLen; i++)
		bCBD[5+i] = bPW[i];

	//iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	if(bType ==0)
		iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	return iResult;
}
int	CNTCommand::NTU__Set_Read_Password(BYTE bType,HANDLE DeviceHandle,char drive, WORD wPageIndex, BYTE *bPW)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 0;					// 0Byte: 
	BYTE  bPWLen = 7;
	int i;

	if( bPW==NULL )		return 1;

	//bPW[6] = 0;		// only for buffalo

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3HA__Set_Read_Password_Op;			// 0x65  (0x0065)
	bCBD[2] = (BYTE)(U3HA__Set_Read_Password_Op>>8);	// 0x00;

	bCBD[3] = (BYTE)(wPageIndex);
	bCBD[4] = (BYTE)(wPageIndex>>8);

	for(i=0; i<bPWLen; i++)
		bCBD[5+i] = bPW[i];

	//iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, dwByte, NULL, 10);
	if(bType ==0)
		iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, dwByte, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, dwByte, NULL, 10);

	return iResult;
}


int	CNTCommand::NTU__Set_Write_Password(BYTE bType,HANDLE DeviceHandle,char drive, WORD wPageIndex, BYTE *bPW)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 0;					// 0Byte: 
	BYTE  bPWLen = 7;
	int i;

	if( bPW==NULL ) return 1;
	memset( bCBD, 0, sizeof(bCBD));

	//bPW[6] = 0;		// only for buffalo

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3HA__Set_Write_Password_Op;		// 0x64  (0x0064 )
	bCBD[2] = (BYTE)(U3HA__Set_Write_Password_Op>>8);	// 0x00;

	bCBD[3] = (BYTE)(wPageIndex);
	bCBD[4] = (BYTE)(wPageIndex>>8);

	for(i=0; i<bPWLen; i++)
		bCBD[5+i] = bPW[i];

	//iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, dwByte, NULL, 10);
	if(bType ==0)
		iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, dwByte, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, dwByte, NULL, 10);

	return iResult;
}
//---------------------------------------------------------------------------
/***
FlashType : 00 滅
01 閃爍
02 恆亮
***/
int CNTCommand::NT_StartLED_All(BYTE bType,HANDLE DeviceHandle,  BYTE bDrvNum ,BYTE FlashType)
{
	BOOL status = 0;
	ULONG length = 0, returned = 0;
	unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p=NULL; 

	if( bType==0)
		DeviceHandle_p = MyCreateFile(bDrvNum);
	else if( bType==1)
		DeviceHandle_p = DeviceHandle;
	//	else return 1;

	DWORD err=GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE)
		return err;

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x0D;
	CDB[2] = FlashType;
	CDB[3] = 0x00;
	CDB[8] = 'L'; //4c
	CDB[9] = 'E'; //45
	CDB[10] = 'D';//44

	SCSIPtdBuilder(&sptdwb, 12, CDB, SCSI_IOCTL_DATA_IN, 0, 1, NULL);
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);
	if(status==0)
		err=GetLastError();
	//MyCloseHandle(&DeviceHandle_p);

	if( bType==0)
		MyCloseHandle(&DeviceHandle_p);
	//else if( bType==1)
	//	DeviceHandle_p = DeviceHandle;
	//memcpy(senseBuf , sptdwb.ucSenseBuf , 18 );
	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return err;
}

//---------------------------------------------------------------------------
/*
#define	U3PRA__Get_Area_Parameters_Op				0xA0	// U3PRA__Get_Area_Parameters_OpCode
This command gets the configuration and current states of the Private Area

FAIL: 1. The domain is not a Removable Disk

ex: Flash Map like follow
Hidden area		x
Removable Disk	bMdIndex = 0
CD				bMdIndex = 1
*/
int CNTCommand::NTU3_Get_Area_Parameters(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, TPrivateArea *tPA)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	unsigned char bBuf[512];				
	DWORD dwByte = 32;			// 16Bytes, LunSize:PrivateSize:InPrivate:current NOA

	if( tPA==NULL ) return 1;
	if( sizeof(TPrivateArea)!=dwByte) return 1;	// ?
	memset( tPA, 0, sizeof(TPrivateArea));

	memset( bCBD, 0, sizeof(bCBD));
	memset( bBuf, 0, sizeof(bBuf));

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3PRA__Get_Area_Parameters_Op;			// 0xA0 (0x00A0 )
	bCBD[2] = (BYTE)(U3PRA__Get_Area_Parameters_Op>>8);		// 0x00;

	bCBD[3] = bMdIndex;

	//iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);


	if( iResult==0 )	
	{			
		memcpy( tPA, bBuf, dwByte);
	} 	
	return iResult;
}



//---------------------------------------------------------------------------
/*
//#define	U3PRA__Configure_Private_Area_Op			0xA2	// U3PRA__Configure_Private_Area_OpCode
This command configures the Private Area

FAIL: 1. The domain is not a Removable Disk
2. The domain is Write-Protected

note : If the Private Area is already configured with password and this command is used,
the Private Area will be erased.

ex: Flash Map like follow
Hidden area		x
Removable Disk	bMdIndex = 0
CD				bMdIndex = 1
*/
int CNTCommand::NTU3_U3PRA__Configure_Private_Area(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	DWORD dwByte = 20;					// 20Bytes, PrivateAreaSec(4):Password(16) 

	if(bBuf==NULL) return 1;
	if(wBufLen<dwByte) return 1;

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = 0xA2;	//(BYTE)U3PRA__Configure_Private_Area_Op;			// 0xA2 (0x00A2 ) 
	bCBD[2] = 0x00;	//(BYTE)(U3PRA__Configure_Private_Area_Op>>8);		// 0x00;

	bCBD[3] = bMdIndex;
	bCBD[6] = 'A';	//0x41  for random private area AES key request	// add mode
	bCBD[7] = 0xA2; 										// add mode

	//iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	return iResult;
}

int CNTCommand::NTU3_U3PRA__Configure_Private_Area(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen, BYTE bMode)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	DWORD dwByte = 20;					// 20Bytes, PrivateAreaSec(4):Password(16) 

	if(bBuf==NULL) return 1;
	if(wBufLen<dwByte) return 1;

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = 0xA2;	//(BYTE)U3PRA__Configure_Private_Area_Op;			// 0xA2 (0x00A2 ) 
	bCBD[2] = 0x00;	//(BYTE)(U3PRA__Configure_Private_Area_Op>>8);		// 0x00;

	bCBD[3] = bMdIndex;

	if (bMode == 28 || bMode == 31 || bMode == 33)
		bCBD[6] = 0;	//	because of SCALABLE HIDDEN, do not require random private area AES key
	else
		bCBD[6] = 'A';	//0x41  for random private area AES key request	// add mode

	bCBD[7] = 0xA2; 										// add mode

	//iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	return iResult;
}

int CNTCommand::NTU3_U3PRA__Configure_2nd_Password(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	

	if(bBuf==NULL) return 1;

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = 0xA8;	//(BYTE)U3PRA__Configure_Private_Area_Op;			// 0xA2 (0x00A2 ) 
	bCBD[2] = 0x00;	//(BYTE)(U3PRA__Configure_Private_Area_Op>>8);		// 0x00;

	bCBD[3] = bMdIndex;
	bCBD[4] = 0;

	//iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, wBufLen, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, wBufLen, bBuf, 10);
	return iResult;
}

//---------------------------------------------------------------------------    
/*
#define	U3AUX__Ready_Not_Ready_Pulse_Op				0x100 	// U3AUX__Ready_Not_Ready_Pulse_OpCode
This command instructs the U3 device to set a ready-not-ready pulse
When U3 device gets this command, it enters Not-Ready state

FAIL: 

???
ex: Flash Map like follow
Hidden area		x
Removable Disk	bMdIndex = 0
CD				bMdIndex = 1
*/
int CNTCommand::NTU3_Ready_Not_Ready_Pulse(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bLun)	//BYTE bMdIndex)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	//unsigned char bBuf[512];
	DWORD dwByte = 0;					// 0Bytes

	memset( bCBD, 0, sizeof(bCBD));
	//memset( bBuf, 0, sizeof(bBuf));

	bCBD[0] = 0xFF;	
	bCBD[1] = (BYTE)U3AUX__Ready_Not_Ready_Pulse_Op;			// 0x01  (0x100 )
	bCBD[2] = (BYTE)(U3AUX__Ready_Not_Ready_Pulse_Op>>8);		// 0x00;

	bCBD[3] = bLun;	//bMdIndex;

	//	iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, dwByte, NULL/*bBuf*/, 10);
	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, dwByte, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, dwByte, NULL, 10);

	if( iResult==0 )	
	{	
	} 	
	return iResult;
}	

//---------------------------------------------------------------------------
/*
#define	U3PRA__Open_Op								0xA4	// U3PRA__Open_OpCode
This command instructs the device to open a Private Area

FAIL: 

note : To inform the file system about the media change event 
one can call AUX_Ready-Not -Ready_Pluse		  

ex: Flash Map like follow
Hidden area		x
Removable Disk	bMdIndex = 0
CD				bMdIndex = 1
*/
int CNTCommand::NTU3_Open(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen, BYTE bMode)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	DWORD dwByte = 16;					// 16Bytes, password

	if( bBuf==NULL ) return 1;
	if( wBufLen<dwByte ) return 1;

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;	
	bCBD[1] = (BYTE)U3PRA__Open_Op;			// 0xA4  (0x00A4 )
	bCBD[2] = (BYTE)(U3PRA__Open_Op>>8);	// 0x00;

	bCBD[3] = bMdIndex;
	bCBD[4] = bMode;

	//	iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	if( iResult==0 )	
	{	
	} 	
	return iResult;
}

void CNTCommand::SetTestPagePercentage(int percentage)
{
	_doFrontPagePercentage = percentage;
}


int CNTCommand::GetTestPageCount(BYTE pageType)
{
	int testPageCount = -1;
	if (0 ==_doFrontPagePercentage )
		return testPageCount;

	if (_D1==pageType || _D2==pageType)
	{
		testPageCount = (_fh->PagePerBlock*_doFrontPagePercentage)/100;
	} else if (_D3==pageType) {
		int wordline = 3;
		if (8 == _fh->Cell ) {
			//TLC
			wordline = 3;
		} else if (16 == _fh->Cell) {
			//QLC
			wordline = 4;
		}
		testPageCount = (_fh->PagePerBlock*wordline*_doFrontPagePercentage)/100;
		testPageCount = (testPageCount/wordline)*wordline;
	}

	return testPageCount;
}

int CNTCommand::NTU3_Close(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	DWORD dwByte = 0;
	BYTE  bBuf[512] = {0};

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;	
	bCBD[1] = (BYTE)U3PRA__Close_Op;			// 0xA4  (0x00A4 )
	bCBD[2] = (BYTE)(U3PRA__Close_Op>>8);	// 0x00;

	bCBD[3] = bMdIndex;

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	if( iResult==0 )	
	{	
	} 	
	return iResult;
}	

int	CNTCommand::NTU3__Write_Page(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, WORD wPageIndex, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	DWORD dwByte = 512;					// 512Byte: Tag(4):data(508)

	if(bBuf==NULL) return 1;
	if(wBufLen<dwByte) return 1;		

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3HA__Write_Page_Op;		// 0x60  (0x0060 )
	bCBD[2] = (BYTE)(U3HA__Write_Page_Op>>8);	// 0x00;

	bCBD[3] = (BYTE)(wPageIndex);
	bCBD[4] = (BYTE)(wPageIndex>>8);

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	return iResult;
}

int	CNTCommand::NTU3__Scalable_Write_Page(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk, int wPageIndex, WORD wCount, BYTE *bBuf, DWORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	DWORD dwByte = 65536;					// 512Byte: Tag(4):data(508)

	if (bBuf == NULL) return 1;
	if (wBufLen == 0 || wBufLen > 65536) return 1;		

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0x6;
	bCBD[1] = 0xB;
	bCBD[2] = (char)((wPageIndex >> 24 ) & 0xff);
	bCBD[3] = (char)((wPageIndex >> 16 ) & 0xff);
	bCBD[4] = (char)((wPageIndex >> 8 ) & 0xff);
	bCBD[5] = (char)((wPageIndex ) & 0xff);
	bCBD[7] = (char)((wCount >> 8) & 0xff);
	bCBD[8] = (char)(wCount & 0xff);
	bCBD[9] = 'V';
	bCBD[10] ='W';   

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, wBufLen, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, wBufLen, bBuf, 10);

	return iResult;
}

//---------------------------------------------------------------------------
int	CNTCommand::NTU3_Get_Device_Configuration(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE *Dev)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	unsigned char bBuf[512];	// need size: 1+8*NumOfDomain
	DWORD dwByte = 512;

	if( Dev==NULL ) return 1;
	//	memset( Dev, 0, sizeof(TDeviceConfigure));

	memset( bCBD, 0, sizeof(bCBD));
	memset( bBuf, 0, sizeof(bBuf));

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3CFG__Get_Device_Configuration_Op;		// 0x21  (0x0021 )
	bCBD[2] = (BYTE)(U3CFG__Get_Device_Configuration_Op>>8);	// 0x00;

	bCBD[3] = (BYTE)(dwByte);
	bCBD[4] = (BYTE)(dwByte>>8);
	bCBD[5] = (BYTE)(dwByte>>16);
	bCBD[6] = (BYTE)(dwByte>>24);

	//iResult = NT_CustomCmd_Phy(bPhyNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);


	if( iResult==0 )	
	{	
		memcpy( Dev, bBuf, 512);
	} 	
	return iResult;
}


//---------------------------------------------------------------------------
// #define U3SD__Query_Service_OP	0x00	// U3SD__Query_Service_CMD_OpCode
/*
// OP =0x00 U3SD__Query_Service_OP	
// Service Types
#define U3SD_TYPE_ROOT	0x03	// Service discovery - Root of entire hierarchy 
#define U3SD_TYPE_CFG	0x04	// Configuration/partitioning
#define U3SD_TYPE_CD	0x06	// CD ROM
#define U3SD_TYPE_HA	0x08	// Hidden Area
#define U3SD_TYPE_SS	0x0A	// Secure Session
#define U3SD_TYPE_PRA	0x0C	// Private Area
#define U3SD_TYPE_TH	0x0E	// Trusted Host
#define U3SD_TYPE_DA	0x10	// Device Authentication
#define U3SD_TYPE_CE	0x12	// /0x13	Crypto Engine
#define U3SD_TYPE_AUX	0x14	// Maintenance
*/
// FF 00 00 03 00 77 00 00 00 01 00 00 00 00 00 00
// Flags : 0x02, Fixed, Get the branch root descriptor and all sub-descriptor
// dwBufLen : 1-65536(64K)
int	CNTCommand::NTU3_Query_Service(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, WORD wServiceType, BYTE *bBuf, DWORD dwBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	BYTE bFlags = 0x02;		// 0x00, 0x01, 0x02 : Get the branch root descriptor and all sub-descriptor

	if( bBuf==NULL ) return 1;
	if( (dwBufLen==0)||(dwBufLen>65536) ) return 1;	// 1-65536(64K)
	memset( bBuf, 0, dwBufLen );
	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3SD__Query_Service_OP;			// 0x00  (0x0000 )
	bCBD[2] = (BYTE)(U3SD__Query_Service_OP>>8);	// 0x00;

	bCBD[3] = (BYTE)(wServiceType&0xFF);
	bCBD[4] = (BYTE)(wServiceType>>8);

	bCBD[5] = (BYTE)(dwBufLen);
	bCBD[6] = (BYTE)(dwBufLen>>8);
	bCBD[7] = (BYTE)(dwBufLen>>16);
	bCBD[8] = (BYTE)(dwBufLen>>24);

	bCBD[9] = bFlags; 
	bCBD[10] = 0xA2; 
	//iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwBufLen, bBuf, 10);
	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwBufLen, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwBufLen, bBuf, 10);

	return iResult;
}	


BOOL CNTCommand::NT_Burner_ScanBAD(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk,BYTE EXPBA_CE,BYTE CE,BYTE Zone,BYTE Block_H,BYTE Block_L, ULONG dwBufLen,BYTE *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult;
	unsigned char bCBD[12] = {0};	
	memset( bCBD, 0, sizeof(bCBD));	
	bCBD[0] = 0x06;
	bCBD[1] = 0x08;			// 0x00  (0x0000 )
	bCBD[2] = EXPBA_CE;
	bCBD[3] = CE;
	bCBD[4] = Zone;
	bCBD[5] = Block_H;
	bCBD[6] = Block_L;
	bCBD[10] = 1;


	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwBufLen, bBuf, 3000);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwBufLen, bBuf, 3000);

	return iResult;

}
int CNTCommand::NT_Get_RDYTime(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrv, unsigned char *bBuf,BYTE bMode)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	DWORD dwByte = 512;				

	if( bBuf==NULL ) return 1;

	memset( bBuf, 0, sizeof(dwByte));
	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0x06;
	bCBD[1] = 0x1A;
	bCBD[2] = bMode;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrv, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else return 0xFF;

	return iResult;
}


int CNTCommand::NT_Get_BC(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf,BYTE BCAddr, BYTE CDB5)
{
	int iResult = 0;
	unsigned char bCBD[16] = {0};	
	DWORD dwByte = 528;				

	if( bBuf==NULL ) return 1;

	memset( bBuf, 0, 528);
	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0x06;
	bCBD[1] = 0x05;
	bCBD[2] = 0x52;//'R';
	bCBD[3] = 0x41;//'A';
	bCBD[4] = BCAddr; //0xE0 or 0xE2
	bCBD[5] = CDB5;
	nt_log_cdb(bCBD,"%s",__FUNCTION__);
	_ForceNoSwitch = 1;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else return 0xFF;

	return iResult;
}

int CNTCommand::NT_Get_OTP(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	DWORD dwByte = 528;				

	if( bBuf==NULL ) return 1;

	memset( bBuf, 0, 528);
	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0x06;
	bCBD[1] = 0x05;
	bCBD[2] = 0x52;//'R';
	bCBD[3] = 0x41;//'A';
	bCBD[4] = 0xF4;
	bCBD[5] = 0xC8;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else return 0xFF;

	return iResult;
}

int CNTCommand::NTU3_SetLED(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk, char bMode)
{
	int iResult = 0;
	unsigned char bCBD[12] = {0}; 
	DWORD dwByte = 512;     // 512Byte: Tag(4):data(508)

	BYTE bBuf[512] = {0};

	switch (bMode)
	{
	case 1:
		bBuf[0] = 0x0A;
		bBuf[1] = 0x80;
		break;

	case 2:
		bBuf[0] = 0x00;
		bBuf[1] = 0x00;
		break;
	}

	if(bBuf==NULL) return 1;

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = 0x11;
	bCBD[2] = 0x01;
	bCBD[3] = 0;

	//iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, 512, bBuf, 10);
	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	return iResult;
}

int CNTCommand::NTU3_GetLED(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk, BYTE *bBuf)
{
	int iResult = 0;
	unsigned char bCBD[12] = {0}; 
	DWORD dwByte = 512;     // 512Byte: Tag(4):data(508)

	if(bBuf==NULL) return 1;

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = 0x10;
	bCBD[2] = 0x01;
	bCBD[3] = 0;

	//iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, 512, bBuf, 10);
	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);

	return iResult;
}

int CNTCommand::NTU3_GetHint(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk, WORD wPageIndex, DWORD dwPageTag, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};        
	DWORD dwByte = 512;                                  // 512Byte: Tag(4):data(508)

	if(bBuf==NULL) return 1;
	if(wBufLen<dwByte) return 1;         

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3HA__Read_Page_Op;                  // 0x61  (0x0061 )
	bCBD[2] = (BYTE)(U3HA__Read_Page_Op>>8);  // 0x00;
	bCBD[3] = (BYTE)(wPageIndex);
	bCBD[4] = (BYTE)(wPageIndex>>8);
	bCBD[5] = (BYTE)(dwPageTag);
	bCBD[6] = (BYTE)(dwPageTag>>8);
	bCBD[7] = (BYTE)(dwPageTag>>16);
	bCBD[8] = (BYTE)(dwPageTag>>24);

	//iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, 512, bBuf, 10);
	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 1000);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 1000);

	return iResult;
}

int CNTCommand::NT_EraseSystemUnit30(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	BYTE temp[16] = {0};
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x1E;

	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_IN, 16, temp, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 16, temp, 10);
	Sleep(80);

	return iResult;
}

int CNTCommand::NTU32SS_Command(BYTE bType, HANDLE DeviceHandle_p, BYTE targetDisk, unsigned char directFunc,
								unsigned char *cmd, unsigned char *bBuf, int length)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12] = {0};	
	DWORD dwByte = length;				

	if( bBuf==NULL ) return 1;

	//memset( bBuf, 0, sizeof(dwByte));
	memset( bCBD, 0, sizeof(bCBD));

	memcpy(bCBD, cmd, sizeof(bCBD));

	if( bType==0 )
	{
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 , directFunc, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 , directFunc, dwByte, bBuf, 10);
	} else return 0xFF;

	return iResult;
}

int CNTCommand::TurnFlashPower(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool ON)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x08;
	cdb[3] = 0x00;
	if(ON)
		cdb[3] |= 0x01;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
}


int CNTCommand::FlashReset(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int CE_Sel)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x00;
	cdb[3] = (unsigned char)(CE_Sel); //Bit 0 = CE0, Bit 1= CE1

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
}

int CNTCommand::NT_EMMC_LoadBR(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int datalen ,unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0]   = 0x06;
	cdb[1]   = 0x1f;
	cdb[0xA] = 0xB0;
	cdb[0xB] = 0xC0;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, bBuf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, bBuf, 5);
	return iResult;
}

int CNTCommand::NT_EMMC_LoadFW(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int datalen ,unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0]   = 0x06;
	cdb[1]   = 0x1f;
	cdb[3] = 0xA0;//kim clear info 原先有code原先有code才support for 2312
	cdb[0xA] = 0xB1;
	cdb[0xB] = 0xE0;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, bBuf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, bBuf, 5);
	return iResult;
}

int CNTCommand::NT_EMMC_BuildDefaultSystemArea(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));

	if( buf==NULL ) return 1;

	memset(cdb,0,16);
	cdb[0]   = 0x06;
	cdb[1]   = 0x1f;
	cdb[0xA] = 0xB3;
	//cdb[0xB] = 0xE0;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 30);
	return iResult;
}
int CNTCommand::NTU3_CD_Lock(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	//BYTE targetDisk = 0;
	int iResult = 0;
	unsigned char bCBD[12]; 
	DWORD dwByte = 128;     // 128Bytes, RSA Modulus(64):RSA Public Key(64) 

	//if( DevPath==NULL ) return 1;
	if(bBuf==NULL) return 1;
	if(wBufLen<dwByte) return 1;

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;
	bCBD[1] = (BYTE)U3CD__CD__Lock_Op;   // 0x00 (0x0040 )
	bCBD[2] = (BYTE)(U3CD__CD__Lock_Op>>8);  // 0x40;

	bCBD[3] = bMdIndex;


	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	return iResult;
}

int CNTCommand::NT_EMMC_InitializeMMCHosts(BYTE bType, HANDLE DeviceHandle, char drive)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x1E;
	CDB[2] = 0x06;

	if(bType == 0)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
}

int CNTCommand::NT_EMMC_AdjustCommonFrequency(BYTE bType, HANDLE DeviceHandle, char drive, unsigned char ucChannel, unsigned char ucFrequencySelect, unsigned char ucSDROrDDRModeSelect)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x1E;
	CDB[2] = 0x01;
	CDB[3] = ucChannel;
	CDB[4] = ucFrequencySelect;
	CDB[5] = ucSDROrDDRModeSelect;

	if(bType == 0)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
}

int CNTCommand::NT_EMMC_SwitchDataBusWidth(BYTE bType, HANDLE DeviceHandle, char drive, unsigned char ucChannel, unsigned char ucDataBusWidthSelect)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x1E;
	CDB[2] = 0x02;
	CDB[3] = ucChannel;
	CDB[4] = ucDataBusWidthSelect;

	if(bType == 0)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
}

int CNTCommand::NT_EMMC_OutputClockToeMMC(BYTE bType, HANDLE DeviceHandle, char drive, unsigned char ucChannel, unsigned char ucOutputOrNot)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x1E;
	CDB[2] = 0x08;
	CDB[3] = ucChannel;
	CDB[4] = ucOutputOrNot;

	if(bType == 0)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
}

int CNTCommand::NT_EMMC_SendClocksToeMMC(BYTE bType, HANDLE DeviceHandle, char drive, unsigned char ucChannel, unsigned char ucClockCount)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[16];
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x1E;
	CDB[2] = 0x07;
	CDB[3] = ucChannel;
	CDB[4] = ucClockCount;

	if(bType == 0)
		return NT_CustomCmd(drive, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		return NT_CustomCmd_Phy(DeviceHandle, CDB, 16, SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
}

int CNTCommand::NT_EMMC_GetStatus(BYTE bType, HANDLE DeviceHandle, char drive, BYTE bChannel, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0x1E;
	cdb[2] = 0x09;
	cdb[3] = bChannel;

	if(bType ==0)
		iResult = NT_CustomCmd(drive, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 15);

	return iResult;
}

int CNTCommand::NT_EMMC_StartLED_All(BYTE bType,HANDLE DeviceHandle,  BYTE bDrvNum)
{
	nt_log("%s",__FUNCTION__);
	BOOL status = 0;
	ULONG length = 0, returned = 0;
	unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p=NULL; 

	if( bType==0)
		DeviceHandle_p = MyCreateFile(bDrvNum);
	else if( bType==1)
		DeviceHandle_p = DeviceHandle;
	//	else return 1;

	DWORD err=GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE)
		return err;

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0xFF;
	CDB[1] = 0x00;
	CDB[2] = 0x01;
	CDB[4] = 0xF7;

	SCSIPtdBuilder(&sptdwb, 12, CDB, SCSI_IOCTL_DATA_IN, 0, 1, NULL);
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);
	if(status ==0)
		err=GetLastError();
	//MyCloseHandle(&DeviceHandle_p);

	if( bType==0)
		MyCloseHandle(&DeviceHandle_p);
	//else if( bType==1)
	//	DeviceHandle_p = DeviceHandle;
	//memcpy(senseBuf , sptdwb.ucSenseBuf , 18 );
	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return err;
}

int CNTCommand::NT_CheckVT(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char *buff)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12]; 

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0x06;
	bCBD[1] = 0x87;

	if(bType ==0 )
		iResult = NT_CustomCmd(drive, bCBD, 12 ,SCSI_IOCTL_DATA_IN, 3584, buff, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,bCBD, 12 ,SCSI_IOCTL_DATA_IN, 3584, buff, 10);

	if( iResult!=0 ) 
	{ 
		return iResult;
	}  

	return 0;
}

int CNTCommand::Direct_Read_Status(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[12];
	int iResult = 0;
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xa0;
	CDB[5] = 0x01;

	if(bType ==0)
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);

	return iResult;
}

int  CNTCommand::NT_GetDieInfo(BYTE bType ,HANDLE MyDeviceHandle,char drive, unsigned char *buf) //CheckSpare
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 0x44; //'D'
	CDB[3] = 0x49; //'I'

	if(bType ==0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);

	return iResult;
}

int CNTCommand::NT_WearLeveling_read(BYTE bType ,HANDLE MyDeviceHandle,char drive, int DieNum, unsigned char *buf, int iBufSize)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[12];
	int iResult = 0;
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 'W';//'57'
	CDB[3] = 'E';//'45'
	CDB[4] = DieNum;

	if(bType ==0)
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, iBufSize, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, iBufSize, buf, 10);
	return iResult;

}

int CNTCommand::NT_WearLeveling_write(BYTE bType ,HANDLE MyDeviceHandle,char drive, int DieNum, unsigned char *buf, int iBufSize)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[12];
	int iResult = 0;
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 'W';//'57'
	CDB[3] = 'W';//'45'
	CDB[4] = DieNum;

	if(bType ==0)
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, iBufSize, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_OUT, iBufSize, buf, 10);
	return iResult;

}
int CNTCommand::Get_PowerUpCount(BYTE bType ,HANDLE MyDeviceHandle,char drive, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[12];
	int iResult = 0;
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xA5;

	if(bType ==0)
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 1024, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 1024, buf, 10);
	return iResult;
}

//kim add 2013.8.5 for 2303
int CNTCommand::NT_ErsCntRead(BYTE bType ,HANDLE MyDeviceHandle,char drive, unsigned char *ucBuffer, unsigned short usVirZone, unsigned char ubMode)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;  
	CDB[1] = 0x85;
	if(usVirZone!=0xFFFF)
	{
		CDB[2] = (UCHAR)(usVirZone&0xFF);
		CDB[3] = (UCHAR)(usVirZone>>8);
	}
	else
	{
		CDB[2] = 0xFF;
		CDB[3] = 0xFF;
	}
	CDB[4] = ubMode;	

	if(bType ==0 )
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, ucBuffer, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, ucBuffer, 10);

	if( iResult!=0 ) 
	{ 
		return iResult;
	}

	return 0;
}

int CNTCommand::NT_ErsCntWrite(BYTE bType ,HANDLE MyDeviceHandle,char drive, unsigned char *ucBuffer)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;  
	CDB[1] = 0x8F;

	if(bType ==0 )
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_OUT, 8192*2, ucBuffer, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB, 16 ,SCSI_IOCTL_DATA_OUT, 8192*2, ucBuffer, 10);

	if( iResult!=0 ) 
	{ 
		return iResult;
	}

	return 0;
}

int CNTCommand::NT_Get_Bin(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 64;				

	if( bBuf==NULL ) return 1;

	memset( bBuf, 0, dwByte);	// ? 528
	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0x06;
	bCBD[1] = 0x18;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else return 0xFF;

	return iResult;
	//	return 0;
}

//add by kim for 2312 isp read update coiunt //buf[0-3]
int CNTCommand::NT_ISP_Read_UpdateFWCount(BYTE bType,HANDLE DeviceHandle_p,char drive,unsigned char *buf)
{

	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53JumpISPMode(drive);
	//#endif
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 'U';
	CDB[3] = 'C';


	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 528, buf, 30);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 528, buf, 30);	

	Sleep(80);

	return iResult;
}

//---------------------------------------------------------------------------
// return : 0 = OK
//          others = fail
// note : mode0 buf, 512 Bytes  first 4 bytes for cycle
//        mode1 buf, 65536 Byte Sorting pattern 
int CNTCommand::NT_FWSorting30(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf, BYTE Bmode)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;

	if (Bmode == 0)
	{
		CDB[1] = 0xE0;

		if(bType ==0 )
			iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 512, buf, 10);
		else
			iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB,12,SCSI_IOCTL_DATA_OUT, 512, buf,10);
	}
	else //if (Bmode == 2)
	{
		CDB[1] = 0xE2;

		int iLength = 0;
		if (Bmode == 1)
			iLength = 32768;
		else
			iLength = 65536;			

		if(bType ==0 )
			iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, iLength, buf, 10);
		else
			iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB,12,SCSI_IOCTL_DATA_OUT, iLength, buf,10);
	}

	return iResult;
}

//---------------------------------------------------------------------------
// return : 0 = OK
//          others = fail
// note : buf, 512 Bytes  first byte for check result
int CNTCommand::NT_FWSorting30Result(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xE1;

	if(bType ==0 )
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB,12,SCSI_IOCTL_DATA_IN, 512, buf,10);

	return iResult;
}

int CNTCommand::U32l_SCSI_Write_CD_H(HANDLE hDeviceHandle, long address, int secNo, unsigned char *buf,
									 bool fRead, BYTE bDomainIndex)
{
	nt_log("%s",__FUNCTION__);
	BOOL status = 0;
	ULONG length = 0 ,  returned = 0;
	unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	int count = 0;			

	if( buf==NULL ) return 1;

	DWORD err=0;	//GetLastError();
	if (hDeviceHandle == INVALID_HANDLE_VALUE)
		return 2;	//err;

OnceAgain:

	memset( CDB, 0, sizeof(CDB));
	unsigned int dwDataLen = 2048*secNo;

	if( secNo==0 )  return 0;  // ??

	CDB[0] = 0xF3;
	CDB[2] = 0x42;
	CDB[3] = bDomainIndex;

	CDB[4]  = (unsigned char)((address>>24)&0xff);  // MSB
	CDB[5]  = (unsigned char)((address>>16)&0xff);
	CDB[6]  = (unsigned char)((address>>8)&0xff);
	CDB[7]  = (unsigned char)(address&0xff);

	CDB[8]  = (unsigned char)((secNo>>24)&0xff); // MSB
	CDB[9]  = (unsigned char)((secNo>>16)&0xff);
	CDB[10] = (unsigned char)((secNo>>8)&0xff);
	CDB[11] = (unsigned char)(secNo&0xff);


	SCSIPtdBuilder(&sptdwb, 12, CDB, fRead ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT, dwDataLen, 10, buf);
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(hDeviceHandle, //DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);

	err = GetLastError();

	if (sptdwb.sptd.ScsiStatus && 10 > count)
	{
		//		TRACE("Read/Write ScsiStatus: %d\n", sptdwb.sptd.ScsiStatus);
		count++;
		Sleep(100);
		goto OnceAgain;
	}

	err=GetLastError();

	//MyCloseHandle(&DeviceHandle_p);

	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return err;
}

// cindy add for 2306
int CNTCommand::NT_GetFlashMode(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x77;
	CDB[2] = 0x99;

	if(bType ==0 )
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB,12,SCSI_IOCTL_DATA_IN, 512, buf,10);

	return iResult;
}

int CNTCommand::NT_FWAuthentication(BYTE bType,HANDLE DeviceHandle_p,char drive,DWORD ICType,BOOL BIsInit)
{
	unsigned char buf[512];
	DWORD dwK1 = 0;
	DWORD dwK2 = 0;
	int i,j;

	memset(buf, 0, sizeof(buf));

	if (NT_GetFWAuthentication(bType, DeviceHandle_p, drive, buf))
		return 1;

	if (ICType == ICTYPE_DEF_PS2303)
	{
		i = j = 1;

		do
		{
			dwK1 += (DWORD)buf[i] + (DWORD)buf[j];//1,1,2,3,5,8,13,21...(取前兩碼值相加)
			i += j;
			j += i;
		}while(j<512);
		dwK1 ^= 0x3C3C;

		i = j = 2;
		do
		{
			dwK2 += (DWORD)buf[i] + (DWORD)buf[j];//1,1,2,3,5,8,13,21...(取前兩碼值相加)
			i += j;
			j += i;
		}while(j<512);

		dwK2 ^= 0xC3C3;

		srand(static_cast<unsigned int>(time(NULL)));
		memset(buf, 0, sizeof(buf));

		for (int i =0; i < 512; i++)
			buf[i] = rand();

		buf[0x58] = (unsigned char)(dwK1 & 0xff);
		buf[0x94] = (unsigned char)(dwK2 & 0xff);
		buf[0x111] = (unsigned char)((dwK1 >> 8) & 0xff);
		buf[0x194] = (unsigned char)((dwK2 >> 8) & 0xff);
	}
	else if (ICType == ICTYPE_DEF_PS2307 || ICType == ICTYPE_DEF_PS2309 || ICType == ICTYPE_DEF_PS2311)
	{
		i = j = 3;

		do
		{
			dwK1 += (DWORD)buf[i] + (DWORD)buf[j];//1,1,2,3,5,8,13,21...(取前兩碼值相加)
			i += j;
			j += i;
		}while(j<512);

		//if (BIsInit)		// anton 2017.02.21 close 0xABAB
		dwK1 ^= 0x9696;
		//else
		//	dwK1 ^= 0xABAB;

		i = j = 1;
		do
		{
			dwK2 += (DWORD)buf[i] + (DWORD)buf[j];//1,1,2,3,5,8,13,21...(取前兩碼值相加)
			i += j;
			j += i;
		}while(j<512);

		//if (BIsInit)
		dwK2 ^= 0x6969;
		//else
		//	dwK2 ^= 0xBABA;

		srand(static_cast<unsigned int>(time(NULL)));
		//srand(time(NULL));
		memset(buf, 0, sizeof(buf));

		for (int i =0; i < 512; i++)
			buf[i] = rand();

		//if (BIsInit)
		//{
		buf[0x57] = (unsigned char)(dwK1 & 0xff);
		buf[0x93] = (unsigned char)(dwK2 & 0xff);
		buf[0x110] = (unsigned char)((dwK1 >> 8) & 0xff);
		buf[0x193] = (unsigned char)((dwK2 >> 8) & 0xff);
		//}
		//else
		//{
		//	buf[0x41] = (unsigned char)(dwK1 & 0xff);
		//	buf[0x47] = (unsigned char)(dwK2 & 0xff);
		//	buf[0x186] = (unsigned char)((dwK1 >> 8) & 0xff);
		//	buf[0x1ba] = (unsigned char)((dwK2 >> 8) & 0xff);
		//}
	}
	else if (ICType == ICTYPE_DEF_PS2308 )
	{
		i = j = 9;

		do
		{
			dwK1 += (DWORD)buf[i] + (DWORD)buf[j];//1,1,2,3,5,8,13,21...(取前兩碼值相加)
			i += j;
			j += i;
		}while(j<512);
		dwK1 ^= 0x3C3C;

		i = j = 6;
		do
		{
			dwK2 += (DWORD)buf[i] + (DWORD)buf[j];//1,1,2,3,5,8,13,21...(取前兩碼值相加)
			i += j;
			j += i;
		}while(j<512);

		dwK2 ^= 0xC3C3;

		srand(static_cast<unsigned int>(time(NULL)));
		//srand(time(NULL));
		memset(buf, 0, sizeof(buf));

		for (int i =0; i < 512; i++)
			buf[i] = rand();

		buf[0x57] = (unsigned char)(dwK1 & 0xff);
		buf[0xa4] = (unsigned char)(dwK2 & 0xff);
		buf[0x101] = (unsigned char)((dwK1 >> 8) & 0xff);
		buf[0x1B4] = (unsigned char)((dwK2 >> 8) & 0xff);
	}
	if (NT_SetFWAuthentication(bType, DeviceHandle_p, drive, buf))
		return 1;

	return 0;
}
int CNTCommand::NT_GetFWAuthentication(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	DWORD dwByte = 512;

	if(bBuf == NULL ) return 1;

	memset(bBuf, 0, 512);	// ? 528
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x64;
	CDB[2] = 0x01;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else return 0xFF;
	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);

	return iResult;	
}
int CNTCommand::NT_SetFWAuthentication(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	DWORD dwByte = 512;

	if(bBuf == NULL ) return 1;

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x65;
	CDB[2] = 0x01;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	} else return 0xFF;
	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);

	return iResult;	
}
int CNTCommand::NT_ReviseFWVersion(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	DWORD dwByte = 512;

	if(bBuf == NULL ) return 1;

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x0E;
	CDB[1] = 0x00;
	CDB[2] = 0x03;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	} else return 0xFF;
	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);

	return iResult;	
}
int CNTCommand::NT_GetFlashUniqueID(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk , unsigned char *id, BYTE Chanelcount, BYTE CECount, BYTE DieCount)
{
	nt_log("%s",__FUNCTION__);
	BOOL status = 0;
	ULONG length = 0 ,  returned = 0;
	unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p=NULL; 
	int count = 0;			

	if( id==NULL ) return 1;
	if(bType == 0)
		DeviceHandle_p = MyCreateFile(targetDisk);
	if(bType == 1)
		DeviceHandle_p = MyDeviceHandle;

	DWORD err=GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE)
		return err;

OnceAgain:	
	memset( id, 0, 512);
	memset( CDB , 0 , sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x56;
	CDB[4] = 0x5A;
	CDB[5] = Chanelcount;
	CDB[6] = CECount;
	CDB[7] = DieCount;

	SCSIPtdBuilder(&sptdwb, 12, CDB, SCSI_IOCTL_DATA_IN, 512, 10, id);	
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);

	err = GetLastError();

	if (sptdwb.sptd.ScsiStatus && 5 > count)
	{
		count++;
		Sleep(100);
		goto OnceAgain;
	}

	err=GetLastError();
	if(bType == 0)
		MyCloseHandle(&DeviceHandle_p);

	if (status)
	{
		return -sptdwb.sptd.ScsiStatus;
	} else
		return err;
}

int CNTCommand::NT_Flushcommand(BYTE bType,HANDLE DeviceHandle,  BYTE bDrvNum )
{
	nt_log("%s",__FUNCTION__);
	BOOL status = 0;
	ULONG length = 0, returned = 0;
	unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p=NULL; 

	if( bType==0)
		DeviceHandle_p = MyCreateFile(bDrvNum);
	else if( bType==1)
		DeviceHandle_p = DeviceHandle;
	//	else return 1;

	DWORD err=GetLastError();
	if (DeviceHandle_p == INVALID_HANDLE_VALUE)
		return err;

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x40;


	SCSIPtdBuilder(&sptdwb, 12, CDB, SCSI_IOCTL_DATA_IN, 0,100 , NULL);
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,

		length,
		&returned,
		FALSE);
	err=GetLastError();
	//MyCloseHandle(&DeviceHandle_p);

	if( bType==0)
		MyCloseHandle(&DeviceHandle_p);
	//else if( bType==1)
	//	DeviceHandle_p = DeviceHandle;
	//memcpy(senseBuf , sptdwb.ucSenseBuf , 18 );
	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return err;
}
int CNTCommand::NT_CheckIBStatus(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	//unsigned char bBuf[528];
	unsigned char CDB[12];
	DWORD dwByte = 8;

	CDB[0] = 0x06;
	CDB[1] = 0x1B;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 8, bBuf, 60);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, 8, bBuf, 60);
	} else return 0xFF;
	return iResult;	
}
//cindy add for long scan tool
int CNTCommand::NT_GetEfuseData(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	//	unsigned char bBuf[528];
	unsigned char CDB[12];
	DWORD dwByte = 12;

	if(bBuf == NULL ) return 1;

	memset(bBuf, 0, 528);	
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 0x52;
	CDB[3] = 0x41;
	CDB[4] = 0xFA;
	CDB[5] = 0x00;

	_ForceNoSwitch=1;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, bBuf, 60);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, bBuf, 60);
	} else return 0xFF;
	return iResult;	
}
int CNTCommand::NT_GetNowEfuseData(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf1)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bBuf[528];
	unsigned char CDB[12];
	DWORD dwByte = 12;

	if(bBuf == NULL ) return 1;

	memset(bBuf, 0, 528);	
	memset(bBuf1, 0, 528);	// ? 528
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 0x52;
	CDB[3] = 0x41;
	CDB[4] = 0xFA;
	CDB[5] = 0x00;


	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, bBuf, 60);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, bBuf, 60);
	} else return 0xFF;

	bBuf1[0]=((bBuf[0x80])&0xFF);
	bBuf1[1]=((((bBuf[0x82])<<1)|((bBuf[0x81])&0x01))&0xFF);
	bBuf1[2]=((((bBuf[0x85])<<7)|((bBuf[0x84])<<2)|(((bBuf[0x83])&0x01)<<1)|((bBuf[0x82])>>7))&0xFF);
	bBuf1[3]=((((bBuf[0x86])<<7)|((bBuf[0x85])>>1))&0xFF);
	bBuf1[4]=((((bBuf[0x87])<<7)|((bBuf[0x86])>>1))&0xFF);
	bBuf1[5]=((((bBuf[0x89])<<7)|(((bBuf[0x88])&0x3F)<<1)|(((bBuf[0x87])&0x02)>>1))&0xFF);
	bBuf1[6]=((((bBuf[0x8A])<<7)|((bBuf[0x89])>>1))&0xFF);
	bBuf1[7]=((((bBuf[0x8B])<<7)|((bBuf[0x8A])>>1))&0xFF);

	return iResult;	
}
int CNTCommand::NT_GetCodeEfuseData(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	int iResult = 0;
	unsigned char CDB[12];
	DWORD dwByte = 0;

	if(bBuf == NULL ) return 1;

	memset(bBuf, 0, 528);	// ? 528
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xce;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, bBuf, 10);
	} else return 0xFF;

	return iResult;	
}
int CNTCommand::NT_GetEfuseData70(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	int iResult = 0;
	//	unsigned char bBuf[528];
	unsigned char CDB[12];
	DWORD dwByte = 12;

	if(bBuf == NULL ) return 1;

	memset(bBuf, 0, 528);	
	memset(CDB, 0, sizeof(CDB));
	nt_log("%s",__FUNCTION__);

	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 0x52;
	CDB[3] = 0x41;
	CDB[4] = 0xF1;
	CDB[5] = 0x40;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, bBuf, 60);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, 528, bBuf, 60);
	} else return 0xFF;
	return iResult;	
}
int CNTCommand::NT_GetIBTable(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	DWORD dwByte = 0;

	if(bBuf == NULL ) return 1;

	memset(bBuf, 0, 1088);	// ? 528
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x0f;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 16384, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, 16384, bBuf, 10);
	} else return 0xFF;

	return iResult;	
}

//cindy add for 2270 set ctrl driving
int CNTCommand::NT_GetVoltage(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	DWORD dwByte = 0;

	if(bBuf == NULL ) return 1;

	memset(bBuf, 0, 1088);	// ? 528
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x15;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 1088, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, 1088, bBuf, 10);
	} else return 0xFF;

	return iResult;	
}
int CNTCommand::NT_DetectVoltage(BYTE bType, HANDLE DeviceHandle_p, char drive,BYTE voltage,BYTE type,BYTE burnerFlow,BYTE  bInterleave)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	DWORD dwByte = 0;

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x0E;
	CDB[2] = voltage;
	CDB[3] = 0x80 | type;//0x80 legacy,0x81 toggle,0x82 toggle2.0
	CDB[4] = burnerFlow;
	CDB[5] = bInterleave;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 10);	
	return iResult;	
}
int CNTCommand::NT_SetMediaChanged( BYTE bType ,HANDLE DeviceHandle,char drive)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	memset( CDB, 0, sizeof(CDB));
	CDB[0] = 0x6;
	CDB[1] = 0x30;

	if(bType==0)
		iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_IN,0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy( DeviceHandle, CDB, 12, SCSI_IOCTL_DATA_IN, 0, NULL, 10);

	return iResult;	
}

//---------------------------------------------------------------------------
/*
#define	U3PRA__Open_Op								0xA4	// U3PRA__Open_OpCode
This command instructs the device to open a Private Area

FAIL: 

note : To inform the file system about the media change event 
one can call AUX_Ready-Not -Ready_Pluse		  

ex: Flash Map like follow
Hidden area		x
Removable Disk	bMdIndex = 0
CD				bMdIndex = 1
*/
int CNTCommand::NTU3_Open(BYTE bType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMdIndex, BYTE *bBuf, WORD wBufLen)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char bCBD[12];	
	DWORD dwByte = 16;					// 16Bytes, password

	if( bBuf==NULL ) return 1;
	if( wBufLen<dwByte ) return 1;

	memset( bCBD, 0, sizeof(bCBD));

	bCBD[0] = 0xFF;	
	bCBD[1] = (BYTE)U3PRA__Open_Op;			// 0xA4  (0x00A4 )
	bCBD[2] = (BYTE)(U3PRA__Open_Op>>8);	// 0x00;

	bCBD[3] = bMdIndex;

	//	iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	if(bType ==0)
		iResult = NT_CustomCmd(targetDisk, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);

	if( iResult==0 )	
	{	
	} 	
	return iResult;
}

int CNTCommand::NT_ScanBadBlocks30(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf, int length)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )	return 1;

	memset(buf, 0, length);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x94;
	CDB[2] = 0x00;	//later bad Blocks.
	if(bType ==0 )
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, length, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle, CDB, 12, SCSI_IOCTL_DATA_IN, length, buf, 10);

	return iResult;
}

int CNTCommand::NT_ScanFlashID30(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk, unsigned char *buf, int length)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if( buf==NULL )	return 1;

	memset(buf, 0, length);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x8B;
	CDB[2] = 0x5A;	//later bad Blocks.
	if(bType ==0 )
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, length, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle, CDB, 12, SCSI_IOCTL_DATA_IN, length, buf, 10);

	return iResult;
}

int CNTCommand::NT_Erase_Syetem_Code30(BYTE bType,HANDLE DeviceHandle_p,char drive)
{
	nt_log("%s",__FUNCTION__);
	unsigned char CDB[12];
	unsigned char buf[8];
	int iResult;
	memset(buf,0,8);
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0]  = 0x06;
	CDB[1]  = 0x1E;

	if(!bType)
		iResult =  NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 0, NULL, 100);
	else
		iResult =  NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, 0, NULL, 100);

	return iResult;
}

int CNTCommand::NTU3_CD_Lock(BYTE bType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCDKeyIndex, int datalen, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	if(buf == NULL) return 1;
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0xFF;
	CDB[1] = 0x40;
	CDB[2] = 0x00;
	CDB[3] = bCDKeyIndex;


	if(bType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_OUT, datalen, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_OUT, datalen, buf, 10);
	Sleep(80);

	return iResult;
}

int	CNTCommand::NT_Toggle2Legacy(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	//if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0xf6;
	CDB[2] = 0x45;

	//	int iResult = NT_CustomCmd(bDrv, cmd, 12 ,SCSI_IOCTL_DATA_OUT m_Size, NULL, 10);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);

	return iResult;
}

int CNTCommand::NT_BurnerState_GetIB(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf)
{
	nt_log("%s",__FUNCTION__);
	//#if (Release_Factory == _M53_)
	//if(gTC.sLoadBurner != "1")
	//	return NT_M53ISPSwitchMode(drive,mode);
	//#endif
	int iResult = 0;
	unsigned char CDB[16];

	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0x19;

	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, bBuf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, bBuf, 30);
	Sleep(80);

	return iResult;
}

int CNTCommand::NT_Rom_rewrite_flash_ID(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char CE
										, unsigned char original_dvice_code, unsigned char new_dvice_code)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12] ={0};
	CDB[0]= 0x6;
	CDB[1]= 0xe;
	CDB[2]= 0x91;
	CDB[3]= CE;
	CDB[4]= 0x50;
	CDB[5]= 0x68;
	CDB[6]= 0x49;

	//CDB[7]= 2;

	//CDB[8]= 0;
	//CDB[9]= 0;


	CDB[7]= 0;
	CDB[8]= original_dvice_code;
	CDB[9]= new_dvice_code;

	if(bType ==0 )
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 500);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 500);
	return iResult;
}

int CNTCommand::NT_Get_flash_ID_by_CE_Channel_Die(BYTE bType ,HANDLE MyDeviceHandle,char drive,unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	//. Get flash ID by CE,Channel,Die.
	int iResult = 0;
	unsigned char CDB[12] = {0};
	CDB[0]= 0x6;
	CDB[1]= 0xe;
	CDB[2]= 0x90;

	if(bType ==0 )
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, 4096, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(MyDeviceHandle,CDB, 12 ,SCSI_IOCTL_DATA_IN, 4096, buf, 10);
	return iResult;
}

int CNTCommand::NT_ISPGetStatus(BYTE bType,HANDLE DeviceHandle,char drive, BYTE *bBuf, WORD wBufSize)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];
	int iLen = 64;

	if( bBuf==NULL )	return 1;
	if( wBufSize<iLen ) return 1;

	memset( bBuf, 0, wBufSize );
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xB0;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, iLen, bBuf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, iLen, bBuf, 5);

	return iResult;
}

int CNTCommand::NAND_READ_ID_TEST(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,UCHAR* r_buf , ULONG bufsize)
{
	nt_log("%s",__FUNCTION__);
	unsigned char cdb[12];
	memset(cdb , 0 , 12);
	int iResult = 0;
	cdb[0] = 0x06;
	cdb[1] = 0x0E;
	cdb[2] = 0x90;
	cdb[3] = 0x00;
	cdb[4] = 0x00;
	cdb[5] = 0x00;
	cdb[6] = 0x00;
	cdb[7] = 0x00;
	cdb[8] = 0x00;
	cdb[9] = 0x00;
	cdb[10]= 0x00;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, cdb, 12 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, cdb, 12 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 10);
	} 

	return iResult;
}

int CNTCommand::NAND_SLC_Write(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,UCHAR* w_buf , ULONG bufsize , UINT FBlock, unsigned char WL)
{
	nt_log("%s",__FUNCTION__);
	unsigned char secNo = (unsigned char)(bufsize / 512);
	unsigned char cdb[12];
	memset(cdb , 0 , 12);
	int iResult= 0;
	cdb[0] = 0x06;
	cdb[1] = 0x0E;
	cdb[2] = 0x24;
	cdb[3] = 0x00;
	cdb[4] = 0x00;
	cdb[5] = FBlock>>8;
	cdb[6] = FBlock;
	cdb[7] = 0;
	cdb[8] = 0;
	cdb[9] = secNo;
	cdb[10] = 0x00;
	cdb[11] = WL; //<---bufsize 取代 patten , 都是jack的錯

	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, cdb, 12 ,SCSI_IOCTL_DATA_OUT, bufsize, w_buf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, cdb, 12 ,SCSI_IOCTL_DATA_OUT, bufsize, w_buf, 10);
	} 

	return iResult;
}

int CNTCommand::NAND_MLC_TEST1_Write(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, UCHAR* w_buf, BYTE bCE , ULONG bufsize , UINT FBlock)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char secNo = (unsigned char)((bufsize / 512) & 0xff);
	unsigned char cdb[12];
	memset(cdb , 0 , 12);

	cdb[0] = 0x06;
	cdb[1] = 0x0E;
	cdb[2] = 0x21;
	cdb[3] = bCE;
	cdb[4] = 0x00;
	cdb[5] = FBlock>>8;
	cdb[6] = FBlock;
	cdb[7] = 0x00;
	cdb[8] = 0x00;
	cdb[9] = secNo;
	cdb[10]= 0x00;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, cdb, 12 ,SCSI_IOCTL_DATA_OUT, bufsize, w_buf, 100);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, cdb, 12 ,SCSI_IOCTL_DATA_OUT, bufsize, w_buf, 100);
	} 

	return iResult;
}

int CNTCommand::NAND_BadCol_TEST(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,UCHAR* r_buf , UCHAR PLAN)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[12];
	memset(cdb , 0 , 12);

	cdb[0] = 0x06;
	cdb[1] = 0x0E;
	cdb[2] = 0x30;
	cdb[3] = 0x00;
	cdb[4] = 0x00;
	cdb[5] = 0x00;
	cdb[6] = PLAN;
	cdb[7] = 0x00;
	cdb[8] = 0x00;
	cdb[9] = 0x00;
	cdb[10]= 0x00;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, cdb, 12 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, cdb, 12 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 10);
	} 

	return iResult;
}

int CNTCommand::NAND_WP_TEST_START(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum)
{
	nt_log("%s",__FUNCTION__);
	unsigned char cdb[12];
	memset(cdb , 0 , 12);
	int iResult = 0;
	cdb[0] = 0x06;
	cdb[1] = 0x0E;
	cdb[2] = 0x00;
	cdb[3] = 0x00;
	cdb[4] = 0x00;
	cdb[5] = 0x00;
	cdb[6] = 0x00;
	cdb[7] = 0x00;
	cdb[8] = 0x00;
	cdb[9] = 0x00;
	cdb[10]= 0x00;
	if(bType ==0)
		iResult = NT_CustomCmd(bDrvNum, cdb, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, cdb, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);

	return iResult;

}

int CNTCommand::NAND_WP_TEST(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,UCHAR* r_buf , UCHAR WP_Enable)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[12];
	memset(cdb , 0 , 12);

	cdb[0] = 0x06;
	cdb[1] = 0x0E;
	cdb[2] = 0x0D;
	cdb[3] = 0x00;
	cdb[4] = 0x00;
	cdb[5] = 0x00;
	cdb[6] = 0x00;
	cdb[7] = 0x00;
	cdb[8] = 0x00;
	cdb[9] = WP_Enable;
	cdb[10]= 0x00;

	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, cdb, 12 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, cdb, 12 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 10);
	} 

	return iResult;
}

int CNTCommand::NAND_WP_TEST_END(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[12];
	memset(cdb , 0 , 12);
	cdb[0] = 0x06;
	cdb[1] = 0x0E;
	cdb[2] = 0x0F;
	cdb[3] = 0x00;
	cdb[4] = 0x00;
	cdb[5] = 0x00;
	cdb[6] = 0x00;
	cdb[7] = 0x00;
	cdb[8] = 0x00;
	cdb[9] = 0x00;
	cdb[10]= 0x00;
	if(bType ==0)
		iResult = NT_CustomCmd(bDrvNum, cdb, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, cdb, 12 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 10);
	return iResult;
}

int CNTCommand::PreWriteInfoBlock(BYTE bType,HANDLE DeviceHandle_p,char drive)
{
	nt_log("%s",__FUNCTION__);
	unsigned char buf[512];
	DWORD dwSumEven = 0;
	DWORD dwSumOdd = 0;

	memset(buf, 0, sizeof(buf));

	if (NT_GetWriteInfoBlockRand(bType, DeviceHandle_p, drive, buf))
		return 1;

	for (int i = 0; i < 512; i += 2)
	{
		dwSumEven += buf[i];
		dwSumOdd += buf[i + 1];
	}

	dwSumEven ^= 0x5A5A;
	dwSumOdd ^= 0xA5A5;

	srand(static_cast<unsigned int>(time(NULL)));
	//srand(time(NULL));
	memset(buf, 0, sizeof(buf));

	for (int i =0; i < 512; i++)
		buf[i] = rand();

	buf[33] = (unsigned char)(dwSumEven & 0xff);
	buf[301] = (unsigned char)((dwSumEven >> 8) &0xff);
	buf[39] = (unsigned char)(dwSumOdd & 0xff);
	buf[199] = (unsigned char)((dwSumOdd >> 8) & 0xff);

	if (NT_SetWriteInfoBlockRand(bType, DeviceHandle_p, drive, buf))
		return 1;

	return 0;
}

// YMTC DBS workaround
int CNTCommand::ST_YMTC_Workaround(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x0c;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	return iResult;
}

int CNTCommand::NT_GetWriteInfoBlockRand(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	DWORD dwByte = 512;

	if(bBuf == NULL ) return 1;

	memset(bBuf, 0, 512);	// ? 528
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x64;


	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_IN, dwByte, bBuf, 10);
	} else return 0xFF;
	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);

	return iResult;	
}

int CNTCommand::NT_SetWriteInfoBlockRand(BYTE bType, HANDLE DeviceHandle_p, char drive, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	DWORD dwByte = 512;

	if(bBuf == NULL ) return 1;

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x65;


	if( bType==0 )
	{
		iResult = NT_CustomCmd(drive, CDB, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 12 ,SCSI_IOCTL_DATA_OUT, dwByte, bBuf, 10);
	} else return 0xFF;
	//iResult = NT_CustomCmd( drive, CDB, 12, SCSI_IOCTL_DATA_OUT, 512, buf, 10);

	return iResult;	
}

int CNTCommand::NT_ScanWindow(BYTE bType,HANDLE DeviceHandel,unsigned char bDrv, BYTE Channle, BYTE CE, unsigned short Length, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if((bBuf == NULL) || (Length != 1040)) return 1;

	memset(bBuf, 0, Length);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xCE;
	CDB[4] = Channle;
	CDB[5] = CE;

	if(!bType)
		iResult =  NT_CustomCmd(bDrv, CDB, 12 ,SCSI_IOCTL_DATA_IN, Length, bBuf, 60);
	else
		iResult =  NT_CustomCmd_Phy(DeviceHandel, CDB, 12 ,SCSI_IOCTL_DATA_IN, Length, bBuf, 60);

	return iResult;
}

int CNTCommand::NT_GetFlashExtendID_1040(BYTE bType,HANDLE DeviceHandel,unsigned char bDrv, unsigned short Length, unsigned char *bBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	if((bBuf == NULL) || (Length != 1040)) return 1;

	memset(bBuf, 0, Length);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x56;

	if(!bType)
		iResult =  NT_CustomCmd(bDrv, CDB, 12 ,SCSI_IOCTL_DATA_IN, Length, bBuf, 60);
	else
		iResult =  NT_CustomCmd_Phy(DeviceHandel, CDB, 12 ,SCSI_IOCTL_DATA_IN, Length, bBuf, 60);

	return iResult;
}

int CNTCommand::NT_GetIBBuf(BYTE bType, HANDLE DeviceHandel, unsigned char bDrv, unsigned char *bBuf)
{
	int iResult = 0;
	unsigned char CDB[12];

	if(bBuf == NULL) return 1;
	nt_log("%s",__FUNCTION__);

	memset(bBuf, 0, 8192);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x1C;

	if(!bType)
		iResult =  NT_CustomCmd(bDrv, CDB, 12 ,SCSI_IOCTL_DATA_IN, 8192, bBuf, 60);
	else
		iResult =  NT_CustomCmd_Phy(DeviceHandel, CDB, 12 ,SCSI_IOCTL_DATA_IN, 8192, bBuf, 60);

	return iResult;
}

int CNTCommand::ST_TurnFlashPower(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool ON)
{
	nt_log("%s",__FUNCTION__);
	unsigned char cdb[16];
	//	memset(CDB , 0 , 16);
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x08;
	cdb[3] = 0x00;
	if(ON)
		cdb[3] |= 0x01;

	int iResult = 0;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 12 ,SCSI_IOCTL_DATA_IN, 0, NULL, 64);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 12 ,SCSI_IOCTL_DATA_IN, 0, NULL, 64);
	return iResult;
}

int CNTCommand::ST_GetB85T_OTP_Table(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet,BYTE bSetOnfi,BYTE *r_buf )
{

	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xbf;
	cdb[3] = bGetSet;
	cdb[4] = bSetOnfi;
	if(bGetSet !=0 )
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN,2048, r_buf, 20);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN,2048, r_buf, 20);
	}
	else
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT,2048, r_buf, 20);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT,2048, r_buf, 20);
	}
	return iResult;



}


int CNTCommand::ST_GetSetRetryTable(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet,BYTE bSetOnfi,BYTE *r_buf, int length,BYTE ce,BYTE bRetryCount, BYTE OTPAddress)
{
	int sqlRetryRead = GetPrivateProfileInt("Sorting","SQLRetryRead",0, _ini_file_name);
	if(m_isSQL)
	{
		return ST_GetSetRetryTable_SQL( bGetSet, bSetOnfi, r_buf, length, GetCeMapping(ce), bRetryCount, OTPAddress);
	}

	if(sqlRetryRead) {
		ST_GetSetRetryTable_SQL( bGetSet, bSetOnfi, r_buf, length, GetCeMapping(ce), bRetryCount, OTPAddress);
	}

	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xbf;
	cdb[3] = bGetSet;
	cdb[4] = bSetOnfi;
	cdb[5] = bRetryCount;
	cdb[6] = GetCeMapping(ce);
	cdb[7] = OTPAddress; // 0: old OTP address, 1: new OTP address
	nt_log_cdb(cdb,"%s",__FUNCTION__);

	for(int i=0;i<2;i++)
	{
		if(bGetSet !=0 )
		{
			if(obType ==0)
				iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN,length, r_buf, 20);
			else
				iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN,length, r_buf, 20);
		}
		else
		{
#if _DEBUG
			assert(r_buf[0]<150);
#endif
			if(obType ==0)
				iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT,length, r_buf, 20);
			else
				iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT,length, r_buf, 20);
		}
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(3000);
	}


	return iResult;


}

int CNTCommand::ST_DirectRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool DelayRead, int DReadCnt, bool SkipECC, bool Inv, bool Ecc, bool Plane, bool Cache, bool Cont, unsigned int CE_Sel,unsigned int block,unsigned int page,unsigned int sector, unsigned char ECCBit,unsigned char *r_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(r_buf , 0 , (sector*512));
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x05;
	cdb[3] = 0x20;

	if(SkipECC)
		cdb[3] |= 0x40;
	/*	if(Inv)
	cdb[3] |= 0x20;
	else
	cdb[3] |= 0x10;
	*/
	if(!Cont)
		cdb[3] |= 0x08;
	if(Ecc)
		cdb[3] |= 0x01;
	if(Plane)
		cdb[3] |= 0x02;
	if(Cache)
		cdb[3] |= 0x04;

	cdb[4] = (unsigned char)(CE_Sel);
	cdb[5] = (unsigned char)(block >> 8);
	cdb[6] = (unsigned char)(block & 0xff);
	cdb[7] = (unsigned char)(page >> 8);
	cdb[8] = (unsigned char)(page & 0xff);
	cdb[9] = (unsigned char)(sector);
	cdb[14] = (unsigned char)(ECCBit);


	if(DelayRead)
	{
		cdb[10]=0x01;
		cdb[11]=(unsigned char) DReadCnt;
	}

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, (sector*512), r_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, (sector*512), r_buf, 5);
	return iResult;
}	

int CNTCommand::ST_DirectWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool Plane,bool CreateTable,bool LastPage,bool FirstPage,bool MarkBad,unsigned int CE_Sel,unsigned int block,unsigned int page,unsigned int sector,unsigned char *w_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];
	memset(CDB , 0 , 16);
	int BufferSize;
	BufferSize = (sector*512);

	memset(CDB , 0 , 16);
	CDB[0] = 0x06;
	CDB[1] = 0xf6;
	CDB[2] = 0x04;
	CDB[3] = 0x00;
	if(MarkBad)
		CDB[3] |= 0x01;
	if(Plane)
		CDB[3] |= 0x02;
	if(CreateTable)
		CDB[3] |= 0x20;
	if(LastPage)
		CDB[3] |= 0x40;
	if(FirstPage)
		CDB[3] |= 0x80;

	CDB[4] = (unsigned char)(CE_Sel);
	CDB[5] = (unsigned char)(block >> 8);
	CDB[6] = (unsigned char)(block);
	CDB[7] = (unsigned char)(page >> 8);
	CDB[8] = (unsigned char)(page);
	CDB[9] = (unsigned char)(sector);

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, BufferSize, w_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_OUT, BufferSize, w_buf, 5);

	return iResult;

}

int CNTCommand::ST_FlashGetID(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,bool Real_Id,unsigned char *r_buf)
{
	nt_log("%s",__FUNCTION__);

	if(bSDCARD==1)
		return SD_TesterGetFlashID (obType, DeviceHandle, targetDisk,r_buf);

	int iResult = 0;
	unsigned char CDB[16];

	memset(r_buf , 0 , 512);
	memset(CDB , 0 , 16);
	CDB[0] = 0x06;
	CDB[1] = 0xf6;
	CDB[2] = 0x01;
	CDB[3] = 0x00;    //Bit0 : 1 Read Real ID
	if(Real_Id)       //     : 0 Read ID by F/W Structure
		CDB[3] = 0x01;
	//	hDeviceHandle = INVALID_HANDLE_VALUE;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 5);

	return iResult;
}
//20190909 : Not Used, Comment
//int CNTCommand::ST_GetInformation(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned int CE, unsigned int Die,unsigned char *r_buf)
//{
//	nt_log("%s",__FUNCTION__);
//	unsigned char cdb[16];
//	memset(r_buf , 0 , 512);
//	memset(cdb , 0 , 16);
//	int iResult = 0;
//	cdb[0] = 0x06;
//	cdb[1] = 0xf6;
//	cdb[2] = 0x16;
//	cdb[3] = (unsigned char)(CE);
//	cdb[4] = (unsigned char)(Die);
//
//	if(obType ==0)
//		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 5);
//	else
//		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 5);
//
//	return iResult;
//}
int CNTCommand::ST_AssignFTAforD1Read(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE FTA0,BYTE FTA1,BYTE FTA2,BYTE FTA3,BYTE FTA4 )
{
	nt_log("%s",__FUNCTION__);
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	int iResult = 0;
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x16;
	cdb[3] = FTA0;	//LSB
	cdb[4] = FTA1;
	cdb[5] = FTA2;
	cdb[6] = FTA3;
	cdb[7] = FTA4;
	
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	return iResult;
}
int CNTCommand::ST_GetInfoBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int type,BYTE blk_addr1,BYTE blk_addr2, unsigned char *r_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(r_buf , 0 , 8);
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x17;
	cdb[3] = 0;
	cdb[4] = 0;
	//	cdb[5] = 0x00;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 5);

	return iResult;

}


int CNTCommand::ST_2A_GetBadBlockInfo(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int Sector,unsigned char *r_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x2A;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, (Sector*512), r_buf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, (Sector*512), r_buf, 30);
	return iResult;
}



int CNTCommand::ST_ReliabilityTestingDirectWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bCE,WORD wBLOCK,int Sector ,unsigned char *w_buf)
{

	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	int len = Sector << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xEE;

	cdb[4] = bCE;
	cdb[5] = wBLOCK >> 8;
	cdb[6] = wBLOCK & 0xFF;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, w_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, w_buf, 5);
	return iResult;
}

int CNTCommand::ST_GetCodeVersionDate(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char* buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x00;
	_realCommand = 1;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 32, buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 32, buf, 5);
	return iResult;


}

int CNTCommand::ST_FlashReset(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int CE_Sel,BYTE TestBusy)
{

	if(!_doFlashReset) return 0;
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x09;
	cdb[3] = TestBusy;
	cdb[4] = (unsigned char)(CE_Sel); //Bit 0 = CE0, Bit 1= CE1
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;


}

int CNTCommand::ST_MarkBadBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bCE,WORD wBLOCK)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];

	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x07;
	cdb[3] = (unsigned char)(bCE);
	cdb[4] = (unsigned char)(wBLOCK >> 8);
	cdb[5] = (unsigned char)(wBLOCK);

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
}

int CNTCommand::ST_ReliabilityTestingDirectRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bCE,WORD wBLOCK)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];

	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xEF;

	cdb[4] = bCE;
	cdb[5] = wBLOCK >> 8;
	cdb[6] = wBLOCK & 0xFF;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
}

int CNTCommand::ST_DirectErase(BYTE bType,HANDLE DeviceHandle,char drive, unsigned int CE_Sel,unsigned int block)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x03;
	cdb[3] = (unsigned char)(CE_Sel);
	cdb[4] = (unsigned char)(block >> 8);
	cdb[5] = (unsigned char)(block);
	if(bType == 0)
		iResult = NT_CustomCmd(drive, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
}

int CNTCommand::ST_GetBootcodeVersion(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE *w_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x03;

	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 32, w_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 32, w_buf, 5);
	return iResult;
}

int CNTCommand::ST_SetClock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE param3,BYTE param4,BYTE param7,unsigned int FlashReadDiv,unsigned int FlashWriteDiv,bool TurnOffEDO)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);


	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x0a;
	cdb[3] = param3; //8 0
	cdb[4] = param4; //4
	cdb[5] = (unsigned char)(FlashReadDiv); //3
	cdb[6] = (unsigned char)(FlashWriteDiv);//4
	cdb[7] = param7;

	if(TurnOffEDO)	
		cdb[8] = 0x01;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
}

int CNTCommand::ST_SetSram(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE address,BYTE Bit,BYTE Open)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x0b;
	cdb[3] = address;
	cdb[4] = Bit;
	cdb[5] = Open;	// Open=1: SRAM[address] |= (1<<Bit)
					// Open=0: SRAM[address] &= (1<<Bit)

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
}

int CNTCommand::ST_2PlaneCopyBackWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int TestBlock,unsigned int SourceBlock)//,int len,BYTE*w_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x62;
	cdb[3] = 0x00;
	cdb[4] = (unsigned char)(SourceBlock >> 8);
	cdb[5] = (unsigned char)(SourceBlock);
	cdb[6] = (unsigned char)(TestBlock >> 8);
	cdb[7] = (unsigned char)(TestBlock);

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);

	return iResult;
}

int CNTCommand::ST_WriteOneBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned int TestBlock,int len,BYTE*w_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x60;

	cdb[5] = (unsigned char)(TestBlock >> 8);
	cdb[6] = (unsigned char)(TestBlock);


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, w_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, w_buf, 5);
	return iResult;
}



//read one block
int CNTCommand::ST_ReadOneBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ECC_Bit
								,unsigned int TestBlock,BYTE bPlaneMode,bool bCopyBack)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	if(bPlaneMode == 0x02)
		cdb[2] = 0x63;
	else
		cdb[2] = 0x61;
	if(bCopyBack)
		cdb[3] |= 0x40;
	cdb[6] = ECC_Bit;
	cdb[8] = (unsigned char)(TestBlock >> 8);
	cdb[9] = (unsigned char)(TestBlock);

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
}

//Clock 請設定:  6  f6  a  8  4  3  4  3
//
//Write one block : (2 plane cache write)
//
//6  f6  60  x x  block_h  block_L     8192 byte
//
//Read one block :
//6  f6 61 0 x  x  ecc_bit x block_h block_L
//
//其它command 都不變


int CNTCommand::ST_EraseAll(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char PBADie, unsigned char AP_Code, unsigned int StartBlock, unsigned int EndBlock)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x0E;
	cdb[3] = (unsigned char)(AP_Code);
	cdb[4] = (unsigned char)(PBADie);
	cdb[8] = (unsigned char)(StartBlock >> 8);
	cdb[9] = (unsigned char)(StartBlock);
	cdb[10] = (unsigned char)(EndBlock >> 8);
	cdb[11] = (unsigned char)(EndBlock);

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
}

int CNTCommand::ST_PollStatus(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf,bool bReturnAverage)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x55;
	if(bReturnAverage){
		cdb[7] |= 0x01;
	}
	for(int i=0;i<2;i++)
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 32, r_buf, 30);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 32, r_buf, 30);

		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);
	}
	return iResult;
}

int CNTCommand::ST_2311TranSRam(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf,int startAdd,int len,int _write)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x57;
	cdb[3] = (startAdd >> 8) & 0xff;
	cdb[4] = startAdd & 0xff;
	cdb[5] = (len >> 8) & 0xff;
	cdb[6] = len & 0xff;
	cdb[7] = _write;
	if(_write){
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, r_buf, 30);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, r_buf, 30);
	}
	else{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, r_buf, 30);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, r_buf, 30);
	}
	return iResult;
}

int CNTCommand::ST_TranSRamThroughFlash(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf,int startAdd,int block,int page,int len,int _write)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x57;
	cdb[3] = (startAdd >> 8) & 0xff;
	cdb[4] = startAdd & 0xff;
	cdb[5] = (len >> 8) & 0xff;
	cdb[6] = len & 0xff;
	cdb[7] = _write;
	cdb[8] = (block >> 8) & 0xff;
	cdb[9] = block & 0xff;
	cdb[10] = (page >> 8) & 0xff;
	cdb[11] = page & 0xff;
	if(_write){
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, r_buf, 30);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, r_buf, 30);
	}
	else{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, r_buf, 30);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, r_buf, 30);
	}
	return iResult;
}

int CNTCommand::ST_PollData(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb , 0 , 16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x56;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 1024, r_buf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 1024, r_buf, 30);
	return iResult;
}



/*******************************************************
ED3 -> for D1 page write set conversion  -- new
********************************************************/
int CNTCommand::ST_DirectPageReadByConv(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
										,WORD wBLOCK,WORD wStartPage,WORD bPageCount,BYTE bECCBit,BYTE Retry_Read,BYTE bOffset,int datalen ,BYTE BlockORPageSeed,BYTE Seed,unsigned char *w_buf)
{
	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xFF;
	cdb[3] = ReadMode;
	cdb[4] = bCE;
	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	cdb[9] = Retry_Read&0xff;
	cdb[10] = bPageCount & 0xff; //WL
	cdb[11] = bPageCount >> 8; //WL
	cdb[12] = bECCBit;
	cdb[13] = BlockORPageSeed;  //skip 
	cdb[14] = Seed;
	cdb[15] = bOffset;
	for(int i=0;i<2;i++)
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(3000);
	}

	return iResult;
#else
	memset(w_buf,0x01,datalen);
	return 0;
#endif
}

/*
0xFB & 0xFF	D1_READ_NEW_FORMAT/D1_READ	
CDB[3] bit1: 2p mode 
CDB[3] bit5: cache
CDB[3] bit6: 4p mode
CDB[3] bit7: read完做erase
CDB[4]: 執行CE
CDB[5]: block (low byte)
CDB[6]: block (high byte)
CDB[7]: page (low byte)
CDB[8]: page (high byte)
CDB[9] bit0: retry read
CDB[9] bit4: 不檢查LCA
CDB[9] bit6: 執行A2 (Toshbia 1z MLC使用)
CDB[10]: page count (low byte)
CDB[11]: page count (high byte)
CDB[12]: ECC threshold
CDB[15]: Skip CE number (0-7)
CDB[16]: Skip CE number (8-16)
*/
int CNTCommand::ST_DirectPageRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
								  ,WORD wBLOCK,WORD wStartPage,WORD bPageCount,BYTE bECCBit,BYTE Retry_Read,UINT32 bCeMask,int datalen ,unsigned char *w_buf, bool bNewEccFormat)
{

#if _TEST_MLC_A2_SORTING_
	//沒開 a2 assert
	assert((Retry_Read & 0x40));
#endif
#ifndef __SKIP__TEST__
	int iResult = 0;
	int rtn=0;
	if (IsFakeBlock(wBLOCK))
	{
		// w_buf[8192] = 0xFF; // 有些地方會會check, spare回FF (for FL_MarkImpactedBlock)
		return 0;
	}
	if (m_isSQL)
	{
		return ST_DirectPageRead_SQL( obType, DeviceHandle, targetDisk, ReadMode,GetCeMapping(bCE)
			, wBLOCK, wStartPage, bPageCount, bECCBit, Retry_Read, bCeMask, datalen ,  w_buf,  bNewEccFormat);
	}


	bool isCDB512 = true;
	unsigned char cdb[512] = {0};

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = bNewEccFormat?0xFB:0xFF;
	cdb[3] = ReadMode;
	cdb[4] = GetCeMapping(bCE);
	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	cdb[9] = Retry_Read&0xff;
	cdb[10] = bPageCount & 0xff; //WL
	cdb[11] = bPageCount >> 8; //WL
	cdb[12] = bECCBit;
	cdb[15] = bCeMask & 0xff; //CE Offset .. 決定哪顆ce 不要做Write
	cdb[16] = (bCeMask>>8) & 0xff; 
	cdb[17] = (bCeMask>>16) & 0xff; 
	cdb[18] = (bCeMask>>24) & 0xff; 
	nt_log_cdb(cdb,"%s1",__FUNCTION__);


	for(int i=0;i<2;i++)
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime, isCDB512);
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(3000);
	}

	return iResult;
#else
	memset(w_buf,0x01,datalen);
	return 0;
#endif
}


int CNTCommand::NewSort_SetPageSize ( int pagesize )
{
	return 0;
}
int CNTCommand::ST_MicroSetRandomize(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE OpenClose)
{
	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xad;
	cdb[3] = 0x92;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = OpenClose;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	return iResult;
#else
	return 0x00;
#endif

}

//interface for A2
int CNTCommand::ST_DirectPageRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
								  ,WORD wBLOCK,WORD wStartPage,WORD bPageCount,BYTE bECCBit,BYTE Retry_Read,int a2command,UINT32 bCeMask,int datalen ,unsigned char *w_buf, bool bNewEccFormat)
{
#ifndef __SKIP__TEST__
	int iResult = 0;
	int rtn =0;
	if (IsFakeBlock(wBLOCK))
	{
		return 0;
	}
	if(m_isSQL)
	{
		return ST_DirectPageRead_SQL( obType, DeviceHandle, targetDisk, ReadMode, GetCeMapping(bCE),
			wBLOCK, wStartPage, bPageCount, bECCBit, Retry_Read, a2command, bCeMask, datalen ,  w_buf,  bNewEccFormat);
	}
	
	bool isCDB512 = true;
	unsigned char cdb[512] = {0};

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = bNewEccFormat?0xFB:0xFF;
	cdb[3] = ReadMode;
	cdb[4] = GetCeMapping(bCE);
	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	cdb[9] = Retry_Read&0xff;
	if(a2command)
	{
		//assert(wStartPage%2==0);
		cdb[9] |= 0x40;
	}
	cdb[10] = bPageCount & 0xff; //WL
	cdb[11] = bPageCount >> 8; //WL
	cdb[12] = bECCBit;
	cdb[15] = bCeMask & 0xff; //CE Offset .. 決定哪顆ce 不要做Read
	cdb[16] = (bCeMask>>8) & 0xff; 
	cdb[17] = (bCeMask>>16) & 0xff; 
	cdb[18] = (bCeMask>>24) & 0xff; 
	nt_log_cdb(cdb,"%s2",__FUNCTION__);

	for(int i=0;i<2;i++)
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime, isCDB512);
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(3000);
	}

	return iResult;
#else
	memset(w_buf,0x01,datalen);
	return 0;
#endif
}


int CNTCommand::ST_MarkBad(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
						   ,WORD wBLOCK,WORD wStartPage,WORD wPageCount)
{
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xEF;
	cdb[3] = WriteMode;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = GetCeMapping(bCE);
	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	cdb[9] = 0x08;
	cdb[10] = wPageCount & 0xff; //WL
	cdb[11] = wPageCount >> 8; //WL
	cdb[12] = 0x00;
	cdb[15] = 0x00;
	nt_log_cdb(cdb,"%s",__FUNCTION__);


	for(int i=0;i<2;i++)
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);

		if(ST_CheckResult(iResult))
		{
			break;
		}

		Sleep(1000);
	}
	//Sleep(15);
	return iResult;
#else
	return 0x00;
#endif
}


/*******************************************************
ED3 -> for D3 write

********************************************************/
int CNTCommand::ST_CheckResult(int iResult)
{
	if(iResult ==0 || iResult == 0xfffffffe )
	{
		return 1;
	}

	if(iResult == 0x48f){
		return 0;
	}
	return 0;
}

int CNTCommand::ST_DirectPageWriteBufferMode(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
											 ,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckECC,WORD bRetry_Read,int a2command,int datalen ,int bOffset,unsigned char *w_buf,BYTE TYPE,DWORD Bank)
{
	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xED;
	cdb[3] = WriteMode;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = bCE;
	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	cdb[9] = bRetry_Read&0xff;
	if(a2command)
	{
		cdb[9] |= 0x40;
	}
	cdb[10] = wPageCount & 0xff; //WL
	cdb[11] = wPageCount >> 8; //WL
	cdb[12] = CheckECC;
	cdb[15] = bOffset; //CE Offset .. 決定哪顆ce 不要做Write


	for(int i=0;i<2;i++)
	{			


		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);

		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);

	}
	return iResult;
#else
	return 0x00;
#endif
}


int CNTCommand::ST_WriteIB(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
						   ,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckECC,WORD bRetry_Read,int a2command,int datalen ,int bOffset,unsigned char *w_buf,
						   BYTE TYPE,DWORD Bank,BYTE S0,BYTE S1,BYTE S2,BYTE S3)
{
	nt_log("%s",__FUNCTION__);
	if (SSD_NTCMD == m_cmdtype) {
		return 0; //S11 not support write ib, just return 0;
	}
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xEF;
	cdb[3] = WriteMode;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = bCE;
	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	cdb[9] = bRetry_Read&0xff;
	if(a2command)
	{
		cdb[9] |= 0x40;
	}
	cdb[10] = wPageCount & 0xff; //WL
	cdb[11] = wPageCount >> 8; //WL
	cdb[12] = CheckECC;
	cdb[15] = bOffset; //CE Offset .. 決定哪顆ce 不要做Write

	//正常D1 Write , MP是要收DATA 但是要update IB 時候MP要送data , CLOSE SEED 被打開的時候代表要送data
	//如果WriteMode 被打開則cdb[15]是代表別的意思
	if((WriteMode & 0x04) == 0x04)  //_CLOSE_SEED_
	{
		if((TYPE == 1))  //WRITE CP 
		{
			cdb[15] = bOffset;
			cdb[12] = 'C';  //block mode
			cdb[13] = 'P';  //skip 
		}
		else if(TYPE == 2) //CZ
		{
			cdb[12] = S0;  //block mode
			cdb[13] = S1;  //skip 
			cdb[14] = S2;
			cdb[15] = S3;
		}
		else if(TYPE == 3) // IB
		{
			cdb[12] = S0; 
			cdb[13] = S1;
			cdb[14] = S2;
			cdb[15] = S3;
		}
		else if(TYPE == 4) // My Record
		{
			cdb[12] = 0xAA; 
			cdb[13] = 0xAA; 
		}
		else if(TYPE == 5) //page IB
		{
			cdb[12] = 'I'; 
			cdb[13] = 'B'; 
			cdb[14] = 0x4D; 
		}

		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
	}
	else
	{
		for(int i=0;i<2;i++)
		{			
			if((WriteMode & 0x80) == 0x80)
			{
				if(obType ==0)
					iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf,_SCSI_DelayTime);
				else
					iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);
			}
			else
			{
				if(wPageCount != 1)
				{
					if(obType ==0)
						iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, _SCSI_DelayTime);
					else
						iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, _SCSI_DelayTime);
				}
				else
				{
					if(obType ==0)
						iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
					else
						iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
				}
			}

			if(ST_CheckResult(iResult))
			{
				break;
			}
			Sleep(500);
		}
	}
	return iResult;
#else
	return 0x00;
#endif
}

int CNTCommand::ST_SetBlockSeed(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,WORD wBLOCK)
{

	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xa9;
	cdb[3] = (wBLOCK >> 8 ) & 0xFF;
	cdb[4] = wBLOCK &0xff;

	for(int i=0;i<2;i++)
	{			

		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, _SCSI_DelayTime);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, _SCSI_DelayTime);

		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);
	}

	return iResult;
#else
	return 0x00;
#endif

}
int CNTCommand::ST_DirectPageWriteByConv(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
										 ,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckECC,BYTE bRetry_Read,int a2command,int datalen ,int bOffset,unsigned char *w_buf,
										 BYTE TYPE,BYTE BlockORPageSeed,BYTE Seed,DWORD Reserve)
{
	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	assert(WriteMode & 0x01);
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xEF;
	cdb[3] = WriteMode;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = bCE;
	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	cdb[9] = bRetry_Read;
	if(a2command)
	{
		cdb[9] |= 0x40;
	}
	cdb[10] = wPageCount & 0xff; //WL
	cdb[11] = wPageCount >> 8; //WL
	cdb[12] = CheckECC;
	cdb[13] = BlockORPageSeed;  //skip 
	cdb[14] = Seed;
	cdb[15] = bOffset; //CE Offset .. 決定哪顆ce 不要做Write

	for(int i=0;i<2;i++)
	{			
		if((WriteMode & 0x80) == 0x80)
		{
			if(obType ==0)
				iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf,_SCSI_DelayTime);
			else
				iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);
		}
		else
		{
			if(wPageCount != 1)
			{
				if(obType ==0)
					iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, _SCSI_DelayTime);
				else
					iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, _SCSI_DelayTime);
			}
			else
			{
				if(obType ==0)
					iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
				else
					iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
			}
		}

		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);
	}

	return iResult;
#else
	return 0x00;
#endif
}

int CNTCommand::ST_DirectPageWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
								   ,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckECC,BYTE bRetry_Read,int a2command,int datalen ,int bOffset,unsigned char *w_buf,
								   BYTE TYPE,DWORD Bank,BYTE bIBmode,BYTE bIBSkip, BYTE bRRFailSkip, bool bNewFormat)
{
#ifndef __SKIP__TEST__
	int iResult = 0;


	if (IsFakeBlock(wBLOCK))
	{
		return 0;
	}
	if(m_isSQL)
	{
		return ST_DirectPageWrite_SQL( obType, DeviceHandle, targetDisk, WriteMode, GetCeMapping(bCE),
			wBLOCK, wStartPage, wPageCount, CheckECC, bRetry_Read, a2command, datalen , bOffset, w_buf,
			 TYPE, Bank, bIBmode, bIBSkip,  bRRFailSkip);
	}
	bool isCDB512 = true;
	unsigned char cdb[512] = {0};
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = bNewFormat?0xEB:0xEF;
	cdb[3] = WriteMode;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = GetCeMapping(bCE);
	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	cdb[9] = bRetry_Read;
	if(a2command)
	{
		cdb[9] |= 0x40;
	}
	cdb[10] = wPageCount & 0xff; //WL
	cdb[11] = wPageCount >> 8; //WL
	cdb[12] = CheckECC;
	cdb[13] = bRRFailSkip;
	cdb[15] = bOffset & 0xff; //CE Offset .. 決定哪顆ce 不要做Write
	cdb[16] = (bOffset>>8) & 0xff; 
	cdb[17] = (bOffset>>16) & 0xff; 
	cdb[18] = (bOffset>>24) & 0xff; 

	//正常D1 Write , MP是要收DATA 但是要update IB 時候MP要送data , CLOSE SEED 被打開的時候代表要送data
	//如果WriteMode 被打開則cdb[15]是代表別的意思
	if((WriteMode & 0x04) == 0x04 && wPageCount==1)  //_CLOSE_SEED_
	{
		if((TYPE == 1))  //WRITE CP 
		{
			cdb[15] = bOffset;
			cdb[12] = 'C';  //block mode
			cdb[13] = 'P';  //skip 
		}
		else if(TYPE == 2) //CZ
		{
			cdb[12] = 'C';  //block mode
			cdb[13] = 'Z';  //skip 
			cdb[14] = (Bank >> 8)&0xff;
			cdb[15] = Bank & 0xff;
		}
		else if(TYPE == 3) // IB
		{
			cdb[12] = 'I'; 
			cdb[13] = 'B';
			cdb[14] = bIBmode;
			cdb[15] = bIBSkip;
		}
		else if(TYPE == 4) // My Record
		{
			cdb[12] = 0xAA; 
			cdb[13] = 0xAA; 
		}
		else if(TYPE == 5) //page IB
		{
			cdb[12] = 'I'; 
			cdb[13] = 'B'; 
			cdb[14] = 0x4D; 
		}
		else if(TYPE == 6) //CK
		{
			cdb[12] = 'C'; 
			cdb[13] = 'K'; 
			//cdb[14] = 0x4D; 
		}
		else if(TYPE == 7) //re write OK for SSD
		{
			cdb[12] = 'O'; 
			cdb[13] = 'K'; 
			cdb[14] = 'S'; 
		}
		else if(TYPE == 8) //Page-based CZ
		{
			cdb[12] = 'C';  
			cdb[13] = 'Z';
			cdb[14] = 0xEA;
		}
		nt_log_cdb(cdb,"%s",__FUNCTION__);
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime, isCDB512);
	}
	else
	{
		nt_log_cdb(cdb,"%s",__FUNCTION__);
		for(int i=0;i<2;i++)
		{			
			if((WriteMode & 0x80) == 0x80)
			{
				if(obType ==0)
					iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf,_SCSI_DelayTime);
				else
					iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime, isCDB512);
			}
			else
			{
				if(wPageCount != 1)
				{
					if(obType ==0)
						iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, _SCSI_DelayTime);
					else
						iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, _SCSI_DelayTime, isCDB512);
				}
				else
				{
					if(obType ==0)
						iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
					else
						iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime, isCDB512);
				}
			}

			if(ST_CheckResult(iResult))
			{
				break;
			}
			Sleep(500);
		}
	}
	return iResult;
#else
	return 0x00;
#endif
}

int CNTCommand::ST_BlockWriteByPage(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
									,WORD wStartBLOCK,WORD wPage,WORD wBlockCount,BYTE CheckECC,BYTE bRetry_Read,int a2command,BYTE bBlockPageMode,int datalen ,int bOffset,unsigned char *w_buf,BYTE TYPE,DWORD Bank)
{
	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xEF;
	cdb[3] = WriteMode;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = GetCeMapping(bCE);
	cdb[5] = wStartBLOCK & 0xFF;
	cdb[6] = wStartBLOCK >> 8;
	cdb[7] = wPage & 0xFF;
	cdb[8] = wPage >> 8;
	cdb[9] = bRetry_Read;
	if(a2command)
	{
		cdb[9] |= 0x40;
	}
	if(bBlockPageMode)
	{
		cdb[9] |= 0x80;
	}
	cdb[10] = wBlockCount & 0xff; //WL
	cdb[11] = wBlockCount >> 8; //WL
	cdb[12] = CheckECC;
	cdb[15] = bOffset; //CE Offset .. 決定哪顆ce 不要做Write

	//正常D1 Write , MP是要收DATA 但是要update IB 時候MP要送data , CLOSE SEED 被打開的時候代表要送data
	//如果WriteMode 被打開則cdb[15]是代表別的意思
	if((WriteMode & 0x04) == 0x04)  //_CLOSE_SEED_
	{
		if((TYPE == 1))  //WRITE CP 
		{
			cdb[15] = bOffset;
			cdb[12] = 'C';  //block mode
			cdb[13] = 'P';  //skip 
		}
		else if(TYPE == 2) //CZ
		{
			cdb[12] = 'C';  //block mode
			cdb[13] = 'Z';  //skip 
			cdb[14] = (Bank >> 8)&0xff;
			cdb[15] = Bank & 0xff;
		}
		else if(TYPE == 3) // IB
		{
			cdb[12] = 'I'; 
			cdb[13] = 'B'; 
		}
		else if(TYPE == 4) // My Record
		{
			cdb[12] = 0xAA; 
			cdb[13] = 0xAA; 
		}
		else if(TYPE == 5) //page IB
		{
			cdb[12] = 'I'; 
			cdb[13] = 'B'; 
			cdb[14] = 0x4D; 
		}

		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
	}
	else
	{

		for(int i=0;i<2;i++)
		{			
			if(obType ==0)
				iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);
			else
				iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);

			if(ST_CheckResult(iResult))
			{
				break;
			}
			Sleep(500);
		}
	}
	return iResult;
#else
	return 0x00;
#endif
}

/*******************************************************
ED3 -> for D3 write
bCE 如果下全部CE數 就是一次CE全寫 , 否則可以針對單CE去寫
wBLOCK : block address
SectorCount : 長度 多少BYTE 
********************************************************/

int CNTCommand::ST_DirectOneBlockWrite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
									   ,WORD wBLOCK,BYTE SkipSector,int datalen ,unsigned char *w_buf,UINT32 bDontDirectWriteCEMask, bool bNewFormat)
{
	int iResult = 0;
	unsigned char cdb[512] = {0};
	bool isCDB512 = true;
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	if (IsFakeBlock(wBLOCK))
	{
		return 0;
	}
	if(m_isSQL)
	{
		return ST_DirectOneBlockWrite_SQL( obType, DeviceHandle, targetDisk, WriteMode, GetCeMapping(bCE), 
			 wBLOCK, SkipSector, datalen ,w_buf, bDontDirectWriteCEMask);
	}
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = bNewFormat?0xEC:0xEE;
	cdb[3] = WriteMode;
	cdb[4] = GetCeMapping(bCE);

	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	//SCSI_IOCTL_DATA_UNSPECIFIED
	cdb[9] = SkipSector;
	cdb[15] = bDontDirectWriteCEMask & 0xFF;
	cdb[16] = (bDontDirectWriteCEMask>>8) & 0xFF;
	cdb[17] = (bDontDirectWriteCEMask>>16) & 0xFF;
	cdb[18] = (bDontDirectWriteCEMask>>24) & 0xFF;
	nt_log_cdb(cdb,"%s",__FUNCTION__);



	for(int i=0;i<3;i++)
	{
		if(obType ==0)
		{
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, w_buf, 60);
		}
		else
		{
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, w_buf, 60, isCDB512);
		}
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(700);
	}
	return iResult;
}


int CNTCommand::ST_D3PageWriteRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
								   ,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckEcc,BYTE bRetry_Read,int datalen ,unsigned char *w_buf,UINT32 bDontDirectWriteCEMask,BYTE DumpWriteData, bool bNewFormat)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[512] = {0};
	bool isCDB512 = true;


	if (IsFakeBlock(wBLOCK))
	{
		return 0;
	}

	if(!DumpWriteData){
		//PageWriteRead
		WriteMode |= 0x80; 
	}

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = bNewFormat?0xEC:0xEE;
	cdb[3] = WriteMode;
	cdb[4] = GetCeMapping(bCE);

	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;
	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	//SCSI_IOCTL_DATA_UNSPECIFIED
	cdb[9] = bRetry_Read;

	cdb[10] = wPageCount & 0xFF;
	cdb[11] = wPageCount >> 8;
	cdb[12] = CheckEcc;
	cdb[15] = bDontDirectWriteCEMask & 0xFF;
	cdb[16] = (bDontDirectWriteCEMask>>8) & 0xFF;
	cdb[17] = (bDontDirectWriteCEMask>>16) & 0xFF;
	cdb[18] = (bDontDirectWriteCEMask>>24) & 0xFF;
	if(DumpWriteData)//dump write
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf,60);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, 60, isCDB512);
	}
	else{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, 60);//針對 B16 拉長時間
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, 60, isCDB512);
	}
	return iResult;
}

int CNTCommand::ST_SkipSector(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bReadSkip,unsigned char *w_buf)
{
	int iResult = 0;
	unsigned char cdb[16];

	return 0; //ckzhang todo, workaround due to 2307 may fill worng buffer.
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xBE;
	cdb[3] = bReadSkip; // 1 read, 0 set
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,bReadSkip==1?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, 512, w_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,bReadSkip==1?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, 512, w_buf, 5);
	return iResult;
}

void CNTCommand::LogHex(const unsigned char* buf, int len )
{
	MemStr str(16*20);
	int cnt = (len+15)/16;
	for( int j=0; j<cnt; j++ )  {
		str.SetEmpty();
		str.Cat("   %04X: ",j*16 );
		int remain = len-j*16;
		for(int i=0; i<remain && i<16; i++)  {
			str.Cat("%02X",buf[j*16+i] );
			str.Cat("%c", (i==7) ? '-' : ' '    );
		}
		str.Cat( "  " );

		for(int i=0; i<remain && i<16; i++)  {
			unsigned char da = buf[j*16+i];
			str.Cat( "%c", (  ((da>='A')&&(da<='Z')) || ((da>='a')&&(da<='z')) ) ? da : '.'  );
		}
		nt_file_log( "%s",str.Data() );
	}
}

int CNTCommand::ST_DumpSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf)
{
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0x05;
	cdb[2] = 0x52;
	cdb[3] = 0x41;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = BufferPoint;
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	_ForceNoSwitch = 1;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 528, r_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 528, r_buf, 5);
	return iResult;
}


int CNTCommand::ST_WriteSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf)
{
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0x05;
	cdb[2] = 0x52;
	cdb[3] = 0x41;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = BufferPoint;
	cdb[6] = 1;
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	_ForceNoSwitch = 1;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, r_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, r_buf, 5);
	return iResult;
}

int CNTCommand::ST_TestINOUT(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE Mode,BYTE bCE,BYTE bEraeTimes,unsigned char *w_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0xA0;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);
	else
		iResult = NT_CustomCmd_in_out_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, w_buf, 15);	
	return iResult;
}


int CNTCommand::ST_HynixCheckWritePortect(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum )
{
	nt_log("%s",__FUNCTION__);
	UCHAR bCBD[16];
	ZeroMemory(bCBD, 16);

	unsigned char bDataBuf[512];
	memset(bDataBuf, 0x00, sizeof(bDataBuf));
	int iResult = 0;

	// Get CE Protect Status
	bCBD[0] = 0x06;  
	bCBD[1] = 0xf6;  
	bCBD[2] = 0x22;

	if(bType == 0)
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 16, SCSI_IOCTL_DATA_IN, 512, bDataBuf, 10);
	}
	else if( bType == 1 )
	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 16, SCSI_IOCTL_DATA_IN, 512, bDataBuf, 10);
	}
	else return 0xFF;

	if( (bDataBuf[0] & 0x01) || (bDataBuf[0] & 0x02) || (bDataBuf[0] & 0x04) || (bDataBuf[0] & 0x08)
		|| (bDataBuf[0] & 0x10) || (bDataBuf[0] & 0x20) || (bDataBuf[0] & 0x40) || (bDataBuf[0] & 0x80) )
	{
		// Set Write Protect CE To Enable Use
		ZeroMemory(bCBD, 16);

		bCBD[0] = 0x06;  
		bCBD[1] = 0xf6;  
		bCBD[2] = 0x21;

		if(bType == 0)
		{
			iResult = NT_CustomCmd(bDrvNum, bCBD, 16, SCSI_IOCTL_DATA_OUT, 512, bDataBuf, 10);
		}
		else if( bType == 1 )
		{
			iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 16, SCSI_IOCTL_DATA_OUT, 512, bDataBuf, 10);
		}
		else return 0xFF;
	}

	return 0;
}

int CNTCommand::ST_CheckDRY_DQS(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE TYPE,BYTE CE_bit_map,DWORD Block)
{
#if 1
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x06; //CD
	//Byte 3 Bit 2 (0x02) Option = 0 Check Copy Data, 
	//       Bit 2               =1 Check  D3 Write Data
	cdb[3] = TYPE; //
	cdb[4] = CE_bit_map; // 1: 要跳過的CE
	cdb[5] = (Block) & 0xff;
	cdb[6] = (Block >> 8) & 0xff;
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	for(int i=0;i<2;i++){
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 5);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 5);	
		if(!iResult) break;
		Sleep(500);
	}
	return iResult;
#else
	return 0;
#endif
}

int CNTCommand::ST_CopyBackWiteD3(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE Mode,BYTE bCE,BYTE bEraeTimes,unsigned char *w_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xCF; //CD
	//Byte 3 Bit 2 (0x02) Option = 0 Check Copy Data, 
	//       Bit 2               =1 Check  D3 Write Data
	cdb[3] = Mode; //
	cdb[4] = bCE;
	cdb[13] = bEraeTimes; //Earse Time .. default 5 times


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, w_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, w_buf, 5);	
	return iResult;
}
int CNTCommand::ST_GetFixtureHubID(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf)
{
	nt_log("%s",__FUNCTION__);
	OutputDebugString("$$$ST_GetFixtureHubID\n");
	//06 F6 BA 00 Byte 4 CE Byte 5 CAU(Die) Data 512
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x30;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 8, r_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 8, r_buf, 5);
	return iResult;
}

int CNTCommand::ST_E2NAMEGetEarlyTable(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char ce, unsigned char die,unsigned char *r_buf)
{
	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__
	//06 F6 BA 00 Byte 4 CE Byte 5 CAU(Die) Data 512
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xba;
	cdb[3] = 0x00;  //如果用set parameter 要做mark earley ,則convertion 要關掉
	cdb[4] = ce;
	cdb[5] = die;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 5);
	return iResult;
#else
	return 0;
#endif
}

//臨時的 jeff會整再一起
int CNTCommand::ST_CopyBackWite(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE Mode,BYTE bCE,unsigned short Page,BYTE bEraeTimes,unsigned char *w_buf,unsigned char bOffet,unsigned char SpecialErase)
{
	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xCF; //CD
	//Byte 3 Bit 2 (0x02) Option = 0 Check Copy Data, 
	//       Bit 2               =1 Check  D3 Write Data
	cdb[3] = Mode; //
	cdb[4] = bCE;
	if(SpecialErase==1)
		cdb[9] |= 0x04;
	cdb[10] = Page & 0xff;
	cdb[11] = Page >> 8;
	cdb[13] = bEraeTimes; //Earse Time .. default 5 times
	cdb[14] = 0x00;  //for page mode 一個bit 代表一個ce 
	cdb[15] = bOffet; //決定哪個ce不要做 bit 拉起來代表不要做

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, w_buf, 15);	
	return iResult;
#else
	//	memset(w_buf,0,512);
	return 0;
#endif
}

void CNTCommand::Log(LogLevel level, const char* fmt, ...)
{
	char logbuf[1024];
	va_list arglist;
	va_start(arglist, fmt);
	_vsnprintf(logbuf, sizeof(logbuf), fmt, arglist);
	va_end(arglist);
	nt_file_log(logbuf);
}

void CNTCommand::nt_file_log( const char* fmt, ... )
{
	char logbuf[1024];
	va_list arglist;
	va_start ( arglist, fmt );
	_vsnprintf ( logbuf, sizeof(logbuf), fmt, arglist );
	va_end ( arglist );
	if (_pSortingBase)
		_pSortingBase->Log(logbuf);
	if(logCallBack)
		logCallBack(logbuf);

	if (!_pSortingBase)
	{
		va_list arglist;
		va_start(arglist, fmt);
		_vsnprintf(logbuf, sizeof(logbuf), fmt, arglist);
		va_end(arglist);

		double finish = clock();
		double duration = (double)(finish - _LogStartTime) / CLOCKS_PER_SEC;
		double test_duration = duration;
		unsigned int d_duration = (int)duration;
		unsigned int d_sec = (d_duration % 60);
		unsigned int d_min = (d_duration / 60);
		unsigned int d_hour = (d_min / 60);
		d_min = (d_min % 60);
		char sResultTime[64];
		memset(sResultTime, 0, 64);
		sprintf(sResultTime, "%02d:%02d:%02d", d_hour, d_min, d_sec);
		UFWInterface::Log(LOGDEBUG, "%s:%s", sResultTime, logbuf);
	}

}

int CNTCommand::NT_SetMediaChange(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	nt_log("%s",__FUNCTION__);

	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0x10;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 15);	
	return iResult;

}
int CNTCommand::GetDataRateByClockIdx(int clk_idx)
{
	return 0;
}

int CNTCommand::ST_SetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMode, unsigned char *w_buf, BYTE bONFIType, BYTE bONFIMode, BOOL bToggle20)
{
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xaf;
	cdb[3] = bMode;
	int int_interface = 0;

	if ((bONFIMode == 5) || !bToggle20)
		cdb[4] = 0;

	if ((bONFIMode == 7) || bToggle20)
		cdb[4] = 1;

	if (bONFIType == 4)
	{
		cdb[4] = 2; //Toggle3.0
	}
	cdb[5] = bONFIMode;

	if (0 == bMode) {
		m_flashfeature = FEATURE_LEGACY;
		int_interface = 0;
		nt_log("## FEATURE_LEGACY ##");
	} else if(1 == bMode && 0 == cdb[4]) {
		m_flashfeature = FEATURE_T1;
		int_interface = 1;
		nt_log("## FEATURE_T1 ##");
	} else if (1 == bMode && 1 == cdb[4]) {
		m_flashfeature = FEATURE_T2;
		int_interface = 2;
		nt_log("## FEATURE_T2 ##");
	} else if (1 == bMode && 2 == cdb[4]) {
		m_flashfeature = FEATURE_T3;
		int_interface = 3;
		nt_log("## FEATURE_T3 ##");
	} else {
		nt_log("## UNKNOWN FEATURE ##");
	}

	if ((0x2C == _FH.bMaker) && (3 == int_interface))
	{
		int current_clk_idx = GetDefaultDataRateIdxForInterFace(3);
		cdb[5] = MyFlashInfo::GetMicroClockTable(GetDataRateByClockIdx(current_clk_idx), int_interface, &_FH);
	}

	nt_log_cdb(cdb,"%s",__FUNCTION__);

	_realCommand =1;
	_ForceNoSwitch = 0;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);	
	return iResult;

}
int CNTCommand::GetDefaultDataRateIdxForInterFace(int interFace)
{
	return 0;
}

int	CNTCommand::ST_GetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];

	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0xF6;
	CDB[2] = 0xAE;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512,w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);	

	return iResult;
}
//For 2048 buffer 用
int CNTCommand::ST_GetProgramCountStatus1(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf)
{
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x50;

	for(int i=0;i<2;i++)
	{
		if(obType ==0)
		{
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 2048, w_buf, 5);
		}
		else
		{
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 2048, w_buf, 5);
		}
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);
	}
	return iResult;
#else
	memset(w_buf,0x00,512);
	return 0;
#endif
}

int CNTCommand::ST_GetProgramCountStatus(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf)
{
	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x50;

	for(int i=0;i<2;i++)
	{
		if(obType ==0)
		{
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 1024, w_buf, 5);
		}
		else
		{
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 1024, w_buf, 5);
		}
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);
	}
	return iResult;
#else
	memset(w_buf,0x00,512);
	return 0;
#endif
}

int CNTCommand::ST_GetReadWriteStatus(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf)
{
#ifndef __SKIP__TEST__
	if (m_isSQL)
	{
		return ST_GetReadWriteStatus_SQL(w_buf);
	}
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xbc;
	nt_log_cdb(cdb,"%s",__FUNCTION__);	
	for(int i=0;i<2;i++)
	{
		if(obType ==0)
		{
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 5);
		}
		else
		{
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 5);
		}
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);
	}
	return iResult;
#else
	memset(w_buf,0x00,512);
	return 0;
#endif

}

/*
0xFC & 0xFE	D3_READ_NEW_FORMAT/D3_READ	
CDB[3] bit1: 2p mode
CDB[3] bit5: cache
CDB[3] bit6: 4p mode
CDB[3] bit7: read完做erase
CDB[4]: 執行CE
CDB[5]: block (low byte)
CDB[6]: block (high byte)
CDB[7]: page (low byte)
CDB[8]: page (high byte)
CDB[9] bit0: retry read
CDB[9] bit4: 不檢查LCA
CDB[10]: page count (low byte)
CDB[11]: page count (high byte)
CDB[12]: ECC threshold
CDB[15]: Skip CE number (0-7)
CDB[16]: Skip CE number (8-16)
*/
int CNTCommand::ST_DirectOneBlockRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
									  ,WORD wBLOCK,WORD wStartPage,WORD wPCount,BYTE SkipSector
									  ,BYTE CheckECCBit, BYTE patternBase, int len ,unsigned char *w_buf,int LowPageRetryEcc, bool bNewEccFormat, UINT32 CeMask)
{
#ifndef SKIP__TEST
	int iResult = 0;
	int rtn = 0;
	int sqlRetryRead = GetPrivateProfileInt("Sorting","SQLRetryRead",0, _ini_file_name);
	if (IsFakeBlock(wBLOCK))
	{
		return 0;
	}
	if(m_isSQL || (sqlRetryRead && SkipSector))
	{
		return ST_DirectOneBlockRead_SQL( obType, DeviceHandle, targetDisk, ReadMode, GetCeMapping(bCE), 
			wBLOCK, wStartPage, wPCount, SkipSector, 
			CheckECCBit,  patternBase,  len ,  w_buf, LowPageRetryEcc,  bNewEccFormat);
	}

	bool isCDB512 = true;
	unsigned char cdb[512] = {0};

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = bNewEccFormat?0xFC:0xFE;
	//Byte 3 Bit 2 (0x02) Option = 0 Check Copy Data, 
	//       Bit 2               =1 Check  D3 Write Data
	cdb[3] = ReadMode; 
	cdb[4] = GetCeMapping(bCE);
	cdb[5] = wBLOCK & 0xFF;
	cdb[6] = wBLOCK >> 8;

	cdb[7] = wStartPage & 0xFF;
	cdb[8] = wStartPage >> 8;
	cdb[9] = SkipSector;
	cdb[10] = wPCount & 0xff; //WL
	cdb[11] = wPCount >> 8; //WL

	//cdb[9] = SkipSector;
	//cdb[10] = wPageCount & 0xff; //WL
	//cdb[11] = wPageCount >> 8; //WL
	cdb[12] = CheckECCBit;
	cdb[13] |= patternBase;  // bit 0 舉起, dump read 會使用 pattern base 來當 seed
	//************************************************************************/
	// A19 low middle up page .. 針對low page 用不同的retry read ecc條件      /
	//************************************************************************/
	cdb[14]  = LowPageRetryEcc;
	cdb[15] = CeMask & 0xff; //CE Offset .. 決定哪顆ce 不要做Write
	cdb[16] = (CeMask>>8) & 0xff; 
	cdb[17] = (CeMask>>16) & 0xff; 
	cdb[18] = (CeMask>>24) & 0xff; 
	nt_log_cdb(cdb,"%s",__FUNCTION__);




	for(int i=0;i<2;i++)
	{
		if(obType ==0)
		{
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, w_buf, _SCSI_DelayTime);
		}
		else
		{
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, w_buf, _SCSI_DelayTime, isCDB512);
		}
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(3000);
	}

	return iResult;

#else
	return 0;
#endif
}


int CNTCommand::ST_SectorBitMapGetSet(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet,BYTE Die,BYTE *w_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xBE;

	cdb[3] = bGetSet;  //0 set , 1 get

	for(int i=0;i<2;i++)
	{			
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,bGetSet==1?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, 512, w_buf, 5);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,bGetSet==1?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, 512, w_buf, 5);


		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);
	}



	return iResult;
}

int CNTCommand::ST_PageBitMapGetSet(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet,BYTE Die,BYTE *w_buf)
{
	int iResult = 0;
	unsigned char cdb[16];
	int sqlRetryRead = GetPrivateProfileInt("Sorting","SQLRetryRead",0, _ini_file_name);
	if(m_isSQL)
	{
		return ST_PageBitMapGetSet_SQL(bGetSet,Die,w_buf);
	}

	if(sqlRetryRead) {
		ST_PageBitMapGetSet_SQL(bGetSet,Die,w_buf);
	}
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xbd;

	cdb[3] = bGetSet;  //0 set , 1 get
	nt_log_cdb(cdb,"%s",__FUNCTION__);

	for(int i=0;i<2;i++)
	{			
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,bGetSet==1?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, 1024, w_buf, 5);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,bGetSet==1?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT, 1024, w_buf, 5);


		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);
	}

	return iResult;
}

/************************************************************************/
/*                                                                   
parameter:
BYTE PBAVer 只有在get的時後需要設定0(expba1 or 2(expba2)				*/
/************************************************************************/
int CNTCommand::ST_PBADieGetSet(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bGetSet,BYTE Die,int flashType,BYTE PBAVer,BYTE *w_buf)
{
	nt_log("%s",__FUNCTION__);
#ifndef __SKIP__TEST__1
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x02;

	cdb[3] = bGetSet; 
	cdb[4] = Die;
	cdb[6] = flashType;
	cdb[7] = PBAVer;
	_realCommand =1;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 5);
	return iResult;
#else
	return 0;
#endif
}

int CNTCommand::ST_HynixTLC16ModeSwitch(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bMode)
{
	nt_log("%s",__FUNCTION__);
	/*
	Set boot mode
	06    f6     20   01    00    …

	Set normal mode
	06    f6     20   00    00    …*/
	if(m_isSQL)
	{
		return ST_HynixTLC16ModeSwitch_SQL( obType, DeviceHandle, targetDisk, bMode );
	}
#ifndef __SKIP__TEST__
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x20;
	cdb[3] = bMode; //1 boot Mode , 0 NormalMode 
	_realCommand = 1;
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	return iResult;
#else
	return 0;
#endif
	return 0;
}

/*
0xCE	ERASE	
CDB[3] bit0: SLC mode
CDB[3] bit1: 2-plane
CDB[3] bit6: 4-plane
CDB[4]: 執行CE
CDB[5]: block low byte
CDB[6]: block high byte
*/
int CNTCommand::ST_DirectOneBlockErase(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bSLCMode,BYTE bChipEnable,WORD wBLOCK,UINT32 bCEOffset,int len ,unsigned char *w_buf,int bDF)
{
#ifndef __SKIP__TEST__
	int iResult = 0;
	int rtn = 0;
	unsigned char cdb[512] = {0};
	bool isCDB512 = true;
	int status = 0;

	if(m_isSQL)
	{
		status = ST_DirectOneBlockErase_SQL(obType, DeviceHandle, targetDisk, bSLCMode, GetCeMapping(bChipEnable), wBLOCK, bCEOffset, len, w_buf, bDF);
		return status;
	}
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xCE;
	cdb[3] = bSLCMode; //1 D1Mode 0xA2, 0 D3Mode 
	cdb[4] = GetCeMapping(bChipEnable); //CE
	cdb[6] = wBLOCK >> 8;
	cdb[5] = wBLOCK & 0xFF;
	if(bDF){
		cdb[9] = 0x04;
		cdb[10] = bDF;
	}
	cdb[15] = bCEOffset & 0xFF; //CE Offset .. 決定哪顆ce 不要做Write
	cdb[16] = (bCEOffset>>8) & 0xff; 
	cdb[17] = (bCEOffset>>16) & 0xff; 
	cdb[18] = (bCEOffset>>24) & 0xff; 
	
	nt_log_cdb(cdb, "%s",__FUNCTION__);

	for(int i=0;i<2;i++)
	{			
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 15);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 15, isCDB512);


		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(500);
	}
	return iResult;

	//if(obType ==0)
	//	iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	//else
	//	iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 5);
	//return iResult;
#else
	return 0;
#endif
}

#define BIT3 0x08

int CNTCommand::Trim_U3_to_U2(unsigned char *U2_PI_H_Reg,unsigned char *U2_PI_L_Reg,unsigned char *U2_DIV_Reg,
							  unsigned char U3_DIV_Reg,unsigned char U3_PI_H_Reg,unsigned char U3_PI_L_Reg)
{

	signed long U2_PI;
	signed long U2_DIV;
	signed long U3_PI;
	signed long U3_DIV;
	signed long fRC;
	signed long Dividend;//被除數
	signed long Divisor;//除數
	signed long tmpX;
	signed long tmpY;
	signed long Enlarge = 100;//不能小於100

	/*
	fRC  =      5000/(2*DIV*(1+24.3*PI/1000000))
	fRC  =      2500/(DIV*(1+24.3*PI/1000000))

	fRC  =      (2500*1000000)      /       ((1000000 * DIV)+(24.3*DIV*PI));
	_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
	=      Dividend  /       Divisor;
	=      Dividend  /       (tmpX+tmpY)

	(2500*1000000)     :       Dividend
	(1250*1000000)     :       Dividend預除2之後會補償

	(tmpX+tmpY)           :       Divisor
	(1000000 * DIV)      :       tmpX

	(24.3*DIV*PI)          :       tmpY
	((243*DIV*PI)/10)   :       tmpY 求精確


	*/


#if 0
	//Debug
	memset ( USB_BULK_BUFFER, 0x00,  1024 );

	U3_PI = ((X_CMDBLOCK[3] << 8) | (X_CMDBLOCK[4]));
	U3_DIV = (X_CMDBLOCK[5]);

	if (X_CMDBLOCK[3] & (BIT7 | BIT6 | BIT5 | BIT4 | BIT3))
#else

	U3_PI = ((U3_PI_H_Reg << 8) | U3_PI_L_Reg);
	U3_DIV = U3_DIV_Reg;
	if (U3_PI_H_Reg & BIT3)
#endif
	{
		//負數
		U3_PI = (0xFFFFF000 | U3_PI);
	} else {
		//確保取正確正整數
		U3_PI = (U3_PI & 0x000007FF);
	}

	Dividend = (1250 * 1000000);


	tmpX = (1000000 * U3_DIV);

	tmpY = ((243 * U3_DIV * U3_PI) / 10);


	Divisor = tmpX + tmpY;




	/*
	為了取得精確度
	fRC         會比實際大10000倍(Enlarge)
	21.1174 => 211174

	*/
	Enlarge = 10000;

	/*
	為了取得精確度
	*2是對Dividend補償
	*/
	Divisor = (Divisor / (Enlarge * 2 ));





	fRC = Dividend / Divisor;

	U2_DIV    =      (1920  * Enlarge * 10 /* *10取小數*/) / (fRC);




	/*四捨五入*/
	if ((U2_DIV % 10) >= 5) {
		U2_DIV = ((U2_DIV / 10) + 1);
	} else {
		U2_DIV = ((U2_DIV / 10) + 0);
	}

	U2_PI       =      ((((1920 * Enlarge) - (U2_DIV * fRC)) * 1000 * 10/* *10取小數*/) / (3 * (Enlarge / 10) * 192));



	/*四捨五入*/
	if ((U2_PI % 10) >= 5) {
		U2_PI = ((U2_PI / 10) + 1);
	} else {
		U2_PI = ((U2_PI / 10) + 0);
	}

	*U2_DIV_Reg = U2_DIV&0xff;

	*U2_PI_H_Reg = ((U2_PI >> 8) & 0x0F);
	*U2_PI_L_Reg = (U2_PI & 0xFF);

#if 0
	//Debug
	BUFA_Base[0] = U3_PI >> 24;
	BUFA_Base[1] = U3_PI >> 16;
	BUFA_Base[2] = U3_PI >> 8;
	BUFA_Base[3] = U3_PI;


	BUFA_Base[0 + 0x10] = U3_DIV >> 24;
	BUFA_Base[1 + 0x10] = U3_DIV >> 16;
	BUFA_Base[2 + 0x10] = U3_DIV >> 8;
	BUFA_Base[3 + 0x10] = U3_DIV;

	BUFA_Base[0 + 0x20] = Dividend >> 24;
	BUFA_Base[1 + 0x20] = Dividend >> 16;
	BUFA_Base[2 + 0x20] = Dividend >> 8;
	BUFA_Base[3 + 0x20] = Dividend;

	BUFA_Base[0 + 0x30] = Divisor >> 24;
	BUFA_Base[1 + 0x30] = Divisor >> 16;
	BUFA_Base[2 + 0x30] = Divisor >> 8;
	BUFA_Base[3 + 0x30] = Divisor;

	BUFA_Base[0 + 0x40] = fRC >> 24;
	BUFA_Base[1 + 0x40] = fRC >> 16;
	BUFA_Base[2 + 0x40] = fRC >> 8;
	BUFA_Base[3 + 0x40] = fRC;

	BUFA_Base[0 + 0x50] = tmpX >> 24;
	BUFA_Base[1 + 0x50] = tmpX >> 16;
	BUFA_Base[2 + 0x50] = tmpX >> 8;
	BUFA_Base[3 + 0x50] = tmpX;

	BUFA_Base[0 + 0x60] = tmpY >> 24;
	BUFA_Base[1 + 0x60] = tmpY >> 16;
	BUFA_Base[2 + 0x60] = tmpY >> 8;
	BUFA_Base[3 + 0x60] = tmpY;

	BUFA_Base[0 + 0x70] = U2_PI >> 24;
	BUFA_Base[1 + 0x70] = U2_PI >> 16;
	BUFA_Base[2 + 0x70] = U2_PI >> 8;
	BUFA_Base[3 + 0x70] = U2_PI;

	BUFA_Base[0 + 0x80] = U2_DIV >> 24;
	BUFA_Base[1 + 0x80] = U2_DIV >> 16;
	BUFA_Base[2 + 0x80] = U2_DIV >> 8;
	BUFA_Base[3 + 0x80] = U2_DIV;

	Usb_Bulk_In_512 ( 2, 0, TOP_BUF );
	Set_Media_OK();
#endif
	return 0;


#if 0
	signed long u2Div;
	signed long u2Ppm;
	signed long u3Div;
	signed long u3Ppm;
	signed long xxtmp;
	signed long aScale = 10000000;

	if ( _FH.AutoTrim4 == 0x0F ) {
		u3Ppm = ( signed long ) _FH.AutoTrim3 - ( signed long ) ( 0xFF );
	} else {
		u3Ppm = _FH.AutoTrim3;
	}

	u3Div = _FH.AutoTrim5;
	xxtmp = aScale;
	xxtmp += 243 * u3Ppm;
	xxtmp /= 125;
	xxtmp *= u3Div;
	xxtmp *= 96;
	u2Div = ( ( xxtmp + 5000000 ) / aScale );
	xxtmp -= u2Div * aScale;
	u2Ppm = ( xxtmp / u2Div ) / 243;
	_FH.AutoTrim0 = ( u2Ppm & 0xFF );
	_FH.AutoTrim1 = ( u2Ppm >> 8 & 0x0F );
	_FH.AutoTrim2 = u2Div;
	return 0;
#endif

}

int CNTCommand::Trim_U2_to_U3(unsigned char U2_PI_H_Reg,unsigned char U2_PI_L_Reg,unsigned char U2_DIV_Reg,
							  unsigned char *U3_DIV_Reg,unsigned char *U3_PI_H_Reg,unsigned char *U3_PI_L_Reg)
{


	signed long U2_PI;
	signed long U2_DIV;
	signed long U3_PI;
	signed long U3_DIV;
	signed long fRC;
	signed long Dividend;//被除數
	signed long Divisor;//除數
	signed long tmpX;
	signed long tmpY;
	signed long Enlarge = 100;//不能小於100

	/*
	fRC  =      (1920)                      /       (DIV * (1+((30*PI)/(1000000))));
	=      (1920*1000000)      /       (DIV * (1000000+(30*PI)));
	=      (1920*1000000)      /       ((1000000 * DIV)+(30*DIV*PI));
	_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
	=      Dividend  /       Divisor;
	=      Dividend  /       (tmpX+tmpY)

	(1920*1000000)     :       Dividend
	(tmpX+tmpY)           :       Divisor
	(1000000 * DIV)      :       tmpX
	(30*DIV*PI)             :       tmpY


	*/


#if 0
	//Debug
	memset ( USB_BULK_BUFFER, 0x00,  1024 );

	U2_PI = ((X_CMDBLOCK[3] << 8) | (X_CMDBLOCK[4]));
	U2_DIV = (X_CMDBLOCK[5]);

	if (X_CMDBLOCK[3] & (BIT7 | BIT6 | BIT5 | BIT4 | BIT3))
#else
	U2_PI      = ((U2_PI_H_Reg << 8) | U2_PI_L_Reg);
	U2_DIV   = U2_DIV_Reg;
	if (U2_PI_H_Reg & BIT3)
#endif
	{
		//負數
		U2_PI = (0xFFFFF000 | U2_PI);
	} else {
		//確保取正確正整數
		U2_PI = (U2_PI & 0x000007FF);
	}


	Dividend = (1920 * 1000000);

	tmpX = (1000000 * U2_DIV);

	tmpY = (30 * U2_DIV * U2_PI);

	Divisor = tmpX + tmpY;

	/*
	為了取得精確度
	fRC         會比實際大10000倍(Enlarge)
	21.1174 => 211174

	*/
	Enlarge = 10000;

	Divisor = (Divisor / Enlarge)/*為了取得精確度*/;

	fRC = Dividend / Divisor;

	U3_DIV    =      (5000  * Enlarge * 10 /* *10取小數*/) / ( 2 * fRC);

	/*四捨五入*/
	if ((U3_DIV % 10) >= 5) {
		U3_DIV = ((U3_DIV / 10) + 1);
	} else {
		U3_DIV = ((U3_DIV / 10) + 0);
	}

	U3_PI       =      ((((5000  * Enlarge) - (2 * U3_DIV * fRC)) * 1000 * 10/* *10取小數*/) / (243 * (Enlarge / 10) * 5));

	/*四捨五入*/
	if ((U3_PI % 10) >= 5) {
		U3_PI = ((U3_PI / 10) + 1);
	} else {
		U3_PI = ((U3_PI / 10) + 0);
	}

	*U3_DIV_Reg = U3_DIV&0xff;

	*U3_PI_H_Reg = ((U3_PI >> 8) & 0x0F);
	*U3_PI_L_Reg = (U3_PI & 0xFF);
#if 0
	//Debug

	BUFA_Base[0] = U2_PI >> 24;
	BUFA_Base[1] = U2_PI >> 16;
	BUFA_Base[2] = U2_PI >> 8;
	BUFA_Base[3] = U2_PI;


	BUFA_Base[0 + 0x10] = U2_DIV >> 24;
	BUFA_Base[1 + 0x10] = U2_DIV >> 16;
	BUFA_Base[2 + 0x10] = U2_DIV >> 8;
	BUFA_Base[3 + 0x10] = U2_DIV;

	BUFA_Base[0 + 0x20] = Dividend >> 24;
	BUFA_Base[1 + 0x20] = Dividend >> 16;
	BUFA_Base[2 + 0x20] = Dividend >> 8;
	BUFA_Base[3 + 0x20] = Dividend;

	BUFA_Base[0 + 0x30] = Divisor >> 24;
	BUFA_Base[1 + 0x30] = Divisor >> 16;
	BUFA_Base[2 + 0x30] = Divisor >> 8;
	BUFA_Base[3 + 0x30] = Divisor;

	BUFA_Base[0 + 0x40] = fRC >> 24;
	BUFA_Base[1 + 0x40] = fRC >> 16;
	BUFA_Base[2 + 0x40] = fRC >> 8;
	BUFA_Base[3 + 0x40] = fRC;

	BUFA_Base[0 + 0x50] = tmpX >> 24;
	BUFA_Base[1 + 0x50] = tmpX >> 16;
	BUFA_Base[2 + 0x50] = tmpX >> 8;
	BUFA_Base[3 + 0x50] = tmpX;

	BUFA_Base[0 + 0x60] = tmpY >> 24;
	BUFA_Base[1 + 0x60] = tmpY >> 16;
	BUFA_Base[2 + 0x60] = tmpY >> 8;
	BUFA_Base[3 + 0x60] = tmpY;

	BUFA_Base[0 + 0x70] = U3_PI >> 24;
	BUFA_Base[1 + 0x70] = U3_PI >> 16;
	BUFA_Base[2 + 0x70] = U3_PI >> 8;
	BUFA_Base[3 + 0x70] = U3_PI;

	BUFA_Base[0 + 0x80] = U3_DIV >> 24;
	BUFA_Base[1 + 0x80] = U3_DIV >> 16;
	BUFA_Base[2 + 0x80] = U3_DIV >> 8;
	BUFA_Base[3 + 0x80] = U3_DIV;
	Usb_Bulk_In_512 ( 2, 0, TOP_BUF );
	Set_Media_OK();
#endif
	return 0;

#if 0
	signed long u2Div = 0 ;
	signed long u2Ppm = 0;
	signed long u3Div = 0;
	signed long u3Ppm = 0;
	signed long xxtmp = 0;
	signed long Scale = 100000;

	if ( _FH.AutoTrim1 == 0x0F ) {
		u2Ppm = ( signed long ) _FH.AutoTrim0 - ( signed long ) ( 0xFF );
	} else {
		u2Ppm = _FH.AutoTrim0;
	}

	u2Div = _FH.AutoTrim2;
	xxtmp  = Scale;
	xxtmp += 3 * u2Ppm;
	xxtmp *= 125;
	xxtmp *= u2Div;
	xxtmp /= 96;
	u3Div = ( ( xxtmp + 50000 ) / Scale );
	xxtmp -= u3Div * Scale;
	u3Ppm = ( xxtmp / u3Div ) / 3;
	_FH.AutoTrim3 = ( u3Ppm & 0xFF );
	_FH.AutoTrim4 = ( u3Ppm >> 8 & 0x0F );
	_FH.AutoTrim5 = u3Div;
	return 0;
#endif

}


int CNTCommand::ST_SetBadColumn(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int *currentDie, int die, int useMBC
								, int MBCSet,bool isED3,bool bSetOBCBadColumn,DWORD DieOffset,unsigned char * apParameter
								,unsigned char *badcolumndata,unsigned char *mbcBuffer)
{
	if (!useMBC ) {
		if (isED3 && bSetOBCBadColumn) {
			if (*currentDie != die) {
				Sleep(1);
				int badcolumnBlock = (DieOffset * die);
				if (ST_Set_Flash_All_Bad_Column_Collect(obType, DeviceHandle, targetDisk, 1, badcolumnBlock)) {
					return 1;
				}


				if(mbcBuffer != NULL){
					if (ST_Set_Get_BadColumn(obType, DeviceHandle, targetDisk, false, 0 , mbcBuffer)) {
						return 1;
					}
				}

				*currentDie = die;
			}

		} else { //if(!_FH.beD3)//d2
			if (*currentDie != die && useMBC) {

				int tMBCSet = MBCSet;
				/************************************************************************/
				/* MBC 混mode                                                           */
				/************************************************************************/
				if(apParameter[0x23] & 0x80){
					tMBCSet = apParameter[0x82] > apParameter[0x83] ? apParameter[0x82]:apParameter[0x83] ;
				}
				unsigned char mbc[4096] = {0xff};
				memset(mbc, 0xff, 4096);
				if (ST_Set_Get_BadColumn(obType, DeviceHandle, targetDisk, false, 1 , mbc, tMBCSet)) {
					return 1;
				}
				if(mbcBuffer != NULL)
					memcpy(mbcBuffer,mbc,4096);
				*currentDie = die;
			}
		}
	} else {
		if (*currentDie != die) {
			unsigned char mbc[4096];
			int tMBCSet = MBCSet;
			/************************************************************************/
			/* MBC 混mode                                                           */
			/************************************************************************/
			if(apParameter[0x23] & 0x80){
				tMBCSet = apParameter[0x82] > apParameter[0x83] ? apParameter[0x82]:apParameter[0x83] ;
			}

			if (ST_Set_Get_BadColumn(obType, DeviceHandle, targetDisk, false, 1 , badcolumndata, tMBCSet)) {
				return 1;
			}
			if (ST_Set_Get_BadColumn(obType, DeviceHandle, targetDisk, false, 0 , mbc, tMBCSet)) {
				return 1;
			}
			if(mbcBuffer != NULL)
				memcpy(mbcBuffer,mbc,4096);
			*currentDie = die;
		}
	}
	return 0;
}


int CNTCommand::ST_Set_Flash_All_Bad_Column_Collect(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE DummyReadEnable,DWORD BadColumnBlock , BYTE SinglePlane, BYTE BadColumnCE, BYTE BadColumnPlane, BYTE _32nmPattern)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];

	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x10;
	if(DummyReadEnable)
		cdb[3] |= 0x01;
	if(SinglePlane)
		cdb[3] |= 0x02;
	if(_32nmPattern)
		cdb[3] |= 0x04;

	cdb[4] = BadColumnCE;
	cdb[5] = BadColumnBlock &0xff;
	cdb[6] = (BadColumnBlock >> 8) & 0xff;
	cdb[7] = BadColumnPlane;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	return iResult;

}


int CNTCommand::ST_Set_DummyRead(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BadColumnCE, DWORD BadColumnBlock , BYTE BadColumnPlane)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];

	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x11;

	cdb[4] = BadColumnCE;
	cdb[5] = BadColumnBlock &0xff;
	cdb[6] = (BadColumnBlock >> 8) & 0xff;
	cdb[7] = BadColumnPlane;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	return iResult;
}


int CNTCommand::ST_BadColumn_Read(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BC_CE, DWORD BC_Block , DWORD BC_Page, int len, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];

	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x12;

	cdb[4] = BC_CE;
	cdb[5] = BC_Block &0xff;
	cdb[6] = (BC_Block >> 8) & 0xff;
	cdb[7] = BC_Page &0xff;
	cdb[8] = (BC_Page >> 8) & 0xff;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	return iResult;
}

int CNTCommand::ST_Set_Get_BadColumn(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE SinglePlane, BYTE  bGetSet, unsigned char *buf,int bSet128B)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	int len=4096;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xab;
	if(bSet128B==128){
		cdb[4] = 1;
	}
	if(bSet128B==256){
		cdb[4] = 2;
	}
	if(bSet128B==512){
		cdb[4] = 4;
	}
	if(SinglePlane)
	{
		cdb[3] |= 0x01;
		len=1024;
	}
	if(bGetSet)//0=Get , 1=Set
		cdb[3] |= 0x02;


	for(int i=0;i<2;i++)
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,bGetSet==1?SCSI_IOCTL_DATA_OUT:SCSI_IOCTL_DATA_IN, len, buf, 5);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,bGetSet==1?SCSI_IOCTL_DATA_OUT:SCSI_IOCTL_DATA_IN, len, buf, 5);
		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(3000);
	}
	return iResult;


}

int CNTCommand::ST_SetPatternTable(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int datalen ,unsigned char *w_buf)
{
	int iResult = 0;
	unsigned char cdb[16];
	if (m_isSQL)
		return 0;
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xAA;
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	for(int i=0;i<2;i++){
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, 5);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, 5);

		if(ST_CheckResult(iResult))
		{
			break;
		}
		Sleep(3000);
	}
	return iResult;
}

int CNTCommand::ST_SetGet_Parameter(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bSetGet,unsigned char *buf)
{
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xBB;
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	_realCommand = 1;
	for(int i=0;i<2;i++)
	{
		if(bSetGet)
		{
			if(obType ==0)
				iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 5);
			else
				iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 5);
		}

		else
		{
			cdb[3] = 1;
			if(obType ==0)
				iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 1024, buf, 5);
			else
				iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 1024, buf, 5);
		}
		if(ST_CheckResult(iResult))
		{
			break;
		}

		Sleep(10);
		OutputDebugString("ST_SetGet_Parameter delay \r\n");
	}
	memcpy(m_apParameter, buf, 1024);
	if((GetSQLCmd()!=NULL)&&(bSetGet))
	{	
		ST_SetGet_Parameter_SQL();
	}

	//Sleep(15);
	return iResult;
}

int CNTCommand::ST_SetQuickFindIBBuf(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xCC;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 1024, buf, 5);

	return iResult;
}

int CNTCommand::ST_QuickFindIB(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int ce, unsigned int dieoffset, unsigned int BlockCount, unsigned char *buf, int buflen,
							   int iTimeCount, BYTE bIBSearchPage, BYTE CheckPHSN)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	memset(buf, 0, buflen);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xCD;
	cdb[3] = ce;
	cdb[4] = dieoffset;
	cdb[5] = dieoffset >> 8;
	cdb[6] = BlockCount;
	cdb[7] = BlockCount >> 8;
	cdb[8] = iTimeCount;
	cdb[9] = iTimeCount >> 8;
	cdb[10] = bIBSearchPage;
	cdb[11] = 1;	//1時, 同時找IB、CK, 先找到IB就回IB 位置, 先找到CK就回CK位置, Cdb[11] = 0時, 只找IB
	cdb[12] = CheckPHSN ? 0 : 1;	// 如果 conversion 不同, 底層沒辦法 check PHSN


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, (iTimeCount / 100) + 2);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, (iTimeCount  / 100) + 2);

	return iResult;
}

int	CNTCommand::ST_T2LAndL2T(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,BYTE Type, BYTE DieCount, BYTE bCE)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];

	//if( buf==NULL )	return 1;

	//memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));

	CDB[0] = 0x6;
	CDB[1] = 0xF6;
	CDB[2] = 0xAD;
	//CDB[3] = 0x08;		//CE select, 0: CE0; 1:CE1……. 8: All CE
	CDB[4] = DieCount;	//Die configure: 1: 1die, 2: 2die, 4: 4die
	CDB[5] = Type;		//0: Legacy, 1: Toggle
	CDB[6] = 0x01;		//1: flash reset. 

	for(int i=0;i<bCE;i++)
	{
		CDB[3] = i;
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 20);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 20);
	}

	return iResult;
}

int	CNTCommand::ST_ModifyFlashPara(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE CeStart, BYTE CeEnd)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb, 0, sizeof(cdb));

	cdb[0] = 0x6;
	cdb[1] = 0xF6;
	cdb[2] = 0xAD;

	for (int i = CeStart; i < CeEnd; i++)
	{
		cdb[3] = i;
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 20);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 20);
	}

	return iResult;
}

int CNTCommand::ST_CheckModifyFlashPara(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *r_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x49;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 8, r_buf, 5);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 8, r_buf, 5);
	return iResult;
}


int CNTCommand::NetStartUp(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	int iResult = 0;
	return iResult;
}

int CNTCommand::NetCheckBin(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BinValue)
{
	return 0;
}

int CNTCommand::NetSendCheckIB(BYTE obType,unsigned char *ib)
{
	return 0;
}

int CNTCommand::NetStopAuto( char drive )
{
	return 0;
}

int CNTCommand::NetGetBin1Value(DWORD *flashCapacity,int flhTotalCapacity ,char *paramfile)
{
	return 0;
}

int CNTCommand::NetSetStatus(BYTE obType,DWORD status)
{
	int iResult = 0;
	return iResult;
}

int CNTCommand::ST_FWInit(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0]   = 0x06;
	cdb[1]   = 0xF6;
	cdb[2]	 = 0x41;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);
	return iResult;
}

int CNTCommand::ST_SetGetPara(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE CE, BYTE Die, BYTE bSetGet, unsigned short addr, unsigned short wdata, unsigned char *w_buf)
{
	int iResult = 0;
	unsigned char cdb[16];
	memset(w_buf, 0, 512);

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x44;
	cdb[3] = GetCeMapping(CE);
	cdb[4] = Die;
	cdb[5] = bSetGet;	//0: Get, 1:Set
	if(bSetGet==1){
		cdb[6] = addr >>8;
		cdb[7] = addr & 0xff;
		cdb[8] = wdata & 0xff;
	}
	nt_log_cdb(cdb,"%s",__FUNCTION__);

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);	
	return iResult;

}

int CNTCommand::ST_SetSD_D1(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE CE)
{
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0]   = 0x06;
	cdb[1]   = 0xF6;
	cdb[2]	 = 0x45;
	cdb[3]	 = CE;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);
	return iResult;

}

int CNTCommand::ST_DisableIPR(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE CE, BYTE Die)
{
	int iResult = 0;
	unsigned char cdb[16];
	nt_log("%s",__FUNCTION__);
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xa3;
	cdb[3] = GetCeMapping(CE);
	cdb[4] = Die;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);
	return iResult;
}

// 06 f6 b1
int CNTCommand::ST_2306_SetGet_Conversion(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bSetGet, unsigned char *settingBuf, unsigned char *buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	//int len = SectorCount << 9;
	//	memset(r_buf,0,(Sector*512));
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xB1;
	cdb[3] = bSetGet;	// 0:Enable conversion seed
	// 1:Get conversion seed
	// 2:Disable conversion seed
	if(settingBuf==NULL){
		cdb[4] = 3; // FLASH_REG[CONV_ADDR0];
		cdb[5] = 4; // FLASH_REG[CONV_ADDR1];  只有 bit0 有效, 填 4 讀回來會是 0
		cdb[6] = 0; // FLASH_REG[CONV_SEED0];
		cdb[7] = 1; // FLASH_REG[CONV_SEED1];
		cdb[8] = 2; // FLASH_REG[CONV_SEED2];
	}
	else{
		cdb[4] = settingBuf[0]; // FLASH_REG[CONV_ADDR0];
		cdb[5] = settingBuf[1]; // FLASH_REG[CONV_ADDR1];  只有 bit0 有效, 填 4 讀回來會是 0
		cdb[6] = settingBuf[2]; // FLASH_REG[CONV_SEED0];
		cdb[7] = settingBuf[3]; // FLASH_REG[CONV_SEED1];
		cdb[8] = settingBuf[4]; // FLASH_REG[CONV_SEED2];
	}


	if(bSetGet == 1)
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 5);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 5);
	}
	else{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 5);
	}
	return iResult;
}

int CNTCommand::ST_OpenExtentByteWrite(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE addrHigh, BYTE addrLow, BYTE _1zMLC, BYTE wdata){
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x42;
	cdb[3] = 0x01;		// 1:write   0:read
	cdb[4] = bCE;
	cdb[5] = 0x00;		// no use
	cdb[6] = addrHigh;	// address High byte
	cdb[7] = addrLow;	// address Low byte
	cdb[8] = wdata;		// data
	cdb[9] = _1zMLC;	// 1: 1z MLC    0:others

	// 	if(obType ==0)
	// 		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 30);
	// 	else
	// 		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 30);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, _SCSI_DelayTime);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, _SCSI_DelayTime);

	return iResult;
}

int CNTCommand::ST_OpenExtentByteRead(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE addrHigh, BYTE addrLow, BYTE _1zMLC, int datalen, unsigned char *w_buf){
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x42;
	cdb[3] = 0x00;		// 1:write   0:read
	cdb[4] = bCE;
	cdb[5] = 0x00;		// no use
	cdb[6] = addrHigh;	// address High byte
	cdb[7] = addrLow;	// address Low byte
	cdb[8] = 0x00;		// data
	cdb[9] = _1zMLC;	// 1: 1z MLC    0:others

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, datalen, w_buf, _SCSI_DelayTime);

	return iResult;
}

int CNTCommand::ST_CmdWriteFlashFuse(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE bED3, unsigned char* buf, int len )
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0xAB;
	cdb[3] = bCE;
	cdb[6] = bED3;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, buf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, buf, 30);

	return iResult;
}

int CNTCommand::ST_CmdGetFlashFuse(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE bED3, int block, int page, unsigned char* buf, int len )
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0xAC;
	cdb[3] = bCE;
	cdb[4] = block;
	cdb[5] = page;
	cdb[6] = bED3;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, buf, _SCSI_DelayTime);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, buf, _SCSI_DelayTime);

	return iResult;
}

int CNTCommand::CmdWriteFlashFuseAD(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0xAD;
	cdb[3] = bCE;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);

	return iResult;
}


int CNTCommand::ST_CmdFDReset(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	if(!_doFDReset) return 0;
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0]   = 0x06;
	cdb[1]   = 0xF6;
	cdb[2]	 = 0x09;
	cdb[3]	 = 0x00;
	cdb[4]	 = 0x00;
	cdb[5]	 = 0x01;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 30);
	return iResult;
}

int CNTCommand::ST_CmdWriteFlashFuse_GWK(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, unsigned int uiBlk, unsigned int uiPage, BYTE bED3, unsigned char* buf, int len )
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0xAB;
	cdb[3] = bCE;
	cdb[4] = (uiPage&0xff);
	cdb[5] = (uiBlk&0xff);
	cdb[6] = 1;    // FD Reset

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, buf, 30);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, buf, 30);

	return iResult;
}

int CNTCommand::ST_CmdGetFlashFuse_GWK(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, BYTE bED3, int block, int page, unsigned char* buf, int len )
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0x42;
	cdb[3] = bCE;
	cdb[4] = block;
	cdb[5] = page;
	cdb[6] = bED3;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, buf, _SCSI_DelayTime);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, buf, _SCSI_DelayTime);

	return iResult;
}

int CNTCommand::ST_CmdGetFlashFuse2_GWK(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, int block, int page, unsigned char* buf, int len )
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0x41;
	cdb[3] = bCE;
	cdb[4] = page;
	cdb[5] = block;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, buf, _SCSI_DelayTime);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, buf, _SCSI_DelayTime);

	return iResult;
}

int CNTCommand::ST_FlashGetID_GWK(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* buf, int len ){
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0x01;
	cdb[3] = 0x01;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, buf, _SCSI_DelayTime);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, len, buf, _SCSI_DelayTime);

	return iResult;
}

int CNTCommand::ST_GetSRAM_GWK(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char* w_buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(w_buf, 0, 512);

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0x05;
	cdb[2] = 0x52;
	cdb[2] = 0x41;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);	
	return iResult;

}

// network support functions.
BOOL CNTCommand::NetDeviceIoControl(
									HANDLE hDevice,              // handle to device of interest
									DWORD dwIoControlCode,       // control code of operation to perform
									LPVOID lpInBuffer,           // pointer to buffer to supply input data
									DWORD nInBufferSize,         // size, in bytes, of input buffer
									LPVOID lpOutBuffer,          // pointer to buffer to receive output data
									DWORD nOutBufferSize,        // size, in bytes, of output buffer
									LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
									LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
									)
{
	return DeviceIoControl( hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped );;
}

int CNTCommand::NetCloseHandle( HANDLE *devHandle )
{
	return MyCloseHandle( devHandle );
}

HANDLE CNTCommand::NetCreateFile( char drive )
{
	return MyCreateFile( drive );
}

DWORD CNTCommand::NetGetLastError()
{
	return GetLastError();
}

DWORD CNTCommand::NetGetLogicalDrives()
{
	return GetLogicalDrives();
}

int CNTCommand::NetUnLockVolume( BYTE bType, HANDLE *hDeviceHandle )
{
	return MyUnLockVolume( bType, hDeviceHandle );
}

int CNTCommand::NetLockVolume( BYTE bType, BYTE bDrv, HANDLE *hDeviceHandle )
{
	return MyLockVolume( bType, bDrv, hDeviceHandle );
}

void CNTCommand::NetCloseVolume( HANDLE *hDev )
{
	NetCloseHandle( hDev );
	*hDev = INVALID_HANDLE_VALUE;
}

// return : 0, is floppy type
int CNTCommand::NetIsFloppyType( BYTE bDrv )
{
#if 0
	char sDebugString[128];
	sprintf( sDebugString,"%s [%c]\r\n","****CNTCommand::NetIsFloppyType",bDrv );
	OutputDebugString( sDebugString );
	DWORD ReturnedByteCount;
	DISK_GEOMETRY Geometry;
	char bTemp[32];
	HANDLE hDrive = INVALID_HANDLE_VALUE;

	memset( bTemp, 0, sizeof( bTemp ) );
	sprintf( bTemp, "\\\\.\\%c:", ( char * )bDrv );
	hDrive = CreateFile(
		bTemp,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
		);
	if ( hDrive == INVALID_HANDLE_VALUE )
		return 1;

	if ( DeviceIoControl(
		hDrive,
		IOCTL_DISK_GET_DRIVE_GEOMETRY,
		NULL,
		0,
		&Geometry,
		sizeof( Geometry ),
		&ReturnedByteCount,
		NULL
		)==0 )
	{
		CloseHandle( hDrive );
		return 2;
	}
	CloseHandle( hDrive );

	switch ( Geometry.MediaType )
	{
	case F5_1Pt2_512:	// MediaType = "5.25, 1.2MB,  512 bytes/sector";break;
	case F3_1Pt44_512:	// MediaType = "3.5,  1.44MB, 512 bytes/sector";break;
	case F3_2Pt88_512:	// MediaType = "3.5,  2.88MB, 512 bytes/sector";break;
	case F3_20Pt8_512:	// MediaType = "3.5,  20.8MB, 512 bytes/sector";break;
	case F3_720_512:	// MediaType = "3.5,  720KB,  512 bytes/sector";break;
	case F5_360_512:	// MediaType = "5.25, 360KB,  512 bytes/sector";break;
	case F5_320_512:	// MediaType = "5.25, 320KB,  512 bytes/sector";break;
	case F5_320_1024:	// MediaType = "5.25, 320KB,  1024 bytes/sector";break;
	case F5_180_512:	// MediaType = "5.25, 180KB,  512 bytes/sector";break;
	case F5_160_512:	// MediaType = "5.25, 160KB,  512 bytes/sector";break;
		return 0;
		break;
	case RemovableMedia:// MediaType = "Removable media other than floppy";break;
		return 0xF;

	case FixedMedia:	// MediaType = "Fixed hard disk media";break;
	default:			// MediaType = "Unknown";break;
		return 1;
		break;
	}
#endif
	return 1;
}

int CNTCommand::NetGetFileSystem(char bDrv,char *FormatType)
{
#if 0
	TCHAR RootPathName[MAX_PATH+1];				// root directory
	TCHAR VolumeNameBuffer[MAX_PATH+1];		// volume name buffer
	DWORD nVolumeNameSize;				// length of name buffer
	DWORD nBackupnVolumeNameSize;
	DWORD VolumeSerialNumber;			// volume serial number
	DWORD MaximumComponentLength;		// maximum file name length
	DWORD FileSystemFlags;				// file system options
	TCHAR FileSystemNameBuffer[MAX_PATH+1];	// file system name buffer
	DWORD nFileSystemNameSize;			// length of file system name buffer

	//LPTSTR szRootFormat = TEXT("%c:\\");
	TCHAR szRootFormat[20];
	sprintf(szRootFormat, "%c:\\", bDrv);	

	nVolumeNameSize		= sizeof(VolumeNameBuffer);
	nBackupnVolumeNameSize = nVolumeNameSize;
	nFileSystemNameSize = sizeof(FileSystemNameBuffer);


	if( GetVolumeInformation(RootPathName, VolumeNameBuffer, nVolumeNameSize,
		&VolumeSerialNumber, &MaximumComponentLength, &FileSystemFlags,
		FileSystemNameBuffer, nFileSystemNameSize)==false)
	{
		//OutReport(IDMS_ERR_GetVolume);
	}
	memcpy(FormatType,FileSystemNameBuffer,strlen(FileSystemNameBuffer));
#endif
	return 0;
}

int CNTCommand::NetGetDriveType(char bDrv)
{
	char szRootName[128];
	sprintf(szRootName, "%c:\\", bDrv);
	return GetDriveTypeA(szRootName);	
}

int CNTCommand::NetTestCommnad(BYTE TcpCommand,unsigned short *usFlashid)
{
	return 0;
}

int CNTCommand::ST_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk , unsigned char *id, bool bGetUID,BYTE full_ce_info,BYTE get_ori_id)
{
	nt_log("%s",__FUNCTION__);
	BOOL status = 0;
	ULONG length = 0 ,  returned = 0;
	unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p=NULL; 
	int count = 0;			

	if( id==NULL ) return 1;
	if(bType == 0)
		DeviceHandle_p = NetCreateFile(targetDisk);
	if(bType == 1)
		DeviceHandle_p = MyDeviceHandle;

	if (DeviceHandle_p == INVALID_HANDLE_VALUE && bSCSIControl)
		return NetGetLastError();

OnceAgain:	
	memset( id, 0, 512);
	memset( CDB , 0 , sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x56;
	CDB[2] = full_ce_info;
	CDB[3] = get_ori_id; //sorting code下 get 原始ce mapping id
	if (bGetUID)
	{
		CDB[4] = 0x5A;
	}


	SCSIPtdBuilder(&sptdwb, 12, CDB, SCSI_IOCTL_DATA_IN, 512, 10, id);	
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = NetDeviceIoControl(DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);

	DWORD err = 0;
	if ( status==FALSE ) err = NetGetLastError();

	if (sptdwb.sptd.ScsiStatus && 5 > count)
	{
		count++;
		Sleep(100);
		goto OnceAgain;
	}

	//err=GetLastError();
	if(bType == 0)
		NetCloseHandle(&DeviceHandle_p);

	if (status)
	{
		return -sptdwb.sptd.ScsiStatus;
	} else
		return err;
}

int CNTCommand::ST_CustomCmd_Path(unsigned char* bDrvPath, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut)
{
	BOOL status  = 0;
	ULONG length = 0, returned = 0;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p = NULL; 

	if (0 > bCDBLen || 16 < bCDBLen)	return 1;
	if( CDB==NULL )						return 1;
	if( (iDataLen!=0)&&(buf==NULL) )	return 1;
	if( iDataLen>SCSI_MAX_TRANS )		return 1;

	unsigned char *bankBuf = NULL;

	char ipath[1024];
	memset(ipath,0x00,1024);
	sprintf(ipath,"%s",bDrvPath);

	//DeviceHandle_p = CreateFile((LPCSTR)ipath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	DeviceHandle_p=ST_CreateHandleByPath(ipath);
	if (DeviceHandle_p == INVALID_HANDLE_VALUE && bSCSIControl) return 2;	//dwErr;


	//Find Command in Bank database
	if(isFWBank && (CDB[0] == 0x06) && (_bankBufTotal != NULL) && (_ForceNoSwitch == 0) )//if _bankBufTotal == NULL -> MP Calling,don't check bank
	{
		int Bank = BankSearchByCmd(CDB);
		if(Bank == -1)	//Error
		{
			assert(0);
			return 1;
		}
		/*if(_debugBankgMsg){
		if(CDB[2]!= 0x53)
		{
		LogBank(Bank);
		unsigned char *bankBuf_cmp = new unsigned char[BankSize*1024];
		memset(bankBuf_cmp,0,BankSize*1024);
		ST_BankBufCheck(1,DeviceHandle_p,NULL,bankBuf_cmp);
		delete[] bankBuf_cmp;

		}
		}*/

		//unsigned char *bankBuf_cmp1 = new unsigned char[BankSize*1024];
		//ST_BankBufCheck(1,DeviceHandle_p,NULL,bankBuf_cmp1);
		//int a=bankBuf_cmp1[4];
		//delete bankBuf_cmp1;
		if(_alwaySyncBank)
			this->ST_BankSynchronize(1,DeviceHandle_p,0);


		if(Bank != currentBank && (Bank!=0x99))//if command not in current bank & not in common,do switch
		{
			//get bank buf
			bankBuf = new unsigned char[BankSize*1024];
			int bankOffset = 512 + BankCommonSize*1024 + Bank*BankSize*1024;//Tag 512,Common 20K
			memset(bankBuf,0,BankSize*1024);
			memcpy(bankBuf,_bankBufTotal+bankOffset,BankSize*1024);
			if(ST_BankSwitch(1,DeviceHandle_p,0,bankBuf)){
				delete[] bankBuf;
				return 1;
			}
			currentBank = Bank;
			delete[] bankBuf;
		}
	}
	_ForceNoSwitch = 0;
	SCSIPtdBuilder(&sptdwb, bCDBLen, CDB, SCSI_IOCTL_DATA_IN, iDataLen, timeOut, buf);

	if (SCSI_IOCTL_DATA_IN==direc)				sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	else if (SCSI_IOCTL_DATA_OUT==direc)		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	else if(SCSI_IOCTL_DATA_UNSPECIFIED==direc)	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	else return 3;	

	if(SCSI_IOCTL_DATA_IN==direc) memset( buf, 0,  iDataLen);

	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = NetDeviceIoControl( DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);
	DWORD dwErr = 0;
	if ( status==FALSE ) dwErr = NetGetLastError();

	NetCloseHandle(&DeviceHandle_p);

	memcpy(m_CMD,CDB,16);
	if (status)
		return -sptdwb.sptd.ScsiStatus;
	else
		return dwErr;		
	return 0;
}

HANDLE CNTCommand::ST_CreateHandleByPath(char *MyDrivePath)
{
	DWORD  accessMode1, accessMode2, accessMode3, accessMode4;
	DWORD  shareMode = 0;
	char   devicename[1024];
	HANDLE DeviceHandle_p = NULL;

	memset( devicename, 0, sizeof(devicename));
	sprintf( devicename , "%s" , MyDrivePath );
	OutputDebugString(devicename);

	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;		// default
	accessMode1 = GENERIC_READ | GENERIC_WRITE;			// default
	accessMode2 = accessMode1 | GENERIC_EXECUTE;		// default
	accessMode3 = accessMode2 | GENERIC_ALL;			// default
	accessMode4 = accessMode3 | MAXIMUM_ALLOWED;		// default

	DeviceHandle_p = CreateFileA( devicename , 
		accessMode1 , 
		shareMode , 
		NULL ,
		OPEN_EXISTING , 
		FILE_FLAG_NO_BUFFERING, //FILE_ATTRIBUTE_NORMAL,		// ? why 
		0 );

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
	{
		DeviceHandle_p = CreateFileA( devicename, accessMode2, shareMode, NULL ,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL );
		OutputDebugString("accessmode2");

	}
	else return DeviceHandle_p;

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
	{
		DeviceHandle_p = CreateFileA( devicename, accessMode3, shareMode, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
		OutputDebugString("accessmode3");

	}
	else
		return DeviceHandle_p;

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
	{
		DeviceHandle_p = CreateFileA( devicename, accessMode4, shareMode, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
		OutputDebugString("accessmode4");
	}
	else
		return DeviceHandle_p;

	if ( INVALID_HANDLE_VALUE == DeviceHandle_p )
		return INVALID_HANDLE_VALUE;
	else
		return DeviceHandle_p;
}

int CNTCommand::ST_GetE2NANDFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle,char targetDisk , unsigned char *id, bool bGetUID,BYTE full_ce_info,BYTE get_ori_id)
{
	nt_log("%s",__FUNCTION__);
	BOOL status = 0;
	ULONG length = 0 ,  returned = 0;
	unsigned char CDB[12];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	HANDLE DeviceHandle_p=NULL; 
	int count = 0;			

	if( id==NULL ) return 1;
	if(bType == 0)
		DeviceHandle_p = NetCreateFile(targetDisk);
	if(bType == 1)
		DeviceHandle_p = MyDeviceHandle;

	if (DeviceHandle_p == INVALID_HANDLE_VALUE && bSCSIControl)
		return NetGetLastError();

OnceAgain:	
	memset( id, 0, 512);
	memset( CDB , 0 , sizeof(CDB));

	CDB[0] = 0x06;
	CDB[1] = 0x56;
	CDB[2] = full_ce_info;
	CDB[3] = get_ori_id; //sorting code下 get 原始ce mapping id
	if (bGetUID)
	{
		CDB[4] = 0x5A;
	}
	CDB[5] = 0x30;

	SCSIPtdBuilder(&sptdwb, 12, CDB, SCSI_IOCTL_DATA_IN, 512, 10, id);	
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = NetDeviceIoControl(DeviceHandle_p,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);

	DWORD err = 0;
	if ( status==FALSE ) err = NetGetLastError();

	if (sptdwb.sptd.ScsiStatus && 5 > count)
	{
		count++;
		Sleep(100);
		goto OnceAgain;
	}

	//err=GetLastError();
	if(bType == 0)
		NetCloseHandle(&DeviceHandle_p);

	if (status)
	{
		return -sptdwb.sptd.ScsiStatus;
	} else
		return err;
}

int CNTCommand::ST_FAddrGet(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,int CE,int Die,int addr,unsigned char * data )
{
	nt_log("%s",__FUNCTION__);
	UCHAR bCBD[16];
	unsigned long Length = 8;
	ZeroMemory(bCBD,16);
	bCBD[0] = 0x06;  
	bCBD[1] = 0xf6;  
	bCBD[2] = 0x21;
	bCBD[3] = CE;
	bCBD[4] = Die;
	bCBD[5] = addr;

	int iResult=0;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, Length, data, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, Length, data, 10);
	} else return 0xFF;

	return iResult;
};

int CNTCommand::ST_FAddrSet(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,int CE,int Die,int addr,int data )
{
	nt_log("%s",__FUNCTION__);
	UCHAR bCBD[16];

	ZeroMemory(bCBD,16);
	bCBD[0] = 0x06;  
	bCBD[1] = 0xf6;  
	bCBD[2] = 0x20;
	bCBD[3] = CE;
	bCBD[4] = Die;
	bCBD[5] = addr;
	bCBD[6] = data;


	int iResult=0;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);
	} else return 0xFF;

	return iResult;
};

/*for USB FW USE get VT*/
int CNTCommand::NT_Send_Dist_read(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,int ce, WORD wBlock,WORD WL,WORD VTH,BYTE Die,PUCHAR pucBuffer,UCHAR pagesize)
{
	nt_log("%s",__FUNCTION__);
	UCHAR bCBD[16];
	//BOOL status;
	unsigned long Length;

	ZeroMemory(bCBD,16);
#if 0
	if(wBlock > 2083) {
		wBlock -= 2084;
		ucCDB[4] = 0x01;
	} else {
		ucCDB[4] = 0x00;
	}
#else
	bCBD[4] = 0x00;

#endif


	bCBD[0] = 0x06;  
	bCBD[1] = 0x9E;  
	bCBD[2] = 0x00;
	bCBD[3] = ce;
	bCBD[4] = Die;
	bCBD[5] = (wBlock >> 8);
	bCBD[6] = (UCHAR)wBlock;
	bCBD[7] =	(WL >> 8); // D1 mode enable.
	bCBD[8] =	(UCHAR)WL;
	bCBD[9] =	(VTH >> 8); 
	bCBD[10] =	(UCHAR)VTH;

	Length = pagesize*1024 ;


	int iResult=0;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, Length, pucBuffer, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, Length, pucBuffer, 10);
	} else return 0xFF;

	return iResult;

}

/*for ST FW USE get VT*/
int CNTCommand::ST_Send_Dist_read(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,int ce, WORD wBlock,WORD WL,WORD VTH,PUCHAR pucBuffer,UCHAR pagesize,int is3D,bool isSLC,unsigned char ShiftReadLevel)
{
	nt_log("%s",__FUNCTION__);
	UCHAR bCBD[16];
	//BOOL status;
	unsigned long Length;
	if(m_isSQL||_fh->bIsMicronB16)
	{
		return ST_Send_Dist_read_SQL( ce, wBlock, WL, VTH, pucBuffer, pagesize, is3D, isSLC, ShiftReadLevel);
	}

	ZeroMemory(bCBD,16);
#if 0
	if(wBlock > 2083) {
		wBlock -= 2084;
		ucCDB[4] = 0x01;
	} else {
		ucCDB[4] = 0x00;
	}
#else
	bCBD[4] = 0x00;

#endif

	/*bCBD[0] = 0x06;  
	bCBD[1] = 0x9E;  
	bCBD[2] = 0x00;*/
	bCBD[0] = 0x06;  
	bCBD[1] = 0xf6;  
	bCBD[2] = 0x46;
	bCBD[3] = GetCeMapping(ce);
	bCBD[4] = (wBlock >> 8);
	bCBD[5] = (UCHAR)wBlock;
	bCBD[6] =	(WL >> 8); // D1 mode enable.
	bCBD[7] =	(UCHAR)WL;
	bCBD[8] =	(VTH >> 8); 
	bCBD[9] =	(UCHAR)VTH;
	if(is3D)
		bCBD[10] = 1;

	Length = pagesize*1024 ;

	int iResult=0;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, Length, pucBuffer, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, Length, pucBuffer, 10);
	} else return 0xFF;

	return iResult;
}

int CNTCommand::ST_Send_Dist_readST(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum,int ce, WORD wBlock,WORD WL,WORD VTH,PUCHAR pucBuffer,UCHAR pagesize)
{
	nt_log("%s",__FUNCTION__);
	UCHAR bCBD[16];
	//BOOL status;
	unsigned long Length;

	ZeroMemory(bCBD,16);
#if 0
	if(wBlock > 2083) {
		wBlock -= 2084;
		ucCDB[4] = 0x01;
	} else {
		ucCDB[4] = 0x00;
	}
#else
	bCBD[4] = 0x00;

#endif

	bCBD[0] = 0x06;  
	bCBD[1] = 0xf6;  
	bCBD[2] = 0x13;
	bCBD[3] = ce;

	bCBD[5] = (wBlock >> 8);
	bCBD[6] = (UCHAR)wBlock;
	bCBD[7] =	(WL >> 8); // D1 mode enable.
	bCBD[8] =	(UCHAR)WL;
	bCBD[9] =	(VTH >> 8); 
	bCBD[10] =	(UCHAR)VTH;

	Length = pagesize*1024 ;

	int iResult=0;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_IN, Length, pucBuffer, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_IN, Length, pucBuffer, 10);
	} else return 0xFF;

	return iResult;
}

int	CNTCommand::NT_Toggle2LagcyReadId(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char*buf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[12];
	//if( buf==NULL )	return 1;
	//memset( buf, 0, 528);
	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x6;
	CDB[1] = 0xf6;
	CDB[2] = 0x01;
	CDB[3] = 0x01;
	//	int iResult = NT_CustomCmd(bDrv, cmd, 12 ,SCSI_IOCTL_DATA_OUT m_Size, NULL, 10);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, CDB, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 12 ,SCSI_IOCTL_DATA_IN, 64, buf, 10);

	return iResult;
}

int CNTCommand::ST_SetFlashParameter(int DelayTime)
{

	_SCSI_DelayTime = DelayTime & 0xffff;//
	return 0;
}

int CNTCommand::ST_SetGlobalSCSITimeout(int DelayTime)
{

	_SCSI_DelayTime = DelayTime & 0xffff;//
	_timeOut = _SCSI_DelayTime;
	return 0;
}

int CNTCommand::ST_FAddrReset(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum )
{
	nt_log("%s",__FUNCTION__);
	UCHAR bCBD[16];

	ZeroMemory(bCBD,16);
	bCBD[0] = 0x06;  
	bCBD[1] = 0xf6;  
	bCBD[2] = 0x22;
	int iResult=0;
	if( bType==0 )
	{
		iResult = NT_CustomCmd(bDrvNum, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);
	} else if( bType==1 )	{
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, bCBD, 12 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 10);
	} else return 0xFF;

	return iResult;
};

int CNTCommand::ST_GetUnitID(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce, BYTE m_val)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x43;
	cdb[3] = GetCeMapping(c_ce);
	cdb[4] = m_val;


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 1024, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 1024, w_buf, 15);	
	return iResult;

}

int CNTCommand::ST_GetTSBIdentificationTable(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(w_buf, 0, 512);

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xa8;
	cdb[4] = GetCeMapping(c_ce);

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);	
	return iResult;
}

//20190604 : For BicS4 Flash
int CNTCommand::ST_GetTSBRevisionCode(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(w_buf, 0, 512);

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xa2;
	cdb[4] = GetCeMapping(c_ce);


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 4096, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 4096, w_buf, 15);	
	return iResult;
}

int CNTCommand::ST_GetMicronUnitID(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xA5;
	cdb[3] = GetCeMapping(c_ce);

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 2048, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 2048, w_buf, 15);	
	return iResult;

}

int CNTCommand::ST_GetMicroParePage(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char *w_buf, BYTE c_ce)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(w_buf, 0, 9216);

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xa6;
	cdb[4] = c_ce;
	cdb[5] = 18;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 9216, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 9216, w_buf, 15);	
	return iResult;

}

int CNTCommand::ST_ScanBadBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk ,int CE, unsigned char *w_buf,BOOL isCheckSpare00, unsigned int DieOffSet, unsigned int BlockPerDie)
{

	int iResult = 0;
	unsigned char cdb[16];
	memset(w_buf, 0, 4096);

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x14;
	cdb[3] = (unsigned char)CE;
	if(isCheckSpare00)//Default: spare!=0xff; 1:spare==0x00
		cdb[4] |= 0x01;

	cdb[5] = (unsigned char)((DieOffSet >> 8) & 0xff);
	cdb[6] = (unsigned char)(DieOffSet & 0xff);
	cdb[7] = (unsigned char)((BlockPerDie >> 8) & 0xff);
	cdb[8] = (unsigned char)(BlockPerDie & 0xff);

	nt_log_cdb(cdb,"%s",__FUNCTION__);
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 4096, w_buf, 60);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 4096, w_buf, 60);	
	return iResult;
}

// 06 f6 a7
int CNTCommand::ST_SetSeed(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, int Seed)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xa7;
	cdb[3] = Seed>>8;
	cdb[4] = (Seed&0xff);

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 50);

	return iResult;
}
// 06 f6 80 set toggle 1.0/2.0
int CNTCommand::ST_SetToggle1020(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE toggleMode, unsigned char vcc, unsigned char* buf){
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0x80;

	if(toggleMode==10)		cdb[3]=4;
	else if(toggleMode==20)	cdb[3]=2;

	if(vcc == 18)		cdb[4]=1;
	else if(vcc == 33)	cdb[4]=0;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, _SCSI_DelayTime);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, _SCSI_DelayTime);

	return iResult;

}

// 06 f6 81 scan passwindow
int CNTCommand::ST_ScanPasswindow(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, int  PhyBlock,unsigned char* buf){
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0x81;
	cdb[3] = GetCeMapping(bCE);
	cdb[4] = (PhyBlock &0xff);
	cdb[5] = ((PhyBlock >> 8) & 0xff);


	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, _SCSI_DelayTime);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, _SCSI_DelayTime);

	return iResult;
}

// 06 f6 82 check 3.0 port
int CNTCommand::ST_CheckU3Port(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* buf){
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);

	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0x82;

	// return:
	// 第一個 byte 回傳 1 代表 USB3.0
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, _SCSI_DelayTime);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, _SCSI_DelayTime);

	return iResult;
}

//06 f6 83
int CNTCommand::ST_GetHWMDLL90Degree(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char *w_buf){
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(w_buf, 0, 512);

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x83;

	// return: byte 0 = HW MDLL 90 degree 的 high byte 
	//         byte 1 = HW MDLL 90 degree 的 low byte
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, w_buf, 15);	
	return iResult;
}

//06 f6 84
int CNTCommand::ST_SetWindowMidValue(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned int SDLL){
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x84;
	cdb[3] = SDLL&0xff;			//SDLL low byte
	cdb[4] = ((SDLL>>8)&0xff);	//SDLL high byte

	// return: byte 0 = HW MDLL 90 degree 的 high byte 
	//         byte 1 = HW MDLL 90 degree 的 low byte
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 50);

	return iResult;
}

//06 f6 85
int CNTCommand::ST_SetDegree(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int Phase, short delayOffset){

	int iResult = 0;
	unsigned char cdb[16];

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x85;
	cdb[3] = Phase;
	cdb[4] = ((delayOffset>>8)&0xff);	//high byte
	cdb[5] = (delayOffset&0xff);		//low byte
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	// return: byte 0 = HW MDLL 90 degree 的 high byte 
	//         byte 1 = HW MDLL 90 degree 的 low byte
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 0, NULL, 50);

	return iResult;
}


/*		20170410 Jasper:
*		
*	底層Tag格式			  Command     Bank      Type(Flash)
*	(Defined by James)
*	ex:
*							00         FF        FF   (所有flash的00都在Common Bank)
*							03         00        FF   (所有flash的03都在Bank 0)
*							CF         02        00   (TSB     的0xCF在Bank 2)
*							CF         03        01	  (TSB3D   的0xCF在Bank 3)
*							CF         04        02   (Micron  的0xCF在Bank 4)
*                          CF         05        03   (Micron3D的0xCF在Bank 5)
*                          CF         06        04   (Micron3D-N18B27的0xCF在Bank 6)
*                          CF         07        05   (YMTC的0xCF在Bank 7)
*		Type
*		0xff	代表這個Command 對所有Flash type都用同一個Bank，只會有一筆
*		0x00	For ED3(Toshiba、SanDisk、Hynix、Sumsung) 
*		0x01	Toshiba/SanDisk 3D
*		0x02	Micron B95A	(Micron)
*		0x03	Micron B0KB	(Micron3D)
*
*		**底層的Tag以0xff代表Common bank，由於會與-1撞，不易代表error
*		  因此AP改以0x99代表Common bank	
*/
int CNTCommand::BankInfoCreateFromFile()
{
#if 0
	int key;
	int value;
	memset(mCMD2BankMapping,0x63,0xff+1);
	std::ifstream inFile("Bank.txt",std::ios::in | std::ios::binary);
	char line[10] = {0};
	if(inFile == NULL)
	{
		//ASSERT(0);
		return 1;
	}
	while (!inFile.eof())
	{
		inFile >> std::hex >> key;
		inFile >> value;
		mCMD2BankMapping[key] = value;	
	}
#endif
	/*舊版Command-Bank Mapping*/
	//if(CommandList.size() != 0)	//init過了
	//	return 0;
	//int CommandCounts = (int)_bankBufTotal[0x40];
	//assert(CommandCounts);
	//for(int count = 0; count < CommandCounts; count++)
	//{
	//	int offset = 0x41+count*3;

	//	int command = _bankBufTotal[offset];
	//	int bank = _bankBufTotal[offset+1];
	//	BYTE types = _bankBufTotal[offset+2];

	//	std::vector<BankingCommand>::iterator commandFound;
	//	commandFound = std::find_if(CommandList.begin(),CommandList.end(),find_command(command));
	//	if(commandFound == CommandList.end())	//command not found, new command
	//	{
	//		BankingCommand CommandSet;
	//		memset(CommandSet.BankforTypes,0,10);
	//		CommandSet.Command = command;
	//		if(types == 0xff)	//this command is for all flash
	//		{
	//			for(int j = 0; j < 10; j++)
	//				CommandSet.BankforTypes[j] = bank;
	//		}
	//		else
	//		{
	//			CommandSet.BankforTypes[types] = bank;
	//		}
	//		CommandList.push_back(CommandSet);

	//	}
	//	else	//command already in list
	//	{
	//		(*commandFound).BankforTypes[types] = bank;
	//	}
	//}

	/*		20170410 走舊方法的Command-Bank Mapping		*/
	/*													*/
	/*													*/
	//if(mCMD2BankMapping_NEW == 0){
	//	mCMD2BankMapping_NEW = new BYTE*[0xff+1];
	//	for(int i = 0; i < 0xff+1; ++i)
	//	{
	//		if(mCMD2BankMapping_NEW)
	//		mCMD2BankMapping_NEW[i] = new BYTE[10];		
	//		memset(mCMD2BankMapping_NEW[i],0xff,10);		//預設為-1
	//	}
	//}

	int CommandCounts = (int)_bankBufTotal[0x40];
	assert(CommandCounts);
	for(int count = 0; count < CommandCounts; count++)
	{
		int offset = 0x41+count*3;

		BYTE command = _bankBufTotal[offset];
		if(command == 0xfd)
			int _debug = 1;
		BYTE bank = _bankBufTotal[offset+1];
		if(bank == 0xff)	//common
			bank = 0x99;
		BYTE types = _bankBufTotal[offset+2];

		if(types == 0xff)	//this command is for all flash
		{
			for(int j = 0; j < 10; j++)
				mCMD2BankMapping_NEW[command][j] = bank;
		}
		else
		{
			mCMD2BankMapping_NEW[command][types] = bank;
		}

	}
#ifdef __Debug_Bank__
	char message[100];
	sprintf(message,"\n		Command		TSB		TSB3D	Micron	Micron3D  Micron-N18B27  YMTC\n");
	OutputDebugString(message);
	/*for (int i = 0; i < CommandList.size();i++)
	{
	BankingCommand CommandSet =  CommandList.at(i);
	char message[100];
	sprintf(message,"          0x%02x        %d        %d       %d           %d\n",CommandSet.Command,CommandSet.BankforTypes[TSB],CommandSet.BankforTypes[TSB3D],CommandSet.BankforTypes[Micron],CommandSet.BankforTypes[Micron3D]);
	OutputDebugString(message);
	}*/
	for (int i = 0; i < 0xff+1;i++)
	{
		char message[100];
		if(		(mCMD2BankMapping_NEW[i][TSB] == (BYTE)-1)		//皆-1代表沒有此Command，不用print
			&&	(mCMD2BankMapping_NEW[i][TSB3D] == (BYTE)-1)
			&&	(mCMD2BankMapping_NEW[i][Micron] == (BYTE)-1)
			&&	(mCMD2BankMapping_NEW[i][Micron3D] == (BYTE)-1)
			&&	(mCMD2BankMapping_NEW[i][Micron3D_N18_B27_N28] == (BYTE)-1)
			&&	(mCMD2BankMapping_NEW[i][YMTC] == (BYTE)-1)
			) continue;

		sprintf(message,"          0x%02x		 %2x		 %2x		%2x		%2x		    %2x		%2x\n",
			i,
			mCMD2BankMapping_NEW[i][TSB],
			mCMD2BankMapping_NEW[i][TSB3D],
			mCMD2BankMapping_NEW[i][Micron],
			mCMD2BankMapping_NEW[i][Micron3D],
			mCMD2BankMapping_NEW[i][Micron3D_N18_B27_N28],
			mCMD2BankMapping_NEW[i][YMTC]);
		OutputDebugString(message);
	}
#endif





	return 0;
}


int CNTCommand::BankSearchByCmd(unsigned char *CDB)
{
#if 0
	BYTE cdb2 = CDB[2];
	int bank = mCMD2BankMapping[cdb2];
	if(bank == -1)	//Not found in Database
		return -1;

	switch (cdb2)
	{
	case 0xCF:
		if(_fh == NULL)	//Need check flash info but not set yet,error
			return -1;
		if(_fh->bIsBics2 && _fh->bMaker == 0x98)
			bank = 3;
		if(!_fh->bIsMicron3DTLC && _fh->beD3 && (_fh->bMaker == 0x2c || _fh->bMaker == 0x89))
			bank = 4;
		if ((_fh->bMaker == 0x2c || _fh->bMaker == 0x89) && _fh->bIsMicron3DTLC)
			bank = 5;
		break;
	case 0xEE:
		if(_fh == NULL)	//Need check flash info but not set yet,error
			return -1;
		if(_fh->bIsBics2 && _fh->bMaker == 0x98)
			bank = 3;
		if(!_fh->bIsMicron3DTLC && _fh->beD3 && (_fh->bMaker == 0x2c || _fh->bMaker == 0x89))
			bank = 4;
		if ((_fh->bMaker == 0x2c || _fh->bMaker == 0x89) && _fh->bIsMicron3DTLC)
			bank = 5;
		break;
	case 0xFE:
		if(_fh == NULL)	//Need check flash info but not set yet,error
			return -1;
		if(_fh->bIsBics2 && _fh->bMaker == 0x98)
			bank = 3;
		if(!_fh->bIsMicron3DTLC && _fh->beD3 && (_fh->bMaker == 0x2c || _fh->bMaker == 0x89))
			bank = 4;
		if ((_fh->bMaker == 0x2c || _fh->bMaker == 0x89) && _fh->bIsMicron3DTLC)
			bank = 5;
		break;
	default:
		break;
	}

	if(bank<0)
		return -1;
	return bank;
#endif

	//int command = (int)CDB[2];
	//std::vector<BankingCommand>::iterator commandFound;
	//commandFound = std::find_if(CommandList.begin(),CommandList.end(),find_command(command));
	//if(commandFound == CommandList.end())	//Command not found
	//	return -1;

	//int bank = (*commandFound).BankforTypes[TSB];
	//if(_fh == NULL)
	//	return bank;
	//if(_fh->bIsBics2 && _fh->bMaker == 0x98)
	//	bank = (*commandFound).BankforTypes[TSB3D];
	//if(!_fh->bIsMicron3DTLC && _fh->beD3 && (_fh->bMaker == 0x2c || _fh->bMaker == 0x89))
	//	bank = (*commandFound).BankforTypes[Micron];
	//if ((_fh->bMaker == 0x2c || _fh->bMaker == 0x89) && _fh->bIsMicron3DTLC)
	//	bank = (*commandFound).BankforTypes[Micron3D];

	//return bank;
	BYTE command = CDB[2];
//#ifdef _SHAWN_DEBUG_
	command = CDB[1];
	if(		(command == 0xe6) || (command == 0xe7)
		||	(command == 0xfa) || (command == 0xfb)
		||	(command == 0xfc) || (command == 0xfd)
		||	(command == 0xfe) || (command == 0xff)
		){
			return 8;
	}
//#endif
	command = CDB[2];
	int bank = mCMD2BankMapping_NEW[command][TSB];
	if(_fh == NULL)
		return bank;
	if(_fh->beD3 && _fh->bIs3D && ((_fh->bMaker==0x98) || (_fh->bMaker==0x45) || (_fh->bMaker==0xAd)) )
		bank = mCMD2BankMapping_NEW[command][TSB3D];
	if(!_fh->bIsMicron3DTLC && _fh->beD3 && ((_fh->bMaker == 0x2c) || (_fh->bMaker == 0x89)) )
		bank = mCMD2BankMapping_NEW[command][Micron];
	if ((_fh->bMaker == 0x2c || _fh->bMaker == 0x89) && _fh->bIsMicron3DTLC){
		if(_fh->bIsMIcronN18 || _fh->bIsMicronB27 || _fh->bIsMIcronN28){
			bank = mCMD2BankMapping_NEW[command][Micron3D_N18_B27_N28];
			if(bank == 0xff)
				bank = mCMD2BankMapping_NEW[command][Micron3D];
		}
		else
			bank = mCMD2BankMapping_NEW[command][Micron3D];
	}
	if(_fh->bMaker == 0x9B)
		bank = mCMD2BankMapping_NEW[command][YMTC];

	return bank;

}

int CNTCommand::ST_BankSwitch(BYTE obType,HANDLE DeviceHandle
							  ,BYTE targetDisk, unsigned char *w_buf)
{

	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x54;
	cdb[3] = BankSize;//Bank size
	if (BankSize == 0)
	{
		assert(0);
		return 1;
	}
	nt_log_cdb(cdb,"%s",__FUNCTION__);
#if 0
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 1024*BankSize, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 1024*BankSize, w_buf, 15);
#endif
	HANDLE DeviceHandle_p ;
	if(obType==0){
		DeviceHandle_p = MyCreateFile(targetDisk);
		DWORD dwErr = GetLastError();
		if (DeviceHandle_p == INVALID_HANDLE_VALUE && bSCSIControl) return 2;	//dwErr;
	}
	else{
		DeviceHandle_p = DeviceHandle;
	}
	iResult = NT_CustomCmd_Common(DeviceHandle_p, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 1024*BankSize, w_buf, 15);
	if(obType==0){
		MyCloseHandle(&DeviceHandle_p);
	}

#if 0
#ifdef __Debug_Bank__
	//unsigned char bankBuf_cmp[BankSize*1024] = {0};
	unsigned char *bankBuf_cmp = new unsigned char[BankSize*1024];
	if(ST_BankBufCheck(obType,DeviceHandle,targetDisk,bankBuf_cmp))
		return 1;
	if(memcmp(w_buf,bankBuf_cmp,sizeof(bankBuf_cmp)))
	{
		assert(0);
		return 1;
	}
	delete[] bankBuf_cmp;
#endif
#endif

	return iResult;

}
//Function take a buffer,and assign the bank you set to FW
int CNTCommand::ST_BankBufCheck(BYTE obType,HANDLE DeviceHandle,
								BYTE targetDisk,unsigned char *w_buf)
{
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x53;
	cdb[3] = BankSize;//Bank size
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	if (BankSize == 0)
	{
		assert(0);
		return 1;
	}
#if 0
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, BankSize*1024, w_buf, 15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, BankSize*1024, w_buf, 15);
#endif

	HANDLE DeviceHandle_p ;
	if(obType==0){
		DeviceHandle_p = MyCreateFile(targetDisk);
		DWORD dwErr = GetLastError();
		if (DeviceHandle_p == INVALID_HANDLE_VALUE && bSCSIControl) return 2;	//dwErr;
	}
	else{
		DeviceHandle_p = DeviceHandle;
	}

	iResult = NT_CustomCmd_Common(DeviceHandle_p, cdb, 16 ,SCSI_IOCTL_DATA_IN, BankSize*1024, w_buf, 15);

	if(obType ==0){
		MyCloseHandle(&DeviceHandle_p);
	}
	//OutputDebugString("[ST_BankBufCheck] Bank reply is Bank %u",w_buf[4]);
	char message[100];
	sprintf(message,"Bank replied from FW is %x\n",w_buf[4]);
	//OutputDebugString(message);
	return iResult;


}


//cdb[3]	bit0  舉起NAND flash XNOR
//			Bit1  舉起C state Level indicated
//			Bit2  舉起E state Level indicated
//wWordLine 0~127
int CNTCommand::ST_XNOR_or_Level_Indicated(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,
										   BYTE mode,BYTE ce,WORD wBlock,WORD wWordLine,unsigned char* wBuf)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x90;
	cdb[3] = mode;
	cdb[4] = ce;
	cdb[5] = wBlock & 0xff;
	cdb[6] = wBlock >> 8;
	cdb[7] = wWordLine & 0xff;
	cdb[8] = wWordLine >> 8;


	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk,cdb,16,SCSI_IOCTL_DATA_IN,18432,wBuf,15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,cdb,16,SCSI_IOCTL_DATA_IN,18432,wBuf,15);
	return iResult;
}

/************************************************************************/
/*   Get: 0x01
Set: 0x00                                                            */
/************************************************************************/
int CNTCommand::ST_APGetSetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE SetGet,BYTE Address,BYTE Param0,BYTE Param1,BYTE Param2,BYTE Param3,unsigned char *buf){
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xad;
	cdb[3] = Address;
	cdb[4] = Param0;
	cdb[5] = Param1;
	cdb[6] = Param2;
	cdb[7] = Param3;

	cdb[8] = SetGet;

	if (SSD_NTCMD == m_cmdtype){
		return 0;
	}
	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk,cdb,16,SCSI_IOCTL_DATA_IN,512,buf,15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,cdb,16,SCSI_IOCTL_DATA_IN,512,buf,15);


	return iResult;
}

//06 F6 A1
int CNTCommand::ST_Set_pTLCMode(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk){

	nt_log("%s",__FUNCTION__);
	if(m_isSQL)
	{
		return ST_Set_pTLCMode_SQL();
	}
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xA1;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 50);

	return iResult;
}

void CNTCommand::SetDoFlashReset(BOOL doReset){
	_doFlashReset = doReset;
}
void CNTCommand::SetDoFDReset(BOOL doReset){
	_doFDReset = doReset;
}
void CNTCommand::SetForceID(unsigned char *forceid){
	memcpy(_forceID, forceid, 8);
}
void CNTCommand::InitSQL(void)
{
	//_sqlCmd = new SQLCmd(this, 0, 0, 0);
	_fh = &_FH;
	if (!_sql)
		_sql = SQLCmd::Instance(this, _fh, 2, 0);
	_Init_GMP = 1;
}
int CNTCommand::SetAPKey_NVMe(void){
	return 0;
}
void CNTCommand::LogBank(int targetBank)
{
	char message[100];
	sprintf(message,"Current Bank is %d.\n",currentBank);
	OutputDebugString(message);
	sprintf(message,"Destination Bank is %d.\n",targetBank);
	OutputDebugString(message);
	return;

}
int CNTCommand::ST_BankSynchronize(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk)
{
	//unsigned char bankBufReply[BankSize*1024] = {0};

	unsigned char *bankBufReply = new unsigned char[BankSize*1024];
	if(ST_BankBufCheck(obType,DeviceHandle,targetDisk,bankBufReply))
	{
		delete[] bankBufReply;
		return 1;
	}
	currentBank = bankBufReply[4];
	delete[] bankBufReply;
	return 0;


}

//Power Cycling 製具
DWORD CNTCommand::TPowerResetAll(BYTE obType,HANDLE DeviceHandle,char targetDisk)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];
	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0xCB;
	CDB[1] = 0xF0;
	CDB[2] = 0x60;

	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk,CDB,16,SCSI_IOCTL_DATA_UNSPECIFIED,0,NULL,15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,CDB,16,SCSI_IOCTL_DATA_UNSPECIFIED,0,NULL,15);

	return iResult;
}


DWORD CNTCommand::TPowerOnOff(BYTE obType,HANDLE DeviceHandle,char targetDisk,unsigned char Mode,unsigned char channel,int DelayTime)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];
	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0xCB;
	CDB[1] = 0xF0;
	CDB[2] = 0x63;
	CDB[3] = Mode;
	CDB[4] = channel;
	CDB[5] = (unsigned char) DelayTime;

	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk,CDB,16,SCSI_IOCTL_DATA_OUT,0,NULL,15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,CDB,16,SCSI_IOCTL_DATA_OUT,0,NULL,15);

	return iResult;
}

int CNTCommand::ST_DetectVoltage(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE Voltage)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char CDB[16];
	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0xf6;
	CDB[2] = 0x05;
	CDB[3] = Voltage;

	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk,CDB,16,SCSI_IOCTL_DATA_IN,0,NULL,15);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle,CDB,16,SCSI_IOCTL_DATA_IN,0,NULL,15);
	return iResult;
}

int CNTCommand::ST_2309CheckUECC(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, char Vcc, char ToggleMode)
{
	nt_log("%s",__FUNCTION__);
	int iResult = 0;
	unsigned char cdb[16];

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xC0;
	cdb[3] = Vcc;
	cdb[4] = ToggleMode;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 50);

	return iResult;
}

void CNTCommand::ST_SetCloseTimeOutForPreformat(bool blCloseTimeOut)	// close AP time out 180s
{
	_CloseTimeout = blCloseTimeOut;
}
int  CNTCommand::ST_SetPageSeed(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int pageseed)
{

	int iResult = 0;
	unsigned char cdb[16];

	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x04;
	cdb[3] = pageseed&0xff;	//Low
	cdb[4] = pageseed>>8;	//High
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 0, NULL, 50);

	return iResult;
}

int CNTCommand::SD_TesterGetInfo (BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* r_buf)
{
	int iResult = 0;
	memset ( r_buf , 0 , 512 );
	unsigned char cdb[16];
	memset ( cdb , 0 , 16 );
	cdb[0] = 0x06;
	cdb[1] = 0xF0;
	cdb[2] = 0x74;
	cdb[3] = 0x85;

	//int err = _IssueCmd_byHandle ( cdb,true,r_buf,512,10 );
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, r_buf, 50);

	return iResult;
}

int CNTCommand::SD_TesterForceToBoot(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* r_buf )
{
	int iResult = 0;
	unsigned char cdb[16];
	memset ( cdb , 0 , 16 );
	cdb[0] = 0x06;
	cdb[1] = 0xf0;
	cdb[2] = 0x65;
	cdb[3] = 0x11;
	cdb[4] = 0x00;
	cdb[5] = 0x00;
	cdb[6] = 0x2;
	//int err = _IssueCmd_byHandle ( cdb,false,NULL,0,10 );
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 6, r_buf, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 6, r_buf, 50);

	return iResult;
}

//type = 0, first buf, 1=next buf
int CNTCommand::SD_TesterISPBurner ( BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,unsigned char* buf,int secCnt,int totalSecCnt, int type )
{
	int iResult = 0;
	unsigned char cdb[16];
	memset ( cdb , 0 , 16 );
	cdb[0] = 0x06;
	cdb[1] = 0xF0;
	cdb[2] = 0xC1;
	cdb[3] = 0x01;
	cdb[4] = ( UCHAR ) ( ( totalSecCnt ) >> 8 );
	cdb[5] = ( UCHAR ) ( totalSecCnt );
	cdb[6] = secCnt;
	cdb[7] = (type==0)? 0x00:0x03;

	//int iResult = _IssueCmd_byHandle ( cdb,_CDB_WRITE,buf,512*secCnt,10 );    // Write Mode
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512*secCnt, buf, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512*secCnt, buf, 50);

	return iResult;
}

int CNTCommand::SD_TesterJumpBurner ( BYTE obType, HANDLE DeviceHandle, BYTE targetDisk)
{
	int iResult = 0;
	unsigned char cdb[16];
	memset ( cdb , 0 , 16 );
	cdb[0] = 0x06;
	cdb[1] = 0xF0;
	cdb[2] = 0xC1;
	cdb[3] = 0x02;
	cdb[4] = 0x00;
	cdb[5] = 0x00;
	cdb[6] = 0x00;
	cdb[7] = 0x02;
	//iResult = _IssueCmd_byHandle ( cdb,_CDB_NONE,NULL,0,30 );        // timeout=30 sec
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0,NULL, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_UNSPECIFIED, 0, NULL, 50);

	return iResult;
}

int CNTCommand::SD_TesterGetFlashID ( BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* r_buf )
{
	int iResult = 0;
	unsigned char buf[512];
	memset(buf,0,512);
	buf[0] = 0x8e;  //action key for EMMC Burner
	unsigned char cdb[16];
	memset ( cdb , 0 , 16 );

	cdb[0] = 0x06;
	cdb[1] = 0xF0;
	cdb[2] = 0x79;
	cdb[3] = 0x10;
	//int err = _IssueCmd_byHandle ( cdb,_CDB_WRITE,buf,512,10 );    // Write Mode
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512,buf, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 50);

	if(iResult)return iResult;

	cdb[0] = 0x06;
	cdb[1] = 0xF0;
	cdb[2] = 0x79;
	cdb[3] = 0x12;
	cdb[4] = 0x01;
	//err = _IssueCmd_byHandle ( cdb,true,r_buf,512 );
	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512,r_buf, 50);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN,512, r_buf, 50);

	return iResult;
}



bool CNTCommand::Identify_ATA(char * version){
	return 1;//not implement
}
int CNTCommand::NT_ReadCount1_2307(BYTE bType,HANDLE DeviceHandle_p,char drive, int * count1/*Output*/){
	UCHAR buf[528];
	int iResult = 0;
	memset(buf,0,528);
	unsigned char CDB[16];
	memset(CDB,0,16);
	CDB[0] = 0x06;
	CDB[1] = 0x05;
	CDB[2] = 0x52;
	CDB[3] = 0x41;
	CDB[4] = 0xf4;

	_ForceNoSwitch=1;
	if (count1 == NULL)
		return 1;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, 528, buf, 10);	

	if (iResult == 0) {
		*count1 = buf[0x94] | (buf[0x95]&0x3F)<<8; //14bit of count size
		return 0;
	} else {
		return 1;
	}

}
int CNTCommand::NT_ISP_ReadTarget(BYTE bType,HANDLE DeviceHandle_p,char drive, unsigned char target, unsigned char target1,unsigned char plane,unsigned char page,unsigned char mode,unsigned char eccbit,unsigned char SecCnt,unsigned char rowRead,unsigned char *bBuf)
{
	int iResult = 0;
	unsigned char CDB[16];
	unsigned long bufsize = 0;
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xC1;
	CDB[2] = target;
	CDB[3] = target1;
	CDB[4] = plane;
	CDB[5] = 0;
	CDB[6] = 0;
	CDB[7] = page&0xff;
	CDB[8] = 0x10;
	CDB[9] = mode;
	CDB[10] = eccbit;
	CDB[11] = 1;
	CDB[12] = 1;
	CDB[13] = SecCnt;
	CDB[14] = 0;
	CDB[15] = rowRead;
	bufsize = (SecCnt << 9) +8;
	if(bType == 0)
		iResult = NT_CustomCmd(drive, CDB, 16 ,SCSI_IOCTL_DATA_IN, bufsize, bBuf, 60);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle_p, CDB, 16 ,SCSI_IOCTL_DATA_IN, bufsize, bBuf, 60);	


	return iResult;
}
int CNTCommand::SetThreadStatus(int status){
	_ThreadStatus = status;
	return 0;
}

int CNTCommand::GetThreadStatus(void){
	return _ThreadStatus;
}


//the base temperature is refer from flash spec.
#define BASE_TEMP_BICS3 (-42)
#define BASE_TEMP_B16   (-37)

int CNTCommand::convertTemperature(int fw_value, FH_Paramater * fh)//return 9999 means failed
{
	nt_file_log("%s:BICS3_BASE=%d, B16_BASE=%d, isBics3=%d, isB16%d, buf[0]=%d",
		__FUNCTION__, BASE_TEMP_BICS3, BASE_TEMP_B16, fh->bIsBics3, fh->bIsMicronB16, fw_value);
	int temperature = 9999;

	if (fh->bIsBics3 || fh->bIsBics2)
		temperature = fw_value+BASE_TEMP_BICS3;
	else if (fh->bIsMicronB16)
		temperature = fw_value+BASE_TEMP_B16;
	else
		return temperature;//unknown or not support flash.

	return temperature;

}

int CNTCommand::GetTemperature(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, FH_Paramater * fh) 
{
	int rtn_temperature = 9999;
	int iResult = 0;
	unsigned char CDB[16];
	unsigned char buf[512] = {0};
	CDB[0] = 0x06;
	CDB[1] = 0xF6;
	CDB[2] = 0x45;
	CDB[3] = 0x00;//CE
	CDB[4] = 0x00;//Die



	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, 512, buf, 10);

	if (iResult)
		return rtn_temperature;

	rtn_temperature = convertTemperature(buf[0],fh);
	return rtn_temperature;

}

int CNTCommand::ST_Soft_bit_dump_read(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE RWMode,BYTE SBMode,BYTE bCE
									  ,WORD wBLOCK,WORD Page,BYTE Shiftvalue1,BYTE Shiftvalue2,BYTE Shiftvalue3 ,BYTE Deltavalue1
									  ,BYTE Deltavalue2,WORD DataLen,unsigned char *w_buf)
{
	int iResult = 0;
	unsigned char CDB[16];
	memset( CDB , 0 , sizeof(CDB) );
	CDB[0] = 0x06;
	CDB[1] = 0xf6;
	CDB[2] = 0xFD;
	CDB[3] = RWMode;
	CDB[4] = bCE;
	CDB[5] = wBLOCK & 0xff;;
	CDB[6] = (wBLOCK >> 8)&0xff ;
	CDB[7] = Page&0xff;
	CDB[8] = (Page>>8)&0xff;
	CDB[9] = 0;
	CDB[10] = SBMode;
	CDB[11] = Shiftvalue1;
	CDB[12] = Shiftvalue2;
	CDB[13] = Shiftvalue3;
	CDB[14] = Deltavalue1;
	CDB[15] = Deltavalue2;
	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk, CDB, 16 ,SCSI_IOCTL_DATA_IN, DataLen, w_buf, 60);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, CDB, 16 ,SCSI_IOCTL_DATA_IN, DataLen, w_buf, 60);	

	return iResult;

}

int CNTCommand::ST_Get_Flash_status(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, BYTE bCE, unsigned char *buf)
{
	if (m_isSQL) {
		return ST_DirectGetFlashStatus_SQL(bCE, buf, 0x70);
	}
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	memset(buf,0,4);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x19;
	cdb[4] = bCE;
	nt_log_cdb(cdb,"%s",__FUNCTION__);
	_realCommand = 1;

	if(obType ==0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 4, buf, 60);
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 4, buf, 60);

	return iResult;
}
int CNTCommand::NT_Set_Preformat_BlockRatio(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk,unsigned char CECount,unsigned char DieCount, unsigned char *buf)
{	
	//IB2/IB3 Use
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	memset(buf,0,4);
	cdb[0] = 0x06;
	cdb[1] = 0x25;
	cdb[2] = CECount;
	cdb[3] = DieCount;

	nt_log_cdb(cdb,"%s",__FUNCTION__);
	_ForceNoSwitch=1;

	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 60);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 60);

	return iResult;
}

int CNTCommand::C2012PowerOnOff(BYTE bType,HANDLE MyDeviceHandle,char targetDisk,UINT Mode,UINT Delay,UINT Voltage_SEL,UINT DDR_PWR_Flag,bool m_unit_100ms)
{
	return 0;
}

int CNTCommand::NT_GetD3RRT(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char *buf)
{	
	int iResult = 0;
	unsigned char cdb[16];
	memset(cdb,0,16);
	memset(buf,0,2048);
	cdb[0] = 0x06;
	cdb[1] = 0x52;
	cdb[2] = 0x01;

	if(obType == 0)
		iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 2048, buf, 60);	
	else
		iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 2048, buf, 60);

	return iResult;
}

int	CNTCommand::nps_set_server_info(char *ip, int port){
	return 0;
}
int	CNTCommand::nps_client_update_param(char *command_buf, int commandLen, char * recBuf){
	return 0;
}
int	CNTCommand::ST_Init_SQL_Buffer(unsigned char *buf)
{
	unsigned char cdb[16];
	
	memset(cdb,0,16);
	memset(buf,0,6);
	cdb[0] = 0x06;
	cdb[1] = 0xFA;
	cdb[2] = 0x01;
	
	
    //int ret = _IssueCmd_byHandle ( bCBD,_CDB_READ,buff,sizeof(buff) );
	int ret = NT_CustomCmd_Phy(_hUsb, cdb, 16 ,SCSI_IOCTL_DATA_IN, 6, buf, 60);

	assert(buf[4]!=0);
	return ret;
}
int CNTCommand::SQL_MeasureRDY_TimeGet(unsigned int &RDY_TimeMicrosecond)
{
	int rtn = 0;
	unsigned char readBuf[512] = {0};
	rtn = _ReadDDR(readBuf, 512, 3);
	RDY_TimeMicrosecond =  readBuf[0] | (readBuf[1]<<8) | (readBuf[2]<<16) | (readBuf[3]<<24);
	return rtn;
}
int CNTCommand::SQL_MeasureRDY_TimeReset()
{
	CharMemMgr readBuf(512);
	return _ReadDDR(readBuf.Data(), 512, 4);
}
int CNTCommand::_WriteDDR(UCHAR* w_buf,int Length, int ce)
{
    unsigned char cdb[16];
	memset(cdb,0x00,sizeof(cdb));
	cdb[0] = 0x06;
	cdb[1] = 0xFC;
	cdb[2] = 0;;//_sqlCmd->_dataBufferAddr; // used data mode
    cdb[3] = Length/512;
    int ret = NT_CustomCmd_Phy(_hUsb, cdb, 16 ,SCSI_IOCTL_DATA_OUT, Length, w_buf, 60);

	return ret;
}
int CNTCommand::_TrigerDDR(int CE, const unsigned char* sqlbuf, int slen, int buflen, unsigned char channels )
{
	unsigned char cdb[16];
	int len = ((slen+511)/512)*512;
	unsigned char *tempbuf;

	tempbuf = (unsigned char *)malloc(len);

	memset(cdb,0x00,sizeof(cdb));
	memset(tempbuf,0x00,len);
	
	cdb[0] = 0x06;
	cdb[1] = 0xFE;
	cdb[2] = 0;//_sqlCmd->_dataBufferAddr;	// used data mode
    cdb[3] = len/512;
	cdb[6] = CE;
	memcpy(tempbuf,sqlbuf,slen);
	int ret = NT_CustomCmd_Phy(_hUsb, cdb, 16 ,SCSI_IOCTL_DATA_OUT, len, tempbuf, 60);;
	free(tempbuf);
	return ret;
}
//datatype: 0=data, 1=vender, 2=ecc 3=time, 4=time+reset
int CNTCommand::_ReadDDR(UCHAR* r_buf,int Length, int datatype )
{
	unsigned char cdb[16];
	unsigned char tempbuf[512];
	memset(cdb,0x00,sizeof(cdb));
	memset(tempbuf,0x00,sizeof(tempbuf));
	
	cdb[0] = 0x06;
	cdb[1] = 0xFD;
	/*
	if(datatype==2)
	{
		cdb[2] = _sqlCmd->_eccBufferAddr;
	}
	else
	{
		cdb[2] = _sqlCmd->_dataBufferAddr;
	}
	*/
	cdb[2] = datatype;
	
    cdb[3] = Length/512;
	
	int ret = NT_CustomCmd_Phy(_hUsb, cdb, 16 ,SCSI_IOCTL_DATA_IN, Length, r_buf, 60);;

	return ret;
}


bool CNTCommand::recomposeCDB_base(UCHAR *cdb/*i/o*/, int direct/*i*/){
#if 0
	UCHAR temp[16];
	memset(temp, 0, 16);
	if (SCSI_IOCTL_DATA_UNSPECIFIED == direct) {
		memcpy(temp,sat_vendor_cmd_type[3],3);
	} else if  (SCSI_IOCTL_DATA_OUT == direct) {
		memcpy(temp,sat_vendor_cmd_type[5],3);
	} else if (SCSI_IOCTL_DATA_IN == direct) {
		memcpy(temp,sat_vendor_cmd_type[4],3);
	} else {
		return 1;//unknow direction, just report fail.
	}
	memcpy(temp+3, cdb, 13);
	memcpy(cdb, temp, 16);
#endif
	return 0;
}


int CNTCommand::IssueNVMeCmd(UCHAR *nvmeCmd,UCHAR OPCODE,int direct,UCHAR *buf,ULONG bufLength,ULONG timeOut)
{
	return MyIssueNVMeCmd(I_NVME==GetInterFaceType()?_nvmeHandle:_hUsb,nvmeCmd,OPCODE,direct,buf,bufLength,timeOut, 0, 0);

}

HANDLE CNTCommand::FindFirstNvmeHandle(int * scsi_port/*input / output*/, char * io_produceName/*caller need prepare 40 char*/)
{
	_nvmeHandle = INVALID_HANDLE_VALUE;
	unsigned char ncmd[16];
	memset(ncmd,0x00,16);
	unsigned char buf[4096];
	char produceName[49];
	produceName[48]='\0';
	for (int port=0; port < 16; port++)
	{
		_nvmeHandle = INVALID_HANDLE_VALUE;
		_nvmeHandle = NVMeMyCreateFile(port);
		if(INVALID_HANDLE_VALUE != _nvmeHandle )
		{
			*scsi_port = port;
			int err = MyIssueNVMeCmd(_nvmeHandle, ncmd,ADMIN_IDENTIFY,1,buf,sizeof(ADMIN_IDENTIFY_CONTROLLER),15,0,true);
			//ProgramCmd::MyCloseHandle(&hdl);
			if ( err ) {
				CloseHandle(_nvmeHandle);
				continue;
			}
			memcpy(produceName,&buf[24],40);
			memcpy(io_produceName,produceName,40);
			//nt_file_log("found valid nvme port=%d, produceName=%s, handle=", port, produceName, nvmeHandle);
			return _nvmeHandle;
		}
	}
	return _nvmeHandle;
}

HANDLE CNTCommand::NVMeMyCreateFile( int drive )
{
	DWORD accessMode = 0, shareMode = 0;
	char devicename[32];

	sprintf(devicename , "\\\\.\\Scsi%d:", drive); //Logical Drive
	//sprintf(devicename , "\\\\.\\PhysicalDrive%d", drive); //Logical Drive
	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	accessMode = GENERIC_WRITE | GENERIC_READ;

	HANDLE hUsb = CreateFileA ( devicename,
		accessMode,
		shareMode,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL );

	//int err = GetLastError();
	//assert(err==0);
	//assert(hUsb != INVALID_HANDLE_VALUE);

	return hUsb;

}
int CNTCommand::MyIssueNVMeCmd(HANDLE hUsb, UCHAR *nvmeCmd,UCHAR OPCODE,int direct,UCHAR *buf,ULONG bufLength,ULONG timeOut, int deviceType, bool silence)
{
	return -1;
}
bool CNTCommand::InitDrive(unsigned char drivenum, const char* ispfile, int swithFW  ){
	return 0;
}
int CNTCommand::LogTwo(int n)
{
	int tmp;
	int result;

	if (!n) return 0;

	tmp = n-1;
	result = 0;

	while(tmp)
	{
		tmp>>=1;
		result++;
	}

	return result;
}

int CNTCommand::IsFakeBlock(int blk)
{

	if ( -1 == _fakeRanges[0] || -1 == _fakeRanges[1] || -1 == _fakeRanges[2] || -1 == _fakeRanges[3])
		return false;

	if (blk >= 0 && blk <= 10) {
		return false;
	}

	if (blk >= 256 && blk <= 259) {
		return false;
	}

	if (blk >= _fakeRanges[0] && blk <= _fakeRanges[1]) {
		return false;
	}

	if (blk >= _fakeRanges[2] && blk <= _fakeRanges[3]) {
		return false;
	}

	return true;
}
int CNTCommand::ST_SetGet_RetryTableByFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BOOL isGet, int ce, int die, unsigned char *buf, int length)
{
	unsigned char cdb[16]={0};
	int iResult = 0;
	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0xAC;
	cdb[3] = GetCeMapping(ce);
	cdb[4] = die;

	cdb[5] = isGet;
	cdb[6] = length;
	
	unsigned char Buf512[512] = {0};


	if(isGet)
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, Buf512, 30);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_IN, 512, Buf512, 30);

		memcpy(buf, Buf512, length);
	}
	else
	{
		if(obType ==0)
			iResult = NT_CustomCmd(targetDisk, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 30);
		else
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, 512, buf, 30);
	}
	return iResult;

}


int CNTCommand::ST_SetGetDefaultLevel(bool bSet,unsigned char ce,unsigned char *buf)
{
	if(m_isSQL||_fh->bIsMicronB16)
	{
		assert(0); //james help fix
		//return _sqlCmd->NewSort_SetGet_DefaultLevel(bSet,ce,buf);
	}
	else
	{
		assert(0);
		return 1;
	}
	return 0;
}

int CNTCommand:: ST_SetLDPCIterationCount(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk ,int count)
{
	return 0;
}


int CNTCommand::GetLogicalToPhysicalCE_Mapping(unsigned char * flash_id_buf, int * phsical_ce_array)
{
	return 0;
}


int CNTCommand::GetCeMapping(int input_ce)
{
	if (-1 == _ActiveCe)
	return input_ce;
	return _ActiveCe;
}

void CNTCommand::ForceSQL(bool enable)
{
	m_isSQL = enable;
	if(m_isSQL)
		InitSQL();
}
bool CNTCommand::isForceSQL()
{
	return m_isSQL;
}

HANDLE CNTCommand::Get_SCSI_Handle()
{
	return _hUsb;
}
void CNTCommand::Set_SCSI_Handle(HANDLE h)
{
	_hUsb = h;
}

int CNTCommand::Dev_SetFWSpare_WR(unsigned char Spare, bool isRead)
{
	return 0;
}
int CNTCommand::Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len )
{
    //if ( len>_sramSize ) {
    //    return _Dev_IssueIOCmd9K( sql,sqllen, mode, ce, buf, len );
    //}
    //0=write,1=write+buf,2=read
    int err;
    int hmode = mode&0xF0;
    mode = mode&0x0F;

    if ( !hmode ) {
        assert(sql[sql[1]+3-1]==0xCE);
    }

    if ( (mode&0x03)==1 ) {
		err = _WriteDDR(buf,len,ce);
		/*
        if ( err ) {
            Log (LOGDEBUG, "write _Dev_IssueIOCmd %s ret=%d\n", cdb, err );
            LogMEM ( sql,sqllen );
            LogFlush();
        }
		*/
        assert(err==0);
    }

    //trigger command
    if ( !hmode )  {
		err = _TrigerDDR(ce, sql, sqllen, 512, 0 );
		/*
        if ( err ) {
            Log (LOGDEBUG, "trigger _Dev_IssueIOCmd %s ret=%d\n", cdb, err );
            LogMEM ( sql,sqllen );
            LogFlush();
        }
		*/
        assert(err==0);
        if ( mode==2 ) {
            err = _ReadDDR ( buf, len,_dataBufferAddr );
        }
        else if ( mode==3 ) {
            //read ecc
			unsigned char cmdbuf[512];
            //assert( len>=_FH.PageFrameCnt );
            err = _ReadDDR ( cmdbuf, 512 ,_eccBufferAddr);
			/*
            if ( err ) {
                Log (LOGDEBUG, "get SRAM _Dev_IssueIOCmd %s ret=%d\n", cdb, err );
                LogMEM ( sql,sqllen );
                LogFlush();
            }
			*/
            assert(err==0);
            memcpy(buf,cmdbuf,_FH.PageFrameCnt);
        }
    }
    return err;
}
CNTCommandNet::CNTCommandNet( DeviveInfo* info, const char* logFolder )
:USBSCSICmd_ST(info,logFolder)
{

}
// init and create handle + system log pointer
CNTCommandNet::CNTCommandNet( DeviveInfo* info, FILE *logPtr )
:USBSCSICmd_ST(info,logPtr)
{

}

CNTCommandNet::CNTCommandNet()
:USBSCSICmd_ST(&_default_drive_info,(FILE *)NULL)
{

}
CNTCommandNet::CNTCommandNet(int port)
:USBSCSICmd_ST(&_default_drive_info,(FILE *)NULL)
{

}
CNTCommandNet::~CNTCommandNet()
{

}

//===========================================================================
// For PPS (NES)
// timeout : NES_BASECMD_TIMEOUT
//===========================================================================
int CNTCommand::MyLockVolume_PPS(BYTE bType, BYTE bDrv, HANDLE *hDeviceHandle)  // #wai#MyLockVolume#merge#
{
	OutputDebugString("MyLockVolume ");

	if (bType == 0)
	{
		*hDeviceHandle = MyOpenVolume(bDrv);
	}
	else if (bType == 2) {
		*hDeviceHandle = MyOpenVolume(bDrv);
	}

	if (*hDeviceHandle == INVALID_HANDLE_VALUE) return 2;

	FlushFileBuffers(*hDeviceHandle);
	Sleep(1000);			// <S> #wait#debug

	if (bType == 0)
	{
		if (LockVolume(*hDeviceHandle))
		{
			MyCloseVolume(hDeviceHandle);
			*hDeviceHandle = NULL;
			return 3;
		}
		if (DismountVolume(*hDeviceHandle))
		{
			MyCloseVolume(hDeviceHandle);
			*hDeviceHandle = NULL;
			return 3;
		}
	}

	if (bType == 1)		// <S> #wait#using retry
	{
		OutputDebugString("MyLockVolume::bType == 1\r\n");
		LockVolume(*hDeviceHandle);
		Sleep(1000);		// <S>
		if (LockVolume(*hDeviceHandle) != 5)
		{
			OutputDebugString("LockVolume1 fail != 5");
			Sleep(2000);
			LockVolume(*hDeviceHandle);
			if (LockVolume(*hDeviceHandle) != 5)
			{
				OutputDebugString("LockVolume2 != 5");
				Sleep(2000);
				LockVolume(*hDeviceHandle);
				if (LockVolume(*hDeviceHandle) != 5)
				{
					OutputDebugString("LockVolume3 != 5, FAIL");
					return 0x10;		// ?? <S>
				}
			}
		}
		if (DismountVolume(*hDeviceHandle))
		{
			OutputDebugString("MyLockVolume::DismountVolume, FAIL");
			return 0x11;
		}
	}

	if (bType == 2)
	{
		if (LockVolume(*hDeviceHandle))
		{
			MyCloseVolume(hDeviceHandle);
			*hDeviceHandle = NULL;
			//*hDeviceHandle = MyOpenVolume(bDrv);
		}

	}
	Sleep(10);
	return 0;
}

//---------------------------------------------------------------------------
#if 0
int CNTCommand::NT_SendCommand(BYTE bType, HANDLE DeviceHandel, char targetDisk, unsigned char *bCmd, int bCmdlen, int direc, unsigned int iDataLen, unsigned char *buf, int timeout)
{
	unsigned char CDB[16];
	int TimeOut;

	if (bCmd == NULL)	  return 1;
	if (bCmdlen > 16)	  return 1;
	if (timeout == 0)	  return 1;	// #wait#check#

	memset(CDB, 0, sizeof(CDB));
	memcpy(CDB, bCmd, bCmdlen);

	if (!bType)
		return NT_CustomCmd(targetDisk, CDB, 16, direc, iDataLen, buf, timeout);
	else
		return NT_CustomCmd_Phy(DeviceHandel, CDB, 16, direc, iDataLen, buf, timeout);
	return 0;
}
#endif

int USBSCSICmd_ST::NT_SendCommand(BYTE bType, HANDLE DeviceHandel, char targetDisk, unsigned char *bCmd, int bCmdlen, int direc, unsigned int iDataLen, unsigned char *buf, int timeout)
{
	unsigned char CDB[16];
	int TimeOut;

	if (bCmd == NULL) return 1;
	if (bCmdlen > 16) return 1;

	memset(CDB, 0, sizeof(CDB));
	memcpy(CDB, bCmd, bCmdlen);

	if (timeout == 0)
	{
		TimeOut = NES_BASECMD_TIMEOUT * 3;
	}
	else
	{
		TimeOut = timeout;
	}

	if (!bType)
		return NT_CustomCmd(targetDisk, CDB, 16, direc, iDataLen, buf, TimeOut);
	else
		return NT_CustomCmd_Phy(DeviceHandel, CDB, 16, direc, iDataLen, buf, TimeOut);
	return 0;
}

//---------------------------------------------------------------------------
int CNTCommand::NT_GetFwAutoCheckError(BYTE bType, HANDLE DeviceHandel, char targetDisk, unsigned char *buf, int len)
{
	int iResult = 0;
	unsigned char CDB[16];

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x89;
	CDB[2] = 0x00;
	CDB[3] = 0x00;
	CDB[4] = 0x00;
	CDB[5] = 0x00;
	CDB[6] = 0x00;

	if (!bType)
		return NT_CustomCmd(targetDisk, CDB, 12, SCSI_IOCTL_DATA_IN, len, buf, NES_BASECMD_TIMEOUT);
	else
		return NT_CustomCmd_Phy(DeviceHandel, CDB, 12, SCSI_IOCTL_DATA_IN, len, buf, NES_BASECMD_TIMEOUT);

	return iResult;
}

//---------------------------------------------------------------------------
int CNTCommand::NT_GetFwErrorHandleTag(BYTE bType, HANDLE DeviceHandel, char targetDisk, BYTE Param, unsigned char *buf, int len)
{
	int iResult = 0;
	unsigned char CDB[16];

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;		
	CDB[1] = 0xa1;
	CDB[2] = 0x00;
	CDB[3] = 0x00;
	CDB[4] = 0x00;
	CDB[5] = 0x00;
	CDB[6] = 1;   // Param

	if (!bType)
		return NT_CustomCmd(targetDisk, CDB, 12, SCSI_IOCTL_DATA_IN, len, buf, NES_BASECMD_TIMEOUT);
	else
		return NT_CustomCmd_Phy(DeviceHandel, CDB, 12, SCSI_IOCTL_DATA_IN, len, buf, NES_BASECMD_TIMEOUT);

	return iResult;
}

//---------------------------------------------------------------------------
/*
CDB[0] = 0x06
CDB[1] = 0xC1
CDB[2] = CE  **
CDB[3] = Block (high byte)  **
CDB[4] = Block (low byte)   **
CDB[5] = Page (high byte)  **
CDB[6] = Page (low byte)   **
CDB[7] = Die               **
CDB[8] = SLC: 0x10 / TLC: 0x00   SLC
CDB[9] = 1 (conversion)
CDB[10] = 0 (max ECC)
CDB[11] = 1 (reverse)
CDB[12] = 1 (spare inverse)
CDB[13] = sector count               fix: 2 scan block
CDB[14] = 0 (retry read)
CDB[15] = 1 (read target轉成read block)

只要設定紅色部份
其他照著上面填
我有用4CE8DIE flash稍微驗過
麻煩用這個CMD掃所有block的page0 spare
如果spare 8個byte全為0xF0就是mark bad block
*/
int CNTCommand::NT_ScanMarkBad(BYTE bType, HANDLE DeviceHandel, char targetDisk, BYTE bCE, WORD wBlock, WORD wPage, BYTE bDie, BYTE *bBuf, unsigned int iSize)
{
	int iRT = 0;
	unsigned char CDB[16];
	unsigned int iLen = 1024 + 8;			// 1032, firmware: 16K+16k ( 512 倍數讀上 16K, 不是讀下 16K )

	if (bBuf == NULL) return 0x01;
	if (iSize < iLen) return 0x02;
	memset(bBuf, 0, iSize);

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0xC1;
	CDB[2] = bCE;							//CE  **
	CDB[3] = (BYTE)((wBlock >> 8) & 0xff);		// Block(high byte)  **  
	CDB[4] = (BYTE)wBlock;					// (low byte);  // **
	CDB[5] = (BYTE)((wPage >> 8) & 0xff);	// (high byte)  **
	CDB[6] = (BYTE)wPage;					// (low byte)   **
	CDB[7] = bDie;							// Die               **
	CDB[8] = 0x10;							// SLC: 0x10 / TLC : 0x00   SLC
	CDB[9] = 1;								//(conversion)
	CDB[10] = 0;							//(max ECC)
	CDB[11] = 1;							//(reverse)
	CDB[12] = 1;							//(spare inverse)
	CDB[13] = 2;							// sector count               fix : 2 scan block
	CDB[14] = 0;							//(retry read)
	CDB[15] = 0x80; //(read target轉成read block)   2019 / 12 / 17 FW RD fix

	if (!bType)
		iRT = NT_CustomCmd(targetDisk, CDB, 16, SCSI_IOCTL_DATA_IN, iLen, bBuf, NES_BASECMD_TIMEOUT);
	else
		iRT = NT_CustomCmd_Phy(DeviceHandel, CDB, 16, SCSI_IOCTL_DATA_IN, iLen, bBuf, NES_BASECMD_TIMEOUT);
	if (iRT) return iRT;

	return 0;
}

// channel, CE, blcok (Die*block)
int CNTCommand::NT_ScanMarkBad_FW(BYTE bType, HANDLE DeviceHandel, char targetDisk, BYTE bChannel, BYTE bCE, WORD wTotalBlock, BYTE *bBuf, unsigned int iSize)
{
	int iRT = 0;
	unsigned char CDB[16];
	unsigned int iLen = 512;			// fW fixed : 512

	if (bBuf == NULL) return 0x01;
	if (iSize < iLen) return 0x02;

	memset(bBuf, 0, iSize);
	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x89;
	CDB[2] = 0x02;
	CDB[3] = bChannel;
	CDB[4] = bCE;
	CDB[5] = (BYTE)wTotalBlock & 0xff;				// (low byte); 
	CDB[6] = (BYTE)((wTotalBlock >> 8) & 0xff);		// Block(high byte)  **  				// (low byte)   **

	if (!bType)
		iRT = NT_CustomCmd(targetDisk, CDB, 16, SCSI_IOCTL_DATA_IN, iLen, bBuf, NES_BASECMD_TIMEOUT);
	else
		iRT = NT_CustomCmd_Phy(DeviceHandel, CDB, 16, SCSI_IOCTL_DATA_IN, iLen, bBuf, NES_BASECMD_TIMEOUT);
	if (iRT) return iRT;

	return 0;
}

int SwapH2L_32_UFW(BYTE *bScr, BYTE *bDec)
{
	if (bScr == NULL) return 0x01;
	if (bDec == NULL) return 0x01;

	bDec[0] = bScr[3];
	bDec[1] = bScr[2];
	bDec[2] = bScr[1];
	bDec[3] = bScr[0];
	return 0;
}

//---------------------------------------------------------------------------
// vendor CMD 0x06, 0x89, 0x01, 512
// 16-255 script var, H->L 
// ** depend on fw def, swap
int CNTCommand::NT_GetFlashSizeInfo(BYTE bType, HANDLE DeviceHandel, char targetDisk, FW_FlashSizeInfo *fInfo)
{
	int iRT = 0;
	unsigned char CDB[16];
	unsigned int iLen = 512;
	unsigned char bBuf[512];
	unsigned char bTemp[512];
	int i;

	if (fInfo == NULL) return 0x01;
	if (sizeof(FW_FlashSizeInfo) != 512) return 0x02;	// ? check FW_FlashSizeInfo size 		
	if (sizeof(FW_FlashSizeInfo_Block) != 512)	return 0x03;
	if (sizeof(FW_FlashSizeInfo_Page) != 512)	return 0x04;
	memset(fInfo, 0, sizeof(FW_FlashSizeInfo));

	memset(CDB, 0, sizeof(CDB));
	CDB[0] = 0x06;
	CDB[1] = 0x89;
	CDB[2] = 0x01;

	memset(bBuf, 0, sizeof(bBuf));
	if (!bType)
		iRT = NT_CustomCmd(targetDisk, CDB, 12, SCSI_IOCTL_DATA_IN, iLen, bBuf, NES_BASECMD_TIMEOUT);
	else
		iRT = NT_CustomCmd_Phy(DeviceHandel, CDB, 12, SCSI_IOCTL_DATA_IN, iLen, bBuf, NES_BASECMD_TIMEOUT);
	if (iRT) return iRT;

	memcpy(bTemp, bBuf, sizeof(bTemp));
	for (i = 16; i < 256; i = i + 4)		// ** 4byte swap from FW (FW using data area: 256-16
	{
		SwapH2L_32_UFW(bBuf + i, bTemp + i);
	}

	for (i = 256; i < 512; i = i + 4)		// ** 4byte swap from FW (SW using data area: 256-512 )
	{
		SwapH2L_32_UFW(bBuf + i, bTemp + i);
	}
	memcpy(fInfo->bData, bTemp, 512);

	return 0;
}

// End PPS 
//===========================================================================