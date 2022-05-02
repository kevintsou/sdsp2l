

#include "UFWSCSICmd.h"
//#include <QtCore>
#include <ntddscsi.h>
#include <winioctl.h>
#include <stdio.h>
#include <vector>
#include "spti.h"
#include "nvme.h"
#include "nvmeIoctl.h"
#include "nvmeReg.h"



const int DEVICE_SATA_3110 = 0x3110;
const int DEVICE_SATA_3111 = 0x3111;
const int DEVICE_PCIE_5007 = 0x5007;
const int DEVICE_PCIE_5008 = 0x5008;
const int DEVICE_PCIE_5012 = 0x5012;
const int DEVICE_PCIE_5013 = 0x5013;
const int DEVICE_PCIE_5016 = 0x5016;
const int DEVICE_PCIE_5017 = 0x5017;
const int DEVICE_PCIE_5019 = 0x5019;
const int DEVICE_PCIE_5020 = 0x5020;

int _SCSIPtBuilder1 ( SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER* sptdwb ,unsigned int cdbLength , unsigned char* CDB ,
					 unsigned char direction , unsigned int transferLength ,unsigned long timeOut, int deviceType );

char _GetDriveLetter( int drivenum, int &deviceType )
{
    HANDLE hDeviceHandle = NULL;
    char drive[] = {'\\', '\\', '.', '\\', 'A', ':', 0};
    unsigned int driveMask = GetLogicalDrives();
    for(int i = 0; i < 26; i++)
    {
        bool b = ((driveMask>>i) & 1);
        if( b )
        {
            drive[4] = 'A' + i;
            //printf("Drive: %s\n", drive);
            hDeviceHandle = CreateFileA(drive , 0, 0, NULL,
                                        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
            if (hDeviceHandle != INVALID_HANDLE_VALUE )
            {
                STORAGE_DEVICE_NUMBER sdn;
                DWORD returned;
                if (!DeviceIoControl(
                            hDeviceHandle,IOCTL_STORAGE_GET_DEVICE_NUMBER,NULL,0,&sdn,sizeof(sdn),&returned,NULL))  {
                    CloseHandle(hDeviceHandle);
                    continue;
                }
                CloseHandle(hDeviceHandle);
                //printf("\t%C:Device type: %d number: %d/%d partition: %d\n",'A'+i,sdn.DeviceType,sdn.DeviceNumber, drivenum,sdn.PartitionNumber);
                //if( sdn.DeviceType==FILE_DEVICE_DISK && sdn.DeviceNumber==drivenum ) {
                if( sdn.DeviceType==FILE_DEVICE_DISK && sdn.DeviceNumber==drivenum ) {
                    deviceType = sdn.DeviceType;
                    return 'A'+i;
                }
            }
        }
    }
    return 0;
}


unsigned char _forceFlashID[8]={0};

void UFWSCSICmd::Init()
{
    _enabled = false;
	_sql = 0;

    //ssd datatype: 0=data, 1=vender, 2=ecc 3=time, 4=time+reset
    _eccBufferAddr  = 2;
	_dataBufferAddr = 0;
	_userSelectLDPC_mode = -1;
	_userSelectBlockSeed = 0x8299;
	_channelCE=1;
	_workingChannels=0;
}

UFWSCSICmd::UFWSCSICmd( DeviveInfo* info, const char* logFolder )
:UFWInterface(info, logFolder )
{
    Init();
	

}
UFWSCSICmd::UFWSCSICmd( DeviveInfo* info, FILE *logPtr )
:UFWInterface(info, logPtr )
{
    Init();
}

UFWSCSICmd::~UFWSCSICmd()
{
    if ( _sql ) {
        delete _sql;
    }
}

bool UFWSCSICmd::Enabled()
{
    return _enabled;
}

void UFWSCSICmd::UserSelectLDPC_mode(int ldpc)
{
	_userSelectLDPC_mode = ldpc;
}

void UFWSCSICmd::UserSelectBlockSeed(int block_seed)
{
	_userSelectBlockSeed= block_seed;
}

int UFWSCSICmd::GetSelectLDPC_mode()
{
	return _userSelectLDPC_mode;
}
int UFWSCSICmd::GetSelectBlockSeed()
{
	return _userSelectBlockSeed;
}

int UFWSCSICmd::_static_Inquiry ( int physical, unsigned char* r_buf, bool all, unsigned char length)
{
    // memset ( r_buf , 0 , 528 );
    // unsigned char cdb[16];
    // memset ( cdb , 0 , 16 );
    // cdb[0] = 0x12;
    // cdb[4] = all ? 0x44:0x24;

    
    HANDLE hdl = _static_MyCreateHandle(physical,true,false);  //true:physical
    if ( hdl== INVALID_HANDLE_VALUE )   {
        return 1;
    }
	int err = UFWInterface::_static_Inquiry (hdl, r_buf,all, length);
    _static_MyCloseHandle(&hdl);
    return err;
}

//
//int UFWSCSICmd::IdentifyATA( int physical, unsigned char * buf, int len, int bridge )
//{
//    HANDLE hdl = MyCreateHandle(physical,true,false);  //true:physical
//    if ( hdl== INVALID_HANDLE_VALUE )   {
//        return 1;
//    }
//    int err = 0;
//    err = UFWInterface::IdentifyATA( hdl, buf, len, bridge );
//    MyCloseHandle(&hdl);
//    return err;
//}
//
int UFWSCSICmd::_static_IdentifyNVMe( int physical, unsigned char * buf, int len, int bridge )
{
    assert(len==4096);
	HANDLE hdl = _static_MyCreateHandle ( physical, true, IF_NVNE==(bridge&INTERFACE_MASK) );
    
    if ( hdl== INVALID_HANDLE_VALUE )    {
        return 1;
    }

	unsigned char ncmd[16];
	memset(ncmd,0x00,16);
    int err = _static_MyIssueNVMeCmdHandle(hdl, ncmd,ADMIN_IDENTIFY,NVME_DIRCT_READ,buf,sizeof(ADMIN_IDENTIFY_CONTROLLER),15,true,bridge);
    if( err ) {
        _DEBUGPRINT( "%s command fail\n",__FUNCTION__ );
        _static_MyCloseHandle(&hdl);
        return 1;
    }
    _static_MyCloseHandle(&hdl);
    return err;
}


DeviveInfo* _DeviceList[128]={0};
DeviveInfo* UFWSCSICmd::GetDevice( int idx )
{
	return _DeviceList[idx];
}

void UFWSCSICmd::_static_FreeDevices()
{
	for ( int i=0;i<128;i++ ) {
		if ( _DeviceList[i] ) {
			delete _DeviceList[i];
			_DeviceList[i]=0;
		}
	}
}
#if 0
//unsigned char driveLetterMap['A'+1][2];
//device 0=usb, sata, 1=nvme; 
int UFWSCSICmd::ScanDevice(int useBridge, int startidx)
{

	std::vector<DeviveInfo*> list;
    char produceName[49];
    produceName[48]='\0';
    int nvmeidx = 0;

	if ( startidx ) {
		for ( int i=0;i<startidx;i++ ) {
			list.push_back(NULL);
		}
	}

    {
        unsigned char CDB[16],driveBuf[128];
        memset(CDB,0,sizeof(CDB));
        CDB[0] = 0x12;
        CDB[4] = 0x32;
        for ( int i=0; i<'A'; i++) {
            HANDLE hdl = MyCreateHandle(i,true,false);  //true:physical
            //_DEBUGPRINT("num: %d, hdl=%d\n",i,hdl);
            if ( hdl== INVALID_HANDLE_VALUE )   {
                continue;
            }
            if (_static_MyIssueCmdHandle(hdl,CDB,true,driveBuf,sizeof(driveBuf),10,useBridge) ) {
                MyCloseHandle(&hdl);
                continue;
            }
            MyCloseHandle(&hdl);

            memcpy(produceName,&driveBuf[8],48);
			//_DEBUGPRINT("produceName: %s\n",produceName);
            int deviceType;
            char drive = _GetDriveLetter( i, deviceType );
            if ( drive>'A' && drive<='C' ) continue;
            if ( strstr(produceName,"NVMe") ) continue;

			DeviveInfo* info = new DeviveInfo();
			info->physical = i;
			info->drive=drive;
			memcpy(info->label,&driveBuf[8],48);
			info->label[48]='\0';
			list.push_back(info);
            nvmeidx=i+1;
        }
    }

    {
        unsigned char ncmd[16];
        memset(ncmd,0x00,16);
        unsigned char buf[4096];
        //scan nvme
        for ( int scsiPort = 0; scsiPort < 16; scsiPort++) {
            HANDLE hdl = _static_MyCreateNVMeHandle ( scsiPort );
			//_DEBUGPRINT("num: scsi %d, hdl=%d\n",scsiPort,hdl);
            if( hdl == INVALID_HANDLE_VALUE) continue;

            int err = __static_MyIssueNVMeCmdHandle(hdl, ncmd,ADMIN_IDENTIFY,1,buf,sizeof(ADMIN_IDENTIFY_CONTROLLER),15,true,IF_NVNE);

            MyCloseHandle(&hdl);

            if ( err ) continue;

            //memcpy(produceName,&buf[24],40);
			DeviveInfo* info = new DeviveInfo();
			info->physical = scsiPort;
			info->drive=' ';
			info->nvme=true;
			memcpy(info->label,&buf[24],40);
			info->label[40]='\0';
			list.push_back(info);
            //printf("[%d][%C]:%s\n",nvmeidx,' ',produceName);
            nvmeidx++;
        }
    }

	FreeDevices();
	for ( unsigned int i=0;i<list.size();i++ ) {
		_DeviceList[i]=list[i];
		_DeviceList[i+1]=0;
	}

	return list.size();
	
}
#endif
int UFWSCSICmd::Dev_SQL_GroupWR_Setting(int blockSeed, int pageSeed, bool isWrite, int LdpcMode, char Spare)
{
	return 0;
}

int UFWSCSICmd::GetFlashID(unsigned char* r_buf)
{
    if ( _forceFlashID[0] ) {
        memcpy( r_buf,_forceFlashID,sizeof(_forceFlashID) );
    }
    else {
        return Dev_GetFlashID ( r_buf );
    }
    return 0;
}

int UFWSCSICmd::InitFlashInfoBuf ( const char *fn,const unsigned char* buf, long len, int frameK )
{
    MyFlashInfo* inst=0;
    if ( (buf && len>0 ) || fn!=0) {
        inst = MyFlashInfo::Instance(fn,buf,(long)len);
    }
    else {
        inst = MyFlashInfo::Instance();
    }

    unsigned char flashID[528];
	memset(flashID, 0 ,528);
    if ( GetFlashID ( flashID ) ) {
        Log (LOGDEBUG, "### Err: FlashGetID fail.\n" );
        return 1;
    }

    //memset(_forceFlashID,0,sizeof(_forceFlashID));
    if ( !inst->GetParameter ( flashID, &_FH, "flow.ini",0,0,true ) ) {
        Log (LOGDEBUG, "Flash not in DB \"%02X%02X%02X%02X%02X%02X%02X%02X\"",
             flashID[0],flashID[1],flashID[2],flashID[3],flashID[4],flashID[5],flashID[6],flashID[7] );
        return 1;
    }
	if (_FH.FlashCommandCycle == 0) {
		Log(LOGDEBUG, "error CSV");
		return 1;
	}
	_FH.bPBADie = 1;
    _FH.PageFrameCnt = _FH.PageSizeK/frameK;

	Log (LOGDEBUG, "Parameter:\n" );
    Log (LOGDEBUG, "    Flash ID=%s/%02X%02X%02X%02X%02X%02X%02X%02X\n",_FH.PartNumber,
         flashID[0],flashID[1],flashID[2],flashID[3],flashID[4],flashID[5],flashID[6],flashID[7] );
    Log (LOGDEBUG, "    FlhNumber=%u\n",_FH.FlhNumber );
    Log (LOGDEBUG, "    DieNumber=%u\n",_FH.DieNumber );
    Log (LOGDEBUG, "    Plane=%u\n",_FH.Plane );
    Log (LOGDEBUG, "    MaxBlock=%u\n",_FH.MaxBlock );
    Log (LOGDEBUG, "    Block_Per_Die=%u\n",_FH.Block_Per_Die );
    Log (LOGDEBUG, "    PagePerBlock=%u\n",_FH.PagePerBlock );
    Log (LOGDEBUG, "    PagePerBlockD3=%u\n",_FH.BigPage );
    Log (LOGDEBUG, "    PageSizeK=%u\n",_FH.PageSizeK );
    Log (LOGDEBUG, "    PageSize=%u\n",_FH.PageSize );
    Log (LOGDEBUG, "    PageFrameCnt=%u\n",_FH.PageFrameCnt );
    Log (LOGDEBUG, "    ED3=%u\n",_FH.beD3 );
    Log (LOGDEBUG, "    ECC=%u\n",_FH.ECC );
    Log (LOGDEBUG, "    Design=%02X\n",_FH.Design );
    Log (LOGDEBUG, "    bToggle=%02X\n",_FH.bToggle );

    return 0;
}



int UFWSCSICmd::Dev_SetFlashClock ( int timing )
{
    return 0;
}

int UFWSCSICmd::Dev_GetRdyTime ( int action, unsigned int* busytime )
{
    return 0;
}

int UFWSCSICmd::Dev_SetFlashDriving ( int driving )
{
    return 0;
}

int UFWSCSICmd::Dev_SetEccMode ( unsigned char Ecc )
{
	return 0;
}

int UFWSCSICmd::Dev_SetFWSpare ( unsigned char Spare )
{
	return 0;
}

int UFWSCSICmd::Dev_SetFWSpareTag ( unsigned char* Tag )
{
	return 0;
}

int UFWSCSICmd::Dev_SetFlashPageSize ( int pagesize )
{
    return 0;
}

int UFWSCSICmd::Dev_SetFWSeed ( int page,int block )
{
    return 0;
}

SQLCmd* UFWSCSICmd::GetSQLCmd()
{
    return _sql;
}

FH_Paramater* UFWSCSICmd::GetParam()
{
    return &_FH;
}
