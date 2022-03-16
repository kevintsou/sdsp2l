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

int iIssueFlashCmd(int cmd, int ch, int blk, int plane, int page, int* pPayload) {
	return iFlashCmdHandler(cmd, ch, blk, plane, page, pPayload);
}

int iInitDeviceConfig(int devCap, int ddrSize, int chCnt, int planeCnt, int pageCnt, int* bufPtr) {
	return iInitDevConfig(devCap, ddrSize, chCnt, planeCnt, pageCnt, bufPtr);
}

int iGetTableSize() {
	return p2l_mgr.tableSize;
}