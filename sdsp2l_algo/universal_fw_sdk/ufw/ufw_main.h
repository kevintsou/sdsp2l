// 下列 ifdef 區塊是建立巨集以協助從 DLL 匯出的標準方式。
// 這個 DLL 中的所有檔案都是使用命令列中所定義 PHISON_TEST_PATTERN_EXPORTS 符號編譯的。
// 在命令列定義的符號。任何專案都不應定義這個符號
// 這樣一來，原始程式檔中包含這檔案的任何其他專案
// 會將 PHISON_TEST_PATTERN_API 函式視為從 DLL 匯入的，而這個 DLL 則會將這些符號視為
// 匯出的。



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

