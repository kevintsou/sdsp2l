#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <windows.h>

int iReadViaSCSIPassThrough(HANDLE _hDevice);
int iWriteViaSCSIPassThrough(HANDLE _hDevice);
int iFlushViaSCSIPassThrough(HANDLE _hDevice);
int iSecurityReceiveViaSCSIPassThrough(HANDLE _hDevice);


int iWriteViaSCSIPassThroughEx(HANDLE _hDevice, PVOID databuffer, ULONG SectorCnt, ULONG lba, ULONG timeoutVal);
int iReadViaSCSIPassThroughEx(HANDLE _hDevice, PVOID databuffer, ULONG SectorCnt, ULONG lba, ULONG timeoutVal);

int iWriteViaWriteFileEx(HANDLE _hDevice, PVOID databuffer, ULONG SectorCnt, ULONG lba);
int iReadViaReadFileEx(HANDLE _hDevice, PVOID databuffer, ULONG SectorCnt, ULONG lba);

#ifdef __cplusplus
}
#endif