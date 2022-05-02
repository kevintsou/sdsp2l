
#include "USBSCSICmd.h"
#include "spti.h"
#include "nvmeIoctl.h"
#include <stddef.h>
// commands for flash
#define ISP_JUMP_OK    0x9c
#define ISP_CONFIG_OK  0x55
#define ISP_PROG_OK    0xA5

int _SCSIPtBuilder1 ( SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER* sptdwb ,unsigned int cdbLength , unsigned char* CDB ,
					 unsigned char direction , unsigned int transferLength ,unsigned long timeOut, int deviceType );
int _ATAPtBuilder(ATA_PASS_THROUGH_WITH_BUFFERS *aptwb,ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS *aptdwb, 
				  unsigned char *PRegister, unsigned char *CRegister, USHORT direction, unsigned long transferLength, unsigned long timeOut, bool Flag, bool Is48bit);
unsigned short Swap_Uint(unsigned short value);
unsigned short Crc16_Ccitt(unsigned char *buffer, unsigned int length);
bool checkIfJMS583VersionIsNew(unsigned char * inquiryBuf);

USBSCSICmd::USBSCSICmd( DeviveInfo* info, const char* logFolder )
:UFWFlashCmd(info,logFolder)
{
    Init();
}

USBSCSICmd::USBSCSICmd( DeviveInfo* info, FILE *logPtr )
:UFWFlashCmd(info,logPtr)
{
    Init();
}

USBSCSICmd::~USBSCSICmd()
{
}



void USBSCSICmd::MyCloseHandle( HANDLE *hUsb )
{
	//_DEBUGPRINT("MyCloseHandle %p\n",*hUsb);
#ifndef WIN32
	close(*hUsb);
#else
	CloseHandle(*hUsb);
#endif
	hUsb = 0;
}


#ifndef WIN32
HANDLE USBSCSICmd::MyCreateNVMeHandle ( char* devicepath )
#else
HANDLE USBSCSICmd::MyCreateNVMeHandle ( unsigned char scsi )
#endif
{
#ifndef WIN32
	return MyCreateHandle ( devicepath, false, false );
#else
	//_DEBUGPRINT("MyCreateNVMeHandle %d\n",scsi);
	DWORD accessMode = 0, shareMode = 0;
	char devicename[32];

	sprintf_s(devicename , "\\\\.\\Scsi%d:", scsi); //Logical Drive
	//sprintf(devicename , "\\\\.\\PhysicalDrive%d", drive); //Logical Drive
	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	accessMode = GENERIC_WRITE | GENERIC_READ;

	HANDLE hUsb = CreateFileA ( devicename,
		accessMode,
		shareMode,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL );
	//assert(hUsb != INVALID_HANDLE_VALUE);

	return hUsb;
#endif
}

#ifndef WIN32
HANDLE USBSCSICmd::MyCreateHandle ( char* devicepath, bool physical, bool nvmemode )
#else
HANDLE USBSCSICmd::MyCreateHandle ( unsigned char drive, bool physical, bool nvmemode )
#endif
{
#ifndef WIN32
	HANDLE m_handle = open(devicepath, O_RDWR | O_LARGEFILE | O_DIRECT | O_NONBLOCK | O_SYNC);
	if(m_handle == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	else
	{
		if(flock(m_handle, LOCK_SH) == -1)
		{
			printf("lock fail: %s\n", strerror(errno));
			close(m_handle);
			return INVALID_HANDLE_VALUE;
		}

		struct flock lock;
		memset(&lock, 0, sizeof(lock));

		lock.l_type = F_WRLCK | F_RDLCK;

		if(fcntl(m_handle, F_SETLKW, &lock) == -1)
		{
			printf("fcntl lock fail: %s\n", strerror(errno));
			close(m_handle);
			return INVALID_HANDLE_VALUE;
		}

		return m_handle;
	}
#else
	HANDLE hUsb;
	if ( nvmemode ) {
		assert(physical);
		hUsb = MyCreateNVMeHandle ( drive );
	}
	else {
		DWORD accessMode = 0, shareMode = 0;
		char devicename[32];
		if ( !physical ) {
			sprintf_s( devicename, "\\\\.\\%1C:", drive );
		}
		else {
			sprintf_s(devicename , "\\\\.\\PHYSICALDRIVE%d", drive); //Logical Drive
		}
		shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		accessMode = GENERIC_WRITE | GENERIC_READ;

		hUsb = CreateFileA ( devicename,
			accessMode,
			shareMode,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
	}

	//_DEBUGPRINT("MyCreateHandle %d,%d,%d %p\n",drive,physical,nvmemode,hUsb);
	return hUsb;
#endif
}

extern unsigned char sat_vendor_cmd_type[16][3];
#ifndef WIN32
int USBSCSICmd::MyIssueATACmdHandle(HANDLE hUsb, unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd, int deviceType)
{
	if(direct == 2)//write
		return sg16(hUsb, 1, DMACmd, (Ata_Tf *)cRegister, buf, bufLength, timeOut);
	else
		return sg16(hUsb, 0, DMACmd, (Ata_Tf *)cRegister, buf, bufLength, timeOut);
}
#else
int USBSCSICmd::MyIssueATACmdHandle(HANDLE hUsb, unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd, int deviceType)
{
	/* cRegiter
	0	Features Register
	1	Sector Count Register
	2	Sector Number Register
	3	Cylinder Low Register
	4	Cylinder High Register
	5	Device/Head Register
	6	Command Register
	7	Reserved
	*/
    //printf( "%s[%02X %02X %02X %02X %02X %02X %02X %02X] \n",__FUNCTION__,cRegister[0],cRegister[1],cRegister[2],cRegister[3],cRegister[4],cRegister[5],cRegister[6],cRegister[7] );
	int connector = (deviceType&INTERFACE_MASK);
    if ( IF_ASMedia==connector ) {
        unsigned char cdb[16];
        memset(cdb,0,sizeof(cdb));

		if ( bufLength==0 ) {
			memcpy(cdb,sat_vendor_cmd_type[3],3);
		}
		else if ( direct==2 ) {
			memcpy(cdb,sat_vendor_cmd_type[5],3);
		}
		else {
			memcpy(cdb,sat_vendor_cmd_type[4],3);
		}

		//phison rule
		cdb[3] = cRegister[0];
		cdb[4] = cRegister[1];
		cdb[5] = cRegister[2];
		cdb[6] = cRegister[3];
		cdb[7] = cRegister[4];
		cdb[8] = cRegister[5];
		cdb[9] = cRegister[6];

		oRegister[6]=0x50; //skip check from outer usage.
        return MyIssueCmdHandle ( hUsb, cdb, direct==2? _CDB_WRITE:_CDB_READ, buf, bufLength,timeOut, deviceType );
    }
	else if ( IF_ATA==connector ) {
		BOOL status = FALSE;
		unsigned long Length = 0, Returned = 0;
		//HANDLE DeviceHandle1;
		USHORT ATAFlag=0;

		if (hUsb == INVALID_HANDLE_VALUE)
			return -0x100;

		ATA_PASS_THROUGH_WITH_BUFFERS aptwb;
		ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS aptdwb;

		//===== AtaFlags =====
		//Indicates the direction of data transfer and specifies the kind of operation to be performed. The value of this member must be some combination of the following flags:
		//ATA flags Meaning
		//ATA_FLAGS_DRDY_REQUIRED
		//    Wait for DRDY status from the device before sending the command to the device.
		//ATA_FLAGS_DATA_IN
		//    Read data from the device.
		//ATA_FLAGS_DATA_OUT
		//    Write data to the device.
		//ATA_FLAGS_48BIT_COMMAND
		//    The ATA command to be sent uses the 48-bit logical block address (LBA) feature set. When this flag is set, the contents of the PreviousTaskFile member in the ATA_PASS_THROUGH_EX structure should be valid.
		//ATA_FLAGS_USE_DMA
		//    Set the transfer mode to DMA.
		//ATA_FLAGS_NO_MULTIPLE
		//    Read single sector only.

		//#define ATA_FLAGS_DRDY_REQUIRED         (1 << 0)
		//#define ATA_FLAGS_DATA_IN               (1 << 1)
		//#define ATA_FLAGS_DATA_OUT              (1 << 2)
		//#define ATA_FLAGS_48BIT_COMMAND         (1 << 3)
		if(direct==0)
			ATAFlag=ATA_FLAGS_DRDY_REQUIRED;
		else if(direct==1)
		{
			//ATAFlag=ATA_FLAGS_DATA_IN;  //ATA_FLAGS_USE_DMA
			if(DMACmd)
				ATAFlag=(ATA_FLAGS_DATA_IN | ATA_FLAGS_USE_DMA);
			else
				ATAFlag=(ATA_FLAGS_DATA_IN);   // | ATA_FLAGS_NO_MULTIPLE
		}
		else if(direct==2)
		{
			//ATAFlag=ATA_FLAGS_DATA_OUT;
			if(DMACmd)
				ATAFlag=(ATA_FLAGS_DATA_OUT | ATA_FLAGS_USE_DMA);
			else
				ATAFlag=(ATA_FLAGS_DATA_OUT);  // | ATA_FLAGS_NO_MULTIPLE
		}

		if(Is48bit)
			ATAFlag |= ATA_FLAGS_48BIT_COMMAND;

		_ATAPtBuilder(&aptwb,&aptdwb,pRegister, cRegister, ATAFlag, bufLength, timeOut, true, Is48bit);
		//ATAPtBuilder(pRegister, cRegister, ATAFlag, bufLength << 9, timeOut, true, Is48bit);
		aptdwb.aptd.DataBuffer = bufLength ? buf : NULL;
		//aptdwb.aptd.DataBuffer=buf;

		if(Flag)
			Length = sizeof(ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS);
		else
			Length = offsetof(ATA_PASS_THROUGH_WITH_BUFFERS, ucDataBuf) + aptwb.apt.DataTransferLength;

		try
		{
			status = DeviceIoControl(hUsb,
									 Flag ? IOCTL_ATA_PASS_THROUGH_DIRECT : IOCTL_ATA_PASS_THROUGH,
									 Flag ? (void *)&aptdwb : (void *)&aptwb,
									 Length,
									 Flag ? (void *)&aptdwb : (void *)&aptwb,
									 Length,
									 &Returned,
									 NULL);
			memcpy(oRegister,aptdwb.aptd.CurrentTaskFile,7);
			//for(i=0;i<7;i++)
			//{
			//   CurOutRegister[i]=aptdwb.aptd.CurrentTaskFile[i];
			//         if(Is48bit)
			//            PreOutRegister[i]=aptdwb.aptd.PreviousTaskFile[i];
			//}

			int err=GetLastError();

			if(!status)  {//DeviceIoControl fail-> Status=0
				//Log(LOGDEBUG, "IssueATACmd status=%d err=%d\n",status,err);
				return err;
			}
			else
			{
				//CloseHandle(DeviceHandle1);
				return 0;
			}
		}
		catch (...)
		{
			//Log(LOGDEBUG, "IssueATACmd exception\n");
			//CloseHandle(DeviceHandle1);
			return -1;
		}
	} else if ( IF_C2012==connector ) {
		unsigned char cdb_c2012[16] = {0};

		cdb_c2012[0] = 0x06;
		cdb_c2012[1] = 0xF0;
		cdb_c2012[2] = 0x71;
		cdb_c2012[4] = cRegister[6];//command

		if (2 == direct) {//2 for write
			cdb_c2012[3] = 0x03;
		} else if (1 == direct) {//1 for read
			cdb_c2012[3] = 0x01;
		} else {//direction unspecified

		}
		cdb_c2012[5] = cRegister[0];//feature
		cdb_c2012[6] = cRegister[1];//SC
		cdb_c2012[7] = cRegister[2];//SN
		cdb_c2012[8] = cRegister[3];//CylLo
		cdb_c2012[9] = cRegister[4];//CylHi
		cdb_c2012[10] = cRegister[5];//DH
		cdb_c2012[11] = ((bufLength+511)/512) & 0xff;
		return MyIssueCmdHandle ( hUsb, cdb_c2012, direct==2? _CDB_WRITE:_CDB_READ, buf, bufLength,timeOut, deviceType );
	} else {
		assert(0);
	}
	return -1;
}
#endif


#define SCSI_IOCTL_DATA_OUT          0
#define SCSI_IOCTL_DATA_IN           1
#define SCSI_IOCTL_DATA_UNSPECIFIED  2


int USBSCSICmd::Inquiry ( HANDLE hUsb, unsigned char* r_buf, bool all, unsigned char length )
{
	memset ( r_buf , 0 , 512 );
	unsigned char cdb[16];
	memset ( cdb , 0 , 16 );
	cdb[0] = 0x12;
	cdb[4] = all ? 0x44 : 0x24;
	if(length)
		cdb[4] = length;

	int err = 0;

	for ( int retry=0; retry<3; retry++ ) {        // Jorry
		err = MyIssueCmdHandle(hUsb,cdb,true,r_buf,cdb[4],10,IF_USB);
		if ( err==0 ) {
			break;
		}
		UFWSleep( 200 );
	}

	return err;
}


int USBSCSICmd::Bridge_ReadFWVersion(HANDLE hUsb, unsigned char * read_buffer)
{
	unsigned char cdb[16]={0};
	int fw_length = 16;
	cdb[0] = 0xE0;
	cdb[1] = 0xF4;
	cdb[2] = 0xE7;

	return MyIssueCmdHandle(hUsb, cdb, true, read_buffer, fw_length, 15, IF_USB);
}



int USBSCSICmd::JMS583_Bridge_I2c_Write(HANDLE hUsb,unsigned char i2c_address, unsigned char cmd, unsigned char * buffer, int length)
{
	unsigned char cdb[16];
	int rtn = 0;

	memset(cdb, 0x00, sizeof(cdb));
	cdb[0] = 0xFF;
	cdb[1] = 0x04;
	cdb[2] = 0xC1;
	cdb[3] = 0x00;
	cdb[4] = i2c_address;
	cdb[5] = 0x00;
	cdb[6] = cmd;
	cdb[7] = (length >> 8) & 0xff;
	cdb[8] = length & 0xff;
	rtn = MyIssueCmdHandle( hUsb, cdb , false, buffer , length, 30, IF_USB );
	if (rtn) {
		_DEBUGPRINT( "Bridge_I2c_Write fail but don't care!\n");
	}
	return 0;

}

// control_id = 1: Power OFF
// control_id = 2: Power ON
// control_id = 4: Initial
// control_id = 5: Reset MCU
// control_id = 6: Normal shutdown
// control_id = 7: Get Initial Status
int USBSCSICmd::JMS583_Bridge_Control(HANDLE hUsb,int control_id, int timeout, unsigned char * read_buffer)
{
	unsigned char cdb[16] = {0};
	unsigned char data[512] = {0};
	int length = 512;
	int rtn=0;

	if (!read_buffer)
		return -1;

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = 0;		//	NVM Command Set Payload
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)

	data[0] = 'N';
	data[1] = 'V';
	data[2] = 'M';
	data[3] = 'E';

	if(control_id == 0x07 || control_id == 0x0C)
		data[8] = 0x1C;


	data[72] = control_id;
	rtn = MyIssueCmdHandle ( hUsb, cdb , false, data , length, timeout, IF_USB );
	if(control_id == 0x05) {
		return rtn;
	}

	if(control_id == 0x07 || control_id == 0x0C)
		cdb[1] = 0x02;				//	NVM Command Get Resp
	else
		cdb[1] = 0x0F;				//	NVM Command Get Resp

	return MyIssueCmdHandle ( hUsb, cdb , true, read_buffer , 512, timeout, IF_USB );

}

int USBSCSICmd::JMS583_Bridge_I2c_Read(HANDLE hUsb,unsigned char i2c_address, unsigned char cmd, unsigned char * buffer, int length)
{
	unsigned char cdb[16];
	int rtn = 0;
	memset(cdb, 0x00, sizeof(cdb));
	cdb[0] = 0xFF;
	cdb[1] = 0x04;
	cdb[2] = 0xC0;
	cdb[3] = 0x00;
	cdb[4] = i2c_address;
	cdb[5] = 0x00;
	cdb[6] = cmd;
	cdb[7] = (length >> 8) & 0xff;
	cdb[8] = length & 0xff;

	rtn = MyIssueCmdHandle ( hUsb, cdb , true, buffer , length, 30, IF_USB );
	if (rtn) {
		_DEBUGPRINT( "Bridge_I2c_READ fail but don't care!\n");
	}
	return 0;
}


int USBSCSICmd::JMS583_BridgeReset( int physical, bool doI2c ) 
{
	unsigned char inq_buffer[1024] = {'\0'};
	unsigned char inq_buffer_cache[1024] = {'\0'};
	unsigned char i2c_buffer[512] = {'\0'};
	unsigned char fw_version[16] = {'\0'};
	unsigned char status_buffer[512] = {0};
	int bridge_support_level = 0;

	DWORD dwBytesReturned = 0;
	bool isNewTester = false;

	//DeviceIoControl(*_pScsiHandle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwBytesReturned, NULL);
	//_pScsiCmd->_iPhysicalNum = sdn.DeviceNumber;
	HANDLE hdl = MyCreateHandle ( physical, true, false );
	assert(hdl != INVALID_HANDLE_VALUE);

	if (Inquiry(hdl, inq_buffer, true))
	{
		_DEBUGPRINT( "%s:Inquiry fail\n", __FUNCTION__);
		goto failend;
	}
	memcpy(inq_buffer_cache, inq_buffer, sizeof(inq_buffer));


	Bridge_ReadFWVersion(hdl, fw_version);
	if(fw_version[12] == 195 && fw_version[13] == 1 && fw_version[14] >= 1) // Support Wait PCIe Link Status
		bridge_support_level = 1;
	if(fw_version[12] == 195 && fw_version[13] == 1 && fw_version[14] == 1 && fw_version[15] >= 3) // Support Wait GPIO7/PCIe Link Status/NVMe Status
		bridge_support_level = 2;
	if(fw_version[12] == 254 && fw_version[13] == 1 && fw_version[14] >= 1) // Support Wait GPIO7/PCIe Link Status/NVMe Status
		bridge_support_level = 2;

	isNewTester = checkIfJMS583VersionIsNew(inq_buffer_cache);
	if (doI2c || isNewTester){
		i2c_buffer[0] = 0x18;
		_DEBUGPRINT( "%s:I2c_write cmd 0x01 and write value 0x18 to addr 0x48\n", __FUNCTION__);
		if (JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)){
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] = 0x18; // Set GPIO3/GPIO4 Input, others Output
		_DEBUGPRINT( "%s:I2c_write cmd 0x03 and write value 0x18 to addr 0x48\n", __FUNCTION__);
		if (JMS583_Bridge_I2c_Write(hdl,0x48, 3, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge I2C Init Fail\n", __FUNCTION__);
			goto failend;
		}

	}


	_DEBUGPRINT( "%s:Bridge Power Off\n", __FUNCTION__);
	if (JMS583_Bridge_Control(hdl,1, DEFAULT_TIME_OUT, status_buffer)){
		_DEBUGPRINT( "%s:Bridge Power Off Fail\n", __FUNCTION__);
		goto failend;
	}

	if (doI2c || isNewTester){
		_DEBUGPRINT( "%s:Bridge Power Off\n", __FUNCTION__);
		if (JMS583_Bridge_Control(hdl,1, DEFAULT_TIME_OUT, status_buffer)) {
			_DEBUGPRINT( "%s:Bridge Power Off Fail\n", __FUNCTION__);
			goto failend;
		}
		_DEBUGPRINT( "%s:I2c_read cmd 0x0 from 0x48 and set to _i2c_buffer[0]\n", __FUNCTION__);
		if (JMS583_Bridge_I2c_Read(hdl,0x48, 0, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Read fail\n", __FUNCTION__);
			goto failend;
		}
		_DEBUGPRINT( "%s:I2c_read buffer 0x%X\n", __FUNCTION__, i2c_buffer[0]);
		i2c_buffer[0] &= 0x18;

		_DEBUGPRINT( "%s:I2c_write cmd 1 to 0x48 with buf=0x%X\n", __FUNCTION__, i2c_buffer[0]);
		if (JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] |= 0xC0;
		_DEBUGPRINT( "%s:I2c_write cmd 1 to 0x48 with buf=0x%X\n", __FUNCTION__, i2c_buffer[0]);
		if (JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] &= 0x18;
		_DEBUGPRINT( "%s:I2c_write cmd 1 to 0x48 with buf=0x%X\n", __FUNCTION__, i2c_buffer[0]);
		if (JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
	}

	UFWSleep(1000);
	if (doI2c || isNewTester){
		//bridge power on
		// Keep GPIO3/GPIO4, others Set Low
		i2c_buffer[0] &= 0x18;
		_DEBUGPRINT( "%s:I2c_write cmd 1 to 0x48 with buf=0x%X\n", __FUNCTION__, i2c_buffer[0]);
		if (JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] |= 0x03;
		_DEBUGPRINT( "%s:I2c_write cmd 1 to 0x48 with buf=0x%X\n", __FUNCTION__, i2c_buffer[0]);
		if (JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
	}

	if (JMS583_Bridge_Control(hdl,4, DEFAULT_TIME_OUT, status_buffer)) {
		_DEBUGPRINT( "%s:Bridge Init Fail\n", __FUNCTION__);
		goto failend;
	}



	if (bridge_support_level) {
		int wait_cnt = 0;
		do {
			memset(status_buffer, 0, sizeof(status_buffer));
			Sleep(200);
			_DEBUGPRINT("%s:wait nvme ready...", __FUNCTION__);
			if (1 == bridge_support_level) {
				JMS583_Bridge_Control(hdl, 0x07, 15, status_buffer);
				if ('N' == status_buffer[0] && 'V' == status_buffer[1] && 'M' == status_buffer[2] && 'E' == status_buffer[3] && (status_buffer[4]&0x01))
				{
					_DEBUGPRINT("%s:ready for using.", __FUNCTION__);
					Sleep(500);
					break;
				}
			}
			else if (2 == bridge_support_level) {
				JMS583_Bridge_Control(hdl, 0x0C, 15, status_buffer);
				if ('N' == status_buffer[0] && 'V' == status_buffer[1] && 'M' == status_buffer[2] && 'E' == status_buffer[3] && !status_buffer[4] && !status_buffer[5] && !status_buffer[6])
				{
					_DEBUGPRINT("%s:ready for using.", __FUNCTION__);
					Sleep(500);
					break;
				}
			}
			else {
				Sleep(5*1000);
				break;
			}
			wait_cnt++;
		} while(wait_cnt<20);
	}
	else {
		Sleep(5*1000);
	}
	MyCloseHandle(&hdl);
	return physical;

failend:
	MyCloseHandle(&hdl);
	return -1;
}

int USBSCSICmd::JMS583_Bridge_Read_Data(HANDLE hUsb,int timeout, int admin, unsigned char *data, int length)
{
	unsigned char cdb[16] = {0};

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = (admin << 7) + 2;		//	DMA-IN
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)
	return MyIssueCmdHandle ( hUsb, cdb , true, data , length, timeout, IF_USB );
}
int USBSCSICmd::JMS583_Bridge_Write_Data(HANDLE hUsb,int timeout, int admin, unsigned char *data, int length)
{
	unsigned char cdb[16] = {0};

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = (admin << 7) + 3;		//	DMA-OUT
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)

	return MyIssueCmdHandle ( hUsb, cdb , false, data , length, timeout, IF_USB );
}



int USBSCSICmd::JMS583_Bridge_Send_Sq(HANDLE hUsb,_NVMe_COMMAND cmd, int timeout, int admin)
{
	unsigned char cdb[16] = {0};
	unsigned char data[512] = {0};
	int length = 512;

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = (admin << 7) + 0;		//	NVM Command Set Payload
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)

	data[0] = 'N';
	data[1] = 'V';
	data[2] = 'M';
	data[3] = 'E';

	//byte 8 - byte 71
	memcpy(data + 8, &cmd, 64);

	return MyIssueCmdHandle ( hUsb, cdb , false, data , 512, timeout, IF_USB );

}
int USBSCSICmd::JMS583_Bridge_Read_Cq(HANDLE hUsb,int timeout, int admin, unsigned char *data)
{
	unsigned char cdb[16] = {0};
	int length = 512;

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = (admin << 7) + 15;		//	Return Response Information
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)
	return MyIssueCmdHandle ( hUsb, cdb , true, data , 512, timeout, IF_USB );

}
int USBSCSICmd::JMS583_Bridge_Trigger(HANDLE hUsb,int timeout, int admin)
{
	unsigned char cdb[16] = {0};
	int length = 0;

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = (admin << 7) + 1;		//	Non-data
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)
	return MyIssueCmdHandle ( hUsb, cdb , false, NULL , 0, timeout, IF_USB );
}

int USBSCSICmd::JMS583_Bridge_Flow(HANDLE hUsb,_NVMe_COMMAND cmd, int data_type, bool ap_key, int admin, int timeout, unsigned char *data, int length)
{

	int result;
	unsigned char cq_buffer[512] = {0};
	char temp[100];
	bool disable_read_cq = false;
	if((cmd.CDW12 & 0xff) == 0x2 ){ // isp jump dont need cq
		disable_read_cq = true;
	}

	//if(ap_key)
	//{
	//	//result = JMS583_Bridge_Ap_Key();
	//	if(result != 0)
	//	{
	//		//Dll_Log::Write_Log("dlldebug.log", "Nvme Bridge AP Key Fail");
	//		return result;
	//	}
	//}

	result = JMS583_Bridge_Send_Sq(hUsb,cmd, timeout, admin);
	if(result != 0)
	{
		_DEBUGPRINT("%s:Nvme Bridge Send SQ Fail\n", __FUNCTION__);
		return result;
	}

	if(data_type == SCSI_IOCTL_DATA_UNSPECIFIED)
	{
		result = JMS583_Bridge_Trigger(hUsb,timeout, admin);
		if(result != 0)
		{
			_DEBUGPRINT("%s:Nvme Bridge Trigger Fail\n", __FUNCTION__);
			return result;
		}
	}
	else if(data_type == SCSI_IOCTL_DATA_OUT)
	{
		result = JMS583_Bridge_Write_Data(hUsb,timeout, admin, data, length);
		if(result != 0)
		{
			_DEBUGPRINT("%s:Nvme Bridge Write Data Fail\n", __FUNCTION__);
			return result;
		}
	}
	else if(data_type == SCSI_IOCTL_DATA_IN)
	{
		result = JMS583_Bridge_Read_Data(hUsb,timeout, admin, data, length);
		if(result != 0)
		{
			_DEBUGPRINT("%s:Nvme Bridge Read Data Fail\n", __FUNCTION__);
			return result;
		}
	}
	else
	{
		assert(0);
		return -1;
	}

	if(disable_read_cq){
		return 0;
	}

	result = JMS583_Bridge_Read_Cq(hUsb,timeout, admin, cq_buffer);
	if(result != 0)
	{
		assert(0);
		return result;
	}

	int command_specific = cq_buffer[8] + (cq_buffer[9] << 8) + (cq_buffer[10] << 16) + (cq_buffer[11] << 24); 
	//Set_Command_Specific(command_specific);

	int status_code = (cq_buffer[22] >> 1) + ((cq_buffer[23] & 0x1) << 7); 
	int status_code_type = (cq_buffer[23] >> 1) & 0x07;

	if(status_code == 0xb && status_code_type == 1){//Firmware Activation Requires Conventional Reset
		return 0;
	}
	if(status_code == 0x10 && status_code_type == 1){//Firmware Activation Requires NVM Subsystem Reset
		return 0;
	}
	if(status_code == 0x11 && status_code_type == 1){//Firmware Activation Requires Reset:
		return 0;
	}

	if(status_code != 0xFE && status_code != 0){
		sprintf_s(temp, "SC : %d, SCT : %d", status_code, status_code_type);
		return -1;
	}

	return 0;
}

BOOL USBSCSICmd::MyDeviceIoControl(HANDLE hDevice,DWORD dwIoControlCode,void* lpInBuffer, DWORD nInBufferSize,void* lpOutBuffer
									 , DWORD nOutBufferSize,LPDWORD lpBytesReturned,LPOVERLAPPED lpOverlapped){
										 BOOL status = FALSE;
										 static int cmdindex = 0;
										 //char filename[256];
										 SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER *sptdwb = (SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER*)lpInBuffer;
										 //if(UvsDebugMode & 0x01){ //backup mode
										 //	CreateDirectory(".\\debug_data",NULL);
										 //	char filename[256];
										 //	sprintf_s(filename,"debug_data\\%d_scsi_%x_%02x_%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x.bin",cmdindex,dwIoControlCode,sptdwb->sptd.DataIn,
										 //		sptdwb->sptd.Cdb[0],
										 //		sptdwb->sptd.Cdb[1],
										 //		sptdwb->sptd.Cdb[2],
										 //		sptdwb->sptd.Cdb[3],
										 //		sptdwb->sptd.Cdb[4],
										 //		sptdwb->sptd.Cdb[5],
										 //		sptdwb->sptd.Cdb[6],
										 //		sptdwb->sptd.Cdb[7],
										 //		sptdwb->sptd.Cdb[8],
										 //		sptdwb->sptd.Cdb[9],
										 //		sptdwb->sptd.Cdb[10],
										 //		sptdwb->sptd.Cdb[11],
										 //		sptdwb->sptd.Cdb[12],
										 //		sptdwb->sptd.Cdb[13],
										 //		sptdwb->sptd.Cdb[14],
										 //		sptdwb->sptd.Cdb[15]);
										 //}
										 //

										 //if(!(UvsDebugMode &= 0x05)){ //0x05 = backup mode + read mode 直接讀取file
										 status = DeviceIoControl (  hDevice,
											 dwIoControlCode,
											 sptdwb,
											 nInBufferSize,
											 sptdwb,
											 nOutBufferSize,
											 lpBytesReturned,
											 lpOverlapped );
										 //}
										 //if(UvsDebugMode &= 0x01){ //backup mode enable
										 //	FILE *f;
										 //	errno_t err;
										 //	err = fopen_s(&f,filename,"wb");
										 //	if(err) assert(0);
										 //	cmdindex++;
										 //	if(UvsDebugMode & 0x02){ //write mode
										 //	
										 //		if(sptdwb->sptd.DataTransferLength!=0){
										 //			fwrite(sptdwb->sptd.DataBuffer,1,sptdwb->sptd.DataTransferLength,f);
										 //		}
										 //	}
										 //	else if(UvsDebugMode & 0x04){ //write mode
										 //		return status;
										 //	}
										 //	fclose(f);
										 //}
										 return status;
}

int USBSCSICmd::IdentifyATA( HANDLE hUsb, unsigned char* buf, int len, int bridge )
{
	assert(len==512);
	unsigned char pRegister[8],cRegister[8],oRegister[8];
	memset(pRegister,0x00,8);
	memset(cRegister,0x00,8);
	memset(oRegister,0x00,8);
	cRegister[5] = 0xE0;
	cRegister[6] = 0xEC;

	int err = MyIssueATACmdHandle(hUsb,pRegister,cRegister,cRegister,1,buf,len,true,false,1000,false,bridge);
	if( err ) {
		_DEBUGPRINT( "%s command fail\n",__FUNCTION__ );
		_DEBUGPRINT( "out fail [%02X %02X %02X %02X %02X %02X %02X %02X]-->[%02X %02X %02X %02X %02X %02X %02X %02X] \n",
			cRegister[0],cRegister[1],cRegister[2],cRegister[3],cRegister[4],cRegister[5],cRegister[6],cRegister[7] ,
			oRegister[0],oRegister[1],oRegister[2],oRegister[3],oRegister[4],oRegister[5],oRegister[6],oRegister[7]
		);
		return 1;
	}
	return err;
}

int USBSCSICmd::MyIssueATACmd(unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd)
{
	return MyIssueATACmdHandle( _hUsb, pRegister, cRegister, oRegister, direct, buf, bufLength, Flag, Is48bit, timeOut, DMACmd, _deviceType );
}

int USBSCSICmd::MyIssueCmd(  unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout )
{
	return MyIssueCmdHandle( _hUsb, cdb, read, buf, bufLength, timeout, _deviceType );
}

int USBSCSICmd::MyIssueNVMeCmd( unsigned char *nvmeCmd,unsigned char OPCODE,int direct,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence )
{
	return MyIssueNVMeCmdHandle( _hUsb, nvmeCmd, OPCODE, direct, buf, bufLength, timeOut,  silence, _deviceType );
}


#ifndef WIN32
int USBSCSICmd::MyIssueNVMeCmdHandle(HANDLE hUsb, unsigned char *nvmeCmd,unsigned char OPCODE,int direct/*0:NON 1:Read 2:Write*/,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence, int deviceType)
{
	return Nvme_Basic::Issue_Nvm_Cmd(hUsb, (Nvme_Command)nvmeCmd, direct, timeOut, buf, bufLength);
}
#else
int USBSCSICmd::MyIssueNVMeCmdHandle(HANDLE hUsb, unsigned char *nvmeCmd,unsigned char OPCODE,int direct/*0:NON 1:Read 2:Write*/,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence, int deviceType)
{
	//printf("%s hUsb=%d OPCODE %u direct %d bufLength %d\n",__FUNCTION__,hUsb,OPCODE,direct,bufLength );
	//for ( int i=0;i<16;i++ ) {		printf("%02X ",nvmeCmd[i]); 	}	printf("\n");
	//Command Dword0		03:00
	//Namespace Identifier  07:04
	//Reserve				15:08
	//Metadata Pointer		23:16
	//PRPEntry1				31:24
	//PRPEntry2				39:32
	//Command Dword10		43:40 (Optional)Number of Dwords in Data Transfer
	//Command Dword11		47:44
	//Command Dword12		51:48
	//Command Dword13		55:52
	//Command Dword14		59:56
	//Command Dword15		63:60
	BOOL status = FALSE;
	unsigned long Length = 0, Returned = 0;
	//HANDLE DeviceHandle1;
	int i,j;
	unsigned int tmp_long;

	unsigned char *myIoctlBuf;				//153
	unsigned int Struct_BufferSize;
	Struct_BufferSize= sizeof(NVME_PASS_THROUGH_IOCTL)+bufLength;
	myIoctlBuf = (unsigned char *)malloc(Struct_BufferSize);
	memset(myIoctlBuf,0,Struct_BufferSize);
	PNVME_PASS_THROUGH_IOCTL     pMyIoctl = (PNVME_PASS_THROUGH_IOCTL)myIoctlBuf;
	DWORD                        dw;
	PNVMe_COMMAND				 pCmd;

	//DeviceHandle1 = MyCreateFile();

	if (hUsb == INVALID_HANDLE_VALUE)
	{
		free(myIoctlBuf);
		return -0x100;
	}

	memset(pMyIoctl, 0, Struct_BufferSize);

	// Set up the SRB IO Control header
	pMyIoctl->SrbIoCtrl.ControlCode = (unsigned long)NVME_PASS_THROUGH_SRB_IO_CODE;
	memcpy(pMyIoctl->SrbIoCtrl.Signature, NVME_SIG_STR, strlen(NVME_SIG_STR));
	pMyIoctl->SrbIoCtrl.HeaderLength = (unsigned long) sizeof(SRB_IO_CONTROL);
	pMyIoctl->SrbIoCtrl.Timeout = timeOut;
	pMyIoctl->SrbIoCtrl.Length = Struct_BufferSize - sizeof(SRB_IO_CONTROL);

	// Set up the NVMe pass through IOCTL buffer
	//memcpy(&(pMyIoctl->NVMeCmd), &nvmeCmd, sizeof(nvmeCmd));
	pCmd = (PNVMe_COMMAND)pMyIoctl->NVMeCmd;
	pCmd->CDW0.OPC = OPCODE;

	pMyIoctl->QueueId = 0; // Admin queue

	if (OPCODE==ADMIN_IDENTIFY)
	{
		pCmd->NSID =0;
		//if ( 0x5007==(deviceType&0x5007) ) {
		//   pCmd->NSID =1;
		//}
		PADMIN_IDENTIFY_COMMAND_DW10 dw10;
		dw10 = (PADMIN_IDENTIFY_COMMAND_DW10)&(pCmd->CDW10);
		//dw10->CNS = m_NameSpace;
		dw10->CNS = 1;
		pMyIoctl->Direction = NVME_FROM_DEV_TO_HOST;
		pMyIoctl->ReturnBufferLen = sizeof(ADMIN_IDENTIFY_CONTROLLER) + sizeof(NVME_PASS_THROUGH_IOCTL);
		pMyIoctl->VendorSpecific[0] = (DWORD) 0;
		pMyIoctl->VendorSpecific[1] = (DWORD) 0;
		memset(pMyIoctl->DataBuffer, 0x55, sizeof(ADMIN_IDENTIFY_CONTROLLER));
	}
	else if (OPCODE==ADMIN_FIRMWARE_IMAGE_DOWNLOAD)
	{
		pMyIoctl->Direction = NVME_FROM_HOST_TO_DEV;
		pMyIoctl->DataBufferLen = bufLength + sizeof(NVME_PASS_THROUGH_IOCTL);
		pMyIoctl->ReturnBufferLen =  sizeof(NVME_PASS_THROUGH_IOCTL);
		pMyIoctl->VendorSpecific[0] = (DWORD) 0;
		pMyIoctl->VendorSpecific[1] = (DWORD) 0;
		memcpy(pMyIoctl->DataBuffer, buf, bufLength);
		pMyIoctl->NVMeCmd[10]=(bufLength/4)-1;
		//pMyIoctl->NVMeCmd[11]=(Offset/4);
		pMyIoctl->NVMeCmd[11]=0; //only for burner.
	}
	else if (OPCODE==NVM_WRITE || OPCODE==NVM_READ || OPCODE==NVM_FLUSH)
	{
		pMyIoctl->NVMeCmd[11]=0;
		pMyIoctl->QueueId = 1; // Submission queue
		pCmd->NSID =1;
		if (OPCODE==NVM_WRITE)
		{
			pMyIoctl->Direction = NVME_FROM_HOST_TO_DEV;
			pMyIoctl->DataBufferLen = bufLength + sizeof(NVME_PASS_THROUGH_IOCTL);
			pMyIoctl->ReturnBufferLen =  sizeof(NVME_PASS_THROUGH_IOCTL);
			pMyIoctl->VendorSpecific[0] = (DWORD) 0;
			pMyIoctl->VendorSpecific[1] = (DWORD) 0;
			memcpy(pMyIoctl->DataBuffer, buf, bufLength);
			tmp_long = ((*(nvmeCmd+3)&0xFF)<<24)+((*(nvmeCmd+2)&0xFF)<<16)+((*(nvmeCmd+1)&0xFF)<<8)+((*(nvmeCmd)&0xFF));
			pMyIoctl->NVMeCmd[10]=(unsigned long)tmp_long;
			tmp_long = ((*(nvmeCmd+7)&0xFF)<<24)+((*(nvmeCmd+6)&0xFF)<<16)+((*(nvmeCmd+5)&0xFF)<<8)+((*(nvmeCmd+4)&0xFF));
			//pMyIoctl->NVMeCmd[11]=(unsigned long )((tmp_long>>32)&0xffffffff);
			tmp_long = ((*(nvmeCmd+11)&0xFF)<<24)+((*(nvmeCmd+10)&0xFF)<<16)+((*(nvmeCmd+9)&0xFF)<<8)+((*(nvmeCmd+8)&0xFF));
			pMyIoctl->NVMeCmd[12]=(unsigned long)((tmp_long-1)&0xffff);
		}
		else if (OPCODE==NVM_READ)
		{
			pMyIoctl->Direction = NVME_FROM_DEV_TO_HOST;
			pMyIoctl->DataBufferLen = 0;
			pMyIoctl->ReturnBufferLen = bufLength + sizeof(NVME_PASS_THROUGH_IOCTL);
			pMyIoctl->VendorSpecific[0] = (DWORD) 0;
			pMyIoctl->VendorSpecific[1] = (DWORD) 0;
			tmp_long = ((*(nvmeCmd+3)&0xFF)<<24)+((*(nvmeCmd+2)&0xFF)<<16)+((*(nvmeCmd+1)&0xFF)<<8)+((*(nvmeCmd)&0xFF));
			pMyIoctl->NVMeCmd[10]=(unsigned long)tmp_long;
			tmp_long = ((*(nvmeCmd+7)&0xFF)<<24)+((*(nvmeCmd+6)&0xFF)<<16)+((*(nvmeCmd+5)&0xFF)<<8)+((*(nvmeCmd+4)&0xFF));
			//pMyIoctl->NVMeCmd[11]=(unsigned long )((tmp_long>>32)&0xffffffff);
			tmp_long = ((*(nvmeCmd+11)&0xFF)<<24)+((*(nvmeCmd+10)&0xFF)<<16)+((*(nvmeCmd+9)&0xFF)<<8)+((*(nvmeCmd+8)&0xFF));
			pMyIoctl->NVMeCmd[12]=(unsigned long)((tmp_long-1)&0xffff);
		}
	}
	else if (direct==0)
	{
		pMyIoctl->Direction = NVME_NO_DATA_TX;
		pMyIoctl->DataBufferLen = 0;
		pMyIoctl->ReturnBufferLen = sizeof(NVME_PASS_THROUGH_IOCTL);
		pMyIoctl->VendorSpecific[0] = (DWORD) 0;
		pMyIoctl->VendorSpecific[1] = (DWORD) 0;
	}
	else if (direct==1)
	{
		pMyIoctl->Direction = NVME_FROM_DEV_TO_HOST;
		pMyIoctl->DataBufferLen = 0;
		pMyIoctl->ReturnBufferLen = bufLength + sizeof(NVME_PASS_THROUGH_IOCTL);
		pMyIoctl->VendorSpecific[0] = (DWORD) 0;
		pMyIoctl->VendorSpecific[1] = (DWORD) 0;
		//pMyIoctl->NVMeCmd[0] = (DWORD) 0xC1;
		//memset(pMyIoctl->DataBuffer, 0x55, sizeof(ADMIN_IDENTIFY_CONTROLLER));
		pMyIoctl->NVMeCmd[10]=(bufLength/4);
	}
	else if (direct==2)
	{
		pMyIoctl->Direction = NVME_FROM_HOST_TO_DEV;
		pMyIoctl->DataBufferLen = bufLength + sizeof(NVME_PASS_THROUGH_IOCTL);
		pMyIoctl->ReturnBufferLen =  sizeof(NVME_PASS_THROUGH_IOCTL);
		pMyIoctl->VendorSpecific[0] = (DWORD) 0;
		pMyIoctl->VendorSpecific[1] = (DWORD) 0;
		memcpy(pMyIoctl->DataBuffer, buf, bufLength);
		pMyIoctl->NVMeCmd[10]=(bufLength/4);
	}

	// Fill CMD Register
	// Vcmd 48~63
	if ( OPCODE==0xC0 || OPCODE==0xC1 || OPCODE==0xC2 || OPCODE==0xD0 || OPCODE==0xD1 || OPCODE==0xD2 || OPCODE==0xE0 || OPCODE==0xE1 || OPCODE==0xE2 )
	{
		for(i=0,j=12; i<16; i+=4)
		{
			tmp_long = ((*(nvmeCmd+i+3)&0xFF)<<24)+((*(nvmeCmd+i+2)&0xFF)<<16)+((*(nvmeCmd+i+1)&0xFF)<<8)+((*(nvmeCmd+i)&0xFF));
			pMyIoctl->NVMeCmd[j] = tmp_long;
			j++;
		}
	}

	/*some burner will check CRC*/
	unsigned short * nvme_word = (unsigned short * )pCmd;
	nvme_word[31] = Swap_Uint(Crc16_Ccitt((unsigned char *)pCmd, 64));

	int connector = (deviceType&INTERFACE_MASK);
	int rtn = 1;
	if (IF_NVNE == connector ) {
		try
		{
			status = DeviceIoControl(hUsb,
				IOCTL_SCSI_MINIPORT,
				pMyIoctl,
				Struct_BufferSize,
				pMyIoctl,
				Struct_BufferSize,
				&dw,
				NULL);

			DWORD err=GetLastError();

			unsigned long NVMeReturnCode = pMyIoctl->SrbIoCtrl.ReturnCode;
			unsigned int Status_Code=(pMyIoctl->CplEntry[3] >> 17) & 0xFF;
			unsigned int Status_Code_Type=(pMyIoctl->CplEntry[3] >> 25) & 0x07;

			if(!status) //DeviceIoControl fail-> Status=0
			{
				if ( !silence ) {
					_DEBUGPRINT("%s status %d getlasterror %u\n",__FUNCTION__,status,err );
				}
				rtn = err;
			}
			else
			{
				if (NVMeReturnCode != NVME_IOCTL_SUCCESS)
				{
					if ( !silence ) {
						_DEBUGPRINT("%s NVMeReturnCode %u\n",__FUNCTION__,NVMeReturnCode );
					}
					rtn = -1;
				}
				else if (Status_Code || Status_Code_Type ) {
					if ( !silence ) {
						_DEBUGPRINT("%s Status_Code %u Status_Code_Type %u\n",__FUNCTION__,Status_Code,Status_Code_Type );
					}
					rtn = -1;
				}
				else
				{
					//if (OPCODE==ADMIN_IDENTIFY)
					//	pIdCtrlr = (PADMIN_IDENTIFY_CONTROLLER)pMyIoctl->DataBuffer;
					if (direct==1 || OPCODE==ADMIN_IDENTIFY)
						memcpy(buf, pMyIoctl->DataBuffer, bufLength);

					rtn = 0;
				}
			}
		}
		catch (...)
		{
			//CloseHandle(DeviceHandle1);
			rtn = -1;
		}
	} 
	else if (IF_JMS583 == connector) {
		NVMe_COMMAND				 tempNvmeCmd;
		memset((void*)&tempNvmeCmd, 0, sizeof(NVMe_COMMAND));
		memcpy((void *)&tempNvmeCmd, (void*)pCmd, sizeof(NVMe_COMMAND));
		int scsi_direction = SCSI_IOCTL_DATA_UNSPECIFIED;
		if (1 == direct) {
			scsi_direction = SCSI_IOCTL_DATA_IN;
		} else if (2 == direct) {
			scsi_direction = SCSI_IOCTL_DATA_OUT;
		}

		rtn = JMS583_Bridge_Flow(hUsb,tempNvmeCmd, scsi_direction, false, true, timeOut, buf, (int)bufLength);

	} 
	else {
		assert(0);
	}

	if (myIoctlBuf)
		free(myIoctlBuf);

	return rtn;
}
#endif

int USBSCSICmd::MyIssueCmdHandle (  HANDLE hUsb, unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout, int deviceType, int cdb_length )
{
	BOOL status = FALSE;
	unsigned long length = 0, returned = 0;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	::SetLastError(0);
	_SCSIPtBuilder1 ( &sptdwb, cdb_length, cdb, !bufLength ? SCSI_IOCTL_DATA_UNSPECIFIED : read ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT , bufLength , timeout, deviceType );
	sptdwb.sptd.DataBuffer = bufLength ? buf : NULL;
	length = sizeof ( SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER );
	status = DeviceIoControl (    hUsb,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		NULL );
	DWORD err = GetLastError();

	/*
	Return Value
	If the operation completes successfully, the return value is nonzero.
	If the operation fails or is pending, the return value is zero. To get extended error information, call GetLastError.
	ScsiStatus->Reports the SCSI status that was returned by the HBA or the target device.
	*/
    if ( status ) {
        if ( -sptdwb.sptd.ScsiStatus == 0xFE ) {
            return ( 0 );
        }
        else {
            return ( -sptdwb.sptd.ScsiStatus );
        }
    }

    return ( err );

}

void USBSCSICmd::Init()
{
    _deviceType = IF_USB|DEVICE_USB;
	memset(_fwVersion, 0, sizeof(_fwVersion));
	memset(&_FH, 0, sizeof(_FH_Paramater));
}

int USBSCSICmd::Dev_InitDrive( const char* ispfile )
{
	sprintf_s(_ispFile,"%s",ispfile);
	InitFlashInfoBuf ( 0,0, 0, 1 );

  //  //init flash
  //  if ( InitFlashInfoBuf ( 0, 0, 1 ) ) {
		////try again
  //      if ( InitFlashInfoBuf ( 0, 0, 1 ) ) {
  //          _DEBUGPRINT( "Init fail\n" );
  //          return false;
  //      }
  //  }

    for ( int loop=0; loop<3; loop++ ) {
        //-- Must back to ROM code --
        if ( Dev_CheckJumpBackISP ( true,false ) !=0 )        {
            Log (LOGDEBUG, "### Err: Can't jump to ROM code\n" );
        }
        else {
            break;
        }
        ::Sleep ( 1000 );
    }
    Log (LOGFULL, "\n" );

    if ( !USB_ISPInstall (ispfile,false ) )  {
        if ( !USB_ISPInstall ( ispfile,false ) )  {
            _DEBUGPRINT ( "USB_ISPInstall fail\n" );
            //cmd->MyCloseHandle(&_hUsb);
            return 1;
        }
    }
	unsigned char rBuf[528];
	USB_Inquiry ( rBuf, true );
	if ( memcmp ( &rBuf[0x15],"SORTER",6 ) ==0 ) // only Vth tool fw do SmartTestInit
	{
	    USB_SmartTestInit();
	}
	return 0;
}

int USBSCSICmd::Dev_GetFlashID(unsigned char* r_buf)
{
    return USB_GetFlashID ( r_buf,0x00,0 );
}

void USBSCSICmd::Dev_GenSpare_32(unsigned char *buf,unsigned char mode,unsigned char FrameNo,unsigned char Section, int buf_len)
{
	return;
}

int USBSCSICmd::Dev_CheckJumpBackISP ( bool needJump,bool bSilence )
{
    unsigned char    buf[528];

    Log (LOGDEBUG, "+ Jump ISP:" );
    for ( int retry=0; retry<5; retry++ ) {
        //UpdateProgress();                                            // Update UI message
        if ( !USB_Inquiry ( buf ) ) {
            char    tmpstr[128];
			sprintf_s( tmpstr,"   - INQ:" );
            for ( int x=0; x<36; x++ )    {
                tmpstr[x+9] = ( ( buf[x]<' ' ) || ( buf[x]>'z' ) ) ? '.' : buf[x];
            }
            tmpstr[36+9]=0;
            if ( !bSilence ) {
                Log (LOGDEBUG, "%s\n",tmpstr );
            }

            if ( memcmp ( &buf[21],"PRAM",4 ) ==0 ) {
                if ( !bSilence )    {
                    Log (LOGDEBUG, "   - Stage: ROM code." );
                }
                return ( 0 );        // it's confirmed, in ROM stage
            }

            if ( needJump ) {
                if ( !bSilence )    {
                    Log (LOGDEBUG, "   * Jump ROM.\n" );
                }
                USB_JumpBackISP ();
                ::Sleep ( 1000 );
                //if(USB_JumpBackISP())                    return(1);        // Jump fail
            }
        }
        else {
            if ( !bSilence )    {
                Log (LOGDEBUG, "   ### INQ failed ###\n" );
            }
        }
        ::Sleep ( 500 );
    }

    //-- Error --
    Log (LOGDEBUG, "### Err: Check USB_JumpBackISP process fail.\n" );
    return ( 2 );
}


bool USBSCSICmd::USB_ISPInstall ( const char* ispfilename, bool bSilence )
{
    long    isplen,n;
    unsigned char*    bnISPbuf;
    char    sMsg[512];

    Log (LOGDEBUG, "+ Install ISP : %s\n",ispfilename );

    //-- Open ISP Code --
	sprintf_s( sMsg, "%s",ispfilename );
	errno_t err;
	FILE *ispfp;
	err = fopen_s(&ispfp, sMsg,"rb" );
    if ( ispfp==NULL ) {
        Log (LOGDEBUG, "### Err: ISP file not found\n" );
        return ( false );
    }

    //-- Check File Length --
	fseek ( ispfp, 0L, SEEK_END );
    isplen = ftell ( ispfp );
	fseek ( ispfp,0,SEEK_SET );
		//filelength ( fileno ( ispfp ) );
    if ( isplen<=512 ) {
        Log (LOGDEBUG, "### Err: ISP file too small(len=%d)\n",isplen );
        fclose ( ispfp );
        return ( false );
    }

    //-- Allocate Buffer for Burner --
    bnISPbuf = ( unsigned char* ) malloc ( isplen );
    //-- Read Burner ISP into Buffer for later used --
	if (bnISPbuf) {
		n = fread(bnISPbuf, 1, isplen, ispfp);

		if (n != isplen) {
			Log(LOGDEBUG, "### Err: Burner ISP read fail (%d <> %d)\n", n, isplen);
			free(bnISPbuf);
			return (false);
		}
	}
	else {
		Log(LOGDEBUG, "### Err: Allocate Buffer for Burner fail\n");
		fclose(ispfp);

		return (false);
	}
    fclose ( ispfp );

    bool ret  = USB_ISPInstallBuf ( bnISPbuf, isplen, bSilence );
    free ( bnISPbuf);
    return ret;
}


bool USBSCSICmd::USB_ISPInstallBuf ( const unsigned char* bnISPbuf, int isplen, bool bSilence )
{
    unsigned char*    tmpbuf;
    int        iRT,s;
    unsigned char    status[8];
    int        Sec,Ln;

    Log (LOGDEBUG, "+ Install ISP :\n" );

    if ( isplen<=512 ) {
        Log (LOGDEBUG, "### Err: ISP file too small(len=%d)\n",isplen );
        return ( false );
    }

    //-- Allocate Buffer for Burner --
    tmpbuf = ( unsigned char* ) malloc ( 8192 );        // 8K for verify
    if ( tmpbuf==NULL ) {
        Log (LOGDEBUG, "### Err: ISP allocate buffer fail.\n" );
        if ( tmpbuf!=NULL ) {
            free ( tmpbuf );
        }
        return ( false );
    }

    //-- Retry 3 to install ISP code --
    for ( int loop=0; loop<3; loop++ ) {
        //-- Must back to ROM code --
        if ( Dev_CheckJumpBackISP ( true,bSilence ) !=0 )        {
            Log (LOGDEBUG, "### Err: Can't jump to ROM code\n" );
            goto ErrRetry;
        }
        ::Sleep ( 1000 );

        iRT = 0;
        //-- Install ISP Header (512B) --
        Log (LOGDEBUG, "Install ISP Header (512B) loop=%d\n",loop );
        if ( iRT=USB_TransferISPData ( 0,512,0,3,(unsigned char*)bnISPbuf ) )            {
            Log (LOGDEBUG, "### Err: Install Header fail\n" );
            goto ErrRetry;
        }
        if ( iRT=USB_GetISPStatus ( status ) )                        {
            Log (LOGDEBUG, "### Err: Check Header Status fail\n" );
            goto ErrRetry;
        }
        if ( status[0]!=ISP_CONFIG_OK ) {
            Log (LOGDEBUG, "### Err: Install Header Status(%02X) fail",status[0] );
            //if (m_DummyWL==0)        { iRT=3; goto ErrRetry; }                                // Jorry 2012.02.16
        }

        //-- Install ISP Body --
        Log (LOGDEBUG, "Install ISP Body loop=%d\n",loop );
        Sec = ( isplen/512 ) + ( ( ( isplen%512 ) !=0 ) ? 1 : 0 ) - 1;
        Ln = Sec*512;
        if ( iRT=USB_TransferISPData ( 0,Ln,0,2,(unsigned char*)&bnISPbuf[512] ) )        {
            Log (LOGDEBUG, "### Err: Install Body fail\n" );
            goto ErrRetry;
        }
        if ( iRT=USB_GetISPStatus ( status ) )                        {
            Log (LOGDEBUG, "### Err: Check Body Status fail\n" );
            goto ErrRetry;
        }
        if ( status[0]!=ISP_PROG_OK )                {
            Log (LOGDEBUG, "### Err: Install Body Status(%02X) fail\n",status[0] );
            iRT=6;
            goto ErrRetry;
        }

        if ( iRT!=0 ) {
            goto ErrRetry;
        }

        //-- Read Back and Verify --
        for ( s=0; s<Sec; s+=16 ) {
            int slen = Sec-s;
            if ( slen>16 )    {
                slen=16;
            }
            iRT=USB_BufferReadISPData ( s,slen,tmpbuf );
            if ( iRT!=0 ) {
                break;
            }

            if ( memcmp ( &bnISPbuf[s*512+512],tmpbuf,slen*512 ) !=0 ) {
                Log (LOGDEBUG, "### Err: ISP code compare fail (%d)\n",s );
                iRT = 7;
                break;
            }
        }

        if ( iRT==0 ) {
            break;
        }

        //-- Fail, reCreate Handle and try again --
ErrRetry:
        Log (LOGDEBUG, "+ Install ISP fail, retry-%d.\n",loop+1 );
        //CloseHandle();
        //m_hUsb=INVALID_HANDLE_VALUE;
        ::Sleep ( 1000 );

        //-- Re Get Handle --
        //m_hUsb = OpenDevice( fTask->DriverLetter );
        //if (m_hUsb==INVALID_HANDLE_VALUE )        { Log( "### Err: Can't open device as HANDLE" );    iRT=1; break; }
        ::Sleep ( 100 );
    }
    //if (!bSilence)        UpdateProgress();

    //-- free allocated buffer --
    free ( tmpbuf );

    if ( iRT!=0 ) {
        Log (LOGDEBUG, "ERR_LOADSMARTISP\n" );
        return ( false );
    }
    Log (LOGDEBUG, "   - SmartTest ISP Code Installed.\n" );

    //-- Jump to Burner ISP Code --
    iRT = USB_ISPSwitchMode ( 0 );        // 0:Jump then CSW, 2:CSW first (for New Burner)
    if ( iRT!=0 ) {
        Log (LOGDEBUG, "ERR_BURNERBOOTUPFAIL\n" );
        return ( false );
    }
    if ( !bSilence ) {
        Log (LOGDEBUG, "   - ISP JUMP OK.\n" );
    }


    //-- Get Inquiry --                                                                        // Jorry 2012.06.14
    ::Sleep ( 1000 );
    unsigned char    buf[528];
    if ( USB_Inquiry ( buf ) ==0 ) {
        char    tmpstr[128];
		sprintf_s( tmpstr,"- INQ:" );
        for ( int x=0; x<36; x++ )    {
            tmpstr[x+6] = ( ( buf[x]<' ' ) || ( buf[x]>'z' ) ) ? '.' : buf[x];
        }
        tmpstr[36+6]=0;
        Log (LOGDEBUG, "%s\n",tmpstr );
    }
    else    {
        Log (LOGDEBUG, "*** INQUIRY Failed ***\n" );
    }

//if ( _bankNo>0 ) {
//		assert( NewBurner_SwitchBank ( (unsigned char*)bnISPbuf,_bankNo )==0);
//	}

    return ( true );
}


int USBSCSICmd::USB_SmartTestInit ()
{
    //-- Check New Burner installed OK or Not ? by check version string --
    //char    version[32];
    char    version[528];
    //sprintf(version,"+ Smart Test ISP Version : ");
    if ( USB_GetFWVersion ( version ) !=0 ) {
        Log (LOGDEBUG, "### Err: Cannot get ISP version.\n" );
        return 1;
    }
    Log (LOGDEBUG, "+ Smart Test ISP Version : %s\n",version );

    //-- Check GPIO status --                                        // Jorry 2012.03.15
   // unsigned char    iobuf[4];
    //Log ( "SmartTest NewSort_Get_GPIO \n" );
    /*if ( NewSort_Get_GPIO ( iobuf ) ) {
        Log (LOGDEBUG, "### Err: NewSort_Get_GPIO.\n" );
        return 1;
    }
    Log (LOGDEBUG, "+ GPIO = %02X,%02X,%02X,%02X\n", iobuf[0],iobuf[1],iobuf[2],iobuf[3] );*/

    // kelvinlam 把ce 的接法算出. 再給burner
    USB_ReArrangeCE ();

    //unsigned char parameters[528];
    MyFlashInfo* inst = MyFlashInfo::Instance();
    

    inst->ParamToBufferUPTool ( &_FH, _PARAMETERS ); // 512

    USB_SetFlashParameters ( _PARAMETERS );
    ::Sleep ( 200 );


    {
        unsigned char    bCBD[16];
        unsigned char    buff[6];
        memset ( bCBD, 0, sizeof ( bCBD ) );
        memset ( buff, 0 , sizeof(buff) );

        bCBD[0] = 0x06;
        bCBD[1] = 0xFA;
        int ret = MyIssueCmd ( bCBD,_CDB_READ,buff,sizeof(buff) );
        if ( ret==0 ) {
            //BC_CMD_BUF_BASE=     buff[0];
            //DEFAULT_CMD_BUF_BASE=buff[1];
            //USER_CMD_BUF_BASE=   buff[2];
            //DBG_CMD_BUF_BASE=    buff[3];
            //ECC_CMD_BUF_BASE=    buff[4];
            _eccBufferAddr  = buff[4];
			_dataBufferAddr = buff[5];
			_sql = SQLCmd::Instance(this, &_FH, _eccBufferAddr, _dataBufferAddr);
			Log (LOGDEBUG, "_eccBufferAddr=%02X _dataBufferAddr=%02X\n", _eccBufferAddr,_dataBufferAddr );
        }
        //assert(_eccBufferAddr!=0);
    }

    return 0;
}


void USBSCSICmd::USB_ReArrangeCE ( )
{
    int        ce;
    unsigned char    ID[512];
    unsigned char    CEMask,Mode3;
    unsigned char    mfr=0xAA,Device=0x55;

    //-- Get RAW ID --
    USB_GetFlashID ( ID,0x00,0 );

    //-- Step.1: Find 1st Flsh ID
    for ( ce=0; ce<8; ce++ ) {
        if ( ( ID[ce*16+0]==0 ) || ( ID[ce*16+0]==0xFF ) || ( ID[ce*16+1]==ID[ce*16+0] ) ) {
            continue;    // Not good ID        // Jorry 2011.12.13
        }

        mfr     = ID[ce*16+0];
        Device = ID[ce*16+1] & 0xCF;
        break;
    }

    //-- Step.2: Find CE Mask --
    // kelvinlam CE--controller CE pins Mapping.
    CEMask = 0;
    for ( ce=0; ce<8; ce++ ) {
        Log (LOGDEBUG, "   ID-%d: %02X-%02X...\n",ce,ID[ce*16],ID[ce*16+1] );
        if ( ( ID[ce*16+1]&0xCF ) ==Device ) {
            CEMask |= ( 1<<ce );
        }
    }

    //-- Mode, check --
    if ( CEMask==0x11 ) {
        Mode3 = 0x02;    // [0,4]-->[0,1]
    }
    else if ( ( CEMask==0x05 ) || ( CEMask==0x55 ) )    {
        Mode3 = 0x04;    // [0,2]-->[0,1], [0,2,4,6]-->[0,1,2,3]
    }
    else if ( CEMask==0x33 ) {
        Mode3 = 0x06;    // [0,1,4,5]-->[0,1,2,3]
    }
    else {
        Mode3 = 0x00;    // [0,1,2,3...]-->[0,1,2,3....]
    }

    //-- Set New Mode3 and Read ID again --
    USB_GetFlashID ( ID,Mode3,0 );
    Log (LOGDEBUG, " %s  - New CE Mode_3 = %02X\n",__FUNCTION__,Mode3 );
}

//--------------------------------------------------------------------------------------------------------------
int USBSCSICmd::USB_GetFlashID ( unsigned char* buf, unsigned char Mode3, unsigned char Decoder )
{
    unsigned char    bCBD[16];
    memset ( bCBD, 0, sizeof ( bCBD ) );

    bCBD[0] = 0x06;
    bCBD[1] = 0x56;
    bCBD[2] = Mode3;    // CE arrange mode
    bCBD[3] = Decoder;
    int err = MyIssueCmd ( bCBD,_CDB_READ,buf,512 );
    Log (LOGDEBUG,"%s ret=%d\n", __FUNCTION__, err );
    return err;
}


int USBSCSICmd::USB_GetFWVersion ( char* ibuf )
{
    unsigned char    bCBD[16];
	int err=0;
    memset ( bCBD, 0, sizeof ( bCBD ) );

    bCBD[0] = 0x06;
    bCBD[1] = 0xE6;
    bCBD[2] = 0x00;

	err = MyIssueCmd ( bCBD,_CDB_READ, ( unsigned char* ) ibuf,32 );

    return err;
}


int USBSCSICmd::USB_SetFlashParameters ( unsigned char* FlashPara )
{
    unsigned char    cdb[16];

    memset ( cdb , 0 , 16 );
	int err = 0;

    cdb[0] = 0x06;
    cdb[1] = 0xE6;
    cdb[2] = 0x01;

    err = MyIssueCmd ( cdb,_CDB_WRITE,FlashPara,512 );
    LogMEM ( FlashPara,512 );

    return err;
}


int USBSCSICmd::USB_Inquiry ( unsigned char* r_buf, bool all )
{
    memset ( r_buf , 0 , 528 );
    unsigned char cdb[16];
    memset ( cdb , 0 , 16 );
    cdb[0] = 0x12;
    cdb[4] = all ? 0x44:0x24;

    int err = 0;
    for ( int retry=0; retry<3; retry++ ) {        // Jorry
        err = MyIssueCmd ( cdb,true,r_buf,44,1 );        // Time-out=1 sec
        if ( err==0 ) {
            break;
        }
        ::Sleep ( 200 );
    }

    return err;
}

int USBSCSICmd::USB_JumpBackISP ( )
{
    unsigned char    cdb[16];
    memset ( cdb , 0 , 16 );
    cdb[0] = 0x06;
    cdb[1] = 0xB3;
    cdb[2] = 0x02;        // b0=DISC, 1=Respone CSW                                                // Jorry 2012.05.15
    // b1 must NOT turn OFF (ROM Bug)
    cdb[4] = 0x08; //fix for 2307

    int err = MyIssueCmd ( cdb,false,NULL,0,10 );        // Longer Time-out for SORTING ISP Code's bug (Search F/W flag)
    return err;
}

int USBSCSICmd::USB_TransferISPData ( int offset,int xlen,unsigned char verify,unsigned char mode,unsigned char* buf )
{
    int                iResult;
    unsigned char    bCBD[16];

    memset ( bCBD, 0, sizeof ( bCBD ) );

    bCBD[0] = 0x06;
    bCBD[1] = verify ? 0xb2 : 0xb1;
    bCBD[2] = mode;
    bCBD[3] = ( unsigned char ) ( ( offset >> 9 ) >> 8 );
    bCBD[4] = ( unsigned char ) ( offset >> 9 );
    bCBD[5] = 0x00;
    bCBD[6] = 0x00;
    bCBD[7] = ( unsigned char ) ( ( xlen >> 9 ) >> 8 );
    bCBD[8] = ( unsigned char ) ( xlen >> 9 );
    bCBD[9] = 0x00;

    if ( mode&0x10 ) {
        iResult = MyIssueCmd ( bCBD,_CDB_READ, buf,xlen,10 );    // Read Mode
    }
    else {
        iResult = MyIssueCmd ( bCBD,_CDB_WRITE,buf,xlen,10 );    // Write Mode
    }

    return ( iResult );
}

int USBSCSICmd::USB_GetISPStatus ( unsigned char* r_buf )
{
    int                iResult;
    unsigned char    bCBD[16];

    memset ( bCBD, 0, sizeof ( bCBD ) );
    memset ( r_buf , 0 , 8 );

    bCBD[0] = 0x06;
    bCBD[1] = 0xb0;
    bCBD[2] = 0x00;
    bCBD[3] = 0x00;
    bCBD[4] = 0x08;
    bCBD[5] = 0x00;
    iResult = MyIssueCmd ( bCBD,_CDB_READ,r_buf,8 );
    return ( iResult );
}


int USBSCSICmd::USB_BufferReadISPData ( unsigned long Offset, int SecCnt, unsigned char* buf )
{
    int                iResult;
    unsigned char    bCBD[16];

    memset ( bCBD , 0 , sizeof ( bCBD ) );
    bCBD[0] = 0x06;
    bCBD[1] = 0x21;
    bCBD[2] = 0x10;                                    // 0x10 for read
    bCBD[3] = ( unsigned char ) ( ( Offset>>8 ) &0xff );
    bCBD[4] = ( unsigned char ) ( Offset &0xff );
    bCBD[7] = ( unsigned char ) ( ( SecCnt>>8 ) &0xff );
    bCBD[8] = ( unsigned char ) ( SecCnt&0xff );

    for ( int retry=0; retry<3; retry++ ) {
        iResult = MyIssueCmd ( bCBD,_CDB_READ,buf,SecCnt*512 );
        if ( iResult==0 ) {
            break;
        }
        ::Sleep ( 200 );
    }
    return ( iResult );
}

int USBSCSICmd::USB_ISPSwitchMode ( unsigned char mode )
{
    // Mode bit0:    1=Disconnect,            0=No disconnect
    //        bit1:    1=CSW then Jump,        0=Jump then CSW
    //        bit7:    1=Force ROM search CZ,    0=disable ROM search
    int iResult = 0;
    unsigned char bCBD[16];

    memset ( bCBD , 0 , sizeof ( bCBD ) );
    bCBD[0] = 0x06;
    bCBD[1] = 0xB3;
    bCBD[2] = mode | 0x04;    // BootROM --ISP burner--> burner = 0, Burner --ISP firmware--> BootROM = 1.
    // b0 & b1 must NOT turn ON (ROM Bug)

    bCBD[3] = 0xFF;

    iResult = MyIssueCmd ( bCBD,_CDB_NONE,NULL,0,30 );        // timeout=30 sec ?
    ::Sleep ( 100 );

    //-- mode-0, CSW is always fail on TAG --                        // Jorry 2011.09.08
    if ( mode==0 ) {
        return ( 0 );
    }

    return ( iResult );
}

int USBSCSICmd::Dev_GetSRAMData ( unsigned char offset, unsigned char* buf, int len )
{
//    if ( !_Mode2306 ) {
//        //assert(len>=9216);
//        assert(len>=512);
//    }
    unsigned char cdb[16];
    memset ( cdb,0,16 );
    cdb[0] = 0x06;
    cdb[1] = 0xFD;
    cdb[2] = offset;
    cdb[3] = len/512;
    int err = MyIssueCmd ( cdb,(len? _CDB_READ:_CDB_WRITE), buf, len );
    //Log ( __FUNCTION__,"%s ret=%d\n", _cdb, err );
    return err;
}

int USBSCSICmd::Dev_SetSRAMData ( unsigned char offset, const unsigned char* buf, int len )
{
//    if ( !_Mode2306 ) {
//        //assert(len>=9216);
//        assert(len>=512);
//    }
    unsigned char cdb[16];
    memset ( cdb,0,16 );
    cdb[0] = 0x06;
    cdb[1] = 0xFC;
    cdb[2] = offset;
    cdb[3] = len/512;
    int err = MyIssueCmd ( cdb,_CDB_WRITE, (unsigned char*)buf, len );
    return err;
}

int USBSCSICmd::Dev_SetFlashClock ( int timing )
{
    unsigned char    cdb[16];
    memset ( cdb,0,16 );
    cdb[0] = 0x06;
    cdb[1] = 0xE6;
    cdb[2] = 0x02;
    cdb[3] = 0x00;  // timing
    cdb[4] = timing;

    int err = MyIssueCmd ( cdb, _CDB_NONE, 0, 0 );
    return err;
}
int USBSCSICmd::Dev_SetFlashDriving ( int driving )
{
    unsigned char    cdb[16];
    memset ( cdb,0,16 );
    cdb[0] = 0x06;
    cdb[1] = 0xE6;
    cdb[2] = 0x02;
    cdb[3] = 0x02;  // driving
    cdb[4] = 0x01;// pull up
	cdb[5] = 0x00;// pull down
	cdb[6] = driving;
    int err = MyIssueCmd ( cdb, _CDB_NONE, 0, 0 );
    return err;
}
int USBSCSICmd::Dev_GetRdyTime ( int action, unsigned int* busytime )
{
    unsigned char  cdb[16];
    memset ( cdb,0,16 );
    cdb[0] = 0x06;
    cdb[1] = 0xE6;
    cdb[2] = 0x09;
    cdb[3] = action;

    int err=0;
    if ( action!=2 ) {
        unsigned char t[4];
        err = MyIssueCmd ( cdb,_CDB_READ, t, 4 );
        *busytime = (t[3]<<24) | (t[2]<<16) | (t[1]<<8) | t[0];
    }
    else {
        err = MyIssueCmd ( cdb,_CDB_WRITE, 0, 0 );
    }
    return err;
}
int USBSCSICmd::Dev_SetEccMode ( unsigned char Ecc )
{
    unsigned char    cdb[16];
    memset ( cdb,0,16 );
    cdb[0] = 0x06;
    cdb[1] = 0xE6;
    cdb[2] = 0x02;
    cdb[3] = 0x01;  // Ecc
    cdb[4] = Ecc;

    int err = MyIssueCmd ( cdb, _CDB_NONE, 0, 0 );
    return err;
}
int USBSCSICmd::Dev_SetFWSpare ( unsigned char Spare )
{
    unsigned char    cdb[16];
    memset ( cdb,0,16 );
    cdb[0] = 0x06;
    cdb[1] = 0xE6;
    cdb[2] = 0x02;
    cdb[3] = 0x04;  // Spare
    cdb[4] = Spare;

    int err = MyIssueCmd ( cdb, _CDB_NONE, 0, 0 );
    return err;
}

int USBSCSICmd::Dev_IssueIOCmd( const unsigned char* sql,int sqllen, int mode, int ce, unsigned char* buf, int len )
{
    //if ( len>_sramSize ) {
    //    return _Dev_IssueIOCmd9K( sql,sqllen, mode, ce, buf, len );
    //}
    //0=write,1=write+buf,2=read
    int err = 0;
    int hmode = mode&0xF0;
    mode = mode&0x0F;

    if ( !hmode ) {
        assert(sql[sql[1]+3-1]==0xCE);
    }

    if ( (mode&0x03)==1 ) {
        unsigned char cdb[16] = {0x06,0xFC,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
		cdb[2] = _dataBufferAddr;
        cdb[3] = len/512;
        err = MyIssueCmd ( cdb,_CDB_WRITE, buf, len );
        if ( err ) {
            Log (LOGDEBUG, "write _Dev_IssueIOCmd %s ret=%d\n", cdb, err );
            LogMEM ( sql,sqllen );
            LogFlush();
        }
        assert(err==0);
    }

    //trigger command
    if ( !hmode )  {
        unsigned char cmdbuf[512];
        memcpy(cmdbuf,sql,sqllen);
        unsigned char cdb[16] = {0x06,0xFE,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
        cdb[5] = mode!=3 ? len/512:_FH.PageSizeK;
        cdb[6] = ce;
        cdb[7] = mode;
        //Log("%s\n",__FUNCTION__);
        //LogMEM ( cmdbuf, sizeof(cmdbuf) );
        err = MyIssueCmd ( cdb,_CDB_WRITE, cmdbuf, sizeof(cmdbuf) );
        if ( err ) {
            Log (LOGDEBUG, "trigger _Dev_IssueIOCmd %s ret=%d\n", cdb, err );
            LogMEM ( sql,sqllen );
            LogFlush();
        }
        assert(err==0);
        if ( mode==2 ) {
            err = Dev_GetSRAMData ( _dataBufferAddr, buf, len );
        }
        else if ( mode==3 ) {
            //read ecc
            assert( len>=_FH.PageFrameCnt );
            err = Dev_GetSRAMData ( _eccBufferAddr, cmdbuf, 512 );
            if ( err ) {
                Log (LOGDEBUG, "get SRAM _Dev_IssueIOCmd %s ret=%d\n", cdb, err );
                LogMEM ( sql,sqllen );
                LogFlush();
            }
            assert(err==0);
            memcpy(buf,cmdbuf,_FH.PageFrameCnt);
        }
    }
    return err;
}