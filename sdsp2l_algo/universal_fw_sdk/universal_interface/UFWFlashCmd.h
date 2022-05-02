#ifndef _UFWSORTINGCMD_H_
#define _UFWSORTINGCMD_H_

#include "NetSCSICmd.h"

//sorting function.
class UFWFlashCmd : public NetSCSICmd
{
public:
    // init and create handle + create log file pointer from single app;
    UFWFlashCmd( DeviveInfo*, const char* logFolder );
    // init and create handle + system log pointer
    UFWFlashCmd( DeviveInfo*, FILE *logPtr );
    ~UFWFlashCmd();
    
	virtual int ST_EraseAll( int ce, int block, int mode );
	virtual int ST_DirectOneBlockErase(unsigned char obType,HANDLE DeviceHandle,unsigned char targetDisk,unsigned char bSLCMode,unsigned char bChipEnable,WORD wBLOCK,unsigned char bCEOffset,int len ,unsigned char *w_buf,int bDF);
	virtual int ST_DirectPageWrite(unsigned char obType,HANDLE DeviceHandle,unsigned char targetDisk,unsigned char WriteMode,unsigned char bCE
		,WORD wBLOCK,WORD wStartPage,WORD wPageCount,unsigned char CheckECC,unsigned char bRetry_Read,int a2command,int datalen ,int bOffset,unsigned char *w_buf,
		unsigned char TYPE,unsigned int Bank,unsigned char bIBmode,unsigned char bIBSkip, unsigned char bRRFailSkip);


private:
	void Init();
};

#endif /*_UFWSORTINGCMD_H_*/