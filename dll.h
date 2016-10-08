#ifndef _DLL_H_
#define _DLL_H_

#if BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */


// DLLIMPORT void HelloWorld (void);

typedef void (*TcpServerReply)();

typedef void* (*GetTcpRWBuf)();
typedef int (*TcpServerRead)();
typedef int (*TcpServerWrite)(int len);
typedef void* (*OpenTcpServerInThread)(int serverPort, TcpServerReply reply);
typedef void (*CloseTcpServer)();

#define GTA_GET_TCP_RWBUF 1
#define GTA_TCP_SERVER_READ 2
#define GTA_TCP_SERVER_WRITE 3
#define GTA_OPEN_TCP_SERVER_IN_THREAD 4
#define GTA_CLOSE_TCP_SERVER 5
void* GetTcpAddr(int id);


#endif /* _DLL_H_ */
