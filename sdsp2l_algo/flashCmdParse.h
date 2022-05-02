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

typedef enum e_node_type {
	E_NODE_ROOT,
	E_NODE_INTERNAL,
	E_NODE_END,
}t_node_type;

typedef enum e_ins_type {
	E_INS_ADJECNET,
	E_INS_CHILD,
}t_ins_type;

typedef struct s_fsh_node {
	U8 uOp;		// op code
	U8 uCmp;	// cmd seq complete
	U8 uFunc;	// callback function op index
	t_node_type uNdType; // node type : {root, internal, end}

	struct s_fsh_node* pAdjNd;		// point to adjecent node
	struct s_fsh_node* pSubNd;		// point to child node

}t_fsh_node;

typedef struct s_fsh_cmd_head {
	U8 uCmdSeqCnt;				// total command seq count
	U8 uNew;					// new cmd seq in
	t_cmd_sts uCmdState;		// current command state {begine, middle, end}
	
	t_fsh_node* pHeadPtr;		// point to first root node
	t_fsh_node* pLastPtr;		// point to last root node
	t_fsh_node* pCurHeadNd;		// current parsing cmd root node
	t_fsh_node* pCurNode;		// current parsing cmd node

	U32	u32MemRest;				// rest memory space for node
	t_fsh_node* pAllocPtr;		// node space allocate ptr
}t_fsh_cmd_head;

extern t_fsh_cmd_head sFlashCmdInfo;

#ifdef __cplusplus
}
#endif
#endif