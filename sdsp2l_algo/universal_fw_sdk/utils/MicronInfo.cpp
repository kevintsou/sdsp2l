
#include "../ufw/stdafx.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "MIcronInfo.h"

#ifdef _DEBUG
#define _ASSERTINFO(v)  assert(v)
#define _DEBUGINFO(...)  printf(__VA_ARGS__)
#else
#define _ASSERTINFO(v)
#define _DEBUGINFO(...)
#endif

#pragma warning(disable: 4996)

MicronTimingInfo* MicronTimingInfo::_inst = 0;

inline bool _IsRECTag( char* ftable )
{
	return ( 0 == memcmp(&ftable[0], ALEMENT_KEY, strlen(ALEMENT_KEY))  );
   // return ('@'== ftable[0] && 'O'== ftable[1] && 'D'== ftable[2] && 'T'== ftable[3] && '@'== ftable[4] );
}
inline bool _IsENDTag( char* ftable )
{
    return ('@'== ftable[0] && 'E'== ftable[1] && 'N'== ftable[2] && 'D'== ftable[3] && '@'== ftable[4] );
}

inline unsigned char _HexToByte(const char *s)
{
    return
        (((s[0] >= 'A' && s[0] <= 'Z') ? (10 + s[0] - 'A') :
          (s[0] >= 'a' && s[0] <= 'z') ? (10 + s[0] - 'a') :
          (s[0] >= '0' && s[0] <= '9') ? (s[0] - '0') : 0) << 4) |
        ((s[1] >= 'A' && s[1] <= 'Z') ? (10 + s[1] - 'A') :
         (s[1] >= 'a' && s[1] <= 'z') ? (10 + s[1] - 'a') :
         (s[1] >= '0' && s[1] <= '9') ? (s[1] - '0') : 0);
}

static int clk_mhz_to_idx[]={10,33,40,100,166,200,266,333,400,533,600};
unsigned char  MicronTimingInfo::GetParam(const char* ctrl, const char* flash, const char* flh_interface, int flh_clk )
{

    int clen = strlen(ctrl);
    int flen = strlen(flash);
    int mlen = strlen(flh_interface);

	bool isFoundParameter = 0;
    for( int r=0; r<_rowCnt; r++ ) {
        char* key = _keys[r];
        if ( strncmp(key,ctrl,clen) ) {
            continue;
        }

        key=key+clen+1;
        if ( strncmp(key,flash,flen) ) {
            continue;
        }

        key=key+flen+1;
        if ( strncmp(key,flh_interface,mlen) ) {
            continue;
        }

        return _datas[r][flh_clk];
    }
	assert( isFoundParameter );
    return 0;
}

MicronTimingInfo* MicronTimingInfo::Instance(const char* fn)
{
    if ( _inst ) return _inst;

    FILE* op = fopen( fn, "rb" );
    if ( !op ) {
        _DEBUGINFO("file not exist: %ls\n", (wchar_t *)fn);
		return 0;
    }
    fseek ( op, 0, SEEK_END );
    int lSize = (int)ftell (op);
    rewind (op);
    char* ftable = (char*)malloc(lSize);
    if (ftable) {
        int len = (int)fread(ftable, 1, lSize, op);
    }
    fclose(op);

    int startIdx=-1;
    int endIdx=-1;
    int recCnt=0;
    for ( int i=0; i<lSize; i++ ) {
        if ( -1== startIdx) {
            if ( _IsRECTag(&ftable[i]) ) {
                startIdx = i;
                recCnt++;
            }
            continue;
        }
        else {
            if ( _IsENDTag(&ftable[i]) ) {
                endIdx = i;
                break;
            }

            if ( _IsRECTag(&ftable[i]) ) {
                recCnt++;
            }
        }
    }

    //_DEBUGINFO("SSD_ODTInfo StartIdx=%08X, EndIdx=%08X recCnt=%d\n",startIdx,endIdx,recCnt);

    _inst = new MicronTimingInfo();
    _inst->_rowCnt = recCnt;
    _inst->_keys = new char*[_inst->_rowCnt];
    _inst->_datas = new unsigned char*[_inst->_rowCnt];
    for ( int i=0; i<_inst->_rowCnt; i++ ) {
        _inst->_keys[i] = 0;
        _inst->_datas[i] = 0;
    }

    recCnt=0;
    for ( int i=startIdx; i<endIdx; i++ ) {
        if ( !_IsRECTag(&ftable[i]) ) continue;

        int colCnt = 0;
        int starCol = 0;
        for ( int r=i; i<endIdx; r++ ) {
            if ( ','==ftable[r] ) {
                colCnt++;
                if ( 1==colCnt ) {
                    starCol=r+1;
                }

                if ( 4!=colCnt )  {
                    continue;
                }
                int endCol = r;
                int len = endCol-starCol;

                char* keydata = new char[len+1];
                memcpy(keydata,&ftable[starCol],len);
                keydata[len] = '\0';
                //_DEBUGINFO("keydata=%s\n",keydata);
                _inst->_keys[recCnt]=keydata;

                starCol = r+1;
                for( int r2=r+1; r2<=endIdx; r2++) {
                    if ( '\n'==ftable[r2] ) {
                        i=r; //faster;
                        endCol = r2-2;
                        break;
                    }
                }

                ////for delete use
                len = endCol-starCol;
                assert( (len%2)==0 );

                // char* sdata = new char[len+1];
                // memcpy(sdata,&ftable[starCol],len);
                // sdata[len] = '\0';
                // _DEBUGINFO("sdata=%s\n",sdata);

                //_DEBUGINFO("idata[%d]=",recCnt);
                unsigned char* data = new unsigned char[len];
                memset(data,0,len);
                for ( int l=0; l<len/2; l++ ) {
                    data[l]=_HexToByte(&ftable[starCol+l*2]);
                    //_DEBUGINFO("%02X",data[l]);
                }
                //_DEBUGINFO("\n");
                _inst->_datas[recCnt]=data;
                recCnt++;
                assert( recCnt<=_inst->_rowCnt);
            }
            else if ( '\n'==ftable[r] ) {
                i=r; //faster
                break;
            }
        }
    }

    free (ftable);
    return _inst;
}

void MicronTimingInfo::Free()
{
    if ( _inst ) {
        delete _inst;
        _inst = 0;
    }
}

MicronTimingInfo::MicronTimingInfo()
    : _keys(0)
    , _datas(0)
{

}

MicronTimingInfo::~MicronTimingInfo()
{
    if ( !_keys ) {
        return;
    }

    for( int i=0; i<_rowCnt; i++ ) {
        if ( _keys[i] ) {
            delete[] _keys[i];
        }
        if ( _datas[i] ) {
            delete[] _datas[i];
        }
    }
    delete[] _keys;
    delete[] _datas;
}
