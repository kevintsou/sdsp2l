#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "p2l.h"
#include "data.h"

// for p2l.c internal use, don't export
static int* pP2lBitmap;                    // P2L Bitmap
static int* pP2lValBitmap[D_MAX_CH_CNT];   // Dram P2L entry valid bitmap
static int* pP2lDramTable[D_MAX_CH_CNT];   // P2L Table 
static int* pP2lPageIdx[D_MAX_CH_CNT];     // P2L Page Idx on the dram table
static int* pP2lSrcTable;                  // P2L Table on device


t_dev_mgr dev_mgr;
t_p2l_mgr p2l_mgr;
t_lbn_mgr lbn_mgr;

// get flash info
// pAddr : CH/CE/BLK/PLANE/PAGE
#define D_GET_CH_ADDR(x)        ((x >> dev_mgr.chSftCnt) & (dev_mgr.chCnt - 1))
#define D_GET_BLOCK_ADDR(x)     ((x >> dev_mgr.blkSftCnt) & (dev_mgr.blkCnt - 1))
#define D_GET_PLANE_ADDR(x)     ((x >> dev_mgr.planeSftCnt) & (dev_mgr.planeCnt - 1))
#define D_GET_PAGE_ADDR(x)      ((x >> dev_mgr.pageSftCnt) & (dev_mgr.pageCnt - 1))

#define D_CHK_P2L_BITMAP(x)     (pP2lBitmap[(x/p2l_mgr.entryPerPage) / 32] & (1 << ((x/p2l_mgr.entryPerPage) % 32)))

//
// function declaration
int iCopyP2lPageToDram(int pAddr);

/*
    Check if the entry in dram table is empty
*/
int iChkP2lValBitmap(int pAddr) {
    int ch = D_GET_CH_ADDR(pAddr);
    pAddr = pAddr & (~(ch << dev_mgr.chSftCnt));
    // get the entry index in dram table
    int dramPageIdx = (pAddr / p2l_mgr.entryPerPage) % p2l_mgr.pagePerBank;
    return pP2lValBitmap[ch][dramPageIdx / 32] & (1 << (dramPageIdx % 32));
}


/*
    set dram valid bitmap
*/
int iSetP2lValBitmap(int pAddr) {
    int ch = D_GET_CH_ADDR(pAddr);
    pAddr = pAddr & (~(ch << dev_mgr.chSftCnt));
    // get the entry index in dram table
    int dramPageIdx = (pAddr / p2l_mgr.entryPerPage) % p2l_mgr.pagePerBank;

    pP2lValBitmap[ch][dramPageIdx / 32] |= (1 << (dramPageIdx % 32));
    return 0;
}


/*
    Translate to LBA info from phisical address

    Notice : it will cause unexpected failure is the p2l is not in dram,
             so make sure the p2l page already swap in dram
*/
int iGetLbnInfo(int pAddr){
    int ch = D_GET_CH_ADDR(pAddr);
    
    pAddr = pAddr & (~(ch << dev_mgr.chSftCnt));

    return pP2lDramTable[ch][((pAddr / p2l_mgr.entryPerPage) % p2l_mgr.pagePerBank)* p2l_mgr.entryPerPage + (pAddr % p2l_mgr.entryPerPage)];
}


/*
    P2L main procedure

    return value:
    LBA information use to data read/write
*/
int iGetDataLbn(int pAddr){
    int lbn = 0;

    p2l_mgr.chkCnt++;

    if(D_CHK_P2L_BITMAP(pAddr)){   // p2l page hit
        // get the lba from p2l
        lbn =  iGetLbnInfo(pAddr);
        p2l_mgr.hitCnt++;
    }
    else{   // p2l page miss
        if (iChkP2lValBitmap(pAddr) == 0) {
            iCopyP2lPageToDram(pAddr);
            lbn = iGetLbnInfo(pAddr);
        }
        else {
            if (iSwapP2lPage(pAddr) == 1) {
                while (1);
            }
            else {
                lbn = iGetLbnInfo(pAddr);
            }
        }
    }

    return lbn;
}


/*
    update lbn of pAddr
*/
int iUpdateDataLbn(int pAddr, int lbn) {
    int ch = D_GET_CH_ADDR(pAddr);

    if (!D_CHK_P2L_BITMAP(pAddr)) {   
        if (iChkP2lValBitmap(pAddr) == 0) {
            iCopyP2lPageToDram(pAddr);
        }
        else if (iSwapP2lPage(pAddr) == 1) {
            while (1);
        }
    }

    pAddr = pAddr & (~(ch << dev_mgr.chSftCnt));
    pP2lDramTable[ch][((pAddr / p2l_mgr.entryPerPage) % p2l_mgr.pagePerBank) * p2l_mgr.entryPerPage + (pAddr % p2l_mgr.entryPerPage)] = lbn;
    return 0;
}


int iCopyP2lPageToDram(int pAddr) {

    int ch = D_GET_CH_ADDR(pAddr);
    // read the new p2l set from the src table to dram
    int addr = pAddr & (~(ch << dev_mgr.chSftCnt));
    int ddrPageOff = ((addr / p2l_mgr.entryPerPage) % p2l_mgr.pagePerBank);
    int pageIdx = pAddr / p2l_mgr.entryPerPage;

    pP2lPageIdx[ch][ddrPageOff] = pageIdx;

    if (pP2lSrcTable == NULL) {
        while (1);
    }
    int *pSrc = &pP2lSrcTable[pageIdx * p2l_mgr.entryPerPage];
    int *pDes = &pP2lDramTable[ch][ddrPageOff * p2l_mgr.entryPerPage];
    memcpy(pDes, pSrc, p2l_mgr.entryPerPage * 4);

    addr = (pAddr / p2l_mgr.entryPerPage) * p2l_mgr.entryPerPage;
    // set p2l bitmap table
    pP2lBitmap[(addr / p2l_mgr.entryPerPage) / 32] |= 1 << ((addr / p2l_mgr.entryPerPage) % 32);

    // set valid bitmap indicate that the dram table entry is valid
    iSetP2lValBitmap(pAddr);
    return 0;
}

/*
    Swap P2L page
*/
int iSwapP2lPage(int pAddr){
    int ch = D_GET_CH_ADDR(pAddr);
    int pageIdx = 0;
    int addr = 0;
    int ddrPageOff = 0;
    int *pSrc, *pDes;

    // flush the p2l page from dram to p2l src table
    addr = pAddr & (~(ch << dev_mgr.chSftCnt));
    ddrPageOff = ((addr / p2l_mgr.entryPerPage) % p2l_mgr.pagePerBank);

    pageIdx = pP2lPageIdx[ch][ddrPageOff];
    pSrc = &pP2lDramTable[ch][ddrPageOff * p2l_mgr.entryPerPage];
    pDes = &pP2lSrcTable[pageIdx * p2l_mgr.entryPerPage];

    memcpy(pDes, pSrc, p2l_mgr.entryPerPage * 4);

    // clear p2l bitmap table
    pP2lBitmap[pageIdx / 32] &= ~(1 << (pageIdx % 32));

    // read the new p2l set from the src table to dram
    pageIdx = pAddr / p2l_mgr.entryPerPage;
    pP2lPageIdx[ch][ddrPageOff] = pageIdx;
    pSrc = &pP2lSrcTable[pageIdx * p2l_mgr.entryPerPage];
    pDes = &pP2lDramTable[ch][ddrPageOff * p2l_mgr.entryPerPage];

    memcpy(pDes, pSrc, p2l_mgr.entryPerPage * 4);

    pP2lBitmap[pageIdx / 32] |= 1 << (pageIdx % 32);

    return 0;
}


/*
    Allocate new block lbn
*/
int iAllocBlkLbn(int pAddr) {
    int ch = D_GET_CH_ADDR(pAddr);
    int blk = D_GET_BLOCK_ADDR(pAddr);
    int page = D_GET_PAGE_ADDR(pAddr);
    int lbn = 0;

    if (lbn_mgr.availLbnCnt) {
        lbn = lbn_mgr.pLbnBuff[lbn_mgr.headPtr];
        lbn_mgr.pBlk2Lbn[ch][blk] = lbn;
        lbn_mgr.pChRestLbnNum[ch][blk] = (dev_mgr.pageCnt * dev_mgr.planeCnt) - 1;
        lbn_mgr.pChAllocLbn[ch][blk] = lbn + 5;   // already allocated to a page, point to next 1
        
        lbn_mgr.headPtr = (lbn_mgr.headPtr + 1) % lbn_mgr.lbnQdepth;
        lbn_mgr.availLbnCnt--;
    }
    else {
        while (1);
    }

    return lbn;
}


/*
    Recycle lbn
*/
int iRecycleBlkLbn(int pAddr) {
    int ch = D_GET_CH_ADDR(pAddr);
    int blk = D_GET_BLOCK_ADDR(pAddr);

    lbn_mgr.pLbnBuff[lbn_mgr.tailPtr] = lbn_mgr.pBlk2Lbn[ch][blk];

    lbn_mgr.pChRestLbnNum[ch][blk] = 0;
    lbn_mgr.pChAllocLbn[ch][blk] = 0;
    lbn_mgr.tailPtr = (lbn_mgr.tailPtr + 1) % lbn_mgr.lbnQdepth;
    lbn_mgr.availLbnCnt++;
    
    pAddr = (pAddr >> dev_mgr.blkSftCnt) << dev_mgr.blkSftCnt;
    // clear lbn in p2l table for each page
    // use dma instead in future
    for (int i = 0; i < (dev_mgr.pageCnt * dev_mgr.planeCnt); i++) {
        iUpdateDataLbn(pAddr+i, 0x0);
    }
    return 0;
}


/*
    Allocate new lbn for write
*/
int iAllocPageLbn(int pAddr) {
    int ch = D_GET_CH_ADDR(pAddr);
    int blk = D_GET_BLOCK_ADDR(pAddr);
    int lbn = 0;

    lbn = lbn_mgr.pChAllocLbn[ch][blk];
    lbn_mgr.pChAllocLbn[ch][blk] += 5;
    lbn_mgr.pChRestLbnNum[ch][blk] -= 1;
    return lbn;
}


/*
    Init device config
    
    Parameter:
    dev_cap : device capacity in GB 
    ddr_size : ddr size in MB
*/
int iInitDevConfig(int devCap, int ddrSize, int chCnt, int planeCnt, int pageCnt, int *bufPtr) {
    int bufSize = 0;

    if ((chCnt > D_MAX_CH_CNT) || (planeCnt > D_MAX_PLANE_CNT) || (pageCnt > D_MAX_PAGE_CNT)) {
        return 1;
    }

    if (ddrSize > (devCap / 4)) {
        ddrSize = (devCap / 4);
    }

    // clear structure content
    memset(&dev_mgr, 0, sizeof(dev_mgr));
    memset(&p2l_mgr, 0, sizeof(p2l_mgr));
    memset(&lbn_mgr, 0, sizeof(lbn_mgr));


    dev_mgr.dev_cap = devCap;
    dev_mgr.ddr_size = ddrSize;

    dev_mgr.chCnt = chCnt;
    dev_mgr.pageCnt = pageCnt;
    dev_mgr.planeCnt = planeCnt;
    dev_mgr.blkCnt = (devCap * 1024 * 1024) / (16 * dev_mgr.chCnt * dev_mgr.pageCnt * dev_mgr.planeCnt);    // die block count

    dev_mgr.chBitNum = (int)log2(dev_mgr.chCnt);
    dev_mgr.blkBitNum = (int)log2(dev_mgr.blkCnt);
    dev_mgr.pageBitNum = (int)log2(dev_mgr.pageCnt);
    dev_mgr.planeBitNum = (int)log2(dev_mgr.planeCnt);

    dev_mgr.pageSftCnt = 0;
    dev_mgr.planeSftCnt = dev_mgr.pageSftCnt + dev_mgr.pageBitNum;
    dev_mgr.blkSftCnt = dev_mgr.planeSftCnt + dev_mgr.planeBitNum;
    dev_mgr.chSftCnt = dev_mgr.blkSftCnt + dev_mgr.blkBitNum;
     
    // initialize lbn manager relative parameters
    lbn_mgr.lbnEntryCnt = dev_mgr.blkCnt * dev_mgr.chCnt * dev_mgr.pageCnt * dev_mgr.planeCnt; // 1 lbn represent 1 page (16KB), ex. lbn0 represent lbn0-1bn4(20KB)
    lbn_mgr.lbnEntryPerBlk = lbn_mgr.lbnEntryCnt / dev_mgr.blkCnt;  
    lbn_mgr.lbnQdepth = dev_mgr.blkCnt * dev_mgr.chCnt;

    lbn_mgr.pLbnBuff = (bufPtr + (bufSize/4));
    bufSize += lbn_mgr.lbnQdepth * 4;
    memset(lbn_mgr.pLbnBuff, 0 , lbn_mgr.lbnQdepth * 4);
   

    lbn_mgr.headPtr = 0;
    lbn_mgr.tailPtr = 0;

    // allocate lbn queue
    for (int i = 0; i < lbn_mgr.lbnQdepth; i++) {
        lbn_mgr.pLbnBuff[i] = (lbn_mgr.lbnEntryCnt/ 1024) + (i * dev_mgr.pageCnt * dev_mgr.planeCnt * 5); // 16KB page + 2KB spare share a entry , there are 5 lbn in a entry
        lbn_mgr.tailPtr = (lbn_mgr.tailPtr + 1) % lbn_mgr.lbnQdepth;
        lbn_mgr.availLbnCnt++;
    }

    p2l_mgr.hitCnt = p2l_mgr.chkCnt = 0;
    p2l_mgr.entryPerPage = 1024;
    p2l_mgr.entryPerBank = (ddrSize * 1024 * 1024) / (dev_mgr.chCnt * sizeof(int));
    p2l_mgr.pagePerBank = p2l_mgr.entryPerBank / p2l_mgr.entryPerPage;


    // allocate p2l bitmap memory
    pP2lBitmap = (bufPtr + (bufSize / 4));
    bufSize += ((lbn_mgr.lbnEntryCnt / p2l_mgr.entryPerPage) * 4);
    memset(pP2lBitmap, 0, ((lbn_mgr.lbnEntryCnt / p2l_mgr.entryPerPage) * 4));
    
    // allocate p2l table memory
    for (int idx = 0; idx < dev_mgr.chCnt; idx++) {

        pP2lValBitmap[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * (p2l_mgr.entryPerBank / p2l_mgr.entryPerPage / 32));
        if (pP2lValBitmap[idx] != NULL) {
            memset(pP2lValBitmap[idx], 0, sizeof(int) * (p2l_mgr.entryPerBank / p2l_mgr.entryPerPage / 32));
        }

        pP2lDramTable[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * p2l_mgr.entryPerBank);
        if (pP2lDramTable[idx] != NULL) {
            memset(pP2lDramTable[idx], 0, sizeof(int) * p2l_mgr.entryPerBank);
        }

        pP2lPageIdx[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * (p2l_mgr.entryPerBank / 1024));
        if (pP2lPageIdx[idx] != NULL) {
            memset(pP2lPageIdx[idx], 0, sizeof(int) * (p2l_mgr.entryPerBank / 1024));
        }
       
        lbn_mgr.pChRestLbnNum[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * dev_mgr.blkCnt);
        if (lbn_mgr.pChRestLbnNum[idx] != NULL) {
            memset(lbn_mgr.pChRestLbnNum[idx], 0, (sizeof(int) * dev_mgr.blkCnt));
        }

        lbn_mgr.pChAllocLbn[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * dev_mgr.blkCnt);
        if (lbn_mgr.pChAllocLbn[idx] != NULL) {
            memset(lbn_mgr.pChAllocLbn[idx], 0, (sizeof(int) * dev_mgr.blkCnt));
        }

        lbn_mgr.pBlk2Lbn[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * dev_mgr.blkCnt);
        memset(lbn_mgr.pBlk2Lbn[idx], 0, (sizeof(int)* dev_mgr.blkCnt));

        pBlkErCntTbl[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * dev_mgr.blkCnt);
        if (pBlkErCntTbl[idx] != NULL) {
            memset(pBlkErCntTbl[idx], 0, sizeof(int) * dev_mgr.blkCnt);
        }

        pRdCntTbl[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * dev_mgr.blkCnt);
        if (pRdCntTbl[idx] != NULL) {
            memset(pRdCntTbl[idx], 0, sizeof(int) * dev_mgr.blkCnt);
        }

        pIdlTimeTbl[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * dev_mgr.blkCnt);
        if (pIdlTimeTbl[idx] != NULL) {
            memset(pIdlTimeTbl[idx], 0, sizeof(int) * dev_mgr.blkCnt);
        }
    }
    
    pP2lSrcTable = (bufPtr + (bufSize / 4));
    bufSize += (sizeof(int) * lbn_mgr.lbnEntryCnt);
    if (pP2lSrcTable != NULL) {
        memset(pP2lSrcTable, 0, sizeof(int) * lbn_mgr.lbnEntryCnt);
    }

    // allocate data payload buffer
    pDataPayload = (bufPtr + (bufSize / 4));
    bufSize += (sizeof(int) * lbn_mgr.lbnEntryCnt * 4);
    if (pDataPayload != NULL) {
        memset(pDataPayload, 0, sizeof(int) * lbn_mgr.lbnEntryCnt * 4);
    }

    p2l_mgr.tableSize = bufSize;

    file_mgr.blk_lbn_per_plane_rd = 0;
    file_mgr.blk_lbn_per_plane_wr = 0;
    file_mgr.fd_write = -1;

    return 0;
}

/*
    check if the address violate the flash configuration
*/
int iChkFlashAddr(int ch, int blk, int plane, int page) {
    int err = 0;
    if (ch >= dev_mgr.chCnt) {
        err = 1;
    }
    else if(blk >= dev_mgr.blkCnt) {
        err = 1;
    }
    else if (plane >= dev_mgr.planeCnt) {
        err = 1;
    }
    else if (page >= dev_mgr.pageCnt) {
        err = 1;
    }
    return err;
}

/*
    Handle flash command here
*/
int iFlashCmdHandler(int cmd, int ch, int blk, int plane, int page, int* pPayload){
    int lbn = 0;
    
    if (iChkFlashAddr(ch, blk, plane, page)) {
        return lbn; // cmd hander completed, ileagal flash address, return lbn = 0 to host.
    }

    int pAddr = (ch << dev_mgr.chSftCnt) | (blk << dev_mgr.blkSftCnt) | (plane << dev_mgr.planeSftCnt) | page;

    lbn = iGetDataLbn(pAddr);

    switch (cmd)
    {
    case E_CMD_WRITE:
        if (!lbn) {
            // new block
            //if ( (page == 0) && (plane == 0)) {
            if (lbn_mgr.pChRestLbnNum[ch][blk] == 0) {  // notice that the write page may out of order
                lbn = iAllocBlkLbn(pAddr); // get a block start lbn
            }
            else {
                // allocate a page lbn
                lbn = iAllocPageLbn(pAddr);
            }
        }
        iWritePageData(lbn, pPayload, page, plane); // use lbn as the data write in to storage for comparing 
        iUpdateDataLbn(pAddr, lbn);
        break;

    case E_CMD_READ:
        if (lbn == 0) {
            *pPayload = 0xFFFFFFFF;
            lbn = 0xFFFFFFFF;
            return lbn;
        }

        iReadPageData(lbn, pPayload, page, plane);

        // debug code
        if (*pPayload != pAddr) {
            int ch = D_GET_CH_ADDR(*pPayload);
            int blk = D_GET_BLOCK_ADDR(*pPayload);
            int plane = D_GET_PLANE_ADDR(*pPayload);
            int page = D_GET_PAGE_ADDR(*pPayload);
            while (1);
        }

        pRdCntTbl[ch][blk]++;
        break;

    case E_CMD_ERASE:
        // if data block programed
        if (lbn) {
            iRecycleBlkLbn(pAddr);
        }
        else {
            lbn = 0xFFFFFFFF;
        }

        pBlkErCntTbl[ch][blk]++;
        break;
    default:
        break;
    }

    chIoBurstCnt[ch]++;
    return lbn;
}

int iIssueFlashCmdpAddr(int cmd, int pAddr, int* pPayload) {
    int ch = D_GET_CH_ADDR(pAddr);
    int blk = D_GET_BLOCK_ADDR(pAddr);
    int page = D_GET_PAGE_ADDR(pAddr);
    int plane = D_GET_PLANE_ADDR(pAddr);

    return iFlashCmdHandler(cmd, ch, blk, plane, page, pPayload);
}