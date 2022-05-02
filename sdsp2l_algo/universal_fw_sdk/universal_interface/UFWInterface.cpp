
#include "UFWInterface.h"
//#include <windows.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <ntddscsi.h>
#include <winioctl.h>
    #include <stddef.h>
#include "spti.h"
#include "nvme.h"
#include "nvmeIoctl.h"
#include "nvmeReg.h"

#include <windows.h>
#include <winioctl.h>
#else
    #include <stdint.h>
    #include <sys/ioctl.h>
    #include <sys/file.h>
    #include <fcntl.h>
#endif
//static int UvsDebugMode;


DeviveInfo::DeviveInfo()
{
	drive		= 0;
	physical	= 0;
	nvme		= false;
	label[0]	= '\0';
}

void UFWInterface::Init(DeviveInfo* info)
{
	_deviceType = 0;
	_hUsb = INVALID_HANDLE_VALUE;
    _info.drive = info->drive;
    _info.nvme = info->nvme;
	if(strlen(info->label) >64)
	{
		info->label[0] = '\0';
	}
    sprintf_s(_info.label,"%s",info->label);
#ifndef WIN32
    strcpy(_info.physical,info->physical);
#else
    _info.physical = info->physical;
#endif
	_timeOut = 10;
}

UFWInterface::UFWInterface(DeviveInfo* info, const char* logFolder)
#ifndef WIN32
	:UFWLoger(logFolder, info->drive, info->nvme)
#else
	: UFWLoger(logFolder, info->drive ? info->drive : info->physical, info->drive ? false : true)
#endif
{
	Init(info);
}

UFWInterface::UFWInterface(DeviveInfo* info, FILE *logPtr)
:UFWLoger( logPtr )
{
	Init(info);
}

UFWInterface::~UFWInterface()
{
    if (_hUsb!= INVALID_HANDLE_VALUE ) {
        _static_MyCloseHandle(&_hUsb);
    }
}

const DeviveInfo * UFWInterface::GetDriveInfo()
{
	return &_info;
}
int UFWInterface::GetDeviceType()
{
	return _deviceType;
}
void UFWInterface::SetDeviceType(int deviceType)
{
	_deviceType = deviceType;
	
    if (_hUsb!= INVALID_HANDLE_VALUE ) {
        _static_MyCloseHandle(&_hUsb);
		_hUsb = INVALID_HANDLE_VALUE;
    }
	
	_hUsb = _static_MyCreateHandle(_info.physical,true,IF_NVNE==(_deviceType&INTERFACE_MASK)); //physical
}

HANDLE UFWInterface::GetHandle()
{
	return _hUsb;
}

void UFWInterface::SetTimeOut( int timeout )
{
	assert(timeout>0);
	_timeOut = timeout;
}

#ifndef WIN32
HANDLE UFWInterface::_static_MyCreateNVMeHandle ( char* devicepath )
#else
HANDLE UFWInterface::_static_MyCreateNVMeHandle ( unsigned char scsi )
#endif
{
#ifndef WIN32
    return _static_MyCreateHandle ( devicepath, false, false );
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
HANDLE UFWInterface::_static_MyCreateHandle ( char* devicepath, bool physical, bool nvmemode )
#else
HANDLE UFWInterface::_static_MyCreateHandle ( unsigned char drive, bool physical, bool nvmemode )
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
        hUsb = _static_MyCreateNVMeHandle ( drive );
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

void UFWInterface::_static_MyCloseHandle( HANDLE *hUsb )
{
	//_DEBUGPRINT("MyCloseHandle %p\n",*hUsb);
#ifndef WIN32
    close(*hUsb);
#else
    CloseHandle(*hUsb);
#endif
	hUsb = 0;
}


int _SCSIPtBuilder1 ( SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER* sptdwb ,unsigned int cdbLength , unsigned char* CDB ,
                                  unsigned char direction , unsigned int transferLength ,unsigned long timeOut, int deviceType )
{
    if ( 0 > cdbLength || 16 < cdbLength ) {
        return -1;
    }
	
	int connector = (deviceType&INTERFACE_MASK);

    ZeroMemory ( sptdwb , sizeof ( SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER ) );
    if ( IF_ASMedia==connector /*|| IF_JMS578==connector*/ ) {
        sptdwb->sptd.SenseInfoLength = 32;
        sptdwb->sptd.Length = sizeof ( SCSI_PASS_THROUGH_DIRECT );
        //sptdwb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
        //sptdwb->sptd.PathId = 0;
        //sptdwb->sptd.TargetId = 0;
        //sptdwb->sptd.Lun = 0;
        //sptdwb->SenseInfoLength = sizeof(ucSenseBuf);
        sptdwb->sptd.CdbLength = cdbLength;
        sptdwb->sptd.TimeOutValue = timeOut;
        sptdwb->sptd.DataIn = direction;
        sptdwb->sptd.DataTransferLength = transferLength;
        sptdwb->sptd.SenseInfoOffset = offsetof ( SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER , ucSenseBuf );
        memcpy ( sptdwb->sptd.Cdb , CDB , cdbLength );
    }
    else if ( IF_USB==connector || IF_C2012==connector || IF_JMS583==connector)
	{
        sptdwb->sptd.SenseInfoLength = 24;
        sptdwb->sptd.Length = sizeof ( SCSI_PASS_THROUGH );
        sptdwb->sptd.PathId = 0;
        sptdwb->sptd.TargetId = 0;
        sptdwb->sptd.Lun = 0;
        sptdwb->sptd.CdbLength = cdbLength;
        sptdwb->sptd.TimeOutValue = timeOut;
        sptdwb->sptd.DataIn = direction;
        sptdwb->sptd.DataTransferLength = transferLength;
        sptdwb->sptd.SenseInfoOffset = offsetof ( SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER , ucSenseBuf );
        memcpy ( sptdwb->sptd.Cdb , CDB , cdbLength );
    }
	else {
		assert(0);
	}
    return 0;
}

#ifndef WIN32
int UFWInterface::MyIssueCmdHandle (  HANDLE hUsb, unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout, int deviceType )
{
	if(!bufLength){
		return Scsi_Basic::Issue_Scsi_Cmd(hUsb, 16, cdb, SG_DXFER_NONE, timeout, (uint8_t*)buf, bufLength);
	}
	else if(read){
		return Scsi_Basic::Issue_Scsi_Cmd(hUsb, 16, cdb, SG_DXFER_FROM_DEV, timeout, (uint8_t*)buf, bufLength);
	}
	else{
		return Scsi_Basic::Issue_Scsi_Cmd(hUsb, 16, cdb, SG_DXFER_TO_DEV, timeout, (uint8_t*)buf, bufLength);
	}
}
#else

void UFWInterface::_static_SetDebugMode(int mode){
	//UvsDebugMode = mode;
}


int UFWInterface::_static_MyIssueCmdHandle (  HANDLE hUsb, unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout, int deviceType, int cdb_length )
{
    BOOL status = FALSE;
    unsigned long length = 0, returned = 0;
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;

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
#endif


unsigned short Crc16_Ccitt(unsigned char *buffer, unsigned int length){
	unsigned short crc = 0;
	for(unsigned int i = 0; i < length; i++){
		crc ^= *(char *)buffer++ << 8;
		for(int j = 0; j < 8; j++ ) {
			if( crc & 0x8000 )
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
		}
	}
	return crc;
}
unsigned short Swap_Uint(unsigned short value){
	return (value >> 8) | (value << 8);
}

#ifndef WIN32
int UFWInterface::MyIssueNVMeCmdHandle(HANDLE hUsb, unsigned char *nvmeCmd,unsigned char OPCODE,int direct/*0:NON 1:Read 2:Write*/,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence, int deviceType)
{
	return Nvme_Basic::Issue_Nvm_Cmd(hUsb, (Nvme_Command)nvmeCmd, direct, timeOut, buf, bufLength);
}
#else
int UFWInterface::_static_MyIssueNVMeCmdHandle(HANDLE hUsb, unsigned char *nvmeCmd,unsigned char OPCODE,int direct/*0:NON 1:Read 2:Write*/,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence, int deviceType)
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

		rtn = _static_JMS583_Bridge_Flow(hUsb,tempNvmeCmd, scsi_direction, false, true, timeOut, buf, (int)bufLength);

	} 
	else {
		assert(0);
	}

	if (myIoctlBuf)
        free(myIoctlBuf);

	return rtn;
}
#endif

int _ATAPtBuilder(ATA_PASS_THROUGH_WITH_BUFFERS *aptwb,ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS *aptdwb, unsigned char *PRegister, unsigned char *CRegister, USHORT direction, unsigned long transferLength, unsigned long timeOut, bool Flag, bool Is48bit)
{
    if(Flag)
    {
        ZeroMemory(aptdwb,sizeof(ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS));
        aptdwb->aptd.Length = sizeof(ATA_PASS_THROUGH_DIRECT);
        aptdwb->aptd.AtaFlags = (ATA_FLAGS_DRDY_REQUIRED | direction);
        //aptdwb->aptd.AtaFlags = direction;
        //aptdwb->aptd.PathId = 0;
        //aptdwb->aptd.TargetId = 0;
        //aptdwb->aptd.Lun = 0;
        //aptdwb->aptd.ReservedAsunsigned char = 0;
        aptdwb->aptd.DataTransferLength = transferLength;
        aptdwb->aptd.TimeOutValue = timeOut;
        //aptd.aptd.ReservedAsUlong = 0;

        if(Is48bit)
            memcpy(aptdwb->aptd.PreviousTaskFile, PRegister, 8);

        memcpy(aptdwb->aptd.CurrentTaskFile, CRegister, 8);
    }
    else
    {
        ZeroMemory(aptwb, sizeof(ATA_PASS_THROUGH_WITH_BUFFERS));
        aptwb->apt.Length = sizeof(ATA_PASS_THROUGH_EX);
        aptwb->apt.AtaFlags = ATA_FLAGS_DRDY_REQUIRED | direction;
        aptwb->apt.DataTransferLength = transferLength;
        aptwb->apt.TimeOutValue = timeOut;
        aptwb->apt.DataBufferOffset = offsetof(ATA_PASS_THROUGH_WITH_BUFFERS, ucDataBuf);
        if(Is48bit)
            memcpy(aptwb->apt.PreviousTaskFile, PRegister, 8);

        memcpy(aptwb->apt.CurrentTaskFile, CRegister, 8);
    }

    return 0;
}


unsigned char sat_vendor_cmd_type[16][3] =
{
    {0x85, 0x00, 0x00},  /* 0. SAT_CMD_HARD_RESET */
    {0x85, 0x00, 0x00},  /* 1. SAT_CMD_SRST */
    {0x85, 0x00, 0x00},  /* 2. SAT_CMD_RESERVED_B2 */
    {0xA1, (0x03<<1), 0x00}, /* 3. SAT_CMD_NON_DATA */
    {0xA1, (0x04<<1), (0x01<<3)|(0x1<<2)|0x02}, /* 4. SAT_CMD_PIO_DATA_IN */
    {0xA1, (0x05<<1), (0x1<<2)|0x02}, /* 5. SAT_CMD_PIO_DATA_OUT */
    {0x85, (6<<1)|0x01, (0x01<<2)|0x02},  /* 6. SAT_CMD_DMA */
    {0x85, 0x00, 0x00},  /* 7. SAT_CMD_DMA_QUEUED */
    {0x85, 0x00, 0x00},  /* 8. SAT_CMD_DEVICE_DIAGNOSTIC */
    {0x85, 0x00, 0x00},  /* 9. SAT_CMD_DEVICE_RESET */
    {0xA1, (0x0A<<1), (0x01<<3)|(0x1<<2)|0x02}, /* 10. SAT_CMD_UDMA_DATA_IN */
    {0xA1, (0x0B<<1), (0x1<<2)|0x02},  /* 11. SAT_CMD_UDMA_DATA_OUT */
    {0x85, 0x00, 0x00},  /* 12. SAT_CMD_FPDMA */
    {0x85, 0x00, 0x00},  /* 13. SAT_CMD_RESERVED_B13 */
    {0x85, 0x00, 0x00},  /* 14. SAT_CMD_RESERVED_B14 */
    {0x85, 0x00, 0x00},  /* 15. SAT_CMD_RET_RESPONSE_INFO */
};

#ifndef WIN32
int UFWInterface::_static_MyIssueATACmdHandle(HANDLE hUsb, unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd, int deviceType)
{
	if(direct == 2)//write
		return sg16(hUsb, 1, DMACmd, (Ata_Tf *)cRegister, buf, bufLength, timeOut);
	else
		return sg16(hUsb, 0, DMACmd, (Ata_Tf *)cRegister, buf, bufLength, timeOut);
}
#else
int UFWInterface::_static_MyIssueATACmdHandle(HANDLE hUsb, unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd, int deviceType)
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
        return _static_MyIssueCmdHandle ( hUsb, cdb, direct==2? _CDB_WRITE:_CDB_READ, buf, bufLength,timeOut, deviceType );
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
		oRegister[6]=0x50; 
		return _static_MyIssueCmdHandle ( hUsb, cdb_c2012, direct==2? _CDB_WRITE:_CDB_READ, buf, bufLength,timeOut, deviceType );
	} else {
		assert(0);
	}
	return -1;
}
#endif

int UFWInterface::_static_MyIssueCmd(  unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout )
{
    return _static_MyIssueCmdHandle( _hUsb, cdb, read, buf, bufLength, timeout, _deviceType );
}

int UFWInterface::_static_MyIssueATACmd(unsigned char *pRegister,unsigned char *cRegister,unsigned char *oRegister,int direct,unsigned char *buf,unsigned long bufLength,bool Flag,bool Is48bit,unsigned long timeOut,bool DMACmd)
{
	return _static_MyIssueATACmdHandle( _hUsb, pRegister, cRegister, oRegister, direct, buf, bufLength, Flag, Is48bit, timeOut, DMACmd, _deviceType );
}

int UFWInterface::_static_MyIssueNVMeCmd( unsigned char *nvmeCmd,unsigned char OPCODE,int direct,unsigned char *buf,unsigned long bufLength,unsigned long timeOut, bool silence )
{
	return _static_MyIssueNVMeCmdHandle( _hUsb, nvmeCmd, OPCODE, direct, buf, bufLength, timeOut,  silence, _deviceType );
}





#define SCSI_IOCTL_DATA_OUT          0
#define SCSI_IOCTL_DATA_IN           1
#define SCSI_IOCTL_DATA_UNSPECIFIED  2
int UFWInterface::_static_JMS583_Bridge_Flow(HANDLE hUsb,_NVMe_COMMAND cmd, int data_type, bool ap_key, int admin, int timeout, unsigned char *data, int length)
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

	result = _static_JMS583_Bridge_Send_Sq(hUsb,cmd, timeout, admin);
	if(result != 0)
	{
		_DEBUGPRINT("%s:Nvme Bridge Send SQ Fail\n", __FUNCTION__);
		return result;
	}

	if(data_type == SCSI_IOCTL_DATA_UNSPECIFIED)
	{
		result = _static_JMS583_Bridge_Trigger(hUsb,timeout, admin);
		if(result != 0)
		{
			_DEBUGPRINT("%s:Nvme Bridge Trigger Fail\n", __FUNCTION__);
			return result;
		}
	}
	else if(data_type == SCSI_IOCTL_DATA_OUT)
	{
		result = _static_JMS583_Bridge_Write_Data(hUsb,timeout, admin, data, length);
		if(result != 0)
		{
			_DEBUGPRINT("%s:Nvme Bridge Write Data Fail\n", __FUNCTION__);
			return result;
		}
	}
	else if(data_type == SCSI_IOCTL_DATA_IN)
	{
		result = _static_JMS583_Bridge_Read_Data(hUsb,timeout, admin, data, length);
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

	result = _static_JMS583_Bridge_Read_Cq(hUsb,timeout, admin, cq_buffer);
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

int UFWInterface::_static_JMS583_Bridge_Send_Sq(HANDLE hUsb,_NVMe_COMMAND cmd, int timeout, int admin)
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

	return _static_MyIssueCmdHandle ( hUsb, cdb , false, data , 512, timeout, IF_USB );

}
int UFWInterface::_static_JMS583_Bridge_Read_Cq(HANDLE hUsb,int timeout, int admin, unsigned char *data)
{
	unsigned char cdb[16] = {0};
	int length = 512;

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = (admin << 7) + 15;		//	Return Response Information
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)
	return _static_MyIssueCmdHandle ( hUsb, cdb , true, data , 512, timeout, IF_USB );

}
int UFWInterface::_static_JMS583_Bridge_Trigger(HANDLE hUsb,int timeout, int admin)
{
	unsigned char cdb[16] = {0};
	int length = 0;

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = (admin << 7) + 1;		//	Non-data
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)
	return _static_MyIssueCmdHandle ( hUsb, cdb , false, NULL , 0, timeout, IF_USB );
}
int UFWInterface::_static_JMS583_Bridge_Read_Data(HANDLE hUsb,int timeout, int admin, unsigned char *data, int length)
{
	unsigned char cdb[16] = {0};

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = (admin << 7) + 2;		//	DMA-IN
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)
	return _static_MyIssueCmdHandle ( hUsb, cdb , true, data , length, timeout, IF_USB );
}
int UFWInterface::_static_JMS583_Bridge_Write_Data(HANDLE hUsb,int timeout, int admin, unsigned char *data, int length)
{
	unsigned char cdb[16] = {0};

	cdb[0] = 0xA1;					//	JMS583
	cdb[1] = (admin << 7) + 3;		//	DMA-OUT
	cdb[2] = 0x00;					//	Reserved
	cdb[3] = (length >> 16) & 0xff;	//	PARAMETER LIST LENGTH (23:16)		
	cdb[4] = (length >> 8) & 0xff;	//	PARAMETER LIST LENGTH (15:08)
	cdb[5] = length & 0xff;			//	PARAMETER LIST LENGTH (07:00)

	return _static_MyIssueCmdHandle ( hUsb, cdb , false, data , length, timeout, IF_USB );
}


int UFWInterface::_static_Inquiry ( HANDLE hUsb, unsigned char* r_buf, bool all, unsigned char length )
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
        err = _static_MyIssueCmdHandle(hUsb,cdb,true,r_buf,cdb[4],10,IF_USB);
        if ( err==0 ) {
            break;
        }
        UFWSleep( 200 );
    }

    return err;
}

int UFWInterface::_static_IdentifyATA( HANDLE hUsb, unsigned char* buf, int len, int bridge )
{
    assert(len==512);
    unsigned char pRegister[8],cRegister[8],oRegister[8];
    memset(pRegister,0x00,8);
    memset(cRegister,0x00,8);
    memset(oRegister,0x00,8);
    cRegister[5] = 0xE0;
    cRegister[6] = 0xEC;

    int err = _static_MyIssueATACmdHandle(hUsb,pRegister,cRegister,cRegister,1,buf,len,true,false,1000,false,bridge);
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

bool checkIfJMS583VersionIsNew(unsigned char * inquiryBuf)
{
	if (strstr((char *)&inquiryBuf[8],"V2") ||
		strstr((char *)&inquiryBuf[8],"V3")	||
		strstr((char *)&inquiryBuf[8],"V4")	||
		strstr((char *)&inquiryBuf[8],"V5")	||
		strstr((char *)&inquiryBuf[8],"V6")
		)
		return true;
	else
		return false;
}

int UFWInterface::_static_JMS583_BridgeReset( int physical, bool doI2c ) 
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
	HANDLE hdl = _static_MyCreateHandle ( physical, true, false );
	assert(hdl != INVALID_HANDLE_VALUE);
	
	if (_static_Inquiry(hdl, inq_buffer, true))
	{
		_DEBUGPRINT( "%s:Inquiry fail\n", __FUNCTION__);
		goto failend;
	}
	memcpy(inq_buffer_cache, inq_buffer, sizeof(inq_buffer));


	_static_Bridge_ReadFWVersion(hdl, fw_version);
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
		if (_static_JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)){
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] = 0x18; // Set GPIO3/GPIO4 Input, others Output
		_DEBUGPRINT( "%s:I2c_write cmd 0x03 and write value 0x18 to addr 0x48\n", __FUNCTION__);
		if (_static_JMS583_Bridge_I2c_Write(hdl,0x48, 3, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge I2C Init Fail\n", __FUNCTION__);
			goto failend;
		}

	}

	
	//_DEBUGPRINT( "%s:Bridge Reset\n", __FUNCTION__);
	//if (JMS583_Bridge_Control(hdl, 5, DEFAULT_TIME_OUT)){
	//	_DEBUGPRINT( "%s:Bridge Reset Init Fail\n", __FUNCTION__);
	//	goto failend;
	//}
	//MyCloseHandle(&hdl);
	//UFWSleep(2000);

	//memset(inq_buffer, 0, sizeof(inq_buffer));
	//int wait_cnt = 0;
	//int max_wait_cnt=30;
	//UFWSleep(2000);
	//for (wait_cnt=0; wait_cnt<max_wait_cnt; wait_cnt++) {
	//	hdl = MyCreateHandle ( physical, true, false );
	//	if ( hdl!=INVALID_HANDLE_VALUE) {		
	//		if (Inquiry(hdl, inq_buffer)==0) {
	//			if(strstr((char *)&inq_buffer[8], "JMicron") || strstr((char *)&inq_buffer[8], "Bridge")) {
	//				break;
	//			}
	//		}
	//	}
	//	_DEBUGPRINT( "%s:Wait for Bridge Reset\n", __FUNCTION__);
	//	UFWSleep(1000);
	//}
	//assert( hdl!=INVALID_HANDLE_VALUE);
	
	//if (wait_cnt>= max_wait_cnt) {
	//	_DEBUGPRINT( "can't find new handle");
	//	goto failend;
	//}
	_DEBUGPRINT( "%s:Bridge Power Off\n", __FUNCTION__);
	if (_static_JMS583_Bridge_Control(hdl,1, DEFAULT_TIME_OUT, status_buffer)){
		_DEBUGPRINT( "%s:Bridge Power Off Fail\n", __FUNCTION__);
		goto failend;
	}

	if (doI2c || isNewTester){
		_DEBUGPRINT( "%s:Bridge Power Off\n", __FUNCTION__);
		if (_static_JMS583_Bridge_Control(hdl,1, DEFAULT_TIME_OUT, status_buffer)) {
			_DEBUGPRINT( "%s:Bridge Power Off Fail\n", __FUNCTION__);
			goto failend;
		}
		_DEBUGPRINT( "%s:I2c_read cmd 0x0 from 0x48 and set to _i2c_buffer[0]\n", __FUNCTION__);
		if (_static_JMS583_Bridge_I2c_Read(hdl,0x48, 0, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Read fail\n", __FUNCTION__);
			goto failend;
		}
		_DEBUGPRINT( "%s:I2c_read buffer 0x%X\n", __FUNCTION__, i2c_buffer[0]);
		i2c_buffer[0] &= 0x18;

		_DEBUGPRINT( "%s:I2c_write cmd 1 to 0x48 with buf=0x%X\n", __FUNCTION__, i2c_buffer[0]);
		if (_static_JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] |= 0xC0;
		_DEBUGPRINT( "%s:I2c_write cmd 1 to 0x48 with buf=0x%X\n", __FUNCTION__, i2c_buffer[0]);
		if (_static_JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] &= 0x18;
		_DEBUGPRINT( "%s:I2c_write cmd 1 to 0x48 with buf=0x%X\n", __FUNCTION__, i2c_buffer[0]);
		if (_static_JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
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
		if (_static_JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] |= 0x03;
		_DEBUGPRINT( "%s:I2c_write cmd 1 to 0x48 with buf=0x%X\n", __FUNCTION__, i2c_buffer[0]);
		if (_static_JMS583_Bridge_I2c_Write(hdl,0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT( "%s:Bridge_I2c_Write fail\n", __FUNCTION__);
			goto failend;
		}
	}

	if (_static_JMS583_Bridge_Control(hdl,4, DEFAULT_TIME_OUT, status_buffer)) {
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
				_static_JMS583_Bridge_Control(hdl, 0x07, 15, status_buffer);
				if ('N' == status_buffer[0] && 'V' == status_buffer[1] && 'M' == status_buffer[2] && 'E' == status_buffer[3] && (status_buffer[4]&0x01))
				{
					_DEBUGPRINT("%s:ready for using.", __FUNCTION__);
					Sleep(500);
					break;
				}
			}
			else if (2 == bridge_support_level) {
				_static_JMS583_Bridge_Control(hdl, 0x0C, 15, status_buffer);
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
	_static_MyCloseHandle(&hdl);

	return physical;

failend:
	_static_MyCloseHandle(&hdl);
	return -1;
}

int UFWInterface::_static_JMS583_BridgOff(int physical, bool doI2c) {
	unsigned char inq_buffer[1024] = {'\0'};
	unsigned char inq_buffer_cache[1024] = {'\0'};
	unsigned char i2c_buffer[512] = {0};
	bool isNewVersion = false;

	HANDLE hdl = _static_MyCreateHandle ( physical, true, false );
	assert(hdl != INVALID_HANDLE_VALUE);

	if (_static_Inquiry(hdl, inq_buffer, true))
	{
		_DEBUGPRINT( "%s:Inquiry fail\n", __FUNCTION__);
		goto failend;
	}

	memcpy(inq_buffer_cache, inq_buffer, sizeof(inq_buffer));
	isNewVersion = checkIfJMS583VersionIsNew(inq_buffer_cache);
	if (doI2c || isNewVersion){
		i2c_buffer[0] = 0x18;
		_DEBUGPRINT("%s:I2c_write cmd 0x01 and write value 0x18 to addr 0x48", __FUNCTION__);
		if (_static_JMS583_Bridge_I2c_Write(hdl, 0x48, 1, i2c_buffer, 1)){
			_DEBUGPRINT("%s:Bridge_I2c_Write fail", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] = 0x18; // Set GPIO3/GPIO4 Input, others Output
		_DEBUGPRINT("%s:I2c_write cmd 0x03 and write value 0x18 to addr 0x48", __FUNCTION__);
		if (_static_JMS583_Bridge_I2c_Write(hdl, 0x48, 3, i2c_buffer, 1)) {
			_DEBUGPRINT("%s:Bridge I2C Init Fail", __FUNCTION__);
			goto failend;
		}

		if (_static_JMS583_Bridge_I2c_Read(hdl, 0x48, 0, i2c_buffer, 1)) {
			_DEBUGPRINT("%s:Bridge_I2c_Read fail", __FUNCTION__);
			goto failend;
		}
	}

	if (_static_JMS583_Bridge_Control(hdl, 1, DEFAULT_TIME_OUT, NULL)) {
		_DEBUGPRINT("%s:Bridge Power Off", __FUNCTION__);
		goto failend;
	}


	if (doI2c || isNewVersion){
		i2c_buffer[0] &= 0x18;
		_DEBUGPRINT("%s:I2c_write cmd 1 to 0x48 with buf=0x%X", __FUNCTION__, i2c_buffer[0]);
		if (_static_JMS583_Bridge_I2c_Read(hdl, 0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT("%s:Bridge_I2c_Write fail", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] |= 0xC0;
		_DEBUGPRINT("%s:I2c_write cmd 1 to 0x48 with buf=0x%X", __FUNCTION__, i2c_buffer[0]);
		if (_static_JMS583_Bridge_I2c_Read(hdl, 0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT("%s:Bridge_I2c_Write fail", __FUNCTION__);
			goto failend;
		}
		i2c_buffer[0] &= 0x18;
		_DEBUGPRINT("%s:I2c_write cmd 1 to 0x48 with buf=0x%X", __FUNCTION__, i2c_buffer[0]);
		if (_static_JMS583_Bridge_I2c_Read(hdl, 0x48, 1, i2c_buffer, 1)) {
			_DEBUGPRINT("%s:Bridge_I2c_Write fail", __FUNCTION__);
			goto failend;
		}
	}
	return 0;

failend:
	_static_MyCloseHandle(&hdl);
	return -1;
}

int UFWInterface::_static_JMS583_Bridge_I2c_Write(HANDLE hUsb,unsigned char i2c_address, unsigned char cmd, unsigned char * buffer, int length)
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
	rtn = _static_MyIssueCmdHandle( hUsb, cdb , false, buffer , length, 30, IF_USB );
	if (rtn) {
		_DEBUGPRINT( "Bridge_I2c_Write fail but don't care!\n");
	}
	return 0;

}
int UFWInterface::_static_JMS583_Bridge_I2c_Read(HANDLE hUsb,unsigned char i2c_address, unsigned char cmd, unsigned char * buffer, int length)
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

	rtn = _static_MyIssueCmdHandle ( hUsb, cdb , true, buffer , length, 30, IF_USB );
	if (rtn) {
		_DEBUGPRINT( "Bridge_I2c_READ fail but don't care!\n");
	}
	return 0;
}
// control_id = 1: Power OFF
// control_id = 2: Power ON
// control_id = 4: Initial
// control_id = 5: Reset MCU
// control_id = 6: Normal shutdown
// control_id = 7: Get Initial Status
int UFWInterface::_static_JMS583_Bridge_Control(HANDLE hUsb,int control_id, int timeout, unsigned char * read_buffer)
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
	rtn = _static_MyIssueCmdHandle ( hUsb, cdb , false, data , length, timeout, IF_USB );
	if(control_id == 0x05) {
		return rtn;
	}

	if(control_id == 0x07 || control_id == 0x0C)
		cdb[1] = 0x02;				//	NVM Command Get Resp
	else
		cdb[1] = 0x0F;				//	NVM Command Get Resp

	return _static_MyIssueCmdHandle ( hUsb, cdb , true, read_buffer , 512, timeout, IF_USB );

}

int UFWInterface::_static_C2012_PowerOnOff(int  physical, unsigned int Mode,unsigned int Delay,unsigned int Voltage_SEL,unsigned int DDR_PWR_Flag,bool m_unit_100ms)
{

	unsigned char inq_buffer[1024] = {'\0'};
	unsigned char inq_buffer_cache[1024] = {'\0'};


	HANDLE hdl = _static_MyCreateHandle ( physical, true, false );
	assert(hdl != INVALID_HANDLE_VALUE);

	if (_static_Inquiry(hdl, inq_buffer, true))
	{
		_DEBUGPRINT( "%s:Inquiry fail\n", __FUNCTION__);
		return 1;
	}
	int cmd_result=1;
	unsigned char CDB[16];
	memset(CDB, 0, 16);

	CDB[0]=0x06;
	CDB[1]=0xF0;
	CDB[2]=0x84;
	CDB[3]=Mode;				// 0: OFF (0.9v~1.2v),  1: ON
	CDB[4]=Delay;				// 00~ff (unit:20ms), on or off �� delay ���ɶ�
	CDB[5]=Voltage_SEL;			// 0: Auto Detect, 1: 5V(P-MOS1), 2:3V3(P-MOS2)			
	CDB[6]=DDR_PWR_Flag;		// 0: DDR Power off,  1: DDR Power on
	CDB[7]=0;
	CDB[8]=0;					// 10ms base
	if(m_unit_100ms)
		CDB[8]=0x01;

	return _static_MyIssueCmdHandle ( hdl, CDB , false, NULL , 0, 30, IF_USB );

}

int UFWInterface::_static_JMS583_Bridge_WriteParameterPage(HANDLE hUsb,unsigned char *w_buf)
{
	unsigned char cdb[16];
	memset(cdb , 0 , sizeof(cdb));
	cdb[0] = 0xDF;
	cdb[1] = 0x01;
	cdb[3] = 0x02;
	cdb[8] = 0x01;
	cdb[9] = 0x10;
	cdb[11]= 0xFB;

	return _static_MyIssueCmdHandle ( hUsb, cdb , false, w_buf , 512, 5, IF_USB );
}

int UFWInterface::_static_JMS583_Bridge_ReadParameterPage(HANDLE hUsb,unsigned char *r_buf)
{
	unsigned char cdb[16];
	memset(cdb , 0 , sizeof(cdb));
	cdb[0] = 0xDF;
	cdb[1] = 0x10;
	cdb[3] = 0x02;
	cdb[8] = 0x01;
	cdb[9] = 0x10;
	cdb[11]= 0xFA;

	return _static_MyIssueCmdHandle ( hUsb, cdb , true, r_buf , 512, 5, IF_USB );
}

int UFWInterface::_static_Bridge_ReadFWVersion(HANDLE hUsb, unsigned char * read_buffer)
{
	unsigned char cdb[16]={0};
	int fw_length = 16;
	cdb[0] = 0xE0;
	cdb[1] = 0xF4;
	cdb[2] = 0xE7;

	return _static_MyIssueCmdHandle(hUsb, cdb, true, read_buffer, fw_length, 15, IF_USB);
}

