#include <assert.h>
#include "myQtSocket.h"


#pragma warning(disable : 4996) 

//#define DEBUGALL 1

#ifdef DEBUGALL
#define _SKTEBUG_(...) printf(__VA_ARGS__);
#else
#define _SKTEBUG_(...)
#endif


//const int MSG_HEADER_LEN = 256;
const char _hdrKey[3] =  {'H','D','R'};
inline void _SetHDR (SOCKET_MSG_HDR &hdr)
{
	hdr.key[0]=_hdrKey[0];
	hdr.key[1]=_hdrKey[1];
	hdr.key[2]=_hdrKey[2];
	hdr.data = 0;
}
inline bool _ChkHDR (SOCKET_MSG_HDR &hdr)
{
	if ( hdr.key[0]!=_hdrKey[0] ) return false;
	if ( hdr.key[1]!=_hdrKey[1] ) return false;
	if ( hdr.key[2]!=_hdrKey[2] ) return false;
	if ( hdr.opcode<=0 ) return false;
	return true;
}

myQtSocketException::myQtSocketException(int errCode,const char* errMsg)
{
	_errorCode = errCode;
	if ( 0==errCode ) {
#if defined(_WINDOWS) || defined(WIN32)
		_errorCode = WSAGetLastError();
		FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,   // flags
               NULL,                // lpsource
               _errorCode,                 // message id
               MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),    // languageid
               _errorMsg,              // output buffer
               sizeof (_errorMsg),     // size of msgbuf, bytes
               NULL);               // va_list of arguments		
		
#else
		_errorCode = errno;
		sprintf(_errorMsg,"%s[%d]",strerror(_errorCode),_errorCode);
#endif
	}
	else {
		sprintf(_errorMsg,"%s[%d]",errMsg,_errorCode);
	}
}

myQtSocketException::~myQtSocketException()
{

}

int myQtSocketException::getErrCode()    
{ 
	return _errorCode; 
}

const char* myQtSocketException::getErrMsg() 
{ 
	return _errorMsg; 
}

///////////////////////////////////////////////////
myQtSocket::myQtSocket(int socketId)
{
	this->socketId = socketId;
}

myQtSocket::myQtSocket(int pNumber, char* customIP)
{
    portNumber = pNumber;
    blocking = 1;
	if ( (socketId=socket(AF_INET,SOCK_STREAM,0)) == -1) {
		int errormsg = GetLastError();
		myQtSocketException* openWinSocketException = new myQtSocketException();
		throw openWinSocketException;
	}

    /* 
	   set the initial address of client that shall be communicated with to 
	   any address as long as they are using the same port number. 
	   The clientAddr structure is used in the future for storing the actual
	   address of client applications with which communication is going
	   to start
	*/

	if(customIP!=NULL && customIP[0]!=0){
		LPHOSTENT	lphostent;
		char* strHost = customIP;//strHostName;
		unsigned long	uAddr = inet_addr( strHost );
		if ( (INADDR_NONE == uAddr) && (strcmp( strHost, "255.255.255.255" )) ) {
			// It's not an address, then try to resolve it as a hostname
			if ( lphostent = gethostbyname( strHost ) )
				uAddr = *((unsigned long *) lphostent->h_addr_list[0]);
		}

		clientAddr.sin_addr.s_addr =  ntohl(ntohl( uAddr));
	}
	else{
		clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
    clientAddr.sin_family = AF_INET;
 //   clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = htons(portNumber);
}

void myQtSocket::clockSocket()
{
	if ( -1==socketId ) return;
	#if defined(_WINDOWS) || defined(WIN32)
		closesocket(socketId);
	#else
		close(socketId);
	#endif
	socketId = -1;
}
myQtSocket::~myQtSocket()
{
	clockSocket();
 }

#define HOST_NAME_LENGTH 64
const char* myQtSocket::getIPAddress()
{
	char sName[HOST_NAME_LENGTH+1];
	memset(sName,0,sizeof(sName));
	gethostname(sName,HOST_NAME_LENGTH);
	
	struct hostent *hostPtr;
	hostPtr = gethostbyname(sName);
    struct in_addr *addr_ptr;
	// the first address in the list of host addresses
    addr_ptr = (struct in_addr *)*hostPtr->h_addr_list;
	// changed the address format to the Internet address in standard dot notation
    return inet_ntoa(*addr_ptr);
}
 
int myQtSocket::_recvLoop(SOCKET skid,char* buf,int len, int flags ) 
{
	int gotBytes=0, leftBytes=len;
	while ( gotBytes<len ) {
		int got = recv( skid, buf, leftBytes<QT_SKT_MAX_RECV_LEN? leftBytes:QT_SKT_MAX_RECV_LEN, flags );
		if ( got>0 ) {
			buf += got;
			leftBytes -= got;
			gotBytes+=got;
		}
		else {//if ( got==-1 ) {
			myQtSocketException* exception = new myQtSocketException();
			throw exception;
		}
		// else {
			// return -1;
		// }
	}
	return gotBytes;
}
    
void myQtSocket::setDebug(int debugToggle)
{
	if ( setsockopt(socketId,SOL_SOCKET,SO_DEBUG,(char *)&debugToggle,sizeof(debugToggle)) == -1 )  {
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
}

void myQtSocket::setReuseAddr(int reuseToggle)
{
	if ( setsockopt(socketId,SOL_SOCKET,SO_REUSEADDR,(char *)&reuseToggle,sizeof(reuseToggle)) == -1 ) {
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
} 

void myQtSocket::setKeepAlive(int aliveToggle)
{
	if ( setsockopt(socketId,SOL_SOCKET,SO_KEEPALIVE,(char *)&aliveToggle,sizeof(aliveToggle)) == -1 ) {
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
} 

void myQtSocket::setLingerSeconds(int seconds)
{
	struct linger lingerOption;
	
	if ( seconds > 0 )
	{
		lingerOption.l_linger = seconds;
		lingerOption.l_onoff = 1;
	}
	else lingerOption.l_onoff = 0;
	 
	if ( setsockopt(socketId,SOL_SOCKET,SO_LINGER,(char *)&lingerOption,sizeof(struct linger)) == -1 ) {
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
}

void myQtSocket::setLingerOnOff(bool lingerOn)
{
	struct linger lingerOption;

	if ( lingerOn ) lingerOption.l_onoff = 1;
	else lingerOption.l_onoff = 0;
	
	if ( setsockopt(socketId,SOL_SOCKET,SO_LINGER,(char *)&lingerOption,sizeof(struct linger)) == -1 ){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
}

void myQtSocket::setSendBufSize(int sendBufSize)
{
	if ( setsockopt(socketId,SOL_SOCKET,SO_SNDBUF,(char *)&sendBufSize,sizeof(sendBufSize)) == -1 ){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
} 

void myQtSocket::setReceiveBufSize(int receiveBufSize)
{
	if ( setsockopt(socketId,SOL_SOCKET,SO_RCVBUF,(char *)&receiveBufSize,sizeof(receiveBufSize)) == -1 ){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
}

void myQtSocket::setSocketBlocking(int blockingToggle)
{
    if (blockingToggle)
    {
        if (getSocketBlocking()) return;
        else blocking = 1;
	}
	else
	{
		if (!getSocketBlocking()) return;
		else blocking = 0;
	}

	#ifdef UNIX
		if (ioctl(socketId,FIONBIO,(char *)&blocking) == -1){
			myQtSocketException* exception = new myQtSocketException();
			throw exception;
		}
	#else
		if (ioctlsocket(socketId,FIONBIO,(unsigned long *)blocking) == -1){
			myQtSocketException* exception = new myQtSocketException();
			throw exception;
		}
	#endif
}

int myQtSocket::getDebug()
{
    int myOption;
    int myOptionLen = sizeof(myOption);
	
	if ( getsockopt(socketId,SOL_SOCKET,SO_DEBUG,(char *)&myOption,&myOptionLen) == -1 ){
			myQtSocketException* exception = new myQtSocketException();
			throw exception;
		}
    return myOption;
}

int myQtSocket::getReuseAddr()
{
    int myOption;        
    int myOptionLen = sizeof(myOption);
	if ( getsockopt(socketId,SOL_SOCKET,SO_REUSEADDR,(char *)&myOption,&myOptionLen) == -1 ){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}

	return myOption;
}

int myQtSocket::getKeepAlive()
{
    int myOption;        
    int myOptionLen = sizeof(myOption);

	if ( getsockopt(socketId,SOL_SOCKET,SO_KEEPALIVE,(char *)&myOption,&myOptionLen) == -1 ){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}

    return myOption;    
}

int myQtSocket::getLingerSeconds()
{
	struct linger lingerOption;
	int myOptionLen = sizeof(struct linger);
	
	if ( getsockopt(socketId,SOL_SOCKET,SO_LINGER,(char *)&lingerOption,&myOptionLen) == -1 ){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}

	return lingerOption.l_linger;
}

bool myQtSocket::getLingerOnOff()
{
	struct linger lingerOption;
	int myOptionLen = sizeof(struct linger);

	if ( getsockopt(socketId,SOL_SOCKET,SO_LINGER,(char *)&lingerOption,&myOptionLen) == -1 ){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}

	if ( lingerOption.l_onoff == 1 ) return true;
	else return false;
}

int myQtSocket::getSendBufSize()
{
    int sendBuf;
    int myOptionLen = sizeof(sendBuf);

	if ( getsockopt(socketId,SOL_SOCKET,SO_SNDBUF,(char *)&sendBuf,&myOptionLen) == -1 ){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}

    return sendBuf;
}    

int myQtSocket::getReceiveBufSize()
{
    int rcvBuf;
    int myOptionLen = sizeof(rcvBuf);
	
	if ( getsockopt(socketId,SOL_SOCKET,SO_RCVBUF,(char *)&rcvBuf,&myOptionLen) == -1 ){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}

    return rcvBuf;
}

int myQtSocket::getSocketBlocking()
{
	return blocking;
}

_SOCKET myQtSocket::getSocketId() 
{ 
	return socketId; 
}

int myQtSocket::getPortNumber() 
{ 
	return portNumber; 
}


int myQtSocket::sendData(const char *buf, size_t len, int flags)
{
    return send(socketId ,buf,len,flags);
}

int myQtSocket::receiveData(char *buf, size_t len, int flags)
{
	return _recvLoop( socketId,buf,len, flags ); 
}

///////////////////////////////////
myQtTcpSocket::myQtTcpSocket( int socketId) 
: myQtSocket(socketId)
{
	isServer = 0;
}

myQtTcpSocket::myQtTcpSocket(int portId, char* customIP) 
: myQtSocket(portId, customIP)
{
	isServer = 0;
}

myQtTcpSocket::~myQtTcpSocket()
{

}

void myQtTcpSocket::bindSocket()
{
	if (bind(socketId,(struct sockaddr *)&clientAddr,sizeof(struct sockaddr_in))==-1){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
}

void myQtTcpSocket::reConnectToServer()
{
    // Connect to the given address
	if (connect(socketId,(struct sockaddr *)&serverAddress,sizeof(serverAddress)) == -1){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
}

void myQtTcpSocket::connectToServer(const char* serverNameOrAddr, int customPort)
{ 
	if ( customPort!=-1 ) {
		portNumber = customPort;
	}
	/* 
	   when this method is called, a client socket has been built already,
	   so we have the socketId and portNumber ready.

       a myHostInfo instance is created, no matter how the server's name is 
	   given (such as www.yuchen.net) or the server's address is given (such
	   as 169.56.32.35), we can use this myHostInfo instance to get the 
	   IP address of the server
	*/

	//myHostInfo serverInfo(serverNameOrAddr,hType);
	struct hostent *hostPtr;    // Entry within the host address database
	hostPtr = gethostbyname(serverNameOrAddr);
	struct in_addr *addr_ptr;
	// the first address in the list of host addresses
	addr_ptr = (struct in_addr *)*hostPtr->h_addr_list;
	// changed the address format to the Internet address in standard dot notation
    // Store the IP address and socket port number	
	//struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(inet_ntoa(*addr_ptr));
    serverAddress.sin_port = htons(portNumber);
	reConnectToServer();
}

myQtTcpSocket* myQtTcpSocket::acceptClient()
{
	int newSocket;   // the new socket file descriptor returned by the accept systme call

    // the length of the client's address
    int clientAddressLen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddress;    // Address of the client that sent data

    // Accepts a new client connection and stores its socket file descriptor
	if ((newSocket = (int)accept(socketId, (struct sockaddr *)&clientAddress,&clientAddressLen)) == -1){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}

    // Create and return the new myQtTcpSocket object
    myQtTcpSocket* retSocket = new myQtTcpSocket(newSocket);
	retSocket->isServer = 1;
    return retSocket;
}

void myQtTcpSocket::listenToClient(int totalNumPorts)
{
	if (listen(socketId,totalNumPorts) == -1){
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
}

int myQtTcpSocket::sendEcho()
{
	SOCKET_MSG_HDR hdr;
	_SetHDR(hdr);
	hdr.opcode = OP_ECHO;
	int numBytes=0;
	if ( (numBytes = send(socketId,(char*)&hdr,sizeof(SOCKET_MSG_HDR),0)) == -1) {
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}

	char done[4];
	done[0]=0;
	_recvLoop(socketId,done,3,0);
	if ( strncmp(done,"end",3 )!=0 ) {
		myQtSocketException* exception = new myQtSocketException(1,"send HDR fail");
		throw exception;
	}
	return 0;
}

int myQtTcpSocket::sendExit()
{
	SOCKET_MSG_HDR hdr;
	_SetHDR(hdr);
	hdr.opcode = OP_EXIT;
	int numBytes=0;
	if ( (numBytes = send(socketId,(char*)&hdr,sizeof(SOCKET_MSG_HDR),0)) == -1) {
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}

	char done[4];
	done[0]=0;
	_recvLoop(socketId,done,3,0);
	if ( strncmp(done,"end",3 )!=0 ) {
		myQtSocketException* exception = new myQtSocketException(1,"send HDR fail");
		throw exception;
	}
	return 0;
}
	
int myQtTcpSocket::sendFile(const char* filename, const char* data, int len, bool confirm )
{
	assert(len>0);
	int numBytes;  // the number of bytes sent
	int retry=0;
	for( ; retry<QT_SKT_SOCKET_RETRY; retry++ ) {
		try {
			SOCKET_MSG_HDR hdr;
			_SetHDR(hdr);
			hdr.opcode = OP_SAVEFILE;
			if ( confirm ) {
				hdr.opcode |= OP_RECV_CHECK;
			}
			hdr.datasize = len+MAX_PATH;
			if ( (numBytes = send(socketId,(char*)&hdr,sizeof(SOCKET_MSG_HDR),0)) == -1) {
				myQtSocketException* exception = new myQtSocketException();
				throw exception;
			}

			char done[4];
			done[0]=0;
			_recvLoop(socketId,done,3,0);
			if ( strncmp(done,"end",3 )!=0 ) {
				myQtSocketException* exception = new myQtSocketException(1,"send HDR fail");
				throw exception;
			}			

			_SKTEBUG_("%s(%d, %d)%d\n",__FUNCTION__,hdr.opcode,len,numBytes);
			{
				if ( (numBytes = send(socketId,filename,MAX_PATH,0)) == -1){
					myQtSocketException* exception = new myQtSocketException();
					throw exception;
				}

				if ( numBytes!=MAX_PATH ) {
					myQtSocketException* exception = new myQtSocketException(2,"send FOLDER fail");
					throw exception;
				}
			}
			{
				if ( (numBytes = send(socketId,data,len,0)) == -1){
					myQtSocketException* exception = new myQtSocketException();
					throw exception;
				}

				if ( numBytes!=len ) {
					myQtSocketException* exception = new myQtSocketException(2,"send DATA fail");
					throw exception;
				}
			}

			if ( confirm ) {
				_recvLoop(socketId,done,3,0);
				if ( strncmp(done,"end",3 )!=0 ) {
					myQtSocketException* exception = new myQtSocketException(1,"send HDR fail");
					throw exception;
				}
			}

			goto end;
		}
		catch( myQtSocketException* excp)  {
			delete excp;
			if ( !isServer ) {
				clockSocket();
				if ( (socketId=socket(AF_INET,SOCK_STREAM,0)) == -1) {
					myQtSocketException* openWinSocketException = new myQtSocketException();
					throw openWinSocketException;
				}
				reConnectToServer();
				::Sleep ( 200 );
			}
			continue;
		}
	}
end:
	return numBytes;
}

/*
	cmdinfo[256];
	opcode !=0;
*/
int myQtTcpSocket::sendMessage( const int opcode, const char *data, int len, int id, CustomReceiveSocketData process )
{
	int numBytes;  // the number of bytes sent
	int retry=0;
	for( ; retry<QT_SKT_SOCKET_RETRY; retry++ ) {
		try {
			SOCKET_MSG_HDR hdr;
			_SetHDR(hdr);
			hdr.opcode = opcode;
			hdr.datasize = len;
			if ( (numBytes = send(socketId,(char*)&hdr,sizeof(SOCKET_MSG_HDR),0)) == -1) {
				myQtSocketException* exception = new myQtSocketException();
				throw exception;
			}

			char done[4];
			done[0]=0;
			_recvLoop(socketId,done,3,0);
			if ( strncmp(done,"end",3 )!=0 ) {
				myQtSocketException* exception = new myQtSocketException(1,"send HDR fail");
				throw exception;
			}

			_SKTEBUG_("%s(%d, %d)%d\n",__FUNCTION__,opcode,len,numBytes);

			if ( len>0 ) {
				if ( (numBytes = send(socketId,data,len,0)) == -1){
					myQtSocketException* exception = new myQtSocketException();
					throw exception;
				}

				if ( numBytes!=len ) {
					myQtSocketException* exception = new myQtSocketException(2,"send DATA fail");
					throw exception;
				}
			}
			
			if ( process ) {
				recieveMessage(id, process);
				//myQtSocketException* exception = new myQtSocketException(3,"do Process fail");
				//throw exception;
			}

			goto end;
		}
		catch( myQtSocketException* excp)  {
			delete excp;
			if ( !isServer ) {
				clockSocket();
				if ( (socketId=socket(AF_INET,SOCK_STREAM,0)) == -1) {
					myQtSocketException* openWinSocketException = new myQtSocketException();
					throw openWinSocketException;
				}
				reConnectToServer();
				::Sleep ( 200 );
			}
			continue;
		}
	}
end:
	return numBytes;
}

int myQtTcpSocket::recieveMessage(int id, CustomReceiveSocketData process)
{
	int numBytes;  // The number of bytes recieved
	// retrieve the length of the message received

	SOCKET_MSG_HDR hdr;
	numBytes = _recvLoop(socketId,(char*)&hdr,sizeof(SOCKET_MSG_HDR),0);
	if ( numBytes!=sizeof(SOCKET_MSG_HDR) ) {
		myQtSocketException* exception = new myQtSocketException(1,"get HDR fail");
		throw exception;
	}
	if ( !_ChkHDR(hdr) ) {
		myQtSocketException* exception = new myQtSocketException(1,"format HDR fail");
		throw exception;
	}
	char done[4];
	memcpy(done,"end",3);
	if (send(socketId,"end",3,0) == -1) {
		myQtSocketException* exception = new myQtSocketException();
		throw exception;
	}
	
	if ( hdr.opcode==OP_EXIT ) {
		return -1;
	}
	else if ( hdr.opcode==OP_ECHO ) {
		return 0;
	}
		
	numBytes = hdr.datasize;
	if ( hdr.datasize>0 ) {
		hdr.data = new char[hdr.datasize];
		numBytes = _recvLoop(socketId,hdr.data,hdr.datasize,0);
		if ( numBytes!=hdr.datasize ) {
			myQtSocketException* exception = new myQtSocketException(2,"get DATA fail");
			delete[] hdr.data;
			hdr.data=0;
			throw exception;
		}
	}

	int opcode = hdr.opcode;
	hdr.opcode = hdr.opcode&OP_CMD_MASK;
	if ( process && (process(id, &hdr, this)!=0) ) {
		myQtSocketException* exception = new myQtSocketException(3,"do Process fail");
		if ( hdr.datasize>0 ) {
			delete[] hdr.data;
			hdr.data=0;
		}
		throw exception;
	}

	hdr.opcode = opcode;
	if ( OP_RECV_CHECK==(hdr.opcode&OP_RECV_CHECK) ) {
		if (send(socketId,"end",3,0) == -1) {
			myQtSocketException* exception = new myQtSocketException();
			throw exception;
		}
	}

	if ( hdr.datasize>0 ) {
		delete[] hdr.data;
	}

	return numBytes;
}


