#ifndef _DLL_H_
#define _DLL_H_

#if BUILDING_DLL
#define DLLIMPORT __declspec(dllexport)
#else
#define DLLIMPORT __declspec(dllimport)
#endif

extern "C" {

DLLIMPORT void InitZJParam(int id, int val);

DLLIMPORT void InitZJParamDate(float* days, int len);

DLLIMPORT void CalcZJ(float *out, int len);

DLLIMPORT void CalcHgtZJ(float *out, int len);
DLLIMPORT void CalcHgtZJAbs(float *out, int len);

DLLIMPORT void CalcZJAbs(float *out, int len);

DLLIMPORT void GetZJMax(float *out, int len);

// zjType = 0:���� 1:�����л� 2:�л�
DLLIMPORT void GetZJSum(float *out, int len, int days, int zjType);

DLLIMPORT void GetThsPM(int code, int *pm, int *num);

//���������ʽ�����
DLLIMPORT void GetLastZJ(float *out, int len, int code, int dayNum);

}

#endif
