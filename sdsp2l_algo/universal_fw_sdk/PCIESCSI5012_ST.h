#pragma once
//#include "USBSCSICmd_ST.h"
#include "PCIESCSI5013_ST.h"
//#include "stdint.h"

	enum vuc_5012_opcode
{
	vuc_5012_opcode_no_data                 = 0xd0,
	vuc_5012_opcode_write                   = 0xd1,
	vuc_5012_opcode_read                    = 0xd2,
};

enum vuc_5012_feature
{
	vuc_5012_ap_key                         = 0x0,
	vuc_5012_isp_jump                       = 0x2,
	vuc_5012_cut_efuse                      = 0x3,
	vuc_5012_security_control               = 0x5,
	vuc_5012_erase_all_blocks               = 0x8,
	vuc_5012_direct_erase_flash             = 0x9,
	vuc_5012_direct_erase_pca               = 0xa,
	vuc_5012_clear_smart                    = 0xb,
	vuc_5012_set_virtual_temp               = 0xc,
	vuc_5012_preformat                      = 0x10,
	vuc_5012_isp_program_mem                = 0x12,
	vuc_5012_flash_feature_conf             = 0x16,
	vuc_5012_direct_write_sram              = 0x20,
	vuc_5012_direct_write_register          = 0x22,
	vuc_5012_isp_program_flash              = 0x30,
	vuc_5012_isp_program_ex_rom             = 0x31,
	vuc_5012_write_infoblock                = 0x32,
	vuc_5012_write_product_history          = 0x33,
	vuc_5012_direct_write_flash             = 0x40,
	vuc_5012_direct_write_pca               = 0x41,
	vuc_5012_cache_write_flash              = 0x42,
	vuc_5012_aute_preformat                 = 0x45,
	vuc_5012_ddr_scan_setup_hoid_time       = 0x48,
	vuc_5012_lba_protect_conf               = 0x49,
	vuc_5012_fake_power_lost                = 0x4a,
	vuc_5012_nand_veri_set                  = 0x50,
	vuc_5012_nand_veri_write                = 0x51,
	vuc_5012_nand_veri_trigger              = 0x52,
	vuc_5012_scan_flash_windows_setting     = 0x53,
	vuc_5012_power_cap_test                 = 0x54,
	vuc_5012_download_retry_table           = 0x55,
	vuc_5012_set_scan_flash_sdll_window_parm= 0x60,
	vuc_5012_set_ddr_find_window_parm       = 0x61,
	vuc_5012_simulation_for_linklost        = 0x62,
	vuc_5012_reset_smart_temp               = 0x63,
	vuc_5012_get_ismart                     = 0x65,
	vuc_5012_set_smart_value                = 0x66,
	vuc_5012_search_system_block            = 0x70,
	vuc_5012_read_system_info               = 0x80,
	vuc_5012_dump_system_table              = 0x81,
	vuc_5012_translate_v2p2v                = 0x82,
	vuc_5012_module_test                    = 0x83,
	vuc_5012_get_module_test_result         = 0x84,
	vuc_5012_fw_feature_control             = 0x85,
	vuc_5012_list_flash_block_info          = 0x88,
	vuc_5012_smart_get_info                 = 0x89,
	vuc_5012_write_l2p_table                = 0x8a,
	vuc_5012_read_rma_info                  = 0x8d,
	vuc_5012_get_flash_info                 = 0x90,
	vuc_5012_get_trim_table                 = 0x91,
	vuc_5012_ddr_conf                       = 0x94,
	vuc_5012_vendor_rs_frame_verify         = 0x97,
	vuc_5012_read_vendor_buffer             = 0x99,
	vuc_5012_direct_read_sram               = 0xa0,
	vuc_5012_direct_read_register           = 0xa2,
	vuc_5012_tcg_device_conf                = 0xa5,
	vuc_5012_set_psid		                = 0xa6,
	vuc_5012_read_infoblock                 = 0xb1,
	vuc_5012_read_product_history           = 0xb2,
	vuc_5012_direct_read_flash              = 0xc0,
	vuc_5012_direct_read_pca                = 0xc1,
	vuc_5012_cache_read_flash               = 0xc2,
	vuc_5012_send_handshake_request			= 0xc4,
	vuc_5012_receive_encryption_data		= 0xc5,
	vuc_5012_send_encryption_data			= 0xc6,
	vuc_5012_disable_vuc					= 0xc7,
	vuc_5012_uart_engineer_mode				= 0xc8,
	vuc_5012_nand_veri_get_conf             = 0xd0,
	vuc_5012_nand_veri_read                 = 0xd1,
	vuc_5012_read_flash_sdll_find_window    = 0xd2,
	vuc_5012_read_ddr_find_window           = 0xd3,
	vuc_5012_get_status                     = 0xe0,
	vuc_5012_get_vuc_data_size_info         = 0xe1,
	vuc_5012_read_sense_condition           = 0xf0,
	vuc_5012_get_rdt_log                    = 0xf1,
	vuc_5012_peripheral_detection           = 0xf2,
	vuc_5012_scan_rdt_log			        = 0xf3,
	vuc_5012_vuc_standard					= 0xff,
};
class PCIESCSI5012_ST :
	public PCIESCSI5013_ST
{
public:

	// init and create handle + create log file pointer from single app;
	PCIESCSI5012_ST( DeviveInfo*, const char* logFolder );
	// init and create handle + system log pointer
	PCIESCSI5012_ST( DeviveInfo*, FILE *logPtr );
	PCIESCSI5012_ST(void);
	~PCIESCSI5012_ST(void);

//B//////override from UFWSCSICmd////////////

	int Dev_CheckJumpBackISP ( bool needJump, bool bSilence );
	int Dev_GetSRAMData ( unsigned char offset, unsigned char* buf, int len );
	int Dev_SetSRAMData ( unsigned char offset, const unsigned char* buf, int len );
	int Dev_InitDrive(char * isp_file);
	void SetAlwasySync(BOOL sync);
	int ST_GetCodeVersionDate(BYTE obType,HANDLE DeviceHandle,BYTE targetDisk,unsigned char* buf);
	int NT_SwitchCmd(void);
//E//////override from UFWSCSICmd////////////

	virtual int GetBurnerAndRdtFwFileName(FH_Paramater * fh,  char * burnerFileName,  char * RdtFileName);
	bool InitDrive(unsigned char  drivenum, const char* ispfile, int swithFW );
	const char* GetCmdMsg();
	const char* GetCmdCDB();
	int GetDeviceECCFrame();
	int CheckJumpBootCode ( bool ,BOOL );
	int NT_GetFlashExtendID(BYTE bType ,HANDLE MyDeviceHandle, char targetDisk , unsigned char *buf, bool bGetUID,BYTE full_ce_info,BYTE get_ori_id);
	int Read_System_Info_5012(unsigned char * pBuf, int dataSize);
	int ISPProgPRAM(const char* bin_file_name);
	int GetFlashID ( unsigned char* r_buf, int ce8=0 );
	int ReadDDRByCE(int ce,UCHAR* r_buf,int Length,int type ); 
	int SetAPKey_NVMe(void);

	// commands for PCIE
private:
	int _burnerMode;
	UCHAR _dummyBuf[8*1024];
	int _timeOut;

	ULONG m_Boot2PRAMCodeSize;
	ULONG m_Boot2FlashCodeSize;
	ULONG m_PRAMCodeSize;
	ULONG m_FlashCodeSize;
	int _channels;
	int _channelCE;
	int _SQLMerge( const unsigned char* sql, int len );
	int _TrigerDDR(unsigned char drive, const unsigned char* sqlbuf, unsigned int transfer_length, unsigned char ce, const UCHAR *buffer);
	int _IssueIOCmd( const UCHAR* sql,int sqllen, int mode, int ce, UCHAR* buf, int len ); 
	int Isp_Jump_5012(unsigned char jump_type);
	int CheckJumpBootCode (void);
	int ISPPromgramRAMLoop(unsigned char type, unsigned char scrambled, unsigned char aes_key, unsigned char *buffer, unsigned int length);
	int ISPPromgramRAM(unsigned char type, unsigned char scrambled, unsigned char commit, unsigned char aes_key, unsigned int offset, unsigned int total_size, unsigned char *buffer, unsigned int length);
	bool Load_Burner(unsigned char * bin_buffer, int burner_size);
};

typedef PCIESCSI5012_ST MyPCIEScsiUtil_5012;