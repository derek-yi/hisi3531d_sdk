/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv_fmwext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/06/16
  Description   :
  History       :
  1.Date        : 2008/06/16
    Author      :
    Modification: Created file
******************************************************************************/
#ifndef __PCIV_FMWEXT_H__
#define __PCIV_FMWEXT_H__

#include "hi_comm_pciv.h"

#define PCIVFMW_MAX_CHN_NUM 128

typedef struct hiPCIV_PIC_S
{
	HI_U32           u32Index;   /* index of pciv channnel buffer */
    HI_U32           u32Count;   /* total number of picture */
	
    HI_U32           u32PhyAddr; /* physical address of video buffer which store pcicture info */
    HI_U32           u32PoolId;  /* pool Id of video buffer which store picture info */

    HI_U64           u64Pts;     /* time stamp */
    HI_U32           u32TimeRef; /* time reference */
    PCIV_BIND_TYPE_E enSrcType;  /* bind type for pciv */
    VIDEO_FIELD_E    enFiled;    /* video field type */
	VI_MIXCAP_STAT_S stMixCapState;
    HI_BOOL          bBlock;
} PCIV_PIC_S;

typedef HI_S32 FN_PCIV_SrcSendPic(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic);
typedef HI_S32 FN_PCIV_RecvPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pRecvPic);
typedef HI_S32 FN_PCIV_GetChnShareBufState(PCIV_CHN pcivChn);

typedef struct hiPCIVFMW_CALLBACK_S
{
    FN_PCIV_SrcSendPic           *pfSrcSendPic;
    FN_PCIV_RecvPicFree          *pfRecvPicFree;
    FN_PCIV_GetChnShareBufState  *pfQueryPcivChnShareBufState;
} PCIVFMW_CALLBACK_S;

#define PCIV_FMW_TRACE(level, fmt...)\
do{ \
    HI_TRACE(level, HI_ID_PCIVFMW, "[Func]:%s [Line]:%d [Info]:", __FUNCTION__, __LINE__);\
    HI_TRACE(level, HI_ID_PCIVFMW, ##fmt);\
}while(0)

#define PCIVFMW_CHECK_CHNID(ChnID)\
do{\
    if(((ChnID) < 0) || ((ChnID) >= PCIVFMW_MAX_CHN_NUM))\
    {\
        PCIV_FMW_TRACE(HI_DBG_ERR, "invalid chn id:%d \n", ChnID);\
        return HI_ERR_PCIV_INVALID_CHNID;\
    }\
}while(0)

#define PCIVFMW_CHECK_PTR(ptr)\
do{\
    if(NULL == (ptr))\
    {\
        return HI_ERR_PCIV_NULL_PTR;\
    }\
}while(0)


HI_S32  PCIV_FirmWareCreate(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr, HI_S32 s32LocalId);
HI_S32  PCIV_FirmWareDestroy(PCIV_CHN pcivChn);
HI_S32  PCIV_FirmWareSetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr, HI_S32 s32LocalId);
HI_S32  PCIV_FirmWareStart(PCIV_CHN pcivChn);
HI_S32  PCIV_FirmWareStop(PCIV_CHN pcivChn);
HI_S32  PCIV_FirmWareMalloc(HI_U32 u32Size, HI_S32 s32LocalId, HI_U32 *pPhyAddr);
HI_S32  PCIV_FirmWareFree(HI_S32 s32LocalId, HI_U32 u32PhyAddr);
HI_S32  PCIV_FirmWareMallocChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_U32 u32Size, HI_S32 s32LocalId, HI_U32 *pPhyAddr);
HI_S32  PCIV_FirmWareFreeChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_S32 s32LocalId);
HI_S32  PCIV_FirmWareSrcPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic);
HI_S32  PCIV_FirmWareRecvPicAndSend(PCIV_CHN pcivChn, PCIV_PIC_S *pRecvPic);
HI_S32  PCIV_FirmWareWindowVbCreate(PCIV_WINVBCFG_S *pCfg);
HI_S32  PCIV_FirmWareWindowVbDestroy(HI_VOID);

HI_S32  PCIV_FirmWareRegisterFunc(PCIVFMW_CALLBACK_S *pstCallBack);

HI_S32  PCIV_FirmWareGetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr);
HI_S32  PCIV_FirmWareSetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr);


/*------------------------------------------------------------------------------------------*/


typedef struct hiPCIV_FMWEXPORT_FUNC_S
{
    HI_S32 (*pfnPcivSendPic)(VI_DEV ViDevId, VI_CHN ViChn,
        const VIDEO_FRAME_INFO_S *pstPic, const VI_MIXCAP_STAT_S *pstMixStat);
} PCIV_FMWEXPORT_FUNC_S;

#endif

