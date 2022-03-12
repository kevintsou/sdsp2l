#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <windows.h>
#include <stdbool.h>



void vGetOSVersion(void);
bool bCanUseGetDeviceInternalLog(void);
void vPrintSystemError(unsigned long _ulErrorCode, const char* _strFunc);

int iIssueDeviceIoControl(HANDLE _hDevice,
    DWORD _dwControlCode,
    LPVOID _lpInBuffer,
    DWORD _nInBufferSize,
    LPVOID _lpOutBuffer,
    DWORD _nOutBufferSize,
    LPDWORD _lpBytesReturned,
    LPOVERLAPPED _lpOverlapped);

HANDLE hIssueCreateFile(const char* _strDeviceNo, TCHAR* ptchar);
HANDLE hUartIssueCreateFile(const char* _strDeviceNo);
HANDLE hIssueCreateFileEx(TCHAR* tcDevName);

#ifdef __cplusplus
}
#endif