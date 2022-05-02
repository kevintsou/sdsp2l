#include "PCIESCSI5012_ST.h"
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>


#define E12_BURNER_TSB        "B5012_TSB.bin"
#define E12_BURNER_MICRON     "B5012_MICRON.bin"
#define E12_BURNER_YMTC       "B5012_YMTC.bin"
#define E12_BURNER_HYNIX      "B5012_HYNIX.bin"
#define E12_BURNER_DEFAULT    "B5012.bin"

#define E12_RDT_TSB           "B5012_FW_TSB.bin"
#define E12_RDT_MICRON        "B5012_FW_MICRON.bin"
#define E12_RDT_YMTC          "B5012_FW_YMTC.bin"
#define E12_RDT_HYNIX         "B5012_FW_HYNIX.bin"
#define E12_RDT_DEFAULT       "B5012_FW.bin"


int PCIESCSI5012_ST::GetBurnerAndRdtFwFileName(FH_Paramater * fh,  char * burnerFileName,  char * RdtFileName)
{

	if (fh) {
		if (fh->bIsYMTC3D) {
			sprintf_s(burnerFileName, 64,"%s", E12_BURNER_YMTC);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_YMTC);
		} else if (fh->bIsBiCs4QLC) {
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_TSB);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_TSB);
		} else if ((fh->bMaker == 0x98 ||fh->bMaker == 0x45 ) && (fh->bIsBics4 ||fh->bIsBics2 || fh->bIsBics3) && (fh->beD3)){
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_TSB);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_TSB);
		} else if ((fh->bMaker == 0x98) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_DEFAULT);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_DEFAULT);
		} else if ((fh->bMaker == 0x45) && (fh->Design == 0x15) && (!fh->beD3)){
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_DEFAULT);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_DEFAULT);
		} else if (fh->bIsMicronB16 || fh->bIsMicronB17) {
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_MICRON);
		} else if (fh->bIsMIcronN18) {
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_MICRON);
		} else if (fh->bIsMIcronN28) {
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_MICRON);
		} else if (fh->bIsHynix3DV4 || fh->bIsHynix3DV5 ||fh->bIsHynix3DV6) {
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_HYNIX);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_HYNIX);
		} else if (fh->bIsMIcronB05) {
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_MICRON);
		} else if (fh->bIsMicronB27) {
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_MICRON);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_MICRON);
		} else {
			sprintf_s(burnerFileName, 64, "%s", E12_BURNER_DEFAULT);
			sprintf_s(RdtFileName, 64, "%s"   , E12_RDT_DEFAULT);
		}
	}
	return 0;
}




PCIESCSI5012_ST::~PCIESCSI5012_ST(void)
{
}
//B//////override from UFWSCSICmd////////////

PCIESCSI5012_ST::PCIESCSI5012_ST( DeviveInfo* info, const char* logFolder )
:PCIESCSI5013_ST(info,logFolder)
{
	//Init();
}

PCIESCSI5012_ST::PCIESCSI5012_ST( DeviveInfo* info, FILE *logPtr )
:PCIESCSI5013_ST(info,logPtr)
{
	//Init();
}

int PCIESCSI5012_ST::NT_SwitchCmd(void)
{
	return 0;
}
void PCIESCSI5012_ST::SetAlwasySync(BOOL sync)
{

}

int PCIESCSI5012_ST::ST_GetCodeVersionDate(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char* buf)
{
   return 1;
}


int PCIESCSI5012_ST::Dev_CheckJumpBackISP ( bool needJump, bool bSilence )
{
	return 1;
}
int PCIESCSI5012_ST::Dev_GetSRAMData ( unsigned char offset, unsigned char* buf, int len )
{
	return 1;
}
int PCIESCSI5012_ST::Dev_SetSRAMData ( unsigned char offset, const unsigned char* buf, int len )
{
	return 1;
}
//E//////override from UFWSCSICmd////////////
int PCIESCSI5012_ST::SetAPKey_NVMe()  //Enable AP-KEY
{
	unsigned char ncmd[16];
	memset(ncmd,0x00,16);	

	ncmd[0] = 0x00;  //Byte48 //Feature
	//ncmd[1] = 0x00;  //B49
	//ncmd[2] = 0x00;  //B50
	//ncmd[3] = 0x00;  //B51
	ncmd[4] = 0x6F;  //B52    //SC
	ncmd[5] = 0xFE;  //B53    //SN
	ncmd[6] = 0xEF;  //B54    //CL
	ncmd[7] = 0xFA;  //B55    //CH
	//ncmd[8] = 0x00;
	//ncmd[9] = 0x00;
	//ncmd[10] = 0x00;
	//ncmd[11] = 0x00;
	int ret = MyIssueNVMeCmd(ncmd, vuc_5012_opcode_no_data, NVME_DIRCT_NON, NULL, 0, 15, 0);
	//Log ( LOGDEBUG, "%s ret=%d\n",__FUNCTION__,ret );
	return ret;
}
bool PCIESCSI5012_ST::InitDrive(unsigned char  drivenum, const char* ispfile, int swithFW ) {
	int rtn = Dev_InitDrive((char*)ispfile);
	return rtn?true:false;
}

int PCIESCSI5012_ST::Dev_InitDrive(char * isp_file)
{
	char burner_file_name[256] = {0};
	int isPass = 0;

	char burner_file_name_ruled[256] = {0};
	char offline_file_name_ruled[256] = {0};
	GetBurnerAndRdtFwFileName(_fh, burner_file_name_ruled, offline_file_name_ruled);
	
	if (isp_file)
		sprintf_s(burner_file_name, "%s", isp_file);
	else
		sprintf_s(burner_file_name, "%s", burner_file_name_ruled);

	if (!ISPProgPRAM(burner_file_name))
		isPass = 1;

	return isPass;
}


int PCIESCSI5012_ST::NT_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle, char targetDisk , unsigned char *buf, bool bGetUID,BYTE full_ce_info,BYTE get_ori_id)
{
	int rtn=0;

	int connector_type = (_deviceType&INTERFACE_MASK);
	if (connector_type==IF_JMS583)
		SetAPKey_NVMe();
	unsigned char ncmd[16];
	memset(ncmd,0x00,16);
	unsigned char buf2[512] = {0};

	ncmd[0] = 0x90;	//Byte48
	
	if ('B' != _fwVersion[2])
	{
		memcpy(buf,"NOT IN BURNER, ABORT",20);
		return 0;
	}
	rtn = MyIssueNVMeCmd(ncmd, vuc_5012_opcode_read, NVME_DIRCT_READ, buf, 512, 15, 0);
	for (int ce = 0;ce<32; ce++){
		memcpy(buf2+16*ce,buf+8*ce,8);
	}
	if (0 == buf2[0] && 0 == buf2[1] && 0 ==buf2[2]) {
		memcpy(&buf2[0], &buf2[16], 16);
		memset(&buf2[16], 0, 16);
	}
	memcpy(buf,buf2,512);

	return rtn;
}

int PCIESCSI5012_ST::Read_System_Info_5012(unsigned char * pBuf, int dataSize)
{
	int rtn=0;
	unsigned char ncmd[16] = {0};
	ncmd[0] = vuc_5012_read_system_info;	
	memset(_fwVersion, 0, sizeof(_fwVersion));

	int connector_type = (_deviceType&INTERFACE_MASK);
	if (connector_type==IF_JMS583){
		if (SetAPKey_NVMe())
			return 1;
	}
	rtn = MyIssueNVMeCmd(ncmd, vuc_5012_opcode_read, NVME_DIRCT_READ, pBuf, dataSize, 15, 0);
	if (!rtn) {
		_fwVersion[0] = pBuf[410];
		_fwVersion[1] = pBuf[411];
		_fwVersion[2] = pBuf[412];
		_fwVersion[3] = pBuf[413];
	}
	return rtn;
}


int PCIESCSI5012_ST::CheckJumpBootCode (void)
{
	unsigned char pBuf[4096] = {0};
	// only test 5-1 times.
	for ( int retry=0; retry<5; retry++ ) {
		if (retry == 0) {
			if (Read_System_Info_5012(pBuf, 4096) != 0) {
				return 1;
			}
		}
		Read_System_Info_5012( pBuf, 4096);
		nt_file_log("Reading PCIe Device System information...\n" );


		nt_file_log("FW Version %C%C%C%C\n", _fwVersion[0], _fwVersion[1], _fwVersion[2], _fwVersion[3]);
		if (_fwVersion[2]=='R' ) {
			//already in boot code
			return 0;
		}		
		if ( Isp_Jump_5012(0x05) ) {
			nt_file_log("Force to Boot fail\n");
			return 1;
		}
		::Sleep ( 3000 );
	}
	return 1;
}
int PCIESCSI5012_ST::Isp_Jump_5012(unsigned char jump_type)
{
	unsigned char ncmd[16] = {0};
	int connector_type = (_deviceType&INTERFACE_MASK);
	if (connector_type==IF_JMS583)
		SetAPKey_NVMe();
	ncmd[0] = vuc_5012_isp_jump;
	ncmd[1] = jump_type;
	//cmd.cdw15 = (Common_Library::Swap_Uint(Common_Library::Crc16_Ccitt((unsigned char *)&cmd, 64)) << 16);
	return MyIssueNVMeCmd(ncmd, vuc_5012_opcode_no_data, NVME_DIRCT_NON, NULL, 0, 15, 0);
}

int PCIESCSI5012_ST::ISPPromgramRAM(unsigned char type, unsigned char scrambled, unsigned char commit, unsigned char aes_key, unsigned int offset, unsigned int total_size, unsigned char *buffer, unsigned int length)
{
	unsigned char ncmd[16] = {0};

	int connector_type = (_deviceType&INTERFACE_MASK);
	if (connector_type==IF_JMS583)
		SetAPKey_NVMe();
	ncmd[0] = vuc_5012_isp_program_mem;                           //48
	ncmd[1] = type | (scrambled <<1) | (commit<<2) | (aes_key<<3);//49
	ncmd[4] = (unsigned char)(offset & 0xff);                           //52
	ncmd[5] = (unsigned char)((offset>>8) & 0xff);               //53
	ncmd[6] = (unsigned char)((offset>>16) & 0xff);              //54
	ncmd[7] = (unsigned char)((offset>>24) & 0xff);              //55
	ncmd[8] = (unsigned char)(total_size & 0xff);                //56
	ncmd[9] = (unsigned char)((total_size>>8) & 0xff);           //57
	ncmd[10] = (unsigned char)((total_size>>16) & 0xff);         //58
	ncmd[11] = (unsigned char)((total_size>>24) & 0xff);         //59
	return MyIssueNVMeCmd(ncmd, vuc_5012_opcode_write, NVME_DIRCT_WRITE, buffer, length, 15, 0);
}


int PCIESCSI5012_ST::ISPProgPRAM(const char* burner_file_name)
{
	int ret = 1;

	unsigned char pBuf[4096] = {0}; 
	ULONG filesize;
	UCHAR *m_srcbuf = NULL;
	FILE * fp;
	errno_t err;
	//err = fopen_s(&pbooks, "C:\\Users\\sunny\\Desktop\\testt.bat", "a+b");

	err = fopen_s(&fp,burner_file_name, "rb");

	if (err)
	{
		nt_file_log("Can not open MP BIN File: %s successfully \n!",burner_file_name);
		return 1;
	}
	
	fseek(fp, 0, SEEK_END); 
	filesize = ftell(fp); 
	fseek(fp, 0, SEEK_SET); 

	m_srcbuf = new unsigned char[filesize]; 
	memset(m_srcbuf,0x00,filesize);
	if (fread(m_srcbuf, 1, filesize, fp)<=0)  //Bin file have problem
	{
		//Err_str = "S01";
		nt_file_log("Reading MP BIN File Failed !!!\n");
		fclose(fp);
		ret = 1;
		goto END;	
	}
	fclose(fp);
	if (Load_Burner(m_srcbuf, filesize)) {
		ret = 1;
		goto END;
	}
	//unsigned char CardStateBuf[4096];
	//if(Vuc_Identify_5012(drive_number, interface_type_nvme_security_bridge, 1, 0, CardStateBuf, 4096) != 0){
	//identify controller fail
	//return true;
	//}
	nt_file_log("Reading PCIe Device System information...\n" );
	if (Read_System_Info_5012(pBuf, 4096) != 0) {
		nt_file_log("Read system info fail\n" );
		ret = 1;
		goto END;
	}

	nt_file_log("FW Version %C%C%C%C\n", _fwVersion[0], _fwVersion[1], _fwVersion[2], _fwVersion[3]);
	if ( _fwVersion[2]=='B' ) {
		ret = 0;
	}
	else {
		nt_file_log("Fail running on Burner Code Now !\n");
		ret =1;
	}
END:
	if (m_srcbuf)
		delete [] m_srcbuf;
	return ret;
}



bool PCIESCSI5012_ST::Load_Burner(unsigned char * bin_buffer, int burner_size){
	printf( "%s\n",__FUNCTION__ );

	//ISP Jump To Ex Rom Boot
	if(Isp_Jump_5012(0x5) != 0){
		return 1;
	}

	//wait isp jump ready
	Sleep(500);

	//Isp Program Burner
	if(ISPPromgramRAMLoop( 0, 0/*nes*/, 0, bin_buffer , burner_size) != 0){
		return 1;
	}

	//Isp Jump To Burner
	if(Isp_Jump_5012( 0x2) != 0){
		return 1;
	}

	//wait isp jump ready
	Sleep(1000);
	return 0;
}

int PCIESCSI5012_ST::ISPPromgramRAMLoop(unsigned char type, unsigned char scrambled, unsigned char aes_key, unsigned char *buffer, unsigned int length)
{
	int remainder = 0, transfer = 0, packet = 0, commit = 0;
	int result;

	remainder = length;
	while(remainder > 0){
		if(remainder <= 65536){ // 128 sectors
			transfer = remainder;
			commit = 1;
		}
		else{
			transfer = 65536;
			commit = 0;
		}

		result = ISPPromgramRAM(type, scrambled, commit, aes_key, 65536 * packet, length, buffer + (65536 * packet), transfer);
		if(result != 0){
			return result;
		}
		packet++;
		remainder -= transfer;
		nt_file_log("ISP RAM OK, remainder=%d", remainder);
	}
	return 0;
}