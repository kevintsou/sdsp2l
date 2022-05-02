#if !defined(AFX_LDPCECC_H__80919B9E_C62D_4AB6_9ADD_F5850AFE5737__INCLUDED_)
#define AFX_LDPCECC_H__80919B9E_C62D_4AB6_9ADD_F5850AFE5737__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LdpcEcc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// LdpcEcc dialog
class ldpc5013
{
public:
	ldpc5013(const char* ldpc_mode, int fwmode);
	~ldpc5013();
	int encode(const unsigned char* data, int datalen, unsigned char* buf, int buflen );
	//int decode(const unsigned char* data, int datalen, unsigned char* buf, int buflen );
private:
	void *_LDPC_P;
	void *_LDPC_G;
	void *_Info_P;
	int* _ori_data;
	int* _enc_data;
};

class ldpc5019
{
public:
	ldpc5019(const char* ldpc_mode, int fwmode);
	~ldpc5019();
	int encode(const unsigned char* data, int datalen, unsigned char* buf, int buflen );
	//int decode(const unsigned char* data, int datalen, unsigned char* buf, int buflen );
private:
	void *_LDPC_P;
	void *_LDPC_G;
	void *_Info_P;
	int* _ori_data;
	int* _enc_data;
};

class ldpc5017
{
public:
	ldpc5017(const char* ldpc_mode, int fwmode);
	~ldpc5017();
	int encode(const unsigned char* data, int datalen, unsigned char* buf, int buflen );
	//int decode(const unsigned char* data, int datalen, unsigned char* buf, int buflen );
private:
	void *_LDPC_P;
	void *_LDPC_G;
	void *_Info_P;
	int* _ori_data;
	int* _enc_data;
};

class LdpcEcc
{
// Construction
public:
	LdpcEcc(int controller);   // standard constructor
	~LdpcEcc(void);

	int		m_BlockSeed = 0;
	int		m_PageSeed = 0;
	int		m_LDPCMode = 0;


	bool flashIP_CRC16_Loop( const unsigned char* din, const unsigned char* c, unsigned char* CRC_OUT_16 );
	bool flashIP_CRC16( const unsigned char* din, unsigned char* dout );
	int pageToFrame_5013(ldpc5013* pldpc5013,const unsigned char* pageBuf, int frameidx, int framelen, int sparelen, int paritylen, unsigned char* data );
	int pageToFrame_5017(ldpc5017* pldpc5017,const unsigned char* pageBuf, int frameidx, int framelen, int sparelen, int paritylen, unsigned char* data );
	int pageToFrame_5019(ldpc5019* pldpc5019,const unsigned char* pageBuf, int frameidx, int framelen, int sparelen, int paritylen, unsigned char* data );
	void convertionBack(unsigned char *buf, unsigned char *Outbuf, int len, int FlhpageSizeK);
	void SetLDPCMode(unsigned char);
	void SetPageSeed(unsigned int PageSeed);
	void SetBlockSeed(unsigned int BlockSeed);
	void GenRAWData(unsigned char *data, unsigned int DataLen, int flashSizeK);
	unsigned int E13_FW_CRC32(unsigned int *data, unsigned int Length);
	void SoftWaveEcc(unsigned char *data, unsigned int DataLen);
	void Conversion2307(unsigned char *data, unsigned int DataLen);
	ldpc5013 * CreateLdpc5013(int ldpc_mode);
	ldpc5017 * CreateLdpc5017(int ldpc_mode);
	ldpc5019 * CreateLdpc5019(int ldpc_mode);
protected:

private:
	ldpc5013 * m_current_ldpc5013;
	ldpc5017 * m_current_ldpc5017;
	ldpc5019 * m_current_ldpc5019;
	int  m_current_ldpc_mode = 0;
	int  m_controller = 0;



	

};

#endif // !defined(AFX_LDPCECC_H__80919B9E_C62D_4AB6_9ADD_F5850AFE5737__INCLUDED_)
