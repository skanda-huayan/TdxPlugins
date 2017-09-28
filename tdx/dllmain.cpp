/* Replace "dll.h" with the name of your header */
#include "dll.h"
#include "eastmoney.h"
#include <windows.h>
#include <stdio.h>
#include "mysql.h"
#define _MAX(a, b) (a > b ? a : b)
#define _ABS(a) (a > 0 ? a : -a)

Mysql db;
Statement *stmt, *stmt2;
char thsPmName[32];

void OpenIO();

enum ID_PARAM {
	IP_CODE, IP_CDD, IP_DD, IP_ZD, IP_XD, IP_JME, IP_CJJE, IP_NUM
};
struct ZJXX {
	int day;
	int cdd, dd, zd, xd;
};

struct HGT {
	int day;
	int jme, cjje;
};

int params[IP_NUM];
int days[250];
int daysLen;
ZJXX zjxx[250];
int zjxxLen;
int zjMax;
HGT hgt[500];
int hgtLen;

BOOL needQuery;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved) {
	switch(fdwReason) {
		case DLL_PROCESS_ATTACH:
			//OpenIO();
			break;
		case DLL_PROCESS_DETACH:
			//FreeConsole();
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

void InitMysql() {
	if (stmt != 0) return;
	db.connect("tdx_f10");
	stmt = db.prepare("select _day,_cdd, _dd, _zd, _xd from _zjxx where _code = ?");
	stmt2 = db.prepare("select _day, _jme, _cjje from _hgt where _code = ?");
	if (stmt) stmt->setBindCapacity(48, 128);
	if (stmt2) stmt2->setBindCapacity(48, 256);
	
	ResultSet *rs = db.query("show tables");
	int n = 0;
	while (rs && rs->next()) {
		char *s = rs->getString(0);
		if (memcmp(s, "_ths_pm", 7) == 0) {
			s += 7;
			int xn = atoi(s);
			if (n < xn) n = xn;
		}
	}
	sprintf(thsPmName, "_ths_pm%d", n);
}

void InitZJParam(int id, int val) {
	if (id == IP_CODE) {
		needQuery = val != params[id];
		if (needQuery) zjMax = 0;
	}
	params[id] = val;
	//printf("InitZJParam id=%d  val=%d \n", id, val);
}

void InitZJParamDate(float* ds, int len) {
	int DL = sizeof(days)/sizeof(int);
	int ml = len < DL ? len : DL;
	for (int i = 0; i < ml; ++i) {
		days[i] = (int)ds[len - ml + i] + 19000000;
	}
	daysLen = ml;
	//printf("InitZJParamDate date=[%d - %d] len:%d \n", (int)ds[0], (int)ds[len-1], len);
	//printf("     days=[%d - %d] len:%d \n", days[0], days[daysLen-1], daysLen);
}

void QueryResult() {
	InitMysql();
	//printf("QueryResult IN need:%d stmt=%p \n", needQuery, stmt);
	if (! needQuery) return;
	zjxxLen = 0;
	hgtLen = 0;
	if (stmt == 0 || stmt2 == 0) return;
	int MAX_DL = sizeof(days)/sizeof(int);
	char code[8];
	sprintf(code, "%06d", params[IP_CODE]);
	memset(zjxx, 0, sizeof zjxx);
	memset(hgt, 0, sizeof hgt);
	
	stmt->reset();
	stmt->setStringParam(0, code);
	stmt->bindParams();
	stmt->setResult(0, Statement::CT_INT);
	stmt->setResult(1, Statement::CT_INT);
	stmt->setResult(2, Statement::CT_INT);
	stmt->setResult(3, Statement::CT_INT);
	stmt->setResult(4, Statement::CT_INT);
	stmt->bindResult();
	stmt->exec();
	stmt->storeResult();
	int rc = stmt->getRowsCount();
	for (int i = 0; i < rc - MAX_DL; ++i) {
		stmt->fetch();
	}
	while (stmt->fetch()) {
		ZJXX *p = &zjxx[zjxxLen];
		p->day = stmt->getInt(0);
		p->cdd = stmt->getInt(1);
		p->dd = stmt->getInt(2);
		p->zd = stmt->getInt(3);
		p->xd = stmt->getInt(4);
		++zjxxLen;
		//printf("QueryResult: %d %d %d %d %d \n", p->day, p->cdd, p->dd, p->zd, p->xd);
	}
	
	stmt2->reset();
	stmt2->setStringParam(0, code);
	stmt2->bindParams();
	stmt2->setResult(0, Statement::CT_INT);
	stmt2->setResult(1, Statement::CT_INT);
	stmt2->setResult(2, Statement::CT_INT);
	stmt2->bindResult();
	stmt2->exec();
	stmt2->storeResult();
	rc = stmt2->getRowsCount();
	for (int i = 0; i < rc - MAX_DL; ++i) {
		stmt2->fetch();
	}
	while (stmt2->fetch()) {
		HGT *p = &hgt[hgtLen];
		p->day = stmt2->getInt(0);
		p->jme = stmt2->getInt(1);
		p->cjje = stmt2->getInt(2);
		++hgtLen;
	}
}

inline int FindDay(int day, int from) {
	for (int i = from; i < daysLen; ++i) {
		if (days[i] == day)
			return i;
	}
	return -1;
}

inline int CalcZJVal(int idx) {
	ZJXX *p = &zjxx[idx];
	int val = 0;
	if (params[IP_CDD]) val += p->cdd;
	if (params[IP_DD]) val += p->dd;
	if (params[IP_ZD]) val += p->zd;
	if (params[IP_XD]) val += p->xd;
	return val;
}

void CalcZJ(float *out, int len) {
	QueryResult();
	int begin = len - daysLen;
	int from = 0;
	for (int i = 0; i < zjxxLen; ++i) {
		ZJXX *p = &zjxx[i];
		int j = FindDay(p->day, from);
		if (j == -1) {
			out[begin + j] = 0;
			continue;
		}
		from = j + 1;
		out[begin + j] = CalcZJVal(i);
		//printf("day = %d out[%d]=%d \n", p->day, j+begin, (int)out[begin + j]);
	}
	for (int i = 0; i < begin; ++i) out[i] = 0;
}

void CalcHgtZJ(float *out, int len) {
	QueryResult();
	memset(out, 0, sizeof(float) * len);
	int begin = len - daysLen;
	int from = 0;
	for (int i = 0; i < hgtLen; ++i) {
		HGT *p = &hgt[i];
		int j = FindDay(p->day, from);
		if (j == -1) {
			//out[begin + j] = 0;
			continue;
		}
		from = j + 1;
		out[begin + j] = p->jme;
		//printf("day = %d out[%d]=%d \n", p->day, j+begin, (int)out[begin + j]);
	}
	//for (int i = 0; i < begin; ++i) out[i] = 0;
}

void CalcHgtZJAbs(float *out, int len) {
	CalcHgtZJ(out, len);
	for (int i = 0; i < len; ++i) {
		if (out[i] < 0) out[i] = -out[i];
	}
}

void CalcZJAbs(float *out, int len) {
	QueryResult();
	int begin = len - daysLen;
	int from = 0;
	for (int i = 0; i < zjxxLen; ++i) {
		ZJXX *p = &zjxx[i];
		int j = FindDay(p->day, from);
		if (j == -1)  {
			out[begin + j] = 0;
			continue;
		}
		from = j + 1;
		int val = CalcZJVal(i);
		val = _ABS(val);
		out[begin + j] = val;
		zjMax = _MAX(zjMax, val);
	}
	for (int i = 0; i < begin; ++i) out[i] = 0;
}

void GetZJMax(float *out, int len) {
	out[len - 1] = zjMax;
}

void OpenIO() {
	static int op = 0;
	if (op) return;
	op = 1;
	AllocConsole();
    freopen("CONOUT$", "wb", stdout);
}

void GetZJSum(float *out, int len, int days, int zjType) {
	//OpenIO();
	QueryResult();
	if (days > len) days = len;
	if (days > zjxxLen) days = zjxxLen;
	int begin = len - days;
	for (int i = 0; i < begin; ++i) out[i] = 0;
	for (int i = begin, j = zjxxLen - days; i < len; ++i, ++j) {
		float last = i > 0 ? out[i - 1] : 0;
		float cur = 0;
		if (zjType == 0) cur = zjxx[j].cdd + zjxx[j].dd;
		if (zjType == 1) cur = zjxx[j].cdd + zjxx[j].dd + zjxx[j].zd;
		if (zjType == 2) cur = zjxx[j].zd;
		out[i] = last + cur;
	}
}

void GetThsPM(int code, int *pm, int *num) {
	InitMysql();
	char scode[12];
	static Statement *stmt = NULL;
	static char sql[128];
	if (stmt == NULL) {
		sprintf(sql, "select _bkpm from %s where _code = ?", thsPmName);
		stmt = db.prepare(sql);
	}
	if (stmt == NULL) return;

	sprintf(scode, "%06d", code);
	stmt->reset();
	stmt->setBindCapacity(64, 64);
	stmt->setStringParam(0, scode);
	stmt->bindParams();
	stmt->setResult(0, Statement::CT_STRING, 12);
	stmt->bindResult();
	stmt->exec();
	stmt->storeResult();
	if (! stmt->fetch()) return;
	char *p = stmt->getString(0);
	strcpy(scode, p);
	p = strchr(scode, '/');
	if (p == NULL) return;
	*p = 0;
	++p;
	*pm = atoi(scode);
	*num = atoi(p);
}

void GetLastZJ(float *out, int len, int code, int dayNum) {
	if (len <= 10) {
		memset(out, 0, sizeof(float) * len);
		return;
	}
	//OpenIO();
	InitMysql();
	static int zjArr[60];
	static char sql[200];
	sprintf(sql, "select _zl from _zjxx where _code = ? order by _id desc limit %d", dayNum);
	
	Statement *stmt2 = db.prepare(sql);
	if (stmt2) stmt2->setBindCapacity(48, 128);
	char scode[8];
	sprintf(scode, "%06d", code);
	stmt2->reset();
	stmt2->setStringParam(0, scode);
	stmt2->bindParams();
	stmt2->setResult(0, Statement::CT_INT);
	stmt2->bindResult();
	stmt2->exec();
	stmt2->storeResult();
	// int rc = stmt->getRowsCount();
	int idx = 0;
	while (stmt2->fetch()) {
		zjArr[idx++] = stmt2->getInt(0);
	}
	delete stmt2;
	// 累计流入, 只考虑1千万以上的资金
	out[len - 1] = out[len - 2] = out[len - 3] = 0;
	for (int i = 0; i < idx; ++i) {
		out[len - 1] += zjArr[i];
	}
}










