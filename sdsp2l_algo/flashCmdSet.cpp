#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "flashCmdParse.h"
#include "flashCmdSet.h"

#define MI_FLASH	1	

int flashCmdSet[] = {
#if (MI_FLASH == 1)
	// prefix cmd
	TCMD, 0xDA, TEOC,	// slc en
	TCMD, 0xDF, TEOC,	// slc dis
	// program cmd
	TCMD, 0x80, TADDR, TADDR, TADDR, TADDR, TADDR, TWRD, TCMD, 0x10, TFUNC, D_FUNC_N_WRITE, TEOC,
	TCMD, 0x80, TADDR, TADDR, TADDR, TADDR, TADDR, TWRD, TCMD, 0x11, TFUNC, D_FUNC_N_WRITE, TEOC,	// multi-plane
	TCMD, 0x80, TADDR, TADDR, TADDR, TADDR, TADDR, TWRD, TCMD, 0x15, TFUNC, D_FUNC_CACHE_WR, TEOC,	// cache program
	// read cmd
	TCMD, 0x00, TADDR, TADDR, TADDR, TADDR, TADDR, TCMD, 0x30, TFUNC, D_FUNC_N_READ, TEOC,
	// erase cmd
	TCMD, 0x60, TADDR, TADDR, TADDR, TCMD, 0xD0, TFUNC, D_FUNC_N_ERASE, TEOC,
	TCMD, 0x60, TADDR, TADDR, TADDR, TCMD, 0xD1, TFUNC, D_FUNC_N_ERASE, TEOC,	// multi plane erase
	TCMD, 0x60, TADDR, TADDR, TADDR, TCMD, 0x60, TADDR, TADDR, TADDR, 0xD0, TFUNC, D_FUNC_N_ERASE, TEOC, // multi plane erase
	
	0x00,	// end of array
#endif
};