#ifndef DEF_FLASH_CMD_SET
#define DEF_FLASH_CMD_SET
#ifdef __cplusplus
extern "C" {
#endif
#include "pch.h"

// Flash command prefix token
#define TADDR						(0xAB)
#define TCMD						(0xC2)
#define TRDKB						(0xDA)
#define TRDB						(0xDB)
#define TWRD						(0xDC)
#define TFUNC						(0xEE)		// TBD
#define TEOC						(0xEC)		// TBD
	
// Flash cmd seq function idx 
#define D_FUNC_N_WRITE				(0x01)
#define D_FUNC_CACHE_WR				(0x02)
#define D_FUNC_N_READ				(0x20)
#define D_FUNC_CACHE_RD				(0x21)
#define D_FUNC_N_ERASE				(0x60)


extern int flashCmdSet[];

#ifdef __cplusplus
}
#endif
#endif#pragma once
