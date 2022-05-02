#ifndef _P_CLASS_1_
#define _P_CLASS_1_

#pragma pack(1)
typedef struct _nes_update_format
{
	unsigned char ce;
	unsigned char reserve[15];
	char AppName[128];
	char KeyName[128];
	char Value[32];
} nes_update_format;
#pragma pack()


//===========================================================================
// Only for Block mode FW
/*
CMD 0x06 0x89 0x01, in 512 bytes
Byte[0] = 0: block mode / ??: page mode
Byte[1] = 0: mix mode disable / 1: mix mode enable

Byte[12] Byte[0xC]: uwVirBurstH佔幾bits
Byte[13] Byte[0xD]: uwVirZone佔幾bits
Byte[14] Byte[0xE]: uwVirBurstL佔幾bits
Byte[15] Byte[0xF]: ubVirUnit佔幾bits

以下變數都佔4 bytes
Byte[16~19]: total ce
Byte[20~23]: max IL
Byte[24~27]: total group
Byte[28~31]: planes per die
Byte[32~35]: sectors per burst
Byte[36~39]: sectors per page
Byte[40~43]: sectors per unit
Byte[44~47]: sectors per D1
Byte[48~51]: bursts per page
Byte[52~55]: bursts per unit
Byte[56~59]: bursts per D1
Byte[60~63]: pages per unit
Byte[64~67]: pages per D1
Byte[68~71]: MC num
Byte[72~75]: D1 retry num
Byte[76~79]: D3 retry num
Byte[256~384]: plane spare
*/


// 16+240+128+128
#pragma pack(1)
typedef struct _FW_FlashSizeInfo_Block	//512
{									// first 16
	BYTE ubBlockMode;				// 0: 0:block mode  1: page mode ?
	BYTE ubMixModeEnable;			// 1: mix mode disable / 1: mix mode enable
	BYTE ubM_Rev[10];				// wait add 

	BYTE ubBurstH;					// 4 bytes, bitmap (space) 
	BYTE ubZone;
	BYTE ubBurstL;
	BYTE ubUnit;

	DWORD ulTotalCE;				// 0, 16(4)  4*16=64, 240-64= 176 (rev )
	DWORD ulMaxIL;					// 1.20
	DWORD ulTotalGroup;				// 2, 24
	DWORD ulPlanesPerDie;			// 3,
	DWORD ulSectorsPerBurst;		// 4
	DWORD ulSectorsPerPage;			// 5
	DWORD ulSectorsPerUnit;
	DWORD ulSectorsPerDie;
	DWORD ulBurstsPerPage;
	DWORD ulBurstsPerUnit;
	DWORD ulBurstsPerD1;			// 10
	DWORD ulPagesPerUnit;	
	DWORD ulPagesPerD1;
	DWORD ulMcNum;
	DWORD ulD1RetryNum;
	DWORD ulD3RetryNum;				// 15
	DWORD ulPhyUnitPerGroup;
	DWORD ulTotalUserUnit;
	DWORD ulMaxGrtIndex;			// 20200109 add
	DWORD ulMaxD1FreeCount;			// 20200109 add	
	DWORD ulMaxGrPlaneMapCnt;		// 20200114 add		[ADD FW SCRIPT VAR]

	DWORD ulRev0;
	DWORD ulRev1;
	DWORD ulRev2;

	BYTE ubRev[144];				// [ADD FW SCRIPT VAR]	

	DWORD ulPlaneSpare[32];			// [256-383]: 128, 128/4 = 32
	DWORD ulS_Rev[32];				// [384-511]: 128, 128/4 = 32
} FW_FlashSizeInfo_Block;
#pragma pack()

#pragma pack(1)
typedef struct _FW_FlashSizeInfo_Page	//512
{									// first 16
	BYTE bFwMode;					// 0: 0:block mode  1: page mode ?	
	BYTE bMixMode;					// 1: mix mode disable / 1: mix mode enable
	BYTE bRes0[14];

	DWORD dwMaxCE;
	DWORD dwMaxILWay;
	DWORD dwSectorPerDataBuffer;
	DWORD dwMaxPartition;
	DWORD dwSectorPerBurst;
	DWORD dwSectorPerUnitPage;
	DWORD dwSectorPerPartition;
	DWORD dwSectorPerPool;
	DWORD dwSectorPerPlane;
	DWORD dwPlanePerPage;
	DWORD dwUnitPagePerBlock;
	DWORD dwSLCUnitPerBlock;
	DWORD dwUnitPagePerPartition;
	DWORD dwUnitPagePerPool;
	DWORD dwD1RetryNum;
	DWORD dwD3RetryNum;

	DWORD dwRev0;
	DWORD dwRev1;
	DWORD dwRev2;

	BYTE bRes1[164];

	DWORD dwSLCFreeQ;
	DWORD dwDataFreeQ;
	BYTE bRes2[248];
} FW_FlashSizeInfo_Page;
#pragma pack()

#pragma pack(1)
typedef struct _FW_FlashSizeInfo_AP //512
{
	// first 16 can not be used
	BYTE bAPMode;					// 0: 0:block mode    1: page mode     0x10: AP Get Information
	BYTE bRes0[15];

	DWORD dwMaxCE;
	DWORD dwMaxUnit;
	DWORD dwRes[14];

	DWORD dwRev0;
	DWORD dwRev1;
	DWORD dwRev2;

	BYTE bRes1[164];

	DWORD dwSLCFreeQ;
	DWORD dwDataFreeQ;
	BYTE bRes2[248];
} FW_FlashSizeInfo_AP;
#pragma pack()

#pragma pack(1)
typedef union FW_FlashSizeInfo {
	unsigned char bData[512];
	FW_FlashSizeInfo_Block b;
	FW_FlashSizeInfo_Page p;
} Data;
#pragma pack()

#endif