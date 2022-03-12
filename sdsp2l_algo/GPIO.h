#pragma once
// For GP0
BOOL PL2303_GP0_Enalbe(HANDLE hDrv, BOOL enable);
BOOL PL2303_GP0_GetValue(HANDLE hDrv, BYTE *val);
BOOL PL2303_GP0_SetValue(HANDLE hDrv, BYTE val);

// For GP1
BOOL PL2303_GP1_Enalbe(HANDLE hDrv, BOOL enable);
BOOL PL2303_GP1_GetValue(HANDLE hDrv, BYTE *val);
BOOL PL2303_GP1_SetValue(HANDLE hDrv, BYTE val);

// For GP2
BOOL PL2303_GP2_Enalbe(HANDLE hDrv, BOOL enable);
BOOL PL2303_GP2_GetValue(HANDLE hDrv, BYTE *val);
BOOL PL2303_GP2_SetValue(HANDLE hDrv, BYTE val);

// For GP3
BOOL PL2303_GP3_Enalbe(HANDLE hDrv, BOOL enable);
BOOL PL2303_GP3_GetValue(HANDLE hDrv, BYTE *val);
BOOL PL2303_GP3_SetValue(HANDLE hDrv, BYTE val);