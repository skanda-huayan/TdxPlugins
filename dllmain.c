/* Replace "dll.h" with the name of your header */
#include "dll.h"
#include "comm.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern InitDownload();

static char DLL_PATH[500];
static void *TCP_ADDR[20];
static int TCP_ADDR_NUM = 0;

char *GetDllPath() {
	return DLL_PATH;
}

void* GetTcpAddr(int id) {
	return TCP_ADDR[id];
}

extern void TcpServerReply_CALL();

//DLLIMPORT void HelloWorld () {}

BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
                       DWORD reason        /* Reason this function is being called. */ ,
                       LPVOID reserved     /* Not used. */ )
{
	GetModuleFileName(hInst, DLL_PATH, sizeof(DLL_PATH));
	char *p = strrchr(DLL_PATH, '\\');
	if (p != NULL) {
		p[1] = 0;
	}
	
	if (TCP_ADDR_NUM == 0) {
		HMODULE mo = LoadLibrary("Tcp.dll");
		TCP_ADDR[GTA_GET_TCP_RWBUF] = (void*)GetProcAddress(mo, "GetTcpRWBuf");
		TCP_ADDR[GTA_TCP_SERVER_READ] = (void*)GetProcAddress(mo, "TcpServerRead");
		TCP_ADDR[GTA_TCP_SERVER_WRITE] = (void*)GetProcAddress(mo, "TcpServerWrite");
		TCP_ADDR[GTA_OPEN_TCP_SERVER_IN_THREAD] = (void*)GetProcAddress(mo, "OpenTcpServerInThread");
		TCP_ADDR[GTA_CLOSE_TCP_SERVER] = (void*)GetProcAddress(mo, "CloseTcpServer");
		TCP_ADDR_NUM = 5;
		
		OpenTcpServerInThread osp = (OpenTcpServerInThread)TCP_ADDR[GTA_OPEN_TCP_SERVER_IN_THREAD];
		osp(8088, TcpServerReply_CALL);
		InitDownload();
	}
	
    switch (reason) {
      case DLL_PROCESS_ATTACH:
        break;

      case DLL_PROCESS_DETACH: {
      	CloseTcpServer cts = (CloseTcpServer)TCP_ADDR[GTA_CLOSE_TCP_SERVER];
      	cts();
        break;
      }
      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    /* Returns TRUE on success, FALSE on failure */
    return TRUE;
}
