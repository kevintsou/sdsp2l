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

enum {
    E_CMD_READ = 0,
    E_CMD_WRITE,
    E_CMD_ERASE
};

// get flash info
// pAddr : CH/CE/BLK/PLANE/PAGE
#define D_GET_CH_ADDR(x)        ((x >> dev_mgr.chSftCnt) & (dev_mgr.chBitNum - 1))
#define D_GET_BLOCK_ADDR(x)     ((x >> dev_mgr.blkSftCnt) & (dev_mgr.blkBitNum - 1))
#define D_GET_PLANE_ADDR(x)     ((x >> dev_mgr.planeSftCnt) & (dev_mgr.planeBitNum - 1))
#define D_GET_PAGE_ADDR(x)      ((x >> dev_mgr.pageSftCnt) & (dev_mgr.pageBitNum - 1))

#define D_CHK_P2L_BITMAP(x)     (pP2lBitmap[x / 32] & (1 << (x % 32)))


// function declaration
int iCopyP2lPageToDram(int pAddr);

/*
    Check if the entry in dram table is empty
*/
int iChkP2lValBitmap(int pAddr) {
    int ch = D_GET_CH_ADDR(pAddr);
    // get the entry index in dram table
    pAddr = (pAddr & (~(ch << dev_mgr.chSftCnt))) % p2l_mgr.bankSize;
    return pP2lValBitmap[ch][pAddr / 32] & (1 << (pAddr % 32));
}


/*
    set dram valid bitmap
*/
int iSetP2lValBitmap(int pAddr) {
    int ch = D_GET_CH_ADDR(pAddr);
    // get the entry index in dram table
    pAddr = (pAddr & (~(ch << dev_mgr.chSftCnt))) % p2l_mgr.bankSize;
    pAddr = (pAddr / p2l_mgr.pageSize) * p2l_mgr.pageSize;

    // set p2l bitmap table
    for (int i = 0; i < (p2l_mgr.pageSize / 32); i++) {
        pP2lValBitmap[ch][i + (pAddr / 32)] = 0xFFFFFFFF;
    }
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

    return pP2lDramTable[ch][pAddr % p2l_mgr.bankSize];
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

    pAddr = pAddr & (~(ch << dev_mgr.chSftCnt));

    if (!D_CHK_P2L_BITMAP(pAddr)) {   
        if (iChkP2lValBitmap(pAddr) == 0) {
            iCopyP2lPageToDram(pAddr);
        }
        else if (iSwapP2lPage(pAddr) == 1) {
            while (1);
        }
    }
    pP2lDramTable[ch][pAddr % p2l_mgr.bankSize] = lbn;
    return 0;
}


int iCopyP2lPageToDram(int pAddr) {

    int ch = D_GET_CH_ADDR(pAddr);
    // read the new p2l set from the src table to dram
    int addr = pAddr & (~(ch << dev_mgr.chSftCnt));
    int ddrPageOff = (addr % p2l_mgr.bankSize) / p2l_mgr.pageSize;
    int idx = pAddr / p2l_mgr.pageSize;

    pP2lPageIdx[ch][ddrPageOff] = idx;

    if (pP2lSrcTable == NULL) {
        while (1);
    }
    int *pSrc = &pP2lSrcTable[idx * p2l_mgr.pageSize * 4];
    int *pDes = &pP2lDramTable[ch][ddrPageOff * p2l_mgr.pageSize];
    memcpy(pDes, pSrc, p2l_mgr.pageSize * 4);

    pAddr = (pAddr / p2l_mgr.pageSize) * p2l_mgr.pageSize;
    // set p2l bitmap table
    for (int i = 0; i < (p2l_mgr.pageSize / 32); i++) {
        pP2lBitmap[i + (pAddr / 32)] = 0xFFFFFFFF;
    }

    // set valid bitmap indicate that the dram table entry is valid
    iSetP2lValBitmap(pAddr);

    return 0;
}

/*
    Swap P2L page
*/
int iSwapP2lPage(int pAddr){
    int ch = D_GET_CH_ADDR(pAddr);
    int idx = 0;
    int addr = 0;
    int ddrPageOff = 0;
    int *pSrc, *pDes;

    // store the p2l set to p2l src table
    addr = pAddr & (~(ch << dev_mgr.chSftCnt));
    ddrPageOff = (addr % p2l_mgr.bankSize) / p2l_mgr.pageSize;

    idx = pP2lPageIdx[ch][ddrPageOff];
    pSrc = &pP2lDramTable[ch][ddrPageOff * p2l_mgr.pageSize];
    pDes = &pP2lSrcTable[idx * p2l_mgr.pageSize * 4];

    memcpy(pDes, pSrc, p2l_mgr.pageSize * 4);

    // clear p2l bitmap table
    for (int i = 0; i < (p2l_mgr.pageSize / 32); i++) {
        pP2lBitmap[i + ((idx * p2l_mgr.pageSize) / 32)] = 0x0;
    }

    // read the new p2l set from the src table to dram
    idx = pAddr / p2l_mgr.pageSize;
    pP2lPageIdx[ch][ddrPageOff] = idx;
    pSrc = &pP2lSrcTable[idx * p2l_mgr.pageSize * 4];
    pDes = &pP2lDramTable[ch][ddrPageOff * p2l_mgr.pageSize];

    memcpy(pDes, pSrc, p2l_mgr.pageSize * 4);

    pAddr = (pAddr / p2l_mgr.pageSize) * p2l_mgr.pageSize;
    // set p2l bitmap table
    for (int i = 0; i < (p2l_mgr.pageSize /32); i ++) {
        pP2lBitmap[i + (pAddr / 32)] = 0xFFFFFFFF;
    }

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

    // error chck
    if (page) {
        //while (1);
    }

    if (lbn_mgr.availLbnCnt) {
        lbn = lbn_mgr.pLbnBuff[lbn_mgr.headPtr];
        lbn_mgr.chStartLbn[ch] = lbn;
        lbn_mgr.chAllocLbn[ch] = lbn + 1;   // already allocated to a page, point to next 1
        lbn_mgr.pBlk2Lbn[blk] = lbn;

        lbn_mgr.headPtr = (lbn_mgr.headPtr + 1) % dev_mgr.blkCnt;
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

    lbn_mgr.pLbnBuff[lbn_mgr.tailPtr] = lbn_mgr.pBlk2Lbn[blk];

    lbn_mgr.chStartLbn[ch] = 0;
    lbn_mgr.chAllocLbn[ch] = 0;
    lbn_mgr.tailPtr = (lbn_mgr.tailPtr + 1) % dev_mgr.blkCnt;
    lbn_mgr.availLbnCnt++;
    
    pAddr = (pAddr / p2l_mgr.pageSize) * p2l_mgr.pageSize;
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

    lbn = lbn_mgr.chAllocLbn[ch];
    lbn_mgr.chAllocLbn[ch]++;
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
    lbn_mgr.lbnEntryCnt = dev_mgr.blkCnt * dev_mgr.chCnt * dev_mgr.pageCnt * dev_mgr.planeCnt; // 1 lbn represent 1 page (16KB), ex. lbn0 represent lbn0-1bn3(16KB)
    lbn_mgr.lbnEntryPerBlk = lbn_mgr.lbnEntryCnt / dev_mgr.blkCnt;  
    
    lbn_mgr.pLbnBuff = (bufPtr + (bufSize/4));
    bufSize += dev_mgr.blkCnt * 4;
    memset(lbn_mgr.pLbnBuff, 0 , dev_mgr.blkCnt * 4);
    
    lbn_mgr.pBlk2Lbn = (bufPtr + (bufSize / 4));
    bufSize += dev_mgr.blkCnt * 4;
    memset(lbn_mgr.pBlk2Lbn, 0, dev_mgr.blkCnt * 4);

    lbn_mgr.headPtr = 0;
    lbn_mgr.tailPtr = 0;

    for (int i = 0; i < dev_mgr.blkCnt; i++) {
        lbn_mgr.pLbnBuff[i] = (lbn_mgr.lbnEntryCnt/1024) + (i * dev_mgr.pageCnt * dev_mgr.planeCnt);
        lbn_mgr.tailPtr = (lbn_mgr.tailPtr + 1) % dev_mgr.blkCnt;
        lbn_mgr.availLbnCnt++;
    }

    p2l_mgr.hitCnt = p2l_mgr.chkCnt = 0;
    p2l_mgr.pageSize = 1024;
    p2l_mgr.bankSize = (ddrSize * 1024 * 1024) / (dev_mgr.chCnt * sizeof(int));


    // allocate p2l bitmap memory
    pP2lBitmap = (bufPtr + (bufSize / 4));
    bufSize += (lbn_mgr.lbnEntryCnt / (32 / 4));
    memset(pP2lBitmap, 0, lbn_mgr.lbnEntryCnt / (32 / 4));
    
    // allocate p2l table memory
    for (int idx = 0; idx < dev_mgr.chCnt; idx++) {

        pP2lValBitmap[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * (p2l_mgr.bankSize / 32));

        pP2lDramTable[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * p2l_mgr.bankSize);

        pP2lPageIdx[idx] = (bufPtr + (bufSize / 4));
        bufSize += (sizeof(int) * (p2l_mgr.bankSize / 1024));

       
        if (pP2lValBitmap[idx] != NULL) {
            memset(pP2lValBitmap[idx], 0, sizeof(int) * (p2l_mgr.bankSize / 32));
        }
        if (pP2lDramTable[idx] != NULL) {
            memset(pP2lDramTable[idx], 0, sizeof(int) * p2l_mgr.bankSize);
        }
        if (pP2lPageIdx[idx] != NULL) {
            memset(pP2lPageIdx[idx], 0, sizeof(int) * (p2l_mgr.bankSize / 1024));
        }
    }
    
    pP2lSrcTable = (bufPtr + (bufSize / 4));
    bufSize += (sizeof(int) * lbn_mgr.lbnEntryCnt);
    if (pP2lSrcTable != NULL) {
        memset(pP2lSrcTable, 0, sizeof(int) * lbn_mgr.lbnEntryCnt);
    }

    // allocate data payload buffer
    pDataPayload = (bufPtr + (bufSize / 4));
    bufSize += (sizeof(int) * lbn_mgr.lbnEntryCnt);
    if (pDataPayload != NULL) {
        memset(pDataPayload, 0, sizeof(int) * lbn_mgr.lbnEntryCnt);
    }

    p2l_mgr.tableSize = bufSize;
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
    else if(blk >= dev_mgr.blkBitNum) {
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
int iFlashCmdHandler(int cmd, int ch, int blk, int plane, int page, int *pPayload){
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
            if (lbn_mgr.chAllocLbn[ch] == 0) {  // notice that the write page may out of order
                lbn = iAllocBlkLbn(pAddr); // get a block start lbn
            }
            else {
                // allocate a page lbn
                lbn = iAllocPageLbn(pAddr);
            }
        }
        iWritePageData(lbn, &lbn); // use lbn as the data write in to storage for comparing 
        iUpdateDataLbn(pAddr, lbn);
        break;

    case E_CMD_READ:
        if (lbn == 0) {
            *pPayload = 0xFFFFFFFF;
            lbn = 0xFFFFFFFF;
            return lbn;
        }
        iReadPageData(lbn, pPayload);
        break;

    case E_CMD_ERASE:
        // if data block programed
        if (lbn) {
            iRecycleBlkLbn(pAddr);
        }
        else {
            lbn = 0xFFFFFFFF;
        }
        break;
    default:
        break;
    }
    return lbn;
}
