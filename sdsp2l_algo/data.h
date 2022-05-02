#ifndef DEF_DATA_H
#define DEF_DATA_H
#ifdef __cplusplus
extern "C" {
#endif
#include "p2l.h"


extern int iWritePageData(int lba, int* pPayload, int page_num, int plane_num);
extern int iReadPageData(int lba, int* pPayload, int page_num, int plane_num);
extern int* pDataPayload;	                
extern int* pBlkErCntTbl[D_MAX_CH_CNT];					// erase count table with slc mode bit
extern int* pRdCntTbl[D_MAX_CH_CNT];						// die block read count table
extern int* pIdlTimeTbl[D_MAX_CH_CNT];
extern long chIoBurstCnt[D_MAX_CH_CNT];

#ifdef __cplusplus
}
#endif
#endif