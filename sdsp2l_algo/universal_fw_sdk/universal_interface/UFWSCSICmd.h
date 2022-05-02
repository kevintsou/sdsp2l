#ifndef _UFWSCSICMD_H_
#define _UFWSCSICMD_H_


#include "UFWInterface.h"
#include <assert.h>
#include <stdio.h>
#include "MyFlashInfo.h"
#include "NewSQLCmd.h"



#define DEVICE_MASK 0x00FF0000
enum ATADEVICE_TYPE { DEVICE_UNKNOWN=0, DEVICE_USB=0x10000, DEVICE_EMMC=0x20000,  DEVICE_SATA=0x40000, DEVICE_PCIE=0x80000 };

#define CONTROLLER_MASK 0x0000FFFF
extern const int DEVICE_SATA_3110;
extern const int DEVICE_SATA_3111;
extern const int DEVICE_PCIE_5007;
extern const int DEVICE_PCIE_5008;
extern const int DEVICE_PCIE_5012;
extern const int DEVICE_PCIE_5013;
extern const int DEVICE_PCIE_5016;
extern const int DEVICE_PCIE_5017;
extern const int DEVICE_PCIE_5019;
extern const int DEVICE_PCIE_5020;

class SQLCmd;


class UFWSCSICmd : public UFWInterface
{
public:
    // init and create handle + create log file pointer from single app;
    UFWSCSICmd( DeviveInfo*, const char* logFolder );
    // init and create handle + system log pointer
    UFWSCSICmd( DeviveInfo*, FILE *logPtr );
    ~UFWSCSICmd();
#if 0
	static int ScanDevice(int useBridge, int startidx=0);
#endif
	static DeviveInfo* GetDevice( int );
	static void _static_FreeDevices();
	static int _static_Inquiry ( int physical, unsigned char* r_buf, bool all, unsigned char length = 0);
	//static int IdentifyATA( int physical, unsigned char * buf, int len, int bridge );
	static int _static_IdentifyNVMe( int physical, unsigned char * buf, int len, int bridge );

	//////////////////////////////////////
    virtual int Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len )=0;	
	virtual int Dev_InitDrive( const char* ispfile )=0;		
	virtual int Dev_GetFlashID(unsigned char* r_buf)=0;
    virtual int Dev_CheckJumpBackISP ( bool needJump, bool bSilence )=0;
    virtual int Dev_GetSRAMData ( unsigned char offset, unsigned char* buf, int len )=0;
    virtual int Dev_SetSRAMData ( unsigned char offset, const unsigned char* buf, int len )=0;
    //////////////////////////////////////
    virtual int Dev_SetFlashClock ( int timing );
    virtual int Dev_GetRdyTime ( int action, unsigned int* busytime );
	virtual int Dev_SetFlashDriving ( int driving );
	virtual int Dev_SetEccMode ( unsigned char Ecc );
	virtual int Dev_SetFWSpare ( unsigned char Spare );
	virtual int Dev_SetFWSpareTag ( unsigned char* Tag );
	virtual int Dev_SetFWSeed ( int page,int block=0x8299 );
	virtual int Dev_SetFlashPageSize ( int pagesize );
	virtual int Dev_SQL_GroupWR_Setting(int blockSeed, int pageSeed, bool isWrite, int LdpcMode, char Spare);
	void UserSelectLDPC_mode(int ldpc);
	void UserSelectBlockSeed(int block_seed);
	int GetSelectLDPC_mode();
	int GetSelectBlockSeed();

	//////////////////////////////////////
    int GetFlashID(unsigned char* r_buf);
	bool Enabled();	
	SQLCmd* GetSQLCmd();
	FH_Paramater* GetParam();
	int InitFlashInfoBuf(const char *fn, const unsigned char* buf, long len, int frameK);
protected:
	char _fwVersion[8]; 
    char _ispFile[512];
    ////////////////////////////////////
    unsigned char _eccBufferAddr;
	unsigned char _dataBufferAddr;
	int _userSelectLDPC_mode;
	int _userSelectBlockSeed;
	int _channelCE;
	int _workingChannels;
    SQLCmd* _sql;
    FH_Paramater _FH;
	

private:
    bool _enabled;
    void Init();
};

#endif /*_UFWSCSICMD_H_*/