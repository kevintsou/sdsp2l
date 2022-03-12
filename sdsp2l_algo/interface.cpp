#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdbool.h>
#include <stdlib.h>

#include "interface.h"
#include "GPIO.h"
#include "WinFunc.h"
#include "NVMeSCSIPassThrough.h"
#include "NVMeDeallocate.h"
#include "NVMeIdentify.h"
#include "NVMeIdentifyController.h"
#include "NVMeGetLogPage.h"
#include "NVMeGetFeatures.h"
#include "NVMeSetFeatures.h"
#include "NVMeDeviceSelftest.h"
#include "NVMeFormatNVM.h"
#include "NVMeSMART.h"
#include "NVMeUtils.h"

T_AVAIL_DEV_MANAGEMENT              g_stAvailDevManager;
NVME_IDENTIFY_CONTROLLER_DATA14     g_stController;
NVME_SMART_INFO_LOG                 g_stSMARTLog;
HANDLE                              g_uartDevHandle;

BOOL IsWow64();

int testdll(int x) {
	return x + 1;
}

bool bCheckDevExist(int idx) {

    int iResult = iNVMeIdentifyController(g_stAvailDevManager.pHandlePtr[iGetAvailableNvmeDevIdx(idx)]);
    if (!iResult) {
        return true;
    }
    else {
        return false;
    }
}

int iScanAvailableNvmeDevice(int iScanCnt, HANDLE* phDevice) {
    int iResult = -1;
    //g_stAvailDevManager.iAvalDevIdx = (int*)malloc(iScanCnt * sizeof(int));
    g_stAvailDevManager.iAvalDevCnt = 0;
    if (g_stAvailDevManager.iAvalDevIdx != NULL) {
        ZeroMemory(g_stAvailDevManager.iAvalDevIdx, iScanCnt * sizeof(int));

        for (int i = 0; i < iScanCnt; i++) {
            iResult = iNVMeIdentifyController(phDevice[i]);
            if (!iResult)
            {
                fprintf(stderr, "[W] Getting controller identify data done, idx: 0x%x\n\n", i);
                memcpy(&g_stAvailDevManager.pStController[g_stAvailDevManager.iAvalDevCnt], &g_stController, sizeof(NVME_IDENTIFY_CONTROLLER_DATA14));
                g_stAvailDevManager.iAvalDevIdx[g_stAvailDevManager.iAvalDevCnt] = i;
                g_stAvailDevManager.iAvalDevCnt++;
            }
            else {
                fprintf(stderr, "[W] Close Handle, idx: 0x%x\n\n", i);
                CloseHandle(phDevice[i]); // close handle
                phDevice[i] = INVALID_HANDLE_VALUE;
            }
        }
    }

    return g_stAvailDevManager.iAvalDevCnt;
}

int iGetAvailableNvmeDevIdx(int idx) {
    if (idx < g_stAvailDevManager.iAvalDevCnt) {
        return g_stAvailDevManager.iAvalDevIdx[idx];
    }
    else {
        printf("[E] Index out of bound: 0x%x\n", idx);
        return -1;
    }
}

int iScanPhysicalDevice(int iScanCnt, HANDLE* phDevice) {
    int     iDeviceNum = 0;
    char    tmpChar[256];

    ZeroMemory(tmpChar, 256);
    ZeroMemory(g_stAvailDevManager.pcDevPhyName, 256 * D_SCAN_DEVICE_CNT);

    for (int i = 0; i < iScanCnt; i++) {
        tmpChar[0] = (char)(i + 48);
        phDevice[iDeviceNum] = hIssueCreateFile((const char*)tmpChar, g_stAvailDevManager.pcDevPhyName[iDeviceNum]);
        if (phDevice[iDeviceNum] != INVALID_HANDLE_VALUE) {
            printf("Device Handle Valid, Physical Drive Idx: 0x%x, Drive Num: 0x%x\n", i, iDeviceNum);
            //strncpy_s(g_stAvailDevManager.pcDevPhyName[iDeviceNum], 255, tmpChar,255);
            iDeviceNum++;
        }
    }
    return iDeviceNum;
}

int iScanUartController(char* portName) {
    char    tmpChar[4];

    ZeroMemory(tmpChar, 4);
    memcpy(tmpChar, portName, 4);
    g_uartDevHandle = hUartIssueCreateFile((const char*)portName);

    if (g_uartDevHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    else {
        return 0;
    }
}

int iScanAndGetAvailDeviceList(bool alloc) {
    int     iDeviceNum = 0;
    int     iResult = -1;

    if (alloc) {
        iResult = iAllocateMemSpace();
        if (iResult) {
            return -1;
        }
    }

    iDeviceNum = iScanPhysicalDevice(D_SCAN_DEVICE_CNT, g_stAvailDevManager.pHandlePtr);
    if (iDeviceNum == 0)
    {
        return -3;
    }

    iDeviceNum = iScanAvailableNvmeDevice(iDeviceNum, g_stAvailDevManager.pHandlePtr);

    return iDeviceNum;
}

int iAllocateMemSpace() {
    g_stAvailDevManager.iAvalDevCnt = 0;
    g_stAvailDevManager.iAvalDevIdx = (int*)malloc(D_SCAN_DEVICE_CNT * sizeof(int));
    g_stAvailDevManager.pHandlePtr = (HANDLE*)malloc(D_SCAN_DEVICE_CNT * sizeof(HANDLE));
    g_stAvailDevManager.pStController = (PNVME_IDENTIFY_CONTROLLER_DATA14)malloc(D_SCAN_DEVICE_CNT * sizeof(NVME_IDENTIFY_CONTROLLER_DATA14));

    if ((g_stAvailDevManager.iAvalDevIdx == NULL) ||
        (g_stAvailDevManager.pHandlePtr == NULL)  ||
        (g_stAvailDevManager.pStController == NULL)) {
        return -1;
    }
    else {
        return 0;
    }
}

int iGetNVMeDevName(int idx, char* pDevName) {
    
    ZeroMemory(pDevName, 41);
    strncpy_s(pDevName, 41, (const char*)(g_stAvailDevManager.pStController[idx].MN), 40);
    pDevName[40] = '\0';
    return 0;
}

int iGetNVMeDevSn(int idx, char* pSn) {
    ZeroMemory(pSn, 21);
    strncpy_s(pSn, 21, (const char*)(g_stAvailDevManager.pStController[idx].SN), 20);
    pSn[20] = '\0';
    return 0;
}

int iGetNVMeDevFwVer(int idx, char* pFwVer) {
    ZeroMemory(pFwVer, 9);
    strncpy_s(pFwVer, 9, (const char*)(g_stAvailDevManager.pStController[idx].FR), 8);
    pFwVer[8] = '\0';
    return 0;
}

int iGetNVMeDevRevision(int idx, char* pRevision) {
    ZeroMemory(pRevision, 16);
    strncpy_s(pRevision, 16, (const char*)(g_stAvailDevManager.pStController[idx].VER), 15);
    pRevision[15] = '\0';
    return 0;
}

int iGetNVMeDevIdentifyData(int idx, PNVME_IDENTIFY_CONTROLLER_DATA14 ptr) {
    ZeroMemory(ptr, sizeof(NVME_IDENTIFY_CONTROLLER_DATA14));
    memcpy(ptr, &g_stAvailDevManager.pStController[idx], sizeof(NVME_IDENTIFY_CONTROLLER_DATA14));
    return 0;
}

int iGetNVMeSmartInfo(int idx, PNVME_SMART_INFO_LOG ptr) {
    ZeroMemory(ptr, sizeof(NVME_SMART_INFO_LOG));
    iNVMeGetLogPage(g_stAvailDevManager.pHandlePtr[iGetAvailableNvmeDevIdx(idx)], NVME_LOG_PAGE_HEALTH_INFO, 1);
    memcpy(ptr, &g_stSMARTLog, sizeof(NVME_SMART_INFO_LOG));
    return 0;
}

int iGetFeatureCmd(int devIdx, DWORD _dwFId, int _iType, DWORD _dwCDW11, uint32_t* _pulData) {
    return iNVMeGetFeature32(g_stAvailDevManager.pHandlePtr[iGetAvailableNvmeDevIdx(devIdx)], _dwFId, _iType, _dwCDW11, _pulData);
}

int iSetFeatureCmd(int devIdx, DWORD _dwFId, int _save, DWORD _dwCDW11) {
    return siNVMeSetFeatures(g_stAvailDevManager.pHandlePtr[iGetAvailableNvmeDevIdx(devIdx)], (((_save & 0x1) << 31) | (_dwFId & 0xFF)),_dwCDW11);
}

int iGetGpioValue(int gpioIdx, BYTE* val) {
    if (g_uartDevHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    switch (gpioIdx)
    {
    case 0:
        PL2303_GP0_GetValue(g_uartDevHandle, val);
        break;
    case 1:
        PL2303_GP1_GetValue(g_uartDevHandle, val);
        break;
    case 2:
        PL2303_GP2_GetValue(g_uartDevHandle, val);
        break;
    case 3:
        PL2303_GP3_GetValue(g_uartDevHandle, val);
        break;
    default:
        break;
    }
    return 0;
}

int iSetGpioValue(int gpioIdx, BYTE val) {
    if (g_uartDevHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    switch (gpioIdx)
    {
    case 0:
        PL2303_GP0_SetValue(g_uartDevHandle, val);
        break;
    case 1:
        PL2303_GP1_SetValue(g_uartDevHandle, val);
        break;
    case 2:
        PL2303_GP2_SetValue(g_uartDevHandle, val);
        break;
    case 3:
        PL2303_GP3_SetValue(g_uartDevHandle, val);
        break;
    default:
        break;
    }
    return 0;
}

int iEnableGpio(int gpioIdx, BOOL enable) {
    if (g_uartDevHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    switch (gpioIdx)
    {
    case 0:
        PL2303_GP0_Enalbe(g_uartDevHandle, enable);
        break;
    case 1:
        PL2303_GP1_Enalbe(g_uartDevHandle, enable);
        break;
    case 2:
        PL2303_GP2_Enalbe(g_uartDevHandle, enable);
        break;
    case 3:
        PL2303_GP3_Enalbe(g_uartDevHandle, enable);
        break;
    default:
        break;
    }
    return 0;
}

void vEnableRelayUsedGpio() {
    iEnableGpio(0, true);
    iEnableGpio(1, true);
    iEnableGpio(2, true);
    iEnableGpio(3, true);
}

void vDisableRelayUsedGpio() {
    iEnableGpio(0, false);
    iEnableGpio(1, false);
    iEnableGpio(2, false);
    iEnableGpio(3, false);
}

unsigned char outBuffer[1024 * 512];
unsigned char inBuffer[1024 * 512];

int iNVMeWrite(int idx, int lba, int sectorCount, PVOID databuffer) {
    int iResult = 0;
    int iRealIdx = 0;

    ZeroMemory(outBuffer, 1024 * 512);
    memcpy(outBuffer, databuffer, 512 * 512);
    //outBuffer[0] = 'K';
    //outBuffer[1] = 'E';
    //outBuffer[2] = 'V';
    iRealIdx = iGetAvailableNvmeDevIdx(idx);
    iResult = iWriteViaWriteFileEx(g_stAvailDevManager.pHandlePtr[iRealIdx], outBuffer, sectorCount, lba);
    
    //iResult = iWriteViaSCSIPassThroughEx(g_stAvailDevManager.pHandlePtr[idx], databuffer, sectorCount, lba, D_WRITE_TIMEOUT_VALUE);
    return iResult;
}

int iNVMeRead(int idx, int lba, int sectorCount, PVOID databuffer) {
    int iResult = 0;
    int iRealIdx = 0;

    ZeroMemory(inBuffer, 1024 *512);
    iRealIdx = iGetAvailableNvmeDevIdx(idx);
    iResult = iReadViaReadFileEx(g_stAvailDevManager.pHandlePtr[iRealIdx], inBuffer, sectorCount, lba);
    memcpy(databuffer, inBuffer, 512 * 512);
    return iResult;
}


int iRescanDevice() {
    char clpVerb[] = "runas";
    WCHAR wsz1[64];
    swprintf(wsz1, 64, L"%S", clpVerb);
    LPCWSTR ptrlpVerb = wsz1;

    char clpFile[] = "cmd";
    WCHAR wsz2[64];
    swprintf(wsz2, 64, L"%S", clpFile);
    LPCWSTR ptrlpFile = wsz2;
    WCHAR wsz3[64];

    // /c 是執行完命令後關閉命令窗口。
    // / k 是執行完命令後不關閉命令窗口。
    if (IsWow64()) {
        char clpParameters[] = "/c devcon_64 rescan";
        swprintf(wsz3, 64, L"%S", clpParameters);
    }
    else {
        char clpParameters[] = "/c devcon rescan";
        swprintf(wsz3, 64, L"%S", clpParameters);
    }

    LPCWSTR ptrlpParameters = wsz3;

    SHELLEXECUTEINFO ShExecInfo = { 0 };
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = ptrlpVerb;      // the cmd to execute
    ShExecInfo.lpFile = ptrlpFile;      // the file would like to open
    ShExecInfo.lpParameters = ptrlpParameters;      // parameter 
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_HIDE;         // the way to show the running program
    ShExecInfo.hInstApp = NULL;
    ShellExecuteEx(&ShExecInfo);
    WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
    return 0;
}

void vReleaseAllDevHandle() {
    if (g_stAvailDevManager.pHandlePtr == NULL) {
        //g_stAvailDevManager.pHandlePtr = (HANDLE*)malloc(D_SCAN_DEVICE_CNT * sizeof(HANDLE));
        return;
    }
    for (int idx = 0; idx < g_stAvailDevManager.iAvalDevCnt; idx++) {
        if (g_stAvailDevManager.pHandlePtr[iGetAvailableNvmeDevIdx(idx)] != NULL) {
            CloseHandle(g_stAvailDevManager.pHandlePtr[iGetAvailableNvmeDevIdx(idx)]);
        }
    }
}

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;
BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;
    //IsWow64Process is not available on all supported versions
    //of Windows. Use GetModuleHandle to get a handle to the
    //DLL that contains the function and GetProcAddress to get
    //a pointer to the function if available.
    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
            //handle error
        }
    }
    return bIsWow64;
}