#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdbool.h>
#include <stdlib.h>

#include "interface.h"
#include "p2l.h"
#include "data.h"

long lp2lHitCnt() {
	return p2l_mgr.hitCnt;
}

long lpl2ChkCnt() {
	return p2l_mgr.chkCnt;
}

int iGetDevCap() {
	return dev_mgr.dev_cap;
}

int iGetDdrSize() {
	return dev_mgr.ddr_size;
}

int iIssueFlashCmd(int cmd, int pAddr, int* pPayload) {
	return iFlashCmdHandler(cmd, pAddr, pPayload);
}

int iInitDeviceConfig(int dev_size, int ddr_size) {
	return iInitDevConfig(dev_size, ddr_size);
}
