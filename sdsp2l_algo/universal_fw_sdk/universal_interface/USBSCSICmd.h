#ifndef _USBSCSICMD_H_
#define _USBSCSICMD_H_

#include "UFWFlashCmd.h"

// commands for flash
#define ISP_JUMP_OK    0x9c
#define ISP_CONFIG_OK  0x55
#define ISP_PROG_OK    0xA5

class USBSCSICmd : public UFWFlashCmd
{
public:
    // init and create handle + create log file pointer from single app;
    USBSCSICmd( DeviveInfo*, const char* logFolder );
    // init and create handle + system log pointer
    USBSCSICmd( DeviveInfo*, FILE *logPtr );
    ~USBSCSICmd();
    int Inquiry ( HANDLE hUsb, unsigned char* r_buf, bool all, unsigned char length=0 );
	static BOOL MyDeviceIoControl(HANDLE hDevice,DWORD dwIoControlCode,void* lpInBuffer, DWORD nInBufferSize,void* lpOutBuffer
		, DWORD nOutBufferSize,LPDWORD lpBytesReturned,LPOVERLAPPED lpOverlapped);
    virtual int Dev_InitDrive( const char* ispfile );
    virtual int Dev_GetFlashID(unsigned char* r_buf);
    virtual int Dev_CheckJumpBackISP ( bool needJump, bool bSilence );
    virtual int Dev_GetSRAMData ( unsigned char offset, unsigned char* buf, int len );
    virtual int Dev_SetSRAMData ( unsigned char offset, const unsigned char* buf, int len );
    virtual int Dev_SetFlashClock ( int timing );
    virtual int Dev_GetRdyTime ( int action, unsigned int* busytime );
	virtual int Dev_SetFlashDriving ( int driving );
	virtual int Dev_SetEccMode ( unsigned char Ecc );
	virtual int Dev_SetFWSpare ( unsigned char Spare );
    virtual int Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len );
	virtual void Dev_GenSpare_32(unsigned char *buf,unsigned char mode,unsigned char FrameNo,unsigned char Section, int buf_len);
	virtual int MyIssueCmdHandle(  HANDLE hUsb, unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout,int deviceType=IF_USB, int cdb_length=16 );
	virtual int MyIssueCmd(  unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout=20 );
	virtual int MyIssueATACmd(unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd);
	virtual int MyIssueNVMeCmd( unsigned char *nvmeCmd,unsigned char OPCODE,int direct /*NVME_DIRECTION_TYPE*/,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence );	
	virtual int IdentifyATA( HANDLE hUsb, unsigned char* buf, int len, int bridge );
	virtual int MyIssueATACmdHandle(HANDLE hUsb, unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd, int deviceType=IF_ATA );
	virtual int MyIssueNVMeCmdHandle(HANDLE hUsb, unsigned char *nvmeCmd,unsigned char OPCODE,int direct/*0:NON 1:Read 2:Write*/,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence, int deviceType=IF_NVNE );
	virtual int JMS583_Bridge_Flow(HANDLE hUsb, _NVMe_COMMAND cmd, int data_type, bool ap_key, int admin, int timeout, unsigned char *data, int length);
	virtual int JMS583_Bridge_Send_Sq(HANDLE hUsb, _NVMe_COMMAND cmd, int timeout, int admin);
	virtual int JMS583_Bridge_Read_Cq(HANDLE hUsb, int timeout, int admin, unsigned char *data);
	virtual int JMS583_Bridge_Trigger(HANDLE hUsb, int timeout, int admin);
	virtual int JMS583_Bridge_Read_Data(HANDLE hUsb, int timeout, int admin, unsigned char *data, int length);
	virtual int JMS583_Bridge_Write_Data(HANDLE hUsb, int timeout, int admin, unsigned char *data, int length);
	virtual int JMS583_BridgeReset( int physical, bool doI2c );

#ifndef WIN32
	HANDLE MyCreateNVMeHandle ( char* devicepath ); //devie path in linux
	HANDLE MyCreateHandle ( char* devicepath, bool physical, bool nvmemode );
#else
	HANDLE MyCreateNVMeHandle ( unsigned char scsi );
	HANDLE MyCreateHandle ( unsigned char drive, bool physical, bool nvmemode );
#endif

	void MyCloseHandle( HANDLE *hUsb );
private:
    void Init();
	int Bridge_ReadFWVersion(HANDLE hUsb, unsigned char * read_buffer);
	int JMS583_Bridge_I2c_Write(HANDLE hUsb,unsigned char i2c_address, unsigned char cmd, unsigned char * buffer, int length);
	int JMS583_Bridge_Control(HANDLE hUsb,int control_id, int timeout, unsigned char * read_buffer);
	int JMS583_Bridge_I2c_Read(HANDLE hUsb,unsigned char i2c_address, unsigned char cmd, unsigned char * buffer, int length);
protected:
	unsigned char _PARAMETERS[1024];

    virtual bool USB_ISPInstall ( const char* ispfilename, bool bSilence );
    bool USB_ISPInstallBuf ( const unsigned char* bnISPbuf, int isplen, bool bSilence );
	int  USB_SmartTestInit ();
	void USB_ReArrangeCE ();
	int  USB_GetFlashID ( unsigned char* buf, unsigned char Mode3, unsigned char Decoder );
	int  USB_GetFWVersion ( char* ibuf );
	int  USB_SetFlashParameters ( unsigned char* FlashPara );
	int  USB_Inquiry ( unsigned char* r_buf, bool all=false );
	int  USB_JumpBackISP ();
	int  USB_TransferISPData ( int offset,int xlen,unsigned char verify,unsigned char mode,unsigned char* buf );
	int  USB_GetISPStatus ( unsigned char* r_buf );
	int  USB_BufferReadISPData ( unsigned long Offset, int SecCnt, unsigned char* buf );
	int  USB_ISPSwitchMode ( unsigned char mode );
	
};

#endif /*_USBSCSICMD_H_*/