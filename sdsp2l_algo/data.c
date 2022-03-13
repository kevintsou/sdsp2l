#include "data.h"
#include <malloc.h>

int* pDataPayload;	                // payload buffer

// write 16384+2208=18592 bytes to storage
int iWritePageData(int lbn, int *pPayload) {
	pDataPayload[lbn] = *pPayload;
	return 0;
}

// read 18592 bytes from storage
int iReadPageData(int lbn, int *pPayload) {
	*pPayload = pDataPayload[lbn];
	return 0;
}