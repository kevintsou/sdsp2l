#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "flashCmdParse.h"
#include "flashCmdSet.h"

t_fsh_cmd_head sFlashCmdInfo;

int iParseFlashCmd() {


	return 0;
}


int iInsertNode(t_fsh_node *pSrcFshNd, t_fsh_node* pInsFshNd, U8 uNdType, U8 uDir) {

	switch (uDir) {
		case E_INS_ADJECNET:
		
			break;
		case E_INS_CHILD:


			break;
		default:
			break;
	}

	return 0;
}


t_fsh_node* iSearchNode(t_fsh_node* pFshNd,  U8 uOp) {
	t_fsh_node* pFindNd = (t_fsh_node *)NULL;


	if (pFindNd != NULL) {
		sFlashCmdInfo.pCurHeadNd = pFindNd;
		sFlashCmdInfo.pCurNode = pFindNd;
	}
	return pFindNd;
}


int iAllocCmdNode(t_fsh_node* pFshNd) {
	// debug code
	while (!sFlashCmdInfo.u32MemRest);

	pFshNd = sFlashCmdInfo.pAllocPtr;

	if (pFshNd == NULL) {
		return -1;	// memory space allocate error
	}

	sFlashCmdInfo.pAllocPtr++;
	sFlashCmdInfo.u32MemRest--;
	sFlashCmdInfo.pCurHeadNd = pFshNd;
	sFlashCmdInfo.pCurNode = pFshNd;
	
	return 0;
	
}

/*
	create sFlashCmdHead and flash node linked list
*/
int iInitFlashCmdSeq() {
	int idx = 0;
	t_fsh_node *pFshNd, *pFindNd;

	memset(&sFlashCmdInfo, 0, sizeof(sFlashCmdInfo));
	sFlashCmdInfo.uNew = 1;

	// allocate node space
	sFlashCmdInfo.u32MemRest = 1024; // TBD size
	sFlashCmdInfo.pAllocPtr = (t_fsh_node*)malloc(sizeof(t_fsh_node) * sFlashCmdInfo.u32MemRest);

	while (flashCmdSet[idx] != 0x00) {	// if not reach end of array

		switch (flashCmdSet[idx]) {
		case TCMD:	// 2 
			idx++;
			if (sFlashCmdInfo.uNew) {	// find if the cmd seq exist
				pFindNd = iSearchNode(sFlashCmdInfo.pHeadPtr, flashCmdSet[idx]);

				// find same cmd seq 
				if (pFindNd == NULL) {
					iAllocCmdNode(pFshNd);
					pFshNd->uOp = flashCmdSet[idx];		// flash cmd op

					iInsertNode(sFlashCmdInfo.pLastPtr, pFshNd, TCMD, E_INS_ADJECNET);

					// if the list is empty, assign current flash node as the head
					if (!sFlashCmdInfo.uCmdSeqCnt) {
						sFlashCmdInfo.pHeadPtr = pFshNd;
					}
					sFlashCmdInfo.pLastPtr = pFshNd;
					
					// add in EOC?? TBD
					//sFlashCmdInfo.uCmdSeqCnt++;
				}
			}
			else {
				// check if child node exist
				if (sFlashCmdInfo.pCurNode->pSubNd != NULL) {
					pFindNd = iSearchNode(sFlashCmdInfo.pCurNode->pSubNd, flashCmdSet[idx]);
					
					// find same cmd seq 
					if (pFindNd == NULL) {
						iAllocCmdNode(pFshNd);
						pFshNd->uOp = flashCmdSet[idx];		// flash cmd op		
						
						iInsertNode(sFlashCmdInfo.pCurNode->pSubNd, pFshNd, TCMD, E_INS_ADJECNET);
					}
				}
				else {
					iAllocCmdNode(pFshNd);
					pFshNd->uOp = flashCmdSet[idx];		// flash cmd op			

					iInsertNode(sFlashCmdInfo.pCurNode, pFshNd, TCMD, E_INS_CHILD);
				}
			}

			idx++;
			break;
		case TADDR:	
			break;
		case TRDKB:	
			break;
		case TRDB:
			break;
		case TWRD:
			break;
		case TFUNC:
			break;
		case TEOC:

			sFlashCmdInfo.uNew = 1;
			break;
		}
	}

	return 0;
}

