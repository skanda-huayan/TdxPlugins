#include "F.h"
#include "comm.h"
#include "kdata.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#include <windows.h>

static char buf[300];
#define BUF (buf+strlen(buf))

typedef struct _KMInfo {
    int dateLen;
    int code;
} KMInfo;

CRITICAL_SECTION commMutex;

KMInfo di = {0};

void BeginLock_REF(int len, float* out, float* a, float* b, float* c) {
    EnterCriticalSection(&commMutex);
}

void EndLock_REF(int len, float* out, float* a, float* b, float* c) {
    LeaveCriticalSection(&commMutex);
}

void Reset_REF(int len, float* out, float* rLen, float* code, float* tmp) {
    memset(buf, 0, sizeof buf);
    di.dateLen = (int) rLen[0];
    di.code = (int) code[0];
}

// 近回idx属于[from, to]时， a 上穿 b 的idx
// 若未上穿时，返回-1
int GetCrossAB(float *a, float *b, int from, int to) {
	for (int i = from; i < to; ++i) {
		if (a[i] < b[i] && a[i + 1] >= b[i + 1]) {
			return i + 1;
		}
	}
	return -1;
}

// MACD 底背离
void CrossInfo_REF(int len, float* out, float* k, float* m, float* c) {
    static int cp[100]; // 交叉点位置
    static int cpLow[100];
    static float price[100]; // 最低价

    int num = 0;
    for (int i = len - di.dateLen; i < len; ++i) {
        while (k[i] >= m[i] && i < len) ++i;
        if (i >= len) break;
        while (k[i] < m[i] && i < len) ++i;
        if (i >= len) break;
        cp[num++] = i; // len - 1 - i;
    }

    for (int i = 0; i < num; ++i) {
        float min = 10000;
        int j = cp[i];
        for (; j >= 0; --j) {
            float d = k[j] - m[j];
            if (min >= d) min = d;
            else break;
        }
        price[i] = c[j];
        cpLow[i] = j;

        for (int vx = j + 1; vx >= j - 3; --vx) {
            if (price[i] > c[vx]) price[i] = c[vx];
        }
    }

    int val = 0;
    for (int i = num - 1; i > 0; --i) {
        int last = i;
        int last2 = i - 1;
        int vv = (k[cpLow[last]] >= k[cpLow[last2]]) && (price[last] < price[last2]);
        val = val || vv;
    }

    for (int i = 0; i < len; ++i)
        out[i] = (float) val;

    /*
    sprintf(BUF, "[%06d] CP:%d", di.code, num);
    for (int i = 0; i < num; ++i) {
            sprintf(BUF, " %.2f %.2f |", k[cpLow[i]], price[i]);
    }
    sprintf(BUF, " *%d", val);
    WriteLn();
     */
}

// 计算最大跌幅  close = [..., ref(cose,1), ref(cose,0)]
void CalcMaxDF_REF(int len, float* pfOUT, float* days, float* close, float* code) {
    int dayLen = days[0];
    float maxDay = 0, maxVal = 0, minDay = 0, minVal = 1000000;

    if (dayLen > len) dayLen = len;
    for (int i = 0, begin = len - dayLen; i < dayLen; ++i) {
        if (close[i + begin] > maxVal) {
            maxVal = close[i + begin];
            maxDay = i + begin;
        }
    }

    minVal = maxVal;
    for (int i = maxDay; i < len; ++i) {
        if (close[i] < minVal) {
            minVal = close[i];
            minDay = i;
        }
    }

    pfOUT[len - 1] = (maxVal - minVal) * 100 / maxVal;
}

// 计算最大涨幅 close = [..., ref(cose,1), ref(cose,0)]
void CalcMaxZF_REF(int len, float* pfOUT, float* days, float* close, float* code) {
    int dayLen = days[0];
    float maxDay = 0, maxVal = 0, minDay = 0, minVal = 1000000;

    if (dayLen > len) dayLen = len;
    for (int i = 0, begin = len - dayLen; i < dayLen; ++i) {
        if (close[i + begin] > maxVal) {
            maxVal = close[i + begin];
            maxDay = i + begin;
        }
    }

    minVal = maxVal;
    for (int i = len - dayLen; i < maxDay; ++i) {
        if (close[i] < minVal) {
            minVal = close[i];
            minDay = i;
        }
    }

    pfOUT[len - 1] = (maxVal - minVal) * 100 / minVal;
}

//------------------------------------------------------------------------------
// 在指定的日期上， 是第几个交易日（以最近交易日为零开始算）
// Eg: 如果当前日期是2016.8.19， 那么8.19就返回0； 8.18返回1
// out = [日数] ; 假设date=[..., 2016.8.19] 而days=2016.8.20则 返回0
void CalcTradeDayInfo_REF(int len, float* out, float* days, float* date, float* code) {
    int d = (int) days[0] + 20000000;
    int rb = 0;
    int last = (int) date[len - 1] + 19000000;
    if (d > last) {
        out[len - 1] = 0;
        return;
    }

    for (int i = len - 1, j = 0; i >= 0; --i, ++j) {
        int cd = (int) date[i] + 19000000;
        if (d == cd) {
            rb = j;
            break;
        }
        if (cd < d) {
            rb = j - 1;
            break;
        }
    }
    out[len - 1] = rb;
}

//------------------------------------------------------------------------------
// 指定的日期是否是交易日
void IsTradDay_REF(int len, float* out, float* days, float* b, float* c) {
    int d = (int) days[0];
    int v = IsTradeDay(d / 10000, d / 100 % 100, d % 100);
    out[len - 1] = v;
}

//------------------------------------------------------------------------------
//是否是停牌股 out = [1:是  0：不是]
void IsTP_REF(int len, float* out, float* date, float* vol, float* c) {
    int last = GetLastTradeDay();
    int day0 = (int) date[len - 1] + 19000000;
    int day1 = (int) date[len - 2] + 19000000;

    if (last == day0) {
        out[len - 1] = (vol[len - 1] == 0);
    } else if (last == day1) {
        out[len - 1] = (vol[len - 2] == 0);
    } else {
        out[len - 1] = 1;
    }
}

//------------------------------------------------------------------------------
// 最近N日内复牌股停止交易日数 out = [日数]
void IsNFP_REF(int len, float* out, float* date, float* n, float* vol) {
    int nd = (int) n[0];
    if (len < nd + 1) {
        out[len - 1] = 0;
        return;
    }

    IsTP_REF(len, out, date, vol, NULL);
    if (out[len - 1] == 1) { //停牌股
        out[len - 1] = 0;
        return;
    }

    int last = GetLastTradeDay();
    int firstDay = (int) date[len - nd - 1] + 19000000;
    int lastDay = (int) date[len - 1] + 19000000;
    int days = GetTradeDayBetween(firstDay, lastDay);
    int re = days - nd - 1;
    out[len - 1] = re;
}

//------------------------------------------------------------------------------
//  .401 排序信息
typedef struct _SortInfo {
    int code; //股票代码
    float val;
    // 排序相关
    int idx;
    int prev;
    int next;
} SortInfo;

#define SI_LIST_NUM 2

struct _SI {
    List *list[SI_LIST_NUM]; // List<SortInfo>  = [区间数据list, ...]
    int firstIdx[SI_LIST_NUM]; // [区间数据开始idx , ...]
    DWORD lastTime;
    CRITICAL_SECTION mutex;
} si;

static void SetSortInfo(int len, int code, float val, int* firstIdx, List *list) {
    SortInfo info = {0};
    info.code = code;
    info.val = val;
    info.idx = list->size;
    info.prev = info.next = -1;

    // begin sort
    if (*firstIdx == -1) {
        *firstIdx = 0;
        ListAdd(list, &info);
    } else {
        int findIdx = -1;
        int before = -1; //向前插入?
        for (int i = *firstIdx; i >= 0 && i < list->size;) {
            SortInfo* t = (SortInfo*) ListGet(list, i);
            if (info.val > t->val) {
                findIdx = i;
                before = 1;
                break;
            }
            if (t->next == -1) {
                findIdx = i;
                before = 0;
                break;
            }
            i = t->next;
        }
        if (before == 1) {
            SortInfo* t = (SortInfo*) ListGet(list, findIdx);
            info.prev = t->prev;
            info.next = t->idx;
            if (t->prev != -1) {
                SortInfo* p = (SortInfo*) ListGet(list, t->prev);
                p->next = info.idx;
            } else {
                *firstIdx = info.idx;
            }
            t->prev = info.idx;
            ListAdd(list, &info);
        } else if (before == 0) {
            SortInfo* t = (SortInfo*) ListGet(list, findIdx);
            info.prev = t->idx;
            info.next = -1;
            t->next = info.idx;
            ListAdd(list, &info);
        } else {
            printf("SetSortInfo Error: before = -1  code = %d val=%f", info.code, info.val);
        }
    }
}

// id = [0:区间数据, 1:布林开口]
void SetSortInfo_REF(int len, float* out, float* code, float* val, float* id) {
    EnterCriticalSection(&si.mutex);
    if (si.list[0] == NULL) {
        for (int i = 0; i < SI_LIST_NUM; ++i) {
            si.list[i] = ListNew(5000, sizeof (SortInfo));
        }
    }
    DWORD diff = GetTickCount() - si.lastTime;
    if (diff > 1000) { // 大于1000毫秒，表示重新开始的排序
        for (int i = 0; i < SI_LIST_NUM; ++i) {
            ListClear(si.list[i]);
            si.firstIdx[i] = -1;
        }
    }

    int _id = (int) id[0];
    SetSortInfo(len, (int) code[0], val[len - 1], &(si.firstIdx[_id]), si.list[_id]);

    si.lastTime = GetTickCount();
    LeaveCriticalSection(&si.mutex);
}

void GetSortInfo_REF(int len, float* out, float* code, float* id, float* c) {
    int cc = (int) code[0];
    out[len - 1] = 0;
    int _id = (int) id[0];

    for (int i = si.firstIdx[_id], j = 0; i >= 0 && si.list[_id] != NULL && i < si.list[_id]->size; ++j) {
        SortInfo* t = (SortInfo*) ListGet(si.list[_id], i);
        if (t->code == cc) {
            out[len - 1] = j + 1;
            break;
        }
        i = t->next;
    }
}

//------------------------------------------------------------------------------
struct _BOLLSK {
    float um[30]; // up - mid
    float ml[30]; // mid - low
    int num;
    CRITICAL_SECTION mutex;
} bollSK;

// BOLL线收口 out=[收口大小]
void BOLLSK_REF(int len, float* out, float* up, float* mid, float* low) {
    EnterCriticalSection(&bollSK.mutex);
    bollSK.num = GET_MIN(len, 30);
    for (int i = 0, j = len - bollSK.num; i < bollSK.num; ++i, ++j) {
        bollSK.um[i] = up[j] - mid[j];
        bollSK.ml[i] = mid[j] - low[j];
    }


    LeaveCriticalSection(&bollSK.mutex);
}

//------------------------------------------------------------------------------
// 是否是回踩Boll线中轨
static int IsStepBackBollMid(int len, float* close, float* mid, float* low) {
    int c1 = fabs(low[len - 1] - mid[len - 1]) / mid[len - 1] <= 0.03;
    int c2 = close[len - 1] >= mid[len - 1] && (close[len - 1] - mid[len - 1]) / mid[len - 1] <= 0.03;
    // 近6日下跌超过 8%
    float max6 = GET_MAX6(close[len - 1], close[len - 2], close[len - 3], close[len - 4], close[len - 5], close[len - 6]);
    float min2 = GET_MIN(close[len - 1], close[len - 2]);
    int c3 = (max6 - min2) / min2 >= 0.08;
    // 近3日下跌超过 5%
    float max3 = GET_MAX3(close[len - 1], close[len - 2], close[len - 3]);
    float min2l = GET_MIN(low[len - 1], low[len - 2]);
    int c4 = (max3 - min2l) / min2l >= 0.05;
    // 近4日内的收盘价大于mid 或小于mid的3%
    float min4 = GET_MIN4(close[len - 1], close[len - 2], close[len - 3], close[len - 4]);
    float minMid4 = GET_MIN4(mid[len - 1], mid[len - 2], mid[len - 3], mid[len - 4]);
    int c5 = min4 >= minMid4 || (minMid4 - min4) / min4 <= 0.03;

    return c1 && c2 && (c3 || c4) && c5;
}

void IsStepBackBollMid_REF(int len, float* out, float* close, float* mid, float* low) {
    if (len < 20) {
        out[len - 1] = 0;
        return;
    }
    // 最近3天内是否存在回踩mid线
    out[len - 1] = IsStepBackBollMid(len, close, mid, low) ||
            IsStepBackBollMid(len - 1, close, mid, low) ||
            IsStepBackBollMid(len - 2, close, mid, low);
}

//--------------------金叉共振----------------------------------------------------------
struct _GZInfoClass {
	int days; // 参数：最近N日
	int code; //代码
	
	int gzDays[10];
	int gzDaysNum;
    CRITICAL_SECTION mutex;
} GZInfoObj;

void CrossGZInit_REF(int len, float* out, float* a, float* b, float* id) {
	if ((int)id[0] == 0) {
		EnterCriticalSection(&GZInfoObj.mutex);
		GZInfoObj.days = (int)a[0];
		GZInfoObj.code = (int) b[0];
		memset(&GZInfoObj.gzDays, 0, sizeof(GZInfoObj.gzDays));
		GZInfoObj.gzDaysNum = 0;
	} else if ((int)id[0] == 1) {
		if (len < 30 || len <= GZInfoObj.days) return;
		GZInfoObj.gzDays[GZInfoObj.gzDaysNum] = GetCrossAB(a, b, len - GZInfoObj.days - 1, len - 1);
		++GZInfoObj.gzDaysNum;
	}
}

void CrossGZ_REF(int len, float* out, float* a, float* b, float* c) {
	int gz[20] = {0};
	int gzNum = 0;
	for (int i = 0; i < GZInfoObj.gzDaysNum; ++i) {
		if (GZInfoObj.gzDays[i] <= 0) {
			continue;
		}
		for (int j = 0; j <= gzNum; j += 2) {
			if (j == gzNum) {
				gz[gzNum] = GZInfoObj.gzDays[i];
				gz[gzNum + 1] = 1;
				gzNum += 2;
				break;
			}
			if (GZInfoObj.gzDays[i] == gz[j]) {
				++gz[j + 1];
				break;
			}
		}
	}
	
	int maxv = 0;
	for (int j = 0; j < gzNum; j += 2) {
		if (maxv < gz[j + 1]) {
			maxv = gz[j + 1];
		}
	}
	out[len - 1] = (float)maxv;
	LeaveCriticalSection(&GZInfoObj.mutex);
}

struct {
    int witchDay; //哪一天 0,1,...  0:表示当日  1:昨日 ,...
    float close[10];
    float low[10];
    float high[10];
    float mid[10];
    float up[10];
    float ma5[10];
    CRITICAL_SECTION mutex;
} start;

#define C(x) start.close[len-1-(x)]
#define H(x) start.high[len-1-(x)]
#define L(x) start.low[len-1-(x)]
#define M(x) start.mid[len-1-(x)]
#define A(X) start.mid[len-1-(x)]

int GPStart(int len) {
    int x = 1;
    //3日的mid呈上升形态
    int c1 = M(0) >= M(1) && M(1) >= M(2);
    x = x && c1;
    //3日的MA5呈上升形态
    int c2 = A(0) >= A(1) && A(1) >= A(2);
    x = x && c2;
    // 3日收盘价在mid之上
    int c3 = (C(0) >= M(0) && C(1) >= M(1) && C(2) >= M(2));
    x = x && c3;
    // 3日的最低价距离mid不超过10%
    int c4 = fabs(L(0) - M(0)) / M(0) <= 0.1 && fabs(L(1) - M(1)) / M(1) <= 0.1 && fabs(L(2) - M(2)) / M(2) <= 0.1;
    x = x && c4;
    //当日最低价下探2%以上， 当日收盘价小于昨是收盘价
    int c5 = (C(1) - L(0)) / C(1) >= 0.02 && (C(0) < C(1));
    x = x && c5;

    if (C(1) < C(2)) { //当日和昨日都是下跌的
        int cx4 = (GET_MAX(C(2), H(1)) - L(0)) / C(2) >= 0.05; //这两日下跌超过5%
        x = x && cx4;
    } else { //昨日是上涨的
        if (C(2) > C(3)) { //前日也是上涨的
            // 昨日或前日必须有一日振幅超过5% 且涨幅超过3%
            int cx5 = (H(1) - C(2)) / C(2) >= 0.05 && (C(1) - C(2)) / C(2) >= 0.03;
            int cx6 = (H(2) - C(3)) / C(3) >= 0.05 && (C(2) - C(3)) / C(3) >= 0.03;
            x = x && (cx5 || cx6);
        } else {//前日是下跌的
            int cx5 = (H(1) - C(2)) / C(2) >= 0.05 && (C(1) - C(2)) / C(2) >= 0.03;
            x = x && cx5;
        }
    }
    return x;
}
#undef C 
#undef H
#undef M
#undef L
#undef A

void GPStartSetParam_REF(int len, float* out, float* close, float* low, float* high) {
    EnterCriticalSection(&start.mutex);
    for (int i = 0; i < 10; ++i) {
        start.close[9 - i] = close[len - 1 - i];
        start.low[9 - i] = low[len - 1 - i];
        start.high[9 - i] = high[len - 1 - i];
    }
}

void GPStartSetParam2_REF(int len, float* out, float* witchDay, float* b, float* c) {
    start.witchDay = (int) witchDay[0];
}

// 【形态选股】之股票启动  out = [0 or 1]
void GPStart_REF(int len, float* out, float* mid, float* up, float *ma5) {
    if (len < 15) {
        out[len - 1] = 0;
        goto _end;
    }
    for (int i = 0; i < 10; ++i) {
        start.mid[9 - i] = mid[len - 1 - i];
        start.up[9 - i] = up[len - 1 - i];
        start.ma5[9 - i] = ma5[len - 1 - i];
    }
    //近两日内存在？
    out[len - 1] = GPStart(10 - start.witchDay); // || GPStart(9);

_end:
    LeaveCriticalSection(&start.mutex);
}

//------------------------------------------------------------------------------
struct _Graphics {
	HWND topWnd;
	HWND kWnd;
} GraphicsObj;
// 在K线图上fill一个rect
void FillRect_REF(int len, float* out, float* leftTop, float* widthHeight, float *color) {
	if (GraphicsObj.topWnd == 0) {
		GraphicsObj.topWnd = FindWindow("TdxW_MainFrame_Class", NULL);
		HWND mdiClient = GetDlgItem(GraphicsObj.topWnd, 0xE900);
		HWND ff01 = GetDlgItem(mdiClient, 0xFF01);
		GraphicsObj.kWnd = GetDlgItem(ff01, 0xE900);
	}
	if (GraphicsObj.kWnd == 0)
		return;
	HDC dc = GetDC(GraphicsObj.kWnd);
	int lt = (int)leftTop[0];
	int wh = (int)widthHeight[0];
	RECT r = {lt/1000, lt % 1000, wh/1000, wh%1000};
	HBRUSH brsh = CreateSolidBrush((int)color[0]);
	FillRect(dc, &r, brsh);
	DeleteObject(brsh);
	ReleaseDC(GraphicsObj.kWnd, dc);
}

//-----------------------------------------------------------------------------
// 资金
struct _ZJ {
	void (*InitZJParam)(int id, int val);
	void (*InitZJParamDate)(float* days, int len);
	void (*CalcZJ)(float *out, int len);
	void (*CalcZJAbs)(float *out, int len);
	void (*GetZJMax)(float *out, int len);
	void (*GetZJSum)(float *out, int len, int days, int zjType);
	void (*GetThsPM)(int code, int *pm, int *num);
	void (*GetLastZJ)(float *out, int len, int code, int dayNum);
	void (*CalcHgtZJ)(float *out, int len);
	void (*CalcHgtZJAbs)(float *out, int len);
	
	void* (*FetchEastMoneyZJ)(int code);
	void (*CalcEastMoneyZJ)(void *task, float *out, float *days, int len);
	void (*CalcEastMoneyZJAbs)(void *task, float *out, float *days, int len);
	int load;
	DWORD tlsIdx;
	HMODULE dll;
} ZJObj;

static void LoadTdxZJModule() {
	if (ZJObj.load) return;
	ZJObj.load = 1;
	char buf[200];
	sprintf(buf, "%s\\%s", GetDllPath(), "TdxZJ.dll");
	HMODULE m = LoadLibraryEx(buf, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (m == 0) return;
	ZJObj.dll = m;
	ZJObj.InitZJParam = GetProcAddress(m, "InitZJParam");
	ZJObj.InitZJParamDate = GetProcAddress(m, "InitZJParamDate");
	ZJObj.CalcZJ = GetProcAddress(m, "CalcZJ");
	ZJObj.CalcZJAbs = GetProcAddress(m, "CalcZJAbs");
	ZJObj.GetZJMax = GetProcAddress(m, "GetZJMax");
	ZJObj.GetZJSum = GetProcAddress(m, "GetZJSum");
	ZJObj.GetThsPM = GetProcAddress(m, "GetThsPM");
	ZJObj.GetLastZJ = GetProcAddress(m, "GetLastZJ");
	ZJObj.CalcHgtZJ = GetProcAddress(m, "CalcHgtZJ");
	ZJObj.CalcHgtZJAbs = GetProcAddress(m, "CalcHgtZJAbs");
	ZJObj.FetchEastMoneyZJ = GetProcAddress(m, "FetchEastMoneyZJ");
	ZJObj.CalcEastMoneyZJ = GetProcAddress(m, "CalcEastMoneyZJ");
	ZJObj.CalcEastMoneyZJAbs = GetProcAddress(m, "CalcEastMoneyZJAbs");
	ZJObj.tlsIdx = TlsAlloc();
}

void UnloadTdxZJModule() {
	if (ZJObj.dll) FreeLibrary(ZJObj.dll);
}

void TdxZJ_REF(int len, float* out, float* ids, float* vals, float *c) {
	LoadTdxZJModule();
	if (ZJObj.InitZJParam == 0) return;
	int id = (int)ids[0];
	if (id <= 6) {
		int v = (int)vals[0];
		ZJObj.InitZJParam(id, (int)vals[0]);
	} else if (id == 9) {
		ZJObj.InitZJParamDate(vals, len);
	} else if (id == 10) {
		ZJObj.CalcZJ(out, len);
	} else if (id == 11) {
		ZJObj.CalcZJAbs(out, len);
	} else if (id == 12) {
		ZJObj.GetZJMax(out, len);
	} else if (id == 13) {
		ZJObj.GetZJSum(out, len, (int)vals[0], (int)c[0]);
	} else if (id == 20) {
		ZJObj.CalcHgtZJ(out, len);
	} else if (id == 21) {
		ZJObj.CalcHgtZJAbs(out, len);
	}
}

void EastMoneyZJ_REF(int len, float* out, float* ids, float* vals, float *c) {
	LoadTdxZJModule();
	if (ZJObj.FetchEastMoneyZJ == 0) return;
	int id = (int)ids[0];
	if (id == 0) {
		void *d = ZJObj.FetchEastMoneyZJ((int)vals[0]);
		TlsSetValue(ZJObj.tlsIdx, d);
	} else if (id == 1) {
		void *d = TlsGetValue(ZJObj.tlsIdx);
		ZJObj.CalcEastMoneyZJ(d, out, vals, len);
	} else if (id == 2) {
		void *d = TlsGetValue(ZJObj.tlsIdx);
		ZJObj.CalcEastMoneyZJAbs(d, out, vals, len);
	}
}

// 同花顺行业排名
void THS_PM_REF(int len, float* out, float* code, float* b, float* c) {
    int cc = (int) code[0];
    int pm = 0, num = 0;
    LoadTdxZJModule();
	if (ZJObj.GetThsPM != 0) {
		ZJObj.GetThsPM(cc, &pm, &num);
	}
    out[len-1] = pm;
    out[len-2] = num;
}

void STRING_REF(int len, float* out, float* code, float* b, float* c) {
	//OpenIO();
	printf("STRING_REF: %x %f %d \n", code, code[len-1], (int)code[0]);
	for (int i = 0; i < len; ++i) out[i] = code[i];
}

static BOOL _UpBBI(float bbi, float open, float close) {
	if (bbi < 0.001 || open < 0.001 || close < 0.001)
		return FALSE;
	BOOL v = (bbi - close) * 100 / bbi <= 0.5;
	if (close >= bbi) return TRUE;
	if (open > bbi /*&& v*/) return TRUE;
	return FALSE;
}

//在BBI线之上的天数
void UpBBI_REF(int len, float* out, float* bbi, float* open, float* close) {
	//OpenIO();
	if (len < 250 * 3) {
		// 小于三年的股票都不要
		out[len - 1] = 0;
	}
	int n = 0;
	for (int i = len - 1; i > 0; --i, ++n) {
		//连续两天在BBI下
		if (!_UpBBI(bbi[i], open[i], close[i]) && !_UpBBI(bbi[i - 1], open[i - 1], close[i - 1]))
			break; 
	}
	out[len - 1] = n;
}

//DEA DIF 上0轴的天数
void UpMACD_REF(int len, float* out, float* dif, float* dea, float* c) {
	//OpenIO();
	int n = 0;
	for (int i = len - 1; i > 0; --i, ++n) {
		if (dif[i] < 0 || dea[i] < 0) break;
	}
	out[len - 1] = n;
}

// 最近几日的流入资金
void GetLastZJ_REF(int len, float* out, float* code, float* days, float* c) {
	LoadTdxZJModule();
	ZJObj.GetLastZJ(out, len, (int)code[0], (int)days[0]);
}

// MACD 0轴线上的第一次金叉 的日期  out = [num, day1, day2, ...]
void GetMACDFirstCrossDay_REF(int len, float* out, float* dea, float* dif, float* date) {
	int first = len;
	for (int i = len - 1; i >= 0; --i) {
		if (dea[i] < 0 || dif < 0) {
			first = i;
			break;
		}
	}
	int k = 0;
	for (int i = first; i < len - 1; ++i) {
		out [i] = 0;
		if (dif[i] < dea[i] && dif[i + 1] >= dea[i + 1]) {
			out[ len - 2 - k] = ((int)date[i + 1]) % 10000 ;
			++k;
			++i;
		}
	}
	out[len - 1] = k;
}

void Download_Code_REF(int len, float* out, float* fcode, float* in2, float *ids) {
	static FILE *f = NULL;
	if (f == NULL) {
		f = fopen("D:\\CPP\\GP\\gp.txt", "wb");
	}
	int cc = (int)fcode[0];
	fwrite(&cc, sizeof(int), 1, f);
}

//------------------------------------------------------------------------------
extern void Download_REF(int len, float* out, float* in1, float* in2, float *ids);
extern void BOLLXT_REF(int len, float* out, float* mid, float* close, float* dwn);
extern void BOOLXT_INIT_REF(int len, float* out, float* days, float* a, float* b);
//------------------------------------------------------------------------------
PluginTCalcFuncInfo g_CalcFuncSets[] = {
	{1, (pPluginFUNC) & BeginLock_REF},
	{2, (pPluginFUNC) & EndLock_REF},
    {10, (pPluginFUNC) & Reset_REF},
    {11, (pPluginFUNC) & CrossInfo_REF},

    {20, (pPluginFUNC) & CalcMaxDF_REF},
    {21, (pPluginFUNC) & CalcMaxZF_REF},

    //{30, (pPluginFUNC) & BOLLXT_REF},
    //{31, (pPluginFUNC) & BOOLXT_INIT_REF},
    //{33, (pPluginFUNC) & BOLLXT_3_test}, // test
    //{34, (pPluginFUNC) & BOLLXT_mrg_test}, // test

    {40, (pPluginFUNC) & SetSortInfo_REF},
    {41, (pPluginFUNC) & GetSortInfo_REF},

    {50, (pPluginFUNC) & BOLLSK_REF},

    {60, (pPluginFUNC) & IsStepBackBollMid_REF},

    {70, (pPluginFUNC) & GPStart_REF},
    {71, (pPluginFUNC) & GPStartSetParam_REF},
    {72, (pPluginFUNC) & GPStartSetParam2_REF},
    
    {80, (pPluginFUNC) & CrossGZ_REF},
    {81, (pPluginFUNC) & CrossGZInit_REF},

    {100, (pPluginFUNC) & CalcTradeDayInfo_REF},
    {101, (pPluginFUNC) & IsTradDay_REF},
    {102, (pPluginFUNC) & IsTP_REF},
    {103, (pPluginFUNC) & IsNFP_REF},

	{110, (pPluginFUNC) & FillRect_REF},
	
	{120, (pPluginFUNC) & TdxZJ_REF},
    {121, (pPluginFUNC) & THS_PM_REF},
    {122, (pPluginFUNC) & STRING_REF},
    {123, (pPluginFUNC) & EastMoneyZJ_REF},
    
    {130, (pPluginFUNC) & UpBBI_REF},
    {131, (pPluginFUNC) & UpMACD_REF},
    {132, (pPluginFUNC) & GetLastZJ_REF},
    
    {140, (pPluginFUNC) & GetMACDFirstCrossDay_REF},
	
    {200, (pPluginFUNC) & Download_REF},
    
    {210, (pPluginFUNC) & Download_Code_REF},

    {0, NULL},
};

DLLIMPORT BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun) {
    if (*pFun == NULL) {
        (*pFun) = g_CalcFuncSets;
        InitHolidays();
        InitializeCriticalSection(&si.mutex);
        InitializeCriticalSection(&bollSK.mutex);
        InitializeCriticalSection(&start.mutex);
        InitializeCriticalSection(&GZInfoObj.mutex);
        InitializeCriticalSection(&commMutex);
    	//InitDownload();
        return TRUE;
    }

    return FALSE;
}
