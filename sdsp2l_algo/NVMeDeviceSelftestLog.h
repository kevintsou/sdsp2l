#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <windows.h>
#include <stdbool.h>

int iNVMeGetDeviceSelftestLog(HANDLE _hDevice, bool _bPrint, bool *_bInProgress);
#ifdef __cplusplus
}
#endif