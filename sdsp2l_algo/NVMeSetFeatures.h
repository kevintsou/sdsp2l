#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <windows.h>

int iNVMeSetFeatures(HANDLE _hDevice);
int siNVMeSetFeatures(HANDLE _hDevice, DWORD _cdw10, DWORD _cdw11);
#ifdef __cplusplus
}
#endif