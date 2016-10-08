#pragma once

typedef struct _KdjItem {
	float mK;
	float mD;
	float mJ;
} KdjItem;

typedef struct _MacdItem {
	float mDif;
	float mDea;
} MacdItem;

typedef struct _KItem {
	int mDate;	// YYYYMMDD
	float mOpen;
	float mClose;
	float mHigh;
	float mLow;
	int mVol;
} KItem;


typedef struct _KLineItem {
	KItem mKItem;
	MacdItem mMacdItem;
	KdjItem mKdjItem;
} KLineItem;


