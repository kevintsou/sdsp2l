#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdbool.h>
#include <stdlib.h>

#include "interface.h"
#include "p2l.h"
#include "data.h"

int lp2lHitCnt() {
	int hitRate = ((float)p2l_mgr.hitCnt  / (float)p2l_mgr.chkCnt) * 100;
	return hitRate;
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

int iIssueFlashCmdEn(int cmd, int pAddr, int* pPayload) {
	return iIssueFlashCmdpAddr(cmd, pAddr, pPayload);
}

int iInitDeviceConfig(int devCap, int ddrSize, int chCnt, int planeCnt, int pageCnt, int* bufPtr) {
	return iInitDevConfig(devCap, ddrSize, chCnt, planeCnt, pageCnt, bufPtr);
}

int iGetTableSize() {
	return p2l_mgr.tableSize;
}

int* iGetEraseCntTable(int ch) {
	return pBlkErCntTbl[ch];
}

int* iGetReadCntTable(int ch) {
	return pRdCntTbl[ch];
}

long iGetIoBurstCnt(int ch) {
	return chIoBurstCnt[ch];
}

int iClearChkHitCnt() {
	p2l_mgr.hitCnt = 0;
	p2l_mgr.chkCnt = 0;
	for (int ch = 0; ch < dev_mgr.chCnt; ch++) {
		chIoBurstCnt[ch] = 0;
	}
	return 0;
}
