#pragma once
#include "PCIESCSI5012_ST.h"

class PCIESCSI5016_ST :
	public PCIESCSI5012_ST
{
public:
	PCIESCSI5016_ST( DeviveInfo* info, const char* logFolder );
	PCIESCSI5016_ST( DeviveInfo* info, FILE *logPtr );
	~PCIESCSI5016_ST(void);
/////////override//////////
virtual int GetBurnerAndRdtFwFileName(FH_Paramater * fh,  char * burnerFileName,  char * RdtFileName);
virtual int Dev_InitDrive( const char* ispfile );	

};
