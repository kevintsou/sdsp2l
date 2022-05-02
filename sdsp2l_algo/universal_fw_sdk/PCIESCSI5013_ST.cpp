
#include "PCIESCSI5013_ST.h"
#include "IDPageAndCP_Hex_SSD.h"
//#include "ldpcecc.h"
#include "universal_interface/MemManager.h"
#include <time.h>
#include "utils\CheckSSDBin.h"

/*	COMMAND     Description
	0x00	 	GET_VERSION_DATE
	0x01		READ_FLASH_ID
	0x09		RESET
	0x14		SCAN_EARLY_BAD
	0x40		GET_FLASH_TEMPERATURE_CODE
	0x43		READ_TSB_UNIQUE_ID
	0x44		SET_GET_TSB_PARAMETER
	0x81		READ_SCAN_WINDOW
	0xA1		TSB_BICS4_QLC_PSEUDO
	0xA2		BICS4_GET_REVISION
	0xA3		BICS3_DISABLE_IPR
	0xA5		READ_MICRON_UNIQUE_ID
	0xA8		READ_TSB_PARAMETER_PAGE
	0xAA		SET_PATTERN
	0xAC		VERIFY_RETRY
	0xAF		SET_FEATURE
	0xBB		SET_GET_PARAMETER
	0xBC		GET_STATUS
	0xBD		SET_GET_PAGE_MAP
	0xBF		SET_GET_RETRY_TABLE
	0xCE		ERASE
	0xD0		SET_LDPC_ITERATION
	0xEC & 0xEE		D3_WRITE_NEW_FORMAT/D3_WRITE
	0xEB & 0xEF		D1_WRITE_NEW_FORMAT/D1_WRITE
	0xFC & 0xFE		D3_READ_NEW_FORMAT/D3_READ
	0xFB & 0xFF		D1_READ_NEW_FORMAT/D1_READ
*/

#define OPCMD_COUNT_5013 30
#define AP_PARAMETER_INI   ".\\OVERRIDE_AP_PARAMETER.ini"

static const unsigned char OpCmd5013[OPCMD_COUNT_5013] = 
{0x00, 0x01, 0x09, 0x14, 0x40, 0x43, 0x44, 0x81, 0xA1, 0xA2, 0xA3, 0xA5, 0xA8, 0xAA, 0xAC, 0xAF,
 0xBB, 0xBC, 0xBD, 0xBF, 0xCE, 0xD0, 0xEC, 0xEE, 0xEB, 0xEF, 0xFC, 0xFE, 0xFB, 0xFF};

const int MAX_FRAME_CNT = 4;
const int L4K_SIZE_PER_FRAME = 32;
const int L4K_BUF_SIZE_WR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT*2;
const int L4K_WRITE_START_ADDR = 0;
const int L4K_READ_START_ADDR = L4K_SIZE_PER_FRAME*MAX_FRAME_CNT;

unsigned char P4K_WR[L4K_BUF_SIZE_WR] = {0};

#pragma warning(disable: 4996)

int clock_idx_to_Mhz_5013[] = {10, 33, 40, 200, 332, 400, 533, 667, 800, 1066, 1200};

PCIESCSI5013_ST::PCIESCSI5013_ST( DeviveInfo* info, const char* logFolder )
:USBSCSICmd_ST(info,logFolder)
{
    Init();
}

PCIESCSI5013_ST::PCIESCSI5013_ST( DeviveInfo* info, FILE *logPtr )
:USBSCSICmd_ST(info,logPtr)
{
    Init();
}

PCIESCSI5013_ST::~PCIESCSI5013_ST()
{
	if (_ldpc)
		delete _ldpc;
}

int PCIESCSI5013_ST::GetDataRateByClockIdx(int clk_idx)
{
	return clock_idx_to_Mhz_5013[clk_idx];
}

void PCIESCSI5013_ST::Init()
{
	memset (m_Section_Sec_Cnt, 0, sizeof(m_Section_Sec_Cnt));
	memset (m_Section_CRC32, 0, sizeof(m_Section_CRC32));
	memset (_fwVersion, 0, sizeof(_fwVersion));
	_deviceType = IF_NVNE|DEVICE_PCIE|DEVICE_PCIE_5013;
	_burnerMode = false;
	_timeOut = 20;
	_ldpc = NULL;
	_flash_driving = 0;
	_flash_ODT = 0;
	_controller_driving = 0;
	_controller_ODT = 0;
	_last_offline_time = 0;
}

int PCIESCSI5013_ST::Dev_SQL_GroupWR_Setting(int blockSeed, int pageSeed, bool isWrite, int LdpcMode, char Spare)
{
	int rtn = -1;
	if (-1 == LdpcMode)
	{
		rtn = Dev_SetFlashPageSize(_FH.PageSize);
	}
	else
	{
		rtn =Dev_SetEccMode(LdpcMode);
	}

	if (rtn)
		return rtn;
	
	rtn = Dev_SetFWSeed(pageSeed, blockSeed);

	if (rtn)
		return rtn;

	rtn = Dev_SetFWSpare_WR(Spare, isWrite);

	return rtn;

}

int PCIESCSI5013_ST::ATA_SetAPKey(void){

	int rtn = 0;
	unsigned char apkey_cdb[16] = {0};
	unsigned char CurOutRegister[8],PreATARegister[8];
	unsigned char tempbuf[512]={0};
	apkey_cdb[0] = 0x00;
	apkey_cdb[1] = 0x6F;
	apkey_cdb[2] = 0xFE;
	apkey_cdb[3] = 0xEF;
	apkey_cdb[4] = 0xFA;
	apkey_cdb[5] = 0xAF;
	apkey_cdb[6] = 0xE0;
	rtn = MyIssueATACmd(PreATARegister, apkey_cdb, CurOutRegister, 0, tempbuf, 0, true, false, _timeOut, false);

	return rtn;

}


int PCIESCSI5013_ST::Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len )
{
	int hmode = mode&0xF0;
	mode = mode&0x0F;

	if ( (mode&0x03)==1 ) {
		if ( _WriteDDR(buf,len,ce) ) {
			Log ( LOGDEBUG, "Write DDR fail \n" );
			return 1;
		}        
	}

//#ifdef DEBUGSATABIN
//	if ( _debugCmdPause ) {
//		cmd->Log ( LOGDEBUG, "triger DDR command pattern:\n" );
//		cmd->LogMEM(sql,sqllen);
//		cmd->Log ( LOGDEBUG, "press any key to triger DDR\n" );
//		getch();
//	}
//#endif

	if ( !hmode )  {
		if ( _TrigerDDR(ce,sql,sqllen,len,_workingChannels) ) {
			Log ( LOGDEBUG, "Trigger DDR fail \n" );
			return 2;
		}
		//#ifdef DEBUGSATABIN
		//Log ( LOGDEBUG, "Trigger ce %d DDR time consume: %ul \n", ce, std::clock()-cstart );
		//#endif

		if ( mode==2 ) {
//#ifdef DEBUGSATABIN
//			if ( _debugCmdPause ) {
//				cmd->Log ( LOGDEBUG, "read DDR command pattern:\n" );
//				cmd->LogMEM(sql,sqllen);
//				cmd->Log ( LOGDEBUG, "press any key to read DDR\n" );
//				getch();
//			}
//#endif
			if( _ReadDDR(buf,len,0) ) {
				Log ( LOGDEBUG, "Read DDR fail \n" );
				return 3;
			}
		}
		else if ( mode==3 ) {
//#ifdef DEBUGSATABIN
//			if ( _debugCmdPause ) {
//				cmd->Log ( LOGDEBUG, "read DDR command pattern:\n" );
//				cmd->LogMEM(sql,sqllen);
//				cmd->Log ( LOGDEBUG, "press any key to read DDR\n" );
//				getch();
//			}
//#endif
			unsigned char tmpbuf[512];
			if( _ReadDDR(tmpbuf,512,2) ) {
				Log ( LOGDEBUG, "Read DDR fail \n" );
				return 3;
			}
			memcpy(buf,tmpbuf,len);
		}
	}
	return 0;
}

float PCIESCSI5013_ST::CaculateWindowsWidthPicoSecond(unsigned short win_start, unsigned short win_end, unsigned short DLL_Lock_Center, unsigned short data_rate)
{
	const float SDLL_Overhead = 26;
	float flash_data_rate = (float)data_rate;
	float flash_clock_period =  1000000 / (flash_data_rate/2) ;//pico seconds
	int zeroCnt = win_end - win_start;
	float windows_pico_seconds_read= ( (flash_clock_period/4)/ (SDLL_Overhead + DLL_Lock_Center) ) * zeroCnt;
	return windows_pico_seconds_read;
}

int PCIESCSI5013_ST::Dev_InitDrive( const char* ispfile )
{
	//_iFF->BridgeReset();
	InitFlashInfoBuf ( 0,0, 0, 4 );

	char* fw;
	unsigned char bufinq[4096];
	if ( NVMe_Identify(bufinq, sizeof(bufinq) )!=0 ) {
		Log(LOGDEBUG, "Identify fail\n");
		return false;
	}
	fw = _fwVersion;
	Log(LOGDEBUG, "FW Version %s\n",fw);
	if ( fw[0]=='E' && fw[1]=='D' && fw[2]=='B' ) {
		_burnerMode = 1;
	}
	
	NVMe_SetAPKey();

	unsigned char buf[4096];
	memset(buf, 0, sizeof(buf));
	unsigned char *pBuf = &buf[0];
	if ( ispfile[0]!='N' || ispfile[1]!='A') {
		unsigned char bufinq[4096];
		if ( NVMe_Identify(bufinq, sizeof(bufinq) )!=0 ) {
			//cmd->MyCloseHandle(&_hUsb);
			Log(LOGDEBUG, "Identify fail\n");
			return false;
		}
		const char* fw = _fwVersion;
		Log(LOGDEBUG, "FW Version %s\n",fw);

		//if ( sataCmd->_workCECnt==512 )  {
		//	sataCmd->_workCECnt= 128; //default for buffer.
		//	//sataCmd->InitBuffer();
		//}

		int maxEraseCnt=0;
		if ( fw[0]=='E' && fw[2]=='F' ) {
			// get system info
			//sataCmd->SmartGetSystemInfo( pBuf, 4096 );
			//nt_file_log( "PCIE Device System information\n" );
			////cmd->LogMEM(pBuf, 512);
			////const int sysFlashBlockCnt = *((unsigned short*)&pBuf[224]); 
			//const int sysFlashBlockCnt = *(&pBuf[13]) * 256 + *(&pBuf[12]); 
			//const int maxBlock = _fh->MaxBlock;
			//const int blockFactor = maxBlock/sysFlashBlockCnt;
			//sataCmd->_channels = pBuf[48];
			//sataCmd->_channelCE = pBuf[57];
			//const int ceCount = sataCmd->_channels*sataCmd->_channelCE; //channel*ce
			//sataCmd->_workCECnt = ceCount;
			////sataCmd->InitBuffer();

			//int eraseBufLen = ((sysFlashBlockCnt*4+4095)/4096)*4096;
#if 0
			CharMemMgr blkMapBuf(eraseBufLen);
			unsigned char *pBlkMapBuf = blkMapBuf.Data();
			for ( int ce=0; ce<ceCount; ce++) {
				int channel = ce/sataCmd->_channelCE;
				//int wce = sataCmd->GetCEByMap(ce);
				cmd->Log ( LOGDEBUG, "BlockMap information for ce[%d]\n",ce );
				//sataCmd->SmartGetCeBlockMapping(channel,ce,pBlkMapBuf,eraseBufLen);

				for ( int b=0; b<maxBlock; b++ ) {
					int blk = b/blockFactor; //in ed3, SATA block size = USB block size/2, because it use 16K page size in A19TLc
					if ( b>0 && (b%8)==0 )  {
						cmd->Log ( LOGDEBUG, "\n" );
					}
					int btype = (((pBlkMapBuf[blk*4+3]<<24)&0xFF000000) | ((pBlkMapBuf[blk*4+2]<<16)&0xFF0000) | ((pBlkMapBuf[blk*4+1]<<8)&0xFF00) | (pBlkMapBuf[blk*4]&0xFF));
					cmd->Log ( LOGDEBUG, "%02X%02X%02X%02X ",btype&0xFF,(btype>>8)&0xFF,(btype>>16)&0xFF,(btype>>24)&0xFF );

					cmd->SetBlockType( ce,b,btype );
				}
				cmd->Log ( LOGDEBUG, "\n" );
			}
			fn.SetEmpty();
			fn.Cat("%s\\blockType.csv",cmd->GetLogDir());
			FileMgr fmBlockType( fn.Data(), "w" );
			fprintf( fmBlockType.Ptr(), "Block" );
			for ( int ce=0; ce<ceCount; ce++ ) {
				fprintf( fmBlockType.Ptr(), ",ce_%d",ce );
			}
			fprintf( fmBlockType.Ptr(), "\n" ); 
			// #ifndef 	__BLOCKMAP__
			// const int type=sataCmd->GetParam()->beD3? 3:2;
			// int eraMargin=maxEraseCnt/2;
			// #endif
			for ( int b=0; b<maxBlock; b++ ) {
				fprintf( fmBlockType.Ptr(), "%d",b );
				for ( int ce=0; ce<ceCount; ce++) {
					// #ifndef 	__BLOCKMAP__
					// int erCnt = cmd->GetEraseCnt(ce,b);
					// int btype = (erCnt>eraMargin)? type:1;
					// if ( erCnt==-1 )  btype = btype&0xFFFC;
					// sataCmd->SetBlockType( ce,b,btype );
					// #endif
					fprintf( fmBlockType.Ptr(), ",%08X", sataCmd->GetBlockType(ce,b) );
				}
				fprintf( fmBlockType.Ptr(), "\n" );
			}
			fflush(fmBlockType.Ptr());
			fmBlockType.Close();
#endif
		}

		if ( fw[0]!='E' || fw[2]!='B'  ) {
			bool jump = false;
			for ( int loop=0; loop<3; loop++ ) {
				//-- Must back to ROM code --
				if ( Dev_CheckJumpBackISP (true,false ) !=0 )  {
					Log(LOGDEBUG,   "### Err: Can't jump to ROM code\n" );
					::Sleep ( 500 );
				}
				else {
					jump = true;
					break;
				}
			}
			if (!jump ) {
				Log(LOGDEBUG,  "### Err: Can't jump to ROM code\n" ); 
				//cmd->MyCloseHandle(&_hUsb);
				return false;
			}

			NVMe_SetAPKey();
			//const char* fisp = (ispfile? ispfile:ispFile);
			//cmd->Log ( LOGDEBUG, "start ATA_ISPProgPRAM %s\n",fisp );
			if ( NVMe_ISPProgPRAM(ispfile)!=0 ) {
				Log(LOGDEBUG, "### Err: ISP program fail\n" );
				//cmd->MyCloseHandle(&_hUsb);
				return false;
			}
			::Sleep ( 200 );
			//cmd->Log ( LOGDEBUG, "start ATA_ISPJumpMode\n" );
			//            if(sataCmd->ATA_ISPJumpMode())  {
			//                cmd->Log(LOGDEBUG,"Jump ISP Fail!\n");
			//                return false;
			//            }     
			Log(LOGDEBUG, "Jump ISP Pass!\n");
			//::Sleep ( 1000 );
			_burnerMode = 1;
			NVMe_SetAPKey();
		}
		//cmd->Log ( LOGDEBUG, "start ADMIN_NVMe_Identify\n" );
		for ( int l=0; l<=16; l++ ) {
			if ( NVMe_Identify(bufinq, sizeof(bufinq) )==0 ) {
				fw = _fwVersion;
				Log(LOGDEBUG, "FW Version %s\n",fw);
				//if ( memcmp(sataCmd->_fwVersion,"SABM",4)!=0 ) {
				if (  fw[2]=='B' ) {
					break;
				}
			}
			::Sleep ( 1000 );
		}

		//if ( memcmp(sataCmd->_fwVersion,"SABM",4)!=0 ) {
		if (  fw[2]!='B' ) {
			Log(LOGDEBUG,  "### Err: Can not switch bruner\n" );
			//cmd->MyCloseHandle(&_hUsb);
			return false;
		}

	}
    
	_sql = SQLCmd::Instance(this, &_FH, _eccBufferAddr, _dataBufferAddr);

	//SetAPParameter2CTL();
	return true;
}

void PCIESCSI5013_ST::InitLdpc()
{
	if (_ldpc)
	{
		//assert(0);
	}
	else
	{
	  _ldpc = new LdpcEcc(0x5013);
	}
}

int PCIESCSI5013_ST::Dev_GetFlashID(unsigned char* r_buf)
{
	return NT_GetFlashExtendID(1, _hUsb, 0, r_buf, 0, 0, 0);
}

int PCIESCSI5013_ST::Dev_CheckJumpBackISP ( bool needJump, bool bSilence )
{
	NVMe_SetAPKey();
	unsigned char bufinq[4096];
	int ret= 0;
	// only test 5-1 times.
	for ( int retry=0; retry<5; retry++ ) {
		NVMe_Identify(bufinq, sizeof(bufinq) );

		const char* fw = _fwVersion;
		Log(LOGDEBUG,"FW Version %s\n",fw);
		//if ( memcmp(_fwVersion,"SARM",4)==0 ) {
		if ( fw[0]=='E' && fw[2]=='R' ) {
			return 0;
		}

		ret=NVMe_ForceToBootRom ( 1 ); //To enter boot-code mode
		if ( ret ) {
			Log(LOGDEBUG,"Force to Boot fail %d\n",ret );
			continue;
		}
		::Sleep ( 1000 );
	}
	return 2;
}

int PCIESCSI5013_ST::Dev_GetSRAMData ( unsigned char offset, unsigned char* buf, int len )
{
    return _ReadDDR(buf,len, offset);
}

int PCIESCSI5013_ST::Dev_SetSRAMData ( unsigned char offset, const unsigned char* buf, int len )
{
    return _WriteDDR((unsigned char*)buf,len, 0);
}

//////////////////////////////
int PCIESCSI5013_ST::Dev_SetFlashClock ( int timing_idx )
{
/*
    1   33.3MHz
    2   41.7MHz
    3   50MHz
    4   200MHz
    5   333MHz
    6   400MHz
    7   533MHz
    8   800MHz
*/
	unsigned char ncmd[16];
	memset(ncmd,0x00,16);
	ncmd[0] = 0x54;//nvme 48byte	
	ncmd[1] = timing_idx;//nvme 49byte
	UCHAR buf[16];
	memset(buf,0x00,16);
	if(MyIssueNVMeCmd(ncmd,0xD1, NVME_DIRCT_WRITE,buf,sizeof(buf),15, 0)){
		return -1;
	}
	return 0;
}

int PCIESCSI5013_ST::Dev_GetRdyTime ( int action, unsigned int* busytime )
{
	return 0;
}

int PCIESCSI5013_ST::Dev_SetFlashDriving ( int driving_idx )
{
	/*
        0    50 ohm
        1    37.5 ohm
        2    30 ohm
        3    25 ohm
        4    21.43 ohm
     */
	unsigned char ncmd[16];
	memset(ncmd,0x00,16);
	ncmd[0] = 0x53;//nvme 48byte	
	ncmd[1] = driving_idx;//nvme 49byte
	UCHAR buf[16];
	memset(buf,0x00,16);
	if(MyIssueNVMeCmd(ncmd,0xD1, NVME_DIRCT_WRITE,buf,sizeof(buf),15, 0)){
		return -1;
	}
	return 0;
}

int PCIESCSI5013_ST::GetEccRatio(void)
{
	return 2;
}


int PCIESCSI5013_ST::OverrideAPParameterFromIni( unsigned char * ap_parameter)
{
	int app_override_cnt=0;
	int value = 0;
	for(int addr_idx=0; addr_idx<1024; addr_idx++)
	{
		char group_key_name[128] = {0};
		sprintf(group_key_name, "0x%x", addr_idx);
		value = GetPrivateProfileInt(group_key_name, "value", 0xFFFF, AP_PARAMETER_INI);
		if ( 0xFFFF != value  )
		{
			ap_parameter[addr_idx] = value;
			app_override_cnt++;
		}
	}
	if (app_override_cnt)
	{
		nt_file_log("ovrride parameter cnt=%d", app_override_cnt);
	}
	return app_override_cnt;
}

int PCIESCSI5013_ST::ST_Set_Parameter(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf)
{
	unsigned char cdb512[512]={0};
	cdb512[0] = 0x06;
	cdb512[1] = 0xF6;
	cdb512[2] = 0xBB;

	GetODT_DriveSettingFromIni();
	buf[0xE0] = _flash_driving;
	buf[0xE1] = _flash_ODT;
	buf[0xE2] = _controller_driving;
	buf[0xE3] = _controller_ODT;

	OverrideAPParameterFromIni(buf);

	Issue512CdbCmd(cdb512, buf, 1024, SCSI_IOCTL_DATA_OUT, _SCSI_DelayTime);
	return 0;
}


/************************************************************************/
/* For 5013
/* Toggle 3.0 set flash clock to 200M  buf[0x4D] = 8
/* Toggle 2.0 set flash clock to 200M  buf[0x4D] = 5
/* Toggle 1.0 set flash clock to 100M  buf[0x4D] = 3
/* Legacy     set flash clock to 40M   buf[0x4D] = 2 
// CLK=>0:10 1:30 2:44 3:200 4:332 5:400 6:533 7:677 8:800 Mhz
/************************************************************************/
int PCIESCSI5013_ST::GetE13ClkMapping(int clk_mhz) 
{
	if (10 == clk_mhz) return 0;
	if (30 == clk_mhz) return 1;
	if (40 == clk_mhz) return 2;
	if (200 == clk_mhz) return 3;
	if (332 == clk_mhz) return 4;
	if (400 == clk_mhz) return 5;
	if (533 == clk_mhz) return 6;
	if (677 == clk_mhz) return 7;
	if (800 == clk_mhz) return 8;
	return -1;
}

void PCIESCSI5013_ST::GetODT_DriveSettingFromIni()
{
	_flash_driving = GetPrivateProfileIntA ( "E13","FlashDriving", 2,  _ini_file_name); //0:UNDER 1:NORMAL 2:OVER
	_flash_ODT = GetPrivateProfileIntA ( "E13","FlashODT", 4, _ini_file_name ); //0:off 1:150 2:100 3:75 4:50 (ohm)
	_controller_driving = GetPrivateProfileIntA ( "E13","ControlDriving", 4, _ini_file_name );//0:50 1:37.5 2:35 3:25 4:21.43 (ohm)
	_controller_ODT = GetPrivateProfileIntA ( "E13","ControlODT", 3, _ini_file_name );//0:150 1:100 2:75 3:50 4:0ff (ohm)

	if (_fh->bIsMIcronN28)
	{
		_flash_driving = 1; //N28A spec says it is not support OVER drive.
	}
	if (_fh->bIsYMTC3D)
	{
		if (1 == _fh->DieNumber)
		{
			_flash_ODT = 0;
		}
	}
}

int PCIESCSI5013_ST::Dev_SetP4K( unsigned char * buf, int Length, bool isWrite)
{
	int ret = NVMe_SetAPKey();
	if ( ret ) {
		return ret;
	}        

	unsigned char ncmd[16];
	unsigned int GetLength=16;
	memset(ncmd,0x00,16);

	ncmd[0] = 0x58;	
	if (isWrite)
		ncmd[1] = 0x00;
	else
		ncmd[1] = 0x01;

	ncmd[2] = 0x00;

	ncmd[4] = (Length & 0xff);
	ncmd[5] = ((Length>>8) & 0xff);
	ncmd[6] = ((Length>>16) & 0xff);
	ncmd[7] = ((Length>>24) & 0xff);

	return MyIssueNVMeCmd(ncmd,0xD1,2, buf,Length,15,false) ;
}

int PCIESCSI5013_ST::GetDefaultDataRateIdxForInterFace(int interFace)
{
	int user_specify_clk_idx = -1;
	if (0 == interFace)
	{
		user_specify_clk_idx = GetPrivateProfileInt("E13","LegacyClk",-1,_ini_file_name);
		if (-1 != user_specify_clk_idx)
		{
			return user_specify_clk_idx;
		}
		else
			return 2;//40Mhz
	}
	else if (1 == interFace)
	{
		user_specify_clk_idx = GetPrivateProfileInt("E13","T1Clk",-1,_ini_file_name);
		if (-1 != user_specify_clk_idx)
		{
			return user_specify_clk_idx;
		}
		else
			return 3;//200Mhz
	}
	else if (2 == interFace)
	{
		user_specify_clk_idx = GetPrivateProfileInt("E13","T2Clk",-1,_ini_file_name);
		if (-1 != user_specify_clk_idx)
		{
			return user_specify_clk_idx;
		}
		else
			return 5;//400Mhz
	}
	else if (3 == interFace)
	{
		user_specify_clk_idx = GetPrivateProfileInt("E13","T3Clk",-1,_ini_file_name);
		if (-1 != user_specify_clk_idx)
		{
			return user_specify_clk_idx;
		}
		else
			return 8;//800Mhz
	}
	else
	{
		assert(0);
	}
	return 0;
}

int PCIESCSI5013_ST::ST_SetGet_Parameter(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bSetGet,unsigned char *buf)
{
	unsigned char cdb512[512]={0};
	cdb512[0] = 0x06;
	cdb512[1] = 0xF6;
	cdb512[2] = 0xBB;

	if (bSetGet)
	{
		memcpy(m_apParameter, buf, 1024);

		int user_clk = 0;
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
			case FEATURE_T3:
				m_apParameter[0x4D] = GetDefaultDataRateIdxForInterFace(FEATURE_T3);
				break;
			case FEATURE_T2:
				m_apParameter[0x4D] = GetDefaultDataRateIdxForInterFace(FEATURE_T2);
				break;
			case FEATURE_T1:
				m_apParameter[0x4D] = GetDefaultDataRateIdxForInterFace(FEATURE_T1);
				break;
			case FEATURE_LEGACY:
				m_apParameter[0x4D] = GetDefaultDataRateIdxForInterFace(FEATURE_LEGACY);
				break;
		}
		

		int current_clk_idx = m_apParameter[0x4D];

		int tADL = GetPrivateProfileInt("[E13]", "tADL", -1, _ini_file_name);
		int tWHR = GetPrivateProfileInt("[E13]", "tWHR", -1, _ini_file_name);
		int tWHR2 = GetPrivateProfileInt("[E13]", "tWHR2", -1, _ini_file_name);
		if (-1 == tADL && -1 == tWHR && -1 == tWHR2)
		{
			if(FEATURE_LEGACY == m_flashfeature) { //legacy 
				GetACTimingTable( GetDataRateByClockIdx(current_clk_idx), tADL, tWHR, tWHR2 );
			} else { //toggle
				GetACTimingTable( GetDataRateByClockIdx(current_clk_idx)/2, tADL, tWHR, tWHR2 );
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
		if (GetPrivateProfileInt("E13", "GlobalRandomSeed", 1, _ini_file_name))
		{
			m_apParameter[0xDF] |= 0x01;
		}

		GetODT_DriveSettingFromIni();
		m_apParameter[0xE0] = _flash_driving;
		m_apParameter[0xE1] = _flash_ODT;
     	m_apParameter[0xE2] = _controller_driving;
		m_apParameter[0xE3] = _controller_ODT;

		m_apParameter[0x4D] = 0x02;
		OverrideAPParameterFromIni(m_apParameter);
		Issue512CdbCmd(cdb512, m_apParameter, 1024, SCSI_IOCTL_DATA_OUT, _SCSI_DelayTime);
	}
	else
	{
		cdb512[3]=1;
		Issue512CdbCmd(cdb512, buf, 1024, SCSI_IOCTL_DATA_IN, _SCSI_DelayTime);
	}

	return 0;//always return 0 for win10 issue
}

int PCIESCSI5013_ST::Dev_SetEccMode ( unsigned char ldpc_mode )
{
	unsigned char ncmd[16];
	memset(ncmd,0x00,16);
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

	int e13_ldpc_bit[]={2048, 1920, 1792, 1664, 1408, 1152, 1024, 2432, 2304};


	int frame_count = _fh->PageSize/4096;

	int frame_size_with_parity_crc_ldpc =  _fh->PageSize/frame_count;
	int remains_size = frame_size_with_parity_crc_ldpc - 4112;
	int max_ldpc_bytes = remains_size/2;


	//Log(LOGDEBUG, "%s:pageSize=%d, frameCounts=%d", __FUNCTION__,_fh->PageSize ,frame_count);
	//Log(LOGDEBUG, "frame_size_with_parity_crc_ldpc=%d,remains=%d,ldpcMaxBits=%d", frame_size_with_parity_crc_ldpc, remains_size, max_ldpc_bytes*8);
	//Log(LOGDEBUG, "input ldpc mode=%d bit=%d",ldpc_mode, e13_ldpc_bit[ldpc_mode]);

	//if (-1 == ldpc_mode) {
	//	//Log(LOGDEBUG, "e13 auto select ldpc");
	//	return 0;
	//}
	//else if (e13_ldpc_bit[ldpc_mode] > max_ldpc_bytes*8)
	//{
	//	Log(LOGDEBUG, "the selected ldpc bit too big");
	//	//return -1;
	//}

	ncmd[0] = 0x50;//nvme 48byte	
	ncmd[1] = ldpc_mode;//nvme 49byte
	UCHAR buf[16];
	memset(buf,0x00,16);
	if(MyIssueNVMeCmd(ncmd,0xD1, NVME_DIRCT_WRITE,buf,sizeof(buf),15, 0)){
		return -1;
	}
	return 0;
}

int PCIESCSI5013_ST::Dev_SetFWSpare ( unsigned char Spare )
{
	return 0;
}

int PCIESCSI5013_ST::Dev_SetFWSpare_WR(unsigned char Spare, bool isWrite)
{
	int FRAME_CNT_PER_PAGE = 4;
	if (8 == _fh->PageSizeK)
	{
		FRAME_CNT_PER_PAGE = 2;
	}
	Dev_GenSpare_32(P4K_WR, Spare, FRAME_CNT_PER_PAGE, 0,  L4K_BUF_SIZE_WR);
	return Dev_SetP4K(  isWrite? (&P4K_WR[L4K_WRITE_START_ADDR]): (&P4K_WR[L4K_READ_START_ADDR]), L4K_BUF_SIZE_WR/2 /*read 128, write 128*/, isWrite);

}

void PCIESCSI5013_ST::Dev_GenSpare_32(unsigned char *buf,unsigned char mode,unsigned char FrameNo,unsigned char Section, int buf_len)
{

	const int L4KBADR_READ  = 1;
	const int L4KBADR_WRITE = 0;
	const int MAX_FRAME_CNT = 4;
	const int L4K_SIZE_PER_FRAME = 32;

	/*1st 128 byte for write L4K, 2nd 128 bytes for read L4K*/
	if ( (  (L4K_SIZE_PER_FRAME*MAX_FRAME_CNT*2)  != buf_len)  || !buf )
	{
		assert(0);
	}

	

	/*follows cfg is depends on CTL, need to check with fw team!*/
	unsigned char L4k_default[2][16] = {
                                      { 
                                         0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00,
                                         0xFF, 0xFF, 0x00, 0x00,
                                         0x00, 0x00, 0x01, 0x11
                                      },
                                      {
                                         0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00,
                                         0xFF, 0xFF, 0x07, 0x00,
                                         0x00, 0x00, 0x01, 0x11
                                      },
                                  };



	/*follows cfg is depends on CTL, for override the byte L4K[13], need to check with fw team!*/
	unsigned char L4KBADR[2][MAX_FRAME_CNT] = { {0xA1, 0xA9, 0xB1, 0xB9},   //W
									            {0xE1, 0xE9, 0xF1, 0xF9}};  //R
	
	for (int wr_idx=0; wr_idx<2; wr_idx++)//0 for write, 1 for read
	{
		for(int frame_idx = 0; frame_idx <FrameNo ;  frame_idx++)
		{
			int start_addr = (wr_idx*MAX_FRAME_CNT*L4K_SIZE_PER_FRAME) + frame_idx*L4K_SIZE_PER_FRAME;
			memset(&buf[start_addr], 0x00, L4K_SIZE_PER_FRAME);
			memcpy(&buf[start_addr], &L4k_default[wr_idx][0], 16);
			buf[start_addr] = 0xCB;
			buf[start_addr+1] = 0xFF;
			buf[start_addr+2] = 0xFF;
			buf[start_addr+3] = 0xFF;
			buf[start_addr+6] = mode;
			buf[start_addr+7] = 0xFF;	// buffer vaild(all vaild)
			buf[start_addr+13] = 0x90;
			if(mode=='C')
			{
				buf[start_addr+4] = 0x00;
				buf[start_addr+5] = Section;

			}
			else if(mode=='I')
			{
				buf[start_addr+4] = 0x01;
				buf[start_addr+5] = 0x48;

			}
			else if(mode=='P')
			{
				buf[start_addr+4] = 0x01;
				buf[start_addr+5] = 0x19;
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
			buf[start_addr+13] = L4KBADR[wr_idx][frame_idx];

		}
	}//end of w/r loop

}

void PCIESCSI5013_ST::GenE13Spare(unsigned char *buf,unsigned char mode,unsigned char FrameNo,unsigned char Section)
{

	unsigned char i;

	for(i=0;i<FrameNo;i++)
	{
		memset(&buf[i*16],0x00,16);
		buf[i*16] = 0xCB;
		buf[i*16+1] = 0xFF;
		buf[i*16+2] = 0xFF;
		buf[i*16+3] = 0xFF;
		buf[i*16+6] = mode;
		buf[i*16+7] = 0xFF;	// buffer vaild(all vaild)
		buf[i*16+13] = 0x90;
		if(mode=='C')
		{
			buf[i*16+4] = 0x00;
			buf[i*16+5] = Section;

		}
		else if(mode=='I')
		{
			buf[i*16+4] = 0x01;
			buf[i*16+5] = 0x48;

		}
		else if(mode=='P')
		{
			buf[i*16+4] = 0x01;
			buf[i*16+5] = 0x19;
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
		if(i==1)
			buf[i*16+13] = 0xA0;
		else if(i==2)
			buf[i*16+13] = 0xF0;
		else if(i==3)
			buf[i*16+13] = 0xC0;
	}
}

int PCIESCSI5013_ST::_TrigerDDR(int CE, const unsigned char* sqlbuf, int slen, int buflen, unsigned char channel )
{
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	int ret = NVMe_SetAPKey();
	if ( ret ) {
		return ret;
	}	

	unsigned char ncmd[16];
	unsigned char GetLength=16;
	memset(ncmd,0x00,16);

	//	ncmd[0] = 0x1A;
	//	ncmd[1] = 0x01;
	//	ncmd[2] = 0x00;
	//
	//	ncmd[4] = (slen & 0xff);
	//	ncmd[5] = ((slen>>8) & 0xff);
	//    ncmd[6] = ((slen>>16) & 0xff);
	//    ncmd[7] = ((slen>>24) & 0xff);
	//
	//	//ncmd[8] = CH;
	//
	//	if(IssueNVMeCmd(ncmd,0xC2,2,(unsigned char*)sqlbuf,slen,15)) 
	//		return -1;
	//		
	//	memset(ncmd,0x00,16);

	int newlen = _SQLMerge(sqlbuf,slen);

	ncmd[0] = 0x52;
	//ncmd[1] = 0x01;
	ncmd[2] = 0x00;

	ncmd[4] = (newlen & 0xff);
	ncmd[5] = ((newlen>>8) & 0xff);
	ncmd[6] = ((newlen>>16) & 0xff);
	ncmd[7] = ((newlen>>24) & 0xff);
	ncmd[8] = buflen/1024;

	//ncmd[9] = channel;
	ncmd[10] = channel*_channelCE+CE;

	//_WriteBin5013(this,"_TrigerDDR",_dummyBuf,newlen);
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	ret= MyIssueNVMeCmd(ncmd,0xD1,2,(unsigned char*)_dummyBuf,newlen,15,false);

	//if (0 )  {
	//	HANDLE hdl = MyCreateHandle ( _info.physical, true, false );
	//	Log ( LOGDEBUG, "MyIssueNVMeCmd2\n" );
	//	ret= MyIssueNVMeCmdHandle(hdl,ncmd,0xD1,2,(unsigned char*)_dummyBuf,newlen,15,false,_deviceType);
	//}
	return ret;
}

int PCIESCSI5013_ST::_WriteDDR(unsigned char* w_buf,int Length, int ce)
{
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	int ret = NVMe_SetAPKey();
	if ( ret ) {
		return ret;
	}        

	unsigned char ncmd[16];
	unsigned int GetLength=16;
	memset(ncmd,0x00,16);

	ncmd[0] = 0x51;	
	ncmd[1] = 0x00;
	ncmd[2] = 0x00;

	ncmd[4] = (Length & 0xff);
	ncmd[5] = ((Length>>8) & 0xff);
	ncmd[6] = ((Length>>16) & 0xff);
	ncmd[7] = ((Length>>24) & 0xff);

	//_WriteBin5013(this,"_WriteDDR",w_buf,Length);
	//ncmd[8] = CH;
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	return MyIssueNVMeCmd(ncmd,0xD1,2,w_buf,Length,15,false) ;
}



//datatype: 0=data, 1=vender, 2=ecc 3=time, 4=time+reset
int PCIESCSI5013_ST::_ReadDDR(unsigned char* r_buf,int Length, int datatype )
{
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	int ret = NVMe_SetAPKey();
	if ( ret ) {
		return ret;
	}    

	unsigned char ncmd[16];
	unsigned int GetLength=16;
	memset(ncmd,0x00,16);

	ncmd[0] = 0xD1;	
	ncmd[1] = datatype;

	ncmd[4] = (Length & 0xff);
	ncmd[5] = ((Length>>8) & 0xff);
	ncmd[6] = ((Length>>16) & 0xff);
	ncmd[7] = ((Length>>24) & 0xff);

	//ncmd[8] = CH;
	//ncmd[9] = mCE;
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	return MyIssueNVMeCmd(ncmd,0xD2,1,r_buf,Length,15,false) ;
}

int PCIESCSI5013_ST::NVMe_SetAPKey()
{
	int connector = _deviceType&INTERFACE_MASK;
	if ( ( IF_ATA==connector) ||  ( IF_C2012==connector)  ||  ( IF_ASMedia==connector) ) {
		return ATA_SetAPKey();
	}

	unsigned char ncmd[16];
	memset(ncmd,0x00,16);	

	ncmd[0] = 0x00;  //Byte48 //Feature
	//ncmd[1] = 0x00;  //B49
	//ncmd[2] = 0x00;  //B50
	//ncmd[3] = 0x00;  //B51
	ncmd[4] = 0x6F;  //B52    //SC
	ncmd[5] = 0xFE;  //B53    //SN
	ncmd[6] = 0xEF;  //B54    //CL
	ncmd[7] = 0xFA;  //B55    //CH
	//ncmd[8] = 0x00;
	//ncmd[9] = 0x00;
	//ncmd[10] = 0x00;
	//ncmd[11] = 0x00;
	//log_nvme("%s\n", __FUNCTION__);
	int ret = MyIssueNVMeCmd(ncmd, 0xD0, NVME_DIRCT_NON, NULL, 0, 15, 0);
	//Log ( LOGDEBUG, "%s ret=%d\n",__FUNCTION__,ret );
	return ret;  
}

int PCIESCSI5013_ST::GetFwVersion(char * version/*I/O*/)
{
	unsigned char buf[4096] = {0};
	if (NVMe_SetAPKey())
		return 1;

	if (NVMe_Identify(buf, 4096))
	{
		return 1;
	}
	else {
		memcpy(version, _fwVersion, 7);
		return 0;
	}

}

bool PCIESCSI5013_ST::Identify_ATA(char * version)
{
	return 0;
}

int PCIESCSI5013_ST::NVMe_Identify(unsigned char* buf, int len )
{

	int connector = _deviceType&INTERFACE_MASK;
	if ( ( IF_ATA==connector) ||  ( IF_C2012==connector)  ||  ( IF_ASMedia==connector) ) {
		unsigned char readBuf[512] = {0};
		int rtn = IdentifyATA(_hUsb,readBuf, sizeof(readBuf), connector);

		int idx=0;
		for(int i=46 ; i<=53 ; idx+=2,i+=2) {
			_fwVersion[idx] = readBuf[i+1];
			_fwVersion[idx+1] = readBuf[i];
		}
		_fwVersion[7] = '\0';
		return rtn;
	}

	NVMe_SetAPKey();


	assert( len>=sizeof(ADMIN_IDENTIFY_CONTROLLER) );
	unsigned char ncmd[16];
	memset(ncmd,0x00,16);


	if (MyIssueNVMeCmd(ncmd, ADMIN_IDENTIFY, NVME_DIRCT_READ, buf, sizeof(ADMIN_IDENTIFY_CONTROLLER), 15,0))
		return 1;
	PADMIN_IDENTIFY_CONTROLLER pIdCtrlr = (PADMIN_IDENTIFY_CONTROLLER)buf;

	memcpy(_fwVersion,&pIdCtrlr->FR[0],8);
	_fwVersion[7] = '\0';

	return 0;
}

int PCIESCSI5013_ST::NVMe_ForceToBootRom ( int type )
{
	unsigned int BuferSize=512;
	unsigned char buf[512];
	unsigned char ncmd[16];
	memset(buf,0x00,512);
	memset(ncmd,0x00,16);

	ncmd[0] = 0x02;	
	//ncmd[1] = Mode;
	//ncmd[2] = ResetCPUID;	
	ncmd[1] = type; //0 = burner , 1 = boot2
	//ncmd[1] = 0; //0 = burner , 1 = boot2
	//ncmd[2] = 1; //default cpu1		

	//ncmd[4] = (BuferSize & 0xff);
	//ncmd[5] = ((BuferSize>>8) & 0xff);
	//ncmd[6] = ((BuferSize>>16) & 0xff);
	//ncmd[7] = ((BuferSize>>24) & 0xff);
	//log_nvme("%s\n", __FUNCTION__);
	return MyIssueNVMeCmd(ncmd, 0xD0, NVME_DIRCT_WRITE, buf, 512, 15, 0);
}

int PCIESCSI5013_ST::_SQLMerge( const unsigned char* sql, int len )
{
	assert(len<sizeof(_dummyBuf));
	int newlen = ((len+511)/512)*512;
	memset(&_dummyBuf[0], 0, sizeof(_dummyBuf));
	memcpy(_dummyBuf,sql,len);
	return newlen;
}

//========================================================================
//Type=0, Boot2 PRAM FW
//    =1, Boot2 Normal FW
//    =2, PRAM FW
//    =3, Normal FW
//=========================================================================


int PCIESCSI5013_ST::NVMe_ISPProgPRAM(const char* m_MPBINCode)
{
	//Log(LOGDEBUG, "%s %s\n",__FUNCTION__, m_MPBINCode );

	//bool Scrambled=true;
	bool Scrambled=false;
	bool with_jump=true;

	//QString message,FWName;
	ULONG filesize;
	unsigned char *m_srcbuf;
	unsigned int sector_no,remainder,quotient,ubCount,loop;  
	bool status,is_lasted;
	//double timeout;
	//clock_t begin;

	//begin = clock();
	FILE * fp = fopen(m_MPBINCode, "rb");
	if(!fp)
	{
		//Err_str = "S01";
		Log(LOGDEBUG, "Can not open MP BIN File: %s successfully \n!",m_MPBINCode);
		return 1;
	}
	
	fseek ( fp, 0L, SEEK_END );
	filesize = ftell ( fp );
	fseek ( fp,0,SEEK_SET );


	if(filesize == -1L) 
	{	
		//Err_str = "S01";  //Bin file have problem
		Log(LOGDEBUG, "Failed ! Check FW BIN File size(%s) = 0 \n",m_MPBINCode);
		fclose(fp);
		return 1;
	}

	m_srcbuf = (unsigned char *)malloc(filesize);  
	memset(m_srcbuf,0x00,filesize);

	if (fread(m_srcbuf, 1, filesize, fp)<= 0)
	//if ( _read(fh,m_srcbuf,filesize) <= 0 )  //Bin file have problem
	{
		//Err_str = "S01";
		Log(LOGDEBUG, "Reading MP BIN File Failed !!!\n");
		fclose(fp);
		free(m_srcbuf);
		return 1;	
	}
	fclose(fp);

	ubCount=128;  //Nvme Cmd. Max Data Length: 512K
	int ISPFW_SingleBin_NoHeader = true;
	if(ISPFW_SingleBin_NoHeader)
	{
		if((filesize%512))
		{
			Log(LOGDEBUG, "BIN File size must be multiple of 512 bytes !!!\n");
			free(m_srcbuf);
			return 1;
		}

		sector_no=filesize/512;
		//if((filesize%512))
		//   sector_no+=1;

		quotient=sector_no/ubCount;  
		remainder=sector_no%ubCount;

		Log(LOGDEBUG, "ISP Program Burner:  (Bin No Header)\n");
		Log(LOGDEBUG, "Burner Code Size= %d bytes. loop=%d, remin.=%d\n",filesize,quotient,remainder);
	}
	else
	{

	}  

	for(loop=0;loop<quotient;loop++)
	{
		if ( NVMe_SetAPKey() ) {
			free(m_srcbuf);
			return false;
		}

		is_lasted=false;
		if(remainder==0)
		{
			if(loop==(quotient-1))
				is_lasted=true;
		}

		if(ISPFW_SingleBin_NoHeader)
			status=NVMe_ISPProgramPRAM(m_srcbuf+(loop*ubCount*512),ubCount*512,Scrambled,is_lasted,loop)==0;
		else {
			//status=NVMe_ISPProgramPRAM(m_srcbuf+m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+loop*ubCount)*512,ubCount*512,Scrambled,is_lasted,loop)==0;
		}
		if(!status)
		{
			//Err_str = "I_x03";
			Log(LOGDEBUG, "ISP Program Burner Code Failed_loop:%d !!\n",loop);
			free(m_srcbuf);
			return 1;
		}
		Log(LOGDEBUG, "%d done\n", loop);
	}

	if (remainder)
	{
		if ( NVMe_SetAPKey() ) {
			free(m_srcbuf);
			return false;
		}

		if(ISPFW_SingleBin_NoHeader)
			status=NVMe_ISPProgramPRAM(m_srcbuf+(quotient*ubCount*512),remainder*512,Scrambled,true,loop)==0;
		else
		{
			//status=NVMe_ISPProgramPRAM(m_srcbuf+(m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+quotient*ubCount)*512),remainder*512,Scrambled,true,loop)==0;
		}
			

		if(!status)
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

	if ( NVMe_SetAPKey() ) {
		free(m_srcbuf);
		return false;
	}

	if (with_jump)
	{
		//if(!ATA_ISPJumpMode(false,0))
		//{
		//    return false;
		//}

		//if(!NVMe_ForceToBootRom(0))  //CPU Index 1

		//if(NVMe_ForceToBootRom(1))  //CPU Index 1
		if(NVMe_ForceToBootRom(2))  //To enter burner mode
		{
			//Err_str = "I_x05";
			Log(LOGDEBUG, "Failed! Jump to burner mode fail !!\n");
			return 1;
		}
		Log(LOGDEBUG, "ISP Jump ok.\n");

		Sleep(5000);		
		unsigned char CardStateBuf[4096];
		if ( NVMe_Identify(CardStateBuf, 4096 )==0 ) {
			char *fw = _fwVersion;
			Log(LOGDEBUG, "FW Version %s\n",fw);
		}	
		//start = clock();
		//Sleep(3000);
		//		timeout = 3;
		//		for( int stl=0;stl<5;stl++ ) 
		//		{
		//			memset(CardStateBuf,0,512);
		//			if(!GetDeviceStatus(CardStateBuf)) {
		//				return 1;
		//			}
		//
		//			Log ( LOGDEBUG,"[%02x][%02x][%02x][%02x][%02x]\n",CardStateBuf[0],CardStateBuf[1],CardStateBuf[2],CardStateBuf[3],CardStateBuf[4]);
		//			//if (CardStateBuf[3]==0x00) //PASS
		//			//	if(CardStateBuf[2] == 100) //Progress
		//			if(CardStateBuf[4] == 2) //Burner
		//				break;
		//			
		////			finish = clock();
		////			PretestTime = ( double )( finish - start ) / CLOCKS_PER_SEC;
		////			if(PretestTime > timeout)
		////			{
		////				//Err_str = "F_E04";
		////				Msg_str.Format("ISP Jump to Burner Timeout");
		////				//ErrorRecord();
		////				return false;
		////			}
		//
		//			Sleep(2000);	
		//		}	

		if ( NVMe_Identify(CardStateBuf, 4096 )==0 ) {
			char *fw = _fwVersion;
			Log(LOGDEBUG, "FW Version %s\n",fw);
			if ( fw[2]=='B' ) {
				return 0;
			}
			else {
				Log(LOGDEBUG, "Not successfully running on Burner Code Now !\n");
			}
		}		
	}	

	return 1;
}

int PCIESCSI5013_ST::NVMe_ISPProgramPRAM(unsigned char* buf,unsigned int BuferSize,bool mScrambled,bool mLast,unsigned int m_Segment)
{
	unsigned int tempint;
	unsigned char ncmd[16];
	memset(ncmd,0x00,16);
	unsigned int BuferSize_temp = BuferSize;
	ncmd[0] = 0x12;                                        //Byte48, feature
	tempint=0x00;
	if(mScrambled)
		tempint = 0x02;

	if(mLast) {
		tempint = 0x04;
		//m_Segment += 1;
		BuferSize_temp = 128 * 512;
	}

	ncmd[1] = tempint;                                     //byte49	
	ncmd[4] = ((BuferSize_temp * m_Segment) & 0xff);       //byte52
	ncmd[5] = (((BuferSize_temp * m_Segment)>>8) & 0xff);  //byte53
	ncmd[6] = (((BuferSize_temp * m_Segment)>>16) & 0xff);  //byte54
	ncmd[7] = (((BuferSize_temp * m_Segment)>>24) & 0xff);  //byte55
	ncmd[8] = (BuferSize & 0xff);         //byte56
	ncmd[9] = ((BuferSize>>8) & 0xff);    //byte57
	ncmd[10] = ((BuferSize>>16) & 0xff);  //byte58
	ncmd[11] = ((BuferSize>>24) & 0xff);  //byte59
	return MyIssueNVMeCmd(ncmd, 0xD1, NVME_DIRCT_WRITE, buf, BuferSize, 15, 0);
}

int phisonNvmeCover2Nvme(unsigned char *phisonNvme, int direct, int OPCODE, int bufLength, NVMe_COMMAND * standNvme)
{
	ULONG * pNvme = (ULONG  * )standNvme;
	UINT tmp_long;

	standNvme->CDW0.OPC = OPCODE;

	if (OPCODE==ADMIN_IDENTIFY)
	{
		standNvme->NSID =0;
		//if ( 0x5007==(deviceType&0x5007) ) {
		//   pCmd->NSID =1;
		//}
	}
	else if (OPCODE==ADMIN_FIRMWARE_IMAGE_DOWNLOAD)
	{
		pNvme[10]=(bufLength/4)-1;
		pNvme[11]=0; //only for burner.
	}
	else if (OPCODE==NVM_WRITE || OPCODE==NVM_READ || OPCODE==NVM_FLUSH)
	{
		pNvme[11]=0;
		standNvme->NSID =1;
		if (OPCODE==NVM_WRITE)
		{

			tmp_long = ((*(phisonNvme+3)&0xFF)<<24)+((*(phisonNvme+2)&0xFF)<<16)+((*(phisonNvme+1)&0xFF)<<8)+((*(phisonNvme)&0xFF));
			pNvme[10]=(ULONG)tmp_long;
			tmp_long = ((*(phisonNvme+7)&0xFF)<<24)+((*(phisonNvme+6)&0xFF)<<16)+((*(phisonNvme+5)&0xFF)<<8)+((*(phisonNvme+4)&0xFF));
			//pMyIoctl->NVMeCmd[11]=(ULONG)((tmp_long>>32)&0xffffffff);
			tmp_long = ((*(phisonNvme+11)&0xFF)<<24)+((*(phisonNvme+10)&0xFF)<<16)+((*(phisonNvme+9)&0xFF)<<8)+((*(phisonNvme+8)&0xFF));
			pNvme[12]=(ULONG)((tmp_long-1)&0xffff);
		}
		else if (OPCODE==NVM_READ)
		{
			tmp_long = ((*(phisonNvme+3)&0xFF)<<24)+((*(phisonNvme+2)&0xFF)<<16)+((*(phisonNvme+1)&0xFF)<<8)+((*(phisonNvme)&0xFF));
			pNvme[10]=(ULONG)tmp_long;
			tmp_long = ((*(phisonNvme+7)&0xFF)<<24)+((*(phisonNvme+6)&0xFF)<<16)+((*(phisonNvme+5)&0xFF)<<8)+((*(phisonNvme+4)&0xFF));
			//pMyIoctl->NVMeCmd[11]=(ULONG)((tmp_long>>32)&0xffffffff);
			tmp_long = ((*(phisonNvme+11)&0xFF)<<24)+((*(phisonNvme+10)&0xFF)<<16)+((*(phisonNvme+9)&0xFF)<<8)+((*(phisonNvme+8)&0xFF));
			pNvme[12]=(ULONG)((tmp_long-1)&0xffff);
		}
	}
	else if (direct==0)
	{
	}
	else if (direct==1)
	{
		pNvme[10]=(bufLength/4);
	}
	else if (direct==2)
	{
		pNvme[10]=(bufLength/4);
	}

	// Fill CMD Register
	// Vcmd 48~63
	if ( OPCODE==0xC0 || OPCODE==0xC1 || OPCODE==0xC2 || OPCODE==0xD0 || OPCODE==0xD1 || OPCODE==0xD2 )
	{
		int i=0,j=0;
		for(i=0,j=12; i<16; i+=4)
		{
			tmp_long = ((*(phisonNvme+i+3)&0xFF)<<24)+((*(phisonNvme+i+2)&0xFF)<<16)+((*(phisonNvme+i+1)&0xFF)<<8)+((*(phisonNvme+i)&0xFF));
			pNvme[j] = tmp_long;
			j++;
		}
	}
	return 0;
}

int PCIESCSI5013_ST::MyIssueNVMeCmd( unsigned char *nvmeCmd,unsigned char OPCODE,int direct /*NVME_DIRECTION_TYPE*/,unsigned char *buf,ULONG bufLength,ULONG timeOut, bool silence )
{
	int connector = _deviceType&INTERFACE_MASK;
	
	if ( ( IF_ATA==connector) ||  ( IF_C2012==connector)  ||  ( IF_ASMedia==connector) ) {
		NVMe_COMMAND nvme_stand;
		unsigned char * nvme64 = (unsigned char * )&nvme_stand;
		memset(&nvme_stand, 0, sizeof(nvme_stand));
		phisonNvmeCover2Nvme(nvmeCmd, direct, OPCODE, bufLength, &nvme_stand);
		unsigned char PreATARegister[8]={0},ResultATARegister[8]={0};
		//if (0x6F == nvme64[52] && 0xFE == nvme64[53] && 0xEF == nvme64[54]){
		//	if (ATA_SetAPKey())
		//		return 1;
		//	else
		//		return 0;

		//}

		if (ATA_SetAPKey())
			return 1;

		//push nvme cmd 
		unsigned char trigger_cdb[16] = {0};
		unsigned char nvme_cmd_buf[512] = {0};
		trigger_cdb[0] = 0xF1;
		trigger_cdb[1] = 0x01;
		trigger_cdb[2] = 0x00;
		trigger_cdb[3] = 0x00;
		trigger_cdb[4] = 0x00;
		trigger_cdb[5] = 0xA0;
		trigger_cdb[6] = 0x31;
		memcpy(nvme_cmd_buf, nvme64, 64);

		if (MyIssueATACmd(PreATARegister, trigger_cdb, ResultATARegister, 2, nvme_cmd_buf, 512, true, false, _timeOut, false))
		{
			return 1;
		}
		if (ATA_SetAPKey())
			return 1;


		if (0 == direct) {//NON
			unsigned char ata_trigger_cdb[16] = {0};
			unsigned char empty_data[512] = {0};
			ata_trigger_cdb[0] = 0xF2;
			ata_trigger_cdb[1] = 0x01;
			ata_trigger_cdb[2] = 0x00;
			ata_trigger_cdb[3] = 0x00;
			ata_trigger_cdb[4] = 0x00;
			ata_trigger_cdb[5] = 0xA0;
			ata_trigger_cdb[6] = 0x31;
			if (MyIssueATACmd(PreATARegister, ata_trigger_cdb, ResultATARegister, 2, empty_data, 512, true, false, _timeOut, false))
			{
				return 1;
			}

		} else if (2 == direct) {//Write
			unsigned char ata_write_cdb[16] = {0};
			ata_write_cdb[0] = 0xF2;
			ata_write_cdb[1] = (bufLength/512)&0xff;
			ata_write_cdb[2] = 0x00;
			ata_write_cdb[3] = 0x00;
			ata_write_cdb[4] = 0x00;
			ata_write_cdb[5] = 0xA0;
			ata_write_cdb[6] = 0x31;
			if (MyIssueATACmd(PreATARegister, ata_write_cdb, ResultATARegister, 2, buf, bufLength, true, false, _timeOut, false))
			{
				return 1;
			}

		} else if (1 == direct) {//Read
			unsigned char ata_read_cdb[16] = {0};
			ata_read_cdb[0] = 0xF3;
			ata_read_cdb[1] = (bufLength/512)&0xff;
			ata_read_cdb[2] = 0x00;
			ata_read_cdb[3] = 0x00;
			ata_read_cdb[4] = 0x00;
			ata_read_cdb[5] = 0xE0;
			ata_read_cdb[6] = 0x21;
			if (MyIssueATACmd(PreATARegister, ata_read_cdb, ResultATARegister, 1, buf, bufLength, true, false, _timeOut, false))
			{
				return 1;
			}
		} else {
			assert(0);
			return 1;
		}
		return 0;
	}
	else {
		return _static_MyIssueNVMeCmd( nvmeCmd,OPCODE,direct ,buf,bufLength,timeOut, silence );
	}
}
int PCIESCSI5013_ST::Dev_SetCtrlODT (int odt_idx)
{
	/*
    0    150 ohm
    1    100 ohm
    2    75 ohm
    3    50 ohm
	*/
	unsigned char ncmd[16];
	memset(ncmd,0x00,16);
	ncmd[0] = 0x55;//nvme 48byte	
	ncmd[1] = odt_idx;//nvme 49byte
	UCHAR buf[16];
	memset(buf,0x00,16);
	if(MyIssueNVMeCmd(ncmd,0xD1, NVME_DIRCT_WRITE,buf,sizeof(buf),15, 0)){
		return -1;
	}
	return 0;
}

bool PCIESCSI5013_ST::InitDrive(unsigned char drivenum, const char* ispfile, int swithFW  )
{
	char auto_select_rdt_file[256] = {0};
	char auto_select_burner_file[256] = {0};

	if (!ispfile)
	{
		GetBurnerAndRdtFwFileName(_fh, auto_select_burner_file, auto_select_rdt_file);
	}
	return Dev_InitDrive(ispfile?ispfile:auto_select_burner_file)?true:false;
}

/*This is E13 default boot code search code block rule, refer from larry_huang@phison.com*/
//                                      1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32
extern int BOOT_CODE_BLK_E13[7][32]={{  0,  1,  2,  4,  8, 16, 32, 64,128,  3,  6, 12, 24, 48, 96,192,256,  5, 10, 20, 40, 80,160,320,384,  7, 14, 28, 56,112,224,448},//PAGE_BIT_WIDTH_7
{  0,  1,  2,  4,  8, 16, 32, 64,  3,  6, 12, 24, 48, 96,128,  5, 10, 20, 40, 80,160,192,  7, 14, 28, 56,112,224, -1, -1, -1, -1},//PAGE_BIT_WIDTH_8 //Bics3,Bics2
{  0,  1,  2,  4,  8, 16, 32,  3,  6, 12, 24 ,48, 64,  5, 10, 20, 40, 80, 96,  7, 14, 28, 56,112, -1, -1, -1, -1, -1, -1, -1, -1},//PAGE_BIT_WIDTH_9 //Bics4,
{  0,  1,  2,  4,  8, 16,  3,  6, 12, 24, 32,  5, 10, 20, 40, 48,  7, 14, 28, 56, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},//PAGE_BIT_WIDTH_10
{  0,  1,  2,  4,  8,  3,  6, 12, 16,  5, 10, 20, 24, 48,  7, 14, 28, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},//PAGE_BIT_WIDTH_11
{  0,  1,  2,  4,  8,  3,  6, 12, 16,  5, 10, 20, 24, 48,  7, 14, 28, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},//PAGE_BIT_WIDTH_12
{  0,  1,  2,  3,  4,  5,  6,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};//PAGE_BIT_WIDTH_13


void * PCIESCSI5013_ST::GetBootCodeRule(void)
{
	return (void*)&BOOT_CODE_BLK_E13[0][0];
}


void PCIESCSI5013_ST::SetAlwasySync(BOOL sync)
{

}

int PCIESCSI5013_ST::ST_GetCodeVersionDate(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char* buf)
{
	return 0;
}

int PCIESCSI5013_ST::NT_SwitchCmd(void)
{
	return 0;
}

int PCIESCSI5013_ST::ST_ScanBadBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int CE, unsigned char *w_buf,BOOL isCheckSpare00, unsigned int DieOffSet, unsigned int BlockPerDie)
{
	unsigned char cdb512[512] = {0};
	cdb512[0] = 0x06;
	cdb512[1] = 0xF6;
	cdb512[2] = 0x14;
	cdb512[3] = GetCeMapping(CE);
	cdb512[4] = DieOffSet & 0xFF;
	cdb512[5] = (DieOffSet>>8) & 0xFF;
	cdb512[6] = BlockPerDie & 0xFF;
	cdb512[7] = (BlockPerDie>>8) & 0xFF;

	return Issue512CdbCmd(cdb512, w_buf, 4096, SCSI_IOCTL_DATA_IN, 30);
}

int PCIESCSI5013_ST::ST_SetLDPCIterationCount(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk ,int count)
{//TODO
	return 0;
}

int PCIESCSI5013_ST::NT_Inquiry_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf, unsigned char *DriveInfo)
{
	return UFWInterface::_static_Inquiry(DeviceHandle_p, bBuf, true);
}

int PCIESCSI5013_ST::NT_CustomCmd(char targetDisk, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut)
{
	unsigned char cdb512[512] = {0};
	memcpy(cdb512, CDB, 16);
	if (NVMe_SetAPKey())
		return -1;
	return Issue512CdbCmd(cdb512, buf, iDataLen, direc, timeOut);
}

int PCIESCSI5013_ST::GetSDLL_Overhead()
{
	return 26;
}

bool Check5013CDBisSupport(unsigned char *CDB)
{
	if((CDB[0] != 0x06) || (CDB[1] != 0xF6))
		return FALSE;
	for(int i=0; i<OPCMD_COUNT_5013; i++) {
		if(CDB[2] == OpCmd5013[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

bool check_PE_MARK(unsigned char * input_buf, int length)
{
	char PE_MARK[] = "PE BLOCK";
	int mark_str_len = strlen(PE_MARK);
	int ren = length/mark_str_len;
	for (int idx = 0; idx < ren; idx++)
	{
		if (!memcmp(&input_buf[idx*mark_str_len], &PE_MARK[0], mark_str_len))
		{
			return true;
		}
	}
	return false;
}

bool PCIESCSI5013_ST::CheckIs_PE_Block(USBSCSICmd_ST *cmd, HANDLE handle, unsigned char * ap_parameter, FH_Paramater *fh,int ce_idx, int phy_block)
{
	bool isPE_Block = false;
	int page_size_alignK = ((fh->PageSize+1023)/1024)*1024;
	unsigned char * read_buf = new unsigned char[page_size_alignK];
	cmd->SetDumpWriteReadAPParameter(handle, ap_parameter);
	cmd->ST_DirectPageRead(1, handle, 0, 0/*RWMode*/, ce_idx, phy_block, 1/*page_idx*/, 1/*page_cnt*/, 0, 0, 0, 0, page_size_alignK, read_buf, 0 );

	if (check_PE_MARK(read_buf , page_size_alignK))
	{
		isPE_Block = true;
	}
	else
	{
		isPE_Block = false;
	}
	cmd->ST_SetGet_Parameter(1, handle, 0, true, ap_parameter);
	if (read_buf)
		delete [] read_buf;
	return isPE_Block;
}

int	PCIESCSI5013_ST::Mark_PE_Blocks(USBSCSICmd_ST * cmd,  HANDLE handle, unsigned char * ap_parameter ,FH_Paramater * fh ,int ce_idx, int phy_block)
{
	char PE_MARK[] = "PE BLOCK";
	int mark_len = strlen(PE_MARK);
	cmd->ST_DirectOneBlockErase(1, handle, 0, true, 0, phy_block, 0, 0, 0, 0);
	cmd->SetDumpWriteReadAPParameter(handle ,ap_parameter);
	int page_size_alignK = ((fh->PageSize+1023)/1024)*1024;
	unsigned char * pageBuf = new unsigned char[page_size_alignK];
	unsigned char * pageBuf_verify = new unsigned char[page_size_alignK];
	memset(pageBuf_verify, 0, page_size_alignK);
	int ren = page_size_alignK/mark_len;
	Utilities u;
	int total_diff = 0;

	/*fill buffer by PE MARK*/
	for (int i = 0; i<ren ; i++)
	{
		memcpy(&pageBuf[i*mark_len], PE_MARK, mark_len);
	}

	cmd->ST_DirectOneBlockErase(1, handle, 0, true, ce_idx, phy_block, 0, 0, 0, 0);
	cmd->ST_DirectPageWrite(1, handle, 0, 0/*RWMode*/, ce_idx, phy_block, 1/*Page 1*/, 1/*page cnt*/, 0, 0 ,0 ,page_size_alignK, 0, pageBuf, 0, 0);
	cmd->ST_DirectPageRead(1, handle, 0, 0/*RWMode*/, ce_idx, phy_block, 1/*Page 1*/, 1, 0, 0, 0, 0, page_size_alignK, pageBuf_verify, 0 );
	u.DiffBitCheckPerK(pageBuf, pageBuf_verify,fh->PageSizeK*1024, 10, NULL, &total_diff);
	cmd->nt_file_log("Mark PE CE%d PhyBlock% Page%d: Diff=%d", ce_idx, phy_block, 1, total_diff);

	delete [] pageBuf;
	delete [] pageBuf_verify;
	cmd->ST_SetGet_Parameter(1, handle, 0, true, ap_parameter);
	return 0;
}



int PCIESCSI5013_ST::List_PE_Block(USBSCSICmd_ST *cmd, HANDLE handle, unsigned char * ap_parameter,  FH_Paramater *fh, int ce_idx, unsigned short * pe_phy_blocks_list, int max_pe_cnt)
{
	int found_cnt = 0;
	int FIND_SEGAMENT_CNT=50;
	cmd->SetDumpWriteReadAPParameter(handle, ap_parameter);
	int page_size_alignK = ((fh->PageSize+1023)/1024)*1024;
	unsigned char * read_buf = new unsigned char[page_size_alignK];
	unsigned short physical_block = 0;

	for (int find_group=0; find_group<3; find_group++)//find for header, mid, tail
	{
		for (int die_idx=0; die_idx<fh->DieNumber; die_idx++)
		{
			for (int find_idx=0; find_idx<FIND_SEGAMENT_CNT;  find_idx++)
			{	

				if (0 == find_group)
				{
					physical_block =  die_idx*fh->DieOffset + find_idx;
				}
				else if (1 == find_group)
				{
					physical_block = die_idx*fh->DieOffset + find_idx + fh->Block_Per_Die/2;
				}
				else {
					physical_block = die_idx*fh->DieOffset + fh->Block_Per_Die -1 - find_idx;
				}
				cmd->ST_DirectPageRead(1, handle, 0, 0/*RWMode*/, ce_idx, physical_block, 1/*page_idx*/, 1/*page_cnt*/, 0, 0, 0, 0, page_size_alignK, read_buf, 0 );
				if (check_PE_MARK(read_buf , page_size_alignK))
				{
					cmd->nt_file_log("found PE BLOCK, ce=%d, phycial block=%d", ce_idx, physical_block);
					pe_phy_blocks_list[found_cnt] = physical_block;
					found_cnt++;
					if (found_cnt>=max_pe_cnt)
						break;
				}
			}
		}
	}




	cmd->ST_SetGet_Parameter(1, handle, 0, true, ap_parameter);
	if (read_buf)
		delete [] read_buf;
	return found_cnt;

}

int PCIESCSI5013_ST::NT_CustomCmd_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool is512CDB)
{
	int userioTimeOut = GetPrivateProfileInt("Sorting","UserIOTimeOut", -1 ,_ini_file_name);
	if(userioTimeOut != -1) {
		timeOut = userioTimeOut;
	}

	unsigned char cdb512[512] = {0};
	if(!Check5013CDBisSupport(CDB)) {
		nt_file_log("CDB [0x%x 0x%x 0x%x 0x%x] not support.", CDB[0], CDB[1], CDB[2], CDB[3]);
		assert(0);
	}
	if (is512CDB)
		memcpy(cdb512, CDB, 512);
	else
		memcpy(cdb512, CDB, 16);
	if (NVMe_SetAPKey())
		return -1;
#if _DEBUG
	//nt_file_log("cdb512[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]", cdb512[0], cdb512[1], cdb512[2], cdb512[3], cdb512[4],
		//cdb512[5], cdb512[6], cdb512[7], cdb512[8], cdb512[9], cdb512[10], cdb512[11], cdb512[12], cdb512[13], cdb512[14], cdb512[15]);
#endif
	return Issue512CdbCmd(cdb512, buf, iDataLen, direc, timeOut);
}

int PCIESCSI5013_ST::Issue512CdbCmd(unsigned char * cdb512, unsigned char * iobuf, int lenght, int direction, int timeout) {
	int rtn = 0;
	unsigned char nvmeCmd[64] = {0};

	int opCode = 0;
	int nvmeCmdLenght = lenght + 512; //512 for cdb512
	nvmeCmdLenght = ((nvmeCmdLenght+511)/512)*512;
	int nvmeDirection = 0;//0: unspec. 1:In   2:Out
	unsigned char * nvmeBuf = new unsigned char[nvmeCmdLenght];
	memset(nvmeBuf, 0, nvmeCmdLenght);
	memcpy(nvmeBuf, cdb512, 512);
	memcpy(nvmeBuf + 512, iobuf, lenght);

	nvmeCmd[4] = (nvmeCmdLenght & 0xff);
	nvmeCmd[5] = ((nvmeCmdLenght>>8) & 0xff);
	nvmeCmd[6] = ((nvmeCmdLenght>>16) & 0xff);
	nvmeCmd[7] = ((nvmeCmdLenght>>24) & 0xff);

	if (SCSI_IOCTL_DATA_IN == direction && 0 ==lenght )
	{
		direction = SCSI_IOCTL_DATA_UNSPECIFIED;
	}

	if (SCSI_IOCTL_DATA_UNSPECIFIED == direction) {
		nvmeCmd[0] = 0x66;
		rtn = MyIssueNVMeCmd(nvmeCmd, 0xD1, NVME_DIRCT_WRITE, nvmeBuf, 512, timeout, 0);
	} else if (SCSI_IOCTL_DATA_IN == direction) {
		//put command for 512cdb.
		nvmeCmd[0] = 0x66;
		nvmeCmd[4] = (512 & 0xff);
		nvmeCmd[5] = ((512>>8) & 0xff);
		nvmeCmd[6] = ((512>>16) & 0xff);
		nvmeCmd[7] = ((512>>24) & 0xff);
		rtn = MyIssueNVMeCmd(nvmeCmd, 0xD1, NVME_DIRCT_WRITE, nvmeBuf, 512, timeout, 0);

		//then get result back.
		nvmeCmd[0] = 0x66;
		nvmeCmd[4] = (lenght & 0xff);
		nvmeCmd[5] = ((lenght>>8) & 0xff);
		nvmeCmd[6] = ((lenght>>16) & 0xff);
		nvmeCmd[7] = ((lenght>>24) & 0xff);
		rtn = MyIssueNVMeCmd(nvmeCmd, 0xD2, NVME_DIRCT_READ, iobuf, lenght, timeout, 0);
	} else if (SCSI_IOCTL_DATA_OUT == direction) {
		nvmeCmd[0] = 0x66;
		rtn = MyIssueNVMeCmd(nvmeCmd, 0xD1, NVME_DIRCT_WRITE, nvmeBuf, nvmeCmdLenght, timeout, 0);
	} else {
		assert(0);
	}

	if (nvmeBuf)
		delete [] nvmeBuf;

	if (rtn)
		rtn = -2;

	return rtn;
}

int PCIESCSI5013_ST::NT_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle, char targetDisk , unsigned char *buf, bool bGetUID,BYTE full_ce_info,BYTE get_ori_id)
{

	int rtn=0;
	FILE * fp =fopen("FAKE_FLASH_ID.bin", "rb");

	if (fp)
	{
		fread(buf, 1, 512, fp);
		fclose(fp);
		return 0;
	}
	NVMe_SetAPKey();
	unsigned char ncmd[16];
	UINT GetLength=16;
	memset(ncmd,0x00,16);
	unsigned char buf2[512] = {0};

	ncmd[0] = 0x90;	//Byte48
	ncmd[1] = 0x00;
	ncmd[4] = 0x00;         //Byte52

	log_nvme("%s\n", __FUNCTION__);


	bool isGetID_FromBootCodeONLY = GetPrivateProfileInt("Sorting","E13GetFlashIDFromBootONLY", 0, _ini_file_name);
	Utilities u;

	bool isFromCache = ( (0!= u.Count1(_flash_id_boot_cache,512)) && isGetID_FromBootCodeONLY);

	if (isFromCache)
	{
		memcpy(buf, _flash_id_boot_cache, 512);
		nt_file_log("E13 get ID from cache\n");
	}
	else
	{
		if (_fwVersion[2] != 'B') {
			//Not in Burner
			rtn = MyIssueNVMeCmd(ncmd,0xD2,1,buf,512,15, false);
			for (int ce = 0;ce<32; ce++) {
				memcpy(buf2+(16*ce),buf+(8*ce),8);
			}
			memcpy(buf,buf2,512);
		} else {
			//In Burner
			unsigned char cdb512[512] = {0};
			cdb512[0] = 0x06;
			cdb512[1] = 0x56;
			Issue512CdbCmd(cdb512, buf, 512, SCSI_IOCTL_DATA_IN, _SCSI_DelayTime);
		}
		memcpy(_flash_id_boot_cache, buf, 512);

		if (isGetID_FromBootCodeONLY)
		{
			nt_file_log("cache ID from boot code\n");
			LogHex(_flash_id_boot_cache, 512);
		}

	}


	if (-1 != _ActiveCe){
		int exist_ce_addr[32]={0};//keep start addrs it have id number
		int ce_addr_index =0;
		for (int ce = 0, ce_addr_index=0;ce<32; ce++){
			if (buf[16*ce] && buf[16*ce+1] && (0xFF != buf[16*ce])){
				exist_ce_addr[ce_addr_index]=16*ce;
				ce_addr_index++;
			}
		}
		memcpy(buf2,buf,512);
		memset(buf,0,512);
		memcpy(buf,&buf2[exist_ce_addr[_ActiveCe]],16);
	}

	return rtn;
}

int PCIESCSI5013_ST::HandleL3_QC_page( USBSCSICmd_ST * cmd, unsigned char * input_parameter)
{

	int ce_idx=0;
	int die_idx=0;
	int PICK_GROUP = 3;
	int isDataValid = true;
	int BootCodeSearchRule[7][32];
	int select_idx=1;
	int codeBlock = 0;
	HANDLE h = cmd->Get_SCSI_Handle();

	memcpy(BootCodeSearchRule, this->GetBootCodeRule(), sizeof(BootCodeSearchRule));
	int page_size_alignK = (_FH.PageSize+1023)/1024*1024;
	unsigned char * page_buffer = new unsigned char[page_size_alignK];
	unsigned char * L3_qc_buffer = new unsigned char[E13_L3_QC_PAGE_SEGEMENT_LENGT];
	int block_fail_cnt = 0;
	unsigned short blkInfo =  0;
	unsigned short blkFailTask =  0;
	cmd->_L3_total_fail_cnt_PE = 0;
	cmd->_L3_total_fail_cnt_NON_PE = 0;

	Utilities u;
	cmd->SetDumpWriteReadAPParameter(h, input_parameter);
	int isCE_Found_QC = false;
	for (int ce_idx = 0; ce_idx < _FH.FlhNumber; ce_idx++)
	{
		for (int search_idx=0; search_idx<32; search_idx++) {
			codeBlock = BootCodeSearchRule[select_idx][search_idx];
			if(-1==codeBlock)
				continue;

			int qc_page = _FH.MaxPage-1;
			cmd->ST_DirectPageRead(1, h, 0, 0/*RWMode*/, ce_idx, codeBlock, qc_page , 1, 0, 0, 0, 0, page_size_alignK, page_buffer, 0 );
			if (0 == u.FindRightData(page_buffer, E13_L3_QC_PAGE_SEGEMENT_LENGT, page_size_alignK, L3_qc_buffer)) {
				//found valid data
				isCE_Found_QC++;
				cmd->nt_file_log("List L3 Block:CE%d  Die%d", ce_idx, die_idx);
				cmd->LogHex(L3_qc_buffer, E13_L3_QC_PAGE_SEGEMENT_LENGT);
				for (int die_idx=0; die_idx<_FH.DieNumber; die_idx++)
				{
					L3_BLOCKS_PER_CE * L3_blks =      (L3_BLOCKS_PER_CE *)&L3_qc_buffer[0];
					unsigned char * fail_addr_start = L3_qc_buffer + 0x180;
					L3_BLOCKS_PER_CE * L3_fail_task = (L3_BLOCKS_PER_CE *)fail_addr_start;

					/*-----PE BLOCK-----------------*/
					for (int g_idx = 0; g_idx < PICK_GROUP; g_idx++)
					{
					
						for (int plane_idx = 0; plane_idx < _FH.Plane; plane_idx++)
						{
							int blk_addr_in_die = g_idx * 8/*Max support planeX2 */ + plane_idx;
							blkInfo = L3_blks->die[die_idx][blk_addr_in_die];
							
							if (0xFFFF == blkInfo)
								continue;
							int isFailed = 0x8000&blkInfo;
							if (isFailed) {
								blkFailTask = L3_fail_task->die[die_idx][blk_addr_in_die];
								cmd->nt_file_log("  PE Block:%d Fail, Task=%d", (0x7FFF&blkInfo), blkFailTask);
								cmd->_L3_total_fail_cnt_PE++;
							}
							else {
								cmd->nt_file_log("  PE Block:%d", (0x7FFF&blkInfo), blkFailTask);
							}
						}//plane loop
					}//pick group loop

					/*-----NON PE BLOCK-----------------*/
					for (int g_idx = 0; g_idx < PICK_GROUP; g_idx++)
					{

						for (int plane_idx = 0; plane_idx < _FH.Plane; plane_idx++)
						{
							int blk_addr_in_die = g_idx * 8/*Max support planeX2 */ + plane_idx + 4/*shift Max 4 Plane*/;
							blkInfo = L3_blks->die[die_idx][blk_addr_in_die];

							if (0xFFFF == blkInfo)
								continue;

							int isFailed = 0x8000&blkInfo;
							if (isFailed) {
								blkFailTask = L3_fail_task->die[die_idx][blk_addr_in_die];
								cmd->nt_file_log("  NON_PE Block:%d Fail, Task=%d", (0x7FFF&blkInfo), blkFailTask);
								cmd->_L3_total_fail_cnt_NON_PE++;
							}
							else {
								cmd->nt_file_log("  NON_PE Block:%d", (0x7FFF&blkInfo), blkFailTask);
							}
						}//plane loop
					}//pick group loop


				}//die loop
				break; //break boot code search due to already found QC
			} 
			else {
				//can't find valid data, up to codeBlk loop
			}
		}

	}
	//recovery setting
	cmd->ST_SetGet_Parameter(1, h, 0, true, input_parameter);

	cmd->nt_file_log("Total fail count:PE=%d, NON_PE=%d", cmd->_L3_total_fail_cnt_PE, cmd->_L3_total_fail_cnt_NON_PE);
	if (page_buffer)
		delete [] page_buffer;

	if (isCE_Found_QC != _FH.FlhNumber)
	{
		cmd->_L3_total_fail_cnt_NON_PE=-1;
		cmd->_L3_total_fail_cnt_PE=-1;
		cmd->nt_file_log("Some CE miss QC report %d/%d", isCE_Found_QC, _FH.FlhNumber);
		return -1;
	}
	return 0;
}

int PCIESCSI5013_ST::OverrideOfflineInfoPage(USBSCSICmd_ST *cmd, unsigned char * info_buf)
{
	int targetSortingTemperature = GetPrivateProfileInt("E13", "Temperature", -999, _ini_file_name);
	bool isCheckCTL_temperature = GetPrivateProfileInt("E13", "isCheckCTL_temperature", 1, _ini_file_name)?true:false;

	cmd->nt_file_log("Offline Target Temperature=%d, isCheckCTL=%d", targetSortingTemperature, isCheckCTL_temperature);
	int info_idx = 0;
	if (-999 != targetSortingTemperature)
	{
		if (isCheckCTL_temperature)
		{
			targetSortingTemperature += 15;
		}
		do
		{
			if (TASK_ID_WAIT_TEMPERATURE == info_buf[info_idx])
			{
				info_buf[info_idx+1] = targetSortingTemperature;
				info_idx += 3; //1 tag 2 parameters
				continue;
			}
			else if (TASK_ID_LOOP_START == info_buf[info_idx])
			{
				info_idx += 5; //1 tag 4 parammters
				continue;
			}
			info_idx++;
		} while (info_idx < 4096);
	}

	/********************FOLLOW TAKE CARE D3/D4 CACHE WRITE MODE*******************/
	info_idx = 0;
	enum
	{
		NON_SPECIFY,
		FORCE_CACHE,
		FORCE_NON_CACHE
	};
	int D3D4_Cache_program_mode =  GetPrivateProfileInt("E13", "D3D4CacheWriteMode", NON_SPECIFY, _ini_file_name);
	if ( (NON_SPECIFY == D3D4_Cache_program_mode) && (  _fh->bIsMIcronN18 || -_fh->bIsMIcronN28))
	{
		D3D4_Cache_program_mode = FORCE_NON_CACHE;
	}
	do
	{
		if (  (FORCE_NON_CACHE == D3D4_Cache_program_mode)  &&  (TASK_MP_CACHE_WRITE == info_buf[info_idx])  )
		{
			info_buf[info_idx] = TASK_MP_WRITE;
		}
		else if (  (FORCE_CACHE == D3D4_Cache_program_mode)  &&  (TASK_MP_WRITE == info_buf[info_idx])  )
		{
			info_buf[info_idx] = TASK_MP_CACHE_WRITE;
		}
		info_idx++;
	} while (info_idx < 4096);

	cmd->nt_file_log("Offline INFO Page:");
	cmd->LogHex(info_buf, 4096);
	return 0;
}

void PCIESCSI5013_ST::GetPossiblePEblockList(int * pe_list, int array_len, FH_Paramater * fh)
{
	int PE_BLOCK_LIST[100] = {0};
	memset(PE_BLOCK_LIST, 0xFF, sizeof(PE_BLOCK_LIST));
	int list_cnt=0;


	int half_dieblock_idx = fh->Block_Per_Die/2;
	for (int i=0; i<30; i++)
	{
		PE_BLOCK_LIST[list_cnt++] = i;
	}

	for (int i=0; i<30; i++)
	{
		PE_BLOCK_LIST[list_cnt++] = half_dieblock_idx+i;
	}

	for (int i=0; i<30; i++)
	{
		PE_BLOCK_LIST[list_cnt++] = fh->Block_Per_Die-1-i;
	}

	memcpy(pe_list, PE_BLOCK_LIST, sizeof(int)*array_len);

}

int PCIESCSI5013_ST::PrepareInFormationPage(USBSCSICmd_ST  * inputCmd, FH_Paramater * fh, unsigned char * task_buf, char * app_version, unsigned char * retry_buf_d1, unsigned char * retry_buf)
{
	if (!inputCmd || !task_buf)
	{
		assert(0);
		return -1;
	}
	int * PE_blk_Cnt = (int * )&task_buf[PE_BLOCK_LIST_START_ADDR];
	if (*PE_blk_Cnt == -1)
	{
		assert(0);
		(*PE_blk_Cnt) = 0;
	}

	if ( 0 != memcmp(&task_buf[0x2D6], "INFOBLOCK", 9) )
	{
		assert(0);
		return -1;
	}
	//settings from ini
	int isEnableRetry   = GetPrivateProfileInt("E13", "RetryRead",1, _ini_file_name);
	int normal_read_ecc_threshold_d1     = GetPrivateProfileInt("E13", "EccThresholdD1", 60, _ini_file_name);
	int retry_read_ecc_threshold_d1 = GetPrivateProfileInt("E13", "EccThresholdRRD1", 50, _ini_file_name);
	int normal_read_ecc_threshold     = GetPrivateProfileInt("E13", "EccThreshold", 60, _ini_file_name);
	int retry_read_ecc_threshold = GetPrivateProfileInt("E13", "EccThresholdRR",50, _ini_file_name);
	task_buf[INFOBLOCK_RETRY_KEEP_LEVEL] = GetPrivateProfileInt("E13", "KeepRetryLevel", 0, _ini_file_name);
	task_buf[INFOBLOCK_SCAN_WIN_THRESHOLD] = GetPrivateProfileInt("E13", "ScanWindowsThreshold", 0, _ini_file_name);

	task_buf[INFOPAGE_PAGE_CNT_PER_RECORD] = _fh->DieNumber + 2/*one for detial page, one for other page*/;
	task_buf[INFOBLOCK_ENABLE_RETRY] = isEnableRetry;
	task_buf[INFOBLOCK_RETRY_FROM] = 1;//retry table from app
	task_buf[INFOBLOCK_ECC_THRESHOLD_D1] = normal_read_ecc_threshold_d1;
	task_buf[INFOBLOCK_ECC_THRESHOLD_RR_D1] = retry_read_ecc_threshold_d1;
	task_buf[INFOBLOCK_ECC_THRESHOLD] = normal_read_ecc_threshold;
	task_buf[INFOBLOCK_ECC_THRESHOLD_RR] = retry_read_ecc_threshold;

	task_buf[INFOBLOCK_RETRY_GROUPS_CNT_D1] = retry_buf_d1[0];
	task_buf[INFOBLOCK_RETRY_LEN_PER_GROUP_D1] = retry_buf_d1[1];
	task_buf[INFOBLOCK_RETRY_GROUPS_CNT] = retry_buf[0];
	task_buf[INFOBLOCK_RETRY_LEN_PER_GROUP] = retry_buf[1];


	int retry_table_size_bytes_d1 = retry_buf_d1[0]*retry_buf_d1[1];
	if (retry_table_size_bytes_d1>RETRYTABLE_MAX_LENGTH_D1)
	{
		assert(0);
		retry_table_size_bytes_d1 = RETRYTABLE_MAX_LENGTH_D1;
	}

	int retry_table_size_bytes = retry_buf[0]*retry_buf[1];
	if (retry_table_size_bytes>RETRYTABLE_MAX_LENGTH)
	{
		assert(0);
		retry_table_size_bytes = RETRYTABLE_MAX_LENGTH;
	}

	memcpy(&task_buf[INFOBLOCK_RETRYTABLE_D1], retry_buf_d1+2, retry_table_size_bytes_d1 );
	memcpy(&task_buf[INFOBLOCK_RETRYTABLE], retry_buf+2, retry_table_size_bytes);

	task_buf[INFOPAGE_CODEBODY_LDPC_MODE] =  PCIESCSI5013_ST::SelectLdpcModeByPageSize(fh->PageSize);
	memcpy(&task_buf[INFOBLOCK_AP_VERSION], (unsigned char *)&app_version[0], 16);

	OverrideOfflineInfoPage( inputCmd, task_buf);
	return 0;

}

//#include "IDPageAndCP_Hex_5013_ST.cpp"
int PCIESCSI5013_ST::WriteRDTCode(char * rdt_fileName, char * output_infopage_filename, CNTCommand * inputCmd,
									  unsigned char * input_ap_parameter, unsigned char * input_4k_buf, char * ap_version, int userSpecifyBlkCnt, int clk,
									  int flh_interface, BOOL isEnableRetry, unsigned char * d1_retry_buffer, unsigned char * retry_buffer,
									  int normal_read_ecc_threshold_d1, int retry_read_ecc_threshold_d1,
									  int normal_read_ecc_threshold   , int retry_read_ecc_threshold, bool isSQL_groupRW)
{
	int rtn = 0;
	int file_size = 0, offset=0;
	int verify_fail = 0;
	bool is8K_InfoPage = GetPrivateProfileInt("E13", "InFoPage8K", 1, _ini_file_name);
	bool isWritingINFO_Page = false;
	char rdtFileNameRuled[128] = {0};
	char burnerFileNameRuled[128] = {0};
	FILE * fp = NULL;
	unsigned char * file_buf = NULL;
	unsigned char * page_buf = NULL;
	unsigned char * page_buf_verify = NULL;

	Utilities u;
	unsigned short blk[8],TPgCnt,SectionMaxSecCnt=0;
	unsigned char IDPBuf[4096],CPBuf[4096],section=0,FrameNo,WriteSec;//test
	unsigned int TmpCRC;
	//User need to set ap parameter to dump write read first!
	HANDLE handle = inputCmd->Get_SCSI_Handle();

	PCIESCSI5013_ST * input5013Cmd = (PCIESCSI5013_ST * )inputCmd;
	SQLCmd * sqlCmd = inputCmd->GetSQLCmd();

	int board1v2 = GetPrivateProfileInt("Configuration","1V2",0, _ini_file_name);
	FLH_ABILITY flh_ability = MyFlashInfo::GetFlashSupportInterFaceAndMaxDataRate(board1v2?12:18, _fh);

	int code_point_blk = 0;
	int addressGap = 0;

	int ldpc_mode = 0;

	int common_code_block = 0;
	int total_diff = 0;
	
	if ( (0 == flh_interface && (int)flh_ability.MaxClk_Legacy<GetDataRateByClockIdx(clk) ) ||
		 (1 == flh_interface && (int)flh_ability.MaxClk_T1<GetDataRateByClockIdx(clk) ) ||
         (2 == flh_interface && (int)flh_ability.MaxClk_T2<GetDataRateByClockIdx(clk) ) ||
		 (3 == flh_interface && (int)flh_ability.MaxClk_T3<GetDataRateByClockIdx(clk) ) )
	{
		inputCmd->nt_file_log("flash ability check fail: interface=%d clk=%d board1v2=%d", flh_interface, GetDataRateByClockIdx(clk), board1v2);
		inputCmd->nt_file_log("LegacyMax=%d, T1Max=%d, T2Max=%d, T3Max=%d", flh_ability.MaxClk_Legacy, flh_ability.MaxClk_T1, flh_ability.MaxClk_T2, flh_ability.MaxClk_T3);
		if (!GetPrivateProfileInt("E13", "SkipCheckFlashAbility", 0, _ini_file_name))
		{
			return 1;
		}
	}
	
	
	PrepareInFormationPage(inputCmd, _fh, input_4k_buf, (char *)"DEBUGTOOL123456789", d1_retry_buffer, retry_buffer);

	bool isOnly4Kdata = false;

	double total_w_clk=0;
	double total_r_clk=0;
	int page_size_alignK = ((_fh->PageSize+1023)/1024)*1024;
	/*prepare dummp data*/
	CharMemMgr dummyPage(page_size_alignK);
	dummyPage.Rand(true);
	memcpy(dummyPage.Data(), "DUMMY", 5);

	if (isSQL_groupRW && !sqlCmd) {
		rtn = -1;
		goto END;
	}

	memset(blk, 0xff, sizeof(blk));
	if (!rdt_fileName)
	{
		GetBurnerAndRdtFwFileName(_fh, burnerFileNameRuled, rdtFileNameRuled);
		fp = fopen(rdtFileNameRuled, "rb");
		inputCmd->nt_file_log("E13 RDT file name=%s",rdtFileNameRuled);

	}
	else
	{
		fp = fopen(rdt_fileName, "rb");
		inputCmd->nt_file_log("E13 RDT file name=%s",rdt_fileName);
	}

	if (!fp)
	{
		rtn = 1;
		goto END;
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	file_buf = new unsigned char[file_size];
	fread(file_buf,1,file_size, fp);
	fclose(fp);

	ParsingBinFile(file_buf);


	page_buf = new unsigned char[page_size_alignK];
	page_buf_verify = new unsigned char[page_size_alignK];
	common_code_block = 0;
	total_diff = 0;


	InitialCodePointer(CPBuf,blk);
	PrepareIDpage(IDPBuf, flh_interface, clk, userSpecifyBlkCnt);


	if (output_infopage_filename)
	{
		fp = fopen(output_infopage_filename, "wb");
		if (fp)
		{
			fwrite(input_4k_buf, 1, 4096, fp);
			fflush(fp);
			fclose(fp);
		}
	}

	InitLdpc();
	TmpCRC=_ldpc->E13_FW_CRC32((unsigned int*)IDPBuf, 4096);

	IDPBuf[0x14]=(unsigned char)TmpCRC;
	IDPBuf[0x15]=(unsigned char)(TmpCRC>>8);
	IDPBuf[0x16]=(unsigned char)(TmpCRC>>16);
	IDPBuf[0x17]=(unsigned char)(TmpCRC>>24);

	if (isSQL_groupRW)
	{
		inputCmd->Dev_SetFWSeed(0, 0x8299);
	}
	else
	{
		_ldpc->SetBlockSeed(0x8299);
	}
	TPgCnt = ((IDPBuf[0x25]<<8)+IDPBuf[0x24])+1+1;//will write page cnt
	SectionMaxSecCnt = (file_buf[0x81]<<8)+file_buf[0x80];

	inputCmd->nt_file_log("ID PAGE:");
	inputCmd->LogHex(&IDPBuf[0], 1024);

	ldpc_mode = IDPBuf[0x20];

	{//to found common code block
		inputCmd->SetDumpWriteReadAPParameter(handle, input_ap_parameter);
		for (int blk_idx = 0; blk_idx<128 && !isSQL_groupRW ; blk_idx++)
		{
			common_code_block = -1;
			for (int ce_idx=0; ce_idx<_fh->FlhNumber ; ce_idx++)
			{
				inputCmd->ST_DirectOneBlockErase(1, handle, 0, true, ce_idx, blk_idx, 0, 0, 0);
				for (int page_idx=0; page_idx<TPgCnt; page_idx++)
				{
					inputCmd->ST_DirectPageWrite(1, handle, 0, 0/*RWMode*/, ce_idx, blk_idx, page_idx, 1/*page cnt*/, 0, 0 ,0 ,page_size_alignK, 0, dummyPage.Data(), 0, 0);
					inputCmd->ST_DirectPageRead(1, handle, 0, 0/*RWMode*/, ce_idx, blk_idx, page_idx, 1, 0, 0, 0, 0, page_size_alignK, page_buf_verify, 0 );
					verify_fail = u.DiffBitCheckPerK(dummyPage.Data(), page_buf_verify, _fh->PageSizeK*1024, 10, NULL, &total_diff);
					if (total_diff) 
						inputCmd->nt_file_log("ce %2d ,blk %5d ,page %5d ,PageDiff=%d", ce_idx, blk_idx, page_idx ,total_diff);
					if (verify_fail) {
						break;
					}
				}
				inputCmd->ST_DirectOneBlockErase(1, handle, 0, true, ce_idx, blk_idx, 0, 0, 0);
				if (verify_fail) {
					break;
				}
			}
			if (!verify_fail)
			{
				common_code_block = blk_idx;
				break;
			}
		}
	}

	if (-1 == common_code_block)
	{
		inputCmd->nt_file_log("can't find vaild code block, abort");
		rtn = -1;
		goto END;
	}

	addressGap = MyFlashInfo::GetAddressGapBit(_fh);
	code_point_blk = common_code_block<<addressGap;
	/*need update code pointer for code block*/
	blk[0] = code_point_blk;
	blk[1] = code_point_blk;
	InitialCodePointer(CPBuf,blk);

	isOnly4Kdata = false;
	isWritingINFO_Page = false;
	
	for( int pageIdx = 0; pageIdx< TPgCnt; pageIdx++) {
		memset(page_buf,0x00, page_size_alignK);
		memset(page_buf_verify,0x00, page_size_alignK);

		if (isSQL_groupRW)
		{
			inputCmd->Dev_SetFWSeed(pageIdx%64, 0x8299);
			inputCmd->Dev_SetEccMode(7);
		}
		else
		{
			_ldpc->SetLDPCMode(7);	
			_ldpc->SetPageSeed(pageIdx%64);// mod 64
		}

		if (0 == pageIdx){// page 0 ==> IDPage
			isOnly4Kdata = true;
			memcpy(page_buf,IDPBuf,4096);

			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'I',1,0, L4K_BUF_SIZE_WR);
				input5013Cmd->Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32*4, true);
			}
			else
			{
				GenE13Spare(&page_buf[_fh->PageSizeK*1024],'I',1,0);
				_ldpc->GenRAWData(&page_buf[0],4112, _fh->PageSizeK);
			}

		} else if (1 == pageIdx) {// Page 1 ==> Code sign header
			isOnly4Kdata = true;
			memcpy(page_buf,file_buf,1024);	// for same with burner do
			offset = 512;

			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'H',1,0, L4K_BUF_SIZE_WR);
				input5013Cmd->Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32*4, true);
			}
			else
			{
				GenE13Spare(&page_buf[_fh->PageSizeK*1024],'H',1,0);
				_ldpc->GenRAWData(&page_buf[0],4112, _fh->PageSizeK);
			}
		} else if (TPgCnt-3 == pageIdx) {// // Page last-1 Signature
			isOnly4Kdata = true;
			memcpy(page_buf,&file_buf[offset],1024+512);
			GenE13Spare(&page_buf[_fh->PageSizeK*1024],'S',1,0);


			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'S',1,0,  L4K_BUF_SIZE_WR);
				input5013Cmd->Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32*4, true);
			}
			else
			{
				GenE13Spare(&page_buf[_fh->PageSizeK*1024],'S',1,0);
				_ldpc->GenRAWData(&page_buf[0],4112, _fh->PageSizeK);
			}
		} else if (TPgCnt-2 == pageIdx) {//// Page last code pointer
			isOnly4Kdata = true;
			memcpy(page_buf,CPBuf,4096);
			if (isSQL_groupRW)
			{
				Dev_GenSpare_32(P4K_WR, 'P',1,0, L4K_BUF_SIZE_WR);
				input5013Cmd->Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32*4, true);
			}
			else
			{
				GenE13Spare(&page_buf[_fh->PageSizeK*1024],'P',1,0);
				_ldpc->GenRAWData(&page_buf[0],4112, _fh->PageSizeK);
			}

		} else if (TPgCnt-1 == pageIdx) {//Write Information Page
			isWritingINFO_Page = true;
			if (is8K_InfoPage)
				isOnly4Kdata = false;
			else
				isOnly4Kdata = true;
			memcpy(page_buf, input_4k_buf, 8192);

			if (isSQL_groupRW)
			{
				inputCmd->Dev_SetEccMode(is8K_InfoPage?ldpc_mode:7);
				inputCmd->Dev_SetFWSeed(pageIdx, 0x8299);
			}
			else
			{
				_ldpc->SetLDPCMode(is8K_InfoPage?ldpc_mode:7);
				_ldpc->SetBlockSeed(0x8299);
				_ldpc->SetPageSeed(pageIdx); //YC teams setting
				_ldpc->GenRAWData(&page_buf[0], is8K_InfoPage?(4112*2):4112, _fh->PageSizeK);
			}

		} else if (pageIdx >= TPgCnt) {

		} else {
			isOnly4Kdata = false;
			if(section<5)
			{					
				if(SectionMaxSecCnt>(_fh->PageSizeK*2))
				{
					SectionMaxSecCnt-=(_fh->PageSizeK*2);
					WriteSec = (_fh->PageSizeK*2);
				}
				else
				{
					WriteSec = SectionMaxSecCnt&0xff;
					SectionMaxSecCnt=0;
				}	
				memcpy(page_buf,&file_buf[offset],WriteSec*512);
				FrameNo = (WriteSec*512)/4096;
				if((WriteSec*512)%4096)
					FrameNo++;

				if (isSQL_groupRW)
				{
					inputCmd->Dev_SetEccMode(ldpc_mode);

					Dev_GenSpare_32(P4K_WR, 'C',FrameNo, section, L4K_BUF_SIZE_WR);
					input5013Cmd->Dev_SetP4K(&P4K_WR[L4K_WRITE_START_ADDR], 32*4, true);;
				}
				else
				{
					_ldpc->SetLDPCMode(ldpc_mode);
					GenE13Spare(&page_buf[_fh->PageSizeK*1024],'C',FrameNo,section);				
					_ldpc->GenRAWData(&page_buf[0],FrameNo*4112, _fh->PageSizeK);
				}

				offset+=WriteSec*512;
				if(!SectionMaxSecCnt)
				{
					do
					{	
						section++;
						SectionMaxSecCnt=(file_buf[0x81+(section*2)]<<8)+file_buf[0x80+(section*2)];
					}
					while((SectionMaxSecCnt==0)&&(section<=5));
				}
			}

		}

		verify_fail = 0;
		for (int ce_idx=0; ce_idx<_fh->FlhNumber; ce_idx++)
		{
			if (isSQL_groupRW)
			{
				unsigned char eccbuf[512] = {0};
				sqlCmd->GroupWriteD1( ce_idx ,common_code_block, pageIdx, 1, page_buf, _FH.PageSize, 2, 1, false );
				input5013Cmd->Dev_SetP4K(&P4K_WR[L4K_READ_START_ADDR], 32*4, false);
				sqlCmd->GroupReadDataD1(ce_idx, common_code_block, pageIdx, 1, page_buf_verify, _FH.PageSize ,1, false, false );
			}
			else
			{
				inputCmd->ST_DirectPageWrite(1, handle, 0, 0/*RWMode*/, ce_idx, common_code_block, pageIdx, 1/*page cnt*/, 0, 0 ,0 ,page_size_alignK, 0, page_buf, 0, 0);
				inputCmd->ST_DirectPageRead(1, handle, 0, 0/*RWMode*/, ce_idx, common_code_block, pageIdx, 1, 0, 0, 0, 0, page_size_alignK, page_buf_verify, 0 );
			}

			if (isWritingINFO_Page)
			{
				verify_fail = u.DiffBitCheckPerK(page_buf, page_buf_verify, 4096*2 , 10, NULL, &total_diff);
			}
			else
			{
				verify_fail = u.DiffBitCheckPerK(page_buf, page_buf_verify, isOnly4Kdata?4096:_fh->PageSizeK*1024, 10, NULL, &total_diff);
			}
			inputCmd->nt_file_log("ce %2d ,blk %5d ,page %5d ,writeCodePageDiff=%d", ce_idx, common_code_block, pageIdx ,total_diff);
			if (verify_fail) {
				inputCmd->ST_DirectOneBlockErase(1, handle, 0, true, ce_idx, common_code_block, 0, 0, 0);
				break;
			}
		}


	}//end of page loop

END:
	if (NULL != file_buf)
		delete [] file_buf;
	if (NULL != page_buf)
		delete [] page_buf;
	if (NULL != page_buf_verify)
		delete [] page_buf_verify;
	if (NULL != fp)
		fclose(fp);

	inputCmd->ST_SetGet_Parameter(1, handle, 0,true, input_ap_parameter);

	return rtn;
}

void PCIESCSI5013_ST::Fill_L3BlocksToInfoPage(unsigned char * id_page, L3_BLOCKS_ALL * L3_blocks)
{
	memcpy(&id_page[INFOBLOCK_L3_BLK_START_ADDR], L3_blocks, sizeof(L3_BLOCKS_ALL));
}
unsigned char CodeBlockMark_E13[16] = {
	0x43, 0x4F, 0x44, 0x45, 0x20, 0x42, 0x4C, 0x4F, 0x43, 0x4B, 0x55, 0xAA, 0xAA, 0x55, 0x00, 0xFF
};
void PCIESCSI5013_ST::ParsingBinFile(unsigned char *buf)
{
	unsigned char i;
	//unsigned int j;
	LdpcEcc ldpcecc(0x5013);
	unsigned int *offset;
	if(memcmp(buf,CodeBlockMark_E13,sizeof(CodeBlockMark_E13))!=0)
		return;

	m_Section_Sec_Cnt[0] = ((unsigned short *)buf)[0x80>>1];	// ATCM
	m_Section_Sec_Cnt[1] = ((unsigned short *)buf)[0x82>>1];	// Code Bank
	m_Section_Sec_Cnt[2] = ((unsigned short *)buf)[0x84>>1];	// BTCM
	m_Section_Sec_Cnt[3] = ((unsigned short *)buf)[0x86>>1];	// Andes
	m_Section_Sec_Cnt[4] = ((unsigned short *)buf)[0x88>>1];	// FIP
	m_Section_Sec_Cnt[5] = ((unsigned short *)buf)[0x8A>>1];	// Total sec count
	memset(m_Section_CRC32,0x00,sizeof(m_Section_CRC32));
	offset=(unsigned int*)&buf[512];
	for(i=0;i<5;i++)
	{
		if(m_Section_Sec_Cnt[i]!=0)
		{
			m_Section_CRC32[i]=ldpcecc.E13_FW_CRC32(offset, m_Section_Sec_Cnt[i]<<9);
			offset+=(m_Section_Sec_Cnt[i]<<7);// x512 /4
		}
	}

}

short handleRDTTemperature(unsigned char flash_temperature_idx, FH_Paramater * fh)
{
	if (0x98 == fh->bMaker && (fh->bIsBics3 || fh->bIsBics3pMLC || fh->bIsBiCs3QLC || fh->bIsBics4 || fh->bIsBiCs4QLC))
	{
		return (-42 + flash_temperature_idx);
	}
	else
	{
		return -1000;//not support
	}
}

char timeString[128];
char * GetTimeString(unsigned long long time_sec)
{
	int ss = time_sec%60;
	int mm = ((time_sec%3600)/60) & 0xffff;
	int hh = (time_sec/3600) & 0xffff;
	sprintf( timeString, "%d:%d:%d", hh, mm, ss);
	return (&timeString[0]);
}


/*
SCHEDULER_DISPATCHING = 0,
SCHEDULER_DISPATCH_DONE,
SCHEDULER_SUSPEND_SCANWINDOW_FAIL,
SCHEDULER_SUSPEND_INIT_BT_FAIL,
SCHEDULER_SUSPEND_PTO_FAIL,
SCHEDULER_SUSPEND_FPU_FAIL,
SCHEDULER_SUSPEND_TIMEOUT_FAIL,
SCHEDULER_SUSPEND_INIT_SCHEDULER_FAIL,
SCHEDULER_SUSPEND_ZQ_FAIL,
*/
int PCIESCSI5013_ST::LogOut_QC_report(USBSCSICmd_ST * cmd, QC_Page_t * e13_qc_info, int ce_idx)
{
	char fw_version[16] = {0};
	char boot_version[9] = {0};
	char schler_type[9][128] = {"DISPATCHING", "DISPATCH_DONE", "SCANWINDOWS_FAIL", "INIT_BT_FAIL", "PTO_FAIL", "FPU_FAIL", "TIMEOUT", "INIT_FAIL", "ZQ_FAIL"};
	memcpy(fw_version, e13_qc_info->ubSortingFW_ver, 10);
	memcpy(boot_version, e13_qc_info->ubBootCode_Ver, 8);
	cmd->nt_file_log("  FW    version:%s", fw_version);
	cmd->nt_file_log("  Boot  version:%s", boot_version);
	cmd->nt_file_log("  App   version:%x%x.%x", e13_qc_info->ubSortingAP_ver[0], e13_qc_info->ubSortingAP_ver[1], e13_qc_info->ubSortingAP_ver[2]);
	cmd->nt_file_log("  Interface:%d  CLK:%dMhz",e13_qc_info->ubInterface, GetDataRateByClockIdx(e13_qc_info->ubClock));
	unsigned char * id = (unsigned char * )&e13_qc_info->ulChipID[0];
	cmd->nt_file_log("  Chip ID:%02X%02X%02X%02X%02X%02X%02X%02X_%d", id[0], id[7], id[6], id[5], id[4], id[3], id[2], id[1], e13_qc_info->ubIC_Channel_Number);
	cmd->nt_file_log("  Channel ID:%d", e13_qc_info->ubIC_Channel_Number);
	cmd->nt_file_log("  Report number:%d of %d", e13_qc_info->ubCurRecordNumber, e13_qc_info->ubTotalRecordNumber);
	cmd->nt_file_log("  DLL center:%d", e13_qc_info->uwMDLL_Center);
	cmd->nt_file_log("  DLL offset w:%d", e13_qc_info->uwMDLL_W_Offset);
	cmd->nt_file_log("  DLL offset r:%d", e13_qc_info->uwMDLL_R_Offset);
	cmd->nt_file_log("  SCHEDLER:%s", schler_type[e13_qc_info->ubSchedulerState]);
	cmd->nt_file_log("  DF2D:%d", e13_qc_info->ubFuncFail);
	cmd->nt_file_log("  Mark bad:%d", e13_qc_info->uwMarkBadFailCount);
	
	unsigned short TotalRetry = 0;
	unsigned short DieMaxBad = 0;
	unsigned short PlaneMaxBad = 0;
	unsigned short CeBad = 0;
	unsigned short WindowsWidth_W = e13_qc_info->uwW_Count0_SDLLMax - e13_qc_info->uwW_Count0_SDLLMin;
	unsigned short WindowsWidth_R = e13_qc_info->uwR_Count0_SDLLMax - e13_qc_info->uwR_Count0_SDLLMin;
	float W_picosecond = CaculateWindowsWidthPicoSecond(e13_qc_info->uwW_Count0_SDLLMin, e13_qc_info->uwW_Count0_SDLLMax, e13_qc_info->uwMDLL_Center,  GetDataRateByClockIdx(e13_qc_info->ubClock));
	float R_picosecond = CaculateWindowsWidthPicoSecond(e13_qc_info->uwR_Count0_SDLLMin, e13_qc_info->uwR_Count0_SDLLMax, e13_qc_info->uwMDLL_Center,  GetDataRateByClockIdx(e13_qc_info->ubClock));
	cmd->nt_file_log("  WindowsWidth_W:%d", WindowsWidth_W);
	cmd->nt_file_log("  WindowsWidth_R:%d", WindowsWidth_R);
	CheckSSDBin * ssdbin = new CheckSSDBin(*_fh);
	int intSSDBin = 0;
	TotalRetry = e13_qc_info->uwRR_PassCnt[0] + e13_qc_info->uwRR_PassCnt[1] + e13_qc_info->uwRR_PassCnt[2] + e13_qc_info->uwRR_PassCnt[3];
	for (int die_idx=0; die_idx< _fh->DieNumber; die_idx++ )
	{
		unsigned short DieBad = 0;
		unsigned short PlaneBadBuf[4] = {0};
		for (int plane_idx=0; plane_idx< _fh->Plane; plane_idx++)
		{
			unsigned short PlaneBad = e13_qc_info->uwFailBlkCntPerDie[die_idx][plane_idx];
			PlaneBadBuf[plane_idx] = PlaneBad;
			DieBad += PlaneBad;

			if (PlaneBad > PlaneMaxBad)
				PlaneMaxBad = PlaneBad;
		}

		if (DieBad > DieMaxBad)
			DieMaxBad = DieBad;

		CeBad += DieBad;
		// Retry=%-3d:[P0=%-3d,P1=%-3d,P2=%-3d,P3=%-3d]
		intSSDBin = ssdbin->SSD_GetBinResultFromCSV(PlaneMaxBad, DieMaxBad, CeBad);
		if (4 == _fh->Plane)
		{
			cmd->nt_file_log("GR Idx=%d,  #CE %-2d DIE %-2d:Bad=%-3d:[P0=%-3d,P1=%-3d,P2=%-3d,P3=%-3d], WinW:%-3d~%-3d %-3d %4.2fps, WinR:%-3d~%-3d %-3d %4.2fps, SSD_Bin%X",
					e13_qc_info->ubCurRecordNumber,
				    ce_idx, die_idx, CeBad,
					PlaneBadBuf[0], PlaneBadBuf[1], PlaneBadBuf[2], PlaneBadBuf[3],
					e13_qc_info->uwW_Count0_SDLLMin, e13_qc_info->uwW_Count0_SDLLMax, WindowsWidth_W, W_picosecond,
					e13_qc_info->uwR_Count0_SDLLMin, e13_qc_info->uwR_Count0_SDLLMax, WindowsWidth_R, R_picosecond,
					intSSDBin
				);
		}
		else if (2 == _fh->Plane)
		{
			cmd->nt_file_log("GR Idx=%d,  #CE %-2d DIE %-2d:Bad=%-3d:[P0=%-3d,P1=%-3d], WinW:%-3d~%-3d %-3d %4.2fps, WinR:%-3d~%-3d %-3d %4.2fps, SSD_Bin%X",
				e13_qc_info->ubCurRecordNumber,
				ce_idx, die_idx, CeBad,
				PlaneBadBuf[0], PlaneBadBuf[1],
				e13_qc_info->uwW_Count0_SDLLMin, e13_qc_info->uwW_Count0_SDLLMax, WindowsWidth_W, W_picosecond,
				e13_qc_info->uwR_Count0_SDLLMin, e13_qc_info->uwR_Count0_SDLLMax, WindowsWidth_R, R_picosecond,
				intSSDBin
				);
		}
	}
	if (4 == _fh->Plane)
	{
		cmd->nt_file_log("  #CE %-2d Retry=%-3d [P0=%-3d,P1=%-3d,P2=%-3d,P3=%-3d]",
				ce_idx, TotalRetry,
				 e13_qc_info->uwRR_PassCnt[0], e13_qc_info->uwRR_PassCnt[1], e13_qc_info->uwRR_PassCnt[2], e13_qc_info->uwRR_PassCnt[3]);
	}
	else if ( 2 == _fh->Plane)
	{
		cmd->nt_file_log("  #CE %-2d Retry=%-3d [P0=%-3d,P1=%-3d]",
			ce_idx, TotalRetry,
			e13_qc_info->uwRR_PassCnt[0], e13_qc_info->uwRR_PassCnt[1]);
	}

	if (0xFF == intSSDBin || 0x05 == intSSDBin )
	{
		_fh->isCheckSSDBinPass = false;
	}
	else
	{
		_fh->isCheckSSDBinPass = true;
	}
	
	delete ssdbin;

	//short flh_temperature = handleRDTTemperature(e13_qc_info->uwFlaTemperature&0xff, cmd->_fh);
	short flh_temperature = e13_qc_info->uwFlaTemperature;
	short ctl_temperature = (short) e13_qc_info->uwCtrlTemperature;

	unsigned long long  gr_time_delta = 0;
	if (_last_offline_time)
	{
		gr_time_delta = e13_qc_info->ullTime - _last_offline_time;
	}
	_last_offline_time = (unsigned short)e13_qc_info->ullTime;
	
	char gr_time_string[128] = {0};
	char gr_time_delta_string[128] = {0};;
	sprintf(gr_time_string, "%s", GetTimeString(e13_qc_info->ullTime));
	sprintf(gr_time_delta_string, "%s", GetTimeString(gr_time_delta));

	if (flh_temperature>500 || flh_temperature<-500)
	{
		cmd->nt_file_log("  Time=%s ,Time Delta=%s ,Temperature:CTL=%d", gr_time_string, gr_time_delta_string, e13_qc_info->uwCtrlTemperature );
	}
	else
	{
		cmd->nt_file_log("  Time=%s ,Time Delta=%s ,Temperature:CTL=%d, FLH=%d, Diff=%d", gr_time_string, gr_time_delta_string, e13_qc_info->uwCtrlTemperature, flh_temperature, (ctl_temperature-flh_temperature));
	}


	return 0;
}

int PCIESCSI5013_ST::LogOut_L3_QC_report(USBSCSICmd_ST * cmd, unsigned char *L3_qc_buffer,int ce_idx ,bool isCntL3Fail)
{
	int PICK_GROUP = 3;
	unsigned short blkInfo =  0;
	unsigned short blkFailTask =  0;

	FH_Paramater FH;
	memcpy(&FH, cmd->_fh, sizeof(FH));
	for (int die_idx=0; die_idx<FH.DieNumber; die_idx++)
	{
		L3_BLOCKS_PER_CE * L3_blks =      (L3_BLOCKS_PER_CE *)&L3_qc_buffer[0];
		unsigned char * fail_addr_start = L3_qc_buffer + 0x180;
		L3_BLOCKS_PER_CE * L3_fail_task = (L3_BLOCKS_PER_CE *)fail_addr_start;
		int pe_block_cnt = 0;
		/*-----PE BLOCK-----------------*/
		for (int g_idx = 0; g_idx < PICK_GROUP; g_idx++)
		{

			for (int plane_idx = 0; plane_idx < FH.Plane; plane_idx++)
			{
				int blk_addr_in_die = g_idx * 8/*Max support planeX2 */ + plane_idx;
				blkInfo = L3_blks->die[die_idx][blk_addr_in_die];

				if (0xFFFF == blkInfo)
					continue;


				cmd->_PE_blocks[ce_idx][die_idx][pe_block_cnt] = (0x7FFF&blkInfo);
				pe_block_cnt++;

				int isFailed = 0x8000&blkInfo;
				if (isFailed) {
					blkFailTask = L3_fail_task->die[die_idx][blk_addr_in_die];
					cmd->nt_file_log("  PE Block:%d Fail, Task=%d", (0x7FFF&blkInfo), blkFailTask);
					if (isCntL3Fail)
						cmd->_L3_total_fail_cnt_PE++;
				}
				else {
					cmd->nt_file_log("  PE Block:%d", (0x7FFF&blkInfo), blkFailTask);
				}
			}//plane loop
		}//pick group loop

		/*-----NON PE BLOCK-----------------*/
		for (int g_idx = 0; g_idx < PICK_GROUP; g_idx++)
		{

			for (int plane_idx = 0; plane_idx < FH.Plane; plane_idx++)
			{
				int blk_addr_in_die = g_idx * 8/*Max support planeX2 */ + plane_idx + 4/*shift Max 4 Plane*/;
				blkInfo = L3_blks->die[die_idx][blk_addr_in_die];

				if (0xFFFF == blkInfo)
					continue;

				int isFailed = 0x8000&blkInfo;
				if (isFailed) {
					blkFailTask = L3_fail_task->die[die_idx][blk_addr_in_die];
					cmd->nt_file_log("  NON_PE Block:%d Fail, Task=%d", (0x7FFF&blkInfo), blkFailTask);
					if (isCntL3Fail)
						cmd->_L3_total_fail_cnt_NON_PE++;
				}
				else {
					cmd->nt_file_log("  NON_PE Block:%d", (0x7FFF&blkInfo), blkFailTask);
				}
			}//plane loop
		}//pick group loop
	}//die loop
	return 0;
}

int PCIESCSI5013_ST::FindTotalValidRecordCnt(USBSCSICmd_ST * inputCmd, unsigned char * input_ap_parameter, int ce_idx, int blk_idx, int data_segment_length, int fw_reserved_cnt, int max_find_cnt, bool isCntL3Fail)
{
	int information_address = fw_reserved_cnt-1;
	int page_size_alignK = (_FH.PageSize + 1023)/1024*1024;
	unsigned char * page_buf = new unsigned char[page_size_alignK];
	unsigned char  qc_buf[4096] = {0};
	int validCnt = 0;
	HANDLE handle = inputCmd->Get_SCSI_Handle();
	inputCmd->SetDumpWriteReadAPParameter(handle, input_ap_parameter);
	Utilities u;
	int total_QC_cnt=0;
	int total_QC_L3_cnt=0;
	_last_offline_time = 0;

	for (int report_idx = 0;  report_idx< max_find_cnt; report_idx++)
	{
		int report_page = information_address + 1 + (report_idx * (_FH.DieNumber + 2));
	
		if ((max_find_cnt-1) == report_idx)
		{
			report_page = (int)_FH.MaxPage-1;
			inputCmd->nt_file_log("---->LIST LAST PAGE");
		}

		inputCmd->ST_DirectPageRead(1, handle, 0, 0/*RWMode*/, ce_idx, blk_idx, report_page, 1, 0, 0, 0, 0, page_size_alignK, page_buf, 0 );
		//verify_fail = u.DiffBitCheckPerK(page_buf, page_buf_verify, _fh->PageSize, 10, NULL, &total_diff);


		if (0 == u.FindRightData(page_buf, E13_L3_QC_PAGE_SEGEMENT_LENGT, page_size_alignK, qc_buf))
		{
			//if ((0 == page_buf[1]) && (0 == page_buf[3]) && (0 == page_buf[9])&&  (0 == page_buf[11]) )
			{
				inputCmd->nt_file_log("Found E13 L3 QC: CE%d  Block%d  Page%d  cnt=%d", ce_idx, blk_idx, report_page, (total_QC_L3_cnt+total_QC_cnt));
				inputCmd->LogHex(qc_buf, 16*10);//print 10 line
				LogOut_L3_QC_report(inputCmd, qc_buf, ce_idx ,isCntL3Fail);
				total_QC_L3_cnt++;
			}
		}
		else
		{
			if (0 == u.FindRightData(page_buf,  sizeof(QC_Page_t), page_size_alignK, qc_buf))
			{
				if(!memcmp("E13STFW", &qc_buf[0x20], 6) || !memcmp("E19STFW", &qc_buf[0x20], 6))
				{
					inputCmd->nt_file_log("=====>Found E13 QC: CE%d  Block%d  Page%d  cnt=%d", ce_idx, blk_idx, report_page, (total_QC_L3_cnt+total_QC_cnt));
					LogOut_QC_report(inputCmd, (QC_Page_t*)qc_buf, ce_idx);
					total_QC_cnt++;
				}
			}
		}
		if (report_page >= (int)_FH.MaxPage-1)
			break;
	}

	inputCmd->nt_file_log("Found offline QC Summary: total=%d QC=%d L3_QC=%d",(total_QC_cnt+total_QC_L3_cnt),total_QC_cnt, total_QC_L3_cnt);


	//recovery
	inputCmd->ST_SetGet_Parameter(1, handle, 0,true, input_ap_parameter);
	if (page_buf)
		delete [] page_buf;

	return (total_QC_L3_cnt+total_QC_cnt);
}

int PCIESCSI5013_ST::CalculatePageCntForFWBin(char * fw_bin_file, int pageSizeK)
{
	unsigned char buf[512] = {0};
	unsigned short section_sec_cnt[6] = {0};
	if (!fw_bin_file)
		return -1;

	FILE * fp = fopen(fw_bin_file, "rb");
	if (!fp)
		return -1;
	
	fread(buf, 1, 512, fp);

	section_sec_cnt[0] = ((unsigned short *)buf)[0x80>>1];	// ATCM
	section_sec_cnt[1] = ((unsigned short *)buf)[0x82>>1];	// Code Bank
	section_sec_cnt[2] = ((unsigned short *)buf)[0x84>>1];	// BTCM
	section_sec_cnt[3] = ((unsigned short *)buf)[0x86>>1];	// Andes
	section_sec_cnt[4] = ((unsigned short *)buf)[0x88>>1];	// FIP
	section_sec_cnt[5] = ((unsigned short *)buf)[0x8A>>1];	// Total sec count

	fclose(fp);

	int pageCnt = 0;


	pageCnt++;/*ID page*/
	pageCnt++;/*Code Sign Header page*/

	for (int sec_idx = 0; sec_idx < 5; sec_idx++)
	{
		pageCnt += (section_sec_cnt[sec_idx]>>1)/pageSizeK;
		if ((section_sec_cnt[sec_idx]>>1)%pageSizeK)
			pageCnt++;
	}

	pageCnt++;/*Signature page*/
	pageCnt++;/*Code Pointer page*/
	pageCnt++;/*INFO page*/

	return pageCnt;

}

int PCIESCSI5013_ST::SelectLdpcModeByPageSize( int pageSize)
{
	int ldpcMode = 0;
	int frameCountPerPage = (pageSize>>12);
	int frameSize = pageSize/frameCountPerPage;
	frameSize-= 4112;
	frameSize /= 2;
	if(frameSize>304)
		ldpcMode = 7;
	else if(frameSize>288)
		ldpcMode = 8;
	else if(frameSize>256)
		ldpcMode = 0;
	else if(frameSize>240)
		ldpcMode = 1;
	else if(frameSize>224)
		ldpcMode = 2;
	else if(frameSize>208)
		ldpcMode = 3;
	else if(frameSize>176)
		ldpcMode = 4;
	else if(frameSize>144)
		ldpcMode = 5;
	else
		ldpcMode = 6;

	return ldpcMode;

}

void PCIESCSI5013_ST::PrepareIDpage(unsigned char *buf, int target_interface, int target_clock, int userSpecifyBlkCnt)
{

	UCHAR ubEN_SpeedEnhance = 0;
	UINT uli;
#if 0	
	if ( (_fh->bMaker == 0x98) || (_fh->bMaker == 0x45) ) {	//No Legacy: T32 SLC, T24 SLC
		if ( (_fh->id[5] == 0xD6) || (_fh->id[5] == 0xD5) ) {
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
	if (GetPrivateProfileInt("Configuration","1V2",0,_ini_file_name))
	{
		if (_fh->bIsMicronB17 || _fh->bIsMicronB27 || _fh->bIsMicronB36R || _fh->bIsMicronB37R|| _fh->bIsMicronB47R|| _fh->bIsMicronQ1428A)
			ubEN_SpeedEnhance = 1;

		if (0x98 == _fh->bMaker && (_fh->bIsBics4|| _fh->bIsBiCs4QLC))
			ubEN_SpeedEnhance = 1;
	}

#endif

	
/*0x00 -- XModemBuf[0] = "I", [1] = "D" */	
	//IDPGB[IDB_HEADER_0] = 'I';
	//IDPGB[IDB_HEADER_1] = 'D';
	IDPGB[0x00] = 'I';
	IDPGB[0x01] = 'D';
	
/*0x02 -- Set Burner Number */
/*
#if FAIL_PATTERN_2
	//IDPGB[IDB_FW_REVISION_ID] = 0x21;
	IDPGB[0x02] = 0x21;
#else 
	//IDPGB[IDB_FW_REVISION_ID] = 0x12;
	IDPGB[0x02] = 0x12;
#endif
*/
	IDPGB[0x02] = 0x01;
/*0x03 -- If this byte is equal to 0x33, then bootcode will check CRC of 4KB ID Table and code ptr */
	//IDPGB[IDB_CRC32_CHECK_IDTABLE] = 0x33;
	IDPGB[0x03] = 0x33;
/*0x04 -- If this byte is equal to 0x33, then bootcode will check CRC32 of each FW section */
	//IDPGB[IDB_CRC32_CHECK_FWSECTION] = 0x33;
	IDPGB[0x04] = 0x33;
/*0x05*/
	//**這邊要交由誰決定?
	//IDPGB[IDB_LOAD_CODE_SPEED_ENHANCE] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;			//If 0x33, bootcode will issue 0x28~0x2D to do set feature command
	IDPGB[0x05] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;			//If 0x33, bootcode will issue 0x28~0x2D to do set feature command
/*0x06*/
	//IDPGB[IDB_LOAD_CODE_SPEED_ENHANCE_CHECK] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;	//If 0x33, bootcode will issue get feature command to check feature
	IDPGB[0x06] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;	//If 0x33, bootcode will issue get feature command to check feature
/*0x07 -- Flash Interface after set feature 0:Legacy mode 2:toggle mode */
	/*
	if (ubEN_SpeedEnhance) {
		if (UFS_ONLY) {
			IDPGB[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_TOGGLE;
		}
		else if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == LEGACY_INTERFACE ) {	// Default lefacy force to toggle	. Reference CH0 setting
			IDPGB[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_TOGGLE;
		}
		else if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == TOGGLE_INTERFACE ) {	// Default toggle force to legacy
			IDPGB[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_LEGACY;
		}
	}
	else {
		IDPGB[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_LEGACY;
	}
	*/
	if (ubEN_SpeedEnhance)
		IDPGB[0x07] = 2;
	else
		IDPGB[0x07] = 0;
	
/*0x08 -- FW version */
	//IDPGB[IDB_FW_VERSION_CHAR + 0] = 'E';
	//IDPGB[IDB_FW_VERSION_CHAR + 1] = 'D';
	//IDPGB[IDB_FW_VERSION_CHAR + 2] = 'R';
	//IDPGB[0x08] = 'E';
	//IDPGB[0x09] = 'D';
	//IDPGB[0x0A] = 'R';

/*0x10 -- Vendor ID and Device ID (PCIE usage) */
	//IDPGL[IDL_PCIE_VENDOR_DEVICE_ID] = 0xEDBB1987;	//0x00;
	//IDPGL[0x10>>2] = 0xEDBB1987;	//0x00;
/*0x14 -- The CRC32 value of the 1st 4KB of ID table */
	// Before CRC calculate, set this region as 0x00
	//IDPGL[IDL_IDPAGE_CRC] = 0x00000000;
	IDPGL[0x14>>2] = 0x00000000;
	
/*0x18 -- The number of sectors per page represented in powers of 2 (sector per Page == 1 << sector per Page bit) */
	//IDPGB[IDB_SECTOR_PER_PAGE_LOGBIT] = gpFlhEnv->ubSectorsPerPageLog;	//log value for sectors per page
	//IDPGB[0x18] = 0x05;	// 32 sector (16K)	//log value for sectors per page
	int sector_cnt = (_fh->PageSizeK*1024)/512;
	IDPGB[0x18] = LogTwo(sector_cnt);
/*0x19 -- The number of page per block represented in powers of 2 (page per block == 1 << page per block bit) */
	//IDPGB[IDB_PAGE_ADR_BIT_WIDTH] = gpFlhEnv->ubPageADRBitWidth;
	//IDPGB[0x19] = gpFlhEnv->ubPageADRBitWidth;
	IDPGB[0x19] = LogTwo(_fh->PagePerBlock);
	//_fh.PagePerBlock
	uli = (_fh->PageSize_H<<8)+_fh->PageSize_L;
/*0x1A -- The number of DMA frames are involved in one page */
	//IDPGB[IDB_FRAME_PER_PAGE] = (U8)((gpFlhEnv->uwPageByteCnt) >> 12);	// Page size / 4KB
	IDPGB[0x1A] = (unsigned char)(uli >> 12);	// Page size / 4KB
	
/*0x1B -- */
	//IDPGB[IDB_MDLL_DIRVAL_EN] = 0x00;
	IDPGB[0x1B] = 0x00;
/*0x1C -- Enable FPU_PTR_CXX_AX_C00_A5_C30 */
	//IDPGB[IDB_NEW_FPU_EN] = 0x00;
	IDPGB[0x1C] = 0x00;
/*0x1D -- */
	//IDPGB[IDB_NEW_FPU_CMD] = 0x00;
	IDPGB[0x1D] = 0x00;
/*0x1E -- */
	//IDPGB[IDB_NEW_FPU_ADR_NUM] = 0x00;
	IDPGB[0x1E] = 0x00;

/*0x20 -- The LDPC mode for loading FW code */
	//IDPGB[IDB_LDPC_MODE] = (U8)(r32_FCON[FCONL_LDPC_CFG] & CHK_ECC_MODE_MASK);		//前面已經算好了
	//  Mode 0: 2048bit 
	//	Mode 1: 1920bit 
	//	Mode 2: 1792bit
	//	Mode 3: 1664bit
	//	Mode 4: 1408bit
	//	Mode 5: 1152bit
	//	Mode 6: 1024bit
	//	Mode 7: 2432bit
	//	Mode 8: 2304bit
	uli /= IDPGB[0x1A];
	uli-= 4112;
	uli /= 2;
	if(uli>304)
		IDPGB[0x20] = 7;
	else if(uli>288)
		IDPGB[0x20] = 8;
	else if(uli>256)
		IDPGB[0x20] = 0;
	else if(uli>240)
		IDPGB[0x20] = 1;
	else if(uli>224)
		IDPGB[0x20] = 2;
	else if(uli>208)
		IDPGB[0x20] = 3;
	else if(uli>176)
		IDPGB[0x20] = 4;
	else if(uli>144)
		IDPGB[0x20] = 5;
	else
		IDPGB[0x20] = 6;

	_RDTCodeBodyLDPCMode = IDPGB[0x20];
/*0x21 -- If this byte is equal to 0x33, then we will not set PCIe link in boot code, only in FW */
	//IDPGB[IDB_HOST_DELAY_LINK] = 0x33;
	IDPGB[0x21] = 0x33;
	//IDPGB[IDB_HOST_DELAY_LINK] = 0x00;		//Force let PCIe link at IDPG
/*0x22 -- If this byte is equal to 0x33, Set FPU to 6 Address else Set FPU to 5 Address */
	//IDPGB[IDB_FLH_6ADR_NUM] = 0x00;
	IDPGB[0x22] = 0x00;
/*0x23 -- Micron 0xDA 0x91 0x40 Hynix 0xDA */
	//IDPGB[IDB_SLC_METHOD] = gpFlhEnv->ubSLCMethod;
	if (_fh->bIsHynix3DTLC)
		IDPGB[0x23] = 0xDA;
	else
		IDPGB[0x23] = 0xA2;
/*0x24 -- The logical page index for code pointer */
	//IDPGW[IDW_CODEPTR_PAGEIDX] = FlaDealCodeSectionData();		//在知道slot如何擺後才去算

/*0x26 -- If this byte is equal to 0x33, do IP ZQ calibration on when speed enhance */
/*
#ifdef VERIFY_PIO_CODE_BLOCK
	IDPGB[IDB_ZQ_NAND_ENHANCE] = 0x00;
#else
	IDPGB[IDB_ZQ_NAND_ENHANCE] = 0x00;
#endif
*/
	IDPGB[0x26] = 0x00;
/*0x27 -- If this byte is equal to 0x33, do NAND ZQ calibration on when speed enhance */
/*
#ifdef VERIFY_PIO_CODE_BLOCK
	IDPGB[IDB_ZQ_IP_ENHANCE] = 0x00;
#else
	IDPGB[IDB_ZQ_IP_ENHANCE] = 0x00;
#endif
*/
	IDPGB[0x27] = 0x00;
/*0x28 -- The 1st Set Feature Command that will be issued to Flash */
/*
	IDPGB[IDB_SETFEATURE_CMD] = 0x00;
	IDPGB[IDB_SETFEATURE_ADR] = 0x00;
	IDPGB[IDB_SETFEATURE_DAT0] = 0x00;
	IDPGB[IDB_SETFEATURE_DAT1] = 0x00;
	IDPGB[IDB_SETFEATURE_DAT2] = 0x00;
	IDPGB[IDB_SETFEATURE_DAT3] = 0x00;
	if (ubEN_SpeedEnhance) {
		if (UFS_ONLY) {		// Toshiba
			IDPGB[IDB_SETFEATURE_CMD] = 0xEF;
			IDPGB[IDB_SETFEATURE_ADR] = 0x80;
			IDPGB[IDB_SETFEATURE_DAT0] = 0x00;	// Toggle: 0x00,  SDR: 0x01
			IDPGB[IDB_SETFEATURE_DAT1] = 0x00;
			IDPGB[IDB_SETFEATURE_DAT2] = 0x00;
			IDPGB[IDB_SETFEATURE_DAT3] = 0x00;
		}
		else if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == LEGACY_INTERFACE ) {		// Default lefacy force to toggle
			Dump_Debug(uart_printf("Default legacy\n"));
			if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == TOSHIBASANDISK ) {		// Toshiba (Bisc3)				
				IDPGB[IDB_SETFEATURE_CMD] = 0xEF;
				IDPGB[IDB_SETFEATURE_ADR] = 0x80;
				IDPGB[IDB_SETFEATURE_DAT0] = 0x00;	// Toggle: 0x00,  SDR: 0x01
				IDPGB[IDB_SETFEATURE_DAT1] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT2] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT3] = 0x00;
			}
			else if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == INTELMICRON ) {		// Micron (B16A)
				IDPGB[IDB_SETFEATURE_CMD] = 0xEF;
				IDPGB[IDB_SETFEATURE_ADR] = 0x01;
				IDPGB[IDB_SETFEATURE_DAT0] = 0x20;	// [4:5] NVDDR2(toggle): 0x2, Asychronous(Legacy): 0x0
	#ifdef VERIFY_PIO_CODE_BLOCK
				//IDPGB[IDB_SETFEATURE_DAT0] = 0x00;
				//IDPGB[IDB_FLASH_INTERFACE_AFTER_ENHANCE] = INTERFACE_LEGACY;
	
				//IDPGB[IDB_SETFEATURE_DAT0] |= 0x0;
				//IDPGB[IDB_SETFEATURE_DAT0] |= 0x4; // Toggle 100Mhz or Legacy 40Mhz
				//IDPGB[IDB_SETFEATURE_DAT0] |= 0x7; // 200Mhz
				IDPGB[IDB_SETFEATURE_DAT0] |= 0xA; // 400Mhz
	#endif
				IDPGB[IDB_SETFEATURE_DAT1] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT2] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT3] = 0x00;		
			}
			else if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == SKHYNIX ) {			// Hynix (H14)
				IDPGB[IDB_SETFEATURE_CMD] = 0xEF;
				IDPGB[IDB_SETFEATURE_ADR] = 0x01;
				IDPGB[IDB_SETFEATURE_DAT0] = 0x20;	// [4:5] DDR(toggle): 0x2, SDR(Legacy): 0x0
				IDPGB[IDB_SETFEATURE_DAT1] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT2] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT3] = 0x00;		
			}
			else if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == SAMSUNG ) {			// Samsung (MLC)
				
			}
		}
		else if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == TOGGLE_INTERFACE ) {		// Default toggle force to legacy
			Dump_Debug(uart_printf("Default toggle\n"));
			if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == TOSHIBASANDISK ) {		//Toshiba(1Z TLC, 1Z MLC)
				IDPGB[IDB_SETFEATURE_CMD] = 0xEF;
				IDPGB[IDB_SETFEATURE_ADR] = 0x80;
				IDPGB[IDB_SETFEATURE_DAT0] = 0x01;	// Toggle: 0x00,  SDR: 0x01
				IDPGB[IDB_SETFEATURE_DAT1] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT2] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT3] = 0x00;
			}
			else if ( gpFlhEnv->FlashDefaultType.BitMap.ubVendorType == INTELMICRON ) {		// Micron (B16A)
				IDPGB[IDB_SETFEATURE_CMD] = 0xEF;
				IDPGB[IDB_SETFEATURE_ADR] = 0x01;
				IDPGB[IDB_SETFEATURE_DAT0] = 0x00;	// [4:5] NVDDR2(toggle): 0x2, Asychronous(Legacy): 0x0
				IDPGB[IDB_SETFEATURE_DAT1] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT2] = 0x00;
				IDPGB[IDB_SETFEATURE_DAT3] = 0x00;		
			}
		}
	}
*/
	if (ubEN_SpeedEnhance) {
		if (_fh->bIsMicronB17)
		{
			IDPGB[0x28] = 0xEF;
			IDPGB[0x29] = 0x01;
			IDPGB[0x2A] = 0x21;
			IDPGB[0x2B] = 0x00;
			IDPGB[0x2C] = 0x00;
			IDPGB[0x2D] = 0x00;
		}
		else if (0x98 ==_fh->bMaker && _fh->bIsBics4)
		{
			IDPGB[0x28] = 0xEF;
			IDPGB[0x29] = 0x80;
			IDPGB[0x2A] = 0x00;
			IDPGB[0x2B] = 0x00;
			IDPGB[0x2C] = 0x00;
			IDPGB[0x2D] = 0x00;
		}
	} 
	else
	{
		IDPGB[0x28] = 0x00;
		IDPGB[0x29] = 0x00;
		IDPGB[0x2A] = 0x00;
		IDPGB[0x2B] = 0x00;
		IDPGB[0x2C] = 0x00;
		IDPGB[0x2D] = 0x00;
	}
/*0x2E -- Set feature busy time*/
	//IDPGW[IDW_SETFEATURE_BUSYTIME] = (ubEN_SpeedEnhance) ? 0x10 : 0x00;
	IDPGW[0x2E>>1] = (ubEN_SpeedEnhance) ? 0x10 : 0x00;
	IDPGW[0x31] = 0;
	// Back door used IDPage default
/*0x30 -- The first PCIe use back door entry number, these back door will process before host interface init */
	//IDPGB[IDB_PCIE_BACKDOOR0_START_ENTRY] = 0x00;
/*0x31 -- The total number of PCIe back door entry count,  these back door will process before host interface init */
	//IDPGB[IDB_PCIE_BACKDOOR0_ENTRY_COUNT] = 0x00;
/*0x32 -- The first PCIe use back door entry number, these back door will process after host interface init */
	//IDPGB[IDB_PCIE_BACKDOOR1_START_ENTRY] = 0x00;
/*0x33 -- The total number of PCIe back door entry count,  these back door will process after host interface init */
	//IDPGB[IDB_PCIE_BACKDOOR1_ENTRY_COUNT] = 0x00;
/*0x34 -- The first PCIe use back door entry number, These back door will process after ID Page load */
	//IDPGB[IDB_NORMAL_BACKDOOR_START_ENTRY] = 0x00;
/*0x35 -- The total number of normal back door entry count. These back door will process after ID Page load */
	//IDPGB[IDB_NORMAL_BACKDOOR_ENTRY_COUNT] = 0x00;
/*0x36 -- The Last back door entry number,  these back door will process before PRAM enable jump to FW */
	//IDPGB[IDB_LAST_BACKDOOR_START_ENTRY] = 0x00;
/*0x37 -- The Last back door entry count, these back door will process before PRAM enable jump to FW */
	//IDPGB[IDB_LAST_BACKDOOR_ENTRY_COUNT] = 0x00;

/*0x38 -- 0 : According to Efuse BIT0: Code sign enable  BIT1: HMAC Enable  BIT2: FIPS-140 Enable */
/*
#if 1
	IDPGB[IDB_SECURITY_OPTION] = 0;	//BIT0 | BIT2;
	uart_printf("===========================\n");
	uart_printf("This is no Code Sign Burner\n");
	uart_printf("This is no Code Sign Burner\n");
	uart_printf("This is no Code Sign Burner\n");
	uart_printf("===========================\n");
#else
	IDPGB[IDB_SECURITY_OPTION] = BIT0 | BIT2;
#endif
*/

	IDPGB[0x38] = 0;	//BIT0 | BIT2;

/*0x39 -- If this byte is equal to 0x33, set VREF on when speed enhance */
// toggle need take care
/*
#ifdef VERIFY_PIO_CODE_BLOCK
	if(IDPGB[IDB_FLASH_INTERFACE_AFTER_ENHANCE] == INTERFACE_TOGGLE){
		IDPGB[IDB_VREF_ENHANCE] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;
	}
	else{
		IDPGB[IDB_VREF_ENHANCE] = 0x00;
	}
#else
	IDPGB[IDB_VREF_ENHANCE] = 0x00;
#endif
*/
	IDPGB[0x39] = (ubEN_SpeedEnhance) ? 0x33 : 0x00;

/*0x3A -- Cell Type.  0 : According to ID Detection 1: SLC  2: MLC 3: TLC 4: QLC  */
	//IDPGB[IDB_CELL_TYPE] = gpFlhEnv->FlashDefaultType.BitMap.ubCellType + 1;
	IDPGB[0x3A] = 3; // TLC test only
/*0x3B -- Remapping Info.  //0 : 4CH(Default) 1: 2CH(CH0+CH1)  2: 2CH(CH2+CH3)  */
	//gsIPL_OptionInfo.dw4.ubRemappingCEInfo = 0;
	//IDPGB[IDB_REMAPPING_INFO] = gsIPL_OptionInfo.dw4.ubRemappingCEInfo;
	IDPGB[0x3B] = 0x00;

/*0x3C -- 0x33 Enable DQS High when set feature */
	//IDPGB[IDB_SET_FEATURE_DQSH] = 0x00;
	IDPGB[0x3C] = 0x00;
/*0x3D -- 0x33 EDO On */
/*
	if ( (gpFlhEnv->ubDefaultInterfaceMap & BIT0) == TOGGLE_INTERFACE ) {
		IDPGB[IDB_EDO_ENABLE] = 0x00;
	}
	else {
		IDPGB[IDB_EDO_ENABLE] = 0x33;
	}
*/
	IDPGB[0x3D] = 0x33;
/*0x3E -- 0x33 Uart Disable */
	//IDPGB[IDB_UART_DISABLE] = 0x00;
	IDPGB[0x3E] = 0x00;
/*0x3F -- 0x33 CE Decoder Enable */
	//IDPGB[IDB_CE_DECODER_ENABLE] = gsIPL_OptionInfo.dw0.btCeDecoder ? 0x33 : 0x00;
	IDPGB[0x3F] = 0x00;

/*0x40 -- */
	//IDPGB[IDB_SYSx2_CLK_SOURCE] = OSC1_KOUT2_SEL;
/*0x41 -- */
	//IDPGB[IDB_SYSx2_CLK_DIVIDER] = CLK_DIV_1;
/*0x42 -- */
	//IDPGB[IDB_ECC_CLK_SOURCE] = OSC2_KOUT1_SEL;
/*0x43 -- */
	//IDPGB[IDB_ECC_CLK_DIVIDER] = CLK_DIV_2;
/*0x44 -- FLH_IF_CLK */
	//0: 10Mhz (Toggle/Legacy)
	//1: 33Mhz (Toggle/Legacy)
	//2: 40Mhz (Toggle/Legacy)
	//3: 100Mhz (Toggle)
	//4: 200Mhz (Toggle2)(Uper this speed one more set feature need)
	//5: 267Mhz (Toggle2)(Upper this ZQ Calibration need)
	//6: 333Mhz (Toggle2)
	//7: 400Mhz (Toggle2)

	//IDPGB[IDB_FLH_IF_CLK] = DEF_FLASH_10MHZ;
	//IDPGB[0x44] = 0x00;
/*0x45 -- 0x45 Do Clock setting */
/*
#ifdef VERIFY_PIO_CODE_BLOCK
	IDPGB[IDB_CLK_CHANGE_EN] = 0x00;	//0x33;
	if(IDPGB[IDB_SETFEATURE_DAT0] == 0x24) {
		IDPGB[IDB_FLH_IF_CLK] = FLH_IF_100MHz;
	}
	if(IDPGB[IDB_SETFEATURE_DAT0] == 0x27) {
		IDPGB[IDB_FLH_IF_CLK] = FLH_IF_200MHz;
	}
	if(IDPGB[IDB_SETFEATURE_DAT0] == 0x2A) {
		IDPGB[IDB_FLH_IF_CLK] = FLH_IF_400MHz;
	}
	if(IDPGB[IDB_SETFEATURE_DAT0] == 0x04) {
		IDPGB[IDB_FLH_IF_CLK] = FLH_IF_40MHz;
	}
#else
	IDPGB[IDB_CLK_CHANGE_EN] = 0x00;
#endif
*/
	IDPGB[0x45] = 0x00;
/*0x46 -- */
	//IDPGB[IDB_SHA_CLK_SOURCE] = OSC1_KOUT2_SEL;
	IDPGB[0x46] = 0x00;
/*0x47 -- */
	//IDPGB[IDB_SHA_CLK_DIVIDER] = CLK_DIV_2;
	IDPGB[0x47] = 0x00;

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
	//IDPGB[IDB_SATA_GEN] = 0x0;		// 0: Gen3 (default), 1: Gen2, 2: Gen1
/*0x5B -- PCIE GEN*/
	//IDPGB[IDB_PCIE_GEN] = 0x3;	//0x1;		// 2: Support Gen2. 3: Support Gen3
	//IDPGB[0x5B] = 0x03;		//0x1;		// 2: Support Gen2. 3: Support Gen3

/*0x5C -- DCC Setting */	// 彥廷說不用設沒關係
	//IDPGB[IDB_DCC_SETTING] = 0x00;
	//IDPGB[IDB_DCC_SETTING] |= (0x2 & DCC_TRAIN_SIZE_MASK) << DCC_TRAIN_SIZE_SHIFT;		// Frame count.    0: 4k, 1:8k, 2: 16k
	//IDPGB[IDB_DCC_SETTING] |= (0x1 & DCC_TSB_MODE_MASK) << DCC_TSB_MODE_SHIFT;			// 1: Toshiba mode

/*0x5D -- CRS delay off */
	//IDPGB[IDB_CRS_DELAY_OFF] = 0x00; //0x00;
/*0x5E -- PCIE Lane */
	//IDPGB[IDB_PCIE_LANE] = PCIE_LANEx1;		// PCIE_LANEx1 (0x0), PCIE_LANEx2 (0x1), PCIE_LANEx4 (0x3), PCIE_LANEx8 (0x7), PCIE_LANEx16 (0xF)
	//IDPGB[IDB_PCIE_LANE] = PCIE_LANEx4;		// PCIE_LANEx1 (0x0), PCIE_LANEx2 (0x1), PCIE_LANEx4 (0x3), PCIE_LANEx8 (0x7), PCIE_LANEx16 (0xF)
	//IDPGB[0x5E] = 0x03;		// PCIE_LANEx1 (0x0), PCIE_LANEx2 (0x1), PCIE_LANEx4 (0x3), PCIE_LANEx8 (0x7), PCIE_LANEx16 (0xF)


/*0x400 -- Converts logical page to the coresponding physical page, there are 256 page rules*/	
	//for (uli = 0; uli < MAX_FAST_PG; uli++) {
	//	IDPGW[IDW_FAST_PAGE(uli)] = flaFastPageRule(uli);
	//}
	PrepareIDpageForRDTBurner( target_interface,  target_clock, userSpecifyBlkCnt);
	memcpy(buf,IDPage,sizeof(IDPage));
	return;
}


void PCIESCSI5013_ST::InitialCodePointer(unsigned char *buf,unsigned short *blk)
{

	UCHAR ubSectorIdx, ubMaxSlotValue;
	UCHAR ubSectorNum = 5;
	USHORT uwPageIdx;
	UINT ulFWSlot_Base, ulFWSector_Base;
	LdpcEcc ldpcecc(0x5013);
	// Initial CP Base Zero value
	ubMaxSlotValue = 1;

/*0x00 -- Code Pointer Mark. [0] = 'C', [1] = 'P' */
	CODEPTRB[0] = 'C';
	CODEPTRB[1] = 'P';
/*0x02 -- ID Page FW Revision*/
	//CODEPTRB[CPB_IDPG_REVISION] = 0x01;
	CODEPTRB[0x02] = 0x01;
/*0x04 -- CRC32 value of 4KB Code Pointer*/
	//晚一點才有辦法算
/*0x08 -- Record which slot has valid, and FW BIT15 is reserved*/
	//CODEPTRW[CPW_VALIDSLOT_BITMAP] = 0x0001;	//先開FW slot 0
	CODEPTRW[0x08>>1] = 0x0001;	//先開FW slot 0
	
/*0x0B -- Record the number of valid slot FW*/
	//CODEPTRB[CPB_VALID_SLOTNUM] = 0x01;			//先開FW slot 0
	CODEPTRB[0x0B] = 0x01;			//先開FW slot 0
/*0x10 -- Record which FW slot to active*/
	//CODEPTRB[CPB_ACTIVE_SLOTINDEX] = 0x00;		//先開FW slot 0
	CODEPTRB[0x10] = 0x00;		//先開FW slot 0
/*
#if MULTI_FW_SLOT
	CODEPTRW[CPW_VALIDSLOT_BITMAP] = 0x0003;	//開FW slot 0, 1
	CODEPTRB[CPB_VALID_SLOTNUM] = 0x02;			//開FW slot 0, 1
#if	DEBUG_MAKE_FAIL_PATTERN
	CODEPTRB[CPB_ACTIVE_SLOTINDEX] = 0x00;		//Active FW slot 0, and will do retry list
#else
	CODEPTRB[CPB_ACTIVE_SLOTINDEX] = 0x00;		//Active FW slot 1, Normal but not fail pattern, setting to read slot 1
#endif

#endif
*/

#if 0	
/*0x11 -- RetryList Slot Index 0*/
	CODEPTRB[CPB_RETRYLIST_0] = 0x01;
/*0x12 -- RetryList Slot Index 1*/
	CODEPTRB[CPB_RETRYLIST_1] = 0x00;
/*0x13 -- RetryList Slot Index 2*/
	CODEPTRB[CPB_RETRYLIST_2] = 0x00;
/*0x14 -- RetryList Slot Index 3*/
	CODEPTRB[CPB_RETRYLIST_3] = 0x00;
/*0x15 -- RetryList Slot Index 4*/
	CODEPTRB[CPB_RETRYLIST_4] = 0x00;
/*0x16 -- RetryList Slot Index 5*/
	CODEPTRB[CPB_RETRYLIST_5] = 0x00;
/*0x17 -- RetryList Slot Index 6*/
	CODEPTRB[CPB_RETRYLIST_6] = 0x00;
/*0x18 -- RetryList Slot Index 7*/
	CODEPTRB[CPB_RETRYLIST_7] = 0x00;
/*0x19 -- RetryList Slot Index 8*/
	CODEPTRB[CPB_RETRYLIST_8] = 0x00;
/*0x1A -- RetryList Slot Index 9*/
	CODEPTRB[CPB_RETRYLIST_9] = 0x00;
/*0x1B -- RetryList Slot Index 10*/
	CODEPTRB[CPB_RETRYLIST_10] = 0x00;
/*0x1C -- RetryList Slot Index 11*/
	CODEPTRB[CPB_RETRYLIST_11] = 0x00;
/*0x1D -- RetryList Slot Index 12*/
	CODEPTRB[CPB_RETRYLIST_12] = 0x00;
/*0x1E -- RetryList Slot Index 13*/
	CODEPTRB[CPB_RETRYLIST_13] = 0x00;
/*0x1F -- Record the number of Slot in List should retry, 0 means no retry other slot*/
	CODEPTRB[CPB_RETRY_LIST_NUM] = 0x00;
#if MULTI_FW_SLOT
	CODEPTRB[CPB_RETRY_LIST_NUM] = 0x01;
#endif
#endif


//=================================================================
// Initial FW Slot ID: 0. Start Section: 0, Section Number: 2.
//=================================================================
	//CODEPTRB[(CP_FWSLOT0_OFFSET + CP_FWSLOT_ID)] = 0x00;
	//CODEPTRB[(CP_FWSLOT0_OFFSET + CP_FWTYPE)] 	 = 0x00;	// 0x00: Normal FW, 0x01: Loader
	//CODEPTRB[(CP_FWSLOT0_OFFSET + CP_CODEBANK_NUM)] = gCODE_SECTION[1].size / DEF_KB(32);	//暫定每個 Code bank 都是32K
	//CODEPTRW[(CP_FWSLOT0_OFFSET + CP_CSHPAGE_IDX) >> 1]  = 0x01;	// Zero page for IDPG, 1st Page for Code Sign Header
	//CODEPTRB[(CP_FWSLOT0_OFFSET + CP_CODEBANK_SECTORCNT)] = gCODE_SECTION[1].size / 512;;
	//CODEPTRW[(CP_FWSLOT0_OFFSET + CP_SIGNPAGE_IDX) >> 1] = 在後面算出數值後才給值;
	//CODEPTRB[(CP_FWSLOT0_OFFSET + CP_FWSTART_SECTION)] = 0x00;
	CODEPTRB[(CP_FWSLOT0_OFFSET + 0x00)] = 0x00;
	CODEPTRB[(CP_FWSLOT0_OFFSET + 0x01)] = 0x00;	// 0x00: Normal FW, 0x01: Loader
	CODEPTRB[(CP_FWSLOT0_OFFSET + 0x02)] = (unsigned char)m_Section_Sec_Cnt[1] / 64;	//暫定每個 Code bank 都是32K
	CODEPTRB[(CP_FWSLOT0_OFFSET + 0x03)] = (unsigned char)m_Section_Sec_Cnt[1];		// need 2 byte but Spec only allocate one byte 
	CODEPTRW[(CP_FWSLOT0_OFFSET + 0x04) >> 1]  = 0x01;	// Zero page for IDPG, 1st Page for Code Sign Header
	
	//CODEPTRW[(CP_FWSLOT0_OFFSET + CP_SIGNPAGE_IDX) >> 1] = 在後面算出數值後才給值;
	CODEPTRB[(CP_FWSLOT0_OFFSET + 0x08)] = 0x00;
	/*
	for (ubSectorIdx = 0; ubSectorIdx < CP_MAX_SECTOR_NUM; ubSectorIdx++) {
		if (gCODE_SECTION[CP_MAX_SECTOR_NUM - 1 - ubSectorIdx].size != 0) {
			break;
		}
		ubSectorNum = ubSectorNum - 1;
	}
	*/
	CODEPTRB[(CP_FWSLOT0_OFFSET + 0x09)] = ubSectorNum;	
	//CODEPTRB[(CP_FWSLOT0_OFFSET + CP_CBANKINATCM_SECCNT)] = (gCODE_SECTION[1].size - (32 << 10) ) / 512;
	CODEPTRB[(CP_FWSLOT0_OFFSET + 0x0A)] = 64; //CodeBank In ATCM SectorCnt, one bank(bank0)->32KByte->64 sectors

	ulFWSlot_Base = CP_FWSLOT0_OFFSET;
/* 0x10 ~ 0x1E -- Record each channel Block Index */
	CODEPTRW[(ulFWSlot_Base + 0x10) >> 1] = blk[0];		// default value	
	CODEPTRW[(ulFWSlot_Base + 0x12) >> 1] = blk[1];		// default value	
	CODEPTRW[(ulFWSlot_Base + 0x14) >> 1] = blk[0];		// default value
	CODEPTRW[(ulFWSlot_Base + 0x16) >> 1] = blk[1];		// default value
	CODEPTRW[(ulFWSlot_Base + 0x18) >> 1] = blk[0];		// default value
	CODEPTRW[(ulFWSlot_Base + 0x1A) >> 1] = blk[1];		// default value
	CODEPTRW[(ulFWSlot_Base + 0x1C) >> 1] = blk[0];		// default value
	CODEPTRW[(ulFWSlot_Base + 0x1E) >> 1] = blk[1];		// default value
	
	uwPageIdx = 0x00002;	// First page is for IDPG; Second Page is for Code Sign Header

	ulFWSlot_Base = CP_FWSLOT0_OFFSET;		//FW slot 0
	for(ubSectorIdx = 0; ubSectorIdx < 5; ubSectorIdx++) {
		ulFWSector_Base = ulFWSlot_Base + 0x80 + 16 * ubSectorIdx;
		
		CODEPTRW[(ulFWSector_Base) >> 1] = uwPageIdx;
		CODEPTRW[(ulFWSector_Base + 0x02) >> 1] = (m_Section_Sec_Cnt[ubSectorIdx]>>1) / _fh->PageSizeK;
		int ss = (m_Section_Sec_Cnt[ubSectorIdx]>>1) / _fh->PageSizeK;
		if ( ((m_Section_Sec_Cnt[ubSectorIdx]>>1) % _fh->PageSizeK) > 0 ) {
			CODEPTRW[(ulFWSector_Base + 0x02) >> 1] += 1;
		}
		CODEPTRW[(ulFWSector_Base + 0x04) >> 1] = (m_Section_Sec_Cnt[ubSectorIdx]);
		uwPageIdx = uwPageIdx + CODEPTRW[(ulFWSector_Base + 0x02) >> 1];
		// Every section CRC 32
		CODEPTRL[(ulFWSector_Base + 0x08)>>2]=m_Section_CRC32[ubSectorIdx];
		if(m_Section_Sec_Cnt[ubSectorIdx]==0)
		{
			memset(&CODEPTRW[(ulFWSector_Base) >> 1],0x00,16);
		}
	}
#if 0	// 開不開code都起的來
	// Set Target Base
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0x80 + 0x0C) >> 2] = 0x00000000;		//ATCM at CPU 0x00000000
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0x90 + 0x0C) >> 2] = 0x22180000;		//CODEBANK at CPU 0x22180000
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0xA0 + 0x0C) >> 2] = 0x000C0000;		//BTCM at CPU 0x000C0000
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0xB0 + 0x0C) >> 2] = 0x00281000;		//Andes at CPU 0x00280000 
	CODEPTRL[(CP_FWSLOT0_OFFSET + 0xC0 + 0x0C) >> 2] = 0x22180000 + 48*1024;	//FIP at CPU 0x000C0000
#endif
/*FW Slot0 Signature(Digest) Page Index*/
	CODEPTRW[(CP_FWSLOT0_OFFSET + 0x06) >> 1] = uwPageIdx;
	uwPageIdx = uwPageIdx + 0x01;	//已寫了Signature, Idx+1

	IDPGW[0x24>>1] = CODEPTRW[(CP_FWSLOT0_OFFSET + 0x06) >> 1] + 1;

/*0x04 -- CRC32 value of 4KB Code Pointer*/
	CODEPTRL[0x04>>2]=0x00000000;
	CODEPTRL[0x04>>2]=ldpcecc.E13_FW_CRC32((unsigned int*)CPdata, 4096);
	memcpy(buf,CPdata,sizeof(CPdata));
}

int PCIESCSI5013_ST::ScanWindows(unsigned char * output_buf, bool isDumpDllCenter)
{
	unsigned char cdb[512] = {0};
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x81;
	if (isDumpDllCenter)
		cdb[3] = 0x01;
	//-----CE0-----:4K Bytes
	//Die 0 Read: 512Bytes
	//Die 1 Read: 512Bytes
	//Die 2 Read: 512Bytes
	//Die 3 Read: 512Bytes
	//Die 0 Write: 512Bytes
	//Die 1 Write: 512Bytes
	//Die 1 Write: 512Bytes
	//Die 1 Write: 512Bytes

	//Max support 16CE
	if (isDumpDllCenter)
	{
		int rtn = Issue512CdbCmd(cdb, output_buf, 2048,  SCSI_IOCTL_DATA_IN, _SCSI_DelayTime);
		if (!rtn)
		{
			memcpy(_DQS_delay, output_buf+256, 2);
			memcpy(_DQ_delay, output_buf+512, 512);
			memcpy(_SDLL_offset, output_buf+1024, 256); //8CE*4DIE*4Byte*2CH = 128Byte * 2CH
		}
		return rtn;

	}
	else
		return Issue512CdbCmd(cdb, output_buf, 16*4096,  SCSI_IOCTL_DATA_IN, _SCSI_DelayTime);
}

void PCIESCSI5013_ST::PrepareIDpageForRDTBurner(int target_interface, int target_clock, int userSpecifyBlkCnt)
{
	IDPGB[IP_FLASH_ID+0] = _fh->bMaker;
	IDPGB[IP_FLASH_ID+1] = _fh->id[0];
	IDPGB[IP_FLASH_ID+2] = _fh->id[1];
	IDPGB[IP_FLASH_ID+3] = _fh->id[2];
	IDPGB[IP_FLASH_ID+4] = _fh->id[3];
	IDPGB[IP_FLASH_ID+5] = _fh->id[4];

	//if (0x9B == _fh->bMaker) {
	//	IDPGB[IP_VENDOR_TYPE] = UNIV_YMTC;
	//}
	//else if(0xAD == _fh->bMaker) {
	//	IDPGB[IP_VENDOR_TYPE] = UNIV_SKHYNIX;
	//}
	//else if(0x89 == _fh->bMaker || 0x2C == _fh->bMaker) {
	//	IDPGB[IP_VENDOR_TYPE] = UNIV_INTELMICRON;
	//}
	//else if(0x98 == _fh->bMaker || 0x45 == _fh->bMaker) {
	//	IDPGB[IP_VENDOR_TYPE] = UNIV_TOSHIBASANDISK;
	//}
	//else {
	//	IDPGB[IP_VENDOR_TYPE] = UNIV_ALL_SUPPORT_VENDOR_NUM;
	//}


	if (_fh->bIsBics2) {
		IDPGB[IP_FLASH_TYPE] = TOSHIBA_BICS2;
	}
	else if (   (0x98 == _fh->bMaker) &&  (_fh->bIsBics3 || _fh->bIsBics3pMLC || _fh->bIsBiCs3QLC) ) {
		IDPGB[IP_FLASH_TYPE] = TOSHIBA_BICS3;
	}
	else if (_fh->bIsBics4 && 0x45 == _fh->bMaker) {
		IDPGB[IP_FLASH_TYPE] = SANDISK_BICS4;
	}
	else if (_fh->bIsBiCs4QLC &&  0x98 == _fh->bMaker) {
		IDPGB[IP_FLASH_TYPE] = TOSHIBA_QLC_BICS4;
	}
	else if (_fh->bIsBics4 && 0x98 == _fh->bMaker) {
		IDPGB[IP_FLASH_TYPE] = TOSHIBA_BICS4;
	}
	else if (_fh->bIsMicronB16) {
		IDPGB[IP_FLASH_TYPE] = MICRON_B16A;
	}
	else if (_fh->bIsMicronB17) {
		IDPGB[IP_FLASH_TYPE] = MICRON_B17A;
	}
	else if ((_fh->bIsBics3) && (0x45 == _fh->bMaker)) {
		IDPGB[IP_FLASH_TYPE] = SANDISK_BICS3;
	}
	else if (_fh->bIsYMTCMLC3D || _fh->bIsYMTC3D ) {
		IDPGB[IP_FLASH_TYPE] = YMTC_GEN2;
	}
	else if (_fh->bIsMIcronN18) {
		IDPGB[IP_FLASH_TYPE] = MICRON_N18A;
	}
	else if (_fh->bIsMicronB27 && 0xA2 == _fh->id[3] ) {
		IDPGB[IP_FLASH_TYPE] = MICRON_B27A;
	}
	else if (_fh->bIsMIcronN28) {
		IDPGB[IP_FLASH_TYPE] = MICRON_N28A;
	}
	else if ( (0x98 == _fh->bMaker) && (0x15 == _fh->Design) && (0x04 == _fh->Cell)) {
		IDPGB[IP_FLASH_TYPE] = TOSHIBA_1Z_MLC;
	}
	else if ( (0x45 == _fh->bMaker) && (0x15 == _fh->Design) && (0x04 == _fh->Cell)) {
		IDPGB[IP_FLASH_TYPE] = SANDISK_1Z_MLC;
	}
	else if (_fh->bIsHynix3DV4) {
		IDPGB[IP_FLASH_TYPE] = HYNIX_3DV4;
	}
	else if (_fh->bIsMicronB27 && 0xE6 == _fh->id[3]) {
		IDPGB[IP_FLASH_TYPE] = MICRON_B27B;
	}
	else if (_fh->bIsMIcronB05) {
		IDPGB[IP_FLASH_TYPE] = MICRON_B05A;
	}
	else if (_fh->bIsHynix3DV5) {
		IDPGB[IP_FLASH_TYPE] = HYNIX_3DV5;
	}
	else if (_fh->bIsHynix3DV6) {
		IDPGB[IP_FLASH_TYPE] = HYNIX_3DV6;
	}
	else if (_fh->bIsMicronB36R) {
		IDPGB[IP_FLASH_TYPE] = MICRON_B36R;
	}
	else if (_fh->bIsMicronB37R) {
		IDPGB[IP_FLASH_TYPE] = MICRON_B37R;
	}
	else if (_fh->bIsMicronB47R) {
		IDPGB[IP_FLASH_TYPE] = MICRON_B47R;
	}
	else if (_fh->bIsMicronQ1428A) {
		IDPGB[IP_FLASH_TYPE] = MICRON_Q1428A;
	}
	int page_number_ratio = 1;
	if (0x02 == _fh->Cell) {
		//IDPGB[IP_CELL_TYPE] = UNIV_FLH_SLC;
		page_number_ratio = 1;
	}
	else if (0x04 == _fh->Cell) {
		//IDPGB[IP_CELL_TYPE] = UNIV_FLH_MLC;
		page_number_ratio = 2;
	}
	else if (0x08 == _fh->Cell) {
		//IDPGB[IP_CELL_TYPE] = UNIV_FLH_TLC;
		page_number_ratio = 3;
	}
	else if (0x010 == _fh->Cell) {
		//IDPGB[IP_CELL_TYPE] = UNIV_FLH_QLC;
		page_number_ratio = 4;
	}

	if (MyFlashInfo::IsSixAdressFlash(_fh))
	{
		IDPGB[IP_SPECIFY_FUNTION]=  IDPGB[IP_SPECIFY_FUNTION]  | SPECIFY_BIT_ENABLE_6_ADD ;
	}

	if (MyFlashInfo::IsAddressCycleRetryReadFlash(_fh))
	{
		IDPGB[IP_SPECIFY_FUNTION]=  IDPGB[IP_SPECIFY_FUNTION]  | SPECIFY_ACRR_ENABLE ;
	}

	if (GetPrivateProfileInt("E13", "GlobalRandomSeed", 1, _ini_file_name))
	{
		IDPGB[IP_SPECIFY_FUNTION]=  IDPGB[IP_SPECIFY_FUNTION]  | SPECIFY_GLOBAL_RANDOM_SEED ;
	}

	if ('T' == _fh->bToggle) {
		IDPGB[IP_DEFAULT_INTERFACE] =  FLH_INTERFACE_T1;
	}
	else {
		IDPGB[IP_DEFAULT_INTERFACE] =  FLH_INTERFACE_LEGACY;
	}

	if (0x2C == _fh->bMaker)
	{
		int micronTiming = 	MyFlashInfo::GetMicroClockTable(GetDataRateByClockIdx(target_clock), target_interface, _fh);
		IDPGB[IP_MICRON_TIMING] = micronTiming;
	}

	int tADL = GetPrivateProfileInt("[E13]", "tADL", -1, _ini_file_name);
	int tWHR = GetPrivateProfileInt("[E13]", "tWHR", -1, _ini_file_name);
	int tWHR2 = GetPrivateProfileInt("[E13]", "tWHR2", -1, _ini_file_name);
	if (-1 == tADL && -1 == tWHR && -1 == tWHR2)
	{
		GetACTimingTable( GetDataRateByClockIdx(target_clock)/2  , tADL, tWHR, tWHR2 );
	}

	IDPGB[IP_TIMING_ADL] = tADL;
	IDPGB[IP_TIMING_WHR] = tWHR;
	IDPGB[IP_TIMING_WHR2] = tWHR2;

	if (GetPrivateProfileInt("Configuration","1v2",0,_ini_file_name))
	{
		int  default_interface_1v2 = MyFlashInfo::GetFlashDefaultInterFace(12, _fh);
		if (GetPrivateProfileInt("Sorting", "UART_ST", 0, _ini_file_name))
		{

		}
		else
		{
			//NOT in uart sorting mode, boot code will read ID page 0x33 then enable differential pin, flash interface will be T2
			if (1 == default_interface_1v2)
			{
				default_interface_1v2 = 2;
			}
		}
		IDPGB[IP_DEFAULT_INTERFACE] = default_interface_1v2;
	}

	IDPGB[IP_TARGET_INTERFACE] = target_interface;

	IDPGB[IP_FLASH_CLOCK] = target_clock;

	GetODT_DriveSettingFromIni();
	IDPGB[IP_FLASH_DRIVE] = _flash_driving;
	IDPGB[IP_FLASH_ODT] = _flash_ODT;
	IDPGB[IP_CONTROLLER_DRIVING] = _controller_driving;
	IDPGB[IP_CONTROLLER_ODT] = _controller_ODT;


	IDPGB[IP_DIE_PER_CE] = _fh->DieNumber;
	IDPGB[IP_PLANE_PER_DIE] = _fh->Plane;

	int BlockCntPerPlane = _fh->Block_Per_Die/_fh->Plane;
	if (-1 != userSpecifyBlkCnt) {
		BlockCntPerPlane = userSpecifyBlkCnt/_fh->Plane;
	}
	IDPGB[IP_BLOCK_PER_PLANE<<1] = BlockCntPerPlane&0xFF;
	IDPGB[(IP_BLOCK_PER_PLANE<<1) + 1] = (BlockCntPerPlane>>8)&0xFF;
	IDPGB[IP_BLOCK_NUM_FOR_DIE_BIT<<1] = (_fh->DieOffset)&0xFF;
	IDPGB[(IP_BLOCK_NUM_FOR_DIE_BIT<<1) + 1] = (_fh->DieOffset>>8)&0xFF;
	IDPGB[IP_D1_PAGE_PER_BLOCK<<1] = _fh->PagePerBlock&0xFF;
	IDPGB[(IP_D1_PAGE_PER_BLOCK<<1) + 1] = (_fh->PagePerBlock>>8)&0xFF;
	IDPGB[IP_PAGE_PER_BLOCK<<1] = (_fh->PagePerBlock*page_number_ratio)&0xFF;
	IDPGB[(IP_PAGE_PER_BLOCK<<1)+1] = ((_fh->PagePerBlock*page_number_ratio)>>8) &0xFF;
	IDPGB[IP_DATA_BYTE_PER_PAGE<<1] = (_fh->PageSizeK*1024)&0xFF;
	IDPGB[(IP_DATA_BYTE_PER_PAGE<<1)+1] = ((_fh->PageSizeK*1024)>>8)&0xFF;
	IDPGB[IP_FLASH_BYTE_PER_PAGE<<1] = (_fh->PageSize)&0xFF;
	IDPGB[(IP_FLASH_BYTE_PER_PAGE<<1)+1] = (_fh->PageSize>>8)&0xFF;
	IDPGB[IP_FRAME_PER_PAGE] = _fh->PageSizeK/4;
	IDPGB[IP_PAGE_NUM_PER_WL] = page_number_ratio; 
}

#define E13_BURNER_TSB        "B5013_TSB"
#define E13_BURNER_MICRON     "B5013_MICRON"
#define E13_BURNER_YMTC       "B5013_YMTC"
#define E13_BURNER_HYNIX      "B5013_HYNIX"
#define E13_BURNER_DEFAULT    "B5013"

#define E13_RDT_TSB           "B5013_FW_TSB.bin"
#define E13_RDT_MICRON        "B5013_FW_MICRON.bin"
#define E13_RDT_YMTC          "B5013_FW_YMTC.bin"
#define E13_RDT_HYNIX         "B5013_FW_HYNIX.bin"
#define E13_RDT_DEFAULT       "B5013_FW.bin"

int PCIESCSI5013_ST::GetBurnerAndRdtFwFileName(FH_Paramater * fh,  char * burnerFileName,  char * RdtFileName)
{

	if (fh) {
		if (fh->bIsYMTC3D) {
			sprintf(burnerFileName,"%s", E13_BURNER_YMTC);
			sprintf(RdtFileName,"%s"   , E13_RDT_YMTC);
		} else if (fh->bIsBiCs4QLC) {
			sprintf(burnerFileName,"%s", E13_BURNER_TSB);
			sprintf(RdtFileName,"%s"   , E13_RDT_TSB);
		} else if ((fh->bMaker == 0x98 ||fh->bMaker == 0x45 ) && (fh->bIsBics4 ||fh->bIsBics2 || fh->bIsBics3) && (fh->beD3)){
			sprintf(burnerFileName,"%s", E13_BURNER_TSB);
			sprintf(RdtFileName,"%s"   , E13_RDT_TSB);
		} else if ((fh->bMaker == 0x98) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf(burnerFileName,"%s", E13_BURNER_DEFAULT);
			sprintf(RdtFileName,"%s"   , E13_RDT_DEFAULT);
		} else if ((fh->bMaker == 0x45) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf(burnerFileName,"%s", E13_BURNER_DEFAULT);
			sprintf(RdtFileName,"%s"   , E13_RDT_DEFAULT);
		} else if (fh->bIsMicronB16 || fh->bIsMicronB17) {
			sprintf(burnerFileName,"%s", E13_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E13_RDT_MICRON);
		} else if (fh->bIsMIcronN18) {
			sprintf(burnerFileName,"%s", E13_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E13_RDT_MICRON);
		} else if (fh->bIsMIcronN28) {
			sprintf(burnerFileName,"%s", E13_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E13_RDT_MICRON);
		} else if (fh->bIsHynix3DV4 || fh->bIsHynix3DV5 ||fh->bIsHynix3DV6) {
			sprintf(burnerFileName,"%s", E13_BURNER_HYNIX);
			sprintf(RdtFileName,"%s"   , E13_RDT_HYNIX);
		} else if (fh->bIsMIcronB05) {
			sprintf(burnerFileName,"%s", E13_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E13_RDT_MICRON);
		} else if (fh->bIsMicronB27 || fh->bIsMicronB37R || fh->bIsMicronB36R || fh->bIsMicronB47R || fh->bIsMicronQ1428A) {
			sprintf(burnerFileName,"%s", E13_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E13_RDT_MICRON);
		} else {
			sprintf(burnerFileName,"%s", E13_BURNER_DEFAULT);
			sprintf(RdtFileName,"%s"   , E13_RDT_DEFAULT);
		}
	}
	unsigned long long current_interface = _deviceType & INTERFACE_MASK;

	if ( (IF_ATA == current_interface) || (IF_ASMedia == current_interface) || (IF_C2012 == current_interface)  )
		strcat(burnerFileName, "ATA");

	strcat(burnerFileName, ".bin");

	return 0;
}

int PCIESCSI5013_ST::Dev_SetFWSeed ( int page,int block )
{
	unsigned char ncmd[16] = {0};
	ncmd[0] = 0x56;//nvme 48byte	
	ncmd[1] = page & 0xFF;
	ncmd[2] = (page>>8)& 0xFF;
	ncmd[3] = block & 0xFF;
	ncmd[4] = (block>>8)& 0xFF;
	unsigned char buf[16];
	memset(buf,0x00,16);
	log_nvme("%s\n", __FUNCTION__);

	if (MyIssueNVMeCmd(ncmd, 0xD1, NVME_DIRCT_WRITE, buf, sizeof(buf), 15, true))
	{
		return -1;
	}

	return 0;
}

int PCIESCSI5013_ST::Dev_SetFlashPageSize ( int pagesize )
{
	int frame_cnt = _fh->PageSizeK/4;
	int ldpc_mode = 2;
	pagesize /= frame_cnt;
	pagesize-= 4112;
	pagesize /= 2;
	if(pagesize>304)
		ldpc_mode = 7;
	else if(pagesize>288)
		ldpc_mode = 8;
	else if(pagesize>256)
		ldpc_mode= 0;
	else if(pagesize>240)
		ldpc_mode= 1;
	else if(pagesize>224)
		ldpc_mode = 2;
	else if(pagesize>208)
		ldpc_mode = 3;
	else if(pagesize>176)
		ldpc_mode = 4;
	else if(pagesize>144)
		ldpc_mode = 5;
	else
		ldpc_mode= 6;

	return Dev_SetEccMode(ldpc_mode);
}



int PCIESCSI5013_ST::GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &tWHR2)
{
	if (100 <= flh_clk_mhz  && 200 >= flh_clk_mhz )
	{
		tADL = 6;  tWHR = 1;  tWHR2 = 41;
	}
	else if (266 == flh_clk_mhz)
	{
		tADL = 11;  tWHR = 3;  tWHR2 = 55;	
	}
	else if (333 == flh_clk_mhz)
	{
		tADL = 15;  tWHR = 4;  tWHR2 = 71;
	}
	else if (400 == flh_clk_mhz)
	{
		tADL = 21;  tWHR = 6;  tWHR2 = 90;
	}
	else if (flh_clk_mhz <=50)
	{
		tADL = 6;  tWHR = 1;  tWHR2 = 1;
	} 
	else {
		nt_file_log("GetACTimingTable:can't get table for flash clk %d Mhz", flh_clk_mhz);
		assert(0);
	}
	return 0;
}


int PCIESCSI5013_ST::GetScanWinWorstPattern(unsigned char * buf, int len)
{
	unsigned char SCAN_WIN_WORST_PATTERN[512] = 
	{
		0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x7F, 0x80, 0x7F, 0x80, 0x7F, 0x80, 0x00, 0xFF, 0x00, 0xFF, 
		0x00, 0xFF, 0xBF, 0x40, 0xBF, 0x40, 0xBF, 0x40, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xDF, 0x20, 
		0xDF, 0x20, 0xDF, 0x20, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xEF, 0x10, 0xEF, 0x10, 0xEF, 0x10, 
		0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xF7, 0x08, 0xF7, 0x08, 0xF7, 0x08, 0x00, 0xFF, 0x00, 0xFF, 
		0x00, 0xFF, 0xFB, 0x04, 0xFB, 0x04, 0xFB, 0x04, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFD, 0x02, 
		0xFD, 0x02, 0xFD, 0x02, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 
		0x80, 0xFF, 0xFF, 0x80, 0x00, 0x7F, 0x7F, 0x00, 0x80, 0xFF, 0x7F, 0x00, 0xFF, 0x80, 0x00, 0x7F, 
		0x80, 0xFF, 0x00, 0x7F, 0xFF, 0x80, 0x7F, 0x00, 0x40, 0xFF, 0xFF, 0x40, 0x00, 0xBF, 0xBF, 0x00, 
		0x40, 0xFF, 0xBF, 0x00, 0xFF, 0x40, 0x00, 0xBF, 0x40, 0xFF, 0x00, 0xBF, 0xFF, 0x40, 0xBF, 0x00, 
		0x20, 0xFF, 0xFF, 0x20, 0x00, 0xDF, 0xDF, 0x00, 0x20, 0xFF, 0xDF, 0x00, 0xFF, 0x20, 0x00, 0xDF, 
		0x20, 0xFF, 0x00, 0xDF, 0xFF, 0x20, 0xDF, 0x00, 0x10, 0xFF, 0xFF, 0x10, 0x00, 0xEF, 0xEF, 0x00, 
		0x10, 0xFF, 0xEF, 0x00, 0xFF, 0x10, 0x00, 0xEF, 0x10, 0xFF, 0x00, 0xEF, 0xFF, 0x10, 0xEF, 0x00, 
		0x08, 0xFF, 0xFF, 0x08, 0x00, 0xF7, 0xF7, 0x00, 0x08, 0xFF, 0xF7, 0x00, 0xFF, 0x08, 0x00, 0xF7, 
		0x08, 0xFF, 0x00, 0xF7, 0xFF, 0x08, 0xF7, 0x00, 0x04, 0xFF, 0xFF, 0x04, 0x00, 0xFB, 0xFB, 0x00, 
		0x04, 0xFF, 0xFB, 0x00, 0xFF, 0x04, 0x00, 0xFB, 0x04, 0xFF, 0x00, 0xFB, 0xFF, 0x04, 0xFB, 0x00, 
		0x02, 0xFF, 0xFF, 0x02, 0x00, 0xFD, 0xFD, 0x00, 0x02, 0xFF, 0xFD, 0x00, 0xFF, 0x02, 0x00, 0xFD, 
		0x02, 0xFF, 0x00, 0xFD, 0xFF, 0x02, 0xFD, 0x00, 0x01, 0xFF, 0xFF, 0x01, 0x00, 0xFE, 0xFE, 0x00, 
		0x01, 0xFF, 0xFE, 0x00, 0xFF, 0x01, 0x00, 0xFE, 0x01, 0xFF, 0x00, 0xFE, 0xFF, 0x01, 0xFE, 0x00, 
		0x00, 0x80, 0xFF, 0xFF, 0x80, 0x00, 0x7F, 0x7F, 0x00, 0x80, 0xFF, 0x7F, 0x00, 0xFF, 0x80, 0x00, 
		0x7F, 0x80, 0xFF, 0x00, 0x7F, 0xFF, 0x80, 0x7F, 0x00, 0x40, 0xFF, 0xFF, 0x40, 0x00, 0xBF, 0xBF, 
		0x00, 0x40, 0xFF, 0xBF, 0x00, 0xFF, 0x40, 0x00, 0xBF, 0x40, 0xFF, 0x00, 0xBF, 0xFF, 0x40, 0xBF, 
		0x00, 0x20, 0xFF, 0xFF, 0x20, 0x00, 0xDF, 0xDF, 0x00, 0x20, 0xFF, 0xDF, 0x00, 0xFF, 0x20, 0x00, 
		0xDF, 0x20, 0xFF, 0x00, 0xDF, 0xFF, 0x20, 0xDF, 0x00, 0x10, 0xFF, 0xFF, 0x10, 0x00, 0xEF, 0xEF, 
		0x00, 0x10, 0xFF, 0xEF, 0x00, 0xFF, 0x10, 0x00, 0xEF, 0x10, 0xFF, 0x00, 0xEF, 0xFF, 0x10, 0xEF, 
		0x00, 0x08, 0xFF, 0xFF, 0x08, 0x00, 0xF7, 0xF7, 0x00, 0x08, 0xFF, 0xF7, 0x00, 0xFF, 0x08, 0x00, 
		0xF7, 0x08, 0xFF, 0x00, 0xF7, 0xFF, 0x08, 0xF7, 0x00, 0x04, 0xFF, 0xFF, 0x04, 0x00, 0xFB, 0xFB, 
		0x00, 0x04, 0xFF, 0xFB, 0x00, 0xFF, 0x04, 0x00, 0xFB, 0x04, 0xFF, 0x00, 0xFB, 0xFF, 0x04, 0xFB, 
		0x00, 0x02, 0xFF, 0xFF, 0x02, 0x00, 0xFD, 0xFD, 0x00, 0x02, 0xFF, 0xFD, 0x00, 0xFF, 0x02, 0x00, 
		0xFD, 0x02, 0xFF, 0x00, 0xFD, 0xFF, 0x02, 0xFD, 0x00, 0x01, 0xFF, 0xFF, 0x01, 0x00, 0xFE, 0xFE, 
		0x00, 0x01, 0xFF, 0xFE, 0x00, 0xFF, 0x01, 0x00, 0xFE, 0x01, 0xFF, 0x00, 0xFE, 0xFF, 0x01, 0xFE, 
		0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x7F, 0x80, 0x7F, 0x80, 0x7F, 0x80, 0x00, 0xFF, 0x00, 
		0xFF, 0x00, 0xFF, 0xBF, 0x40, 0xBF, 0x40, 0xBF, 0x40, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xDF
	};
	
	if (  (0 == len) || !buf )
	{
		return 1;
	}
	int COPY_CNT = len/512;
	int REMAINDER = len%512;
	int write_idx = 0;
	for (int copy_idx=0; copy_idx<COPY_CNT; copy_idx++)
	{
		memcpy(&buf[write_idx], &SCAN_WIN_WORST_PATTERN[0] ,512 );
		write_idx += 512;
	}
	if (REMAINDER)
	{
		memcpy(&buf[write_idx], &SCAN_WIN_WORST_PATTERN[0], REMAINDER);
	}
	return 0;
}

/*E13 not support command*/
int PCIESCSI5013_ST::NT_ReadPage_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, const char *type, unsigned char *bBuf, unsigned char *DriveInfo)
{
	return 0;
}

int PCIESCSI5013_ST::ST_CheckDRY_DQS(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE TYPE,BYTE CE_bit_map,DWORD Block) 
{
	return 0;
}

int PCIESCSI5013_ST::ST_SetDegree(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int Phase, short delayOffset) 
{
	return 0;
}

int PCIESCSI5013_ST::ST_FWInit(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk) 
{
	return 0;
}

int PCIESCSI5013_ST::ST_GetSRAM_GWK(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char* w_buf) 
{
	return 0;
}

int PCIESCSI5013_ST::ST_DumpSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf)
{

	return 0;
}

int PCIESCSI5013_ST::ST_WriteSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf)
{
	return 0;
}

int PCIESCSI5013_ST::ST_SetQuickFindIBBuf(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf)
{
	return 0;
}

int PCIESCSI5013_ST::ST_QuickFindIB(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int ce, unsigned int dieoffset, unsigned int BlockCount, unsigned char *buf, int buflen,
									int iTimeCount, BYTE bIBSearchPage, BYTE CheckPHSN)
{
	return -1; /*0 : pass, -1: fail*/
}

int PCIESCSI5013_ST::ST_CheckU3Port(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* buf)
{
	return 0;
}

int PCIESCSI5013_ST::ST_APGetSetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE SetGet,BYTE Address,BYTE Param0,BYTE Param1,BYTE Param2,BYTE Param3,unsigned char *buf)
{
	return 0;
}

int PCIESCSI5013_ST::NT_JumpISPMode(BYTE obType,HANDLE DeviceHandle,char drive)
{
	return 0;
}

int PCIESCSI5013_ST::ST_HynixCheckWritePortect(BYTE bType,HANDLE DeviceHandle, BYTE bDrvNum)
{
	return 0;
}

int PCIESCSI5013_ST::ST_HynixTLC16ModeSwitch(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bMode)
{
	return 0;
}

