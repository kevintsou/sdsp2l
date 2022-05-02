#include <assert.h>

#include "NewSQLCmd.h"
#define TowPlaneRead3Cycle		_CmdBuf[0]
#define D1_Mode		_CmdBuf[1]
#define D3_Mode		_CmdBuf[2]
#define EraseCmdS1		_CmdBuf[3]
#define EraseCmdS2		_CmdBuf[4]
#define EraseCmdE1		_CmdBuf[5]
#define EraseCmdE2		_CmdBuf[6]
#define ProgramCmdS		_CmdBuf[7]
#define ProgramCmd2PS1		_CmdBuf[8]
#define ProgramCmd2PS2		_CmdBuf[9]
#define ProgramChange		_CmdBuf[10]
#define ProgramCmdEnd		_CmdBuf[11]
#define MultipageCmd		_CmdBuf[12]
#define CacheProgram		_CmdBuf[13]
#define D3_Cache		_CmdBuf[14]
#define ReadCmdS		_CmdBuf[15]
#define ReadCmd2PS1		_CmdBuf[16]
#define ReadCmd2PS2		_CmdBuf[17]
#define ReadChangeS		_CmdBuf[18]
#define ReadChangeE		_CmdBuf[19]
#define ReadCmdEnd		_CmdBuf[20]
#define ReadCmdEnd1		_CmdBuf[21]
#define ReadCmdEnd2		_CmdBuf[22]
#define ReadCmdEnd3		_CmdBuf[23]
#define ReadCache1		_CmdBuf[24]
#define ReadCache2		_CmdBuf[25]
#define CopybackReadS		_CmdBuf[26]
#define CopybackReadS1		_CmdBuf[27]
#define CopybackReadE		_CmdBuf[28]
#define CopybackReadE1		_CmdBuf[29]
#define CopybackReadE2		_CmdBuf[30]
#define CopybackProgramS		_CmdBuf[31]
#define CopybackProgramS1		_CmdBuf[32]
#define CopybackProgramS2		_CmdBuf[33]
#define CopybackProgramE1		_CmdBuf[34]
#define CopybackProgramE2		_CmdBuf[35]
#define D3_ReadCache1		_CmdBuf[36]
#define BOOTMODE_PREFIX		_CmdBuf[37]

SQLCmd::SQLCmd()
{
//    _scsi = 0;
//    _flashID = 0;
//    _toggle = 0;
//    _plane = 0;
//    _blockPerDie = 0;
//    _pagePerBlock = 0;
//    _pageSize = 0;
    _dumpSQL = 0;
    _eccBufferAddr = 0;
	_dataBufferAddr = 0; 
	_BootMode = 0;
	_CmdBuf = NULL;
	_bAddr6 = false;
	_bWaitRDY = 1; 	// default used wait RDY
					// 0x00 --> check status
					// 0xff --> both not to do
	_bDFmode = false;
	_bpTLC = false;
	_bSkipWDMA = false;
}

SQLCmd::~SQLCmd()
{
}

//SQLCmd* SQLCmd::Instance(NetSCSICmd* scsi, FH_Paramater *fh)
SQLCmd* SQLCmd::Instance(UFWSCSICmd* scsi, FH_Paramater *fh, unsigned char sqlECCAddr, unsigned char sqlDATAAddr)
{
    assert(scsi);
    assert(fh);
    
    SQLCmd* cmd = 0;
    
    if( fh->bMaker==0x98 || fh->bMaker==0x45 ) 
	{
		if((fh->bIs3D)&&(fh->beD3))
        	cmd = new TSB3D_SQLCmd(fh);
		else if(fh->beD3)
			cmd = new TSBD3_SQLCmd(fh);
		else
			cmd = new TSBD2_SQLCmd(fh);
    }
    else if( fh->bMaker==0x2C || fh->bMaker==0x89 ) 
	{
       	if((fh->bIs3D)&&(fh->beD3))
        	cmd = new IM3D_SQLCmd(fh);
		else if(fh->beD3)
			cmd = new IMD3_SQLCmd(fh);
		else
			cmd = new IMD2_SQLCmd(fh);
    }    
    else if( fh->bMaker==0xAD ) 
	{
		if((fh->bIs3D)&&(fh->beD3))
        	cmd = new HYX3D_SQLCmd(fh);
		else if(fh->beD3)
			cmd = new HYXD3_SQLCmd(fh);
		else
			cmd = new HYXD2_SQLCmd(fh);
    }
	else if( fh->bMaker==0x9B ) 
	{
		if((fh->bIs3D)&&(fh->beD3))
        	cmd = new YMTC3D_SQLCmd(fh);
		else if(fh->beD3)
			cmd = new YMTCD3_SQLCmd(fh);
		else
			cmd = new YMTCD2_SQLCmd(fh);
    }
    else if( fh->bMaker==0xEC ) {
        cmd = new SamsungSQLCmd();
    }
    else 
	{
#ifndef _USB_
        assert(cmd);
#endif
		cmd = new TSB3D_SQLCmd(fh);
    }
    
    cmd->_scsi = scsi;
    cmd->_fh=fh; //need or not?
    cmd->_eccBufferAddr = sqlECCAddr;
    cmd->_dataBufferAddr = sqlDATAAddr;

    return cmd;
}

int SQLCmd::LogTwo(int n)
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

int SQLCmd::GetStatus(unsigned char ce, unsigned char* buf, unsigned char cmd, bool bSaveStatus)
{
    int ret;
	unsigned char tmpbuf[512];
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char sql[]= 
	{
		// 0,   1,   2,
		0xCA,0x05,0x01,
		// 3,   4,   5,   6,   7,
		0xC2,0x70,0xDB,0x01,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
    sql[4]=cmd;
    int slen=sizeof(sql);
    //ret=_scsi->Dev_IssueIOCmd(sql,slen,2,ce,buf,512);
    if(bSaveStatus)
    {
    	_status[ce] = 0x80;
    }
    ret=Dev_IssueIOCmd(sql,slen,2,ce,tmpbuf,512);
	buf[0]=tmpbuf[0];
	if(bSaveStatus)
	{
		_status[ce] = buf[0];
	}
	return ret;
}

int SQLCmd::OnlyWiatRDY(unsigned char ce)
{
    int ret;
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char sql[]= 
	{
		// 0,   1,   2,
		0xCA,0x02,0x01,
		// 3,   4,
		0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};

    int slen=sizeof(sql);

    ret=Dev_IssueIOCmd(sql,slen,0,ce,NULL,0);
	
	return ret;
}

int SQLCmd::FlashReset(unsigned char ce,unsigned char cmd)
{
    int ret;
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char sql[]= 
	{
		// 0,   1,   2,
		0xCA,0x04,0x01,
		// 3,   4,   5,   6,
		0xC2,0xFF,0xCE,0xCB,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	sql[4]=cmd;
    int slen=sizeof(sql);

    ret=Dev_IssueIOCmd(sql,slen,0,ce,NULL,0);
	
	return ret;
}

int SQLCmd::Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, unsigned char ce, unsigned char* buf, int len )
{
    if ( sqllen ) {
        if ( _dumpSQL==1 ) {
            //_scsi->Log(LOGDEBUG,"Dump sql ce %d mode %d buflen=%d sqllen=%d\n",ce,mode,len,sqllen );
            _scsi->LogMEM ( sql, sqllen );
        }
        //else if ( _dumpSQL==3 ) {
        //    LogCmdAction( sql, sqllen );
        //    return 0;
        //}
        else if ( _dumpSQL==4 ) {
            return 0;
        }
    }
    else if ( _dumpSQL==2 ) {
        //_scsi->Log(LOGDEBUG,"Dump sql ce %d mode %d buflen=%d sqllen=%d\n",ce,mode,len,sqllen );
        _scsi->LogMEM (sql, sqllen );
    }

    return _scsi->Dev_IssueIOCmd( sql,sqllen, mode, ce, buf, len );
}

void SQLCmd::SetDumpSQL( int v )
{
	_dumpSQL = v;
}



//int SQLCmd::_Dev_IssueIOCmd9K( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len )
//{
//    //0=write,1=write+buf,2=read
//    int err;
//    int hmode = mode&0xF0;
//    mode = mode&0x0F;
//
//    if ( !hmode ) {
//        assert(sql[sql[1]+3-1]==0xCE);
//        //SetFTA(&sql[slen-8],page/2,blk);
//    }
//
//    if ( (mode&0x03)==1 ) {
//        unsigned char cdb[16] = {0x06,0xFC,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
//		cdb[2] = _dataBufferAddr;
//        cdb[3] = len/512;
//        err = _IssueCmd_byHandle ( cdb,_CDB_WRITE, buf, len );
//        if ( err ) {
//            Log (LOGDEBUG, "write _Dev_IssueIOCmd %s ret=%d\n", _cdb, err );
//            LogMEM ( sql,sqllen );
//            LogFlush();
//        }
//        assert(err==0);
//    }
//
//    //trigger command
//    if ( !hmode )  {
//        unsigned char cmdbuf[512];
//        memcpy(cmdbuf,sql,sqllen);
//        unsigned char cdb[16] = {0x06,0xFE,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
//        cdb[5] = mode!=3 ? len/512:_FH.PageSizeK;
//        cdb[6] = ce;
//        cdb[7] = mode;
//        //Log("%s\n",__FUNCTION__);
//        //LogMEM ( cmdbuf, sizeof(cmdbuf) );
//        err = _IssueCmd_byHandle ( cdb,_CDB_WRITE, cmdbuf, sizeof(cmdbuf) );
//        if ( err ) {
//            Log (LOGDEBUG, "trigger _Dev_IssueIOCmd %s ret=%d\n", _cdb, err );
//            LogMEM ( sql,sqllen );
//            LogFlush();
//        }
//        assert(err==0);
//        if ( mode==2 ) {
//            err = Dev_GetSRAMData ( _dataBufferAddr, buf, len );
//        }
//        else if ( mode==3 ) {
//            //read ecc
//            assert( len==_FH.PageFrameCnt );
//            err = Dev_GetSRAMData ( _eccBufferAddr, cmdbuf, 512 );
//            if ( err ) {
//                Log (LOGDEBUG, "get SRAM _Dev_IssueIOCmd %s ret=%d\n", _cdb, err );
//                LogMEM ( sql,sqllen );
//                LogFlush();
//            }
//            assert(err==0);
//            memcpy(buf,cmdbuf,len);
//        }
//    }
//    
//    return err;
//}

int SQLCmd::GetPhyBlkFromLogBlk(int block)
{
	int dieOffset;
	if(_fh->Block_Per_Die>16384)
		dieOffset=32768;
	else if(_fh->Block_Per_Die>8192)
		dieOffset=16384;
	else if(_fh->Block_Per_Die>4096)
		dieOffset=8192;
	else if(_fh->Block_Per_Die>2048)
		dieOffset=4096;
	else if(_fh->Block_Per_Die>1024)
		dieOffset=2048;
	else
		dieOffset=1024;

	return block%_fh->Block_Per_Die+(block/_fh->Block_Per_Die)*dieOffset;
}

void SQLCmd::SetFTA(unsigned char* fta, int page, int logicalblock, int maxpage)
{
	int phyblock=GetPhyBlkFromLogBlk(logicalblock);
	fta[0]=0;
	fta[1]=0;
	fta[2]=page&0xFF;

	if((_fh->PagePerBlock>4096)||(maxpage>4096))
	{
		phyblock=((phyblock<<5)|((page>>8)&0x01F));
		fta[3]=(phyblock)&0xFF;
		fta[4]=(phyblock>>8)&0xFF;
		fta[5]=(phyblock>>16)&0xFF;
	}
	else if((_fh->PagePerBlock>2048)||(maxpage>2048))
	{
		phyblock=((phyblock<<4)|((page>>8)&0x0F));
		fta[3]=(phyblock)&0xFF;
		fta[4]=(phyblock>>8)&0xFF;
		fta[5]=(phyblock>>16)&0xFF;
	}
	else if((_fh->PagePerBlock>1024)||(maxpage>1024))
	{
		phyblock=((phyblock<<3)|((page>>8)&0x07));
		fta[3]=(phyblock)&0xFF;
		fta[4]=(phyblock>>8)&0xFF;
		fta[5]=(phyblock>>16)&0xFF;
	}
	else if((_fh->PagePerBlock>512)||(maxpage>512))
	{
		phyblock=((phyblock<<2)|((page>>8)&0x03));
		fta[3]=(phyblock)&0xFF;
		fta[4]=(phyblock>>8)&0xFF;
		fta[5]=(phyblock>>16)&0xFF;
	}
	else if(_fh->PagePerBlock>256)
	{
		phyblock=((phyblock<<1)|((page>>8)&0x01));
		fta[3]=(phyblock)&0xFF;
		fta[4]=(phyblock>>8)&0xFF;
		fta[5]=(phyblock>>16)&0xFF;
	}
	else if(_fh->PagePerBlock<=256) 
	{
		if(_fh->PagePerBlock==128) 
		{
			fta[2]|=((phyblock&0x01)<<7);
			phyblock=(phyblock>>1);
			fta[3]=(phyblock)&0xFF;
			fta[4]=(phyblock>>8)&0xFF;
			fta[5]=(phyblock>>16)&0xFF;
		}
		else if(_fh->PagePerBlock==64) 
		{
			fta[2]|=(phyblock&0x03)<<6;
			phyblock=(phyblock>>2);
			fta[3]=(phyblock)&0xFF;
			fta[4]=(phyblock>>8)&0xFF;
			fta[5]=(phyblock>>16)&0xFF;
		}
		else  
		{
			fta[3]=phyblock&0xFF;
			fta[4]=(phyblock>>8)&0xFF;
			fta[5]=0;
		}
	}
	else 
	{
		assert(0);
	}
}

////////////////////////////vendor command only////////////////////////
int SQLCmd::SetVendorMode ( unsigned char ce )
{
	assert(0);
    return 0;
}

int SQLCmd::CloseVendorMode ( unsigned char ce )
{
	assert(0);
    return 0;
}

int SQLCmd::GetProgramLoop ( int ce, int* loop )
{
	assert(0);
    return 0;
}

int SQLCmd::CLESend ( unsigned char ce, unsigned char value, bool waitrdy )
{
    unsigned char sql1[] = { 0xCA,0x04,0x01,
                             0xC2,0x60,0xCB,0xCE,
                             0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
                           };
    unsigned char sql2[] = { 0xCA,0x03,0x01,
                             0xC2,0x60,0xCE,
                             0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
                           };
    unsigned char *sql = 0;
    int slen = 0;
    if ( waitrdy ) {
        sql = sql1;
        slen = sizeof(sql1);
        sql[4] = value;
    }
    else {
        sql = sql2;
        slen = sizeof(sql2);
        sql[4] = value;
    }

    int ret= Dev_IssueIOCmd(sql,slen, 0, ce, 0, 0 );
    return ret;
}

int SQLCmd::GetFlashID ( unsigned char ce, unsigned char* ibuf )
{
	assert(0);
    return 0;
}

int SQLCmd::SetBootMode( unsigned char bMode )
{
	_BootMode = bMode;
    return 0;
}

int SQLCmd::SetFlashCmdBuf( unsigned char *buf )
{
	//memcpy(_CmdBuf,buf,sizeof(_CmdBuf));
	_CmdBuf = buf;
	return 0;
}

int SQLCmd::SetWaitRDYMode( unsigned char bMode )
{
	_bWaitRDY= bMode;
    return 0;
}

int SQLCmd::SetDFMode( unsigned char bMode )
{
	_bDFmode= bMode?true:false;
    return 0;
}

int SQLCmd::SetpTLC( unsigned char bMode )
{
	_bpTLC= bMode?true:false;
    return 0;
}

int SQLCmd::SetSkipWriteDMA( unsigned char bMode )
{
	_bSkipWDMA= bMode?true:false;
    return 0;
}

int SQLCmd::SetFeature( unsigned char ce, unsigned char offset, unsigned char *value, bool bToggle, char die )
{
	int ret,slen=0,i;
    unsigned char sql[256];
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	/*unsigned char sqlEF[]=
	{
        // 0,   1,   2,
        0xCA,0x10,0x01,
        // 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,
        0xCF,0x01,0xC2,0xEF,0xAB,0x01,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xCB,0xCE,
        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
    };
    */
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0

	sql[slen]=0x01;slen++;	// Operation setting

	if(bToggle)
	{
		sql[slen]=0xCF;slen++;	// Enable High speed mode
		sql[slen]=0x01;slen++;	
	}
	sql[slen]=0xC2;slen++;	
	if(die!=-1)
	{
		sql[slen]=0xD5;slen++;	// by die
		sql[slen]=0xAB;slen++;
		sql[slen]=die;slen++;	// die offset
	}
	else
	{
		sql[slen]=0xEF;slen++;
	}
	sql[slen]=0xAB;slen++;
	sql[slen]=offset;slen++;

	for(i=0;i<4;i++)
	{
		sql[slen]=0xDE;slen++;
		sql[slen]=value[i];slen++;
	}
	sql[slen]=0xCB;slen++;

	if( _fh->bMaker == 0x45 )
	{
		sql[slen]=0xC2;slen++;
		sql[slen]=0x5D;slen++;
	}
	
	sql[slen]=0xCE;slen++;			// command End
	sql[1]=slen-3;
	sql[slen]=0xA0;slen++;

	SetFTA(&sql[slen], 0, 0, 0);
	slen+=6;
	
	sql[slen]=0xAE;slen++;
	sql[slen]=0xAF;slen++;
    
    //ret = _scsi->IssueIOCmd(sql, slen, 0, ce, 0, 0);
    ret = Dev_IssueIOCmd(sql, slen, 0, ce, 0, 0);
	return ret;
}

int SQLCmd::GetFeature( unsigned char ce, unsigned char offset, unsigned char *value, bool bToggle, char die )
{
	int ret,slen=0,i;
    unsigned char sql[256],buf[512];;
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	/*
	unsigned char sqlEE[]=
	{
        // 0,   1,   2,
        0xCA,0x08,0x01,
        // 3,   4,   5,   6,   7,   8,   9,  10,
        0xC2,0xEE,0xAB,0x00,0xCB,0xDB,0x04,0xCE,
        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
    };
    */
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0

	sql[slen]=0x01;slen++;	// Operation setting

	if(bToggle)
	{
		sql[slen]=0xCF;slen++;	// Enable High speed mode
		sql[slen]=0x01;slen++;	
	}
	sql[slen]=0xC2;slen++;	
	if(die!=-1)
	{
		sql[slen]=0xD4;slen++;	// by die
		sql[slen]=0xAB;slen++;
		sql[slen]=die;slen++;	// die offset
	}
	else
	{
		sql[slen]=0xEE;slen++;
	}
	sql[slen]=0xAB;slen++;
	sql[slen]=offset;slen++;

	sql[slen]=0xCB;slen++;

	sql[slen]=0xDB;slen++;
	sql[slen]=4;slen++;

	if( _fh->bMaker == 0x45 )
	{
		sql[slen]=0xC2;slen++;
		sql[slen]=0x5D;slen++;
	}

	sql[slen]=0xCE;slen++;			// command End
	sql[1]=slen-3;
	sql[slen]=0xA0;slen++;

	SetFTA(&sql[slen], 0, 0, 0);
	slen+=6;
	
	sql[slen]=0xAE;slen++;
	sql[slen]=0xAF;slen++;
    
    //ret = _scsi->IssueIOCmd(sql, slen, 0, ce, 0, 0);
    ret = Dev_IssueIOCmd(sql, slen, 2, ce, buf, 512);
	for(i=0;i<4;i++)
	{
    	value[i]=buf[i];
	}
	return ret;
}

int SQLCmd::ParameterOverLoad( unsigned char ce, unsigned char die, unsigned char offset )
{
	int ret,slen=0;
    unsigned char sql[256],buf[512];;
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);

	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0

	sql[slen]=0x01;slen++;	// Operation setting

	sql[slen]=0xC2;slen++;
	sql[slen]=(0xF1+die);slen++;

	sql[slen]=0xC2;slen++;
	sql[slen]=0x98;slen++;

	sql[slen]=0xAB;slen++;
	sql[slen]=offset;slen++;

	sql[slen]=0xCB;slen++;
	
	sql[slen]=0xC2;slen++;
	sql[slen]=0x70;slen++;

	sql[slen]=0xDB;slen++;
	sql[slen]=0x01;slen++;

	sql[slen]=0xCE;slen++;			// command End
	sql[1]=slen-3;
	sql[slen]=0xA0;slen++;

	SetFTA(&sql[slen], 0, 0, 0);
	slen+=6;
	
	sql[slen]=0xAE;slen++;
	sql[slen]=0xAF;slen++;
    
    //ret = _scsi->IssueIOCmd(sql, slen, 0, ce, 0, 0);
    ret = Dev_IssueIOCmd(sql, slen, 2, ce, buf, 512);
	if ((buf[0] & 0x01) == 0x01)
	{
		return 1;
	}
	GetStatus(ce, buf, 0x76, false);

	if (buf[0] == offset )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
int SQLCmd::Erase(unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6)
{
	unsigned char sql[256],slen = 0, temp, i, ftap[6];
	int CE, ret = 0, maxpage = 0;

	if(((_fh->bMaker==0x2C)||(_fh->bMaker==0x89)||(_fh->bMaker==0x9B))&&(_fh->bIs3D))
	{
		maxpage = _fh->BigPage;
	}
	for( CE = 0; CE < _fh->FlhNumber; CE++ )
	{
		slen=0;
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(_fh->FlhNumber==1)
			{
				CE = 0;
			}
			else
			{
				CE = ce;
			}
		}
		//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
		sql[slen]=0xCA;slen++;	// Stare from 0xCA
		sql[slen]=0x00;slen++;	// temp set Command len = 0
		temp=0x01;				// default used one addrress mode

		sql[slen]=temp;slen++;	// Operation setting

		if(bDFmode)
		{
			if(_fh->Design<=0x15)
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xC6;slen++;
				
			}
			else
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xDF;slen++;
			}
		}
		if(bSLCMode)
		{
			if(_BootMode)
			{
				if(_fh->bMaker==0xAD)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xDA;slen++;	// FW SLC mode
				}
				else
				{
					assert(0);
				}
			}
			else
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=D1_Mode;slen++;
			}
		}
		else if((_fh->bIs3D)&&((_fh->bMaker==0x89)||(_fh->bMaker==0x2c)||(_fh->bMaker==0x9B)))
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=D3_Mode;slen++;
		}
		
		if((planemode>=2)||(bAddr6))
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i,maxpage);
				sql[slen]=0xC2;slen++;
				sql[slen]=EraseCmdS1;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
				if(bAddr6)
				{
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
				}
				if((_fh->bMaker==0xAD)&&(i!=(planemode-1)))
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=EraseCmdE2;slen++;
				}
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=EraseCmdE1;slen++;
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=EraseCmdS1;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=EraseCmdE1;slen++;
			sql[slen]=0xCC;slen++;
		}
			
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(bWaitRDY==1)
			{
				sql[slen]=0xCB;slen++;
				if((bSLCMode)&&(_BootMode))
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xFF;slen++;	// reset FW SLC mode to normal mode
					sql[slen]=0xCB;slen++;
				}
			}
		}
		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], 0, block, maxpage);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //int ret=_scsi->Dev_IssueIOCmd(sql, slen, 0, ce, 0, 0);
	    ret=Dev_IssueIOCmd(sql, slen, 0, CE, 0, 0);
		if ( !ret )
		{
			if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
			{
				if(bWaitRDY==0)
				{
					// check status
					int timeout=0;
					do
					{
						GetStatus(CE, ftap, 0x70);	// 750us one time
						if(timeout>=4000)	// Max 3sec timeout
						{
							break;
						}
						timeout++;
					}
					while((ftap[0]&0xE0)!=0xE0);
					if((bSLCMode)&&(_BootMode))
					{
						FlashReset(CE);
					
						timeout=0;
						do
						{
							GetStatus(CE, ftap, 0x70,false);	// no save status
							if(timeout>=4000)
							{
								break;
							}
							timeout++;
						}
						while((ftap[0]&0xE0)!=0xE0);
					}
				}
			}
		}
		else
		{
			break;
		}
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			break;
		}
	}
	if((ce==_fh->FlhNumber)&&(_fh->FlhNumber!=1))
	{
		for( CE = 0; CE < _fh->FlhNumber; CE++ )
		{
			if(bWaitRDY==0)
			{
				// check status
				int timeout=0;
				do
				{
					GetStatus(CE, ftap, 0x70);	// 750us one time
					if(timeout>=4000)	// Max 3sec timeout
					{
						break;
					}
					timeout++;
				}
				while((ftap[0]&0xE0)!=0xE0);
				if((bSLCMode)&&(_BootMode))
				{
					FlashReset(CE);
					timeout=0;
					do
					{
						GetStatus(CE, ftap, 0x70,false);	// no save status
						if(timeout>=4000)
						{
							break;
						}
						timeout++;
					}
					while((ftap[0]&0xE0)!=0xE0);
				}
			}
			else if(bWaitRDY==1)
			{
				OnlyWiatRDY(CE);
				if((bSLCMode)&&(_BootMode))
				{
					FlashReset(CE);
					OnlyWiatRDY(CE);
				}
			}
		}
	}
    return ret;
}

int SQLCmd::Write(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode, unsigned char ProgStep, bool bCache, bool bLastPage, char bWaitRDY, bool bAddr6)
{
	unsigned char sql[256],slen=0,temp,ftap[6],chkStatus=0x40;
	unsigned char cell=LogTwo(_fh->Cell);
	unsigned char p_cnt = (ProgStep>>4);
	int maxpage=0;
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	ProgStep &=0x0F;
	
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x81;				// default used write mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting
	if(((_fh->bMaker==0x2C)||(_fh->bMaker==0x89)||(_fh->bMaker==0x9B))&&(_fh->bIs3D))
	{
		maxpage = _fh->BigPage;
	}
	if(bSLCMode)
	{
		if(_BootMode)
		{
			if(_fh->bMaker==0xAD)
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xDA;slen++;	// FW SLC mode
			}
			else
			{
				assert(0);
			}
		}
		else
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=D1_Mode;slen++;
		}
	}
	else if((_fh->bIs3D)&&((_fh->bMaker==0x89)||(_fh->bMaker==0x2c)||(_fh->bMaker==0x9B)))
	{
		sql[slen]=0xC2;slen++;
		sql[slen]=D3_Mode;slen++;
	}
	else if(_fh->beD3)	// skip Micron/Intel and YMTC==> Toshiba/Sandisk/Hynix
	{	
		if(ProgStep)// 19nm/24nm programming step
		{
			sql[slen]=0xC2;slen++;
			if(ProgStep==1)
			{
				sql[slen]=0x09;slen++;
			}
			else
			{
				sql[slen]=0x0D;slen++;// if QLC set ProgSetp=2 for 0x0D command send
			}
		}
		sql[slen]=0xC2;slen++;
		sql[slen]=p_cnt+1;slen++;
	}
	
	if(bAddr6)
	{
		SetFTA(ftap, page, block, maxpage);
		sql[slen]=0xC2;slen++;
		if(planemode>=2)
		{
			if((block%planemode)!=0)
			{
				sql[slen]=ProgramCmd2PS2;slen++;
			}
			else
			{
				sql[slen]=ProgramCmd2PS1;slen++;
			}
		}
		else
		{
			sql[slen]=ProgramCmdS;slen++;
		}

		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[0];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[1];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[2];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[3];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[4];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[5];slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}
	else
	{			
		sql[slen]=0xC0;slen++;
		if(planemode>=2)
		{
			if((block%planemode)!=0)
			{
				sql[slen]=ProgramCmd2PS2;slen++;
			}
			else
			{
				sql[slen]=ProgramCmd2PS1;slen++;
			}
		}
		else
		{
			sql[slen]=ProgramCmdS;slen++;
		}
		sql[slen]=0xA0;slen++;
		sql[slen]=0xCC;slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}		
	sql[slen]=0xC2;slen++;
	chkStatus=0x40;
	if((block%planemode)!=(planemode-1))
	{
		sql[slen]=MultipageCmd;slen++;
	}	
	else if(_fh->beD3&&(p_cnt!=(cell-1))&&(!bSLCMode))// Micron/Intel and YMTC should not go to here
	{
		sql[slen]=D3_Cache;slen++;						// Used p_cnt = cell -1 to skip it
	}
	else if(bCache&&(!bLastPage))
	{
		sql[slen]=CacheProgram;slen++;
		chkStatus=0x20;
	}
	else if((_fh->beD3)&&(_fh->bIs3D)&&(_fh->bMaker==0xAD))
	{
		sql[slen]=CopybackProgramE1;slen++;
	}
	else
	{
		sql[slen]=ProgramCmdEnd;slen++;
	}

	if(bWaitRDY==1)
	{
		sql[slen]=0xCB;slen++;
		if((bSLCMode)&&(_BootMode))
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xFF;slen++;	// reset FW SLC mode to normal mode
			sql[slen]=0xCB;slen++;
		}
	}
	
	sql[slen]=0xCE;slen++;			// command End
	sql[1]=slen-3;
	sql[slen]=0xA0;slen++;

	SetFTA(&sql[slen], page, block, maxpage);
	slen+=6;
	
	sql[slen]=0xAE;slen++;
	sql[slen]=0xAF;slen++;

	//int ret=_scsi->Dev_IssueIOCmd(sql, slen, 1, ce, buf, len);
	int ret=Dev_IssueIOCmd(sql, slen, 1, ce, (unsigned char*)buf, len);  
	if ( ret ) 
	{
		return ret;
	}
	if(bWaitRDY==0)
	{
		// check status
		int timeout=0;
		do
		{
			GetStatus(ce, ftap, 0x70);	// 750us one time
			if(timeout>=4000)	// Max 3sec timeout
			{
				break;
			}
			timeout++;
		}
		while((ftap[0]&0x60)!=chkStatus);
	}
	return 0;
}

int SQLCmd::Read(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode, bool bReadCmd, bool bReadData, bool bReadEcc, bool bCache, bool bFirstPage, bool bLastPage, bool bCmd36, char bWaitRDY, bool bAddr6)
{
	int ret,maxpage=0;
	unsigned char sql[256],slen=0,temp,ftap[6],mode=0,ReadS,ReadE=0,chkStatus;// mode == 2 is read data
	unsigned char cell=LogTwo(_fh->Cell);
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x01;				// default used read mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting
	if(((_fh->bMaker==0x2C)||(_fh->bMaker==0x89)||(_fh->bMaker==0x9B))&&(_fh->bIs3D))
	{
		maxpage = _fh->BigPage;
	}
	if(bReadCmd)
	{
		chkStatus=0x40;
		if(bCmd36)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0x36;slen++;
		}
		if(bSLCMode)
		{
			if(_BootMode)
			{
				if(_fh->bMaker==0xAD)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xDA;slen++;	// FW SLC mode
				}
				else
				{
					assert(0);
				}
			}
			else
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=D1_Mode;slen++;
			}
		}
		else if((_fh->bIs3D)&&((_fh->bMaker==0x89)||(_fh->bMaker==0x2c)||(_fh->bMaker==0x9B)))
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=D3_Mode;slen++;
		}
		else if(_fh->beD3)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=(page%cell)+1;slen++;
			page/=cell;// change to WL mode
		}

		if(bCache&&(bLastPage))
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=ReadCache2;slen++;
		}
		else
		{
			if(planemode>=2)
			{
				if((block%planemode)!=(planemode-1))
				{
					ReadS=ReadCmd2PS1;
				}
				else
				{
					ReadS=ReadCmd2PS2;
				}
			}
			else
			{
				ReadS=ReadCmdS;
			}
			if((!_fh->beD3)&&bCache&&(planemode==1)&&(!bFirstPage))	// Sequential cache read
			{
				if(bLastPage)
				{
					ReadE=ReadCache2;
				}
				else
				{
					ReadE=ReadCache1;
					chkStatus=0x20;
				}
				sql[slen]=0xC2;slen++;
				sql[slen]=ReadE;slen++;
			}
			else
			{
				if(bCache&&(bLastPage))
				{
					ReadE=ReadCache2;
				}
				else if((block%planemode)!=(planemode-1))
				{
					ReadE=ReadCmdEnd2;
				}
				else if(bCache&&(!bFirstPage))
				{
					ReadE=ReadCache1;
					chkStatus=0x20;
				}
				else
				{
					ReadE=ReadCmdEnd;
				}
				SetFTA(ftap, page, block, maxpage);
				if(bAddr6)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=ReadS;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=ReadE;slen++;
				}
				else
				{	
					sql[slen]=0xC0;slen++;
					sql[slen]=ReadS;slen++;				
					sql[slen]=0xA0;slen++;
					sql[slen]=0xC1;slen++;
					sql[slen]=ReadE;slen++;
					sql[slen]=0xCC;slen++;
				}		
			}
		}
		
		if(bWaitRDY==1)
		{
			sql[slen]=0xCB;slen++;		
			if((bReadData)&&(ReadE==0x30))
			{
				SetFTA(ftap, page, block, maxpage);
				if(_fh->bMaker==0xad)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=ReadS;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					if(bAddr6)
					{
						sql[slen]=0xAB;slen++;
						sql[slen]=ftap[5];slen++;
					}
				
					sql[slen]=0xC2;slen++;
					sql[slen]=ReadChangeS;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=ReadChangeE;slen++;	
				}
				else
				{
					if(bAddr6)
					{
						sql[slen]=0xC2;slen++;
						sql[slen]=ReadChangeS;slen++;
						sql[slen]=0xAB;slen++;
						sql[slen]=ftap[0];slen++;
						sql[slen]=0xAB;slen++;
						sql[slen]=ftap[1];slen++;
						sql[slen]=0xAB;slen++;
						sql[slen]=ftap[2];slen++;
						sql[slen]=0xAB;slen++;
						sql[slen]=ftap[3];slen++;
						sql[slen]=0xAB;slen++;
						sql[slen]=ftap[4];slen++;
						sql[slen]=0xAB;slen++;
						sql[slen]=ftap[5];slen++;
						sql[slen]=0xC2;slen++;
						sql[slen]=ReadChangeE;slen++;
					}
					else
					{	
						sql[slen]=0xC0;slen++;
						sql[slen]=ReadChangeS;slen++;
						sql[slen]=0xAA;slen++;
						sql[slen]=0xC1;slen++;
						sql[slen]=ReadChangeE;slen++;
						sql[slen]=0xCC;slen++;
					}
				}

				if((bConvs)&&(bReadEcc))
					mode=3;
				else
					mode=2;
				sql[slen]=0xDA;slen++;	
				if(mode==3)
				{
					sql[slen]=len;slen++;
				}
				else
				{
					sql[slen]=len/1024;slen++;
				}
				if((block%planemode)!=(planemode-1))// final Plane return to normal mode
				{
					if((bSLCMode)&&(_BootMode))
					{
						sql[slen]=0xC2;slen++;
						sql[slen]=0xFF;slen++;	// reset FW SLC mode to normal mode
						sql[slen]=0xCB;slen++;
					}
				}
			}		
		}

		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, maxpage);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len); 
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);
		if(ret)
		{
	        return ret;
	    }	
		if(bWaitRDY==0)
		{
			// check status
			int timeout=0;
			do
			{
				GetStatus(ce, ftap, 0x70);	// 750us one time
				if(timeout>=4000)	// Max 3sec timeout
				{
					break;
				}
				timeout++;
			}
			while((ftap[0]&0x60)!=chkStatus);
		}
	}

	if(bReadData&&(ReadE!=0x30))
	{
		if((_fh->beD3)&&(!bSLCMode))
		{
			page/=cell;// change to WL mode
		}
		if((bConvs)&&(bReadEcc))
			mode=3;
		else
			mode=2;
		slen=3;
		if(planemode>=2)
		{
			if((block%planemode)!=(planemode-1))
			{
				ReadS=ReadCmd2PS1;
			}
			else
			{
				ReadS=ReadCmd2PS2;
			}
		}
		else
		{
			ReadS=ReadCmdS;
		}
		SetFTA(ftap, page, block, maxpage);
		if(_fh->bMaker==0xad)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=ReadS;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[2];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[3];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[4];slen++;
			if(bAddr6)
			{
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[5];slen++;
			}
		
			sql[slen]=0xC2;slen++;
			sql[slen]=ReadChangeS;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=ReadChangeE;slen++;	
		}
		else
		{
			if(bAddr6)
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=ReadChangeS;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[0];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[1];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[5];slen++;
				sql[slen]=0xC2;slen++;
				sql[slen]=ReadChangeE;slen++;
			}
			else
			{			
				sql[slen]=0xC0;slen++;
				sql[slen]=ReadChangeS;slen++;
				sql[slen]=0xA0;slen++;
				sql[slen]=0xC1;slen++;
				sql[slen]=ReadChangeE;slen++;
				sql[slen]=0xCC;slen++;
			}
		}
		
		sql[slen]=0xDA;slen++;
		if(mode==3)
		{
			sql[slen]=len;slen++;
		}
		else
		{
			sql[slen]=len/1024;slen++;
		}
		if((bSLCMode)&&(_BootMode))
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xFF;slen++;	// reset FW SLC mode to normal mode
			if(bWaitRDY==1)
			{
				sql[slen]=0xCB;slen++;
			}
		}
		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, maxpage);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);   
		if(ret)
		{
	        return ret;
	    }
	}
	return 0;
}

int SQLCmd::ReturnFlashStatus(unsigned char* buf)
{
	memcpy(buf,_status,_fh->FlhNumber);
	return 0;
}

////////////////////////////Toshiba////////////////////////
TSBSQLCmd::TSBSQLCmd()
{
	// Parsing flash ID
	_CHK_VENDOR_MODE = 0;
}

TSBSQLCmd::~TSBSQLCmd()
{
}

/*
int TSBSQLCmd::SetSQLAddr(unsigned char *sql,int page, int logicalblock, int maxpage)
{
	SetFTA( sql,page,logicalblock,maxpage );
}
*/

int TSBSQLCmd::GetFlashParam( unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die )
{
	// Get flash special Parameter
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char  buf[512];

	int offsetCnt = _GetParameterIdxCnt(_fh);
	assert(offset<(unsigned int)offsetCnt);
	if(_fh->Design==0x32)  
	{
	/*
	    unsigned char sqlTSB32NM[] = 
		{
	        // 0,   1,   2,
	        0xCA,0x2C,0x01,
	        // 3,   4,   5,
	        0xC2,0xFF,0xCB,
	        // 6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
	        0xC2,0xC3,0xC2,0x28,0xC2,0xE7,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,
	        //20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
	        0xC2,0x5A,0xC2,0xE5,0xC2,0x55,0xAB,0x01,0xDE,0x02,
	        //30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,
	        0xC2,0xAA,0xCB,0xC2,0x05,0xAB,0x00,0xAB,0x00,0xC2,0xE0,0xDA,0x08,
	        //43,  44,  45,  46,
	        0xC2,0xFF,0xCB,0xCE,
	        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	    };
	    unsigned char sqlTSB32D2_98D794327655[] =
		{
	        // 0,   1,   2,
	        0xCA,0x2C,0x01,
	        // 3,   4,   5,
	        0xC2,0xFF,0xCB,
	        // 6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
	        0xC2,0x14,0xC2,0xF7,0xC2,0x68,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,
	        //20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
	        0xC2,0x5A,0xC2,0xE5,0xC2,0x55,0xAB,0x01,0xDE,0x02,
	        //30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,
	        0xC2,0xAA,0xCB,0xC2,0x05,0xAB,0x00,0xAB,0x00,0xC2,0xE0,0xDA,0x08,
	        //43,  44,  45,  46,
	        0xC2,0xFF,0xCB,0xCE,
	        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	    };
	    unsigned char* sql = 0;
	    int slen = 0;
	    int ret = 0;
		unsigned char TSB32D2[] = {0xD7,0x94,0x32,0x76,0x55};
	    if ( memcmp(_fh->id,TSB32D2,sizeof(TSB32D2))==0 )
	    {
	        sql = sqlTSB32D2_98D794327655;
	        slen = sizeof(sqlTSB32D2_98D794327655);
	        sql[43] = sizeof(_32nmParamBuf)/1024;
	    }
	    else 
		{
	        sql = sqlTSB32NM;
	        slen = sizeof(sqlTSB32NM);
	        sql[43] = sizeof(_32nmParamBuf)/1024;
	    }
	    assert(sql);

	    if ( _32nmParamModifyFlag ) {
	        _32nmParamModifyFlag = 0; //reset modify flag for loaded.
	        ret = Dev_IssueIOCmd(sql,slen, 2, ce, _32nmParamBuf, sizeof(_32nmParamBuf) );
	    }
	    pbuf[offset] = _32nmParamBuf[0x200+offset*2];
	    return ret;
	*/
	    assert(0);
	    return 0;
	}
	else 
	{
	    unsigned char sqlTSBMLC[] = 
		{
	        // 0,   1,   2,
	        0xCA,0x1A,0x01,
	        // 3,   4,  5,   6,   7,   8,
	        0xC2,0x55,0xAB,0x01,0xDE,0x06,
	        // 9,  10,  11,  12,  13,  14,
	        0xC2,0x5F,0xC2,0x00,0xAB,0x00,
	        //15,  16,  17,  18,  19,
	        0xC2,0x5F,0xCB,0xDB,0x01,
	        //20,  21,  22,  23,  24,  25,  26,  27,
	        0xC2,0x5F,0xC2,0x00,0xC2,0x5F,0xC2,0x5F,
	        //28,
	        0xCE,
	        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	    };
	    unsigned char sqlTSB1ZTLC[] = 
		{
	        // 0,   1,   2,
	        0xCA,0x1C,0x01,
	        // 3,   4,
	        0xC2,0xF0,
	        // 5,   6,   7,   8,   9,  10,
	        0xC2,0x55,0xAB,0x01,0xDE,0x06,
	        //11,  12,  13,  14,  15,  16,
	        0xC2,0x5F,0xC2,0x00,0xAB,0x27,
	        //17,  18,  19,  20,  21,
	        0xC2,0x5F,0xCB,0xDB,0x01,
	        //22,  23,  24,  25,  26,  27,  28,  29,
	        0xC2,0x5F,0xC2,0x00,0xC2,0x5F,0xC2,0x5F,
	        //30,
	        0xCE,
	        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	    };
	    unsigned char sqlTSB3DMLC[] = 
		{
	        // 0,   1,   2,
	        0xCA,0x16,0x01,
	        // 3,   4,  5,   6,   7,   8,
	        0xC2,0x55,0xAB,0x01,0xDE,0x06,
	        // 9,  10,  11,  12,  13,  14,
	        0xC2,0x55,0xAB,0xFF,0xDE,0x00,
	        //15,  16,  17,  18,  19,  20,  21,  22,  23,
	        0xC2,0x00,0xAB,0x27,0xC2,0x5F,0xCB,0xDB,0x01,
	        //24,
	        0xCE,
	        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	    };

	    unsigned char* sql = 0;
	    int slen = 0;
	    if ( offsetCnt>256 ) 
		{
	        sql = sqlTSB3DMLC;
	        slen = sizeof(sqlTSB3DMLC);
	        //sql[4] = (0xF0|die)+1;
	        sql[14] = ((offset>>8)&0xFF);
	        sql[18] = (offset&0xFF);
	    }
	    else
	    {
	        if (_fh->beD3 && _fh->Design==0x15)  
			{
	            sql = sqlTSB1ZTLC;
	            slen = sizeof(sqlTSB1ZTLC);
	            sql[4] = (0xF0|die)+1;
	            sql[16] = offset;
	        }
	        else 
			{
	            slen = sizeof(sqlTSBMLC);
	           sql = sqlTSBMLC;
	            sql[14] = offset;
	        }
	    }

	    int ret = Dev_IssueIOCmd( sql, slen, 2, ce, buf, 512 );
	    value[offset] = buf[0];
	    return ret;
	}
}

int TSBSQLCmd::SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value, unsigned char die )
{
    // Set flash special Parameter
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
    int slen;
    int ret = 1;
    int offsetCnt = _GetParameterIdxCnt(_fh);
    assert(offset<(unsigned int)offsetCnt);

    if (_fh->Design==0x32)  
	{/*
        unsigned char sqlTSB32NM[] = 
		{
            // 0,   1,   2,
            0xCA,0x1E,0x01,
            // 3,   4,   5,
            0xC2,0xFF,0xCB,
            // 6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
            0xC2,0xC3,0xC2,0x28,0xC2,0xE7,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,
            //20,  21,  22,  23,  24,  25,
            0xC2,0x55,0xAB,0x00,0xDE,0x01,
            //26,  27,  28,  29,  30,  31,  32,
            0xC2,0xAA,0xCB,0xC2,0xFF,0xCB,0xCE,
            0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
        };
        unsigned char sqlTSB32D2_98D794327655[] = 
		{
            // 0,   1,   2,
            0xCA,0x1E,0x01,
            // 3,   4,   5,
            0xC2,0xFF,0xCB,
            // 6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
            0xC2,0x14,0xC2,0xF7,0xC2,0x68,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,
            //20,  21,  22,  23,  24,  25,
            0xC2,0x55,0xAB,0x00,0xDE,0x01,
            //26,  27,  28,  29,  30,  31,  32,
            0xC2,0xAA,0xCB,0xC2,0xFF,0xCB,0xCE,
            0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
        };

        unsigned char* sql = 0;
        unsigned char TSB32D2[] = {0xD7,0x94,0x32,0x76,0x55};
    	if ( memcmp(_fh->id,TSB32D2,sizeof(TSB32D2))==0 )
    	{
            sql = sqlTSB32D2_98D794327655;
            slen = sizeof(sqlTSB32D2_98D794327655);
            sql[25] = value;
            sql[23] = offset;
        }
        else 
		{
            sql = sqlTSB32NM;
            slen = sizeof(sqlTSB32NM);
            sql[25] = value;
            sql[23] = offset;
        }
        assert(sql);

        ret = Dev_IssueIOCmd( sql, slen, 0, ce, 0, 0 );
        _32nmParamModifyFlag = 1;
        */
    }
    else 
	{
        unsigned char sqlTSB1ZTLC[] = 
		{
            // 0,   1,   2,
            0xCA,0x09,0x01,
            // 3,   4,   5,   6,   7,   8,   9,  10,  11,
            0xC2,0xF1,0xC2,0x56,0xAB,0x27,0xDE,0x50,0xCE,
            0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
        };

        unsigned char sqlTSB3DMLC[] = 
		{
            // 0,   1,   2,
            0xCA,0x0D,0x01,
            // 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,
            0xC2,0x55,0xAB,0xFF,0xDE,0x00,0xC2,0x55,0xAB,0x27,0xDE,0x00,0xCE,
            0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
        };

        unsigned char* sql = 0;

        if ( offsetCnt>256 ) {
            sql = sqlTSB3DMLC;
            slen = sizeof(sqlTSB3DMLC);
            sql[8] = ((offset>>8)&0xFF);
            sql[12] = (offset&0xFF);
            sql[14] = value;
        }
        else
        {
            sql = sqlTSB1ZTLC;
            slen = sizeof(sqlTSB1ZTLC);
            sql[10] = value;
            sql[8] = offset;
            sql[4] = (0xF0|die)+1;
        }
       ret = Dev_IssueIOCmd(sql,slen, 0, ce, 0, 0 );
    }
    return ret;
}

int TSBSQLCmd::Erase(unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6)
{
	unsigned char sql[256], slen=0, temp, i, ftap[6];
	int CE, ret = 0;

	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Erase(ce, block, bSLCMode, planemode, bDFmode, bWaitRDY, bAddr6);
	}
	for( CE = 0; CE < _fh->FlhNumber; CE++ )
	{
		slen=0;
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(_fh->FlhNumber==1)
			{
				CE = 0;
			}
			else
			{
				CE = ce;
			}
		}
		//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
		sql[slen]=0xCA;slen++;	// Stare from 0xCA
		sql[slen]=0x00;slen++;	// temp set Command len = 0
		temp=0x01;				// default used one addrress mode

		sql[slen]=temp;slen++;	// Operation setting

		if(bDFmode)
		{
			if(_fh->Design<=0x15)
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xC6;slen++;
				
			}
			else
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xDF;slen++;
			}
		}
		if(bSLCMode)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xA2;slen++;
		}
		// not sure can work
		/*
		if(planemode>=2)
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC0;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xA3+((i>0)?0x0A:0x00);slen++;	// 0xAD block addr ++
				sql[slen]=0xCC;slen++;
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=0xD0;slen++;
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}*/	
		/*
		if(planemode==2)
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC0;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xA0;slen++;
				sql[slen]=0xCC;slen++;
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=0xD0;slen++;
		}
		else if(planemode==4)
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC2;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=0xD0;slen++;
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}
		*/
		if((planemode>=2)||(bAddr6))
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC2;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
				if(bAddr6)
				{
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
				}
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=0xD0;slen++;
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}
		
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(bWaitRDY==1)
			{
				sql[slen]=0xCB;slen++;
			}
		}
		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], 0, block, 0);
		slen+=6;
		/*
		if(planemode==2)
		{
			SetSQLAddr(&sql[slen],0, block+1, 0);
			slen+=6;
		}*/
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //int ret=_scsi->Dev_IssueIOCmd(sql, slen, 0, ce, 0, 0);
	    ret=Dev_IssueIOCmd(sql, slen, 0, CE, 0, 0);
		if ( !ret )
		{
			if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
			{
				if(bWaitRDY==0)
				{
					// check status
					int timeout=0;
					do
					{
						GetStatus(CE, ftap, 0x70);	// 750us one time
						if(timeout>=4000)	// Max 3sec timeout
						{
							break;
						}
						timeout++;
					}
					while((ftap[0]&0xE0)!=0xE0);
				}
			}
		}
		else
		{
			break;
		}
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			break;
		}
	}
	if((ce==_fh->FlhNumber)&&(_fh->FlhNumber!=1))
	{
		for( CE = 0; CE < _fh->FlhNumber; CE++ )
		{
			if(bWaitRDY==0)
			{
				// check status
				int timeout=0;
				do
				{
					GetStatus(CE, ftap, 0x70);	// 750us one time
					if(timeout>=4000)	// Max 3sec timeout
					{
						break;
					}
					timeout++;
				}
				while((ftap[0]&0xE0)!=0xE0);
			}
			else
			{
				OnlyWiatRDY(CE);
			}
		}
	}
    return ret;
}

int TSBSQLCmd::Write(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode, unsigned char ProgStep, bool bCache, bool bLastPage, char bWaitRDY, bool bAddr6)
{
	unsigned char sql[256],slen=0,temp,ftap[6],chkStatus=0x40;
	unsigned char cell=LogTwo(_fh->Cell);
	unsigned char p_cnt = (ProgStep>>4);
	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Write(ce, block, page, bConvs, bSLCMode, buf, len, planemode, ProgStep, bCache, bLastPage, bWaitRDY, bAddr6);
	}
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	ProgStep &=0x0F;
	
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x81;				// default used write mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting

	if(bSLCMode)
	{
		sql[slen]=0xC2;slen++;
		sql[slen]=0xA2;slen++;
	}
	else
	{
		if(_fh->beD3)
		{
			if(ProgStep)// 19nm/24nm programming step
			{
				sql[slen]=0xC2;slen++;
				if(ProgStep==1)
				{
					sql[slen]=0x09;slen++;
				}
				else
				{
					sql[slen]=0x0D;slen++;// if QLC set ProgSetp=2 for 0x0D command send
				}
			}
			sql[slen]=0xC2;slen++;
			sql[slen]=p_cnt+1;slen++;
		}
	}
	if(bAddr6)
	{
		SetFTA(ftap, page, block);
		sql[slen]=0xC2;slen++;
		if((!_fh->beD3)&&((block%planemode)!=0))
		{
			sql[slen]=0x81;slen++;
		}
		else
		{
			sql[slen]=0x80;slen++;
		}
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[0];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[1];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[2];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[3];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[4];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[5];slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}
	else
	{			
		sql[slen]=0xC0;slen++;
		if((!_fh->beD3)&&((block%planemode)!=0))
		{
			sql[slen]=0x81;slen++;
		}
		else
		{
			sql[slen]=0x80;slen++;
		}
		sql[slen]=0xA0;slen++;
		sql[slen]=0xCC;slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}		
	sql[slen]=0xC2;slen++;
	chkStatus=0x40;
	if((block%planemode)!=(planemode-1))
	{
		sql[slen]=0x11;slen++;
	}	
	else if(_fh->beD3&&(p_cnt!=(cell-1))&&(!bSLCMode))
	{
		sql[slen]=0x1A;slen++;
	}
	else if(bCache&&(!bLastPage))
	{
		sql[slen]=0x15;slen++;
		chkStatus=0x20;
	}
	else
	{
		sql[slen]=0x10;slen++;
	}

	if(bWaitRDY==1)
	{
		sql[slen]=0xCB;slen++;
	}
	sql[slen]=0xCE;slen++;			// command End
	sql[1]=slen-3;
	sql[slen]=0xA0;slen++;

	SetFTA(&sql[slen], page, block, 0);
	slen+=6;
	
	sql[slen]=0xAE;slen++;
	sql[slen]=0xAF;slen++;

	//int ret=_scsi->Dev_IssueIOCmd(sql, slen, 1, ce, buf, len);
	int ret=Dev_IssueIOCmd(sql, slen, 1, ce, (unsigned char*)buf, len);  
	if ( ret ) 
	{
		return ret;
	}
	if(bWaitRDY==0)
	{
		// check status
		int timeout=0;
		do
		{
			GetStatus(ce, ftap, 0x70);	// 750us one time
			if(timeout>=4000)	// Max 3sec timeout
			{
				break;
			}
			timeout++;
		}
		while((ftap[0]&0x60)!=chkStatus);
	}
	return 0;
}

int TSBSQLCmd::Read(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode, bool bReadCmd, bool bReadData, bool bReadEcc, bool bCache, bool bFirstPage, bool bLastPage, bool bCmd36, char bWaitRDY, bool bAddr6)
{
	int ret;
	unsigned char sql[256],slen=0,temp,ftap[6],mode=0,ReadE=0x00,chkStatus;// mode == 2 is read data
	unsigned char cell=LogTwo(_fh->Cell);
	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Read(ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
	}
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x01;				// default used read mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting
	if(bReadCmd)
	{
		chkStatus=0x40;
		if(bCmd36)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0x36;slen++;
		}
		if(bSLCMode)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xA2;slen++;
		}
		else if(_fh->beD3)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=(page%cell)+1;slen++;
			page/=cell;// change to WL mode
		}

		if((!_fh->beD3)&&bCache&&(planemode==1)&&(!bFirstPage))	// Sequential cache read
		{
			if(bLastPage)
			{
				ReadE=0x3F;
			}
			else
			{
				ReadE=0x31;
				chkStatus=0x20;
			}
			sql[slen]=0xC2;slen++;
			sql[slen]=ReadE;slen++;
		}
		else
		{
			if(bCache&&(bLastPage))
			{
				ReadE=0x3F;
			}
			else if((block%planemode)!=(planemode-1))
			{
				ReadE=0x32;
			}
			else if(bCache&&(!bFirstPage))
			{
				ReadE=0x31;
				chkStatus=0x20;
			}
			else
			{
				ReadE=0x30;
			}
			if(bAddr6)
			{
				if(ReadE!=0x3F)
				{
					SetFTA(ftap, page, block);
					sql[slen]=0xC2;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
				}
				sql[slen]=0xC2;slen++;
				sql[slen]=ReadE;slen++;
			}
			else
			{	
				if(ReadE!=0x3F)
				{
					SetFTA(ftap, page, block);
					sql[slen]=0xC0;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xA0;slen++;
					sql[slen]=0xC1;slen++;
					sql[slen]=ReadE;slen++;
					sql[slen]=0xCC;slen++;
				}
				else
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=ReadE;slen++;
				}
			}			
		}				

		if(bWaitRDY==1)
		{
			sql[slen]=0xCB;slen++;		
			if((bReadData)&&(ReadE==0x30))
			{
				SetFTA(ftap, page, block);
				if(bAddr6)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0x05;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0xE0;slen++;
				}
				else
				{	
					sql[slen]=0xC0;slen++;
					sql[slen]=0x05;slen++;
					sql[slen]=0xAA;slen++;
					sql[slen]=0xC1;slen++;
					sql[slen]=0xE0;slen++;
					sql[slen]=0xCC;slen++;
				}
				if((bConvs)&&(bReadEcc))
					mode=3;
				else
					mode=2;
				sql[slen]=0xDA;slen++;	
				if(mode==3)
				{
					sql[slen]=len;slen++;
				}
				else
				{
					sql[slen]=len/1024;slen++;
				}
			}
		}

		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, 0);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len); 
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);
		if(ret)
		{
	        return ret;
	    }	
		if(bWaitRDY==0)
		{
			// check status
			int timeout=0;
			do
			{
				GetStatus(ce, ftap, 0x70);	// 750us one time
				if(timeout>=4000)	// Max 3sec timeout
				{
					break;
				}
				timeout++;
			}
			while((ftap[0]&0x60)!=chkStatus);
		}
	}

	if(bReadData&&(ReadE!=0x30))
	{
		if((_fh->beD3)&&(!bSLCMode))
		{
			page/=cell;// change to WL mode
		}
		if((bConvs)&&(bReadEcc))
			mode=3;
		else
			mode=2;
		slen=3;
		SetFTA(ftap, page, block);
		if(bAddr6)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0x05;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[2];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[3];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[4];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[5];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0xE0;slen++;
		}
		else
		{			
			sql[slen]=0xC0;slen++;
			sql[slen]=0x05;slen++;
			sql[slen]=0xA0;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xE0;slen++;
			sql[slen]=0xCC;slen++;
		}
		sql[slen]=0xDA;slen++;
		if(mode==3)
		{
			sql[slen]=len;slen++;
		}
		else
		{
			sql[slen]=len/1024;slen++;
		}

		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, 0);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);   
		if(ret)
		{
	        return ret;
	    }
	}
	return 0;
}

int TSBSQLCmd::Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd, bool pMLC, bool bSLC)
{
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char TSB_SQLCNT1_toggle[] = //For debug
	{ 
        // 0,   1,   2,
		0xCA,0x3C,0x01,
		// 3    4,
		0xCF,0x00,
        // 5,   6,   7,
		0xC2,0xFF,0xCB,
        // 8,   9,  10,  11,
		0xC2,0x5C,0xC2,0xC5, 
        //12,  13,  14,  15,  16,  17,
		0xC2,0x55,0xAB,0x00,0xDE,0x01,
        //18,  19,  20,  21,  22,  23,
		0xC2,0x55,0xAB,0x04,0xDE,0x00,
        //24,  25,  26,  27,  28,  29,
		0xC2,0x55,0xAB,0x05,0xDE,0x00,
        //30,  31,  32,  33,  34,  35,
		0xC2,0x55,0xAB,0x06,0xDE,0x01,
        //36,  37,         
		0xCF,0x01,
        //38,  39,  
		0xC2,0xBC,
        //40,  41,  42,  43,  44,  45,  46,      
		0xC0,0x00,0xA0,0xC1,0xAE,0xCC,0xCB,
        //47,  48,  49, 
		0xC2,0x3D,0xCB,
        //50,  51,  52,  53,  54,  55,  56,  57,  58,
		0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xCB,0xDA,0x09,
        //59,  60,  61,  62,
		0xC2,0xFF,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_SQLCNT1[] =
	{
		// 0,   1,   2,
		0xCA,0x38,0x01,
		// 3,   4,   5,
		0xC2,0xFF,0xCB,
		//6,   7,   8,   9,
		0xC2,0x5C,0xC2,0xC5,
		//10,  11,  12,  13,  14,  15,
		0xC2,0x55,0xAB,0x00,0xDE,0x01,
		//16,  17,  18,  19,  20,  21,
		0xC2,0x55,0xAB,0x04,0xDE,0x00,
		//22,  23,  24,  25,  26,  27,
		0xC2,0x55,0xAB,0x05,0xDE,0x00,
		//28,  29,  30,  31,  32,  33,
		0xC2,0x55,0xAB,0x06,0xDE,0x01,
		//34,  35,
		0xC2,0xBC,
		//36,  37,  38,  39,  40,  41,  42,
		0xC0,0x00,0xA0,0xC1,0xAE,0xCC,0xCB,
		//43,  44,  45,
		0xC2,0x3D,0xCB,
		//46,  47,  48,  49,  50,  51,  52,  53,  54,
		0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xCB,0xDA,0x09,
		//55,  56,  57,  58,
		0xC2,0xFF,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_SQLCNT1PCMD[] = 
	{
		// 0,   1,   2, 
		0xCA,0x3A,0x01,
        // 3,   4,   5,
		0xC2,0xFF,0xCB,
        // 6,   7,   8,   9,
		0xC2,0x5C,0xC2,0xC5, 
        //10,  11,  12,  13,  14,  15,
		0xC2,0x55,0xAB,0x00,0xDE,0x01,
        //16,  17,  18,  19,  20,  21,
		0xC2,0x55,0xAB,0x04,0xDE,0x00,
        //22,  23,  24,  25,  26,  27,
		0xC2,0x55,0xAB,0x05,0xDE,0x00,
        //28,  29,  30,  31,  32,  33,
		0xC2,0x55,0xAB,0x06,0xDE,0x01,
        //34,  35,  36,  37,
		0xC2,0x26,0xC2,0xBC,
        //38,  39,  40,  41,  42,  43,  44,
		0xC0,0x00,0xA0,0xC1,0xAE,0xCC,0xCB,
        //45,  46,  47,
		0xC2,0x3D,0xCB,
        //48,  49,  50,  51,  52,  53,  54,  55,  56,
		0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xCB,0xDA,0x09,
        //57,  58,  59,  60,
		0xC2,0xFF,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_SQLCNT1_3D_DD[] = 
	{
		// 0,   1,   2,
		0xCA,0x27,0x01,
		// 3,   4,   5,   6,   7,   8,
		0xC2,0x55,0xAB,0x04,0xDE,0x00,
		// 9,  10,  11,  12,  13,  14,
		0xC2,0x55,0xAB,0x05,0xDE,0x00,
		//15,  16,  17,  18,  19,  20,
		0xC2,0x55,0xAB,0x01,0xDE,0x01,
		//21,  22,  23,  24,
		0xC2,0xDD,0xC2,0xD8,
		//25,  26,  27,  28,  29,  30,  31,
		0xC0,0x00,0xA0,0xC1,0xAE,0xCC,0xCB,
		//32,  33,  34,  35,  36,  37,  38,  39,  40,  41,
		0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xCB,0xDA,0x09,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_SQLCNT1_3D_DD2[] = 
	{
		// 0,   1,   2,
		0xCA,0x25,0x01,
		// 3,   4,   5,   6,   7,   8,
		0xC2,0x55,0xAB,0x04,0xDE,0x00,
		// 9,  10,  11,  12,  13,  14,
		0xC2,0x55,0xAB,0x05,0xDE,0x00,
		//15,  16,  17,  18,  19,  20,
		0xC2,0x55,0xAB,0x01,0xDE,0x01,
		//21,  22,
		0xC2,0xD8,
		//23,  24,  25,  26,  27,  28,  29,
		0xC0,0x00,0xA0,0xC1,0xAE,0xCC,0xCB,
		//30,  31,  32,  33,  34,  35,  36,  37,  38,  39,
		0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xCB,0xDA,0x09,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_SQLCNT1_3D_DD3[] = 
	{
		// 0,   1,   2,
		0xCA,0x21,0x01,
		// 3,   4,   5,   6,   7,   8,
		0xC2,0x55,0xAB,0x04,0xDE,0x00,
		// 9,  10,  11,  12,  13,  14,
		0xC2,0x55,0xAB,0x05,0xDE,0x00,
		//15,  16,  17,  18,
		0xC2,0xDD,0xC2,0xD8,
		//19,  20,  21,  22,  23,  24,  25,
		0xC0,0x00,0xA0,0xC1,0xAE,0xCC,0xCB,
		//26,  27,  28,  29,  30,  31,  32,  33,  34,  35,
		0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xCB,0xDA,0x09,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_SQLCNT1D2[] = 
	{
		// 0,   1,   2,
		0xCA,0x2B,0x01,
		// 3,   4,   5,
		0xC2,0xFF,0xCB,
		//6,   7,   8,   9,
		0xC2,0x5C,0xC2,0xC5,
		//10,  11,  12,  13,  14,  15,
		0xC2,0x55,0xAB,0x00,0xDE,0x01,
		//16,  17,  18,  19,  20,  21,
		0xC2,0x55,0xAB,0x04,0xDE,0x00,
		//22,  23,
		0xC2,0xBC,
		//24,  25,  26,  27,  28,  29,  30,
		0xC0,0x00,0xA0,0xC1,0xAE,0xCC,0xCB,
		//31,  31,  33,
		0xC2,0x3D,0xCB,
		//34,  35,  36,  37,  38,  39,  40,  41,
		0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xDA,0x09,
		//42,  43,  44,  45,
		0xC2,0xFF,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_SQLCNT1D2PRECMD[] = 
	{
        // 0,   1,   2,
		0xCA,0x2D,0x01,
        // 3,   4,   5,
		0xC2,0xFF,0xCB,
        // 6,   7,   8,   9, 
		0xC2,0x5C,0xC2,0xC5, 
        //10,  11,  12,  13,  14,  15,
		0xC2,0x55,0xAB,0x00,0xDE,0x01,
        //16,  17,  18,  19,  20,  21,
		0xC2,0x55,0xAB,0x04,0xDE,0x00,
        //22,  23,  24,  25,
		0xC2,0x26,0xC2,0xBC,
        //26,  27,  18,  19,  20,  21,  22,
		0xC0,0x00,0xA0,0xC1,0xAE,0xCC,0xCB,
        //23,  24,  25,
		0xC2,0x3D,0xCB,
        //26,  27,  28,  29,  30,  31,  32,  33,
		0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xDA,0x09,
        //34,  35,  36,  37,
		0xC2,0xFF,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};

	int slen = 0;
	unsigned char *sql = 0;

	if( !pMLC ) 
	{
		if ( !ddmode ) 
		{
			if ( precmd == 0xFF ) 
			{
				sql = TSB_SQLCNT1;
				slen = sizeof(TSB_SQLCNT1);
				sql[21] = (level&0xFF);
				sql[27] = ((level>>8)&0xFF);
				sql[54] = len/1024;
				if ( _fh->bIs3D ) 
				{
					sql[19] = 0x03;
					sql[25] = 0x04;
					sql[31] = 0x05;
				}
			}
			else 
			{
				sql = TSB_SQLCNT1PCMD;
				slen = sizeof(TSB_SQLCNT1PCMD);
				sql[21] = (level&0xFF);
				sql[27] = ((level>>8)&0xFF);
				sql[35] = precmd;
				sql[56] = len/1024;
				if( _fh->bIs3D ) 
				{
					sql[19] = 0x03;
					sql[25] = 0x04;
					sql[31] = 0x05;
				}           
			}
		}
		else 
		{
			if( ddmode==1 ) 
			{
				sql = TSB_SQLCNT1_3D_DD;
				slen = sizeof(TSB_SQLCNT1_3D_DD);
				sql[8] = (level&0xFF);
				sql[14] = ((level>>8)&0xFF);
				sql[40] = len/1024;
			}
			else if( ddmode==2 ) 
			{
				sql = TSB_SQLCNT1_3D_DD2;
				slen = sizeof(TSB_SQLCNT1_3D_DD2);
				sql[8] = (level&0xFF);
				sql[14] = ((level>>8)&0xFF);
				sql[38] = len/1024;
			}
			else if ( ddmode==3 ) 
			{
				sql = TSB_SQLCNT1_3D_DD3;
				slen = sizeof(TSB_SQLCNT1_3D_DD3);
				sql[8] = (level&0xFF);
				sql[14] = ((level>>8)&0xFF);
				sql[34] = len/1024;
			}
			if ( _fh->bIs3D ) 
			{
				sql[6] = 0x03;
				sql[12] = 0x04;
			}
		}
	}
	else 
	{
		if( precmd == 0xFF )
		{
			sql = TSB_SQLCNT1D2;
			slen = sizeof(TSB_SQLCNT1D2);
			sql[21] = (level&0xFF);
			sql[41] = len/1024;
		}
		else
		{
			sql = TSB_SQLCNT1D2PRECMD;
			slen = sizeof(TSB_SQLCNT1D2PRECMD);
			sql[21] = (level&0xFF);
			sql[23] = precmd;
			sql[43] = len/1024;
		}
	}
            
	if ( ddmode ) 
	{
		//if (!tsb3DNand ) {
		//	assert(0);
		//}
		SetVendorMode(ce);
	}
	SetFTA( &sql[slen-8], Page, block );
	int ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf, len );

	if ( ddmode ) 
	{
		//NewSort_CLE_SEND ( ce, 0xFF );
		//_CHK_VENDOR_MODE = 0;
		CloseVendorMode(ce);
	}
	return ret;
}

int TSBSQLCmd::NewSort_SetVendorMode2 ( unsigned char ce )
{
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char TSB_SQL3DTLC[] = {
		// 0,   1,   2,
		0xCA,0x10,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18
		0xC2,0xFF,0xCB,0xC2,0x9D,0xC2,0xE7,0xC2,0x46,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};

	unsigned char TSB_Entry1ZTLC[] = {
		// 0,   1,   2,
		0xCA,0x10,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
		0xC2,0x19,0xC2,0x96,0xC2,0x17,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_Entry1ZMLC[] = {
		// 0,   1,   2,
		0xCA,0x10,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
		0xC2,0x19,0xC2,0x96,0xC2,0x17,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};

	unsigned char TSB_Entry1YTLC[] = {
		// 0,   1,   2,
		0xCA,0x10,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
		0xC2,0x6C,0xC2,0x49,0xC2,0xDB,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_Entry1YMLC[] = {
		// 0,   1,   2,
		0xCA,0x10,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
		0xC2,0x6C,0xC2,0x49,0xC2,0xDB,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};

	unsigned char TSB_Entry19TLC[] = {
		// 0,   1,   2,
		0xCA,0x10,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
		0xC2,0xC3,0xC2,0x28,0xC2,0xE7,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_Entry19MLC[] = {
		// 0,   1,   2,
		0xCA,0x10,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
		0xC2,0xC3,0xC2,0x28,0xC2,0xE7,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x9E,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};

	int entryLen = 0;
	unsigned char* TSB_Entry =0;
	unsigned char tsb3D = ((_fh->bIs3D)&&(_fh->beD3));

	if ( tsb3D ) {
		if ( _fh->beD3 ) {
			TSB_Entry = TSB_SQL3DTLC;
			entryLen = sizeof(TSB_SQL3DTLC);
		}
		else {
			assert(0);
		}
	}
	else if ( _fh->Design==0x15 ) {
		if ( _fh->beD3 ) {
			TSB_Entry = TSB_Entry1ZTLC;
			entryLen = sizeof(TSB_Entry1ZTLC);
		}
		else {
			TSB_Entry = TSB_Entry1ZMLC;
			entryLen = sizeof(TSB_Entry1ZMLC);
		}
	}
	else if ( _fh->Design==0x16 ) {
		if ( _fh->beD3 ) {
			TSB_Entry = TSB_Entry1YTLC;
			entryLen = sizeof(TSB_Entry1YTLC);
		}
		else {
			TSB_Entry = TSB_Entry1YMLC;
			entryLen = sizeof(TSB_Entry1YMLC);
		}
	}
	else if ( _fh->Design==0x19 ) {
		if ( _fh->beD3 ) {
			TSB_Entry = TSB_Entry19TLC;
			entryLen = sizeof(TSB_Entry19TLC);
		}
		else {
			TSB_Entry = TSB_Entry19MLC;
			entryLen = sizeof(TSB_Entry19MLC);
		}
	}
	assert(TSB_Entry);
	return Dev_IssueIOCmd( TSB_Entry,entryLen, 0, ce, 0, 0 );
}

int TSBSQLCmd::CloseVendorMode ( unsigned char ce )
{
    assert( _CHK_VENDOR_MODE==1 );
    _CHK_VENDOR_MODE = 0;
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	int offsetCnt = _GetParameterIdxCnt(_fh);

	if ( offsetCnt>256 ) 
	{
		unsigned char TSB_3DSQL[] = 
		{
			// 0,   1,   2,
			0xCA,0x0A,0x01,
			// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,
			0xC2,0x55,0xAB,0xFF,0xDE,0x00,0xC2,0xFF,0xCB,0xCE,
            0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
		};
        int slen = sizeof(TSB_3DSQL);
        int ret =  Dev_IssueIOCmd(TSB_3DSQL,slen, 0, ce, 0, 0 );
		return ret;
	}
	else
	{
		unsigned char TSB_SQL[] = 
		{
			// 0,   1,   2,
			0xCA,0x04,0x01,
			// 3,   4,   5,   6,
			0xC2,0xFF,0xCB,0xCE,
  			0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
		};
		int slen = sizeof(TSB_SQL);
		int ret =  Dev_IssueIOCmd( TSB_SQL, slen, 0, ce, 0, 0 );
		return ret;
	}
}

int TSBSQLCmd::SetVendorMode ( unsigned char ce )
{
    assert( _CHK_VENDOR_MODE==0 );
    _CHK_VENDOR_MODE = 1;

    unsigned char TSB_SQL1ZTLC[] = 
	{
        // 0,   1,   2,
        0xCA,0x0B,0x01,
        // 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
        0xC2,0x5C,0xC2,0xC5,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xCE,
        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
    };
    unsigned char* sql = 0;
    int slen = 0;
	sql = TSB_SQL1ZTLC;
	slen = sizeof(TSB_SQL1ZTLC);
	int ret =  Dev_IssueIOCmd( sql, slen, 0, ce, 0, 0 );
    return ret;
}

int TSBSQLCmd::GetProgramLoop ( int ce, int* loop )
{
    unsigned char buf[512];
    int ret;
    unsigned char sql[] = {
        // 0,   1,   2,
        0xCA,0x18,0x01,
        // 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
        //0xC2,0x56,0xAB,0x01,0xDE,0x00,0xC2,0x5F,0xCB,0xDB,0x01,0xCE,
        // 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,
        0xC2,0x5C,0xC2,0xC5,0xC2,0x55,0xAB,0x00,0xDE,0x01,0xC2,0x00,0xC2,0x56,0xAB,0x01,0xDE,0x00,0xC2,0x5F,0xCB,0xDB,0x01,0xCE,
        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
    };
    int slen = sizeof(sql);
    ret = Dev_IssueIOCmd(   sql,slen, 2, ce, buf, 512 );
    *loop = buf[0];
    return ret;   
}

//ibuf = 16 bytes
int TSBSQLCmd::GetFlashID(unsigned char ce, unsigned char* ibuf )
{
    unsigned char buf[512];
    int ret;
    unsigned char sql[] = {
        // 0,   1,   2,
        0xCA,0x07,0x01,
        // 3,   4,   5,   6,   7,   8,  9,
        0xC2,0x90,0xAB,0x00,0xDB,0x16,0xCE,
        0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
    };
    int slen = sizeof(sql);
    ret = Dev_IssueIOCmd(sql,slen, 2, ce, buf, 512 );
    memcpy(ibuf,buf,16);
    return ret;
}

/*
int TSBSQLCmd::TestFlashParam( IntMemMgr* skips,IntMemMgr* dumps )
{
    const int bufLen = GetParameterIdxCnt();

    CharMemMgr buf ( bufLen );
    unsigned char* pbuf = buf.Data();

    CharMemMgr bufKeep ( bufLen );
    unsigned char* pbufKeep = bufKeep.Data();

    CharMemMgr flashIDBuf ( 528 );
    unsigned char* pFlashIDBuf = flashIDBuf.Data();
    unsigned char flashID[7];
    if ( cmd->USB_GetFlashID ( pFlashIDBuf ,0,0 ) ) {
        cmd->Log ( MyAppUtil::LOGDEBUG,"### Err: FlashGetID fail.\n" );
        return 1;
    }
    memcpy(flashID,pFlashIDBuf,sizeof(flashID));
    cmd->Log ( MyAppUtil::LOGDEBUG,"Flash ID [%02X %02X %02X %02X %02X %02X %02X]\n",
               flashID[0],flashID[1],flashID[2],flashID[3],flashID[4],flashID[5],flashID[6]);


    CharMemMgr bufSave ( bufLen,2 ); //value,type(0=valid/1=skip/2=readonly/3=flashid)
    cmd->Dev_SetFlashClock ( _flashTiming );
   	NewSort_SetVendorMode ( _cmd_CE );

    for ( int pi=0; pi<bufLen; pi++ ) {
        NewSort_ParamReadFlashMLC (_cmd_CE, _cmd_DIE, pi,pbufKeep,bufLen );
        bufSave.Set(pbufKeep[pi],pi,0);
    }

    if ( skips ) {
        cmd->Log ( MyAppUtil::LOGDEBUG,"Read flash parameters for readonly\n" );
        for ( int pi=0; pi<bufLen; pi++ ) {
            const unsigned char defaultv = pbufKeep[pi];
            cmd->Log ( MyAppUtil::LOGDEBUG,"param[[%d/0x%02X][%02X] offset=%d ",pi,pi,defaultv,pi);
            if ( skips->Get(pi) ) {
                cmd->Log ( MyAppUtil::LOGDEBUG,"skiped" );
                bufSave.Set(1,pi,1);
            }
            else {
                //cmd->NewSort_ParamReadFlashMLC (hUsb, _cmd_CE, _cmd_DIE, pi,pbuf,bufLen );
                unsigned char nvv = ~defaultv;
                cmd->NewSort_ParamWriteFlashMLC( _cmd_CE, _cmd_DIE, pi, nvv );
                cmd->NewSort_ParamReadFlashMLC (_cmd_CE, _cmd_DIE, pi,pbuf,bufLen );
                if ( cmd->GetParam()->Design!=0x32 && pbuf[pi]!=nvv ) {
                    cmd->Log ( MyAppUtil::LOGDEBUG,"is read only [%02x!=%02X]",nvv,(unsigned char)pbuf[pi] );
                    bufSave.Set(2,pi,1);
                }
                else {
                    cmd->CloseVendorMode ( _cmd_CE );
                    int ret = cmd->USB_GetFlashID ( pFlashIDBuf ,0,0 );
                    if ( ret ) {
                        cmd->Log ( MyAppUtil::LOGDEBUG,"### Err: FlashGetID fail." );
                        return 1;
                    }
                    //after get flash id, need to re-enter vendor mode.
                    cmd->NewSort_SetVendorMode ( _cmd_CE );
                    if ( memcmp(flashID,pFlashIDBuf,sizeof(flashID)) || cmd->GetParam()->Design==0x32 ) {
                        cmd->Log ( MyAppUtil::LOGDEBUG,"for Flash ID [%02X %02X %02X %02X %02X %02X %02X]=[%02X %02X %02X %02X %02X %02X %02X]",
                                   flashID[0],flashID[1],flashID[2],flashID[3],flashID[4],flashID[5],flashID[6],
                                   pFlashIDBuf[0],pFlashIDBuf[1],pFlashIDBuf[2],pFlashIDBuf[3],pFlashIDBuf[4],pFlashIDBuf[5],pFlashIDBuf[6]);
                        bufSave.Set(3,pi,1);
                    }
                }
                cmd->NewSort_ParamWriteFlashMLC( _cmd_CE, _cmd_DIE, pi, defaultv );
            }
            cmd->Log ( MyAppUtil::LOGDEBUG,"\n" );
        }
    }
    cmd->CloseVendorMode ( _cmd_CE );
    cmd->Dev_SetFlashClock ( _flashTiming );
    CMDFlashParamResetTSB();

    //write summary
    MemStr fn ( 512 );
    fn.SetEmpty();
    fn.Cat("%s\\param_table.txt",cmd->GetLogDir());
    {
        FileMgr fm( fn.Data(), "w" );
        fprintf( fm.Ptr(), "index,value,type(0=valid/1=skip/2=readonly/3=flashid)\n" );
        for ( int pi=0; pi<bufLen; pi++ ) {
            if ( dumps && dumps->Get(pi)==0 ) {
                continue;
            }
            fprintf( fm.Ptr(), "%02X,%02X,%02X\n",pi,bufSave.Get(pi,0),bufSave.Get(pi,1) );
        }
        fflush(fm.Ptr());
        fm.Close();
    }
    return 0;
}
*/
int TSBSQLCmd::GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf )
{
	assert(0);
	return 0;
}

int TSBSQLCmd::Get_RetryFeature( unsigned char ce, unsigned char *buf )
{
	assert(0);
	return 0;
}

int TSBSQLCmd::XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len )
{
	CharMemMgr tmpbuf1(len);
	CharMemMgr tmpbuf2(len);
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char cell = LogTwo(_fh->Cell);

	unsigned char TSB_SQLXNOR1[] =
	{
		// 0,   1,   2,
		0xCA,0x2E,0x01,
		// 3,   4,   5,   6,
		0xC2,0xD5,0xAB,0x00,
		// 7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,
		0xAB,0x89,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xCB,
		//18,  19,  20,  21,
		0xC2,0xD5,0xAB,0x00,
		//22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
		0xAB,0x8A,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xCB,
		//33,  34,  35,  36,  37,  38,  39,  40,
		0xC2,0x01,0xC2,0x00,0xA0,0xC2,0x30,0xCB,
		//41,  42,  43,  44,  44,  46,  47,  48,
		0xC2,0x05,0xAA,0xC2,0xE0,0xDA,0x09,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_SQLXNOR2[] =
	{
		// 0,   1,   2,
		0xCA,0x10,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,
		0xC2,0x01,0xC2,0x00,0xA0,0xC2,0x30,0xCB,
		//11,  12,  13,  14,  15,  16,  17,  18,
		0xC2,0x05,0xAA,0xC2,0xE0,0xDA,0x09,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char TSB_SQLXNOR3[] =
	{
		// 0,   1,   2,
		0xCA,0x16,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,
		0xC2,0x01,0xC2,0x00,0xA0,0xC2,0x3C,0xCB,
		//11,  12,  13,  14,  15,  16,
		0xC2,0xCB,0xCB,0xC2,0x3F,0xCB,
		//17,  18,  19,  20,  21,  22,  23,  24,
		0xC2,0x05,0xAA,0xC2,0xE0,0xDA,0x09,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};

	int slen = 0;
	unsigned char *sql =0;

	sql = TSB_SQLXNOR1;
	slen = sizeof(TSB_SQLXNOR1);
	sql[6] = die;
	sql[21] = die;
	sql[34] = (Page%cell)+1;
	sql[47] = len/1024;

            
	SetFTA( &sql[slen-8], Page, block );
	int ret= Dev_IssueIOCmd(sql,slen, 2, ce, tmpbuf1.Data(), len );
	if(ret)
	{
		return ret;
	}
	
	sql = TSB_SQLXNOR2;
	slen = sizeof(TSB_SQLXNOR2);
	sql[4] = (Page%cell)+1;
	sql[17] = len/1024;
     
	SetFTA( &sql[slen-8], Page, block );
	ret= Dev_IssueIOCmd(sql,slen, 2, ce, tmpbuf2.Data(), len );
	if(ret)
	{
		return ret;
	}
	
	sql = TSB_SQLXNOR3;
	slen = sizeof(TSB_SQLXNOR3);
	sql[4] = (Page%cell)+1;
	sql[23] = len/1024;
     
	SetFTA( &sql[slen-8], Page, block );
	ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf1, len );
	if(ret)
	{
		return ret;
	}
	for(int i=0;i<len;i++)
	{
		//page buf 4 = ~ ( page buf 2 ^ page buf 1)  //!!¡ÓXNOR operation!!L
		buf2[i] = ~ (tmpbuf1.Get(i)^tmpbuf2.Get(i));
	}
	return ret;
}

int TSBSQLCmd::LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len )
{
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);

	unsigned char TSB_SQLLIC[] =
	{
		// 0,   1,   2,
		0xCA,0x31,0x01,
		// 3,   4,   5,   6,
		0xC2,0xD5,0xAB,0x00,
		// 7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,
		0xAB,0x89,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xCB,
		//18,  19,  20,  21,
		0xC2,0xD5,0xAB,0x00,
		//22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
		0xAB,0x8A,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xCB,
		//33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
		0xC2,0xC9,0xAB,0x00,0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,
		//44,  45,  46,  47,  48,  49,  50,  51,
		0xC2,0x05,0xAA,0xC2,0xE0,0xDA,0x09,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};

	int slen = 0;
	unsigned char *sql =0;

	sql = TSB_SQLLIC;
	slen = sizeof(TSB_SQLLIC);
	sql[6] = die;
	sql[21] = die;
	sql[36] = Level;
	sql[50] = len/1024;

            
	SetFTA( &sql[slen-8], Page, block );
	int ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf, len );	

	return ret;
}

////////////////////////////Micron////////////////////////
MicronSQLCmd::MicronSQLCmd()
{
}

MicronSQLCmd::~MicronSQLCmd()
{
}

int MicronSQLCmd::GetFlashParam( unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die )
{
	assert(0);
	return 0;
}

int MicronSQLCmd::SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value, unsigned char die )
{
	unsigned char sqlMCR[] = 
	{
		// 0,   1,   2,
		0xCA,0x0C,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
		0xC2,0xEB,0xAB,0x2F,0xAB,0xF0,0xAB,0x00,0xDE,0x18,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	int slen = sizeof(sqlMCR);
	unsigned char addr = (offset>>8)&0xFF;
	unsigned char table = offset&0xFF;
	sqlMCR[slen-12] = value;
	sqlMCR[slen-16] = table;
	sqlMCR[slen-18] = addr;
	int ret = Dev_IssueIOCmd(sqlMCR,slen, 0, ce, 0, 0 );
	return ret;
}

int MicronSQLCmd::Erase(unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6)
{
	unsigned char sql[256], slen = 0, temp, i, ftap[6];
	int maxpage = 0, CE, ret = 0;

	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Erase(ce, block, bSLCMode, planemode, bDFmode, bWaitRDY, bAddr6);
	}
	for( CE = 0; CE < _fh->FlhNumber; CE++ )
	{
		slen=0;
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(_fh->FlhNumber==1)
			{
				CE = 0;
			}
			else
			{
				CE = ce;
			}
		}
		//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
		sql[slen]=0xCA;slen++;	// Stare from 0xCA
		sql[slen]=0x00;slen++;	// temp set Command len = 0
		temp=0x01;				// default used one addrress mode

		sql[slen]=temp;slen++;	// Operation setting

		if(_fh->bIs3D)
		{
			maxpage = _fh->BigPage;
		}
		if(bDFmode)
		{
			//not sure have DF erase
		}
		if(bSLCMode)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xDA;slen++;
		}
		else if(_fh->bIs3D)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xDF;slen++;
		}
		/*
		if(planemode>=2)
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC2;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
				if(i==planemode-1)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xD0;slen++;
				}
				else
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xD1;slen++;
				}
			}	
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}*/

		if((planemode>=2)||(bAddr6))
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i,maxpage);
				sql[slen]=0xC2;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
				if(bAddr6)
				{
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
				}
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=0xD0;slen++;
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}
		
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(bWaitRDY==1)
			{
				sql[slen]=0xCB;slen++;
			}
		}
		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], 0, block, maxpage);
		slen+=6;
		/*
		if(planemode==2)
		{
			SetSQLAddr(&sql[slen],0, block+1, 0);
			slen+=6;
		}*/
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

		//int ret=_scsi->Dev_IssueIOCmd(sql, slen, 0, ce, 0, 0);
	    ret=Dev_IssueIOCmd(sql, slen, 0, CE, 0, 0);
		if ( !ret )
		{
			if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
			{
				if(bWaitRDY==0)
				{
					// check status
					int timeout=0;
					do
					{
								GetStatus(CE, ftap, 0x70);	// 750us one time
						if(timeout>=4000)	// Max 3sec timeout
						{
							break;
						}
						timeout++;
					}
					while((ftap[0]&0xE0)!=0xE0);
				}
			}
		}
		else
		{
			break;
		}
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			break;
		}
	}
	if((ce==_fh->FlhNumber)&&(_fh->FlhNumber!=1))
	{
		for( CE = 0; CE < _fh->FlhNumber; CE++ )
		{
			if(bWaitRDY==0)
			{
				// check status
				int timeout=0;
				do
				{
					GetStatus(CE, ftap, 0x70);	// 750us one time
					if(timeout>=4000)	// Max 3sec timeout
					{
						break;
					}
					timeout++;
				}
				while((ftap[0]&0xE0)!=0xE0);
			}
			else if(bWaitRDY==1)
			{
				OnlyWiatRDY(CE);
			}
		}
	}
    return ret;
}

int MicronSQLCmd::Write(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode, unsigned char ProgStep, bool bCache, bool bLastPage, char bWaitRDY, bool bAddr6 )
{
	unsigned char sql[256],slen=0,temp,ftap[6],chkStatus=0x40;
	unsigned char cell=LogTwo(_fh->Cell);
	unsigned char p_cnt = (ProgStep>>4);
	int maxpage=0;
	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Write(ce, block, page, bConvs, bSLCMode, buf, len, planemode, ProgStep, bCache, bLastPage, bWaitRDY, bAddr6);
	}
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	ProgStep &=0x0F;
	
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x81;				// default used write mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting
	if(_fh->bIs3D)
	{
		maxpage = _fh->BigPage;
	}
	if(bSLCMode)
	{
		sql[slen]=0xC2;slen++;

		//if()// 130 Series
		//	sql[slen]=0x3B;slen++;
		//else
			sql[slen]=0xDA;slen++;
	}
	else if(_fh->bIs3D)
	{			
		sql[slen]=0xC2;slen++;
		//if()
		//	sql[slen]=0x3C;slen++;
		//else
			sql[slen]=0xDF;slen++;
	}
	if(bAddr6)
	{
		SetFTA(ftap, page, block, maxpage);
		sql[slen]=0xC2;slen++;
		if((!_fh->beD3)&&((block%planemode)!=0))
		{
			sql[slen]=0x81;slen++;
		}
		else
		{
			sql[slen]=0x80;slen++;
		}
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[0];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[1];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[2];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[3];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[4];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[5];slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}
	else
	{			
		sql[slen]=0xC0;slen++;
		if((!_fh->beD3)&&((block%planemode)!=0))
		{
			sql[slen]=0x81;slen++;
		}
		else
		{
			sql[slen]=0x80;slen++;
		}
		sql[slen]=0xA0;slen++;
		sql[slen]=0xCC;slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}		
	sql[slen]=0xC2;slen++;
	chkStatus=0x40;
	if((block%planemode)!=(planemode-1))
	{
		sql[slen]=0x11;slen++;
	}
	else if(bCache&&(!bLastPage))
	{
		sql[slen]=0x15;slen++;
		chkStatus=0x20;
	}
	else
	{
		sql[slen]=0x10;slen++;
	}

	if(bWaitRDY==1)
	{
		sql[slen]=0xCB;slen++;
	}
	sql[slen]=0xCE;slen++;			// command End
	sql[1]=slen-3;
	sql[slen]=0xA0;slen++;

	SetFTA(&sql[slen], page, block, maxpage);
	slen+=6;
	
	sql[slen]=0xAE;slen++;
	sql[slen]=0xAF;slen++;

	//int ret=_scsi->Dev_IssueIOCmd(sql, slen, 1, ce, buf, len);
	int ret=Dev_IssueIOCmd(sql, slen, 1, ce, (unsigned char*)buf, len);  
	if ( ret ) 
	{
		return ret;
	}
	if(bWaitRDY==0)
	{
		// check status
		int timeout=0;
		do
		{
			GetStatus(ce, ftap, 0x70);	// 750us one time
			if(timeout>=4000)	// Max 3sec timeout
			{
				break;
			}
			timeout++;
		}
		while((ftap[0]&0x60)!=chkStatus);
	}
	return 0;
}

int MicronSQLCmd::Read(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode, bool bReadCmd, bool bReadData, bool bReadEcc, bool bCache, bool bFirstPage, bool bLastPage, bool bCmd36, char bWaitRDY, bool bAddr6 )
{
	int ret;
	unsigned char sql[256],slen=0,temp,ftap[6],mode=0,ReadE=0x00,chkStatus;// mode == 2 is read data
	unsigned char cell=LogTwo(_fh->Cell);
	int maxpage=0;
	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Read(ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
	}
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x01;				// default used read mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting
	if(_fh->bIs3D)
	{
		maxpage = _fh->BigPage;
	}
	if(bReadCmd)
	{
		chkStatus=0x40;
		if(bCmd36)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0x36;slen++;
		}
		// Micron only B85T/B95A and 3D TLC have D1 mode
		// but B85T/B95A has some difficult, will improve later
		if(bSLCMode)
		{
			sql[slen]=0xC2;slen++;
			//if()// 130 Series
			//	sql[slen]=0x3B;slen++;
			//else
				sql[slen]=0xDA;slen++;
		}
		else if(_fh->bIs3D)
		{			
			sql[slen]=0xC2;slen++;
			//if()
			//	sql[slen]=0x3C;slen++;
			//else
				sql[slen]=0xDF;slen++;
		}

		if((!_fh->beD3)&&bCache&&(planemode==1)&&(!bFirstPage))	// Sequential cache read
		{
			if(bLastPage)
			{
				ReadE=0x3F;
			}
			else
			{
				ReadE=0x31;
				chkStatus=0x20;
			}
			sql[slen]=0xC2;slen++;
			sql[slen]=ReadE;slen++;
		}
		else
		{
			if(bCache&&(bLastPage))
			{
				ReadE=0x3F;
			}
			else if((block%planemode)!=(planemode-1))
			{
				ReadE=0x32;
			}
			else if(bCache&&(!bFirstPage))
			{
				ReadE=0x31;
				chkStatus=0x20;
			}
			else
			{
				ReadE=0x30;
			}
			if(bAddr6)
			{
				if(ReadE!=0x3F)
				{
					SetFTA(ftap, page, block, maxpage);
					sql[slen]=0xC2;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
				}
				sql[slen]=0xC2;slen++;
				sql[slen]=ReadE;slen++;
			}
			else
			{	
				if(ReadE!=0x3F)
				{
					SetFTA(ftap, page, block, maxpage);
					sql[slen]=0xC0;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xA0;slen++;
					sql[slen]=0xC1;slen++;
					sql[slen]=ReadE;slen++;
					sql[slen]=0xCC;slen++;
				}
				else
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=ReadE;slen++;
				}
			}			
		}				

		if(bWaitRDY==1)
		{
			sql[slen]=0xCB;slen++;		
			if((bReadData)&&(ReadE==0x30))
			{
				SetFTA(ftap, page, block, maxpage);
				if(bAddr6)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0x05;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0xE0;slen++;
				}
				else
				{			
					sql[slen]=0xC0;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAA;slen++;
					sql[slen]=0xC1;slen++;
					sql[slen]=0x05;slen++;
					sql[slen]=0xCC;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0xE0;slen++;
				}
				if((bConvs)&&(bReadEcc))
					mode=3;
				else
					mode=2;
				sql[slen]=0xDA;slen++;	
				if(mode==3)
				{
					sql[slen]=len;slen++;
				}
				else
				{
					sql[slen]=len/1024;slen++;
				}
			}
		}

		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, maxpage);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len); 
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);
		if(ret)
		{
	        return ret;
	    }	
		if(bWaitRDY==0)
		{
			// check status
			int timeout=0;
			do
			{
				GetStatus(ce, ftap, 0x70);	// 750us one time
				if(timeout>=4000)	// Max 3sec timeout
				{
					break;
				}
				timeout++;
			}
			while((ftap[0]&0x60)!=chkStatus);
		}
	}

	if(bReadData&&(ReadE!=0x30))
	{
		if((_fh->beD3)&&(!bSLCMode))
		{
			page/=cell;// change to WL mode
		}
		if((bConvs)&&(bReadEcc))
			mode=3;
		else
			mode=2;
		slen=3;
		SetFTA(ftap, page, block, maxpage);
		if(bAddr6)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0x00;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[2];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[3];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[4];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[5];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0x05;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0xE0;slen++;
		}
		else
		{			
			sql[slen]=0xC0;slen++;
			sql[slen]=0x00;slen++;
			sql[slen]=0xA0;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0x05;slen++;
			sql[slen]=0xCC;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0xE0;slen++;
		}
		sql[slen]=0xDA;slen++;
		if(mode==3)
		{
			sql[slen]=len;slen++;
		}
		else
		{
			sql[slen]=len/1024;slen++;
		}

		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, maxpage);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);   
		if(ret)
		{
	        return ret;
	    }
	}
	return 0;
}

int MicronSQLCmd::Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd, bool pMLC, bool bSLC)
{
	if(_fh->bIsMicronB16)
	{
		// leave high byte 0~2 low byte max 0xff step 4
		unsigned short LevelOrig[6];
		unsigned char tembuf[512]; 
		unsigned char *sql,PType;
		int i,slen,ret,count1pg;
		unsigned char offset[6][2] = 
		{
			{0x82, 0x04}, 	// SLC Low byte
			{0x83, 0x04}, 	// SLC High byte
			{0xA2, 0x04}, 	// MLC/TLC 1 Low byte
			{0xA3, 0x04}, 	// MLC/TLC 1 Higt byte
			{0xA4, 0x04}, 	// MLC/TLC 2 Low byte
			{0xA5, 0x04}	// MLC/TLC 2 High byte
		}; 
     	unsigned char MICR_SQL_trim_mode_1[] = { //for getting original features only
			// 0,   1,   2,
			0xCA,0x22,0x01,
			// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
			0xC2,0xEB,0xAB,0xDF,0xAB,0x80,0xAB,0x00,0xDE,0x2F,0xCB,
			//14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
			0xC2,0xEA,0xAB,0x00,0xAB,0x00,0xAB,0x00,0xCB,0xDB,0x04, //get value
			//25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
			0xC2,0xEB,0xAB,0xDF,0xAB,0x80,0xAB,0x00,0xDE,0x2D,0xCB,0xCE,
			0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
		};
		unsigned char MICR_SQL_trim_mode_2[] = { //for set/get feature & check 
			// 0,   1,   2,
			0xCA,0x2D,0x01,
			// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
			0xC2,0xEB,0xAB,0xDF,0xAB,0x80,0xAB,0x00,0xDE,0x2F,0xCB,
			//14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
			0xC2,0xEB,0xAB,0x00,0xAB,0x00,0xAB,0x00,0xDE,0x00,0xCB, //set value
			//25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,
			0xC2,0xEA,0xAB,0x00,0xAB,0x00,0xAB,0x00,0xCB,0xDB,0x04, //get value
			//36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
			0xC2,0xEB,0xAB,0xDF,0xAB,0x80,0xAB,0x00,0xDE,0x2D,0xCB,0xCE,
			0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
		};
		unsigned char MICR_SQL_trim_Read_Data[] = { //for set/get feature & check 
			// 0,   1,   2,
			0xCA,0x17,0x01,
			// 3,   4,
			0xC2,0xDF,
			// 5,   6,   7,   8,   9,  10,  11,
			0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,
			//12,  13,  14,  15,  16,  17,  18,  19,
			0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xDA,0x09,
			//20,  21,  22,  23,  24,  25,
			0xC2,0x78,0xA3,0xDB,0x01,0xCE,
			0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
			
		};
		if(!bSLC)
		{
			count1pg=Micron_TLC_Page_to_CountOnePage(Page,&PType);

			if(count1pg==0xffff)
				return 1;	// only test SLC page
		}
		// back up offset value

		sql = MICR_SQL_trim_mode_1;
		slen = sizeof(MICR_SQL_trim_mode_1);
		for(i=0;i<6;i++)
		{
			sql[17] = offset[i][0];
			sql[19] = offset[i][1];
			ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
			LevelOrig[i]=tembuf[0];
		}

		// D3 mode SLC used TLCaopage mapping	==>SLC page number
		// D3 mode MLC/TLC used SLC page mapping==>WL
		sql = MICR_SQL_trim_mode_2;
		slen = sizeof(MICR_SQL_trim_mode_2);
		if(!bSLC)
		{
			//count1pg=NewSort_Micron_TLC_Page_to_CountOnePage(Page,&PType);

			//if(count1pg==0xffff)
			//	return 1;	// only test SLC page
			if(PType=='S')
			{
				for(i=0;i<2;i++)
				{
					sql[17] = offset[i][0];
					sql[19] = offset[i][1];
					sql[23] = (level>>(8*(i%2)))&0xff;
					sql[28] = offset[i][0];
					sql[30] = offset[i][1];
					ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
				}
			}
			else
			{
				for(i=2;i<6;i++)
				{
					sql[17] = offset[i][0];
					sql[19] = offset[i][1];
					sql[23] = (level>>(8*(i%2)))&0xff;
					sql[28] = offset[i][0];
					sql[30] = offset[i][1];
					ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
				}
			}
			// read data
			sql=MICR_SQL_trim_Read_Data;
			slen = sizeof(MICR_SQL_trim_Read_Data);
			if(PType=='S')
			{
				sql[4] = 0xDA;
			}
			sql[19] = (len/1024);
			SetFTA(&sql[slen-8],count1pg,block,_fh->BigPage);
			ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf, len );
			
			sql = MICR_SQL_trim_mode_2;
			slen = sizeof(MICR_SQL_trim_mode_2);
			for(i=0;i<6;i++)
			{
				sql[17] = offset[i][0];
				sql[19] = offset[i][1];
				sql[23] = (LevelOrig[i]>>(8*(i%2)))&0xff;
				sql[28] = offset[i][0];
				sql[30] = offset[i][1];
				ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
			}
		}
		else
		{
			for(i=2;i<6;i++)
			{
				sql[17] = offset[i][0];
				sql[19] = offset[i][1];
				sql[23] = (level>>(8*(i%2)))&0xff;
				sql[28] = offset[i][0];
				sql[30] = offset[i][1];
				ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
			}
			
			// read data
			sql=MICR_SQL_trim_Read_Data;
			slen = sizeof(MICR_SQL_trim_Read_Data);
			sql[4] = 0xDA;
			sql[19] = (len/1024);
			SetFTA(&sql[slen-8],Page,block,_fh->BigPage);
			ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf, len );	// buffer for count1 must don't overwrite
			
			sql = MICR_SQL_trim_mode_2;
			slen = sizeof(MICR_SQL_trim_mode_2);
			for(i=2;i<6;i++)
			{
				sql[17] = offset[i][0];
				sql[19] = offset[i][1];
				sql[23] = (LevelOrig[i]>>(8*(i%2)))&0xff;
				sql[28] = offset[i][0];
				sql[30] = offset[i][1];
				ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
			}
		}
		return ret;
	}
#if 0
	else if()
	{
		unsigned char offsetW = 0xB, offsetR=0;
		unsigned char msb = 1;
		int d3pageidx,d3lmu,hmlc;
		MicronB95APageConv( Page, &d3pageidx, &d3lmu, &hmlc );
		if ( hmlc==1 ) {
			//Log(LOGDEBUG,"command not support hmlcx1\n");
			assert(0);
		}

		unsigned char v = (unsigned char)level;
#if 0
		unsigned char param[4];
		//enable super calibration
		//NewSort_FeatureFlashSet( ce, 0x96, 0x02 ); //enable Super Calibration
		// NewSort_FeatureFlashSet( ce, 0x96, 0x03 ); //enable Super Calibration
		// NewSort_SetMicronFlashMLBI(ce,0xFC,0x00,ce,0x01); //Super Calibration offset only
		// NewSort_GetMicronFlashMLBI(ce,0xFC,0x00,ce,param);
		// if ( param[0]!=0x01 )  {
		// Log(LOGDEBUG,"set mlbi fail %02X!=0x01\n",param[0]);
		// assert(0);
		// }


		assert(v!=0);

		NewSort_SetMicronFlashMLBI(ce,offsetW,msb,ce,v);
		//NewSort_GetMicronFlashMLBI(ce,offsetW,0x01,ce,param);
		//NewSort_SetMicronFlashMLBI(ce,offsetW,0x01,ce,v);
		//NewSort_GetMicronFlashMLBI(ce,offsetW,0x01,ce,param);
		//assert(v==param[0]);
		int d3pgidx = Page*3+2;
		d3pgidx++;
		d3pgidx++;
		//int ret = NewSort_DumpRead (ce,block,d3pgidx/3,buf,len,0x80|(d3pgidx%3),0 );
		int ret = NewSort_DumpRead (ce,block,d3pgidx/3,buf,len,0,0 );
		_CmdLog ( __FUNCTION__,"ret=%d\n", ret );
		NewSort_SetMicronFlashMLBI(ce,offsetW,mb,ce,0);
		//NewSort_SetMicronFlashMLBI(ce,0xFC,0x00,ce,0x00); //Super Calibration auto cal
		//NewSort_FeatureFlashSet( ce, 0x96, 0x00 ); //disable Super Calibration
#else
		unsigned char sqlMCR[] = {
            // 0,   1,   2,
			0xCA,0x4D,0x01,
            // 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
			0xC2,0xEB,0xAB,0x2F,0xAB,0xF0,0xAB,0x00,0xDE,0x18,0xCB,
            //14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  22,  23,  24,
			0xC2,0xEF,0xAB,0xD1,0xDE,0x01,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xCB,
            //25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
			0xC2,0xEF,0xAB,0xD6,0xDE,0x07,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xCB,
            //38,  39,  40,  41,  42,  43,  44,
			0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,
            //45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,
			0xC2,0xEF,0xAB,0xD1,0xDE,0x01,0xDE,0x01,0xDE,0x00,0xDE,0x00,0xCB,
            //58,  59,  60,  61,  62,  63,  64,  65,
			0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xDA,0x00,
            //66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,
			0xC2,0xEB,0xAB,0x2F,0xAB,0xF0,0xAB,0x00,0xDE,0x00,0xCB,
            //77,
			0xCE,
			0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
		};
		int d3pgidx = Page*3+2;
		unsigned char *sql = sqlMCR;
		int slen = sizeof(sqlMCR);
		sql[6] = offsetW;
		sql[8] = msb;  //
		sql[71] = sql[6];
		sql[73] = sql[8];

		sql[10] = 0;
		sql[12] = v;
		sql[21] = 3;
		sql[32] = 0x0D + d3pgidx%3;
		sql[67] = len/1024;
		int page = d3pgidx/3;

		SetFTA(&sql[slen-8],page,block);
		int ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf, len );
		_pNTCmd->nt_file_log ( __FUNCTION__,"ret=%d\n", ret );
		return ret;
#endif
	}
#endif
	else
	{
		assert(0);
	}
	return 0;
}

int MicronSQLCmd::GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf )
{
	assert(0);
	return 0;
}

int MicronSQLCmd::Get_RetryFeature( unsigned char ce, unsigned char *buf )
{
	assert(0);
	return 0;
}

/*
void MicronSQLCmd::MicronB95APageConv( int d1PageIdx, int *d3pageidx, int *d3lmu, int *hmlc )
{
    unsigned char isGen4 = _IsMicronB95AGen4(_GetFh());
 
    if ( d1PageIdx<2 ) {
        if ( isGen4 ) {
            *d3pageidx = d1PageIdx;
        }
        else {
            *d3pageidx = d1PageIdx+2;
        }
        *d3lmu = 3;
        if ( hmlc ) {
            *hmlc = 1;
        }
    }
    else if ( d1PageIdx>=766) {
        if ( isGen4 ) {
            *d3pageidx = d1PageIdx-766+2;
        }
        else {
            *d3pageidx = d1PageIdx-766+4;
        }
        *d3lmu = 3;
        if ( hmlc ) {
            *hmlc = 1;
        }
    }
    else if ( d1PageIdx>=758 && d1PageIdx<766 ) {
        int d3pgidx = (d1PageIdx-758);
        *d3pageidx = 252+d3pgidx/2;
        *d3lmu = d3pgidx&1;
        if ( hmlc ) {
            *hmlc = 2;
        }
    }
    else {
        int d3pgidx = (d1PageIdx-2);
        *d3pageidx = d3pgidx/3;
        *d3lmu = d3pgidx%3;
        if ( hmlc ) {
           *hmlc = 3;
        }
    }
}
*/
int MicronSQLCmd::Micron_TLC_Page_to_CountOnePage(int page_num, unsigned char *Type)
{
	if (_fh->bIsMicronB16)
	{
		if ( page_num <= 11 )
		{
			*Type = 'S';
			return page_num;
		}
		else if ( page_num > 11 && page_num <= 35 )
		{
			*Type = 'M';
			if(( page_num - 12 ) % 2)
			{
				return 0xffff;
			}
			else
			{
				return (( page_num - 12 ) / 2);
			}
			
		}
		else if ( page_num > 35 && page_num <= 59 )
		{
			*Type = 'T';
			return (12 + page_num - 36) ;
		}
		else if ( page_num > 59 && page_num <= 2221 && ((page_num - 59) % 3) )
		{
			*Type = 'T';
			return 0xffff;
		}
		else if ( page_num > 61 && page_num <= 2219)
		{
			*Type = 'T';
			return (36 + ( page_num - 62 ) / 3) ;
		}
		else if ( page_num > 2223 && page_num <= 2269 && ((page_num - 2222) % 4) && ((page_num - 2223) % 4))
		{
			*Type = 'T';
			return 0xffff ;
		}
		else if ( page_num > 2221 && page_num <= 2267)
		{
			*Type = 'M';
			if((( page_num - 2222 ) % 4)==1)
			{
				return 0xffff;
			}
			else
			{
				return (756 + ( page_num - 2222 ) / 4);
			}
		}
		else if ( page_num > 2269 && page_num <= 2291)
		{
			*Type = 'T';
			return 0xffff;
		}
		else if ( page_num > 2291 && page_num <= 2303)
		{
			*Type = 'S';
			return page_num;
		}
		else
		{
			return 0xffff;
		}
	}
	return 0xffff;
}

int MicronSQLCmd::XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len )
{	
	assert(0);
	return 0;
}

int MicronSQLCmd::LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len )
{
	assert(0);
	return 0;
}

////////////////////////////Hynix////////////////////////
HynixSQLCmd::HynixSQLCmd()
{
}

HynixSQLCmd::~HynixSQLCmd()
{
}

int HynixSQLCmd::GetFlashParam( unsigned char ce,unsigned int offset, unsigned char *value , unsigned char die )
{
	// Get flash special Parameter
    unsigned char buf[512];
    int ret =1 ;
	
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char sql[] = 
	{
		// 0,   1,   2,
	    0xCA,0x07,0x01,
	    // 3,   4,   5,   6,   7,   8,   9, 
	    0xC2,0x37,0xAB,0x71,0xDB,0x01,0xCE,
	    
	    0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	int slen = sizeof(sql);
	sql[6] = offset;
	ret = Dev_IssueIOCmd( sql,slen, 2, ce, buf, 512 );
	*value = buf[0];
    
    return ret;
}

int HynixSQLCmd::SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value, unsigned char die )
{
	// Set flash special Parameter
    unsigned char buf[512];
    int ret = 1;
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char sql[] = 
	{
		// 0,   1,   2,
		0xCA,0x09,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11, 
		0xC2,0x36,0xAB,0x71,0xDE,0x00,0xC2,0x16,0xCE,

		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	int slen = sizeof(sql);
	sql[6] = offset;
	sql[8] = value;
	ret = Dev_IssueIOCmd( sql, slen, 2, ce, buf, 512 );
    return ret;
}

int HynixSQLCmd::Erase(unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6)
{
	unsigned char sql[256], slen=0, temp, i, ftap[6];
	int CE, ret = 0;

	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Erase(ce, block, bSLCMode, planemode, bDFmode, bWaitRDY, bAddr6);
	}
	for( CE = 0; CE < _fh->FlhNumber; CE++ )
	{
		slen=0;
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(_fh->FlhNumber==1)
			{
				CE = 0;
			}
			else
			{
				CE = ce;
			}
		}
		//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
		sql[slen]=0xCA;slen++;	// Stare from 0xCA
		sql[slen]=0x00;slen++;	// temp set Command len = 0
		temp=0x01;				// default used one addrress mode

		sql[slen]=temp;slen++;	// Operation setting

		if(bDFmode)
		{
			if(_fh->Design<=0x15)
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xC6;slen++;
				
			}
			else
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xDF;slen++;
			}
		}
		if(bSLCMode)
		{
			if(_BootMode)
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xDA;slen++;	// FW SLC mode
			}
			else
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xA2;slen++;
			}
		}
		// not sure can work
		/*
		if(planemode>=2)
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC0;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xA3+((i>0)?0x0A:0x00);slen++;	// 0xAD block addr ++
				sql[slen]=0xCC;slen++;
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=0xD0;slen++;
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}*/	
		/*
		if(planemode==2)
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC0;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xA0;slen++;
				sql[slen]=0xCC;slen++;
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=0xD0;slen++;
		}
		else if(planemode==4)
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC2;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=0xD0;slen++;
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}
		*/
		if((planemode>=2)||(bAddr6))
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC2;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
				if(bAddr6)
				{
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
				}
				if(i==planemode-1)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xD0;slen++;
				}
				else
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xD1;slen++;
				}
			}	
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}
		
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(bWaitRDY==1)
			{
				sql[slen]=0xCB;slen++;
				if((bSLCMode)&&(_BootMode))
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xFF;slen++;	// reset FW SLC mode to normal mode
					sql[slen]=0xCB;slen++;
				}
			}
		}
		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], 0, block, 0);
		slen+=6;
		/*
		if(planemode==2)
		{
			SetSQLAddr(&sql[slen],0, block+1, 0);
			slen+=6;
		}*/
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

    	//int ret=_scsi->Dev_IssueIOCmd(sql, slen, 0, ce, 0, 0);
	    ret=Dev_IssueIOCmd(sql, slen, 0, CE, 0, 0);
		if ( !ret )
		{
			if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
			{
				if(bWaitRDY==0)
				{
					// check status
					int timeout=0;
					do
					{
						GetStatus(CE, ftap, 0x70);	// 750us one time
						if(timeout>=4000)	// Max 3sec timeout
						{
							break;
						}
						timeout++;
					}
					while((ftap[0]&0xE0)!=0xE0);
					if((bSLCMode)&&(_BootMode))
					{
						FlashReset(CE);
					
						timeout=0;
						do
						{
							GetStatus(CE, ftap, 0x70,false);	// no save status
							if(timeout>=4000)
							{
								break;
							}
							timeout++;
						}
						while((ftap[0]&0xE0)!=0xE0);
					}
				}
			}
		}
		else
		{
			break;
		}
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			break;
		}
	}
	if((ce==_fh->FlhNumber)&&(_fh->FlhNumber!=1))
	{
		for( CE = 0; CE < _fh->FlhNumber; CE++ )
		{
			if(bWaitRDY==0)
			{
				// check status
				int timeout=0;
				do
				{
					GetStatus(CE, ftap, 0x70);	// 750us one time
					if(timeout>=4000)	// Max 3sec timeout
					{
						break;
					}
					timeout++;
				}
				while((ftap[0]&0xE0)!=0xE0);
				if((bSLCMode)&&(_BootMode))
				{
					FlashReset(CE);
				
					timeout=0;
					do
					{
						GetStatus(CE, ftap, 0x70,false);	// no save status
						if(timeout>=4000)
						{
							break;
						}
						timeout++;
					}
					while((ftap[0]&0xE0)!=0xE0);
				}
			}
			else if(bWaitRDY==1)
			{
				OnlyWiatRDY(CE);
				if((bSLCMode)&&(_BootMode))
				{
					FlashReset(CE);
					OnlyWiatRDY(CE);
				}
			}
		}
	}
    return ret;
}

int HynixSQLCmd::Write(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode, unsigned char ProgStep, bool bCache, bool bLastPage, char bWaitRDY, bool bAddr6)
{
	unsigned char sql[256],slen=0,temp,ftap[6],chkStatus=0x40;
	unsigned char cell=LogTwo(_fh->Cell);
	unsigned char p_cnt = (ProgStep>>4);
	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Write(ce, block, page, bConvs, bSLCMode, buf, len, planemode, ProgStep, bCache, bLastPage, bWaitRDY, bAddr6);
	}
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	ProgStep &=0x0F;
	
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x81;				// default used write mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting

	if(bSLCMode)
	{
		if(_BootMode)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xDA;slen++;	// FW SLC mode
		}
		else
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xA2;slen++;
		}
	}
	else
	{
		if(_fh->beD3)
		{
			if(ProgStep)// 19nm/24nm programming step
			{
				sql[slen]=0xC2;slen++;
				if(ProgStep==1)
				{
					sql[slen]=0x09;slen++;
				}
				else
				{
					sql[slen]=0x0D;slen++;
				}
			}
			sql[slen]=0xC2;slen++;
			sql[slen]=p_cnt+1;slen++;
		}
	}
	if(bAddr6)
	{
		SetFTA(ftap, page, block);
		sql[slen]=0xC2;slen++;
		if(((!_fh->beD3)||(_fh->bIs3D))&&(block%planemode)!=0)
		{
			sql[slen]=0x81;slen++;
		}
		else
		{
			sql[slen]=0x80;slen++;
		}
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[0];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[1];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[2];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[3];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[4];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[5];slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}
	else
	{			
		sql[slen]=0xC0;slen++;
		if(((!_fh->beD3)||(_fh->bIs3D))&&(block%planemode)!=0)
		{
			sql[slen]=0x81;slen++;
		}
		else
		{
			sql[slen]=0x80;slen++;
		}
		sql[slen]=0xA0;slen++;
		sql[slen]=0xCC;slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}		
	sql[slen]=0xC2;slen++;
	chkStatus=0x40;
	if((block%planemode)!=(planemode-1))
	{
		sql[slen]=0x11;slen++;
	}	
	else if(_fh->beD3&&(p_cnt!=(cell-1))&&(!bSLCMode))
	{
		if(_fh->bIs3D)
		{
			sql[slen]=0x22;slen++;
		}
		else
		{
			sql[slen]=0x1A;slen++;
		}
	}
	else if(bCache&&(!bLastPage))
	{
		sql[slen]=0x15;slen++;
		chkStatus=0x20;
	}
	else
	{
		if((_fh->bIs3D)&&(!bSLCMode))
		{
			sql[slen]=0x23;slen++;
		}
		else
		{
			sql[slen]=0x10;slen++;
		}
	}

	if(bWaitRDY==1)
	{
		sql[slen]=0xCB;slen++;
		if((bSLCMode)&&(_BootMode))
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xFF;slen++;	// reset FW SLC mode to normal mode
			sql[slen]=0xCB;slen++;
		}
	}
	sql[slen]=0xCE;slen++;			// command End
	sql[1]=slen-3;
	sql[slen]=0xA0;slen++;

	SetFTA(&sql[slen], page, block, 0);
	slen+=6;
	
	sql[slen]=0xAE;slen++;
	sql[slen]=0xAF;slen++;

	//int ret=_scsi->Dev_IssueIOCmd(sql, slen, 1, ce, buf, len);
	int ret=Dev_IssueIOCmd(sql, slen, 1, ce, (unsigned char*)buf, len);  
	if ( ret ) 
	{
		return ret;
	}
	if(bWaitRDY==0)
	{
		// check status
		int timeout=0;
		do
		{
			GetStatus(ce, ftap, 0x70);	// 750us one time
			if(timeout>=4000)	// Max 3sec timeout
			{
				break;
			}
			timeout++;
		}
		while((ftap[0]&0x60)!=chkStatus);
	}
	return 0;
}

int HynixSQLCmd::Read(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode, bool bReadCmd, bool bReadData, bool bReadEcc, bool bCache, bool bFirstPage, bool bLastPage, bool bCmd36, char bWaitRDY, bool bAddr6)
{
	int ret;
	unsigned char sql[256],slen=0,temp,ftap[6],mode=0,ReadE=0x00,chkStatus;// mode == 2 is read data
	unsigned char cell=LogTwo(_fh->Cell);
	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Read(ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
	}
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x01;				// default used read mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting
	if(bReadCmd)
	{
		chkStatus=0x40;
		if(bCmd36)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0x36;slen++;
		}
		if(bSLCMode)
		{
			if(_BootMode)
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xDA;slen++;	// FW SLC mode
			}
			else
			{
				sql[slen]=0xC2;slen++;
				sql[slen]=0xA2;slen++;
			}
		}
		else if(_fh->beD3)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=(page%cell)+1;slen++;
			page/=cell;// change to WL mode
		}

		if((!_fh->beD3||bSLCMode)&&bCache&&(planemode==1))	// Sequential cache read
		{
			if(bLastPage)
			{
				ReadE=0x3F;
			}
			else if(bFirstPage)
			{
				ReadE=0x30;
			}
			else
			{
				ReadE=0x31;
				chkStatus=0x20;
			}
			sql[slen]=ReadE;slen++;
		}
		else
		{
			if(bCache&&(bLastPage))
			{
				ReadE=0x3F;
			}
			else if((block%planemode)!=(planemode-1))
			{
				ReadE=0x32;
			}
			else if(bCache&&(!bFirstPage))
			{
				ReadE=0x31;
				chkStatus=0x20;
			}
			else
			{
				ReadE=0x30;
			}
			if(bAddr6)
			{
				if(ReadE!=0x3F)
				{
					SetFTA(ftap, page, block);
					sql[slen]=0xC2;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
				}
				sql[slen]=0xC2;slen++;
				sql[slen]=ReadE;slen++;
			}
			else
			{	
				if(ReadE!=0x3F)
				{
					SetFTA(ftap, page, block);
					sql[slen]=0xC0;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xA0;slen++;
					sql[slen]=0xC1;slen++;
					sql[slen]=ReadE;slen++;
					sql[slen]=0xCC;slen++;
				}
				else
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=ReadE;slen++;
				}
			}			
		}				

		if(bWaitRDY==1)
		{
			sql[slen]=0xCB;slen++;		
			if((bReadData)&&(ReadE==0x30))
			{
				SetFTA(ftap, page, block);
				if(bAddr6)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0x05;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0xE0;slen++;
				}
				else
				{	
					SetFTA(ftap, page, block);
					sql[slen]=0xC0;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAA;slen++;
					sql[slen]=0xC1;slen++;
					sql[slen]=0x05;slen++;
					sql[slen]=0xCC;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0xE0;slen++;
				}
				if((bConvs)&&(bReadEcc))
					mode=3;
				else
					mode=2;
				sql[slen]=0xDA;slen++;		
				if(mode==3)
				{
					sql[slen]=len;slen++;
				}
				else
				{
					sql[slen]=len/1024;slen++;
				}
				if((bSLCMode)&&(_BootMode))
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xFF;slen++;	// reset FW SLC mode to normal mode
					sql[slen]=0xCB;slen++;
				}
			}
		}
		
		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, 0);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len); 
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, (unsigned char*)buf, len); 
		if(ret)
		{
	        return ret;
	    }	
		if(bWaitRDY==0)
		{
			// check status
			int timeout=0;
			do
			{
				GetStatus(ce, ftap, 0x70);	// 750us one time
				if(timeout>=4000)	// Max 3sec timeout
				{
					break;
				}
				timeout++;
			}
			while((ftap[0]&0x60)!=chkStatus);
		}
	}

	if(bReadData&&(ReadE!=0x30))
	{
		if((_fh->beD3)&&(!bSLCMode))
		{
			page/=cell;// change to WL mode
		}
		if((bConvs)&&(bReadEcc))
			mode=3;
		else
			mode=2;
		slen=3;
		SetFTA(ftap, page, block);
		if(bAddr6)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0x00;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[2];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[3];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[4];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[5];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0x05;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0xE0;slen++;
		}
		else
		{			
			sql[slen]=0xC0;slen++;
			sql[slen]=0x00;slen++;
			sql[slen]=0xA0;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0x05;slen++;
			sql[slen]=0xCC;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0xE0;slen++;
		}
		sql[slen]=0xDA;slen++;
		if(mode==3)
		{
			sql[slen]=len;slen++;
		}
		else
		{
			sql[slen]=len/1024;slen++;
		}
		if((bSLCMode)&&(_BootMode))
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xFF;slen++;	// reset FW SLC mode to normal mode
			if(bWaitRDY==1)
			{
				sql[slen]=0xCB;slen++;
			}
		}
		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, 0);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, (unsigned char*)buf, len);    
		if(ret)
		{
	        return ret;
	    }
	}
	return 0;
}

int HynixSQLCmd::Get_RetryFeature( unsigned char ce, unsigned char *buf )
{
	unsigned char sql[256],slen=0,temp;
	int ret,Count;

	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x01;				// default used one addrress mode

	sql[slen]=temp;slen++;	// Operation setting
	

	sql[slen]=0xC2;slen++;	// Stare from 0xCA
	sql[slen]=0x37;slen++;	// temp set Command len = 0


	if( _fh->Design == 0x16 ) //8T2E
	{
		for( Count = 0; Count < 4; Count++ )
		{
			sql[slen]=0xAB;slen++;
			sql[slen]=(0x38+Count);slen++;
			sql[slen]=0xDB;slen++;
		}
	}
	else
	{
		for( Count = 0; Count < 8; Count++ ) //8T2B
		{
			sql[slen]=0xAB;slen++;
			sql[slen]=(0xB0+Count);slen++;
			sql[slen]=0xDB;slen++;
		}
	}
	ret=Dev_IssueIOCmd(sql, slen, 2, ce, buf, 512);

	return ret;
}

int HynixSQLCmd::Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd, bool pMLC, bool bSLC)
{
	const unsigned char hynix3D = _IsHynix3D(_fh);
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	unsigned char HYX_SQLCNT1_MLC[] = 
	{
        // 0,   1,   2,
		0xCA,0x2F,0x01,
        // 4,   5,   6,
		0xC2,0xFF,0xCB,
        // 7,   8
		0xC2,0x36,
        // 9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
		0xAB,0x65,0xDE,0x00,0xAB,0x30,0xDE,0x00,0xAB,0x25,0xDE,0x01,
        //21,  22,
		0xC2,0x16,
        //23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,
		0xC3,0x01,0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,0xDA,0x09,
        //34,  35,
		0xC2,0x36,
        //36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
		0xAB,0x65,0xDE,0x00,0xAB,0x30,0xDE,0x00,0xAB,0x25,0xDE,0x00,
        //48,  49,
		0xC2,0x16,
        //50,
		0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char HYX_SQLCNT1_TLC[] = 
	{
		// 0,   1,   2,
		0xCA,0x2F,0x01,
		// 3,   4,   5,
		0xC2,0xFF,0xCB,
		// 6,   7,
		0xC2,0x36,
		// 8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
		0xAB,0x2A,0xDE,0x00,0xAB,0x2B,0xDE,0x00,0xAB,0x72,0xDE,0x01,
		//20,  21,
		0xC2,0x16,
		//22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
		0xC3,0x01,0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,0xDA,0x09,
		//33,  34,
		0xC2,0x36,
		//35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,
		0xAB,0x2A,0xDE,0x00,0xAB,0x2B,0xDE,0x00,0xAB,0x72,0xDE,0x00,
		//47,  48,
		0xC2,0x16,
		//49,
		0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char HYX_SQLCNT1_TLC_F14[] = 
	{
		// 0,   1,   2,
		0xCA,0x2F,0x01,
		// 3,   4,   5,
		0xC2,0xFF,0xCB,
		// 6,   7,
		0xC2,0x36,
		// 8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
		0xAB,0x38,0xDE,0x00,0xAB,0x5D,0xDE,0x00,0xAB,0x7E,0xDE,0x01,
		//20,  21,
		0xC2,0x16,
		//22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
		0xC3,0x01,0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,0xDA,0x09,
		//33,  34,
		0xC2,0x36,
		//35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,
		0xAB,0x38,0xDE,0x00,0xAB,0x5D,0xDE,0x00,0xAB,0x7E,0xDE,0x00,
		//47,  48,
		0xC2,0x16,
		//49,
		0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char HYX_SQLCNT1_3DMLC[] = 
	{
		// 0,   1,   2,
		0xCA,0x25,0x01,
		// 3,   4,   5,
		0xC2,0xFF,0xCB,
		// 6,   7,
		0xC2,0x36,
		// 8,   9,  10,  11,  12,  13,  14,  15,
		0xAB,0x71,0xDE,0x00,0xAB,0x02,0xDE,0x00,
		//16,  17,
		0xC2,0x16,
		//18,  19,  20,  21,  22,  23,  24,  25,  26,
		0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,0xDA,0x09,
		//27,  28,
		0xC2,0x36,
		//29,  30,  31,  32,  33,  34,  35,  36,
		0xAB,0x71,0xDE,0x00,0xAB,0x02,0xDE,0x00,
		//37,  38,
		0xC2,0x16,
		//39,
		0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char HYX_SQLCNT1_3DV3[] = 
	{
		// 0,   1,   2,
		0xCA,0x27,0x01,
		// 3,   4,   5,
		0xC2,0xFF,0xCB,
		// 6,   7,
		0xC2,0x36,
		// 8,   9,  10,  11,  12,  13,  14,  15,
		0xAB,0x57,0xDE,0x00,0xAB,0x00,0xDE,0x00,
		//16,  17,
		0xC2,0x16,
		//18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
		0xC2,0xA2,0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,0xDA,0x09,
		//29,  30,
		0xC2,0x36,
		//31,  32,  33,  34,  35,  36,  37,  38,
		0xAB,0x57,0xDE,0x00,0xAB,0x00,0xDE,0x00,
		//39,  40, 
		0xC2,0x16,
		//41,
		0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	unsigned char HYX_SQLCNT1_3DV4[] = 
	{
        // 0,   1,   2,
		0xCA,0x32,0x01,
        // 3,   4,   5,
		0xC2,0xFF,0xCB,
        // 6,   7,
		0xC2,0x36,
        // 8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
		0xAB,0x59,0xDE,0xB0,0xAB,0xF1,0xDE,0x4F,0xAB,0x03,0xDE,0x00,
        //20,  21,
		0xC2,0x16,
        //22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
		0xC2,0xA2,0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,0xDA,0x09,
        //33,  34,  35,
		0xC2,0xFF,0xCB,
        //36,  37,           
		0xC2,0x36,
		//38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,
		0xAB,0x59,0xDE,0xB5,0xAB,0xF1,0xDE,0x75,0xAB,0x03,0xDE,0x00,
		//50,  51,
		0xC2,0x16,
		//52,
		0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};      
	unsigned char HYX_SQLCNT1_3DV5[] = 
	{
        // 0,   1,   2,       
		0xCA,0x1D,0x01,
        // 3,   4,   5,
		0xC2,0xFF,0xCB,
        // 6,   7,
		0xC2,0x36,
        // 8,   9,  10,  11,
		0xAB,0x11,0xDE,0x22, //9 //11           
        //12,  13,,
		0xC2,0x16,
        //14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
		0xC2,0x01,0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,0xDA,0x09,0xC2,0xFF,0xCB,//22
        //28,  29,  30,  31,
		0xC2,0x38,0xCB,0xCE,
        
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	int slen = 0;
	unsigned char *sql =0;

	if( !_fh->bIsHynix3DTLC && !hynix3D )  
	{
		if( _fh->Design==0x14 ) 
		{
			sql = HYX_SQLCNT1_TLC_F14;
			slen = sizeof(HYX_SQLCNT1_TLC_F14);
			int vlevel = 0x23+level;
			sql[15] = ((vlevel>>8)&0xFF);
			sql[11] = (vlevel&0xFF);
			sql[32] = len/1024;

			SetFTA( &sql[slen-8], Page, block );
			int ret= Dev_IssueIOCmd( sql, slen, 2, ce, buf, len );
			return ret;
		}
		else  
		{
			//level define for 0-512;
			if( !_fh->beD3 ) 
			{
				unsigned char def0x65 = 0, def0x30=0, def0x25=0;
				GetFlashParam( ce, 0x65, &def0x65 );
				GetFlashParam( ce, 0x30, &def0x30 );
				GetFlashParam( ce, 0x25, &def0x25 );

				sql = HYX_SQLCNT1_MLC;
				slen = sizeof(HYX_SQLCNT1_MLC);
				sql[11] = (def0x65 | 4);
				if( level<0xFB ) 
				{
					sql[19] = (def0x25 | 128) - 128;
					sql[15] = (level+0x04);
				}
				else 
				{
					sql[19] = (def0x25 | 128);
					sql[15] = (level - 0xFB + 0x70);
				}
                
				if( Page==85 ) 
				{
					sql[23] = 0x02;
				}

				sql[32] = len/1024;

				SetFTA( &sql[slen-8], Page, block );
				int ret= Dev_IssueIOCmd( sql, slen, 2, ce, buf, len );
	
				SetFlashParam( ce, 0x65, def0x65 );
				SetFlashParam( ce, 0x30, def0x30 );
				SetFlashParam( ce, 0x25, def0x25 );
				return ret;
			}
			else 
			{
				sql = HYX_SQLCNT1_TLC;
				slen = sizeof(HYX_SQLCNT1_TLC);
				if( level<0x64 ) 
				{
					sql[15] = 0x10;
					sql[11] = (level&0xFF);
				}
				else if( level<(0x64+0xFE) ) 
				{
					sql[15] = 0x00;
					sql[11] = level-0x64+1;
				}
				else 
				{
					sql[15] = 0x01;
					sql[11] = (level+0xB0)-0x64-0xFE;
				}

				if ( Page==85 )
				{
					sql[23] = 0x02;
				}
				sql[32] = len/1024;

				SetFTA( &sql[slen-8], Page, block );
				int ret= Dev_IssueIOCmd( sql, slen, 2, ce, buf, len );
				return ret;
			}
		}
	}
	else 
	{
		if (!_fh->beD3) 
		{
			unsigned char def0x71 = 0, def0x02=0;
			GetFlashParam( ce, 0x71, &def0x71 );
			GetFlashParam( ce, 0x02, &def0x02 );
			sql = HYX_SQLCNT1_3DMLC;
			slen = sizeof(HYX_SQLCNT1_3DMLC);
			if( level>(0xFF-0x45) ) 
			{
				sql[11] = def0x71+0x20;
				sql[15] = 0x75+((level-(0xFF-0x45))&0xFF);
			}
			else 
			{
				sql[11] = def0x71;
				sql[15] = 0x45+(level)&0xFF;
			}
			sql[26] = len/1024;

			SetFTA(&sql[slen-8],Page,block);
			int ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf, len );
			SetFlashParam( ce, 0x71, def0x71 );
			SetFlashParam( ce, 0x02, def0x02 );
			return ret;
		}
		else 
		{
			if ( hynix3D==3 ) 
			{
				unsigned char def0x57 = 0, def0x00=0;
				GetFlashParam( ce, 0x57, &def0x57 );
				GetFlashParam( ce, 0x00, &def0x00 );
				sql = HYX_SQLCNT1_3DV3;
				slen = sizeof(HYX_SQLCNT1_3DV3);

				if( level>(0xFF) ) 
				{
					sql[11] = 0x61;
					sql[15] = (level-0xFF + 0x7A);
				}
				else 
				{
					sql[11] = 0x60;
					sql[15] = level&0xFF;
				}
				sql[28] = len/1024;
				SetFTA( &sql[slen-8], Page, block );
				int ret= Dev_IssueIOCmd( sql, slen, 2, ce, buf, len );
				SetFlashParam( ce, 0x57, def0x57 );
				SetFlashParam( ce, 0x00, def0x00 );
				return ret;
			}
			else if ( hynix3D==4 ) 
			{
				sql = HYX_SQLCNT1_3DV4;
				slen = sizeof(HYX_SQLCNT1_3DV4);
				unsigned char def0x59 = 0, def0xF1=0, def0x03=0;
				SetFlashParam( ce, 0x59, 0xB0 );
				GetFlashParam( ce, 0xF1, &def0xF1 );
				//GetFlashParam( ce, 0x59, &def0x59 );
				//SetFlashParam( ce, 0x59, 0xB5 );
				//GetFlashParam( ce, 0x03, &def0x03 );
				//GetFlashParam( ce, 0xF1, &def0xF1 );
				//GetFlashParam( ce, 0x59, &def0x59 );
				//
				//GetFlashParam( ce, 0xF1, &def0xF1 );
				//GetFlashParam( ce, 0x03, &def0x03 );
				if ( level>=(0xFF) ) {
					//SetFlashParam( ce, 0x59, 0xB5 );
					//GetFlashParam( ce, 0xF1, &def0xF1 );
					//SetFlashParam( ce, 0xF1, def0xF1 + 32 );

					//SetFlashParam( ce, 0x59, def0x59+0x05 );
					//sql[9] = 0x59;
					//sql[11] = 0xB5;
					//sql[15] = 0x75;//0x75;// + 32;
					//sql[19] = level-0xFF + 0x7A;//(level-0xFF + 0x7A);
					//sql[19] = 0;
					//SetFlashParam( ce, 0x59, def0x59+0x05 );
					//SetFlashParam( ce, 0xF1, 0x75 );
					//SetFlashParam( ce, 0x03, level-0xFF + 0x7A);//(level-0xFF + 0x7A) );
					SetFlashParam( ce, 0x59, 0xB5 );
					GetFlashParam( ce, 0xF1, &def0xF1 );
					GetFlashParam( ce, 0x03, &def0x03 );
					sql[9] = 0x59;
					sql[11] = 0xB5;
					sql[15] = def0xF1 | 0x20;
					sql[19] = level-0xFF + 0x7A;
					sql[41] = 0xB5;
					sql[45] = def0xF1;
					sql[49] = def0x03;
				}
				else 
				{
					SetFlashParam( ce, 0x59, 0xB0 );
					GetFlashParam( ce, 0xF1, &def0xF1 );
					GetFlashParam( ce, 0x03, &def0x03 );
					sql[9] = 0x59;
					sql[11] = 0xB0;
					sql[15] = 0x00;
					sql[19] = level&0xFF;
					sql[41] = 0xB0;
					sql[45] = def0xF1;
					sql[49] = def0x03;
				}
				sql[32] = len/1024;
				SetFTA( &sql[slen-8], Page, block );
				int ret= Dev_IssueIOCmd( sql, slen, 2, ce, buf, len );
				//GetFlashParam( ce, 0x59, &def0x59 );
				SetFlashParam( ce, 0x59, 0xB0 );
				GetFlashParam( ce, 0x59, &def0x59 );
				//SetFlashParam( ce, 0xF1, def0xF1 );
				//SetFlashParam( ce, 0x03, def0x03 );
				return ret;
			}
			else if ( hynix3D == 5 )
			{
				sql = HYX_SQLCNT1_3DV5;
				slen = sizeof(HYX_SQLCNT1_3DV5);
				if( level  > 0xE0 ) //level > 224
				{
					SetFlashParam( ce, 0x11, 0x3E  ); //add 0x11:RV3
					//SetFlashParam( ce, 0x0F, 0x3E  ); //add 0x11:RV3
					sql[9] = 0x15;
					sql[11] = level - 0xE4;
				}
				else 
				{
					SetFlashParam( ce, 0x15, 0xD9 ); //add 0x15:RV7
					//SetFlashParam( ce, 0x13, 0xD9 ); //add 0x15:RV7
					sql[9] = 0x11;
					sql[11] = level + 0x1A;
				}
				//sql[15] = 3;
				sql[24] = len/1024;
				SetFTA( &sql[slen-8], Page, block );
				int ret = Dev_IssueIOCmd( sql, slen, 2, ce, buf, len );
				return ret;
			}
		}      
	}
	return 0;
}

int HynixSQLCmd::XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len )
{	
	assert(0);
	return 0;
}

int HynixSQLCmd::LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len )
{
	assert(0);
	return 0;
}

////////////////////////////YMTC////////////////////////
// YMTC flash No Multi plane mode now
YMTCSQLCmd::YMTCSQLCmd()
{
}

YMTCSQLCmd::~YMTCSQLCmd()
{
}

int YMTCSQLCmd::GetFlashParam( unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die )
{
	assert(0);
	return 0;
}

int YMTCSQLCmd::SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value, unsigned char die )
{
	unsigned char sqlMCR[] = 
	{
		// 0,   1,   2,
		0xCA,0x0C,0x01,
		// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
		0xC2,0xEB,0xAB,0x2F,0xAB,0xF0,0xAB,0x00,0xDE,0x18,0xCB,0xCE,
		0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
	};
	int slen = sizeof(sqlMCR);
	unsigned char addr = (offset>>8)&0xFF;
	unsigned char table = offset&0xFF;
	sqlMCR[slen-12] = value;
	sqlMCR[slen-16] = table;
	sqlMCR[slen-18] = addr;
	int ret = Dev_IssueIOCmd(sqlMCR,slen, 0, ce, 0, 0 );
	return ret;
}

int YMTCSQLCmd::Erase(unsigned char ce,unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6)
{
	unsigned char sql[256], slen = 0, temp, i, ftap[6];
	int CE, ret = 0;
	int maxpage = 0;

	assert((_fh->beD3)||(planemode==1));
	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Erase(ce, block, bSLCMode, planemode, bDFmode, bWaitRDY, bAddr6);
	}
	
	for( CE = 0; CE < _fh->FlhNumber; CE++ )
	{
		slen=0;
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(_fh->FlhNumber==1)
			{
				CE = 0;
			}
			else
			{
				CE = ce;
			}
		}
		//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
		sql[slen]=0xCA;slen++;	// Stare from 0xCA
		sql[slen]=0x00;slen++;	// temp set Command len = 0
		temp=0x01;				// default used one addrress mode

		sql[slen]=temp;slen++;	// Operation setting

		if(_fh->bIs3D)
		{
			maxpage = _fh->BigPage;
		}
		if(bDFmode)
		{
			//not sure have DF erase
		}
		if(bSLCMode)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xDA;slen++;
		}
		else if(_fh->bIs3D)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0xDF;slen++;
		}
		/*
		if(planemode>=2)
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i);
				sql[slen]=0xC2;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
				if(i==planemode-1)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xD0;slen++;
				}
				else
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0xD1;slen++;
				}
			}	
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}*/
		
		
		if((planemode>=2)||(bAddr6))
		{
			for(i=0;i<planemode;i++)			// multi plane erase
			{	
				SetFTA(ftap,0,block+i,maxpage);
				sql[slen]=0xC2;slen++;
				sql[slen]=0x60;slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[2];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[3];slen++;
				sql[slen]=0xAB;slen++;
				sql[slen]=ftap[4];slen++;
				if(bAddr6)
				{
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
				}
			}	
			sql[slen]=0xC2;slen++;
			sql[slen]=0xD0;slen++;
		}
		else
		{
			sql[slen]=0xC0;slen++;
			sql[slen]=0x60;slen++;
			sql[slen]=0xA3;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0xD0;slen++;
			sql[slen]=0xCC;slen++;
		}
		
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			if(bWaitRDY==1)
			{
				sql[slen]=0xCB;slen++;
			}
		}
		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], 0, block, maxpage);
		slen+=6;
		/*
		if(planemode==2)
		{
			SetSQLAddr(&sql[slen],0, block+1, 0);
			slen+=6;
		}*/
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

    	//int ret=_scsi->Dev_IssueIOCmd(sql, slen, 0, ce, 0, 0);
	    ret=Dev_IssueIOCmd(sql, slen, 0, CE, 0, 0);
		if ( !ret )
		{
			if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
			{
				if(bWaitRDY==0)
				{
					// check status
					int timeout=0;
					do
					{
						GetStatus(CE, ftap, 0x70);	// 750us one time
						if(timeout>=4000)	// Max 3sec timeout
						{
							break;
						}
						timeout++;
					}
					while((ftap[0]&0xE0)!=0xE0);
				}
			}
		}
		else
		{
			break;
		}
		if((ce!=_fh->FlhNumber)||(_fh->FlhNumber==1))
		{
			break;
		}
	}
	if((ce==_fh->FlhNumber)&&(_fh->FlhNumber!=1))
	{
		for( CE = 0; CE < _fh->FlhNumber; CE++ )
		{
			if(bWaitRDY==0)
			{
				// check status
				int timeout=0;
				do
				{
					GetStatus(CE, ftap, 0x70);	// 750us one time
					if(timeout>=4000)	// Max 3sec timeout
					{
						break;
					}
					timeout++;
				}
				while((ftap[0]&0xE0)!=0xE0);
			}
			else if(bWaitRDY==1)
			{
				OnlyWiatRDY(CE);
			}
		}
	}
    return ret;
}

int YMTCSQLCmd::Write(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode, unsigned char ProgStep, bool bCache, bool bLastPage, char bWaitRDY, bool bAddr6 )
{
	unsigned char sql[256],slen=0,temp,ftap[6],chkStatus=0x40;
	unsigned char cell=LogTwo(_fh->Cell);
	unsigned char p_cnt = (ProgStep>>4);
	int maxpage=0;
	assert((_fh->beD3)||(planemode==1));
	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Write(ce, block, page, bConvs, bSLCMode, buf, len, planemode, ProgStep, bCache, bLastPage, bWaitRDY, bAddr6);
	}
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	ProgStep &=0x0F;
	
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x81;				// default used write mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting
	if(_fh->bIs3D)
	{
		maxpage = _fh->BigPage;
	}
	if(bSLCMode)
	{
		sql[slen]=0xC2;slen++;

		//if()// 130 Series
		//	sql[slen]=0x3B;slen++;
		//else
			sql[slen]=0xDA;slen++;
	}
	else if(_fh->bIs3D)
	{			
		sql[slen]=0xC2;slen++;
		//if()
		//	sql[slen]=0x3C;slen++;
		//else
			sql[slen]=0xDF;slen++;
	}
	if(bAddr6)
	{
		SetFTA(ftap, page, block, maxpage);
		sql[slen]=0xC2;slen++;
		if(((!_fh->beD3)||(_fh->bIs3D))&&(block%planemode)!=0)
		{
			sql[slen]=0x81;slen++;
		}
		else
		{
			sql[slen]=0x80;slen++;
		}
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[0];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[1];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[2];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[3];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[4];slen++;
		sql[slen]=0xAB;slen++;
		sql[slen]=ftap[5];slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}
	else
	{
		sql[slen]=0xC0;slen++;			
		if(((!_fh->beD3)||(_fh->bIs3D))&&(block%planemode)!=0)
		{
			sql[slen]=0x81;slen++;
		}
		else
		{
			sql[slen]=0x80;slen++;
		}
		sql[slen]=0xA0;slen++;
		sql[slen]=0xCC;slen++;
		if(_bSkipWDMA==false)
		{
			sql[slen]=0xDC;slen++;
		}
	}		
	sql[slen]=0xC2;slen++;
	chkStatus=0x40;
	if((block%planemode)!=(planemode-1))
	{
		sql[slen]=0x11;slen++;
	}
	else if(_fh->beD3&&(p_cnt!=(cell-1))&&(!bSLCMode))
	{
		sql[slen]=0x1A;slen++;
	}
	else if(bCache&&(!bLastPage))
	{
		sql[slen]=0x15;slen++;
		chkStatus=0x20;
	}
	else
	{
		sql[slen]=0x10;slen++;
	}

	if(bWaitRDY==1)
	{
		sql[slen]=0xCB;slen++;
	}
	sql[slen]=0xCE;slen++;			// command End
	sql[1]=slen-3;
	sql[slen]=0xA0;slen++;

	SetFTA(&sql[slen], page, block, maxpage);
	slen+=6;
	
	sql[slen]=0xAE;slen++;
	sql[slen]=0xAF;slen++;

	//int ret=_scsi->Dev_IssueIOCmd(sql, slen, 1, ce, buf, len);
	int ret=Dev_IssueIOCmd(sql, slen, 1, ce, (unsigned char*)buf, len);  
	if ( ret ) 
	{
		return ret;
	}
	if(bWaitRDY==0)
	{
		// check status
		int timeout=0;
		do
		{
			GetStatus(ce, ftap, 0x70);	// 750us one time
			if(timeout>=4000)	// Max 3sec timeout
			{
				break;
			}
			timeout++;
		}
		while((ftap[0]&0x60)!=chkStatus);
	}
	return 0;
}

int YMTCSQLCmd::Read(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode, bool bReadCmd, bool bReadData, bool bReadEcc, bool bCache, bool bFirstPage, bool bLastPage, bool bCmd36, char bWaitRDY, bool bAddr6 )
{
	int ret;
	unsigned char sql[256],slen=0,temp,ftap[6],mode=0,ReadE=0x00,chkStatus;// mode == 2 is read data
	unsigned char cell=LogTwo(_fh->Cell);
	int maxpage=0;
	assert((_fh->beD3)||(planemode==1));
	if(_CmdBuf!= NULL)
	{
		return SQLCmd::Read(ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
	}
	//_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	sql[slen]=0xCA;slen++;	// Stare from 0xCA
	sql[slen]=0x00;slen++;	// temp set Command len = 0
	temp=0x01;				// default used read mode and one addrress mode
	if(bConvs)				// Group wirte
	{
		temp |= 0x5C;		// Show ecc,Enable Conversion,Enable inverse,Enable Ecc
	}

	sql[slen]=temp;slen++;	// Operation setting
	if(_fh->bIs3D)
	{
		maxpage = _fh->BigPage;
	}
	if(bReadCmd)
	{
		chkStatus=0x40;
		if(bCmd36)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0x36;slen++;
		}
		// Micron only B85T/B95A and 3D TLC have D1 mode
		// but B85T/B95A has some difficult, will improve later
		if(bSLCMode)
		{
			sql[slen]=0xC2;slen++;
			//if()// 130 Series
			//	sql[slen]=0x3B;slen++;
			//else
				sql[slen]=0xDA;slen++;
		}
		else if(_fh->bIs3D)
		{			
			sql[slen]=0xC2;slen++;
			//if()
			//	sql[slen]=0x3C;slen++;
			//else
				sql[slen]=0xDF;slen++;
		}

		if((!_fh->beD3)&&bCache&&(planemode==1)&&(!bFirstPage))	// Sequential cache read
		{
			if(bLastPage)
			{
				ReadE=0x3F;
			}
			else
			{
				ReadE=0x31;
				chkStatus=0x20;
			}
			sql[slen]=0xC2;slen++;
			sql[slen]=ReadE;slen++;
		}
		else
		{
			if(bCache&&(bLastPage))
			{
				ReadE=0x3F;
			}
			else if((block%planemode)!=(planemode-1))
			{
				ReadE=0x32;
			}
			else if(bCache&&(!bFirstPage))
			{
				ReadE=0x31;
				chkStatus=0x20;
			}
			else
			{
				ReadE=0x30;
			}
			if(bAddr6)
			{
				if(ReadE!=0x3F)
				{
					SetFTA(ftap, page, block, maxpage);
					sql[slen]=0xC2;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
				}
				sql[slen]=0xC2;slen++;
				sql[slen]=ReadE;slen++;
			}
			else
			{	
				if(ReadE!=0x3F)
				{
					SetFTA(ftap, page, block, maxpage);
					sql[slen]=0xC0;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xA0;slen++;
					sql[slen]=0xC1;slen++;
					sql[slen]=ReadE;slen++;
					sql[slen]=0xCC;slen++;
				}
				else
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=ReadE;slen++;
				}
			}			
		}				

		if(bWaitRDY==1)
		{
			sql[slen]=0xCB;slen++;		
			if((bReadData)&&(ReadE==0x30))
			{
				SetFTA(ftap, page, block, maxpage);
				if(bAddr6)
				{
					sql[slen]=0xC2;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[2];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[3];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[4];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[5];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0x05;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0xE0;slen++;
				}
				else
				{			
					sql[slen]=0xC0;slen++;
					sql[slen]=0x00;slen++;
					sql[slen]=0xAA;slen++;
					sql[slen]=0xC1;slen++;
					sql[slen]=0x05;slen++;
					sql[slen]=0xCC;slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[0];slen++;
					sql[slen]=0xAB;slen++;
					sql[slen]=ftap[1];slen++;
					sql[slen]=0xC2;slen++;
					sql[slen]=0xE0;slen++;
				}
				if((bConvs)&&(bReadEcc))
					mode=3;
				else
					mode=2;
				sql[slen]=0xDA;slen++;	
				if(mode==3)
				{
					sql[slen]=len;slen++;
				}
				else
				{
					sql[slen]=len/1024;slen++;
				}
			}
		}

		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, maxpage);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len); 
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);
		if(ret)
		{
	        return ret;
	    }	
		if(bWaitRDY==0)
		{
			// check status
			int timeout=0;
			do
			{
				GetStatus(ce, ftap, 0x70);	// 750us one time
				if(timeout>=4000)	// Max 3sec timeout
				{
					break;
				}
				timeout++;
			}
			while((ftap[0]&0x60)!=chkStatus);
		}
	}

	if(bReadData&&(ReadE!=0x30))
	{
		if((_fh->beD3)&&(!bSLCMode))
		{
			page/=cell;// change to WL mode
		}
		if((bConvs)&&(bReadEcc))
			mode=3;
		else
			mode=2;
		slen=3;
		SetFTA(ftap, page, block, maxpage);
		if(bAddr6)
		{
			sql[slen]=0xC2;slen++;
			sql[slen]=0x00;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[2];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[3];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[4];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[5];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0x05;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0xE0;slen++;
		}
		else
		{			
			sql[slen]=0xC0;slen++;
			sql[slen]=0x00;slen++;
			sql[slen]=0xA0;slen++;
			sql[slen]=0xC1;slen++;
			sql[slen]=0x05;slen++;
			sql[slen]=0xCC;slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[0];slen++;
			sql[slen]=0xAB;slen++;
			sql[slen]=ftap[1];slen++;
			sql[slen]=0xC2;slen++;
			sql[slen]=0xE0;slen++;
		}
		sql[slen]=0xDA;slen++;
		if(mode==3)
		{
			sql[slen]=len;slen++;
		}
		else
		{
			sql[slen]=len/1024;slen++;
		}

		sql[slen]=0xCE;slen++;			// command End
		sql[1]=slen-3;
		sql[slen]=0xA0;slen++;

		SetFTA(&sql[slen], page, block, maxpage);
		slen+=6;
		
		sql[slen]=0xAE;slen++;
		sql[slen]=0xAF;slen++;

	    //ret=_scsi->Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);
	    ret=Dev_IssueIOCmd(sql, slen, mode, ce, buf, len);   
		if(ret)
		{
	        return ret;
	    }
	}
	return 0;
}

int YMTCSQLCmd::Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd, bool pMLC, bool bSLC)
{
	if(_fh->bIsMicronB16)
	{
		// leave high byte 0~2 low byte max 0xff step 4
		unsigned short LevelOrig[6];
		unsigned char tembuf[512]; 
		unsigned char *sql,PType;
		int i,slen,ret,count1pg;
		unsigned char offset[6][2] = 
		{
			{0x82, 0x04}, 	// SLC Low byte
			{0x83, 0x04}, 	// SLC High byte
			{0xA2, 0x04}, 	// MLC/TLC 1 Low byte
			{0xA3, 0x04}, 	// MLC/TLC 1 Higt byte
			{0xA4, 0x04}, 	// MLC/TLC 2 Low byte
			{0xA5, 0x04}	// MLC/TLC 2 High byte
		}; 
     	unsigned char MICR_SQL_trim_mode_1[] = { //for getting original features only
			// 0,   1,   2,
			0xCA,0x22,0x01,
			// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
			0xC2,0xEB,0xAB,0xDF,0xAB,0x80,0xAB,0x00,0xDE,0x2F,0xCB,
			//14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
			0xC2,0xEA,0xAB,0x00,0xAB,0x00,0xAB,0x00,0xCB,0xDB,0x04, //get value
			//25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
			0xC2,0xEB,0xAB,0xDF,0xAB,0x80,0xAB,0x00,0xDE,0x2D,0xCB,0xCE,
			0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
		};
		unsigned char MICR_SQL_trim_mode_2[] = { //for set/get feature & check 
			// 0,   1,   2,
			0xCA,0x2D,0x01,
			// 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
			0xC2,0xEB,0xAB,0xDF,0xAB,0x80,0xAB,0x00,0xDE,0x2F,0xCB,
			//14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
			0xC2,0xEB,0xAB,0x00,0xAB,0x00,0xAB,0x00,0xDE,0x00,0xCB, //set value
			//25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,
			0xC2,0xEA,0xAB,0x00,0xAB,0x00,0xAB,0x00,0xCB,0xDB,0x04, //get value
			//36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
			0xC2,0xEB,0xAB,0xDF,0xAB,0x80,0xAB,0x00,0xDE,0x2D,0xCB,0xCE,
			0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
		};
		unsigned char MICR_SQL_trim_Read_Data[] = { //for set/get feature & check 
			// 0,   1,   2,
			0xCA,0x17,0x01,
			// 3,   4,
			0xC2,0xDF,
			// 5,   6,   7,   8,   9,  10,  11,
			0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,
			//12,  13,  14,  15,  16,  17,  18,  19,
			0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xDA,0x09,
			//20,  21,  22,  23,  24,  25,
			0xC2,0x78,0xA3,0xDB,0x01,0xCE,
			0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
			
		};
		if(!bSLC)
		{
			count1pg=Micron_TLC_Page_to_CountOnePage(Page,&PType);

			if(count1pg==0xffff)
				return 1;	// only test SLC page
		}
		// back up offset value

		sql = MICR_SQL_trim_mode_1;
		slen = sizeof(MICR_SQL_trim_mode_1);
		for(i=0;i<6;i++)
		{
			sql[17] = offset[i][0];
			sql[19] = offset[i][1];
			ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
			LevelOrig[i]=tembuf[0];
		}

		// D3 mode SLC used TLCaopage mapping	==>SLC page number
		// D3 mode MLC/TLC used SLC page mapping==>WL
		sql = MICR_SQL_trim_mode_2;
		slen = sizeof(MICR_SQL_trim_mode_2);
		if(!bSLC)
		{
			//count1pg=NewSort_Micron_TLC_Page_to_CountOnePage(Page,&PType);

			//if(count1pg==0xffff)
			//	return 1;	// only test SLC page
			if(PType=='S')
			{
				for(i=0;i<2;i++)
				{
					sql[17] = offset[i][0];
					sql[19] = offset[i][1];
					sql[23] = (level>>(8*(i%2)))&0xff;
					sql[28] = offset[i][0];
					sql[30] = offset[i][1];
					ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
				}
			}
			else
			{
				for(i=2;i<6;i++)
				{
					sql[17] = offset[i][0];
					sql[19] = offset[i][1];
					sql[23] = (level>>(8*(i%2)))&0xff;
					sql[28] = offset[i][0];
					sql[30] = offset[i][1];
					ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
				}
			}
			// read data
			sql=MICR_SQL_trim_Read_Data;
			slen = sizeof(MICR_SQL_trim_Read_Data);
			if(PType=='S')
			{
				sql[4] = 0xDA;
			}
			sql[19] = (len/1024);
			SetFTA(&sql[slen-8],count1pg,block,_fh->BigPage);
			ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf, len );
			
			sql = MICR_SQL_trim_mode_2;
			slen = sizeof(MICR_SQL_trim_mode_2);
			for(i=0;i<6;i++)
			{
				sql[17] = offset[i][0];
				sql[19] = offset[i][1];
				sql[23] = (LevelOrig[i]>>(8*(i%2)))&0xff;
				sql[28] = offset[i][0];
				sql[30] = offset[i][1];
				ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
			}
		}
		else
		{
			for(i=2;i<6;i++)
			{
				sql[17] = offset[i][0];
				sql[19] = offset[i][1];
				sql[23] = (level>>(8*(i%2)))&0xff;
				sql[28] = offset[i][0];
				sql[30] = offset[i][1];
				ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
			}
			
			// read data
			sql=MICR_SQL_trim_Read_Data;
			slen = sizeof(MICR_SQL_trim_Read_Data);
			sql[4] = 0xDA;
			sql[19] = (len/1024);
			SetFTA(&sql[slen-8],Page,block,_fh->BigPage);
			ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf, len );	// buffer for count1 must don't overwrite
			
			sql = MICR_SQL_trim_mode_2;
			slen = sizeof(MICR_SQL_trim_mode_2);
			for(i=2;i<6;i++)
			{
				sql[17] = offset[i][0];
				sql[19] = offset[i][1];
				sql[23] = (LevelOrig[i]>>(8*(i%2)))&0xff;
				sql[28] = offset[i][0];
				sql[30] = offset[i][1];
				ret= Dev_IssueIOCmd(sql,slen, 2, ce, tembuf, 512 );
			}
		}
		return ret;
	}
#if 0
	else if()
	{
		unsigned char offsetW = 0xB, offsetR=0;
		unsigned char msb = 1;
		int d3pageidx,d3lmu,hmlc;
		MicronB95APageConv( Page, &d3pageidx, &d3lmu, &hmlc );
		if ( hmlc==1 ) {
			//Log(LOGDEBUG,"command not support hmlcx1\n");
			assert(0);
		}

		unsigned char v = (unsigned char)level;
#if 0
		unsigned char param[4];
		//enable super calibration
		//NewSort_FeatureFlashSet( ce, 0x96, 0x02 ); //enable Super Calibration
		// NewSort_FeatureFlashSet( ce, 0x96, 0x03 ); //enable Super Calibration
		// NewSort_SetMicronFlashMLBI(ce,0xFC,0x00,ce,0x01); //Super Calibration offset only
		// NewSort_GetMicronFlashMLBI(ce,0xFC,0x00,ce,param);
		// if ( param[0]!=0x01 )  {
		// Log(LOGDEBUG,"set mlbi fail %02X!=0x01\n",param[0]);
		// assert(0);
		// }


		assert(v!=0);

		NewSort_SetMicronFlashMLBI(ce,offsetW,msb,ce,v);
		//NewSort_GetMicronFlashMLBI(ce,offsetW,0x01,ce,param);
		//NewSort_SetMicronFlashMLBI(ce,offsetW,0x01,ce,v);
		//NewSort_GetMicronFlashMLBI(ce,offsetW,0x01,ce,param);
		//assert(v==param[0]);
		int d3pgidx = Page*3+2;
		d3pgidx++;
		d3pgidx++;
		//int ret = NewSort_DumpRead (ce,block,d3pgidx/3,buf,len,0x80|(d3pgidx%3),0 );
		int ret = NewSort_DumpRead (ce,block,d3pgidx/3,buf,len,0,0 );
		_CmdLog ( __FUNCTION__,"ret=%d\n", ret );
		NewSort_SetMicronFlashMLBI(ce,offsetW,mb,ce,0);
		//NewSort_SetMicronFlashMLBI(ce,0xFC,0x00,ce,0x00); //Super Calibration auto cal
		//NewSort_FeatureFlashSet( ce, 0x96, 0x00 ); //disable Super Calibration
#else
		unsigned char sqlMCR[] = {
            // 0,   1,   2,
			0xCA,0x4D,0x01,
            // 3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
			0xC2,0xEB,0xAB,0x2F,0xAB,0xF0,0xAB,0x00,0xDE,0x18,0xCB,
            //14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  22,  23,  24,
			0xC2,0xEF,0xAB,0xD1,0xDE,0x01,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xCB,
            //25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
			0xC2,0xEF,0xAB,0xD6,0xDE,0x07,0xDE,0x00,0xDE,0x00,0xDE,0x00,0xCB,
            //38,  39,  40,  41,  42,  43,  44,
			0xC0,0x00,0xA0,0xC1,0x30,0xCC,0xCB,
            //45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,
			0xC2,0xEF,0xAB,0xD1,0xDE,0x01,0xDE,0x01,0xDE,0x00,0xDE,0x00,0xCB,
            //58,  59,  60,  61,  62,  63,  64,  65,
			0xC0,0x05,0xAA,0xC1,0xE0,0xCC,0xDA,0x00,
            //66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,
			0xC2,0xEB,0xAB,0x2F,0xAB,0xF0,0xAB,0x00,0xDE,0x00,0xCB,
            //77,
			0xCE,
			0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0xAE,0xAF
		};
		int d3pgidx = Page*3+2;
		unsigned char *sql = sqlMCR;
		int slen = sizeof(sqlMCR);
		sql[6] = offsetW;
		sql[8] = msb;  //
		sql[71] = sql[6];
		sql[73] = sql[8];

		sql[10] = 0;
		sql[12] = v;
		sql[21] = 3;
		sql[32] = 0x0D + d3pgidx%3;
		sql[67] = len/1024;
		int page = d3pgidx/3;

		SetFTA(&sql[slen-8],page,block);
		int ret= Dev_IssueIOCmd(sql,slen, 2, ce, buf, len );
		_pNTCmd->nt_file_log ( __FUNCTION__,"ret=%d\n", ret );
		return ret;
#endif
	}
#endif
	else
	{
		assert(0);
	}
	return 0;
}

int YMTCSQLCmd::GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf )
{
	assert(0);
	return 0;
}

int YMTCSQLCmd::Get_RetryFeature( unsigned char ce, unsigned char *buf )
{
	assert(0);
	return 0;
}

/*
void MicronSQLCmd::MicronB95APageConv( int d1PageIdx, int *d3pageidx, int *d3lmu, int *hmlc )
{
    unsigned char isGen4 = _IsMicronB95AGen4(_GetFh());
 
    if ( d1PageIdx<2 ) {
        if ( isGen4 ) {
            *d3pageidx = d1PageIdx;
        }
        else {
            *d3pageidx = d1PageIdx+2;
        }
        *d3lmu = 3;
        if ( hmlc ) {
            *hmlc = 1;
        }
    }
    else if ( d1PageIdx>=766) {
        if ( isGen4 ) {
            *d3pageidx = d1PageIdx-766+2;
        }
        else {
            *d3pageidx = d1PageIdx-766+4;
        }
        *d3lmu = 3;
        if ( hmlc ) {
            *hmlc = 1;
        }
    }
    else if ( d1PageIdx>=758 && d1PageIdx<766 ) {
        int d3pgidx = (d1PageIdx-758);
        *d3pageidx = 252+d3pgidx/2;
        *d3lmu = d3pgidx&1;
        if ( hmlc ) {
            *hmlc = 2;
        }
    }
    else {
        int d3pgidx = (d1PageIdx-2);
        *d3pageidx = d3pgidx/3;
        *d3lmu = d3pgidx%3;
        if ( hmlc ) {
           *hmlc = 3;
        }
    }
}
*/
int YMTCSQLCmd::Micron_TLC_Page_to_CountOnePage(int page_num, unsigned char *Type)
{
	if (_fh->bIsMicronB16)
	{
		if ( page_num <= 11 )
		{
			*Type = 'S';
			return page_num;
		}
		else if ( page_num > 11 && page_num <= 35 )
		{
			*Type = 'M';
			if(( page_num - 12 ) % 2)
			{
				return 0xffff;
			}
			else
			{
				return (( page_num - 12 ) / 2);
			}
			
		}
		else if ( page_num > 35 && page_num <= 59 )
		{
			*Type = 'T';
			return (12 + page_num - 36) ;
		}
		else if ( page_num > 59 && page_num <= 2221 && ((page_num - 59) % 3) )
		{
			*Type = 'T';
			return 0xffff;
		}
		else if ( page_num > 61 && page_num <= 2219)
		{
			*Type = 'T';
			return (36 + ( page_num - 62 ) / 3) ;
		}
		else if ( page_num > 2223 && page_num <= 2269 && ((page_num - 2222) % 4) && ((page_num - 2223) % 4))
		{
			*Type = 'T';
			return 0xffff ;
		}
		else if ( page_num > 2221 && page_num <= 2267)
		{
			*Type = 'M';
			if((( page_num - 2222 ) % 4)==1)
			{
				return 0xffff;
			}
			else
			{
				return (756 + ( page_num - 2222 ) / 4);
			}
		}
		else if ( page_num > 2269 && page_num <= 2291)
		{
			*Type = 'T';
			return 0xffff;
		}
		else if ( page_num > 2291 && page_num <= 2303)
		{
			*Type = 'S';
			return page_num;
		}
		else
		{
			return 0xffff;
		}
	}
	return 0xffff;
}

int YMTCSQLCmd::XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len )
{	
	assert(0);
	return 0;
}

int YMTCSQLCmd::LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len )
{
	assert(0);
	return 0;
}

////////////////////////////Samsung////////////////////////
SamsungSQLCmd::SamsungSQLCmd()
{
}

SamsungSQLCmd::~SamsungSQLCmd()
{
}

int SamsungSQLCmd::GetFlashParam( unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value, unsigned char die )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::Erase(unsigned char ce,unsigned int block,bool bSLCMode,unsigned char planemode,bool bDFmode,char bWaitRDY, bool bAddr6)
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::Write(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len,unsigned char planemode, unsigned char ProgStep, bool bCache, bool bLastPage, char bWaitRDY, bool bAddr6)
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::Read(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode, bool bReadCmd, bool bReadData, bool bReadEcc, bool bCache, bool bFirstPage, bool bLastPage, bool bCmd36, char bWaitRDY, bool bAddr6)
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level, unsigned char* buf, int len, int ddmode, unsigned char precmd, bool pMLC, bool bSLC)
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::Get_RetryFeature( unsigned char ce, unsigned char *buf )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ) 
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	assert(0);
	return 0;
}
int SamsungSQLCmd::DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	assert(0);
	return 0;
}

int SamsungSQLCmd::XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len )
{	
	assert(0);
	return 0;
}

int SamsungSQLCmd::LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len )
{
	assert(0);
	return 0;
}