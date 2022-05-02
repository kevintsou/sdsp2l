#include "stdafx.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <wchar.h>
#include <time.h>
#include "MemManager.h"

#pragma warning(disable: 4996)

CharMemMgr::CharMemMgr ( int c, int r, bool default0 )
{
#ifdef _DEBUGMEM
    assert ( c*r>0 );
#endif
    _col = c;
    _row = r;
    _data = ( unsigned char* ) malloc ( c*r*sizeof(unsigned char) );
    if ( default0 ) {
        MemSet ( 0 );
    }
}

CharMemMgr::CharMemMgr ()
{
    _col = 0;
    _row = 0;
    _data = 0;
}

CharMemMgr::CharMemMgr ( int c, int r )
{
	CharMemMgr (  c,  r, true);
}
CharMemMgr::~CharMemMgr()
{
    if ( _data )  {
        free ( _data );
        _data = 0;
    }
}
void CharMemMgr::ReSize( int c, int r )
{
	return Resize(  c,  r );
}

void CharMemMgr::Resize( int c, int r )
{
    if ( _data )  {
        free ( _data );
        _data = 0;
    }
#ifdef _DEBUGMEM
    assert ( c*r>0 );
#endif
    _col = c;
    _row = r;
    _data = ( unsigned char* ) malloc ( c*r*sizeof(unsigned char) );
    MemSet ( 0 );
}
int CharMemMgr::CountOne( int len )
{
#ifdef _DEBUGMEM
	assert(len<=_col*_row);
#endif
	int cnt = 0;
	for ( int i=0; i<len; i++ ) {
		char by = _data[i];
		for ( int b=0; b<8; b++ ) {
			cnt+= ( by>>b ) & 0x01;
		}
	}
	return cnt;
}

int CharMemMgr::CountOne( int offset, int len )
{
#ifdef _DEBUGMEM
	assert(len<=_col*_row);
	assert((offset+len)<=_col*_row);
#endif
	int cnt = 0;
	for ( int i=0; i<len; i++ ) {
		char by = _data[i+offset];
		for ( int b=0; b<8; b++ ) {
			cnt+= ( by>>b ) & 0x01;
		}
	}
	return cnt;
}
unsigned char* CharMemMgr::Data()
{
    return _data;
}

int CharMemMgr::Size()
{
    return _col*_row;
}

void CharMemMgr::MemSet ( unsigned char v )
{
    int len = _col*_row;
    for ( int i=0; i<len; i++ ) {
        _data[i] = v;
    }
}

int CharMemMgr::Count ( unsigned char v )
{
    int cnt = 0;
    int len = _col*_row;
    for ( int i=0; i<len; i++ ) {
        if ( _data[i]== v ) {
            cnt++;
        }
    }
    return cnt;
}

bool CharMemMgr::Sort( bool asc )
{
    int len = _col*_row;
    for ( int i=0; i< len; i++ ) {
        for ( int j=( i+1 ); j<len; j++ )  {
        		if ( asc ) {
	            if ( _data[i]>_data[j] ) {
	                unsigned char tmp = _data[i];
	                _data[i] = _data[j];
	                _data[j] = tmp;
	            }
        		}
        		else {
	            if ( _data[i]<_data[j] ) {
	                unsigned char  tmp = _data[i];
	                _data[i] = _data[j];
	                _data[j] = tmp;
	            }
	          }
        }
    }
    return true;
}

unsigned char CharMemMgr::Get ( int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c>=0 && r>=0 && c<_col&&r<_row );
#endif
    return _data[c*_row+r];
}

void CharMemMgr::Set ( unsigned char v, int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c>=0 && r>=0 && c<_col&&r<_row );
#endif
    _data[c*_row+r] = v;
}

void CharMemMgr::ValueAdd ( unsigned char v, int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c<_col&&r<_row );
#endif
    _data[c*_row+r] += v;
}

void CharMemMgr::ValueOr ( unsigned char v, int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c<_col&&r<_row );
#endif
    _data[c*_row+r] |= v;
}

void CharMemMgr::ValueAnd ( unsigned char v, int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c<_col&&r<_row );
#endif
    _data[c*_row+r] &= v;
}


void CharMemMgr::Rand()
{
	Rand(true);
}

void CharMemMgr::Rand( bool setSeed )
{
	static unsigned int _rand=0;
	if (setSeed) {
		time_t seconds;
		time ( &seconds );
		srand ( ( unsigned int ) seconds+_rand ); //set random seed.
		_rand++;
	}

	int len = _col*_row;
	for ( int i=0; i<len; i++ ) {
		_data[i] = rand() & 0xFF;
	}
}

int inline _CharBitDiff ( unsigned char c1, unsigned char c2 )
{
    int ret = 0;
    unsigned char c = c1^c2;
    for ( int i=0; i<8; i++ ) {
        ret+= ( c>>i ) &0x01;
    }
    return ret;
}

int CharMemMgr::BitDiff ( const unsigned char* target )
{
    int total = 0;
    int len = _col*_row;
    for ( int i=0; i<len; i++ ) {
        total += _CharBitDiff ( _data[i], target[i] );
    }
    return total;
}

int CharMemMgr::BitDiff ( unsigned char* target, int len )
{
#ifdef _DEBUGMEM
	assert(len<=_col*_row);
#endif
	int total = 0;
	//int len = _col*_row;
	for ( int i=0; i<len; i++ ) {
		total += _CharBitDiff ( _data[i], target[i] );
	}
	return total;
}

void CharMemMgr::BitXOR ( const unsigned char* buf )
{
    unsigned int len = _col*_row;
    for ( unsigned int i=0; i<len; i++ ) {
        _data[i] ^= buf[i];
    }
}

void CharMemMgr::BitXORTo ( unsigned char* buf )
{
    unsigned int len = _col*_row;
    for ( unsigned int i=0; i<len; i++ ) {
        buf[i] ^= _data[i];
    }
}
void CharMemMgr::BitXNOR ( const unsigned char* buf )
{
    unsigned int len = _col*_row;
    for ( unsigned int i=0; i<len; i++ ) {
        _data[i] = ~(_data[i]^buf[i]);
    }
}
void CharMemMgr::BitXNORTo ( unsigned char* buf )
{
    unsigned int len = _col*_row;
    for ( unsigned int i=0; i<len; i++ ) {
        buf[i] = ~(_data[i]^buf[i]);
    }
}

bool CharMemMgr::_IsHexChar( const char c )
{
    if ( c>='0' && c<='9' )  return true;
    if ( c>='A' && c<='Z' )  return true;
    if ( c>='a' && c<='z' )  return true;
    if ( c=='-' )  return true;

    return false;
}

int CharMemMgr::LoadCSV( const char* fn, int col, bool isHex, int startLine )
{
    //assert(_data==0);
    FileMgr fm(fn,"r");
    if ( !fm.Ptr() ) {
        assert(0);
        return 0;
    }

    _col = col;
    _row = 0;
    char line[2048];
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        _row++;
    }

    assert(_row);
    _data = ( unsigned char* ) malloc ( _col*_row*sizeof(unsigned char) );
    //MemSet ( 0 );

    _row = _row-startLine;


    fseek ( fm.Ptr(),0,SEEK_SET );
    int v=0,vidx=0,idx=0, len;
    int lineCnt = -1;
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        lineCnt++;
        if (lineCnt<startLine) continue;

        vidx=idx=0;
        len = strlen(line);
        for ( int i=0; i<len; i++ ) {
            if ( !_IsHexChar(line[i]) ) {
                line[i] = '\0';
                if ( (i-idx)<2 ) {
                    idx = i;
                    continue;
                }
                if ( isHex ) {
                    sscanf(&line[idx],"%X",&v);
                }
                else {
                    v = atoi(&line[idx]);
                }
                Set ( v,vidx,lineCnt-startLine );
                vidx++;
                idx = i+1;
            }
            if( vidx>=col) break;
        }
        if ( vidx>0 && vidx<col && (len-idx)>1)
        {
            if ( isHex ) {
                sscanf(&line[idx],"%X",&v);
            }
            else {
                v = atoi(&line[idx]);
            }
            Set(v,vidx,lineCnt-startLine);
            vidx++;
        }
    }
    fm.Close();
    return _row;
}

int CharMemMgr::LoadBinnary( const char* fn )
{
    if ( _data )  {
        free(_data);
        _data = 0;
    }

    FileMgr fm(fn,"rb");
    fseek ( fm.Ptr(), 0L, SEEK_END );
    unsigned long len = ftell ( fm.Ptr() );
    _col = len;
    _row=1;
    _data = ( unsigned char* ) malloc ( _col*_row*sizeof(unsigned char) );

    fseek ( fm.Ptr(),0,SEEK_SET );
    len = fread ( _data, 1, len, fm.Ptr() );
    fm.Close();
    return _col;
}
void CharMemMgr::DumpBinary(const char* fn, const unsigned char*buf, unsigned int len )
{
	FileMgr fm1 ( fn, "wb" );
	unsigned int wlen = (int)fwrite ( buf, 1, len, fm1.Ptr() );
	assert ( wlen==len );
	fflush(fm1.Ptr());
	fm1.Close();
}
void CharMemMgr::Rand_rang(bool setSeed, unsigned char min, unsigned char max)		// <S>
{
	if (setSeed) {
		time_t seconds;
		time(&seconds);
		srand((unsigned int)seconds); //set random seed.
	}

	int len = _col*_row;
	int iData;
	int iDiff;

	iDiff =( max - min ) + 1;
	if (iDiff <= 0)
	{
		memset(_data, min, len);
		return;
	}

	for (int i = 0; i<len; i++) 
	{
		//_data[i] = rand() & 0xFF;
		iData = rand() & 0xFF;
		_data[i] = (iData%iDiff) + min;		// min~max

		// for debug
#if 0
		int itest = 0;
		if (_data[i] < min)
			itest = 1;
		if(_data[i]>max)
			itest = 0xff;
#endif

	}
}

int CharMemMgr::Rand_rang_len(bool setSeed, unsigned char min, unsigned char max, unsigned int iIndex, unsigned int iLen)		// <S>
{
	if (setSeed) {
		time_t seconds;
		time(&seconds);
		srand((unsigned int)seconds); //set random seed.
	}

	int len = _col*_row;
	int iData;
	int iDiff;

	// ?? len==0

	if (min > max) return 0x01;
	if ( (iIndex + iLen) > (unsigned int)len )
		return 0x02;

	if (min == max)
	{
		memset(_data, min, len);
		return 0;
	}

	iDiff = (max - min) + 1;
	for (unsigned int i = iIndex; i<(iIndex+iLen); i++)
	{
		//_data[i] = rand() & 0xFF;
		iData = rand() & 0xFF;
		_data[i] = (iData%iDiff) + min;		// min~max

		// for debug
#if 0
		int itest = 0;
		if (_data[i] < min)
			itest = 1;
		if (_data[i]>max)
			itest = 0xff;
#endif
	}

	return 0;
}

int CharMemMgr::Rand_rang_fixed_0(bool setSeed, unsigned char min, unsigned char max, unsigned int iIndex, unsigned int iLen)		// <S>
{
	if (setSeed) {
		time_t seconds;
		time(&seconds);
		srand((unsigned int)seconds); //set random seed.
	}

	int len = _col*_row;
	int iData;
	int iDiff;
	unsigned char bBuf[16];
	int i, j;
	bool blFound = false;

	if (min > max) return 0x01;
	if ((iIndex + iLen) > (unsigned int)len)
		return 0x02;

	if (min == max)
	{
		memset(_data, min, len);
		return 0;
	}

	iDiff = (max - min) + 1;
	memset(bBuf, 0, sizeof(bBuf));
	for (i = 0; i < 16; i++)
	{
		do 
		{
			blFound = true;
			iData = rand() & 0xFF;
			iData = (iData%iDiff) + min;		// min~max
			for (j = 0; j < i; j++)
			{
				if (iData == bBuf[j])
				{
					blFound = false;
					break;
				}
			}
		} while (blFound==false);
		bBuf[i] = iData;
	}

	memcpy(&_data[iIndex], bBuf, iLen);
	return 0;
}

/*
rand
no repeat
sort: small->large
unit : 2byte

_data = ( unsigned char* ) malloc ( c*r );
*/
int CharMemMgr::Rand_rang_fixed_1(bool setSeed, unsigned int min, unsigned int max, unsigned int iIndex, unsigned int iLen)		// <S>
{
	if (setSeed) {
		time_t seconds;
		time(&seconds);
		srand((unsigned int)seconds); //set random seed.
	}
	const int iBufSize = 16 * 1024;
	int len = _col*_row;
	int iData;
	int iDiff;
	unsigned short wBuf[iBufSize];
	unsigned int i, j;
	bool blFound = false;

	if ((iLen % 2) != 0)
		return 0x01;
	if (min > max) return 0x02;
	if ( (iIndex + iLen) > (unsigned int)len )
		return 0x03;

	if (min == max)
	{
		memset(_data, min, len);
		return 0x04;			// ??
	}
	if ((max - min + 1) < (unsigned int)(len/2) )
		return 0x10;		  // data no repeat
	if (iLen > iBufSize)
		return 0x11;			// ?? support 32K now

	iDiff = (max - min) + 1;
	memset(wBuf, 0, sizeof(wBuf));

	for (i = 0; i <iLen/2; i++)
	{
		do
		{
			blFound = true;
			iData = rand() & 0xFFFF;
			iData = (iData%iDiff) + min;		// min~max
			for (j = 0; j < i; j++)
			{
				if (iData == wBuf[j])
				{
					blFound = false;
					break;
				}
			}
		} while (blFound == false);
		wBuf[i] = iData;
	}

	memcpy(&_data[iIndex], wBuf, iLen);
	return 0;
}

// data only even
int CharMemMgr::Rand_rang_fixed_2(bool setSeed, unsigned int min, unsigned int max, unsigned int iIndex, unsigned int iLen)		// <S>
{
	if (setSeed) {
		time_t seconds;
		time(&seconds);
		srand((unsigned int)seconds); //set random seed.
	}
	const int iBufSize = 16 * 1024;
	int len = _col*_row;
	int iData;
	int iDiff;
	unsigned short wBuf[iBufSize];
	unsigned int i, j;
	bool blFound = false;

	if ((iLen % 2) != 0)
		return 0x01;
	if (min > max) return 0x02;
	if ((iIndex + iLen) > (unsigned int)len)
		return 0x03;

	if (min == max)
	{
		memset(_data, min, len);
		return 0x04;			// ??
	}
	//if ((max - min + 1) < (len / 2))
	//	return 0x10;		  // data no repeat
	if( (max/2)-((min+1)/2) + 1 < (unsigned int)(len / 2))
		return 0x10;		  // data no repeat and only even

	if (iLen > iBufSize)
		return 0x11;			// ?? support 32K now

	iDiff = (max - min) + 1;
	memset(wBuf, 0, sizeof(wBuf));

	for (i = 0; i <iLen / 2; i++)
	{
		do
		{
			blFound = true;
			iData = rand() & 0xFFFF;
			iData = (iData%iDiff) + min;		// min~max

			if ((iData % 2) != 0)
			{
				blFound = false;
				continue;
			}

			for (j = 0; j < i; j++)
			{
				if (iData == wBuf[j])
				{
					blFound = false;
					break;
				}
			}
		} while (blFound == false);
		wBuf[i] = iData;
	}

	memcpy(&_data[iIndex], wBuf, iLen);
	return 0;
}

UShortMemMgr::UShortMemMgr  ( int c, int r )
{
#ifdef _DEBUGMEM
	assert(c*r>0);
#endif
	_col = c;
	_row = r;
	_data = new unsigned short[c*r];
	MemSet ( 0 );
}
UShortMemMgr::~UShortMemMgr()
{
	if ( _data )  {
		delete[] _data;
		_data = 0;
	}
}
void UShortMemMgr::MemSet ( unsigned short v )
{
	int len = _col*_row;
	for ( int i=0; i<len; i++ ) {
		_data[i] = v;
	}
}

unsigned short UShortMemMgr::Get ( int c, int r )
{
#ifdef _DEBUGMEM
	assert(c<_col&&r<_row);
#endif
	return _data[c*_row+r];
}

void UShortMemMgr::ValueAdd ( unsigned short v, int c, int r )
{
#ifdef _DEBUGMEM
	assert(c<_col&&r<_row);
#endif
	_data[c*_row+r] += v;
}

void UShortMemMgr::Set ( unsigned short v, int c, int r )
{
#ifdef _DEBUGMEM
assert(c<_col&&r<_row);
#endif
	_data[c*_row+r] = v;
}
int CharMemMgr::getCol(){
	return _col;
}
int CharMemMgr::getRow()
{
	return _row;
}

unsigned short* UShortMemMgr::Data()
{
	return _data;
}

void UShortMemMgr::Swap( int c0, int c1 )
{
#ifdef _DEBUGMEM
	assert(c0<_col&&c1<_col);
#endif
	unsigned short tmp = _data[c0*_row];
	_data[c0*_row] = _data[c1*_row];
	_data[c1*_row] = tmp;
}

int UShortMemMgr::Size()
{
	return _col*_row;
}
/////////////////////////////////
IntMemMgr::IntMemMgr ( int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c*r>0 );
#endif
    _col = c;
    _row = r;
    _data = (int*) malloc( c*r*sizeof(int) );
    MemSet ( 0 );
}

IntMemMgr::IntMemMgr ()
{
    _col = 0;
    _row = 0;
    _data = 0;
}

IntMemMgr::~IntMemMgr()
{
    if ( _data )  {
        free(_data);
        _data = 0;
    }
}

void IntMemMgr::Resize( int c, int r )
{
    if ( _data )  {
        free ( _data );
        _data = 0;
    }
#ifdef _DEBUGMEM
    assert ( c*r>0 );
#endif
    _col = c;
    _row = r;
    _data = ( int* ) malloc ( c*r*sizeof(int) );
    MemSet ( 0 );
}

int* IntMemMgr::Data()
{
    return _data;
}

int IntMemMgr::Size()
{
    return _col*_row;
}

void IntMemMgr::MemSet ( int v )
{
    int len = _col*_row;
    for ( int i=0; i<len; i++ ) {
        _data[i] = v;
    }
}

int IntMemMgr::Get ( int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c>=0 && r>=0 && c<_col&&r<_row );
#endif
    return _data[c*_row+r];
}

void IntMemMgr::Set ( int v, int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c>=0 && r>=0 && c<_col&&r<_row );
#endif
    _data[c*_row+r] = v;
}

void IntMemMgr::ValueAdd ( int v, int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c<_col&&r<_row );
#endif
    _data[c*_row+r] += v;
}

double IntMemMgr::Avg ( int c )       // col, return avg value;
{
    int total = 0;
    for ( int r=0; r<_row; r++ )  {
        total += Get ( c,r );
    }
    return ( ( double ) total ) /_row;
}

bool IntMemMgr::FindPeak ( int c, int& pr, int& pv ) // col, peak col, peak value;
{
    pr = -1;
    pv = 0;
    int peakvalue;
    int rlen = _row-2;
    for ( int r=2; r<rlen; r++ )  {
        peakvalue = Get ( c,r ) + ( Get ( c,r-1 ) +Get ( c,r+1 ) ) /2+ ( Get ( c,r-2 ) +Get ( c,r+2 ) ) /4;
        if ( peakvalue>pv ) {
            pr = r;
            pv = peakvalue;
        }
    }
    return pr!=-1;
}

bool IntMemMgr::ValueAt ( int v, int& col, int& row )
{
    for ( int c =0; c<_col; c++ ) {
        for ( int r =0; r<_row; r++ ) {
            if ( v==_data[c*_row+r] ) {
                col = c;
                row = r;
                return true;
            }
        }
    }

    return false;
}

void IntMemMgr::QuickSort ( int* a, int lo, int hi )
{
    //  lo is the lower index, hi is the upper index
    //  of the region of array a that is to be sorted
    int i=lo, j=hi, h;
    int x=a[ ( lo+hi ) /2];

    //  partition
    do {
        while ( a[i]<x ) {
            i++;
        }
        while ( a[j]>x ) {
            j--;
        }
        if ( i<=j ) {
            h=a[i];
            a[i]=a[j];
            a[j]=h;
            i++;
            j--;
        }
    }
    while ( i<=j );

    //  recursion
    if ( lo<j ) {
        QuickSort ( a, lo, j );
    }
    if ( i<hi ) {
        QuickSort ( a, i, hi );
    }
}

void IntMemMgr::DumpCSV ( const char* file, const char* hdr )
{
    FileMgr fm( file, "w" );
    fprintf( fm.Ptr(), "%s\n", hdr );
    for ( int i=0; i<_row; i++ ) {
        for ( int j=0; j<_col; j++ ) {
            fprintf( fm.Ptr(), "%d,",_data[j*_row+i]);
        }
        fprintf( fm.Ptr(), "\n" );
    }
    fprintf( fm.Ptr(), "\n" );
    fm.Close();
}

bool IntMemMgr::_IsHexChar( const char c )
{
    if ( c>='0' && c<='9' )  return true;
    if ( c>='A' && c<='Z' )  return true;
    if ( c>='a' && c<='z' )  return true;
    if ( c=='-' )  return true;

    return false;
}

int IntMemMgr::LoadBinCSV( const char* fn, int col, bool isHex, int startLine, int startCol )
{
    //assert(_data==0);
    FileMgr fm(fn,"r");
    if ( !fm.Ptr() ) {
        assert(0);
        return 0;
    }

    _col = col;
    _row = 0;
    if ( _data ) free(_data);

    char line[8*1024];
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        _row++;
    }

    assert(_row);

    _data = (int*) malloc( _col*_row*sizeof(int) );
    //MemSet ( 0 );

    _row = _row-startLine;


    fseek ( fm.Ptr(),0,SEEK_SET );
    int v=0,vidx=0,idx=0;
	size_t len;
    int lineCnt = -1;
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        lineCnt++;
        if (lineCnt<startLine) continue;

        vidx=idx=0;
        len = strlen(line);
        for ( unsigned int i=0; i<len; i++ ) {
            if ( ',' == (line[i]) ) {
                line[i] = '\0';

				if(vidx < startCol){
					vidx++;
					idx = i+1;
					continue;
				}

                if ( isHex ) {
                    sscanf(&line[idx],"%X",&v);
                }
                else {
                    v = atoi(&line[idx]);
                }
                Set ( v,vidx,lineCnt-startLine );
                vidx++;
                idx = i+1;
            }
            if( vidx>=col) break;
        }
        if ( vidx>0 && vidx<col && (len-idx)>1)
        {
            if ( isHex ) {
                sscanf(&line[idx],"%X",&v);
            }
            else {
                v = atoi(&line[idx]);
            }
            Set(v,vidx,lineCnt-startLine);
            vidx++;
        }
    }
    fm.Close();
    return _row;
}
int IntMemMgr::LoadCSV( const char* fn, int col, bool isHex, int startLine )
{
    //assert(_data==0);
    FileMgr fm(fn,"r");
    if ( !fm.Ptr() ) {
        assert(0);
        return 0;
    }

    _col = col;
    _row = 0;
    if ( _data ) free(_data);

    char line[8*1024];
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        _row++;
    }

    assert(_row);

    _data = (int*) malloc( _col*_row*sizeof(int) );
    //MemSet ( 0 );

    _row = _row-startLine;


    fseek ( fm.Ptr(),0,SEEK_SET );
    int v=0,vidx=0,idx=0, len;
    int lineCnt = -1;
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        lineCnt++;
        if (lineCnt<startLine) continue;

        vidx=idx=0;
        len = strlen(line);
        for ( int i=0; i<len; i++ ) {
            if ( !_IsHexChar(line[i]) ) {
                line[i] = '\0';
                /*if ( (i-idx)<2 ) {
                    idx = i;
                    continue;
                }*/
                if ( isHex ) {
                    sscanf(&line[idx],"%X",&v);
                }
                else {
                    v = atoi(&line[idx]);
                }
                Set ( v,vidx,lineCnt-startLine );
                vidx++;
                idx = i+1;
            }
            if( vidx>=col) break;
        }
        if ( vidx>0 && vidx<col && (len-idx)>1)
        {
            if ( isHex ) {
                sscanf(&line[idx],"%X",&v);
            }
            else {
                v = atoi(&line[idx]);
            }
            Set(v,vidx,lineCnt-startLine);
            vidx++;
        }
    }
    fm.Close();
    return _row;
}


// *a[len][idx,cnt]
void _quickSort ( int** a, int lo, int hi, int len )
{
    //  lo is the lower index, hi is the upper index
    //  of the region of array a that is to be sorted
    int i=lo, j=hi;
    int h0,h1;
    int idx = ( lo+hi ) /2;
    int x=a[idx][1];
    int y=a[idx][0];

    //  partition
    do {
        while ( i<len && ( a[i][1]<x || ( a[i][1]==x && a[i][0]<y ) ) ) {
            i++;
        }
        while ( j>=0 && ( a[j][1]>x || ( a[j][1]==x && a[j][0]>y ) ) ) {
            j--;
        }
        if ( i<=j ) {
            h0 = a[i][0];
            h1 = a[i][1];

            a[i][0]=a[j][0];
            a[i][1]=a[j][1];

            a[j][0]=h0;
            a[j][1]=h1;

            i++;
            j--;
        }
    }
    while ( i<=j );

    //  recursion
    if ( lo<j ) {
        _quickSort ( a, lo, j, len );
    }
    if ( i<hi ) {
        _quickSort ( a, i, hi, len );
    }
}

bool IntMemMgr::Sort ( int slen )
{
    int len = _col*_row;
    if ( slen>len ) {
        assert ( 0 );
        return false;
    }

    for ( int i=0; i< ( slen-1 ); i++ ) {
        for ( int j= ( i+1 ); j<slen; j++ )  {
            if ( _data[i]<_data[j] ) {
                int tmp = _data[i];
                _data[i] = _data[j];
                _data[j] = tmp;
            }
        }
    }
    return true;
}

bool IntMemMgr::Sort2D ( int* seqs, int* freqs, int slen )
{
    //assert(_row==1);
    if ( _row!=1 ) {
        return false;
    }
    //_data = new int[c*r];
    int len = _col*_row;
    if ( slen>len ) {
        assert ( 0 );
        return false;
    }

    int** ary = new int*[slen];
    int max = 0;
    int i=0;
    for ( i=0; i<slen; ++i ) {
        ary[i] = new int[2];
        ary[i][0] = i;
        ary[i][1] = _data[i];
        if ( _data[i]>max ) {
            max = _data[i];
        }
    }
    _quickSort ( ary, 0, slen-1, slen );

    //    FILE* fptr = fopen( ".\\LOG\\quicksort.csv", "w+b" );
    //    for(int i = 0; i < len; ++i) {
    //        fprintf(fptr, "%d, %d, %d, %d\n",i, _data[i], ary[i][0],ary[i][1]);
    //    }
    //    fflush(fptr);
    //    fclose(fptr);

    //reverse get as desc.
    int cnt=0;
    for ( i=slen-1; i>=0; i-- )  {
        if ( cnt<slen ) {
            seqs[cnt] = ary[i][0];
            if ( freqs )  {
                freqs[cnt] = ary[i][1];
            }
        }
        delete [] ary[i];
        cnt++;
    }

    delete [] ary;

    return true;
}

UIntMemMgr::UIntMemMgr ( int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c*r>0 );
#endif
    _col = c;
    _row = r;
    _data = (unsigned long long*) malloc( _col*_row*sizeof(unsigned long long) );
    MemSet ( 0 );
}

UIntMemMgr::UIntMemMgr ()
{
    _col = 0;
    _row = 0;
    _data = 0;
}

UIntMemMgr::~UIntMemMgr()
{
    if ( _data )  {
        free(_data);
        _data = 0;
    }
}

unsigned long long* UIntMemMgr::Data()
{
    return _data;
}

int UIntMemMgr::Size()
{
    return _col*_row;
}

void UIntMemMgr::MemSet ( unsigned long long v )
{
    int len = _col*_row;
    for ( int i=0; i<len; i++ ) {
        _data[i] = v;
    }
}

unsigned long long UIntMemMgr::Get ( int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c>=0 && r>=0 && c<_col&&r<_row );
#endif
    return _data[c*_row+r];
}

void UIntMemMgr::Set ( unsigned long long v, int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c>=0 && r>=0 && c<_col&&r<_row );
#endif
    _data[c*_row+r] = v;
}

void UIntMemMgr::ValueAdd ( unsigned long long v, int c, int r )
{
#ifdef _DEBUGMEM
    assert ( c<_col&&r<_row );
#endif
    _data[c*_row+r] += v;
}

void UIntMemMgr::Swap( int c0, int c1 )
{
#ifdef _DEBUGMEM
	assert(c0<_col&&c1<_col);
#endif
	unsigned long long tmp = _data[c0*_row];
	_data[c0*_row] = _data[c1*_row];
	_data[c1*_row] = tmp;
}

void UIntMemMgr::DumpCSV ( const char* file, const char* hdr )
{
    FileMgr fm( file, "w" );
    fprintf( fm.Ptr(), "%s\n", hdr );
    for ( int i=0; i<_row; i++ ) {
        for ( int j=0; j<_col; j++ ) {
            fprintf( fm.Ptr(), "%llu,",_data[j*_row+i]);
        }
        fprintf( fm.Ptr(), "\n" );
    }
    fprintf( fm.Ptr(), "\n" );
    fm.Close();
}

bool UIntMemMgr::_IsHexChar( const char c )
{
    if ( c>='0' && c<='9' )  return true;
    if ( c>='A' && c<='Z' )  return true;
    if ( c>='a' && c<='z' )  return true;
    if ( c=='-' )  return true;

    return false;
}

int UIntMemMgr::LoadCSV( const char* fn, int col, bool isHex, int startLine )
{
    assert(_data==0);
    FileMgr fm(fn,"r");
    if ( !fm.Ptr() ) {
        assert(0);
        return 0;
    }

    _col = col;
    _row=0;
    char line[8*1024];
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        _row++;
    }

    assert(_row);

    _data = (unsigned long long*) malloc( _col*_row*sizeof(unsigned long long) );
    //MemSet ( 0 );

    _row = _row-startLine;


    fseek ( fm.Ptr(),0,SEEK_SET );
    unsigned long long v=0;
    int vidx=0,idx=0, len;
    int lineCnt = -1;
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        lineCnt++;
        if (lineCnt<startLine) continue;

        vidx=idx=0;
        len = strlen(line);
        for ( int i=0; i<len; i++ ) {
            if ( !_IsHexChar(line[i]) ) {
                line[i] = '\0';
                /*if ( (i-idx)<2 ) {
                    idx = i;
                    continue;
                }*/
                if ( isHex ) {
                    sscanf(&line[idx],"%llX",&v);
                }
                else {
                    v = atoi(&line[idx]);
                }
                Set ( v,vidx,lineCnt-startLine );
                vidx++;
                idx = i+1;
            }
            if( vidx>=col) break;
        }
        if ( vidx>0 && vidx<col && (len-idx)>1)
        {
            if ( isHex ) {
                sscanf(&line[idx],"%llX",&v);
            }
            else {
                v = atoi(&line[idx]);
            }
            Set(v,vidx,lineCnt-startLine);
            vidx++;
        }
    }
    fm.Close();
    return _row;
}

/////////////////////////////////
Int3DMgr::Int3DMgr ( int c, int r, int p )
{
#ifdef _DEBUGMEM
    assert ( c*r*p>0 );
#endif
    _col = c;
    _row = r;
    _phase = p;
    _len = c*r*p;
    _data = (int*) malloc( _len*sizeof(int) );
    MemSet ( 0 );
}

Int3DMgr::~Int3DMgr()
{
    if ( _data )  {
        free(_data);
        _data = 0;
    }
}

void Int3DMgr::MemSet ( int v )
{
    for ( int i=0; i<_len; i++ ) {
        _data[i] = v;
    }
}

int Int3DMgr::Get ( int c, int r, int p )
{
#ifdef _DEBUGMEM
    assert ( c>=0 && r>=0 && c<_col&&r<_row&&p<_phase );
#endif

    int pos = ( c*_row+r ) *_phase+p;
    assert( pos<_len );
    //assert ( pos<_col*_row*_phase );
    return _data[pos];
}

void Int3DMgr::Set ( int v, int c, int r, int p )
{
#ifdef _DEBUGMEM
    assert ( c>=0 && r>=0 && p>=0 && c<_col&&r<_row&&p<_phase );
#endif

    int pos = ( c*_row+r ) *_phase+p;
    assert( pos<_len );
    //assert ( pos<_col*_row*_phase );
    _data[pos] = v;
}


DoubleMemMgr::DoubleMemMgr ( int c, int r )
{
#ifdef _DEBUGMEM
    assert(c*r>0);
#endif
    _col = c;
    _row = r;
    _data = (double*) malloc( _col*_row*sizeof(double) );
    MemSet ( 0 );
}

DoubleMemMgr::DoubleMemMgr ()
{
    _col = 0;
    _row = 0;
    _data = 0;
}

double DoubleMemMgr::Get ( int c, int r )
{
#ifdef _DEBUGMEM
    assert(c>=0 && r>=0 && c<_col&&r<_row);
#endif
    return _data[c*_row+r];
}

void DoubleMemMgr::Set ( double v, int c, int r )
{
#ifdef _DEBUGMEM
    assert(c>=0 && r>=0 && c<_col&&r<_row);
#endif
    _data[c*_row+r] = v;
}

DoubleMemMgr::~DoubleMemMgr()
{
    if ( _data )  {
        free(_data);
        _data = 0;
    }
}

void DoubleMemMgr::MemSet ( double v )
{
    int len = _col*_row;
    for ( int i=0; i<len; i++ ) {
        _data[i] = v;
    }
}

double* DoubleMemMgr::Data()
{
    return _data;
}

int DoubleMemMgr::Size()
{
    return _col*_row;
}

void DoubleMemMgr::DumpCSV ( const char* file, const char* hdr )
{
    FileMgr fm( file, "w" );
    fprintf( fm.Ptr(), "%s\n", hdr );
    for ( int i=0; i<_row; i++ ) {
        for ( int j=0; j<_col; j++ ) {
            fprintf( fm.Ptr(), "%.05f,",_data[j*_row+i]);
        }
        fprintf( fm.Ptr(), "\n" );
    }
    fprintf( fm.Ptr(), "\n" );
    fm.Close();
}

int DoubleMemMgr::LoadCSV( const char* fn, int col, int startLine )
{
    //assert(_data==0);
    FileMgr fm(fn,"r");
    if ( !fm.Ptr() ) {
        assert(0);
        return 0;
    }

    _col = col;
    _row = 0;
    char line[2048];
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        _row++;
    }

    assert(_row);
    _data = ( double* ) malloc ( _col*_row*sizeof(double) );
    //MemSet ( 0 );

    _row = _row-startLine;


    fseek ( fm.Ptr(),0,SEEK_SET );
    int vidx=0,idx=0, len;
    double v=0;
    int lineCnt = -1;
    while( fgets(line,sizeof(line),fm.Ptr()) ) {
        lineCnt++;
        if (lineCnt<startLine) continue;

        vidx=idx=0;
        len = strlen(line);
        for ( int i=0; i<len; i++ ) {
            if ( line[i]==',' ) {
                line[i] = '\0';
                if ( (i-idx)<2 ) {
                    idx = i;
                    continue;
                }
                v = atof(&line[idx]);
                Set ( v,vidx,lineCnt-startLine );
                vidx++;
                idx = i+1;
            }
            if( vidx>=col) break;
        }
        if ( vidx>0 && vidx<col && (len-idx)>1)
        {
            v = atof(&line[idx]);
            Set(v,vidx,lineCnt-startLine);
            vidx++;
        }
    }
    fm.Close();
    return _row;
}

/////////////////////////////////

FileMemMgr::FileMemMgr ( const wchar_t* fn, bool readOnly )
{
    _readOnly = readOnly;
    swprintf ( _wfn, fn );
    if ( _readOnly ) {
        _fptr = _wfopen ( fn, L"rb" );
    }
    else {
        _wremove ( fn );
        _fptr = _wfopen ( fn, L"w+b" );
    }
    assert ( _fptr );
}

FileMemMgr::FileMemMgr ( const char* fn, bool readOnly )
{
    _readOnly = readOnly;
    sprintf ( _fn, fn );
    if ( _readOnly ) {
        _fptr = fopen ( fn, "rb" );
    }
    else {
        remove ( fn );
        _fptr = fopen ( fn, "w+b" );
    }
    assert ( _fptr );
}

wchar_t* FileMemMgr::WFileName()
{
    return _wfn;
}

char* FileMemMgr::FileName()
{
    return _fn;
}

FileMemMgr::~FileMemMgr()
{
    Close();
}

void FileMemMgr::Close()
{
    if ( _fptr ) {
        if ( !_readOnly )  {
            fflush ( _fptr );
        }
        fclose ( _fptr );
        _fptr = 0;
    }
}

void FileMemMgr::Rewind()
{
    fseek ( _fptr ,0, SEEK_SET );
}

long FileMemMgr::Read ( long pos, unsigned char* buf, int size )
{
    if ( fseek ( _fptr , pos, SEEK_SET ) !=0 ) {
        return -1;
    }
    int r = fread ( buf,1,size,_fptr );
    assert ( r==size );
    return r;
}

long FileMemMgr::Read ( unsigned char* buf, int size )
{
    int r = fread ( buf,1,size,_fptr );
    assert ( r==size );
    return r;
}

long FileMemMgr::Append ( unsigned char* buf, int size )
{
    assert ( !_readOnly );
    //fseek ( _fptr , 0, SEEK_END );
    int r = fwrite ( buf, 1, size, _fptr );
    assert ( r==size );
    return r;
}

bool FileMemMgr::End()
{
    return fseek ( _fptr , 0, SEEK_END ) ==0;
}

FILE* FileMemMgr::Ptr()
{
    return  _fptr;
}

MemStr::MemStr ( int len )
{
    _size = len;
    _data = (char*) malloc( (_size+1)*sizeof(char) );
    _data[_size]='\0';
    SetEmpty();
}

MemStr::~MemStr()
{
    free(_data);
}

int MemStr::Size()
{
    return _size;
}

char* MemStr::Data()
{
    return _data;
}

void MemStr::SetEmpty()
{
    _data[0]='\0';
    _data[1]='\0';
}

bool MemStr::IsEmpty()
{
    return _data[0]=='\0';
}

const char* MemStr::Cat ( const char* fmt, ... )
{
    char buf[512];
    va_list arglist;
    va_start ( arglist, fmt );
    _vsnprintf ( buf, 512, fmt, arglist );
    va_end ( arglist );
    strcat ( _data,buf );
    return _data;
}

FileMgr::FileMgr ()
    : _fptr(0)
{
}

FileMgr::FileMgr ( const wchar_t* filename, const char* mode  )
    : _fptr(0)
{
    Open(filename,mode);
}

FileMgr::FileMgr ( const char* filename, const char* mode )
    : _fptr(0)
{
    Open(filename,mode);
}

void FileMgr::Open(const wchar_t* filename, const char* mode)
{
    assert(!_fptr);
    sprintf ( _option, mode );
    wchar_t opt[32];
    size_t s = mbstowcs ( opt, mode, sizeof ( _option ) );
    assert ( s>0 );
    _fptr = _wfopen ( filename, opt );
    assert ( _fptr );
}
void FileMgr::Open(const char* filename, const char* mode)
{
    assert(!_fptr);
    sprintf ( _option, mode );
    _fptr = fopen ( filename, mode );
    //assert(_fptr);
}

FileMgr::~FileMgr()
{
    Close();
}

size_t FileMgr::Size()
{
    fseek ( _fptr , 0 , SEEK_END );
    size_t size = ftell ( _fptr );
    rewind ( _fptr );
    return size;
}

void FileMgr::Close()
{
    if ( !_fptr ) {
        return;
    }

    if ( strstr ( _option,"w" ) ) {
        fflush ( _fptr );
    }

    fclose ( _fptr );
    _fptr = 0;
}

FILE* FileMgr::Ptr()
{
    return     _fptr;
}

void FileMgr::Rewind()
{
    fseek ( _fptr ,0, SEEK_SET );
}



_FileArrayNode::_FileArrayNode( FILE* file, const char* fn )
{
    _file = file;
    int slen = strlen(fn);
    assert(slen);
    _fnfull = new char[slen+1];
    strcpy(_fnfull,fn);

    _fn = new char[slen+1];
    for ( int i=slen-1; i>=0; i-- ) {
        if ( fn[i]=='\\' || fn[i]=='/' ) {
            fn = &fn[i+1];
            break;
        }
    }
    strcpy(_fn,fn);

    _next = 0;
}

_FileArrayNode::~_FileArrayNode()
{
    delete[] _fn;
    delete[] _fnfull;
}

FileArrayMgr* FileArrayMgr::_instance=0;

bool FileArrayMgr::Valid()
{
    return _instance!=0;
}

void FileArrayMgr::Create(const char* logfolder)
{
    assert(!_instance);
    _instance = new FileArrayMgr(logfolder);
}

void FileArrayMgr::Free()
{
    assert(_instance);
    delete _instance;
}

FileArrayMgr::FileArrayMgr(const char* logfolder)
{
    int slen = strlen(logfolder);
    assert(slen);
    _logFolder = new char[slen+1];
    strcpy(_logFolder,logfolder);

    _root = _end = 0;
    _size = 0;
}

const char* FileArrayMgr::LogFolder(const char* folder )
{
    if ( _instance ) {
        return _instance->_logFolder;
    }
    return folder;
}

FileArrayMgr::~FileArrayMgr()
{
    delete[] _logFolder;

    _FileArrayNode* item = _root;
    while ( item ) {
        _FileArrayNode* next = item->_next;
        delete item;
        item = next;
    }
}

FILE* FileArrayMgr::GetFile( const char* fn )
{
    if ( _instance ) {
        _FileArrayNode* item = _instance->_root;
        while ( item ) {
            if ( strcmp(fn,item->_fn)==0 ) return item->_file;
            _FileArrayNode* next = item->_next;
            item = next;
        }
    }
    return 0;
}

void FileArrayMgr::AddFile( FILE* file, const char* fn )
{
    if ( !_instance ) return;

    _FileArrayNode* item = new _FileArrayNode( file, fn );
    if ( !_instance->_root ) {
        _instance->_root = item;
    }
    else {
        _instance->_end->_next = item;
    }
    _instance->_end = item;
    _instance->_size++;
}

bool FileArrayMgr::GroupFiles( const char* fn, bool delfile )
{
    assert(_instance->_size);
    if ( !_instance ) return false;

    FILE* fptr = fopen(fn,"wb");
    fwrite( &_instance->_size,1,sizeof(unsigned int),fptr);

    _FileArrayNode* item = _instance->_root;
    while ( item ) {
        //printf("load file %s ",item->_fn);
        unsigned int slen = strlen(item->_fn)+1;
        FileMgr fm( item->_fnfull, "rb" );

        fseek ( fm.Ptr(), 0L, SEEK_END );
        unsigned int len = ftell ( fm.Ptr() );
        fseek ( fm.Ptr(),0,SEEK_SET );
        CharMemMgr buf ( len );
        fread ( buf.Data(), 1, len, fm.Ptr() );
        //printf("len %d \n",len);

        fwrite( &slen,1,sizeof(unsigned int),fptr);
        fwrite( item->_fn,1,slen,fptr);
        fwrite( &len,1,sizeof(unsigned int),fptr);
        fwrite( buf.Data(),1,len,fptr);
        //fflush(fptr);

        item = item->_next;
        if ( delfile ) {
            ::remove ( item->_fn );
        }
        fm.Close();
    }

    fflush(fptr);
    fclose(fptr);

    return 0;
}

bool FileArrayMgr::ExtractFiles( const char* fn )
{
    FILE* fptr = fopen(fn,"rb");

    int size = 0;
    fread(&size,1,sizeof(unsigned int),fptr);

    int fnlen=0,flen=0;
    char nfn[512];
    for ( int f=0; f<size; f++ ) {
        fread(&fnlen,1,sizeof(unsigned int),fptr);
        fread(nfn,1,fnlen,fptr);
        fread(&flen,1,sizeof(unsigned int),fptr);
        CharMemMgr data(flen);
        fread(data.Data(),1,flen,fptr);
        FileMgr fm( nfn, "wb" );
        fwrite( data.Data(),1,flen,fm.Ptr());
        fflush(fm.Ptr());
        fm.Close();
    }

    fclose(fptr);

    return 0;
}

int FileArrayMgr::Size()
{
    return _instance->_size;
}


Utilities::Utilities(void)
{

}
/*
Check data accuracy by XOR,

eg, input data = {0x00 0xaa 0x55 0xff 0xff 0x55 0xaa 0x00} , and the real data len is 4

the input parameter:
data_len => 4
and the output will be 0.
*/
int Utilities::FindRightData(unsigned char * input_buf, int data_len, int max_search_len, unsigned char * output_buf)
{
	int len2 = data_len*2;
	int search_max_loop = max_search_len/len2;
	int done=1;
	unsigned char * ip = input_buf;
	int ok_idx=0;
	int cannt_find=0;
	unsigned char * pMemCheck = new unsigned char [data_len];
	memset(pMemCheck, 0, data_len);
	for (int i=0; i<search_max_loop; i++) {
		done=1;
		cannt_find=0;
		int base_shift = len2*i;
		for (int data_addr=0; data_addr<data_len; data_addr++) {
			int addr1 = base_shift + data_addr;
			int addr2 = base_shift + data_addr + data_len;
			if (pMemCheck[data_addr])
				continue;
			if ( 0xFF == (ip[addr1]^ip[addr2]) ) {
				output_buf[data_addr] = ip[addr1];
				pMemCheck[data_addr]=1;
			} else {
				done=0;
			}
		}
		if (!done)
			cannt_find=1;
		else
			break;
	}

	if (pMemCheck)
		delete [] pMemCheck;
	if (cannt_find)
		return 1;//fail
	else
		return 0;//ok
}

int Utilities::Count1 ( unsigned char *buf, int len )
{
	int cnt = 0;
	for ( int i=0; i<len; i++ ) {
		char by = buf[i];
		for ( int b=0; b<8; b++ ) {
			cnt+= ( by>>b ) & 0x01;
		}
	}
	return cnt;
}
int Utilities::MakeRandomErrorBitPerK(unsigned char * buffer1, int len,  int errbit)
{
	int SEGMENT_SZ = len/1024;
	for (int segment_idx = 0; segment_idx < SEGMENT_SZ; segment_idx++)
	{
		MakeRandomErrorBit(&buffer1[segment_idx*1024], 1024, errbit);
	}
	return 0;
}

int Utilities::MakeRandomErrorBit(unsigned char * buffer1, int len,  int errbit)
{

	int * inverse_arry =  new int[len*8];
	memset((unsigned char *)inverse_arry, 0, len*8*sizeof(int));
	int target_bit_idx = 0;
	bool found_new_err_idx = false;
	for (int idx=0; idx<errbit ; idx++)
	{
		do {
			found_new_err_idx = false;
			target_bit_idx = rand()%(len*8);
			if (!inverse_arry[target_bit_idx])
			{
				inverse_arry[target_bit_idx]=1;
				found_new_err_idx = true;
			}

		} while (!found_new_err_idx);


		int byte_addr = target_bit_idx/8;
		int bit_addr = target_bit_idx%8;
		buffer1[byte_addr] = buffer1[byte_addr] ^ (1<<bit_addr);
	}
	if (inverse_arry)
		delete [] inverse_arry;
	return 0;
}

int Utilities::DiffBit(unsigned char * buffer1, unsigned char * buffer2, int buffer_size)
{
	unsigned char * temp_buf = new unsigned char[buffer_size];

	for (int idx=0; idx<buffer_size; idx++) {
		temp_buf[idx] = (unsigned char)(buffer1[idx] ^ buffer2[idx]);
	}
	int count1 = Count1(temp_buf, buffer_size);
	delete [] temp_buf;
	return count1;
}
/*return 0 if diff less then threshold, return 1 if diff bigger then threshold*/
bool Utilities::DiffBitCheckPerK(unsigned char * buffer1, unsigned char * buffer2, int buffer_size, int threshold, int * max_diff_per_k, int * total_diff)
{
	int quotient = buffer_size/1024;
	int remainder = buffer_size%1024;
	int _max_diff_per_k = -1;
	int current_diff = 0;
	int _total_diff = 0;
	for (int iter=0; iter < quotient; iter++)
	{
		current_diff = DiffBit(&buffer1[iter*1024], &buffer2[iter*1024], 1024);
		_total_diff += current_diff;
		if (current_diff > _max_diff_per_k)
			_max_diff_per_k = current_diff;
	}
	current_diff = 0;
	int remainder_threshold = threshold * remainder / 1024;
	if (remainder) 
	{
		current_diff = DiffBit(&buffer1[quotient*1024], &buffer2[quotient*1024], remainder);

		_total_diff += current_diff;
		if (current_diff > _max_diff_per_k)
			_max_diff_per_k = current_diff;

	}

	if (max_diff_per_k)
		*max_diff_per_k = _max_diff_per_k;
	if (total_diff)
		*total_diff = _total_diff;
	//condition check
	if ( (_max_diff_per_k > threshold) || (current_diff > remainder_threshold))
		return 1;

	return 0;
}