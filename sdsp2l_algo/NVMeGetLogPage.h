#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <windows.h>

int iNVMeGetLogPage(HANDLE _hDevice, int iLId, int iNSID);

#ifdef __cplusplus
}
#endif