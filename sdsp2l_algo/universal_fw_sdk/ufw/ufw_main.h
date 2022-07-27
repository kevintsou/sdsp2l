// �U�C ifdef �϶��O�إߥ����H��U�q DLL �ץX���зǤ覡�C
// �o�� DLL �����Ҧ��ɮ׳��O�ϥΩR�O�C���ҩw�q PHISON_TEST_PATTERN_EXPORTS �Ÿ��sĶ���C
// �b�R�O�C�w�q���Ÿ��C����M�׳������w�q�o�ӲŸ�
// �o�ˤ@�ӡA��l�{���ɤ��]�t�o�ɮת������L�M��
// �|�N PHISON_TEST_PATTERN_API �禡�����q DLL �פJ���A�ӳo�� DLL �h�|�N�o�ǲŸ�����
// �ץX���C



#if 1//def PHISON_TEST_PATTERN_EXPORTS
#define PHISON_TEST_PATTERN_API __declspec(dllexport)
#else
#define PHISON_TEST_PATTERN_API __declspec(dllimport)
#endif

extern "C" {

	PHISON_TEST_PATTERN_API int iGetPcieBridgeDrive();
	PHISON_TEST_PATTERN_API int InitPhyDrive(unsigned char PhyDrive);
	PHISON_TEST_PATTERN_API int InitialFlash(int PhyDrv);
	PHISON_TEST_PATTERN_API int GetFlashID(unsigned char* BufOut);
	PHISON_TEST_PATTERN_API int GetCVD(int m_CE, int m_Block, int m_PageStart, bool isSLC, int start_vth, int Vth_Step);
	PHISON_TEST_PATTERN_API int SendCmd(unsigned char* CmdBuf, int CmdLen, int mode, int CE, unsigned char * Buf, int BufLen);
	PHISON_TEST_PATTERN_API int SetFeature(BYTE FeatureMode);
	PHISON_TEST_PATTERN_API void EndDLL();
	PHISON_TEST_PATTERN_API void SetTempLogFile(char* LogFileName, int FileNameLen);

	PHISON_TEST_PATTERN_API int InitialBurner(int PhyDrv);
	PHISON_TEST_PATTERN_API int ReadPage(int CE, int Block, int Page, unsigned char* BufOut, bool isSLC);
	PHISON_TEST_PATTERN_API int ReadECC(int CE, int Block, int Page, unsigned char* BufOut);
	PHISON_TEST_PATTERN_API int WritePage(int CE, int Block, int Page, unsigned char* WriteBuf, bool isSLC);
	PHISON_TEST_PATTERN_API int EraseBlock(int CE, int Block, bool isSLC);
	PHISON_TEST_PATTERN_API int LoadBurner(unsigned char PhyDrive);
	PHISON_TEST_PATTERN_API void ReadWriteLDPCSetting(bool isWrite, int Page, int _userSelectLDPC_mode=-1);
	PHISON_TEST_PATTERN_API int PhySendCmd(int PhyDrv, unsigned char* CmdBuf, int CmdLen, int mode, int CE, unsigned char * Buf, int BufLen);
	PHISON_TEST_PATTERN_API int SetTiming(unsigned char PhyDrive, BYTE Timing);
	PHISON_TEST_PATTERN_API int SetAPParameter(unsigned char PhyDrive, unsigned char *APParam);
	PHISON_TEST_PATTERN_API int UpdateAPParameter(unsigned char PhyDrive, int idx, BYTE ParamData);
	PHISON_TEST_PATTERN_API int SetFlashID (unsigned char *FlashID);
	PHISON_TEST_PATTERN_API void SetDebugMessage(bool isEnable);

}

