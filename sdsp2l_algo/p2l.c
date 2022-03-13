#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "p2l.h"
#include "data.h"

// for p2l.c internal use, don't export
int* pP2lBitmap;                    // P2L Bitmap
int* pP2lTable[D_MAX_CH_CNT];       // P2L Table 
int* pP2lPageIdx[D_MAX_CH_CNT];     // P2L Page Idx on the src table
int* pP2lSrcTable;                  // P2L Table on device

t_dev_mgr dev_mgr;
t_p2l_mgr p2l_mgr;
t_lbn_mgr lbn_mgr;

enum {
    D_CMD_READ = 0,
    D_CMD_WRITE,
    D_CMD_ERASE
};

// get flash info
// pAddr : CH/CE/BLK/PLANE/PAGE
// lAddr : CH/CE/BLK/PAGE1/PLANE/PAGE0, PAGE0:1024 entry, 10bits
#define D_GET_CH_ADDR(x)        ((x >> dev_mgr.chSftCnt) & (dev_mgr.chBitNum - 1))
#define D_GET_BLOCK_ADDR(x)     ((x >> dev_mgr.blkSftCnt) & (dev_mgr.blkBitNum - 1))
#define D_GET_PLANE_ADDR(x)     ((x >> dev_mgr.planeSftCnt) & (dev_mgr.planeBitNum - 1))
#define D_GET_PAGE_ADDR(x)      ((x >> dev_mgr.pageSftCnt) & (dev_mgr.pageBitNum - 1))

#define D_CHK_P2L_BITMAP(x)     (pP2lBitmap[x / 32] & (1 << (x % 32)))


/*
    Translate to LBA info from phisical address

    Notice : it will cause unexpected failure is the p2l is not in dram,
             so make sure the p2l page already swap in dram
*/
int iGetLbnInfo(int pAddr){
    int ch = D_GET_CH_ADDR(pAddr);
    
    pAddr = pAddr & (~(ch << dev_mgr.chSftCnt));

    return pP2lTable[ch][pAddr % p2l_mgr.bankSize];
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
        if(iSwapP2lPage(pAddr) == 1){
            while(1);
        }
        else {
            lbn = iGetLbnInfo(pAddr);
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
        if (iSwapP2lPage(pAddr) == 1) {
            while (1);
        }
    }
    pP2lTable[ch][pAddr % p2l_mgr.bankSize] = lbn;
}


/*
    Swap P2L page
*/
int iSwapP2lPage(int pAddr){
    int ch = D_GET_CH_ADDR(pAddr);
    int idx = 0;
    int addr = 0;
    int pageOff = 0;
    int *pSrc, *pDes;

    // store the p2l set to p2l src table
    addr = pAddr & (~(ch << dev_mgr.chSftCnt));
    pageOff = (addr % p2l_mgr.bankSize) / 1024;

    idx = pP2lPageIdx[ch][pageOff];
    pSrc = &pP2lTable[ch][pageOff * 1024];
    pDes = &pP2lSrcTable[idx * 4096];

    memcpy(pDes, pSrc, 4096);

    // clear p2l bitmap table
    for (int i = 0; i < (1024 / 32); i++) {
        pP2lBitmap[i + (idx / 8)] = 0x0;
    }

    // read the new p2l set from the src table to dram
    idx = pAddr / 1024;
    pP2lPageIdx[ch][pageOff] = idx;
    pSrc = &pP2lSrcTable[idx * 4096];
    pDes = &pP2lTable[ch][pageOff * 1024];

    memcpy(pDes, pSrc, 4096);

    // set p2l bitmap table
    for (int i = 0; i < (1024/32); i ++) {
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
        while (1);
    }

    if (lbn_mgr.availLbnCnt) {
        lbn = lbn_mgr.pLbnBuff[lbn_mgr.headPtr];
        lbn_mgr.chStartLbn[ch] = lbn;
        lbn_mgr.chAllocLbn[ch] = lbn + 1;   // already allocated to a page, point to next 1
        lbn_mgr.pBlk2Lbn[blk] = lbn;

        lbn_mgr.headPtr = (lbn_mgr.headPtr + 1) % lbn_mgr.lbnBufCnt;
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
    lbn_mgr.tailPtr = (lbn_mgr.tailPtr + 1) % lbn_mgr.lbnBufCnt;
    lbn_mgr.availLbnCnt++;
    
    // clear lbn in p2l table for each page
    // use dma instead in future
    for (int i = 0; i < (dev_mgr.pageCnt * dev_mgr.planeCnt); i++) {
        iUpdateDataLbn(pAddr, 0x0);
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
int iInitDevConfig(int devCap, int ddrSize) {
    dev_mgr.chCnt = 8;
    dev_mgr.pageCnt = 1024;
    dev_mgr.planeCnt = 4;
    dev_mgr.blkCnt = (devCap * 1024 * 1024) / (16 * dev_mgr.pageCnt * dev_mgr.planeCnt * dev_mgr.chCnt);

    dev_mgr.chBitNum = (int)log2(dev_mgr.chCnt);
    dev_mgr.blkBitNum = (int)log2(dev_mgr.blkCnt);
    dev_mgr.pageBitNum = (int)log2(dev_mgr.pageCnt);
    dev_mgr.planeBitNum = (int)log2(dev_mgr.planeCnt);

    dev_mgr.pageSftCnt = 0;
    dev_mgr.planeSftCnt = dev_mgr.pageSftCnt + dev_mgr.pageBitNum;
    dev_mgr.blkSftCnt = dev_mgr.planeSftCnt + dev_mgr.planeBitNum;
    dev_mgr.chSftCnt = dev_mgr.blkSftCnt + dev_mgr.blkBitNum;

    // initialize lbn manager relative parameters
    lbn_mgr.lbnEntryCnt = (devCap * 1024 / 4);
    lbn_mgr.lbnBufCnt = lbn_mgr.lbnEntryCnt / dev_mgr.blkCnt;
    lbn_mgr.pLbnBuff = (int*)malloc(lbn_mgr.lbnBufCnt * 4);
    lbn_mgr.headPtr = 0;
    lbn_mgr.tailPtr = 0;

    for (int i = 0; i < lbn_mgr.lbnBufCnt; i++) {
        lbn_mgr.pLbnBuff[i] = lbn_mgr.lbnEntryCnt + (i * dev_mgr.pageCnt * dev_mgr.planeCnt);
        lbn_mgr.tailPtr = lbn_mgr.tailPtr + 1;
        lbn_mgr.availLbnCnt++;
    }

    // allocate p2l ddr table 
    int bitMapSize = ((devCap * 1024 * 1024) / 16) / 8;
    int payloadSize = sizeof(int) * ((devCap * 1024 * 1024) / 16);
    int p2lEntryCnt = ((devCap * 1024 * 1024) / 16);

    p2l_mgr.hitCnt = p2l_mgr.chkCnt = 0;
    p2l_mgr.pageSize = 1024;
    p2l_mgr.bankSize = (ddrSize * 1024 * 1024) / (dev_mgr.chCnt * sizeof(int));

    // allocate p2l bitmap memory
    pP2lBitmap = (int *)malloc(bitMapSize);

    // allocate p2l table memory
    for (int idx = 0; idx < dev_mgr.chCnt; idx++) {
        pP2lTable[idx] = (int *)malloc(sizeof(int) * p2l_mgr.bankSize);
        pP2lPageIdx[idx] = (int*)malloc(p2l_mgr.bankSize / 4096);
    }
    
    pP2lSrcTable = (int*)malloc(sizeof(int) * p2lEntryCnt);

    // allocate data payload buffer
    pDataPayload = (int*)malloc(payloadSize);
    return 0;
}


int iFlashCmdHandler(int cmd, int pAddr, int *pPayload){
    int lbn = 0;
    int page = D_GET_PAGE_ADDR(pAddr);
    int plane = D_GET_PLANE_ADDR(pAddr);

    switch (cmd)
    {
    case D_CMD_WRITE:
        lbn = iGetDataLbn(pAddr);
        if (!lbn) {
            // new block
            if ( (page == 0) && (plane == 0)) {
                lbn = iAllocBlkLbn(pAddr); // get a block start lbn
            }
            else {
                // allocate a page lbn
                lbn = iAllocPageLbn(pAddr);
            }
        }
        iWritePageData(lbn, pPayload);
        iUpdateDataLbn(pAddr, lbn);
        break;

    case D_CMD_READ:
        lbn = iGetDataLbn(pAddr);
        iReadPageData(lbn, pPayload);
        break;

    case D_CMD_ERASE:
        break;
    default:
        break;
    }
    return 0;
}

/*
    main funciton
*/
int sdsp2l_main(){

    iInitDevConfig(128, 32);    // 128GB/32MB ddr, 256 block


    return 0;
}