#include "kdata.h"
#include "comm.h"
#include "dll.h"
#include <windows.h>
#include <stdio.h>

typedef struct _TcpServer TcpServer;
typedef void (*TcpServerReply)(TcpServer*);
typedef void* (*TcpGetRWBuf)();
typedef int (*TcpServerRead)(TcpServer*);
typedef int (*TcpServerWrite)(TcpServer*, int len);
typedef void (*TcpOpenServer)(int serverPort, TcpServerReply reply, int blockMode);
typedef int (*TcpInit)(int size);

struct _Tcp {
	HMODULE module;
	TcpGetRWBuf getRWBuf;
	TcpServerRead read;
	TcpServerWrite write;
} TcpObj;

typedef struct _KLineHead {
    int mCode;
    int mNum;
    KLineItem *mItems;
} KLineHead;

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

#define CMD_LEN 32

void InitTcpServer();

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
    hd.mItems = (KLineItem*) malloc(sizeof (KLineItem) * (len + 10));
    ListAdd(dwnDataList, &hd);
    return dwnDataList->size - 1;
}

static int curIdx;
static KLineHead* curHead;
static CRITICAL_SECTION dwnMutex;

void InitDownload() {
    if (dwnDataList == NULL) {
        dwnDataList = ListNew(5000, sizeof (KLineHead));
        InitializeCriticalSection(&dwnMutex);
    }
}

void UnInitDownload() {
	if (TcpObj.module)
		FreeLibrary(TcpObj.module);
	TcpObj.module = 0;
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
    int code = (int) a[0];
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
        p->mKItem.mVol = (int) a[i];
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
    int id = (int) ids[0];
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

int CopyData(KLineHead *hd, int fromDay, BOOL zb, char *buf) {
	KLineItem *items = hd->mItems;
	int begin = 0;
	for (int i = hd->mNum - 1; i >= 0; --i) {
		int day = items[i].mKItem.mDate;
		if (day == fromDay) {
			begin = i;
			break;
		} else if (day < fromDay) {
			begin = i + 1;
			break;
		}
	}
	
	int num = hd->mNum - begin;
	if (num < 0) num = 0;
	memcpy(buf, &hd->mCode, 4);
    memcpy(buf + 4, &num, 4);
    memcpy(buf + 8, &hd->mNum, 4);
	buf += 12;
	KLineItem *zbbuf = (KLineItem *)buf;
	KItem *kbuf = (KItem*)buf;
	
	for (int i = begin; i < hd->mNum; ++i) {
		if (zb) {
			*zbbuf++ = items[i];
		} else {
			*kbuf++ = items[i].mKItem;
		}
	}
	return 12 + num * (zb ? sizeof(KLineItem) : sizeof(KItem));
}

int DoGetByCodeReply(char *buf) {
    int wlen = 0;
    char *p = buf + CMD_LEN;
    int code = *(int*)p;
    p += 4;
    if (code == 0) goto _end;
    if (dwnDataList == NULL) goto _end;
    int idx = FindByCode(code);
    if (idx < 0) goto _end;
    int fromDay = *(int*)p;
    p += 4;
    int zb = *(int*)p;
    
    KLineHead *hd = (KLineHead*) ListGet(dwnDataList, idx);
    wlen = CopyData(hd, fromDay, zb, buf);

_end:
    return wlen;
}

int DoGetNumReply(char *buf) {
	*(int*)buf = dwnDataList->size;
    return sizeof(int);
}

int DoGetByRangeReply(char *buf) {
    int wlen = 0;
    char *p = buf + CMD_LEN;
    int beginIdx = *(int*)p;
    p += 4;
    int endIdx = *(int*)p;
    p += 4;
    if (dwnDataList == NULL) goto _end;
    if (beginIdx < 0 || endIdx < 0) goto _end;
    int fromDay = *(int*)p;
    p += 4;
    int zb = *(int*)p;
    if (endIdx >= dwnDataList->size) endIdx = dwnDataList->size - 1;
    
    *(int*)buf = endIdx - beginIdx + 1;
    wlen = 4;
    for (int i = beginIdx; i <= endIdx; ++i) {
    	KLineHead *hd = (KLineHead*) ListGet(dwnDataList, i);
    	wlen += CopyData(hd, fromDay, zb, buf + wlen);
	}
	//printf("Range: %d - %d \n", beginIdx, endIdx);

_end:
    return wlen;
}

int DoGetByIdxReply(char *buf) {
    int wlen = 0;
    char *p = buf + CMD_LEN;
    int idx = *(int*)p;
    p += 4;
    if (dwnDataList == NULL) goto _end;
    if (idx < 0) goto _end;
    int fromDay = *(int*)p;
    p += 4;
    int zb = *(int*)p;

    KLineHead *hd = (KLineHead*) ListGet(dwnDataList, idx);
    wlen = CopyData(hd, fromDay, zb, buf);

_end:
    return wlen;
}

int DoPing(char *buf) {
    sprintf(buf, "Ping OK");
    int len = strlen(buf) + 1;
    return len;
}

void TcpServerReply_CALL(TcpServer* svr) {
    char *buf = (char*) TcpObj.getRWBuf();

	// Get-By-Idx/Get-By-Code : {char[32]:cmd, int:code/idx, int:fromDay, int:zb}
	// Get-By-Range : {char[32]:cmd, int:beginIdx, int:endIdx, int:fromDay, int:zb}
    while (1) {
        int len = TcpObj.read(svr);
        if (len <= 0) break;
        buf[len] = 0;
        //printf("    read: [%s] len=%d \n", buf, len);
        len = 0;
        if (strcmp(buf, "Get-By-Code") == 0) {
            len = DoGetByCodeReply(buf);
        } else if (strcmp(buf, "Get-Num") == 0) {
            len = DoGetNumReply(buf);
        } else if (strcmp(buf, "Get-By-Idx") == 0) {
            len = DoGetByIdxReply(buf);
        } else if (strcmp(buf, "Get-By-Range") == 0) {
            len = DoGetByRangeReply( buf);
        }
		int len2 = TcpObj.write(svr, len);
		//printf("    write: len=%d len2=%d \n", len, len2);
        if (len2 <= 0) break;
    }
}

void InitTcpServer() {
	static BOOL inited = FALSE;
    if (inited) {
        return;
    }
	//OpenIO();
    inited = TRUE;
    char tcppath[100];
    sprintf(tcppath, "%sTcp.dll", GetDllPath());
    HMODULE mo = LoadLibrary(tcppath);
    TcpObj.module = mo;
    TcpObj.getRWBuf = (TcpGetRWBuf)GetProcAddress(mo, "TcpGetRWBuf");
    TcpObj.read = (TcpServerRead)GetProcAddress(mo, "TcpServerRead");
    TcpObj.write = (TcpServerWrite)GetProcAddress(mo, "TcpServerWrite");
    TcpInit init = (TcpInit)GetProcAddress(mo, "TcpInit");
    TcpOpenServer openSvr = (TcpOpenServer)GetProcAddress(mo, "TcpOpenServer");
    init(1024 * 1024);
    openSvr(8088, TcpServerReply_CALL, 0);
}

// -----------------------------------------------



