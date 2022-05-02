
#include "NetSCSICmd.h"

extern unsigned char _forceFlashID[8];
NetSCSICmd::NetSCSICmd( DeviveInfo* info, const char* logFolder )
:UFWSCSICmd(info,logFolder)
{
    Init();
}

NetSCSICmd::NetSCSICmd( DeviveInfo* info, FILE *logPtr )
:UFWSCSICmd(info,logPtr)
{
    Init();
}
void NetSCSICmd::Init()
{
    _useNet = 0;
}

NetSCSICmd::~NetSCSICmd()
{
}

void NetSCSICmd::NetEnabled()
{
    assert(_useNet==1);
    _useNet = 1;
}

void NetSCSICmd::NetDisabled()
{
    assert(_useNet==0);
    _useNet = 0;
}


int NetSCSICmd::MyIssueCmd(  unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout,int useBridge )
{
    if ( _useNet ) {
        //socket 
        return _static_MyIssueCmdHandle( _hUsb, cdb, read, buf, bufLength, timeout, _deviceType );
    }
    else {
        return _static_MyIssueCmdHandle( _hUsb, cdb, read, buf, bufLength, timeout, _deviceType );
    }
}

