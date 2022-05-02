#ifndef DEF_P2L
#define DEF_P2L
#ifdef __cplusplus
extern "C" {
#endif

	#define D_MAX_CE_CNT 1
	#define D_MAX_CH_CNT 8
	#define D_MAX_PLANE_CNT	8
	#define D_MAX_PAGE_CNT 4096

	enum {
		E_CMD_READ = 0,
		E_CMD_WRITE,
		E_CMD_ERASE
	};

	typedef struct s_p2l_mgr {
		int hitCnt;
		int chkCnt;
		int entryPerBank;       // entry in a bank
		int pagePerBank;		// page in a bank
		int entryPerPage;       // entry in a page
		int tableSize;
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
		int lbnQdepth;			// lbn queue depth
		int* pLbnBuff;          // lbn buffer
	

		// lbn info current use
		int *pBlk2Lbn[D_MAX_CH_CNT];          // block to lbn lookup table
		int *pChRestLbnNum[D_MAX_CH_CNT];   // available lbn count for this block
		int *pChAllocLbn[D_MAX_CH_CNT];   // current allocated lbn for the write cmd for each channel
	}t_lbn_mgr;

	typedef struct s_file_mgr {
		int fd_write;
		int fd_read;
		int blk_lbn_per_plane_rd;
		int blk_lbn_per_plane_wr;
	}t_file_mgr;

	extern t_p2l_mgr p2l_mgr;
	extern t_lbn_mgr lbn_mgr;
	extern t_dev_mgr dev_mgr;
	extern t_file_mgr file_mgr;

	extern int iSwapP2lPage(int pAddr);
	extern int iInitDevConfig(int devCap, int ddrSize, int chCnt, int planeCnt, int pageCnt, int* bufPtr);
	extern int iFlashCmdHandler(int cmd, int ch, int blk, int plane, int page, int* pPayload);
	extern int iIssueFlashCmdpAddr(int cmd, int pAddr, int* pPayload);


#ifdef __cplusplus
}
#endif
#endif