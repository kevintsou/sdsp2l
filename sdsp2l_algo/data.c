#include "data.h"
#include "p2l.h"
#include <malloc.h>

int* pDataPayload;	                // payload buffer

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