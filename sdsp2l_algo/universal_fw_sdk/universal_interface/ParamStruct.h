#include <stdio.h>


#pragma pack(1)

struct General_Info
{
	char TestStation[10];
	char SlotNumber[30];
	char WorkingNo[10];;
	char ModelName[30];
	char SerialNum[30];
	char MPVersion[20];
};


struct Flash_Info
{
	char Flh_Type[10];
	char Flh_MakeCode[20];
	char Flh_Capacity[20];
	char Flh_Number[10];
	char Flh_ID[20];
};

struct Device_Info
{
	char ControllerType[20];
	char ICFWVersion[20];
	char ISP_CodeVersion[20];
	char ISP_MPBINCode[250];
	char ConnectorType[20];
	char SSD_Size[20];
	char VID[20];
	char SSVID[20];
	char DID[20];
	char SSID[20];	
	char CE_Bitmap[20];
	char PMIC[20];
};

struct Options
{
	char Skip_Check_ChipIDZero[10];
	char AutoIncreseSerialNum[10];
	char CheckBurninMarkFirst[10];
	char Do_BurnIn_PreTest[10];
	char Check_RDT_Table[10];
	char SaveLogOnServer[10];
	char OUI[10];
	char Force_PCIe_Flow[10];
	char Check_NAND_Type[10];
	char Put_PowerIC[10];	
	char Put_Thermal_Sensor[10];
	char TS[10];
	char Enable_Hmb[10];
};


struct param_Ini
{
	General_Info General_param;
	Flash_Info Flash_param;
	Device_Info Device_param;	
	Options Options_param;
};


#pragma pack()


