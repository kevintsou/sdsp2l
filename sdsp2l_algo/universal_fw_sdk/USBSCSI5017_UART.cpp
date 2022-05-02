#include "USBSCSI5017_UART.h"
#define TRANSFER_SIZE 65536




int _GetGroupIDandMuxIDinGroup(int port_id, int &group_id, int &mux_id)
{
	group_id = port_id / MAX_UART_ST_PORT_NUMBER_PER_GROUP;
	mux_id = port_id % MAX_UART_ST_PORT_NUMBER_PER_GROUP;
	return 0;
}

USBSCSI5017_UART::~USBSCSI5017_UART()
{

}

USBSCSI5017_UART::USBSCSI5017_UART(DeviveInfo* info, const char* logFolder)
	:USBSCSI5017_ST(info, logFolder)
{
	Init();
}

USBSCSI5017_UART::USBSCSI5017_UART(DeviveInfo* info, FILE *logPtr)
	: USBSCSI5017_ST(info, logPtr)
{
	Init();
}
void USBSCSI5017_UART::Init(void)
{

}

void USBSCSI5017_UART::Log(LogLevel level, const char* fmt, ...)
{
	char logbuf[1024];
	va_list arglist;
	va_start(arglist, fmt);
	_vsnprintf(logbuf, sizeof(logbuf), fmt, arglist);
	va_end(arglist);
	nt_file_log("%s", logbuf);
}
long long USBSCSI5017_UART::LoadHostBurner(HANDLE h)
{
	return 0;
}
int USBSCSI5017_UART::LoadClientBurner(int MuxNo)
{
	Log(LOGDEBUG, "%s", __FUNCTION__);
	USB_SwitchMux(MuxNo);
	return 0;
}
int USBSCSI5017_UART::EmitIDpage(int MuxNo, unsigned char * buf)
{
	Log(LOGDEBUG, "%s", __FUNCTION__);
	USB_SwitchMux(MuxNo);
	return 0;
}
int USBSCSI5017_UART::EmitINFOpage(int MuxNo, unsigned char * buf)
{
	Log(LOGDEBUG, "%s", __FUNCTION__);
	USB_SwitchMux(MuxNo);
	return 0;
}
int USBSCSI5017_UART::GetSortingStatus(int MuxNo, unsigned char * buf)
{
	Log(LOGDEBUG, "%s", __FUNCTION__);
	USB_SwitchMux(MuxNo);
	return 0;
}
int USBSCSI5017_UART::SwitchMux(int port_id) //need to check if we can switch to muxN if we already in muxN?
{
	int group_id = 0;
	int mux_id = 0;
	_GetGroupIDandMuxIDinGroup(port_id, group_id, mux_id);
	Log(LOGDEBUG, "Switching Mux:group = %d, mux = %d", group_id, mux_id);
	return 0;
}

int USBSCSI5017_UART::IssueUARTCmd(int port_id, unsigned char * buf,UART_ST_STAGE cmd)
{
	int group_id = 0;
	int mux_id = 0;
	int rtn = 0;
	
	return 0;
}


#ifdef U17_UART_FAKE_DEVICE
int USBSCSI5017_UART::LoadClientBurner_F(int port_id)
{
	int group_id = 0;
	int mux_id = 0;
	_GetGroupIDandMuxIDinGroup(port_id, group_id, mux_id);	
	USB_SwitchMux(port_id);
	USB_SetAPKey();
	USB_SetBaudrateTX();
	USB_SetBaudrateRX();

	Log(LOGDEBUG, "%s: group_id=%d mux_id=%d", __FUNCTION__, group_id, mux_id);
	Sleep(100);
	return 0;
}
int USBSCSI5017_UART::EmitIDpage_F(int port_id, unsigned char * buf)
{
	int group_id = 0;
	int mux_id = 0;
	_GetGroupIDandMuxIDinGroup(port_id, group_id, mux_id);
	USB_SwitchMux(port_id);
	Log(LOGDEBUG, "%s: group_id=%d mux_id=%d", __FUNCTION__, group_id, mux_id);
	Sleep(100);
	return 0;
}
int USBSCSI5017_UART::EmitINFOpage_F(int port_id, unsigned char * buf)
{
	int group_id = 0;
	int mux_id = 0;
	_GetGroupIDandMuxIDinGroup(port_id, group_id, mux_id);
	USB_SwitchMux(port_id);
	Log(LOGDEBUG, "%s: group_id=%d mux_id=%d", __FUNCTION__, group_id, mux_id);
	Sleep(100);
	return 0;
}
int USBSCSI5017_UART::GetSortingStatus_F(int port_id, unsigned char * buf)
{
	int group_id = 0;
	int mux_id = 0;
	_GetGroupIDandMuxIDinGroup(port_id, group_id, mux_id);
	USB_SwitchMux(port_id);
	Log(LOGDEBUG, "%s: group_id=%d mux_id=%d", __FUNCTION__, group_id, mux_id);
	Sleep(100);
	return 0;
}
#endif

int USBSCSI5017_UART::USB_SetBaudrateTX()
{
	const unsigned int bufferSize = 512;
	unsigned char buf[bufferSize];
	memset(buf, 0x00, bufferSize);
	buf[0] = 0x1A;//921600

	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xD1;
	ncmd[40] = bufferSize / 4; //Byte40
	ncmd[48] = 0x22;  //Byte48 //Feature
	ncmd[49] = 0x01;  //Byte49 
	ncmd[50] = 0x01;  //B50    1st SetBaudrate
	ncmd[52] = 0x00;  //B52    //SC
	ncmd[53] = 0xF0;  //B53    //SN
	ncmd[54] = 0x00;  //B54    //CL
	ncmd[55] = 0xF8;  //B55    //CH

	int result = Nvme3PassVenderCmd(ncmd, buf, bufferSize, SCSI_IOCTL_DATA_OUT, 20);
	return result;
}

int USBSCSI5017_UART::USB_SetBaudrateRX()
{
	const unsigned int bufferSize = 512;
	unsigned char buf[bufferSize];
	memset(buf, 0x00, bufferSize);
	buf[0] = 0x1B;//921600
	buf[4] = 0x0D;//921600
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xD1;
	ncmd[40] = bufferSize / 4; //Byte40
	ncmd[48] = 0x22;  //Byte48 //Feature
	ncmd[49] = 0x08;  //Byte49 
	ncmd[50] = 0x02;  //B50    2nd SetBaudrate
	ncmd[52] = 0xAC;  //B52    SC
	ncmd[53] = 0xF0;  //B53    //SN
	ncmd[54] = 0x00;  //B54    //CL
	ncmd[55] = 0xF8;  //B55    //CH

	int result = Nvme3PassVenderCmd(ncmd, buf, bufferSize, SCSI_IOCTL_DATA_OUT, 20);
	return result;
}
int USBSCSI5017_UART::USB_SwitchMux(int MuxNo)
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xD0;

	ncmd[48] = 0x11;		    //Byte48 Feature				
	ncmd[49] = MuxNo + 0x40 ;   //Byte49 bit6:COW EN
								//		 bit5:3 MUX ROW NO
								//		 bit2:0 MUX COL NO
	ncmd[50] = 0x20;			//       bit5: ROW EN 

	int result = Nvme3PassVenderCmd(ncmd, NULL, 0, SCSI_IOCTL_DATA_UNSPECIFIED, 15);
	return result;
}

int USBSCSI5017_UART::USB_GetStatus(unsigned char* iobuf, int idpage)
{
	unsigned char ncmd[512] = { 0 };
	const unsigned int bufferSize = 512;
	ncmd[0] = 0xD2;
	ncmd[40] = bufferSize / 4;
	ncmd[48] = 0xEE;  //Byte48 //Feature
	if (idpage)
		ncmd[50] = 0x01;  //Byte50 //IDPAGE & INFO page 

	int result = Nvme3PassVenderCmd(ncmd, iobuf, 512, SCSI_IOCTL_DATA_IN, 15);
	return result;
}

int USBSCSI5017_UART::USB_GetChipID(unsigned char* iobuf)
{
	unsigned char ncmd[512] = { 0 };
	const unsigned int bufferSize = 512;
	ncmd[0] = 0xD2;
	ncmd[40] = bufferSize / 4;
	ncmd[48] = 0x13;  //Byte48 //Feature

	int result = Nvme3PassVenderCmd(ncmd, iobuf, bufferSize, SCSI_IOCTL_DATA_IN, 15);
	return result;
}

int USBSCSI5017_UART::USB_ResetHostBaudrate()
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xD0;
	ncmd[48] = 0x15;  //Byte48 //Feature

	int result = Nvme3PassVenderCmd(ncmd, NULL, 0, SCSI_IOCTL_DATA_IN, 15);
	return result;
}

int USBSCSI5017_UART::USB_Power_Reset(char Voltage, char PowerOnOff)
{
	unsigned char ncmd[512] = { 0 };
	ncmd[0] = 0xD0;

	ncmd[48] = 0x10;		//Byte48 Feature				
	ncmd[49] = Voltage;		//Byte49 bit0: 1:1V2 0:1V8

	ncmd[50] = PowerOnOff;	//Byte50 bit0: 1:On 0:Off

	int result = Nvme3PassVenderCmd(ncmd, NULL, 0, SCSI_IOCTL_DATA_UNSPECIFIED, 15);
	return result;
}

int USBSCSI5017_UART::USB_Client_ISPProgPRAM(const char* m_MPBINCode, int type)
{
	bool Scrambled = false;
	bool with_jump = true;
	ULONG file_size;
	unsigned char *m_srcbuf;
	unsigned int package_num, remainder, ubCount, loop;
	bool status, is_lasted;

	errno_t err;
	FILE * fp;
	err = fopen_s(&fp, m_MPBINCode, "rb");
	if (!fp)
	{
		Log(LOGDEBUG, "Can not open MP BIN File: %s successfully \n!", m_MPBINCode);
		return 1;
	}

	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);


	if (file_size == -1L)
	{
		Log(LOGDEBUG, "Failed ! Check FW BIN File size(%s) = 0 \n", m_MPBINCode);
		fclose(fp);
		return 1;
	}


	m_srcbuf = (unsigned char *)malloc(file_size);
	memset(m_srcbuf, 0x00, file_size);

	if (fread(m_srcbuf, 1, file_size, fp) <= 0)
	{
		Log(LOGDEBUG, "Reading MP BIN File Failed !!!\n");
		fclose(fp);
		free(m_srcbuf);
		return 1;
	}
	fclose(fp);

	ubCount = 128;  //Nvme Cmd. Max Data Length: 512K
	int ISPFW_SingleBin_NoHeader = true;
	if (ISPFW_SingleBin_NoHeader)
	{
		package_num = file_size / TRANSFER_SIZE;
		remainder = file_size % TRANSFER_SIZE;

		Log(LOGDEBUG, "ISP Program Burner:  (Bin No Header)\n");
		Log(LOGDEBUG, "Burner Code Size= %d bytes. loop=%d, remin.=%d\n", file_size, package_num, remainder);
	}
	else
	{

	}

	for (loop = 0; loop < package_num; loop++)
	{
		is_lasted = false;
		if (remainder == 0)
		{
			if (loop == (package_num - 1))
				is_lasted = true;
		}

		if (ISPFW_SingleBin_NoHeader)
		{
			status = USB_Client_ISPProgPRAM(m_srcbuf + (loop * TRANSFER_SIZE), TRANSFER_SIZE, Scrambled, is_lasted, loop, type) ? true : false, type;
		}
		else {
			//status=NVMe_ISPProgramPRAM(m_srcbuf+m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+loop*ubCount)*512,ubCount*512,Scrambled,is_lasted,loop)==0;
		}
		if (status != 0)
		{
			Log(LOGDEBUG, "ISP Program Burner Code Failed_loop:%d !!\n", loop);
			free(m_srcbuf);
			return 1;
		}
		Log(LOGDEBUG, "%d done\n", loop);
	}

	if (remainder)
	{
		if (ISPFW_SingleBin_NoHeader)
			status = USB_Client_ISPProgPRAM(m_srcbuf + (loop * TRANSFER_SIZE), remainder, Scrambled, true, package_num, type) ? true : false;
		else
		{
			//status=NVMe_ISPProgramPRAM(m_srcbuf+(m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+quotient*ubCount)*512),remainder*512,Scrambled,true,loop)==0;
		}


		if (status != 0)
		{
			Log(LOGDEBUG, "ISP Program Burner Code Failed_lasted !!\n");
			free(m_srcbuf);
			return 1;
		}
		Log(LOGDEBUG, "last part done\n");
	}
	Log(LOGDEBUG, "ISP Prog Burner Code OK.\n");
	free(m_srcbuf);

	Sleep(500);
	return 0;
}

int USBSCSI5017_UART::USB_IDINFO_Page_ISPProgPRAM(const char* m_MPBINCode, int type, int portinfo, USBSCSICmd_ST* inputCmd)
{
	bool Scrambled = false;
	bool with_jump = true;
	ULONG file_size, file_max_size = 8192;
	unsigned char *m_srcbuf, *m_srcbuf2;
	unsigned int package_num, remainder, ubCount, loop;
	bool status, is_lasted;
	unsigned char IDPBuf[4096] = { 0 };
	int total_size_len = 0;
	char file_name[128] = { 0 };
	unsigned char d1_retry_buffer[4096] = { 0 }, retry_buffer[4096] = { 0 };
	errno_t err;
	FILE * fp;
	err = fopen_s(&fp, m_MPBINCode, "rb");

	if (!fp)
	{
		Log(LOGDEBUG, "Can not open MP BIN File: %s successfully \n!", m_MPBINCode);
		return 1;
	}

	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);


	if (file_size == -1L)
	{
		Log(LOGDEBUG, "Failed ! Check FW BIN File size(%s) = 0 \n", m_MPBINCode);
		fclose(fp);
		return 1;
	}

	char flh_interface = GetPrivateProfileInt("E13", "FlashInterFace", 2, _ini_file_name);
	char clk = GetPrivateProfileInt("E13", "CLK", 5, _ini_file_name);
	int userSpecifyBlkCnt = GetPrivateProfileInt("E13", "UserSpecifyBlkNum", _fh->MaxBlock, _ini_file_name);

	PCIESCSI5013_ST::PrepareIDpage(IDPBuf, flh_interface, clk, userSpecifyBlkCnt);
	total_size_len = sizeof(IDPBuf) + file_max_size;
	m_srcbuf = (unsigned char *)malloc(total_size_len);
	memset(m_srcbuf, 0x00, total_size_len);
	IDPBuf[352] = portinfo;	//for device U17 mark mux no
	memcpy(m_srcbuf, IDPBuf, sizeof(IDPBuf));

	m_srcbuf2 = (unsigned char *)malloc(file_max_size);
	memset(m_srcbuf2, 0x00, file_max_size);

	if (fread(m_srcbuf2, 1, file_size, fp) <= 0)
	{
		Log(LOGDEBUG, "Reading MP BIN File Failed !!!\n");
		fclose(fp);
		free(m_srcbuf2);
		return 1;
	}
	fclose(fp);

	GetRetryTables(d1_retry_buffer, retry_buffer);
	PrepareInFormationPage(inputCmd, _fh, m_srcbuf2, (char *)"DEBUGTOOL123456789", d1_retry_buffer, retry_buffer);
	memcpy(&m_srcbuf[4096], m_srcbuf2, file_max_size);

	sprintf(file_name, "%s", "U17_ID_PAGE_DUMP.bin");//IDPAGE
	err = fopen_s(&fp, file_name, "wb");
	fwrite(m_srcbuf, 1, total_size_len, fp);
	fclose(fp);

	ubCount = 128;  //Nvme Cmd. Max Data Length: 512K
	int ISPFW_SingleBin_NoHeader = true;
	if (ISPFW_SingleBin_NoHeader)
	{
		package_num = total_size_len / TRANSFER_SIZE;
		remainder = total_size_len % TRANSFER_SIZE;
		Log(LOGDEBUG, "ISP Program Burner:  (Bin No Header)\n");
		Log(LOGDEBUG, "Burner Code Size= %d bytes. loop=%d, remin.=%d\n", total_size_len, package_num, remainder);
	}
	else
	{

	}

	for (loop = 0; loop < package_num; loop++)
	{
		is_lasted = false;
		if (remainder == 0)
		{
			if (loop == (package_num - 1))
				is_lasted = true;
		}

		if (ISPFW_SingleBin_NoHeader)
		{
			status = USB_Client_ISPProgPRAM(m_srcbuf + (loop * TRANSFER_SIZE), TRANSFER_SIZE, Scrambled, is_lasted, loop, type) ? true : false, type;
		}
		else {
			//status=NVMe_ISPProgramPRAM(m_srcbuf+m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+loop*ubCount)*512,ubCount*512,Scrambled,is_lasted,loop)==0;
		}
		if (status != 0)
		{
			Log(LOGDEBUG, "ISP Program Burner Code Failed_loop:%d !!\n", loop);
			free(m_srcbuf);
			return 1;
		}
		Log(LOGDEBUG, "%d done\n", loop);
	}

	if (remainder)
	{
		if (ISPFW_SingleBin_NoHeader)
			status = USB_Client_ISPProgPRAM(m_srcbuf + (loop * TRANSFER_SIZE), remainder, Scrambled, true, package_num, type) ? true : false;
		else
		{
			//status=NVMe_ISPProgramPRAM(m_srcbuf+(m_Boot2PRAMCodeSize+m_Boot2FlashCodeSize+(1+quotient*ubCount)*512),remainder*512,Scrambled,true,loop)==0;
		}


		if (status != 0)
		{
			Log(LOGDEBUG, "ISP Program Burner Code Failed_lasted !!\n");
			free(m_srcbuf);
			return 1;
		}
		Log(LOGDEBUG, "last part done\n");
	}
	Log(LOGDEBUG, "ISP Prog Burner Code OK.\n");
	free(m_srcbuf);
	return 0;
}

int USBSCSI5017_UART::USB_Client_ISPProgPRAM(unsigned char* buf, unsigned int BuferSize, bool mScrambled, bool mLast, unsigned int m_Segment, int type)
{
	unsigned char ncmd[512] = { 0 };

	ncmd[0] = 0xD1;  //Byte0
	unsigned int temp = BuferSize / 4;
	ncmd[0x28] = temp & 0xFF; //B40
	ncmd[0x29] = (temp >> 8) & 0xFF; //B41
	ncmd[0x2A] = (temp >> 16) & 0xFF;  //B42	 
	ncmd[0x2B] = (temp >> 24) & 0xFF;  //B43
	ncmd[0x30] = 0x12;  //B48
	ncmd[0x31] = mLast << 2;//B49
	ncmd[0x32] = type;		//B50
	ncmd[0x36] = m_Segment;//B54
#if FUNTIONRECORD
	Log(LOGDEBUG, "*****************");
	Log(LOGDEBUG, __func__);
	UFWLoger::LogMEM(ncmd, 512);
	Log(LOGDEBUG, "*****************");
#endif
	int result = Nvme3PassVenderCmd(ncmd, buf, BuferSize, false, 20);
	return result;
}