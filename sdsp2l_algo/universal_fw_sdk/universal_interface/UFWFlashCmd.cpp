
#include "UFWFlashCmd.h"


UFWFlashCmd::UFWFlashCmd( DeviveInfo* info, const char* logFolder )
:NetSCSICmd(info,logFolder)
{
    Init();
}

UFWFlashCmd::UFWFlashCmd( DeviveInfo* info, FILE *logPtr )
:NetSCSICmd(info,logPtr)
{
    Init();
}

void UFWFlashCmd::Init()
{

}

UFWFlashCmd::~UFWFlashCmd()
{
}

int UFWFlashCmd::ST_DirectPageWrite(unsigned char obType,HANDLE DeviceHandle,unsigned char targetDisk,unsigned char WriteMode,unsigned char bCE
	,WORD wBLOCK,WORD wStartPage,WORD wPageCount,unsigned char CheckECC,unsigned char bRetry_Read,int a2command,int datalen ,int bOffset,unsigned char *w_buf,
	unsigned char TYPE,unsigned int Bank,unsigned char bIBmode,unsigned char bIBSkip, unsigned char bRRFailSkip)
{
#if 0
	int iResult = 0;
	if(m_isSQL)
	{
		NewSort_SetPageSize ( datalen );
		iResult = _sqlCmd->NewSort_SQL_Write( bCE, wBLOCK, wStartPage, wPageCount, WriteMode, (_fh->beD3)?(bRetry_Read|0x40):(a2command?(bRetry_Read|0x40):bRetry_Read), w_buf, datalen);
		return iResult;
	}
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
	cdb[9] = bRetry_Read;
	if(a2command)
	{
		cdb[9] |= 0x40;
	}
	cdb[10] = wPageCount & 0xff; //WL
	cdb[11] = wPageCount >> 8; //WL
	cdb[12] = CheckECC;
	cdb[13] = bRRFailSkip;
	cdb[15] = bOffset; //CE Offset .. 決定哪顆ce 不要做Write

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
			iResult = NT_CustomCmd_Phy(DeviceHandle, cdb, 16 ,SCSI_IOCTL_DATA_OUT, datalen, w_buf, _SCSI_DelayTime);
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

/*
Erase a specify block
*/
int UFWFlashCmd::ST_DirectOneBlockErase(unsigned char obType,HANDLE DeviceHandle,unsigned char targetDisk,unsigned char bSLCMode,unsigned char bChipEnable,WORD wBLOCK,unsigned char bCEOffset,int len ,unsigned char *w_buf,int bDF)
{
	int rtn = 0;
	unsigned char cdb[16] = {0};
	cdb[0] = 0x06;
	cdb[1] = 0xf6;
	cdb[2] = 0xCE;
	cdb[3] = bSLCMode; //1 D1Mode 0xA2, 0 D3Mode 
	cdb[4] = bChipEnable; //CE
	cdb[6] = wBLOCK >> 8;
	cdb[5] = wBLOCK & 0xFF;
	if(bDF){
		cdb[9] = 0x04;
		cdb[10] = bDF;
	}
	cdb[15] = bCEOffset; //CE Offset .. 決定哪顆ce 不要做Write
	rtn = MyIssueCmd(cdb, true, NULL, 0, _timeOut, false);
	return rtn;
}
int UFWFlashCmd::ST_EraseAll( int ce, int block, int mode )
{
    //define sql command to do erase all.
	printf("UFWFlashCmd::ST_EraseAll\n");
	return 0;
}