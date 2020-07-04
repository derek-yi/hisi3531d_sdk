/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv_firmware.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/16
  Description   :
  History       :
  1.Date        : 2009/07/16
    Author      : Z44949
    Modification: Created file
  2.Date        : 2009/10/13
    Author      : P00123320
    Modification: Start timer when the front is vdec
  3.Date        : 2009/10/15
    Author      : P00123320
    Modification: Add the Synchronization mechanism between VDEC and VO
  4.Date        : 2010/2/24
    Author      : P00123320
    Modification:
  5.Date        : 2010/4/14
    Author      : P00123320
    Modification:
  6.Date        : 2010/11/12
    Author      : P00123320
    Modification: Add the Osd function
******************************************************************************/

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "hi_common.h"
#include "hi_comm_pciv.h"
#include "hi_comm_region.h"
#include "hi_comm_vo.h"
#include "pciv_firmware.h"
#include "mod_ext.h"
#include "dev_ext.h"
#include "vb_ext.h"
#include "vgs_ext.h"
#include "sys_ext.h"
#include "proc_ext.h"
#include "pciv_fmwext.h"
#include "pciv_pic_queue.h"
#include "vpss_ext.h"
#include "region_ext.h"
#include "hi_ipcm_usrdev.h"

#define  PCIVFMW_STATE_STARTED   0
#define  PCIVFMW_STATE_STOPPING  1
#define  PCIVFMW_STATE_STOPED    2

/*get the function entrance*/
#define FUNC_ENTRANCE(type,id) ((type*)(g_astModules[id].pstExportFuncs))
static PCIV_VBPOOL_S     g_stVbPool;


typedef enum hiPCIVFMW_SEND_STATE_E
{
    PCIVFMW_SEND_OK = 0,
    PCIVFMW_SEND_NOK,
    PCIVFMW_SEND_ING,
    PCIVFMW_SEND_BUTT
} PCIVFMW_SEND_STATE_E;

typedef struct hiPCIV_FMWCHANNEL_S
{
    HI_BOOL  bCreate;
    HI_BOOL  bStart;
    HI_BOOL  bBlock;
	HI_BOOL  bMaster;		   /* the flag of master or not */

    HI_U32 u32RgnNum;          /* number of channel region */
    HI_U32 u32TimeRef;         /*The serial number of VI source Image*/
    HI_U32 u32GetCnt;          /*The times of sender get VI Image, or the receiver get the Image fo VO's Dispay*/
    HI_U32 u32SendCnt;         /*The times of sender send the Image, or the receiver send to VO displaying*/
    HI_U32 u32RespCnt;         /*The times of sender finish sending the Image and releasing, or the receiver finsh sending to VO displaying*/
    HI_U32 u32LostCnt;         /*The times of sender fail sending the Image , or the receiver fail sending to VO displaying*/
    HI_U32 u32TimerCnt;        /*The times of the timer runing to send VDEC Image*/

    HI_U32 u32AddJobSucCnt;    /*Success submitting the job*/
    HI_U32 u32AddJobFailCnt;   /*fail submitting the job*/

    HI_U32 u32MoveTaskSucCnt;  /*Move Task success*/
    HI_U32 u32MoveTaskFailCnt; /*Move Task fail*/

    HI_U32 u32OsdTaskSucCnt;   /*osd  task success*/
    HI_U32 u32OsdTaskFailCnt;  /*osd task  fail*/

    HI_U32 u32ZoomTaskSucCnt;  /*zoom task success*/
    HI_U32 u32ZoomTaskFailCnt; /*zoom task fail*/

    HI_U32 u32EndJobSucCnt;    /*vgs end job success*/
    HI_U32 u32EndJobFailCnt;   /*vgs end job fail*/

    HI_U32 u32MoveCbCnt;       /*vgs move callback success*/
    HI_U32 u32OsdCbCnt;        /*vgs osd callback success*/
    HI_U32 u32ZoomCbCnt;       /*vgs zoom callback success*/

    HI_U32 u32NewDoCnt;
    HI_U32 u32OldUndoCnt;

    PCIVFMW_SEND_STATE_E enSendState;
	MOD_ID_E             enModId;

    PCIV_PIC_QUEUE_S stPicQueue;   /*vdec image queue*/
    PCIV_PIC_NODE_S *pCurVdecNode; /*the current vdec node*/

	/*record the tartget Image attr after zoom*/
    PCIV_PIC_ATTR_S stPicAttr;    /*record the target Image attr of PCI transmit*/
    HI_U32          u32Offset[3];
    HI_U32          u32BlkSize;
    HI_U32          u32Count;     /* The total buffer count */
    HI_U32          au32PhyAddr[PCIV_MAX_BUF_NUM];
    HI_U32          au32PoolId[PCIV_MAX_BUF_NUM];
    VB_BLKHANDLE    vbBlkHdl[PCIV_MAX_BUF_NUM];     /*Vb handle,used to check the VB is release by VO or not*/
    HI_BOOL         bPcivHold[PCIV_MAX_BUF_NUM];     /*Buffer is been hold by the pciv queue or not*/
    VGS_ONLINE_OPT_S   stVgsOpt;
    PCIV_PREPROC_CFG_S stPreProcCfg;

    struct timer_list stBufTimer;
} PCIV_FWMCHANNEL_S;

static osal_dev_t * g_stPcivfmwumapd;
static PCIV_FWMCHANNEL_S g_stFwmPcivChn[PCIVFMW_MAX_CHN_NUM];
static struct timer_list g_timerPcivSendVdecPic;

static  spinlock_t           g_PcivFmwLock;
#define PCIVFMW_SPIN_LOCK   spin_lock_irqsave(&g_PcivFmwLock,flags)
#define PCIVFMW_SPIN_UNLOCK spin_unlock_irqrestore(&g_PcivFmwLock,flags)

#define VDEC_MAX_SEND_CNT 	6

static PCIVFMW_CALLBACK_S g_stPcivFmwCallBack;

static HI_U32 s_u32PcivFmwState = PCIVFMW_STATE_STOPED;

static int drop_err_timeref = 1;

HI_VOID PcivFirmWareRecvPicFree(unsigned long data);
HI_VOID PcivFmwPutRegion(PCIV_CHN PcivChn, RGN_TYPE_E enType);


HI_S32 PCIV_FirmWareResetChnQueue(PCIV_CHN pcivChn)
{
	HI_S32               i          = 0;
	HI_S32               s32Ret;
	HI_U32               u32BusyNum = 0;
	PCIV_PIC_NODE_S     *pNode   = NULL;
    PCIV_FWMCHANNEL_S   *pFmwChn = NULL;
	unsigned long      	flags;

	PCIVFMW_SPIN_LOCK;
    pFmwChn = &g_stFwmPcivChn[pcivChn];

	if (HI_TRUE == pFmwChn->bStart)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "pcivChn:%d has not stop, please stop it first!\n", pcivChn);
	    return HI_FAILURE;
	}
	PCIVFMW_SPIN_UNLOCK;

	/*It is in the sending proccess,wait for sending finshed*/
	while(1)
	{
		PCIVFMW_SPIN_LOCK;
		if (PCIVFMW_SEND_ING == pFmwChn->enSendState)
		{
			PCIVFMW_SPIN_UNLOCK;
			msleep(10);
			continue;
		}
		else
		{
			/*if send fail, release the vdec buffer*/
			if ((PCIVFMW_SEND_NOK == pFmwChn->enSendState) && (HI_ID_VDEC == pFmwChn->enModId))
			{
				HI_ASSERT(pFmwChn->pCurVdecNode != NULL);
				s32Ret = CALL_VB_UserSub(pFmwChn->pCurVdecNode->stPcivPic.stVideoFrame.u32PoolId, pFmwChn->pCurVdecNode->stPcivPic.stVideoFrame.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
		        HI_ASSERT(s32Ret == HI_SUCCESS);

		        PCIV_PicQueuePutFree(&pFmwChn->stPicQueue, pFmwChn->pCurVdecNode);
				pFmwChn->enSendState  = PCIVFMW_SEND_OK;
		        pFmwChn->pCurVdecNode = NULL;
			}

			break;
		}
	}

	/*put the node in the busy queue to free queue*/
	u32BusyNum = PCIV_PicQueueGetBusyNum(&pFmwChn->stPicQueue);

	for (i=0; i<u32BusyNum; i++)
	{
		pNode = PCIV_PicQueueGetBusy(&pFmwChn->stPicQueue);
		if (NULL == pNode)
		{
			PCIVFMW_SPIN_UNLOCK;
			PCIV_FMW_TRACE(HI_DBG_ERR, "PCIV_PicQueueGetBusy failed! pciv chn %d.\n", pcivChn);
			return HI_FAILURE;
		}

		if (HI_FALSE == pFmwChn->bMaster)
		{
			s32Ret = CALL_VB_UserSub(pNode->stPcivPic.stVideoFrame.u32PoolId, pNode->stPcivPic.stVideoFrame.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        	HI_ASSERT(s32Ret == HI_SUCCESS);
		}

		PCIV_PicQueuePutFree(&pFmwChn->stPicQueue, pNode);
		pNode = NULL;
	}

	PCIVFMW_SPIN_UNLOCK;

	return HI_SUCCESS;

}


HI_S32 PCIV_FirmWareCreate(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr, HI_S32 s32LocalId)
{
    HI_S32 s32Ret, i;
    HI_U32 u32NodeNum;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    if (HI_TRUE == pChn->bCreate)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d have already created \n", pcivChn);
        return HI_ERR_PCIV_EXIST;
    }

    s32Ret = PCIV_FirmWareSetAttr(pcivChn, pAttr, s32LocalId);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "attr of pciv chn%d is invalid \n", pcivChn);
        return s32Ret;
    }

    pChn->bStart             = HI_FALSE;
    pChn->bBlock             = HI_FALSE;
	/*[HSCP201307040004],l00181524 The member shoud initialize to 0,otherwise int re-creat and re-dostroy and switch the bind-ship situation,
	it may occur receive the wrong image for out-of-order*/
    pChn->u32TimeRef         = 0;
    pChn->u32SendCnt         = 0;
    pChn->u32GetCnt          = 0;
    pChn->u32RespCnt         = 0;
    pChn->u32LostCnt         = 0;
    pChn->u32NewDoCnt        = 0;
    pChn->u32OldUndoCnt      = 0;
    pChn->enSendState        = PCIVFMW_SEND_OK;
    pChn->u32TimerCnt        = 0;
    pChn->u32RgnNum          = 0;

    pChn->u32AddJobSucCnt    = 0;
    pChn->u32AddJobFailCnt   = 0;

    pChn->u32MoveTaskSucCnt  = 0;
    pChn->u32MoveTaskFailCnt = 0;

    pChn->u32OsdTaskSucCnt   = 0;
    pChn->u32OsdTaskFailCnt  = 0;

    pChn->u32ZoomTaskSucCnt  = 0;
    pChn->u32ZoomTaskFailCnt = 0;

    pChn->u32EndJobSucCnt    = 0;
    pChn->u32EndJobFailCnt   = 0;

    pChn->u32MoveCbCnt       = 0;
    pChn->u32OsdCbCnt        = 0;
    pChn->u32ZoomCbCnt       = 0;

    for (i=0; i<PCIV_MAX_BUF_NUM; i++)
    {
        pChn->bPcivHold[i] = HI_FALSE;
    }

     /*Master Chip*/
    if (0 == s32LocalId)
    {
        u32NodeNum = pAttr->u32Count;
    }
    else
    {
        u32NodeNum = VDEC_MAX_SEND_CNT;
    }

    s32Ret = PCIV_CreatPicQueue(&pChn->stPicQueue, u32NodeNum);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d create pic queue failed\n", pcivChn);
        return s32Ret;
    }
    pChn->pCurVdecNode = NULL;

    PCIVFMW_SPIN_LOCK;
    pChn->bCreate = HI_TRUE;
	if (0 == s32LocalId)
    {
		pChn->bMaster = HI_TRUE;
    }
    else
    {
		pChn->bMaster = HI_FALSE;
    }
    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d create ok \n", pcivChn);
    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareDestroy(PCIV_CHN pcivChn)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;
	HI_S32        s32Ret;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    if (HI_FALSE == pChn->bCreate)
    {
        return HI_SUCCESS;
    }
    if (HI_TRUE == pChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d is running,you should stop first \n", pcivChn);
        return HI_ERR_PCIV_NOT_PERM;
    }

	s32Ret = PCIV_FirmWareResetChnQueue(pcivChn);
	if (HI_SUCCESS != s32Ret)
	{
		PCIV_FMW_TRACE(HI_DBG_ERR, "pcivfmw chn%d stop failed!\n", pcivChn);
		return s32Ret;

	}

	PCIVFMW_SPIN_LOCK;
    PCIV_DestroyPicQueue(&pChn->stPicQueue);

    pChn->bCreate = HI_FALSE;
    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d destroy ok \n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PcivFirmWareCheckAttr(PCIV_ATTR_S *pAttr)
{
    PIXEL_FORMAT_E enPixFmt = pAttr->stPicAttr.enPixelFormat;
    HI_U32         u32BlkSize;

	/*Check The Image Width and Height*/
    if (!pAttr->stPicAttr.u32Height || !pAttr->stPicAttr.u32Width)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pic w:%u, h:%u invalid\n",
            pAttr->stPicAttr.u32Width, pAttr->stPicAttr.u32Height);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    if (pAttr->stPicAttr.u32Height%2 || pAttr->stPicAttr.u32Width%2)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pic w:%u, h:%u invalid\n",
            pAttr->stPicAttr.u32Width, pAttr->stPicAttr.u32Height);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    if (pAttr->stPicAttr.u32Stride[0] < pAttr->stPicAttr.u32Width)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pic stride0:%u,stride1:%u invalid\n",
            pAttr->stPicAttr.u32Stride[0], pAttr->stPicAttr.u32Stride[1]);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
	if ((pAttr->stPicAttr.u32Stride[0] & 0xf)||(pAttr->stPicAttr.u32Stride[1] & 0xf))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "illegal param: not align Stride(YStride:%u,CStride:%u)\n",
			pAttr->stPicAttr.u32Stride[0],pAttr->stPicAttr.u32Stride[1]);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

	/*Check the Image format and calculate the buffer size and yuv offset*/
    u32BlkSize   = pAttr->stPicAttr.u32Height * pAttr->stPicAttr.u32Stride[0];
    switch (enPixFmt)
    {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
        {
            u32BlkSize += (pAttr->stPicAttr.u32Height/2) * pAttr->stPicAttr.u32Stride[1];
            break;
        }
        case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
        {
            u32BlkSize += pAttr->stPicAttr.u32Height * pAttr->stPicAttr.u32Stride[1];
            break;
        }
        default:
        {
            PCIV_FMW_TRACE(HI_DBG_ERR, "Pixel format(%u) unsupported\n", enPixFmt);
            return HI_ERR_PCIV_ILLEGAL_PARAM;
        }
    }

	/*Check the Image attr is match or not with the buffer size*/
    if (pAttr->u32BlkSize < u32BlkSize)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Buffer block is too smail(%u < %u)\n", pAttr->u32BlkSize, u32BlkSize);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareSetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr, HI_S32 s32LocalId)
{
	HI_S32 i;
    HI_S32 s32Ret;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;

    PCIVFMW_CHECK_CHNID(pcivChn);
    PCIVFMW_CHECK_PTR(pAttr);

    pChn = &g_stFwmPcivChn[pcivChn];

    PCIVFMW_SPIN_LOCK;

	/*The channel is in the process of start,it cannot alter*/
    if (HI_TRUE == pChn->bStart)
    {
        PCIVFMW_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d is running\n", pcivChn);
        return HI_ERR_PCIV_NOT_PERM;
    }

	/*check the valid of attr*/
    s32Ret = PcivFirmWareCheckAttr(pAttr);
    if (s32Ret != HI_SUCCESS)
    {
        PCIVFMW_SPIN_UNLOCK;
        return s32Ret;
    }

    memcpy(&pChn->stPicAttr, &pAttr->stPicAttr, sizeof(PCIV_PIC_ATTR_S));
	if (0 == s32LocalId)
	{
		for (i=0; i<pAttr->u32Count; i++)
		{
			if (0 == pChn->au32PhyAddr[i])
			{
				pChn->au32PhyAddr[i] = pAttr->u32PhyAddr[i];
			}
			else
			{
				if (pChn->au32PhyAddr[i] != pAttr->u32PhyAddr[i])
				{
					PCIVFMW_SPIN_UNLOCK;
					PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn: %d, buffer address: 0x%x is invalid!\n", pcivChn, pAttr->u32PhyAddr[i]);
		        	return HI_ERR_PCIV_NOT_PERM;
				}
			}
		}
	}
	else
	{
		memcpy(pChn->au32PhyAddr, pAttr->u32PhyAddr, sizeof(pAttr->u32PhyAddr));
	}

    pChn->u32BlkSize = pAttr->u32BlkSize;
    pChn->u32Count   = pAttr->u32Count;

	/*Set the YUV offset of tartget Image */
    pChn->u32Offset[0] = 0;
    switch (pAttr->stPicAttr.enPixelFormat)
    {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
        /* fall through */
        case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
        {
            /* Sem-planar format do not need u32Offset[2](V offset) */
            pChn->u32Offset[1] = pAttr->stPicAttr.u32Stride[0] * pAttr->stPicAttr.u32Height;
            break;
        }

        default:
        {
            PCIVFMW_SPIN_UNLOCK;
            PCIV_FMW_TRACE(HI_DBG_ERR, "Pixel format(%u) unsupported\n", pAttr->stPicAttr.enPixelFormat);
            return HI_ERR_PCIV_NOT_SUPPORT;
        }
    }

    PCIVFMW_SPIN_UNLOCK;

    return HI_SUCCESS;
}

static HI_VOID PCIV_FirmWareInitPreProcCfg(PCIV_CHN pcivChn)
{
    VGS_ONLINE_OPT_S *pstVgsCfg = &g_stFwmPcivChn[pcivChn].stVgsOpt;
    PCIV_PREPROC_CFG_S *pstPreProcCfg = &g_stFwmPcivChn[pcivChn].stPreProcCfg;

    /* init VGS cfg */
    memset(pstVgsCfg, 0, sizeof(VGS_ONLINE_OPT_S));

    pstVgsCfg->bCrop       = HI_FALSE;
    pstVgsCfg->bCover      = HI_FALSE;
    pstVgsCfg->bOsd        = HI_FALSE;
    pstVgsCfg->bHSharpen   = HI_FALSE;
    pstVgsCfg->bBorder     = HI_FALSE;
    pstVgsCfg->bDci        = HI_FALSE;
    pstVgsCfg->bForceHFilt = HI_FALSE;
    pstVgsCfg->bForceVFilt = HI_FALSE;
    pstVgsCfg->bDeflicker  = HI_FALSE;

    /* init pre-process cfg */
    pstPreProcCfg->enFieldSel   = PCIV_FIELD_BOTH;
    pstPreProcCfg->enFilterType = PCIV_FILTER_TYPE_NORM;

}

HI_S32 PCIV_FirmWareSetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);

    if ((pAttr->enFieldSel >= PCIV_FIELD_BUTT) || (pAttr->enFilterType >= PCIV_FILTER_TYPE_BUTT))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "args invalid \n");
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    pChn = &g_stFwmPcivChn[pcivChn];

	/*store the pre-process configuration of user set*/
    memcpy(&pChn->stPreProcCfg, pAttr, sizeof(PCIV_PREPROC_CFG_S));

    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d set preproccfg ok\n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareGetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    memcpy(pAttr, &pChn->stPreProcCfg, sizeof(PCIV_PREPROC_CFG_S));

    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareStart(PCIV_CHN pcivChn)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    if (pChn->bCreate != HI_TRUE)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d not create\n", pcivChn);
        return HI_ERR_PCIV_UNEXIST;
    }

    PCIVFMW_SPIN_LOCK;
    pChn->bStart = HI_TRUE;
    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d start ok \n", pcivChn);
    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareStop(PCIV_CHN pcivChn)
{
    unsigned long flags;
    PCIVFMW_CHECK_CHNID(pcivChn);

    PCIVFMW_SPIN_LOCK;
    g_stFwmPcivChn[pcivChn].bStart = HI_FALSE;

    if (0 != g_stFwmPcivChn[pcivChn].u32RgnNum)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "Region number of channel %d is %u, now free the region!\n", pcivChn, g_stFwmPcivChn[pcivChn].u32RgnNum);
        PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
        g_stFwmPcivChn[pcivChn].u32RgnNum = 0;
    }

    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pcivfmw chn%d stop ok \n", pcivChn);

    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareWindowVbCreate(PCIV_WINVBCFG_S *pCfg)
{
    HI_S32 	s32Ret, i;
    HI_U32 	u32PoolId;
	HI_CHAR *pBufName = "PcivVbFromWindow";

    if( g_stVbPool.u32PoolCount != 0)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Video buffer pool has created\n");
        return HI_ERR_PCIV_BUSY;
    }

    for(i=0; i < pCfg->u32PoolCount; i++)
    {
        s32Ret = CALL_VB_CreatePool(&u32PoolId, pCfg->u32BlkCount[i],
                                         pCfg->u32BlkSize[i], RESERVE_MMZ_NAME, pBufName, VB_REMAP_MODE_NONE);
        if(HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "Create pool(Index=%d, Cnt=%u, Size=%u) fail\n",
                                  i, pCfg->u32BlkCount[i], pCfg->u32BlkSize[i]);
            break;
        }
        g_stVbPool.u32PoolCount = i + 1;
        g_stVbPool.u32PoolId[i] = u32PoolId;
        g_stVbPool.u32Size[i]   = pCfg->u32BlkSize[i];
    }

	/*if one pool created not success, then rollback*/
    if ( g_stVbPool.u32PoolCount != pCfg->u32PoolCount)
    {
        for(i=0; i < g_stVbPool.u32PoolCount; i++)
        {
            (HI_VOID)CALL_VB_DestroyPool(g_stVbPool.u32PoolId[i]);
            g_stVbPool.u32PoolId[i] = VB_INVALID_POOLID;
        }

        g_stVbPool.u32PoolCount = 0;

        return HI_ERR_PCIV_NOMEM;
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareWindowVbDestroy(HI_VOID)
{
    HI_S32 i;

    for(i=0; i < g_stVbPool.u32PoolCount; i++)
    {
        (HI_VOID)CALL_VB_DestroyPool(g_stVbPool.u32PoolId[i]);
        g_stVbPool.u32PoolId[i] = VB_INVALID_POOLID;
    }

    g_stVbPool.u32PoolCount = 0;
    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareMalloc(HI_U32 u32Size, HI_S32 s32LocalId, HI_U32 *pPhyAddr)
{
    HI_S32       s32Ret = HI_SUCCESS;
    HI_S32       i;
    HI_U32       u32PoolId;
    VB_BLKHANDLE handle = VB_INVALID_HANDLE;

    HI_CHAR azMmzName[MAX_MMZ_NAME_LEN] = {0};

    if(s32LocalId == 0)
    {
        handle = CALL_VB_GetBlkBySize(u32Size, VB_UID_PCIV, azMmzName);
        if(VB_INVALID_HANDLE == handle)
        {
            PCIV_FMW_TRACE(HI_DBG_ERR,"CALL_VB_GetBlkBySize fail,size:%d!\n", u32Size);
            return HI_ERR_PCIV_NOBUF;
        }

        *pPhyAddr = CALL_VB_Handle2Phys(handle);

        u32PoolId = CALL_VB_Handle2PoolId(handle);

        s32Ret = CALL_VB_UserAdd(u32PoolId,*pPhyAddr,VB_UID_USER);
        HI_ASSERT(HI_SUCCESS == s32Ret);

        return HI_SUCCESS;
    }

	/*if in the slave chip, then alloc buffer from special VB*/
    for(i=0; i < g_stVbPool.u32PoolCount; i++)
    {
        if(u32Size > g_stVbPool.u32Size[i]) continue;

        handle = CALL_VB_GetBlkByPoolId(g_stVbPool.u32PoolId[i], VB_UID_PCIV);

        if(VB_INVALID_HANDLE != handle) break;
    }

    if(VB_INVALID_HANDLE == handle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR,"CALL_VB_GetBlkBySize fail,size:%d!\n", u32Size);
        return HI_ERR_PCIV_NOBUF;
    }

    *pPhyAddr = CALL_VB_Handle2Phys(handle);
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareFree(HI_S32 s32LocalId, HI_U32 u32PhyAddr)
{
	HI_S32        s32Ret = HI_SUCCESS;
    VB_BLKHANDLE  vbHandle;
	unsigned long flags;

	PCIVFMW_SPIN_LOCK;
    vbHandle = CALL_VB_Phy2Handle(u32PhyAddr);
    if(VB_INVALID_HANDLE == vbHandle)
    {
    	PCIVFMW_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR, "Invalid Physical Address 0x%08x\n", u32PhyAddr);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    if(CALL_VB_InquireOneUserCnt(vbHandle,VB_UID_USER) > 0)
    {
    s32Ret = CALL_VB_UserSub(CALL_VB_Handle2PoolId(vbHandle), u32PhyAddr, VB_UID_PCIV);
       s32Ret |= CALL_VB_UserSub(CALL_VB_Handle2PoolId(vbHandle), u32PhyAddr, VB_UID_USER);
    }

	PCIVFMW_SPIN_UNLOCK;

	return s32Ret;
}


HI_S32 PCIV_FirmWareMallocChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_U32 u32Size, HI_S32 s32LocalId, HI_U32 *pPhyAddr)
{
    VB_BLKHANDLE       handle = VB_INVALID_HANDLE;
    HI_CHAR            azMmzName[MAX_MMZ_NAME_LEN] = {0};
	unsigned long      flags;
	PCIV_FWMCHANNEL_S *pFmwChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);

	PCIVFMW_SPIN_LOCK;
	if (0 != s32LocalId)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "Slave Chip: %d, doesn't need chn buffer!\n", s32LocalId);
		return HI_ERR_PCIV_NOT_PERM;
	}

    pFmwChn = &g_stFwmPcivChn[pcivChn];
	if (HI_TRUE == pFmwChn->bStart)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv Chn: %d, has malloc chn buffer!\n", pcivChn);
		return HI_ERR_PCIV_NOT_PERM;
	}
	
    handle = CALL_VB_GetBlkBySize(u32Size, VB_UID_PCIV, azMmzName);
    if(VB_INVALID_HANDLE == handle)
    {
    	PCIVFMW_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR,"CALL_VB_GetBlkBySize fail,size:%d!\n", u32Size);
        return HI_ERR_PCIV_NOBUF;
    }

    *pPhyAddr = CALL_VB_Handle2Phys(handle);	
	pFmwChn->au32PhyAddr[u32Index] = *pPhyAddr;
	PCIVFMW_SPIN_UNLOCK;

    return HI_SUCCESS;

}


HI_S32 PCIV_FirmWareFreeChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_S32 s32LocalId)
{
	HI_S32             s32Ret = HI_SUCCESS;
    VB_BLKHANDLE       vbHandle;
	unsigned long      flags;
	PCIV_FWMCHANNEL_S *pFmwChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);

    PCIVFMW_SPIN_LOCK;
	if (0 != s32LocalId)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "Slave Chip: %d, has no chn buffer, doesn't need to free chn buffer!\n", s32LocalId);
		return HI_ERR_PCIV_NOT_PERM;
	}

	pFmwChn = &g_stFwmPcivChn[pcivChn];
	if (HI_TRUE == pFmwChn->bStart)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv Chn: %d has not stopped! Please stop it first!\n", pcivChn);
		return HI_ERR_PCIV_NOT_PERM;
	}

    vbHandle = CALL_VB_Phy2Handle(pFmwChn->au32PhyAddr[u32Index]);
    if(VB_INVALID_HANDLE == vbHandle)
    {
    	PCIVFMW_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR, "Invalid Physical Address 0x%08x\n", pFmwChn->au32PhyAddr[u32Index]);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

	PCIV_FMW_TRACE(HI_DBG_WARN, "start to free buffer, PoolId: %d, u32PhyAddr: 0x%x.\n", CALL_VB_Handle2PoolId(vbHandle), pFmwChn->au32PhyAddr[u32Index]);
    s32Ret = CALL_VB_UserSub(CALL_VB_Handle2PoolId(vbHandle), pFmwChn->au32PhyAddr[u32Index], VB_UID_PCIV);
	PCIV_FMW_TRACE(HI_DBG_WARN, "finish to free buffer, PoolId: %d, u32PhyAddr: 0x%x.\n", CALL_VB_Handle2PoolId(vbHandle), pFmwChn->au32PhyAddr[u32Index]);
	PCIVFMW_SPIN_UNLOCK;

	return s32Ret;
}


HI_S32 PCIV_FirmWarePutPicToQueue(PCIV_CHN pcivChn, const VIDEO_FRAME_INFO_S *pstVideoFrmInfo, HI_U32 u32Index, HI_BOOL bBlock)
{
    PCIV_PIC_NODE_S     *pNode   = NULL;
    PCIV_FWMCHANNEL_S   *pFmwChn = NULL;

    pFmwChn = &g_stFwmPcivChn[pcivChn];

	if(HI_TRUE != pFmwChn->bStart)
    {
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

    pFmwChn->bPcivHold[u32Index] = HI_TRUE;
    pNode = PCIV_PicQueueGetFree(&pFmwChn->stPicQueue);
    if (NULL == pNode)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pcivChn:%d no free node\n", pcivChn);
        return HI_FAILURE;
    }

    pNode->stPcivPic.enModId  = HI_ID_VDEC;
    pNode->stPcivPic.bBlock   = bBlock;
    pNode->stPcivPic.u32Index = u32Index;
    memcpy(&pNode->stPcivPic.stVideoFrame, pstVideoFrmInfo, sizeof(VIDEO_FRAME_INFO_S));

    PCIV_PicQueuePutBusy(&pFmwChn->stPicQueue, pNode);

    return HI_SUCCESS;

}

HI_S32 PCIV_FirmWareGetPicFromQueueAndSend(PCIV_CHN pcivChn)
{
    HI_S32               s32Ret   = HI_SUCCESS;
    HI_S32               s32DevId = 0;
    PCIV_FWMCHANNEL_S   *pFmwChn  = NULL;
    VIDEO_FRAME_INFO_S  *pstVFrameInfo = NULL;

    pFmwChn = &g_stFwmPcivChn[pcivChn];

	if(HI_TRUE != pFmwChn->bStart)
    {
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

	/*send the data in cycle queue, until the data in queue is not less or send fail */
    while(PCIV_PicQueueGetBusyNum(&pFmwChn->stPicQueue))
    {
        pFmwChn->pCurVdecNode = PCIV_PicQueueGetBusy(&pFmwChn->stPicQueue);
        if (NULL == pFmwChn->pCurVdecNode)
        {
            PCIV_FMW_TRACE(HI_DBG_ERR, "pcivChn:%d busy list is empty, no vdec pic\n", pcivChn);
            return HI_FAILURE;
        }

		/*send the vdec image to vpss or venc or vo, if success, put the node to fee queue, else nothing to do
		it will send by the time or next interrupt*/
        pFmwChn->enSendState  = PCIVFMW_SEND_ING;
        pstVFrameInfo         = &pFmwChn->pCurVdecNode->stPcivPic.stVideoFrame;
        PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, start send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
        s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, pcivChn, pFmwChn->pCurVdecNode->stPcivPic.bBlock, MPP_DATA_VDEC_FRAME, pstVFrameInfo);
        if ((HI_SUCCESS != s32Ret) && (HI_TRUE == pFmwChn->pCurVdecNode->stPcivPic.bBlock))
        {
			/* bBlock is ture(playback mode),if failed,get out of the circle,do nothing ,it will send by the timer or next DMA interrupt*/
            /* set the point NULL,put the node to the head of busy,whie send again ,get it from the header of busy */
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
			PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic failed, put to queue and send again. s32Ret: 0x%x\n", pcivChn, s32Ret);
			PCIV_PicQueuePutBusyHead(&pFmwChn->stPicQueue, pFmwChn->pCurVdecNode);
            pFmwChn->enSendState  = PCIVFMW_SEND_NOK;
            pFmwChn->pCurVdecNode = NULL;
            s32Ret                = HI_SUCCESS;
            break;
        }
        else
        {
			/*bBlock is true(playback mode),if success, put the node to free*/
			/*bBlock is false(preview mode),no matter success or not, put the node to free,and do not send the Image again*/
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
			PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic ok\n", pcivChn);
			HI_ASSERT(pFmwChn->pCurVdecNode != NULL);
            pFmwChn->bPcivHold[pFmwChn->pCurVdecNode->stPcivPic.u32Index] = HI_FALSE;
            PCIV_PicQueuePutFree(&pFmwChn->stPicQueue, pFmwChn->pCurVdecNode);
            pFmwChn->enSendState  = PCIVFMW_SEND_OK;
            pFmwChn->pCurVdecNode = NULL;
        }
    }

    return s32Ret;
}

HI_S32 PCIV_FirmWareReceiverSendVdecPic(PCIV_CHN pcivChn, VIDEO_FRAME_INFO_S  *pstVideoFrmInfo, HI_U32 u32Index, HI_BOOL bBlock)
{
    HI_S32               s32Ret;
    HI_S32               s32DevId = 0;
    HI_S32               s32ChnId = 0;
    HI_U32               u32BusyNum = 0;
    PCIV_FWMCHANNEL_S   *pFmwChn = NULL;

    s32ChnId = pcivChn;
    pFmwChn = &g_stFwmPcivChn[pcivChn];

	if(HI_TRUE != pFmwChn->bStart)
    {
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

    u32BusyNum = PCIV_PicQueueGetBusyNum(&pFmwChn->stPicQueue);
    if (0 != u32BusyNum)
    {
		/*if the current queue has data, put the Image to the tail of the queue*/
        s32Ret = PCIV_FirmWarePutPicToQueue(pcivChn, pstVideoFrmInfo, u32Index, bBlock);
        if (HI_SUCCESS != s32Ret)
        {
            return HI_FAILURE;
        }

		/*Get the data from the header to send*/
        s32Ret = PCIV_FirmWareGetPicFromQueueAndSend(pcivChn);
        if (HI_SUCCESS != s32Ret)
        {
            return s32Ret;
        }
    }
    else
    {
		/*if the current queue has no data, send the current Image directly, if success,return success,else return failure,put the Image to the tail of the queue*/
		PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, start send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
		pFmwChn->bPcivHold[u32Index] = HI_TRUE;
		s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, s32ChnId, bBlock, MPP_DATA_VDEC_FRAME, pstVideoFrmInfo);
        if ((HI_SUCCESS != s32Ret) && (HI_TRUE == bBlock))
        {
			/*bBlock is true(playback mode),if failure,put the Image to the tail of the queue*/
			/*bBlock is true(preview mode),no matter success or not, we think it as success, do not put it to queue to send again*/
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
			PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic failed, put to queue and send again. s32Ret: 0x%x\n", pcivChn, s32Ret);
			if (PCIV_FirmWarePutPicToQueue(pcivChn, pstVideoFrmInfo, u32Index, bBlock))
            {
                return HI_FAILURE;
            }
            s32Ret = HI_SUCCESS;
        }
		else
		{
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
			PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic ok\n", pcivChn);
			pFmwChn->bPcivHold[u32Index] = HI_FALSE;
		}
    }

    return s32Ret;
}


HI_S32 PCIV_FirmWareRecvPicAndSend(PCIV_CHN pcivChn, PCIV_PIC_S *pRecvPic)
{
    HI_S32               s32Ret;
    VIDEO_FRAME_INFO_S   stVideoFrmInfo;
    VI_FRAME_INFO_S      stViFrmInfo;
    PCIV_FWMCHANNEL_S   *pFmwChn;
    VIDEO_FRAME_S       *pVfrm = NULL;
    unsigned long        flags;
    HI_S32               s32DevId = 0;

    HI_S32 s32ChnId = pcivChn;

    PCIVFMW_CHECK_CHNID(pcivChn);

    PCIVFMW_SPIN_LOCK;
    pFmwChn = &g_stFwmPcivChn[pcivChn];
    if(HI_TRUE != pFmwChn->bStart)
    {
        PCIVFMW_SPIN_UNLOCK;
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

    pFmwChn->u32GetCnt++;
    s32Ret = HI_SUCCESS;


    if (PCIV_BIND_VI == pRecvPic->enSrcType)
    {
        memset(&stViFrmInfo, 0, sizeof(stViFrmInfo));
        stViFrmInfo.stViFrmInfo.u32PoolId       = CALL_VB_Handle2PoolId(CALL_VB_Phy2Handle(pFmwChn->au32PhyAddr[pRecvPic->u32Index]));
        pVfrm                                   = &stViFrmInfo.stViFrmInfo.stVFrame;
        pVfrm->u32Width                         = pFmwChn->stPicAttr.u32Width;
        pVfrm->u32Height                        = pFmwChn->stPicAttr.u32Height;
        pVfrm->u32Field                         = pRecvPic->enFiled;
        pVfrm->enPixelFormat                    = pFmwChn->stPicAttr.enPixelFormat;
        pVfrm->u32PhyAddr[0]                    = pFmwChn->au32PhyAddr[pRecvPic->u32Index];
        pVfrm->u32PhyAddr[1]                    = pFmwChn->au32PhyAddr[pRecvPic->u32Index] + pFmwChn->u32Offset[1];
        pVfrm->u32PhyAddr[2]                    = pFmwChn->au32PhyAddr[pRecvPic->u32Index] + pFmwChn->u32Offset[2];
        pVfrm->u32Stride[0]                     = pFmwChn->stPicAttr.u32Stride[0];
        pVfrm->u32Stride[1]                     = pFmwChn->stPicAttr.u32Stride[1];
        pVfrm->u32Stride[2]                     = pFmwChn->stPicAttr.u32Stride[2];
        pVfrm->u64pts                           = pRecvPic->u64Pts;
        pVfrm->u32TimeRef                       = pRecvPic->u32TimeRef;
        stViFrmInfo.stMixCapState.bHasDownScale = pRecvPic->stMixCapState.bHasDownScale;
        stViFrmInfo.stMixCapState.bMixCapMode   = pRecvPic->stMixCapState.bMixCapMode;

        s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, s32ChnId, pRecvPic->bBlock, MPP_DATA_VIU_FRAME, &stViFrmInfo);
    }
    else if (PCIV_BIND_VDEC == pRecvPic->enSrcType || PCIV_BIND_VO == pRecvPic->enSrcType)
    {
        memset(&stVideoFrmInfo, 0, sizeof(stVideoFrmInfo));
        stVideoFrmInfo.u32PoolId = CALL_VB_Handle2PoolId(CALL_VB_Phy2Handle(pFmwChn->au32PhyAddr[pRecvPic->u32Index]));
        pVfrm                    = &stVideoFrmInfo.stVFrame;
        pVfrm->u32Width          = pFmwChn->stPicAttr.u32Width;
        pVfrm->u32Height         = pFmwChn->stPicAttr.u32Height;
        pVfrm->u32Field          = pRecvPic->enFiled;
        pVfrm->enPixelFormat     = pFmwChn->stPicAttr.enPixelFormat;
        pVfrm->u32PhyAddr[0]     = pFmwChn->au32PhyAddr[pRecvPic->u32Index];
        pVfrm->u32PhyAddr[1]     = pFmwChn->au32PhyAddr[pRecvPic->u32Index] + pFmwChn->u32Offset[1];
        pVfrm->u32PhyAddr[2]     = pFmwChn->au32PhyAddr[pRecvPic->u32Index] + pFmwChn->u32Offset[2];
        pVfrm->u32Stride[0]      = pFmwChn->stPicAttr.u32Stride[0];
        pVfrm->u32Stride[1]      = pFmwChn->stPicAttr.u32Stride[1];
        pVfrm->u32Stride[2]      = pFmwChn->stPicAttr.u32Stride[2];
        pVfrm->u64pts            = pRecvPic->u64Pts;
        pVfrm->u32TimeRef        = pRecvPic->u32TimeRef;

        if (PCIV_BIND_VDEC == pRecvPic->enSrcType)
        {
            /*When the DMA arrive,first query the queue if has data or not, if yes,send the data in the queue first,if not,send the cuurent Image directly; if success, return,
			else put the node queue,and trig by the timer next time*/
            s32Ret = PCIV_FirmWareReceiverSendVdecPic(pcivChn, &stVideoFrmInfo, pRecvPic->u32Index, pRecvPic->bBlock);

        }
        else
        {
            s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, s32ChnId, pRecvPic->bBlock, MPP_DATA_VOU_FRAME, &stVideoFrmInfo);
        }

    }
    else
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv chn %d bind type error, type value: %u.\n", pcivChn, pRecvPic->enSrcType);
    }

    if((HI_SUCCESS != s32Ret) && (HI_ERR_VO_CHN_NOT_ENABLE != s32Ret) )
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv chn %d send failed, ret:0x%x\n", pcivChn, s32Ret);
        pFmwChn->u32LostCnt++;
    }
    else
    {
        pFmwChn->u32SendCnt++;
    }

    /* HSCP201306030001 when start timer, PcivFirmWareVoPicFree not have lock synchronous
       int the PcivFirmWareVoPicFree function, need separate lock */
    PCIVFMW_SPIN_UNLOCK;
    PcivFirmWareRecvPicFree(pcivChn);

    return s32Ret;
}


/*After vo displaying and vpss and venc used,the function register by PCIV or FwmDccs mode is been called*/
HI_VOID PcivFirmWareRecvPicFree(unsigned long data)
{
    HI_U32   			i = 0;
	HI_U32   			u32Count = 0;
    HI_S32   			s32Ret;
    HI_BOOL  			bHit    = HI_FALSE;
    PCIV_FWMCHANNEL_S 	*pFmwChn;
    PCIV_CHN 			pcivChn = (PCIV_CHN)data;
    PCIV_PIC_S 			stRecvPic;
    unsigned long 		flags;

    PCIVFMW_SPIN_LOCK;
    pFmwChn = &g_stFwmPcivChn[pcivChn];

    if (pFmwChn->bStart != HI_TRUE)
    {
        PCIVFMW_SPIN_UNLOCK;
        return;
    }

    for(i=0; i<pFmwChn->u32Count; i++)
    {
		/*if bPcivHold is false,only when the VB count occupied by vo/vpss/venc is 0,the VB can release*/
        if(CALL_VB_InquireOneUserCnt(CALL_VB_Phy2Handle(pFmwChn->au32PhyAddr[i]), VB_UID_VOU) != 0)
        {
            continue;
        }
        if(CALL_VB_InquireOneUserCnt(CALL_VB_Phy2Handle(pFmwChn->au32PhyAddr[i]), VB_UID_VPSS) != 0)
        {
            continue;
        }
        if(CALL_VB_InquireOneUserCnt(CALL_VB_Phy2Handle(pFmwChn->au32PhyAddr[i]), VB_UID_GRP) != 0)
        {
            continue;
        }
        if (HI_TRUE == pFmwChn->bPcivHold[i])
        {
            continue;
        }

		/*The function register by the PCIV is called to handle the action afte the VO displaying or vpss using and venc coding*/
        if(g_stPcivFmwCallBack.pfRecvPicFree)
        {
            stRecvPic.u32Index = i;      /*the index of buffer can release*/
            stRecvPic.u64Pts   = 0;       /* PTS */
            stRecvPic.u32Count = u32Count;/* not used*/
            s32Ret = g_stPcivFmwCallBack.pfRecvPicFree(pcivChn, &stRecvPic);
            if (s32Ret != HI_SUCCESS)
            {
                //PCIV_FMW_TRACE(HI_DBG_ERR, "pcivfmw chn%d pfRecvPicFree failed with:0x%x.\n", pcivChn, s32Ret);
                continue;
            }
            bHit = HI_TRUE;
            pFmwChn->u32RespCnt++;
        }
    }

	/*if the buffer has not release by vo/vpss/venc,then start the time after 10ms to check*/
    if(bHit != HI_TRUE)
    {
        pFmwChn->stBufTimer.function = PcivFirmWareRecvPicFree;
        pFmwChn->stBufTimer.data     = data;
        mod_timer(&(pFmwChn->stBufTimer), jiffies + 1);
    }
    PCIVFMW_SPIN_UNLOCK;

    return ;
}


/*After transmit, release the Image buffer after VGS Zoom*/
HI_S32 PCIV_FirmWareSrcPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic)
{
    HI_S32 s32Ret;

    g_stFwmPcivChn[pcivChn].u32RespCnt++;

	/*if the mpp is deinit,the the sys will release the buffer*/
    if (PCIVFMW_STATE_STOPED == s_u32PcivFmwState)
    {
        return HI_SUCCESS;
    }

    PCIV_FMW_TRACE(HI_DBG_DEBUG,"- --> addr:0x%x\n", pSrcPic->u32PhyAddr);
    s32Ret = CALL_VB_UserSub(pSrcPic->u32PoolId, pSrcPic->u32PhyAddr, VB_UID_PCIV);
    //HI_ASSERT(HI_SUCCESS == s32Ret);
    return s32Ret;
}

static HI_S32 PcivFmwSrcPicSend(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pBindObj,
  const VIDEO_FRAME_INFO_S *pstVFrame, const VIDEO_FRAME_INFO_S *pstVdecFrame, VI_MIXCAP_STAT_S *pstMixCapState)
{
    HI_S32 s32Ret;
    PCIV_FWMCHANNEL_S *pChn = &g_stFwmPcivChn[pcivChn];
    PCIV_PIC_S stSrcPic;
    unsigned long flags;

    stSrcPic.u32PoolId  = pstVFrame->u32PoolId;
    stSrcPic.u32PhyAddr = pstVFrame->stVFrame.u32PhyAddr[0];
    stSrcPic.u64Pts     = pstVFrame->stVFrame.u64pts;
    stSrcPic.u32TimeRef = pstVFrame->stVFrame.u32TimeRef;
    stSrcPic.enFiled    = pstVFrame->stVFrame.u32Field;
    stSrcPic.enSrcType  = pBindObj->enType;
    stSrcPic.bBlock     = pChn->bBlock;
	stSrcPic.stMixCapState.bMixCapMode   = pstMixCapState->bMixCapMode;
	stSrcPic.stMixCapState.bHasDownScale = pstMixCapState->bHasDownScale;

    PCIVFMW_SPIN_LOCK;

    if (pChn->bStart != HI_TRUE)
    {
        PCIVFMW_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        return HI_FAILURE;
    }
    PCIVFMW_SPIN_UNLOCK;

	/*the callback function register by the upper mode is called to send the zoom Image by VGS*/
    s32Ret = g_stPcivFmwCallBack.pfSrcSendPic(pcivChn, &stSrcPic);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d pfSrcSendPic failed! s32Ret: 0x%x.\n", pcivChn, s32Ret);
        return HI_FAILURE;
    }

	/*if success,add the VB( release int PCIV_FirmWareSrcPicFree)*/
    s32Ret = CALL_VB_UserAdd(pstVFrame->u32PoolId, pstVFrame->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
	HI_ASSERT(HI_SUCCESS == s32Ret);

    if ((NULL != pstVdecFrame) && (PCIV_BIND_VDEC == pBindObj->enType) && (HI_FALSE == pBindObj->bVpssSend))
    {
		/*if success,the Image send by vdec directly bind PCIV need release here */
        PCIVFMW_SPIN_LOCK;
        s32Ret = CALL_VB_UserSub(pstVdecFrame->u32PoolId, pstVdecFrame->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        HI_ASSERT(s32Ret == HI_SUCCESS);

        HI_ASSERT(pChn->pCurVdecNode != NULL);
        PCIV_PicQueuePutFree(&pChn->stPicQueue, pChn->pCurVdecNode);
        pChn->pCurVdecNode = NULL;
        PCIVFMW_SPIN_UNLOCK;
    }

    pChn->u32SendCnt++;

    return HI_SUCCESS;
}

HI_S32 PcivFmwGetRegion(PCIV_CHN PcivChn, RGN_TYPE_E enType, RGN_INFO_S *pstRgnInfo)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;

    if ((!CKFN_RGN()) || (!CKFN_RGN_GetRegion()))
    {
        return HI_FAILURE;
    }

    stChn.enModId  = HI_ID_PCIV;
    stChn.s32ChnId = PcivChn;
    stChn.s32DevId = 0;
    s32Ret = CALL_RGN_GetRegion(enType, &stChn, pstRgnInfo);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    return s32Ret;
}

HI_VOID PcivFmwPutRegion(PCIV_CHN PcivChn, RGN_TYPE_E enType)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;

    if ((!CKFN_RGN()) || (!CKFN_RGN_PutRegion()))
    {
        return;
    }

    stChn.enModId  = HI_ID_PCIV;
    stChn.s32ChnId = PcivChn;
    stChn.s32DevId = 0;

	s32Ret = CALL_RGN_PutRegion(enType, &stChn);
    HI_ASSERT(HI_SUCCESS == s32Ret);
    return;
}

static HI_VOID PcivFmwSrcPicZoomCb(MOD_ID_E enCallModId, HI_U32 u32CallDevId, const VGS_TASK_DATA_S *pstVgsTask)
{

    HI_S32                   s32Ret;
    PCIV_CHN                 pcivChn;
    PCIV_BIND_OBJ_S          stBindObj = {0};
    VI_MIXCAP_STAT_S         stMixCapState;
    PCIV_FWMCHANNEL_S        *pFmwChn = NULL;
    const VIDEO_FRAME_INFO_S *pstImgIn = NULL;
    const VIDEO_FRAME_INFO_S *pstImgOut = NULL;
    unsigned long            flags;

    if (NULL == pstVgsTask)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "In Function PcivFmwSrcPicZoomCb: pstVgsTask is null, return!\n");
        return;
    }

    pstImgIn  = &pstVgsTask->stImgIn;
    pstImgOut = &pstVgsTask->stImgOut;
    pcivChn   = pstVgsTask->u32CallChnId;
    HI_ASSERT((pcivChn >= 0) && (pcivChn < PCIVFMW_MAX_CHN_NUM));
    pFmwChn   = &g_stFwmPcivChn[pcivChn];

    pFmwChn->u32ZoomCbCnt++;

    CALL_VB_UserSub(pstImgIn->u32PoolId, pstImgIn->stVFrame.u32PhyAddr[0], VB_UID_PCIV);

    stBindObj.enType    = pstVgsTask->au32privateData[0];
    stBindObj.bVpssSend = pstVgsTask->au32privateData[1];

	/*In VGS interrupt,maybe the pcivchn is stopped*/
    if (HI_TRUE != pFmwChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

   /***********************************************************************************
     * [HSCP201308020003] l00181524,2013.08.16,if the task fail,maybe the action of cancle job occur in pciv ower interrupt
     * maybe out of its ower job,it need judge next step, we cannot add lock,else maybe lock itself and other abnormal
     ************************************************************************************/
     /*if the vgs task fail,then retuen with failure*/
    if (VGS_TASK_FNSH_STAT_OK != pstVgsTask->enFinishStat)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "PcivFmwSrcPicZoomCb vgs task finish status is no ok, chn:%d\n", pcivChn);
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            //PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            //PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

    stMixCapState.bMixCapMode   = pstVgsTask->au32privateData[2];
    stMixCapState.bHasDownScale = pstVgsTask->au32privateData[3];

	/*send the video after zoom*/
    s32Ret = PcivFmwSrcPicSend(pcivChn, &stBindObj, pstImgOut, pstImgIn, &stMixCapState);
    if (HI_SUCCESS != s32Ret)
    {
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }
    if (PCIV_BIND_VDEC == stBindObj.enType)
    {
        PCIVFMW_SPIN_LOCK;
        pFmwChn->enSendState = PCIVFMW_SEND_OK;
        PCIVFMW_SPIN_UNLOCK;
    }

    if (0 != pFmwChn->u32RgnNum)
    {
        pFmwChn->u32RgnNum = 0;
    }

out:
	/*no matter success or not,in this callback ,the output vb will released*/
    s32Ret = CALL_VB_UserSub(pstImgOut->u32PoolId, pstImgOut->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    return;
}


static HI_S32 PcivFmwSrcPicZoom(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pObj, const VIDEO_FRAME_INFO_S *pstSrcFrame, VI_MIXCAP_STAT_S *pstMixCapState)
{
    HI_S32            s32Ret;
    MPP_CHN_S         stChn;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    VIDEO_FRAME_S     *pstOutFrame = NULL;
    HI_VOID           *pMmzName = HI_NULL;
    VB_BLKHANDLE      VbHandle;
    VGS_HANDLE        VgsHandle;
    VGS_TASK_DATA_S   stVgsTask;
    VGS_JOB_DATA_S    JobData;
    VGS_EXPORT_FUNC_S *pfnVgsExpFunc = (VGS_EXPORT_FUNC_S *)CMPI_GetModuleFuncById(HI_ID_VGS);

    pChn = &g_stFwmPcivChn[pcivChn];

	/*configure the input video frame*/
    memcpy(&stVgsTask.stImgIn, pstSrcFrame, sizeof(VIDEO_FRAME_INFO_S));

	/*Get the output Image VB after VGS zoom*/
    stChn.enModId  = HI_ID_PCIV;
    stChn.s32DevId = 0;
    stChn.s32ChnId = pcivChn;
    CALL_SYS_GetMmzName(&stChn, &pMmzName);

    VbHandle = CALL_VB_GetBlkBySize(pChn->u32BlkSize, VB_UID_PCIV, pMmzName);
    if (VB_INVALID_HANDLE == VbHandle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Get VB(%dByte) fail\n",pChn->u32BlkSize);
        return HI_FAILURE;
    }
    PCIV_FMW_TRACE(HI_DBG_DEBUG, "+ --> addr:0x%x\n", CALL_VB_Handle2Phys(VbHandle));

    /*config the output video info*/
    stVgsTask.stImgOut.u32PoolId = CALL_VB_Handle2PoolId(VbHandle);
    pstOutFrame                  = &stVgsTask.stImgOut.stVFrame;
    pstOutFrame->u32Width        = pChn->stPicAttr.u32Width;
    pstOutFrame->u32Height       = pChn->stPicAttr.u32Height;
    pstOutFrame->u32Field        = stVgsTask.stImgIn.stVFrame.u32Field;
    pstOutFrame->enPixelFormat   = pChn->stPicAttr.enPixelFormat;
    pstOutFrame->u32PhyAddr[0]   = CALL_VB_Handle2Phys(VbHandle);
    pstOutFrame->u32PhyAddr[1]   = pstOutFrame->u32PhyAddr[0] + pChn->u32Offset[1];
    pstOutFrame->u32PhyAddr[2]   = pstOutFrame->u32PhyAddr[0] + pChn->u32Offset[2];
    pstOutFrame->pVirAddr[0]     = HI_NULL;
    pstOutFrame->pVirAddr[1]     = HI_NULL;
    pstOutFrame->pVirAddr[2]     = HI_NULL;
    pstOutFrame->u32Stride[0]    = pChn->stPicAttr.u32Stride[0];
    pstOutFrame->u32Stride[1]    = pChn->stPicAttr.u32Stride[1];
    pstOutFrame->u32Stride[2]    = pChn->stPicAttr.u32Stride[2];
    pstOutFrame->u64pts          = stVgsTask.stImgIn.stVFrame.u64pts;
    pstOutFrame->u32TimeRef      = stVgsTask.stImgIn.stVFrame.u32TimeRef;
    pstOutFrame->enCompressMode  = COMPRESS_MODE_NONE;
    pstOutFrame->enVideoFormat   = VIDEO_FORMAT_LINEAR;

    JobData.enJobType    = VGS_JOB_TYPE_NORMAL;
    JobData.pJobCallBack = HI_NULL;

    s32Ret = CALL_VB_UserAdd(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    if(PCIVFMW_STATE_STARTED!=s_u32PcivFmwState)
       {
    	   CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
           return HI_FAILURE;
       }
    /* 1.Begin VGS job-----------------------------------------------------------------------------------------------------------*/

    s32Ret = pfnVgsExpFunc->pfnVgsBeginJob(&VgsHandle, VGS_JOB_PRI_NORMAL, HI_ID_PCIV, pcivChn, &JobData);
    if (HI_SUCCESS != s32Ret)
    {
        pChn->u32AddJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnVgsBeginJob failed ! pcivChn:%d \n", pcivChn);
		CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        return HI_FAILURE;
    }
    pChn->u32AddJobSucCnt++;

    /* 2.zoom the pic-----------------------------------------------------------------------------------------------------------*/

	/*configure other item of the vgs task info(in the callback of vgs will perform the action send image)*/
    stVgsTask.pCallBack          = PcivFmwSrcPicZoomCb;
    stVgsTask.enCallModId        = HI_ID_PCIV;
    stVgsTask.u32CallChnId       = pcivChn;
    stVgsTask.au32privateData[0] = pObj->enType;
    stVgsTask.au32privateData[1] = pObj->bVpssSend;
    stVgsTask.au32privateData[2] = pstMixCapState->bMixCapMode;
    stVgsTask.au32privateData[3] = pstMixCapState->bHasDownScale;
    stVgsTask.reserved           = 0;

    s32Ret = pfnVgsExpFunc->pfnVgsAddOnlineTask(VgsHandle, &stVgsTask, &pChn->stVgsOpt);
    if (HI_SUCCESS != s32Ret)
    {
        pChn->u32ZoomTaskFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "create vgs task failed,errno:%x,will lost this frame\n",s32Ret);
        CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
		pfnVgsExpFunc->pfnVgsCancelJob(VgsHandle);
        return HI_FAILURE;
    }

    pChn->u32ZoomTaskSucCnt++;

    /* 3.End VGS job-----------------------------------------------------------------------------------------------------------*/

    /* Notes: If EndJob failed, callback will called auto */
    s32Ret = pfnVgsExpFunc->pfnVgsEndJob(VgsHandle, HI_TRUE,HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        pChn->u32EndJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnVgsEndJob failed ! pcivChn:%d \n", pcivChn);
		CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        pfnVgsExpFunc->pfnVgsCancelJob(VgsHandle);
        return HI_FAILURE;
    }

    pChn->u32EndJobSucCnt++;

    return HI_SUCCESS;
}


static HI_VOID PcivFmwSrcPicMoveOSdZoomCb(MOD_ID_E enCallModId, HI_U32 u32CallDevId, const VGS_TASK_DATA_S *pstVgsTask)
{
    HI_S32                   s32Ret;
    PCIV_CHN                 pcivChn;
    PCIV_BIND_OBJ_S          stBindObj = {0};
    VI_MIXCAP_STAT_S         stMixCapState;
    PCIV_FWMCHANNEL_S        *pFmwChn = NULL;
    const VIDEO_FRAME_INFO_S *pstImgIn = NULL;
    const VIDEO_FRAME_INFO_S *pstImgOut = NULL;
    unsigned long            flags;

    if (NULL == pstVgsTask)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "In Function PcivFmwSrcPicMoveOSdZoomCb: pstVgsTask is null, return!\n");
        return;
    }

    pstImgIn  = &pstVgsTask->stImgIn;
    pstImgOut = &pstVgsTask->stImgOut;
    pcivChn   = pstVgsTask->u32CallChnId;
    HI_ASSERT((pcivChn >= 0) && (pcivChn < PCIVFMW_MAX_CHN_NUM));
    pFmwChn   = &g_stFwmPcivChn[pcivChn];

    pFmwChn->u32MoveCbCnt++;
    pFmwChn->u32OsdCbCnt++;
    pFmwChn->u32ZoomCbCnt++;

	/*the Image finish used should released*/
    CALL_VB_UserSub(pstImgIn->u32PoolId, pstImgIn->stVFrame.u32PhyAddr[0], VB_UID_PCIV);

    /* release the region gotten*/
    PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
    pFmwChn->u32RgnNum = 0;

    stBindObj.enType    = pstVgsTask->au32privateData[0];
    stBindObj.bVpssSend = pstVgsTask->au32privateData[1];

	/*In the VGS interrupt,the channel maybe stop*/
    if (HI_FALSE == pFmwChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

	/*if vgs task failue, return with failure*/
    if (VGS_TASK_FNSH_STAT_OK != pstVgsTask->enFinishStat)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "PcivFmwSrcPicMoveOSdZoomCb vgs task finish status is no ok, chn:%d\n", pcivChn);
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            //PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            //PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

    stMixCapState.bMixCapMode   = pstVgsTask->au32privateData[2];
    stMixCapState.bHasDownScale = pstVgsTask->au32privateData[3];

	/*send the video after zoom*/
    s32Ret = PcivFmwSrcPicSend(pcivChn, &stBindObj, pstImgOut, pstImgIn, &stMixCapState);
    if (HI_SUCCESS != s32Ret)
    {
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

    if (PCIV_BIND_VDEC == stBindObj.enType)
    {
        PCIVFMW_SPIN_LOCK;
        pFmwChn->enSendState = PCIVFMW_SEND_OK;
        PCIVFMW_SPIN_UNLOCK;
    }

out:
	/*no matter success or not,this callback mst release the VB*/
    s32Ret = CALL_VB_UserSub(pstImgOut->u32PoolId, pstImgOut->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    return;
}


static HI_S32 PcivFmwSrcPicMoveOsdZoom(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pObj,
        const VIDEO_FRAME_INFO_S *pstSrcFrame, VI_MIXCAP_STAT_S *pstMixCapState)
{
    HI_S32            		i;
    HI_S32            		s32Ret;
    HI_S32            		s32Value;
    MPP_CHN_S         		stChn;
    RGN_INFO_S        		stRgnInfo;
    VGS_HANDLE        		VgsHandle;
    VGS_JOB_DATA_S    		JobData;
    VGS_TASK_DATA_S   		stVgsTask;
    VB_BLKHANDLE      		VbHandle;
    VIDEO_FRAME_S     		*pstOutFrame = NULL;
    HI_VOID           		*pMmzName = HI_NULL;
    PCIV_FWMCHANNEL_S 		*pFmwChn = &g_stFwmPcivChn[pcivChn];
	VGS_EXPORT_FUNC_S 		*pfnVgsExpFunc = (VGS_EXPORT_FUNC_S *)CMPI_GetModuleFuncById(HI_ID_VGS);
	static VGS_ONLINE_OPT_S stVgsOpt;

    stRgnInfo.u32Num = 0;
    s32Ret   = PcivFmwGetRegion(pcivChn, OVERLAYEX_RGN, &stRgnInfo);
    s32Value = s32Ret;
    if (stRgnInfo.u32Num <= 0)
    {
        PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
        s32Ret = PcivFmwSrcPicZoom(pcivChn, pObj, pstSrcFrame, pstMixCapState);
        return s32Ret;
    }

    /* config VGS optional */
	memset(&stVgsOpt, 0, sizeof(VGS_ONLINE_OPT_S));
    memcpy(&stVgsOpt, &pFmwChn->stVgsOpt, sizeof(VGS_ONLINE_OPT_S));

    /* the Input Image info is from pstSrcFrame */
    memcpy(&stVgsTask.stImgIn,  pstSrcFrame, sizeof(VIDEO_FRAME_INFO_S));

    /* the output Image is same with the input Image */
    memcpy(&stVgsTask.stImgOut, &stVgsTask.stImgIn, sizeof(VIDEO_FRAME_INFO_S));

	/*the output Image is same to pciv channel size, request VB*/
    stChn.enModId  = HI_ID_PCIV;
    stChn.s32DevId = 0;
    stChn.s32ChnId = pcivChn;
    CALL_SYS_GetMmzName(&stChn, &pMmzName);

    VbHandle = CALL_VB_GetBlkBySize(pFmwChn->u32BlkSize, VB_UID_PCIV, pMmzName);
    if (VB_INVALID_HANDLE == VbHandle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Get VB(%uByte) buffer for image out fail,chn: %d.=======\n", pFmwChn->u32BlkSize, pcivChn);
        return HI_FAILURE;
    }

	/*config the outout video info*/
    stVgsTask.stImgOut.u32PoolId = CALL_VB_Handle2PoolId(VbHandle);
    pstOutFrame                  = &stVgsTask.stImgOut.stVFrame;
    pstOutFrame->u32Width        = pFmwChn->stPicAttr.u32Width;
    pstOutFrame->u32Height       = pFmwChn->stPicAttr.u32Height;
    pstOutFrame->u32Field        = pstSrcFrame->stVFrame.u32Field;
    pstOutFrame->enPixelFormat   = pFmwChn->stPicAttr.enPixelFormat;
    pstOutFrame->u32PhyAddr[0]   = CALL_VB_Handle2Phys(VbHandle);
    pstOutFrame->u32PhyAddr[1]   = pstOutFrame->u32PhyAddr[0] + pFmwChn->u32Offset[1];
    pstOutFrame->u32PhyAddr[2]   = pstOutFrame->u32PhyAddr[0] + pFmwChn->u32Offset[2];
    pstOutFrame->pVirAddr[0]     = HI_NULL;
    pstOutFrame->pVirAddr[1]     = HI_NULL;
    pstOutFrame->pVirAddr[2]     = HI_NULL;
    pstOutFrame->u32Stride[0]    = pFmwChn->stPicAttr.u32Stride[0];
    pstOutFrame->u32Stride[1]    = pFmwChn->stPicAttr.u32Stride[1];
    pstOutFrame->u32Stride[2]    = pFmwChn->stPicAttr.u32Stride[2];
    pstOutFrame->u64pts          = pstSrcFrame->stVFrame.u64pts;
    pstOutFrame->u32TimeRef      = pstSrcFrame->stVFrame.u32TimeRef;
    pstOutFrame->enCompressMode  = COMPRESS_MODE_NONE;
	pstOutFrame->enVideoFormat   = VIDEO_FORMAT_LINEAR;

	/*config vgs task, put osd or not*/
    if (s32Value == HI_SUCCESS && stRgnInfo.u32Num > 0)
    {
        HI_ASSERT(stRgnInfo.u32Num <= OVERLAYEX_MAX_NUM_PCIV);
        pFmwChn->u32RgnNum = stRgnInfo.u32Num;

        for (i=0; i<stRgnInfo.u32Num; i++)
        {
            stVgsOpt.stOsdOpt[i].bOsdEn         = HI_TRUE;
            stVgsOpt.stOsdOpt[i].u32GlobalAlpha = 255;

            stVgsOpt.stOsdOpt[i].stVgsOsd.u32PhyAddr    = stRgnInfo.ppstRgnComm[i]->u32PhyAddr;
            stVgsOpt.stOsdOpt[i].stVgsOsd.enPixelFormat = stRgnInfo.ppstRgnComm[i]->enPixelFmt;
            stVgsOpt.stOsdOpt[i].stVgsOsd.u32Stride     = stRgnInfo.ppstRgnComm[i]->u32Stride;

            if (PIXEL_FORMAT_RGB_1555 == stVgsOpt.stOsdOpt[i].stVgsOsd.enPixelFormat)
            {
                stVgsOpt.stOsdOpt[i].stVgsOsd.bAlphaExt1555 = HI_TRUE;
                stVgsOpt.stOsdOpt[i].stVgsOsd.u8Alpha0      = stRgnInfo.ppstRgnComm[i]->u32BgAlpha;
                stVgsOpt.stOsdOpt[i].stVgsOsd.u8Alpha1      = stRgnInfo.ppstRgnComm[i]->u32FgAlpha;
            }

            stVgsOpt.stOsdOpt[i].stOsdRect.s32X      = stRgnInfo.ppstRgnComm[i]->stPoint.s32X;
            stVgsOpt.stOsdOpt[i].stOsdRect.s32Y      = stRgnInfo.ppstRgnComm[i]->stPoint.s32Y;
            stVgsOpt.stOsdOpt[i].stOsdRect.u32Height = stRgnInfo.ppstRgnComm[i]->stSize.u32Height;
            stVgsOpt.stOsdOpt[i].stOsdRect.u32Width  = stRgnInfo.ppstRgnComm[i]->stSize.u32Width;
        }

        stVgsOpt.bOsd = HI_TRUE;

    }

	/*config the member or vgs taks structure(sending Image in VGS callback function)*/
    stVgsTask.pCallBack          = PcivFmwSrcPicMoveOSdZoomCb;
    stVgsTask.enCallModId        = HI_ID_PCIV;
    stVgsTask.u32CallChnId       = pcivChn;
    stVgsTask.au32privateData[0] = pObj->enType;
    stVgsTask.au32privateData[1] = pObj->bVpssSend;
    stVgsTask.au32privateData[2] = pstMixCapState->bMixCapMode;
    stVgsTask.au32privateData[3] = pstMixCapState->bHasDownScale;
    stVgsTask.reserved           = 0;

    JobData.enJobType            = VGS_JOB_TYPE_NORMAL;
    JobData.pJobCallBack         = HI_NULL;

    CALL_VB_UserAdd(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
    
    if (PCIVFMW_STATE_STARTED != s_u32PcivFmwState)
    {
        CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        return HI_FAILURE;
    }

    /* 1.Begin VGS job-----------------------------------------------------------------------------------------------------------*/

    s32Ret = pfnVgsExpFunc->pfnVgsBeginJob(&VgsHandle, VGS_JOB_PRI_NORMAL, HI_ID_PCIV, pcivChn, &JobData);
    if (s32Ret != HI_SUCCESS)
    {
        pFmwChn->u32AddJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnVgsBeginJob failed ! pcivChn:%d \n", pcivChn);
        return HI_FAILURE;
    }
    pFmwChn->u32AddJobSucCnt++;

    /* 2.move the picture, add osd, scale picture------------------------------------------------------------------------------*/

    /* add task to VGS job */
    s32Ret = pfnVgsExpFunc->pfnVgsAddOnlineTask(VgsHandle, &stVgsTask, &stVgsOpt);
    if (HI_SUCCESS != s32Ret)
    {
        pFmwChn->u32MoveTaskFailCnt++;
        pFmwChn->u32OsdTaskFailCnt++;
        pFmwChn->u32ZoomTaskFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "create vgs task failed,errno:%x,will lost this frame\n", s32Ret);
		CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
		CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        pfnVgsExpFunc->pfnVgsCancelJob(VgsHandle);
        PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
        return HI_FAILURE;
    }

    pFmwChn->u32MoveTaskSucCnt++;
    pFmwChn->u32OsdTaskSucCnt++;
    pFmwChn->u32ZoomTaskSucCnt++;

    /* 3.End DSU job------------------------------------------------------------------------------------------------------*/

    /* Notes: If EndJob failed, callback will called auto */
    s32Ret = pfnVgsExpFunc->pfnVgsEndJob(VgsHandle, HI_TRUE,HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        pFmwChn->u32EndJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnVgsEndJob failed ! pcivChn:%d \n", pcivChn);
        pfnVgsExpFunc->pfnVgsCancelJob(VgsHandle);
        return HI_FAILURE;
    }

    pFmwChn->u32EndJobSucCnt++;

    return HI_SUCCESS;

}

static HI_S32 PcivFirmWareSrcPreProc(PCIV_CHN pcivChn,PCIV_BIND_OBJ_S *pObj,
            const VIDEO_FRAME_INFO_S *pstSrcFrame, VI_MIXCAP_STAT_S *pstMixCapState)
{
    HI_S32 s32Ret = HI_SUCCESS;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    pChn = &g_stFwmPcivChn[pcivChn];

    if (drop_err_timeref == 1)
    {
		/*Prevent out-of-order when send VI source Image, drop the out-of-order fram in the video(u32TimeRef must increased) */
        if ((PCIV_BIND_VI == pObj->enType) || (PCIV_BIND_VO == pObj->enType))
        {
            if (pstSrcFrame->stVFrame.u32TimeRef < pChn->u32TimeRef)
            {
                PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv %d, TimeRef err, (%u,%u)\n",
                pcivChn, pstSrcFrame->stVFrame.u32TimeRef, pChn->u32TimeRef);
                return HI_FAILURE;
            }
            pChn->u32TimeRef = pstSrcFrame->stVFrame.u32TimeRef;
        }
    }

	/*if need put osd to the source vide, Process:Move->put OSD->Zoom */
    if ((PCIV_BIND_VI == pObj->enType) || (PCIV_BIND_VO == pObj->enType) || (PCIV_BIND_VDEC == pObj->enType))
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "Pciv channel %d support osd right now\n", pcivChn);
        s32Ret = PcivFmwSrcPicMoveOsdZoom(pcivChn, pObj, pstSrcFrame, pstMixCapState);
    }
    else
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv channel %d not support type:%u\n", pcivChn, pObj->enType);
    }

    return s32Ret;
}

/* be called in VIU interrupt handler or VDEC interrupt handler. */
HI_S32 PCIV_FirmWareSrcPicSend(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pObj, const VIDEO_FRAME_INFO_S *pPicInfo, VI_MIXCAP_STAT_S *pstMixCapState)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    HI_BOOL bWaitDsu;

    bWaitDsu = HI_FALSE;

    pChn = &g_stFwmPcivChn[pcivChn];

   /****************************************************************************************
     * The Image send to PCIV from VDEC is 16-byte align,but the Image send by pciv is not align.Then when PCIV send the Image date from vdec directly, at this
     * time, when the master chip get data,it will calculate the address of UV, the err of 8 lines image data dislocation will appear, So we propose to the data should
     * through VPSS or VGS in the slave chip.It will reload as the format that PCIV sending need.
     * But,if the Image data do not through VPSS, because of the performance of VGS if weak, At this time, the system send spead is limited by VGS, so for the diffrence
     * diffrence of align format, PCIV cannot send the Image directly from vdec, but through this function handle.
     ****************************************************************************************/


	/*PCIV channel must be start*/
    if (pChn->bStart != HI_TRUE)
    {
        return HI_FAILURE;
    }
    pChn->u32GetCnt++;

	/*perform the pre_process then send the Image*/
    if (PcivFirmWareSrcPreProc(pcivChn, pObj, pPicInfo, pstMixCapState))
    {
        pChn->u32LostCnt++;
        return HI_FAILURE;
    }

    if (PCIV_BIND_VDEC == pObj->enType)
    {
        pChn->enSendState = PCIVFMW_SEND_ING;
    }
    bWaitDsu = HI_TRUE;

    return HI_SUCCESS;
}

/* be called in VIU interrupt handler */
HI_S32 PCIV_FirmWareViuSendPic(VI_DEV viDev, VI_CHN viChn,
        const VIDEO_FRAME_INFO_S *pPicInfo, const VI_MIXCAP_STAT_S *pstMixStat)
{
    return HI_SUCCESS;
}


/* timer function of Receiver(master chip): Get data from pciv and send to vpss/venc/vo */
/* timer function of sender(slave chip): Get data from vdec and send to pciv */

HI_VOID PCIV_SendVdecPicTimerFunc(unsigned long data)
{
    HI_S32 s32Ret;
    PCIV_CHN pcivChn;
    PCIV_FWMCHANNEL_S *pChn;
    VIDEO_FRAME_INFO_S *pstVFrameInfo;
    unsigned long flags;
    HI_S32 s32DevId = 0;
    PCIV_BIND_OBJ_S Obj = {0};
    VI_MIXCAP_STAT_S  stMixCapState;

    /* timer will be restarted after 1 tick */
    g_timerPcivSendVdecPic.expires  = jiffies + msecs_to_jiffies(PCIV_TIMER_EXPIRES);
    g_timerPcivSendVdecPic.function = PCIV_SendVdecPicTimerFunc;
    g_timerPcivSendVdecPic.data     = 0;
    if (!timer_pending(&g_timerPcivSendVdecPic))
    {
        add_timer(&g_timerPcivSendVdecPic);
    }

    for (pcivChn=0; pcivChn<PCIVFMW_MAX_CHN_NUM; pcivChn++)
	{
        PCIVFMW_SPIN_LOCK;
        pChn = &g_stFwmPcivChn[pcivChn];

        pChn->u32TimerCnt++;

        if (HI_FALSE == pChn->bStart)
        {
            PCIVFMW_SPIN_UNLOCK;
            continue;
        }

        if (PCIVFMW_SEND_ING == pChn->enSendState)
        {
            PCIVFMW_SPIN_UNLOCK;
            continue;
        }
        else if (PCIVFMW_SEND_OK == pChn->enSendState)
        {
			/*check the last time is success or not(the first is the success state)
			get the new vdec Image info*/
            pChn->pCurVdecNode = PCIV_PicQueueGetBusy(&pChn->stPicQueue);
            if (NULL == pChn->pCurVdecNode)
            {
                PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d no vdec pic\n", pcivChn);
                PCIVFMW_SPIN_UNLOCK;
                continue;
            }
        }
	    else if (PCIVFMW_SEND_NOK == pChn->enSendState)
        {
        	if (HI_TRUE == pChn->bMaster)
        	{
				/*If the last time is not success,get and send  the data of last time again */
	            pChn->pCurVdecNode = PCIV_PicQueueGetBusy(&pChn->stPicQueue);
	            if (NULL == pChn->pCurVdecNode)
	            {
	                PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d no vdec pic\n", pcivChn);
	                PCIVFMW_SPIN_UNLOCK;
	                continue;
	            }
        	}
			else
			{
				/*If the last time is not success, get the data of last time to send again*/
	            if (pChn->pCurVdecNode == NULL)
	            {
	                PCIVFMW_SPIN_UNLOCK;
	                continue;
	            }
			}
        }
	    else
        {
            PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn %d send vdec pic state error %#x\n", pcivChn, pChn->enSendState);
            PCIVFMW_SPIN_UNLOCK;
            continue;
        }

		if (HI_TRUE == pChn->bMaster)
		{

	        pChn->enSendState = PCIVFMW_SEND_ING;
	        pstVFrameInfo     = &pChn->pCurVdecNode->stPcivPic.stVideoFrame;

			/*send the Image frome vdec to VPSS/VENC/VO*/
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, start send pic to VPSS/VO.\n", pcivChn, pChn->bStart);
	        s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, pcivChn, pChn->pCurVdecNode->stPcivPic.bBlock, MPP_DATA_VDEC_FRAME, pstVFrameInfo);
	        if ((s32Ret != HI_SUCCESS) && (HI_TRUE == pChn->pCurVdecNode->stPcivPic.bBlock))
	        {

				PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pChn->bStart);
				PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic failed, put to queue and send again. s32Ret: 0x%x\n", pcivChn, s32Ret);
				PCIV_PicQueuePutBusyHead(&pChn->stPicQueue, pChn->pCurVdecNode);
	            pChn->enSendState  = PCIVFMW_SEND_NOK;
	            pChn->pCurVdecNode = NULL;
	            PCIVFMW_SPIN_UNLOCK;
	        }
	        else
	        {
				PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pChn->bStart);
				PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic ok\n", pcivChn);
				HI_ASSERT(pChn->pCurVdecNode != NULL);
	            pChn->bPcivHold[pChn->pCurVdecNode->stPcivPic.u32Index] = HI_FALSE;
	            PCIV_PicQueuePutFree(&pChn->stPicQueue, pChn->pCurVdecNode);
	            pChn->enSendState  = PCIVFMW_SEND_OK;
	            pChn->pCurVdecNode = NULL;
	            PCIVFMW_SPIN_UNLOCK;

	            PcivFirmWareRecvPicFree(pcivChn);
	        }
		}
		else
		{

	        pstVFrameInfo = &pChn->pCurVdecNode->stPcivPic.stVideoFrame;
	        stMixCapState.bHasDownScale = HI_FALSE;
	        stMixCapState.bMixCapMode   = HI_FALSE;
			/*send the vdec Image to PCI target*/
	        Obj.enType    = PCIV_BIND_VDEC;
	        Obj.bVpssSend = HI_FALSE;
			pChn->enModId = HI_ID_VDEC;
	        s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, pstVFrameInfo, &stMixCapState);
	        if (s32Ret != HI_SUCCESS)/*if send failed,the next time use the backup data*/
	        {
	            pChn->enSendState = PCIVFMW_SEND_NOK;
	        }
	        PCIVFMW_SPIN_UNLOCK;
		}
	}

	return;
}


/* Called in VIU, virtual VOU or VDEC interrupt handler */
HI_S32 PCIV_FirmWareSendPic(HI_S32 s32DevId, HI_S32 s32ChnId, HI_BOOL bBlock, MPP_DATA_TYPE_E enDataType, void *pPicInfo)
{
    HI_S32             s32Ret;
    PCIV_CHN           pcivChn = s32ChnId;
    unsigned long      flags;
    PCIV_BIND_OBJ_S    Obj = {0};
    RGN_INFO_S         stRgnInfo;
    VI_MIXCAP_STAT_S   stMixCapState;
    PCIV_FWMCHANNEL_S  *pChn = NULL;
    VI_FRAME_INFO_S    *pstVifInfo   = NULL;
    VIDEO_FRAME_INFO_S *pstVofInfo   = NULL;
    VIDEO_FRAME_INFO_S *pstVdecfInfo = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);
    PCIVFMW_CHECK_PTR(pPicInfo);

    if (MPP_DATA_VIU_FRAME == enDataType)
    {
        Obj.enType    = PCIV_BIND_VI;
        Obj.bVpssSend = HI_FALSE;
        pstVifInfo = (VI_FRAME_INFO_S *)pPicInfo;

        stRgnInfo.u32Num = 0;
        s32Ret = PcivFmwGetRegion(pcivChn, OVERLAYEX_RGN, &stRgnInfo);
        if (HI_SUCCESS != s32Ret)
        {
            PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
            return s32Ret;
        }

        PCIVFMW_SPIN_LOCK;

        pChn          = &g_stFwmPcivChn[s32ChnId];
        pChn->bBlock  = bBlock;
        pChn->enModId = HI_ID_VIU;

        if (pChn->stPicAttr.u32Width  != pstVifInfo->stViFrmInfo.stVFrame.u32Width
         || pChn->stPicAttr.u32Height != pstVifInfo->stViFrmInfo.stVFrame.u32Height
         || (stRgnInfo.u32Num > 0))
        {
        	PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
            /* the pic size is not the same or it's needed to add osd */
            s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, &(pstVifInfo->stViFrmInfo), &(pstVifInfo->stMixCapState));
        }
        else
        {
            /* send picture directly */
            PCIVFMW_SPIN_UNLOCK;
            s32Ret = PcivFmwSrcPicSend(pcivChn, &Obj, &pstVifInfo->stViFrmInfo, NULL, &(pstVifInfo->stMixCapState));
            PCIVFMW_SPIN_LOCK;
        }
        PCIVFMW_SPIN_UNLOCK;

        if (HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d viu frame pic send failed\n",s32DevId);
            return HI_FAILURE;
        }
    }
    else if (MPP_DATA_VOU_FRAME == enDataType)
    {
        Obj.enType                  = PCIV_BIND_VO;
        Obj.bVpssSend               = HI_FALSE;
        stMixCapState.bHasDownScale = HI_FALSE;
        stMixCapState.bMixCapMode   = HI_FALSE;
        pstVofInfo                  = (VIDEO_FRAME_INFO_S *)pPicInfo;

        PCIVFMW_SPIN_LOCK;

        pChn          = &g_stFwmPcivChn[s32ChnId];
        pChn->bBlock  = bBlock;
        pChn->enModId = HI_ID_VOU;

        s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, pstVofInfo, &stMixCapState);
        PCIVFMW_SPIN_UNLOCK;

        if (HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d virtual vou frame pic send failed\n",s32DevId);
            return HI_FAILURE;
        }
    }
    else if (MPP_DATA_VDEC_FRAME == enDataType)
    {
        PCIV_PIC_NODE_S *pNode = NULL;

        pstVdecfInfo = (VIDEO_FRAME_INFO_S *)pPicInfo;

        PCIVFMW_SPIN_LOCK;

        pChn = &g_stFwmPcivChn[pcivChn];

		if (HI_TRUE != pChn->bStart)
		{
			PCIVFMW_SPIN_UNLOCK;
        	PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        	return HI_FAILURE;
		}

        pChn->bBlock = bBlock;

        pNode = PCIV_PicQueueGetFree(&pChn->stPicQueue);
        if (NULL == pNode)
        {
            PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d no free node\n",s32DevId);
            PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }

        pNode->stPcivPic.enModId = HI_ID_VDEC;

        memcpy(&pNode->stPcivPic.stVideoFrame, pstVdecfInfo, sizeof(VIDEO_FRAME_INFO_S));
        s32Ret = CALL_VB_UserAdd(pstVdecfInfo->u32PoolId, pstVdecfInfo->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
		HI_ASSERT(HI_SUCCESS == s32Ret);

        PCIV_PicQueuePutBusy(&pChn->stPicQueue, pNode);

        PCIVFMW_SPIN_UNLOCK;
    }
    else
    {
        PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d data type:%u error\n",s32DevId,enDataType);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 PcivFmw_VpssQuery(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_QUERY_INFO_S *pstQueryInfo, VPSS_INST_INFO_S *pstInstInfo)
{
    HI_U32 u32PicSize;
    VB_BLKHANDLE handle;
    MOD_ID_E enModId;
    VIDEO_FRAME_S *pstVFrame;
    HI_U32   u32PicWidth;
    HI_U32   u32PicHeight;
    HI_U32   u32Stride;
    HI_U32   u32CurTimeRef;
    HI_S32   s32Ret;
    MPP_CHN_S  stChn ={0};
    HI_CHAR *pMmzName = NULL;
    unsigned long flags;

    PCIV_FWMCHANNEL_S   *pChn = NULL;
    PCIV_CHN PcivChn = s32ChnId;

    PCIVFMW_CHECK_CHNID(PcivChn);
    PCIVFMW_CHECK_PTR(pstQueryInfo);
    PCIVFMW_CHECK_PTR(pstInstInfo);
	
	PCIVFMW_SPIN_LOCK;
    if(PCIVFMW_STATE_STARTED!=s_u32PcivFmwState)
       {
           PCIVFMW_SPIN_UNLOCK;
           return HI_FAILURE;
       }
    pChn = &g_stFwmPcivChn[PcivChn];
    if (HI_FALSE == pChn->bCreate)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel doesn't exist, please create it!\n");
		PCIVFMW_SPIN_UNLOCK;
		return HI_FAILURE;
	}

	if (HI_FALSE == pChn->bStart)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel has stoped!\n");
		PCIVFMW_SPIN_UNLOCK;
        return HI_FAILURE;
	}

    if (NULL == pstQueryInfo->pstSrcPicInfo)
    {
		PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn:%d SrcPicInfo is NULL!\n",PcivChn);
		PCIVFMW_SPIN_UNLOCK;
        goto OLD_UNDO;
    }

    enModId = pstQueryInfo->pstSrcPicInfo->enModId;
    u32CurTimeRef = pstQueryInfo->pstSrcPicInfo->stVideoFrame.stVFrame.u32TimeRef;

    if (((HI_ID_VIU == enModId) || (HI_ID_VOU == enModId)) && (pChn->u32TimeRef == u32CurTimeRef))
    {
        //Duplicate frame not recived again
		PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn:%d duplicated frame!\n",PcivChn);
		PCIVFMW_SPIN_UNLOCK;
        goto OLD_UNDO;
    }

    if (HI_ID_VDEC == enModId)
    {
        //Duplicate frame not recived again
        s32Ret = g_stPcivFmwCallBack.pfQueryPcivChnShareBufState(PcivChn);
        if (s32Ret != HI_SUCCESS)
        {
			PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn:%d has no free share buf to receive pic!\n",PcivChn);
			PCIVFMW_SPIN_UNLOCK;
			goto OLD_UNDO;
        }

		if (PCIVFMW_SEND_OK != pChn->enSendState)
		{
			PCIVFMW_SPIN_UNLOCK;
			PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn:%d last pic send no ok, send again!\n", PcivChn);
			goto OLD_UNDO;
		}
    }

    if (HI_TRUE == pstQueryInfo->bScaleCap)
    {
        u32PicWidth       = pChn->stPicAttr.u32Width;
        u32PicHeight      = pChn->stPicAttr.u32Height;

    }
    else
    {
        u32PicWidth  = pstQueryInfo->pstSrcPicInfo->stVideoFrame.stVFrame.u32Width;
        u32PicHeight = pstQueryInfo->pstSrcPicInfo->stVideoFrame.stVFrame.u32Height;

    }

	if (HI_FALSE == pstQueryInfo->bMalloc)
	{
		/* bypass */
        /* if picture from VIU or USER, send to PCIV Firmware directly */
        if ((HI_ID_VIU == enModId) || (HI_ID_USR == enModId))
        {
		    pstInstInfo->bNew = HI_TRUE;
		    pstInstInfo->bVpss = HI_TRUE;
        }
        /* if picture from VEDC, send to PCIV queue and then deal with DSU, check the queue is full or not  */
        else if (HI_ID_VDEC == enModId)
        {
            //PCIVFMW_SPIN_LOCK;
            if (0 == PCIV_PicQueueGetFreeNum(&(pChn->stPicQueue)))
            {
                /* if PCIV queue is full, old undo */
                PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn:%d queue is full!\n",PcivChn);
                PCIVFMW_SPIN_UNLOCK;
                goto OLD_UNDO;
            }
            //PCIVFMW_SPIN_UNLOCK;

            pstInstInfo->bNew = HI_TRUE;
		    pstInstInfo->bVpss = HI_TRUE;
        }
	}
	else
	{
        u32Stride = pChn->stPicAttr.u32Stride[0];

        if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pChn->stPicAttr.enPixelFormat)
        {
            u32PicSize = u32Stride * u32PicHeight * 3 / 2;
        }
        else if (PIXEL_FORMAT_YUV_SEMIPLANAR_422 == pChn->stPicAttr.enPixelFormat)
        {
            u32PicSize = u32Stride * u32PicHeight * 2;
        }
        else
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "[PcivChn:%d]enPixelFormat(%u) not support\n",PcivChn,pChn->stPicAttr.enPixelFormat);
			PCIVFMW_SPIN_UNLOCK;
			goto OLD_UNDO;
        }

        stChn.enModId = HI_ID_PCIV;
        stChn.s32DevId = s32DevId;
        stChn.s32ChnId = s32ChnId;
        s32Ret = CALL_SYS_GetMmzName(&stChn, (void**)&pMmzName);
        HI_ASSERT(HI_SUCCESS == s32Ret);

        handle = CALL_VB_GetBlkBySize(u32PicSize, VB_UID_VPSS, pMmzName);
        if (VB_INVALID_HANDLE == handle)
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "get VB for u32PicSize: %u from ddr:%s fail,for grp %d VPSS Query\n",u32PicSize,pMmzName,s32DevId);
			PCIVFMW_SPIN_UNLOCK;
			goto OLD_UNDO;
        }

        pstInstInfo->astDestPicInfo[0].stVideoFrame.u32PoolId = CALL_VB_Handle2PoolId(handle);

        pstVFrame = &pstInstInfo->astDestPicInfo[0].stVideoFrame.stVFrame;
        pstVFrame->u32Width      = u32PicWidth;
        pstVFrame->u32Height     = u32PicHeight;
        pstVFrame->u32PhyAddr[0] = CALL_VB_Handle2Phys(handle);
        pstVFrame->u32PhyAddr[1] = pstVFrame->u32PhyAddr[0] + u32PicHeight*u32Stride;
        pstVFrame->pVirAddr[0] 	 = HI_NULL;
        pstVFrame->pVirAddr[1] 	 = HI_NULL;
		pstVFrame->pVirAddr[2] 	 = HI_NULL;
        pstVFrame->u32Stride[0]  = u32Stride;
        pstVFrame->u32Stride[1]  = u32Stride;
        pstVFrame->enPixelFormat = pChn->stPicAttr.enPixelFormat;

        pstInstInfo->bVpss = HI_TRUE;
        pstInstInfo->bNew  = HI_TRUE;
	}

    pChn->u32NewDoCnt++;
	PCIVFMW_SPIN_UNLOCK;
	return HI_SUCCESS;

OLD_UNDO:
    pstInstInfo->bVpss   = HI_FALSE;
    pstInstInfo->bNew    = HI_FALSE;
    pstInstInfo->bDouble = HI_FALSE;
    pstInstInfo->bUpdate = HI_FALSE;
    pChn->u32OldUndoCnt++;
	return HI_SUCCESS;
}


HI_S32 PcivFmw_VpssSend(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_SEND_INFO_S *pstSendInfo)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    MOD_ID_E          enModId;
    HI_S32            s32Ret;
    PCIV_BIND_OBJ_S   BindObj;
    unsigned long     flags;
    RGN_INFO_S        stRgnInfo = {0};
    VI_MIXCAP_STAT_S  stMixCapState;
    PCIV_CHN PcivChn  = s32ChnId;

    PCIVFMW_CHECK_CHNID(PcivChn);
    PCIVFMW_CHECK_PTR(pstSendInfo);
    PCIVFMW_CHECK_PTR(pstSendInfo->pstDestPicInfo[0]);

    if(PCIVFMW_STATE_STARTED!=s_u32PcivFmwState)
       {
           return HI_FAILURE;
       }
    pChn = &g_stFwmPcivChn[PcivChn];
    if (HI_FALSE == pChn->bCreate)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel doesn't exist, please create it!\n");
        return HI_FAILURE;
	}

	if (HI_FALSE == pChn->bStart)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel has stoped!\n");
        return HI_FAILURE;
	}

    if (HI_FALSE == pstSendInfo->bSuc)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "bsuc is failure.\n");
        return HI_FAILURE;
    }

    enModId = pstSendInfo->pstDestPicInfo[0]->enModId;

    if (HI_ID_VDEC == enModId)
    {
        BindObj.enType = PCIV_BIND_VDEC;
	}
    else if (HI_ID_VOU == enModId)
    {
        BindObj.enType = PCIV_BIND_VO;
    }
    else
    {
        BindObj.enType = PCIV_BIND_VI;
    }

    BindObj.bVpssSend           = HI_TRUE;
    stMixCapState.bHasDownScale = HI_FALSE;
    stMixCapState.bMixCapMode   = HI_FALSE;

    s32Ret = PcivFmwGetRegion(PcivChn, OVERLAYEX_RGN, &stRgnInfo);

    /* use which flag to know VPSS is bypass or not */
    if (pChn->stPicAttr.u32Width  != pstSendInfo->pstDestPicInfo[0]->stVideoFrame.stVFrame.u32Width
     || pChn->stPicAttr.u32Height != pstSendInfo->pstDestPicInfo[0]->stVideoFrame.stVFrame.u32Height
     || (stRgnInfo.u32Num > 0))
	{
		/* bypass */

        PcivFmwPutRegion(PcivChn, OVERLAYEX_RGN);
        PCIVFMW_SPIN_LOCK;
        pChn->bBlock = pstSendInfo->pstDestPicInfo[0]->bBlock;
        s32Ret = PCIV_FirmWareSrcPicSend(PcivChn, &BindObj, &(pstSendInfo->pstDestPicInfo[0]->stVideoFrame), &stMixCapState);
        if (HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "PCIV_FirmWareSrcPicSend failed! pciv chn %d\n", PcivChn);
            PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }
        PCIVFMW_SPIN_UNLOCK;
	}
	else
	{
        pChn->u32GetCnt++;
        /* [HSCP201308020003] l00181524,2013.08.16, PcivFmwSrcPicSend add lock inner, if add lock the will appeare lock itself bug,cannot add lock*/
        //PCIVFMW_SPIN_LOCK;
        pChn->bBlock = pstSendInfo->pstDestPicInfo[0]->bBlock;
        s32Ret = PcivFmwSrcPicSend(PcivChn, &BindObj, &(pstSendInfo->pstDestPicInfo[0]->stVideoFrame), NULL, &stMixCapState);
        if (HI_SUCCESS != s32Ret)
        {
            pChn->u32LostCnt++;
            PCIV_FMW_TRACE(HI_DBG_ALERT, "PcivFmwSrcPicSend failed! pciv chn %d, s32Ret: 0x%x.\n", PcivChn, s32Ret);
            //PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }
        //PCIVFMW_SPIN_UNLOCK;
	}

	return HI_SUCCESS;
}

HI_S32 PcivFmw_ResetCallBack(HI_S32 s32DevId, HI_S32 s32ChnId, HI_VOID *pvData)
{
	HI_S32   i;
	HI_S32   s32Ret;
    PCIV_CHN PcivChn = s32ChnId;
    PCIV_FWMCHANNEL_S *pChn = NULL;
	unsigned long      flags;
	PCIV_PIC_NODE_S   *pNode = NULL;
	HI_U32             u32BusyNum;

	PCIVFMW_SPIN_LOCK;
    pChn         = &g_stFwmPcivChn[PcivChn];
	pChn->bStart = HI_FALSE;
	PCIVFMW_SPIN_UNLOCK;

	/*during the send process wait for finished sending*/
	while(1)
	{
		PCIVFMW_SPIN_LOCK;
		if (PCIVFMW_SEND_ING == pChn->enSendState)
		{
			PCIVFMW_SPIN_UNLOCK;
			msleep(10);
			continue;
		}
		else
		{
			/*send failure,release the vb add by vdec*/
			if ((PCIVFMW_SEND_NOK == pChn->enSendState) && (HI_ID_VDEC == pChn->enModId))
			{
				HI_ASSERT(pChn->pCurVdecNode != NULL);
				s32Ret = CALL_VB_UserSub(pChn->pCurVdecNode->stPcivPic.stVideoFrame.u32PoolId, pChn->pCurVdecNode->stPcivPic.stVideoFrame.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
		        HI_ASSERT(s32Ret == HI_SUCCESS);

		        PCIV_PicQueuePutFree(&pChn->stPicQueue, pChn->pCurVdecNode);
				pChn->enSendState  = PCIVFMW_SEND_OK;
		        pChn->pCurVdecNode = NULL;
			}

			break;
		}
	}

	/*put the node from busy to free*/
	u32BusyNum = PCIV_PicQueueGetBusyNum(&pChn->stPicQueue);
	for (i=0; i<u32BusyNum; i++)
	{
		pNode = PCIV_PicQueueGetBusy(&pChn->stPicQueue);
		if (NULL == pNode)
		{
			PCIVFMW_SPIN_UNLOCK;
			PCIV_FMW_TRACE(HI_DBG_ERR, "PCIV_PicQueueGetBusy failed! pciv chn %d.\n", PcivChn);
			return HI_FAILURE;
		}

		s32Ret = CALL_VB_UserSub(pNode->stPcivPic.stVideoFrame.u32PoolId, pNode->stPcivPic.stVideoFrame.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        HI_ASSERT(s32Ret == HI_SUCCESS);

		PCIV_PicQueuePutFree(&pChn->stPicQueue, pNode);
	}

	pChn->stPicQueue.u32Max     = VDEC_MAX_SEND_CNT;
	pChn->stPicQueue.u32FreeNum = VDEC_MAX_SEND_CNT;
	pChn->stPicQueue.u32BusyNum = 0;
	//pChn->bStart = HI_TRUE;

	PCIVFMW_SPIN_UNLOCK;

    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareRegisterFunc(PCIVFMW_CALLBACK_S *pstCallBack)
{
    PCIVFMW_CHECK_PTR(pstCallBack);
    PCIVFMW_CHECK_PTR(pstCallBack->pfSrcSendPic);
    PCIVFMW_CHECK_PTR(pstCallBack->pfRecvPicFree);
    PCIVFMW_CHECK_PTR(pstCallBack->pfQueryPcivChnShareBufState);

    memcpy(&g_stPcivFmwCallBack, pstCallBack, sizeof(PCIVFMW_CALLBACK_S));

    return HI_SUCCESS;
}


HI_S32 PCIV_FmwInit(void *p)
{
    HI_S32 i, s32Ret;
    BIND_SENDER_INFO_S stSenderInfo;
    BIND_RECEIVER_INFO_S stReceiver;
    VPSS_REGISTER_INFO_S stVpssRgstInfo;
    RGN_REGISTER_INFO_S stRgnInfo;
	
    if (PCIVFMW_STATE_STARTED == s_u32PcivFmwState)
    {
        return HI_SUCCESS;
    }

    spin_lock_init(&g_PcivFmwLock);

    stSenderInfo.enModId      = HI_ID_PCIV;
    stSenderInfo.u32MaxDevCnt = 1;
    stSenderInfo.u32MaxChnCnt = PCIV_MAX_CHN_NUM;
    s32Ret = CALL_SYS_RegisterSender(&stSenderInfo);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    stReceiver.enModId        = HI_ID_PCIV;
    stReceiver.u32MaxDevCnt   = 1;
    stReceiver.u32MaxChnCnt   = PCIV_MAX_CHN_NUM;
    stReceiver.pCallBack      = PCIV_FirmWareSendPic;
	stReceiver.pResetCallBack = PcivFmw_ResetCallBack;
    s32Ret = CALL_SYS_RegisterReceiver(&stReceiver);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    init_timer(&g_timerPcivSendVdecPic);
    g_timerPcivSendVdecPic.expires  = jiffies + msecs_to_jiffies(3*PCIV_TIMER_EXPIRES);
    g_timerPcivSendVdecPic.function = PCIV_SendVdecPicTimerFunc;
    g_timerPcivSendVdecPic.data     = 0;
    add_timer(&g_timerPcivSendVdecPic);

    g_stVbPool.u32PoolCount = 0;

    memset(g_stFwmPcivChn, 0, sizeof(g_stFwmPcivChn));
    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
        g_stFwmPcivChn[i].bStart     = HI_FALSE;
        g_stFwmPcivChn[i].u32SendCnt = 0;
        g_stFwmPcivChn[i].u32GetCnt  = 0;
        g_stFwmPcivChn[i].u32RespCnt = 0;
        g_stFwmPcivChn[i].u32Count   = 0;
        g_stFwmPcivChn[i].u32RgnNum  = 0;
        init_timer(&g_stFwmPcivChn[i].stBufTimer);
        PCIV_FirmWareInitPreProcCfg(i);
    }

    if ((CKFN_VPSS_ENTRY()) && (CKFN_VPSS_Register()))
    {
        stVpssRgstInfo.enModId        = HI_ID_PCIV;
        stVpssRgstInfo.pVpssQuery     = PcivFmw_VpssQuery;
        stVpssRgstInfo.pVpssSend      = PcivFmw_VpssSend;
        stVpssRgstInfo.pResetCallBack = PcivFmw_ResetCallBack;
        s32Ret = CALL_VPSS_Register(&stVpssRgstInfo);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    /* register OVERLAY to region*/
    if ((CKFN_RGN()) && (CKFN_RGN_RegisterMod()))
    {
        RGN_CAPACITY_S stCapacity;

		/*the level of region*/
        stCapacity.stLayer.bComm = HI_FALSE;

        stCapacity.stLayer.bSptReSet = HI_TRUE;
        stCapacity.stLayer.u32LayerMin = 0;
        stCapacity.stLayer.u32LayerMax = 7;

		/*the start position of region*/
        stCapacity.stPoint.bComm = HI_FALSE;
        stCapacity.stPoint.bSptReSet = HI_TRUE;
        stCapacity.stPoint.u32StartXAlign = 2;
        stCapacity.stPoint.u32StartYAlign = 2;
        stCapacity.stPoint.stPointMin.s32X = 0;
        stCapacity.stPoint.stPointMin.s32Y = 0;
        stCapacity.stPoint.stPointMax.s32X = 8190;
        stCapacity.stPoint.stPointMax.s32Y = 8190;

		/*the weight and height of region*/
        stCapacity.stSize.bComm = HI_TRUE;
        stCapacity.stSize.bSptReSet = HI_FALSE;
        stCapacity.stSize.u32WidthAlign = 2;
        stCapacity.stSize.u32HeightAlign = 2;
        stCapacity.stSize.stSizeMin.u32Width = 2;
        stCapacity.stSize.stSizeMin.u32Height = 2;
        stCapacity.stSize.stSizeMax.u32Width = RGN_MAX_WIDTH;
        stCapacity.stSize.stSizeMax.u32Height = RGN_MAX_HEIGHT;
        stCapacity.stSize.u32MaxArea = RGN_MAX_WIDTH * RGN_MAX_HEIGHT;

        /*the pixel format of region*/
        stCapacity.bSptPixlFmt = HI_TRUE;
        stCapacity.stPixlFmt.bComm = HI_TRUE;
        stCapacity.stPixlFmt.bSptReSet = HI_FALSE;
        stCapacity.stPixlFmt.u32PixlFmtNum = 3;
        stCapacity.stPixlFmt.aenPixlFmt[0] = PIXEL_FORMAT_RGB_1555;
        stCapacity.stPixlFmt.aenPixlFmt[1] = PIXEL_FORMAT_RGB_4444;
        stCapacity.stPixlFmt.aenPixlFmt[2] = PIXEL_FORMAT_RGB_8888;

        /*the front Alpha of region*/
        stCapacity.bSptFgAlpha = HI_TRUE;
        stCapacity.stFgAlpha.bComm = HI_FALSE;
        stCapacity.stFgAlpha.bSptReSet = HI_TRUE;
        stCapacity.stFgAlpha.u32AlphaMax = 128;
        stCapacity.stFgAlpha.u32AlphaMin = 0;

        /*the back Alpha of region*/
        stCapacity.bSptBgAlpha = HI_TRUE;
        stCapacity.stBgAlpha.bComm = HI_FALSE;
        stCapacity.stBgAlpha.bSptReSet = HI_TRUE;
        stCapacity.stBgAlpha.u32AlphaMax = 128;
        stCapacity.stBgAlpha.u32AlphaMin = 0;

        /*the panoramic Alpha of region*/
        stCapacity.bSptGlobalAlpha = HI_FALSE;
        stCapacity.stGlobalAlpha.bComm = HI_FALSE;
        stCapacity.stGlobalAlpha.bSptReSet = HI_TRUE;
        stCapacity.stGlobalAlpha.u32AlphaMax = 128;
        stCapacity.stGlobalAlpha.u32AlphaMin = 0;

        /*suport backcolor or not */
        stCapacity.bSptBgClr = HI_TRUE;
        stCapacity.stBgClr.bComm = HI_TRUE;
        stCapacity.stBgClr.bSptReSet = HI_TRUE;

        /*rank*/
        stCapacity.stChnSort.bNeedSort = HI_TRUE;
        stCapacity.stChnSort.enRgnSortKey = RGN_SORT_BY_LAYER;
        stCapacity.stChnSort.bSmallToBig = HI_TRUE;

        /*QP*/
        stCapacity.bSptQp = HI_FALSE;
        stCapacity.stQp.bComm = HI_FALSE;
        stCapacity.stQp.bSptReSet = HI_TRUE;
        stCapacity.stQp.s32QpAbsMax = 51;
        stCapacity.stQp.s32QpAbsMin = 0;
        stCapacity.stQp.s32QpRelMax = 51;
        stCapacity.stQp.s32QpRelMin = -51;

        /*inverse color*/
        stCapacity.bSptInvColoar = HI_FALSE;
        stCapacity.stInvColor.bComm = HI_FALSE;
        stCapacity.stInvColor.bSptReSet = HI_TRUE;
        stCapacity.stInvColor.stSizeCap.stSizeMax.u32Height = 64;
        stCapacity.stInvColor.stSizeCap.stSizeMin.u32Height = 16;
        stCapacity.stInvColor.stSizeCap.stSizeMax.u32Width= 64;
        stCapacity.stInvColor.stSizeCap.stSizeMin.u32Width = 16;
        stCapacity.stInvColor.stSizeCap.u32HeightAlign = 16;
        stCapacity.stInvColor.stSizeCap.u32WidthAlign = 16;
        stCapacity.stInvColor.u32LumMax = 255;
        stCapacity.stInvColor.u32LumMin = 0;
        stCapacity.stInvColor.enInvModMax = MORETHAN_LUM_THRESH;
        stCapacity.stInvColor.enInvModMin = LESSTHAN_LUM_THRESH;
        stCapacity.stInvColor.u32StartXAlign = 16;
        stCapacity.stInvColor.u32StartYAlign = 16;
        stCapacity.stInvColor.u32WidthAlign = 16;
        stCapacity.stInvColor.u32HeightAlign = 16;
        stCapacity.stInvColor.stSizeCap.u32MaxArea = RGN_MAX_WIDTH * RGN_MAX_HEIGHT;

        /*suport bitmap or not */
        stCapacity.bSptBitmap = HI_TRUE;

        /*suport  Overlap or not*/
        stCapacity.bsptOverlap = HI_TRUE;

        /*memery STRIDE*/
        stCapacity.u32Stride = 64;

        stCapacity.u32RgnNumInChn = OVERLAYEX_MAX_NUM_PCIV;

        stRgnInfo.enModId      = HI_ID_PCIV;
        stRgnInfo.u32MaxDevCnt = 1;
        stRgnInfo.u32MaxChnCnt = PCIV_MAX_CHN_NUM;

        /* register OVERLAY */
        s32Ret = CALL_RGN_RegisterMod(OVERLAYEX_RGN, &stCapacity, &stRgnInfo);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    s_u32PcivFmwState = PCIVFMW_STATE_STARTED;
	return HI_SUCCESS;
}


HI_VOID PCIV_FmwExit(HI_VOID)
{
	HI_S32 i,s32Ret;
    if (PCIVFMW_STATE_STOPED == s_u32PcivFmwState)
    {
        return;
    }

    CALL_SYS_UnRegisterReceiver(HI_ID_PCIV);
    CALL_SYS_UnRegisterSender(HI_ID_PCIV);
    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
        /* stop channel */
        (HI_VOID)PCIV_FirmWareStop(i);

        /* destroy channel */
        (HI_VOID)PCIV_FirmWareDestroy(i);

        del_timer_sync(&g_stFwmPcivChn[i].stBufTimer);
    }
    del_timer_sync(&g_timerPcivSendVdecPic);
    if ((CKFN_VPSS_ENTRY()) && (CKFN_VPSS_UnRegister()))
    {
        HI_ASSERT(HI_SUCCESS == CALL_VPSS_UnRegister(HI_ID_PCIV));
    }

    if ((CKFN_RGN()) && (CKFN_RGN_UnRegisterMod()))
    {
        /* unregister OVERLAY */
        s32Ret = CALL_RGN_UnRegisterMod(OVERLAYEX_RGN, HI_ID_PCIV);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    s_u32PcivFmwState = PCIVFMW_STATE_STOPED;
    return;
}

static HI_VOID PCIV_FmwNotify(MOD_NOTICE_ID_E enNotice)
{
//    HI_S32 i, s32Ret;
//    if (PCIVFMW_STATE_STOPED == s_u32PcivFmwState)
//    {
//        return;
//    }
//
//    CALL_SYS_UnRegisterReceiver(HI_ID_PCIV);
//    CALL_SYS_UnRegisterSender(HI_ID_PCIV);
//    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
//    {
//        /* stop channel */
//        (HI_VOID)PCIV_FirmWareStop(i);
//
//        /* destroy channel */
//        (HI_VOID)PCIV_FirmWareDestroy(i);
//
//        del_timer_sync(&g_stFwmPcivChn[i].stBufTimer);
//    }
//    del_timer_sync(&g_timerPcivSendVdecPic);
//    if ((CKFN_VPSS_ENTRY()) && (CKFN_VPSS_UnRegister()))
//    {
//        HI_ASSERT(HI_SUCCESS == CALL_VPSS_UnRegister(HI_ID_PCIV));
//    }
//
//    if ((CKFN_RGN()) && (CKFN_RGN_UnRegisterMod()))
//    {
//        /* unregister OVERLAY */
//        s32Ret = CALL_RGN_UnRegisterMod(OVERLAYEX_RGN, HI_ID_PCIV);
//        HI_ASSERT(HI_SUCCESS == s32Ret);
//    }
//
//    s_u32PcivFmwState = PCIVFMW_STATE_STOPED;
    if(MOD_NOTICE_STOP == enNotice)
    {
        s_u32PcivFmwState = PCIVFMW_STATE_STOPPING;
    }
}

static HI_VOID PCIV_FmwQueryState(MOD_STATE_E *pstState)
{
    *pstState = MOD_STATE_FREE;
    return;
}

HI_S32 PCIV_FmwProcShow(osal_proc_entry_t *s)
{
    PCIV_FWMCHANNEL_S  *pstChnCtx;
    PCIV_CHN         pcivChn;
    HI_CHAR *pszPcivFieldStr[] = {"top","bottom","both"}; /* PCIV_PIC_FIELD_E */

    osal_seq_printf(s, "\n[PCIVF] Version: ["MPP_VERSION"], Build Time:["__DATE__", "__TIME__"]\n");

	osal_seq_printf(s, "\n-----MODULE_PARM----------------------------------------------------------------\n");
    osal_seq_printf(s,"%18s\n",
		              "drop_err_timeref");
	osal_seq_printf(s,"        %d\n",drop_err_timeref);	              
	
    osal_seq_printf(s, "\n-----PCIV FMW CHN INFO----------------------------------------------------------\n");
    osal_seq_printf(s, "%6s"     "%8s"   "%8s"    "%8s"     "%12s"    "%12s"    "%12s"    "%12s"    "%12s"  "%12s"     "%8s\n" ,
                  "PciChn", "Width","Height","Stride0","GetCnt","SendCnt","RespCnt","LostCnt","NewDo","OldUndo","PoolId0");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        osal_seq_printf(s, "%6d" "%8d" "%8d" "%8d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%8d\n",
            pcivChn,
            pstChnCtx->stPicAttr.u32Width,
            pstChnCtx->stPicAttr.u32Height,
            pstChnCtx->stPicAttr.u32Stride[0],
            pstChnCtx->u32GetCnt,
            pstChnCtx->u32SendCnt,
            pstChnCtx->u32RespCnt,
            pstChnCtx->u32LostCnt,
            pstChnCtx->u32NewDoCnt,
            pstChnCtx->u32OldUndoCnt,
            pstChnCtx->au32PoolId[0]);
    }

    osal_seq_printf(s, "\n-----PCIV FMW CHN PREPROC INFO--------------------------------------------------\n");
    osal_seq_printf(s, "%6s"     "%8s"    "%8s\n"
                 ,"PciChn", "FiltT", "Field");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        osal_seq_printf(s, "%6d" "%8d" "%8s\n",
            pcivChn,
            pstChnCtx->stPreProcCfg.enFilterType,
            pszPcivFieldStr[pstChnCtx->stPreProcCfg.enFieldSel]);
    }

    osal_seq_printf(s, "\n-----PCIV CHN QUEUE INFO----------------------------------------------------------\n");
    osal_seq_printf(s, "%6s"     "%8s"     "%8s"     "%8s"    "%12s\n",
                  "PciChn", "busynum","freenum","state", "Timer");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        osal_seq_printf(s, "%6d" "%8d" "%8d" "%8d" "%12d\n",
            pcivChn,
            pstChnCtx->stPicQueue.u32BusyNum,
            pstChnCtx->stPicQueue.u32FreeNum,
            pstChnCtx->enSendState,
            pstChnCtx->u32TimerCnt);
    }

    osal_seq_printf(s, "\n-----PCIV CHN CALL VGS INFO----------------------------------------------------------\n");
    osal_seq_printf(s, "%6s"     "%12s"     "%12s"    "%12s"   "%12s"     "%12s"      "%12s"     "%12s"     "%12s"     "%12s"     "%12s"     "%12s"    "%12s"  "%12s\n",
                  "PciChn", "JobSuc","JobFail","EndSuc", "EndFail", "MoveSuc", "MoveFail", "OsdSuc", "OsdFail", "ZoomSuc", "ZoomFail", "MoveCb", "OsdCb","ZoomCb");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        osal_seq_printf(s, "%6d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d\n",
            pcivChn,
            pstChnCtx->u32AddJobSucCnt,
            pstChnCtx->u32AddJobFailCnt,
            pstChnCtx->u32EndJobSucCnt,
            pstChnCtx->u32EndJobFailCnt,
            pstChnCtx->u32MoveTaskSucCnt,
            pstChnCtx->u32MoveTaskFailCnt,
            pstChnCtx->u32OsdTaskSucCnt,
            pstChnCtx->u32OsdTaskFailCnt,
            pstChnCtx->u32ZoomTaskSucCnt,
            pstChnCtx->u32ZoomTaskFailCnt,
            pstChnCtx->u32MoveCbCnt,
            pstChnCtx->u32OsdCbCnt,
            pstChnCtx->u32ZoomCbCnt);
    }

    return HI_SUCCESS;
}

static PCIV_FMWEXPORT_FUNC_S s_stExportFuncs = {
    .pfnPcivSendPic = PCIV_FirmWareViuSendPic,
};

static HI_U32 PCIV_GetVerMagic(HI_VOID)
{
	return VERSION_MAGIC;
}

static UMAP_MODULE_S s_stPcivFmwModule = {
    //.pstOwner 		= THIS_MODULE,
    .enModId  		= HI_ID_PCIVFMW,
	.aModName   	= "pcivfmw",

    .pfnInit        = PCIV_FmwInit,
    .pfnExit        = PCIV_FmwExit,
    .pfnQueryState  = PCIV_FmwQueryState,
    .pfnNotify      = PCIV_FmwNotify,
    .pfnVerChecker  = PCIV_GetVerMagic,

    .pstExportFuncs = &s_stExportFuncs,
    .pData          = HI_NULL,
};

static int pcivfmwOpen(void *private_data)
{
    return 0;
}

static int pcivfmwClose(void *private_data)
{
    return 0;
}

static long pcivfmwIoctl(unsigned int  cmd, unsigned long arg,void *private_data)
{
	return 0;
}

static struct osal_fileops pcivfmw_fops =
{
	.open		= pcivfmwOpen,
	.unlocked_ioctl = pcivfmwIoctl,
	.release    = pcivfmwClose
};
static int  PCIV_FmwModInit(void)
{
    //CMPI_PROC_ITEM_S *item;


#ifndef DISABLE_DEBUG_INFO
    osal_proc_entry_t *proc;
#endif
    if(1 != drop_err_timeref && 0 != drop_err_timeref)
    {
        osal_printk(  "PcivFirmWare module param (drop_err_timeref) should be 1 or 0 which is %d, it will be set to 1!!\n",drop_err_timeref);
        drop_err_timeref = 1;
    }
	 
    g_stPcivfmwumapd = osal_createdev(UMAP_DEVNAME_PCIVFMW_BASE);
    if(!g_stPcivfmwumapd)
	{
        osal_printk("create device err. \n");
        return HI_FAILURE;
	}
	g_stPcivfmwumapd->fops = &pcivfmw_fops;
    g_stPcivfmwumapd->minor = UMAP_PCIVFMW_MINOR_BASE;
	g_stPcivfmwumapd->osal_pmops = NULL;
    /*register all venc device*/
    if(osal_registerdevice(g_stPcivfmwumapd)<0)
    {
        osal_printk("Regist device err. \n");
		osal_destroydev(g_stPcivfmwumapd);
        return HI_FAILURE;
    }

    if (CMPI_RegisterModule(&s_stPcivFmwModule))
    {
        osal_deregisterdevice(g_stPcivfmwumapd);
		osal_destroydev(g_stPcivfmwumapd);
		osal_printk("Regist pciv firmware module err.\n");
        return HI_FAILURE;
    }
	
#ifndef DISABLE_DEBUG_INFO
    proc = osal_create_proc_entry(PROC_ENTRY_PCIVFMW, NULL);
    if(proc==NULL)
    {
        CMPI_UnRegisterModule(HI_ID_PCIVFMW);
		osal_deregisterdevice(g_stPcivfmwumapd);
		osal_destroydev(g_stPcivfmwumapd);
        HI_PRINT(  "Vpss module create proc failed\n");
        return HI_FAILURE;
    }
	proc->read = PCIV_FmwProcShow;
#endif	
    osal_printk("load pcivfmw.ko ...OK!\n");
    return HI_SUCCESS;
}


static void  PCIV_FmwModExit(void)
{	
	osal_remove_proc_entry(PROC_ENTRY_PCIVFMW, HI_NULL);
    CMPI_UnRegisterModule(HI_ID_PCIVFMW);
    osal_deregisterdevice(g_stPcivfmwumapd);
	osal_destroydev(g_stPcivfmwumapd);
	osal_printk("unload pcivfmw.ko ...OK!\n");
    return ;
}

EXPORT_SYMBOL(PCIV_FirmWareCreate);
EXPORT_SYMBOL(PCIV_FirmWareDestroy);
EXPORT_SYMBOL(PCIV_FirmWareSetAttr);
EXPORT_SYMBOL(PCIV_FirmWareStart);
EXPORT_SYMBOL(PCIV_FirmWareStop);
EXPORT_SYMBOL(PCIV_FirmWareWindowVbCreate);
EXPORT_SYMBOL(PCIV_FirmWareWindowVbDestroy);
EXPORT_SYMBOL(PCIV_FirmWareMalloc);
EXPORT_SYMBOL(PCIV_FirmWareFree);
EXPORT_SYMBOL(PCIV_FirmWareMallocChnBuffer);
EXPORT_SYMBOL(PCIV_FirmWareFreeChnBuffer);
EXPORT_SYMBOL(PCIV_FirmWareRecvPicAndSend);
EXPORT_SYMBOL(PCIV_FirmWareSrcPicFree);
EXPORT_SYMBOL(PCIV_FirmWareRegisterFunc);
EXPORT_SYMBOL(PCIV_FirmWareSetPreProcCfg);
EXPORT_SYMBOL(PCIV_FirmWareGetPreProcCfg);

module_param(drop_err_timeref, uint, S_IRUGO);
MODULE_PARM_DESC(drop_err_timeref, "whether drop err timeref video frame. 1:yes,0:no.");

module_init(PCIV_FmwModInit);
module_exit(PCIV_FmwModExit);

MODULE_AUTHOR(MPP_AUTHOR);
MODULE_LICENSE(MPP_LICENSE);
MODULE_VERSION(MPP_VERSION);

