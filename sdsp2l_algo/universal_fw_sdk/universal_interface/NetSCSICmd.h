#ifndef _NETSCSICMD_H_
#define _NETSCSICMD_H_

#include "UFWSCSICmd.h"

class NetSCSICmd : public UFWSCSICmd
{
public:
    // init and create handle + create log file pointer from single app;
    NetSCSICmd( DeviveInfo*, const char* logFolder );
    // init and create handle + system log pointer
    NetSCSICmd( DeviveInfo*, FILE *logPtr );
    ~NetSCSICmd();
    void NetEnabled();
    void NetDisabled();
	virtual int MyIssueCmd(  unsigned char* cdb , bool read, unsigned char* buf , unsigned int bufLength, int timeout=20,int useBridge=false );

protected:
    bool _useNet;
	
private:
    void Init();
};

#endif /*_NETSCSICMD_H_*/