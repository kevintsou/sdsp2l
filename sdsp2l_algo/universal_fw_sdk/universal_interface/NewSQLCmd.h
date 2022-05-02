#ifndef _NEWSQLCMD_H_
#define _NEWSQLCMD_H_

#pragma once

#include "UFWSCSICmd.h"
#include "MemManager.h"
#define _MAX_CE_ 16
class UFWSCSICmd;
class SQLCmd
{
public:
    ~SQLCmd();
    //use this command to new sql object.
    static SQLCmd* Instance(UFWSCSICmd* scsi, FH_Paramater *fh, unsigned char sqlECCAddr, unsigned char sqlDATAAddr);
	virtual int Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, unsigned char ce, unsigned char* buf, int len );
    void SetDumpSQL( int );
    int SetBootMode( unsigned char bMode );
	int SetFlashCmdBuf( unsigned char *buf );
	int SetWaitRDYMode( unsigned char bMode );
	int SetDFMode( unsigned char bMode );
	int SetpTLC( unsigned char bMode );
	int SetSkipWriteDMA( unsigned char bMode );
	int ReturnFlashStatus(unsigned char* buf);
    ////////////////////////for sorting only//////////////////////////////////
    //write/read 裡面的mode 可以決定是做dump or group , 還有要不要開cache.
	virtual int Erase(unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6=0)=0;
	virtual int Write(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode=1, unsigned char ProgStep=0, bool bCache=0, bool bLastPage=0, char bWaitRDY=1, bool bAddr6=0)=0;
	virtual int Read(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode, bool bReadCmd, bool bReadData, bool bReadEcc, bool bCache, bool bFirstPage, bool bLastPage, bool bCmd36, char bWaitRDY, bool bAddr6)=0;
    ////////////////////////for sorting and VTH//////////////////////////////////
	virtual int GetStatus(unsigned char ce, unsigned char* buf, unsigned char cmd, bool bSaveStatus=1);
	virtual int OnlyWiatRDY(unsigned char ce);
	virtual int FlashReset(unsigned char ce,unsigned char cmd=0xff);
	virtual int GetFlashParam(unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die=0 )=0;
	virtual int SetFlashParam( unsigned char ce, unsigned int offset, unsigned char v1, unsigned char die=0 )=0;
	virtual int SetFeature( unsigned char ce, unsigned char offset, unsigned char *value, bool bToggle=false, char die=-1 );
	virtual int GetFeature( unsigned char ce, unsigned char offset, unsigned char *value, bool bToggle=false, char die=-1 );
	virtual int ParameterOverLoad( unsigned char ce, unsigned char die, unsigned char offset );
	virtual int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 )=0;
	virtual int GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf )=0;
	virtual int Get_RetryFeature( unsigned char ce, unsigned char *buf )=0;
	virtual int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0; 
	virtual int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0;
	virtual int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	////////////////////////for VTH only//////////////////////////////////
	virtual int Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd=0xff, bool pMLC=false, bool bSLC=false )=0;	
    virtual int SetVendorMode ( unsigned char ce );
    virtual int CloseVendorMode ( unsigned char ce );
    virtual int GetProgramLoop ( int ce, int* loop );
	virtual int CLESend ( unsigned char ce, unsigned char value, bool waitrdy=true );
	virtual int GetFlashID ( unsigned char ce, unsigned char* ibuf );
	virtual int XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len )=0;
	virtual int LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len )=0;

protected:
    ////////////////////////for internal only//////////////////////////////////
	virtual int GetPhyBlkFromLogBlk(int block);
	virtual void SetFTA(unsigned char* fta, int page, int logicalblock, int maxpage=0 );
	virtual int LogTwo(int n);

protected:
	UFWSCSICmd*	_scsi;
    FH_Paramater *_fh;
    SQLCmd();
    unsigned char _eccBufferAddr;
	unsigned char _dataBufferAddr;
	unsigned char _BootMode;
	unsigned char *_CmdBuf;
	unsigned char _status[_MAX_CE_];
    bool _bAddr6;
	char _bWaitRDY;
	bool _bDFmode;
	bool _bpTLC;
	bool _bSkipWDMA;
	
private:    
    int _dumpSQL;
};

class TSBSQLCmd : public SQLCmd
{
public:
    ////////////////////////for sorting only//////////////////////////////////
    //write/read 裡面的mode 可以決定是做dump or group , 還有要不要開cache.
	int Erase(unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6=0);
	int Write(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode=1, unsigned char ProgStep=0, bool bCache=0, bool bLastPage=0, char bWaitRDY=1, bool bAddr6=0);
	int Read(unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode, bool bReadCmd, bool bReadData, bool bReadEcc, bool bCache, bool bFirstPage, bool bLastPage, bool bCmd36, char bWaitRDY, bool bAddr6);
    ////////////////////////for sorting and VTH//////////////////////////////////
	int GetFlashParam( unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die=0 );
	int SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value, unsigned char die=0 );
	int GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf );
	int Get_RetryFeature( unsigned char ce, unsigned char *buf );

	virtual int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 )=0;
	virtual int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0; 
	virtual int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0;
	virtual int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	////////////////////////for VTH only//////////////////////////////////
	virtual int Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd, bool pMLC, bool bSLC=false );
	virtual int SetVendorMode ( unsigned char ce );
	virtual int CloseVendorMode ( unsigned char ce );
	virtual int GetProgramLoop ( int ce, int* loop ); 
	virtual int NewSort_SetVendorMode2 ( unsigned char ce );
	virtual int GetFlashID ( unsigned char ce, unsigned char* ibuf );
	virtual int XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len );
	virtual int LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len );
	
protected:
    ////////////////////////for internal only//////////////////////////////////

protected:
    TSBSQLCmd();
    ~TSBSQLCmd();
	bool _CHK_VENDOR_MODE;    
};

class TSBD2_SQLCmd : public TSBSQLCmd
{
public:

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    
protected:
	friend class SQLCmd;
    TSBD2_SQLCmd(FH_Paramater *fh);
    ~TSBD2_SQLCmd();
};

class TSBD3_SQLCmd : public TSBSQLCmd
{
public:

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
	virtual unsigned char Get_WL_And_Step( unsigned int idx,unsigned char *Step );
protected:
	friend class SQLCmd;
    TSBD3_SQLCmd(FH_Paramater *fh);
    ~TSBD3_SQLCmd();
};

class TSB3D_SQLCmd : public TSBSQLCmd
{
public:

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    
protected:
	friend class SQLCmd;
    TSB3D_SQLCmd(FH_Paramater *fh);
    ~TSB3D_SQLCmd();
};

class MicronSQLCmd : public SQLCmd
{
public:
	////////////////////////for sorting only//////////////////////////////////
	//write/read ﹐I-±aomode ￥i￥H‥Mcw?O°μdump or group , AU|3-n?￡-n?}cache.
	int Erase( unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6=0);
	int Write( unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode=1, unsigned char ProgStep=0, bool bCache=0, bool bLastPage=0, char bWaitRDY=1, bool bAddr6=0);
	int Read( unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode=1, bool bReadCmd=1, bool bReadData=0, bool bReadEcc=0, bool bCache=0, bool bFirstPage=1, bool bLastPage=0, bool bCmd36=0, char bWaitRDY=1, bool bAddr6=0 );
	////////////////////////for sorting and VTH//////////////////////////////////
	int GetFlashParam( unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die=0 );
	int SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value, unsigned char die=0 );
	int GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf );
	int Get_RetryFeature( unsigned char ce, unsigned char *buf );

	virtual int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 )=0;
	virtual int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0; 
	virtual int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0;
	virtual int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	int Micron_TLC_Page_to_CountOnePage( int page_num, unsigned char *Type );
	//int MicronB95APageConv( int d1PageIdx, int *d3pageidx, int *d3lmu, int *hmlc );
	////////////////////////for VTH only//////////////////////////////////
	int Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd=0xff, bool pMLC=false, bool bSLC=false);
	virtual int XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len );
	virtual int LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len );

protected:
    ////////////////////////for internal only//////////////////////////////////

protected:
    MicronSQLCmd();
    ~MicronSQLCmd();
};

class IMD2_SQLCmd : public MicronSQLCmd
{
public:

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    
private:
	friend class SQLCmd;
    IMD2_SQLCmd(FH_Paramater *fh);
    ~IMD2_SQLCmd();
};

class IMD3_SQLCmd : public MicronSQLCmd
{
public:

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    
private:
	friend class SQLCmd;
    IMD3_SQLCmd(FH_Paramater *fh);
    ~IMD3_SQLCmd();
};

class IM3D_SQLCmd : public MicronSQLCmd
{
public:

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:

private:
	friend class SQLCmd;
    IM3D_SQLCmd(FH_Paramater *fh);
    ~IM3D_SQLCmd();
};

class HynixSQLCmd : public SQLCmd
{
public:

    ////////////////////////for sorting only//////////////////////////////////
    //write/read 裡面的mode 可以決定是做dump or group , 還有要不要開cache.

	int Erase( unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6=0 );
	int Write( unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode=1, unsigned char ProgStep=0, bool bCache=0, bool bLastPage=0, char bWaitRDY=1,bool bAddr6=0 );
	int Read( unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode=1, bool bReadCmd=1, bool bReadData=0, bool bReadEcc=0, bool bCache=0, bool bFirstPage=1, bool bLastPage=0, bool bCmd36=0, char bWaitRDY=1, bool bAddr6=0 );

    ////////////////////////for sorting and VTH//////////////////////////////////

	int GetFlashParam( unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die=0 );
	int SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value , unsigned char die=0 );
	int Get_RetryFeature( unsigned char ce, unsigned char *buf );
	
	virtual int GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf )=0;
	virtual int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 )=0;
	virtual int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0; 
	virtual int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0;
	virtual int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;

	////////////////////////for VTH only//////////////////////////////////
	int Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd=0xff, bool pMLC=false, bool bSLC=false);
	virtual int XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len );
	virtual int LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len );
	
protected:
    
protected:
	friend class SQLCmd;
    HynixSQLCmd();
    ~HynixSQLCmd();
};

class HYXD2_SQLCmd : public HynixSQLCmd
{
public:

	int GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf );
	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    
protected:
	friend class SQLCmd;
    HYXD2_SQLCmd(FH_Paramater *fh);
    ~HYXD2_SQLCmd();
};

class HYXD3_SQLCmd : public HynixSQLCmd
{
public:

	int GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf );
	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    virtual unsigned char  Get_WL_And_Step( unsigned int idx,unsigned char *Step );
protected:
	friend class SQLCmd;
    HYXD3_SQLCmd(FH_Paramater *fh);
    ~HYXD3_SQLCmd();
};

class HYX3D_SQLCmd : public HynixSQLCmd
{
public:

	int GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf );
	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    
protected:
	friend class SQLCmd;
    HYX3D_SQLCmd(FH_Paramater *fh);
    ~HYX3D_SQLCmd();
};

class YMTCSQLCmd : public SQLCmd
{
public:
	////////////////////////for sorting only//////////////////////////////////
	//write/read ﹐I-±aomode ￥i￥H‥Mcw?O°μdump or group , AU|3-n?￡-n?}cache.
	int Erase( unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6=0);
	int Write( unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode=1, unsigned char ProgStep=0, bool bCache=0, bool bLastPage=0, char bWaitRDY=1, bool bAddr6=0);
	int Read( unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode=1, bool bReadCmd=1, bool bReadData=0, bool bReadEcc=0, bool bCache=0, bool bFirstPage=1, bool bLastPage=0, bool bCmd36=0, char bWaitRDY=1, bool bAddr6=0 );
	////////////////////////for sorting and VTH//////////////////////////////////
	int GetFlashParam( unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die=0 );
	int SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value, unsigned char die=0 );
	int GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf );
	int Get_RetryFeature( unsigned char ce, unsigned char *buf );

	virtual int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 )=0;
	virtual int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode )=0;
	virtual int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )=0;
	virtual int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0; 
	virtual int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )=0;
	virtual int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	virtual int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )=0;
	int Micron_TLC_Page_to_CountOnePage( int page_num, unsigned char *Type );
	//int MicronB95APageConv( int d1PageIdx, int *d3pageidx, int *d3lmu, int *hmlc );
	////////////////////////for VTH only//////////////////////////////////
	int Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd=0xff, bool pMLC=false, bool bSLC=false);
	virtual int XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len );
	virtual int LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len );

protected:
    ////////////////////////for internal only//////////////////////////////////

protected:
    YMTCSQLCmd();
    ~YMTCSQLCmd();
};

class YMTCD2_SQLCmd : public YMTCSQLCmd
{
public:

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    
private:
	friend class SQLCmd;
    YMTCD2_SQLCmd(FH_Paramater *fh);
    ~YMTCD2_SQLCmd();
};

class YMTCD3_SQLCmd : public YMTCSQLCmd
{
public:

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    
private:
	friend class SQLCmd;
    YMTCD3_SQLCmd(FH_Paramater *fh);
    ~YMTCD3_SQLCmd();
};

class YMTC3D_SQLCmd : public YMTCSQLCmd
{
public:

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

protected:
    
private:
	friend class SQLCmd;
    YMTC3D_SQLCmd(FH_Paramater *fh);
    ~YMTC3D_SQLCmd();
};

class SamsungSQLCmd : public SQLCmd
{
public:
    ////////////////////////for sorting only//////////////////////////////////
    //write/read 裡面的mode 可以決定是做dump or group , 還有要不要開cache.

	int Erase( unsigned char ce, unsigned int block, bool bSLCMode, unsigned char planemode, bool bDFmode, char bWaitRDY, bool bAddr6=0 );
	int Write( unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, const unsigned char* buf, int len, unsigned char planemode=1, unsigned char ProgStep=0, bool bCache=0, bool bLastPage=0, char bWaitRDY=1, bool bAddr6=0 );
	int Read( unsigned char ce, unsigned int block, unsigned int page, bool bConvs, bool bSLCMode, unsigned char* buf, int len, unsigned char planemode=1, bool bReadCmd=1, bool bReadData=0, bool bReadEcc=0, bool bCache=0, bool bFirstPage=1, bool bLastPage=0, bool bCmd36=0, char bWaitRDY=1, bool bAddr6=0 );

    ////////////////////////for sorting and VTH//////////////////////////////////

	int GetFlashParam( unsigned char ce, unsigned int offset, unsigned char *value, unsigned char die=0 );
	int SetFlashParam( unsigned char ce, unsigned int offset, unsigned char value, unsigned char die=0 );
	int GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf );
	int Get_RetryFeature( unsigned char ce, unsigned char *buf );

	int SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block=0 );
	int PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode );
	int PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode );
	int DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache );
	int GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache ); 
	int GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache );
	int DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );
	int GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry );

	////////////////////////for VTH only//////////////////////////////////
	int Count1Tracking ( unsigned char ce, unsigned int block,unsigned int Page, unsigned int level,unsigned char* buf, int len, int ddmode, unsigned char precmd=0xff, bool pMLC=false, bool bSLC=false );
	virtual int XnorCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf1, unsigned char *buf2, int len );
	virtual int LevelIndicatedCheck ( unsigned char ce, unsigned char die, unsigned int block, unsigned int Page, unsigned char *buf, unsigned char Level, int len );

protected:
    
protected:
	friend class SQLCmd;
    SamsungSQLCmd();
    ~SamsungSQLCmd();
};

#endif 
