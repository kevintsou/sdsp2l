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
	TCMD, 0x80, TADDR, TADDR, TADDR, TADDR, TADDR, TWRD, TCMD, 0x10, TEOC,
	TCMD, 0x80, TADDR, TADDR, TADDR, TADDR, TADDR, TWRD, TCMD, 0x11, TEOC,	// multi-plane
	TCMD, 0x80, TADDR, TADDR, TADDR, TADDR, TADDR, TWRD, TCMD, 0x15, TEOC,	// cache program
	// read cmd
	TCMD, 0x00, TADDR, TADDR, TADDR, TADDR, TADDR, TCMD, 0x30, TEOC,
	// erase cmd
	TCMD, 0x60, TADDR, TADDR, TADDR, TCMD, 0xD0, TEOC,
	TCMD, 0x60, TADDR, TADDR, TADDR, TCMD, 0xD1, TEOC,	// multi plane erase
	TCMD, 0x60, TADDR, TADDR, TADDR, TCMD, 0x60, TADDR, TADDR, TADDR, 0xD0, TEOC, // multi plane erase
#endif
};