#include "kdata.h"
#include "comm.h"
#include "dll.h"
#include <windows.h>
#include <stdio.h>

void InitTcpServer();

typedef struct _KLineHead {
	int mCode;
	int mNum;
	KLineItem *mItems;
} KLineHead;

static void *TCP_ADDR[20];
static int TCP_ADDR_NUM = 0;
static HMODULE tcpModule;

void* GetTcpAddr(int id) {
	return TCP_ADDR[id];
}

static List *dwnDataList;

#define DWN_ID_BEGIN 100
#define DWN_ID_END 101

#define DWN_ID_CODE_DATE 1
#define DWN_ID_OPEN_CLOSE 2
#define DWN_ID_LOW_HIGH 3
#define DWN_ID_VOL 4
#define DWN_ID_DIF_DEA 5
#define DWN_ID_K_D 6
#define DWN_ID_J  7
#define DWN_ID_BOLL_UP_MID  8
#define DWN_ID_BOLL_LOW  9

int FindByCode(int code) {
	if (dwnDataList == NULL) return -1;
	int sz = dwnDataList->size;
	for (int i = 0; i < sz; ++i) {
		KLineHead *hd = ListGet(dwnDataList, i);
		if (hd->mCode == code)
			return i;
	}
	return -1;
}

int AddKLineHead(int len, int code) {
	KLineHead hd = {code, len, NULL};
	hd.mItems = (KLineItem*)malloc(sizeof(KLineItem) * (len + 10));
	ListAdd(dwnDataList, &hd);
	return dwnDataList->size - 1;
}

static int curIdx;
static KLineHead* curHead;
static CRITICAL_SECTION dwnMutex;

void InitDownload() {
	if (dwnDataList == NULL) {
		dwnDataList = ListNew(4000, sizeof(KLineHead));
		InitializeCriticalSection(&dwnMutex);
	}	
}

static void DoBeginEnd(int id) {
	if (id == DWN_ID_BEGIN) {
		EnterCriticalSection(&dwnMutex);
		InitTcpServer();
	} else {
		LeaveCriticalSection(&dwnMutex);
	}
}

static void DoCodeDate(int len, float* a, float* b) {
	int code = (int)a[0];
	curIdx = FindByCode(code);
	if (curIdx < 0) {
		curIdx = AddKLineHead(len, code);
	}
	curHead = ListGet(dwnDataList, curIdx);
	KLineItem *p = curHead->mItems;
	for (int i = 0; i < len; ++i, ++p) {
		p->mKItem.mDate = b[i] + 19000000;
	}
}

static void DoOpenClose(int len, float* a, float* b) {
	KLineItem *p = curHead->mItems;
	for (int i = 0; i < len; ++i, ++p) {
		p->mKItem.mOpen = a[i];
		p->mKItem.mClose = b[i];
	}
}

static void DoLowHigh(int len, float* a, float* b) {
	KLineItem *p = curHead->mItems;
	for (int i = 0; i < len; ++i, ++p) {
		p->mKItem.mLow = a[i];
		p->mKItem.mHigh = b[i];
	}
}

static void DoVol(int len, float* a, float* b) {
	KLineItem *p = curHead->mItems;
	for (int i = 0; i < len; ++i, ++p) {
		p->mKItem.mVol = (int)a[i];
	}
}

static void DoDifDea(int len, float* a, float* b) {
	KLineItem *p = curHead->mItems;
	for (int i = 0; i < len; ++i, ++p) {
		p->mMacdItem.mDif = a[i];
		p->mMacdItem.mDea = b[i];
	}
}

static void DoKD(int len, float* a, float* b) {
	KLineItem *p = curHead->mItems;
	for (int i = 0; i < len; ++i, ++p) {
		p->mKdjItem.mK = a[i];
		p->mKdjItem.mD = b[i];
	}
}

static void DoJ(int len, float* a, float* b) {
	KLineItem *p = curHead->mItems;
	for (int i = 0; i < len; ++i, ++p) {
		p->mKdjItem.mJ = a[i];
	}
}

static void DoBollUpMid(int len, float* a, float* b) {
	KLineItem *p = curHead->mItems;
	for (int i = 0; i < len; ++i, ++p) {
		p->mBollItem.mUp = a[i];
		p->mBollItem.mMid = b[i];
	}
}

static void DoBollLow(int len, float* a, float* b) {
	KLineItem *p = curHead->mItems;
	for (int i = 0; i < len; ++i, ++p) {
		p->mBollItem.mLow = a[i];
	}
}

void Download_REF(int len, float* out, float* a, float* b, float *ids) {
	int id = (int)ids[0];
	switch (id) {
		case DWN_ID_BEGIN:
		case DWN_ID_END:
			DoBeginEnd(id);
			break;	
		case DWN_ID_CODE_DATE:
			DoCodeDate(len, a, b);
			break;
		case DWN_ID_OPEN_CLOSE:
			DoOpenClose(len, a, b);
			break;
		case DWN_ID_LOW_HIGH:
			DoLowHigh(len, a, b);
			break;
		case DWN_ID_VOL:
			DoVol(len, a, b);
			break;
		case DWN_ID_DIF_DEA:
			DoDifDea(len, a, b);
			break;
		case DWN_ID_K_D:
			DoKD(len, a, b);
			break;
		case DWN_ID_J:
			DoJ(len, a, b);
			break;
		case DWN_ID_BOLL_UP_MID:
			DoBollUpMid(len, a, b);
			break;
		case DWN_ID_BOLL_LOW:
			DoBollLow(len, a, b);
			break;
	}
}

int DoGetReply(int len, char *buf, TcpServerWrite tsw) {
	int wlen = 0;
	char *p = buf + 3;
	while (*p == ' ') ++p;
	int code = atoi(p);
	if (code == 0) goto _end;
	if (dwnDataList == NULL) goto _end;
	int idx = FindByCode(code);
	if (idx < 0) goto _end;
	KLineHead *hd = (KLineHead*)ListGet(dwnDataList, idx);
	wlen = hd->mNum * sizeof(KLineItem);
	memcpy(buf, hd->mItems, wlen);
	
	_end:
	wlen = tsw(wlen);
	return wlen <= 0 ? -1 : wlen;
}

int DoPing(char *buf, TcpServerWrite tsw) {
	sprintf(buf, "Ping OK");
	int len = tsw(strlen(buf));
	return len <= 0 ? -1 : len;
}

void TcpServerReply_CALL() {
	TcpServerRead tsr = (TcpServerRead)GetTcpAddr(GTA_TCP_SERVER_READ);
	TcpServerWrite tsw = (TcpServerWrite)GetTcpAddr(GTA_TCP_SERVER_WRITE);
	GetTcpRWBuf gtb = (GetTcpRWBuf)GetTcpAddr(GTA_GET_TCP_RWBUF);
	char *buf = (char*)gtb();
	
	while (1) {
		int len = tsr();
		if (len <= 0) break;
		buf[len] = 0;
		if (memcmp(buf, "Get", 3) == 0) {
			len = DoGetReply(len, buf, tsw);
			if (len < 0) break;
		} else if (memcmp(buf, "Ping", 4) == 0) {
			len = DoPing(buf, tsw);
			if (len < 0) break;
		} else {
			strcat(buf, "\n[Invaide cmd]");
			len = tsw(strlen(buf));
			if (len <= 0) break;
		}
	}
}

void InitTcpServer() {
	if (TCP_ADDR_NUM != 0) {
		return;
	}
	char tcppath[100];
	sprintf(tcppath, "%sTcp.dll", GetDllPath());
	HMODULE mo = LoadLibrary(tcppath);
	tcpModule = mo;
	
	TCP_ADDR[GTA_GET_TCP_RWBUF] = (void*)GetProcAddress(mo, "GetTcpRWBuf");
	TCP_ADDR[GTA_TCP_SERVER_READ] = (void*)GetProcAddress(mo, "TcpServerRead");
	TCP_ADDR[GTA_TCP_SERVER_WRITE] = (void*)GetProcAddress(mo, "TcpServerWrite");
	TCP_ADDR[GTA_OPEN_TCP_SERVER_IN_THREAD] = (void*)GetProcAddress(mo, "OpenTcpServerInThread");
	TCP_ADDR[GTA_CLOSE_TCP_SERVER] = (void*)GetProcAddress(mo, "CloseTcpServer");
	TCP_ADDR_NUM = 5;
	OpenTcpServerInThread osp = (OpenTcpServerInThread)TCP_ADDR[GTA_OPEN_TCP_SERVER_IN_THREAD];
	osp(8088, TcpServerReply_CALL);
}

// -----------------------------------------------



