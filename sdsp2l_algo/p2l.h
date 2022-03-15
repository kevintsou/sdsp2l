#ifndef DEF_P2L
#define DEF_P2L
#ifdef __cplusplus
extern "C" {
#endif

	#define D_MAX_CE_CNT 1
	#define D_MAX_CH_CNT 8
	#define D_MAX_PLANE_CNT	8
	#define D_MAX_PAGE_CNT 4096

	typedef struct s_p2l_mgr {
		long hitCnt;
		long chkCnt;
		int bankSize;      // entry in a bank
		int pageSize;      // entry in a page
	}t_p2l_mgr;
	
	// device configuration
	typedef struct s_dev_mgr {
		int dev_cap;
		int ddr_size;
		
		int chCnt;
		int blkCnt;	// die block count
		int pageCnt;
		int planeCnt;

		int chBitNum;
		int blkBitNum;
		int pageBitNum;
		int planeBitNum;

		int chSftCnt;
		int blkSftCnt;
		int pageSftCnt;
		int planeSftCnt;
	}t_dev_mgr;

	// LBN manager
	typedef struct s_lbn_mgr {
		int lbnEntryCnt;    // max entries in this lbn buffer
		// lbn buffer header
		int headPtr;			// get lbn ptr
		int tailPtr;			// return lbn ptr
		int lbnEntryPerBlk;     // ibn buf entry cnt
		int availLbnCnt;        // available lbn cnt
		int* pLbnBuff;          // lbn buffer
		int* pBlk2Lbn;          // block to lbn lookup table

		// lbn info current use
		int chStartLbn[D_MAX_CH_CNT];   // ch lbn queue for queue current lbn start value    
		int chAllocLbn[D_MAX_CH_CNT];   // current allocated lbn for the write cmd for each channel
	}t_lbn_mgr;

	extern t_p2l_mgr p2l_mgr;
	extern t_lbn_mgr lbn_mgr;
	extern t_dev_mgr dev_mgr;

	extern int iSwapP2lPage(int pAddr);
	extern int iInitDevConfig(int devCap, int ddrSize, int chCnt, int planeCnt, int pageCnt, int* bufPtr);
	extern int iFlashCmdHandler(int cmd, int ch, int blk, int plane, int page, int* pPayload);



#ifdef __cplusplus
}
#endif
#endif