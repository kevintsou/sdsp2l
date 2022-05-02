//#include "StdAfx.h"
#include "PsConversion.h"
#include <iostream>  
#include <math.h> 
#include <assert.h>

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef ULONG
#define ULONG unsigned long
#endif

#ifndef ASSERT
#define ASSERT assert
#endif
//#if _DEBUG
//#include "vld.h"
//#endif
unsigned long RSeed1[1];
unsigned long RSeed[64]={	0x00056c,0x000fbc,0x001717,0x000a52,0x0007cf,0x00064e,0x00098c,0x001399,0x001246,0x000150,0x0011f1,0x001026,0x001c45,0x00012a,0x000849,0x001d4c,
							0x01bc77,0x00090c,0x01e1ad,0x00d564,0x01cb3b,0x0167a7,0x01bd54,0x011690,0x014794,0x000787,0x017bad,0x0173ab,0x008b63,0x003a3a,0x01cb9b,0x00af6e,
							0x05d356,0x07f880,0x06db67,0x06fbac,0x037cd1,0x00f1d2,0x02c2af,0x01d310,0x00f34f,0x073450,0x046eaa,0x079634,0x042924,0x0435d5,0x028350,0x00949e,
							0x1f645d,0x3d9e86,0x7d7ea0,0x6823dd,0x5c91f0,0x506be8,0x4b5fb7,0x27e53b,0x347bc4,0x3d8927,0x5403f5,0x01b987,0x4bf708,0x1a7baa,0x1e8309,0x00193a};

NPsConversion::NPsConversion(void)
{
	_block_rdm_bs = NULL;
	seedinit = 0;
}

NPsConversion::~NPsConversion(void)
{
	if(_block_rdm_bs != NULL)
		delete []_block_rdm_bs;
	_block_rdm_bs = NULL;
}
unsigned long NPsConversion::MakeConv( unsigned char *sourceBuffer , unsigned char *FinalBuff  ,int ConvSizeByte)
{

	int blockBit = ConvSizeByte * 8; //change to bit
	//unsigned long blockBit = BytePerFrame*8;
	for(int i=0;i<blockBit;i+=8){
		unsigned int RdmSeed = 0;
		int shiftbit = 7;
		for(int n=i;n<i+8;n++){
			if(_block_rdm_bs[n]) RdmSeed |= 0x01 << shiftbit;
			shiftbit--;
		}
		FinalBuff[i/8] = sourceBuffer[i/8] ^ RdmSeed;
	}
	return 0;
}

unsigned long NPsConversion::seed_selector4( unsigned long Cell_Length , unsigned long reserve1 , unsigned long page ,int ed3Con,int FramePerPage, int mode, bool isE13 )
{
	/*
	parameter = [2, 73728,16,86*3];*/
	int Cell_Type = 2;
	//int Cell_Length = 33554432;//73728;
//	int FramePerPage = 8;
	int PagesPerBlock = 1;
	int BytePerFrame = Cell_Length/FramePerPage/8;
	int poly13[] = {0 ,1 ,3 ,4 ,13};  
	int poly17[] = {0 ,3 ,17};
	int poly19[] = {0 ,1 ,2 ,5 ,19};
	int poly23[] = {0 ,5 ,23};//length_poly23 = 3;
	//unsigned long seedinit = (0x8299<<22)+0x8299;	// E13 seed init = (seed[9..0]<<22)+seed[21..0]
	unsigned char *b7_tmp = new unsigned char[BytePerFrame];
	unsigned char *b6_tmp = new unsigned char[BytePerFrame];
	unsigned char *b5_tmp = new unsigned char[BytePerFrame];
	unsigned char *b4_tmp = new unsigned char[BytePerFrame];
	unsigned char *b3_tmp = new unsigned char[BytePerFrame];
	unsigned char *b2_tmp = new unsigned char[BytePerFrame];
	unsigned char *b1_tmp = new unsigned char[BytePerFrame];
	unsigned char *b0_tmp = new unsigned char[BytePerFrame];
	unsigned char *frame_rdm_bs = new unsigned char[BytePerFrame*8];
	unsigned long PageBit = FramePerPage*BytePerFrame*8;
	unsigned char *pg_rdm_bs_tmp = new unsigned char[PageBit];
	unsigned long blockBit = PagesPerBlock*PageBit;
	if(_block_rdm_bs == NULL)
		_block_rdm_bs = new unsigned char[blockBit];
	memset(frame_rdm_bs,0,BytePerFrame*8);
	memset(pg_rdm_bs_tmp,0,FramePerPage*BytePerFrame*8);
	memset(b0_tmp,0,BytePerFrame);
	memset(b1_tmp,0,BytePerFrame);
	memset(b2_tmp,0,BytePerFrame);
	memset(b3_tmp,0,BytePerFrame);
	memset(b4_tmp,0,BytePerFrame);
	memset(b5_tmp,0,BytePerFrame);
	memset(b6_tmp,0,BytePerFrame);
	memset(b7_tmp,0,BytePerFrame);
	



	int pg_seed_b7_4[16] = {0};
	int pg_seed_b3_0[16] = {0};
	int length_poly23 = 3;
	int length_poly19 = 5;
	int length_poly17 = 3;
	int length_poly13 = 5;
	int i;
	for(unsigned long pg_no=page;pg_no<page+1;pg_no++){
		unsigned long pg_no_b3_0 = pg_no % 16;//mod(pg_no-1,16);
		unsigned long pg_no_b7_4 = (pg_no)/16;
		if ( mode==0 ) 
        {
			for (i=0;i<16;i++){
				pg_seed_b7_4[i] =  ((int)(pg_no_b3_0 + i ) % 16);
			}
			for (i=0;i<16;i++){
				pg_seed_b3_0[i] =  ((int)(pg_seed_b7_4[i]+1) % 16);
				//pg_seed_b3_0[i] =  ((int)(pg_no_b3_0 + i+1 ) % 16);
			}
		}
		else if ( mode==1 ) {
			for (i=0;i<16;i++){
				if ( (i&1)==0) {
					pg_seed_b3_0[i] =  ((int)(pg_seed_b7_4[i]) % 16);
					pg_seed_b7_4[i] =  ((int)(pg_seed_b7_4[i]) % 16);
				}
				else {
					pg_seed_b3_0[i] =  ((int)(pg_seed_b7_4[i]+7) % 16);
					pg_seed_b7_4[i] =  ((int)(pg_seed_b7_4[i]+7) % 16);
				}
			}		
		}
		else if ( mode==2 ) 
		{
			for (i=0;i<16;i++){
				pg_seed_b7_4[i] =  ((int)(pg_no_b3_0 + i ) % 16);
			}
			for (i=0;i<16;i++){
				pg_seed_b3_0[i] =  ((int)(pg_seed_b7_4[i]+7) % 16);
			}
		}
		else if ( mode==3 ) 
		{
			for (i=0;i<16;i++){
				pg_seed_b7_4[i] =  ((int)(pg_no_b3_0 + i ) % 16);
			}
			for (i=0;i<16;i++){
				pg_seed_b3_0[i] =  ((int)(pg_seed_b7_4[i]+7) % 16);
			}
		}
        

		for(int frame_no=0;frame_no<FramePerPage;frame_no++){
			unsigned long b4_seed_ori = 0;
			unsigned long b4_seed_rot_tmp1=0;
			//b7_seed_ori=RSeed[3*16+(int)pg_seed_b7_4[frame_no]];
			b4_seed_ori=RSeed[3*16+(int)pg_seed_b7_4[frame_no % 16]];//bit 4
			b4_seed_rot_tmp1 =seed_rotate(b4_seed_ori,pg_no_b7_4,poly23[length_poly23-1]); //13
			/************************************************************************/
			/* //b7_seed_rot_tmp2 = mod( + bitxor(hex2dec(b7_seed_rot_tmp1), seedinit),2^23);                                                                     */
			/************************************************************************/
			b4_seed_rot_tmp1 = b4_seed_rot_tmp1 ^ seedinit; //xor
			//b4_seed_rot_tmp1 = b4_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			unsigned long b4_seed_rot  = b4_seed_rot_tmp1;
			
			//crc_prsn_t2_v1(poly13,length_poly13,b7_seed_rot,BytePerFrame,b7_tmp);
			crc_prsn_t2(poly23,length_poly23,b4_seed_rot,BytePerFrame,b4_tmp);
			//-------b6--------------
			unsigned long b5_seed_ori = 0;
			unsigned long b5_seed_rot_tmp1=0; //b5
			b5_seed_ori=RSeed[2*16 +(int)pg_seed_b7_4[frame_no % 16]];
			b5_seed_rot_tmp1=seed_rotate(b5_seed_ori,pg_no_b7_4,poly19[length_poly19-1]); 
			b5_seed_rot_tmp1 = b5_seed_rot_tmp1 ^ seedinit; //xor
			//b5_seed_rot_tmp1 = b5_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			unsigned long b5_seed_rot  = b5_seed_rot_tmp1;
			
			//crc_prsn_t2_v1(poly17,length_poly17,b6_seed_rot,BytePerFrame,b6_tmp);
			crc_prsn_t2(poly19,length_poly19,b5_seed_rot,BytePerFrame,b5_tmp);
			//-------b5--------------
			unsigned long b6_seed_ori = 0;
			unsigned long b6_seed_rot_tmp1=0; //b6
			b6_seed_ori=RSeed[1*16 +(int)pg_seed_b7_4[frame_no % 16]];
			b6_seed_rot_tmp1=seed_rotate(b6_seed_ori,pg_no_b7_4,poly17[length_poly17-1]); 
			b6_seed_rot_tmp1 = b6_seed_rot_tmp1 ^ seedinit; //xor
			//b6_seed_rot_tmp1 = b6_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			unsigned long b6_seed_rot  = b6_seed_rot_tmp1;
			
			crc_prsn_t2(poly17,length_poly17,b6_seed_rot,BytePerFrame,b6_tmp);
			//-------b4--------------
			unsigned long b7_seed_ori = 0;
			unsigned long b7_seed_rot_tmp1=0; //b7
			b7_seed_ori=RSeed[0*16 +(int)pg_seed_b7_4[frame_no % 16]];
			b7_seed_rot_tmp1=seed_rotate(b7_seed_ori,pg_no_b7_4,poly13[length_poly13-1]); 
			b7_seed_rot_tmp1 = b7_seed_rot_tmp1 ^ seedinit; //xor
			//b7_seed_rot_tmp1 = b7_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			unsigned long b7_seed_rot  = b7_seed_rot_tmp1;
			
			crc_prsn_t2(poly13,length_poly13,b7_seed_rot,BytePerFrame,b7_tmp);

			//-------b3--------------
			unsigned long b0_seed_ori = 0;
			unsigned long b0_seed_rot_tmp1=0;
			b0_seed_ori=RSeed[3*16 +(int)pg_seed_b3_0[frame_no % 16]];
			b0_seed_rot_tmp1=seed_rotate(b0_seed_ori,pg_no_b7_4,poly23[length_poly23-1]); 
			b0_seed_rot_tmp1 = b0_seed_rot_tmp1 ^ seedinit; //xor
			//b3_seed_rot_tmp1 = b3_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			unsigned long b0_seed_rot  = b0_seed_rot_tmp1;
			crc_prsn_t2(poly23,length_poly23,b0_seed_rot,BytePerFrame,b0_tmp);

			//-------b2--------------
			unsigned long b1_seed_ori = 0;
			unsigned long b1_seed_rot_tmp1=0;
			b1_seed_ori=RSeed[2*16 +(int)pg_seed_b3_0[frame_no % 16]];
			b1_seed_rot_tmp1=seed_rotate(b1_seed_ori,pg_no_b7_4,poly19[length_poly19-1]); 
			b1_seed_rot_tmp1 = b1_seed_rot_tmp1 ^ seedinit; //xor
			//b2_seed_rot_tmp1 = b2_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			unsigned long b1_seed_rot  = b1_seed_rot_tmp1;
			crc_prsn_t2(poly19,length_poly19,b1_seed_rot,BytePerFrame,b1_tmp);
			//-------b1--------------
			unsigned long b2_seed_ori = 0;
			unsigned long b2_seed_rot_tmp1=0;
			b2_seed_ori=RSeed[1*16 +(int)pg_seed_b3_0[frame_no % 16]];
			b2_seed_rot_tmp1=seed_rotate(b2_seed_ori,pg_no_b7_4,poly17[length_poly17-1]); //17
			b2_seed_rot_tmp1 = b2_seed_rot_tmp1 ^ seedinit; //xor
			//b1_seed_rot_tmp1 = b1_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			unsigned long b2_seed_rot  = b2_seed_rot_tmp1;
			crc_prsn_t2(poly17,length_poly17,b2_seed_rot,BytePerFrame,b2_tmp);
			//-------b0--------------
			unsigned long b3_seed_ori = 0;
			unsigned long b3_seed_rot_tmp1=0;
			b3_seed_ori=RSeed[0*16 +(int)pg_seed_b3_0[frame_no % 16]];
			b3_seed_rot_tmp1=seed_rotate(b3_seed_ori,pg_no_b7_4,poly13[length_poly13-1]); //13
			b3_seed_rot_tmp1 = b3_seed_rot_tmp1 ^ seedinit; //xor
			//b0_seed_rot_tmp1 = b0_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			unsigned long b3_seed_rot  = b3_seed_rot_tmp1;
			crc_prsn_t2(poly13,length_poly13,b3_seed_rot,BytePerFrame,b3_tmp);
#if 0
			make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame);
			make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame);
			make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame);
			make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame);
			make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame);
			make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame);
			make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame);
			make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame);
#else
			
			//3210 7654
			//7654 3210
			if(ed3Con){
				make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame); //bit0
				make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame); //bit1
				make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame); //bit2
				make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame); //bit3
				make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame); //bit4
				make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame); //bit5
				make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame); //bit6
				make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame); //bit7
			}
			else{

				//{  POLY[1],POLY[0],POLY[ 3],POLY[ 2],POLY[ 5],POLY[ 4],POLY[ 7],POLY[ 6] };
				if(isE13)
				{
					make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame); //bit0
					make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame); //bit1
					make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame); //bit2
					make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame); //bit3
					make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame); //bit4
					make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame); //bit5
					make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame); //bit6
					make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame); //bit7
					for(i=0;i<BytePerFrame*8;i+=16)
					{
						unsigned char tmp;
						tmp=frame_rdm_bs[i];
						frame_rdm_bs[i]=frame_rdm_bs[i+6];
						frame_rdm_bs[i+6]=tmp;
						tmp=frame_rdm_bs[i+1];
						frame_rdm_bs[i+1]=frame_rdm_bs[i+7];
						frame_rdm_bs[i+7]=tmp;
						tmp=frame_rdm_bs[i+2];
						frame_rdm_bs[i+2]=frame_rdm_bs[i+4];
						frame_rdm_bs[i+4]=tmp;
						tmp=frame_rdm_bs[i+3];
						frame_rdm_bs[i+3]=frame_rdm_bs[i+5];
						frame_rdm_bs[i+5]=tmp;
					}
				}
				else
				{
					make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame); //bit0
					make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame); //bit1
					make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame); //bit2
					make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame); //bit3
					make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame); //bit4
					make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame); //bit5
					make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame); //bit6
					make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame); //bit7
				}
			}

#endif
			memcpy(&pg_rdm_bs_tmp[frame_no*BytePerFrame*8],&frame_rdm_bs[0],BytePerFrame*8);
			memset(frame_rdm_bs,0,BytePerFrame*8);
			memset(b0_tmp,0,BytePerFrame);
			memset(b1_tmp,0,BytePerFrame);
			memset(b2_tmp,0,BytePerFrame);
			memset(b3_tmp,0,BytePerFrame);
			memset(b4_tmp,0,BytePerFrame);
			memset(b5_tmp,0,BytePerFrame);
			memset(b6_tmp,0,BytePerFrame);
			memset(b7_tmp,0,BytePerFrame);
		}

		

		memcpy(&_block_rdm_bs[0],&pg_rdm_bs_tmp[0],PageBit);
		memset(pg_rdm_bs_tmp,0,PageBit);

	}
	delete[] b7_tmp;
	delete[] b6_tmp;
	delete[] b5_tmp;
	delete[] b4_tmp;
	delete[] b3_tmp;
	delete[] b2_tmp;
	delete[] b1_tmp;
	delete[] b0_tmp;
	delete[] frame_rdm_bs;
	delete[] pg_rdm_bs_tmp;

	return 0;
}
unsigned long NPsConversion::make_frame_bit(int startaddr,int Spacing,unsigned char *frame_bs,int bs_len,unsigned char* source,int source_len)
{
	unsigned char *start_bs = &frame_bs[startaddr];
	int index = 0;
	for(int i=0;i<bs_len;i+=Spacing){
		start_bs[i] = source[index];
		//ASSERT(index != source_len);
		index++;
	}
	return 0;
}


unsigned long NPsConversion::crc_prsn_t2_v1(int *crc_poly,int length_poly,int crc_seed,int length,unsigned char *out_ary)
{

	int poly_ord = crc_poly[length_poly-1];
	unsigned char *one_ary = new unsigned char[length_poly-1]; 
	unsigned char *temp_ary = new unsigned char[length_poly-1];
	unsigned char *seed_ary = new unsigned char[poly_ord]; //多項式最高次方
	unsigned char *crc_reg = new unsigned char[poly_ord];  
	unsigned char *add_ary = new unsigned char[poly_ord];
	unsigned char *add_ary1 = new unsigned char[poly_ord];
	int i,n;

	memset(one_ary, 1, length_poly-1);
	memset(temp_ary, 1, length_poly-1);

	memset(crc_reg, 0, poly_ord);
	memset(add_ary, 0, poly_ord);

	//for(int i=0;i<length_poly-1;i++){
	//	temp_ary[i] = crc_poly[i]+1;
	//}
	for(i=0;i<length_poly-1;i++){
		add_ary[crc_poly[i]] = 0x01;
	}

	unsigned long tmp1 = crc_seed;
	for(n=0;n<poly_ord;n++){  //把crc_seed轉成2進位放入seed_ary
		seed_ary[n] = (tmp1 % 2);
		tmp1  =  (unsigned long)floor((float)(tmp1/2));
	}
	int idx = 0;
	for(i=poly_ord-1;i>=0;i--) {
		crc_reg[idx++] = seed_ary[i];
	}
	//memcpy(crc_reg , seed_ary,poly_ord);
	for(n=0;n<length;n++){
		////mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2)
		//		out_ary[n]=crc_reg[poly_ord-1];//[0 crc_reg(1:poly_ord-1)]

		out_ary[n]=crc_reg[poly_ord-1];
		unsigned char TempB = crc_reg[poly_ord-1];

		//memcpy(add_ary1,crc_reg,poly_ord);
		//-----------------------------------------------------------
		for(i=0;i<poly_ord;i++){
			add_ary1[i] = add_ary[i] * crc_reg[poly_ord-1];// add_ary*crc_reg(poly_ord)
		}
#if 1
		memcpy(&crc_reg[1],&crc_reg[0],poly_ord-1); //向右shift一個bit
#endif
		crc_reg[0] = 0;//TempB; //前面補0
#if 0
		crc_reg[0] = TempB; //rotation
		for(i=1;i<length_poly-1;i++){
			add_ary1[crc_poly[i]] ^= TempB;
		}
		for(i=1;i<length_poly-1;i++){
			crc_reg[crc_poly[i]+1] = add_ary1[crc_poly[i]];
		}
#endif
		for(i=0;i<poly_ord;i++){
			crc_reg[i] += add_ary1[i]; //mod([0 crc_reg(1:poly_ord-1)] + add_ary,2)
			crc_reg[i] = crc_reg[i] % 2;
		}
	}
	//crc_reg = mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2);	
	//add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);
	//	add_ary=zeros(1,poly_ord);
	//	add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);
#if 0
	%crc_poly = [from low order to high order]
	%EX:    polynomial = 1+ x^2 +x^5 + x^6 + x^9 +x^11 + x^14
		%EX: ==>  crc_poly = [0 2 5 6 9 11 14];
	%crc_seed = 'Hex data' : MSB ... LSB

		length_poly = length(crc_poly);
	poly_ord = crc_poly(length(crc_poly));

	add_ary=zeros(1,poly_ord);
	add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);

	%reg_log=zeros(length_output,poly_ord);

	%convert seed from hex to binary array
		%ex: 0x056c ==> [0 0 1 1 0 1 1 0 1 0 1 0 0] 13 bits seed
		seed_ary = zeros(1,poly_ord);
	tmp1=hex2dec(crc_seed);
	for n=1:poly_ord
		seed_ary(n) = mod(tmp1,2);
	tmp1=floor(tmp1/2);
	end


		crc_reg = seed_ary;
	for n=1:length_output;
	%reg_log(n,:)=crc_reg;
	output(n)=crc_reg(poly_ord);
	crc_reg = mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2);
	end
#endif

	delete[] add_ary;
	delete[] add_ary1;
	delete[] one_ary;
	delete[] temp_ary;
	delete[] seed_ary;
	delete[] crc_reg;

	return 0;
}

unsigned long NPsConversion::crc_prsn_t2(int *crc_poly,int length_poly,int crc_seed,int length,unsigned char *out_ary)
{

	int poly_ord = crc_poly[length_poly-1];
	unsigned char *one_ary = new unsigned char[length_poly-1]; 
	unsigned char *temp_ary = new unsigned char[length_poly-1];
	unsigned char *seed_ary = new unsigned char[poly_ord]; //多項式最高次方
	unsigned char *crc_reg = new unsigned char[poly_ord];  
	unsigned char *add_ary = new unsigned char[poly_ord];
	unsigned char *add_ary1 = new unsigned char[poly_ord];
	int i,n;
	memset(one_ary,1,length_poly-1);
	memset(temp_ary,1,length_poly-1);
	
	memset(add_ary,0,poly_ord);
	memset(seed_ary,0,poly_ord);

	//for(int i=0;i<length_poly-1;i++){
	//	temp_ary[i] = crc_poly[i]+1;
	//}
	for(i=0;i<length_poly-1;i++){
		add_ary[crc_poly[i]] = 0x01;
	}

	unsigned long tmp1 = crc_seed;
	for(n=0;n<poly_ord;n++){  //把crc_seed轉成2進位放入seed_ary
		if(((crc_seed >> n) & 0x01 )== 0x01) seed_ary[n] = 1;
		//seed_ary[n] = (tmp1 % 2);
		//tmp1  =  (unsigned long)floor((float)(tmp1/2));
	}
	int idx = 0;
	for(i=poly_ord-1;i>=0;i--) {
		crc_reg[idx++] = seed_ary[i];
	}
//	memcpy(crc_reg , seed_ary,poly_ord);
	for(n=0;n<length;n++){
		////mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2)
//		out_ary[n]=crc_reg[poly_ord-1];//[0 crc_reg(1:poly_ord-1)]

		//ASSERT(n<=492);
		out_ary[n]=crc_reg[poly_ord-1];
		unsigned char TempB = crc_reg[poly_ord-1];

		memcpy(add_ary1,crc_reg,poly_ord);
		
		//-----------------------------------------------------------
		//for(int i=0;i<poly_ord;i++){
		//	add_ary1[i] = add_ary[i] * crc_reg[poly_ord-1];// add_ary*crc_reg(poly_ord)
		//}
#if 1
		memcpy(&crc_reg[1],&crc_reg[0],poly_ord-1); //向右shift一個bit
		//memcpy(add_ary1,crc_reg,poly_ord);
#endif
	//	memcpy(&crc_reg[0],&crc_reg[1],poly_ord-1); //向右shift一個bit
		//crc_reg[0] = 0;//TempB; //前面補0
		crc_reg[0] = TempB; //rotation
		for(i=1;i<length_poly-1;i++){
			/*int maxlen = crc_poly[length_poly-1]-1;
			int ds= crc_poly[i];*/
			//0,1,3,4,13
			//add_ary1[maxlen-ds] ^= TempB;
			add_ary1[crc_poly[i]-1] ^= TempB;
		}
		for(i=1;i<length_poly-1;i++){
			crc_reg[crc_poly[i]] = add_ary1[crc_poly[i]-1];
		}

		//for(int i=0;i<poly_ord;i++){
		//	crc_reg[i] += add_ary1[i]; //mod([0 crc_reg(1:poly_ord-1)] + add_ary,2)
		//	crc_reg[i] = crc_reg[i] % 2;
		//}
	}
	//crc_reg = mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2);	
	//add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);
//	add_ary=zeros(1,poly_ord);
//	add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);
#if 0
	%crc_poly = [from low order to high order]
	%EX:    polynomial = 1+ x^2 +x^5 + x^6 + x^9 +x^11 + x^14
		%EX: ==>  crc_poly = [0 2 5 6 9 11 14];
	%crc_seed = 'Hex data' : MSB ... LSB

	length_poly = length(crc_poly);
	poly_ord = crc_poly(length(crc_poly));

	add_ary=zeros(1,poly_ord);
	add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);

	%reg_log=zeros(length_output,poly_ord);

	%convert seed from hex to binary array
		%ex: 0x056c ==> [0 0 1 1 0 1 1 0 1 0 1 0 0] 13 bits seed
		seed_ary = zeros(1,poly_ord);
	tmp1=hex2dec(crc_seed);
	for n=1:poly_ord
		seed_ary(n) = mod(tmp1,2);
	tmp1=floor(tmp1/2);
	end


		crc_reg = seed_ary;
	for n=1:length_output;
	%reg_log(n,:)=crc_reg;
	output(n)=crc_reg(poly_ord);
	crc_reg = mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2);
	end
#endif

	delete[] add_ary;
	delete[] add_ary1;
	delete[] one_ary;
	delete[] temp_ary;
	delete[] seed_ary;
	delete[] crc_reg;

	return 0;
}

unsigned long NPsConversion::seed_rotate( unsigned long crc_seed , unsigned long rot_no , unsigned long poly_ord )
{
	unsigned long tmp1 = crc_seed;
	unsigned char *seed_ary = new unsigned char[poly_ord];
	int i,n;
	memset(seed_ary,0,poly_ord);

	//unsigned long tmp5 = 0x0A;
	/*for(int n=0;n<poly_ord;n++){
		seed_ary[n] = (tmp5 % 2);
		tmp5  =  (unsigned long)floor((float)(tmp5/2));
	}*/

	/*for(int n=0;n<poly_ord;n++){
		seed_ary[n] = (tmp1 % 2);
		tmp1  =  (unsigned long)floor((float)(tmp1/2));
	}*/

	for(n=0;n<(int)poly_ord;n++){  //把crc_seed轉成2進位放入seed_ary
		if(((crc_seed >> n) & 0x01 )== 0x01) seed_ary[n] = 1;
		//seed_ary[n] = (tmp1 % 2);
		//tmp1  =  (unsigned long)floor((float)(tmp1/2));
	}


	for(i=0;i<(int)rot_no;i++){
		unsigned char  tempv = seed_ary[0];
		memcpy(&seed_ary[0],&seed_ary[1],poly_ord-1);
		seed_ary[poly_ord-1] = tempv;
	}

	unsigned long tmp2=0;

	/*int shiftbit = 7;
	for(int n=0;n<8;n++){
		if(_block_rdm_bs[n]) tmp2 |= 0x01 << shiftbit;
		shiftbit--;
	}*/

	for(n=poly_ord-1;n>=0;n--){
		if(seed_ary[n]) tmp2 |= 0x01 << n;
		//tmp2 = tmp2 << 1;
	}

	delete[] seed_ary;

	return tmp2;
}
void NPsConversion::SetBlockSeed( unsigned int BlockSeed, bool isNeedShift )
{
	if (isNeedShift)
		seedinit = (BlockSeed<<22)+BlockSeed; 
	else
		seedinit = BlockSeed;	// E13 seed init = (seed[9..0]<<22)+seed[21..0]
}

PsConversion::PsConversion(void)
{
	_block_rdm_bs = NULL;
	FramePerPage = 8;
}

PsConversion::~PsConversion(void)
{
	if(_block_rdm_bs != NULL)
		delete []_block_rdm_bs;
	_block_rdm_bs = NULL;
}
unsigned long PsConversion::MakeConv( unsigned char *sourceBuffer , unsigned char *FinalBuff  ,int ConvSizeByte)
{

	int blockBit = ConvSizeByte * 8; //change to bit
	//ULONG blockBit = BytePerFrame*8;;
	for(int i=0;i<blockBit;i+=8){
		BYTE RdmSeed = 0;
		int shiftbit = 7;
		for(int n=i;n<i+8;n++){
			if(_block_rdm_bs[n]) RdmSeed |= 0x01 << shiftbit;
			shiftbit--;
		}
		FinalBuff[i/8] = sourceBuffer[i/8] ^ RdmSeed;
	}
	return 0;
}
#if 0
unsigned long PsConversion::seed_selector( unsigned long Cell_Length , unsigned long reserve1 , unsigned long reserve2 )
{
	/*
	parameter = [2, 73728,16,86*3];*/
	int Cell_Type = 2;
	//int Cell_Length = 33554432;//73728;
	int FramePerPage = 1;
	int PagesPerBlock = 1;
	int BytePerFrame = Cell_Length/FramePerPage/8;
	int poly13[] = {0 ,1 ,3 ,4 ,13};  
	int poly17[] = {0 ,3 ,17};
	int poly19[] = {0 ,1 ,2 ,5 ,19};
	int poly23[] = {0 ,5 ,23};//length_poly23 = 3;
	BYTE seedinit = 0;
	unsigned char *b7_tmp = new unsigned char[BytePerFrame];
	unsigned char *b6_tmp = new unsigned char[BytePerFrame];
	unsigned char *b5_tmp = new unsigned char[BytePerFrame];
	unsigned char *b4_tmp = new unsigned char[BytePerFrame];
	unsigned char *b3_tmp = new unsigned char[BytePerFrame];
	unsigned char *b2_tmp = new unsigned char[BytePerFrame];
	unsigned char *b1_tmp = new unsigned char[BytePerFrame];
	unsigned char *b0_tmp = new unsigned char[BytePerFrame];
	unsigned char *frame_rdm_bs = new unsigned char[BytePerFrame*8];
	ULONG PageBit = FramePerPage*BytePerFrame*8;
	unsigned char *pg_rdm_bs_tmp = new unsigned char[PageBit];
	ULONG blockBit = PagesPerBlock*PageBit;
	_block_rdm_bs = new unsigned char[blockBit];
	memset(frame_rdm_bs,0,BytePerFrame*8);
	memset(pg_rdm_bs_tmp,0,FramePerPage*BytePerFrame*8);
	memset(b0_tmp,0,BytePerFrame);
	memset(b1_tmp,0,BytePerFrame);
	memset(b2_tmp,0,BytePerFrame);
	memset(b3_tmp,0,BytePerFrame);
	memset(b4_tmp,0,BytePerFrame);
	memset(b5_tmp,0,BytePerFrame);
	memset(b6_tmp,0,BytePerFrame);
	memset(b7_tmp,0,BytePerFrame);
	



	int pg_seed_b7_4[16];
	int pg_seed_b3_0[16];
	for(int pg_no=0;pg_no<PagesPerBlock;pg_no++){
		double pg_no_b3_0 = pg_no % 16;//mod(pg_no-1,16);
		double pg_no_b7_4 = floor((double)(pg_no)/(double)16);
		for (int i=0;i<16;i++){
			pg_seed_b7_4[i] =  ((int)(pg_no_b3_0 + i ) % 16);
		}
		for (int i=0;i<16;i++){
			pg_seed_b3_0[i] =  ((int)(pg_seed_b7_4[i]+1) % 16);
		}

		for(int frame_no=0;frame_no<FramePerPage;frame_no++){
			ULONG b7_seed_ori = 0;
			ULONG b7_seed_rot_tmp1=0;
			b7_seed_ori=RSeed[3*16+(int)pg_seed_b7_4[frame_no]];
			b7_seed_rot_tmp1 =seed_rotate(b7_seed_ori,pg_no_b7_4,poly23[2]);
			/************************************************************************/
			/* //b7_seed_rot_tmp2 = mod( + bitxor(hex2dec(b7_seed_rot_tmp1), seedinit),2^23);                                                                     */
			/************************************************************************/
			b7_seed_rot_tmp1 = b7_seed_rot_tmp1 ^ seedinit; //xor
			b7_seed_rot_tmp1 = b7_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			ULONG b7_seed_rot  = b7_seed_rot_tmp1;
			int length_poly23 = 3;
			crc_prsn_t2(poly23,length_poly23,b7_seed_rot,BytePerFrame,b7_tmp);
			//-------b6--------------
			ULONG b6_seed_ori = 0;
			ULONG b6_seed_rot_tmp1=0;
			b6_seed_ori=RSeed[2*16 +(int)pg_seed_b7_4[frame_no]];
			b6_seed_rot_tmp1=seed_rotate(b6_seed_ori,pg_no_b7_4,poly19[4]);
			b6_seed_rot_tmp1 = b6_seed_rot_tmp1 ^ seedinit; //xor
			b6_seed_rot_tmp1 = b6_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			ULONG b6_seed_rot  = b6_seed_rot_tmp1;
			int length_poly19 = 5;
			crc_prsn_t2(poly19,length_poly19,b6_seed_rot,BytePerFrame,b6_tmp);
			//-------b5--------------
			ULONG b5_seed_ori = 0;
			ULONG b5_seed_rot_tmp1=0;
			b5_seed_ori=RSeed[1*16 +(int)pg_seed_b7_4[frame_no]];
			b5_seed_rot_tmp1=seed_rotate(b5_seed_ori,pg_no_b7_4,poly17[2]);
			b5_seed_rot_tmp1 = b5_seed_rot_tmp1 ^ seedinit; //xor
			b5_seed_rot_tmp1 = b5_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			ULONG b5_seed_rot  = b5_seed_rot_tmp1;
			int length_poly17 = 3;
			crc_prsn_t2(poly17,length_poly17,b5_seed_rot,BytePerFrame,b5_tmp);
			//-------b4--------------
			ULONG b4_seed_ori = 0;
			ULONG b4_seed_rot_tmp1=0;
			b4_seed_ori=RSeed[0 +(int)pg_seed_b7_4[frame_no]];
			b4_seed_rot_tmp1=seed_rotate(b4_seed_ori,pg_no_b7_4,poly13[4]);
			b4_seed_rot_tmp1 = b4_seed_rot_tmp1 ^ seedinit; //xor
			b4_seed_rot_tmp1 = b4_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			ULONG b4_seed_rot  = b4_seed_rot_tmp1;
			int length_poly13 = 5;
			crc_prsn_t2(poly13,length_poly13,b4_seed_rot,BytePerFrame,b4_tmp);

			//-------b3--------------
			ULONG b3_seed_ori = 0;
			ULONG b3_seed_rot_tmp1=0;
			b3_seed_ori=RSeed[3*16 +(int)pg_seed_b3_0[frame_no]];
			b3_seed_rot_tmp1=seed_rotate(b3_seed_ori,pg_no_b7_4,poly23[2]);
			b3_seed_rot_tmp1 = b3_seed_rot_tmp1 ^ seedinit; //xor
			b3_seed_rot_tmp1 = b3_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			ULONG b3_seed_rot  = b3_seed_rot_tmp1;
			crc_prsn_t2(poly23,length_poly23,b3_seed_rot,BytePerFrame,b3_tmp);

			//-------b2--------------
			ULONG b2_seed_ori = 0;
			ULONG b2_seed_rot_tmp1=0;
			b2_seed_ori=RSeed[2*16 +(int)pg_seed_b3_0[frame_no]];
			b2_seed_rot_tmp1=seed_rotate(b2_seed_ori,pg_no_b7_4,poly19[4]);
			b2_seed_rot_tmp1 = b2_seed_rot_tmp1 ^ seedinit; //xor
			b2_seed_rot_tmp1 = b2_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			ULONG b2_seed_rot  = b2_seed_rot_tmp1;
			crc_prsn_t2(poly19,length_poly19,b2_seed_rot,BytePerFrame,b2_tmp);
			//-------b1--------------
			ULONG b1_seed_ori = 0;
			ULONG b1_seed_rot_tmp1=0;
			b1_seed_ori=RSeed[1*16 +(int)pg_seed_b3_0[frame_no]];
			b1_seed_rot_tmp1=seed_rotate(b1_seed_ori,pg_no_b7_4,poly17[2]);
			b1_seed_rot_tmp1 = b1_seed_rot_tmp1 ^ seedinit; //xor
			b1_seed_rot_tmp1 = b1_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			ULONG b1_seed_rot  = b1_seed_rot_tmp1;
			crc_prsn_t2(poly17,length_poly17,b1_seed_rot,BytePerFrame,b1_tmp);
			//-------b0--------------
			ULONG b0_seed_ori = 0;
			ULONG b0_seed_rot_tmp1=0;
			b0_seed_ori=RSeed[0 +(int)pg_seed_b3_0[frame_no]];
			b0_seed_rot_tmp1=seed_rotate(b0_seed_ori,pg_no_b7_4,poly13[4]);
			b0_seed_rot_tmp1 = b0_seed_rot_tmp1 ^ seedinit; //xor
			b0_seed_rot_tmp1 = b0_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			ULONG b0_seed_rot  = b0_seed_rot_tmp1;
			crc_prsn_t2(poly13,length_poly13,b0_seed_rot,BytePerFrame,b0_tmp);
#if 0
			make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame);
			make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame);
			make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame);
			make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame);
			make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame);
			make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame);
			make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame);
			make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame);
#endif
			make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame);
			make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame);
			make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame);
			make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame);
			make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame);
			make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame);
			make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame);
			make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame);
			memcpy(&pg_rdm_bs_tmp[frame_no*BytePerFrame*8],&frame_rdm_bs[0],BytePerFrame*8);
			memset(frame_rdm_bs,0,BytePerFrame*8);
		}
		memcpy(&_block_rdm_bs[pg_no*PageBit],&pg_rdm_bs_tmp[0],PageBit);
		memset(pg_rdm_bs_tmp,0,PageBit);

	}
	delete b7_tmp;
	delete b6_tmp;
	delete b5_tmp;
	delete b4_tmp;
	delete b3_tmp;
	delete b2_tmp;
	delete b1_tmp;
	delete b0_tmp;
	delete frame_rdm_bs;
	delete pg_rdm_bs_tmp;

	return 0;
}

unsigned long PsConversion::seed_selector2( unsigned long Cell_Length , unsigned long reserve1 , unsigned long reserve2 )
{
	/*
	parameter = [2, 73728,16,86*3];*/
	int Cell_Type = 2;
	//int Cell_Length = 33554432;//73728;
	int FramePerPage = 1;
	int PagesPerBlock = 1;
	int BytePerFrame = Cell_Length/FramePerPage/8;
	int poly13[] = {0 ,1 ,3 ,4 ,13};  
	int poly17[] = {0 ,3 ,17};
	int poly19[] = {0 ,1 ,2 ,5 ,19};
	int poly23[] = {0 ,5 ,23};//length_poly23 = 3;
	BYTE seedinit = 0;
	unsigned char *b7_tmp = new unsigned char[BytePerFrame];
	unsigned char *b6_tmp = new unsigned char[BytePerFrame];
	unsigned char *b5_tmp = new unsigned char[BytePerFrame];
	unsigned char *b4_tmp = new unsigned char[BytePerFrame];
	unsigned char *b3_tmp = new unsigned char[BytePerFrame];
	unsigned char *b2_tmp = new unsigned char[BytePerFrame];
	unsigned char *b1_tmp = new unsigned char[BytePerFrame];
	unsigned char *b0_tmp = new unsigned char[BytePerFrame];
	unsigned char *frame_rdm_bs = new unsigned char[BytePerFrame*8];
	ULONG PageBit = FramePerPage*BytePerFrame*8;
	unsigned char *pg_rdm_bs_tmp = new unsigned char[PageBit];
	ULONG blockBit = PagesPerBlock*PageBit;
	_block_rdm_bs = new unsigned char[blockBit];
	memset(frame_rdm_bs,0,BytePerFrame*8);
	memset(pg_rdm_bs_tmp,0,FramePerPage*BytePerFrame*8);
	memset(b0_tmp,0,BytePerFrame);
	memset(b1_tmp,0,BytePerFrame);
	memset(b2_tmp,0,BytePerFrame);
	memset(b3_tmp,0,BytePerFrame);
	memset(b4_tmp,0,BytePerFrame);
	memset(b5_tmp,0,BytePerFrame);
	memset(b6_tmp,0,BytePerFrame);
	memset(b7_tmp,0,BytePerFrame);
	



	int pg_seed_b7_4[16];
	int pg_seed_b3_0[16];
	int length_poly23 = 3;
	int length_poly19 = 5;
	int length_poly17 = 3;
	int length_poly13 = 5;
	for(int pg_no=0;pg_no<PagesPerBlock;pg_no++){
		double pg_no_b3_0 = pg_no % 16;//mod(pg_no-1,16);
		double pg_no_b7_4 = floor((double)(pg_no)/(double)16);
		for (int i=0;i<16;i++){
			pg_seed_b7_4[i] =  ((int)(pg_no_b3_0 + i ) % 16);
		}
		for (int i=0;i<16;i++){
			pg_seed_b3_0[i] =  ((int)(pg_seed_b7_4[i]+1) % 16);
		}

		for(int frame_no=0;frame_no<FramePerPage;frame_no++){
			ULONG b7_seed_ori = 0;
			ULONG b7_seed_rot_tmp1=0;
			//b7_seed_ori=RSeed[3*16+(int)pg_seed_b7_4[frame_no]];
			b7_seed_ori=RSeed[0*16+(int)pg_seed_b7_4[frame_no]];
			b7_seed_rot_tmp1 =seed_rotate(b7_seed_ori,pg_no_b7_4,poly13[length_poly13-1]); //13
			/************************************************************************/
			/* //b7_seed_rot_tmp2 = mod( + bitxor(hex2dec(b7_seed_rot_tmp1), seedinit),2^23);                                                                     */
			/************************************************************************/
			b7_seed_rot_tmp1 = b7_seed_rot_tmp1 ^ seedinit; //xor
			b7_seed_rot_tmp1 = b7_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			ULONG b7_seed_rot  = b7_seed_rot_tmp1;
			
			crc_prsn_t2(poly13,length_poly13,b7_seed_rot,BytePerFrame,b7_tmp);
			//-------b6--------------
			ULONG b6_seed_ori = 0;
			ULONG b6_seed_rot_tmp1=0;
			b6_seed_ori=RSeed[1*16 +(int)pg_seed_b7_4[frame_no]];
			b6_seed_rot_tmp1=seed_rotate(b6_seed_ori,pg_no_b7_4,poly17[length_poly17-1]); //17
			b6_seed_rot_tmp1 = b6_seed_rot_tmp1 ^ seedinit; //xor
			b6_seed_rot_tmp1 = b6_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			ULONG b6_seed_rot  = b6_seed_rot_tmp1;
			
			crc_prsn_t2(poly17,length_poly17,b6_seed_rot,BytePerFrame,b6_tmp);
			//-------b5--------------
			ULONG b5_seed_ori = 0;
			ULONG b5_seed_rot_tmp1=0;
			b5_seed_ori=RSeed[2*16 +(int)pg_seed_b7_4[frame_no]];
			b5_seed_rot_tmp1=seed_rotate(b5_seed_ori,pg_no_b7_4,poly19[length_poly19-1]); //19
			b5_seed_rot_tmp1 = b5_seed_rot_tmp1 ^ seedinit; //xor
			b5_seed_rot_tmp1 = b5_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			ULONG b5_seed_rot  = b5_seed_rot_tmp1;
			
			crc_prsn_t2(poly19,length_poly19,b5_seed_rot,BytePerFrame,b5_tmp);
			//-------b4--------------
			ULONG b4_seed_ori = 0;
			ULONG b4_seed_rot_tmp1=0;
			b4_seed_ori=RSeed[3*16 +(int)pg_seed_b7_4[frame_no]];
			b4_seed_rot_tmp1=seed_rotate(b4_seed_ori,pg_no_b7_4,poly23[length_poly23-1]); //23
			b4_seed_rot_tmp1 = b4_seed_rot_tmp1 ^ seedinit; //xor
			b4_seed_rot_tmp1 = b4_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			ULONG b4_seed_rot  = b4_seed_rot_tmp1;
			
			crc_prsn_t2(poly23,length_poly23,b4_seed_rot,BytePerFrame,b4_tmp);

			//-------b3--------------
			ULONG b3_seed_ori = 0;
			ULONG b3_seed_rot_tmp1=0;
			b3_seed_ori=RSeed[0*16 +(int)pg_seed_b3_0[frame_no]];
			b3_seed_rot_tmp1=seed_rotate(b3_seed_ori,pg_no_b7_4,poly13[length_poly13-1]); //13
			b3_seed_rot_tmp1 = b3_seed_rot_tmp1 ^ seedinit; //xor
			b3_seed_rot_tmp1 = b3_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			ULONG b3_seed_rot  = b3_seed_rot_tmp1;
			crc_prsn_t2(poly13,length_poly13,b3_seed_rot,BytePerFrame,b3_tmp);

			//-------b2--------------
			ULONG b2_seed_ori = 0;
			ULONG b2_seed_rot_tmp1=0;
			b2_seed_ori=RSeed[1*16 +(int)pg_seed_b3_0[frame_no]];
			b2_seed_rot_tmp1=seed_rotate(b2_seed_ori,pg_no_b7_4,poly17[length_poly17-1]); //17
			b2_seed_rot_tmp1 = b2_seed_rot_tmp1 ^ seedinit; //xor
			b2_seed_rot_tmp1 = b2_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			ULONG b2_seed_rot  = b2_seed_rot_tmp1;
			crc_prsn_t2(poly17,length_poly17,b2_seed_rot,BytePerFrame,b2_tmp);
			//-------b1--------------
			ULONG b1_seed_ori = 0;
			ULONG b1_seed_rot_tmp1=0;
			b1_seed_ori=RSeed[2*16 +(int)pg_seed_b3_0[frame_no]];
			b1_seed_rot_tmp1=seed_rotate(b1_seed_ori,pg_no_b7_4,poly19[length_poly19-1]); //19
			b1_seed_rot_tmp1 = b1_seed_rot_tmp1 ^ seedinit; //xor
			b1_seed_rot_tmp1 = b1_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			ULONG b1_seed_rot  = b1_seed_rot_tmp1;
			crc_prsn_t2(poly19,length_poly19,b1_seed_rot,BytePerFrame,b1_tmp);
			//-------b0--------------
			ULONG b0_seed_ori = 0;
			ULONG b0_seed_rot_tmp1=0;
			b0_seed_ori=RSeed[3*16 +(int)pg_seed_b3_0[frame_no]];
			b0_seed_rot_tmp1=seed_rotate(b0_seed_ori,pg_no_b7_4,poly23[length_poly23-1]); //23
			b0_seed_rot_tmp1 = b0_seed_rot_tmp1 ^ seedinit; //xor
			b0_seed_rot_tmp1 = b0_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			ULONG b0_seed_rot  = b0_seed_rot_tmp1;
			crc_prsn_t2(poly23,length_poly23,b0_seed_rot,BytePerFrame,b0_tmp);
#if 1
			make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame);
			make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame);
			make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame);
			make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame);
			make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame);
			make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame);
			make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame);
			make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame);
#else
			make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame);
			make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame);
			make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame);
			make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame);
			make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame);
			make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame);
			make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame);
			make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame);
#endif
			memcpy(&pg_rdm_bs_tmp[frame_no*BytePerFrame*8],&frame_rdm_bs[0],BytePerFrame*8);
			memset(frame_rdm_bs,0,BytePerFrame*8);
		}
		memcpy(&_block_rdm_bs[pg_no*PageBit],&pg_rdm_bs_tmp[0],PageBit);
		memset(pg_rdm_bs_tmp,0,PageBit);

	}
	delete b7_tmp;
	delete b6_tmp;
	delete b5_tmp;
	delete b4_tmp;
	delete b3_tmp;
	delete b2_tmp;
	delete b1_tmp;
	delete b0_tmp;
	delete frame_rdm_bs;
	delete pg_rdm_bs_tmp;

	return 0;
}


unsigned long PsConversion::seed_selector3( unsigned long Cell_Length , unsigned long reserve1 , unsigned long reserve2 )
{
	/*
	parameter = [2, 73728,16,86*3];*/
	int Cell_Type = 2;
	//int Cell_Length = 33554432;//73728;
	int FramePerPage = 1;
	int PagesPerBlock = 1;
	int BytePerFrame = Cell_Length/FramePerPage/8;
	int poly13[] = {0 ,1 ,3 ,4 ,13};  
	int poly17[] = {0 ,3 ,17};
	int poly19[] = {0 ,1 ,2 ,5 ,19};
	int poly23[] = {0 ,5 ,23};//length_poly23 = 3;
	BYTE seedinit = 0;
	unsigned char *b7_tmp = new unsigned char[BytePerFrame];
	unsigned char *b6_tmp = new unsigned char[BytePerFrame];
	unsigned char *b5_tmp = new unsigned char[BytePerFrame];
	unsigned char *b4_tmp = new unsigned char[BytePerFrame];
	unsigned char *b3_tmp = new unsigned char[BytePerFrame];
	unsigned char *b2_tmp = new unsigned char[BytePerFrame];
	unsigned char *b1_tmp = new unsigned char[BytePerFrame];
	unsigned char *b0_tmp = new unsigned char[BytePerFrame];
	unsigned char *frame_rdm_bs = new unsigned char[BytePerFrame*8];
	ULONG PageBit = FramePerPage*BytePerFrame*8;
	unsigned char *pg_rdm_bs_tmp = new unsigned char[PageBit];
	ULONG blockBit = PagesPerBlock*PageBit;
	_block_rdm_bs = new unsigned char[blockBit];
	memset(frame_rdm_bs,0,BytePerFrame*8);
	memset(pg_rdm_bs_tmp,0,FramePerPage*BytePerFrame*8);
	memset(b0_tmp,0,BytePerFrame);
	memset(b1_tmp,0,BytePerFrame);
	memset(b2_tmp,0,BytePerFrame);
	memset(b3_tmp,0,BytePerFrame);
	memset(b4_tmp,0,BytePerFrame);
	memset(b5_tmp,0,BytePerFrame);
	memset(b6_tmp,0,BytePerFrame);
	memset(b7_tmp,0,BytePerFrame);
	



	int pg_seed_b7_4[16];
	int pg_seed_b3_0[16];
	int length_poly23 = 3;
	int length_poly19 = 5;
	int length_poly17 = 3;
	int length_poly13 = 5;
	for(int pg_no=0;pg_no<PagesPerBlock;pg_no++){
		double pg_no_b3_0 = pg_no % 16;//mod(pg_no-1,16);
		double pg_no_b7_4 = floor((double)(pg_no)/(double)16);
		for (int i=0;i<16;i++){
			pg_seed_b7_4[i] =  ((int)(pg_no_b3_0 + i ) % 16);
		}
		for (int i=0;i<16;i++){
			pg_seed_b3_0[i] =  ((int)(pg_seed_b7_4[i]+1) % 16);
		}

		for(int frame_no=0;frame_no<FramePerPage;frame_no++){
			ULONG b7_seed_ori = 0;
			ULONG b7_seed_rot_tmp1=0;
			//b7_seed_ori=RSeed[3*16+(int)pg_seed_b7_4[frame_no]];
			b7_seed_ori=RSeed[3*16+(int)pg_seed_b7_4[frame_no]];
			b7_seed_rot_tmp1 =seed_rotate(b7_seed_ori,pg_no_b7_4,poly13[length_poly13-1]); //13
			/************************************************************************/
			/* //b7_seed_rot_tmp2 = mod( + bitxor(hex2dec(b7_seed_rot_tmp1), seedinit),2^23);                                                                     */
			/************************************************************************/
			b7_seed_rot_tmp1 = b7_seed_rot_tmp1 ^ seedinit; //xor
			b7_seed_rot_tmp1 = b7_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			ULONG b7_seed_rot  = b7_seed_rot_tmp1;
			
			//crc_prsn_t2_v1(poly13,length_poly13,b7_seed_rot,BytePerFrame,b7_tmp);
			crc_prsn_t2(poly13,length_poly13,b7_seed_rot,BytePerFrame,b7_tmp);
			//-------b6--------------
			ULONG b6_seed_ori = 0;
			ULONG b6_seed_rot_tmp1=0;
			b6_seed_ori=RSeed[2*16 +(int)pg_seed_b7_4[frame_no]];
			b6_seed_rot_tmp1=seed_rotate(b6_seed_ori,pg_no_b7_4,poly17[length_poly17-1]); //17
			b6_seed_rot_tmp1 = b6_seed_rot_tmp1 ^ seedinit; //xor
			b6_seed_rot_tmp1 = b6_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			ULONG b6_seed_rot  = b6_seed_rot_tmp1;
			
			//crc_prsn_t2_v1(poly17,length_poly17,b6_seed_rot,BytePerFrame,b6_tmp);
			crc_prsn_t2(poly17,length_poly17,b6_seed_rot,BytePerFrame,b6_tmp);
			//-------b5--------------
			ULONG b5_seed_ori = 0;
			ULONG b5_seed_rot_tmp1=0;
			b5_seed_ori=RSeed[1*16 +(int)pg_seed_b7_4[frame_no]];
			b5_seed_rot_tmp1=seed_rotate(b5_seed_ori,pg_no_b7_4,poly19[length_poly19-1]); //19
			b5_seed_rot_tmp1 = b5_seed_rot_tmp1 ^ seedinit; //xor
			b5_seed_rot_tmp1 = b5_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			ULONG b5_seed_rot  = b5_seed_rot_tmp1;
			
			crc_prsn_t2(poly19,length_poly19,b5_seed_rot,BytePerFrame,b5_tmp);
			//-------b4--------------
			ULONG b4_seed_ori = 0;
			ULONG b4_seed_rot_tmp1=0;
			b4_seed_ori=RSeed[0*16 +(int)pg_seed_b7_4[frame_no]];
			b4_seed_rot_tmp1=seed_rotate(b4_seed_ori,pg_no_b7_4,poly23[length_poly23-1]); //23
			b4_seed_rot_tmp1 = b4_seed_rot_tmp1 ^ seedinit; //xor
			b4_seed_rot_tmp1 = b4_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			ULONG b4_seed_rot  = b4_seed_rot_tmp1;
			
			crc_prsn_t2(poly23,length_poly23,b4_seed_rot,BytePerFrame,b4_tmp);

			//-------b3--------------
			ULONG b3_seed_ori = 0;
			ULONG b3_seed_rot_tmp1=0;
			b3_seed_ori=RSeed[3*16 +(int)pg_seed_b3_0[frame_no]];
			b3_seed_rot_tmp1=seed_rotate(b3_seed_ori,pg_no_b7_4,poly13[length_poly13-1]); //13
			b3_seed_rot_tmp1 = b3_seed_rot_tmp1 ^ seedinit; //xor
			b3_seed_rot_tmp1 = b3_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			ULONG b3_seed_rot  = b3_seed_rot_tmp1;
			crc_prsn_t2(poly13,length_poly13,b3_seed_rot,BytePerFrame,b3_tmp);

			//-------b2--------------
			ULONG b2_seed_ori = 0;
			ULONG b2_seed_rot_tmp1=0;
			b2_seed_ori=RSeed[2*16 +(int)pg_seed_b3_0[frame_no]];
			b2_seed_rot_tmp1=seed_rotate(b2_seed_ori,pg_no_b7_4,poly17[length_poly17-1]); //17
			b2_seed_rot_tmp1 = b2_seed_rot_tmp1 ^ seedinit; //xor
			b2_seed_rot_tmp1 = b2_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			ULONG b2_seed_rot  = b2_seed_rot_tmp1;
			crc_prsn_t2(poly17,length_poly17,b2_seed_rot,BytePerFrame,b2_tmp);
			//-------b1--------------
			ULONG b1_seed_ori = 0;
			ULONG b1_seed_rot_tmp1=0;
			b1_seed_ori=RSeed[1*16 +(int)pg_seed_b3_0[frame_no]];
			b1_seed_rot_tmp1=seed_rotate(b1_seed_ori,pg_no_b7_4,poly19[length_poly19-1]); //19
			b1_seed_rot_tmp1 = b1_seed_rot_tmp1 ^ seedinit; //xor
			b1_seed_rot_tmp1 = b1_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			ULONG b1_seed_rot  = b1_seed_rot_tmp1;
			crc_prsn_t2(poly19,length_poly19,b1_seed_rot,BytePerFrame,b1_tmp);
			//-------b0--------------
			ULONG b0_seed_ori = 0;
			ULONG b0_seed_rot_tmp1=0;
			b0_seed_ori=RSeed[0*16 +(int)pg_seed_b3_0[frame_no]];
			b0_seed_rot_tmp1=seed_rotate(b0_seed_ori,pg_no_b7_4,poly23[length_poly23-1]); //23
			b0_seed_rot_tmp1 = b0_seed_rot_tmp1 ^ seedinit; //xor
			b0_seed_rot_tmp1 = b0_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			ULONG b0_seed_rot  = b0_seed_rot_tmp1;
			crc_prsn_t2(poly23,length_poly23,b0_seed_rot,BytePerFrame,b0_tmp);
#if 0
			make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame);
			make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame);
			make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame);
			make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame);
			make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame);
			make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame);
			make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame);
			make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame);
#else
			make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame);
			make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame);
			make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame);
			make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame);
			make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame);
			make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame);
			make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame);
			make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame);
#endif
			memcpy(&pg_rdm_bs_tmp[frame_no*BytePerFrame*8],&frame_rdm_bs[0],BytePerFrame*8);
			memset(frame_rdm_bs,0,BytePerFrame*8);
		}
		memcpy(&_block_rdm_bs[pg_no*PageBit],&pg_rdm_bs_tmp[0],PageBit);
		memset(pg_rdm_bs_tmp,0,PageBit);

	}
	delete b7_tmp;
	delete b6_tmp;
	delete b5_tmp;
	delete b4_tmp;
	delete b3_tmp;
	delete b2_tmp;
	delete b1_tmp;
	delete b0_tmp;
	delete frame_rdm_bs;
	delete pg_rdm_bs_tmp;

	return 0;
}
#endif


unsigned long PsConversion::seed_selector4( unsigned long Cell_Length , unsigned long reserve1 , unsigned long page ,int ed3Con)
{

	unsigned long RSeed[64]={0x00056c,0x000fbc,0x001717,0x000a52,0x0007cf,0x00064e,0x00098c,0x001399,0x001246,0x000150,0x0011f1,0x001026,0x001c45,0x00012a,0x000849,0x001d4c,
		0x01bc77,0x00090c,0x01e1ad,0x00d564,0x01cb3b,0x0167a7,0x01bd54,0x011690,0x014794,0x000787,0x017bad,0x0173ab,0x008b63,0x003a3a,0x01cb9b,0x00af6e,
		0x05d356,0x07f880,0x06db67,0x06fbac,0x037cd1,0x00f1d2,0x02c2af,0x01d310,0x00f34f,0x073450,0x046eaa,0x079634,0x042924,0x0435d5,0x028350,0x00949e,
		0x1f645d,0x3d9e86,0x7d7ea0,0x6823dd,0x5c91f0,0x506be8,0x4b5fb7,0x27e53b,0x347bc4,0x3d8927,0x5403f5,0x01b987,0x4bf708,0x1a7baa,0x1e8309,0x00193a};


	/*
	parameter = [2, 73728,16,86*3];*/
	int Cell_Type = 2;
	//int Cell_Length = 33554432;//73728;
	//int FramePerPage = 8;
	int PagesPerBlock = 1;
	int BytePerFrame = Cell_Length/FramePerPage/8;
	int poly13[] = {0 ,1 ,3 ,4 ,13};  
	int poly17[] = {0 ,3 ,17};
	int poly19[] = {0 ,1 ,2 ,5 ,19};
	int poly23[] = {0 ,5 ,23};//length_poly23 = 3;
	BYTE seedinit = 0;
	unsigned char *b7_tmp = new unsigned char[BytePerFrame];
	unsigned char *b6_tmp = new unsigned char[BytePerFrame];
	unsigned char *b5_tmp = new unsigned char[BytePerFrame];
	unsigned char *b4_tmp = new unsigned char[BytePerFrame];
	unsigned char *b3_tmp = new unsigned char[BytePerFrame];
	unsigned char *b2_tmp = new unsigned char[BytePerFrame];
	unsigned char *b1_tmp = new unsigned char[BytePerFrame];
	unsigned char *b0_tmp = new unsigned char[BytePerFrame];
	unsigned char *frame_rdm_bs = new unsigned char[BytePerFrame*8];
	ULONG PageBit = FramePerPage*BytePerFrame*8;
	unsigned char *pg_rdm_bs_tmp = new unsigned char[PageBit];
	ULONG blockBit = PagesPerBlock*PageBit;
	if(_block_rdm_bs == NULL)
		_block_rdm_bs = new unsigned char[blockBit];
	memset(frame_rdm_bs,0,BytePerFrame*8);
	memset(pg_rdm_bs_tmp,0,FramePerPage*BytePerFrame*8);
	memset(b0_tmp,0,BytePerFrame);
	memset(b1_tmp,0,BytePerFrame);
	memset(b2_tmp,0,BytePerFrame);
	memset(b3_tmp,0,BytePerFrame);
	memset(b4_tmp,0,BytePerFrame);
	memset(b5_tmp,0,BytePerFrame);
	memset(b6_tmp,0,BytePerFrame);
	memset(b7_tmp,0,BytePerFrame);
	



	//int pg_seed_b7_4[16];
	//int pg_seed_b3_0[16];
	int *pg_seed_b7_4 = new int[16];
	int *pg_seed_b3_0 = new int[16];
	int length_poly23 = 3;
	int length_poly19 = 5;
	int length_poly17 = 3;
	int length_poly13 = 5;
	for(unsigned int pg_no=page;pg_no<page+1;pg_no++){
		double pg_no_b3_0 = pg_no % 16;//mod(pg_no-1,16);
		double pg_no_b7_4 = floor((double)(pg_no)/(double)16);
		for (int i=0;i<16;i++){
			pg_seed_b7_4[i] =  ((int)(pg_no_b3_0 + i ) % 16);
		}
		for (int i=0;i<16;i++){
			pg_seed_b3_0[i] =  ((int)(pg_seed_b7_4[i]+1) % 16);
		}

		for(int frame_no=0;frame_no<FramePerPage;frame_no++){
			ULONG b4_seed_ori = 0;
			ULONG b4_seed_rot_tmp1=0;
			//b7_seed_ori=RSeed[3*16+(int)pg_seed_b7_4[frame_no]];
			b4_seed_ori=RSeed[3*16+(int)pg_seed_b7_4[frame_no]];//bit 4
			b4_seed_rot_tmp1 =seed_rotate(b4_seed_ori,(unsigned long)pg_no_b7_4,poly23[length_poly23-1]); //13
			/************************************************************************/
			/* //b7_seed_rot_tmp2 = mod( + bitxor(hex2dec(b7_seed_rot_tmp1), seedinit),2^23);                                                                     */
			/************************************************************************/
			//b4_seed_rot_tmp1 = b4_seed_rot_tmp1 ^ seedinit; //xor
			//b4_seed_rot_tmp1 = b4_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			ULONG b4_seed_rot  = b4_seed_rot_tmp1;
			
			//crc_prsn_t2_v1(poly13,length_poly13,b7_seed_rot,BytePerFrame,b7_tmp);
			crc_prsn_t2(poly23,length_poly23,b4_seed_rot,BytePerFrame,b4_tmp);
			//-------b6--------------
			ULONG b5_seed_ori = 0;
			ULONG b5_seed_rot_tmp1=0; //b5
			b5_seed_ori=RSeed[2*16 +(int)pg_seed_b7_4[frame_no]];
			b5_seed_rot_tmp1=seed_rotate(b5_seed_ori,(unsigned long)pg_no_b7_4,poly19[length_poly19-1]); 
			//b5_seed_rot_tmp1 = b5_seed_rot_tmp1 ^ seedinit; //xor
			//b5_seed_rot_tmp1 = b5_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			ULONG b5_seed_rot  = b5_seed_rot_tmp1;
			
			//crc_prsn_t2_v1(poly17,length_poly17,b6_seed_rot,BytePerFrame,b6_tmp);
			crc_prsn_t2(poly19,length_poly19,b5_seed_rot,BytePerFrame,b5_tmp);
			//-------b5--------------
			ULONG b6_seed_ori = 0;
			ULONG b6_seed_rot_tmp1=0; //b6
			b6_seed_ori=RSeed[1*16 +(int)pg_seed_b7_4[frame_no]];
			b6_seed_rot_tmp1=seed_rotate(b6_seed_ori,(unsigned long)pg_no_b7_4,poly17[length_poly17-1]); 
			//b6_seed_rot_tmp1 = b6_seed_rot_tmp1 ^ seedinit; //xor
			//b6_seed_rot_tmp1 = b6_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			ULONG b6_seed_rot  = b6_seed_rot_tmp1;
			
			crc_prsn_t2(poly17,length_poly17,b6_seed_rot,BytePerFrame,b6_tmp);
			//-------b4--------------
			ULONG b7_seed_ori = 0;
			ULONG b7_seed_rot_tmp1=0; //b7
			b7_seed_ori=RSeed[0*16 +(int)pg_seed_b7_4[frame_no]];
			b7_seed_rot_tmp1=seed_rotate(b7_seed_ori, (unsigned long)pg_no_b7_4,poly13[length_poly13-1]);
			//b7_seed_rot_tmp1 = b7_seed_rot_tmp1 ^ seedinit; //xor
			//b7_seed_rot_tmp1 = b7_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			ULONG b7_seed_rot  = b7_seed_rot_tmp1;
			
			crc_prsn_t2(poly13,length_poly13,b7_seed_rot,BytePerFrame,b7_tmp);

			//-------b3--------------
			ULONG b0_seed_ori = 0;
			ULONG b0_seed_rot_tmp1=0;
			b0_seed_ori=RSeed[3*16 +(int)pg_seed_b3_0[frame_no]];
			b0_seed_rot_tmp1=seed_rotate(b0_seed_ori, (unsigned long)pg_no_b7_4,poly23[length_poly23-1]);
			//b3_seed_rot_tmp1 = b3_seed_rot_tmp1 ^ seedinit; //xor
			//b3_seed_rot_tmp1 = b3_seed_rot_tmp1 % (int)(pow((double)2,(double)23));
			ULONG b0_seed_rot  = b0_seed_rot_tmp1;
			crc_prsn_t2(poly23,length_poly23,b0_seed_rot,BytePerFrame,b0_tmp);

			//-------b2--------------
			ULONG b1_seed_ori = 0;
			ULONG b1_seed_rot_tmp1=0;
			b1_seed_ori=RSeed[2*16 +(int)pg_seed_b3_0[frame_no]];
			b1_seed_rot_tmp1=seed_rotate(b1_seed_ori, (unsigned long)pg_no_b7_4,poly19[length_poly19-1]);
			//b2_seed_rot_tmp1 = b2_seed_rot_tmp1 ^ seedinit; //xor
			//b2_seed_rot_tmp1 = b2_seed_rot_tmp1 % (int)(pow((double)2,(double)19));
			ULONG b1_seed_rot  = b1_seed_rot_tmp1;
			crc_prsn_t2(poly19,length_poly19,b1_seed_rot,BytePerFrame,b1_tmp);
			//-------b1--------------
			ULONG b2_seed_ori = 0;
			ULONG b2_seed_rot_tmp1=0;
			b2_seed_ori=RSeed[1*16 +(int)pg_seed_b3_0[frame_no]];
			b2_seed_rot_tmp1=seed_rotate(b2_seed_ori, (unsigned long)pg_no_b7_4,poly17[length_poly17-1]); //17
			//b1_seed_rot_tmp1 = b1_seed_rot_tmp1 ^ seedinit; //xor
			//b1_seed_rot_tmp1 = b1_seed_rot_tmp1 % (int)(pow((double)2,(double)17));
			ULONG b2_seed_rot  = b2_seed_rot_tmp1;
			crc_prsn_t2(poly17,length_poly17,b2_seed_rot,BytePerFrame,b2_tmp);
			//-------b0--------------
			ULONG b3_seed_ori = 0;
			ULONG b3_seed_rot_tmp1=0;
			b3_seed_ori=RSeed[0*16 +(int)pg_seed_b3_0[frame_no]];
			b3_seed_rot_tmp1=seed_rotate(b3_seed_ori, (unsigned long)pg_no_b7_4,poly13[length_poly13-1]); //13
			//b0_seed_rot_tmp1 = b0_seed_rot_tmp1 ^ seedinit; //xor
			//b0_seed_rot_tmp1 = b0_seed_rot_tmp1 % (int)(pow((double)2,(double)13));
			ULONG b3_seed_rot  = b3_seed_rot_tmp1;
			crc_prsn_t2(poly13,length_poly13,b3_seed_rot,BytePerFrame,b3_tmp);
#if 0
			make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame);
			make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame);
			make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame);
			make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame);
			make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame);
			make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame);
			make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame);
			make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame);
#else
			//3210 7654
			//7654 3210
			if(ed3Con){
				make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame); //bit0
				make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame); //bit1
				make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame); //bit2
				make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame); //bit3
				make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame); //bit4
				make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame); //bit5
				make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame); //bit6
				make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame); //bit7
			}
			else{

				//{  POLY[1],POLY[0],POLY[ 3],POLY[ 2],POLY[ 5],POLY[ 4],POLY[ 7],POLY[ 6] };

				make_frame_bit(7,8,frame_rdm_bs,BytePerFrame*8,b4_tmp,BytePerFrame); //bit0
				make_frame_bit(6,8,frame_rdm_bs,BytePerFrame*8,b5_tmp,BytePerFrame); //bit1
				make_frame_bit(5,8,frame_rdm_bs,BytePerFrame*8,b6_tmp,BytePerFrame); //bit2
				make_frame_bit(4,8,frame_rdm_bs,BytePerFrame*8,b7_tmp,BytePerFrame); //bit3
				make_frame_bit(3,8,frame_rdm_bs,BytePerFrame*8,b0_tmp,BytePerFrame); //bit4
				make_frame_bit(2,8,frame_rdm_bs,BytePerFrame*8,b1_tmp,BytePerFrame); //bit5
				make_frame_bit(1,8,frame_rdm_bs,BytePerFrame*8,b2_tmp,BytePerFrame); //bit6
				make_frame_bit(0,8,frame_rdm_bs,BytePerFrame*8,b3_tmp,BytePerFrame); //bit7
			}

#endif
			memcpy(&pg_rdm_bs_tmp[frame_no*BytePerFrame*8],&frame_rdm_bs[0],BytePerFrame*8);
			memset(frame_rdm_bs,0,BytePerFrame*8);
			memset(b0_tmp,0,BytePerFrame);
			memset(b1_tmp,0,BytePerFrame);
			memset(b2_tmp,0,BytePerFrame);
			memset(b3_tmp,0,BytePerFrame);
			memset(b4_tmp,0,BytePerFrame);
			memset(b5_tmp,0,BytePerFrame);
			memset(b6_tmp,0,BytePerFrame);
			memset(b7_tmp,0,BytePerFrame);
		}

		

		memcpy(&_block_rdm_bs[0],&pg_rdm_bs_tmp[0],PageBit);
		memset(pg_rdm_bs_tmp,0,PageBit);

	}
	delete[] b7_tmp;
	delete[] b6_tmp;
	delete[] b5_tmp;
	delete[] b4_tmp;
	delete[] b3_tmp;
	delete[] b2_tmp;
	delete[] b1_tmp;
	delete[] b0_tmp;
	delete[] frame_rdm_bs;
	delete[] pg_rdm_bs_tmp;

	delete[] pg_seed_b7_4;
	delete[] pg_seed_b3_0;

	return 0;
}

unsigned long PsConversion::make_frame_bit(int startaddr,int Spacing,unsigned char *frame_bs,int bs_len,unsigned char* source,int source_len)
{
	unsigned char *start_bs = &frame_bs[startaddr];
	int index = 0;
	for(int i=0;i<bs_len;i+=Spacing){
		start_bs[i] = source[index];
		ASSERT(index != source_len);
		index++;
	}
	return 0;
}


unsigned long PsConversion::crc_prsn_t2_v1(int *crc_poly,int length_poly,int crc_seed,int length,unsigned char *out_ary)
{

	int poly_ord = crc_poly[length_poly-1];
	unsigned char *one_ary = new unsigned char[length_poly-1]; 
	unsigned char *temp_ary = new unsigned char[length_poly-1];
	unsigned char *seed_ary = new unsigned char[poly_ord]; //多項式最高次方
	unsigned char *crc_reg = new unsigned char[poly_ord];  
	unsigned char *add_ary = new unsigned char[poly_ord];
	unsigned char *add_ary1 = new unsigned char[poly_ord];
	memset(one_ary,1,length_poly-1);
	memset(temp_ary,1,length_poly-1);

	memset(add_ary,0,poly_ord);

	//for(int i=0;i<length_poly-1;i++){
	//	temp_ary[i] = crc_poly[i]+1;
	//}
	for(int i=0;i<length_poly-1;i++){
		add_ary[crc_poly[i]] = 0x01;
	}

	unsigned long tmp1 = crc_seed;
	for(int n=0;n<poly_ord;n++){  //把crc_seed轉成2進位放入seed_ary
		seed_ary[n] = (tmp1 % 2);
		tmp1  =  (unsigned long)floor((float)(tmp1/2));
	}
	int idx = 0;
	for(int i=poly_ord-1;i>=0;i--) {
		crc_reg[idx++] = seed_ary[i];
	}
	//memcpy(crc_reg , seed_ary,poly_ord);
	for(int n=0;n<length;n++){
		////mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2)
		//		out_ary[n]=crc_reg[poly_ord-1];//[0 crc_reg(1:poly_ord-1)]

		out_ary[n]=crc_reg[poly_ord-1];
		BYTE TempB = crc_reg[poly_ord-1];

		//memcpy(add_ary1,crc_reg,poly_ord);
		//-----------------------------------------------------------
		for(int i=0;i<poly_ord;i++){
			add_ary1[i] = add_ary[i] * crc_reg[poly_ord-1];// add_ary*crc_reg(poly_ord)
		}
#if 1
		memcpy(&crc_reg[1],&crc_reg[0],poly_ord-1); //向右shift一個bit
#endif
		crc_reg[0] = 0;//TempB; //前面補0
#if 0
		crc_reg[0] = TempB; //rotation
		for(int i=1;i<length_poly-1;i++){
			add_ary1[crc_poly[i]] ^= TempB;
		}
		for(int i=1;i<length_poly-1;i++){
			crc_reg[crc_poly[i]+1] = add_ary1[crc_poly[i]];
		}
#endif
		for(int i=0;i<poly_ord;i++){
			crc_reg[i] += add_ary1[i]; //mod([0 crc_reg(1:poly_ord-1)] + add_ary,2)
			crc_reg[i] = crc_reg[i] % 2;
		}
	}
	//crc_reg = mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2);	
	//add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);
	//	add_ary=zeros(1,poly_ord);
	//	add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);
#if 0
	%crc_poly = [from low order to high order]
	%EX:    polynomial = 1+ x^2 +x^5 + x^6 + x^9 +x^11 + x^14
		%EX: ==>  crc_poly = [0 2 5 6 9 11 14];
	%crc_seed = 'Hex data' : MSB ... LSB

		length_poly = length(crc_poly);
	poly_ord = crc_poly(length(crc_poly));

	add_ary=zeros(1,poly_ord);
	add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);

	%reg_log=zeros(length_output,poly_ord);

	%convert seed from hex to binary array
		%ex: 0x056c ==> [0 0 1 1 0 1 1 0 1 0 1 0 0] 13 bits seed
		seed_ary = zeros(1,poly_ord);
	tmp1=hex2dec(crc_seed);
	for n=1:poly_ord
		seed_ary(n) = mod(tmp1,2);
	tmp1=floor(tmp1/2);
	end


		crc_reg = seed_ary;
	for n=1:length_output;
	%reg_log(n,:)=crc_reg;
	output(n)=crc_reg(poly_ord);
	crc_reg = mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2);
	end
#endif

	delete[] add_ary;
	delete[] add_ary1;
	delete[] one_ary;
	delete[] temp_ary;
	delete[] seed_ary;
	delete[] crc_reg;

	return 0;
}

unsigned long PsConversion::crc_prsn_t2(int *crc_poly,int length_poly,int crc_seed,int length,unsigned char *out_ary)
{

	int poly_ord = crc_poly[length_poly-1];
	unsigned char *one_ary = new unsigned char[length_poly-1]; 
	unsigned char *temp_ary = new unsigned char[length_poly-1];
	unsigned char *seed_ary = new unsigned char[poly_ord]; //多項式最高次方
	unsigned char *crc_reg = new unsigned char[poly_ord];  
	unsigned char *add_ary = new unsigned char[poly_ord];
	unsigned char *add_ary1 = new unsigned char[poly_ord];
	memset(one_ary,1,length_poly-1);
	memset(temp_ary,1,length_poly-1);
	
	memset(add_ary,0,poly_ord);
	memset(seed_ary,0,poly_ord);

	//for(int i=0;i<length_poly-1;i++){
	//	temp_ary[i] = crc_poly[i]+1;
	//}
	for(int i=0;i<length_poly-1;i++){
		add_ary[crc_poly[i]] = 0x01;
	}

	unsigned long tmp1 = crc_seed;
	for(int n=0;n<poly_ord;n++){  //把crc_seed轉成2進位放入seed_ary
		if(((crc_seed >> n) & 0x01 )== 0x01) seed_ary[n] = 1;
		//seed_ary[n] = (tmp1 % 2);
		//tmp1  =  (unsigned long)floor((float)(tmp1/2));
	}
	int idx = 0;
	for(int i=poly_ord-1;i>=0;i--) {
		crc_reg[idx++] = seed_ary[i];
	}
//	memcpy(crc_reg , seed_ary,poly_ord);
	for(int n=0;n<length;n++){
		////mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2)
//		out_ary[n]=crc_reg[poly_ord-1];//[0 crc_reg(1:poly_ord-1)]

		//ASSERT(n<=492);
		out_ary[n]=crc_reg[poly_ord-1];
		BYTE TempB = crc_reg[poly_ord-1];

		memcpy(add_ary1,crc_reg,poly_ord);
		
		//-----------------------------------------------------------
		//for(int i=0;i<poly_ord;i++){
		//	add_ary1[i] = add_ary[i] * crc_reg[poly_ord-1];// add_ary*crc_reg(poly_ord)
		//}
#if 1
		memcpy(&crc_reg[1],&crc_reg[0],poly_ord-1); //向右shift一個bit
		//memcpy(add_ary1,crc_reg,poly_ord);
#endif
	//	memcpy(&crc_reg[0],&crc_reg[1],poly_ord-1); //向右shift一個bit
		//crc_reg[0] = 0;//TempB; //前面補0
		crc_reg[0] = TempB; //rotation
		for(int i=1;i<length_poly-1;i++){
			/*int maxlen = crc_poly[length_poly-1]-1;
			int ds= crc_poly[i];*/
			//0,1,3,4,13
			//add_ary1[maxlen-ds] ^= TempB;
			add_ary1[crc_poly[i]-1] ^= TempB;
		}
		for(int i=1;i<length_poly-1;i++){
			crc_reg[crc_poly[i]] = add_ary1[crc_poly[i]-1];
		}

		//for(int i=0;i<poly_ord;i++){
		//	crc_reg[i] += add_ary1[i]; //mod([0 crc_reg(1:poly_ord-1)] + add_ary,2)
		//	crc_reg[i] = crc_reg[i] % 2;
		//}
	}
	//crc_reg = mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2);	
	//add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);
//	add_ary=zeros(1,poly_ord);
//	add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);
#if 0
	%crc_poly = [from low order to high order]
	%EX:    polynomial = 1+ x^2 +x^5 + x^6 + x^9 +x^11 + x^14
		%EX: ==>  crc_poly = [0 2 5 6 9 11 14];
	%crc_seed = 'Hex data' : MSB ... LSB

	length_poly = length(crc_poly);
	poly_ord = crc_poly(length(crc_poly));

	add_ary=zeros(1,poly_ord);
	add_ary(crc_poly(1:length_poly-1)+1)=ones(1,length_poly-1);

	%reg_log=zeros(length_output,poly_ord);

	%convert seed from hex to binary array
		%ex: 0x056c ==> [0 0 1 1 0 1 1 0 1 0 1 0 0] 13 bits seed
		seed_ary = zeros(1,poly_ord);
	tmp1=hex2dec(crc_seed);
	for n=1:poly_ord
		seed_ary(n) = mod(tmp1,2);
	tmp1=floor(tmp1/2);
	end


		crc_reg = seed_ary;
	for n=1:length_output;
	%reg_log(n,:)=crc_reg;
	output(n)=crc_reg(poly_ord);
	crc_reg = mod([0 crc_reg(1:poly_ord-1)] + add_ary*crc_reg(poly_ord),2);
	end
#endif

	delete[] add_ary;
	delete[] add_ary1;
	delete[] one_ary;
	delete[] temp_ary;
	delete[] seed_ary;
	delete[] crc_reg;

	return 0;
}

unsigned long PsConversion::seed_rotate( unsigned long crc_seed , unsigned long rot_no , unsigned long poly_ord )
{

	unsigned long tmp1 = crc_seed;
	unsigned char *seed_ary = new unsigned char[poly_ord];
	memset(seed_ary,0,poly_ord);

	//ULONG tmp5 = 0x0A;
	/*for(int n=0;n<poly_ord;n++){
		seed_ary[n] = (tmp5 % 2);
		tmp5  =  (unsigned long)floor((float)(tmp5/2));
	}*/

	/*for(int n=0;n<poly_ord;n++){
		seed_ary[n] = (tmp1 % 2);
		tmp1  =  (unsigned long)floor((float)(tmp1/2));
	}*/

	for(unsigned int n=0;n<poly_ord;n++){  //把crc_seed轉成2進位放入seed_ary
		if(((crc_seed >> n) & 0x01 )== 0x01) seed_ary[n] = 1;
		//seed_ary[n] = (tmp1 % 2);
		//tmp1  =  (unsigned long)floor((float)(tmp1/2));
	}


	for(unsigned int i=0;i<rot_no;i++){
		unsigned char  tempv = seed_ary[0];
		memcpy(&seed_ary[0],&seed_ary[1],poly_ord-1);
		seed_ary[poly_ord-1] = tempv;
	}

	ULONG tmp2=0;

	/*int shiftbit = 7;
	for(int n=0;n<8;n++){
		if(_block_rdm_bs[n]) tmp2 |= 0x01 << shiftbit;
		shiftbit--;
	}*/

	for(int n=poly_ord-1;n>=0;n--){
		if(seed_ary[n]) tmp2 |= 0x01 << n;
		//tmp2 = tmp2 << 1;
	}

	delete[] seed_ary;

	return tmp2;
}