

#ifndef _MEMMANGER_
#define _MEMMANGER_

#include <stdio.h>
void _quickSort ( int** a, int lo, int hi, int len );
#define _DEBUGMEM 1

class CharMemMgr
{
public:
	CharMemMgr ( int c, int r );
    CharMemMgr ( int, int =1, bool default0=true ); //col , row
    CharMemMgr (); //col , row
    ~CharMemMgr();
    void Resize( int, int =1 );
    void ReSize( int , int =1 );
    int getCol();
    int getRow();
    void MemSet ( unsigned char );
    unsigned char Get ( int, int =0 );
    void Set ( unsigned char, int, int =0 ); // value, col, row
    void ValueAdd ( unsigned char, int, int =0 ); // value+=, col, row
    void ValueOr ( unsigned char, int, int =0 ); // value|=, col, row
    void ValueAnd ( unsigned char, int, int =0 ); // value|=, col, row
    int CountOne( int len );
    int CountOne( int offset, int len );
    unsigned char* Data();
    int Size();
    void Rand( bool setSeed=false );
	void Rand();
    int BitDiff ( const unsigned char* );
    int BitDiff ( unsigned char*, int len );
    void BitXOR ( const unsigned char* );
    void BitXORTo ( unsigned char* );
    void BitXNOR ( const unsigned char* );
    void BitXNORTo ( unsigned char* );
    int Count ( unsigned char ); //count value = v
    int LoadCSV( const char* fn, int col, bool isHex, int startLine=0 );
    int LoadBinnary( const char* fn );
    bool Sort( bool asc=false );
    static void DumpBinary(const char* fn, const unsigned char*buf, unsigned int len );
	void Rand_rang(bool setSeed, unsigned char min, unsigned char max);		// <S>
	int Rand_rang_len(bool setSeed, unsigned char min, unsigned char max, unsigned int iIndex, unsigned int iLen);
	int Rand_rang_fixed_0(bool setSeed, unsigned char min, unsigned char max, unsigned int iIndex, unsigned int iLen);
	int Rand_rang_fixed_1(bool setSeed, unsigned int min, unsigned int max, unsigned int iIndex, unsigned int iLen);
	int Rand_rang_fixed_2(bool setSeed, unsigned int min, unsigned int max, unsigned int iIndex, unsigned int iLen);

private:
    int _col;
    int _row;
    unsigned char* _data;
    bool _IsHexChar( const char c );
};

class UShortMemMgr
{
public:
	UShortMemMgr ( int, int =1 ); //col , row
	~UShortMemMgr();
	void MemSet ( unsigned short );
	unsigned short Get ( int, int =0 );
	void ValueAdd ( unsigned short v, int c, int r=0 );
	void Set ( unsigned short, int, int =0 ); // value, col, row
	unsigned short* Data();
	void Swap( int c0, int c1 );
	int Size();
private:
	int _col;
	int _row;
	unsigned short* _data;
};
class IntMemMgr
{
public:
    IntMemMgr ( int, int =1 );          //col , row
    IntMemMgr ();
    ~IntMemMgr ();
    void Resize( int, int =1 );
    void MemSet ( int );
    int Get ( int, int =0 );
    void Set ( int, int, int =0 );       // value, col, row
    double Avg ( int );       // col, return avg value;
    bool FindPeak ( int, int&, int& );  // col, peak col, peak value;
    bool ValueAt ( int, int&, int& );
    void ValueAdd ( int, int, int =0 ); // value+=, col, row
    bool Sort2D ( int*, int*, int );
    bool Sort ( int );
    static void QuickSort ( int*, int , int );
    int* Data();
    int Size();
    void DumpCSV ( const char* file, const char* hdr );
    int LoadCSV( const char* fn, int col, bool isHex, int startLine=0 );
	int LoadBinCSV( const char* fn, int col, bool isHex, int startLine=0, int strCol=0 );

private:
    int _col;
    int _row;
    int* _data;
    bool _IsHexChar( const char c );
};

class UIntMemMgr
{
public:
    UIntMemMgr ( int, int =1 );          //col , row
    UIntMemMgr ();
    ~UIntMemMgr ();
    void MemSet ( unsigned long long );
    unsigned long long Get ( int, int =0 );
    void Set ( unsigned long long , int, int =0 );       // value, col, row
    void ValueAdd ( unsigned long long , int, int =0 ); // value+=, col, row
    unsigned long long* Data();
    int Size();
    void DumpCSV ( const char* file, const char* hdr );
    int LoadCSV( const char* fn, int col, bool isHex, int startLine=0 );
	void Swap( int c0, int c1 );
private:
    int _col;
    int _row;
    unsigned long long * _data;
    bool _IsHexChar( const char c );
};

class Int3DMgr
{
public:
    Int3DMgr ( int, int, int ); //i = col, j=row, k=phase
    ~Int3DMgr ();
    void MemSet ( int );
    int Get ( int, int, int  );
    void Set ( int, int, int, int ); // value, col, row, phase

private:
    int _len;
    int _col;
    int _row;
    int _phase;
    int* _data;
};

class DoubleMemMgr
{
public:
    DoubleMemMgr ( int, int =1 ); //col , row
    DoubleMemMgr ();
    ~DoubleMemMgr();
    void MemSet ( double );
    double Get ( int, int =0 );
    void Set ( double, int, int =0 ); // value, col, row
    double* Data();
    int Size();
    void DumpCSV ( const char* file, const char* hdr );
    int LoadCSV( const char* fn, int col, int startLine );
private:
    int _col;
    int _row;
    double* _data;
};


class FileMemMgr
{
public:
    FileMemMgr ( const wchar_t*, bool =false ); // file, readonly
    FileMemMgr ( const char*, bool =false ); // file, readonly
    ~FileMemMgr();
    long Read ( long, unsigned char*, int );
    long Read ( unsigned char*, int );
    long Append ( unsigned char*, int );
    bool End();
    void Close();
    void Rewind();
    FILE* Ptr();
    wchar_t* WFileName();
    char* FileName();

private:
    bool _readOnly;
    FILE* _fptr;
    wchar_t _wfn[512];
    char _fn[512];
};

class MemStr
{
public:
    MemStr ( int );
    ~MemStr();
    char* Data();
    void SetEmpty();
    bool IsEmpty();
    int Size();
    const char* Cat ( const char*, ... );
private:
    int _size;
    char* _data;
};

class FileMgr
{
public:
    FileMgr ( const wchar_t*, const char* );
    FileMgr ( const char*, const char* );
    FileMgr ();
    ~FileMgr();
    void Open(const wchar_t*, const char*);
    void Open(const char*, const char*);
    size_t Size();
    void Close();
    FILE* Ptr();
    void Rewind();

private:
    FILE* _fptr;
    char _option[32];
};


class _FileArrayNode
{
public:
    _FileArrayNode(FILE*, const char*);
    ~_FileArrayNode();
    FILE* _file;
    char * _fn;
    char * _fnfull;
    _FileArrayNode *_next;
};

class FileArrayMgr
{
public:
    static void Create(const char*);
    static const char* LogFolder(const char*);
    static bool Valid();
    static void Free();
    static void AddFile( FILE*, const char* );
    static FILE* GetFile( const char* );
    static bool GroupFiles( const char*, bool remove );
    static bool ExtractFiles( const char* );
    static int Size();

private:
    static FileArrayMgr* _instance;

    FileArrayMgr(const char*);
    ~FileArrayMgr();
    char * _logFolder;
    _FileArrayNode *_root;
    _FileArrayNode *_end;
    unsigned int _size;
};

class Utilities {
public:
	Utilities();
	int DiffBit(unsigned char * buffer1, unsigned char * buffer2, int buffer_size);
	bool DiffBitCheckPerK(unsigned char * buffer1, unsigned char * buffer2, int buffer_size, int threshold, int * max_diff_per_k=NULL, int * total_diff= NULL);
	int Count1 ( unsigned char *buf, int len );
	int FindRightData(unsigned char * input_buf, int data_len, int max_search_len, unsigned char * output_buf);
	int MakeRandomErrorBitPerK(unsigned char * buffer1, int len,  int errbit);
	int MakeRandomErrorBit(unsigned char * buffer1, int len,  int errbit);
private:

};
#endif  /*_MEMMANGER_*/