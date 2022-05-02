#ifndef _UFWLOGER_H_
#define _UFWLOGER_H_


#include <assert.h>
#include <stdio.h>

#ifndef WIN32
#include <unistd.h>

typedef int HANDLE;
#define INVALID_HANDLE_VALUE -1
#define _NVMe_COMMAND int
typedef unsigned short WORD;
typedef unsigned long DWORD;
#define _open open
#define _read read
#define _close close

#define UFWSleep(a) { printf("Sleep::(%d)\n",a); sleep(a); }

#else

#define UFWSleep(a) { printf("Sleep::(%d)\n",a); Sleep(a); }

#endif

enum LogLevel { LOGBASIC=0,LOGDEBUG,LOGFULL };
class UFWLoger
{
public:
    UFWLoger( const char* logFolder, int drive, bool isphysical );
    // init and create handle + system log pointer
    UFWLoger( FILE *logPtr );
	~UFWLoger();
	//////////////////////////////////////

	const char* LogDir();
	const char* LogFile();
	FILE* LogPtr();
	void LogFlush();
    void SetLogLevel(LogLevel level);
    LogLevel GetLogLevel();
    virtual void Log ( LogLevel level,const char* fmt, ... );
    void LogMEM ( const unsigned char* mem, int size, bool loghdr=true );
    static void LogMEM ( FILE*, const unsigned char* mem, int size );
	//BOOL logDeviceIoControl(HANDLE hDevice,DWORD dwIoControlCode,void * lpInBuffer,DWORD nInBufferSize,void* lpOutBuffer,DWORD nOutBufferSize,LPDWORD lpBytesReturned,LPOVERLAPPED lpOverlapped);
	//BOOL LogDeviceIoControl(HANDLE hDevice);//,DWORD dwIoControlCode,void * lpInBuffer,DWORD nInBufferSize,void* lpOutBuffer,DWORD nOutBufferSize,LPDWORD lpBytesReturned,LPOVERLAPPED lpOverlapped);

protected:
    char _logFolder[512] = {'\0'};
    char _logFile[256] = {'\0'};
    FILE* _logf = NULL;
    LogLevel _logLevel;

};

//pointer for global debug.
extern UFWLoger* _pUFWLoger;
//#ifdef _DEBUG
void _DEBUGPRINT(const char* fmt, ...);
//#else
//#define _DEBUGPRINT(...)
//#endif

#endif /*_UFWLOGER_H_*/