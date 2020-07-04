

/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_sys.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 
  Description   :
  History       :
  1.Date        : 
    Author      : 
    Modification: 
    
 

******************************************************************************/
#ifndef __VPSS_EXT_H__
#define __VPSS_EXT_H__

#include "hi_comm_vpss.h"

typedef struct HI_VPSS_PIC_INFO_S
{   
    VIDEO_FRAME_INFO_S 	stVideoFrame;
    MOD_ID_E        	enModId;
    HI_BOOL 			bFlashed;       			/* Flashed Video frame or not */
    HI_BOOL 			bBlock;    		
}VPSS_PIC_INFO_S;


typedef struct HI_VPSS_QUERY_INFO_S
{
    VPSS_PIC_INFO_S* pstSrcPicInfo;   				/* source picture information */
    VPSS_PIC_INFO_S* pstOldPicInfo;    	 			/* backup picture information */
    HI_BOOL          bScaleCap;         			/* support scale or not */   
    HI_BOOL          bTransCap;         			/* support double frame or not */
    HI_BOOL          bMalloc;           			/* need malloc buffer or not */
    HI_BOOL          bCompress;     				/* support compress or not */
}VPSS_QUERY_INFO_S;

typedef struct HI_VPSS_INST_INFO_S
{   
    HI_BOOL          bNew;               			/* use new picture to qurey or not */
    HI_BOOL          bVpss;              			/* use Vpss to proc or not */
    HI_BOOL          bDouble;           	 		/* support double frame or not */
    HI_BOOL          bUpdate;            			/* update backup picture or not */
    ASPECT_RATIO_S stAspectRatio;   				/* aspect ratio configuration */
    COMPRESS_MODE_E enCompressMode;      			/* compress mode */
    VPSS_PIC_INFO_S  astDestPicInfo[2]; 			/* dest picture information after Vpss process */
}VPSS_INST_INFO_S;

typedef struct HI_VPSS_HIST_INFO_S
{
   HI_U32          u32PhyAddr;
   HI_VOID         *pVirAddr;
   HI_U32          u32Length;
   HI_U32          u32PoolId;
   HI_BOOL         bQuarter;
   
}VPSS_HIST_INFO_S;


typedef struct HI_VPSS_SEND_INFO_S
{
    HI_BOOL           bSuc;                  		/* Vpss proc success or not */
    VPSS_GRP          VpssGrp;
    VPSS_CHN          VpssChn;
    VPSS_PIC_INFO_S   *pstDestPicInfo[2];    		/* dest picture information after Vpss process */
    VPSS_HIST_INFO_S  *pstHistInfo;          		/* hist information of picture */
}VPSS_SEND_INFO_S;


typedef struct hiVPSS_VGS_NODE_S
{
    HI_U32 u32JobId;                                /* vgs job id */
    HI_U32 u32TaskId;								/* vgs task id */
    
    HI_BOOL bOrderJob;                              /* sort job or not */
    
    HI_BOOL bJobFinished;                           /* job has been submitted all or not */

    HI_U32   u32PhyAddr[VPSS_MAX_SPLIT_NODE_NUM];   /* physical addr of node */
    HI_VOID *pstNode[VPSS_MAX_SPLIT_NODE_NUM];      /* pointer of node */
    HI_U32 u32CropNodeCnt;                          /* node num of one task */

    HI_U32 u32SrcWidth;                   			/* source picture width */
    HI_U32 u32SrcHeight;                        	/* source picture height */        
}VPSS_VGS_NODE_S;


typedef struct HI_VPSS_REGISTER_INFO_S
{
    MOD_ID_E    enModId;
    HI_S32      (*pVpssQuery)(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_QUERY_INFO_S  *pstQueryInfo, VPSS_INST_INFO_S *pstInstInfo);
    HI_S32      (*pVpssSend)(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_SEND_INFO_S  *pstSendInfo);
    HI_S32      (*pResetCallBack)(HI_S32 s32DevId, HI_S32 s32ChnId, HI_VOID *pvData);
 
}VPSS_REGISTER_INFO_S;


typedef struct hiVPSS_EXPORT_FUNC_S
{
    HI_S32  (*pfnVpssRegister)(VPSS_REGISTER_INFO_S *pstInfo);
    HI_S32  (*pfnVpssUnRegister)(MOD_ID_E enModId);
    HI_S32  (*pfnVpssStartVgsTask)(VPSS_VGS_NODE_S* pstNode);
    
}VPSS_EXPORT_FUNC_S;

#define CKFN_VPSS_ENTRY()  CHECK_FUNC_ENTRY(HI_ID_VPSS)

#define CKFN_VPSS_Register() \
    (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssRegister)
#define CALL_VPSS_Register(pstInfo) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssRegister(pstInfo)

#define CKFN_VPSS_UnRegister() \
    (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssUnRegister)
#define CALL_VPSS_UnRegister(enModId) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssUnRegister(enModId)

#define CKFN_VPSS_StartVgsTask() \
    (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssStartVgsTask)
#define CALL_VPSS_StartVgsTask(pstNode) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssStartVgsTask(pstNode)
    
#endif /* __VPSS_EXT_H__ */




