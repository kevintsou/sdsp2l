#pragma once
#include "USBSCSI5017_ST.h"

#define   MAX_UART_ST_GROUP_NUMBER              (16)
#define   MAX_UART_ST_PORT_NUMBER_PER_GROUP     (56)

#define U17_UART_FAKE_DEVICE

enum UART_ST_STAGE
{
	UART_STAGE_LOAD_HOST_BURNER,
	UART_STAGE_LOAD_CLIENT_BURNER,
	UART_STAGE_EMIT_ID_PAGE,
	UART_STAGE_EMIT_INFO_PAGE,
	UART_STAGE_GET_ST_STATUS,
	UART_STAGE_READ_FLASH_ID,
};

class USBSCSI5017_UART :
	public USBSCSI5017_ST
{
public:

	void Init(void);
	~USBSCSI5017_UART();
	USBSCSI5017_UART(DeviveInfo* info, const char* logFolder);
	USBSCSI5017_UART(DeviveInfo* info, FILE *logPtr);

	/*return host chip id if success else get -1*/
	long long LoadHostBurner(HANDLE h);
	int LoadClientBurner(int port_id);

	int EmitIDpage(int MuxNo, unsigned char * buf);

	int EmitINFOpage(int MuxNo, unsigned char * buf);

	int GetSortingStatus(int MuxNo, unsigned char * buf);

	int IssueUARTCmd(int port_id, unsigned char * buf, UART_ST_STAGE cmd);

	int SwitchMux(int MuxNo);

	//int USB_SetBaudrate();
	int USB_SetBaudrateTX();
	int USB_SetBaudrateRX();
	int USB_ResetHostBaudrate();
	int USB_SwitchMux(int MuxNo);
	int USB_GetStatus(unsigned char* iobuf, int idpage);
	int USB_GetChipID(unsigned char* iobuf);
	int USB_Client_ISPProgPRAM(const char* m_MPBINCode, int type);
	int USB_Client_ISPProgPRAM(unsigned char* buf, unsigned int BuferSize, bool mScrambled, bool mLast, unsigned int m_Segment, int type);
	int USB_IDINFO_Page_ISPProgPRAM(const char* m_MPBINCode, int type, int portinfo, USBSCSICmd_ST  * inputCmd);
	int USB_Power_Reset(char Voltage, char PowerOnOff);
	virtual void Log(LogLevel level, const char* fmt, ...);
#ifdef U17_UART_FAKE_DEVICE
	int LoadClientBurner_F(int port_id);

	int EmitIDpage_F(int port_id, unsigned char * buf);

	int EmitINFOpage_F(int port_id, unsigned char * buf);

	int GetSortingStatus_F(int port_id, unsigned char * buf);
#endif
	char Thread_no;
	char s1V2;
};

