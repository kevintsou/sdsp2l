#include "stdafx.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <windows.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <exception>
#include <wchar.h>
#include <assert.h>
#include "../utils/MicronInfo.h"
#ifndef _USE_OLD_PARAMETER

#include "MyFlashInfo.h"


#ifdef _DEBUG
#define _ASSERTINFO(v)  assert(v)
#else
#define _ASSERTINFO(v) 
#endif

//#define PAGE_21  0x21
//#define PAGE_35  0x35
//#define PAGE_43  0x43
//#define PAGE_44  0x44
//#define PAGE_45  0x45
//#define PAGE_46  0x46
//#define PAGE_48  0x48
//#define PAGE_61  0x61

#pragma warning(disable: 4996) 

MyFlashInfo* MyFlashInfo::_inst = 0;
MyFlashInfo* MyFlashInfo::Instance()
{
	if ( !_inst ) {
		_inst = new MyFlashInfo ();
	}
	return _inst;
}
MyFlashInfo* MyFlashInfo::Instance(const wchar_t* fn, const unsigned char* buf, long lSize)
{
    if ( !_inst ) {
        _inst = new MyFlashInfo ( fn, buf, lSize);
    }
	return _inst;
}

MyFlashInfo* MyFlashInfo::Instance(const char* fn, const unsigned char* buf, long lSize)
{

	//struct _stati64	fs;
	//struct _stat64 buf;
	//int result;
	//char buffer[] = "A line to output";

	/* Get data associated with "stat.c": */
	//if( !err ) error
	//int result = _tstati64( fn, &fs );	// 0, OK

	////result = _stat64( fn, &buf );

	////std::string str;
	//char test[1024] = {0};
	//sprintf_s(test, "Time modified : %s", ctime(&fs.st_atime ) );
	//struct tm *today = _localtime64( &fs.st_atime );
	

	wchar_t wfn[255];
	int s=0;
	while( true ) {
		wfn[s] = fn[s];
        if ( fn[s]=='\0' ) {
            break;
        }
		s++;
	} 
	
	//swprintf(wfn,L"%s",fn);
    if ( !_inst ) {
        _inst = new MyFlashInfo ( wfn,buf, lSize );
    }
	return _inst;
}

void MyFlashInfo::Free()
{
	if ( _inst ) {
		delete _inst;
		_inst = 0;
	}
	MicronTimingInfo::Free();
}

char _HexCharToInt(char n)
{
    if ( n >= '0' && n <= '9' ) {
        return (n-'0');
    }
    else if ( n >= 'A' && n <= 'F' ) {
        return (n-'A'+10);
    }
    else if ( n >= 'a' && n <= 'f' ) {
		return (n-'a'+10);
    }
    else {
        return 0;
	}
}

#define _INFO_CNT 40

MyFlashInfo::MyFlashInfo()
: _infos(0)
, _keys(0)
, _partNums(0)
, _flashCnt(0)
{
#if 0
	//FILE* op = _wfopen( fn, L"rb" );
	//fseek ( op , 0 , SEEK_END );
	//long lSize = sizeof(&fn);
	//rewind (op);
	//unsigned char* ftable = fn;//(unsigned char*)malloc(lSize);
	//long result = fread(ftable,1,lSize,op);
	//assert(lSize==result);
	//fclose(op);
	long lSize = strlen(_ftable);
	const char* ftable = _ftable;

	_flashCnt = -1;
	long idx=0;
	while( idx<lSize )  {
		if ( ftable[idx]=='\n' ) {
			_flashCnt++;
		}
		idx++;
	}
	printf("Flash Count=%d\n",_flashCnt);

	_infos = new unsigned char*[_flashCnt];
	_keys = new unsigned char*[_flashCnt];
	_partNums = new unsigned char*[_flashCnt];

	idx=0;
	//skip first line;
	while( ftable[idx]!='\n' )  {
		idx++;
	}
	idx++;

	//ABCDEFG,0x98,0xd7,0x84,0x93,0x72,0xd7,0x08,0x00,0x01,0x04,0x00,0x01,0x00,0x33,0x08,0x0e,0x13,0x80,0x00,0x00,0x01,0x01,0x00,0x24,0x00,0x01,0x00,0x27,0x08,0x02,0x01,0x00,0x00,0x19,0x21,0x00,0x54,0x00,0x30,0x08
	for ( int line=0; line<_flashCnt; line++ ) {
		int fidx = idx;
		while( idx<lSize && ftable[idx]!=',' )  {
			idx++;
		}
		//field 0
		int len = idx-fidx;
		_partNums[line] = 0;
		if ( len>0 ) {
			_partNums[line] = new unsigned char[len+1];
			memcpy(_partNums[line],ftable+fidx,len);
			_partNums[line][len]='\0';
		}
		printf("line: %d partnums[%s] key[",line,(char*)_partNums[line]);
		_keys[line] = new unsigned char[7];
		_infos[line] = new unsigned char[_INFO_CNT];
		//field flashid
		for ( int id=0;id<7; id++ ) {
			idx++;
			fidx = idx;
			while( ftable[idx]!=',' )  {
				idx++;
			}
			len = idx-fidx;
			assert( len==4 );
			_keys[line][id] = _HexCharToInt(ftable[fidx+2])<<4|_HexCharToInt(ftable[fidx+3]);
			printf("%02x",_keys[line][id]);

		}
		printf("]\n info[");
		//idx++;
		//fidx = idx;
		//infos
		for ( int id=0;id<_INFO_CNT; id++ ) {
			idx++;
			fidx = idx;
			while( ftable[idx]!=',' && ftable[idx]!='\r' && ftable[idx]!='\n' )  {
				idx++;
			}
			len = idx-fidx;
			assert( len==4 );
			_infos[line][id] = _HexCharToInt(ftable[fidx+2])<<4|_HexCharToInt(ftable[fidx+3]);
			printf("%02x",_infos[line][id]);
		}
		printf("]\n");
		while( idx<lSize && ftable[idx]!='\n'  )  {
			idx++;
		}
		idx++;
		if ( idx>=lSize ) {
			break;
		}
	}

	//delete[] ftable;
#endif
}

int MyFlashInfo::CheckFileDateTime()
{
	//SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
	FILETIME ftCreate, ftAccess, ftWrite;
	HANDLE        hFile;
	SYSTEMTIME    SystemTime;

	hFile = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
	FileTimeToSystemTime (&ftWrite, &SystemTime);
//	if(!(SystemTime.wYear >=2013 && SystemTime.wMonth >= 6 && SystemTime.wDay >= 25))
//	return 1;
	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;
	return 0;
}

void MyFlashInfo::LoadBuf(const unsigned char* ftable, long lSize)
{
	_flashCnt = -1;
	long idx=0;
//	OutputDebugString("load buff in");
	while( idx<lSize )  {
		if ( ftable[idx]=='\n' ) {
			_flashCnt++;
		}
		if ( idx>=lSize ) {
			break;
		}
		idx++;
	}
	//char temp[512];
	//sprintf_s(temp,"%d %d",_flashCnt,lSize);
	//OutputDebugString(temp);
	// printf("Flash Count=%d\n",_flashCnt);

	_infos = new unsigned char*[_flashCnt];
	_keys = new unsigned char*[_flashCnt];
	_partNums = new unsigned char*[_flashCnt];

	int colCnt = 1;
	idx=0;
	//skip first line;
	while( ftable[idx]!='\n' )  {
		if ( idx>=lSize ) {
			break;
		}
		idx++;
		if ( ftable[idx]==',' ) {
			colCnt++;
		}
	}
	idx++;
	//printf("Flash ColCount=%d\n",colCnt);
	_ASSERTINFO(colCnt>=_INFO_CNT);

	_colCnt = colCnt;
	for ( int line=0; line<_flashCnt; line++ ) {


//		sprintf_s(temp,"for line :%d %d %d",line,lSize,_flashCnt);
//		OutputDebugString(temp);

		int fidx = idx;
		while( idx<lSize && ftable[idx]!=',' )  {
			if ( idx>=lSize ) {
				break;
			}
			idx++;
		}

		//field 0
		int len = idx-fidx;

		_ASSERTINFO( len>=4 );

		_partNums[line] = 0;
		if ( len>0 ) {
			_partNums[line] = new unsigned char[len+1];
			memcpy(_partNums[line],ftable+fidx,len);
			_partNums[line][len]='\0';
		}


		//printf("line: %d partnums[%s] key[",line,(char*)_partNums[line]);
		_keys[line] = new unsigned char[7];
		_infos[line] = new unsigned char[_colCnt-8]; //partnumber + id , 8個col
		//field flashid

		for ( int id=0;id<7; id++ ) {
			idx++;
			fidx = idx;
			while( ftable[idx]!=',' )  {
				if ( idx>=lSize ) {
					break;
				}
				idx++;
			}
			len = idx-fidx;

			if(memcmp(_partNums[line],"AP_VER",6)!=0) {
				_ASSERTINFO( len==4 );
				if ( len!=4 ) {
					return;
				}
			}
			_keys[line][id] = _HexCharToInt(ftable[fidx+2])<<4|_HexCharToInt(ftable[fidx+3]);
			//printf("%02X",_keys[line][id]);

		}

		if((memcmp(_partNums[line],"AP_VER",6)==0))
		{
			this->CsvYear = _keys[line][0];
			this->CsvMonth = _keys[line][1];
			this->CsvDay = _keys[line][2];
			this->CsvCount = _keys[line][3];
			//printf("]\n");
			//break;
			//continue;
		}

		//printf("]\n info[");
		//idx++;
		//fidx = idx;
		//infos
		//sprintf_s(temp,"for col :%d %d",idx,_colCnt);
		//OutputDebugString(temp);
		for ( int id=0;id<_colCnt-8; id++ ) {
			idx++;
			fidx = idx;
			while( ftable[idx]!=',' && ftable[idx]!='\r' && ftable[idx]!='\n' )  {
				if ( idx>=lSize ) {
					break;
				}
				idx++;
			}
		
			len = idx-fidx;
			if(memcmp(_partNums[line],"AP_VER",6)!=0) {
				_ASSERTINFO( len==4 );
				if ( len!=4 ) {
//					OutputDebugString("load buff error");
					return;
				}
			}
			_infos[line][id] = _HexCharToInt(ftable[fidx+2])<<4|_HexCharToInt(ftable[fidx+3]);
			//printf("%02X",_infos[line][id]);
		}
		//printf("]\n");
		while( idx<lSize && ftable[idx]!='\n'  )  {
			if ( idx>=lSize ) {
				break;
			}
			idx++;
		}
		idx++;
		if ( idx>=lSize ) {
			break;
		}

	}
	//OutputDebugString("load buff out");
	_valid = true;
}

MyFlashInfo::MyFlashInfo(const wchar_t* fn, const unsigned char* buf, long lSize1)
: _infos(0)
, _keys(0)
, _partNums(0)
, _flashCnt(0)
, _valid(false)
{
	ForceCeCount = 0;
	ForceSortingCE = 0;
	_SupportWrongID = false;
	memset(b_page_outliers, 0, 32);
	memset(MarkPatternResults, 0, 64);
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 2; j++)
			Curve_Fitting_Theta[i][j] = 0;
	}

	if (fn[0] != 0)
	{
		//CString a = CString(fn);
		//OutputDebugString("MyFlashInfo : filename ");
		//OutputDebugString(a);
		//wmemset(filename,0,256);
		wmemcpy(filename,fn,wcslen(fn));
		// Convert the last-write time to local time.
		//FileTimeToSystemTime(&ftWrite, &stUTC);
		FILE* op = _wfopen( fn, L"rb" );

		if (op == NULL)
			return;
		
		fseek ( op , 0 , SEEK_END );
		long lSize = ftell (op);
		rewind (op);
		unsigned char* ftable = (unsigned char*)malloc(lSize);
		size_t result = fread(ftable,1,lSize,op);
		fclose(op);
		_ASSERTINFO(lSize==result);
		if ( lSize!=result ) {
			_ASSERTINFO(_valid);
			if (ftable) {
				free(ftable);
				ftable = NULL;
			}
		}
		//a.Format("csv size %d",lSize);
		//OutputDebugString(a);
		MyFlashInfo::LoadBuf(ftable, lSize);
		_ASSERTINFO(_valid);
		if (ftable) {
			free(ftable);
			ftable = NULL;
		}
	}
	else
	{
		MyFlashInfo::LoadBuf(buf, lSize1);
		_ASSERTINFO(_valid);
	}

 //   _flashCnt = -1;
 //   long idx=0;
	//while( idx<lSize )  {
 //       if ( ftable[idx]=='\n' ) {
 //           _flashCnt++;
 //       }
 //   	idx++;
 //   }
 //  // printf("Flash Count=%d\n",_flashCnt);
 //   
 //   _infos = new unsigned char*[_flashCnt];
 //   _keys = new unsigned char*[_flashCnt];
 //   _partNums = new unsigned char*[_flashCnt];
 //   
 //   int colCnt = 1;
 //   idx=0;
 //   //skip first line;
 //   while( ftable[idx]!='\n' )  {
 //   	idx++;
	//	if ( ftable[idx]==',' ) {
	//		colCnt++;
	//	}
 //   }
 //   idx++;
	////printf("Flash ColCount=%d\n",colCnt);
 //   _ASSERTINFO(colCnt>=_INFO_CNT);

	//_colCnt = colCnt;
 //   for ( int line=0; line<_flashCnt; line++ ) {
 //   	int fidx = idx;
	//	while( idx<lSize && ftable[idx]!=',' )  {
	//		idx++;
	//	}
	//	//field 0
	//	int len = idx-fidx;
	//	_ASSERTINFO( len>5 );
	//	_partNums[line] = 0;
	//	if ( len>0 ) {
	//		_partNums[line] = new unsigned char[len+1];
	//		memcpy(_partNums[line],ftable+fidx,len);
	//		_partNums[line][len]='\0';
	//	}

	//	
	//	printf("line: %d partnums[%s] key[",line,(char*)_partNums[line]);
	//	_keys[line] = new unsigned char[7];
	//	_infos[line] = new unsigned char[_colCnt-8]; //partnumber + id , 8個col
	//	//field flashid
	//	
	//	for ( int id=0;id<7; id++ ) {
	//		idx++;
	//		fidx = idx;
	//		while( ftable[idx]!=',' )  {
	//			idx++;
	//		}
	//		len = idx-fidx;

	//		if(memcmp(_partNums[line],"AP_VER",6)!=0) {
	//			_ASSERTINFO( len==4 );
	//			if ( len!=4 ) {
	//				goto end;
	//			}
	//		}
	//		_keys[line][id] = _HexCharToInt(ftable[fidx+2])<<4|_HexCharToInt(ftable[fidx+3]);
	//		printf("%02X",_keys[line][id]);

	//	}

	//	if((memcmp(_partNums[line],"AP_VER",6)==0))
	//	{
	//		this->CsvYear = _keys[line][0];
	//		this->CsvMonth = _keys[line][1];
	//		this->CsvDay = _keys[line][2];
	//		this->CsvCount = _keys[line][3];
	//		//printf("]\n");
	//		//break;
	//		//continue;
	//	}
	//
	//	printf("]\n info[");
	//	//idx++;
	//	//fidx = idx;
	//	//infos
	//	for ( int id=0;id<_colCnt-8; id++ ) {
	//		idx++;
	//		fidx = idx;
	//		while( ftable[idx]!=',' && ftable[idx]!='\r' && ftable[idx]!='\n' )  {
	//			idx++;
	//		}
	//		len = idx-fidx;
	//		if(memcmp(_partNums[line],"AP_VER",6)!=0) {
	//			_ASSERTINFO( len==4 );
	//			if ( len!=4 ) {
	//				goto end;
	//			}
	//		}
	//		_infos[line][id] = _HexCharToInt(ftable[fidx+2])<<4|_HexCharToInt(ftable[fidx+3]);
	//		printf("%02X",_infos[line][id]);
	//	}
	//	printf("]\n");
	//	while( idx<lSize && ftable[idx]!='\n'  )  {
	//		idx++;
	//	}
	//	idx++;
 //       if ( idx>=lSize ) {
 //           break;
 //       }

	//}
	//_valid = true;

}

MyFlashInfo::~MyFlashInfo()
{
	try
	{
		if ( !_infos ) {
			return;
		}
		
		for( int i=0;i<_flashCnt; i++ ) {
			if ( _infos[i] ) {
				delete[] _infos[i];
			}
			if ( _keys[i] ) {
				delete[] _keys[i];
			}
			if ( _partNums[i] ) {
				delete[] _partNums[i];
			}
		}
		delete[] _infos;
		delete[] _keys;
		delete[] _partNums;
	}
	catch( const std::exception &e){
		printf("%s", e.what());
		//char sDebugString[512] = {0};
		//sprintf(sDebugString,"MyFlashInfo::~MyFlashInfo-> %s",e.what());
		//OutputDebugString(sDebugString);
		assert(0);
	}
}




bool MyFlashInfo::GetFlashParamFromCSV(const unsigned char* flashID, FH_Paramater* param)
{
    if ( !flashID || !param ) {
        return false;
    }
	
	param->SIDType = 0x00;// sz_use
	param->FlhNumber = 0;
    param->bSetOBCBadColumn = param->bSetBadColumn = 0;
	for( int i=0;i<_flashCnt; i++ ) 
	{
		if ( memcmp ( _keys[i],flashID,6 )!=0 )   
			continue;

        {
			sprintf(param->PartNumber,"%s",_partNums[i]);
			
			param->bMaker = _keys[i][0];
			memcpy(param->id,&_keys[i][1],sizeof(param->id));
			
			memcpy(&param->Configed,_infos[i],_colCnt-8);
			param->ECC = param->ECCByte;

			param->DieNumber = 1<<param->DIE_Shift;
			//計算 die offset
			unsigned int DieBlock = (param->MaxBlock / param->DieNumber)-1;
			param->Block_Per_Die = param->MaxBlock/param->DieNumber;
			if(DieBlock > 16384)
				param->DieOffset = 32768;
			else if(DieBlock > 8192)
				param->DieOffset = 16384;
			else if(DieBlock > 4096)
				param->DieOffset = 8192;
			else if(DieBlock > 2048)
				param->DieOffset = 4096;
			else if(DieBlock > 1024)
				param->DieOffset = 2048;
			else if(DieBlock > 512)
				param->DieOffset = 1024;
			else
				param->DieOffset = 512;

			int pgk = (( param->PageSize_H<<8 & 0xFF00 ) | param->PageSize_L );
			if (pgk >= 4096)
				param->PageSizeK = (pgk/1024)/4*4;// param->SectorPerPage/2;
			else 
				param->PageSizeK = (pgk/1024)/2*2;// param->SectorPerPage/2;
			
            param->PageSize = ( ( param->PageSize_H<<8 & 0xFF00 ) | param->PageSize_L ) + param->PageSize_Ext;
			//verify for FW count capacity in HighLevel.
#ifdef _DEBUG
			switch ( param->PageSizeK ) {
				case 16:
					_ASSERTINFO(param->SecondCol_Shift==0x0F);
					break;
				case 12:
				    _ASSERTINFO(param->SecondCol_Shift==0x0B);
					break;
				case 8:
					_ASSERTINFO(param->SecondCol_Shift==0x0E);
					break;
				case 4:
					_ASSERTINFO(param->SecondCol_Shift==0x0D);
					break;
				case 2:
					_ASSERTINFO(param->SecondCol_Shift==0x0C);
					break;
				default:;
					_ASSERTINFO(0);
			}
#endif

			param->BigPage = (param->Page_H<<8 & 0xFF00) | param->Page_L;
            param->PagePerBlock = param->BigPage;
			param->MaxPage = param->PagePerBlock;
			if ( param->beD3 )  {
                param->PagePerBlock = param->BigPage/3;
			}
            param->SpareMode = ( param->Spare0Len << 4 ) | param->Spare1Len; //0x84// 
			param->Block_Per_Die = param->MaxBlock / param->DieNumber;


			if (param->bMaker == 0xb5)
			{
				param->bMaker = 0x2C;
				param->Type = 0;
				param->SIDType = 0xB5; // Boot code no DA CMD// sz_use
			}

			return true;
		}
	}
	
	return false;
}

bool MyFlashInfo::GetParameterMultipleCe(const unsigned char* flashID, FH_Paramater* param, const char *ini,int over4ce, unsigned char bUFSCE)
{
	int BUFFER_SZ = 512;
	int MAX_CE_ID_BUF = (BUFFER_SZ/16); 
	bool isFind = false;
	for (int ce_idx=0; ce_idx<MAX_CE_ID_BUF; ce_idx++)
	{
		isFind = GetParameter(&flashID[ce_idx*16], param, ini, over4ce, bUFSCE, true);
		if (isFind)
			break;
	}
	return isFind;
}

//where flashId[528] from extendID cmd.
bool MyFlashInfo::GetParameter(const unsigned char* flashID, FH_Paramater* param, const char *ini,int over4ce, unsigned char bUFSCE, bool bIsCheckExtendBuf)
{
    /*printf ( "%s, \"%02X%02X%02X%02X%02X%02X%02X%02X\"\n",__FUNCTION__,
             flashID[0],flashID[1],flashID[2],flashID[3],flashID[4],flashID[5],flashID[6],flashID[7] );*/
    if ( !flashID || !param || 
		//20190611: Bruce lee says this ID is not used.
		 (flashID[0]==0x98 && flashID[1]==0x3C && flashID[2]==0x98 && flashID[3]==0xB3 && flashID[4]==0x76 && flashID[5]==0xEB) ) {
        return false;
    }
	param->SIDType = 0x00; // Boot code no DA CMD// sz_use
	param->FlhNumber = 0;
    param->bSetOBCBadColumn = param->bSetBadColumn = 0;
	//CString tempStr;
	//tempStr.Format("_flashCnt = %d",_flashCnt);
	//OutputDebugString(tempStr);
	for( int i=0;i<_flashCnt; i++ ) {
		//tempStr.Format("i=%d %x %x %x %x %x\n",i,_keys[i][0],_keys[i][1],_keys[i][2],_keys[i][3],_keys[i][4]);
		//OutputDebugString(tempStr);
		if ( memcmp ( _keys[i],flashID,6 )!=0 )   continue;

        {
			sprintf(param->PartNumber,"%s",_partNums[i]);
			//memcpy(param->PartNumber,_partNums[i],strlen((char*)_partNums[i]));
			param->bMaker = _keys[i][0];
			memcpy(param->id,&_keys[i][1],sizeof(param->id));
			//int offset = sizeof(param->PartNumber)+sizeof(param->bMaker)+sizeof(param->id);
			/*Here copy all csv field to FH_Paramater* param*/
			memcpy(&param->Configed,_infos[i],_colCnt-8);
			param->ECC = param->ECCByte;
			param->DieNumber = 1<<param->DIE_Shift;
			//計算 die offset
			unsigned int DieBlock = (param->MaxBlock / param->DieNumber)-1;
			param->Block_Per_Die = param->MaxBlock/param->DieNumber;

			if(DieBlock > 16384)
				param->DieOffset = 32768;
			else if(DieBlock > 8192)
				param->DieOffset = 16384;
			else if(DieBlock > 4096)
				param->DieOffset = 8192;
			else if(DieBlock > 2048)
				param->DieOffset = 4096;
			else if(DieBlock > 1024)
				param->DieOffset = 2048;
			else if(DieBlock > 512)
				param->DieOffset = 1024;
			else
				param->DieOffset = 512;

			//switch( param->PageDefine ) {
			//	case 0x0B:
			//		param->SectorPerPage = 12;  //2K
			//		break;
   //             case 0x0C:
   //                 param->SectorPerPage = 4;  //2K
			//			   break;
   //             case 0x0D:
   //                 param->SectorPerPage = 8;  //4K
			//			   break;
   //             case 0x0E:
   //                 param->SectorPerPage = 16; //8K
			//			   break;
   //             case 0x0F:
   //                 param->SectorPerPage = 32; //16K
			//			   break;
   //             default:
   //                 param->SectorPerPage = 16;
			//}

			int pgk = (( param->PageSize_H<<8 & 0xFF00 ) | param->PageSize_L );
			if (pgk >= 4096)
				param->PageSizeK = (pgk/1024)/4*4;// param->SectorPerPage/2;
			else 
				param->PageSizeK = (pgk/1024)/2*2;// param->SectorPerPage/2;
			
            param->PageSize = ( ( param->PageSize_H<<8 & 0xFF00 ) | param->PageSize_L ) + param->PageSize_Ext;
			//verify for FW count capacity in HighLevel.
			#ifdef _DEBUG
			switch ( param->PageSizeK ) {
				case 16:
					_ASSERTINFO(param->SecondCol_Shift==0x0F);
					break;
				case 12:
				    _ASSERTINFO(param->SecondCol_Shift==0x0B);
					break;
				case 8:
					_ASSERTINFO(param->SecondCol_Shift==0x0E);
					break;
				case 4:
					_ASSERTINFO(param->SecondCol_Shift==0x0D);
					break;
				case 2:
					_ASSERTINFO(param->SecondCol_Shift==0x0C);
					break;
				default:;
					_ASSERTINFO(0);
			}
			#endif

			param->BigPage = (param->Page_H<<8 & 0xFF00) | param->Page_L;
            param->PagePerBlock = param->BigPage;
			param->MaxPage = param->PagePerBlock;
			if ( param->beD3 )  {
				if(param->Cell==0x10) 
					param->PagePerBlock = param->BigPage/4;//QLC
				else
					param->PagePerBlock = param->BigPage/3;
				param->MaxPage = param->PagePerBlock;
			}
			//計算page mapping需要多少byte
            param->SpareMode = ( param->Spare0Len << 4 ) | param->Spare1Len; //0x84// 
			//param->FlashSlot  =1;
			param->Block_Per_Die = param->MaxBlock / param->DieNumber;
//			printf("found\n");
			bool FlashMixMode = ((GetPrivateProfileIntA("Sorting", "FlashMixMode",0 ,ini))?true:false);
            if ( param->beD3 ) {
				getFlashInverseED3(param,NULL,flashID,FlashMixMode,over4ce,bUFSCE,bIsCheckExtendBuf);
				if((param->bMaker == 0x98 || param->bMaker == 0x45) && param->Design > 0x16) {//toshiba 才要 set bad column
					param->bSetOBCBadColumn = param->bSetBadColumn = 1;
				}
            }
            else {
				getFlashInverse(param,NULL,flashID,FlashMixMode,over4ce,bUFSCE);
            }


			param->PageMappingBytes = (((param->MaxPage+7) / 8) + 127) / 128*128;
			//要128的倍數
			if(param->PageMappingBytes % 128){
				assert(0);
				return false;
			}
			if((param->PageMappingBytes != 128 &&  param->MaxPage <= 1024) || (param->PageMappingBytes != 256 &&  param->MaxPage <=2048 && param->MaxPage >1024) || (param->PageMappingBytes != 384 &&  param->MaxPage >2048) ){
				assert(0);
				return false;
			}
			if(param->PageMappingBytes > 384){
				assert(0);
				return false;
			}
			if (param->bMaker == 0xb5)
			{
				param->bMaker = 0x2C;
				param->Type = 0;
				param->SIDType = 0xB5; // Boot code no DA CMD// sz_use
			}
			int vcc = GetPrivateProfileIntA("Sorting", "Vcc",0 ,ini);
			if(vcc)
				param->Vcc =vcc ;

			UpdateFlashTypeFromID(param);
			bool isSandisk_or_Tohiba = (0x45==flashID[0] ) || (0x98==flashID[0]);

			if (isSandisk_or_Tohiba){
				if ((flashID[5]&0x80) == 0x80)
					param->bToggle = 'T';
				else
					param->bToggle = 'N';

			}
			return true;
		}
	}
	//can't found match keys for this flash id, assert it.
	//ASSERT(0);
	return false;
}

int MyFlashInfo::UpdateFlashTypeFromID( FH_Paramater * param)
{
	//20190515 : Added Flash Type definition (moved from testini())
	param->bIsMicron3DTLC =0;
	if(param->bIs3D && param->beD3 && (param->bMaker==0x89|| param->bMaker==0x2c) ){
		param->bIsMicron3DTLC = 1;
		param->bIsMicronB16=0;
		param->bIsMicronB17=0;
		param->bIsMIcronB0kB=0;
		param->bIsMIcronN18=0;
		param->bIsMicronB36R = 0;
		param->bIsMicronB37R = 0;
		param->bIsMicronB47R = 0;
		param->bIsMicronQ1428A = 0;
		if((param->id[0]==0xa4 && param->id[1]==0x08 && param->id[2]==0x32 && param->id[3]==0xa1) ||
			(param->id[0]==0xC4 && param->id[1]==0x89 && param->id[2]==0x32 && param->id[3]==0xa1) ||
			(param->id[0]==0xa4 && param->id[1]==0x88 && param->id[2]==0x32 && param->id[3]==0xa1))
		{
			param->bIsMicronB16 = 1;
		}
		else if((param->id[0]==0xC4 && param->id[1]==0x08 && param->id[2]==0x32 && param->id[3]==0xa6) ||
			(param->id[0]==0xD4 && param->id[1]==0x89 && param->id[2]==0x32 && param->id[3]==0xa6) ||
			(param->id[0]==0xE4 && param->id[1]==0x8A && param->id[2]==0x32 && param->id[3]==0xa6))
		{
			param->bIsMicronB17 = 1;
		}
		else if((param->id[0]==0xB4 && param->id[1]==0x78 && param->id[2]==0x32 && param->id[3]==0xaa) ||
			(param->id[0]==0xCC && param->id[1]==0xF9 && param->id[2]==0x32 && param->id[3]==0xaa))
		{
			//B0K
			param->bIsMicronB17  = 0;
			param->bIsMicronB16  = 0;
			param->bIsMIcronB0kB = 1;
		}
		else if( (param->id[0]==0x84 && param->id[1]==0xd8 && param->id[2]==0x32 && param->id[3]==0xa1) ||
			(param->id[0]==0x84 && param->id[1]==0x58 && param->id[2]==0x32 && param->id[3]==0xa1))
		{
			param->bIsMicronB17  = 0;
			param->bIsMicronB16  = 0;
			param->bIsMIcronB0kB = 0;
			param->bIsMIcronB05  = 1;
		}
		else if(param->id[0]==0xd4 && param->id[1]==0x0c && param->id[2]==0x32 && param->id[3]==0xaa){
			param->bIsMIcronN18 = 1;
		}
		else if( (param->id[0]==0xd3 && param->id[1]==0x1c && param->id[2]==0x32 && param->id[3]==0xc6)||
			(param->id[0]==0xe3 && param->id[1]==0x9D && param->id[2]==0x32 && param->id[3]==0xc6) ||
			(param->id[0]==0xF3 && param->id[1]==0x9E && param->id[2]==0x32 && param->id[3]==0xC6) )
		{
			param->bIsMIcronN28 = 1;
		}
		else if((((param->id[0]==0xc4) || (param->id[0]==0xc3) || (param->id[0]==0xd4))
			&& ((param->id[1]==0x08) || (param->id[1]==0x18) || (param->id[1]==0x99))
			&& (param->id[2]==0x32)
			&& (param->id[3]==0xa2 || param->id[3]==0xe6)))
		{
			param->bIsMicronB27 = 1;
		}
		else if((param->id[0]==0xa3) && (param->id[1]==0x78) &&(param->id[2]==0x32) && (param->id[3]==0xe5) &&(param->id[4]==0x10))
		{
			param->bIsMicronB36R = 1;
		}
		else if ((param->id[0]==0xc3) && (param->id[1]==0x78) &&(param->id[2]==0x32) && (param->id[3]==0xea)  &&(param->id[4]==0x10))
		{
			param->bIsMicronB37R = 1;
		}
		else if ((param->id[0]==0xc3) && (param->id[1]==0x08) &&(param->id[2]==0x32) && (param->id[3]==0xea)  &&(param->id[4]==0x30))
		{
			param->bIsMicronB47R = 1;
		}
		else if((param->id[0]==0xd3) && (param->id[1]==0xac) &&(param->id[2]==0x32) && (param->id[3]==0xc6) )
		{
			param->bIsMicronQ1428A = 1;
		}
		else{
			assert(0);
			return false;
		}
	}
	if ((param->bMaker == 0x2c || param->bMaker == 0x89) && param->bIs3D && !param->beD3) {
		param->bIsMicron3DMLC = 1;
		if((param->id[4]==00) || (param->id[0]==0x84 && param->id[4]==0x04)){	// 0x04 L05B
			param->bIsMIcronL04 = 1;
		}
		else{
			param->bIsMIcronL06 = 1;
		}
	}
	if(param->bMaker == 0x45){
		if(param->id[0] == 0x3c && param->id[1] == 0x96 && param->id[2] == 0x93 && param->id[3] == 0x7e && param->id[4] == 0x50)
		{
			//write ib clock要降33
			param->bIsSD_INand_A19_MLC = 1;
		}
	}

	//20190515 : fixed bics definition for SanDisk flash. 	
	if ((param->bMaker == 0x98 || param->bMaker == 0x45) && param->bIs3D && param->beD3){
		if(((param->id[0x04]&0x0f)==0x02) || ((param->id[0x04] & 0x0f)==0x0A)){
			// 0x72: Bics3
			param->bIsBics3 = 1;
			if(param->id[0]==0x35 || (param->id[0]==0x37&&param->id[1]==0x99) ){
				param->bIsBics3pMLC=1;
			}
		}
		else if(((param->id[0x04]&0x0f)==0x03) || ((param->id[0x04] & 0x0f)==0x0b)){
			// 0x73: Bics4
			param->bIsBics4 = 1;
		}
		else if(((param->id[0x04]&0x0f)==0x01) || ((param->id[0x04] & 0x0f)==0x09)){
			param->bIsBics2 = 1;
		}
		else{
			assert(0);
			return false;
		}
	}
	if (param->bMaker == 0x98) {
		unsigned char BiCs4QLC1[] = {0x73,0x9C,0xB3,0x76,0xEB};
		unsigned char BiCs4QLC2[] = {0x73,0x9C,0xB3,0x76,0xE3};
		unsigned char BiCs4QLC3[] = {0x73,0x9C,0xB3,0x76,0x6B};
		if ( memcmp(param->id,BiCs4QLC1,sizeof(BiCs4QLC1))==0 )
		{
			param->bIsBiCs4QLC = 1;
		}
		if ( memcmp(param->id,BiCs4QLC2,sizeof(BiCs4QLC2))==0 )
		{
			param->bIsBiCs4QLC = 1;
		}
		if ( memcmp(param->id,BiCs4QLC3,sizeof(BiCs4QLC3))==0 )
		{
			param->bIsBiCs4QLC = 1;
		}

		unsigned char BiCs3QLC[] = {0x7C,0x1C,0xB3,0x76,0xF2};
		if ( memcmp(param->id,BiCs3QLC,sizeof(BiCs3QLC))==0 )
		{
			param->bIsBiCs3QLC = 1;
		}
	}
	if (param->bMaker == 0xEC && param->Design == 0x14 && param->beD3 == 0)
		param->bIsSamsungMLC14nm = 1;
	else
		param->bIsSamsungMLC14nm = 0;

	if ( param->bMaker==0x9B) {
		unsigned char YMTC_MLC_3D[] = {0x49,0x01,0x00,0x9B,0x49};
		unsigned char YMTC_TLC_3D[] = {0xC3,0x48,0x25,0x10,0x00};
		//if ( memcmp(param->id,YMTC_MLC_3D,sizeof(YMTC_MLC_3D))==0 ) 
		if(!param->beD3){
			param->bIsYMTC3D = 1;
			param->bIsYMTCMLC3D = 1;
		}
		else /*if (memcmp(param->id,YMTC_TLC_3D,sizeof(YMTC_TLC_3D))==0)*/ {
			param->bIsYMTC3D = 1;	
			param->bIsYMTCMLC3D=0;
		}
	}

	if (param->bMaker == 0xAd && param->beD3 &&param->Design <= 0x16){
		param->bIsHynix16TLC = 1;
		param->bHynixSwitchMode = 1;
	}

	if (param->bMaker == 0xAd && param->beD3 && param->bIs3D ==1){
		param->bIsHynix16TLC = 0;
		param->bIsHynix3DTLC = 1;
		param->bHynixSwitchMode = 1;
		if(param->id[4]==0x80)
			param->bIsHynix3DV3=1;
		if(param->id[4]==0x90)
			param->bIsHynix3DV4=1;
		if(param->id[4]==0xA0)
			param->bIsHynix3DV5=1;
		if(param->id[4]==0xB0)
			param->bIsHynix3DV6=1;
	}

	if (param->bMaker == 0x45 && param->id[0]==0x3E && param->id[1]==0x98 && param->id[2]==0xB3 && param->id[3]==0x76 && param->id[4]==0xF2){
		param->SDK9T23 = 1;
	}
	else{
		param->SDK9T23 = 0;
	}
	param->_AlignExtendBlock=0;

	return true;
}
#if 0
bool MyFlashInfo::ParamToBuffer ( FH_Paramater* param, unsigned char* buffer, const char* ini ,bool mixMode,FH_Paramater* param1)
{
    memset ( buffer,0,512 );

#ifdef OLD_SPEC
    if ( param->beD3 ) {
        buffer[0]=2;
    }
    else if ( param->bToggle=='T' ) {
        buffer[0]=1;
    }
    else if ( param->bToggle=='N' ) {
        buffer[0]=0;
    }

    if ( param->bToggle=='T' ) {
        buffer[0] |= 0x80;    // bit7=Toggle  // Jorry 2012.11.16
    }
    else if ( param->bToggle=='P' ) {
        buffer[0] |= 0x40;
    }
#else
    if ( param->beD3 ) {
        buffer[0]  = buffer[0]  | 0x08;
    }
    else if ( param->Cell == 0x08 ) { //TLC
        buffer[0]  = buffer[0]  | 0x04;
    }
    else if ( param->Cell == 0x04 ) {
    	buffer[0]  = buffer[0]  | 0x02;
    }

    if ( param->bToggle==0x54 ) {
        buffer[0]  = buffer[0]  | 0x11;
    }

#endif
    //if (param->bToggle=='T')          buffer[0] |= 0x80;      // bit7=Toggle  // Jorry 2012.11.16
    //else if ( param->bToggle=='P')                buffer[0] |= 0x40;      // bit6=PBA     // Jorry 2012.12.26

    buffer[0x01] = param->bMaker;               // 0x98=Toshiba, 0xEC=Samsung
    buffer[0x02] = param->FlhNumber;                // Total Chips
    buffer[0x03] = param->DieNumber;                // Die/Chip
    buffer[0x04] = param->Plane;                    // Plane/Chip

    int dieblk = param->MaxBlock / param->DieNumber;
    buffer[0x05] = ( dieblk/1024 ) + ( ( dieblk&0x3FF ) ? 1 : 0 );      // Zone/Die

    buffer[0x06] = ( ( param->MaxBlock>>8 ) &0xFF );        // Block/Chip   (4124)
    buffer[0x07] = ( param->MaxBlock&0x00FF );
    buffer[0x08] = ( ( param->PagePerBlock>>8 ) &0xFF );    // Page/Block
    buffer[0x09] = ( param->PagePerBlock&0xFF );

    buffer[0x0A] = param->PageSizeK;

#ifdef OLD_SPEC
    if ( param->Cell==0x08 ) {
        buffer[0x0B] = 3; //TLC
    }
    else if ( param->Cell==0x04 ) {
        buffer[0x0B] = 2; //MLC
    }
    else {
        buffer[0x0B] = 1; //SLC
    }
#else
    if ( param->Cell == 0x02 ) { //SLC
        buffer[0x0B]  = 0x01;
    }
    if ( param->Cell == 0x04 ) { //MLC
        buffer[0x0B]  = 0x02;
    }
    if ( param->Cell == 0x08 ) { //TLC
        buffer[0x0B]  = 0x04;
    }
#endif

    buffer[0x0C] = ( ( buffer[0]==0 ) && (  param->Design==0x24 ) ) ? 33 : 50;
    if ( GetPrivateProfileIntA ( "Flash","FlashTimming",0,ini ) ) {
        buffer[0x0C] = GetPrivateProfileIntA ( "Flash","FlashTimming",0,ini );  
    }        
    buffer[0x0D] = param->ECC;                  // ECC
    buffer[0x0E] = param->Design;               // 24nm
    buffer[0x0F] = param->SpareMode;

    //-- Conversion Option --
    //buffer[16] = ( mode==0 ) ? 1 : 0;         // Mode=0, use Convert, others NOT
    int bConversionOff = GetPrivateProfileIntA ( "Special","TurnOffConversion",0,ini );
    int pagecount = param->MaxPage;
#ifdef OLD_SPEC
    buffer[0x10]= ( bConversionOff==0 ) ? 1:0;
#else
    buffer[0x10]= ( bConversionOff!=0 ) ? 0:0x80;
    if ( pagecount > 128 ) {
        buffer[0x10] |= 0x04;
    }
    else if ( pagecount > 64 ) {
        buffer[0x10] |= 0x02;
    }
    else {
        buffer[0x10] |= 0x01;
    }
#endif

    buffer[0x11] = 1;                           // Ecc Enable
    param->IODriving        = GetPrivateProfileIntA ( "Flash","FlashDrivingmA",6,ini );
    buffer[0x12] = param->IODriving;
    buffer[0x13] = GetPrivateProfileIntA ( "Special","TurnONInvert",1,ini );
#ifdef OLD_SPEC
    buffer[0x14] = param->IO_Bus==0x16 ? 1 : 0;           // 1=x16
#else
    buffer[0x14] = ( param->IO_Bus==0x16 ) ?1:0;
    buffer[0x14] |= 0x02;   // edo mode open;
    if ( param->beD3 ) {
        buffer[0x14] |= 0x04;   // bad column open;
    }
#endif
    //-- Auto Pattern Mode --
    buffer[0x16] = 1;

    int EnableD2ManualBC        = GetPrivateProfileIntA ( "Special","EnableD2ManualBC",1,ini );
    int Enable2BCMode           = GetPrivateProfileIntA ( "Special","Enable2BCMode",0,ini );
    int BCMode = 1;
    if ( ( param->beD3 ) && ( Enable2BCMode!=0 ) && ( param->Design==0x24||param->Design==0x19 ) ) {
        BCMode=2;
    }
    if ( ( buffer[0]==0 ) && ( Enable2BCMode!=0 ) ) {
        BCMode=2;
    }

    //  if (!OrgMode)
    //  {
    //      //-- D2 Manual Bad Column --
    //      fbuf[23] = ((fTask->ISPType==FLASHTYPE_NORMAL)&&(m_EnableD2ManualBC)) ? 0xD2 : 0;           // Jorry 2012.07.23
    //      //-- 2 BC / Page --
    //      fbuf[24] = (m_BCMode>=2) ? (((m_PageSizeK/2)<<4) | (m_PageSizeK-(m_PageSizeK/2))) : 0;      // Jorry 2012.07.23
    //  }
    //
    //  //-- for special PBA --                                                                         // Jorry 2012.12.24
    //buffer[25] = param->bToggle=='P' ? 1:0;       // Chip/CE


	//l85 , l95 廠內delay..
	//如果用fill full 會把delay拿掉
	if((param->bMaker == 0x2c || param->bMaker == 0x89) && param->Design <=20 && !param->beD3)
	{
		buffer[0x22] =100;
	}
	buffer[0x23] |= 0x01;
    unsigned char HOFFSET = 0x40;
    buffer[0x40] = param->bMaker;
    buffer[0x41] = param->id[0];
    buffer[0x42] = param->id[1];
    buffer[0x43] = param->id[2];
    buffer[0x44] = param->id[3];
    buffer[0x45] = param->id[4];
    buffer[0x46] = param->id[5];
    //  buffer[0x47] = param->id[6];

	//20150716 Annie : Added manual RW Timing for SZ
	int _iniReadTimingHi	= GetPrivateProfileIntA ( "Sorting","Timing105",0,ini );	
	int _iniReadTimingLow	= GetPrivateProfileIntA ( "Sorting","Timing106",0,ini );	
	int _iniWriteTimingHi	= GetPrivateProfileIntA ( "Sorting","Timing10C",0,ini );
	int _iniWriteTimingLow	= GetPrivateProfileIntA ( "Sorting","Timing10D",0,ini );

	if(_iniReadTimingLow||_iniWriteTimingLow)
	{
		buffer[0x48]=(((_iniReadTimingHi&0x0F)<<4) | (_iniReadTimingLow&0x0F));
		buffer[0x49]=(((_iniWriteTimingHi&0x0F)<<4) | (_iniWriteTimingLow&0x0F));
	}
	else
	{

		buffer[0x48] = 0x11;        //read clock;
		buffer[0x49] = 0x11;        //write clock;
	}

    buffer[0x4A] = 0x31;        // set data bus driving - High nibble = (3) 12ma , low (1)=3ma
    buffer[0x4B] = 0x31;        // set driving;
    buffer[0x4C] = 0x00;        // "Driving setting (LowBus)"
    buffer[0x4D] = 0x82;     // ed3 conversion ?? enable 0x80=convertion and 0x02=128 page


    //memset(ALU_LOW,0x00,0x10);//必須清為0,若有殘留值會影響結果
    //if(param->Fla & 0x01)

    int alu_offset = 0x60;
    if ( pagecount > 256 ) {
        buffer[0+alu_offset] = 0x08;
        buffer[1+alu_offset] = 0x21;
        buffer[2+alu_offset] = 0x81;//from bit 16 to bit 23 with 8 bits( max )
        buffer[3+alu_offset] = 0x41;
        buffer[4+alu_offset] = 0x98;//from bit 24 to bit 31 with 8 bits( max )
        buffer[5+alu_offset] = 0x61;
        buffer[6+alu_offset] = 0x18;//from bit 24 to bit 31 with 8 bits( max )
        buffer[7+alu_offset] = 0x82;
    }
    else if ( pagecount > 128 ) {

        buffer[0+alu_offset] = 0x08;
        buffer[2+alu_offset] = 0x88;//from bit 16 to bit 23 with 8 bits( max )
        buffer[4+alu_offset] = 0x08;//from bit 24 to bit 31 with 8 bits( max )
        buffer[5+alu_offset] = 0x82;
        buffer[1+alu_offset] = 0x21;
        buffer[3+alu_offset] = 0x61;
    }
    else if ( pagecount > 64 ) {
        buffer[0+alu_offset] = 0x07;
        buffer[2+alu_offset] = 0x78;//from bit 16 to bit 23 with 8 bits( max )
        buffer[4+alu_offset] = 0xF8;//from bit 24 to bit 31 with 8 bits( max )
        buffer[5+alu_offset] = 0x81;
        buffer[1+alu_offset] = 0x21;
        buffer[3+alu_offset] = 0x61;
        if ( param->beD3 ) {
            buffer[0+alu_offset] = 0x08;// from bit 8 to bit 16 with 8 bits
            buffer[1+alu_offset] = 0x21;
            buffer[2+alu_offset] = 0x88;//from bit 16 to bit 23 with 8 bits( max )
            buffer[3+alu_offset] = 0x61;
            buffer[4+alu_offset] = 0x08;//from bit 24 to bit 31 with 8 bits( max )
            buffer[5+alu_offset] = 0x82;
        }

    }
    else {
        buffer[0+alu_offset] = 0x06;
        buffer[2+alu_offset] = 0x68;//from bit 16 to bit 23 with 8 bits( max )
        buffer[4+alu_offset] = 0xE8;//from bit 24 to bit 31 with 8 bits( max )
        buffer[5+alu_offset] = 0x81;
        buffer[1+alu_offset] = 0x21;
        buffer[3+alu_offset] = 0x61;

        if ( param->beD3 ) {
            buffer[0+alu_offset] = 0x07;// from bit 8 to bit 16 with 7 bits
            buffer[1+alu_offset] = 0x21;
            buffer[2+alu_offset] = 0x88;//from bit 16 to bit 23 with 8 bits( max )
            buffer[3+alu_offset] = 0x61;
            buffer[4+alu_offset] = 0x08;//from bit 24 to bit 31 with 8 bits( max )
            buffer[5+alu_offset] = 0x82;
        }
    }

    for ( int i=0; i<9; i++ ) {
        buffer[0x50+i] = param->inverseCE[i];
    }


	//20150420 Annie : FW SetParameter 移到 AP 設定 0x90~0xB3
	SetFlashCmd_Buffer(param,buffer);

    return true;
}

#endif
bool MyFlashInfo::ParamToBufferUPTool(_FH_Paramater * fh,unsigned char * ap_parameter)
{
    return ParamToBuffer(fh, ap_parameter, NULL, false/*bIsExPBA*/,false/*E2NAM*/
										 ,0x02/*new spec*/ ,false/*flashMixmode*/, NULL/*ap_parameter1*/, false/*setOnfi*/)?true:false; 
}
bool isSixAddressFlash(unsigned char *IdBuf)
{
	const int LIST_CNT = 1;
	unsigned char FLASH_LIST[LIST_CNT][6] = 
	{ {0x2C, 0xC3, 0x78, 0x32, 0xEA, 0x10}, 
	};

	for (int i=0; i<LIST_CNT; i++)
	{
		if (0 == memcmp(IdBuf, FLASH_LIST[i], 6))
			return true;
	}
	return false;
}
bool MyFlashInfo::IsAddressCycleRetryReadFlash(FH_Paramater *fh)
{
	if (fh->bIsMicronB47R)
	{
		return true;
	}

	return false;
}

bool MyFlashInfo::IsSixAdressFlash(FH_Paramater *fh)
{
	if (fh->bIsYMTC3D || fh->bIsMicronB36R || fh->bIsMicronB37R || fh->bIsMicronB47R || fh->bIsMicronQ1428A) {
		return true;
	}

	if (fh->bIsMIcronN28 && (2<= fh->DieNumber))
	{
		return true;
	}

	if ( (fh->bIsMicronB17 || fh->bIsMicronB27 ) && (4 <= fh->DieNumber)  )
	{
		return true;
	}

	if (fh->bIsHynix3DV4  && 8<= fh->DieNumber )
	{
		return true;
	}

	return false;
}

unsigned char MyFlashInfo::ParamToBuffer(FH_Paramater* param, unsigned char* buffer, const char* ini,unsigned char bIsExPBA,unsigned char bE2NAMD
										 ,unsigned char ParamSpec ,bool FlashMixMode,FH_Paramater* param1,int SetOnfi)
{
	memset(buffer,0,1024);

//#ifdef OLD_SPEC
	if(ParamSpec == 0x01) //UPSpec
	{
		if ( param->beD3 ) {
			buffer[0]=2;
		}
		else if ( param->bToggle=='T' ) {
			buffer[0]=1;
		}
		else if ( param->bToggle=='N' ) {
			buffer[0]=0;
		}

		if ( param->bToggle=='T' ) {
			buffer[0] |= 0x80;    // bit7=Toggle  // Jorry 2012.11.16
		}
		else if ( param->bToggle=='P' ) {
			buffer[0] |= 0x40;
	    
		}
	}
	else
	{
		if ( param->beD3 ) {
			buffer[0]  = buffer[0]  | 0x08;
		}
		else if ( param->Cell == 0x08 ) { //TLC
			buffer[0]  = buffer[0]  | 0x04;
		}
		else {
			buffer[0]  = buffer[0]  | 0x01;
		}

		if ( param->bToggle==0x54 ) {
			buffer[0]  = buffer[0]  | 0x10;
		}

	//	if ((param->Type & 0x4) == 0x4) { //onfi
	//		buffer[0]  = buffer[0]  | 0x20;
	//	}

		if(bIsExPBA)
			buffer[0]  = buffer[0]  | 0x40;
		if(bE2NAMD)
			buffer[0]  = buffer[0]  | 0x80;

	}
//#endif
	//if (param->bToggle=='T')			buffer[0] |= 0x80;		// bit7=Toggle	// Jorry 2012.11.16
	//else if ( param->bToggle=='P')				buffer[0] |= 0x40;		// bit6=PBA		// Jorry 2012.12.26


    buffer[0x01] = param->bMaker;				// 0x98=Toshiba, 0xEC=Samsung
    buffer[0x02] = param->FlhNumber;				// Total Chips
    buffer[0x03] = param->DieNumber;				// Die/Chip
    buffer[0x04] = param->Plane;					// Plane/Chip
	if(FlashMixMode){
		buffer[0xc1] = param1->bMaker;				// 0x98=Toshiba, 0xEC=Samsung
		buffer[0xc2] = param1->FlhNumber;				// Total Chips
		buffer[0xc3] = param1->DieNumber;				// Die/Chip
		buffer[0xc4] = param1->Plane;					// Plane/Chip
	}
	
	if(param->DieNumber == 0)
		return 1;
    int dieblk = param->MaxBlock / param->DieNumber;
    buffer[0x05] = ( dieblk/1024 ) + ( ( dieblk&0x3FF ) ? 1 : 0 );		// Zone/Die
    buffer[0x06] = ( ( param->MaxBlock>>8 ) &0xFF );		// Block/Chip	(4124)
    buffer[0x07] = ( param->MaxBlock&0x00FF );
    buffer[0x08] = ( ( param->PagePerBlock>>8 ) &0xFF );    // Page/Block
    buffer[0x09] = ( param->PagePerBlock&0xFF );
    buffer[0x0A] = param->PageSizeK;

	if(FlashMixMode){
		int dieblk1 = param1->MaxBlock / param1->DieNumber;
		buffer[0xc5] = ( dieblk1/1024 ) + ( ( dieblk1&0x3FF ) ? 1 : 0 );		// Zone/Die
		buffer[0xc6] = ( ( param1->MaxBlock>>8 ) &0xFF );		// Block/Chip	(4124)
		buffer[0xc7] = ( param1->MaxBlock&0x00FF );
		buffer[0xc8] = ( ( param1->PagePerBlock>>8 ) &0xFF );    // Page/Block
		buffer[0xc9] = ( param1->PagePerBlock&0xFF );
		buffer[0xcA] = param1->PageSizeK;
	}
	if(ParamSpec == 0x01) //UPSpec
	{
		if ( param->Cell==0x08 ) {
			buffer[0x0B] = 3; //TLC
			if(FlashMixMode) buffer[0xcB] = 3;
		}
		else if ( param->Cell==0x04 ) {
			buffer[0x0B] = 2; //MLC
			if(FlashMixMode) buffer[0xcB] = 2;
		}
		else {
			buffer[0x0B] = 1; //SLC
			if(FlashMixMode) buffer[0xcB] = 1;
		}
	}
	else
	{
		if ( param->Cell == 0x02 ) { //SLC
			buffer[0x0B]  = 0x01;
			if(FlashMixMode) buffer[0xcB] = 1;
		}
		if ( param->Cell == 0x04 ) { //MLC
			buffer[0x0B]  = 0x02;
			if(FlashMixMode) buffer[0xcB] = 2;
		}
		if ( param->Cell == 0x08 ) { //TLC
			buffer[0x0B]  = 0x04;
			if(FlashMixMode) buffer[0xcB] = 4;
		}
		if ( param->Cell == 0x10 ) { //QLC
			buffer[0x0B]  = 0x08;
			if(FlashMixMode) buffer[0xcB] = 8;
		}
	}

	if (IsSixAdressFlash(param))
	{
		buffer[0x3B] |= 1<<3; //6 address flash
	}
	
	if (IsAddressCycleRetryReadFlash(param))
	{
		buffer[0x3B] |= 1<<4; //support Address Cycle Retry Read (ACRR)
	}
	
    buffer[0x0C] = ( ( buffer[0]==0 ) && (  param->Design==0x24 ) ) ? 33 : 50;	
    buffer[0x0D] = param->ECC;					// ECC
	buffer[0x0E] = param->Design;				// 24nm
    buffer[0x0F] = param->SpareMode;
	if(FlashMixMode){
		buffer[0xCC] = ( ( buffer[0]==0 ) && (  param1->Design==0x24 ) ) ? 33 : 50;	
		buffer[0xCD] = param1->ECC;					// ECC
		buffer[0xCE] = param1->Design;				// 24nm
		buffer[0xCF] = param1->SpareMode;
		param1->MaxPage = param1->PagePerBlock;
	}
	param->MaxPage = param->PagePerBlock;
	int pagecount=0;
	int	bConversionOff = 0;
    //-- Conversion Option --		
    //buffer[16] = ( mode==0 ) ? 1 : 0;			// Mode=0, use Convert, others NOT
    bConversionOff = GetPrivateProfileIntA ( "Special","TurnOffConversion",0,ini );
	if(ParamSpec == 0x01){ //UPSpec
		buffer[0x10]= ( bConversionOff==0 ) ? 1:0;
	}
	else{
		buffer[0x10]=(bConversionOff!=0) ? 0:0x80;
		pagecount = param->MaxPage;// (param->Page_H << 8 | param->Page_L);
		if ( pagecount > 128 ) {
			buffer[0x10] |= 0x04;
		}
		else if ( pagecount > 64 ) {
			buffer[0x10] |= 0x02;
		}
		else{
			buffer[0x10] |= 0x01;
		}
	}


    buffer[0x11] = 1;							// Ecc Enable
    param->IODriving= GetPrivateProfileIntA ( "Flash","FlashDrivingmA",6,ini );
    buffer[0x12] = param->IODriving;
    buffer[0x13] = GetPrivateProfileIntA ( "Special","TurnONInvert",1,ini );
	if(ParamSpec == 0x01){ //UPSpec
		buffer[0x14] = param->IO_Bus==0x10 ? 1 : 0;           // 1=x16
	}
	else
	{
		buffer[0x14] = (param->IO_Bus==0x10) ?1:0;
		buffer[0x14] |= 0x02;	// edo mode open;
		if(param->bSetOBCBadColumn)
			buffer[0x14] |= 0x04;	// bad column open;
	}

	if (param->bToggle == 0x54) 
		buffer[0x14] &= ~0x02; // toggle close edo

	buffer[0x15] = param->bFunctionMode;
	buffer[0x2f] = 0x01; //copyback conversion
    //-- Auto Pattern Mode --																	
	buffer[0x16] = 0x01; //fw buffer mode
	buffer[0x17] = param->PageSize_H;
	buffer[0x18] = param->PageSize_L;
	buffer[0x19] = param->PageSize_Ext;
	buffer[0x1A] = (param->PageSize+1023)/1024;
	
    int EnableD2ManualBC		= GetPrivateProfileIntA ( "Special","EnableD2ManualBC",1,ini );	
    int Enable2BCMode			= GetPrivateProfileIntA ( "Special","Enable2BCMode",0,ini );
    int BCMode = 1;
    if ( (param->beD3 ) && ( Enable2BCMode!=0 ) && ( param->Design==0x24||param->Design==0x19 ) ) {
        BCMode=2;
    }
    if ( ( buffer[0]==0 ) && ( Enable2BCMode!=0 ) ) {
        BCMode=2;
    }

    //  if (!OrgMode)
    //  {
    //      //-- D2 Manual Bad Column --
    //      fbuf[23] = ((fTask->ISPType==FLASHTYPE_NORMAL)&&(m_EnableD2ManualBC)) ? 0xD2 : 0;           // Jorry 2012.07.23
    //      //-- 2 BC / Page --
    //      fbuf[24] = (m_BCMode>=2) ? (((m_PageSizeK/2)<<4) | (m_PageSizeK-(m_PageSizeK/2))) : 0;      // Jorry 2012.07.23
    //  }
    //
    //  //-- for special PBA --                                                                         // Jorry 2012.12.24
    //buffer[25] = param->bToggle=='P' ? 1:0;       // Chip/CE
	
	//Parameter 34 RetryDelay
	//Intel or Micro flash .. set 
	//if(param->bMaker == 0x2c || param->bMaker == 0x89)
	//	buffer[0x22] = 10;//10 msec

	//l85 , l95 廠內delay..
	//如果用fill full 會把delay拿掉
	buffer[0x20] = 1;
	buffer[0x21] = 1; //d1 d2 program count
	buffer[0x25] = 1; //d3 copyback program count
	if(param->bIs8T2E){
		buffer[0x20] = 3; //erase count
	}
	//目前全部走fill full mode 不需要在delay
	/*if((param->bMaker == 0x2c || param->bMaker == 0x89) && param->Design <=0x20 && !param->beD3)
	{
		buffer[0x22] = 100;
	}*/
	int retrydelay = GetPrivateProfileIntA ( "Sorting","RetryDelay",0,ini );	
	if ( retrydelay>=0 ) {
		buffer[0x22] = retrydelay;
	}

	buffer[0x23] |= 0x01; //d1 erase write

	
	int EnterpriseRule = GetPrivateProfileIntA ("Sorting", "EnterpriseRule",0,ini);
	if(param->bIsMicron3DTLC){
		if(param->bIsMIcronB0kB)
			buffer[0x28] |= 0x01; //b0kB
		else{
			if(param->bIsMicronB17)
				buffer[0x28] |= 0x04; //b17
			else if(param->bIsMicronB16)
				buffer[0x28] |= 0x02; //b16
			else if(param->bIsMIcronB05)
				buffer[0x28] |= 0x08; //b05
			else if(param->bIsMIcronN18)
				buffer[0x28] |=0x10;
			else if(param->bIsMicronB27){
				if(param->id[3]==0xA2)			// B27A
					buffer[0x28] |=0x20;
				else if(param->id[3]==0xE6)	// B27BS
					buffer[0x28] |=0x40;
			}
			else if(param->bIsMIcronN28){
				buffer[0xDD] |= 0x01;
			}
			else if(param->bIsMicronB37R ){
				buffer[0xDD] |= 0x02;
			}
			else if(param->bIsMicronB47R ){
				buffer[0xDD] |= 0x20;
			}
			else if(param->bIsMicronB36R) {
				buffer[0xDD] |= 0x10;
			}
			else if(param->bIsMicronQ1428A){
				buffer[0xDD] |= 0x04;
			}
			else{
				assert(0);
			}
		}

		if(EnterpriseRule){
			// 底層在 enterprise write/read 會不一樣
			buffer[0x28] |= 0x80;
		}

	}
	
	if(param->bIsBics3pMLC){
		buffer[0x28] |= 0x80;
	}

	unsigned char HOFFSET = 0x40;
	buffer[0x40] = param->bMaker;
	buffer[0x41] = param->id[0];
	buffer[0x42] = param->id[1];
	buffer[0x43] = param->id[2];
	buffer[0x44] = param->id[3];
	buffer[0x45] = param->id[4];
	buffer[0x46] = param->id[5];
	//	buffer[0x47] = param->id[6];
	if(bIsExPBA)
	{
		buffer[0x48] = param->Read_PBATiming;		//read clock;
		buffer[0x49] = param->Write_PBATiming;		//write clock;
	}
	else
	{
		//20150716 Annie : Added manual RW Timing for SZ
		//20151102 Annie : marked , moved to DoF1
		//int _iniReadTimingHi	= GetPrivateProfileIntA ( "Sorting","Timing105",0xff,ini );	
		//int _iniReadTimingLow	= GetPrivateProfileIntA ( "Sorting","Timing106",0xff,ini );	
		//int _iniWriteTimingHi	= GetPrivateProfileIntA ( "Sorting","Timing10C",0xff,ini );
		//int _iniWriteTimingLow	= GetPrivateProfileIntA ( "Sorting","Timing10D",0xff,ini );


		//if((_iniReadTimingLow!=0xff)&&(_iniWriteTimingLow!=0xff))
		//{
		//	buffer[0x48]=(((_iniReadTimingHi&0x0F)<<4) | (_iniReadTimingLow&0x0F));
		//	buffer[0x49]=(((_iniWriteTimingHi&0x0F)<<4) | (_iniWriteTimingLow&0x0F));
		//}
		//else
		{
			buffer[0x48] = param->Read_Timming;		//read clock;
			buffer[0x49] = param->Write_timing;		//write clock;
		}
	}

	//if ( param->bToggle==0x54 ) {
	//	buffer[0x48] = 0x00;		//read clock;
	//	buffer[0x49] = 0x00;		//write clock;
	//}

	//if ((param->Type & 0x04) == 0x04) { //onfi
	//	buffer[0x48] = 0x00;		//read clock;
	//	buffer[0x49] = 0x00;		//write clock;
	//}


	//buffer[0x4A] = GetPrivateProfileIntA ( "Flash","set_data_bus_driving",0x11,ini );
	buffer[0x4A] = GetPrivateProfileIntA ( "Flash","set_data_bus_driving",0x11,ini );
	//buffer[0x4A] = 0x31;		// set data bus driving - High nibble = (3) 12ma , low (1)=3ma
	buffer[0x4B] = GetPrivateProfileIntA ( "Flash","set_driving",0x11,ini );
	//buffer[0x4B] = 0x31;		// set driving;
	buffer[0x4C] = GetPrivateProfileIntA ( "Flash","set_driving_lowbus",0x00,ini );
	//buffer[0x4D] = 0x82;     
	//20180417: Hugo said force soureclock 0x80 avoid overclocking
	//20180502: Hugo與底層定義不同，0x83才是最慢
	buffer[0x4D] = 0x83;

	buffer[0x38] = 0;
#if 0
	int data = param->SLC_RDY_H <<8 | param->SLC_RDY_L;
	int count = (int)(data / 1.796);
	buffer[0x38] = 0;
	if(count > 0) buffer[0x38] = 1;
	buffer[0x30] = (count+1) >> 8;
	buffer[0x31] = (count+1) & 0xff;
	data = param->First_RDY_H <<8 | param->First_RDY_L;
	count = (int)(data / 1.796);
	buffer[0x32] = (count+1) >> 8;
	buffer[0x33] = (count+1) & 0xff;
	data = param->Foggy_RDY_H <<8 | param->Foggy_RDY_L;
	count = (int)(data / 1.796);
	buffer[0x34] = (count+1) >> 8;
	buffer[0x35] = (count+1) & 0xff;

	data = param->Fine_RDY_H <<8 | param->Fine_RDY_L;
	count = (int)(data / 1.796);
	buffer[0x36] = (count+1) >> 8;
	buffer[0x37] = (count+1) & 0xff;
#endif
	//memset(ALU_LOW,0x00,0x10);//必須清為0,若有殘留值會影響結果
	//if(param->Fla & 0x01)
	int alu_offset = 0x60;
	int aluPageCount = pagecount;
	if(param->alu_PageCount !=0)
		aluPageCount = param->alu_PageCount;
	if ( aluPageCount > 512 ) {
		buffer[0+alu_offset] = 0x08;
		buffer[1+alu_offset] = 0x21;
		buffer[2+alu_offset] = 0x82;//from bit 16 to bit 23 with 8 bits( max )
		buffer[3+alu_offset] = 0x41;
		buffer[4+alu_offset] = 0xA8;//from bit 24 to bit 31 with 8 bits( max )
		buffer[5+alu_offset] = 0x61;
		buffer[6+alu_offset] = 0x28;//from bit 24 to bit 31 with 8 bits( max )
		buffer[7+alu_offset] = 0x82;
	}
	else if ( aluPageCount > 256 ) {
		buffer[0+alu_offset] = 0x08;
		buffer[1+alu_offset] = 0x21;
		buffer[2+alu_offset] = 0x81;//from bit 16 to bit 23 with 8 bits( max )
		buffer[3+alu_offset] = 0x41;
		buffer[4+alu_offset] = 0x98;//from bit 24 to bit 31 with 8 bits( max )
		buffer[5+alu_offset] = 0x61;
		buffer[6+alu_offset] = 0x18;//from bit 24 to bit 31 with 8 bits( max )
		buffer[7+alu_offset] = 0x82;
	}
	else if ( aluPageCount > 128 ) {

		buffer[0+alu_offset] = 0x08;
		buffer[1+alu_offset] = 0x21;
		buffer[2+alu_offset] = 0x88;//from bit 16 to bit 23 with 8 bits( max )
		buffer[3+alu_offset] = 0x61;
		buffer[4+alu_offset] = 0x08;//from bit 24 to bit 31 with 8 bits( max )
		buffer[5+alu_offset] = 0x82;
		
		
	}
	else if ( aluPageCount > 64 ) {
		buffer[0+alu_offset] = 0x07;
		buffer[2+alu_offset] = 0x78;//from bit 16 to bit 23 with 8 bits( max )
		buffer[4+alu_offset] = 0xF8;//from bit 24 to bit 31 with 8 bits( max )
		buffer[5+alu_offset] = 0x81;
		buffer[1+alu_offset] = 0x21;
		buffer[3+alu_offset] = 0x61;
		if(param->beD3)
		{
			buffer[0+alu_offset] = 0x08;// from bit 8 to bit 16 with 8 bits
			buffer[1+alu_offset] = 0x21;
			buffer[2+alu_offset] = 0x88;//from bit 16 to bit 23 with 8 bits( max )
			buffer[3+alu_offset] = 0x61;
			buffer[4+alu_offset] = 0x08;//from bit 24 to bit 31 with 8 bits( max )
			buffer[5+alu_offset] = 0x82;
		}
		
	}
	else{
		buffer[0+alu_offset] = 0x06;
		buffer[2+alu_offset] = 0x68;//from bit 16 to bit 23 with 8 bits( max )
		buffer[4+alu_offset] = 0xE8;//from bit 24 to bit 31 with 8 bits( max )
		buffer[5+alu_offset] = 0x81;
		buffer[1+alu_offset] = 0x21;
		buffer[3+alu_offset] = 0x61;

		if(param->beD3)
		{
			//buffer[0+alu_offset] = 0x07;// D3from bit 8 to bit 16 with 7 bits
			buffer[0+alu_offset] = 0x08;
			buffer[1+alu_offset] = 0x21;
			buffer[2+alu_offset] = 0x88;//from bit 16 to bit 23 with 8 bits( max )
			buffer[3+alu_offset] = 0x61;
			buffer[4+alu_offset] = 0x08;//from bit 24 to bit 31 with 8 bits( max )
			buffer[5+alu_offset] = 0x82;
		}
	}
	if( (param->bMaker==0x2c || param->bMaker==0x89) && param->beD3 && param->Design!=0x16) /*&& param->id[0]==0xb4 && param->id[1]==0x78 && param->id[2]==0x32 && param->id[3]==0xaa &&param->id[4]==0x04 */
	{
		// B0K: page = 1536
		int pagePerWl = 3;
		if(param->Cell==0x10)pagePerWl = 4;
		if((aluPageCount * pagePerWl) > 4096){
			buffer[0+alu_offset] = 0x08;
			buffer[1+alu_offset] = 0x21;
			buffer[2+alu_offset] = 0x85;
			buffer[3+alu_offset] = 0x41;
			buffer[4+alu_offset] = 0xD8;
			buffer[5+alu_offset] = 0x61;
			buffer[6+alu_offset] = 0x58;
			buffer[7+alu_offset] = 0x82;
		}
		else if((aluPageCount * pagePerWl) > 2048){
			buffer[0+alu_offset] = 0x08;
			buffer[1+alu_offset] = 0x21;
			buffer[2+alu_offset] = 0x84;
			buffer[3+alu_offset] = 0x41;
			buffer[4+alu_offset] = 0xC8;
			buffer[5+alu_offset] = 0x61;
			buffer[6+alu_offset] = 0x48;
			buffer[7+alu_offset] = 0x82;
		}
		else if((aluPageCount * pagePerWl) > 1024){
			buffer[0+alu_offset] = 0x08;
			buffer[1+alu_offset] = 0x21;
			buffer[2+alu_offset] = 0x83;
			buffer[3+alu_offset] = 0x41;
			buffer[4+alu_offset] = 0xB8;
			buffer[5+alu_offset] = 0x61;
			buffer[6+alu_offset] = 0x38;
			buffer[7+alu_offset] = 0x82;
		}
		else if((aluPageCount * pagePerWl) > 512){
			buffer[0+alu_offset] = 0x08;
			buffer[1+alu_offset] = 0x21;
			buffer[2+alu_offset] = 0x82;//from bit 16 to bit 23 with 8 bits( max )
			buffer[3+alu_offset] = 0x41;
			buffer[4+alu_offset] = 0xA8;//from bit 24 to bit 31 with 8 bits( max )
			buffer[5+alu_offset] = 0x61;
			buffer[6+alu_offset] = 0x28;//from bit 24 to bit 31 with 8 bits( max )
			buffer[7+alu_offset] = 0x82;
		}
		else{
			assert(0);
		}
	}
	if((param->bMaker==0x9b) && (param->beD3)){
		buffer[0+alu_offset] = 0x08;
		buffer[1+alu_offset] = 0x21;
		buffer[2+alu_offset] = 0x83;
		buffer[3+alu_offset] = 0x41;
		buffer[4+alu_offset] = 0xB8;
		buffer[5+alu_offset] = 0x61;
		buffer[6+alu_offset] = 0x38;
		buffer[7+alu_offset] = 0x82;
	}

	if( param->bMaker==0x9b && param->id[0]==0xC4 && param->id[1]==0x28 && param->id[2]==0x49 && param->id[3]==0x20 ){
		buffer[0+alu_offset] = 0x08;
		buffer[1+alu_offset] = 0x21;
		buffer[2+alu_offset] = 0x84;
		buffer[3+alu_offset] = 0x41;
		buffer[4+alu_offset] = 0xC8;
		buffer[5+alu_offset] = 0x61;
		buffer[6+alu_offset] = 0x48;
		buffer[7+alu_offset] = 0x82;
	}

	if(FlashMixMode){
		pagecount = param1->MaxPage;// (param->Page_H << 8 | param->Page_L);
		if ( pagecount > 512 ) {
			buffer[8+alu_offset] = 0x08;
			buffer[9+alu_offset] = 0x21;
			buffer[10+alu_offset] = 0x82;//from bit 16 to bit 23 with 8 bits( max )
			buffer[11+alu_offset] = 0x41;
			buffer[12+alu_offset] = 0xA8;//from bit 24 to bit 31 with 8 bits( max )
			buffer[13+alu_offset] = 0x61;
			buffer[14+alu_offset] = 0x28;//from bit 24 to bit 31 with 8 bits( max )
			buffer[15+alu_offset] = 0x82;
		}
		else if ( pagecount > 256 ) {
			buffer[8+alu_offset] = 0x08;
			buffer[9+alu_offset] = 0x21;
			buffer[10+alu_offset] = 0x81;//from bit 16 to bit 23 with 8 bits( max )
			buffer[11+alu_offset] = 0x41;
			buffer[12+alu_offset] = 0x98;//from bit 24 to bit 31 with 8 bits( max )
			buffer[13+alu_offset] = 0x61;
			buffer[14+alu_offset] = 0x18;//from bit 24 to bit 31 with 8 bits( max )
			buffer[15+alu_offset] = 0x82;
		}
		else if ( pagecount > 128 ) {

			buffer[8+alu_offset] = 0x08;
			buffer[9+alu_offset] = 0x21;
			buffer[10+alu_offset] = 0x88;//from bit 16 to bit 23 with 8 bits( max )
			buffer[11+alu_offset] = 0x61;
			buffer[12+alu_offset] = 0x08;//from bit 24 to bit 31 with 8 bits( max )
			buffer[13+alu_offset] = 0x82;


		}
		else if ( pagecount > 64 ) {
			buffer[8+alu_offset] = 0x07;
			buffer[10+alu_offset] = 0x78;//from bit 16 to bit 23 with 8 bits( max )
			buffer[12+alu_offset] = 0xF8;//from bit 24 to bit 31 with 8 bits( max )
			buffer[13+alu_offset] = 0x81;
			buffer[9+alu_offset] = 0x21;
			buffer[11+alu_offset] = 0x61;
			if(param->beD3)
			{
				buffer[8+alu_offset]	= 0x08;// from bit 8 to bit 16 with 8 bits
				buffer[9+alu_offset]	= 0x21;
				buffer[10+alu_offset]	= 0x88;//from bit 16 to bit 23 with 8 bits( max )
				buffer[11+alu_offset]	 = 0x61;
				buffer[12+alu_offset]	= 0x08;//from bit 24 to bit 31 with 8 bits( max )
				buffer[13+alu_offset]	= 0x82;
			}

		}
		else{
			buffer[8+alu_offset]	= 0x06;
			buffer[10+alu_offset]	= 0x68;//from bit 16 to bit 23 with 8 bits( max )
			buffer[12+alu_offset]	= 0xE8;//from bit 24 to bit 31 with 8 bits( max )
			buffer[13+alu_offset]	= 0x81;
			buffer[9+alu_offset]	= 0x21;
			buffer[11+alu_offset]	= 0x61;

			if(param->beD3)
			{
				//buffer[0+alu_offset] = 0x07;// D3from bit 8 to bit 16 with 7 bits
				buffer[8+alu_offset]	= 0x08;
				buffer[9+alu_offset]	= 0x21;
				buffer[10+alu_offset]	= 0x88;//from bit 16 to bit 23 with 8 bits( max )
				buffer[11+alu_offset]	= 0x61;
				buffer[12+alu_offset]	= 0x08;//from bit 24 to bit 31 with 8 bits( max )
				buffer[13+alu_offset]	= 0x82;
			}
		}

	}

	//if ( !param->beD3 ) {
	//	buffer[0x60] = 0x07;// from bit 8 to bit 16 with 8 bits
	//}
	//else {
	//	buffer[0x60] = 0x08;// from bit 8 to bit 16 with 8 bits
	//}
	////ALU_LOW 0  //from bit 8 to bit 16 with 7 bits
	//buffer[0x61]=  0x21;//ALU_LOW 1
	//buffer[0x62]=  0x88;//ALU_LOW 2  //from bit 16 to bit 23 with 8 bits( max )
	//buffer[0x63] = 0x61;//ALU_LOW 3  //0x61
	//buffer[0x64] = 0x08;//ALU_LOW 4  //from bit 24 to bit 31 with 8 bits( max )
	//buffer[0x65] = 0x82;//ALU_LOW 5
	
    for ( int i=0; i<9; i++ ) {
		buffer[0x50+i] = param->inverseCE[i];
	}


	if( param->beD3 ) {
		assert(param->PageAMap==0);
	}

	int PA = GetPrivateProfileIntA ( "Flash","SetPageAMap",0x0,ini );
	if(PA != 0) param->PageAMap = PA; //手動強迫修改fast page mapping
	if(!param->beD3 && param->PageAMap!=0){
		for(unsigned int i=0;i<param->MaxPage;i++)
		{
			int fpage = GetFastPage(i,param->PageAMap);
			//fast_page_bit
			int ByteAddr = fpage / 8;//第幾個byte ;//一個bit代表一個page
			int offset = (fpage % 8);//第幾個bit
			param->fast_page_bit[ByteAddr] = param->fast_page_bit[ByteAddr] | (1 << offset);
			param->fast_page_list[i] = fpage;// GetFastPage(i,param->PageAMap);
			int breakcount =param->MaxPage;
			if((param->PageAMap==PAGE_21) || (param->PageAMap==PAGE_22)){
				breakcount = param->MaxPage-3;
			}
			else if(param->PageAMap==PAGE_45 || param->PageAMap==PAGE_4A){
				breakcount = param->MaxPage-2;
			}
			else if(param->PageAMap ==PAGE_61 && param->MaxPage>512){
				breakcount = param->MaxPage-3;
			}
			else if(param->PageAMap==PAGE_49|| param->PageAMap==PAGE_42){
				breakcount = param->MaxPage-1;
			}
			else if(param->PageAMap!=PAGE_46 && param->PageAMap!=PAGE_48 ){
				breakcount = param->MaxPage-5;
			}
			
			
			//if(param->MaxPage)
			if(param->fast_page_list[i] >= param->MaxPage || param->fast_page_list[i] >= (unsigned int)breakcount)
				break;
		}

		for(unsigned int i=0;i<param->MaxPage;i++)
		{
			if(((param->fast_page_bit[i/8] >> i%8)&0x01)==0x00) continue;
			int fpage = GetPairPage(i,param->PageAMap,param);
			//int fpage = GetFastPage(i,param->PageAMap);
			//fast_page_bit
			param->PairPageAddress[i] = fpage;
		}
		if(FlashMixMode){
			for(unsigned int i=0;i<param1->MaxPage;i++)
			{
				int fpage = GetFastPage(i,param1->PageAMap);
				//fast_page_bit
				int ByteAddr = fpage / 8;//第幾個byte ;//一個bit代表一個page
				int offset = (fpage % 8);//第幾個bit
				param1->fast_page_bit[ByteAddr] = param1->fast_page_bit[ByteAddr] | (1 << offset);
				param1->fast_page_list[i] = fpage;// GetFastPage(i,param->PageAMap);
				int breakcount =param1->MaxPage;
				if(param->PageAMap==PAGE_21){
					breakcount = param1->MaxPage-3;
				}
				else if(param1->PageAMap==PAGE_45){
					breakcount = param1->MaxPage-2;
				}
				else{
					breakcount = param1->MaxPage-5;
				}
				//if(param->MaxPage)
				if(param1->fast_page_list[i] >= param1->MaxPage || param1->fast_page_list[i] >= (unsigned int)breakcount)
					break;
			}

			for(unsigned int i=0;i<param1->MaxPage;i++)
			{
				if(((param1->fast_page_bit[i/8] >> i%8)&0x01)==0x00) continue;
				int fpage = GetPairPage(i,param1->PageAMap,param1);
				//fast_page_bit
				param1->PairPageAddress[i] = fpage;
			}
		}

	}
	else
	{
		memset(param->fast_page_bit,0xff,256);
		//for(unsigned int i=0;i<param->MaxPage;i++)
		//{
		//	int ByteAddr = i / 8;//第幾個byte ;//一個bit代表一個page
		//	int offset = (i % 8);//第幾個bit
		//	//param->fast_page_bit[ByteAddr] = param->fast_page_bit[ByteAddr] | (1 << offset);
		//	param->fast_page_list[i] = i;// GetFastPage(i,param->PageAMap);
		//	
		//}

	}

	//if(param->bMaker == 0xAd && param->id[0]==0xde && param->id[1]==0x14 &&  param->id[2]==0xa7 && param->id[3]==0x42 && param->id[4]==0x4a)
	//{
	//	param->bIs8T2E = 1;
	//	int index=0;
	//	for(unsigned int i=0;i<param->MaxPage;i++)
	//	{
	//		if(((param->fast_page_bit[i/8] >> i%8)&0x01)==0x00) continue;
	//		int fpage = GetPairPage(i,param->PageAMap,param);
	//		//fast_page_bit
	//		int ByteAddr = fpage / 8;//第幾個byte ;//一個bit代表一個page
	//		int offset = (fpage % 8);//第幾個bit
	//		param->PairPageAddress[i] = fpage;
	//	}

	//}
	if (isSixAddressFlash(&buffer[0x40]))
	{
		buffer[0x3B] |= (1<<3);
	}
	//20150420 Annie : FW SetParameter 移到 AP 設定 0x90~0xB3
	SetFlashCmd_Buffer(param,buffer,bE2NAMD);

	return 0;
}


void MyFlashInfo::SetFlashCmd_Buffer(FH_Paramater* param, unsigned char* buffer, unsigned char bE2NAMD)
{
	int flh_cmd_offset = 0x90;
	switch(param->bMaker)
	{
	case 0x89:
	case 0x2c:
		buffer[flh_cmd_offset+0x00]=0;			//TwoPlaneRead3Cycle
		buffer[flh_cmd_offset+0x01]=0xA2;		//D1_Mode
		if(param->bIs3D)
			buffer[flh_cmd_offset+0x01]=0xDA;   //D1_Mode
		buffer[flh_cmd_offset+0x02]=0xDF;		//D3_Mode
		if(param->bIsMicronB36R || param->bIsMicronB37R){
			buffer[flh_cmd_offset+0x01]=0x3B;   //D1_Mode
			buffer[flh_cmd_offset+0x02]=0x3C;	//D3_Mode
		}	
		buffer[flh_cmd_offset+0x03]=0x60;		//EraseCmdS1
		buffer[flh_cmd_offset+0x04]=0x60;		//EraseCmdS2
		buffer[flh_cmd_offset+0x05]=0xD0;		//EraseCmdE1
		buffer[flh_cmd_offset+0x06]=0xD0;		//EraseCmdE2
		buffer[flh_cmd_offset+0x07]=0x80;		//ProgramCmdS 
		buffer[flh_cmd_offset+0x08]=0x80;		//ProgramCmd2PS1
		buffer[flh_cmd_offset+0x09]=0x80;		//ProgramCmd2PS2
		buffer[flh_cmd_offset+0x0A]=0x85;		//ProgramChange
		buffer[flh_cmd_offset+0x0B]=0x10;		//ProgramCmdEnd
		buffer[flh_cmd_offset+0x0C]=0x11;		//MultipageCmd
		buffer[flh_cmd_offset+0x0D]=0x15;		//CacheProgram
		buffer[flh_cmd_offset+0x0E]=0x1A;		//D3_Cache
		buffer[flh_cmd_offset+0x0F]=0x00;		//ReadCmdS
		buffer[flh_cmd_offset+0x10]=0x00;		//ReadCmd2PS1
		buffer[flh_cmd_offset+0x11]=0x00;		//ReadCmd2PS2
		buffer[flh_cmd_offset+0x12]=0x06;		//ReadChangeS
		buffer[flh_cmd_offset+0x13]=0xE0;		//ReadChangeE
		buffer[flh_cmd_offset+0x14]=0x30;		//ReadCmdEnd
		buffer[flh_cmd_offset+0x15]=0x31;		//ReadCmdEnd1
		buffer[flh_cmd_offset+0x16]=0x32;		//ReadCmdEnd2
		buffer[flh_cmd_offset+0x17]=0x30;		//ReadCmdEnd3
		buffer[flh_cmd_offset+0x18]=0x31;		//ReadCache1
		buffer[flh_cmd_offset+0x19]=0x3F;		//ReadCache2 
		buffer[flh_cmd_offset+0x1A]=0x00;		//CopybackReadS
		buffer[flh_cmd_offset+0x1B]=0x00;		//CopybackReadS1
		buffer[flh_cmd_offset+0x1C]=0x00;		//CopybackReadS2
		buffer[flh_cmd_offset+0x1D]=0x32;		//CopybackReadE1
		buffer[flh_cmd_offset+0x1E]=0x35;		//CopybackReadE2
		buffer[flh_cmd_offset+0x1F]=0x85;		//CopybackProgramS
		buffer[flh_cmd_offset+0x20]=0x85;		//CopybackProgramS1
		buffer[flh_cmd_offset+0x21]=0x85;		//CopybackProgramS2
		buffer[flh_cmd_offset+0x22]=0x10;		//CopybackProgramE1
		buffer[flh_cmd_offset+0x23]=0x10;		//CopybackProgramE2
		buffer[flh_cmd_offset+0x24]=0x31;		//D3_ReadCache1
		//micro 16nm tcl  b95a
		if(param->Design==0x16 && param->beD3){
			buffer[flh_cmd_offset+0x00]=0;
			buffer[flh_cmd_offset+0x01]=0x40;
			buffer[flh_cmd_offset+0x02]=0x43;
			buffer[flh_cmd_offset+0x03]=0x60;
			buffer[flh_cmd_offset+0x04]=0x60;
			buffer[flh_cmd_offset+0x05]=0xD0;
			buffer[flh_cmd_offset+0x06]=0xD0;
			buffer[flh_cmd_offset+0x07]=0x80;
			buffer[flh_cmd_offset+0x08]=0x80;
			buffer[flh_cmd_offset+0x09]=0x80;
			buffer[flh_cmd_offset+0x0A]=0x85;
			buffer[flh_cmd_offset+0x0B]=0x10;
			buffer[flh_cmd_offset+0x0C]=0x11;
			buffer[flh_cmd_offset+0x0D]=0x15;
			buffer[flh_cmd_offset+0x0E]=0x1A;
			buffer[flh_cmd_offset+0x0F]=0x00;
			buffer[flh_cmd_offset+0x10]=0x00;
			buffer[flh_cmd_offset+0x11]=0x00;
			buffer[flh_cmd_offset+0x12]=0x06;
			buffer[flh_cmd_offset+0x13]=0xE0;
			buffer[flh_cmd_offset+0x14]=0x30;
			buffer[flh_cmd_offset+0x15]=0x31;
			buffer[flh_cmd_offset+0x16]=0x32;
			buffer[flh_cmd_offset+0x17]=0x30;
			buffer[flh_cmd_offset+0x18]=0x31;
			buffer[flh_cmd_offset+0x19]=0x3F;
			buffer[flh_cmd_offset+0x1A]=0x00;
			buffer[flh_cmd_offset+0x1B]=0x00;
			buffer[flh_cmd_offset+0x1C]=0x00;
			buffer[flh_cmd_offset+0x1D]=0x32;
			buffer[flh_cmd_offset+0x1E]=0x35;
			buffer[flh_cmd_offset+0x1F]=0x85;
			buffer[flh_cmd_offset+0x20]=0x85;
			buffer[flh_cmd_offset+0x21]=0x85;
			buffer[flh_cmd_offset+0x22]=0x10;
			buffer[flh_cmd_offset+0x23]=0x10;
			buffer[flh_cmd_offset+0x24]=0x31;		//D3_ReadCache1

		}
		break;

	case 0xAD:
		if(!(param->beD3 && param->Design <=0x16)) {
			buffer[flh_cmd_offset+0x00]=1;			//TwoPlaneRead3Cycle
			buffer[flh_cmd_offset+0x01]=0xBF;		//D1_Mode
			buffer[flh_cmd_offset+0x02]=0xDF;		//D3_Mode
			buffer[flh_cmd_offset+0x03]=0x60;		//EraseCmdS1
			buffer[flh_cmd_offset+0x04]=0x60;		//EraseCmdS2
			buffer[flh_cmd_offset+0x05]=0xD0;		//EraseCmdE1
			buffer[flh_cmd_offset+0x06]=0xD0;		//EraseCmdE2
			buffer[flh_cmd_offset+0x07]=0x80;		//ProgramCmdS 
			buffer[flh_cmd_offset+0x08]=0x80;		//ProgramCmd2PS1
			buffer[flh_cmd_offset+0x09]=0x80;		//ProgramCmd2PS2
			buffer[flh_cmd_offset+0x0A]=0x85;		//ProgramChange
			buffer[flh_cmd_offset+0x0B]=0x10;		//ProgramCmdEnd
			buffer[flh_cmd_offset+0x0C]=0x11;		//MultipageCmd
			buffer[flh_cmd_offset+0x0D]=0x15;		//CacheProgram
			buffer[flh_cmd_offset+0x0E]=0x1A;		//D3_Cache
			buffer[flh_cmd_offset+0x0F]=0x00;		//ReadCmdS
			buffer[flh_cmd_offset+0x10]=0x60;		//ReadCmd2PS1
			buffer[flh_cmd_offset+0x11]=0x60;		//ReadCmd2PS2
			buffer[flh_cmd_offset+0x12]=0x05;		//ReadChangeS
			buffer[flh_cmd_offset+0x13]=0xE0;		//ReadChangeE
			buffer[flh_cmd_offset+0x14]=0x30;		//ReadCmdEnd
			buffer[flh_cmd_offset+0x15]=0x31;		//ReadCmdEnd1
			buffer[flh_cmd_offset+0x16]=0x32;		//ReadCmdEnd2
			buffer[flh_cmd_offset+0x17]=0x33;		//ReadCmdEnd3
			buffer[flh_cmd_offset+0x18]=0x31;		//ReadCache1
			buffer[flh_cmd_offset+0x19]=0x3F;		//ReadCache2 
			buffer[flh_cmd_offset+0x1A]=0x00;		//CopybackReadS
			buffer[flh_cmd_offset+0x1B]=0x60;		//CopybackReadS1
			buffer[flh_cmd_offset+0x1C]=0x60;		//CopybackReadS2
			buffer[flh_cmd_offset+0x1D]=0x35;		//CopybackReadE1
			buffer[flh_cmd_offset+0x1E]=0x35;		//CopybackReadE2
			buffer[flh_cmd_offset+0x1F]=0x85;		//CopybackProgramS
			buffer[flh_cmd_offset+0x20]=0x85;		//CopybackProgramS1
			buffer[flh_cmd_offset+0x21]=0x81;		//CopybackProgramS2
			buffer[flh_cmd_offset+0x22]=0x10;		//CopybackProgramE1
			buffer[flh_cmd_offset+0x23]=0x10;		//CopybackProgramE2	
			buffer[flh_cmd_offset+0x24]=0x31;		//D3_ReadCache1
			buffer[flh_cmd_offset+0x25]=0xDA;		//BootMode Command
		}
		else{
			buffer[144]		= 0;//TowPlaneRead3Cycle
			buffer[145]     = 0xA2;//D1_Mode//DA
			buffer[146]     = 0xDF;//D3_Mode
			buffer[147]     = 0x60;//EraseCmdS1
			buffer[148]     = 0x60;//EraseCmdS2
			buffer[149]     = 0xD0;//EraseCmdE1
			buffer[150]     = 0xD1;//EraseCmdE2
			buffer[151]     = 0x80;//ProgramCmdS
			buffer[152]     = 0x80;//ProgramCmd2PS1
			buffer[153]     = 0x80;//ProgramCmd2PS2
			buffer[154]     = 0x85;//ProgramChange
			buffer[155]     = 0x10;//ProgramCmdEnd
			buffer[156]     = 0x11;//ProgramCmdEnd
			buffer[157]     = 0x15;//CacheProgram
			buffer[158]     = 0x1A;//D3_Cache
			buffer[159]     = 0x00;//ReadCmdS
			buffer[160]     = 0x00;//ReadCmd2PS1
			buffer[161]     = 0x00;//ReadCmd2PS2
			buffer[162]     = 0x05;//ReadChangeS
			buffer[163]     = 0xE0;//ReadChangeE
			buffer[164]     = 0x30;//ReadCmdEnd
			buffer[165]     = 0x31;//ReadCmdEnd1
			buffer[166]     = 0x32;//ReadCmdEnd2
			buffer[167]     = 0x33;//ReadCmdEnd3
			buffer[168]     = 0x31;//ReadCache1
			buffer[169]     = 0x3F;//ReadCache2
			buffer[170]     = 0x00;//CopybackReadS
			buffer[171]     = 0x60;//CopybackReadS1
			buffer[172]     = 0x60;//CopybackReadS2
			buffer[173]     = 0x32;//CopybackReadE1
			buffer[174]     = 0x35;//CopybackReadE2
			buffer[175]     = 0x85;//CopybackProgramS
			buffer[176]     = 0x85;//CopybackProgramS1
			buffer[177]     = 0x85;//CopybackProgramS2
			buffer[178]     = 0x10;//CopybackProgramE1
			buffer[179]     = 0x10;//CopybackProgramE2
			buffer[180]		= 0x31;		//D3_ReadCache1
			buffer[181]		= 0xda;		//BootMode Command

		}

		if ((param->id[4] & 0x80) == 0x80)
		{
			buffer[153] = 0x81;
			//buffer[155] = 0x23;
			buffer[155] = 0x10;
			buffer[158] = 0x22;
			buffer[167] = 0x35;
			if (param->bIs3D && (param->id[4] == 0x90 || param->id[4] == 0x80)){	// 20181204: 3DV3
				buffer[167] = 0x30;
			}
			buffer[176] = 0x81;
			buffer[177] = 0x81;
			buffer[178] = 0x23;
		}

		if (bE2NAMD)
		{
			buffer[147] = 0x6a;
			buffer[149] = 0xd7;
			buffer[150] = 0xd7;
			buffer[151] = 0x8a;
			buffer[155] = 0x17;
			buffer[159] = 0x0a;
			buffer[164] = 0x37;
		}
		break;

	case 0xEC:
		buffer[flh_cmd_offset+0x00]=0;			//TwoPlaneRead3Cycle
		buffer[flh_cmd_offset+0x01]=0xDA;		//D1_Mode
		buffer[flh_cmd_offset+0x02]=0xDF;		//D3_Mode
		buffer[flh_cmd_offset+0x03]=0x60;		//EraseCmdS1
		buffer[flh_cmd_offset+0x04]=0x60;		//EraseCmdS2
		buffer[flh_cmd_offset+0x05]=0xD0;		//EraseCmdE1
		buffer[flh_cmd_offset+0x06]=0xD0;		//EraseCmdE2
		buffer[flh_cmd_offset+0x07]=0x80;		//ProgramCmdS 
		buffer[flh_cmd_offset+0x08]=0x80;		//ProgramCmd2PS1
		buffer[flh_cmd_offset+0x09]=0x81;		//ProgramCmd2PS2
		buffer[flh_cmd_offset+0x0A]=0x85;		//ProgramChange
		buffer[flh_cmd_offset+0x0B]=0x10;		//ProgramCmdEnd
		buffer[flh_cmd_offset+0x0C]=0x11;		//MultipageCmd
		buffer[flh_cmd_offset+0x0D]=0x15;		//CacheProgram
		buffer[flh_cmd_offset+0x0E]=0xC0;		//D3_Cache
		buffer[flh_cmd_offset+0x0F]=0x00;		//ReadCmdS
		buffer[flh_cmd_offset+0x10]=0x00;		//ReadCmd2PS1
		buffer[flh_cmd_offset+0x11]=0x00;		//ReadCmd2PS2
		buffer[flh_cmd_offset+0x12]=0x05;		//ReadChangeS
		buffer[flh_cmd_offset+0x13]=0xE0;		//ReadChangeE
		buffer[flh_cmd_offset+0x14]=0x30;		//ReadCmdEnd
		buffer[flh_cmd_offset+0x15]=0x31;		//ReadCmdEnd1
		buffer[flh_cmd_offset+0x16]=0x32;		//ReadCmdEnd2
		buffer[flh_cmd_offset+0x17]=0x30;		//ReadCmdEnd3
		buffer[flh_cmd_offset+0x18]=0x31;		//ReadCache1
		buffer[flh_cmd_offset+0x19]=0x3F;		//ReadCache2 
		buffer[flh_cmd_offset+0x1A]=0x00;		//CopybackReadS
		buffer[flh_cmd_offset+0x1B]=0x60;		//CopybackReadS1
		buffer[flh_cmd_offset+0x1C]=0x60;		//CopybackReadS2
		buffer[flh_cmd_offset+0x1D]=0x35;		//CopybackReadE1
		buffer[flh_cmd_offset+0x1E]=0x35;		//CopybackReadE2
		buffer[flh_cmd_offset+0x1F]=0x85;		//CopybackProgramS
		buffer[flh_cmd_offset+0x20]=0x85;		//CopybackProgramS1
		buffer[flh_cmd_offset+0x21]=0x81;		//CopybackProgramS2
		buffer[flh_cmd_offset+0x22]=0x10;		//CopybackProgramE1
		buffer[flh_cmd_offset+0x23]=0x15;		//CopybackProgramE2
		buffer[flh_cmd_offset+0x24]=0x31;		//D3_ReadCache1
		break;

	case 0xC8:
		buffer[flh_cmd_offset+0x00]=0;			//TwoPlaneRead3Cycle
		buffer[flh_cmd_offset+0x01]=0xDA;		//D1_Mode
		buffer[flh_cmd_offset+0x02]=0xDF;		//D3_Mode
		buffer[flh_cmd_offset+0x03]=0x60;		//EraseCmdS1
		buffer[flh_cmd_offset+0x04]=0x60;		//EraseCmdS2
		buffer[flh_cmd_offset+0x05]=0xD0;		//EraseCmdE1
		buffer[flh_cmd_offset+0x06]=0xD0;		//EraseCmdE2
		buffer[flh_cmd_offset+0x07]=0x80;		//ProgramCmdS 
		buffer[flh_cmd_offset+0x08]=0x80;		//ProgramCmd2PS1
		buffer[flh_cmd_offset+0x09]=0x81;		//ProgramCmd2PS2
		buffer[flh_cmd_offset+0x0A]=0x85;		//ProgramChange
		buffer[flh_cmd_offset+0x0B]=0x10;		//ProgramCmdEnd
		buffer[flh_cmd_offset+0x0C]=0x11;		//MultipageCmd
		buffer[flh_cmd_offset+0x0D]=0x10;		//CacheProgram
		buffer[flh_cmd_offset+0x0E]=0xC0;		//D3_Cache
		buffer[flh_cmd_offset+0x0F]=0x00;		//ReadCmdS
		buffer[flh_cmd_offset+0x10]=0x00;		//ReadCmd2PS1
		buffer[flh_cmd_offset+0x11]=0x00;		//ReadCmd2PS2
		buffer[flh_cmd_offset+0x12]=0x05;		//ReadChangeS
		buffer[flh_cmd_offset+0x13]=0xE0;		//ReadChangeE
		buffer[flh_cmd_offset+0x14]=0x30;		//ReadCmdEnd
		buffer[flh_cmd_offset+0x15]=0x31;		//ReadCmdEnd1
		buffer[flh_cmd_offset+0x16]=0x32;		//ReadCmdEnd2
		buffer[flh_cmd_offset+0x17]=0x30;		//ReadCmdEnd3
		buffer[flh_cmd_offset+0x18]=0x31;		//ReadCache1
		buffer[flh_cmd_offset+0x19]=0x3F;		//ReadCache2 
		buffer[flh_cmd_offset+0x1A]=0x00;		//CopybackReadS
		buffer[flh_cmd_offset+0x1B]=0x60;		//CopybackReadS1
		buffer[flh_cmd_offset+0x1C]=0x60;		//CopybackReadS2
		buffer[flh_cmd_offset+0x1D]=0x35;		//CopybackReadE1
		buffer[flh_cmd_offset+0x1E]=0x35;		//CopybackReadE2
		buffer[flh_cmd_offset+0x1F]=0x85;		//CopybackProgramS
		buffer[flh_cmd_offset+0x20]=0x85;		//CopybackProgramS1
		buffer[flh_cmd_offset+0x21]=0x81;		//CopybackProgramS2
		buffer[flh_cmd_offset+0x22]=0x10;		//CopybackProgramE1
		buffer[flh_cmd_offset+0x23]=0x10;		//CopybackProgramE2
		buffer[flh_cmd_offset+0x24]=0x31;		//D3_ReadCache1
		break;

	case 0x98:
	case 0x45:
	case 0x01:
		if(param->beD3)
		{
			buffer[flh_cmd_offset+0x00]=0;			//TwoPlaneRead3Cycle
			buffer[flh_cmd_offset+0x01]=0xA2;		//D1_Mode
			buffer[flh_cmd_offset+0x02]=0xDF;		//D3_Mode
			buffer[flh_cmd_offset+0x03]=0x60;		//EraseCmdS1
			buffer[flh_cmd_offset+0x04]=0x60;		//EraseCmdS2
			buffer[flh_cmd_offset+0x05]=0xD0;		//EraseCmdE1
			buffer[flh_cmd_offset+0x06]=0xD0;		//EraseCmdE2
			buffer[flh_cmd_offset+0x07]=0x80;		//ProgramCmdS 
			buffer[flh_cmd_offset+0x08]=0x80;		//ProgramCmd2PS1
			buffer[flh_cmd_offset+0x09]=0x80;		//ProgramCmd2PS2
			buffer[flh_cmd_offset+0x0A]=0x85;		//ProgramChange
			buffer[flh_cmd_offset+0x0B]=0x10;		//ProgramCmdEnd
			buffer[flh_cmd_offset+0x0C]=0x11;		//MultipageCmd
			buffer[flh_cmd_offset+0x0D]=0x15;		//CacheProgram
			buffer[flh_cmd_offset+0x0E]=0x1A;		//D3_Cache
			buffer[flh_cmd_offset+0x0F]=0x00;		//ReadCmdS

			if(param->Design<0x24)
			{
				buffer[flh_cmd_offset+0x10]=0x00;		//ReadCmd2PS1
				buffer[flh_cmd_offset+0x11]=0x00;		//ReadCmd2PS2
				buffer[flh_cmd_offset+0x15]=0x3C;		//ReadCmdEnd1
			}
			else
			{
				buffer[flh_cmd_offset+0x00]=1;			//TwoPlaneRead3Cycle
				buffer[flh_cmd_offset+0x10]=0x60;		//ReadCmd2PS1
				buffer[flh_cmd_offset+0x11]=0x60;		//ReadCmd2PS2
				buffer[flh_cmd_offset+0x15]=0x30;		//ReadCmdEnd1
			}

			buffer[flh_cmd_offset+0x12]=0x05;		//ReadChangeS
			buffer[flh_cmd_offset+0x13]=0xE0;		//ReadChangeE
			buffer[flh_cmd_offset+0x14]=0x30;		//ReadCmdEnd

			buffer[flh_cmd_offset+0x16]=0x32;		//ReadCmdEnd2
			buffer[flh_cmd_offset+0x17]=0x30;		//ReadCmdEnd3
			buffer[flh_cmd_offset+0x18]=0x31;		//ReadCache1
			buffer[flh_cmd_offset+0x19]=0x3F;		//ReadCache2 
			buffer[flh_cmd_offset+0x1A]=0x00;		//CopybackReadS
			buffer[flh_cmd_offset+0x1B]=0x60;		//CopybackReadS1
			//if(this->versionYear >= 2018 && this->versionMonth >= 9 &&this->versionDay >= 13){
			if( (this->versionYear >= 2019) ||
				(this->versionYear >= 2018 && this->versionMonth >= 10) ||
				(this->versionYear >= 2018 && this->versionMonth >= 9 && this->versionDay >= 13) ){
				//============================================================
				buffer[flh_cmd_offset+0x1C]=0x30;		//CopybackReadS2
				buffer[flh_cmd_offset+0x1D]=0x32;		//CopybackReadE1
				buffer[flh_cmd_offset+0x1E]=0x35;		//CopybackReadE2
				//============================================================
			}
			else{	
				//============================================================
				buffer[flh_cmd_offset+0x1C]=0x60;		//CopybackReadS2
				buffer[flh_cmd_offset+0x1D]=0x30;		//CopybackReadE1
				buffer[flh_cmd_offset+0x1E]=0x30;		//CopybackReadE2
				//============================================================
			}
			buffer[flh_cmd_offset+0x1F]=0x85;		//CopybackProgramS
			buffer[flh_cmd_offset+0x20]=0x85;		//CopybackProgramS1
			buffer[flh_cmd_offset+0x21]=0x85;		//CopybackProgramS2
			buffer[flh_cmd_offset+0x22]=0x10;		//CopybackProgramE1
			buffer[flh_cmd_offset+0x23]=0x15;		//CopybackProgramE2

			buffer[flh_cmd_offset+0x24]=0x31;		//D3_ReadCache1

			if (param->Design == 0x24)
				buffer[flh_cmd_offset+0x24]=0x3C;	//D3_ReadCache1 for 24nm
		}
		else//D2
		{
			buffer[flh_cmd_offset+0x00]=1;			//TwoPlaneRead3Cycle
			buffer[flh_cmd_offset+0x01]=0xA2;		//D1_Mode
			buffer[flh_cmd_offset+0x02]=0xDF;		//D3_Mode
			buffer[flh_cmd_offset+0x03]=0x60;		//EraseCmdS1
			buffer[flh_cmd_offset+0x04]=0x60;		//EraseCmdS2
			buffer[flh_cmd_offset+0x05]=0xD0;		//EraseCmdE1
			buffer[flh_cmd_offset+0x06]=0xD0;		//EraseCmdE2
			buffer[flh_cmd_offset+0x07]=0x80;		//ProgramCmdS 
			buffer[flh_cmd_offset+0x08]=0x80;		//ProgramCmd2PS1
			buffer[flh_cmd_offset+0x09]=0x80;		//ProgramCmd2PS2
			buffer[flh_cmd_offset+0x0A]=0x85;		//ProgramChange
			buffer[flh_cmd_offset+0x0B]=0x10;		//ProgramCmdEnd
			buffer[flh_cmd_offset+0x0C]=0x11;		//MultipageCmd
			buffer[flh_cmd_offset+0x0D]=0x15;		//CacheProgram
			buffer[flh_cmd_offset+0x0E]=0x1A;		//D3_Cache
			buffer[flh_cmd_offset+0x0F]=0x00;		//ReadCmdS
			buffer[flh_cmd_offset+0x10]=0x60;		//ReadCmd2PS1
			buffer[flh_cmd_offset+0x11]=0x60;		//ReadCmd2PS2
			buffer[flh_cmd_offset+0x12]=0x05;		//ReadChangeS
			buffer[flh_cmd_offset+0x13]=0xE0;		//ReadChangeE
			buffer[flh_cmd_offset+0x14]=0x30;		//ReadCmdEnd
			buffer[flh_cmd_offset+0x15]=0x3C;		//ReadCmdEnd1
			buffer[flh_cmd_offset+0x16]=0x32;		//ReadCmdEnd2
			buffer[flh_cmd_offset+0x17]=0x30;		//ReadCmdEnd3
			buffer[flh_cmd_offset+0x18]=0x31;		//ReadCache1
			buffer[flh_cmd_offset+0x19]=0x3F;		//ReadCache2 
			buffer[flh_cmd_offset+0x1A]=0x00;		//CopybackReadS
			buffer[flh_cmd_offset+0x1B]=0x60;		//CopybackReadS1
			buffer[flh_cmd_offset+0x1C]=0x60;		//CopybackReadS2
			buffer[flh_cmd_offset+0x1D]=0x30;		//CopybackReadE1
			buffer[flh_cmd_offset+0x1E]=0x3A;		//CopybackReadE2
			buffer[flh_cmd_offset+0x1F]=0x8C;		//CopybackProgramS
			buffer[flh_cmd_offset+0x20]=0x8C;		//CopybackProgramS1
			buffer[flh_cmd_offset+0x21]=0x8C;		//CopybackProgramS2
			buffer[flh_cmd_offset+0x22]=0x10;		//CopybackProgramE1
			buffer[flh_cmd_offset+0x23]=0x15;		//CopybackProgramE2	
			buffer[flh_cmd_offset+0x24]=0x31;		//D3_ReadCache1
		}
		break;

	case 0x9B:
		buffer[flh_cmd_offset+0x00]=0;			//TwoPlaneRead3Cycle
		buffer[flh_cmd_offset+0x01]=0xDA;		//D1_Mode
		buffer[flh_cmd_offset+0x02]=0xDF;		//D3_Mode
		buffer[flh_cmd_offset+0x03]=0x60;		//EraseCmdS1
		buffer[flh_cmd_offset+0x04]=0x60;		//EraseCmdS2
		buffer[flh_cmd_offset+0x05]=0xD0;		//EraseCmdE1
		buffer[flh_cmd_offset+0x06]=0xD0;		//EraseCmdE2
		buffer[flh_cmd_offset+0x07]=0x80;		//ProgramCmdS 
		buffer[flh_cmd_offset+0x08]=0x80;		//ProgramCmd2PS1
		buffer[flh_cmd_offset+0x09]=0x80;		//ProgramCmd2PS2	//20190508 0x81->0x80
		buffer[flh_cmd_offset+0x0A]=0x85;		//ProgramChange
		buffer[flh_cmd_offset+0x0B]=0x10;		//ProgramCmdEnd
		buffer[flh_cmd_offset+0x0C]=0x11;		//MultipageCmd		//20190508 0x10->0x11
		buffer[flh_cmd_offset+0x0D]=0x15;		//CacheProgram
		buffer[flh_cmd_offset+0x0E]=0x1A;		//D3_Cache
		buffer[flh_cmd_offset+0x0F]=0x00;		//ReadCmdS
		buffer[flh_cmd_offset+0x10]=0x00;		//ReadCmd2PS1
		buffer[flh_cmd_offset+0x11]=0x00;		//ReadCmd2PS2
		buffer[flh_cmd_offset+0x12]=0x06;		//ReadChangeS
		buffer[flh_cmd_offset+0x13]=0xE0;		//ReadChangeE
		buffer[flh_cmd_offset+0x14]=0x30;		//ReadCmdEnd
		buffer[flh_cmd_offset+0x15]=0x31;		//ReadCmdEnd1
		buffer[flh_cmd_offset+0x16]=0x32;		//ReadCmdEnd2
		buffer[flh_cmd_offset+0x17]=0x30;		//ReadCmdEnd3
		buffer[flh_cmd_offset+0x18]=0x31;		//ReadCache1
		buffer[flh_cmd_offset+0x19]=0x3F;		//ReadCache2 
		buffer[flh_cmd_offset+0x1A]=0x00;		//CopybackReadS
		buffer[flh_cmd_offset+0x1B]=0x00;		//CopybackReadS1
		buffer[flh_cmd_offset+0x1C]=0x00;		//CopybackReadS2
		buffer[flh_cmd_offset+0x1D]=0x32;		//CopybackReadE1	//20190508 0x35->0x32
		buffer[flh_cmd_offset+0x1E]=0x35;		//CopybackReadE2
		buffer[flh_cmd_offset+0x1F]=0x85;		//CopybackProgramS
		buffer[flh_cmd_offset+0x20]=0x85;		//CopybackProgramS1
		buffer[flh_cmd_offset+0x21]=0x85;		//CopybackProgramS2
		buffer[flh_cmd_offset+0x22]=0x10;		//CopybackProgramE1
		buffer[flh_cmd_offset+0x23]=0x10;		//CopybackProgramE2	//20190508 0x15->0x10
		buffer[flh_cmd_offset+0x24]=0x31;		//D3_ReadCache1
		break;
	}
}


int MyFlashInfo::GetPairPage(int FPage,unsigned char MLC_PageA,FH_Paramater* param)
{
	int fastindex = -1;
	int slowindex = -1;
	switch (MLC_PageA)
	{
	case PAGE_21:
			
			/*if(FPage==0xfb) return 0xfe;*/
			if(FPage==0xfd && param->MaxPage==256) return 0xff;
			if(FPage==0x1fd && param->MaxPage==512) return 0x1ff;
			for(unsigned int i=0;i<param->MaxPage;i++){
				if((param->fast_page_bit[i/8] >> (i%8)&0x01)==0x01){
					fastindex++;
					if(FPage == i) break;
				}
			}
			
			for(unsigned int i=0;i<param->MaxPage;i++){
				if((param->fast_page_bit[i/8] >> (i%8)&0x01)==0x00){
					slowindex++;
					if(slowindex == fastindex) {
						return i;
					}
				}
			}
			break;
	case PAGE_43: 
			for(unsigned int i=0;i<param->MaxPage;i++){
				if((param->fast_page_bit[i/8] >> (i%8)&0x01)==0x01){
					fastindex++;
					if(FPage == i) break;
				}
			}
			for(unsigned int i=0;i<param->MaxPage;i++){
				if((param->fast_page_bit[i/8] >> (i%8)&0x01)==0x00){
					slowindex++;
					if(slowindex == fastindex) {
						return i;
					}
				}
			}
			break;
	case PAGE_48: // 0~15 , 16 18 20..
		if(FPage < 16)
			return FPage;
		else if(FPage < 496)
		{
			if((FPage % 2))
				FPage = FPage - 1;
			else
				FPage = FPage + 1;

			return FPage;
		}
		else
			return FPage;
	default:
		return 0;
		break;
	}
	return 0;
}

int MyFlashInfo::GetFastPage(int FPage, unsigned char MLC_PageA)
{
	int VPage = FPage;
	switch (MLC_PageA)
	{

	case PAGE_21: // 0,1,3,5,7
		if (VPage==0) 
			return VPage;//goto FP_Exit;
		VPage--;
		return (1 + (VPage<<1));
		break;
	case PAGE_22: // 0,2,3,5,7,9,11,13,15,17,19,21.........
		if (VPage==0) 
			return VPage;//goto FP_Exit;
		if (VPage==1)
			return VPage+1;
		VPage--;
		return (1 + (VPage<<1));
		break;

	case PAGE_35:
		if (VPage<2) /* 0,1,3,6,9 */
		{
			return 1;
			//break;
		}else
		{
			VPage--;
			return  (VPage*3);
			//VPage++;
		}       
		break;

	case PAGE_42: // (0,1,2,3,4,5),6,7,10,11,14,15,18,19,...
		if (VPage<6) // New mapping for Samsung
			return VPage;
		else
			return (VPage-3)*2-VPage%2;
		break;

	case PAGE_43: // 0,1,2,3,6,7,10,11,14,15,...126,127
		if(FPage >= 254)
			return 0;
		if (VPage<2)
		{
			return VPage;
			//goto FP_Exit;
		}else
		{
			VPage -= 2;
			FPage = 2;
			FPage |= (VPage&0x1);
			FPage += ((VPage>>1)<<2);
			return FPage;
			//VPage += 2;
		}       
		break;

	case PAGE_44: // 0,1,2,3,[4,5,]8,9,12,13...
		if (VPage<4)
		{
			return VPage;
			break;
			//return;
		}
		VPage -= 4;
		FPage= 4;
		FPage |= (VPage&0x01);
		FPage += ((VPage>>1)<<2);
		return FPage;
		//VPage.All += 4U;
		break;

	case PAGE_45: // 0,1,2,3,4,5,7,8,10,11,14,15
		if(VPage < 6)
		{
			FPage = VPage;
		}
		else if(VPage < 8)
		{
			FPage = VPage+1;
		}
		else if(VPage < 8)
		{
			FPage = VPage+1;
		}
		else if(VPage > 257)
		{
			FPage = VPage+251;
		}
		else
		{
			VPage -= 8;
			FPage = (VPage/2)*4+(VPage&0x01)+10;
			VPage += 8;
		}
		return FPage;
		//break;
	case PAGE_46: // 0~63 , 65 67 69 ..
		if(VPage < 64)
		{
			FPage = VPage;
		}
		else if(VPage <128){
			FPage = VPage/2;
		}
		else
		{
			FPage = VPage;
			if(!(VPage % 2))
				FPage = VPage-63;
			return  FPage;
			/*if((VPage/2) < 65)
				return VPage;
			FPage = (VPage/2) + ((VPage/2)-65);*/
		}

		return FPage;
	case PAGE_48: // 0~15 , 16 18 20..
	if(VPage < 16)
		return FPage;
	else if(VPage < 496)
	{
		if((VPage % 2))
			FPage = VPage - 1;
		else
			FPage = VPage;

		return FPage;
	}
	else
		return FPage;
	case PAGE_49: // 0,1,2,3,5,7,9,11...
		if( VPage<=3 )
			return VPage;
		return ((VPage-2)*2+1);
		break;
	case PAGE_4A: // 0,2,4,6,8,10...
		return VPage*2;
		break;
	case PAGE_61: // 0,1,2,3,4,5,(8,9),14,15,20,21...
		if (VPage<6)
		{
			FPage = VPage;
			return FPage; // bugs!!!
		}
		else{
			VPage -= 0x06;
			FPage = 0x08;
			FPage |= (VPage&0x01);
			FPage+= ((VPage>>1)*6);
			return FPage;
			//VPage.All += 6U;
		}       
		break;
	default:
		FPage = VPage;
		return FPage;
		break;
	}



	return 0;
}

int MyFlashInfo::getFlashInverse ( FH_Paramater* param, unsigned char bDrv, const unsigned char* buf ,bool mixmode,int over4ce, unsigned char bUFSCE)
{
	int iRT = 0;

	param->FlhNumber= 1;
    unsigned char CE_Full = 1;
	int secondFlash = 0;
	int Second_Flash_number = 0;

	bool Second_Flash = 0;

	unsigned char bTemp[528] = {0};
	//if( flh==NULL ) return 0x1;
	//memset( flh, 0, sizeof(FlashChar));
	//memset(&_FH,0,sizeof(_FH_Paramater));
	param->FlashSlot  =1;
    unsigned char DefaultInverseCE[9] = {0xFE,0xFD,0xFB,0xF7,0xEF,0xDF,0xBF,0x7F,0x00};
	unsigned char over4ceDefaultInverseCE[9] = {0xFE, 0xFB, 0xFD, 0xFC, 0xFF, 0xFA, 0xF9, 0xF8, 0xF7};
	unsigned char inverseCE[9] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00};
	unsigned char over4ceinverseCE[9] = {0xF7,0xF7,0xF7,0xF7,0xF7,0xF7,0xF7,0xF7,0xf7};
	unsigned char UFS_2CE[9] = {0xFE, 0xF6, 0xFF, 0xFF ,0xFF ,0xFF ,0xFF ,0xFF ,0xF7};
	unsigned char UFS_4CE[9] = {0xFE, 0xFD, 0xF6, 0xF5, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7};
	unsigned char UFS_8CE[9] = {0xFE, 0xFD, 0xFC, 0xFB, 0xF6, 0xF5, 0xF4, 0xF3, 0xF7};

    //unsigned char Flash_ZoneNo_Table[14] = {4,8,2,4,8,16,4,8,1,16,8,2,32,4};
    //unsigned char DataPage_Table[6] = { 0x07,0xF0,0x0c,0x10,0x07,0xF0 };

	

	//memcpy(_FH.Flash_ZoneNo_Table,Flash_ZoneNo_Table,14);
	//memcpy(_FH.DataPage_Table,DataPage_Table,6);

	//---- get flash :maker, id(8), code, count ----
	//count = 0;
	//do {
	//	memset( buf, 0, sizeof(buf) );
	//	iRT = NTCmd->NT_GetFlashExtendID( bType,this->DeviceHandle,bDrv,buf);//bType,this->DeviceHandle,bDrv , buf );
	//	if( iRT==0 ) break;
	//	Sleep( 25 );
	//	count++;		
	//} while ( 15 > count );

	//memcpy(buf,GBuffer2,528);
	unsigned char bCEMAPPING = 1;

	if(!mixmode){
		for ( int i=1; i<32; i++ ) {

			if ( buf[i*16] != 0x00 ) {
				if( ((param->bMaker==buf[i*16]) && (param->id[0]==buf[i*16+1]) && (param->id[1]==buf[i*16+2]) && (param->id[2]==buf[i*16+3]))
					|| (_SupportWrongID && (param->bMaker==buf[i*16])) ) {
						param->FlhNumber++;
						bCEMAPPING |= (1<<i);
				}
				else {
					//error id
					//OutReport(IDMS_ERR_FlashID);
					//	return 1;
				}
			}
		}
	}
	else{
		for ( int i=1; i<32; i++ ) {

			if ( buf[i*16] != 0x00 ) {
				if((param->bMaker == buf[i*16])) {
					param->FlhNumber++;
					bCEMAPPING |= (1<<i);
				}
			}
		}
	}

	////設定強制CE , 指認到1ce 但是要用2ce刷, 目前暫定認為他是連續ce
	//for(int i=param->FlhNumber;i<Force;i++){
	//	bCEMAPPING |= (1<<(i));
	//	param->FlhNumber++;
	//}

	if (ForceCeCount)
	{
		param->FlhNumber = 1;
		bCEMAPPING = 1;
		for(int i=1;i<ForceCeCount;i++){
			bCEMAPPING |= (1<<(i));
			param->FlhNumber++;
		}
	}

	int index = 0;
	for ( int i=0; i<8; i++ ) {
		if(((bCEMAPPING>>i)&0x01) == 0x01) //代表CE i 有東西
		{
			param->CEMap[index] = i;

			if (bUFSCE == 2)
				inverseCE[index++] = UFS_2CE[i];
			else if (bUFSCE == 4)
				inverseCE[index++] = UFS_4CE[i];
			else if (bUFSCE == 8)
				inverseCE[index++] = UFS_8CE[i];			
			else if(!over4ce)
				inverseCE[index++] = DefaultInverseCE[i];
			else{
				over4ceinverseCE[index++] = over4ceDefaultInverseCE[i];
			}
			
		}
	}

	memcpy(param->inverseCE,inverseCE,9);
	if(over4ce){
		memcpy(param->inverseCE,over4ceinverseCE,9);
	}
//
//	int mapp = ~bCEMAPPING;
//	int mapping1[8] = {0,1,2,3,4,5,6,7}; //00
//	int mapping2[8] = {0,2,4,5,1,3,5,7}; //01
//	int mapping3[8] = {0,2,1,3,4,6,6,7}; //10
//	int mapping4[8] = {0,1,4,5,2,3,6,7}; //11
//	
//	int index1= 0;
//	int aa = 1;
//
//	for ( int i=0; i<8; i++ ) {
//		if(((mapp>>i)&0x01) == 0x01) //代表CE i 有東西
//		{
//			if(mapping1[i] != index)
//				aa = 0;
//			else
//				index ++;
//		}
//	}
//	if(aa == 1)
//		aa = 0x00;
//	if(aa == 0)
//	{
//		index1= 0;
//		aa = 1;
//		for ( int i=0; i<8; i++ ) {
//			if(((mapp>>i)&0x01) == 0x01) //代表CE i 有東西
//			{
//				if(mapping1[i] != index)
//					aa = 0;
//				else
//					index ++;
//			}
//		}
//	}
//	if(aa == 1)
//		aa = 0x01;
//
//	if(aa == 0)
//	{
//		index1= 0;
//		aa = 1;
//		for ( int i=0; i<8; i++ ) {
//			if(((mapp>>i)&0x01) == 0x01) //代表CE i 有東西
//			{
//				if(mapping1[i] != index)
//					aa = 0;
//				else
//					index ++;
//			}
//		}
//	}
//if(aa == 1)
//		aa = 0x10;


#if 0
    for ( int i=1; i<4; i++ ) {

        if ( buf[i*16] != 0x00 ) {
			if((param->bMaker == buf[i*16])&&(param->id[0]==buf[i*16+1])&&
                    ( param->id[1] == buf[i*16+2] ) && ( param->id[2] == buf[i*16+3] ) ) {
				param->FlhNumber++;
			}
            else {
				//error id
				//OutReport(IDMS_ERR_FlashID);
				return 1;
			}
		}
	}
    for ( int i=4; i<8; i++ ) {

        if ( buf[i*16] != 0x00 ) {
			if((param->bMaker == buf[i*16])&&(param->id[0]==buf[i*16+1])&&
                    ( param->id[1] == buf[i*16+2] ) && ( param->id[2] == buf[i*16+3] ) ) {
				Second_Flash_number++;
			}
            else {
				//error id
				//OutReport(IDMS_ERR_FlashID);
				return 1;
			}
		}
	}
	int i=0;
    if ( Second_Flash_number!=param->FlhNumber ) {
        Second_Flash = 0;
    }
    else {
		param->FlhNumber<<=1;
		Second_Flash = 1;
        switch ( param->FlhNumber ) {
		case 0x02:
			param->inverseCE[1] = 0xEF;
                for ( i=2; i<8; i++ ) {
                    param->inverseCE[i] = 0xFF;
                }
			param->inverseCE[8] = 0xEE;
			break;
		case 0x04:
			param->inverseCE[2] = 0xEF;
			param->inverseCE[3] = 0xDF;
                for ( i=4; i<8; i++ ) {
                    param->inverseCE[i] = 0xFF;
                }
			param->inverseCE[8] = 0xCC;
			break;
		case 0x08:
			break;
		}   	

	}
#endif

	
	param->DieNumber = 1 << param->DIE_Shift;
#if 0
	param->MaxEccBit = bControllerECC > param->ECC ? bControllerECC : _FH.ECC;
	unsigned int dwCapacity = MpComm->code2size(pDev.flashchar.id[0]);
	dwCapacity *= _FH.FlhNumber;
	pDev.flashchar.FlashSize  = _FH.MaxCapacity = dwCapacity;
#endif
	assert(param->FlhNumber);
	return 0;
}

int MyFlashInfo::getFlashInverseED3 ( FH_Paramater* param,unsigned char bDrv, const unsigned char* buf,bool mixmode, int over4ce, unsigned char bUFSCE, bool bIsCheckExtendBuf)
{

    unsigned char DefaultInverseCE[9] = {0xFE,0xFD,0xFB,0xF7,0xEF,0xDF,0xBF,0x7F,0x00};
	unsigned char inverseCE[9] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00};
	unsigned char over4ceDefaultInverseCE[9] = {0xFE, 0xFB, 0xFD, 0xFC, 0xFF, 0xFA, 0xF9, 0xF8, 0xF7};
	unsigned char over4ceinverseCE[9] = {0xF7,0xF7,0xF7,0xF7,0xF7,0xF7,0xF7,0xF7,0xf7};
    unsigned char Flash_ZoneNo_Table[14] = {4,8,2,4,8,16,4,8,1,16,8,2,32,4};
    unsigned char DataPage_Table[6] = { 0x07,0xF0,0x0c,0x10,0x07,0xF0 };
	unsigned char UFS_2CE[9] = {0xFE, 0xF6, 0xFF, 0xFF ,0xFF ,0xFF ,0xFF ,0xFF ,0xF7};
	unsigned char UFS_4CE[9] = {0xFE, 0xFD, 0xF6, 0xF5, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7};
	unsigned char UFS_8CE[9] = {0xFE, 0xFD, 0xFC, 0xFB, 0xF6, 0xF5, 0xF4, 0xF3, 0xF7};

    unsigned char CE_Full = 1;
	int secondFlash = 0;
	int Second_Flash_number = 0;
	bool Second_Flash = 0;
	param->FlhNumber = 1;
	//param->MaxPage = (param->Page_H << 8 | param->Page_L) / 3; //86 WL

	int EndIndex=8;
	if(bIsCheckExtendBuf){
		EndIndex=16;
	}
	unsigned char bCEMAPPING = 1;
	for ( int i=1; i<EndIndex; i++ ) {

		if ( buf[i*16] != 0x00 || _SupportWrongID ) {
			if( ((param->bMaker==buf[i*16]) && (param->id[0]==buf[i*16+1]) && (param->id[1]==buf[i*16+2]) && (param->id[2]==buf[i*16+3]))
				|| (_SupportWrongID && (param->bMaker==buf[i*16] || buf[i*16]!=0x00)) ) {
					param->FlhNumber++;
					bCEMAPPING |= (1<<i);
			}
			else if( _SupportWrongID && buf[i*16]==0x00 ){
				// 把 ID 全 0x00 的 CE 也算進來, 後面會再用 Die_Test 來跳過.
				// EX:
				// 0x00: 45 3c
				// 0x10: 45 3c
				// 0x20: 00 00
				// 0x30: 45 3c
				bool haveNextCE=false;
				for ( int j=i+1; j<EndIndex; j++ ) {
					if(buf[j*16]!=0x00){
						haveNextCE=true;
					}
				}
				if(haveNextCE){
					param->FlhNumber++;
					bCEMAPPING |= (1<<i);
				}
			}
			else{
				//error id
				//OutReport(IDMS_ERR_FlashID);
				//return 1;
			}
		}
	}

	if (ForceCeCount)
	{
		param->FlhNumber = 1;
		bCEMAPPING = 1;
		for(int i=1;i<ForceCeCount;i++){
			bCEMAPPING |= (1<<(i));
			param->FlhNumber++;
		}
	}

	int index = 0;
	for ( int i=0; i<8; i++ ) {
		if(((bCEMAPPING>>i)&0x01) == 0x01) //代表CE i 有東西
		{
			param->CEMap[index] = i;
			if (bUFSCE == 2)
				inverseCE[index++] = UFS_2CE[i];
			else if (bUFSCE == 4)
				inverseCE[index++] = UFS_4CE[i];
			else if (bUFSCE == 8)
				inverseCE[index++] = UFS_8CE[i];			
			else if(!over4ce)
				inverseCE[index++] = DefaultInverseCE[i];
			else{
				over4ceinverseCE[index++] = over4ceDefaultInverseCE[i];
			}

		}
	}

	memcpy(param->inverseCE,inverseCE,9);
	if(over4ce){
		memcpy(param->inverseCE,over4ceinverseCE,9);
	}

	if (ForceSortingCE)
	{
		inverseCE[0] = inverseCE[ForceSortingCE - 1];
		memset(&inverseCE[1], 0xFF, 7);
		memcpy(param->inverseCE,inverseCE,9);
		param->FlhNumber = 1;
	}

#if 0
    if ( param->IO_Bus==0x16 ) {
        for ( int i=1; i<2; i++ ) {

			if((param->bMaker == buf[i*16])&&(param->id[0]==buf[i*16+1])&&
                    ( param->id[1] == buf[i*16+2] ) && ( param->id[2] == buf[i*16+3] ) ) {
				param->FlhNumber++;
			}
            else {
                break;
		}
        }
        for ( int i=2; i<4; i++ ) {


			if((param->bMaker == buf[i*16])&&(param->id[0]==buf[i*16+1])&&
                    ( param->id[1] == buf[i*16+2] ) && ( param->id[2] == buf[i*16+3] ) ) {
				Second_Flash_number++;
			}
            else {
                break;
		}
	}
    }
    else {
        for ( int i=1; i<4; i++ ) {

            if ( buf[i*16] != 0x00 ) {

				if((param->bMaker == buf[i*16])&&(param->id[0]==buf[i*16+1])&&
                        ( param->id[1] == buf[i*16+2] ) && ( param->id[2] == buf[i*16+3] ) ) {
					param->FlhNumber++;
				}
                else {
					//error id
					//OutReport(IDMS_ERR_FlashID);
					return 1;
				}
			}
		}
        for ( int i=4; i<8; i++ ) {

            if ( buf[i*16] != 0x00 ) {

				if((param->bMaker == buf[i*16])&&(param->id[0]==buf[i*16+1])&&
                        ( param->id[1] == buf[i*16+2] ) && ( param->id[2] == buf[i*16+3] ) ) {
					Second_Flash_number++;
				}
                else {
					//error id
					//OutReport(IDMS_ERR_FlashID);
					return 1;
				}
			}
		}
	}

    if ( Second_Flash_number!=param->FlhNumber ) {
		Second_Flash = 0;
    }
    else {
		param->FlhNumber<<=1;
		Second_Flash = 1;
        if ( param->IO_Bus == 0x16 ) { // 16 bit
            switch ( param->FlhNumber ) { //Max 4 CE for 16 bit
			case 0x02:
				param->inverseCE[1] = 0xFB; //1111 1011
                    for ( int i=2; i<8; i++ ) {
                        param->inverseCE[i] = 0xFF;
                    }
				param->inverseCE[8] = 0xFA; //1111 1010
				break;
			case 0x04:
				param->inverseCE[2] = 0xFB;
				param->inverseCE[3] = 0xF7;
                    for ( int i=4; i<8; i++ ) {
                        param->inverseCE[i] = 0xFF;
                    }
				param->inverseCE[8] = 0xF0;
				break;
				break;
			}
		}
        else {
            switch ( param->FlhNumber ) {
			case 0x02:
				param->inverseCE[1] = 0xEF;
                    for ( int i=2; i<8; i++ ) {
                        param->inverseCE[i] = 0xFF;
                    }
				param->inverseCE[8] = 0xEE;
				break;
			case 0x04:
				param->inverseCE[2] = 0xEF;
				param->inverseCE[3] = 0xDF;
                    for ( int i=4; i<8; i++ ) {
                        param->inverseCE[i] = 0xFF;
                    }
				param->inverseCE[8] = 0xCC;
				break;
			case 0x08:
				break;
			}
		}
	}
#endif
	assert(param->FlhNumber);
	return 0;
}

bool MyFlashInfo::IsValid()
{
	return _valid;
}


unsigned char _IsTSB3D(const FH_Paramater* fh)
{
    if ( fh->bMaker==0x98 || fh->bMaker==0x45 ) {
		if ( fh->id[4]&0x20 ) {
			int gen = (fh->id[4]&0x07) + 1;
			if (gen >= 1) { //BiCS2 (gen=2), BiCS3 (gen=3), BiCS4 (gen=4)
				return gen;
			}
			else
				return 0;
		}
		else {
			return fh->id[4]&0x20;
		}
    }
    else {
        return 0;
    }
}

/*
unsigned char _IsTSB3D(const FH_Paramater* fh)
{
    if ( fh->bMaker==0x98 || fh->bMaker==0x45 ) {
        unsigned char v = fh->id[4]&0x20;
        if ( v ) {
            return v|fh->id[4]&0x0F;
        }
        return 0;
    }
    else {
        return 0;
    }
}
*/
unsigned char _IsMicron3D(const FH_Paramater* fh)
{
    if ( fh->bMaker==0x2C || fh->bMaker==0x89 || fh->bMaker==0xB5 || fh->bMaker==0x32) {//0x32 is for B27B toggle (temporary)
		unsigned char MLC_3DL05[] = {0x84,0x44,0x34,0xAA,0x00};
        unsigned char MLC_3DL06[] = {0xA4,0x64,0x32,0xAA,0x04};
		unsigned char MLC_3DL06Intel[] = {0xA4,0x64,0x32,0xAA,0x01};
		unsigned char MLC_3DL062[] = {0xC4,0xE5,0x32,0xAA,0x04};
        unsigned char MLC_3DL04[] = {0x64,0x44,0x32,0xA5,0x00};
		unsigned char TLC_3D_B0K_0[] = {0xB4,0x78,0x32,0xAA,0x01};
        unsigned char TLC_3D_B0K[] = {0xB4,0x78,0x32,0xAA,0x04};
		unsigned char TLC_3D_B0K_2[] = {0xB4,0x78,0x32,0xAA,0x05};
        unsigned char TLC_3D_B05[] = {0x84,0xD8,0x32,0xA1,0x00};
		unsigned char TLC_3D_B05_2[] = {0x84,0x58,0x32,0xA1,0x00};
		unsigned char TLC_3D_B16[] = {0xA4,0x88,0x32,0xA1,0x00};
		unsigned char TLC_3D_B162[] = {0xA4,0x08,0x32,0xA1,0x00};
		unsigned char TLC_3D_B163[] = {0xC4,0x89,0x32,0xA1,0x00};
		unsigned char TLC_3D_B17[] = {0xC4,0x08,0x32,0xA6,0x00};
		unsigned char TLC_3D_B17Intel[] = {0xC4,0x89,0x32,0xA6,0x00};
		unsigned char TLC_3D_B17Intel1[] = {0xD4,0x89,0x32,0xA6,0x00};
		unsigned char TLC_3D_B27A_1[] = {0xC4,0x08,0x32,0xA2,0x00};
		unsigned char TLC_3D_B27A_2[] = {0xC4,0x18,0x32,0xA2,0x00};
		unsigned char TLC_3D_B27B[] = {0x2C,0xC3,0x08,0x32,0xE6};
		unsigned char TLC_3D_B27B_1[] = {0xC3,0x08,0x32,0xE6,0x00};
		unsigned char TLC_3D_B27B_2[] = {0xD3,0x89,0x32,0xE6,0x00};
		unsigned char QLC_3D_N18A[] = {0xD4,0x0C,0x32,0xAA,0x00};
		unsigned char QLC_3D_N28A[] = {0x2C,0xD3,0x9C,0x32,0xC6};
		unsigned char QLC_3D_N28A_1[] = {0xD3,0x1C,0x32,0xC6,0x00};
		unsigned char QLC_3D_N28A_2[] = {0xD3,0x9C,0x32,0xC6,0x00};
		 if ( memcmp(fh->id,MLC_3DL05,sizeof(MLC_3DL05))==0 )
        {
            return 4;
        }
        if ( memcmp(fh->id,MLC_3DL06,sizeof(MLC_3DL06))==0 )
        {
            return 6;
        }
        if ( memcmp(fh->id,MLC_3DL06Intel,sizeof(MLC_3DL06Intel))==0 )
        {
            return 6;
        }
		if ( memcmp(fh->id,MLC_3DL062,sizeof(MLC_3DL062))==0 )
        {
            return 6;
        }
        if ( memcmp(fh->id,MLC_3DL04,sizeof(MLC_3DL04))==0 )
        {
            return 4;
        }        
        if (( memcmp(fh->id,TLC_3D_B0K,sizeof(TLC_3D_B0K))==0 ) || ( memcmp(fh->id,TLC_3D_B0K_0,sizeof(TLC_3D_B0K_0))==0 ))
        {
            return 1;
        }
		if ( memcmp(fh->id,TLC_3D_B0K_2,sizeof(TLC_3D_B0K_2))==0 )
		{
			return 2;
		}
        if ( memcmp(fh->id,TLC_3D_B05,sizeof(TLC_3D_B05))==0 || memcmp(fh->id,TLC_3D_B05_2,sizeof(TLC_3D_B05_2))==0 )
        {
            return 5;
        }
		if (( memcmp(fh->id,TLC_3D_B16,sizeof(TLC_3D_B16))==0 ) || ( memcmp(fh->id,TLC_3D_B162,sizeof(TLC_3D_B162))==0 ) || ( memcmp(fh->id,TLC_3D_B163,sizeof(TLC_3D_B163))==0 ) )
        {
            return 16;
        }
		if ( ( memcmp(fh->id,TLC_3D_B17Intel,sizeof(TLC_3D_B17Intel))==0 ) || ( memcmp(fh->id,TLC_3D_B17,sizeof(TLC_3D_B17))==0 ) || ( memcmp(fh->id,TLC_3D_B17Intel1,sizeof(TLC_3D_B17Intel1))==0 ) )
        {
            return 17;
        }
		if ( ( memcmp(fh->id,TLC_3D_B27B_1,sizeof(TLC_3D_B27B_1))==0 ) || ( memcmp(fh->id,TLC_3D_B27B_2,sizeof(TLC_3D_B27B_2))==0 ) || ( memcmp(fh->id,TLC_3D_B27B,sizeof(TLC_3D_B27B))==0 ))
        {
            return 26;
        }
		if ( ( memcmp(fh->id,TLC_3D_B27A_1,sizeof(TLC_3D_B27A_1))==0 ) || ( memcmp(fh->id,TLC_3D_B27A_2,sizeof(TLC_3D_B27A_2))==0 ))
        {
            return 27;
        }
		if ( memcmp(fh->id,QLC_3D_N18A,sizeof(QLC_3D_N18A))==0 )
        {
            return 18;
        }
		if ( ( memcmp(fh->id,QLC_3D_N28A,sizeof(QLC_3D_N28A))==0 ) || ( memcmp(fh->id,QLC_3D_N28A_1,sizeof(QLC_3D_N28A_1))==0 ) || ( memcmp(fh->id,QLC_3D_N28A_2,sizeof(QLC_3D_N28A_2))==0 ) )
        {
            return 28;
        }
    }
    return 0;
}

unsigned char _IsHynix3D(const FH_Paramater* fh)
{
    if ( fh->bMaker==0xAD ) {

	unsigned char HYNIX_3D_2A[] = {0x5A,0x14,0xF3,0x00,0x70};
    if ( memcmp(fh->id,HYNIX_3D_2A,sizeof(HYNIX_3D_2A))==0 )
    {
        return 2;
    }
	
	unsigned char HYNIX_3D_2B[] = {0x5A,0x15,0xF3,0x00,0x70};
    if ( memcmp(fh->id,HYNIX_3D_2B,sizeof(HYNIX_3D_2B))==0 )
    {
        return 2;
    }
	
	unsigned char HYNIX_3D_2C[] = {0x5A,0x16,0xF3,0x00,0x70};
    if ( memcmp(fh->id,HYNIX_3D_2C,sizeof(HYNIX_3D_2C))==0 )
    {
        return 2;
    }
	
    unsigned char HYNIX_3D_3A[] = {0x5C,0x28,0xBA,0x00,0x80};
    if ( memcmp(fh->id,HYNIX_3D_3A,sizeof(HYNIX_3D_3A))==0 )
    {
        return 3;
    }

	unsigned char HYNIX_3D_3B[] = {0x5C,0x29,0xBA,0x00,0x80};
    if ( memcmp(fh->id,HYNIX_3D_3B,sizeof(HYNIX_3D_3B))==0 )
    {
        return 3;
    }
	
	unsigned char HYNIX_3D_3C[] = {0x5C,0x2A,0xBA,0x00,0x80};
    if ( memcmp(fh->id,HYNIX_3D_3C,sizeof(HYNIX_3D_3C))==0 )
    {
        return 3;
    }
       
    unsigned char HYNIX_3D_4A[] = {0x5C,0x28,0x2A,0x01,0x90};
    if ( memcmp(fh->id,HYNIX_3D_4A,sizeof(HYNIX_3D_4A))==0 )
    {
        return 4;
    }
  
    unsigned char HYNIX_3D_4B[] = {0x5C,0x29,0x2A,0x01,0x90};
    if ( memcmp(fh->id,HYNIX_3D_4B,sizeof(HYNIX_3D_4B))==0 )
    {
        return 4;
    }

	unsigned char HYNIX_3D_4C[] = {0x5C,0x2A,0x2A,0x01,0x90};
    if ( memcmp(fh->id,HYNIX_3D_4C,sizeof(HYNIX_3D_4C))==0 )
    {
        return 4;
    }

	unsigned char HYNIX_3D_4D[] = {0x5C,0x2B,0x2A,0x01,0x90};
    if ( memcmp(fh->id,HYNIX_3D_4D,sizeof(HYNIX_3D_4D))==0 )
    {
        return 4;
    }
	
	unsigned char HYNIX_3D_4E[] = {0x5E,0x28,0x22,0x10,0x90};
    if ( memcmp(fh->id,HYNIX_3D_4E,sizeof(HYNIX_3D_4E))==0 )
    {
        return 4;
    }

	unsigned char HYNIX_3D_4F[] = {0x5E,0x29,0x22,0x10,0x90};
    if ( memcmp(fh->id,HYNIX_3D_4F,sizeof(HYNIX_3D_4F))==0 )
    {
        return 4;
    }

	unsigned char HYNIX_3D_4G[] = {0x5E,0x2A,0x22,0x10,0x90};
    if ( memcmp(fh->id,HYNIX_3D_4G,sizeof(HYNIX_3D_4G))==0 )
    {
        return 4;
    }

	unsigned char HYNIX_3D_4H[] = {0x5E,0x2B,0x22,0x10,0x90};
    if ( memcmp(fh->id,HYNIX_3D_4H,sizeof(HYNIX_3D_4H))==0 )
    {
        return 4;
    }
	
    unsigned char HYNIX_3D_5[] = {0x5E,0x28,0x33,0x00,0xA0};
    unsigned char HYNIX_3D_5_DDP[] = {0x5E,0x29,0x33,0x00,0xA0};
    if ( (memcmp(fh->id,HYNIX_3D_5,sizeof(HYNIX_3D_5))==0) || (memcmp(fh->id,HYNIX_3D_5_DDP,sizeof(HYNIX_3D_5_DDP))==0) )
    {
        return 5;
    }

    }
    return 0;
}
unsigned char _IsBiCS3pMLC(const FH_Paramater* fh)
{
	if ( fh->bMaker==0x98) {
		unsigned char pMLC_1DIE[] = {0x35,0x98,0xB3,0x76,0x72};
		unsigned char pMLC_2DIE[] = {0x37,0x99,0xB3,0x7A,0x72};
		if ( memcmp(fh->id,pMLC_1DIE,sizeof(pMLC_1DIE))==0 )
        {
            return 1;
        }
		else if ( memcmp(fh->id,pMLC_2DIE,sizeof(pMLC_2DIE))==0 )
        {
            return 2;
        }
	}
	return 0;
}

unsigned char _IsBiCS3QLC(const FH_Paramater* fh)
{
	if ( fh->bMaker==0x98) {
		unsigned char QLC[] = {0x7C,0x1C,0xB3,0x76,0xF2};
		if ( memcmp(fh->id,QLC,sizeof(QLC))==0 )
        {
            return 1;
        }
	}
	return 0;
}

unsigned char _IsBiCS4QLC(const FH_Paramater* fh)
{
	if ( fh->bMaker==0x98) {
		unsigned char QLC1[] = {0x73,0x9C,0xB3,0x76,0xEB};
		unsigned char QLC2[] = {0x73,0x9C,0xB3,0x76,0xE3};
		if ( memcmp(fh->id,QLC1,sizeof(QLC1))==0 )
        {
            return 1;
        }
		if ( memcmp(fh->id,QLC2,sizeof(QLC2))==0 )
        {
            return 2;
        }
	}
	return 0;
}

unsigned char _IsYMTC3D(const FH_Paramater* fh)
{
	unsigned char YMTC_MLC_3D[] = {0x49,0x01,0x00,0x9B,0x49};
	unsigned char YMTC_TLC_3D[] = {0xC3,0x48,0x25,0x10,0x00};
    if ( fh->bMaker==0x9B) {
		if ( memcmp(fh->id,YMTC_MLC_3D,sizeof(YMTC_MLC_3D))==0 ) {
            return 1;
        }
		else if (memcmp(fh->id,YMTC_TLC_3D,sizeof(YMTC_TLC_3D))==0) {
			return 2;	
		}
    }    
    return 0;    
}
unsigned char _IsMicronB95AGen4(const FH_Paramater* fh)
{
    if ( _IsMicron3D(fh) ) {
        return 0;
    }
    //unsigned char B95A_GEN4[] = {0x84,0x78,0x63,0xA9,0x04};
    if ( fh->bMaker==0x2C ) {
        unsigned char B95A_GEN3[] = {0x84,0x48,0x73,0xA9,0x00};

        if ( memcmp(fh->id,B95A_GEN3,sizeof(B95A_GEN3))==0 )
        {
            return 0;
        }
        return 1;
    }
    return 0;
}

int MyFlashInfo::GetAddressGapBit(FH_Paramater * fh)
{
	if (fh->bIsMIcronN28 || fh->bIsYMTC3D)
		return 2;
	return 0;
}

int MyFlashInfo::GetFlashDefaultInterFace(int voltage, FH_Paramater * fh)
{
	int  default_interface = 0;// 0: legacy 1:T1 2:T2 3:T3

	if (12 == voltage)
	{
		if (fh->bIsMicronB16 || fh->bIsMicronB17 || fh->bIsMicronB27 || fh->bIsMicronB37R || fh->bIsMicronB36R || fh->bIsMicronB47R || fh->bIsMicronQ1428A || fh->bIsMIcronN18 || fh->bIsMIcronN28)
		{
			default_interface = 1; 
		}
		else if (  (0x98 == fh->bMaker) &&  (fh->bIsBics4 || fh->bIsBiCs4QLC  )  )
		{
			default_interface = 1; //Toggle1
		}
		else if ( (fh->bIsMIcronB05 || fh->bIsYMTC3D  )  )
		{
			default_interface = 1; 
		}
	}
	else if (18 == voltage)
	{
		if (0x98 == fh->bMaker)
		{
			if (fh->bIsBiCs4QLC || fh->bIsBics4)
			{
				default_interface = 1; 
			}
		}
	}

	return default_interface;
}

FLH_ABILITY MyFlashInfo::GetFlashSupportInterFaceAndMaxDataRate(int voltage, const FH_Paramater *fh)
{
	int support_status = 0;
	FLH_ABILITY ability;
	memset(&ability, 0, sizeof(FLH_ABILITY));
	if (12 == voltage)
	{
		if (fh->bIsYMTC3D)
		{
			ability.isT3 = true;
			ability.MaxClk_T3 = 800;
		}
		else if (fh->bIsMicronB16 || fh->bIsMicronB17 || fh->bIsMIcronB05 || fh->bIsMicronB36R)
		{
			ability.isT3 = true;
			ability.MaxClk_T3 = 800;
		}
		else if (fh->bIsMicronB27 || fh->bIsMicronB37R || fh->bIsMicronB47R || fh->bIsMIcronN28)
		{
			ability.isT3 = true;
			ability.MaxClk_T3 = 1200;
		}
		else if (fh->bIsMicronB47R)
		{
			ability.isT3 = true;
			ability.MaxClk_T3 = 1600;
		}
		else if (fh->bIsMIcronB05)
		{
			ability.isT3 = true;
			ability.MaxClk_T3 = 667;
		}
		else if (fh->bIsHynix3DV5 || fh->bIsHynix3DV6 )
		{
			ability.isT2 = ability.isT1 = ability.isLegacy = true;
			ability.MaxClk_Legacy = 50;
			ability.MaxClk_T1 = 200;
			ability.MaxClk_T2 = 1200;
		}
		else if (0x45 == fh->bMaker)
		{
			if (fh->bIsBics4)
			{
				ability.isT3 = ability.isT2 = ability.isT1 = ability.isLegacy = true;
				ability.MaxClk_Legacy = 40;
				ability.MaxClk_T1 = 200;
				ability.MaxClk_T2 = 533;
				ability.MaxClk_T3 = 800;
			}
		}
		else if (0x98 == fh->bMaker)
		{
			if (fh->bIsBiCs4QLC)
			{
				ability.isT3 = ability.isT2 = ability.isT1 = true;
				ability.MaxClk_T1 = 200;
				ability.MaxClk_T2 = 533;
				ability.MaxClk_T3 = 667;
			}
			else if (fh->bIsBics4)
			{
				ability.isT3 = ability.isT2 = ability.isT1 = true;
				ability.MaxClk_T1 = 200;
				ability.MaxClk_T2 = 533;
				ability.MaxClk_T3 = 800;
			}
		}
	}
	else if (18 == voltage)
	{
		if (fh->bIsYMTC3D)
		{
			ability.isT2 = ability.isLegacy = true;
			ability.MaxClk_T2 = 800;
			ability.MaxClk_Legacy = 50;
		}
		else if (fh->bIsMicronB16 || fh->bIsMicronB17)
		{
			ability.isT2 = ability.isLegacy = true;
			ability.MaxClk_T2 = 533;
			ability.MaxClk_Legacy = 50;
		}
		else if (fh->bIsMicronB27)
		{
			ability.isT2 = ability.isT1 = ability.isLegacy = true;
			ability.MaxClk_T2 = 533;
			ability.MaxClk_T1 = 200;
			ability.MaxClk_Legacy = 50;
		}
		else if (fh->bIsMIcronN18)
		{
			ability.isT2 = ability.isLegacy = true;
			ability.MaxClk_T2 = 400;
			ability.MaxClk_Legacy = 50;
		}
		else if (fh->bIsMIcronB05)
		{
			ability.isT2 = ability.isT1 = ability.isLegacy = true;
			ability.MaxClk_Legacy = 50;
			ability.MaxClk_T1 = 200;
			ability.MaxClk_T2 = 533;
		}
		else if (fh->bIsHynix3DV4)
		{
			ability.isT2 = ability.isT1 = ability.isLegacy = true;
			ability.MaxClk_Legacy = 50;
			ability.MaxClk_T1 = 200;
			ability.MaxClk_T2= 667;
		}
		else if (fh->bIsHynix3DV5)
		{
			ability.isT2 = ability.isT1 = ability.isLegacy = true;
			ability.MaxClk_Legacy = 50;
			ability.MaxClk_T1 = 200;
			ability.MaxClk_T2 = 800;
		}
		else if (0x45 == fh->bMaker)
		{
			if (fh->bIsBics4 || fh->bIsBics3)
			{
				ability.isT2 = ability.isT1 = ability.isLegacy = true;
				ability.MaxClk_Legacy = 50;
				ability.MaxClk_T1 = 200;
				ability.MaxClk_T2 = 533;
			}
		}
		else if (0x98 == fh->bMaker)
		{
			if (fh->bIsBics2 || fh->bIsBics3)
			{
				ability.isT2 = ability.isT1 = ability.isLegacy = true;
				ability.MaxClk_Legacy = 40;
				ability.MaxClk_T1 = 200;
				ability.MaxClk_T2 = 400;
			}
			else if (fh->bIsBics4 || fh->bIsBiCs4QLC)
			{
				ability.isT2 = ability.isT1 = true;
				ability.MaxClk_T1 = 200;
				ability.MaxClk_T2 = 533;
			}
		}
	}

	return ability;

}

int _GetParameterIdxCnt(const FH_Paramater* fh)
{
	if ( _IsTSB3D(fh) && !fh->beD3 ) {
		return 334;
	}
	if ( fh->bMaker==0x98 || fh->bMaker==0x45 ) {
		if ( !fh->beD3 && fh->Design==0x15 ) {
			return 0x132;
		}
		else if ( fh->Cell==0x10 ) {
			return 0x1FF;
		}
		else if (((fh->id[0] == 0x3C) || (fh->id[0] == 0xDE) || (fh->id[0] == 0x3A)) && ((fh->id[1] == 0xA5) ||(fh->id[1] == 0x94) ||(fh->id[1] == 0xA4)) && (fh->id[2] == 0x93) && ((fh->id[4] == 0x50) || (fh->id[4] == 0xD0))) { //A19 MLC 64/128Gb
			return 320;
		}
		else if (fh->id[4] == 0xEB || fh->id[4] == 0xE3 || fh->id[4] == 0x63 || fh->id[4] == 0x6B) { //BiCS4
			return 300;
		}
	}
	return 256;
}


const static int clock_idx_to_mhz[] = {10,33,40,100,166,200,266,333,400,533,600};
unsigned char MyFlashInfo::GetMicroClockTable(int data_rate, int flh_interface, const FH_Paramater *fh)
{
	char interface_str[4][128] = {"Legacy", "DDR", "DDR2", "DDR3"};

	char flh_str[128] = {0};

	int flash_clock_mhz = 0;
	int flash_clock_idx = -1;

	if (0!=flh_interface)
	{
		flash_clock_mhz = data_rate/2;
	}

	for (int idx=0; idx<11; idx++)
	{
		if (clock_idx_to_mhz[idx] == flash_clock_mhz)
		{
			flash_clock_idx = idx;
			break;
		}
	}
	assert(-1 != flash_clock_idx);

	if (fh->bIsMicronB16)
	{
		sprintf(flh_str,"%s","B16A");
	}
	else if (fh->bIsMicronB17)
	{
		sprintf(flh_str,"B17A");
	}
	else if (fh->bIsMicronB27)
	{
		sprintf(flh_str,"B27A");
	}
	else if (fh->bIsMicronB37R)
	{
		sprintf(flh_str,"B37R");
	}
	else if (fh->bIsMicronB47R)
	{
		sprintf(flh_str,"B47R");
	}
	else if (fh->bIsMicronQ1428A)
	{
		sprintf(flh_str,"Q1428A");
	}
	else if (fh->bIsMicronB36R)
	{
		sprintf(flh_str,"B36R");
	}
	else if (fh->bIsMIcronN18)
	{
		sprintf(flh_str,"N18A");
	}
	else if (fh->bIsMIcronN28)
	{
		sprintf(flh_str,"N28A");
	}
	else if (fh->bIsMIcronB05)
	{
		sprintf(flh_str,"B05A");
	}

	MicronTimingInfo * microInfo = MicronTimingInfo::Instance("table_final.bin");
	int Timing = microInfo->GetParam("PS5013", flh_str, interface_str[flh_interface], flash_clock_idx);
	if (0x255 == Timing)
	{
		assert(0);
	}

	return Timing;

}

#endif
