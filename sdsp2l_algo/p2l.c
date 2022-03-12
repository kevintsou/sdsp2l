#include "p2l.h"
#include "data.h"
#include <math.h>

int* pP2lBitmap;    // P2L Bitmap
int* pP2lTable;     // P2L Table 

// device configuration
int ce_cnt;
int ch_cnt;
int blk_cnt;
int page_cnt;
int plane_cnt;

int ce_bit_num;
int ch_bit_num;
int blk_bit_num;
int page_bit_num;
int plane_bit_num;

int ce_sft_cnt;
int ch_sft_cnt;
int blk_sft_cnt;
int page_sft_cnt;
int plane_sft_cnt;


// get flash info
// pAddr : CH/CE/BLK/PLANE/PAGE
// lAddr : CH/CE/BLK/PAGE1/PLANE/PAGE0, PAGE0:1024 entry, 10bits
#define D_GET_CH_ADDR(x)        ((x >> ch_sft_cnt) & (ch_bit_num - 1))
#define D_GET_CE_ADDR(x)        ((x >> ce_sft_cnt) & (1))
#define D_GET_BLOCK_ADDR(x)     ((x >> blk_sft_cnt) & (blk_bit_num - 1))
#define D_GET_PLANE_ADDR(x)     ((x >> plane_sft_cnt) & (plane_bit_num - 1))
#define D_GET_PAGE_ADDR(x)      ((x >> page_sft_cnt) & (page_bit_num - 1))


/*
    Check p2l bitmap

    return value:
    1 : hit
    0 : miss
*/
int iChkP2lBitmap(int pAddr){

    return 0;
}


/*
    Translate to LBA info from phisical address

    Notice : it will cause unexpected failure is the p2l is not in dram,
             so make sure the p2l page already swap in dram
*/
int iGetLbaInfo(int pAddr){

    return 0;
}


/*
    P2L main procedure

    return value:
    LBA information use to data read/write
*/
int iGetDataLba(int pAddr){
    int ch, ce, plane, blk, page;
    int lba = 0;

    if(iChkP2lBitmap(pAddr)){   // p2l page hit
        // get the lba from p2l
        lba =  iGetLbaInfo(pAddr);
    }
    else{   // p2l page miss
        if(iSwapP2lPage(pAddr) == 1){
           // printf("Swap p2l fail, assert\n");
            while(1);
        }
    }

    return lba;
}


/*
    Swap P2L page
*/
int iSwapP2lPage(int pAddr){

    return 0;
}


int iInitDevConfig(){
    ce_cnt = 1;
    ch_cnt = 8;
    blk_cnt = 128;  // TBD
    page_cnt = 1024;    // TBD
    plane_cnt = 4;

    ce_bit_num = (int)log2(ce_cnt);
    ch_bit_num = (int)log2(ch_cnt);
    blk_bit_num = (int)log2(blk_cnt);
    page_bit_num = (int)log2(page_cnt);
    plane_bit_num = (int)log2(plane_cnt);

    page_sft_cnt = 0;
    plane_sft_cnt = page_sft_cnt + page_bit_num;
    blk_sft_cnt = plane_sft_cnt + plane_bit_num;
    ch_sft_cnt = blk_sft_cnt + blk_bit_num;
    return 0;
}


int iRwCmdHandler(int cmd, int pAddr, int *pPayload){
    int lba = 0;

    lba = iGetDataLba(pAddr);

    if(cmd == 0){   // read cmd
        
    }
    else{   // write cmd

    }
    return 0;
}

/*
    main funciton
*/
int sdsp2l_main(){

    iInitDevConfig();


    return 0;
}