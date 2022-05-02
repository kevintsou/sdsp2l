#include "PCIESCSI5019_ST.h"
#include "convertion_libs/ldpcecc.h"

PCIESCSI5019_ST::PCIESCSI5019_ST( DeviveInfo* info, const char* logFolder )
:PCIESCSI5013_ST(info,logFolder)
{
	Init();
}

PCIESCSI5019_ST::PCIESCSI5019_ST( DeviveInfo* info, FILE *logPtr )
:PCIESCSI5013_ST(info,logPtr)
{
	Init();
}
void PCIESCSI5019_ST::Init(void)
{

}

PCIESCSI5019_ST::~PCIESCSI5019_ST(void)
{
}

#define E19_BURNER_TSB        "B5019.bin"
#define E19_BURNER_MICRON     "B5019_MICRON.bin"
#define E19_BURNER_YMTC       "B5019_YMTC.bin"
#define E19_BURNER_HYNIX      "B5019_HYNIX.bin"
#define E19_BURNER_DEFAULT    "B5019.bin"

#define E19_RDT_TSB           "B5019_FW_TSB.bin"
#define E19_RDT_MICRON        "B5019_FW_MICRON.bin"
#define E19_RDT_YMTC          "B5019_FW_YMTC.bin"
#define E19_RDT_HYNIX         "B5019_FW_HYNIX.bin"
#define E19_RDT_DEFAULT       "B5019_FW.bin"


int PCIESCSI5019_ST::GetBurnerAndRdtFwFileName(FH_Paramater * fh,  char * burnerFileName,  char * RdtFileName)
{

	if (fh) {
		if (fh->bIsYMTC3D) {
			sprintf_s(burnerFileName,64,"%s", E19_BURNER_YMTC);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_YMTC);
		} else if (fh->bIsBiCs4QLC) {
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_TSB);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_TSB);
		} else if ((fh->bMaker == 0x98 ||fh->bMaker == 0x45 ) && (fh->bIsBics4 ||fh->bIsBics2 || fh->bIsBics3) && (fh->beD3)){
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_TSB);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_TSB);
		} else if ((fh->bMaker == 0x98) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_DEFAULT);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_DEFAULT);
		} else if ((fh->bMaker == 0x45) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_DEFAULT);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_DEFAULT);
		} else if (fh->bIsMicronB16 || fh->bIsMicronB17) {
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_MICRON);
		} else if (fh->bIsMIcronN18) {
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_MICRON);
		} else if (fh->bIsMIcronN28) {
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_MICRON);
		} else if (fh->bIsHynix3DV4 || fh->bIsHynix3DV5 ||fh->bIsHynix3DV6) {
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_HYNIX);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_HYNIX);
		} else if (fh->bIsMIcronB05) {
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_MICRON);
		} else if (fh->bIsMicronB27 || fh->bIsMicronB37R || fh->bIsMicronB36R || fh->bIsMicronB47R || fh->bIsMicronQ1428A) {
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_MICRON);
		} else {
			sprintf_s(burnerFileName, 64, "%s", E19_BURNER_DEFAULT);
			sprintf_s(RdtFileName, 64, "%s"   , E19_RDT_DEFAULT);
		}
	}
	return 0;
}

int PCIESCSI5019_ST::OverrideOfflineInfoPage(USBSCSICmd_ST * cmd, unsigned char * info_buf)
{
	int info_idx=0;
	enum {FLICKER, BRIGHT, BREATH, OFF};
	do
	{
		if (TASK_ID_LED_MODE == info_buf[info_idx])
		{
			if (  (FLICKER == info_buf[info_idx+1])  || (BREATH == info_buf[info_idx+1])  ) {
				info_buf[info_idx+1] = BRIGHT;
			}
			info_idx += 2; //1 tag 1 parameters
			continue;
		}
		info_idx++;
	} while (info_idx < 4096);
	PCIESCSI5013_ST::OverrideOfflineInfoPage(cmd, info_buf);
	return 0;
}

void PCIESCSI5019_ST::InitLdpc()
{
	if (_ldpc)
	{
		//assert(0);
	}
	else
	{
		_ldpc = new LdpcEcc(0x5019);
	}
}

int PCIESCSI5019_ST::GetACTimingTable(int flh_clk_mhz, int &tADL, int &tWHR, int &tWHR2)
{
	if ( 200 == flh_clk_mhz )
	{
		tADL = 51;  tWHR = 14;  tWHR2 = 50;
	}
	else if (266 == flh_clk_mhz)
	{
		tADL = 11;  tWHR = 3;  tWHR2 = 30;	
	}
	else if (333 == flh_clk_mhz)
	{
		tADL = 15;  tWHR = 4;  tWHR2 = 40;
	}
	else if (400 == flh_clk_mhz)
	{
		tADL = 51;  tWHR = 14;  tWHR2 = 50;
	}
	else if (533 == flh_clk_mhz)
	{
		tADL = 81;  tWHR = 27;  tWHR2 = 81;
	}
	else if (600 == flh_clk_mhz)
	{
		tADL = 81;  tWHR = 27;  tWHR2 = 81;
	}
	else
	{
		tADL = 51;  tWHR = 14;  tWHR2 = 50;
	}
	return 0;
}

