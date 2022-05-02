
#include "SATASCSI3111_ST.h"
#include "universal_interface/MemManager.h"
#include "time.h"

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

#define OPCMD_COUNT_3111 30

int clock_idx_to_Mhz_3111[] =  {0, 10, 40, 50, 83, 100*2, 166*2, 180*2, 200*2, 226*2};

static const unsigned char OpCmd3111[OPCMD_COUNT_3111] = 
{0x00, 0x01, 0x09, 0x14, 0x40, 0x43, 0x44, 0x81, 0xA1, 0xA2, 0xA3, 0xA5, 0xA8, 0xAA, 0xAC, 0xAF,
 0xBB, 0xBC, 0xBD, 0xBF, 0xCE, 0xD0, 0xEC, 0xEE, 0xEB, 0xEF, 0xFC, 0xFE, 0xFB, 0xFF};

#ifndef ASSERT
#define ASSERT assert
#endif
SATASCSI3111_ST::SATASCSI3111_ST( DeviveInfo* info, const char* logFolder )
:USBSCSICmd_ST(info,logFolder)
{
    Init();
}

SATASCSI3111_ST::SATASCSI3111_ST( DeviveInfo* info, FILE *logPtr )
:USBSCSICmd_ST(info,logPtr)
{
    Init();
}

SATASCSI3111_ST::~SATASCSI3111_ST()
{
#ifdef _DUALCORE_SWITCH_
	DualCoreObjectsFree();
#endif
}


int SATASCSI3111_ST::GetDataRateByClockIdx(int clk_idx)
{
	return clock_idx_to_Mhz_3111[clk_idx];
}
void SATASCSI3111_ST::Init()
{
	_deviceType = IF_ATA|DEVICE_SATA|DEVICE_SATA_3111;
	_burnerMode = false;
	_timeOut = 20;
	_flash_driving = 0;
	_flash_ODT = 0;
	_controller_driving = 0;
	_controller_ODT = 0;

	memset(_flash_id_boot_cache, 0 ,sizeof(_flash_id_boot_cache));

#ifdef _DUALCORE_SWITCH_
	DualCoreObjectsInit();
#endif
}

void SATASCSI3111_ST::GetODT_DriveSettingFromIni()
{
	_flash_driving = GetPrivateProfileIntA ( "S11","FlashDriving", 2,  _ini_file_name); //0:UNDER 1:NORMAL 2:OVER
	_flash_ODT = GetPrivateProfileIntA ( "S11","FlashODT", 0, _ini_file_name ); //not support
	_controller_driving = GetPrivateProfileIntA ( "S11","ControlDriving", 3, _ini_file_name );//0:50 1:35 2:25 3:18 (ohm)
	_controller_ODT = GetPrivateProfileIntA ( "S11","ControlODT", 4, _ini_file_name );//0:150 1:100 2:75 3:50 4:0ff (ohm)

	if (_fh->bIsMIcronN28)
	{
		_flash_driving = 1; //N28A spec says it is not support OVER drive.
	}
}

int SATASCSI3111_ST::_MakeSATABuf( const UCHAR* sql,int sqllen, const UCHAR* buf, int len )
{

	if( buf ) {
		assert( (len%512)==0 );
		memcpy(_SATABuf,buf,len);
		return len/512;
	}
	else {
		//assert( (sqllen%512)==0 );
		memcpy(_SATABuf,sql,sqllen);
		return (sqllen+511)/512;
	}
	//return sizeof(_SATABuf);
	assert(0);
}

int SATASCSI3111_ST::Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len )
{
	if (SCSI_NTCMD == m_cmdtype)
	{
		return USBSCSICmd_ST::Dev_IssueIOCmd(sql,  sqllen,  mode,  ce,  buf,  len );
	}
	int hmode = mode&0xF0;
	mode = mode&0x0F;

	int satalen;
	if ( (mode&0x03)==1 ) {
		satalen = _MakeSATABuf( sql,sqllen, buf, len );

		if ( _WriteDDR(_SATABuf,len,ce) ) {
			Log ( LOGDEBUG, "Write DDR fail \n" );
			return 1;
		}
	}

	if ( !hmode ) {
		if ( _TrigerDDR(ce,sql,sqllen,len,_workingChannels) ) {
			Log ( LOGDEBUG, "Trigger DDR fail \n" );
			return 2;
		}
		//#ifdef DEBUGSATABIN
		if ( mode==2 ) {

			if( _ReadDDR(buf,len,0) ) {
				Log ( LOGDEBUG, "Read DDR fail \n" );
				return 3;
			}
		}
		else if ( mode==3 ) {

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

int SATASCSI3111_ST::ATA_Identify( UCHAR* buf, int len )
{
	int ret = IdentifyATA( _hUsb ,buf, len, _deviceType );
	if ( ret==0 ) {
		memset(_fwVersion,0x00,8);
		int idx=0;
		for(int i=46 ; i<=53 ; idx+=2,i+=2) {
			_fwVersion[idx] = buf[i+1];
			_fwVersion[idx+1] = buf[i];
		}
		_fwVersion[idx] = '\0';		
	}
	return ret;
}

bool SATASCSI3111_ST::Identify_ATA(char * version)
{
	unsigned char buf[512]={0};

	int rtn = ATA_Identify( buf, 512);
	memcpy(version, _fwVersion, 8);
	return rtn?true:false;
}

int SATASCSI3111_ST::Dev_InitDrive( const char* ispfile )
{
	sprintf_s(_ispFile,"%s",ispfile);
	InitFlashInfoBuf (0, 0, 0, 4 );

	Log ( LOGDEBUG, "SATA Device ISPFile:%s\n",ispfile );
	//_hUsb = sataCmd->MyCreateHandle ( drivenum, true );
	// //init flash
	// if ( sataCmd->InitFlash (1) ) {
	// if ( sataCmd->InitFlash (1) ) {
	// printf( "Init fail\n" );
	// //MyCloseHandle(&_hUsb);
	// return false;
	// }
	// }

	MemStr fn ( 512 );
	CharMemMgr buf(512);
	unsigned char *pBuf = buf.Data();
	if ( ispfile[0]!='N' || ispfile[1]!='A') {
		unsigned char bufinq[512];
		if ( ATA_Identify( bufinq, sizeof(bufinq) ) ) {
			//MyCloseHandle(&_hUsb);
			return false;
		}
		const char* fw = _fwVersion;

		if ( fw[0]!='S' || fw[2]!='B' || fw[3]!='M'   ) {
			bool jump = false;
			for ( int loop=0; loop<3; loop++ ) {
				//-- Must back to ROM code --
				if ( Dev_CheckJumpBackISP (true,false ) !=0 )  {
					Log ( LOGDEBUG, "### Err: Can't jump to ROM code\n" );
					UFWSleep ( 500 );
				}
				else {
					jump = true;
					break;
				}
			}
			if (!jump ) {
				Log ( LOGDEBUG, "### Err: Can't jump to ROM code\n" );
				//MyCloseHandle(&_hUsb);
				return false;
			}

			const char* fisp = ispfile;
			//Log ( LOGDEBUG, "start ATA_ISPProgPRAM %s\n",fisp );
			if ( ATA_ISPProgPRAM(fisp)!=0 ) {
				Log ( LOGDEBUG, "### Err: ISP program fail\n" );
				//MyCloseHandle(&_hUsb);
				return false;
			}
			UFWSleep ( 200 );
			//Log ( LOGDEBUG, "start ATA_ISPJumpMode\n" );
			//            if(sataCmd->ATA_ISPJumpMode())  {
			//                Log(LOGDEBUG,"Jump ISP Fail!");
			//                return false;
			//            }
			//            Log(LOGDEBUG,"Jump ISP Pass!");
			//            ::Sleep ( 10000 );
		}
		//Log ( LOGDEBUG, "start ATA_Identify\n" );
		for ( int l=0; l<=16; l++ ) {
			if ( ATA_Identify( bufinq, sizeof(bufinq) )==0 ) {
				Log(LOGDEBUG,"FW Version %s\n",fw);
				if ( fw[0]=='S' && fw[2]=='B' && (fw[3]=='M' || fw[3]=='K' || fw[3]=='D') ) {
					break;
				}
			}
			UFWSleep ( 1000 );
		}

		Log(LOGDEBUG,"FW Version %s\n",fw);
		if ( fw[0]!='S' || fw[2]!='B' || (fw[3]!='M' && fw[3]!='K' && fw[3]!='D')   ) {
			Log ( LOGDEBUG, "### Err: Can not switch bruner\n" );
			//MyCloseHandle(&_hUsb);
			return false;
		}
	}

	//if ( _setUSBConvertInterface==0 )
	ATA_GetFlashConfig();
	Dev_SetFlashPageSize( GetParam()->PageSize );
	ATA_GetFlashConfig();

	int ret = ATA_SetAPKey();
	if ( ret ) {
		Log ( LOGDEBUG, "### Err: Set AP Key fail\n" );
		return 1;
	}

	_sql = SQLCmd::Instance(this, &_FH, _eccBufferAddr, _dataBufferAddr);
	//SetAPParameter2CTL();

	return 0;
}


int SATASCSI3111_ST::Dev_GetFlashID(unsigned char* r_buf)
{
	Log ( LOGDEBUG, "%s\n",__FUNCTION__ );

	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0x14;      //Feature
	_CurATARegister[1] = 0x01;      //SC
	_CurATARegister[2] = 0x00;         //SN
	_CurATARegister[3] = 0x00;         //CL
	_CurATARegister[4] = 0;      //CH
	_CurATARegister[5] = 0xA0;      //DH
	_CurATARegister[6] = 0x21;      //CMD

	//(drive,Pre,Cur,Direction,Buf,DataLength,true,48bit)
	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister, 1,r_buf,512,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	//if ( (_CurOutRegister[6]&0x40) || (_CurOutRegister[6]&0x01) ) {
	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::Dev_CheckJumpBackISP ( bool needJump, bool bSilence )
{
	Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	unsigned char bufinq[512];
	int ret= 0;
	// only test 5-1 times.
	for ( int retry=0; retry<5; retry++ ) {
		if ( ATA_Identify( bufinq, sizeof(bufinq) ) ) {
			return 1;
		}
		const char* fw = _fwVersion;
		//if ( memcmp(_fwVersion,"SBRM",4)==0 ) {
		if ( fw[0]=='S' && fw[2]=='R' && fw[3]=='M'   ) {
			return 0;
		}

		ret=ATA_ForceToBootRom ( 2 );
		if ( ret ) {
			Log ( LOGDEBUG, "Force to Boot fail %d\n",ret );
			continue;
		}
		if (ATA_Identify( bufinq, sizeof(bufinq) ) ) {
			return 1;
		}
		//if ( memcmp(_fwVersion,"SBRM",4)==0 ) {
		if ( fw[0]=='S' && fw[2]=='R' && fw[3]=='M'   ) {
			return 0;
		}
		ret=ATA_ISPJumpMode ();
		if ( ret ) {
			Log ( LOGDEBUG, "Jump isp fail %d\n",ret );
			continue;
		}
		UFWSleep ( 1000 );
	}
	return 2;
}

int SATASCSI3111_ST::Dev_GetSRAMData ( unsigned char offset, unsigned char* buf, int len )
{
	//datatype: 0=data, 1=vender, 2=ecc 3=time, 4=time+reset
	assert( offset==0 );
	return _ReadDDR( buf,len, offset );
}
int SATASCSI3111_ST::Dev_SetSRAMData ( unsigned char offset, const unsigned char* buf, int len )
{
	assert( offset==0 );
	return _WriteDDR((unsigned char*)buf,len, 0);
}

//////////////////////////////////////
int SATASCSI3111_ST::Dev_SetFlashClock ( int clock )
{
	//    Log(LOGDEBUG, "SATA: Not support %s!\n",__FUNCTION__);
	//    return 0;

	Log ( LOGDEBUG, "%s\n",__FUNCTION__ );

	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	//use to match 3111 clock only, it is not same frequence.
	int clkMode = 0; // 0x0 ~ 0xE, --> 40/(n+2)Mhz
	if ( clock>=100 ) {
		clkMode = 0;
	}
	else if ( clock>=66 ) {
		clkMode = 1;
	}
	else if ( clock>=50 ) {
		clkMode = 2;
	}
	else if ( clock>=33 ) {
		clkMode = 3;
	}
	else if ( clock>=16 ) {
		clkMode = 4;
	}
	else {
		assert(0);
	}

	memset(_dummyBuf,0xFF,512);
	_dummyBuf[0] = clkMode;

	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0]=0xF1;           //ATA Feature
	_CurATARegister[1]=0x01;           //ATA SecCnt
	_CurATARegister[2]=0x00;           //ATA SecNum
	_CurATARegister[3]=0x00;           //ATA CylLo
	_CurATARegister[4]=0x01;           //ATA CylHi
	_CurATARegister[5]=0x00;           //ATA DrvHd
	_CurATARegister[6]=0x31;           //ATA CMD

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,_dummyBuf,512,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return ATA_GetFlashConfig();;
}

int SATASCSI3111_ST::Dev_GetRdyTime ( int action, unsigned int* busytime )
{
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	unsigned char buf[512];
	_ReadDDR(buf,sizeof(buf), action!=0 ? 4:3 );
	//LogMEM(buf, 512);
	*busytime = buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
	return 0;
}
int SATASCSI3111_ST::Dev_SetFlashDriving ( int driving )
{
	Log(LOGDEBUG, "SATA: Not support %s!\n",__FUNCTION__);
	return 0;
}

int SATASCSI3111_ST::Dev_SetEccMode ( unsigned char ldpcMode )
{
	Log ( LOGDEBUG, "%s\n",__FUNCTION__ );

	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	memset(_dummyBuf,0xFF,512);
	_dummyBuf[2] = ldpcMode;

	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0]=0xF1;           //ATA Feature
	_CurATARegister[1]=0x01;           //ATA SecCnt
	_CurATARegister[2]=0x00;           //ATA SecNum
	_CurATARegister[3]=0x00;           //ATA CylLo
	_CurATARegister[4]=0x01;           //ATA CylHi
	_CurATARegister[5]=0x00;           //ATA DrvHd
	_CurATARegister[6]=0x31;           //ATA CMD

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,_dummyBuf,512,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::Dev_SetFWSpare ( unsigned char Spare )
{
	Log(LOGDEBUG, "SATA: Not support %s!\n",__FUNCTION__);
	assert(0);
	return 0;
}

int SATASCSI3111_ST::GetEccRatio(void)
{
	if (SCSI_NTCMD == m_cmdtype) {
		return USBSCSICmd_ST::GetEccRatio();
	}
	return 4;
}

int SATASCSI3111_ST::_TrigerDDR(int CE, const unsigned char* sqlbuf, int slen, int buflen, unsigned char channel )
{
	if (SCSI_NTCMD == m_cmdtype)
	{
		return USBSCSICmd_ST::_TrigerDDR(CE, sqlbuf, slen, buflen, channel );
	}
	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	//if (CE>=_workCECnt) {
	//    Log ( LOGDEBUG, "CE[%d] > Device CE Count[%d]\n",CE,_workCECnt );
	//    assert(CE<_workCECnt);
	//}

	//UCHAR CurATARegister[8],PreATARegister[8];
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);
	memset(_dummyBuf,0x00,512);
	memcpy(_dummyBuf,sqlbuf,slen);

	_CurATARegister[0] = 0xFE;
	_CurATARegister[1] = 0x01;
	_CurATARegister[2] = channel;
	_CurATARegister[3] = CE;
	_CurATARegister[4] = buflen/1024;
	_CurATARegister[5] = 0xE0;
	_CurATARegister[6] = 0x31;

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,_dummyBuf,512,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::_WriteDDR(unsigned char* w_buf,int Length, int ce)
{
	if (SCSI_NTCMD == m_cmdtype)
	{
		return USBSCSICmd_ST::_WriteDDR(w_buf,Length, ce);
	}
	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	//UCHAR CurATARegister[8],PreATARegister[8];
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);
	UINT SC;
	SC = Length/512;

	_CurATARegister[0] = 0xFC;    //Feature
	_CurATARegister[1] = SC;      //SC
	//    if ( ce==-1 ) {
	//        _CurATARegister[2] = 0x00;    //SN
	//        _CurATARegister[3] = 0x00;    //CL
	//    }
	//    else {
	//        _CurATARegister[2] = 0x01;    //SN
	//        _CurATARegister[3] = ce;    //CL
	//    }
	_CurATARegister[4] = 0x00;    //CH
	_CurATARegister[5] = 0xE0;    //DH
	_CurATARegister[6] = 0x31;    //CMD

	//Log ( LOGDEBUG, "%s %d %d [%02X %02X %02X %02X %02X %02X %02X %02X]\n",__FUNCTION__, Length,ce,
	//      _CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7]
	//    );

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,w_buf,(SC*512),true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

//datatype: 0=data, 1=vender, 2=ecc 3=time, 4=time+reset
int SATASCSI3111_ST::_ReadDDR(unsigned char* r_buf,int Length, int datatype )
{
	if (SCSI_NTCMD == m_cmdtype)
	{
		return USBSCSICmd_ST::_ReadDDR(r_buf,Length, datatype );
	}
	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);
	UINT SC;
	SC = Length/512;

	_CurATARegister[0] = 0xFD;
	_CurATARegister[1] = SC;
	_CurATARegister[2] = 0x00;
	_CurATARegister[3] = 0x00;
	_CurATARegister[4] = datatype;
	_CurATARegister[5] = 0xE0;
	_CurATARegister[6] = 0x21;

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,1,r_buf,(SC*512),true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::ATA_GetFlashConfig()
{
	Log ( LOGDEBUG, "%s\n",__FUNCTION__ );

	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	memset(_dummyBuf,0,512);

	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0]=0xF0;           //ATA Feature
	_CurATARegister[1]=0x01;           //ATA SecCnt
	_CurATARegister[2]=0x00;           //ATA SecNum
	_CurATARegister[3]=0x00;           //ATA CylLo
	_CurATARegister[4]=0x01;           //ATA CylHi
	_CurATARegister[5]=0x00;           //ATA DrvHd
	_CurATARegister[6]=0x21;           //ATA CMD

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,1,_dummyBuf,512,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	Log(LOGDEBUG, "SATA:%s!\n",__FUNCTION__);
	//LogMEM(_dummyBuf,512);

	return 0;
}

int SATASCSI3111_ST::ATA_SetAPKey()  //Enable AP-KEY
{
	//if( _setUSBConvertInterface ) return 0;
	//Log ( LOGDEBUG, "%s\n",__FUNCTION__ );
	memset(_PreATARegister,0x00,8);
	memset(_CurATARegister,0x00,8);

	_CurATARegister[0] = 0x00;
	_CurATARegister[1] = 0x6F;
	_CurATARegister[2] = 0xFE;
	_CurATARegister[3] = 0xEF;
	_CurATARegister[4] = 0xFA;
	_CurATARegister[5] = 0xAF;
	_CurATARegister[6] = 0xE0;

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,0,NULL,0,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if( (_deviceType%INTERFACE_MASK!=IF_ATA) || (_CurOutRegister[1]==0x01) && (_CurOutRegister[2]==0x01) && (_CurOutRegister[6]&0x40) )  {
		return 0; //pass
	}

	{
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
	}

	return 2;
}

int SATASCSI3111_ST::ATA_ISPProgPRAM(const char* m_MPBINCode)
{
	UINT loop,i,tempint,Sector_no,remainder,quotient,Base_Address,Byte_Length,Byte_Length_ICCM;
	UINT Length1,Length2,Length3,Length4,Length5,Length6,Length7;
	UINT rAddress,rByteLength;
	UINT glasted,ICode_Offset;
	UCHAR *m_srcbuf, CommonBuffer[512];
	int state;
	bool pDecode=true;
	bool new_structure=false;
	//MLC:  Old: ICCM+DCCM+FPU
	//      New: ICCM+DCCM+Á©∫Á?+Á©∫Á?+FPU+FPU2+ICode
	//TLC:  old: ICCM+DCCM+Á©∫Á?+Á©∫Á?+FPU+FPU2
	//      New: ICCM+DCCM+Á©∫Á?+Á©∫Á?+FPU+FPU2+ICode
	errno_t err;
	FILE * fp;
	err =  fopen_s(&fp,m_MPBINCode, "rb");
	if(err)
	{
		Log(LOGDEBUG, "Can not open MP BIN File: %s successfully !",m_MPBINCode);
		return 1;
	}
	
	fseek(fp, 0, SEEK_END);
	ULONG filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(filesize == -1L)
	{
		Log(LOGDEBUG, "Failed ! Check MP BIN File size %s = 0 ",m_MPBINCode);
		fclose(fp);
		return 2;
	}

	m_srcbuf = (UCHAR *)malloc(filesize);
	memset(m_srcbuf,0,filesize);
	
	state = fread(m_srcbuf, 1, filesize, fp);
	//state = _read(fh,m_srcbuf,filesize);
	//if ( _read(fh,m_srcbuf,filesize) <= 0 )  //Bin file have problem
	if(state <= 0)
	{
		Log(LOGDEBUG, "Reading MP BIN File Failed !!!,state= %d",state);
		free(m_srcbuf);
		fclose(fp);
		return 3;
	}
	fclose(fp);

	//=== Check BIN Code Header ===
	// if(!BINHeaderCheck_3111(m_srcbuf,filesize))
	// {
	// free(m_srcbuf);
	// return false;
	// }

	//1.?ºVendor Send Seed CMD(?≥Random Seed Header ??6KBË≥áÊ??≤‰?ÔºåÁµ¶boot code‰πãÂ?decode??
	if(ATA_SetAPKey())  //state=0
	{
		Log(LOGDEBUG, "Set APKey fail");
		free(m_srcbuf);
		return 4;
	}

	tempint=512;

	if(ATA_Vender_Send_Seed(m_srcbuf+tempint,0x20)) //16KB
	{
		Log(LOGDEBUG, "Set Seed fail");
		free(m_srcbuf);
		return 5;
	}
	//AppendToListBox("Vender_Send_Seed done.");

	ICode_Offset=0;
	//2.Âæûaddress & length Q  area ‰æùÂ?ËÆÄ 8byte?≤‰?ÔºåÂ?4bytes?æbase addressÔºåÂ?4bytes?æByte Length,
	//3.AP?ºVender Write SRAM  CMDÔºåÂÇ≥Base Address?≤‰?Áµ¶bootcode.
	//4.AP?ºVender Program RAM  CMD(Âæûdata area‰æùÂ??ñburner  code?ÅÈÄ≤‰?ÔºåÁî±Base Address?ãÂ??æÔ?Cylinder LowÊ±∫Â?Ë¶Å‰?Ë¶Ådecode?∂ÈÄ≤‰??Ñcode)
	//  Â¶ÇÊ?codeÂ§ßÊñº128 sectorÔºåÂ??∞Ê≠•È©?  <Code sizeË∂ÖÈ?64KB??  Â§öÈ??®Â??∏Â??â‰??ÄË®àÁ??ácodeË£ÅÂ?Â∏åÊ??ΩÁî±APÁ´ØÂü∑Ë°?
	//  ?çË?Ê≠•È?2~5ÔºåÁõ¥?∞Ê??âÂ?Â°äÈÉΩ?õ‰?Burner code?∫Ê≠¢Q  area ËÆÄ
	for(i=0; i<7; i++) //0: ICCM, 1:DCCM
	{
		Base_Address=((m_srcbuf[512+8*i+3]<<24) | (m_srcbuf[512+8*i+2]<<16) | (m_srcbuf[512+8*i+1]<<8) | m_srcbuf[512+8*i+0]);
		Byte_Length=((m_srcbuf[512+8*i+7]<<24) | (m_srcbuf[512+8*i+6]<<16) | (m_srcbuf[512+8*i+5]<<8) | m_srcbuf[512+8*i+4]);

		if(i==0)
			Byte_Length_ICCM=Byte_Length;

		if(i==0)
			Length1=Byte_Length;
		else if(i==1)
			Length2=Byte_Length;
		else if(i==2)
			Length3=Byte_Length;
		else if(i==3)
			Length4=Byte_Length;
		else if(i==4)
			Length5=Byte_Length;
		else if(i==5)
			Length6=Byte_Length;
		else if(i==6)
			Length7=Byte_Length;

		if(i<6)
			ICode_Offset+=Byte_Length;

		if(Byte_Length==0)
			continue;

		if(Byte_Length % 512)
		{
			if(i==0) {
				Log(LOGDEBUG,"ICCM Byte_Length must multiple of 512 bytes. !!!");
			}
			else if(i==1) {
				Log(LOGDEBUG,"DCCM Byte_Length must multiple of 512 bytes. !!!");
			}
			free(m_srcbuf);
			return 6;
		}

		if(i==6)  //New Structure: ICode
		{
			new_structure=true;
			break;
		}

		loop=0;
		Sector_no=Byte_Length/512;
		quotient=Sector_no/128;
		remainder=Sector_no%128;
		for(loop=0; loop<quotient; loop++)
		{
			rAddress=Base_Address+loop*128*512;
			rByteLength=128*512;

			if(ATA_SetAPKey())   //state=0
			{
				free(m_srcbuf);
				return 7;
			}

			memset(CommonBuffer,0,512);
			CommonBuffer[0]=rAddress & 0xFF;
			CommonBuffer[1]=(rAddress>>8) & 0xFF;
			CommonBuffer[2]=(rAddress>>16) & 0xFF;
			CommonBuffer[3]=(rAddress>>24) & 0xFF;

			CommonBuffer[4]=rByteLength & 0xFF;
			CommonBuffer[5]=(rByteLength>>8) & 0xFF;
			CommonBuffer[6]=(rByteLength>>16) & 0xFF;
			CommonBuffer[7]=(rByteLength>>24) & 0xFF;

			if(ATA_DirectWriteSRAM(CommonBuffer,1,0,0))
			{
				Log(LOGDEBUG,"Issue DirectWriteSRAM cmd. Failed !!");
				free(m_srcbuf);
				return 8;
			}

			if(ATA_SetAPKey())  //state=0
			{
				free(m_srcbuf);
				return 9;
			}

			tempint=512+16*1024+(loop*128*512);

			if(i==1)
				tempint+=Length1;
			else if(i==2)
				tempint+=(Length1+Length2);
			else if(i==3)
				tempint+=(Length1+Length2+Length3);
			else if(i==4)
				tempint+=(Length1+Length2+Length3+Length4);
			else if(i==5)
				tempint+=(Length1+Length2+Length3+Length4+Length5);

			if(ATA_ISPProgramPRAM(m_srcbuf+tempint,128,pDecode))
			{
				if(i==0) {
					Log(LOGDEBUG,"ISPProgramPRAM_ICCM_loop:%d failed !!",loop);
				}
				else if(i==1)
					Log(LOGDEBUG,"ISPProgramPRAM_DCCM_loop:%d failed !!",loop);
				free(m_srcbuf);
				return 10;
			}

			// if(i==0)
			// Log(LOGDEBUG,"     ISPProgramPRAM_ICCM_loop:%d done.",loop);
			// else if(i==1)
			// Log(LOGDEBUG,"     ISPProgramPRAM_DCCM_loop:%d done. !!",loop);
		}

		if(remainder > 0)
		{
			rAddress=Base_Address+loop*128*512;
			rByteLength=remainder*512;

			if(ATA_SetAPKey())  //state=0
			{
				free(m_srcbuf);
				return 11;
			}

			memset(CommonBuffer,0,512);
			CommonBuffer[0]=rAddress & 0xFF;
			CommonBuffer[1]=(rAddress>>8) & 0xFF;
			CommonBuffer[2]=(rAddress>>16) & 0xFF;
			CommonBuffer[3]=(rAddress>>24) & 0xFF;

			CommonBuffer[4]=rByteLength & 0xFF;
			CommonBuffer[5]=(rByteLength>>8) & 0xFF;
			CommonBuffer[6]=(rByteLength>>16) & 0xFF;
			CommonBuffer[7]=(rByteLength>>24) & 0xFF;

			if(ATA_DirectWriteSRAM(CommonBuffer,1,0,0))
			{
				Log(LOGDEBUG,"Issue DirectWriteSRAM cmd. Failed !!");
				free(m_srcbuf);
				return 12;
			}

			if(ATA_SetAPKey())  //state=0
			{
				free(m_srcbuf);
				return 13;
			}

			tempint=512+16*1024+(loop*128*512);

			if(i==1)
				tempint+=Length1;
			else if(i==2)
				tempint+=(Length1+Length2);
			else if(i==3)
				tempint+=(Length1+Length2+Length3);
			else if(i==4)
				tempint+=(Length1+Length2+Length3+Length4);
			else if(i==5)
				tempint+=(Length1+Length2+Length3+Length4+Length5);

			if(ATA_ISPProgramPRAM(m_srcbuf+tempint,remainder,pDecode))
			{
				if(i==0) {
					Log(LOGDEBUG,"ISPProgramPRAM_ICCM_lasted failed !!");
				}
				else if(i==1) {
					Log(LOGDEBUG,"ISPProgramPRAM_DCCM_lasted failed !!");
				}
				free(m_srcbuf);
				return 14;
			}

			// if(i==0)
			// message.Format("     ISPProgramPRAM_ICCM_lasted done.");
			// else if(i==1)
			// message.Format("     ISPProgramPRAM_DCCM_lasted done. !!");
		}
	}
	//AppendToListBox("ISP Program ICCM/DCCM done.");

	//check CRC
	//Âæûaddress & length Q  area ‰æùÂ?ËÆÄ 8byte?≤‰?ÔºåÂ?4bytes?æBase AddressÔºåÂ?4bytes?æByte Length
	//Vender Write SRAM  CMDÔºåÂÇ≥Base AddressË∑üByte Length?≤‰?Áµ¶bootcode.
	//Vender Check CRC(‰æùÊ?Base Address?áByte Length?öcode?ÑCRC CheckÔºåÂ??≥Ê™¢?•Á??úÂ???
	for(i=0; i<7; i++) //0: ICCM, 1:DCCM
	{
		Base_Address=((m_srcbuf[512+8*i+3]<<24) | (m_srcbuf[512+8*i+2]<<16) | (m_srcbuf[512+8*i+1]<<8) | m_srcbuf[512+8*i+0]);
		Byte_Length=((m_srcbuf[512+8*i+7]<<24) | (m_srcbuf[512+8*i+6]<<16) | (m_srcbuf[512+8*i+5]<<8) | m_srcbuf[512+8*i+4]);

		if(Byte_Length==0)
			continue;

		if(i==6)
			break;

		if(ATA_SetAPKey())  //state=0
		{
			free(m_srcbuf);
			return 15;
		}

		memset(CommonBuffer,0,512);
		CommonBuffer[0]=Base_Address & 0xFF;
		CommonBuffer[1]=(Base_Address>>8) & 0xFF;
		CommonBuffer[2]=(Base_Address>>16) & 0xFF;
		CommonBuffer[3]=(Base_Address>>24) & 0xFF;

		CommonBuffer[4]=Byte_Length & 0xFF;
		CommonBuffer[5]=(Byte_Length>>8) & 0xFF;
		CommonBuffer[6]=(Byte_Length>>16) & 0xFF;
		CommonBuffer[7]=(Byte_Length>>24) & 0xFF;

		if(ATA_DirectWriteSRAM(CommonBuffer,1,0,0))
		{
			Log(LOGDEBUG,"Issue DirectWriteSRAM cmd. Failed !!");
			free(m_srcbuf);
			return 16;
		}

		if(ATA_SetAPKey())  //state=0
		{
			free(m_srcbuf);
			return 17;
		}

		memset(CommonBuffer,0,512);
		if(ATA_Vender_Check_CRC(CommonBuffer,1))
		{
			if(i==0) {
				Log(LOGDEBUG,"Issue Vender_Check_CRC_ICCM cmd. failed !!");
			}
			else if(i==1) {
				Log(LOGDEBUG,"Issue Vender_Check_CRC_DCCM cmd. failed !!");
			}
			free(m_srcbuf);
			return 18;
		}

		//Result=0x55AA55AAË°®Á§∫correctÔºåResult=0xFFFFFFFFË°®Á§∫fail??
		tempint=((CommonBuffer[11]<<24) | (CommonBuffer[10]<<16) | (CommonBuffer[9]<<8) | CommonBuffer[8]);
		if(tempint != 0x55AA55AA)
		{
			if(i==0) {
				Log(LOGDEBUG,"CRC for ICCM failed (0x%x) !!",tempint);
			}
			else if(i==1) {
				Log(LOGDEBUG,"CRC for DCCM failed (0x%x) !!",tempint);
			}
			free(m_srcbuf);
			return 19;
		}

		// if(i==0)
		// message.Format("     Vender_Check_CRC_ICCM done.");
		// else if(i==1)
		// message.Format("     Vender_Check_CRC_DCCM done. !!");
		// AppendToListBox(message);
	}
	//AppendToListBox("Check CRC done.");

	Log ( LOGDEBUG, "start ATA_ISPJumpMode\n" );
	if(ATA_ISPJumpMode())  {
		Log(LOGDEBUG,"Jump ISP Fail!");
		free(m_srcbuf);
		return 20;
	}
	Log(LOGDEBUG,"Jump ISP Pass!");
	UFWSleep ( 5000 );
	//free(m_srcbuf);

	//write ICode to SDR
	if(new_structure)
	{
		i=6;
		//Byte_Length_ICode
		{
			Base_Address=((m_srcbuf[512+8*i+3]<<24) | (m_srcbuf[512+8*i+2]<<16) | (m_srcbuf[512+8*i+1]<<8) | m_srcbuf[512+8*i+0]);
			Byte_Length=((m_srcbuf[512+8*i+7]<<24) | (m_srcbuf[512+8*i+6]<<16) | (m_srcbuf[512+8*i+5]<<8) | m_srcbuf[512+8*i+4]);
		}
		//message.Format("ISPProgramPRAM_ICode (Length=%d):",Byte_Length);
		//AppendToListBox(message);
		Sector_no=Byte_Length/512;
		quotient=Sector_no/128;
		remainder=Sector_no%128;
		for(loop=0; loop<quotient; loop++)
		{
			//if(No_Header)
			//   tempint=(16*1024)+ICode_Offset+(loop*128*512);
			//else
			tempint=512+(16*1024)+ICode_Offset+(loop*128*512);

			glasted=0;
			if(remainder==0)
			{
				if(loop==(quotient-1))
					glasted=1;
			}

			if(ATA_SetAPKey())   //state=0
			{
				free(m_srcbuf);
				return 21;
			}

			if(!ATA_ISPProgramPRAM_ICode(m_srcbuf+tempint,128,loop,glasted))
			{
				Log(LOGDEBUG,"Issue ATA_ISPProgramPRAM_ICode, Loop_%d Failed !!",loop);
				//Msg_str.Format("Issue ATA_ISPProgramPRAM_ICode, Loop_%d Failed !!",loop);
				//AppendToListBox(Msg_str);
				//ErrorRecord();
				free(m_srcbuf);
				return 22;
			}
			//message.Format("     ISPProgramPRAM_ICode_Loop:%d done. !!",loop);
			//AppendToListBox(message);
		}

		if(remainder > 0)
		{
			//if(No_Header)
			//  tempint=(16*1024)+ICode_Offset+(loop*128*512);
			//else
			tempint=512+(16*1024)+ICode_Offset+(loop*128*512);

			if(ATA_SetAPKey())   //state=0
			{
				free(m_srcbuf);
				return 23;
			}

			if(!ATA_ISPProgramPRAM_ICode(m_srcbuf+tempint,remainder,loop,1))
			{
				Log(LOGDEBUG,"Issue ATA_ISPProgramPRAM_ICode, lasted Failed !!");
				//Msg_str.Format("Issue ATA_ISPProgramPRAM_ICode, lasted Failed !!");
				//AppendToListBox(Msg_str);
				//ErrorRecord();
				free(m_srcbuf);
				return 24;
			}
			//message.Format("     ISPProgramPRAM_ICode_lasted done. !!");
			//AppendToListBox(message);
		}

		//check
		for(loop=0; loop<quotient; loop++)
		{
			//if(No_Header)
			//   tempint=(16*1024)+ICode_Offset+(loop*128*512);
			//else
			tempint=512+(16*1024)+ICode_Offset+(loop*128*512);

			if(ATA_SetAPKey())   //state=0
			{
				free(m_srcbuf);
				return 25;
			}

			if(!ATA_ISPVerifyPRAM_ICode(m_srcbuf+tempint,128,loop))
			{
				Log(LOGDEBUG,"Issue ISVerifyPRAM_ICode_3111, Loop_%d Failed !!",loop);
				//Msg_str.Format("Issue ISVerifyPRAM_ICode_3111, Loop_%d Failed !!",loop);
				//AppendToListBox(Msg_str);
				//ErrorRecord();
				free(m_srcbuf);
				return 26;
			}
			//message.Format("     ISPVerifyPRAM_ICode_Loop:%d done. !!",loop);
			//AppendToListBox(message);
		}

		if(remainder > 0)
		{
			//if(No_Header)
			//   tempint=(16*1024)+ICode_Offset+(loop*128*512);
			//else
			tempint=512+(16*1024)+ICode_Offset+(loop*128*512);

			if(ATA_SetAPKey())   //state=0
			{
				free(m_srcbuf);
				return 27;
			}

			if(!ATA_ISPVerifyPRAM_ICode(m_srcbuf+tempint,remainder,loop))
			{
				Log(LOGDEBUG,"Issue ATA_ISPVerifyPRAM_ICode, lasted Failed !!");
				//Msg_str.Format("Issue ATA_ISPVerifyPRAM_ICode, lasted Failed !!");
				//AppendToListBox(Msg_str);
				//ErrorRecord();
				free(m_srcbuf);
				return 28;
			}
			//message.Format("     ISPVerifyPRAM_ICode_lasted done. !!");
			//AppendToListBox(message);
		}
	}

	//check running on Burner Code Now?


	//Msg_str.Format("ISP PRAM Code OK");
	//AppendToListBox(Msg_str);
	return 0;
}


bool SATASCSI3111_ST::ATA_ISPProgramPRAM_ICode(UCHAR* w_buf,UINT SectorNum,UINT gOffset,UINT glasted)
{
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0xBD;      //Feature
	_CurATARegister[1] = SectorNum; //SC
	_CurATARegister[2] = 0x00;      //SN
	_CurATARegister[3] = glasted;      //CL
	_CurATARegister[4] = gOffset;      //CH
	_CurATARegister[5] = 0xA0;      //DH
	_CurATARegister[6] = 0x31;      //CMD

	//(drive,Pre,Cur,Direction,Buf,DataLength,true,48bit)
	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,w_buf,(SectorNum*512),true,false,15,false))
		return false;

	//if(CurOutRegister[6] != 0x50)
	if(!(_CurOutRegister[6] & 0x40) || (_CurOutRegister[6] & 0x01))
		return false;

	return true;
}

//-------------------------------------------------------------------------
bool SATASCSI3111_ST::ATA_ISPVerifyPRAM_ICode(UCHAR* w_buf,UINT SectorNum,UINT gOffset)
{
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0xBE;      //Feature
	_CurATARegister[1] = SectorNum; //SC
	_CurATARegister[2] = 0x00;      //SN
	_CurATARegister[3] = 0x00;      //CL
	_CurATARegister[4] = gOffset;      //CH
	_CurATARegister[5] = 0xA0;      //DH
	_CurATARegister[6] = 0x21;      //CMD

	//(drive,Pre,Cur,Direction,Buf,DataLength,true,48bit)
	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,1,w_buf,(SectorNum*512),true,false,15,false))
		return false;

	//if(CurOutRegister[6] != 0x50)
	if(!(_CurOutRegister[6] & 0x40) || (_CurOutRegister[6] & 0x01))
		return false;

	return true;
}

int SATASCSI3111_ST::ATA_Vender_Check_CRC(UCHAR* buf,UINT SectorNum)
{
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0x30;      //Feature
	_CurATARegister[1] = SectorNum; //SC
	_CurATARegister[2] = 0x00;      //SN
	_CurATARegister[3] = 0x00;      //CL
	_CurATARegister[4] = 0x00;      //CH
	_CurATARegister[5] = 0xA0;      //DH
	_CurATARegister[6] = 0x21;      //CMD

	//(drive,Pre,Cur,Direction,Buf,DataLength,true,48bit)
	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,1,buf,(SectorNum*512),true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::ATA_ISPProgramPRAM(UCHAR* buf,UINT SectorNum,bool pDecodeEnable)
{
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0x40;      //Feature
	_CurATARegister[1] = SectorNum; //SC
	_CurATARegister[2] = 0x00;      //SN
	_CurATARegister[3] = (pDecodeEnable? 0x01:0x00);      //CL
	_CurATARegister[4] = 0x00;      //CH
	_CurATARegister[5] = 0xA0;      //DH
	_CurATARegister[6] = 0x31;      //CMD

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,buf,(SectorNum*512),true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}


int SATASCSI3111_ST::ATA_DirectReadSRAM(UCHAR* buf,UCHAR SectorCount,UCHAR CL,UCHAR CH)
{

	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0x12;        //Feature
	_CurATARegister[1] = SectorCount; //SC
	_CurATARegister[2] = 0;           //SN
	_CurATARegister[3] = CL;         //CL
	_CurATARegister[4] = CH;         //CH
	_CurATARegister[5] = 0xA0;        //DH
	_CurATARegister[6] = 0x21;        //CMD


	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,1,buf,(SectorCount*512),true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}
	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}
int SATASCSI3111_ST::ATA_DirectWriteSRAM(UCHAR* buf,UCHAR SectorCount,UINT pCL,UINT pCH)
{
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0x24;        //Feature
	_CurATARegister[1] = SectorCount; //SC
	_CurATARegister[2] = 0;           //SN
	_CurATARegister[3] = pCL;         //CL
	_CurATARegister[4] = pCH;         //CH
	_CurATARegister[5] = 0xA0;        //DH
	_CurATARegister[6] = 0x31;        //CMD

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,buf,(SectorCount*512),true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::ATA_Vender_Send_Seed(UCHAR* buf,UINT SectorNum)
{
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0x80;      //Feature
	_CurATARegister[1] = SectorNum; //SC
	_CurATARegister[2] = 0;         //SN
	_CurATARegister[3] = 0;         //CL
	_CurATARegister[4] = 0;         //CH
	_CurATARegister[5] = 0xA0;      //DH
	_CurATARegister[6] = 0x31;      //CMD

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,buf,SectorNum*512,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::ATA_Vendor_Write4ByteReg(UCHAR* buf,UINT Byte_No)
{
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0x60;      //Feature
	_CurATARegister[1] = 0x01;      //SC
	_CurATARegister[2] = 0;         //SN
	_CurATARegister[3] = Byte_No;   //CL
	_CurATARegister[4] = 0;         //CH
	_CurATARegister[5] = 0xA0;      //DH
	_CurATARegister[6] = 0x31;      //CMD


	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,buf,512,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::ATA_Vendor_Read4ByteReg(UCHAR* buf,UINT Byte_No)
{
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0] = 0x61;      //Feature
	_CurATARegister[1] = 0x01;      //SC
	_CurATARegister[2] = 0;         //SN
	_CurATARegister[3] = Byte_No;   //CL
	_CurATARegister[4] = 0;         //CH
	_CurATARegister[5] = 0xA0;      //DH
	_CurATARegister[6] = 0x21;      //CMD


	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,1,buf,512,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::ATA_ForceToBootRom ( int type )
{
	Log ( LOGDEBUG, "%s\n",__FUNCTION__ );

	UCHAR TmpBuf[512];
	UINT Address=0x04000044;

	memset(TmpBuf,0,512);
	//Address
	TmpBuf[0]=(Address & 0xFF);
	TmpBuf[1]=((Address>>8) & 0xFF);
	TmpBuf[2]=((Address>>16) & 0xFF);
	TmpBuf[3]=((Address>>24) & 0xFF);
	//Value
	TmpBuf[4]=(type & 0xFF);
	TmpBuf[5]=((type>>8) & 0xFF);
	TmpBuf[6]=((type>>16) & 0xFF);
	TmpBuf[7]=((type>>24) & 0xFF);

	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	ret = ATA_Vendor_Write4ByteReg(TmpBuf,1);
	if ( ret ) {
		return ret;
	}

	ret = ATA_ISPJumpMode();
	if ( ret ) {
		return ret;
	}

	TmpBuf[4]=0x00;
	TmpBuf[5]=0x00;
	TmpBuf[6]=0x00;
	TmpBuf[7]=0x00;

	ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	ret = ATA_Vendor_Write4ByteReg(TmpBuf,1);
	if ( ret ) {
		return ret;
	}

	return 0;
}

int SATASCSI3111_ST::Dev_SetFlashPageSize ( int pagesize )
{
	Log ( LOGDEBUG, "%s\n",__FUNCTION__ );

	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	int pageSize  = pagesize; //GetParam()->PageSize;
	int msgSize = (4096+16+32);
	int frameCnt = pageSize/msgSize;
	assert( (frameCnt%2)==0 );
	int msgRSize = msgSize*frameCnt;
	int diff = (pageSize-msgRSize)/frameCnt;
	int ldpcMode = 0;
	if ( diff>=496 ) {
		ldpcMode = 0;
	}
	else if ( diff>=440 ) {
		ldpcMode = 1;
	}
	else if ( diff>=420 ) {
		ldpcMode = 2;
	}
	else if ( diff>=360 ) {
		ldpcMode = 3;
	}
	else if ( diff>=272 ) {
		ldpcMode = 4;
	}
	else {
		assert(0);
	}

	int sizeMode = 0;
	switch(GetParam()->PageSizeK)
	{
	case 4:
		sizeMode=0;
		break;
	case 8:
		sizeMode=1;
		break;
	case 16:
		sizeMode=2;
		break;
	default:
		assert(0);
	};


	memset(_dummyBuf,0xFF,512);
	_dummyBuf[1] = sizeMode;
	_dummyBuf[2] = ldpcMode;

	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	_CurATARegister[0]=0xF1;           //ATA Feature
	_CurATARegister[1]=0x01;           //ATA SecCnt
	_CurATARegister[2]=0x00;           //ATA SecNum
	_CurATARegister[3]=0x00;           //ATA CylLo
	_CurATARegister[4]=0x01;           //ATA CylHi
	_CurATARegister[5]=0x00;           //ATA DrvHd
	_CurATARegister[6]=0x31;           //ATA CMD

	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2,_dummyBuf,512,true,false,_timeOut,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}

int SATASCSI3111_ST::ATA_ISPJumpMode()
{
	Log ( LOGDEBUG, "%s\n",__FUNCTION__ );

	int ret = ATA_SetAPKey();
	if ( ret ) {
		return ret;
	}

	memset(_dummyBuf,0,512);
	memset(_CurATARegister,0x00,8);
	memset(_PreATARegister,0x00,8);

	unsigned char tempbuffer[512] = {0};

	_CurATARegister[0] = 0x0F;      //Feature
	_CurATARegister[1] = 0x01;      //SC
	_CurATARegister[2] = 0x00;      //SN
	_CurATARegister[3] = 0x00;      //CL , 0=default , 1=not send fis to os.
	_CurATARegister[4] = 0x00;      //CH
	_CurATARegister[5] = 0xA0;      //DH
	_CurATARegister[6] = 0x31;      //CMD
	//(drive,Pre,Cur,Direction,Buf,DataLength,true,48bit)
	if(MyIssueATACmd(_PreATARegister,_CurATARegister,_CurOutRegister,2, tempbuffer, 512, true, false,30,false)) {
		Log ( LOGDEBUG, "%s command fail\n",__FUNCTION__ );
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 1;
	}

	// Log ( LOGDEBUG, "out result [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
	// _CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
	// _CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
	// );

	if ( _CurOutRegister[6]!=0x50 ) {
		Log ( LOGDEBUG, "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			_CurATARegister[0],_CurATARegister[1],_CurATARegister[2],_CurATARegister[3],_CurATARegister[4],_CurATARegister[5],_CurATARegister[6],_CurATARegister[7] ,
			_CurOutRegister[0],_CurOutRegister[1],_CurOutRegister[2],_CurOutRegister[3],_CurOutRegister[4],_CurOutRegister[5],_CurOutRegister[6],_CurOutRegister[7]
		);
		return 2;
	}

	return 0;
}




/////////////////////////////////////////////////////////////////////////Sorting API//////////////////////////////////////////////////////////////////////
#define S11_BURNER_MLC             "B3111_MLC.bin"
#define S11_BURNER_SANDISK_MLC     "B3111_SANDISK_MLC.bin"
#define S11_BURNER_TLC             "B3111_TLC.bin"
#define S11_BURNER_QLC             "B3111_QLC.bin"
#define S11_BURNER_SANDISK_TLC     "B3111_SANDISK_TLC.bin"
#define S11_BURNER_B05             "B3111_B05.bin"
#define S11_BURNER_B16             "B3111_B16.bin"
#define S11_BURNER_B27B            "B3111_B27B.bin"
#define S11_BURNER_N18             "B3111_N18.bin"
#define S11_BURNER_N28             "B3111_N28.bin"
#define S11_BURNER_HYNIX           "B3111_HYNIX.BIN"	//hynix 3dv4/3dv6
#define S11_BURNER_YMTC            "B3111_YMTC.BIN"
#define S11_BURNER_DEFAULT         "B3111_DEFAULT.BIN"

/* Hub can't not support multi port switch in the meantime so we need cs the make sure 
there is only one port switch in the same time, H/W limitation*/
using namespace std;


static UCHAR Scan_SDLL_WORST_PATTERN[482]=
{
	0,255,0,255,0,255,254,1,254,1,254,1,0 ,255 ,0 ,255 ,0 ,255 ,253 ,2 ,253 
	,2 ,253 ,2 ,0 ,255 ,0 ,255 ,0 ,255 ,251 ,4 ,251 ,4 ,251 ,4 ,0 ,255 ,0 ,255 ,0 
	,255 ,247 ,8 ,247 ,8 ,247 ,8 ,0 ,255 ,0 ,255 ,0 ,255 ,239 ,16 ,239 ,16 ,239 ,16 ,0 
	,255 ,0 ,255 ,0 ,255 ,223 ,32 ,223 ,32 ,223 ,32 ,0 ,255 ,0 ,255 ,0 ,255 ,191 ,64 ,191 
	,64 ,191 ,64 ,0 ,255 ,0 ,255 ,0 ,255 ,127 ,128 ,127 ,128 ,127 ,128 ,1 ,255 ,255 ,1 ,0 
	,254 ,254 ,0 ,1 ,255 ,254 ,0 ,255 ,1 ,0 ,254 ,1 ,255 ,0 ,254 ,255 ,1 ,254 ,0 ,2 
	,255 ,255 ,2 ,0 ,253 ,253 ,0 ,2 ,255 ,253 ,0 ,255 ,2 ,0 ,253 ,2 ,255 ,0 ,253 ,255 
	,2 ,253 ,0 ,4 ,255 ,255 ,4 ,0 ,251 ,251 ,0 ,4 ,255 ,251 ,0 ,255 ,4 ,0 ,251 ,4 
	,255 ,0 ,251 ,255 ,4 ,251 ,0 ,8 ,255 ,255 ,8 ,0 ,247 ,247 ,0 ,8 ,255 ,247 ,0 ,255 
	,8 ,0 ,247 ,8 ,255 ,0 ,247 ,255 ,8 ,247 ,0 ,16 ,255 ,255 ,16 ,0 ,239 ,239 ,0 ,16 
	,255 ,239 ,0 ,255 ,16 ,0 ,239 ,16 ,255 ,0 ,239 ,255 ,16 ,239 ,0 ,32 ,255 ,255 ,32 ,0 
	,223 ,223 ,0 ,32 ,255 ,223 ,0 ,255 ,32 ,0 ,223 ,32 ,255 ,0 ,223 ,255 ,32 ,223 ,0 ,64 
	,255 ,255 ,64 ,0 ,191 ,191 ,0 ,64 ,255 ,191 ,0 ,255 ,64 ,0 ,191 ,64 ,255 ,0 ,191 ,255 
	,64 ,191 ,0 ,128 ,255 ,255 ,128 ,0 ,127 ,127 ,0 ,128 ,255 ,127 ,0 ,255 ,128 ,0 ,127 ,128 
	,255 ,0 ,127 ,255 ,128 ,127 ,0 ,0 ,1 ,255 ,255 ,1 ,0 ,254 ,254 ,0 ,1 ,255 ,254 ,0 
	,255 ,1 ,0 ,254 ,1 ,255 ,0 ,254 ,255 ,1 ,254 ,0 ,2 ,255 ,255 ,2 ,0 ,253 ,253 ,0 
	,2 ,255 ,253 ,0 ,255 ,2 ,0 ,253 ,2 ,255 ,0 ,253 ,255 ,2 ,253 ,0 ,4 ,255 ,255 ,4 
	,0 ,251 ,251 ,0 ,4 ,255 ,251 ,0 ,255 ,4 ,0 ,251 ,4 ,255 ,0 ,251 ,255 ,4 ,251 ,0 
	,8 ,255 ,255 ,8 ,0 ,247 ,247 ,0 ,8 ,255 ,247 ,0 ,255 ,8 ,0 ,247 ,8 ,255 ,0 ,247 
	,255 ,8 ,247 ,0 ,16 ,255 ,255 ,16 ,0 ,239 ,239 ,0 ,16 ,255 ,239 ,0 ,255 ,16 ,0 ,239 
	,16 ,255 ,0 ,239 ,255 ,16 ,239 ,0 ,32 ,255 ,255 ,32 ,0 ,223 ,223 ,0 ,32 ,255 ,223 ,0 
	,255 ,32 ,0 ,223 ,32 ,255 ,0 ,223 ,255 ,32 ,223 ,0 ,64 ,255 ,255 ,64 ,0 ,191 ,191 ,0 
	,64 ,255 ,191 ,0 ,255 ,64 ,0 ,191 ,64 ,255 ,0 ,191 ,255 ,64 ,191 ,0 ,128 ,255 ,255 ,128 
	,0 ,127 ,127 ,0 ,128 ,255 ,127 ,0 ,255 ,128 ,0 ,127 ,128 ,255 ,0 ,127 ,255 ,128 ,127 
};






void SATASCSI3111_ST::EnableDualMode() 
{


}
static const char CRITICAL_ERR[20][128] = 
{
	"UNDEFINE",              //0
	"UNDEFINE",              //1 
	"FPU Timeout",           //2
	"RAW Write Timeout",     //3
	"RDY Polling Timeout",   //4
	"Set Retry Table Fail",  //5
	"Error Handle Timeout",  //6
	"Auto Polling Timeout",  //7
	"DQS Mismatch",          //8
	"UNDEFINE",               //9
	"Not Support Command",   //A
	"Detect DQSB fail"       //B
};
int SATASCSI3111_ST::ST_GetReadWriteStatus(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *w_buf)
{
	if (SCSI_NTCMD == m_cmdtype)
	{
		return USBSCSICmd_ST::ST_GetReadWriteStatus(obType, DeviceHandle, targetDisk, w_buf);
	}
	int rtn = 0;
	unsigned char cdb[16] = {0};
	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0xBC;
	if (-1 != GetActiveCe()) {
		//ATA Sorting enabled, BaseSorting only handle CE0 buffer, so here do a shift.

		unsigned char tempbuf[512];
		memset(tempbuf, 0, sizeof(tempbuf));
		rtn = IssueVenderFlow(obType, targetDisk, DeviceHandle, cdb, SCSI_IOCTL_DATA_IN, 512, tempbuf, 15);

		if (rtn || (0 != tempbuf[6]) || (0 != tempbuf[6+16]))
		{
			int ActiveCe = GetActiveCe();
			nt_file_log("---Status:0x0000:%02X %02X %02X %02X %02X %02X %02X",
				tempbuf[0], tempbuf[1], tempbuf[2], tempbuf[3], tempbuf[4], tempbuf[5], tempbuf[6]);
			nt_file_log("---Status:0x0010:%02X %02X %02X %02X %02X %02X %02X",
				tempbuf[16], tempbuf[17], tempbuf[18], tempbuf[19], tempbuf[20], tempbuf[21], tempbuf[22]);
			memcpy(w_buf, tempbuf, sizeof(tempbuf));

			if (  (-1!=ActiveCe &&  (8 > ActiveCe)               && ((1<<ActiveCe)     == (tempbuf[1]    & (1<<ActiveCe))))    ||
				(-1!=ActiveCe &&  (ActiveCe>7 && ActiveCe<16)  && ((1<<(ActiveCe%8)) == (tempbuf[16+1] & (1<<(ActiveCe%8)))))    )   	
			{
				nt_file_log("CRITICAL ERROR BREAK SORTING=>%s", CRITICAL_ERR[ tempbuf[6] ]);
				return SCSI_CRITICAL_FAIL;
			}
			else if (-1 == ActiveCe)
			{
				nt_file_log("CRITICAL ERROR BREAK SORTING=>%s", CRITICAL_ERR[ tempbuf[6] ]);
				return SCSI_CRITICAL_FAIL;
			}
			memcpy(w_buf, tempbuf, 512);
			return 0;
		}

		if (GetActiveCe()>7 && GetActiveCe()<16 ){
			//extend status format, 8CE will start in byte[16]
			for (int checkbyte = 0; checkbyte < 6; checkbyte++){
				w_buf[checkbyte] = (tempbuf[checkbyte+16]>>  (GetActiveCe()%8)   );
				if (0 != w_buf[6])
				{//if this is not critical error, assert it
					assert(!(w_buf[checkbyte]&0xFE));
				}
			}	
		} else if (GetActiveCe()<8){
			for (int checkbyte = 0; checkbyte < 6; checkbyte++){
				w_buf[checkbyte] = (tempbuf[checkbyte]>>  (GetActiveCe()%8)   );
				if (0 != w_buf[6])
				{//if this is not critical error, assert it
					assert(!(w_buf[checkbyte]&0xFE));
				}
			} 
		} else {
			assert(0);
			return 1;
		}

		if (GetActiveCe()) {
			nt_log("ActiveCe=%d",GetActiveCe());
			char log_mem[1024]={0};
			LogMEM(log_mem, tempbuf, 32);
			nt_log("orig_buf:\n%s", log_mem);
			memset(log_mem, 0, sizeof(log_mem));
			LogMEM(log_mem, w_buf, 32);
			nt_log("modified_buf:\n%s", log_mem);
		}

		return rtn;
	} else {
		rtn = IssueVenderFlow(obType, targetDisk, DeviceHandle, cdb, SCSI_IOCTL_DATA_IN, 512, w_buf, 15);
		//if (rtn || (0 != w_buf[6]))
		//{
		//	nt_file_log("CRITICAL ERROR BREAK SORTING");
		//	return SCSI_CRITICAL_FAIL;
		//}
	}
	return rtn;

}


// code reference from eden_huang#3002
int SATASCSI3111_ST::NT_ScanWindow_ATA(FH_Paramater * FH, int bank_number, S11_SCAN_WINDOW_OPTIONS * options) {

	UINT SDLL_CH_Write[16][4];
	UINT SDLL_CH_Read[16][4];
	UINT ScanWindowFail[16];
	UINT i,z,ch,bank,Die,mType,MaxDie;
	UINT mAddress;
	clock_t begin;
	UINT ori_control_var_read,ori_control_var_write;
	UINT ori_direct_var_read,ori_direct_var_write;
	UINT ori_direct_var_rd[16],ori_direct_var_wr[16];
	UINT HW_Org_Wr[8],HW_Org_Rd[8];
	UINT readmin,readmax,writemin,writemax,tempint5,tempint6;
	UINT CH0_Read_Min,CH0_Read_Max,CH0_Write_Min,CH0_Write_Max,CH1_Read_Min,CH1_Read_Max,CH1_Write_Min,CH1_Write_Max;
	CH0_Read_Min=CH0_Read_Max=CH0_Write_Min=CH0_Write_Max=CH1_Read_Min=CH1_Read_Max=CH1_Write_Min=CH1_Write_Max = 0;
	//bool A2_Cmd=false;
	UINT ori_control_var_read_enable,ori_control_var_write_enable;
	UINT start_ch,end_ch,start_bank,end_bank,max_bank; 
	start_ch = end_ch = start_bank = end_bank = max_bank = 0;
	max_bank = bank_number;
	UINT start_Window,end_Window,mTimes,start_Window_wr,end_Window_wr,mTimes_wr,window_critieria;
	window_critieria = 50;
	int TempInt1=0;
	begin = clock();
	start_Window=start_Window_wr=0x00;
	end_Window=end_Window_wr=0x200-1;
	mTimes=mTimes_wr=5;
	UCHAR Pad_Drive[1024];
	UCHAR CommonBuffer1[512*9]={0};
	UCHAR CommonBuffer2[512*9]={0};
	UINT DutySequence[16]={0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15};
	UINT FailTime=0;
	bool FailFlag=false,SaveFile=false;

	//Load from INI file
	memset(CommonBuffer1,0x00,512*9);
	memset(CommonBuffer2,0x00,512*9);
	memset(Pad_Drive,0,1024);
	memcpy(Pad_Drive+512,Scan_SDLL_WORST_PATTERN,482);
	memcpy(Pad_Drive+512,Scan_SDLL_WORST_PATTERN,482);
	UINT FlashSDLL[16][512];

	UCHAR GBuffer3[20 * 1024];

	UINT m_SectorsPerPage =  FH->PageSizeK * 2;// ckcktodo;

	Pad_Drive[PAD_PARAMETER_ADDR_FLASH_DRIVE]=options->FlashDrive;
	Pad_Drive[PAD_PARAMETER_ADDR_FLASH_ODT]=options->FlashODT;
	Pad_Drive[PAD_PARAMETER_ADDR_PAD_DRIVE_ENABLE]=options->PadDriveEnable;
	Pad_Drive[PAD_PARAMETER_ADDR_PAD_ODT_ENABLE]=options->PadODTEnable;
	Pad_Drive[PAD_PARAMETER_ADDR_PAD_ODT_VALUE]=options->PadODT;

	if (ATA_SetAPKey()) {
		nt_log("%s: AP KEY failed", __FUNCTION__);
		return false;
	}


	const char FLASH_MODE_NAME[4][128] = {"UNKNOW", "FEATURE_LEGACY", "FEATURE_T1", "FEATURE_T2"};
	//Flash Interface:
	//FLH_MODE_IGNORE = 7
	//FLH_MODE_ONFI_NVDDR2_WITHOUT_TOGGLE2SIGNAL = 6
	//FLH_MODE_ONFI_NVDDR2 = 5  
	//FLH_MODE_ONFI_NVDDR = 4  
	//FLH_MODE_TOGGLE2 = 3
	//FLH_MODE_TOGGLE1 = 2
	//FLH_MODE_LEGACY = 1
	if (m_flashfeature == FEATURE_LEGACY) {
		Pad_Drive[0] = 1;
	} else if (m_flashfeature == FEATURE_T1) {
		Pad_Drive[0] = 2;
	} else if (m_flashfeature == FEATURE_T2) {
		Pad_Drive[0] = 3;
	} else {
		ASSERT(0);
	}


	const char FLASH_CLK_NAME[9][128] = {"UNKNOW", "10MHZ", "40MHZ", "50MHZ", "83MHZ", "100MHZ", "166MHZ", "200MHZ", "266MHZ"};
	//Flash Clock:
	//FLH_CLK_266MHz = 8 
	//FLH_CLK_200MHz =7
	//FLH_CLK_166MHz = 6 
	//FLH_CLK_100MHz = 5 
	//FLH_CLK_83MHz = 4 
	//FLH_CLK_50MHz = 3
	//FLH_CLK_40MHz = 2
	//FLH_CLK_10MHz = 1
	if (m_flashfeature == FEATURE_LEGACY) {
		Pad_Drive[1] = 3;
	} else if (m_flashfeature == FEATURE_T1) {
		Pad_Drive[1] = 5;
	} else if (m_flashfeature == FEATURE_T2) {
		Pad_Drive[1] = 7;
	} else {
		ASSERT(0);
	}

	errno_t err;
	FILE * fp;
	//err = fopen(&fp, m_MPBINCode, "rb");
	err = fopen_s(&fp,"WIN_SETTINGs.BIN", "rb");
	if (fp)
	{
		nt_file_log("-->using WIN_SETTINGs.BIN");
		unsigned char tempBuffer[512] = {0};
		fread(tempBuffer, 1, 512, fp);
		fclose(fp);
		memcpy(&Pad_Drive[0], tempBuffer, 512);
	}
	nt_file_log("***SCAN WINDOWNS***");
	nt_file_log("Flash Interface: %s, Flash CLK:%s, Bank Number:%d", &FLASH_MODE_NAME[Pad_Drive[0]], &FLASH_CLK_NAME[Pad_Drive[1]], bank_number);

	if(!Vender_InitialPadAndDrive_3111(Pad_Drive,0)) //initial 1
	{
		nt_file_log("%s: Vender_InitialPadAndDrive_3111 failed", __FUNCTION__);

		return false;
	}

	//Do_DDR_Phase_Scan=false;
	//enable FW direct control SDLL mode
	for(mType=0;mType<2;mType++)
	{
		if(mType==0)  //read
			mAddress=0x04009800;
		else if(mType==1)
			mAddress=0x04009A00;

		if(!WriteRead_4Byte_Reg_Vendor(1,mAddress,0,4))  //read Reg.
		{
			nt_file_log(" %s:WriteRead_4Byte_Reg_Vendor", __FUNCTION__);
			return false;
		}

		if(mType==0)  //read
			ori_control_var_read=((TempBuf[7] << 24) | (TempBuf[6] << 16) | (TempBuf[5] << 8) | TempBuf[4]);
		else if(mType==1)
			ori_control_var_write=((TempBuf[7] << 24) | (TempBuf[6] << 16) | (TempBuf[5] << 8) | TempBuf[4]);

		TempBuf[4] |= 0x02;  //bit 1 = 1 -> FW direct control SDLL mode.
		if(mType==0)
			ori_control_var_read_enable=((TempBuf[7] << 24) | (TempBuf[6] << 16) | (TempBuf[5] << 8) | TempBuf[4]);
		else if(mType==1)
			ori_control_var_write_enable=((TempBuf[7] << 24) | (TempBuf[6] << 16) | (TempBuf[5] << 8) | TempBuf[4]);

	}

	for (ch=0;ch<16;ch++)
	{
		for(i=0;i<4;i++)
		{
			SDLL_CH_Write[ch][i]=0x00;
			SDLL_CH_Read[ch][i]=0x00;
		}
	}
	for(i=0;i<16;i++)
	{
		ScanWindowFail[i]=0x00;
	}

	MaxDie = FH->DieNumber;//DiesPerFlash
	end_ch = 2;
	start_bank = 0;
	end_bank = bank_number;
	for(ch=start_ch;ch<end_ch;ch++)
	{
		mAddress=0x04009820+0x4*ch;  //read 9820
		if(!WriteRead_4Byte_Reg_Vendor(1,mAddress,0,4))  //read Reg.
			return false;

		ori_direct_var_read=((TempBuf[7] << 24) | (TempBuf[6] << 16) | (TempBuf[5] << 8) | TempBuf[4]);
		ori_direct_var_rd[ch]=ori_direct_var_read;        
		//---
		mAddress=0x04009A20+0x4*ch;  //write 9A20
		if(!WriteRead_4Byte_Reg_Vendor(1,mAddress,0,4))  //read Reg.
			return false;

		ori_direct_var_write=((TempBuf[7] << 24) | (TempBuf[6] << 16) | (TempBuf[5] << 8) | TempBuf[4]);
		ori_direct_var_wr[ch]=ori_direct_var_write;   
	}

	//Scan Read Windows
	//AppendToListBox("Scan Read Windows:");
	//1.defaultßC°”C≥ttcache write
	srand( (unsigned)time( NULL ) );                //Make 512 byte random data in GBuffer
	for (z = 0; z < (512*m_SectorsPerPage) ;z++)                  
		GBuffer3[z] = rand() & 0xFF;

	window_critieria = 50;


	bool FindWindow = true;
	UINT Duty=0,start_Duty=0,end_Duty=1;



	UINT DutyOverRange=0;


	for(Duty=start_Duty;Duty<end_Duty;Duty++)
	{    
		if(DutyOverRange==3)
		{
			ScanWindowFail[DutySequence[Duty-1]]=1;
			for(z=Duty;z<end_Duty;z++)
				ScanWindowFail[DutySequence[z]]=1;
			break;
		}
		else if(DutyOverRange==1)
		{
			ScanWindowFail[DutySequence[Duty]]=1;
			Duty++;
			DutyOverRange++;
		}

		if(DutyOverRange)
		{
			if(ScanWindowFail[DutySequence[Duty-2]]==1)
				DutyOverRange++;
			continue;
		}
		else
		{    
			if(Duty!=0)
			{
				if(ScanWindowFail[DutySequence[Duty-1]]==1)
					DutyOverRange++;
			}
		}

		if(FailFlag)
		{
			if(FailTime==2)
			{
				for(z=Duty;z<end_Duty;z++)
					ScanWindowFail[DutySequence[z]]=1;
				break;
			}
			else
			{
				if (ATA_SetAPKey()) {
					nt_file_log("%s: AP KEY failed", __FUNCTION__);
					return false;
				}

				if(!Vender_InitialPadAndDrive_3111(Pad_Drive,0))//init 2
				{
					nt_file_log("Issue Vender_InitialPadAndDrive Command Failed !!!");
					return false;
				}
				FailFlag=false;
			}
		}

		//Set Duty : Use 0x04002180 Bit8~Bit11
		if(!WriteRead_4Byte_Reg_Vendor(1,0x04002180,0,4))  //Read
			return false;
		tempint5 = ((TempBuf[7] << 24) | (TempBuf[6] << 16) | (TempBuf[5] << 8) | TempBuf[4]);

		tempint6=((tempint5 & 0xFFFFF0FF) | ((DutySequence[Duty] << 8) & 0x00000F00));


		if(!WriteRead_4Byte_Reg_Vendor(0,0x04002180,tempint6,4))  //Write
			return false;

		if(!WriteRead_4Byte_Reg_Vendor(1,0x04002184,0,4))  //Read
			return false;

		tempint5 = ((TempBuf[7] << 24) | (TempBuf[6] << 16) | (TempBuf[5] << 8) | TempBuf[4]);

		tempint6=((tempint5 & 0xFFFFF0FF) | ((DutySequence[Duty] << 8) & 0x000F00));

		if(!WriteRead_4Byte_Reg_Vendor(0,0x04002184,tempint6,4))  //Write
			return false;

		for(bank=start_bank;bank<end_bank;bank++)
		{
			if(FailFlag)
				break;
			for(Die=0;Die<MaxDie;Die++)  //Die_No
			{
				if(FailFlag)
					break;
				if (ATA_SetAPKey()) {
					nt_file_log("%s: AP KEY failed", __FUNCTION__);
					return false;
				}
				if(!Vendor_Scan_SDLL(GBuffer3,0xFF,bank,Die,m_SectorsPerPage))  //Channl = 0xFF => §@@¶∏!M∞µX¢Gg®‚La≠”OChannl
				{
					nt_file_log("Scan_SDLL Command Abort!!.Duty:%d Bank:%d  Die:%d",DutySequence[Duty],bank,Die);
					ScanWindowFail[DutySequence[Duty]]=1;
					continue;
				}

				if (ATA_SetAPKey()) {
					nt_file_log("%s: AP KEY failed", __FUNCTION__);
					return false;
				}

				if(ATA_DirectReadSRAM(CommonBuffer1,9,0,0))
				{
					nt_file_log("Issue DirectReadSRAM Command Failed !!!");
					return false;
				}
				nt_file_log("Duty : %d Bank : %d  Die : %d",DutySequence[Duty],bank,Die);

				readmin = (((CommonBuffer1[1]&0xFF)<< 8) | (CommonBuffer1[0] &0xFF));
				FlashSDLL[DutySequence[Duty]][(0*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+0)] = readmin;
				//CH0_Read_Min = CH0_Read_Min+tempint1;
				readmax = (((CommonBuffer1[3]&0xFF)<< 8) | (CommonBuffer1[2] &0xFF));
				FlashSDLL[DutySequence[Duty]][(0*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+1)] = readmax;
				//CH0_Read_Max =CH0_Read_Max+ tempint2;
				writemin = (((CommonBuffer1[5]&0xFF)<< 8) | (CommonBuffer1[4] &0xFF));
				FlashSDLL[DutySequence[Duty]][(0*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)] = writemin;
				//CH0_Write_Min =CH0_Write_Min+ tempint3;
				writemax = (((CommonBuffer1[7]&0xFF)<< 8) | (CommonBuffer1[6] &0xFF));
				FlashSDLL[DutySequence[Duty]][(0*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)] = writemax;
				//CH0_Write_Max =CH0_Write_Max+ tempint4;

				nt_file_log("CH 0 Read-Min : %d Read-Max : %d Write-Min : %d Write-Max : %d",readmin,readmax,writemin,writemax);


				readmin = (((CommonBuffer1[9]&0xFF)<< 8) | (CommonBuffer1[8] &0xFF));
				FlashSDLL[DutySequence[Duty]][(1*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+0)] = readmin;
				//CH1_Read_Min =CH1_Read_Min+ tempint1;
				readmax = (((CommonBuffer1[11]&0xFF)<< 8) | (CommonBuffer1[10] &0xFF));
				FlashSDLL[DutySequence[Duty]][(1*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+1)] = readmax;
				//CH1_Read_Max =CH1_Read_Max+ tempint2;
				writemin = (((CommonBuffer1[13]&0xFF)<< 8) | (CommonBuffer1[12] &0xFF));
				FlashSDLL[DutySequence[Duty]][(1*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)] = writemin;
				//CH1_Write_Min =CH1_Write_Min+ tempint3; 
				writemax = (((CommonBuffer1[15]&0xFF)<< 8) | (CommonBuffer1[14] &0xFF));
				FlashSDLL[DutySequence[Duty]][(1*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)] = writemax;
				//CH1_Write_Max =CH1_Write_Max+ tempint4; 

				HW_Org_Rd[0]=((CommonBuffer1[17]<<8) | CommonBuffer1[16]);
				HW_Org_Wr[0]=((CommonBuffer1[19]<<8) | CommonBuffer1[18]);
				HW_Org_Rd[1]=((CommonBuffer1[21]<<8) | CommonBuffer1[20]);
				HW_Org_Wr[1]=((CommonBuffer1[23]<<8) | CommonBuffer1[22]);
				nt_file_log("CH 1 Read-Min : %d Read-Max : %d Write-Min : %d Write-Max : %d",readmin,readmax,writemin,writemax);
				int FailCE[2][8]={0};
				int ScanWindowFail[16]={0};
				for(z=0;z<16;z=z+2)
				{
					//nt_log("CommonBuffer[%d] : %2X  CommonBuffer[%d] : %2X",z,CommonBuffer[z],z+1,CommonBuffer[z+1]);
					//ProgressRecord(message,true);
					mType=((CommonBuffer1[z+1]<<8) | CommonBuffer1[z]);
					if((mType > 512) && mType<0xFFFF )
					{

						if(z==0)
						{
							FailCE[0][bank]=1;
							//FindWindow=false;
							ScanWindowFail[DutySequence[Duty]]=1;
							nt_file_log("Issue Can Not Find CH0-Read-Min Bank : %d Die : %d  Data : 0x%2X%2X!!!",bank,Die,CommonBuffer1[z+1],CommonBuffer1[z]);
						}
						else if(z==2)
						{
							FailCE[0][bank]=1;
							//FindWindow=false;
							ScanWindowFail[DutySequence[Duty]]=1;
							nt_file_log("Issue Can Not Find CH0-Read-Max Bank : %d Die : %d Data : 0x%2X%2X!!!",bank,Die,CommonBuffer1[z+1],CommonBuffer1[z]);
						}
						else if(z==4)
						{
							FailCE[0][bank]=1;
							//FindWindow=false;
							ScanWindowFail[DutySequence[Duty]]=1;
							nt_file_log("Issue Can Not Find CH0-Write-Min Bank : %d Die : %d Data : 0x%2X%2X!!!",bank,Die,CommonBuffer1[z+1],CommonBuffer1[z]);
						}
						else if(z==6)
						{
							FailCE[0][bank]=1;
							//FindWindow=false;
							ScanWindowFail[DutySequence[Duty]]=1;
							nt_file_log("Issue Can Not Find CH0-Write-Max Bank : %d Die : %d Data : 0x%2X%2X!!!",bank,Die,CommonBuffer1[z+1],CommonBuffer1[z]);
						}
						else if(z==8)
						{
							FailCE[1][bank]=1;
							//FindWindow=false;
							ScanWindowFail[DutySequence[Duty]]=1;
							nt_file_log("Issue Can Not Find CH1-Read-Min Bank : %d Die : %d Data : 0x%2X%2X!!!",bank,Die,CommonBuffer1[z+1],CommonBuffer1[z]);
						}
						else if(z==10)
						{
							FailCE[1][bank]=1;
							//FindWindow=false;
							ScanWindowFail[DutySequence[Duty]]=1;
							nt_file_log("Issue Can Not Find CH1-Read-Max Bank : %d Die : %d Data : 0x%2X%2X!!!",bank,Die,CommonBuffer1[z+1],CommonBuffer1[z]);
						}
						else if(z==12)
						{
							FailCE[1][bank]=1;
							//FindWindow=false;
							ScanWindowFail[DutySequence[Duty]]=1;
							nt_file_log("Issue Can Not Find CH1-Write-Min : %d Die : %d Data : 0x%2X%2X!!!",bank,Die,CommonBuffer1[z+1],CommonBuffer1[z]);
						}
						else if(z==14)
						{
							//FailCE[1][bank]=1;
							FindWindow=false;
							ScanWindowFail[DutySequence[Duty]]=1;
							nt_file_log("Issue Can Not Find CH1-Write-Max Bank : %d Die : %d Data : 0x%2X%2X!!!",bank,Die,CommonBuffer1[z+1],CommonBuffer1[z]);
						}

						if(mType == 0xB000)
						{
							UINT CHType=1;
							if(z >= 8)
								CHType=0;


							nt_file_log("Check Fail CE Duty:%d Bank:%d Die:%d CH:%d",DutySequence[Duty],bank,Die,CHType);
							return false;

						}
					}// if((mType > 512) && mType<0xFFFF )
				}//for(z=0;z<16;z=z+2)

			}  //Die
		}//bank
	} //Duty


	for(i=0;i<end_Duty;i++)
	{
		if(ScanWindowFail[i]==1)
		{
			nt_file_log("Fail Duty : %d",i);
		}
	}

	FindWindow=false;
	for(i=0;i<end_Duty;i++)
	{
		if(ScanWindowFail[i]!=1)
			FindWindow=true;
	}



	UINT tempint7=0;
	int channel_number = 2;
	mType=writemax=tempint6=tempint5=0;
	if(channel_number==1)
		end_ch++;
	//CH0 Read
	nt_file_log("CH0 Read : ");

	for(Duty=start_Duty;Duty<end_Duty;Duty++)
	{    
		for(ch=start_ch;ch<end_ch-1;ch++)
		{
			readmin = 0;
			readmax = 511 ;
			for(bank=start_bank;bank<end_bank;bank++)
			{
				for(Die=0;Die<MaxDie;Die++)  //Die_No
				{
					if(ScanWindowFail[DutySequence[Duty]] != 1)
					{
						if(FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+0)] >= readmin)   //Read ≥ÃI§pp®˙Lu≥ÃI§jj

							readmin = FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+0)];
						if(FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+1)] <= readmax) //Read ≥ÃI§jj®˙Lu≥ÃI§pp
							readmax = FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+1)];
					}
				}
			}
			if(ScanWindowFail[DutySequence[Duty]] != 1)
			{
				nt_file_log("  Duty_%d: CH_%d: Read_Min : %d Read_Max : %d (Range : %d)",DutySequence[Duty],ch,readmin,readmax,(readmax-readmin)+1);

				if(ch==start_ch)
				{
					if((readmax-readmin)>writemax)
					{
						writemax=readmax-readmin;
						tempint5=DutySequence[Duty];
					}
				}
				else if(ch==end_ch-1)
				{
					if((readmax-readmin)>tempint7)
					{
						tempint7=readmax-readmin;
						tempint6=DutySequence[Duty];
					}
				}

				writemin=(readmin+readmax)/2;
				SDLL_CH_Read[DutySequence[Duty]][ch]=writemin;
			}
		}
	}
	//CH1 Read
	nt_file_log("CH1 Read : ");

	for(Duty=start_Duty;Duty<end_Duty;Duty++)
	{    
		if(channel_number==1)
			break;
		for(ch=start_ch+1;ch<end_ch;ch++)
		{
			readmin = 0;
			readmax = 511 ;
			for(bank=start_bank;bank<end_bank;bank++)
			{
				for(Die=0;Die<MaxDie;Die++)  //Die_No
				{

					if(ScanWindowFail[DutySequence[Duty]] != 1)
					{
						if(FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+0)] >= readmin)   //Read ≥ÃI§pp®˙Lu≥ÃI§jj
							readmin = FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+0)];

						if(FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+1)] <= readmax) //Read ≥ÃI§jj®˙Lu≥ÃI§pp
							readmax = FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+1)];
					}
				}
			}
			if(ScanWindowFail[DutySequence[Duty]] != 1)
			{
				nt_file_log("  Duty_%d: CH_%d: Read_Min : %d Read_Max : %d (Range : %d)",DutySequence[Duty],ch,readmin,readmax,(readmax-readmin)+1);

				if(ch==start_ch)
				{
					if((readmax-readmin)>writemax)
					{
						writemax=readmax-readmin;
						tempint5=DutySequence[Duty];
					}
				}
				else if(ch==end_ch-1)
				{
					if((readmax-readmin)>tempint7)
					{
						tempint7=readmax-readmin;
						tempint6=DutySequence[Duty];
					}
				}

				writemin=(readmin+readmax)/2;
				SDLL_CH_Read[DutySequence[Duty]][ch]=writemin;
			}
		}
	}

	//CH0 Write
	nt_file_log("CH0 Write : ");



	mType=writemax=0;
	for(Duty=start_Duty;Duty<end_Duty;Duty++)
	{    
		for(ch=start_ch;ch<end_ch-1;ch++)
		{
			readmin = 0;
			readmax = 511 ;

			for(bank=start_bank;bank<end_bank;bank++)
			{
				for(Die=0;Die<MaxDie;Die++)  //Die_No
				{
					if(channel_number==1)
						ch=0;
					if(ScanWindowFail[DutySequence[Duty]] != 1)
					{
						nt_file_log( " %d  %d  %d",DutySequence[Duty], (ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2),  FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)]);

						if(FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)] >= readmin)   //Write ≥ÃI§pp®˙Lu≥ÃI§jj
							readmin = FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)];
						nt_file_log( " %d  %d  %d",DutySequence[Duty], (ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3),  FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)]);
						if(FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)] <= readmax) //Write ≥ÃI§jj®˙Lu≥ÃI§pp
							readmax = FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)];
					}
				}
			}
			if(ScanWindowFail[DutySequence[Duty]] != 1)
			{
				nt_file_log("  Duty_%d: CH_%d: Write_Min : %d Write_Max : %d (Range : %d)",DutySequence[Duty],ch,readmin,readmax,(readmax-readmin)+1);


				writemin=(readmin+readmax)/2;
				SDLL_CH_Write[DutySequence[Duty]][ch]=writemin;
			}
		}
	}

	//CH1 Write
	nt_file_log("CH1 Write : ");
	for(Duty=start_Duty;Duty<end_Duty;Duty++)
	{    
		if(channel_number==1)
			break;
		for(ch=start_ch+1;ch<end_ch;ch++)
		{
			readmin = 0;
			readmax = 511 ;

			for(bank=start_bank;bank<end_bank;bank++)
			{
				for(Die=0;Die<MaxDie;Die++)  //Die_No
				{
					if(ScanWindowFail[DutySequence[Duty]] != 1)
					{
						if(FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)] >= readmin)   //Write ≥ÃI§pp®˙Lu≥ÃI§jj
							readmin = FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)];

						if(FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)] <= readmax) //Write ≥ÃI§jj®˙Lu≥ÃI§pp
							readmax = FlashSDLL[DutySequence[Duty]][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)];
					}
					/*
					tempint1=FlashSDLL[(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)];
					tempint2=FlashSDLL[(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)];
					tempint3=(tempint1+tempint2)/2;
					tempint4+=tempint3;
					mType+=1;
					*/
				}
			}
			if(ScanWindowFail[DutySequence[Duty]] != 1)
			{
				nt_file_log("  Duty_%d: CH_%d: Write_Min : %d Write_Max : %d (Range : %d)",DutySequence[Duty],ch,readmin,readmax,(readmax-readmin)+1);

				writemin=(readmin+readmax)/2;
				SDLL_CH_Write[DutySequence[Duty]][ch]=writemin;
			}
		}
	}


	nt_file_log("Find Read Max Duty :Ch_%d Duty_%d Ch_%d Duty_%d",start_ch,tempint5,end_ch-1,tempint6);


	int Ch0Duty;
	int Ch1Duty;
	int ScanValueRd[2] = {0};
	int ScanValueWr[2] = {0};

	Ch0Duty=start_Duty=tempint5;
	Ch1Duty=end_Duty=tempint6;

	if(channel_number==1)
		end_ch--;
	nt_file_log("[Windows]:");


	//show found window    
	for(Duty=start_Duty;Duty<start_Duty+1;Duty++)
	{    
		for(ch=start_ch;ch<start_ch+1;ch++)
		{
			nt_file_log("Duty_%d CH_%d: Read Window=0x%x, Write Window=0x%x",Duty,ch,SDLL_CH_Read[Duty][ch],SDLL_CH_Write[Duty][ch]);

			nt_file_log("Duty_%d CH_%d: HW_Rd=0x%x, HW_Wr=0x%x",Duty,ch,(HW_Org_Rd[ch]),HW_Org_Wr[ch]);

			TempInt1 = SDLL_CH_Read[Duty][ch] - (HW_Org_Rd[ch]);
			if(TempInt1 <= 0)
			{
				TempInt1 = abs(TempInt1);
				TempInt1 = TempInt1 | 0x8000;
				ScanValueRd[ch] = TempInt1;
			}
			else
				ScanValueRd[ch] = TempInt1;

			TempInt1 = SDLL_CH_Write[Duty][ch] - (HW_Org_Wr[ch]);
			if(TempInt1 <= 0)
			{
				TempInt1 = abs(TempInt1);
				TempInt1 = TempInt1 | 0x8000;
				ScanValueWr[ch] = TempInt1;
			}
			else
				ScanValueWr[ch] = TempInt1;

			nt_file_log("ScanValueRe[%d] = 0x%X  ScanValueWr[%d] = 0x%X",ch,ScanValueRd[ch],ch,ScanValueWr[ch]);    

			//show all ch avg value.
			//for(bank=0;bank<mBank;bank++)
			for(bank=start_bank;bank<end_bank;bank++)
			{
				for(Die=0;Die<MaxDie;Die++)  //Die_No
				{
					readmin=FlashSDLL[Duty][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+0)];
					readmax=FlashSDLL[Duty][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+1)];
					writemin=FlashSDLL[Duty][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)];
					writemax=FlashSDLL[Duty][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)];

					nt_file_log("     Duty_%d: Bank_%d: Die_%d: Read Win=0x%x(Min=0x%x, Max=0x%x)",Duty,bank,Die,(readmin+readmax)/2,readmin,readmax);
					nt_file_log("                      Write_Win=0x%x(Min=0x%x, Max=0x%x)",(writemin+writemax)/2,writemin,writemax);




					if(readmin > readmax)
					{
						nt_file_log("     Duty_%d CH_%d Bank_%d Die_%d : Read_Min : %d  > Read_Max : %d !!",Duty,ch,bank,Die,readmin,readmax);    
						return false;
					}

					if(writemin > writemax)
					{
						nt_file_log("     Duty_%dCH_%d Bank_%d Die_%d : Write_Min : %d  > Write_Max : %d !!",Duty,ch,bank,Die,writemin,writemax);   
						return false;
					}

					if (((readmax-readmin)) < window_critieria) //FAE©ww50
					{
						nt_file_log("     Duty_%d CH_%d Bank_%d Die_%d : Read Win = %d  < %d !!",Duty,ch,bank,Die,(readmax-readmin),window_critieria);  
						return false;
					}
					if (((writemax-writemin)) < window_critieria)
					{
						nt_file_log("     Duty_%d CH_%d Bank_%d Die_%d : Write Win = %d  < %d !!",Duty,ch,bank,Die,(writemax-writemin),window_critieria);  
						return false;
					}
				}
			}
		}
	}

	for(Duty=end_Duty;Duty<end_Duty+1;Duty++)
	{    
		for(ch=end_ch-1;ch<end_ch;ch++)
		{
			nt_file_log("Duty_%d CH_%d: Read Window=0x%x, Write Window=0x%x",Duty,ch,SDLL_CH_Read[Duty][ch],SDLL_CH_Write[Duty][ch]);
			nt_file_log("Duty_%d CH_%d: HW_Rd=0x%x, HW_Wr=0x%x",Duty,ch,(HW_Org_Rd[ch]),HW_Org_Wr[ch]);

			TempInt1 = SDLL_CH_Read[Duty][ch] - (HW_Org_Rd[ch]);
			if(TempInt1 <= 0)
			{
				TempInt1 = abs(TempInt1);
				TempInt1 = TempInt1 | 0x8000;
				ScanValueRd[ch] = TempInt1;
			}
			else
				ScanValueRd[ch] = TempInt1;

			TempInt1 = SDLL_CH_Write[Duty][ch] - (HW_Org_Wr[ch]);
			if(TempInt1 <= 0)
			{
				TempInt1 = abs(TempInt1);
				TempInt1 = TempInt1 | 0x8000;
				ScanValueWr[ch] = TempInt1;
			}
			else
				ScanValueWr[ch] = TempInt1;
			nt_file_log("ScanValueRe[%d] = 0x%X  ScanValueWr[%d] = 0x%X",ch,ScanValueRd[ch],ch,ScanValueWr[ch]);    

			//show all ch avg value.
			//for(bank=0;bank<mBank;bank++)
			for(bank=start_bank;bank<end_bank;bank++)
			{
				for(Die=0;Die<MaxDie;Die++)  //Die_No
				{
					readmin=FlashSDLL[Duty][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+0)];
					readmax=FlashSDLL[Duty][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+1)];
					writemin=FlashSDLL[Duty][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+2)];
					writemax=FlashSDLL[Duty][(ch*max_bank*MaxDie*4)+(bank*MaxDie*4)+(Die*4+3)];

					nt_file_log("     Duty_%d: Bank_%d: Die_%d: Read Win=0x%x(Min=0x%x, Max=0x%x)",Duty,bank,Die,(readmin+readmax)/2,readmin,readmax);
					nt_file_log("                      Write_Win=0x%x(Min=0x%x, Max=0x%x)",(writemin+writemax)/2,writemin,writemax);

					if(readmin > readmax)
					{
						nt_file_log("     Duty_%d CH_%d Bank_%d Die_%d : Read_Min : %d  > Read_Max : %d !!",Duty,ch,bank,Die,readmin,readmax);    
						return false;
					}

					if(writemin > writemax)
					{
						nt_file_log("     Duty_%dCH_%d Bank_%d Die_%d : Write_Min : %d  > Write_Max : %d !!",Duty,ch,bank,Die,writemin,writemax);   
						return false;
					}

					if (((readmax-readmin)) < window_critieria) //FAE©ww50
					{
						nt_file_log("     Duty_%d CH_%d Bank_%d Die_%d : Read Win = %d  < %d !!",Duty,ch,bank,Die,(readmax-readmin),window_critieria);  
						return false;
					}
					if (((writemax-writemin)) < window_critieria)
					{
						nt_file_log("     Duty_%d CH_%d Bank_%d Die_%d : Write Win = %d  < %d !!",Duty,ch,bank,Die,(writemax-writemin),window_critieria);  
						return false;
					}
				}
			}
		}
	}
	return true;
}

//return 0 fail, 1 success
bool SATASCSI3111_ST::Vendor_Scan_SDLL(UCHAR* buf,UINT Channel_number,UINT Bank_number,UINT Die_Number,UCHAR SectorCount)
{
	UCHAR CDB[16];
	memset(CDB, 0, 16);
	CDB[0] = 0xAB;      //Feature
	CDB[1] = SectorCount;      //SC
	CDB[2] = Die_Number;         //SN
	CDB[3] = Channel_number;         //CL
	CDB[4] = Bank_number;         //CH
	CDB[5] = 0xA0;      //DH
	CDB[6] = 0x31;      //CMD

	if (MyIssueATACmd(_PreATARegister, CDB, _CurOutRegister, 2, buf, 512* SectorCount, false, true, 30, false)){
		nt_log("Vendor_Write4ByteReg_ATA fail\n");
		return 0;
	} else {
		nt_log("Vendor_Write4ByteReg_ATA success\n");
		return 1;
	}
	return 0;

}

//1 for success, 0 for fail
bool SATASCSI3111_ST::WriteRead_4Byte_Reg_Vendor(UINT mode,UINT Address,UINT Value,UINT ByteNo)
{
	//UINT i;
	if(mode==0)//write
	{
		memset(TempBuf,0,512);
		//Address
		TempBuf[0]=(Address & 0xFF);
		TempBuf[1]=((Address>>8) & 0xFF);
		TempBuf[2]=((Address>>16) & 0xFF);
		TempBuf[3]=((Address>>24) & 0xFF);
		//Value
		TempBuf[4]=(Value & 0xFF);
		TempBuf[5]=((Value>>8) & 0xFF);
		TempBuf[6]=((Value>>16) & 0xFF);
		TempBuf[7]=((Value>>24) & 0xFF);

		if (ATA_SetAPKey()) {
			nt_log("%s: AP KEY failed1", __FUNCTION__);
			return false;
		}

		if (ATA_Vendor_Write4ByteReg(TempBuf, ByteNo))
			//if ( Vendor_Write4ByteReg_3105(TempBuf,ByteNo))
		{
			nt_log("Failed! Write4ByteReg command Failed !!");
			return false;
		}
	}
	else if(mode==1)//read
	{
		memset(CommonBuffer,0,512);
		CommonBuffer[0]=(Address & 0xFF);
		CommonBuffer[1]=((Address>>8) & 0xFF);
		CommonBuffer[2]=((Address>>16) & 0xFF);
		CommonBuffer[3]=((Address>>24) & 0xFF);

		if (ATA_SetAPKey()) {
			nt_log("%s: AP KEY failed2", __FUNCTION__);
			return false;
		}

		//0xFC001000
		//if(!VCmd->DirectWriteSRAM_3016S3(CommonBuffer,0x01,0x00,0x33))  //0x33
		if (ATA_DirectWriteSRAM(CommonBuffer,0x01,0x00,0x33))
		{
			//Err_str = "I_x33";
			nt_log("Failed! DirectWriteSRAM command Failed !!");
			//AppendToListBox(Msg_str);;
			return false;
		}
		if (ATA_SetAPKey()) {
			nt_log("%s: AP KEY failed3", __FUNCTION__);
			return false;
		}

		memset(TempBuf,0,512);
		//if(!VCmd->Vendor_Read4ByteReg_3105(TempBuf,ByteNo))
		if (ATA_Vendor_Read4ByteReg(TempBuf, ByteNo))
		{
			//Err_str = "I_x33";
			nt_log("Failed! Read4ByteReg command Failed !!");
			//AppendToListBox(Msg_str);

			return false;
		}
	}
	return true;
}
//return 0 fail, 1 success
bool SATASCSI3111_ST::Vender_InitialPadAndDrive_3111(UCHAR* buf,UINT sMode)
{
	unsigned char ata_register[7] = {0};
	unsigned char out_register[7] = {0};
	ata_register[0] = 0xAA;      //Feature
	ata_register[1] = 0x02;      //SC
	ata_register[2] = 0x00;         //SN
	ata_register[3] = 0x00;         //CL
	ata_register[4] = sMode;         //CH
	ata_register[5] = 0xA0;      //DH
	ata_register[6] = 0x31;      //CMD



	if (MyIssueATACmd(_PreATARegister, ata_register, out_register, 2, buf, 1024, false, false, 30, false)){
		nt_log("Vendor_Write4ByteReg_ATA fail\n");
		return 0;
	} else {
		nt_log("Vendor_Write4ByteReg_ATA success\n");
		return 1;
	}
	return 0;
}


int SATASCSI3111_ST::ST_GetCodeVersionDate(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char* buf)
{
	if (SCSI_NTCMD == m_cmdtype) {
		return USBSCSICmd_ST::ST_GetCodeVersionDate(obType, DeviceHandle, targetDisk, buf);
	}
	nt_log("%s",__FUNCTION__);
	int iResult = 0, rtn = 0;
	unsigned char cdb[16];
	UCHAR buf2[32];
	memset(buf2, 0 ,32);
	memset(cdb,0,16);
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0x00;
	rtn = NT_CustomCmd_Phy(DeviceHandle, cdb, 16, SCSI_IOCTL_DATA_IN, 32, buf2, 15);
	memcpy(buf, buf2, 9);//date
	memcpy(buf+9, buf2+16, 16);//version
	return rtn;
}
int SATASCSI3111_ST::NT_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle, char targetDisk , unsigned char *buf, bool bGetUID,BYTE full_ce_info,BYTE get_ori_id)
{
	_hUsb = MyDeviceHandle;
	
	if (SCSI_NTCMD == m_cmdtype) {
		return USBSCSICmd_ST::NT_GetFlashExtendID(bType, MyDeviceHandle, targetDisk, buf, bGetUID, full_ce_info, get_ori_id);
	}

	if (ATA_SetAPKey())
		return 1;

	errno_t err;

	FILE * fp;
	err= fopen_s(&fp,"FAKE_FLASH_ID.bin", "rb");

	bool isGetID_FromBootCodeONLY = GetPrivateProfileInt("Sorting","S11GetFlashIDFromBootONLY", 0, _ini_file_name);
	Utilities u;
	bool isFromCache = ((0!= u.Count1(_flash_id_boot_cache,512)) && isGetID_FromBootCodeONLY);

	if (!err)
	{
		fread(buf, 1, 512, fp);
		fclose(fp);
		return 0;
	}
	int rtn = 1;
	unsigned char ata_register[7] = {0};
	unsigned char out_register[7] = {0};
	unsigned char buf2[512] = {0};
	ata_register[0] = 0x15;      //Feature
	ata_register[1] = 0x01;      //SC
	ata_register[2] = 0x00;         //SN
	ata_register[3] = 0x00;         //CL
	ata_register[4] = full_ce_info;         //CH
	ata_register[5] = 0xA0;      //DH
	ata_register[6] = 0x21;      //CMD

	if (isFromCache)
	{
		memcpy(buf, _flash_id_boot_cache, 512);
		nt_file_log("S11 get ID from cache\n");
		rtn=0;
	}
	else
	{
		if (MyIssueATACmd(_PreATARegister, ata_register, out_register, 1, buf, 512, true, false, 30, false)){
			nt_file_log("Vendor_Write4ByteReg_ATA fail\n");
			rtn=1;
		} else {
			nt_file_log("Vendor_Write4ByteReg_ATA success\n");
			rtn=0;
		}
		if (isGetID_FromBootCodeONLY)
		{
			nt_file_log("cache ID from boot code\n");
			memcpy(_flash_id_boot_cache, buf, 512);
			LogHex(_flash_id_boot_cache, 512);
			rtn=0;
		}
	}



	int exist_ce_addr[32]={0};//keep start addrs it have id number
	int ce_addr_index =0;
	for (int ce = 0, ce_addr_index=0;ce<32; ce++){
		memcpy(buf2+16*ce,buf+8*ce,8);
		if (buf2[16*ce] && buf2[16*ce+1] && (0xFF != buf2[16*ce])){
			exist_ce_addr[ce_addr_index]=16*ce;
			ce_addr_index++;
		}
	}

	memcpy(buf,buf2,512);

	if (-1 != _ActiveCe){
		memset(buf,0,512);
		memcpy(buf,&buf2[exist_ce_addr[_ActiveCe]],16);
	}
	if (0 == buf[0] && 0 == buf[1] && 0 ==buf[2]) {
		memcpy(&buf[0], &buf[16], 16);
		memset(&buf[16], 0, 16);
	}
	return rtn;
}

int SATASCSI3111_ST::GetLogicalToPhysicalCE_Mapping(unsigned char * flash_id_buf, int * phsical_ce_array)
{
	int exist_phy_ce_list[32] = {0};
	int out_map[32] = {0};
	int exist_cnt = 0;
	for (int phy_ce_idx=0; phy_ce_idx<32 ;phy_ce_idx++)
	{
		if (flash_id_buf[phy_ce_idx*16])
		{
			exist_phy_ce_list[exist_cnt]=phy_ce_idx;
			exist_cnt++;
		}
	}

	int ch1_start_idx = exist_cnt/2;

	int out_map_cnt = 0;
	for (int ch_jump_idx=0; ch_jump_idx< (exist_cnt/2); ch_jump_idx++)
	{
		out_map[out_map_cnt++] = exist_phy_ce_list[ch_jump_idx];
		out_map[out_map_cnt++] = exist_phy_ce_list[ch1_start_idx + ch_jump_idx];
	}

	memcpy(phsical_ce_array, out_map, sizeof(out_map));
	return 0;
}


/*
0x14	SCAN_EARLY_BAD	
CDB[3]: CE
CDB[4]: start block (low byte)
CDB[5]: start block (low byte)
CDB[6]: block range (high byte)
CDB[7]: block range (high byte)
CDB[15]: Skip CE number (low byte)
CDB[16]: Skip CE number (high byte)
*/
int SATASCSI3111_ST::ST_ScanBadBlock(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,int CE, unsigned char *w_buf,BOOL isCheckSpare00, unsigned int DieOffSet, unsigned int BlockPerDie)
{
	if (SCSI_NTCMD == m_cmdtype)
	{
		return USBSCSICmd_ST::ST_ScanBadBlock(obType,DeviceHandle,targetDisk,CE, w_buf,isCheckSpare00, DieOffSet, BlockPerDie);
	}
	if (ATA_SetAPKey())
		return 1;
	unsigned char cdb512[512] = {0};
	cdb512[0] = 0x06;
	cdb512[1] =	0xF6;
	cdb512[2] =	0x14;
	cdb512[3] =	GetCeMapping(CE);
	cdb512[4] =	DieOffSet & 0xFF;
	cdb512[5] =	(DieOffSet>>8) & 0xFF;
	cdb512[6] =	BlockPerDie & 0xFF;
	cdb512[7] =	(BlockPerDie>>8) & 0xFF;
	return IssueVenderFlow(1, NULL, DeviceHandle, cdb512, 1, 4096, w_buf, 60);

}

/*
0xD0	SET_LDPC_ITERATION	
CDB[3]: FBH maximum iteration
CDB[4]: NHB maximum iteration
CDB[5]: GDBF maximum iteration
CDB[6]: MSA maximum iteration
*/
int SATASCSI3111_ST::ST_SetLDPCIterationCount(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk ,int count)
{
	if (SCSI_NTCMD == m_cmdtype)
	{
		return USBSCSICmd_ST::ST_SetLDPCIterationCount(obType, DeviceHandle, targetDisk, count);
	}
	if (ATA_SetAPKey())
		return 1;
	unsigned char cdb512[512] = {0};
	cdb512[0] = 0x06;
	cdb512[1] =	0xF6;
	cdb512[2] =	0xD0;
	cdb512[3] =	count;
	return IssueVenderFlow(1, NULL, DeviceHandle, cdb512, 2, 0, NULL, 15);
}

int SATASCSI3111_ST::SetLegacyParameter_and_Command(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, bool bCallResetWhenFail, FH_Paramater * FH, BYTE * ap_parameter)
{
	unsigned char buf2[8256] = {0};
	ap_parameter[0x14] = FH->LegacyEDO_14;
	ap_parameter[0x4D] = FH->LegacyDriving_4D;
	ap_parameter[0x48] = FH->LegacyReadClock_48;
	ap_parameter[0x49] = FH->LegacyWriteClock_49;
	//_apParameter[0x14] &= ~0x02 ;//edo off
	ap_parameter[0x00] &= ~0x20 ;//close onfi
	SATASCSI3111_ST::ST_SetGet_Parameter(obType, DeviceHandle, targetDisk, true, ap_parameter);


	if(!SATASCSI3111_ST::ST_SetFeature(obType, DeviceHandle, targetDisk,0,buf2,0,0,0))
	{
		//Check ¶^∂«™∫buf
		for (int ce = 0; ce < FH->FlhNumber; ce++)
		{
			if (((buf2[ce * 16] != 1) && ((FH->bMaker == 0x98) || (FH->bMaker == 0x45))) || ((buf2[ce * 16] != 0) && ((FH->bMaker == 0xad) || (FH->bMaker == 0x2c) || (FH->bMaker == 0x89))))
			{
				//if ¶^legacy fail
				nt_log("Set legacy FAIL.");
				if(bCallResetWhenFail)
				{
					if(FH->bMaker==0x45 || /*_FH.bMaker==0x2C ||*/ FH->bMaker==0x89 || FH->bMaker==0xB5){
						if(SATASCSI3111_ST::ST_CmdFDReset(obType, DeviceHandle, targetDisk))
						{
							nt_log("FD Reset SCSII FAIL.");
							return 1;
						}
						else
						{
							nt_log("FD Reset OK.");
							//resetßπ¶Aset§@¶∏legacy
							if(SATASCSI3111_ST::SetLegacyParameter_and_Command(obType, DeviceHandle, targetDisk, false,FH,ap_parameter))
								return 1;
							return 0;
						}
					}
					else
					{
						nt_log("FD Reset Not Supported Flash.");
						return 1;
					}
				}
				else
				{
					//set legacy fail and not try fd reset
					return 1;
				}
			}
		}
	}
	else
	{
		nt_log("Set Feature SCSII FAIL.");
		return 1;
	}
	nt_log("Set legacy OK.");
	return 0;

}

int SATASCSI3111_ST::ST_SetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, BYTE bMode, unsigned char *w_buf, BYTE bONFIType, BYTE bONFIMode, BOOL bToggle20)
{
	if (SCSI_NTCMD == m_cmdtype)
		return USBSCSICmd_ST::ST_SetFeature( obType, DeviceHandle, targetDisk,  bMode, w_buf,  bONFIType,  bONFIMode,  bToggle20);
	if (0 == bMode) {//set legacy. for s11 firmware if wanna switch to legacy it need set flash clock to 40Mhz first
		m_apParameter[0x4D] = 2;
		m_flashfeature = FEATURE_LEGACY;
		SATASCSI3111_ST::ST_SetGet_Parameter(obType, DeviceHandle, targetDisk, true, m_apParameter);
		return USBSCSICmd_ST::ST_SetFeature( obType, DeviceHandle, targetDisk,  bMode, w_buf,  bONFIType,  bONFIMode,  bToggle20);
	}
	else
	{
		return USBSCSICmd_ST::ST_SetFeature( obType, DeviceHandle, targetDisk,  bMode, w_buf,  bONFIType,  bONFIMode,  bToggle20);
	}
	return 0;
	
}
int SATASCSI3111_ST::SwitchToggleLegacy(BYTE obType, HANDLE DeviceHandle, char bMyDrv, FLASH_FEATURE TYPE,BYTE bONFIType,BYTE bONFIMode,BYTE bToggle20, FH_Paramater * pPH)
{
	FH_Paramater _FH;
	memcpy(&_FH,pPH,sizeof(FH_Paramater));



	//if(_FH.bToggle == 0x54) assert(0);
	if(TYPE == FEATURE_T1)
	{

		unsigned char buf2[8256] = {0};
		BYTE count = 0;
		bool pass = true;
		do
		{
			if (SATASCSI3111_ST::ST_SetFeature(obType, DeviceHandle, bMyDrv, FEATURE_T1, buf2, bONFIType, bONFIMode, bToggle20))		//set toggle to write ib
			{
				if(bToggle20)
					nt_log("Set Toggle 2.0 SCSI FAIL.");
				else
					nt_log("Set Toggle 1.0 SCSI FAIL.");
				return 1;
			}
			for (int ce = 0; ce < _FH.FlhNumber; ce++)
			{
				nt_file_log("setfeature_buffer ce%d 0x%x", ce, buf2[ce*16]);
				if (((_FH.bMaker == 0x98) || (_FH.bMaker == 0x45) || (_FH.bMaker==0xEC) || (_FH.bMaker==0xad)) && bToggle20)
				{
					if (buf2[ce * 16] != 0x07)
					{
						if (count != (_FH.FlhNumber * 10))
						{
							count ++;
							Sleep(300);
							pass = false;
							break;
						}
						else
						{

							return 1;
						}
					}
				}
				else if ((_FH.bMaker == 0x98) || (_FH.bMaker == 0x45) || (_FH.bMaker == 0xad) || (_FH.bMaker==0xEC))
				{
					if (((buf2[ce * 16] != 0) && ((_FH.bMaker == 0x98) || (_FH.bMaker == 0x45)|| (_FH.bMaker==0xEC))) || ((buf2[ce * 16] != 0x20) && (_FH.bMaker == 0xad)))
					{
						if (count != (_FH.FlhNumber * 10))
						{
							count ++;
							Sleep(300);
							pass = false;
							break;
						}
						else
						{
							return 1;
						}
					}
				}
				else if (((_FH.bMaker==0x2c) || (_FH.bMaker==0x89)))
				{
					if( _FH.bSetMicronDDR==2 || _FH.bSetMicronDDR==3 ){//set Toggle
						if( ((buf2[ce*16]>>4) != 0x02 && _FH.bSetMicronDDR==2) || 
							((buf2[ce*16]>>4) != 0x03 && _FH.bSetMicronDDR==3) ){
								if (count != (_FH.FlhNumber * 10))
								{
									count ++;
									Sleep(300);
									pass = false;
									break;
								}
								else
								{
									return 1;
								}
						}
					}
				}
			}
		} while (pass != true);
	}

	return 0;
}

int SATASCSI3111_ST::NT_SwitchCmd()
{
	return 0;
}

/************************************************************************/
/* For S11
/* Toggle 2.0 set flash clock to 200M  buf[0x4D] = 8
/* Toggle 1.0 set flash clock to 100M  buf[0x4D] = 5
/* Legacy     set flash clock to 40M   buf[0x4D] = 2 
/*10MHZ    1
/*40MHZ    2
/*50 MHZ   3
/*83 MHZ   4
/*100 MHZ  5
/*166 MHZ  6
/*180 MHZ  7
/*200 MHZ  8
/*266MHZ   9
/************************************************************************/
int GetS11ClkMapping(int clk_mhz) {
	if (10 == clk_mhz) return 1;
	if (40 == clk_mhz) return 2;
	if (50 == clk_mhz) return 3;
	if (83 == clk_mhz) return 4;
	if (100 == clk_mhz) return 5;
	if (166 == clk_mhz) return 6;
	if (180 == clk_mhz) return 7;
	if (200 == clk_mhz) return 8;
	if (226 == clk_mhz) return 9;
	return 0;
}
int clock_idx_to_Mhz_[] = {0, 10, 40, 50, 83, 100, 166, 180, 200, 226};
int SATASCSI3111_ST::ST_SetGet_Parameter(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BOOL bSetGet,unsigned char *buf)
{
	if (SCSI_NTCMD == m_cmdtype)
	{
		return USBSCSICmd_ST::ST_SetGet_Parameter(obType,DeviceHandle,targetDisk,bSetGet,buf);
	}
	unsigned char cdb[16] = {0};
	cdb[0] = 0x06;
	cdb[1] = 0xF6;
	cdb[2] = 0xBB;


	if (bSetGet) {//Set
		memcpy(m_apParameter, buf, 1024);

		int user_clk = 0;
		switch (m_flashfeature)
		{
			case FEATURE_T2:
				user_clk = GetS11ClkMapping(GetPrivateProfileInt("Sorting","S11T2Clk",0,_ini_file_name));
				m_apParameter[0x4D] = 8;
				if (user_clk)
					m_apParameter[0x4D] = user_clk;
				break;
			case FEATURE_T1:
				user_clk = GetS11ClkMapping(GetPrivateProfileInt("Sorting","S11T1Clk",0,_ini_file_name));
				m_apParameter[0x4D] = 5;
				if (user_clk)
					m_apParameter[0x4D] = user_clk;
				break;
			case FEATURE_LEGACY:
				user_clk = GetS11ClkMapping(GetPrivateProfileInt("Sorting","S11LClk",0,_ini_file_name));
				m_apParameter[0x4D] = 2;
				if (user_clk)
					m_apParameter[0x4D] = user_clk;
				break;
		}


		int current_clk_idx = m_apParameter[0x4D];

		int tADL = GetPrivateProfileInt("[E13]", "tADL", -1, _ini_file_name);
		int tWHR = GetPrivateProfileInt("[E13]", "tWHR", -1, _ini_file_name);
		int tWHR2 = GetPrivateProfileInt("[E13]", "tWHR2", -1, _ini_file_name);
		if (-1 == tADL && -1 == tWHR && -1 == tWHR2)
		{
			if(FEATURE_LEGACY == m_flashfeature) { //legacy
				GetACTimingTable( (GetDataRateByClockIdx(current_clk_idx)) /*flash clock*/, tADL, tWHR, tWHR2 );
			} else { //toggle
				GetACTimingTable( (GetDataRateByClockIdx(current_clk_idx)/2) /*flash clock*/, tADL, tWHR, tWHR2 );
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
		if (FEATURE_T2 == m_flashfeature) {
			_controller_ODT = 3;
		}
		m_apParameter[0xE0] = _flash_driving;
		m_apParameter[0xE2] = _controller_driving;
		m_apParameter[0xE3] = _controller_ODT;

		return IssueVenderFlow(obType,targetDisk, DeviceHandle, cdb, SCSI_IOCTL_DATA_OUT, 1024, m_apParameter, _SCSI_DelayTime);
	} else {//Get
		cdb[3] = 0x01;
		return IssueVenderFlow(obType,targetDisk, DeviceHandle, cdb, SCSI_IOCTL_DATA_IN, 1024, buf, _SCSI_DelayTime);
	}

}
int SATASCSI3111_ST::NT_CustomCmd(char targetDisk, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut)
{
	if ( SSD_NTCMD == m_cmdtype ) {
		if (ATA_SetAPKey())
			return 1;
		return IssueVenderFlow(0, targetDisk, NULL, CDB, direc, iDataLen, buf, timeOut);
	}
	else {
		return USBSCSICmd_ST::NT_CustomCmd(targetDisk, CDB, bCDBLen, direc, iDataLen, buf, timeOut);
	}
}
int SATASCSI3111_ST::BankInit(BYTE bType,HANDLE DeviceHandle_p,BYTE bDrvNum ,_FH_Paramater *_FH,unsigned char *bankBuf)
{
	if ( SCSI_NTCMD == m_cmdtype ) {
		return USBSCSICmd_ST::BankInit( bType, DeviceHandle_p, bDrvNum , _FH, bankBuf);
	}
	return 0;
}

int SATASCSI3111_ST::NT_Inquiry_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, unsigned char *bBuf, unsigned char *DriveInfo)
{
	if (SCSI_NTCMD == m_cmdtype)
		return USBSCSICmd_ST::NT_Inquiry_All(bType, DeviceHandle_p, bDrvNum, bBuf, DriveInfo);
	return UFWInterface::_static_Inquiry(DeviceHandle_p, bBuf, true);
}

bool Check3111CDBisSupport(unsigned char *CDB)
{
	if((CDB[0] != 0x06) || (CDB[1] != 0xF6))
		return FALSE;
	for(int i=0; i<OPCMD_COUNT_3111; i++) {
		if(CDB[2] == OpCmd3111[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

int SATASCSI3111_ST::NT_CustomCmd_Phy(HANDLE DeviceHandle_p, unsigned char *CDB, unsigned char bCDBLen, int direc, unsigned int iDataLen, unsigned char *buf, unsigned long timeOut, bool is512CDB)
{

	int rtn = -1;
	int originalStatus = GetThreadStatus();
	while ( _requestedSuspend) {Sleep(10); SetThreadStatus(DUAL_STATUS::SUSPEND);}
	SetThreadStatus(originalStatus);
	if (m_recovered_handle && _pSortingBase ) _pSortingBase->Log("Using recovered handle, 0x%X", _hUsb);
	_isIOing = true;

	int userioTimeOut = GetPrivateProfileInt("Sorting","UserIOTimeOut", -1 ,_ini_file_name);
	int cdbDebug = GetPrivateProfileInt("Sorting","CDBDebug", 0 ,_ini_file_name);
	if(userioTimeOut != -1) {
		timeOut = userioTimeOut;
	}
	
	if ( SSD_NTCMD == m_cmdtype ) {
		if(!Check3111CDBisSupport(CDB)) {
			nt_file_log("CDB [0x%x 0x%x 0x%x 0x%x] not support.", CDB[0], CDB[1], CDB[2], CDB[3]);
			ASSERT(0);
		}
		if (ATA_SetAPKey())
			rtn = -1;
		else
			rtn = 0;

		if (!rtn) {
			rtn = IssueVenderFlow(1, NULL, m_recovered_handle?_hUsb:DeviceHandle_p, CDB, direc, iDataLen, buf, timeOut);
		}
	}
	else
	{
		rtn = USBSCSICmd_ST::NT_CustomCmd_Phy(m_recovered_handle?_hUsb:DeviceHandle_p, CDB, bCDBLen, direc, iDataLen, buf, timeOut);
	}

	if(cdbDebug) {
		_pSortingBase->Log("[CDB]%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X, rtn=%d",
			CDB[0], CDB[1], CDB[2], CDB[3], CDB[4], CDB[5],
			CDB[6], CDB[7], CDB[8], CDB[9], CDB[10], CDB[11],
			CDB[12], CDB[13], CDB[14], CDB[15], rtn);
	}

	m_recovered_handle = false;
	_isIOing = false;


	return rtn;
}

int SATASCSI3111_ST::GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &tWHR2)
{
	if ( (200 == flh_clk_mhz) || (266 == flh_clk_mhz) ) {
		tADL = 0;  tWHR = 24;  tWHR2 = 90;
	} else if (100 == flh_clk_mhz) {
		tADL = 0;  tWHR = 10;  tWHR2 = 40;
	} else if (40 == flh_clk_mhz) {
		tADL = 0;  tWHR = 1;  tWHR2 = 8;
	} else {
		nt_file_log("GetACTimingTable:can't get table for flash clk %d Mhz", flh_clk_mhz);
		assert(0);
	}
	return 0;
}

int SATASCSI3111_ST::IssueVenderFlow(char opType/*0: disk, 1:handler*/, char targetDisk, HANDLE iHandle, unsigned char *CDB, int direct, int dataLength, unsigned char *buf, int timeOut)
{
	HANDLE hdl = INVALID_HANDLE_VALUE;
	int rtn = -1;
	int rtn_reslut = 0;

	if (0 == opType) {
		hdl = MyCreateHandle(targetDisk, false, false);
	} else {
		hdl = iHandle;
	}
	unsigned char ata_register[7] = {0};
	unsigned char out_register[7] = {0};
	int ata_length = (dataLength+511)/512*512 + 512;
	unsigned char * io_buf = new unsigned char[ata_length];
	memset(io_buf, 0, ata_length);

	if (SCSI_IOCTL_DATA_UNSPECIFIED == direct){
		ata_register[6] = 0x31;
		ata_register[0] = 0x06;
		ata_register[1] = 1; /*sector cnt, 1 for 512 bytes*/
		memcpy(io_buf, CDB, 16);
		rtn = MyIssueATACmdHandle(hdl, _PreATARegister, ata_register, out_register, 2, io_buf, 512, true, 0, timeOut, 0, _deviceType);
	} else if (SCSI_IOCTL_DATA_OUT == direct) {
		ata_register[6] = 0x31;
		ata_register[0] = 0x06;
		ata_register[1] = ata_length/512;
		memcpy(io_buf, CDB, 16);
		memcpy(io_buf+512, buf, dataLength);
		rtn = MyIssueATACmdHandle(hdl, _PreATARegister, ata_register, out_register, 2/*2:write*/, io_buf, ata_length, true, 0, timeOut, 0, _deviceType);
	} else if (SCSI_IOCTL_DATA_IN == direct) {
		//Issue cmd first.
		ata_register[6] = 0x31;
		ata_register[0] = 0x06;
		ata_register[1] = 1;
		memcpy(io_buf, CDB, 16);
		rtn = MyIssueATACmdHandle(hdl, _PreATARegister, ata_register, out_register, 2, io_buf, 512, true, 0, timeOut, 0, _deviceType);

		if ( 0!= dataLength) {
			//Get result.
			int dataLength_aligned_512 = (dataLength+511)/512*512;
			unsigned char * read_buffer_aligned_512 = new unsigned char[dataLength_aligned_512];
			memset(read_buffer_aligned_512, 0, dataLength_aligned_512);
			ata_register[6] = 0x21;
			ata_register[0] = 0x07;
			ata_register[1] = dataLength_aligned_512/512;
			rtn_reslut = MyIssueATACmdHandle(hdl, _PreATARegister, ata_register, out_register, 1, read_buffer_aligned_512, dataLength_aligned_512, true, 0, timeOut, 0, _deviceType);
			memcpy(buf, read_buffer_aligned_512, dataLength);
			delete [] read_buffer_aligned_512;
		}
	} else {
		assert(0);
		if (io_buf)
			delete [] io_buf;
		return -1;
	}

	if (0 == opType) {
		MyCloseHandle(&hdl);
	}
	if (rtn_reslut) {
		rtn = rtn_reslut;
	}
	if (io_buf) {
		delete [] io_buf;
	}
	return rtn;
}


//Bics2/3(61.2)
//1Znm MLC(91.2)
//B16A(71.2)
//return 0 means fail
int SATASCSI3111_ST::CheckIfLoadCorrectBurner(HANDLE devicehandle, FH_Paramater * fh)
{

	int rtn = 0;

	if (!fh)
		return 0;
	unsigned char version[32];
	memset(version, 0, sizeof(version));
	//m_isIssuebyHandle = true;
	if (NULL != devicehandle && INVALID_HANDLE_VALUE != devicehandle)
	{
		_hUsb = devicehandle;
		ST_GetCodeVersionDate(1, devicehandle, NULL, version); // will be "2018Jul17SBBM91.2"
	}

	MyFlashInfo::UpdateFlashTypeFromID(fh);
	if (fh->bIsBiCs3QLC || fh->bIsBiCs4QLC || fh->bIsYMTC3D) {
		rtn = 1;
	} else if ((fh->bIsBics2 || fh->bIsBics3 || fh->bIsBics4) && (fh->beD3)){
		//Bics

		if (!devicehandle)
			return 1;
		if (6 != (version[13] - '0')) {
			rtn = 0;
		} else {
			rtn = 1;
		}
	} else if (((fh->bMaker == 0x98)||(fh->bMaker == 0x45)) && (fh->Design == 0x15) && (!fh->beD3)){
		//1Z MLC

		if (!devicehandle)
			return 1;
		if (9 != (version[13] - '0')) {
			rtn = 0;
		} else {
			rtn = 1;
		}
	} else if (fh->bIsMicronB16 || fh->bIsMicronB17 || fh->bIsMIcronN18 || fh->bIsMIcronN28 || fh->bIsMicronB27) {
		//B16A, B17A, N18, N28, B27

		if (!devicehandle)
			return 1;
		if (7 != (version[13] - '0')) {
			rtn = 0;
		} else {
			rtn = 1;
		}
	} else if (fh->bIsHynix3DV4) {
		if (!devicehandle)
			return 1;
		if ('E' != (version[13])) {
			rtn = 0;
		} else {
			rtn = 1;
		}
	} else {
		if (!devicehandle)
			return 0; 
	}

	if (0xff == version[0]) {
		switch (version[5])
		{
		case 0x01:
			Log(LOGDEBUG, "ERR_INITABLE_FAIL");
			break;
		case 0x02:
			Log(LOGDEBUG,"ERR_FPU_TIMEOUT");
			break;
		case 0x03:
			Log(LOGDEBUG,"ERR_RAWWRITE_TIMEOUT");
			break;
		case 0x04:
			Log(LOGDEBUG,"ERR_RDY_POLLING_TIMEOUT");
			break;
		}		
	}

	if (0 == rtn && 0xff != version[0])
		Log(LOGDEBUG,"load wrong burner, abort");
	return rtn;

}
int SATASCSI3111_ST::NT_ISPRAMS11(char * userSpecifyBurnerName)
{
	int rtn = Dev_CheckJumpBackISP(true, false);
	if (3 == rtn || 0 == rtn){
		if(ATA_ISPProgPRAM(userSpecifyBurnerName)) {
			Log(LOGDEBUG, "check jump pass but program PRAM fail");
			return 1;
		}
	} else if (1 == rtn) {
		Log(LOGDEBUG, "check jump S11 ISP fail");
		return 1;
	} else if (2 == rtn) {
		Log(LOGDEBUG, "check jump S11 ISP, it have code then will skip program PRAM");
		return 0;
	}
	return 0;
}

unsigned char isYMTC3D(const FH_Paramater* fh)
{
	unsigned char YMTC_MLC_3D[] = {0x49,0x01,0x00,0x9B,0x49};
	unsigned char YMTC_TLC_3D[] = {0xC3,0x48,0x25,0x10,0x00};
	if ( fh->bMaker==0x9B) {
		if ( memcmp(fh->id,YMTC_MLC_3D,sizeof(YMTC_MLC_3D))==0 ) {
			return 1;
		}
		else if (memcmp(fh->id,YMTC_TLC_3D,sizeof(YMTC_TLC_3D))==0) {
			return 2;	
		}
	}    
	return 0;    
}

int SATASCSI3111_ST::NT_ISPProgPRAMS11(const char * filename, HANDLE handle, bool isForceProgram, FH_Paramater * fh, char * auto_get_filename)
{
	assert(handle);
	assert(INVALID_HANDLE_VALUE != handle );
	int rtn;
	_hUsb = handle;
	char burner_name[256];
	MyFlashInfo::UpdateFlashTypeFromID(fh);
	if (fh) {
		if (fh->bIsYMTC3D) {
			sprintf_s(burner_name,"%s", S11_BURNER_YMTC);
		} else if (fh->bIsBiCs4QLC) {
			sprintf_s(burner_name,"%s", S11_BURNER_QLC);
		} else if ((fh->bMaker == 0x98) && (fh->bIsBics4 ||fh->bIsBics2 || fh->bIsBics3) && (fh->beD3)){
			sprintf_s(burner_name,"%s", S11_BURNER_TLC);
		} else if((fh->bMaker == 0x45) && (fh->bIsBics4 ||fh->bIsBics2 || fh->bIsBics3) && (fh->beD3)){
			sprintf_s(burner_name,"%s", S11_BURNER_SANDISK_TLC);
		} else if ((fh->bMaker == 0x98) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf_s(burner_name,"%s", S11_BURNER_MLC);
		} else if ((fh->bMaker == 0x45) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf_s(burner_name,"%s", S11_BURNER_SANDISK_MLC);
		} else if (fh->bIsMicronB16 || fh->bIsMicronB17) {
			sprintf_s(burner_name,"%s", S11_BURNER_B16);
		} else if (fh->bIsMIcronN18) {
			sprintf_s(burner_name,"%s", S11_BURNER_N18);
		} else if (fh->bIsMIcronN28) {
			sprintf_s(burner_name,"%s", S11_BURNER_N28);
		} else if (fh->bIsHynix3DV4 || fh->bIsHynix3DV5 ||fh->bIsHynix3DV6) {
			sprintf_s(burner_name,"%s", S11_BURNER_HYNIX);
		} else if (isYMTC3D(fh)) {
			sprintf_s(burner_name,"%s", S11_BURNER_YMTC);
		} else if (fh->bIsMIcronB05) {
			sprintf_s(burner_name,"%s", S11_BURNER_B05);
		} else if (fh->bIsMicronB27) {
			if(fh->id[3]==0xE6) {
				sprintf_s(burner_name,"%s", S11_BURNER_B27B);
			} else {
				sprintf_s(burner_name,"%s", S11_BURNER_B16);
			}
		} else {
			sprintf_s(burner_name,"%s", S11_BURNER_DEFAULT);
		}

	} else {
		sprintf_s(burner_name,"%s", filename);
	}

	if (auto_get_filename)
		sprintf_s(auto_get_filename,256,"%s", burner_name);

	Log(LOGDEBUG, "%S:select bin file=%s", __FUNCTION__, burner_name);
	//return 3 already in bootcode, return 1 failed, return 0 performed jump and success, return 2 means it have core or burner.
	rtn = Dev_CheckJumpBackISP(isForceProgram,false);
	if (3 == rtn || 0 == rtn){

		WriteRead_4Byte_Reg_Vendor(0, 0x5C0D2718, 1, 1);//Issue this vendor in boot code for CD version's controller
		//even it is not CD controllor we issue this looks fine.

		if(ATA_ISPProgPRAM(burner_name)) {
			Log(LOGDEBUG, "check jump pass but program PRAM fail");
			return 1;
		}
	} else if (1 == rtn) {
		Log(LOGDEBUG, "check jump S11 ISP fail");
		return 1;
	} else if (2 == rtn) {
		Log(LOGDEBUG, "check jump S11 ISP, it have code then will skip program PRAM");
		return 0;
	}
	return 0;
}

/*S11 not support command*/
int SATASCSI3111_ST::NT_ReadPage_All(BYTE bType,HANDLE DeviceHandle_p, BYTE bDrvNum, const char *type, unsigned char *bBuf, unsigned char *DriveInfo) 
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::NT_ReadPage_All(bType,DeviceHandle_p, bDrvNum, type, bBuf, DriveInfo);
	}
	return 0;
}

int SATASCSI3111_ST::ST_CheckDRY_DQS(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE TYPE,BYTE CE_bit_map,DWORD Block) 
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_CheckDRY_DQS(obType,DeviceHandle, targetDisk, TYPE, CE_bit_map, Block);
	}
	return 0;
}

int SATASCSI3111_ST::ST_SetDegree(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int Phase, short delayOffset) 
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_SetDegree(obType, DeviceHandle, targetDisk, Phase, delayOffset);
	}
	return 0;
}

int SATASCSI3111_ST::ST_FWInit(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk) 
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_FWInit(obType, DeviceHandle, targetDisk);
	}
	return 0;
}

int SATASCSI3111_ST::ST_GetSRAM_GWK(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk, unsigned char* w_buf) 
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_GetSRAM_GWK(obType, DeviceHandle, targetDisk, w_buf);
	}
	return 0;
}

int SATASCSI3111_ST::ST_DumpSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf)
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_DumpSRAM(obType, DeviceHandle, targetDisk, BufferPoint, r_buf);
	}
	return 0;
}

int SATASCSI3111_ST::ST_WriteSRAM(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE BufferPoint,unsigned char *r_buf)
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_WriteSRAM(obType, DeviceHandle, targetDisk, BufferPoint, r_buf);
	}
	return 0;
}

int SATASCSI3111_ST::ST_SetQuickFindIBBuf(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char *buf)
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_SetQuickFindIBBuf(obType, DeviceHandle, targetDisk, buf);
	}
	return 0;
}

int SATASCSI3111_ST::ST_QuickFindIB(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, int ce, unsigned int dieoffset, unsigned int BlockCount, unsigned char *buf, int buflen,
									int iTimeCount, BYTE bIBSearchPage, BYTE CheckPHSN)
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_QuickFindIB(obType, DeviceHandle, targetDisk, ce, dieoffset, BlockCount, buf, buflen, iTimeCount, bIBSearchPage, CheckPHSN);
	}
	return -1; /*0 : pass, -1: fail*/
}

int SATASCSI3111_ST::ST_CheckU3Port(BYTE obType, HANDLE DeviceHandle, BYTE targetDisk, unsigned char* buf)
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_CheckU3Port(obType, DeviceHandle, targetDisk, buf);
	}
	return 0;
}

int SATASCSI3111_ST::ST_APGetSetFeature(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE SetGet,BYTE Address,BYTE Param0,BYTE Param1,BYTE Param2,BYTE Param3,unsigned char *buf)
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_APGetSetFeature(obType, DeviceHandle, targetDisk, SetGet, Address, Param0, Param1, Param2, Param3, buf);
	}
	return 0;
}

int SATASCSI3111_ST::NT_JumpISPMode(BYTE obType,HANDLE DeviceHandle,char drive)
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::NT_JumpISPMode(obType, DeviceHandle, drive);
	}
	return 0;
}

int SATASCSI3111_ST::ST_HynixCheckWritePortect(BYTE bType,HANDLE DeviceHandle, BYTE bDrvNum)
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_HynixCheckWritePortect(bType, DeviceHandle, bDrvNum);
	}
	return 0;

}

int SATASCSI3111_ST::ST_HynixTLC16ModeSwitch(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bMode)
{
	if (SCSI_NTCMD == m_cmdtype){
		return USBSCSICmd_ST::ST_HynixTLC16ModeSwitch(obType, DeviceHandle, targetDisk, bMode);
	}
	return 0;
}














