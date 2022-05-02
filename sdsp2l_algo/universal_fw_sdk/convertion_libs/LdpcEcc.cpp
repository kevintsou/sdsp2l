// LdpcEcc.cpp : implementation file
//
#ifdef WIN32
#include "windows.h"
#endif

#include <math.h>
#include "LdpcEcc.h"
#include "PsConversion.h"
#include <stdio.h>
//#include "synchapi.h"
//#include "winbase.h"

//#include <iostream>

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif
#define BITMSK(LENGTH, OFFSET)      ((~((~0ULL)<<(LENGTH)))<<(OFFSET))

//int ldpc5013_encode(const unsigned char* data, int datalen, unsigned char* buf, int buflen, const char* LdpcMode, int fwmode );



//extern	char				m_MainPath[];
/////////////////////////////////////////////////////////////////////////////
// LdpcEcc dialog

#ifdef WIN32
static CRITICAL_SECTION g_cs_lock;
static volatile bool g_is_cs_init = 0;
#endif
LdpcEcc::LdpcEcc(int controller)
{
#ifdef WIN32
	m_current_ldpc5013 = NULL;
	m_current_ldpc5017 = NULL;
	m_current_ldpc5019 = NULL;
	if (0 == g_is_cs_init) {
		g_is_cs_init = 1;
		::InitializeCriticalSection(&g_cs_lock);
	}
	m_controller = controller;
#else
	assert(0);//ToDo: Linux need to take care critical section.
#endif
}

LdpcEcc::~LdpcEcc()
{
	if (m_current_ldpc5013) {
		delete m_current_ldpc5013;
		m_current_ldpc5013 = NULL;
	}
	if (m_current_ldpc5017) {
		delete m_current_ldpc5017;
		m_current_ldpc5017 = NULL;
	}
	if (m_current_ldpc5019) {
		delete m_current_ldpc5019;
		m_current_ldpc5019 = NULL;
	}
}


/////////////////////////////////////////////////////////////////////////////

void LdpcEcc::convertionBack(unsigned char *buf, unsigned char *Outbuf, int len, int FlhpageSizeK)
{
	int pagesizeK = 0;

	if (16 == FlhpageSizeK) {
		pagesizeK = (4096 + 16) * 4;
	}
	else if (8 == FlhpageSizeK) {
		pagesizeK = (4096 + 16) * 2;
	}

	unsigned char * TargetBuf = new unsigned char[0x4800];
	unsigned char * temp = new unsigned char[0x4800];
	memcpy(TargetBuf, buf, 2048);
	memcpy(TargetBuf + 2048, buf + 2368, 2048);
	memcpy(TargetBuf + 4096, buf + 2048, 8);
	memcpy(TargetBuf + 4104, buf + 4416, 8);

	NPsConversion PsConv;
	//---------Do conversion

	int pg = 0;
	int datasize = 0;
	unsigned int framPerPage = pagesizeK / (4096 + 16);

	int cellLen = pagesizeK * 8;
	PsConv.SetBlockSeed(m_BlockSeed, (0x5013 == m_controller) ? true : false);
	PsConv.seed_selector4(cellLen, 0, m_PageSeed, 0, framPerPage, 0);	// E13 no turn on ed3Conv
	//memset(TargetBuf, 0, pagesizeK);

	PsConv.MakeConv(TargetBuf, temp, pagesizeK);
	//PsConv.MakeConv(temp, TargetBuf, pagesizeK);
	memcpy(Outbuf, temp, len);
}

ldpc5013 * LdpcEcc::CreateLdpc5013(int mode)
{
	if (!m_current_ldpc5013 || (m_current_ldpc_mode != mode))
	{
		char ldpcmode[512];
		sprintf_s(ldpcmode,"2k_mode%d",mode);
		if (m_current_ldpc5013)
			delete m_current_ldpc5013;
		m_current_ldpc5013 = new ldpc5013(ldpcmode, 0);
		m_current_ldpc_mode = mode;

	}
	return m_current_ldpc5013;
}

ldpc5017 * LdpcEcc::CreateLdpc5017(int mode)
{
	if (!m_current_ldpc5017 || (m_current_ldpc_mode != mode))
	{
		char ldpcmode[512];
		sprintf_s(ldpcmode,"2k_mode%d",mode);
		if (m_current_ldpc5017)
			delete m_current_ldpc5017;
		m_current_ldpc5017 = new ldpc5017(ldpcmode, 0);
		m_current_ldpc_mode = mode;

	}
	return m_current_ldpc5017;
}

ldpc5019 * LdpcEcc::CreateLdpc5019(int mode)
{
	if (!m_current_ldpc5019 || (m_current_ldpc_mode != mode))
	{
		char ldpcmode[512];
		sprintf_s(ldpcmode,"2k_mode%d",mode);
		if (m_current_ldpc5019)
			delete m_current_ldpc5019;
		m_current_ldpc5019 = new ldpc5019(ldpcmode, 0);
		m_current_ldpc_mode = mode;

	}
	return m_current_ldpc5019;
}

void LdpcEcc::GenRAWData(unsigned char *data, unsigned int DataLen, int FlhpageSizeK)
{
	// TODO: Add your control notification handler code here
	unsigned int i;


	// code form kelvin

	int len1=0,len2=0;

	int pagesizeK = 0;

	if (16 == FlhpageSizeK) {
		pagesizeK = (4096+16) * 4;
	} else if (8 == FlhpageSizeK) {
		pagesizeK = (4096+16) * 2;
	}

    unsigned int framPerPage = pagesizeK/(4096+16);
	unsigned char *TargetBuf = new unsigned char[pagesizeK];
	unsigned char tmpdata[4112];
	
	// combin data to 4k+16

	// format 1
	// 16k + sparex4
	memcpy(TargetBuf,data,pagesizeK);
	memset(data,0x00,pagesizeK);
	for(i=0;i<DataLen/(4096+16);i++)
	{
		memcpy(&data[i*4112],&TargetBuf[i*4096],4096);
		memcpy(&data[(i*4112)+4096],&TargetBuf[FlhpageSizeK*1024 + (i*16)],16);
	}
	// chagne to format2
	// format 2
	// 4k+spare
	for(i=0;i<framPerPage;i++)
	{
		memcpy(tmpdata,&data[i*4112],4112);
/*
		// for pattern 0 test
		memset(tmpdata,0x00,4096+16);

		tmpdata[4096+7]=0xff;
		if(i==0)
			tmpdata[4096+13]=0xF0;
		else if(i==1)
			tmpdata[4096+13]=0xC0;
		else if(i==2)
			tmpdata[4096+13]=0x90;
		else if(i==3)
			tmpdata[4096+13]=0xA0;
		// === End test
*/
		tmpdata[4096+14]=0;
		tmpdata[4096+15]=0;	
		flashIP_CRC16( tmpdata, &tmpdata[4096+14] );
		memcpy(&data[i*4112],tmpdata,4112);	
	}
	
	// conversion
	NPsConversion PsConv;
	//---------Do conversion




	int pg=0;
	int datasize = 0;


	int cellLen = pagesizeK*8;
	PsConv.SetBlockSeed(m_BlockSeed, (0x5013 == m_controller)?true:false);
	PsConv.seed_selector4(cellLen,0,m_PageSeed,0,framPerPage,0);	// E13 no turn on ed3Conv
	memset(TargetBuf,0,pagesizeK);
	
	PsConv.MakeConv(data,TargetBuf,pagesizeK);


	//const int framelen=4096, sparelen=8
	// E13 spare 14 byte , CRC 2 byte
	const int framelen=4096, sparelen=16;	
	int paritylen = 0;
	switch(m_LDPCMode)
	{
		case 0:
			paritylen = 256;
			break;
		case 1:
			paritylen = 240;
			break;
		case 2:
			paritylen = 224;
			break;
		case 3:
			paritylen = 208;
			break;
		case 4:
			paritylen = 176;
			break;
		case 5:
			paritylen = 144;
			break;
		case 6:
			paritylen = 128;
			break;
		case 7:
			paritylen = 304;
			break;
		case 8:
			paritylen = 288;
			break;
	}
	paritylen*=2;
	paritylen += 16;	// CRC 64
	int offset=0;

#ifdef WIN32
	EnterCriticalSection(&g_cs_lock);
#endif
	ldpc5013 * templdpc_5013 = NULL;      //CreateLdpc5013(m_LDPCMode);
	ldpc5017 * templdpc_5017 = NULL;
	ldpc5019 * templdpc_5019 = NULL;

	if (0x5013 == m_controller) {
		templdpc_5013 = CreateLdpc5013(m_LDPCMode);
	}
	else if (0x5017 == m_controller) {
		templdpc_5017 = CreateLdpc5017(m_LDPCMode);
	}
	else if (0x5019 == m_controller) {
		templdpc_5019 = CreateLdpc5019(m_LDPCMode);
	}
	else
	{
		//ASSERT(0);
	}

	for(i=0;i<framPerPage;i++)
	{
		if(i<(DataLen/4112))
		{
			if (0x5013 == m_controller) {
				pageToFrame_5013(templdpc_5013, TargetBuf, i, framelen, sparelen, paritylen, data );
			}
			else if (0x5017 == m_controller) {
				pageToFrame_5017(templdpc_5017, TargetBuf, i, framelen, sparelen, paritylen, data );
			}
			else if (0x5019 == m_controller) {
				pageToFrame_5019(templdpc_5019, TargetBuf, i, framelen, sparelen, paritylen, data );
			}
		}
		else
		{
			memcpy(&data[i*(framelen+sparelen+paritylen)],&TargetBuf[i*(framelen+sparelen)],(pagesizeK-(i*(framelen+sparelen))));
		}
		//m
	}
#ifdef WIN32
	LeaveCriticalSection(&g_cs_lock);
#endif
	delete []TargetBuf;
	
}
bool LdpcEcc::flashIP_CRC16_Loop( const unsigned char* din, const unsigned char* c, unsigned char* CRC_OUT_16 )
{
	unsigned char DIN_TMP[128];
	//byte to bit
	for ( int i=0;i<16;i++) {
		unsigned char uc = din[i];
		for ( int j=0;j<8;j++) {
              DIN_TMP[i*8+j] = (uc>>j)&0x01;
		}
	}

     //crc
    CRC_OUT_16[0] = c[8] ^ c[9] ^ c[10] ^ c[11] ^ c[12] ^ c[13] ^ c[15] ^ DIN_TMP[0] ^ DIN_TMP[1] ^ DIN_TMP[2] ^ DIN_TMP[3] ^ DIN_TMP[4] ^ DIN_TMP[5] ^ DIN_TMP[6] ^ DIN_TMP[7] ^ DIN_TMP[8] ^ DIN_TMP[9] ^ DIN_TMP[10] ^ DIN_TMP[11] ^ DIN_TMP[12] ^ DIN_TMP[13] ^ DIN_TMP[15] ^ DIN_TMP[16] ^ DIN_TMP[17] ^ DIN_TMP[18] ^ DIN_TMP[19] ^ DIN_TMP[20] ^ DIN_TMP[21] ^ DIN_TMP[22] ^ DIN_TMP[23] ^ DIN_TMP[24] ^ DIN_TMP[25] ^ DIN_TMP[26] ^ DIN_TMP[27] ^ DIN_TMP[30] ^ DIN_TMP[31] ^ DIN_TMP[32] ^ DIN_TMP[33] ^ DIN_TMP[34] ^ DIN_TMP[35] ^ DIN_TMP[36] ^ DIN_TMP[37] ^ DIN_TMP[38] ^ DIN_TMP[39] ^ DIN_TMP[40] ^ DIN_TMP[41] ^ DIN_TMP[43] ^ DIN_TMP[45] ^ DIN_TMP[46] ^ DIN_TMP[47] ^ DIN_TMP[48] ^ DIN_TMP[49] ^ DIN_TMP[50] ^ DIN_TMP[51] ^ DIN_TMP[52] ^ DIN_TMP[53] ^ DIN_TMP[54] ^ DIN_TMP[55] ^ DIN_TMP[60] ^ DIN_TMP[61] ^ DIN_TMP[62] ^ DIN_TMP[63] ^ DIN_TMP[64] ^ DIN_TMP[65] ^ DIN_TMP[66] ^ DIN_TMP[67] ^ DIN_TMP[68] ^ DIN_TMP[69] ^ DIN_TMP[71] ^ DIN_TMP[72] ^ DIN_TMP[73] ^ DIN_TMP[75] ^ DIN_TMP[76] ^ DIN_TMP[77] ^ DIN_TMP[78] ^ DIN_TMP[79] ^ DIN_TMP[80] ^ DIN_TMP[81] ^ DIN_TMP[82] ^ DIN_TMP[83] ^ DIN_TMP[86] ^ DIN_TMP[87] ^ DIN_TMP[90] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[97] ^ DIN_TMP[99] ^ DIN_TMP[101] ^ DIN_TMP[103] ^ DIN_TMP[105] ^ DIN_TMP[106] ^ DIN_TMP[107] ^ DIN_TMP[108] ^ DIN_TMP[109] ^ DIN_TMP[110] ^ DIN_TMP[111] ^ DIN_TMP[120] ^ DIN_TMP[121] ^ DIN_TMP[122] ^ DIN_TMP[123] ^ DIN_TMP[124] ^ DIN_TMP[125] ^ DIN_TMP[127];
    CRC_OUT_16[1] = c[0] ^ c[9] ^ c[10] ^ c[11] ^ c[12] ^ c[13] ^ c[14] ^ DIN_TMP[1] ^ DIN_TMP[2] ^ DIN_TMP[3] ^ DIN_TMP[4] ^ DIN_TMP[5] ^ DIN_TMP[6] ^ DIN_TMP[7] ^ DIN_TMP[8] ^ DIN_TMP[9] ^ DIN_TMP[10] ^ DIN_TMP[11] ^ DIN_TMP[12] ^ DIN_TMP[13] ^ DIN_TMP[14] ^ DIN_TMP[16] ^ DIN_TMP[17] ^ DIN_TMP[18] ^ DIN_TMP[19] ^ DIN_TMP[20] ^ DIN_TMP[21] ^ DIN_TMP[22] ^ DIN_TMP[23] ^ DIN_TMP[24] ^ DIN_TMP[25] ^ DIN_TMP[26] ^ DIN_TMP[27] ^ DIN_TMP[28] ^ DIN_TMP[31] ^ DIN_TMP[32] ^ DIN_TMP[33] ^ DIN_TMP[34] ^ DIN_TMP[35] ^ DIN_TMP[36] ^ DIN_TMP[37] ^ DIN_TMP[38] ^ DIN_TMP[39] ^ DIN_TMP[40] ^ DIN_TMP[41] ^ DIN_TMP[42] ^ DIN_TMP[44] ^ DIN_TMP[46] ^ DIN_TMP[47] ^ DIN_TMP[48] ^ DIN_TMP[49] ^ DIN_TMP[50] ^ DIN_TMP[51] ^ DIN_TMP[52] ^ DIN_TMP[53] ^ DIN_TMP[54] ^ DIN_TMP[55] ^ DIN_TMP[56] ^ DIN_TMP[61] ^ DIN_TMP[62] ^ DIN_TMP[63] ^ DIN_TMP[64] ^ DIN_TMP[65] ^ DIN_TMP[66] ^ DIN_TMP[67] ^ DIN_TMP[68] ^ DIN_TMP[69] ^ DIN_TMP[70] ^ DIN_TMP[72] ^ DIN_TMP[73] ^ DIN_TMP[74] ^ DIN_TMP[76] ^ DIN_TMP[77] ^ DIN_TMP[78] ^ DIN_TMP[79] ^ DIN_TMP[80] ^ DIN_TMP[81] ^ DIN_TMP[82] ^ DIN_TMP[83] ^ DIN_TMP[84] ^ DIN_TMP[87] ^ DIN_TMP[88] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[97] ^ DIN_TMP[98] ^ DIN_TMP[100] ^ DIN_TMP[102] ^ DIN_TMP[104] ^ DIN_TMP[106] ^ DIN_TMP[107] ^ DIN_TMP[108] ^ DIN_TMP[109] ^ DIN_TMP[110] ^ DIN_TMP[111] ^ DIN_TMP[112] ^ DIN_TMP[121] ^ DIN_TMP[122] ^ DIN_TMP[123] ^ DIN_TMP[124] ^ DIN_TMP[125] ^ DIN_TMP[126];
    CRC_OUT_16[2] = c[0] ^ c[1] ^ c[8] ^ c[9] ^ c[14] ^ DIN_TMP[0] ^ DIN_TMP[1] ^ DIN_TMP[14] ^ DIN_TMP[16] ^ DIN_TMP[28] ^ DIN_TMP[29] ^ DIN_TMP[30] ^ DIN_TMP[31] ^ DIN_TMP[42] ^ DIN_TMP[46] ^ DIN_TMP[56] ^ DIN_TMP[57] ^ DIN_TMP[60] ^ DIN_TMP[61] ^ DIN_TMP[70] ^ DIN_TMP[72] ^ DIN_TMP[74] ^ DIN_TMP[76] ^ DIN_TMP[84] ^ DIN_TMP[85] ^ DIN_TMP[86] ^ DIN_TMP[87] ^ DIN_TMP[88] ^ DIN_TMP[89] ^ DIN_TMP[90] ^ DIN_TMP[91] ^ DIN_TMP[98] ^ DIN_TMP[106] ^ DIN_TMP[112] ^ DIN_TMP[113] ^ DIN_TMP[120] ^ DIN_TMP[121] ^ DIN_TMP[126];
    CRC_OUT_16[3] = c[1] ^ c[2] ^ c[9] ^ c[10] ^ c[15] ^ DIN_TMP[1] ^ DIN_TMP[2] ^ DIN_TMP[15] ^ DIN_TMP[17] ^ DIN_TMP[29] ^ DIN_TMP[30] ^ DIN_TMP[31] ^ DIN_TMP[32] ^ DIN_TMP[43] ^ DIN_TMP[47] ^ DIN_TMP[57] ^ DIN_TMP[58] ^ DIN_TMP[61] ^ DIN_TMP[62] ^ DIN_TMP[71] ^ DIN_TMP[73] ^ DIN_TMP[75] ^ DIN_TMP[77] ^ DIN_TMP[85] ^ DIN_TMP[86] ^ DIN_TMP[87] ^ DIN_TMP[88] ^ DIN_TMP[89] ^ DIN_TMP[90] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[99] ^ DIN_TMP[107] ^ DIN_TMP[113] ^ DIN_TMP[114] ^ DIN_TMP[121] ^ DIN_TMP[122] ^ DIN_TMP[127];
    CRC_OUT_16[4] = c[2] ^ c[3] ^ c[10] ^ c[11] ^ DIN_TMP[2] ^ DIN_TMP[3] ^ DIN_TMP[16] ^ DIN_TMP[18] ^ DIN_TMP[30] ^ DIN_TMP[31] ^ DIN_TMP[32] ^ DIN_TMP[33] ^ DIN_TMP[44] ^ DIN_TMP[48] ^ DIN_TMP[58] ^ DIN_TMP[59] ^ DIN_TMP[62] ^ DIN_TMP[63] ^ DIN_TMP[72] ^ DIN_TMP[74] ^ DIN_TMP[76] ^ DIN_TMP[78] ^ DIN_TMP[86] ^ DIN_TMP[87] ^ DIN_TMP[88] ^ DIN_TMP[89] ^ DIN_TMP[90] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[100] ^ DIN_TMP[108] ^ DIN_TMP[114] ^ DIN_TMP[115] ^ DIN_TMP[122] ^ DIN_TMP[123];
    CRC_OUT_16[5] = c[3] ^ c[4] ^ c[11] ^ c[12] ^ DIN_TMP[3] ^ DIN_TMP[4] ^ DIN_TMP[17] ^ DIN_TMP[19] ^ DIN_TMP[31] ^ DIN_TMP[32] ^ DIN_TMP[33] ^ DIN_TMP[34] ^ DIN_TMP[45] ^ DIN_TMP[49] ^ DIN_TMP[59] ^ DIN_TMP[60] ^ DIN_TMP[63] ^ DIN_TMP[64] ^ DIN_TMP[73] ^ DIN_TMP[75] ^ DIN_TMP[77] ^ DIN_TMP[79] ^ DIN_TMP[87] ^ DIN_TMP[88] ^ DIN_TMP[89] ^ DIN_TMP[90] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[101] ^ DIN_TMP[109] ^ DIN_TMP[115] ^ DIN_TMP[116] ^ DIN_TMP[123] ^ DIN_TMP[124];
    CRC_OUT_16[6] = c[4] ^ c[5] ^ c[12] ^ c[13] ^ DIN_TMP[4] ^ DIN_TMP[5] ^ DIN_TMP[18] ^ DIN_TMP[20] ^ DIN_TMP[32] ^ DIN_TMP[33] ^ DIN_TMP[34] ^ DIN_TMP[35] ^ DIN_TMP[46] ^ DIN_TMP[50] ^ DIN_TMP[60] ^ DIN_TMP[61] ^ DIN_TMP[64] ^ DIN_TMP[65] ^ DIN_TMP[74] ^ DIN_TMP[76] ^ DIN_TMP[78] ^ DIN_TMP[80] ^ DIN_TMP[88] ^ DIN_TMP[89] ^ DIN_TMP[90] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[102] ^ DIN_TMP[110] ^ DIN_TMP[116] ^ DIN_TMP[117] ^ DIN_TMP[124] ^ DIN_TMP[125];
    CRC_OUT_16[7] = c[5] ^ c[6] ^ c[13] ^ c[14] ^ DIN_TMP[5] ^ DIN_TMP[6] ^ DIN_TMP[19] ^ DIN_TMP[21] ^ DIN_TMP[33] ^ DIN_TMP[34] ^ DIN_TMP[35] ^ DIN_TMP[36] ^ DIN_TMP[47] ^ DIN_TMP[51] ^ DIN_TMP[61] ^ DIN_TMP[62] ^ DIN_TMP[65] ^ DIN_TMP[66] ^ DIN_TMP[75] ^ DIN_TMP[77] ^ DIN_TMP[79] ^ DIN_TMP[81] ^ DIN_TMP[89] ^ DIN_TMP[90] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[103] ^ DIN_TMP[111] ^ DIN_TMP[117] ^ DIN_TMP[118] ^ DIN_TMP[125] ^ DIN_TMP[126];
    CRC_OUT_16[8] = c[0] ^ c[6] ^ c[7] ^ c[14] ^ c[15] ^ DIN_TMP[6] ^ DIN_TMP[7] ^ DIN_TMP[20] ^ DIN_TMP[22] ^ DIN_TMP[34] ^ DIN_TMP[35] ^ DIN_TMP[36] ^ DIN_TMP[37] ^ DIN_TMP[48] ^ DIN_TMP[52] ^ DIN_TMP[62] ^ DIN_TMP[63] ^ DIN_TMP[66] ^ DIN_TMP[67] ^ DIN_TMP[76] ^ DIN_TMP[78] ^ DIN_TMP[80] ^ DIN_TMP[82] ^ DIN_TMP[90] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[97] ^ DIN_TMP[104] ^ DIN_TMP[112] ^ DIN_TMP[118] ^ DIN_TMP[119] ^ DIN_TMP[126] ^ DIN_TMP[127];
    CRC_OUT_16[9] = c[1] ^ c[7] ^ c[8] ^ c[15] ^ DIN_TMP[7] ^ DIN_TMP[8] ^ DIN_TMP[21] ^ DIN_TMP[23] ^ DIN_TMP[35] ^ DIN_TMP[36] ^ DIN_TMP[37] ^ DIN_TMP[38] ^ DIN_TMP[49] ^ DIN_TMP[53] ^ DIN_TMP[63] ^ DIN_TMP[64] ^ DIN_TMP[67] ^ DIN_TMP[68] ^ DIN_TMP[77] ^ DIN_TMP[79] ^ DIN_TMP[81] ^ DIN_TMP[83] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[97] ^ DIN_TMP[98] ^ DIN_TMP[105] ^ DIN_TMP[113] ^ DIN_TMP[119] ^ DIN_TMP[120] ^ DIN_TMP[127];
    CRC_OUT_16[10] = c[2] ^ c[8] ^ c[9] ^ DIN_TMP[8] ^ DIN_TMP[9] ^ DIN_TMP[22] ^ DIN_TMP[24] ^ DIN_TMP[36] ^ DIN_TMP[37] ^ DIN_TMP[38] ^ DIN_TMP[39] ^ DIN_TMP[50] ^ DIN_TMP[54] ^ DIN_TMP[64] ^ DIN_TMP[65] ^ DIN_TMP[68] ^ DIN_TMP[69] ^ DIN_TMP[78] ^ DIN_TMP[80] ^ DIN_TMP[82] ^ DIN_TMP[84] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[97] ^ DIN_TMP[98] ^ DIN_TMP[99] ^ DIN_TMP[106] ^ DIN_TMP[114] ^ DIN_TMP[120] ^ DIN_TMP[121];
    CRC_OUT_16[11] = c[3] ^ c[9] ^ c[10] ^ DIN_TMP[9] ^ DIN_TMP[10] ^ DIN_TMP[23] ^ DIN_TMP[25] ^ DIN_TMP[37] ^ DIN_TMP[38] ^ DIN_TMP[39] ^ DIN_TMP[40] ^ DIN_TMP[51] ^ DIN_TMP[55] ^ DIN_TMP[65] ^ DIN_TMP[66] ^ DIN_TMP[69] ^ DIN_TMP[70] ^ DIN_TMP[79] ^ DIN_TMP[81] ^ DIN_TMP[83] ^ DIN_TMP[85] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[97] ^ DIN_TMP[98] ^ DIN_TMP[99] ^ DIN_TMP[100] ^ DIN_TMP[107] ^ DIN_TMP[115] ^ DIN_TMP[121] ^ DIN_TMP[122];
    CRC_OUT_16[12] = c[4] ^ c[10] ^ c[11] ^ DIN_TMP[10] ^ DIN_TMP[11] ^ DIN_TMP[24] ^ DIN_TMP[26] ^ DIN_TMP[38] ^ DIN_TMP[39] ^ DIN_TMP[40] ^ DIN_TMP[41] ^ DIN_TMP[52] ^ DIN_TMP[56] ^ DIN_TMP[66] ^ DIN_TMP[67] ^ DIN_TMP[70] ^ DIN_TMP[71] ^ DIN_TMP[80] ^ DIN_TMP[82] ^ DIN_TMP[84] ^ DIN_TMP[86] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[97] ^ DIN_TMP[98] ^ DIN_TMP[99] ^ DIN_TMP[100] ^ DIN_TMP[101] ^ DIN_TMP[108] ^ DIN_TMP[116] ^ DIN_TMP[122] ^ DIN_TMP[123];
    CRC_OUT_16[13] = c[5] ^ c[11] ^ c[12] ^ DIN_TMP[11] ^ DIN_TMP[12] ^ DIN_TMP[25] ^ DIN_TMP[27] ^ DIN_TMP[39] ^ DIN_TMP[40] ^ DIN_TMP[41] ^ DIN_TMP[42] ^ DIN_TMP[53] ^ DIN_TMP[57] ^ DIN_TMP[67] ^ DIN_TMP[68] ^ DIN_TMP[71] ^ DIN_TMP[72] ^ DIN_TMP[81] ^ DIN_TMP[83] ^ DIN_TMP[85] ^ DIN_TMP[87] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[97] ^ DIN_TMP[98] ^ DIN_TMP[99] ^ DIN_TMP[100] ^ DIN_TMP[101] ^ DIN_TMP[102] ^ DIN_TMP[109] ^ DIN_TMP[117] ^ DIN_TMP[123] ^ DIN_TMP[124];
    CRC_OUT_16[14] = c[6] ^ c[12] ^ c[13] ^ DIN_TMP[12] ^ DIN_TMP[13] ^ DIN_TMP[26] ^ DIN_TMP[28] ^ DIN_TMP[40] ^ DIN_TMP[41] ^ DIN_TMP[42] ^ DIN_TMP[43] ^ DIN_TMP[54] ^ DIN_TMP[58] ^ DIN_TMP[68] ^ DIN_TMP[69] ^ DIN_TMP[72] ^ DIN_TMP[73] ^ DIN_TMP[82] ^ DIN_TMP[84] ^ DIN_TMP[86] ^ DIN_TMP[88] ^ DIN_TMP[96] ^ DIN_TMP[97] ^ DIN_TMP[98] ^ DIN_TMP[99] ^ DIN_TMP[100] ^ DIN_TMP[101] ^ DIN_TMP[102] ^ DIN_TMP[103] ^ DIN_TMP[110] ^ DIN_TMP[118] ^ DIN_TMP[124] ^ DIN_TMP[125];
    CRC_OUT_16[15] = c[7] ^ c[8] ^ c[9] ^ c[10] ^ c[11] ^ c[12] ^ c[14] ^ c[15] ^ DIN_TMP[0] ^ DIN_TMP[1] ^ DIN_TMP[2] ^ DIN_TMP[3] ^ DIN_TMP[4] ^ DIN_TMP[5] ^ DIN_TMP[6] ^ DIN_TMP[7] ^ DIN_TMP[8] ^ DIN_TMP[9] ^ DIN_TMP[10] ^ DIN_TMP[11] ^ DIN_TMP[12] ^ DIN_TMP[14] ^ DIN_TMP[15] ^ DIN_TMP[16] ^ DIN_TMP[17] ^ DIN_TMP[18] ^ DIN_TMP[19] ^ DIN_TMP[20] ^ DIN_TMP[21] ^ DIN_TMP[22] ^ DIN_TMP[23] ^ DIN_TMP[24] ^ DIN_TMP[25] ^ DIN_TMP[26] ^ DIN_TMP[29] ^ DIN_TMP[30] ^ DIN_TMP[31] ^ DIN_TMP[32] ^ DIN_TMP[33] ^ DIN_TMP[34] ^ DIN_TMP[35] ^ DIN_TMP[36] ^ DIN_TMP[37] ^ DIN_TMP[38] ^ DIN_TMP[39] ^ DIN_TMP[40] ^ DIN_TMP[42] ^ DIN_TMP[44] ^ DIN_TMP[45] ^ DIN_TMP[46] ^ DIN_TMP[47] ^ DIN_TMP[48] ^ DIN_TMP[49] ^ DIN_TMP[50] ^ DIN_TMP[51] ^ DIN_TMP[52] ^ DIN_TMP[53] ^ DIN_TMP[54] ^ DIN_TMP[59] ^ DIN_TMP[60] ^ DIN_TMP[61] ^ DIN_TMP[62] ^ DIN_TMP[63] ^ DIN_TMP[64] ^ DIN_TMP[65] ^ DIN_TMP[66] ^ DIN_TMP[67] ^ DIN_TMP[68] ^ DIN_TMP[70] ^ DIN_TMP[71] ^ DIN_TMP[72] ^ DIN_TMP[74] ^ DIN_TMP[75] ^ DIN_TMP[76] ^ DIN_TMP[77] ^ DIN_TMP[78] ^ DIN_TMP[79] ^ DIN_TMP[80] ^ DIN_TMP[81] ^ DIN_TMP[82] ^ DIN_TMP[85] ^ DIN_TMP[86] ^ DIN_TMP[89] ^ DIN_TMP[90] ^ DIN_TMP[91] ^ DIN_TMP[92] ^ DIN_TMP[93] ^ DIN_TMP[94] ^ DIN_TMP[95] ^ DIN_TMP[96] ^ DIN_TMP[98] ^ DIN_TMP[100] ^ DIN_TMP[102] ^ DIN_TMP[104] ^ DIN_TMP[105] ^ DIN_TMP[106] ^ DIN_TMP[107] ^ DIN_TMP[108] ^ DIN_TMP[109] ^ DIN_TMP[110] ^ DIN_TMP[119] ^ DIN_TMP[120] ^ DIN_TMP[121] ^ DIN_TMP[122] ^ DIN_TMP[123] ^ DIN_TMP[124] ^ DIN_TMP[126] ^ DIN_TMP[127];

    return true;
}


const int datalen =4096+16;// ==4096+13+0+0+0 , 13 byte is FW spare.
//dout = unsigned char[2];
bool LdpcEcc::flashIP_CRC16( const unsigned char* din, unsigned char* dout )
{
	int i,j;
	unsigned char ctmp[16] ={1,1,1,1,1,1,1,1,
                                1,1,1,1,1,1,1,1};
	unsigned char dout2[16];
	for ( i=0;i<datalen;i+=16) {
		if ( ! flashIP_CRC16_Loop(&din[i],ctmp,dout2) ) {
			//assert(0);
			return false;
		}
		memcpy(ctmp,dout2,16);
	}
	for (i=0;i<2;i++) {
		unsigned char uc =0;
		for ( j=0;j<8;j++) {
			uc |= (dout2[i*8+j]<<j);
		}
		dout[i] = uc;
	}
	return true;
}


// page 4K data split to 1/2 data+1/2 p4k + 1/2 parity 

int LdpcEcc::pageToFrame_5013(ldpc5013* pldpc5013, const unsigned char* pageBuf, int frameidx, int framelen, int sparelen, int paritylen, unsigned char* data )
{
	//assert( ((framelen&0x01)==0) && ((sparelen&0x01)==0) && ((paritylen&0x01)==0) );
	int spare2 = sparelen/2;
	int parity2 = paritylen/2;
	int frame2 = framelen/2;
	int offsetI = frameidx*(framelen+sparelen);
	int offsetI2 = offsetI+framelen;
	int offsetO = frameidx*(framelen+sparelen+paritylen);
	int offsetO2 = offsetO+frame2+spare2+parity2;
	//int filesize
	int len;
	char ldpcmode[512];
	//FILE *fptr;

	memcpy(&data[offsetO],&pageBuf[offsetI],frame2);
	memcpy(&data[offsetO2],&pageBuf[offsetI+frame2],frame2);

	memcpy(&data[offsetO+frame2],&pageBuf[offsetI2],spare2);
	memcpy(&data[offsetO2+frame2],&pageBuf[offsetI2+spare2],spare2);

	//memcpy(&data[offsetO+frame2+spare2],&pbuf[offsetI+frame2+spare2],parity2);
	//memcpy(&data[offsetO2+frame2+spare2],&pbuf[offsetI2+frame2+spare2],parity2);

	if(paritylen)
	{
		sprintf_s(ldpcmode,"2k_mode%d",m_LDPCMode);

		len = pldpc5013->encode(&data[offsetO],2056, &data[offsetO+frame2+spare2], parity2);
		//len=ldpc5013_encode(&data[offsetO], 2056, &data[offsetO+frame2+spare2], parity2, ldpcmode, 0 );
		/*
		str.Format("dat.bin");
		fptr = fopen(str,"wb");
		fwrite ( &data[offsetO], 1, (frame2+spare2), fptr );
		fclose(fptr);
		str.Format("enc.bin");
		remove ( str );
		str.Format("-enc -i dat.bin -o enc.bin -prj 5013 -ldpc 2k_mode%d -fw_mode 0",m_LDPCMode);
		ShellExecute(NULL,"open","LDPC.exe", str,"",SW_SHOW);	
		str.Format("enc.bin");

		fptr=NULL;
		do
		{
			fptr = fopen(str,"rb");
		}
		while(fptr==NULL);
		do
		{
			fseek(fptr, 0, SEEK_END); // seek to end of file
			filesize = ftell(fptr); // get current file pointer
			fseek(fptr, 0, SEEK_SET); // seek back to beginning of file
		}
		while(filesize!=parity2);
		
		//fptr = fopen(str,"rb");
		fread ( &data[offsetO+frame2+spare2], 1, parity2, fptr );
		fclose(fptr);
		*/
		len = pldpc5013->encode(&data[offsetO2],2056, &data[offsetO2+frame2+spare2], parity2);
		//len=ldpc5013_encode(&data[offsetO2], 2056, &data[offsetO2+frame2+spare2], parity2, ldpcmode, 0 );
		/*
		str.Format("dat.bin");
		fptr = fopen(str,"wb");
		fwrite ( &data[offsetO2], 1, (frame2+spare2), fptr );
		fclose(fptr);
		str.Format("enc.bin");
		remove ( str );
		str.Format("-enc -i dat.bin -o enc.bin -prj 5013 -ldpc 2k_mode%d -fw_mode 0",m_LDPCMode);
		ShellExecute(NULL,"open","LDPC.exe", str,"",SW_SHOW);
		str.Format("enc.bin");
		
		fptr=NULL;
		do
		{
			fptr = fopen(str,"rb");
		}
		while(fptr==NULL);
		do
		{
			fseek(fptr, 0, SEEK_END); // seek to end of file
			filesize = ftell(fptr); // get current file pointer
			fseek(fptr, 0, SEEK_SET); // seek back to beginning of file
		}
		while(filesize!=parity2);
		
		//fptr = fopen(str,"rb");
		//str.Format("2k_mode%d",m_LDPCMode);
		//len=ldpc5013_encode(&data[offsetO2], 2056, &data[offsetO2+frame2+spare2], parity2, str, 0 );
		fread ( &data[offsetO2+frame2+spare2], 1, parity2, fptr );
		fclose(fptr);
		*/
	}


	//printf("data=%02X%02X -->%02X%02X -->\n",data[0],data[1],data[frame2],data[frame2+1]);
	//printf("p4k=%02X%02X -->%02X%02X -->\n",p4k[0],p4k[1],p4k[spare2],p4k[spare2+1]);
	//printf("pty=%02X%02X -->%02X%02X -->\n",pty[0],pty[1],pty[parity2],pty[parity2+1]);

	return 0;
}


int LdpcEcc::pageToFrame_5017(ldpc5017* pldpc5013, const unsigned char* pageBuf, int frameidx, int framelen, int sparelen, int paritylen, unsigned char* data )
{
	//assert( ((framelen&0x01)==0) && ((sparelen&0x01)==0) && ((paritylen&0x01)==0) );
	int spare2 = sparelen/2;
	int parity2 = paritylen/2;
	int frame2 = framelen/2;
	int offsetI = frameidx*(framelen+sparelen);
	int offsetI2 = offsetI+framelen;
	int offsetO = frameidx*(framelen+sparelen+paritylen);
	int offsetO2 = offsetO+frame2+spare2+parity2;
	//int filesize
	int len;
	char ldpcmode[512];
	//FILE *fptr;

	memcpy(&data[offsetO],&pageBuf[offsetI],frame2);
	memcpy(&data[offsetO2],&pageBuf[offsetI+frame2],frame2);

	memcpy(&data[offsetO+frame2],&pageBuf[offsetI2],spare2);
	memcpy(&data[offsetO2+frame2],&pageBuf[offsetI2+spare2],spare2);

	//memcpy(&data[offsetO+frame2+spare2],&pbuf[offsetI+frame2+spare2],parity2);
	//memcpy(&data[offsetO2+frame2+spare2],&pbuf[offsetI2+frame2+spare2],parity2);

	if(paritylen)
	{
		sprintf_s(ldpcmode,"2k_mode%d",m_LDPCMode);

		len = pldpc5013->encode(&data[offsetO],2056, &data[offsetO+frame2+spare2], parity2);
		//len=ldpc5013_encode(&data[offsetO], 2056, &data[offsetO+frame2+spare2], parity2, ldpcmode, 0 );
		/*
		str.Format("dat.bin");
		fptr = fopen(str,"wb");
		fwrite ( &data[offsetO], 1, (frame2+spare2), fptr );
		fclose(fptr);
		str.Format("enc.bin");
		remove ( str );
		str.Format("-enc -i dat.bin -o enc.bin -prj 5013 -ldpc 2k_mode%d -fw_mode 0",m_LDPCMode);
		ShellExecute(NULL,"open","LDPC.exe", str,"",SW_SHOW);	
		str.Format("enc.bin");

		fptr=NULL;
		do
		{
			fptr = fopen(str,"rb");
		}
		while(fptr==NULL);
		do
		{
			fseek(fptr, 0, SEEK_END); // seek to end of file
			filesize = ftell(fptr); // get current file pointer
			fseek(fptr, 0, SEEK_SET); // seek back to beginning of file
		}
		while(filesize!=parity2);
		
		//fptr = fopen(str,"rb");
		fread ( &data[offsetO+frame2+spare2], 1, parity2, fptr );
		fclose(fptr);
		*/
		len = pldpc5013->encode(&data[offsetO2],2056, &data[offsetO2+frame2+spare2], parity2);
		//len=ldpc5013_encode(&data[offsetO2], 2056, &data[offsetO2+frame2+spare2], parity2, ldpcmode, 0 );
		/*
		str.Format("dat.bin");
		fptr = fopen(str,"wb");
		fwrite ( &data[offsetO2], 1, (frame2+spare2), fptr );
		fclose(fptr);
		str.Format("enc.bin");
		remove ( str );
		str.Format("-enc -i dat.bin -o enc.bin -prj 5013 -ldpc 2k_mode%d -fw_mode 0",m_LDPCMode);
		ShellExecute(NULL,"open","LDPC.exe", str,"",SW_SHOW);
		str.Format("enc.bin");
		
		fptr=NULL;
		do
		{
			fptr = fopen(str,"rb");
		}
		while(fptr==NULL);
		do
		{
			fseek(fptr, 0, SEEK_END); // seek to end of file
			filesize = ftell(fptr); // get current file pointer
			fseek(fptr, 0, SEEK_SET); // seek back to beginning of file
		}
		while(filesize!=parity2);
		
		//fptr = fopen(str,"rb");
		//str.Format("2k_mode%d",m_LDPCMode);
		//len=ldpc5013_encode(&data[offsetO2], 2056, &data[offsetO2+frame2+spare2], parity2, str, 0 );
		fread ( &data[offsetO2+frame2+spare2], 1, parity2, fptr );
		fclose(fptr);
		*/
	}


	//printf("data=%02X%02X -->%02X%02X -->\n",data[0],data[1],data[frame2],data[frame2+1]);
	//printf("p4k=%02X%02X -->%02X%02X -->\n",p4k[0],p4k[1],p4k[spare2],p4k[spare2+1]);
	//printf("pty=%02X%02X -->%02X%02X -->\n",pty[0],pty[1],pty[parity2],pty[parity2+1]);

	return 0;
}

int LdpcEcc::pageToFrame_5019(ldpc5019* pldpc5013, const unsigned char* pageBuf, int frameidx, int framelen, int sparelen, int paritylen, unsigned char* data )
{
	//assert( ((framelen&0x01)==0) && ((sparelen&0x01)==0) && ((paritylen&0x01)==0) );
	int spare2 = sparelen/2;
	int parity2 = paritylen/2;
	int frame2 = framelen/2;
	int offsetI = frameidx*(framelen+sparelen);
	int offsetI2 = offsetI+framelen;
	int offsetO = frameidx*(framelen+sparelen+paritylen);
	int offsetO2 = offsetO+frame2+spare2+parity2;
	//int filesize
	int len;
	char ldpcmode[512];
	//FILE *fptr;

	memcpy(&data[offsetO],&pageBuf[offsetI],frame2);
	memcpy(&data[offsetO2],&pageBuf[offsetI+frame2],frame2);

	memcpy(&data[offsetO+frame2],&pageBuf[offsetI2],spare2);
	memcpy(&data[offsetO2+frame2],&pageBuf[offsetI2+spare2],spare2);

	//memcpy(&data[offsetO+frame2+spare2],&pbuf[offsetI+frame2+spare2],parity2);
	//memcpy(&data[offsetO2+frame2+spare2],&pbuf[offsetI2+frame2+spare2],parity2);

	if(paritylen)
	{
		sprintf_s(ldpcmode,"2k_mode%d",m_LDPCMode);

		len = pldpc5013->encode(&data[offsetO],2056, &data[offsetO+frame2+spare2], parity2);
		//len=ldpc5013_encode(&data[offsetO], 2056, &data[offsetO+frame2+spare2], parity2, ldpcmode, 0 );
		/*
		str.Format("dat.bin");
		fptr = fopen(str,"wb");
		fwrite ( &data[offsetO], 1, (frame2+spare2), fptr );
		fclose(fptr);
		str.Format("enc.bin");
		remove ( str );
		str.Format("-enc -i dat.bin -o enc.bin -prj 5013 -ldpc 2k_mode%d -fw_mode 0",m_LDPCMode);
		ShellExecute(NULL,"open","LDPC.exe", str,"",SW_SHOW);	
		str.Format("enc.bin");

		fptr=NULL;
		do
		{
			fptr = fopen(str,"rb");
		}
		while(fptr==NULL);
		do
		{
			fseek(fptr, 0, SEEK_END); // seek to end of file
			filesize = ftell(fptr); // get current file pointer
			fseek(fptr, 0, SEEK_SET); // seek back to beginning of file
		}
		while(filesize!=parity2);
		
		//fptr = fopen(str,"rb");
		fread ( &data[offsetO+frame2+spare2], 1, parity2, fptr );
		fclose(fptr);
		*/
		len = pldpc5013->encode(&data[offsetO2],2056, &data[offsetO2+frame2+spare2], parity2);
		//len=ldpc5013_encode(&data[offsetO2], 2056, &data[offsetO2+frame2+spare2], parity2, ldpcmode, 0 );
		/*
		str.Format("dat.bin");
		fptr = fopen(str,"wb");
		fwrite ( &data[offsetO2], 1, (frame2+spare2), fptr );
		fclose(fptr);
		str.Format("enc.bin");
		remove ( str );
		str.Format("-enc -i dat.bin -o enc.bin -prj 5013 -ldpc 2k_mode%d -fw_mode 0",m_LDPCMode);
		ShellExecute(NULL,"open","LDPC.exe", str,"",SW_SHOW);
		str.Format("enc.bin");
		
		fptr=NULL;
		do
		{
			fptr = fopen(str,"rb");
		}
		while(fptr==NULL);
		do
		{
			fseek(fptr, 0, SEEK_END); // seek to end of file
			filesize = ftell(fptr); // get current file pointer
			fseek(fptr, 0, SEEK_SET); // seek back to beginning of file
		}
		while(filesize!=parity2);
		
		//fptr = fopen(str,"rb");
		//str.Format("2k_mode%d",m_LDPCMode);
		//len=ldpc5013_encode(&data[offsetO2], 2056, &data[offsetO2+frame2+spare2], parity2, str, 0 );
		fread ( &data[offsetO2+frame2+spare2], 1, parity2, fptr );
		fclose(fptr);
		*/
	}


	//printf("data=%02X%02X -->%02X%02X -->\n",data[0],data[1],data[frame2],data[frame2+1]);
	//printf("p4k=%02X%02X -->%02X%02X -->\n",p4k[0],p4k[1],p4k[spare2],p4k[spare2+1]);
	//printf("pty=%02X%02X -->%02X%02X -->\n",pty[0],pty[1],pty[parity2],pty[parity2+1]);

	return 0;
}

void LdpcEcc::SetLDPCMode(unsigned char Mode)
{
	m_LDPCMode = Mode;
}

void LdpcEcc::SetPageSeed(unsigned int PageSeed)
{
	m_PageSeed = PageSeed;
}
void LdpcEcc::SetBlockSeed(unsigned int BlockSeed)
{
	m_BlockSeed = BlockSeed;
}
#include <math.h>


#define ATCM_SECCNT_OFFSET			(0x80)
#define BIN_CRC32_OFFSET			(0x8C)
#define BITMSK(LENGTH, OFFSET)      ((~((~0ULL)<<(LENGTH)))<<(OFFSET))


unsigned int gulInputBuf[0x100000 >> 2] = {0};	//คWญญ1MB


int String2Hex(char* pubSeed)
{
	unsigned int ulCount = 0;
	int slHexNum = 0;
	unsigned int i;
	double Int2HexTemp;
	
	while( pubSeed[ulCount] != '\0') {
		ulCount += 1;
	}

	if( ulCount > (8+2) ) {
		printf("Wrong Rand Seed Format\n");
		return (~0);
	}
	else {
		for (i = 2; i < ulCount; i++) {
			Int2HexTemp = pow(16.0, (double)(ulCount - i - 1));
			printf("HexTemp:%f, count:%d\n", Int2HexTemp, ulCount - i - 1);

			if ((pubSeed[i] >= 'A') && (pubSeed[i] <= 'F')) {
				slHexNum = slHexNum + (pubSeed[i] - 'A' + 10) * (int)Int2HexTemp;
			}
			else if ((pubSeed[i] >= '0') && (pubSeed[i] <= '9')) {
				slHexNum = (int)(slHexNum + (pubSeed[i] - '0') * Int2HexTemp);
			}
		}
	}
	printf("RandSeed:%d\n", slHexNum);

	return slHexNum;
}

unsigned int LdpcEcc::E13_FW_CRC32(unsigned int *data, unsigned int Length)
{
	const unsigned int ulcPolynomial = 0xEDB88320;
	unsigned int ulcrc;	
	unsigned int ulBuf;
	unsigned int uli;
	unsigned char current = 0;
	unsigned char ubByteIdx = 0;
	unsigned char ubj;
	unsigned int ulccc;

//---- Initial value ----
	// Get Rand Seed
	ulcrc = 0x8299;

	// Set length
	ulccc = 0;

	for (ubj = 0; ubj < 32; ubj++) {
		ulccc |= ((ulcrc >> (31 - ubj) & 1) << ubj);
	}
	ulcrc = ulccc;

	for(uli = 0; uli < (Length >> 2); uli++) {
		ulBuf = data[uli];
		for(ubByteIdx=0 ; ubByteIdx<4 ; ubByteIdx++) {
			current = ((ulBuf & BITMSK(8 , 8*ubByteIdx)) >> (8*ubByteIdx));
			ulcrc ^= current;
			for (ubj = 0; ubj < 8; ubj++)
				ulcrc = (ulcrc >> 1) ^ (-(int) (ulcrc & 1) & ulcPolynomial);
		}
	}

	ulccc = 0;

	for (ubj = 0; ubj < 32; ubj++) {
		ulccc |= (ulcrc >> (31 - ubj) & 1) << ubj;
	}

	return ulccc;
}

// James test
void LdpcEcc::SoftWaveEcc(unsigned char *data, unsigned int DataLen)
{
	// TODO: Add your control notification handler code here
	//unsigned int i;

	// code form kelvin
	//FILE *fptr1;
	int len1=0,len2=0;

	const int pagesizeK=16384;	// 4k+16 x 4
	unsigned char *TargetBuf = new unsigned char[pagesizeK];
	
	// combin data to 4k+16

	NPsConversion PsConv;
	//---------Do conversion

	//fptr1 = NULL;
	//fptr1 = fopen ( "Cresult.bin","wb");
	//str.Format("D:\working\STTool Project\SW\STTool\_SSD_USB_Sorting\VerifyTool_SQL\Output\debug\Cresult.bin");
	//str.Format("Cresult.bin");
	//fptr1 = fopen(str,"wb");

	int pagecount = DataLen/4096;
	pagecount += (DataLen % 4096) > 0 ?1:0;

	int ConvSize = 4096;

	for(int pg=0;pg<pagecount;pg++){

		//	fread(sourceBuf,sizeof(BYTE),4096,file1);
		//ASSERT(pg < 17);
		int cellLen = ConvSize*8;//Total 4K bit size
		//UpdateData(TRUE);
		PsConv.SetBlockSeed(m_BlockSeed, (0x5013 == m_controller)?true:false);
		PsConv.seed_selector4(cellLen,0,pg,0,8,0,0);	// softwave Ecc close ed3Conv and none E13 mode
		//memset(TargetBuf,0,pagesizeK);
		
		PsConv.MakeConv(&data[ConvSize*pg],&TargetBuf[ConvSize*pg],ConvSize);
		//if(fptr1!=NULL)
		//{
		//	fwrite(&TargetBuf[ConvSize*pg],1,ConvSize,fptr1);	
		//}
	}
	//if(fptr1!=NULL)
	//{
	//	fclose(fptr1);
	//}
	delete []TargetBuf;
	return;
}
// James test
void LdpcEcc::Conversion2307(unsigned char *data, unsigned int DataLen)
{
	// TODO: Add your control notification handler code here
	//unsigned int i;

	// code form kelvin
	FILE *fptr1;
	int len1=0,len2=0;

	const int pagesizeK=16384;	// 4k+16 x 4
	unsigned char *TargetBuf = new unsigned char[pagesizeK];
	
	// combin data to 4k+16

	NPsConversion PsConv;
	//---------Do conversion

	fptr1 = NULL;
	//fptr1 = fopen ( "Cresult.bin","wb");
	//str.Format("D:\working\STTool Project\SW\STTool\_SSD_USB_Sorting\VerifyTool_SQL\Output\debug\Cresult.bin");
	//str.Format("Cresult.bin");
	//fptr1 = fopen(str,"wb");

	int pagecount = DataLen/4096;
	pagecount += (DataLen % 4096) > 0 ?1:0;

	int ConvSize = 4096;


	//	fread(sourceBuf,sizeof(BYTE),4096,file1);
	//ASSERT(pg < 17);
	int cellLen = 16384*8;//Total 4K bit size
	//UpdateData(TRUE);
	PsConv.SetBlockSeed(m_BlockSeed, (0x5013 == m_controller)?true:false);
	PsConv.seed_selector4(cellLen,0,m_PageSeed,0,16,2,1);	// softwave Ecc close ed3Conv and none E13 mode
	//memset(TargetBuf,0,pagesizeK);
	
	PsConv.MakeConv(&data[0],&TargetBuf[0],16384);
	//if(fptr1!=NULL)
	//{
	//	fwrite(&TargetBuf[0],16384,1,fptr1);
	//	fclose(fptr1);	
	//}
	
	delete []TargetBuf;
	return;
}