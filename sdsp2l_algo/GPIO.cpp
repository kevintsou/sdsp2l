#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdbool.h>
#include "GPIO.h"
#include "ioctl.h"
#include "WinFunc.h"

// For GP0
BOOL PL2303_GP0_Enalbe(HANDLE hDrv, BOOL enable)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP0_OUTPUT_ENABLE,
			&enable, sizeof(BYTE), NULL, 0, &nBytes, NULL);
	
	return bSuccess;
}

BOOL PL2303_GP0_GetValue(HANDLE hDrv, BYTE *val)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP0_GET_VALUE,
			NULL, 0, val, sizeof(BYTE), &nBytes, NULL);
	return bSuccess;
}

BOOL PL2303_GP0_SetValue(HANDLE hDrv, BYTE val)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP0_SET_VALUE,
			&val, sizeof(BYTE), NULL, 0, &nBytes, NULL);

	return bSuccess;
}

// For GP1
BOOL PL2303_GP1_Enalbe(HANDLE hDrv, BOOL enable)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP1_OUTPUT_ENABLE,
			&enable, sizeof(BYTE), NULL, 0, &nBytes, NULL);
	
	return bSuccess;
}

BOOL PL2303_GP1_GetValue(HANDLE hDrv, BYTE *val)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP1_GET_VALUE,
			NULL, 0, val, sizeof(BYTE), &nBytes, NULL);

	return bSuccess;
}

BOOL PL2303_GP1_SetValue(HANDLE hDrv, BYTE val)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP1_SET_VALUE,
			&val, sizeof(BYTE), NULL, 0, &nBytes, NULL);

	return bSuccess;
}


// For GP2
BOOL PL2303_GP2_Enalbe(HANDLE hDrv, BOOL enable)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP2_OUTPUT_ENABLE,
			&enable, sizeof(BYTE), NULL, 0, &nBytes, NULL);
	
	return bSuccess;
}
BOOL PL2303_GP2_GetValue(HANDLE hDrv, BYTE *val)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP2_GET_VALUE,
			NULL, 0, val, sizeof(BYTE), &nBytes, NULL);

	return bSuccess;
}
BOOL PL2303_GP2_SetValue(HANDLE hDrv, BYTE val)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP2_SET_VALUE,
			&val, sizeof(BYTE), NULL, 0, &nBytes, NULL);

	return bSuccess;
}

// For GP3
BOOL PL2303_GP3_Enalbe(HANDLE hDrv, BOOL enable)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP3_OUTPUT_ENABLE,
			&enable, sizeof(BYTE), NULL, 0, &nBytes, NULL);
	
	return bSuccess;
}
BOOL PL2303_GP3_GetValue(HANDLE hDrv, BYTE *val)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP3_GET_VALUE,
			NULL, 0, val, sizeof(BYTE), &nBytes, NULL);

	return bSuccess;
}
BOOL PL2303_GP3_SetValue(HANDLE hDrv, BYTE val)
{
	DWORD nBytes;

	BOOL bSuccess = DeviceIoControl(hDrv, GP3_SET_VALUE,
			&val, sizeof(BYTE), NULL, 0, &nBytes, NULL);

	return bSuccess;
}



