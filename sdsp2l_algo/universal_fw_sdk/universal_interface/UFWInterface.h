#ifndef _UFWINTERFACE_H_
#define _UFWINTERFACE_H_


#include <assert.h>
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#include "nvme.h"
#else
#include <linux/nvme.h>
#include "linux_scsi_basic.h"
#include "linux_nvme_basic.h"
#include "linux_ata_basic.h"
#endif
#include "UFWLoger.h"
//#include "spti.h"
#define        _CDB_READ        true
#define        _CDB_NONE        false
#define        _CDB_WRITE        false
//#define        SCSI_IOCTL_DATA_IN true
//#define        SCSI_IOCTL_DATA_OUT false

#define        NVME_DIRCT_NON        0
#define        NVME_DIRCT_READ       1
#define        NVME_DIRCT_WRITE      2

#define INTERFACE_MASK 0xFF000000
#define IF_USB         0x01000000
#define IF_ATA         0x02000000
#define IF_JMS578      0x05000000 
#define IF_JMS583      0x13000000
#define IF_C2012       0x04000000
#define IF_ASMedia     0x05000000
#define IF_NVNE        0x06000000

#define DEFAULT_TIME_OUT  (30)

class DeviveInfo
{
public:
	DeviveInfo();
	char drive;
#ifndef WIN32
    char physical[512];
#else
	int physical;
#endif
	bool nvme;
	char label[64];

};


class UFWInterface: public UFWLoger
{
public:
	UFWInterface(DeviveInfo*, const char* logFolder );
	UFWInterface( DeviveInfo*, FILE *logPtr );
	~UFWInterface();
	const DeviveInfo * GetDriveInfo();
	int GetDeviceType();
	void SetDeviceType(int deviceType);
	HANDLE GetHandle();
	void SetTimeOut( int timeout );
	static void _static_SetDebugMode(int mode);
	
	virtual int _static_MyIssueCmd(  unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout=20 );
	virtual int _static_MyIssueATACmd(unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd);
	virtual int _static_MyIssueNVMeCmd( unsigned char *nvmeCmd,unsigned char OPCODE,int direct /*NVME_DIRECTION_TYPE*/,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence );	
	
	static int _static_Inquiry ( HANDLE hUsb, unsigned char* r_buf, bool all, unsigned char length=0 );
	static int _static_IdentifyATA( HANDLE hUsb, unsigned char* buf, int len, int bridge );
	static int _static_MyIssueCmdHandle(  HANDLE hUsb, unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout,int deviceType=IF_USB, int cdb_length=16 );
	static int _static_MyIssueATACmdHandle(HANDLE hUsb, unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd, int deviceType=IF_ATA );
	static int _static_MyIssueNVMeCmdHandle(HANDLE hUsb, unsigned char *nvmeCmd,unsigned char OPCODE,int direct/*0:NON 1:Read 2:Write*/,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence, int deviceType=IF_NVNE );

#ifndef WIN32
	static HANDLE _static_MyCreateNVMeHandle ( char* devicepath ); //devie path in linux
	static HANDLE _static_MyCreateHandle ( char* devicepath, bool physical, bool nvmemode );
#else
	static HANDLE _static_MyCreateNVMeHandle ( unsigned char scsi );
	static HANDLE _static_MyCreateHandle ( unsigned char drive, bool physical, bool nvmemode );
#endif
	static void _static_MyCloseHandle( HANDLE *hUsb );
	
	static int _static_JMS583_BridgeReset(int physical, bool i2c);
    static int _static_JMS583_BridgOff(int physical, bool doI2c);
    static int _static_C2012_PowerOnOff(int  physical,  unsigned int Mode,unsigned int Delay,unsigned int Voltage_SEL,unsigned int DDR_PWR_Flag,bool m_unit_100ms);

	static int _static_USB_Trigger(HANDLE hUsb, int timeout, int admin);
	static int _static_USB_Read_Data(HANDLE hUsb, int timeout, int admin, unsigned char *data, int length);
	static int _static_USB_Write_Data(HANDLE hUsb, int timeout, int admin, unsigned char *data, int length);

private:
	//JMS583
	static int _static_JMS583_Bridge_Flow(HANDLE hUsb, _NVMe_COMMAND cmd, int data_type, bool ap_key, int admin, int timeout, unsigned char *data, int length);
	static int _static_JMS583_Bridge_Send_Sq(HANDLE hUsb, _NVMe_COMMAND cmd, int timeout, int admin);
	static int _static_JMS583_Bridge_Read_Cq(HANDLE hUsb, int timeout, int admin, unsigned char *data);
	static int _static_JMS583_Bridge_Trigger(HANDLE hUsb, int timeout, int admin);
	static int _static_JMS583_Bridge_Read_Data(HANDLE hUsb, int timeout, int admin, unsigned char *data, int length);
	static int _static_JMS583_Bridge_Write_Data(HANDLE hUsb, int timeout, int admin, unsigned char *data, int length);
	static int _static_JMS583_Bridge_I2c_Write(HANDLE hUsb,unsigned char i2c_address, unsigned char cmd, unsigned char * buffer, int length);
	static int _static_JMS583_Bridge_I2c_Read(HANDLE hUsb,unsigned char i2c_address, unsigned char cmd, unsigned char * buffer, int length);
	static int _static_JMS583_Bridge_Control(HANDLE hUsb,int control_id, int timeout, unsigned char * read_buffer);
	static int _static_JMS583_Bridge_WriteParameterPage(HANDLE hUsb,unsigned char *w_buf);
	static int _static_JMS583_Bridge_ReadParameterPage(HANDLE hUsb,unsigned char *r_buf);
	static int _static_Bridge_ReadFWVersion(HANDLE hUsb, unsigned char * read_buffer);
	
protected:
	DeviveInfo _info;
	HANDLE _hUsb;
	int _deviceType;
	int _timeOut;
	void Init(DeviveInfo*);
	BOOL logDeviceIoControl(HANDLE hDevice,DWORD dwIoControlCode,void * lpInBuffer,DWORD nInBufferSize,void* lpOutBuffer,DWORD nOutBufferSize,LPDWORD lpBytesReturned,LPOVERLAPPED lpOverlapped);

};


#endif /*_UFWINTERFACE_H_*/