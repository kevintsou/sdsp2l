#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdbool.h>
#include <stdlib.h>

#include "interface.h"
#include "p2l.h"
#include "data.h"

long lp2lHitCnt() {
	return p2l_mgr.hitCnt;
}

long lpl2ChkCnt() {
	return p2l_mgr.chkCnt;
}
