#include "data.h"
#include "p2l.h"
#include <malloc.h>

int* pDataPayload;	                // payload buffer
int* pBlkErCntTbl[D_MAX_CH_CNT];					// erase count table with slc mode bit
int* pRdCntTbl[D_MAX_CH_CNT];						// die block read count table
int* pIdlTimeTbl[D_MAX_CH_CNT];						// block idle time table
long chIoBurstCnt[D_MAX_CH_CNT];						// io burst count for each channel


// write 16384+2208=18592 bytes to storage
int iWritePageData(int lbn, int *pPayload) {
	pDataPayload[lbn - (lbn_mgr.lbnEntryCnt / 1024)] = *pPayload;
	return 0;
}

// read 18592 bytes from storage
int iReadPageData(int lbn, int *pPayload) {
	*pPayload = pDataPayload[lbn - (lbn_mgr.lbnEntryCnt / 1024)];
	return 0;
}