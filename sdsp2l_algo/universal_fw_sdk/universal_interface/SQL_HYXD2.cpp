#include <assert.h>
#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <string.h>
#include "NewSQLCmd.h"
////////////////////////////Hynix D2////////////////////////
HYXD2_SQLCmd::HYXD2_SQLCmd(FH_Paramater *fh)
{
	// Parsing flash ID
	_fh = fh;
}

HYXD2_SQLCmd::~HYXD2_SQLCmd()
{
}

int HYXD2_SQLCmd::GetOTP( unsigned char ce, unsigned char die, bool bSLCMode, unsigned char *buf )
{
	int ret=0;
	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	//Erase( ce, block, bSLCMode, planemode, bDFmode, bWaitRDY );
	//ret = Erase( ce, block, true, planemode, false, true );
	assert(0);
	return ret;
}

int HYXD2_SQLCmd::SetRetryValue( unsigned char ce, bool bSLCMode, int len, unsigned char *value, unsigned int block )
{
	assert(0);
	return 0;
}

int HYXD2_SQLCmd::PlaneEraseD1( unsigned char ce, unsigned int block, unsigned char planemode )
{
	int ret;
	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	//Erase( ce, block, bSLCMode, planemode, bDFmode, bWaitRDY, bAddr6 );
	ret = Erase( ce, block, true, planemode, _bDFmode, _bWaitRDY, _bAddr6 );
	return ret;
}

int HYXD2_SQLCmd::PlaneErase( unsigned char ce, unsigned int block, unsigned char planemode )
{
	int ret;
	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	//Erase( ce, block, bSLCMode, planemode, bDFmode, bWaitRDY, bAddr6 );
	ret = Erase( ce, block, false, planemode, _bDFmode, _bWaitRDY, _bAddr6 );
	return ret;
}

int HYXD2_SQLCmd::DumpWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )
{
	int ret = 0;
	unsigned int blk, pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( ((page+pagecnt)>(_fh->MaxPage>>1)) || (!pagecnt)  )
	{
		pagecnt = ((_fh->MaxPage>>1)-page);
	}
	for( pg=0; pg<pagecnt; pg++ )
	{
		if( (bCache) && (pg==(pagecnt-1)) && (pagecnt!=1) )
		{
			bLastPage = true;
		}
		else if( pg==((_fh->MaxPage>>1)-1) )
		{
			bLastPage = true;
		}
		for( blk=0; blk < planemode; blk++ )
		{
			//Write( ce, block, page, bConvs, bSLCMode, buf, len, planemode, ProgStep, bCache, bLastPage, bWaitRDY, bAddr6);
			ret = Write( ce, (block+blk), ((page+pg)*2), false, true, &buf[pg*len], len, planemode, 0, bCache, bLastPage, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}

int HYXD2_SQLCmd::DumpWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, const unsigned char* buf, int len, unsigned char planemode, bool bCache )
{
	int ret = 0;
	unsigned int blk, pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( ((page+pagecnt)>_fh->MaxPage) || (!pagecnt) )
	{
		pagecnt = (_fh->MaxPage-page);
	}
	for( pg = 0; pg < pagecnt; pg++ )
	{
		if( (bCache) && (pg==(pagecnt-1)) && (pagecnt!=1) )
		{
			bLastPage = true;
		}
		else if( pg==(_fh->MaxPage-1) )
		{
			bLastPage = true;
		}
		for( blk=0; blk < planemode; blk++ )
		{
			//Write( ce, block, page, bConvs, bSLCMode, buf, len, planemode, ProgStep, bCache, bLastPage, bWaitRDY, bAddr6 );
			ret = Write( ce, (block+blk), (page+pg), false, false, &buf[pg*len], len, planemode, 0, bCache, bLastPage, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}

int HYXD2_SQLCmd::GroupWriteD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )
{
	int v, i, ret = 0;
	unsigned int blk, pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( page+pagecnt>(_fh->MaxPage>>1) || (!pagecnt) )
	{
		pagecnt = ((_fh->MaxPage>>1)-page);
	}
	if(patternmode==0) 
	{
		for( i=0; i<len; i++  )
		{
		    buf[i] = (rand()&0xFF);
		}
	}
	else if(patternmode==1)
	{
		memset( buf, 0, len );
		v=0;
		for ( i=0; i<len; i+=2 ) 
		{
			buf[i] = (v&0xFF);
			v++;
		}
	}
	else if(patternmode==2) // used select pattern
	{	
		//memset(buf,0,len);
	}
	else if(patternmode==3)
	{
        	memset( buf, 0xff, len );
	}
	else {
		assert(0);
	}

	for( pg=0; pg<pagecnt; pg++ )
	{
		int block_seed = _scsi->GetSelectBlockSeed();
		int ldpc_mode = _scsi->GetSelectLDPC_mode();
		_scsi->Dev_SQL_GroupWR_Setting(block_seed, pg, true, ldpc_mode, 'C');
		if( (bCache) && (pg==(pagecnt-1)) )
		{
			bLastPage = true;
		}
		for( blk=0; blk<planemode; blk++ )
		{
			if( (patternmode==0) || (patternmode==1) ) 
			{
				for( i=0; i<(len-4); i+=256 ) 
				{
					buf[i] = (block&0xff);
					buf[i+1] = ((block>>8)&0xff);
					buf[i+2] = ((page+pg)&0xff);
					buf[i+3] = (((page+pg)>>8)&0xff);
				}
			}
			//Write(ce, block, page, bConvs, bSLCMode, buf, len, planemode, ProgStep, bCache, bLastPage, bWaitRDY, bAddr6);
			if( patternmode==2 )
			{
				ret = Write( ce, (block+blk), ((page+pg)*2), true, true, &buf[pg*len], len, planemode, 0, bCache, bLastPage, true, _bAddr6 );
			}
			else
			{
				ret = Write( ce, (block+blk), ((page+pg)*2), true, true, buf, len, planemode, 0, bCache, bLastPage, true, _bAddr6 );
			}
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}

int HYXD2_SQLCmd::GroupWrite( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char patternmode, unsigned char planemode, bool bCache )
{
	int v, i, ret = 0;
	unsigned int blk,pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( ((page+pagecnt)>_fh->MaxPage) || (!pagecnt) )
	{
		pagecnt = (_fh->MaxPage-page);
	}
	if(patternmode==0) 
	{
		for( i=0; i<len; i++  )
		{
		    buf[i] = (rand()&0xFF);
		}
	}
	else if(patternmode==1)
	{
		memset( buf, 0, len );
		v=0;
		for ( i=0; i<len; i+=2 ) 
		{
			buf[i] = (v&0xFF);
			v++;
		}
	}
    else if(patternmode==2) // used select pattern
	{	
        //memset(buf,0,len);
    }
    else if(patternmode==3)
	{
        memset( buf, 0xff, len );
    }		

	for( pg=0; pg<pagecnt; pg++ )
	{
		int block_seed = _scsi->GetSelectBlockSeed();
		int ldpc_mode = _scsi->GetSelectLDPC_mode();
		_scsi->Dev_SQL_GroupWR_Setting(block_seed, pg, true, ldpc_mode, 'C');
		if( (bCache) && (pg==(pagecnt-1)) )
		{
			bLastPage = true;
		}
		for( blk=0; blk<planemode; blk++ )
		{
			if( (patternmode==0) || (patternmode==1) ) 
			{
				for( i=0; i<(len-4); i+=256 ) 
				{
					buf[i] = (block&0xff);
					buf[i+1] = ((block>>8)&0xff);
					buf[i+2] = ((page+pg)&0xff);
					buf[i+3] = (((page+pg)>>8)&0xff);
				}
			}
			//Write(ce, block, page, bConvs, bSLCMode, buf, len, planemode, ProgStep, bCache, bLastPage, bWaitRDY, bAddr6);
			if( patternmode==2 )
			{
				ret = Write( ce, block+blk, page+pg, true, false, &buf[pg*len], len, planemode, 0, bCache, bLastPage, true, _bAddr6 );
			}
			else
			{
				ret = Write( ce, block+blk, page+pg, true, false, buf, len, planemode, 0, bCache, bLastPage, true, _bAddr6 );
			}
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}

int HYXD2_SQLCmd::DumpReadD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	int ret = 0;
	unsigned int blk, pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( (page+pagecnt)>(_fh->MaxPage>>1) || (!pagecnt) )
	{
		pagecnt = ((_fh->MaxPage>>1)-page);
	}
	
	if( (planemode==1) && (!bCache) )
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, block, ((page+pg)*2), false, true, &buf[pg*len], len, planemode, true, true, false, false, false, false, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	else if( (!bCache) )	// Plane mode != 1
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), ((page+pg)*2), false, true, buf, len, planemode, true, false, false, false, false, false, false, true, _bAddr6 ); // send
				if(ret)
				{
					return ret;
				}
			}
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), ((page+pg)*2), false, true, &buf[((blk*pagecnt)+pg)*len], len, planemode, false, true, false, false, false, false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
		}
	}
	else
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//send command only
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), ((page+pg)*2), false, true, buf, len, planemode, true, false, false, true, (pg==0), false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
			if(pg!=0)
			{
				//read data
				for( blk=0; blk < planemode; blk++ )
				{
					//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
					ret = Read( ce, (block+blk), ((page+(pg-1))*2), false, true, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, false, true, false, false, false, true, _bAddr6 );
					if(ret)
					{
						return ret;
					}
				}
			}
		}
		// for cache last page 
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), ((page+(pagecnt-1))*2), false, true, buf, len, planemode, true, false, false, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
		//read data
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), ((page+(pagecnt-1))*2), false, true, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, false, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}

int HYXD2_SQLCmd::DumpRead( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	int ret = 0;
	unsigned int blk, pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( ((page+pagecnt)>_fh->MaxPage) || (!pagecnt) )
	{
		pagecnt = (_fh->MaxPage-page);
	}
	
	if( (planemode==1) && (!bCache) )
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, block, (page+pg), false, false, &buf[pg*len], len, planemode, true, true, false, false, false, false, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	else if( (!bCache) )	// Plane mode != 1
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), (page+pg), false, false, buf, len, planemode, true, false, false, false, false, false, false, true, _bAddr6 ); // send
				if(ret)
				{
					return ret;
				}
			}
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), (page+pg), false, false, &buf[((blk*pagecnt)+pg)*len], len, planemode, false, true, false, false, false, false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
		}
	}
	else
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//send command only
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), (page+pg), false, false, buf, len, planemode, true, false, false, true, (pg==0), false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
			if(pg!=0)
			{
				//read data
				for( blk=0; blk < planemode; blk++ )
				{
					//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
					ret = Read( ce, (block+blk), (page+(pg-1)), false, false, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, false, true, false, false, false, true, _bAddr6 );
					if(ret)
					{
						return ret;
					}
				}
			}
		}
		// for cache last page 
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), (page+(pagecnt-1)), false, false, buf, len, planemode, true, false, false, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
		//read data
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), (page+(pagecnt-1)), false, false, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, false, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}

int HYXD2_SQLCmd::GroupReadDataD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	int ret = 0;
	unsigned int blk, pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( (page+pagecnt)>(_fh->MaxPage>>1) || (!pagecnt) )
	{
		pagecnt = ((_fh->MaxPage>>1)-page);
	}
	
	if( (planemode==1) && (!bCache) )
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, block, ((page+pg)*2), true, true, &buf[pg*len], len, planemode, true, true, false, false, false, false, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	else if( (!bCache) )	// Plane mode != 1
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), ((page+pg)*2), true, true, buf, len, planemode, true, false, false, false, false, false, false, true, _bAddr6 ); // send 
				if(ret)
				{
					return ret;
				}
			}
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), ((page+pg)*2), true, true, &buf[((blk*pagecnt)+pg)*len], len, planemode, false, true, false, false, false, false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
		}
	}
	else
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//send command only
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), ((page+pg)*2), true, true, buf, len, planemode, true, false, false, true, (pg==0), false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
			if(pg!=0)
			{
				//read data
				for( blk=0; blk < planemode; blk++ )
				{
					//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
					ret = Read( ce, (block+blk), ((page+(pg-1))*2), true, true, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, false, true, false, false, false, true, _bAddr6 );
					if(ret)
					{
						return ret;
					}
				}
			}
		}
		// for cache last page 
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), ((page+(pagecnt-1))*2), true, true, buf, len, planemode, true, false, false, true, false,true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
		//read data
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), ((page+(pagecnt-1))*2), true, true, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, false, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}

int HYXD2_SQLCmd::GroupReadData( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	int ret = 0;
	unsigned int blk, pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( ((page+pagecnt)>_fh->MaxPage) || (!pagecnt) )
	{
		pagecnt = (_fh->MaxPage-page);
	}
	
	if( (planemode==1) && (!bCache) )
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, block, (page+pg), true, false, &buf[pg*len], len, planemode, true, true, false, false, false, false, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	else if( (!bCache) )	// Plane mode != 1
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), (page+pg), true, false, buf, len, planemode, true, false, false, false, false, false, false, true, _bAddr6 ); // send 
				if(ret)
				{
					return ret;
				}
			}
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), (page+pg), true, false, &buf[((blk*pagecnt)+pg)*len], len, planemode, false, true, false, false, false, false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
		}
	}
	else
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//send command only
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), (page+pg), true, false, buf, len, planemode, true, false, false, true, (pg==0), false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
			if(pg!=0)
			{
				//read data
				for( blk=0; blk < planemode; blk++ )
				{
					//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
					ret = Read( ce, (block+blk), (page+(pg-1)), true, false, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, false, true, false, false, false, true, _bAddr6 );
					if(ret)
					{
						return ret;
					}
				}
			}
		}
		// for cache last page 
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), (page+(pagecnt-1)), true, false, buf, len, planemode, true, false, false, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
		//read data
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), (page+(pagecnt-1)), true, false, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, false, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}

int HYXD2_SQLCmd::GroupReadEccD1( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	int ret = 0;
	unsigned int blk, pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( (page+pagecnt)>(_fh->MaxPage>>1) || (!pagecnt)  )
	{
		pagecnt = ((_fh->MaxPage>>1)-page);
	}
	
	if( (planemode==1) && (!bCache) )
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, block, ((page+pg)*2), true, true, &buf[pg*len], len, planemode, true, true, true, false, false, false, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	else if( (!bCache) )	// Plane mode != 1
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), ((page+pg)*2), true, true, buf, len, planemode, true, false, true, false, false, false, false, true, _bAddr6 ); // send 
				if(ret)
				{
					return ret;
				}
			}
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), ((page+pg)*2), true, true, &buf[((blk*pagecnt)+pg)*len], len, planemode, false, true, true, false, false, false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
		}
	}
	else
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//send command only
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), ((page+pg)*2), true, true, buf, len, planemode, true, false, true, true, (pg==0), false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
			if(pg!=0)
			{
				//read data
				for( blk=0; blk < planemode; blk++ )
				{
					//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
					ret = Read( ce, (block+blk), ((page+(pg-1))*2), true, true, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, true, true, false, false, false, true, _bAddr6 );
					if(ret)
					{
						return ret;
					}
				}
			}
		}
		// for cache last page 
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), ((page+(pagecnt-1))*2), true, true, buf, len, planemode, true, false, true, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
		//read data
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), ((page+(pagecnt-1))*2), true, true, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, true, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}

int HYXD2_SQLCmd::GroupReadEcc( unsigned char ce, unsigned int block, unsigned int page, unsigned int pagecnt, unsigned char* buf, int len, unsigned char planemode, bool bCache, bool bRetry )
{
	int ret = 0;
	unsigned int blk, pg;
	bool bLastPage = false;

	_scsi->Log(LOGDEBUG,"%s:\n", __FUNCTION__);
	if( ((page+pagecnt)>_fh->MaxPage) || (!pagecnt) )
	{
		pagecnt = (_fh->MaxPage-page);
	}
	
	if( (planemode==1) && (!bCache) )
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, block, (page+pg), true, false, &buf[pg*len], len, planemode, true, true, true, false, false, false, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	else if( (!bCache) )	// Plane mode != 1
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), (page+pg), true, false, buf, len, planemode, true, false, true, false, false, false, false, true, _bAddr6 ); // send 
				if(ret)
				{
					return ret;
				}
			}
			for( blk=0; blk < planemode; blk++ )
			{
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), (page+pg), true, false, &buf[((blk*pagecnt)+pg)*len], len, planemode, false, true, true, false, false, false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
		}
	}
	else
	{
		for( pg = 0; pg < pagecnt; pg++ )
		{
			for( blk=0; blk < planemode; blk++ )
			{
				//send command only
				//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
				ret = Read( ce, (block+blk), (page+pg), true, false, buf, len, planemode, true, false, true, true, (pg==0), false, false, true, _bAddr6 );
				if(ret)
				{
					return ret;
				}
			}
			if(pg!=0)
			{
				//read data
				for( blk=0; blk < planemode; blk++ )
				{
					//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
					ret = Read( ce, (block+blk), (page+(pg-1)), true, false, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, true, true, false, false, false, true, _bAddr6 );
					if(ret)
					{
						return ret;
					}
				}
			}
		}
		// for cache last page 
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), (page+(pagecnt-1)), true, false, buf, len, planemode, true, false, true, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
		//read data
		for( blk=0; blk < planemode; blk++ )
		{
			//Read( ce, block, page, bConvs, bSLCMode, buf, len, planemode, bReadCmd, bReadData, bReadEcc, bCache, bFirstPage, bLastPage, bCmd36, bWaitRDY, bAddr6);
			ret = Read( ce, (block+blk), (page+(pagecnt-1)), true, false, &buf[((blk*pagecnt)+(pg-1))*len], len, planemode, false, true, true, true, false, true, false, true, _bAddr6 );
			if(ret)
			{
				return ret;
			}
		}
	}
	return ret;
}
