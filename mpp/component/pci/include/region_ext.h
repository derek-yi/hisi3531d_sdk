/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : region_ext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/03/11
  Description   :
  History       :
  1.Date        : 2009/03/11
    Author      : l64467
    Modification: Created file

  2.Date        : 2010/12/18
    Author      : l64467
    Modification: add and modify some function

******************************************************************************/
#ifndef __REGION_EXT_H__
#define __REGION_EXT_H__

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_region.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef HI_VOID (*RGN_DETCHCHN)(HI_S32 stDevId,HI_S32 stChnId,HI_BOOL bDetchChn);

typedef struct hiRGN_COMM_S
{
    HI_BOOL 				bShow;        		/* whether be showed */
    POINT_S			        astLinePt[2];		/* points of LINE */
    HI_U32                  u32Width;     		/* width of LINE */

    RGN_AREA_TYPE_E		    enCoverType;    	/* COVER type */
    POINT_S 				stPoint;        	/* original point of rect */
    SIZE_S  				stSize;         	/* size of rect */
    RGN_QUADRANGLE_S  	    stQuadRangle;   	/* config of arbitary quadrilateral COVER */

    HI_U32  				u32Layer;       	/* region layer */
    HI_U32  				u32BgColor;     	/* background color */
    HI_U32  				u32GlobalAlpha; 	/* global alpha */
    HI_U32  				u32FgAlpha;     	/* foreground alpha */
    HI_U32  				u32BgAlpha;     	/* background alpha */
    HI_U32  				u32PhyAddr;     	/* physical address of PING_PONG buffer */
	HI_U32  				u32VirtAddr;    	/* virtual address of PING_PONG buffer */
	HI_U32  				u32Stride;      	/* stride of PING_PONG buffer */
    PIXEL_FORMAT_E      	enPixelFmt;     	/* pixel format */
    VIDEO_FIELD_E       	enAttachField;  	/* field info */

    OVERLAY_QP_INFO_S   	stQpInfo;       	/* QP info */

    OVERLAY_INVERT_COLOR_S 	stInvColInfo;   	/* reverse color info */

    MOSAIC_BLK_SIZE_E       enMosaicBlkSize;	/* MOSAIC block size */

	RGN_COORDINATE_E         enCoordinate;

}RGN_COMM_S;

typedef struct hiRGN_INFO_S
{
    HI_U32 u32Num;
    HI_BOOL bModify;
    RGN_COMM_S **ppstRgnComm;
}RGN_INFO_S;

typedef struct hiRGN_REGISTER_INFO_S
{
    MOD_ID_E    enModId;
    HI_U32      u32MaxDevCnt;   				/* If no dev id, should set it 1 */
    HI_U32      u32MaxChnCnt;
    RGN_DETCHCHN      pfnRgnDetchChn;
} RGN_REGISTER_INFO_S;

#define PIXEL_FORMAT_NUM_MAX 3

typedef struct hiRGN_CAPACITY_LAYER_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    HI_U32 	u32LayerMin;
    HI_U32 	u32LayerMax;
}RGN_CAPACITY_LAYER_S;

typedef struct hiRGN_CAPACITY_POINT_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    POINT_S stPointMin;
    POINT_S stPointMax;

    HI_U32 	u32StartXAlign;
    HI_U32 	u32StartYAlign;

}RGN_CAPACITY_POINT_S;

typedef struct hiRGN_CAPACITY_SIZE_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    SIZE_S 	stSizeMin;
    SIZE_S 	stSizeMax;

    HI_U32 	u32WidthAlign;
    HI_U32 	u32HeightAlign;

    HI_U32 	u32MaxArea;
}RGN_CAPACITY_SIZE_S;

typedef struct hiRGN_CAPACITY_QUADRANGLE_S
{
    HI_BOOL bComm;
	HI_BOOL bSptReSet;

	HI_U32 	u32MinLineWidth;
	HI_U32 	u32MaxLineWidth;
	POINT_S stPointMin;
    POINT_S stPointMax;

    HI_U32  u32LineWidthAlign;

    HI_U32  u32StartXAlign;
    HI_U32  u32StartYAlign;
}RGN_CAPACITY_QUADRANGLE_S;

typedef struct hiRGN_CAPACITY_LINE_S
{
    HI_BOOL bComm;
	HI_BOOL bSptReSet;

	HI_U32 	u32MinLineWidth;
	HI_U32 	u32MaxLineWidth;
	POINT_S stPointMin;
    POINT_S stPointMax;

    HI_U32  u32LineWidthAlign;

    HI_U32  u32StartXAlign;
    HI_U32  u32StartYAlign;
}RGN_CAPACITY_LINE_S;

typedef struct hiRGN_CAPACITY_PIXLFMT_S
{
    HI_BOOL 		bComm;
    HI_BOOL 		bSptReSet;

    HI_U32 			u32PixlFmtNum;
    PIXEL_FORMAT_E 	aenPixlFmt[PIXEL_FORMAT_NUM_MAX];
}RGN_CAPACITY_PIXLFMT_S;

typedef struct hiRGN_CAPACITY_ALPHA_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    HI_U32 u32AlphaMax;
    HI_U32 u32AlphaMin;
}RGN_CAPACITY_ALPHA_S;

typedef struct hiRGN_CAPACITY_BGCLR_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;
}RGN_CAPACITY_BGCLR_S;

typedef enum hiRGN_SORT_KEY_E
{
    RGN_SORT_BY_LAYER = 0,
    RGN_SORT_BY_POSITION,
    RGN_SRT_BUTT
}RGN_SORT_KEY_E;

typedef struct hiRGN_CAPACITY_SORT_S
{
    HI_BOOL bNeedSort;

    RGN_SORT_KEY_E enRgnSortKey;

    HI_BOOL bSmallToBig;
}RGN_CAPACITY_SORT_S;

typedef struct hiRGN_CAPACITY_QP_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

	HI_BOOL bSptQpDisable;

    HI_S32 s32QpAbsMin;
    HI_S32 s32QpAbsMax;

    HI_S32 s32QpRelMin;
    HI_S32 s32QpRelMax;

}RGN_CAPACITY_QP_S;

typedef struct hiRGN_CAPACITY_INVCOLOR_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    RGN_CAPACITY_SIZE_S stSizeCap;

    HI_U32 u32LumMin;
    HI_U32 u32LumMax;

    INVERT_COLOR_MODE_E enInvModMin;
    INVERT_COLOR_MODE_E enInvModMax;

    HI_U32 u32StartXAlign;
    HI_U32 u32StartYAlign;
    HI_U32 u32WidthAlign;
    HI_U32 u32HeightAlign;

}RGN_CAPACITY_INVCOLOR_S;

typedef struct hiRGN_CAPACITY_MSCBLKSZ_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    HI_U32 u32MscBlkSzMax;
    HI_U32 u32MscBlkSzMin;
}RGN_CAPACITY_MSCBLKSZ_S;

typedef enum hiRGN_CAPACITY_QUADRANGLE_TYPE_E
{
	RGN_CAPACITY_QUADRANGLE_UNSUPORT = 0x0,
	RGN_CAPACITY_QUADRANGLE_TYPE_SOLID = 0x1,
	RGN_CAPACITY_QUADRANGLE_TYPE_UNSOLID = 0x2,
	RGN_CAPACITY_QUADRANGLE_TYPE_BUTT = 0x4,
}RGN_CAPACITY_QUADRANGLE_TYPE_E;

typedef struct hiRGN_CAPACITY_COVER_ATTR_S
{
    HI_BOOL bSptRect;
    RGN_CAPACITY_QUADRANGLE_TYPE_E enSptQuadRangleType;
}RGN_CAPACITY_COVER_ATTR_S;

typedef HI_BOOL (*RGN_CHECK_CHN_CAPACITY)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn);

typedef struct hiRGN_CAPACITY_S
{
    RGN_CAPACITY_LAYER_S stLayer;

    RGN_CAPACITY_POINT_S stPoint;

    RGN_CAPACITY_SIZE_S  stSize;

    RGN_CAPACITY_QUADRANGLE_S stQuadRangle;

    RGN_CAPACITY_LINE_S stLine;

    RGN_CAPACITY_MSCBLKSZ_S stMscBlkSz;

    HI_BOOL bSptPixlFmt;
    RGN_CAPACITY_PIXLFMT_S stPixlFmt;

    HI_BOOL bSptFgAlpha;     			/* support front ground alpha or not */
    RGN_CAPACITY_ALPHA_S stFgAlpha;

    HI_BOOL bSptBgAlpha;     			/* support background alpha or not */
    RGN_CAPACITY_ALPHA_S stBgAlpha;

    HI_BOOL bSptGlobalAlpha;			/* support global alpha or not */
    RGN_CAPACITY_ALPHA_S stGlobalAlpha;

    HI_BOOL bSptBgClr;     				/* support background color or not */
    RGN_CAPACITY_BGCLR_S stBgClr;

    HI_BOOL bSptChnSort;   				/* support channrl sort or not */
    RGN_CAPACITY_SORT_S stChnSort;

    HI_BOOL bSptQp;
    RGN_CAPACITY_QP_S stQp;

    HI_BOOL bSptInvColoar;  			/* support color invert or not */
    RGN_CAPACITY_INVCOLOR_S stInvColor; /* color invert configuration */

	RGN_CAPACITY_COVER_ATTR_S stCoverAttr; /* cover attribute */

    HI_BOOL bSptBitmap;    				/* support bitmap or not */
    HI_BOOL bsptOverlap;   				/* support overlap or not */
    HI_U32 u32Stride;
    HI_U32 u32RgnNumInChn; 				/* max region number of one channel */

	RGN_CHECK_CHN_CAPACITY pfnCheckChnCapacity; /*Check whether channel support a region type*/
}RGN_CAPACITY_S;


HI_S32 RGN_RegisterMod(RGN_TYPE_E enType, const RGN_CAPACITY_S *pstCapacity,
    const RGN_REGISTER_INFO_S *pstRgtInfo);
HI_S32  RGN_UnRegisterMod(RGN_TYPE_E enType, MOD_ID_E enModId);

HI_S32  RGN_GetRegion(RGN_TYPE_E enType, const MPP_CHN_S *pstChn,
    RGN_INFO_S *pstRgnInfo);
HI_S32  RGN_PutRegion(RGN_TYPE_E enType, const MPP_CHN_S *pstChn);


typedef struct hiRGN_EXPORT_FUNC_S
{
    HI_S32 (*pfnRgnRegisterMod)(RGN_TYPE_E enType,
        const RGN_CAPACITY_S *pstCapacity, const RGN_REGISTER_INFO_S *pstRgtInfo);
    HI_S32 (*pfnUnRgnRegisterMod)(RGN_TYPE_E enType, MOD_ID_E enModId);

    HI_S32 (*pfnRgnGetRegion)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn,
        RGN_INFO_S *pstRgnInfo);
    HI_S32 (*pfnRgnPutRegion)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn);
    HI_S32 (*pfnRgnSetModifyFalse)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn);
}RGN_EXPORT_FUNC_S;


#define CKFN_RGN() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN))


#define CKFN_RGN_RegisterMod() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnRegisterMod)


#define CALL_RGN_RegisterMod(enType, pstCapacity, pstRgtInfo) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnRegisterMod(enType, \
    pstCapacity, pstRgtInfo)


#define CKFN_RGN_UnRegisterMod() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnUnRgnRegisterMod)


#define CALL_RGN_UnRegisterMod(enType, enModId) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnUnRgnRegisterMod(enType, enModId)


#define CKFN_RGN_GetRegion() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnGetRegion)


#define CALL_RGN_GetRegion(enType, pstChn, pstRgnInfo) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnGetRegion(enType, pstChn, pstRgnInfo)



#define CKFN_RGN_PutRegion() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnPutRegion)

#define CALL_RGN_PutRegion(enType, pstChn) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnPutRegion(enType, pstChn)


#define CKFN_RGN_SetModifyFalse() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnSetModifyFalse)

#define CALL_RGN_SetModifyFalse(enType, pstChn) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnSetModifyFalse(enType, pstChn)


#ifdef __cplusplus
    #if __cplusplus
    }
    #endif
#endif /* End of #ifdef __cplusplus */

#endif /* __REGION_EXT_H__ */



