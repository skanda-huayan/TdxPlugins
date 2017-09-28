#ifndef _EAST_MONEY_H_
#define _EAST_MONEY_H_

#if BUILDING_DLL
#define DLLIMPORT __declspec(dllexport)
#else
#define DLLIMPORT __declspec(dllimport)
#endif


extern "C" {

DLLIMPORT void* FetchEastMoneyZJ(int code);

DLLIMPORT void CalcEastMoneyZJ(void *task, float *out, float *days, int len);
DLLIMPORT void CalcEastMoneyZJAbs(void *task, float *out, float *days, int len);

}

#endif
