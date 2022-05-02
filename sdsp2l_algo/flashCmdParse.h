#ifndef DEF_FLASH_CMD_PARSE
#define DEF_FLASH_CMD_PARSE
#ifdef __cplusplus
extern "C" {
#endif
#include "pch.h"

typedef enum e_cmd_sts {
	E_CMD_BEGINE,
	E_CMD_MIDDLE,
	E_CMD_COMPLETE,
}t_cmd_sts;

typedef struct s_fsh_node {
	U8 uOp;		// op code
	U8 uCmp;	// cmd seq complete
	U8 uFunc;	// callback function op index

	struct s_fsh_node* pAdjNd;		// point to adjecent node
	struct s_fsh_node* pSubNd;		// point to child node

}t_fsh_node;

typedef struct s_fsh_cmd_head {
	U8 uCmdSeqCnt;				// total command seq count
	t_cmd_sts uCmdState;		// current command state {begine, middle, end}
	
	t_fsh_node* pHeadPtr;		// point to first node
	t_fsh_node* pCurHeadNd;		// current parsing cmd root node
	t_fsh_node* pCurNode;		// current parsing cmd node

}t_fsh_cmd_head;

#ifdef __cplusplus
}
#endif
#endif