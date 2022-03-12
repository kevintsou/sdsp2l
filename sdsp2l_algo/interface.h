#pragma once
#include "NVMeIdentifyController.h"
#include "NVMeSMART.h"

#define D_SCAN_DEVICE_CNT   30
#define D_WRITE_TIMEOUT_VALUE 5

typedef struct {
    int		iAvalDevCnt;
    int*    iAvalDevIdx;
    TCHAR   pcDevPhyName[D_SCAN_DEVICE_CNT][256];
    HANDLE* pHandlePtr;
    PNVME_IDENTIFY_CONTROLLER_DATA14 pStController;
} T_AVAIL_DEV_MANAGEMENT;

// Interfnal funciton declaration
int iAllocateMemSpace();
int iGetAvailableNvmeDevIdx(int idx);


// Export funciton interface
// NVMe relative funciton
extern "C" __declspec(dllexport) int testdll(int x);
extern "C" __declspec(dllexport) int iScanAndGetAvailDeviceList(bool alloc);
extern "C" __declspec(dllexport) int iGetNVMeDevName(int idx, char* pDevName);
extern "C" __declspec(dllexport) int iGetNVMeDevSn(int idx, char* pSn);
extern "C" __declspec(dllexport) int iGetNVMeDevFwVer(int idx, char* pFwVer);
extern "C" __declspec(dllexport) int iGetNVMeDevRevision(int idx, char* pRevision);
extern "C" __declspec(dllexport) int iGetNVMeDevIdentifyData(int idx, PNVME_IDENTIFY_CONTROLLER_DATA14 ptr);
extern "C" __declspec(dllexport) int iGetNVMeSmartInfo(int idx, PNVME_SMART_INFO_LOG ptr);
extern "C" __declspec(dllexport) int iGetFeatureCmd(int devIdx, DWORD _dwFId, int _iType, DWORD _dwCDW11, uint32_t* _pulData);
extern "C" __declspec(dllexport) int iSetFeatureCmd(int devIdx, DWORD _dwFId, int _save, DWORD _dwCDW11);
extern "C" __declspec(dllexport) int iNVMeWrite(int idx, int lba, int sectorCount, PVOID databuffer);
extern "C" __declspec(dllexport) int iNVMeRead(int idx, int lba, int sectorCount, PVOID databuffer);
extern "C" __declspec(dllexport) bool bCheckDevExist(int idx);
extern "C" __declspec(dllexport) void vReleaseAllDevHandle();

// PL2303 relative function
extern "C" __declspec(dllexport) int iScanUartController(char* portName);
extern "C" __declspec(dllexport) void vEnableRelayUsedGpio();
extern "C" __declspec(dllexport) void vDisableRelayUsedGpio();
extern "C" __declspec(dllexport) int iGetGpioValue(int gpioIdx, BYTE* val);
extern "C" __declspec(dllexport) int iSetGpioValue(int gpioIdx, BYTE val);
extern "C" __declspec(dllexport) int iRescanDevice();
