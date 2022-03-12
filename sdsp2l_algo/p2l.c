#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "p2l.h"
#include "data.h"

int* pP2lBitmap;                    // P2L Bitmap
int* pP2lTable[D_MAX_CH_CNT];       // P2L Table 
int* pP2lPageLbn[D_MAX_CH_CNT];     // P2L Page LBN
int* pP2lSrcTable;                  // P2L Table on device

// device configuration
int ch_cnt;
int blk_cnt;
int page_cnt;
int plane_cnt;

int ch_bit_num;
int blk_bit_num;
int page_bit_num;
int plane_bit_num;

int ch_sft_cnt;
int blk_sft_cnt;
int page_sft_cnt;
int plane_sft_cnt;

int lbn_cnt;
int rest_lbn_cnt;

long p2l_hit_cnt;
long p2l_chk_cnt;
int p2l_bank_size;      // entry in a bank
int p2l_page_size;      // entry in a page


enum {
    D_CMD_READ = 0,
    D_CMD_WRITE,
    D_CMD_ERASE
};

// get flash info
// pAddr : CH/CE/BLK/PLANE/PAGE
// lAddr : CH/CE/BLK/PAGE1/PLANE/PAGE0, PAGE0:1024 entry, 10bits
#define D_GET_CH_ADDR(x)        ((x >> ch_sft_cnt) & (ch_bit_num - 1))
#define D_GET_BLOCK_ADDR(x)     ((x >> blk_sft_cnt) & (blk_bit_num - 1))
#define D_GET_PLANE_ADDR(x)     ((x >> plane_sft_cnt) & (plane_bit_num - 1))
#define D_GET_PAGE_ADDR(x)      ((x >> page_sft_cnt) & (page_bit_num - 1))

#define D_CHK_P2L_BITMAP(x)     (pP2lBitmap[x / 32] & (1 << (x % 32)))


/*
    Translate to LBA info from phisical address

    Notice : it will cause unexpected failure is the p2l is not in dram,
             so make sure the p2l page already swap in dram
*/
int iGetLbnInfo(int pAddr){
    int ch = D_GET_CH_ADDR(pAddr);
    
    pAddr = pAddr & (~(ch << ch_sft_cnt));

    return pP2lTable[ch][pAddr % p2l_bank_size];
}


/*
    P2L main procedure

    return value:
    LBA information use to data read/write
*/
int iGetDataLbn(int pAddr){
    int lbn = 0;

    p2l_chk_cnt++;

    if(D_CHK_P2L_BITMAP(pAddr)){   // p2l page hit
        // get the lba from p2l
        lbn =  iGetLbnInfo(pAddr);
        p2l_hit_cnt++;
    }
    else{   // p2l page miss
        if(iSwapP2lPage(pAddr) == 1){
           // printf("Swap p2l fail, assert\n");
            while(1);
        }
    }

    return lbn;
}


/*
    Swap P2L page
*/
int iSwapP2lPage(int pAddr){
    int ch = D_GET_CH_ADDR(pAddr);
    int lbn = 0;
    int addr = 0;
    int pageOff = 0;
    int *pSrc, *pDes;

    // store the p2l set to p2l src table
    addr = pAddr & (~(ch << ch_sft_cnt));
    pageOff = (addr % p2l_bank_size) / 1024;

    lbn = pP2lPageLbn[ch][pageOff];
    pSrc = &pP2lTable[ch][pageOff * 1024];
    pDes = &pP2lSrcTable[lbn * 4096];

    memcpy(pDes, pSrc, 4096);

    // read the new p2l set from the src table to dram
    lbn = pAddr / 1024;
    pP2lPageLbn[ch][pageOff] = lbn;
    pSrc = &pP2lSrcTable[lbn * 4096];
    pDes = &pP2lTable[ch][pageOff * 1024];

    memcpy(pDes, pSrc, 4096);

    // set p2l bitmap table
    for (int idx = 0; idx < (1024/32); idx ++) {
        pP2lBitmap[idx + pAddr/] = 0xFFFFFFFF;
    }

    return 0;
}


/*
    Allocate new lbn for write
*/
int iAllocLBN(int pAddr) {


}


/*
    Init device config
    
    Parameter:
    dev_cap : device capacity in GB 
    ddr_size : ddr size in MB
*/
int iInitDevConfig(int devCap, int ddrSize) {
    ch_cnt = 8;
    page_cnt = 1024;    // TBD
    plane_cnt = 4;      // TBD
    blk_cnt = (devCap * 1024 * 1024) / (16 * page_cnt * plane_cnt * ch_cnt);

    ch_bit_num = (int)log2(ch_cnt);
    blk_bit_num = (int)log2(blk_cnt);
    page_bit_num = (int)log2(page_cnt);
    plane_bit_num = (int)log2(plane_cnt);

    page_sft_cnt = 0;
    plane_sft_cnt = page_sft_cnt + page_bit_num;
    blk_sft_cnt = plane_sft_cnt + plane_bit_num;
    ch_sft_cnt = blk_sft_cnt + blk_bit_num;

    rest_lbn_cnt = lbn_cnt = (devCap * 1024 * 1024) / 4;   // total device lbn count

    // allocate p2l ddr table 
    int bitMapSize = ((devCap * 1024 * 1024) / 16) / 8;
    int payloadSize = sizeof(int) * ((devCap * 1024 * 1024) / 16);
    int p2lEntryCnt = ((devCap * 1024 * 1024) / 16);

    p2l_page_size = 1024;
    p2l_bank_size = (ddrSize * 1024 * 1024) / (ch_cnt * sizeof(int));
    rest_lbn_cnt = rest_lbn_cnt - (p2l_bank_size / 1024);   // minus p2l table size

    // allocate p2l bitmap memory
    pP2lBitmap = (int *)malloc(bitMapSize);

    // allocate p2l table memory
    for (int idx = 0; idx < ch_cnt; idx++) {
        pP2lTable[idx] = (int *)malloc(sizeof(int) * p2l_bank_size);
        pP2lPageLbn[idx] = (int*)malloc(p2l_bank_size / 4096);
    }
    
    pP2lSrcTable = (int*)malloc(sizeof(int) * p2lEntryCnt);

    // allocate data payload buffer
    pDataPayload = (int*)malloc(payloadSize);

    p2l_hit_cnt = p2l_chk_cnt = 0;

    return 0;
}


int iFlashCmdHandler(int cmd, int pAddr, int *pPayload){
    int lbn = 0;

    switch (cmd)
    {
    case D_CMD_WRITE:
        lbn = iGetDataLbn(pAddr);
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