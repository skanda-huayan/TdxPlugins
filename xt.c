#include "comm.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>

#define DIR_UNKNOW 0
#define DIR_UP 1
#define DIR_DOWN 2
#define DIR_LINE 3

typedef struct _XtInfo {
    int beginPos; // 形态开始位置
    int endPos;
    int dir; // 方向 0:未知  1:上升  2:下降  3:横盘
    int beginVal;
    int endVal;
    int minVal;
    int maxVal;
} XtInfo;

#define GET_DIR(b, e) (b == e ? 3 : (b < e ? 1 : 2))

struct _XtClass {
    List *list;
    List *mergedList;
    int days;

    int *mid;
    int midLen;
    int beginPos;
    int minVal;
    int maxVal;
} XtObj;

// 获取形态的震幅 %
static float _GetXtZF(float from, float to) {
    float a = GET_MIN(from, to);
    float b = GET_MAX(from, to);
    return (b - a) * 100 / a;
}

// 是否是很小的形态
static int _IsSmallXt(XtInfo *p) {
    float zf = _GetXtZF(p->beginVal, p->endVal);
    int days = p->endPos - p->beginPos + 1;
    //区间段震幅小于1.5% ；区间段天数小于4天的
    if (zf < 1.5 && days < 4)
        return 1;

    return 0;
}

// 获取下一个大形态；若没有，则返回-1
static int _GetNextLargeXtIndex(List *xtList, int from) {
    for (int i = from; i < xtList->size; ++i) {
        XtInfo *p = (XtInfo*) ListGet(xtList, i);
        if (!_IsSmallXt(p)) return i;
    }
    return -1;
}

static int _TryMergeXt(List *mrgList, List *xtList, int beginIdx) {
    XtInfo* mlx = ListGet(mrgList, mrgList->size - 1);
    XtInfo xt = {0};
    XtInfo* p = (XtInfo*) ListGet(xtList, beginIdx);
    memcpy(&xt, p, sizeof (XtInfo));
    int largeIdx = _GetNextLargeXtIndex(xtList, beginIdx);
    if (largeIdx == -1) largeIdx = xtList->size;

    int i = beginIdx + 1;
    for (; i < largeIdx; ++i) {
        p = (XtInfo*) ListGet(xtList, i);
        if (_GetXtZF(xt.beginVal, p->endVal) > 2) {
            break;
        }
    }
    --i;
    p = (XtInfo*) ListGet(xtList, i);
    xt.endPos = p->endPos;
    xt.endVal = p->endVal;
    if (i != beginIdx)
        xt.dir = 3;
    ListAdd(mrgList, &xt);
    return i;
}

// 合并形态
static void __MeargeXtList(List *xtList, List *xtMergedList) {
	int reset = 1;
    for (int i = 0; i < xtList->size; ++i) {
        XtInfo *p = (XtInfo*) ListGet(xtList, i);
        if (reset) {
        	reset = 0;
            ListAdd(xtMergedList, p);
        } else {
            i = _TryMergeXt(xtMergedList, xtList, i);
            reset = 1;
        }
    }
}

//全并相邻的两个小形态
static void _MeargeSiblingXt(List *list, List *mlist) {
    for (int i = 0; i < list->size; ) {
        XtInfo *p = (XtInfo*) ListGet(list, i);
        XtInfo *p2 = 0;
        if (i + 1 < list->size) p2 = (XtInfo*) ListGet(list, i + 1);
        if (_IsSmallXt(p) && p2 && _IsSmallXt(p2)) {
        	if (p->dir == DIR_UP) {
        		if (p2->dir == DIR_DOWN) {
        			
				}
			}
		} else {
			ListAdd(mlist, p);
		}
    }
}

static void _CalcXt(List *xtList, int* mid, int len) {
    XtInfo xi = {0};
    for (int i = 0; i < len; ++i) {
        if (xi.beginPos == 0) {
            xi.beginPos = i;
            xi.beginVal = mid[i];
            continue;
        }
        if (xi.dir == 0) {
            xi.endPos = i;
            xi.endVal = mid[i];
            xi.dir = GET_DIR(xi.beginVal, xi.endVal);
            xi.minVal = xi.maxVal = mid[i];
            continue;
        }
        int dir = GET_DIR(xi.endVal, mid[i]);
        if (xi.dir == dir) {
            xi.endPos = i;
            xi.endVal = mid[i];
            if (xi.dir == DIR_UP) xi.maxVal = mid[i];
            else if (xi.dir == DIR_DOWN) xi.minVal = mid[i];
        } else {
            ListAdd(xtList, &xi);
            memset(&xi, 0, sizeof (XtInfo));
            i -= 2;
        }
    }
    if (xi.dir != 0) ListAdd(xtList, &xi);
}

void BOOLXT_INIT_REF(int len, float* out, float* days, float* code, float* scode) {
    if (XtObj.list == NULL)
        XtObj.list = ListNew(1500, sizeof (XtInfo));
    if (XtObj.mergedList == NULL)
        XtObj.mergedList = ListNew(1500, sizeof (XtInfo));
    if (XtObj.mid == NULL)
        XtObj.mid = (int*) malloc(sizeof (int) * 1500);
        
    XtObj.midLen = 0;
    XtObj.minVal = 10000000;
	XtObj.maxVal = 0;
    ListClear(XtObj.list);
    ListClear(XtObj.mergedList);
    XtObj.days = (int) days[0];
    printf("BOOLXT_INIT_REF code = %06d len=%d days=%d %p ", (int)code[0], len, XtObj.days, scode);
}

void BOLLXT_REF(int len, float* out, float* mid, float* b, float* c) {
    if (len < 30) return;
    int rlen = GET_MIN(len, XtObj.days);
    XtObj.midLen = 0;
    for (int i = 0, j = len - rlen, k = 0; i < rlen; ++i, ++j) {
        if (mid[j] == 0)
            continue;
        XtObj.mid[k++] = (int) (mid[j] * 100);
        XtObj.minVal = GET_MIN(XtObj.minVal, XtObj.mid[k - 1]);
        XtObj.maxVal = GET_MAX(XtObj.maxVal, XtObj.mid[k - 1]);
        ++XtObj.midLen;
    }
	XtObj.beginPos = len - XtObj.midLen;
    _CalcXt(XtObj.list, XtObj.mid, XtObj.midLen);
    //_MeargeXtList(XtObj.list, XtObj.mergedList);

    for (int i = 0; i < len; ++i) {
        out[i] = XtObj.minVal * 0.9;
    }
    for (int i = 0; i < XtObj.list->size - 1; ++i) {
        XtInfo *s = (XtInfo*) ListGet(XtObj.list, i);
        out[s->beginPos + XtObj.beginPos] = s->beginVal;
    }
}


