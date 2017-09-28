#include "eastmoney.h"
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <stdlib.h>
#include "curl/curl.h"

// #pragma comment(lib, "D:/CPP/GP/vsproj/Test/Test/libcurl.lib")

extern void OpenIO();

enum TaskStatus {
	TS_READY,
	TS_RUNNING,
	TS_FINISH,
	TS_FAILED
};

struct TaskItemInfo {
	int code;
	int num;
	int days[100];
	int jme[100];
	TaskStatus status;
	TaskItemInfo *prev, *next;
};

static TaskItemInfo finishHead, readyHead;
static HANDLE evt;
static CRITICAL_SECTION g_cs, g_token_cs;

static void DoFetch(TaskItemInfo *item);

void Wait() {
	WaitForSingleObject(evt, INFINITE);
}

void Notify() {
	SetEvent(evt);
}

static TaskItemInfo* FindTaskItem(TaskItemInfo *head, int code) {
	TaskItemInfo *p = head->prev;
	while (p != head) {
		if (code == p->code) return p;
		p = p->prev;
	}
	return NULL;
}

static void ResetTaskItemInfo(TaskItemInfo *t) {
	t->code = 0;
	t->num = 0;
	t->prev = t->next = NULL;
	t->status = TS_READY;
}

static DWORD WINAPI ThreadFunc(void *param) {
	while (1) {
		EnterCriticalSection(&g_cs);
		TaskItemInfo *item = NULL;
		if (readyHead.prev != &readyHead) {
			item = readyHead.prev;
			item->prev->next = &readyHead;
			readyHead.prev = item->prev;
			item->prev = item->next = NULL;

			item->prev = finishHead.prev;
			item->next = &finishHead;
			finishHead.prev->next = item;
			finishHead.prev = item;

			item->status = TS_RUNNING;
		}
		LeaveCriticalSection(&g_cs);

		if (item != NULL) {
			DoFetch(item);
		} else {
			Wait();
		}
	}
}

static void StartThread() {
	static int inited = 0;
	if (inited) return;
	inited = 1;
	InitializeCriticalSection(&g_cs);
	InitializeCriticalSection(&g_token_cs);
	finishHead.prev = finishHead.next = &finishHead;
	readyHead.prev = readyHead.next = &readyHead;
	evt = CreateEvent(NULL, FALSE, FALSE, NULL);
	for (int i = 0; i < 3; ++i) {
		HANDLE hd = CreateThread(NULL,0, (LPTHREAD_START_ROUTINE)ThreadFunc, NULL, 0, NULL);
		CloseHandle(hd);
	}
}

void* FetchEastMoneyZJ(int code) {
	//OpenIO();
	StartThread();
	EnterCriticalSection(&g_cs);
	TaskItemInfo *info = FindTaskItem(&finishHead, code);
	if (info == NULL)  info = FindTaskItem(&readyHead, code);
	TaskItemInfo* r = NULL;
	if (info == NULL) {
		r = (TaskItemInfo*)malloc(sizeof(TaskItemInfo));
		ResetTaskItemInfo(r);
		r->code = code;
		r->prev = readyHead.prev;
		r->next = &readyHead;
		readyHead.prev->next = r;
		readyHead.prev = r;
		info = r;
	}
	LeaveCriticalSection(&g_cs);
	if (r != NULL) {
		Notify();
	}
	//printf("\n------- Fetch code=%06d info=%p -----\n", code, info);
	return info;
}

void CalcEastMoneyZJ(void *vtask, float *out, float *days, int len) {
	TaskItemInfo *task = (TaskItemInfo*)vtask;
	const int ML = sizeof(task->days)/sizeof(int);
	if (task == NULL || task->num <= 0 || task->num > ML) return;

	int mlen = len > ML ? ML : len;
	int lastDayPos = -1, firstDayPos = -1, taskFirstDayPos = 0;
	for (int i = len - 1; i >= 0; --i) {
		if ((int)days[i] + 19000000 == task->days[task->num - 1]) {
			lastDayPos = i;
			break;
		}
	}
	if (lastDayPos == -1) {
		return;
	}
	firstDayPos = lastDayPos - task->num + 1;
	if (firstDayPos < 0) {
		firstDayPos = 0;
		int k = -1;
		for (int i = 0; i < task->num; ++i) {
			if (task->days[i] == (int)days[0] + 19000000) {
				k = i;
				break;
			}
		}
		if (k == -1) {
			return;
		}
		taskFirstDayPos = k;
	}
	if (task->num - 1 - taskFirstDayPos != lastDayPos - firstDayPos) {
		return;
	}
	memset(out, 0, sizeof(float) * len);
	for (int i = firstDayPos, j = 0; i <= lastDayPos; ++i, ++j) {
		out[i] = task->jme[j + taskFirstDayPos];
	}
	//_e:
	//printf("ZJ task=%p len=%d task.num=%d\n", task, len, task->num);
	//printf("   firstDayPos=%d lastDayPos=%d taskFirstDayPos=%d \n", firstDayPos, lastDayPos, taskFirstDayPos);
}

void CalcEastMoneyZJAbs(void *vtask, float *out, float *days, int len) {
	CalcEastMoneyZJ(vtask, out, days, len);
	if (vtask == NULL) return;
	for (int i = 0; i < len; ++i) {
		if (out[i] < 0) out[i] = -out[i];
	}
}

//-------------------------------------
static char token[128];
static time_t lastTime;
struct NetData {
	int wpos;
	char *wbuf;
	TaskItemInfo *task;
};

NetData* CreateNetData() {
	NetData *d = (NetData*)malloc(sizeof(NetData));
	memset(d, 0, sizeof(NetData));
	d->wbuf = (char*)malloc(1024 * 128);
	d->wbuf[0] = 0;
	d->task = NULL;
	return d;
}

void DestroyNetData(NetData *d) {
	if (! d) return;
	if (d->wbuf) free(d->wbuf);
	d->wbuf = NULL;
	free(d);
}

static size_t write_data( void *ptr, size_t size, size_t nmemb, void *stream) {
	NetData *d = (NetData*)stream;
	memcpy(d->wbuf + d->wpos, ptr, size * nmemb);
	d->wpos += size * nmemb;
	d->wbuf[d->wpos] = 0;
	return size * nmemb;
}

curl_slist *create_header() {
	curl_slist *chunk = NULL;
	chunk = curl_slist_append(chunk, "Connection: keep-alive");
	chunk = curl_slist_append(chunk, "Upgrade-Insecure-Requests: 1");
	chunk = curl_slist_append(chunk, "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36");
	chunk = curl_slist_append(chunk, "Accept: */*");
	//chunk = curl_slist_append(chunk, "Accept-Encoding: gzip, deflate, sdch");
	chunk = curl_slist_append(chunk, "Accept-Language: zh-CN,zh;q=0.8");
	return chunk;
}

int FetchToken(NetData *d) {
	CURL *curl;
	CURLcode res;
	curl_slist *chunk = NULL;

	curl = curl_easy_init();
	chunk = create_header();
	res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	curl_easy_setopt(curl, CURLOPT_URL, "http://data.eastmoney.com/zjlx/600000.html");
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, d);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	curl_slist_free_all(chunk);
	if(res != CURLE_OK) {
		fprintf(stderr, "FetchToken failed: %s\n", curl_easy_strerror(res));
		return 0;
	}
	char *p = strstr(d->wbuf, "acces_token=");
	if (p == NULL) return 0;
	char *p2 = strchr(p, '&');
	if (p2 == NULL) return 0;

	lastTime = time(NULL);
	*p2 = 0;
	strcpy(token, p + 12);
	// printf("token={%s} \n", token);
	return 1;
}

int FetchZJ(NetData *d) {
	char url[256];
	int type = (d->task->code >= 600000 ? 1 : 2);
	const static char *sh = "http://ff.eastmoney.com/EM_CapitalFlowInterface/api/js?type=hff&rtntype=2&js=({data:[(x)]})&cb=var%%20aff_data=&check=TMLBMSPROCR&acces_token=%s&id=%06d%d&_=1504169565883";
	sprintf(url, sh, token, d->task->code, type);

	CURL *curl;
	CURLcode res;
	curl_slist *chunk = NULL;

	curl = curl_easy_init();
	chunk = create_header();
	res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, d);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	curl_slist_free_all(chunk);
	if(res != CURLE_OK) {
		fprintf(stderr, "FetchZJ failed: %s\n", curl_easy_strerror(res));
		return 0;
	}
	return 1;
}

int ParseZJ(NetData *d) {
	char *p = d->wbuf;
	const char *tmp = "var aff_data=({data:[[";
	BOOL first = TRUE;
	if (memcmp(p, tmp, strlen(tmp)) != 0)
		return 0;
	if (memcmp(p + d->wpos - 4, "]]})", 4) != 0)
		return 0;
	p += strlen(tmp);

	while (d->task->num < sizeof(d->task->days)/sizeof(int)) {
		if (! first) {
			if (*p == ',') ++p;
			else if (*p == ']') break;
		}
		first = FALSE;
		if (*p != '"') return 0;
		if (p == NULL) break;
		++p; // skip "
		int day = atoi(p) * 10000;
		p += 5;
		day += atoi(p) * 100;
		p += 3;
		day += atoi(p);
		p += 2;
		d->task->days[d->task->num] = day;
		if (*p != ',') return 0;
		++p; // skip ,
		int jme = atoi(p);
		d->task->jme[d->task->num] = jme;
		// printf("%d -> %d \n", day, jme);
		p = strchr(p, '"');
		if (p == NULL) return 0;
		++p;
		++d->task->num;
	}
	return 1;
}

static BOOL IsNeedRefreshToken() {
	time_t ct = time(NULL);
	if (ct - lastTime > 4 * 3600) return TRUE;
	return FALSE;
}

static void DoFetch(TaskItemInfo *item) {
	NetData *d = CreateNetData();
	d->task = item;
	if (IsNeedRefreshToken() && !FetchToken(d)) {
		goto _failed;
	}
	d->wpos = 0;
	d->wbuf[0] = 0;
	if (! FetchZJ(d)) {
		goto _failed;
	}
	if (! ParseZJ(d)) {
		goto _failed;
	}
	d->task->status = TS_FINISH;
	return;

_failed:
	EnterCriticalSection(&g_cs);
	item->status = TS_FAILED;
	item->prev->next = item->next;
	item->next->prev = item->prev;
	item->prev = item->next = NULL;
	DestroyNetData(d);
	LeaveCriticalSection(&g_cs);
}

