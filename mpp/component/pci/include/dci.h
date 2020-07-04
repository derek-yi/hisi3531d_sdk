/******************************************************************************

  Copyright (C), 2001-2013, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : dci.h
  Version       : Initial Draft
  Author        : y45339
  Created       : 2013/05/30
  Last Modified :
  Description   : 
  Function List :
              
  History       :
  1.Date        : 2013/05/30
    Author      : y45339
    Modification: Created file

******************************************************************************/

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/
#ifndef __DCI_H__
#define __DCI_H__

//#define ROUND(x) ((x) > 0 ? (HI_U32)((x) + 0.5) : (-((HI_U32)(-((x) - 0.5)))))
///#define FIX(x) ((x) > 0 ? (HI_U32)(x) : (-((HI_U32)-(x))))
#define ROUND(x) ((x) > 0 ? (HI_S32)((x) + 0.5) : ( (HI_S32) ( (x) - 0.5) ) )
#define RSHFT(x,n) ((x) >= 1 ? (  ( (x) + (1<<((n)-1)) )>>(n)  ) : (-(  ( (-(x)) + (1<<((n)-1)) )>>(n)  )) )


#define BITSHIFT(x, y)  SIGN(x) * ((y) >= 0 ? (ABS(x) << ( ABS (y))) : (ABS(x) >> ABS(y)))
#define NUMDIV  6
#define BINNUM 32
#define METRICNUM 3

#define DCI_SLP_PRC 8
#define HIST_ABLEND  16

#define DCI_HIST_SIZE  128

#define DCI_MIN_STRENGTH  0
#define DCI_MAX_STRENGTH  63


#define DIVLUTBitNum  12 //the precision of the divide LUT

typedef struct {
    HI_S32 s32GlbGainBright;
    HI_S32 s32GlbGainContrast;
    HI_S32 s32GlbGainDark;

	HI_S32 s32InputFRange;
	HI_U32 u32FileBitDep;
	
	HI_S32  s32Histogram[2][BINNUM];    //Histogram[0] for the current hist.Histogram[1] for the previous hist. 
	HI_S32  s32HistRShft[2][BINNUM];
	HI_S32  s32HistCoring[METRICNUM];
    HI_S32  s32HistRShftBit;
	HI_S32  s32HistABlend;

	HI_S32  s32HistLPFEn;
	HI_S32  s32HistLPF0;
	HI_S32  s32HistLPF1;
	HI_S32  s32HistLPF2;
    /****************** scene change detect  ******************/
	HI_S32  s32EnSceneChange;
	HI_S32  s32SCDFlag;
	HI_S32  s32HistSAD[7];
	HI_S32  s32SADRshft;
	HI_S32  s32SADThrsh;
	HI_S32  s32CurFrmNum;

	/****************** Max Min Avg ******************/
    // the weight of each bin
	const HI_S16 *ps16BinWeight[METRICNUM];
	HI_S32  s32SumWeight[METRICNUM];
	HI_S32  s32Metric[METRICNUM];
	HI_S32  s32AdjWgt[METRICNUM];
	HI_S32  s32ManAdjWgt[METRICNUM];
	HI_U8   u8WgtBitNum;

	HI_S32  s32MetricAlpha[METRICNUM];
	HI_S32  s32MetricInit[METRICNUM];

	HI_S32	s32EnManAdj[METRICNUM];

	HI_S32 s32GlbGain0,s32GlbGain1,s32GlbGain2;

	HI_S32  s32OrgABlend[METRICNUM];

	HI_S32 s32AdjGain_ClipL[METRICNUM];
	HI_S32 s32AdjGain_ClipH[METRICNUM];

	// adjusting LUT for brightness and contrast
	HI_S32  ps32AdjLUT[33], ps32AdjLUT_0[33], ps32AdjLUT_1[33], ps32AdjLUT_2[33];

    HI_S32 s32AdjDiff[1024];
#if 1
    HI_S32 s32CbCrSta_en, s32CbCrSta_Y, s32CbCrSta_Cb, s32CbCrSta_Cr, s32CbCrSta_Offset;


    HI_S32 s32CbCrComp_en;
    HI_S32 s32CbCrGainPos[9];
    HI_S32 s32CbCrGainPos_Slp[8];
    HI_S32 s32CbCrGainPos_Ythrsh[9];
    HI_S32 s32CbCrGainNeg[9];
    HI_S32 s32CbCrGainNeg_Slp[8];
    HI_S32 s32CbCrGainNeg_Ythrsh[9];


    HI_S32 s32divLUT[64];
    HI_S32 s32divLUT_Num;
#endif
} DCIParam;
#endif

