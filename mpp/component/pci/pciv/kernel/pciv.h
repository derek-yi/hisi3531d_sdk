/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/03
  Description   :
  History       :
  1.Date        : 2009/07/03
    Author      : Z44949
    Modification: Created file
  2.Date        : 2010/5/29
    Author      : P00123320
    Modification: Move struct PCIV_NOTIFY_PICEND_S from hi_comm_pciv.h to here, and modify.
******************************************************************************/

#ifndef __PCIV_H__
#define __PCIV_H__

#include <linux/list.h>
#include <linux/fs.h>

#include "hi_debug.h"
#include "hi_comm_pciv.h"
#include "pciv_fmwext.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */
#define DEFAULT_PRIVATE_DATA (0x0FFFFFFF)

typedef struct hiPCIV_SENDTASK_S
{
    struct list_head list;

    HI_BOOL bRead;
    HI_U32  u32SrcPhyAddr;
    HI_U32  u32DstPhyAddr;
    HI_U32  u32Len;
    HI_U32  u32PrvData[5];
    HI_U64  u64PrvData[2];
    HI_VOID *pvPrvData;
    void   (*pCallBack)(struct hiPCIV_SENDTASK_S *pstTask);
} PCIV_SENDTASK_S;

typedef struct hiPCIV_NOTIFY_PICEND_S
{
    PCIV_CHN       	pcivChn;       /* The PCI device to be notified */
    PCIV_PIC_S 		stPicInfo;
} PCIV_NOTIFY_PICEND_S;

typedef enum hiPCIV_MSGTYPE_E
{
    PCIV_MSGTYPE_CREATE,
    PCIV_MSGTYPE_START,
    PCIV_MSGTYPE_DESTROY,
    PCIV_MSGTYPE_SETATTR,
    PCIV_MSGTYPE_GETATTR,
    PCIV_MSGTYPE_CMDECHO,
    PCIV_MSGTYPE_WRITEDONE,
    PCIV_MSGTYPE_READDONE,
    PCIV_MSGTYPE_NOTIFY,
    PCIV_MSGTYPE_MALLOC,
    PCIV_MSGTYPE_FREE,
    PCIV_MSGTYPE_BIND,
    PCIV_MSGTYPE_UNBIND,
    PCIV_MSGTYPE_BUTT
} PCIV_MSGTYPE_E;

#define PCIV_MSG_HEADLEN (16)
#define PCIV_MSG_MAXLEN  (384 - PCIV_MSG_HEADLEN)

typedef struct hiPCIV_MSG_S
{
    HI_U32          u32Target; /* The final user of this message */
    PCIV_MSGTYPE_E  enMsgType;
    HI_U32          enDevType;
    HI_U32          u32MsgLen;
    HI_U8           cMsgBody[PCIV_MSG_MAXLEN];
} PCIV_MSG_S;

HI_S32 PCIV_Create(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr);
HI_S32 PCIV_Destroy(PCIV_CHN pcivChn);
HI_S32 PCIV_SetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr);
HI_S32 PCIV_GetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr);
HI_S32 PCIV_SetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr);
HI_S32 PCIV_GetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr);
HI_S32 PCIV_Start(PCIV_CHN pcivChn);
HI_S32 PCIV_Stop(PCIV_CHN pcivChn);
HI_S32 PCIV_Hide(PCIV_CHN pcivChn, HI_BOOL bHide);
HI_S32 PCIV_WindowVbCreate(PCIV_WINVBCFG_S *pCfg);
HI_S32 PCIV_WindowVbDestroy(HI_VOID);
HI_S32 PCIV_Malloc(HI_U32 u32Size, HI_U32 *pPhyAddr);
HI_S32 PCIV_Free(HI_U32 u32PhyAddr);
HI_S32 PCIV_MallocChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_U32 u32Size, HI_U32 *pPhyAddr);
HI_S32 PCIV_FreeChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index);
HI_S32 PCIV_UserDmaTask(PCIV_DMA_TASK_S *pTask);
HI_S32 PCIV_FreeShareBuf(PCIV_CHN pcivChn, HI_U32 u32Index, HI_U32 u32Count);
HI_S32 PCIV_FreeAllBuf(PCIV_CHN pcivChn);
HI_S32 PCIV_SrcPicSend(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic);
HI_S32 PCIV_SrcPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic);
HI_S32 PCIV_ReceivePic(PCIV_CHN pcivChn, PCIV_PIC_S *pRecvPic);
HI_S32 PCIV_RecvPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pRecvPic);
HI_S32 PCIV_Init(void);
HI_VOID  PCIV_Exit(void);
HI_S32 PCIV_ProcShow(struct osal_proc_dir_entry *s);


HI_S32 PCIV_DrvAdp_AddDmaTask(PCIV_SENDTASK_S *pTask);
HI_S32 PCIV_DrvAdp_DmaEndNotify(PCIV_REMOTE_OBJ_S *pRemoteObj, PCIV_PIC_S *pRecvPic);
HI_S32 PCIV_DrvAdp_BufFreeNotify(PCIV_REMOTE_OBJ_S *pRemoteObj, PCIV_PIC_S *pRecvPic);
HI_S32 PcivDrvAdpSendMsg(PCIV_REMOTE_OBJ_S *pRemoteObj, PCIV_MSGTYPE_E enType, PCIV_PIC_S *pRecvPic);
HI_S32 PCIV_DrvAdp_GetBaseWindow(PCIV_BASEWINDOW_S *pBaseWin);
HI_S32 PCIV_DrvAdp_GetLocalId(HI_VOID);
HI_S32 PCIV_DrvAdp_EunmChip(HI_S32 s32ChipArray[PCIV_MAX_CHIPNUM]);
HI_S32 PCIV_DrvAdp_CheckRemote(HI_S32 s32RemoteId);
HI_S32 PCIV_DrvAdp_Init(void);
HI_VOID PCIV_DrvAdp_Exit(void);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __PCIV_H__ */

