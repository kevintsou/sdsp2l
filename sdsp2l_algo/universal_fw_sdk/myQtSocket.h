#ifndef _MYQTSOCKET
#define _MYQTSOCKET

#if defined(_WINDOWS) || defined(WIN32)
	//#include <windows.h>
    #include <winsock.h>

#else	
    #include <sys/socket.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <iostream.h>
    #include <sys/types.h>
    #include <stropts.h>
    #include <sys/filio.h>
#endif

#include <stdio.h>
#include <conio.h>
#include <string.h>

#define QT_SKT_SOCKET_RETRY 10
// so far we only consider the TCP socket, UDP will be added in later release
#define QT_SKT_MAX_RECV_LEN 8096
#define QT_SKT_MAX_MSG_LEN 1024
#define QT_SKT_PORTNUM 1200
#define gt_body_size  7168
typedef struct _SOCKET_MSG_HDR_ { //refer Param.ini setting
	char key[3];// = {'H','D','R'};
	int opcode;
	int datasize;
	char* data;
}SOCKET_MSG_HDR;
#pragma pack()

//#define MSG_HEADER_LEN sizeof(SOCKET_MSG_HDR)

class myQtSocketException
{

public:
    // int: error code, string is the concrete error message
	myQtSocketException(int code=0,const char*msg=0);
	~myQtSocketException();
	/*
	   how to handle the exception is done here, so 
	   far, just write the message to screen and log file
	*/
	int getErrCode();
	const char* getErrMsg();

private:
	int   _errorCode;
	char _errorMsg[MAX_PATH];
};

#ifdef _WIN32
	#define _SOCKET SOCKET
#else
	#define _SOCKET int
#endif

class myQtSocket
{

protected:
	/*
	   only used when the socket is used for client communication
	   once this is done, the next necessary call is setSocketId(int)
	*/
	void setSocketId(int socketFd) { socketId = socketFd; }
    int portNumber;        // Socket port number

	_SOCKET socketId;

    //int socketId;          // Socket file descriptor

    int blocking;          // Blocking flag
    int bindFlag;          // Binding flag

    struct sockaddr_in clientAddr;    // Address of the client that sent data
	int _recvLoop( SOCKET skid,char* buf,int len, int flags );
	void clockSocket();

public:
	myQtSocket(int); 
    myQtSocket(int, char*);                       // given a port number, create a socket
    virtual ~myQtSocket();
	// socket options : ON/OFF

    void setDebug(int);
    void setReuseAddr(int);
    void setKeepAlive(int);
    void setLingerOnOff(bool);
	void setLingerSeconds(int);
    void setSocketBlocking(int);

    // size of the send and receive buffer

    void setSendBufSize(int);
    void setReceiveBufSize(int);

    // retrieve socket option settings
    int  getDebug();
    int  getReuseAddr();
    int  getKeepAlive();
    int  getSendBufSize();
    int  getReceiveBufSize();
    int  getSocketBlocking();
	int  getLingerSeconds();
    bool getLingerOnOff();
	
    // returns the socket file descriptor
    _SOCKET getSocketId();

	// returns the port number
	int getPortNumber();
	
	int sendData(const char *buf, size_t len, int flags=0);
	int receiveData(char *buf, size_t len, int flags=0);
	
	static const char* getIPAddress();	

};

#define OP_CMD_MASK 0xFFFF
#define OP_ECHO 0x1001
#define OP_GETTRIMAP 0x1002
#define OP_SAVEFILE 0x3001
#define OP_RECV_CHECK 0x10000
#define OP_EXIT 0x9002

//return 0 is pass;
typedef int (*CustomReceiveSocketData)(int id, SOCKET_MSG_HDR* hdr,void* socket);
class myQtTcpSocket : public myQtSocket
{
public:
	/* 
	   Constructor. used for creating instances dedicated to client 
	   communication:
	   when accept() is successful, a socketId is generated and returned
	   this socket id is then used to build a new socket using the following
	   constructor, therefore, the next necessary call should be setSocketId()
	   using this newly generated socket fd
	*/
	myQtTcpSocket( int );
	// Constructor.  Used to create a new TCP socket server given a port
	myQtTcpSocket(int portId, char* customIP);
	~myQtTcpSocket();

	/*
	   Sends a message to a connected host. The number of bytes sent is returned
	   can be either server call or client call
	*/
	//int sendMessage(string&);
	int sendMessage( const int opcode, const char *data, int len, int id, CustomReceiveSocketData process );
	int sendFile(const char* filename, const char* data, int len, bool confirm=false );
	int sendEcho();
	int sendExit();

	/*
	   receive messages and stores the message in a buffer
	*/
	//int recieveMessage(string&);
	int recieveMessage(int id, CustomReceiveSocketData process);

	/*
	   Binds the socket to an address and port number
	   a server call
	*/
	void bindSocket();

	/*
	   accepts a connecting client.  The address of the connected client 
	   is stored in the parameter
	   a server call
	*/
	myQtTcpSocket* acceptClient();

	// Listens to connecting clients, a server call
	void listenToClient(int numPorts = 65535 );

	// connect to the server, a client call
	virtual void connectToServer(const char*, int p=-1);
	void* object;
	int isServer;
private:
	struct sockaddr_in serverAddress;
	virtual void reConnectToServer();

};

#endif
        
