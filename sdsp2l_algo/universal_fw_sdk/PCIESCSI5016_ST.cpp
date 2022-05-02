#include "PCIESCSI5016_ST.h"


#define E16_BURNER_TSB        "B5016_TSB.bin"
#define E16_BURNER_MICRON     "B5016_MICRON.bin"
#define E16_BURNER_YMTC       "B5016_YMTC.bin"
#define E16_BURNER_HYNIX      "B5016_HYNIX.bin"
#define E16_BURNER_DEFAULT    "B5016.bin"

#define E16_RDT_TSB           "B5016_FW_TSB.bin"
#define E16_RDT_MICRON        "B5016_FW_MICRON.bin"
#define E16_RDT_YMTC          "B5016_FW_YMTC.bin"
#define E16_RDT_HYNIX         "B5016_FW_HYNIX.bin"
#define E16_RDT_DEFAULT       "B5016_FW.bin"


int PCIESCSI5016_ST::GetBurnerAndRdtFwFileName(FH_Paramater * fh,  char * burnerFileName,  char * RdtFileName)
{

	if (fh) {
		if (fh->bIsYMTC3D) {
			sprintf(burnerFileName,"%s", E16_BURNER_YMTC);
			sprintf(RdtFileName,"%s"   , E16_RDT_YMTC);
		} else if (fh->bIsBiCs4QLC) {
			sprintf(burnerFileName,"%s", E16_BURNER_TSB);
			sprintf(RdtFileName,"%s"   , E16_RDT_TSB);
		} else if ((fh->bMaker == 0x98 ||fh->bMaker == 0x45 ) && (fh->bIsBics4 ||fh->bIsBics2 || fh->bIsBics3) && (fh->beD3)){
			sprintf(burnerFileName,"%s", E16_BURNER_TSB);
			sprintf(RdtFileName,"%s"   , E16_RDT_TSB);
		} else if ((fh->bMaker == 0x98) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf(burnerFileName,"%s", E16_BURNER_DEFAULT);
			sprintf(RdtFileName,"%s"   , E16_RDT_DEFAULT);
		} else if ((fh->bMaker == 0x45) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf(burnerFileName,"%s", E16_BURNER_DEFAULT);
			sprintf(RdtFileName,"%s"   , E16_RDT_DEFAULT);
		} else if (fh->bIsMicronB16 || fh->bIsMicronB17) {
			sprintf(burnerFileName,"%s", E16_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E16_RDT_MICRON);
		} else if (fh->bIsMIcronN18) {
			sprintf(burnerFileName,"%s", E16_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E16_RDT_MICRON);
		} else if (fh->bIsMIcronN28) {
			sprintf(burnerFileName,"%s", E16_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E16_RDT_MICRON);
		} else if (fh->bIsHynix3DV4 || fh->bIsHynix3DV5 ||fh->bIsHynix3DV6) {
			sprintf(burnerFileName,"%s", E16_BURNER_HYNIX);
			sprintf(RdtFileName,"%s"   , E16_RDT_HYNIX);
		} else if (fh->bIsMIcronB05) {
			sprintf(burnerFileName,"%s", E16_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E16_RDT_MICRON);
		} else if (fh->bIsMicronB27) {
			sprintf(burnerFileName,"%s", E16_BURNER_MICRON);
			sprintf(RdtFileName,"%s"   , E16_RDT_MICRON);
		} else {
			sprintf(burnerFileName,"%s", E16_BURNER_DEFAULT);
			sprintf(RdtFileName,"%s"   , E16_RDT_DEFAULT);
		}
	}
	return 0;
}

int PCIESCSI5016_ST::Dev_InitDrive( const char* ispfile )
{
	char burnerFileName[128] = {0};
	char FwFileName[128] = {0};
	int pass = 0;
	if (!ispfile)
	{
		GetBurnerAndRdtFwFileName(_fh, burnerFileName, FwFileName);
	}
	else
	{
		sprintf(burnerFileName, "%s", ispfile);
	}
	if (0 == ISPProgPRAM(burnerFileName))
		pass = 1;
	return pass;
}

PCIESCSI5016_ST::~PCIESCSI5016_ST(void)
{
}

PCIESCSI5016_ST::PCIESCSI5016_ST( DeviveInfo* info, const char* logFolder )
:PCIESCSI5012_ST(info,logFolder)
{
	//Init();
}

PCIESCSI5016_ST::PCIESCSI5016_ST( DeviveInfo* info, FILE *logPtr )
:PCIESCSI5012_ST(info,logPtr)
{
	//Init();
}