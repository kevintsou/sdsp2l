#include "USBSCSICmd_ST.h"

#define _NEW_ECC_BUFFER_SIZE 16384
#define _NEW_ECC_BUFFER_SIZE_SQL (16384*16)

#define Parm_FlashType			m_apParameter[0]
#define Parm_FlashMaker			m_apParameter[1]
#define Parm_MaxCE    			m_apParameter[2]
#define Parm_MaxDie				m_apParameter[3]
#define Parm_MaxPlane			m_apParameter[4]
#define Parm_MaxBlock_H         m_apParameter[6]
#define Parm_MaxBlock_L         m_apParameter[7]
#define Parm_MaxPage_H          m_apParameter[8]
#define Parm_MaxPage_L          m_apParameter[9]

#define Parm_PageSize           m_apParameter[10]
#define Parm_FlashCellLevel     m_apParameter[11]
#define Parm_ECC	            m_apParameter[13]
#define Parm_FlashProcess       m_apParameter[14]
#define Parm_Spare	            m_apParameter[15]
#define Parm_ConversionMode     m_apParameter[16]
#define Parm_FlashMode	        m_apParameter[20]
#define Parm_FunctionMode       m_apParameter[21]
#define Parm_PatternMode		m_apParameter[22]
#define Parm_PageSize_H			m_apParameter[23]
#define Parm_PageSize_L			m_apParameter[24]
#define Parm_PageSize_Ext		m_apParameter[25]
#define Parm_PageSize_Dump		m_apParameter[26]
#define Parm_Dummy_Count		m_apParameter[27]
#define Parm_CopyBack_Read_DMA	m_apParameter[28]
#define Parm_SetFeature_FLH_DIV_CLK	m_apParameter[29]

#define Parm_EraseTimes         m_apParameter[32]
#define Parm_D1_ProgramTimes    m_apParameter[33]
#define Parm_RetryDelay         m_apParameter[34]
#define Parm_FunctionMode_1		m_apParameter[35]
#define Parm_StatusFailDelay	m_apParameter[36]
#define Parm_D3_ProgramTimes	m_apParameter[37]
#define Parm_FunctionMode_2		m_apParameter[38]
#define Parm_Write_ExtraDMA		m_apParameter[39]
#define Parm_FlashType_1		m_apParameter[40]
#define Parm_Multiple_RW		m_apParameter[41]

#define Parm_ContPage_Retry_Fail	m_apParameter[45]
#define Parm_CopyBack_Dummy_Pattern	m_apParameter[46]
#define Parm_CopyBack_Option		m_apParameter[47]

#define Parm_Measure_Busy  			m_apParameter[56]
#define Parm_FlashMode_1	        m_apParameter[59]
#define Parm_FunctionMode_3			m_apParameter[60]
#define Parm_DELAY_Before_WriteRead	m_apParameter[61]

#define Parm_Flash_ID_0			m_apParameter[64]
#define Parm_Flash_ID_1			m_apParameter[65]
#define Parm_Flash_ID_2			m_apParameter[66]
#define Parm_Flash_ID_3			m_apParameter[67]
#define Parm_Flash_ID_4			m_apParameter[68]
#define Parm_Flash_ID_5			m_apParameter[69]
#define Parm_Flash_ID_6			m_apParameter[70]
#define Parm_Flash_ID_7			m_apParameter[71]
#define Parm_ReadClock          m_apParameter[72]
#define Parm_WriteClock         m_apParameter[73]
#define Parm_DrivingLow         m_apParameter[74]
#define Parm_DrivingHigh        m_apParameter[75]
#define Parm_VoltageSetting     m_apParameter[76]
#define Parm_FlashSourceClock   m_apParameter[77]
#define Parm_FlashStatusTimer   m_apParameter[78]//for NES
#define Parm_StatusCheckBit  	m_apParameter[79]

#define Parm_Mode_512      		m_apParameter[90]
#define Parm_DMA_FOR_512		m_apParameter[91]

#if TwoSlot_Diff_MBC
#define Parm_Slot0_CE_Cnt 		m_apParameter[128]
#define Parm_Slot1_CE_Cnt 		m_apParameter[129]
#define Parm_Slot0_MBC			m_apParameter[130]
#define Parm_Slot1_MBC 			m_apParameter[131]
#define Parm_Slot0_ECC 			m_apParameter[132]
#define Parm_Slot1_ECC 			m_apParameter[133]
#endif

#define Parm_Pretest_DMA1K_Status_byPlane 		m_apParameter[142]

int GetFrameCntPerPageByController(int deviceType, FH_Paramater * fh)
{
	int connector = (deviceType&INTERFACE_MASK);
	int controller = (deviceType&CONTROLLER_MASK);
	if (DEVICE_PCIE_5017 == controller || DEVICE_PCIE_5013 == controller || DEVICE_PCIE_5019 == controller || DEVICE_PCIE_5012 == controller || DEVICE_SATA_3111 == controller )
		return fh->PageSizeK/4;

	if (IF_USB == connector)
		return fh->PageSizeK;

	return fh->PageSizeK;
}
int USBSCSICmd_ST::ST_DirectOneBlockErase_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bSLCMode,BYTE bChipEnable,WORD wBLOCK,BYTE bCEOffset,int SectorCount ,unsigned char *w_buf,int bDf)
{
	unsigned char planeMode=1,stauts[_MAX_CE_];
	int rtn;

	if(ST_EMODE_2PLANE == (bSLCMode & ST_EMODE_2PLANE))
	{
		planeMode = 2;
		if(ST_EMODE_4PLANE == (bSLCMode & ST_EMODE_4PLANE))
		{
			planeMode = 4;
		}
	}
	if ( ST_EMODE_SLC == (bSLCMode & ST_EMODE_SLC)) {
		
		rtn=GetSQLCmd()->PlaneEraseD1( bChipEnable, wBLOCK, planeMode);
		if(rtn)
		{
			return rtn;
		}
		if((Parm_FunctionMode&0x08)==0)	// check status
		{
			GetSQLCmd()->ReturnFlashStatus(&stauts[0]);
			if (stauts[bChipEnable]&0x0F)
			{
				nt_file_log("Block %d  Status: 0x%X", wBLOCK , stauts[bChipEnable]);
				return 1;
			}
			return 0;
		}
	}
	else {
		rtn=GetSQLCmd()->PlaneErase( bChipEnable, wBLOCK, planeMode);
		if(rtn)
		{
			return rtn;
		}

		if((Parm_FunctionMode&0x08)==0)	// check status
		{
			GetSQLCmd()->ReturnFlashStatus(&stauts[0]);
			if (stauts[bChipEnable]&0x0F)
			{
				nt_file_log("Block %d  Status: 0x%X", wBLOCK , stauts[bChipEnable]);
				return 1;
			}
			return 0;
		}
	}
	return 0;
}
int USBSCSICmd_ST::ST_DirectPageRead_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
						  ,WORD wBLOCK,WORD wStartPage,WORD bPageCount,BYTE bECCBit,BYTE Retry_Read,BYTE bOffset,int datalen ,unsigned char *w_buf, bool bNewEccFormat)
{
	unsigned int pageCnt = bPageCount, Loop, LoopCnt = 1, RetryLen, eccbuf[16], page, rtn = 0;
	unsigned char planeMode = 1, Frame = 0, FrameCnt =_FH.PageSizeK, FrameSizeK =1;
	unsigned char tmpbuf[_NEW_ECC_BUFFER_SIZE_SQL] = {0}, tmpwbuf[_NEW_ECC_BUFFER_SIZE] = {0};
	bool cacheEnable = false,bRetryPass = false;
	bool retryRead = (Parm_FunctionMode&0x02)?true:false;
	bool bSLCMode = ((Retry_Read&0x40)||(_FH.beD3));
	#define ECCB                                   ((unsigned char *)tmpbuf)
	#define ECCDW                                   ((unsigned int *)tmpbuf)

	int connector = (_deviceType&INTERFACE_MASK);
	memset(eccbuf, 0xff, sizeof(eccbuf));
	if(retryRead==0)
	{
		retryRead = (Retry_Read&0x01);
	}
	FrameCnt = GetFrameCntPerPageByController(_deviceType, &_FH);
	FrameSizeK = _FH.PageSizeK/FrameCnt;


	if(ST_RWMODE_2PLANE == (ReadMode & ST_RWMODE_2PLANE))
	{
		planeMode = 2;
		if(ST_RWMODE_4PLANE == (ReadMode & ST_RWMODE_4PLANE))
		{
			planeMode = 4;
		}
	}
	if(ST_RWMODE_CACHE == (ReadMode & ST_RWMODE_CACHE))
	{
		cacheEnable = true;
	}
	if(retryRead)
	{
		LoopCnt =_Retry_RT[0];
		RetryLen = _Retry_RT[1];
		planeMode = 1;
		cacheEnable = false;
	}
	if (ST_RWMODE_CONVER ==  (ReadMode & ST_RWMODE_CONVER) && 1 == bPageCount )
	{//read data with ecc protected aka group read
		Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, wStartPage, false, _userSelectLDPC_mode, 'C');
		for(Loop=0;Loop<LoopCnt;Loop++)
		{
			if(retryRead)
			{
				GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2+(Loop*RetryLen)]);
			}
			if(bSLCMode)
			{
				rtn = GetSQLCmd()->GroupReadDataD1(bCE, wBLOCK, wStartPage, 1/**pagecnt*/, w_buf, datalen ,planeMode, cacheEnable, retryRead );
			}
			else
			{
				rtn = GetSQLCmd()->GroupReadData(bCE, wBLOCK, wStartPage, 1/**pagecnt*/, w_buf, datalen ,planeMode, cacheEnable, retryRead );
			}
			if(retryRead)
			{
				// check retry pass or fail is key point
				_ReadDDR(tmpbuf,512, 2 );	// Read Ecc back
				// Change Ecc after retry by tmpbuf
				// UpdateEccbyFrame(eccbuf,tmpbuf);
				for(Frame=0;Frame<FrameCnt;Frame++)
				{
					if(FrameSizeK==1)
					{
						if(eccbuf[Frame]>ECCB[Frame])
						{
							eccbuf[Frame]=ECCB[Frame];
						}
					}
					else
					{
						if(eccbuf[Frame]>ECCDW[Frame])
						{
							eccbuf[Frame]=ECCDW[Frame];
						}
					}
				}
				// check all frame Ecc < bEccbit End retry
				// if(CheckFrameEcc(eccbuf,bEccbit))
				//{
				//	break;
				//}
				bRetryPass = true;
				for(Frame=0;Frame<FrameCnt;Frame++)
				{
		
					if(eccbuf[Frame]>bECCBit)
					{
						bRetryPass = false;
					}
					else
					{
						// Ecc pass
						// copy Pass data to w_buf
						memcpy(&w_buf[Frame*FrameSizeK*1024],&tmpwbuf[Frame*FrameSizeK*1024],FrameSizeK*1024);
					}
				}
				if(bRetryPass)
				{
					break;
				}
			}
		}
		// restore retry value to default
		if(retryRead)
		{
			if(Loop)	// Loop 0  retry pass , no need restore again
			{
				GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2]);
			}
		}
		else
		{
			memcpy(w_buf,tmpwbuf,_FH.PageSizeK*1024);
		}
		return rtn;
	}
	else if (ST_RWMODE_CONVER ==  (ReadMode & ST_RWMODE_CONVER) && 1 != bPageCount)
	{//read ecc per pages
		int plane,planeoffset;
		unsigned char bMaxEccBit;

		if(retryRead)
		{
			for(page=wStartPage;page<pageCnt;page++)
			{
				if(Check_PageMapping_Bit(bCE,page,page))
				{
					continue;
				}
				memset(eccbuf, 0xff, sizeof(eccbuf));
				for(Loop=0;Loop<LoopCnt;Loop++)
				{
					if(retryRead)
					{
						GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2+(Loop*RetryLen)]);
					}
					Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, page, false, _userSelectLDPC_mode, 'C');
					if(bSLCMode)
					{
						rtn = GetSQLCmd()->GroupReadEccD1( bCE, wBLOCK,  page, 1, &tmpbuf[page*_FH.PageSizeK], _FH.PageSizeK ,planeMode, cacheEnable, retryRead );
					}
					else
					{
						rtn = GetSQLCmd()->GroupReadEcc( bCE, wBLOCK,  page, 1, &tmpbuf[page*_FH.PageSizeK], _FH.PageSizeK ,planeMode, cacheEnable, retryRead );
					}
					
					// check retry pass or fail is key point
					// Change Ecc after retry by tmpbuf
					// UpdateEccbyFrame(eccbuf,tmpbuf);
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
						if(FrameSizeK==1)
						{
							if(eccbuf[Frame]>ECCB[(page*FrameSizeK)+Frame])
							{
								eccbuf[Frame]=ECCB[(page*FrameSizeK)+Frame];
							}
						}
						else
						{
							if(eccbuf[Frame]>ECCDW[(page*FrameSizeK)+Frame])
							{
								eccbuf[Frame]=ECCDW[(page*FrameSizeK)+Frame];
							}
						}
					}
					// check all frame Ecc < bEccbit End retry
					// if(CheckFrameEcc(eccbuf,bEccbit))
					//{
					//	break;
					//}
					bRetryPass = true;
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
			
						if(eccbuf[Frame]>bECCBit)
						{
							bRetryPass = false;
						}
					}
					if(bRetryPass)
					{
						break;
					}
				}
				
				// restore retry value to default
				if(Loop)	// Loop 0  retry pass , no need restore again
				{
					GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2]);
				}
				if(FrameSizeK==1)
				{
					memcpy(&ECCB[(page*FrameSizeK)+Frame],eccbuf,FrameSizeK);
				}
				else
				{
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
						ECCDW[(page*FrameSizeK)+Frame] = eccbuf[Frame];
					}
				}
			}
		}
		else
		{
			Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, wStartPage, false, _userSelectLDPC_mode, 'C');
			if(bSLCMode)
			{
				rtn = GetSQLCmd()->GroupReadEccD1( bCE, wBLOCK,  wStartPage, pageCnt, tmpbuf, _FH.PageSizeK ,planeMode, cacheEnable, retryRead );
			}
			else
			{
				rtn = GetSQLCmd()->GroupReadEcc( bCE, wBLOCK,  wStartPage, pageCnt, tmpbuf, _FH.PageSizeK ,planeMode, cacheEnable, retryRead );
			}
		}
		
		memset(w_buf,0xf9,_NEW_ECC_BUFFER_SIZE);
		if(bNewEccFormat)
		{
			planeoffset=_NEW_ECC_BUFFER_SIZE/planeMode;
			for(plane=0;plane<planeMode;plane++)
			{
				for(page=0;page<bPageCount;page++)
				{
					bMaxEccBit=0;
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
						if(FrameSizeK==1)
						{
							if(ECCB[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame]>bMaxEccBit)
							{
								bMaxEccBit = ECCB[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame];
							}
						}
						else
						{
							if(ECCDW[(plane*pageCnt*_FH.PageSizeK)+(page*_FH.PageSizeK)+Frame]>bMaxEccBit)
							{
								bMaxEccBit = ECCDW[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame];
							}
						}
					}			
					w_buf[(plane*planeoffset)+page] = bMaxEccBit;
				}
			}
		}
		else
		{
			memcpy(&w_buf[2048],tmpbuf,_NEW_ECC_BUFFER_SIZE-2048);
		}
		return rtn;
	}
	else if ( 0 ==  (ReadMode & ST_RWMODE_CONVER) && 1== bPageCount)
	{//raw read 1 page
		// Dump read only retry once  
		if(retryRead)
		{
			GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2+RetryLen]);
		}
		Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, wStartPage, false, _userSelectLDPC_mode, 'C');
		if(bSLCMode)
		{
			rtn = GetSQLCmd()->DumpReadD1( bCE, wBLOCK,  wStartPage, pageCnt, w_buf, datalen ,planeMode, cacheEnable, retryRead );
		}
		else
		{
			rtn = GetSQLCmd()->DumpRead( bCE, wBLOCK,  wStartPage, pageCnt, w_buf, datalen ,planeMode, cacheEnable, retryRead );
		}
		// restore retry value to default
		if(retryRead)
		{
			GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2]);
		}
		return rtn;
	}
	else
	{
		assert(0);
	}
	return -1;
}
int USBSCSICmd_ST::ST_DirectPageRead_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
									   ,WORD wBLOCK,WORD wStartPage,WORD bPageCount,BYTE bECCBit,BYTE Retry_Read,int a2command,BYTE bOffset,int datalen ,unsigned char *w_buf, bool bNewEccFormat)
{
	unsigned int pageCnt = bPageCount, Loop, LoopCnt = 1, RetryLen, eccbuf[16], page,rtn = 0;
	unsigned char planeMode = 1, Frame = 0, FrameCnt =_FH.PageSizeK, FrameSizeK = 1;
	unsigned char tmpbuf[_NEW_ECC_BUFFER_SIZE_SQL] = {0}, tmpwbuf[_NEW_ECC_BUFFER_SIZE] = {0};
	bool cacheEnable = false,bRetryPass = false;
	bool retryRead = (Parm_FunctionMode&0x02)?true:false;
	bool bSLCMode = ((Retry_Read&0x40)||(_FH.beD3));
	#define ECCB                                   ((unsigned char *)tmpbuf)
	#define ECCDW                                   ((unsigned int *)tmpbuf)

	int connector = (_deviceType&INTERFACE_MASK);
	memset(eccbuf, 0xff, sizeof(eccbuf));
	if(retryRead==0)
	{
		retryRead = (Retry_Read&0x01);
	}
	FrameCnt = GetFrameCntPerPageByController(_deviceType, &_FH);
	FrameSizeK = _FH.PageSizeK/FrameCnt;
	if(ST_RWMODE_2PLANE == (ReadMode & ST_RWMODE_2PLANE))
	{
		planeMode = 2;
		if(ST_RWMODE_4PLANE == (ReadMode & ST_RWMODE_4PLANE))
		{
			planeMode = 4;
		}
	}
	if(ST_RWMODE_CACHE == (ReadMode & ST_RWMODE_CACHE))
	{
		cacheEnable = true;
	}
	if(retryRead)
	{
		LoopCnt =_Retry_RT[0];
		RetryLen = _Retry_RT[1];
		planeMode = 1;
		cacheEnable = false;
	}
	Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, wStartPage, false, _userSelectLDPC_mode, 'C');
	// SQL used converion check RAW write/read or not , but ST need check Spare=0x00 
	if (ST_RWMODE_CONVER ==  (ReadMode & ST_RWMODE_CONVER) && 1 == bPageCount )
	{//read data with ecc protected aka group read

		for(Loop=0;Loop<LoopCnt;Loop++)
		{
			if(retryRead)
			{
				GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2+(Loop*RetryLen)]);
			}
			if(bSLCMode)
			{
				rtn = GetSQLCmd()->GroupReadDataD1(bCE, wBLOCK, wStartPage, pageCnt, tmpwbuf, datalen ,planeMode, cacheEnable, retryRead );
			}
			else
			{
				rtn = GetSQLCmd()->GroupReadData(bCE, wBLOCK, wStartPage, pageCnt, tmpwbuf, datalen ,planeMode, cacheEnable, retryRead );
			}
			if(retryRead)
			{
				// check retry pass or fail is key point
				_ReadDDR(tmpbuf,512, 2 );	// Read Ecc back
				// Change Ecc after retry by tmpbuf
				// UpdateEccbyFrame(eccbuf,tmpbuf);
				for(Frame=0;Frame<FrameCnt;Frame++)
				{
					if(FrameSizeK==1)
					{
						if(eccbuf[Frame]>ECCB[Frame])
						{
							eccbuf[Frame]=ECCB[Frame];
						}
					}
					else
					{
						if(eccbuf[Frame]>ECCDW[Frame])
						{
							eccbuf[Frame]=ECCDW[Frame];
						}
					}
				}
				// check all frame Ecc < bEccbit End retry
				// if(CheckFrameEcc(eccbuf,bEccbit))
				//{
				//	break;
				//}
				bRetryPass = true;
				for(Frame=0;Frame<FrameCnt;Frame++)
				{
		
					if(eccbuf[Frame]>bECCBit)
					{
						bRetryPass = false;
					}
					else
					{
						// Ecc pass
						// copy Pass data to w_buf
						memcpy(&w_buf[Frame*FrameSizeK*1024],&tmpwbuf[Frame*FrameSizeK*1024],FrameSizeK*1024);
					}
				}
				if(bRetryPass)
				{
					break;
				}
			}
		}
		// restore retry value to default
		if(retryRead)
		{
			if(Loop)	// Loop 0  retry pass , no need restore again
			{
				GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2]);
			}
		}
		else
		{
			memcpy(w_buf,tmpwbuf,_FH.PageSizeK*1024);
		}
		return rtn;
	}
	else if (ST_RWMODE_CONVER ==  (ReadMode & ST_RWMODE_CONVER) && 1 != bPageCount)
	{//read ecc per pages
		int plane,planeoffset;
		unsigned char bMaxEccBit;

		if(retryRead)
		{
			for(page=wStartPage;page<pageCnt;page++)
			{
				if(Check_PageMapping_Bit(bCE,page,page))
				{
					continue;
				}
				memset(eccbuf, 0xff, sizeof(eccbuf));
				for(Loop=0;Loop<LoopCnt;Loop++)
				{
					if(retryRead)
					{
						GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2+(Loop*RetryLen)]);
					}
					Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, page, false, _userSelectLDPC_mode, 'C');
					if(bSLCMode)
					{
						rtn = GetSQLCmd()->GroupReadEccD1( bCE, wBLOCK,  page, 1, &tmpbuf[page*_FH.PageSizeK], _FH.PageSizeK ,planeMode, cacheEnable, retryRead );
					}
					else
					{
						rtn = GetSQLCmd()->GroupReadEcc( bCE, wBLOCK,  page, 1, &tmpbuf[page*_FH.PageSizeK], _FH.PageSizeK ,planeMode, cacheEnable, retryRead );
					}
					
					// check retry pass or fail is key point
					// Change Ecc after retry by tmpbuf
					// UpdateEccbyFrame(eccbuf,tmpbuf);
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
						if(FrameSizeK==1)
						{
							if(eccbuf[Frame]>ECCB[(page*FrameSizeK)+Frame])
							{
								eccbuf[Frame]=ECCB[(page*FrameSizeK)+Frame];
							}
						}
						else
						{
							if(eccbuf[Frame]>ECCDW[(page*FrameSizeK)+Frame])
							{
								eccbuf[Frame]=ECCDW[(page*FrameSizeK)+Frame];
							}
						}
					}
					// check all frame Ecc < bEccbit End retry
					// if(CheckFrameEcc(eccbuf,bEccbit))
					//{
					//	break;
					//}
					bRetryPass = true;
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
			
						if(eccbuf[Frame]>bECCBit)
						{
							bRetryPass = false;
						}
					}
					if(bRetryPass)
					{
						break;
					}
				}
				
				// restore retry value to default
				if(Loop)	// Loop 0  retry pass , no need restore again
				{
					GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2]);
				}
				if(FrameSizeK==1)
				{
					memcpy(&ECCB[(page*FrameSizeK)+Frame],eccbuf,FrameSizeK);
				}
				else
				{
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
						ECCDW[(page*FrameSizeK)+Frame] = eccbuf[Frame];
					}
				}
			}
		}
		else
		{
			if(bSLCMode)
			{
				rtn = GetSQLCmd()->GroupReadEccD1( bCE, wBLOCK,  wStartPage, pageCnt, tmpbuf, _FH.PageSizeK ,planeMode, cacheEnable, retryRead );
			}
			else
			{
				rtn = GetSQLCmd()->GroupReadEcc( bCE, wBLOCK,  wStartPage, pageCnt, tmpbuf, _FH.PageSizeK ,planeMode, cacheEnable, retryRead );
			}
		}
		
		memset(w_buf,0xf9,_NEW_ECC_BUFFER_SIZE);
		if(bNewEccFormat)
		{
			planeoffset=_NEW_ECC_BUFFER_SIZE/planeMode;
			for(plane=0;plane<planeMode;plane++)
			{
				for(page=0;page<bPageCount;page++)
				{
					bMaxEccBit=0;
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
						if(FrameSizeK==1)
						{
							if(ECCB[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame]>bMaxEccBit)
							{
								bMaxEccBit = ECCB[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame];
							}
						}
						else
						{
							if(ECCDW[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame]>bMaxEccBit)
							{
								bMaxEccBit = ECCDW[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame];
							}
						}
					}			
					w_buf[(plane*planeoffset)+page] = bMaxEccBit;
				}
			}
		}
		else
		{
			memcpy(&w_buf[2048],tmpbuf,_NEW_ECC_BUFFER_SIZE-2048);
		}
		return rtn;
	}
	else if ( 0 ==  (ReadMode & ST_RWMODE_CONVER) && 1== bPageCount)
	{//raw read 1 page
		// Dump read only retry once
		if(retryRead)
		{
			GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2+RetryLen]);
		}
		if(bSLCMode)
		{
			rtn = GetSQLCmd()->DumpReadD1( bCE, wBLOCK,  wStartPage, pageCnt, w_buf, datalen ,planeMode, cacheEnable, retryRead );
		}
		else
		{
			rtn = GetSQLCmd()->DumpRead( bCE, wBLOCK,  wStartPage, pageCnt, w_buf, datalen ,planeMode, cacheEnable, retryRead );
		}
		// restore retry value to default
		if(retryRead)
		{
			GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2]);
		}
		return rtn;
	}
	else
	{
		assert(0);
	}
	return -1;
}
int USBSCSICmd_ST::ST_DirectPageWrite_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode,BYTE bCE
						   ,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE CheckEcc,BYTE bRetry_Read,int a2commnad,int datalen,int bOffset ,unsigned char *w_buf,
						   BYTE TYPE,DWORD Bank, BYTE bIBmode, BYTE bIBSkip, BYTE bRRFailSkip)
{
	unsigned int pageCnt = 1;
	unsigned char planeMode=1,tmpwbuf[_NEW_ECC_BUFFER_SIZE];
	bool cacheEnable = false;
	int rtn;

	if(ST_RWMODE_2PLANE == (WriteMode & ST_RWMODE_2PLANE))
	{
		planeMode = 2;
		if(ST_RWMODE_4PLANE == (WriteMode & ST_RWMODE_4PLANE))
		{
			planeMode = 4;
		}
	}	
	if(ST_RWMODE_CACHE == (WriteMode & ST_RWMODE_CACHE))
	{
		cacheEnable = true;
	}
	if (ST_RWMODE_CONVER ==  (WriteMode & ST_RWMODE_CONVER))
	{
		unsigned char patternMode=0;// random data

		//write read only for none one page test
		if((ST_RWMODE_WR_PER_PAGE == (WriteMode & ST_RWMODE_WR_PER_PAGE))&&(wPageCount!=1))	// write/read
		{
			// disable cache
			// write not improve new Ecc mode
			// write & read
			cacheEnable = false;

			for(int pg=wStartPage;pg<wPageCount;pg++)
			{
				if((a2commnad)||(_FH.beD3))
				{
					Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, wStartPage, true, _userSelectLDPC_mode, 'C');
					rtn=GetSQLCmd()->GroupWriteD1( bCE, wBLOCK, pg, pageCnt, tmpwbuf, _FH.PageSizeK*1024, patternMode, planeMode, cacheEnable );
					if(rtn)
					{
						return rtn;
					}
					Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, wStartPage, false, _userSelectLDPC_mode, 'C');
					rtn=GetSQLCmd()->GroupReadEccD1(bCE, wBLOCK, pg, pageCnt, &w_buf[2048+pg*_FH.PageSizeK], _fh->PageSizeK, planeMode, cacheEnable, false);
					if(rtn)
					{
						return rtn;
					}
				}
				else
				{
					rtn=GetSQLCmd()->GroupWrite( bCE, wBLOCK, pg, pageCnt, tmpwbuf, _FH.PageSizeK*1024, patternMode, planeMode, cacheEnable );
					if(rtn)
					{
						return rtn;
					}
					rtn=GetSQLCmd()->GroupReadEcc(bCE, wBLOCK, pg, pageCnt, &w_buf[2048+pg*_FH.PageSizeK], _FH.PageSizeK, planeMode, cacheEnable, false);
					if(rtn)
					{
						return rtn;
					}
				}
			}
		}
		else
		{
			Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, wStartPage, true, _userSelectLDPC_mode, 'C');
			//Only Write
			patternMode=2;//user selected in buffer
			if(wPageCount==1)
			{
				if(datalen<(_FH.PageSizeK*1024))
				{
					memcpy(tmpwbuf,w_buf,datalen);
					memcpy(&tmpwbuf[datalen],w_buf,((_FH.PageSizeK*1024)-datalen));
				}
				else
				{
					memcpy(tmpwbuf,w_buf,(_FH.PageSizeK*1024));
				}
			}
			int total_page_cnt = _FH.PagePerBlock *  (_FH.Cell==0x08?3:4);
			int page_size = _FH.PageSizeK*1024;
			unsigned char * block_buffer = new unsigned char[page_size * total_page_cnt];
			for (int page_idx = 0; page_idx<total_page_cnt; page_idx++)
			{
				memcpy(&block_buffer[page_idx*page_size], w_buf, page_size );
			}
			if((a2commnad)||(_FH.beD3))
			{
				return GetSQLCmd()->GroupWriteD1( bCE, wBLOCK, wStartPage, wPageCount, block_buffer, _FH.PageSizeK*1024, patternMode, planeMode, cacheEnable );
			}
			else
			{
				return GetSQLCmd()->GroupWrite( bCE, wBLOCK, wStartPage, wPageCount, block_buffer, _FH.PageSizeK*1024, patternMode, planeMode, cacheEnable );
			}
			if (block_buffer)
				delete [] block_buffer;
		}
	}
	else// raw write used AP buffer,multi page need send multi page buffer
	{
		if((a2commnad)||(_FH.beD3))
		{
			return GetSQLCmd()->DumpWriteD1( bCE, wBLOCK, wStartPage, wPageCount, w_buf, datalen, planeMode, cacheEnable );
		}
		else
		{
			return GetSQLCmd()->DumpWrite( bCE, wBLOCK, wStartPage, wPageCount, w_buf, datalen, planeMode, cacheEnable );
		}
	}
	return -1;
}
int USBSCSICmd_ST::ST_DirectOneBlockRead_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE ReadMode,BYTE bCE
							  ,WORD wBLOCK,WORD wStartPage,WORD wPageCount,BYTE SkipSector,BYTE CheckECCBit,BYTE patternBase,int len ,unsigned char *w_buf,int lowPageEcc, bool bNewEccFormat)
{
	unsigned int pageCnt = wPageCount, Loop, LoopCnt = 1, RetryLen, eccbuf[16], page, rtn = 0;
	unsigned char planeMode = 1, Frame = 0, FrameCnt = _FH.PageSizeK, FrameSizeK = 1;
	unsigned char tmpbuf[_NEW_ECC_BUFFER_SIZE_SQL] = {0}, tmpwbuf[_NEW_ECC_BUFFER_SIZE] = {0};
	bool cacheEnable = false,bRetryPass = false;
	bool retryRead = (Parm_FunctionMode&0x02)?true:false;
	bool bSLCMode = ((SkipSector&0x40)||(_FH.beD3));
	#define ECCB                                   ((unsigned char *)tmpbuf)
	#define ECCDW                                   ((unsigned int *)tmpbuf)

	int connector = (_deviceType&INTERFACE_MASK);
	Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, wStartPage, false, _userSelectLDPC_mode, 'C');
	memset(eccbuf, 0xff, sizeof(eccbuf));
	if(retryRead==0)
	{
		retryRead = (SkipSector&0x01);
	}
	FrameCnt = GetFrameCntPerPageByController(_deviceType, &_FH);
	FrameSizeK = _FH.PageSizeK/FrameCnt;
	if(ST_RWMODE_2PLANE == (ReadMode & ST_RWMODE_2PLANE))
	{
		planeMode = 2;
		if(ST_RWMODE_4PLANE == (ReadMode & ST_RWMODE_4PLANE))
		{
			planeMode = 4;
		}
	}	
	if(ST_RWMODE_CACHE == (ReadMode & ST_RWMODE_CACHE))
	{
		cacheEnable = true;
	}
	if(retryRead)
	{
		LoopCnt =_Retry_RT[0];
		RetryLen = _Retry_RT[1];
		planeMode = 1;
		cacheEnable = false;
	}
	// SQL used converion check RAW write/read or not , but ST need check Spare=0x00 
	if (ST_RWMODE_CONVER ==  (ReadMode & ST_RWMODE_CONVER) && 1 == wPageCount )
	{//read data with ecc protected aka group read
		int page_size_alignK = ((_FH.PageSize+1023)/1024)*1024;
		CharMemMgr readPageBuffer(page_size_alignK);
		for(Loop=0;Loop<LoopCnt;Loop++)
		{
			if(retryRead)
			{
				GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2+(Loop*RetryLen)]);
			}
		
			rtn = GetSQLCmd()->GroupReadData(bCE, wBLOCK, wStartPage, pageCnt, readPageBuffer.Data(), len ,planeMode, cacheEnable, retryRead );
			
			if(retryRead)
			{
				// check retry pass or fail is key point
				_ReadDDR(tmpbuf,512, 2 );	// Read Ecc back
				// Change Ecc after retry by tmpbuf
				// UpdateEccbyFrame(eccbuf,tmpbuf);
				for(Frame=0;Frame<FrameCnt;Frame++)
				{
					if(FrameSizeK==1)
					{
						if(eccbuf[Frame]>ECCB[Frame])
						{
							eccbuf[Frame]=ECCB[Frame];
						}
					}
					else
					{
						if(eccbuf[Frame]>ECCDW[Frame])
						{
							eccbuf[Frame]=ECCDW[Frame];
						}
					}
				}
				// check all frame Ecc < bEccbit End retry
				// if(CheckFrameEcc(eccbuf,bEccbit))
				//{
				//	break;
				//}
				bRetryPass = true;
				for(Frame=0;Frame<FrameCnt;Frame++)
				{
		
					if(eccbuf[Frame]>CheckECCBit)
					{
						bRetryPass = false;
					}
					else
					{
						// Ecc pass
						// copy Pass data to w_buf
						memcpy(&w_buf[Frame*FrameSizeK*1024],&tmpwbuf[Frame*FrameSizeK*1024],FrameSizeK*1024);//may leak?
					}
				}
				if(bRetryPass)
				{
					break;
				}
			}
		}
		// restore retry value to default
		if(retryRead)
		{
			if(Loop)	// Loop 0  retry pass , no need restore again
			{
				GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2]);
			}
		}
		else
		{
			memcpy(w_buf,readPageBuffer.Data() ,_FH.PageSizeK*1024);
		}
	}
	else if (ST_RWMODE_CONVER ==  (ReadMode & ST_RWMODE_CONVER) && 1 != wPageCount)
	{//read ecc per pages
		int plane,planeoffset;
		unsigned char bMaxEccBit;
		
		if (GetPrivateProfileInt("Main", "SQL_READ_ONE_PAGE_ECC", 0, _ini_file_name))
		{
			wPageCount=2;
		}
		if(retryRead)
		{
			for(page=wStartPage;page<pageCnt;page++)
			{
				if(Check_PageMapping_Bit(bCE,page,page))
				{
					continue;
				}
				memset(eccbuf, 0xff, sizeof(eccbuf));
				for(Loop=0;Loop<LoopCnt;Loop++)
				{
					if(retryRead)
					{
						GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2+(Loop*RetryLen)]);
					}
					Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, page, false, _userSelectLDPC_mode, 'C');
					rtn = GetSQLCmd()->GroupReadEcc( bCE, wBLOCK,  page, 1, &tmpbuf[page*_FH.PageSizeK], _FH.PageSizeK ,planeMode, cacheEnable, retryRead );

					// check retry pass or fail is key point
					// Change Ecc after retry by tmpbuf
					// UpdateEccbyFrame(eccbuf,tmpbuf);
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
						if(FrameSizeK==1)
						{
							if(eccbuf[Frame]>ECCB[(page*FrameSizeK)+Frame])
							{
								eccbuf[Frame]=ECCB[(page*FrameSizeK)+Frame];
							}
						}
						else
						{
							if(eccbuf[Frame]>ECCDW[(page*FrameSizeK)+Frame])
							{
								eccbuf[Frame]=ECCDW[(page*FrameSizeK)+Frame];
							}
						}
					}
					// check all frame Ecc < bEccbit End retry
					// if(CheckFrameEcc(eccbuf,bEccbit))
					//{
					//	break;
					//}
					bRetryPass = true;
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
			
						if(eccbuf[Frame]>CheckECCBit)
						{
							bRetryPass = false;
						}
					}
					if(bRetryPass)
					{
						break;
					}
				}
				
				// restore retry value to default
				if(Loop)	// Loop 0  retry pass , no need restore again
				{
					GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2]);
				}
				if(FrameSizeK==1)
				{
					memcpy(&ECCB[(page*FrameSizeK)+Frame],eccbuf,FrameSizeK);
				}
				else
				{
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
						ECCDW[(page*FrameSizeK)+Frame] = eccbuf[Frame];
					}
				}
			}
		}
		else
		{
			rtn = GetSQLCmd()->GroupReadEcc( bCE, wBLOCK,  wStartPage, pageCnt, tmpbuf, _FH.PageSizeK ,planeMode, cacheEnable, retryRead );
		}
		
		memset(w_buf,0xf9,_NEW_ECC_BUFFER_SIZE);
		if(bNewEccFormat)
		{
			planeoffset=_NEW_ECC_BUFFER_SIZE/planeMode;
			for(plane=0;plane<planeMode;plane++)
			{
				for(page=0;page<wPageCount;page++)
				{
					bMaxEccBit=0;
					for(Frame=0;Frame<FrameCnt;Frame++)
					{
						if(FrameSizeK==1)
						{
							if(ECCB[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame]>bMaxEccBit)
							{
								bMaxEccBit = ECCB[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame];
							}
						}
						else
						{
							if(ECCDW[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame]>bMaxEccBit)
							{
								bMaxEccBit = ECCDW[(plane*pageCnt*FrameSizeK)+(page*FrameSizeK)+Frame];
							}
						}
					}			
					w_buf[(plane*planeoffset)+page] = bMaxEccBit;
				}
			}
		}
		else
		{
			memcpy(&w_buf[2048],tmpbuf,_NEW_ECC_BUFFER_SIZE-2048);
		}
		return rtn;
	}
	else if ( 0 ==  (ReadMode & ST_RWMODE_CONVER) && 1== wPageCount)
	{//raw read 1 page
		// Dump read only retry once  
		if(retryRead)
		{
			GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2+RetryLen]);
		}
		rtn = GetSQLCmd()->DumpRead( bCE, wBLOCK,  wStartPage, pageCnt, w_buf, len ,planeMode, cacheEnable, retryRead );
		// restore retry value to default
		if(retryRead)
		{
			rtn = GetSQLCmd()->SetRetryValue(bCE, bSLCMode, RetryLen, &_Retry_RT[2]);
		}
		return rtn;
	}
	else
	{
		assert(0);
	}
	return -1;
	
}
int USBSCSICmd_ST::ST_DirectOneBlockWrite_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE WriteMode
							   ,BYTE bCE,WORD wBLOCK,BYTE SkipSector,int SectorCount ,unsigned char *w_buf,BYTE bDontDirectWriteCEMask)
{
	unsigned char planeMode=1,*tmpwbuf;
	int tmplen; 
	CharMemMgr buf;
	bool cacheEnable = false;

	if(ST_RWMODE_2PLANE == (WriteMode & ST_RWMODE_2PLANE))
	{
		planeMode = 2;
		if(ST_RWMODE_4PLANE == (WriteMode & ST_RWMODE_4PLANE))
		{
			planeMode = 4;
		}
	}	
	if(ST_RWMODE_CACHE == (WriteMode & ST_RWMODE_CACHE))
	{
		cacheEnable = true;
	}
	Dev_SQL_GroupWR_Setting(_userSelectBlockSeed, 0, true, _userSelectLDPC_mode, 'C');
	// D3 Write one block , no support write only one page
	if (ST_RWMODE_CONVER ==  (WriteMode & ST_RWMODE_CONVER))
	{
		//buf.Resize(_FH.PageSizeK*1024);
		//tmpwbuf = buf.Data();
		int total_page_cnt = _FH.PagePerBlock *  (_FH.Cell==0x08?3:4);
		int page_size = _FH.PageSizeK*1024;
		unsigned char * block_buffer = new unsigned char[page_size * total_page_cnt];
		for (int page_idx = 0; page_idx<total_page_cnt; page_idx++)
		{
			memcpy(&block_buffer[page_idx*page_size], w_buf, page_size );
		}
		int rtn = GetSQLCmd()->GroupWrite( bCE, wBLOCK, 0, 0, block_buffer, (_FH.PageSizeK*1024), 2/*2 for user select buf*/, planeMode, cacheEnable );
		delete [] block_buffer;
		return rtn;
	}
	else
	{
		tmplen = ((_FH.PageSize+1023)/1024)*1024;
		if(w_buf==NULL)
		{
			buf.Resize(_FH.MaxPage*(LogTwo(_fh->Cell))*tmplen);
			buf.Rand(false);
			tmpwbuf = buf.Data();
		}
		else
		{
			tmpwbuf=w_buf;
		}
		return GetSQLCmd()->DumpWrite( bCE, wBLOCK, 0, 0, tmpwbuf, tmplen, planeMode, cacheEnable );
	}
	return -1;
}
int USBSCSICmd_ST::ST_HynixTLC16ModeSwitch_SQL(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,BYTE bMode)
{
	return GetSQLCmd()->SetBootMode( bMode );
}

int USBSCSICmd_ST::ST_GetSetRetryTable_SQL(BYTE bGetSet,BYTE bSetOnfi,BYTE *r_buf, int length,BYTE ce,BYTE bRetryCount, BYTE OTPAddress)
{
	int i;

	if( bGetSet )	// Get retry table
	{
		if( bGetSet == 1 )
		{
			memcpy(r_buf,_Retry_RT,sizeof(_Retry_RT));

		}
		else if( bGetSet == 2 )
		{
			// improve later
			//GetSQLCmd()->GetOTP( ce, die, bSLCMode, r_buf );
			//Get_Hynix_RRT_From_OTP( X_CMDBLOCK[4] ); // Hynix - CDB[4] = '0' : is Legacy ( ID Byte6 ,bite 7 is 0)
		}                                                // Hynix - CDB[4] = '1' : is ONFI   ( ID Byte6 ,bite 7 is 1)
		else if( bGetSet == 3 )
		{
			//GetSQLCmd()->Get_RetryFeature(); // Hynix - CDB[4] = '0' : is Legacy ( ID Byte6 ,bite 7 is 0)
		
		}                                                // Hynix - CDB[4] = '1' : is ONFI   ( ID Byte6 ,bite 7 is 1)
	}
	else
	{
		memcpy(_Retry_RT,r_buf,sizeof(_Retry_RT));
	
		for( i = 0; i < 128; i++ )
		{
			_Retry_Order[i] = i;
		}
		/*
		if( bRetryCount > 0 )		//if		cdb5=0 retry table|3¡¦X2O¡¦Nretry¡¦X|¡M, 	//¢DO?e¢Dufor page write read
		{
			//else 	cdb5=1¢Duretry1|¡M ¢DH|1At¡ÓA
			guc_RetryCnt_ByCDB5 = bRetryCount;
		}
		else
		{
			guc_RetryCnt_ByCDB5 = 0;
		}
		*/
	}
	return 0;
}

int USBSCSICmd_ST::ST_SetGet_Parameter_SQL(void)
{
	GetSQLCmd()->SetFlashCmdBuf(&m_apParameter[144]);
	
	GetSQLCmd()->SetWaitRDYMode(Parm_FunctionMode&0x08);// wait RDY
	GetSQLCmd()->SetSkipWriteDMA(Parm_FunctionMode_3&0x08);	// Skip Write DMA
	
	if( _Init_GMP )
	{
		memset( _PageMap, 0x00, 1024 );
		memset( _Good_WL, 0x00, 1024 );
	}
	return 0;
}

int USBSCSICmd_ST::ST_DirectGetFlashStatus_SQL(unsigned char ce,unsigned char *buf,unsigned char cmd)
{
	for(int CE=0;CE<_FH.FlhNumber;CE++)
	{
		if((ce!=_FH.FlhNumber)||(_FH.FlhNumber==1))
		{
			if(_FH.FlhNumber==1)
			{
				CE = 0;
			}
			else
			{
				CE = ce;
			}
		}
		GetSQLCmd()->GetStatus(CE, &buf[CE], cmd);
		if((ce!=_FH.FlhNumber)||(_FH.FlhNumber==1))
		{
			break;
		}
	}
	return 0;
}

int USBSCSICmd_ST::ST_GetReadWriteStatus_SQL(unsigned char *w_buf)
{
	unsigned char sbuf[_MAX_CE_];// for 16 CE , firt 7 byte for ce 0~7 last 7 byte for 8~16
	int ce;
	memset(w_buf,0x00,14);
	// return buffer bit if 1==>CE fail
	GetSQLCmd()->ReturnFlashStatus(sbuf);
	for(ce=0;ce<_FH.FlhNumber;ce++)
	{
		if(sbuf[ce]!=0xE0)
		{
			if(ce<8)
			{
				w_buf[0] |= (0x01<<ce);
			}
			else
			{
				w_buf[7] |= (0x01<<(ce-8));
			}
			// for multi plane (add later)
			//ST_DirectGetFlashStatus_SQL(ce,sbuf[ce],0x74);
		}
	}
	return 0;
}

int USBSCSICmd_ST::ST_PageBitMapGetSet_SQL(BYTE bGetSet,BYTE Die,BYTE *w_buf)
{
	unsigned int i;

	if( bGetSet )
	{
		memcpy(w_buf,_PageMap,sizeof(_PageMap));
	}
	else
	{
		//Init_GMP = 0;
		memcpy(_PageMap,w_buf,sizeof(_PageMap));
	
		_Init_GMP = 1;

		for( i = 0; i < 1024; i++ )
		{
			if( _PageMap[i] )
			{
				_Init_GMP = 0;
				break;
			}
		}

		//Page_Map_Init();
	}
	return 0;
}

int USBSCSICmd_ST::ST_Set_pTLCMode_SQL(void)
{
	unsigned char ce, die;

	for(ce=0;ce<_FH.FlhNumber;ce++)
	{
		for( die = 0; die < _FH.DieNumber; die++ )
		{
			if(GetSQLCmd()->ParameterOverLoad( ce, die, 0x17)==0)
			{	
				GetSQLCmd()->SetpTLC(true);
			}
			else
			{
				GetSQLCmd()->CLESend( ce, 0xF1+die, false );
				GetSQLCmd()->CLESend( ce, 0xFD, true );

				if(GetSQLCmd()->ParameterOverLoad( ce, die, 0x1B)==0)
				{
					GetSQLCmd()->SetpTLC(true);
				}
				else
				{
					GetSQLCmd()->SetpTLC(false);
					//StatusFail[0] = 1;	// need check how to return status
					return 1;
				}
			}
		}
	}
	return 0;
}
// mode 0:	none data command
//		1:	Write command
//		2:	Read Data
//		3:	Read Ecc
int USBSCSICmd_ST::ST_DirectSend_SQL(const unsigned char* sql,int sqllen, int mode, unsigned char ce, unsigned char* buf, int len )
{
	return GetSQLCmd()->Dev_IssueIOCmd( sql, sqllen, mode, ce, buf, len );
}
int USBSCSICmd_ST::ST_Send_Dist_read_SQL(int ce, WORD wBlock,WORD WL,WORD VTH,PUCHAR pucBuffer,UCHAR pagesize,int is3D,bool isSLC,unsigned char ShiftReadLevel)
{
	int ddmode = 0;
	if (is3D)
		ddmode=3;
	else
		ddmode=1;

	if(!ShiftReadLevel)
	{
		return GetSQLCmd()->Count1Tracking(ce, wBlock, WL, VTH, pucBuffer, pagesize*1024, ddmode, 0xff, false, isSLC);
	}
	else
	{
		unsigned char value[7],retrylen=7;
		int rtn;
		memset(value,0x00,sizeof(value));
		if(isSLC)
		{
			retrylen = 4;
			value[0]=VTH;
		}
		else
		{		
			// ABCDEFG==>Level 0~6
			// 89 A C E G
			// 8A B D F
			value[(((ShiftReadLevel-1)%2)*4)+((ShiftReadLevel-1)/2)]=VTH;
		}
		rtn = GetSQLCmd()->SetRetryValue( ce, isSLC, retrylen, value, wBlock );
		if (rtn) return rtn;

		if(isSLC)
		{
			rtn = GetSQLCmd()->DumpReadD1( ce, wBlock,  WL, 1, pucBuffer, pagesize*1024 ,1, false, false );
		}
		else
		{
			rtn = GetSQLCmd()->DumpRead( ce, wBlock,  WL, 1, pucBuffer, pagesize*1024 ,1, false, false );
		}
		if(((isSLC &&(ShiftReadLevel==1))||(ShiftReadLevel==7))&&(VTH==(124)))	// Last VT, set feature to default
		{
			memset(value,0x00,sizeof(value));
			GetSQLCmd()->SetRetryValue( ce, isSLC, retrylen, value, wBlock );
		}
		return rtn;
	}
}
unsigned char USBSCSICmd_ST::Check_PageMapping_Bit( unsigned char ce, unsigned int page_num, unsigned int page_count )
{
	UCHAR offset = 0;

	// for support more page
	if( _FH.MaxPage <= 1024 )		// Support 8CE
	{
		offset = 7;
	}
	else if( _FH.MaxPage <= 2048 )	// Support 4CE
	{
		offset = 8;
	}
	else if( _FH.MaxPage <= 4096 )	// Support 2CE
	{
		offset = 9;
	}
	else if( _FH.MaxPage <= 8192 )	// Only support 1CE
	{
		offset = 10;
	}

	return ( ( _PageMap[( ce << offset ) + ( page_num >> 3 )] ) & ( 0x01 << ( page_count & 0x07 ) ) );
}