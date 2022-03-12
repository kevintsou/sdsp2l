#ifndef DEF_P2L
#define DEF_P2L
#ifdef __cplusplus
extern "C" {
#endif

	#define D_MAX_CE_CNT 1
	#define D_MAX_CH_CNT 8
	
	extern long p2l_hit_cnt;
	extern long p2l_chk_cnt;
	extern int iSwapP2lPage(int pAddr);

#ifdef __cplusplus
}
#endif
#endif