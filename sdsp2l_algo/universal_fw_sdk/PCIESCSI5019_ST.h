#pragma once
#include "PCIESCSI5013_ST.h"

class PCIESCSI5019_ST :
	public PCIESCSI5013_ST
{
public:
	void Init();
	PCIESCSI5019_ST( DeviveInfo* info, const char* logFolder );
	PCIESCSI5019_ST( DeviveInfo* info, FILE *logPtr );
	virtual int GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &tWHR2);
	virtual int GetBurnerAndRdtFwFileName(FH_Paramater * fh,  char * burnerFileName,  char * RdtFileName);
	virtual int OverrideOfflineInfoPage(USBSCSICmd_ST *cmd, unsigned char * info_buf);
	virtual void InitLdpc();
	~PCIESCSI5019_ST(void);
};
