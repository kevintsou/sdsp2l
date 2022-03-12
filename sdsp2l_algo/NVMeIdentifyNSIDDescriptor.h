#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <windows.h>

int iNVMeIdentifyNSIDDescriptor(HANDLE _hDevice, DWORD _dwNSID);

#ifdef __cplusplus
}
#endif