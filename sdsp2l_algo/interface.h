#pragma once

// Export funciton interface
extern "C" __declspec(dllexport) long lp2lHitCnt();
extern "C" __declspec(dllexport) long lpl2ChkCnt();
extern "C" __declspec(dllexport) int iGetDevCap();
extern "C" __declspec(dllexport) int iGetDdrSize();

extern "C" __declspec(dllexport) int iIssueFlashCmd(int cmd, int ch, int blk, int plane, int page, int* pPayload);
extern "C" __declspec(dllexport) int iInitDeviceConfig(int dev_size, int ddr_size, int* bufPtr);
