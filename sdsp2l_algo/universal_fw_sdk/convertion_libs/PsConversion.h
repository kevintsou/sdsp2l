#pragma once

class NPsConversion
{
public:
	NPsConversion(void);
	~NPsConversion(void);
public:
	unsigned long seedinit;
	unsigned long seed_rotate( unsigned long crc_seed , unsigned long rot_no , unsigned long poly_ord );
	unsigned long seed_selector4( unsigned long Cell_Length , unsigned long reserve1 , unsigned long reserve2,int ed3Con=1,int FramePerPage=8, int mode=0, bool isE13=1 );
	unsigned long crc_prsn_t2(int *poly,int ploy_length,int seed_rot,int ByteFame,unsigned char *out_ary);
	unsigned long crc_prsn_t2_v1(int *poly,int ploy_length,int seed_rot,int ByteFame,unsigned char *out_ary);
	unsigned long make_frame_bit(int startaddr,int Spacing,unsigned char *frame_bs,int bs_len,unsigned char* source,int source_len);	
	unsigned long MakeConv( unsigned char *sourceBuffer , unsigned char *FinalBuff ,int ConvSize );
	void SetBlockSeed( unsigned int BlockSeed, bool needShift );


private:
	unsigned char *_block_rdm_bs;
};

class PsConversion
{
public:
	PsConversion(void);
	~PsConversion(void);
public:
	 int FramePerPage;
	 unsigned long seed_rotate( unsigned long crc_seed , unsigned long rot_no , unsigned long poly_ord );
	 unsigned long seed_selector( unsigned long crc_seed , unsigned long rot_no , unsigned long poly_ord );
	 unsigned long seed_selector2( unsigned long Cell_Length , unsigned long reserve1 , unsigned long reserve2 );
	 unsigned long seed_selector3( unsigned long Cell_Length , unsigned long reserve1 , unsigned long reserve2 );
	 unsigned long seed_selector4( unsigned long Cell_Length , unsigned long reserve1 , unsigned long reserve2,int ed3Con=1 );
	 unsigned long crc_prsn_t2(int *poly,int ploy_length,int seed_rot,int ByteFame,unsigned char *out_ary);
	 unsigned long crc_prsn_t2_v1(int *poly,int ploy_length,int seed_rot,int ByteFame,unsigned char *out_ary);
	 unsigned long make_frame_bit(int startaddr,int Spacing,unsigned char *frame_bs,int bs_len,unsigned char* source,int source_len);	
	 unsigned long MakeConv( unsigned char *sourceBuffer , unsigned char *FinalBuff ,int ConvSize );

private:
	unsigned char *_block_rdm_bs;
};