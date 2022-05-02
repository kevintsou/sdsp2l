
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "UFWLoger.h"
#ifndef WIN32
#include <sys/stat.h>
#else
#include <io.h>
#include <direct.h>
#define sprintf sprintf_s
#define strcat strcat_s
#endif


UFWLoger* _pUFWLoger=0;
void _DEBUGPRINT(const char* fmt, ...) 
{ 
#if _DEBUG
	char buf[2048] = {0};
	va_list arglist;
	va_start ( arglist, fmt );
#ifndef WIN32
    vsnprintf ( buf, sizeof(buf), fmt, arglist );
#else
	_vsnprintf_s ( buf, sizeof(buf), fmt, arglist );
#endif
	va_end ( arglist );

	if ( _pUFWLoger ) {
		_pUFWLoger->Log ( LOGFULL,"%s", buf );
	}
	else {
		printf ("%s", buf); 
	}
#endif
}

//////////////////////////////////////////////////////////////////
UFWLoger::UFWLoger( const char* logFolder, int drive, bool isphysical )
{
    _logf = 0;
    _logLevel = LogLevel::LOGFULL;
    _logFile[0]='\0';
	
   // struct tm* ltime;
   // time_t rawtime;
   // time ( &rawtime );
   // ltime = localtime ( &rawtime );

	time_t time_seconds = time(0);
	struct tm now_time;
	localtime_s(&now_time, &time_seconds);


    if ( logFolder && strlen(logFolder)>0 ) {
        char file[128];
        if ( !isphysical ) {
			sprintf ( file,"%d-%02d-%02d-%02d-%02d-%02d_%C",
				now_time.tm_year+1900, now_time.tm_mon+1, now_time.tm_mday,
				now_time.tm_hour, now_time.tm_min, now_time.tm_sec,drive );
        }
        else {
			sprintf ( file,"%d-%02d-%02d-%02d-%02d-%02d_%d",
				now_time.tm_year+1900, now_time.tm_mon+1, now_time.tm_mday,
				now_time.tm_hour, now_time.tm_min, now_time.tm_sec,drive );
        }

		sprintf(_logFolder,"%s/%s",logFolder,file);
        errno_t err;

#ifndef WIN32
        mkdir(_logFolder,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
		err = _mkdir(_logFolder);
#endif
        sprintf(_logFile,"%s/%s.log",_logFolder,file);
		
        err = fopen_s(&_logf, _logFile,"w" );
    }
}

// init and create handle + system log pointer
UFWLoger::UFWLoger(FILE *logPtr )
{
    _logLevel = LOGFULL;
    _logFile[0] = '\0';
	
    _logf = logPtr;
}
//
//bool UFWLoger::logDeviceIoControl(HANDLE hDevice,DWORD dwIoControlCode,void * lpInBuffer,DWORD nInBufferSize,void* lpOutBuffer,DWORD nOutBufferSize,LPDWORD lpBytesReturned,LPOVERLAPPED lpOverlapped){
//	return true;
//}
//


UFWLoger::~UFWLoger()
{
    if ( strlen(_logFile)>0 ) {
		assert(_logf);
		if (_logf) {
			fflush(_logf);
			fclose(_logf);
		}
    }
}


const char* UFWLoger::LogDir()
{
    return _logFolder;
}

const char* UFWLoger::LogFile()
{
    return _logFile;
}

void UFWLoger::LogFlush()
{
	if ( _logf ) {
		fflush(_logf);
	}
}

FILE* UFWLoger::LogPtr()
{
    return _logf;
}

void UFWLoger::SetLogLevel(LogLevel level)
{
    _logLevel = level;
}

LogLevel UFWLoger::GetLogLevel()
{
    return _logLevel;
}

void UFWLoger::Log ( LogLevel level,const char* fmt, ... )
{
	char buf[2048] = {0};
    va_list arglist;
    va_start ( arglist, fmt );
#ifndef WIN32
    _vsnprintf ( buf, sizeof(buf), fmt, arglist );
#else
    vsnprintf ( buf, sizeof(buf), fmt, arglist );
#endif
    va_end ( arglist );

    if (level<=_logLevel ) {
        if ( _logf ) {
			if (  buf[strlen(buf)-1]!='\n'   )
				buf[strlen(buf)] = '\n';
            fprintf ( _logf, "%s", buf );
        }
        printf ( "%s", buf );
		LogFlush();
    }
}
void UFWLoger::LogMEM ( const unsigned char* mem, int size, bool loghdr )
{
    if ( loghdr ) {
        Log (LOGDEBUG, "-------------------------%d--------------------------\n", size );
        Log (LOGDEBUG, "----- 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n" );
        Log (LOGDEBUG, "-----------------------------------------------------\n" );
    }

    int row = 0;
    char line[1024],buf[20];
    sprintf ( line,"" );
    for ( int i=0; i<size; i++ )  {
        if ( i==0 || ( i%16 ) ==0 ) {
            sprintf ( buf,"%04X0 ", row );
            strcat ( line,buf );
            row++;
        }
        sprintf ( buf,"%02X ", mem[i] );
        strcat ( line,buf );
        if ( ( i%16 ) ==15 ) {
            strcat ( line,"\n" );
            Log (LOGDEBUG,  line );
            sprintf ( line,"" );
            //Log("\n");
        }
    }
    strcat ( line,"\n" );
    Log (LOGDEBUG, line );
    if ( loghdr ) {
        Log (LOGDEBUG, "----------------------------------------------------\n" );
    }    
}
void UFWLoger::LogMEM ( FILE* op, const unsigned char* mem, int size )
{
    fprintf ( op, "-------------------------%d--------------------------\n", size );
    fprintf ( op, "----- 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n" );
    fprintf ( op, "-----------------------------------------------------\n" );
    int row = 0;
    char line[1024],buf[20];
    sprintf ( line,"" );
    for ( int i=0; i<size; i++ )  {
        if ( i==0 || ( i%16 ) ==0 ) {
            sprintf ( buf,"%04X0 ", row );
            strcat ( line,buf );
            row++;
        }
        sprintf ( buf,"%02X ", mem[i] );
        strcat ( line,buf );
        if ( ( i%16 ) ==15 ) {
            strcat ( line,"\n" );
            fprintf ( op, line );
            sprintf ( line,"" );
        }
    }
    strcat ( line,"\n" );
    fprintf ( op, line );
}
