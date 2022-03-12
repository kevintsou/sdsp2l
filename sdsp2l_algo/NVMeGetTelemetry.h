#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <windows.h>
#include <stdbool.h>

int iNVMeGetTelemetryHostInitiated(HANDLE _hDevice, bool _bCreate);
int iNVMeGetTelemetryControllerInitiated(HANDLE _hDevice);

int iNVMeGetTelemetryHostInitiatedWithDeviceInternalLog(HANDLE _hDevice, bool _bCreate);
#ifdef __cplusplus
}
#endif