/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv_firmware.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/08/27
  Description   : 
  History       :
  1.Date        : 2009/08/27
    Author      : P00123320
    Modification: Created file
  2.Date        : 2009/10/13
    Author      : P00123320
    Modification: AE6D04213
  3.Date        : 2009/10/15
    Author      : P00123320 
    Modification: Add synchronization  way  between VDEC/VO
  4.Date        : 2009/10/20
    Author      : P00123320 
    Modification: 
******************************************************************************/

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/pagemap.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "hi_comm_pciv.h"
#include "pciv_firmware_dcc.h"
#include "pciv_firmware.h"
#include "pciv_fmwext.h"
#include "simple_cb.h"
#include "hi_ipcm_usrdev.h"
#include "proc_ext.h"
#include "mm_ext.h"
#include "vb_ext.h"


typedef struct hiPCIV_FWM_DCCM_CHN_S
{
    HI_BOOL 			bCreate;
    HI_BOOL 			bStart;
	HI_BOOL 			bMaster;
	
	PCIV_BIND_TYPE_E 	enBindType;
	HI_U32 				u32CbPhyAddr;
    HI_VOID 			*pCbVirtAddr;
    SIMPLE_CB_S 		stPicCb;
    SIMPLE_CB_S 		stRecycleCb;     
    HI_BOOL 			bSendFail;
    PCIV_PIC_S 			stSrcPicBak;
} PCIV_FWM_DCCM_CHN_S;

static struct timer_list g_timerPcivDccmData;
static PCIV_FWM_DCCM_CHN_S s_astFwmDccmChn[PCIVFMW_MAX_CHN_NUM];
static PCIVFMW_CALLBACK_S g_stPcivFmwDccmCallBack;
static PCIV_VBPOOL_S     g_stDccmVbPool;

static spinlock_t           g_PcivFmwmLock;
#define PCIVFMWM_SPIN_LOCK   spin_lock_irqsave(&g_PcivFmwmLock,flags)
#define PCIVFMWM_SPIN_UNLOCK spin_unlock_irqrestore(&g_PcivFmwmLock,flags)

static HI_U32 timeout = 1;
module_param(timeout, uint, S_IRUGO);

static HI_S32 PcivFmwm_SendMsg(PCIVFMW_MSG_S *pstMsg);


/********************************************************************************************
The Function Below
********************************************************************************************/

HI_S32 PCIV_FirmWareRecvPicAndSend(PCIV_CHN pcivChn,PCIV_PIC_S * pRecvPic)
{
    HI_U32 u32FreeLen;
	unsigned long flags;
    PCIV_FWM_DCCM_CHN_S *pstChn;

    PCIVFMW_CHECK_PTR(pRecvPic);

	PCIVFMWM_SPIN_LOCK;
    pstChn = &s_astFwmDccmChn[pcivChn];
        
    if (!pstChn->bStart)
    {
    	PCIVFMWM_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR, "chn %d have stoped\n", pcivChn);
        return HI_ERR_PCIV_UNEXIST;
    }
    
    u32FreeLen = Simple_CB_QueryFree(&pstChn->stPicCb);
    if (u32FreeLen < sizeof(PCIV_PIC_S))
    {
    	PCIVFMWM_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR, "chn %d buf full\n", pcivChn);
        return HI_ERR_PCIV_BUF_FULL;
    }

    Simple_CB_Write(&pstChn->stPicCb, (HI_U8*)pRecvPic, sizeof(PCIV_PIC_S));
    PCIVFMWM_SPIN_UNLOCK;
	
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareSrcPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic)
{
    HI_U32 u32FreeLen;
    PCIV_FWM_DCCM_CHN_S *pstChn;
	
    PCIVFMW_CHECK_PTR(pSrcPic);
    
    pstChn = &s_astFwmDccmChn[pcivChn];

    /* Before the channel stop, releasing the resource is permitted*/
    if (!pstChn->bCreate)
    {
        PCIV_FMW_TRACE(HI_DBG_NOTICE,"chn %d have stoped\n",pcivChn);
        return HI_ERR_PCIV_UNEXIST;
    }
    
    u32FreeLen = Simple_CB_QueryFree(&pstChn->stRecycleCb);
    if (u32FreeLen < sizeof(PCIV_PIC_S))
    {
        PCIV_FMW_TRACE(HI_DBG_NOTICE,"chn %d buf full\n",pcivChn);
        return HI_ERR_PCIV_BUF_FULL;
    }

    Simple_CB_Write(&pstChn->stRecycleCb, (HI_U8*)pSrcPic, sizeof(PCIV_PIC_S));
	
    return HI_SUCCESS;
}

HI_S32 PcivFmws_QueryPcivChnShareBufState(PCIV_CHN PcivChn)
{
	return g_stPcivFmwDccmCallBack.pfQueryPcivChnShareBufState(PcivChn);
}


HI_VOID PcivFmwm_TimerFunc(unsigned long data)
{
    HI_S32 				i;
	HI_S32 				s32Ret;
    HI_U32 				u32Len;
    PCIV_CHN 			pcivChn;
	unsigned long 		flags;
	PCIV_PIC_S 			stSrcPic  = {0};
	PCIV_PIC_S 			stRecvPic = {0};
    PCIV_FWM_DCCM_CHN_S *pstChn;    
    
    g_timerPcivDccmData.expires  = jiffies + msecs_to_jiffies(PCIV_TIMER_EXPIRES);
    g_timerPcivDccmData.function = PcivFmwm_TimerFunc;
    g_timerPcivDccmData.data     = 0;
    if (!timer_pending(&g_timerPcivDccmData))
    {
        add_timer(&g_timerPcivDccmData);
    }

    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
    	PCIVFMWM_SPIN_LOCK;
		pcivChn = i;
	    pstChn = &s_astFwmDccmChn[pcivChn];
			
		if (HI_TRUE == pstChn->bMaster)
		{
			if (HI_TRUE != pstChn->bStart)
	        {
	        	PCIVFMWM_SPIN_UNLOCK;
	            continue;
	        }	        
		
	        /* Query the channel has valid data to read*/
			while(Simple_CB_QueryBusy(&pstChn->stRecycleCb) >= sizeof(stRecvPic))
			{
		        Simple_CB_Read(&pstChn->stRecycleCb, (HI_U8*)&stRecvPic, sizeof(stRecvPic));

		        if (!g_stPcivFmwDccmCallBack.pfRecvPicFree) 
				{
					PCIVFMWM_SPIN_UNLOCK;
					continue;    
		        }
			
		        s32Ret = g_stPcivFmwDccmCallBack.pfRecvPicFree(pcivChn, &stRecvPic);
		        PCIV_FMW_TRACE(HI_DBG_DEBUG, "call func of free receive pic ok \n");
			}
		}
		else
		{	        
			if (HI_TRUE != pstChn->bStart)
			{
				if ((HI_TRUE == pstChn->bSendFail) && (HI_TRUE == pstChn->bCreate))
				{	
					memcpy(&stSrcPic, &pstChn->stSrcPicBak, sizeof(PCIV_PIC_S));
					PCIV_FirmWareSrcPicFree(pcivChn, &stSrcPic);
					pstChn->bSendFail  = HI_FALSE;
				}
				
				PCIVFMWM_SPIN_UNLOCK;
	            continue;
			}
			else
			{
				if (pstChn->bSendFail != HI_TRUE)
		        {                
		            /* Query the channel has valid data to read*/
		            u32Len = Simple_CB_QueryBusy(&pstChn->stPicCb);
		            if (u32Len < sizeof(stSrcPic))
		            {
		            	PCIVFMWM_SPIN_UNLOCK;
		                continue;
		            }
					/* read date from share buffer */
		            Simple_CB_Read(&pstChn->stPicCb, (HI_U8*)&stSrcPic, sizeof(stSrcPic));
		        }
				/*If last time send failed, use the backups*/
		        else
		        {
		            memcpy(&stSrcPic, &pstChn->stSrcPicBak, sizeof(PCIV_PIC_S));
		        }

				/*Send Image to PCI remote object*/
		        pstChn->bSendFail  = HI_FALSE;
				pstChn->enBindType = stSrcPic.enSrcType;

		        /* callback PCIV interface PCIV_SrcPicSend()*/
		        if (!g_stPcivFmwDccmCallBack.pfSrcSendPic)  
		        {
		        	PCIVFMWM_SPIN_UNLOCK;
		            continue;          
		        }
			
		        s32Ret = g_stPcivFmwDccmCallBack.pfSrcSendPic(pcivChn, &stSrcPic);
		        if (s32Ret != HI_SUCCESS)
		        {   
					/*if the Image rource from VDEC,use the backup date send again next time  when send failed  */
		            if (PCIV_BIND_VDEC == pstChn->enBindType)
		            {
		                memcpy(&pstChn->stSrcPicBak, &stSrcPic, sizeof(PCIV_PIC_S));
		                pstChn->bSendFail = HI_TRUE;
		            }
					/*if the Image rource from VI,drop the Image  when send failed  */
					else
		            {
		                PCIV_FirmWareSrcPicFree(pcivChn, &stSrcPic);
		            }                
		        }
				
        		PCIV_FMW_TRACE(HI_DBG_DEBUG, "call func of sending src pci ok \n");
			}
		}

		PCIVFMWM_SPIN_UNLOCK;
		
    }
}

static HI_S32 PcivFmwm_CbInit(PCIV_CHN pcivChn, HI_U32 u32CbSize)
{
    HI_S32 				s32Ret = HI_SUCCESS;
	char     			acName[16] =  {0};
    HI_CHAR 			*pMmzName = HI_NULL;
	PCIV_FWM_DCCM_CHN_S *pstChn;
	
    pstChn = &s_astFwmDccmChn[pcivChn];
    
	/*alloc Image buffer and shared memery of recovery bank*/
	snprintf(acName, 16, "Pciv(%d)FmwmCb", pcivChn);
    s32Ret = CMPI_MmzMallocNocache(pMmzName, acName, &pstChn->u32CbPhyAddr, &pstChn->pCbVirtAddr, u32CbSize);
    if (HI_SUCCESS != s32Ret)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Channel %d malloc and remap memory fail\n", pcivChn);
        return HI_ERR_PCIV_NOMEM;
    }
	
    s32Ret = Simple_CB_Init(&pstChn->stPicCb, pstChn->u32CbPhyAddr, u32CbSize/2);
    if (HI_SUCCESS != s32Ret)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Channel %d init buffer fail\n", pcivChn);
        (HI_VOID)Simple_CB_DeInit(&pstChn->stPicCb);
        (HI_VOID)Simple_CB_DeInit(&pstChn->stRecycleCb);
        	    
        CMPI_MmzFree(pstChn->u32CbPhyAddr, pstChn->pCbVirtAddr);
		pstChn->pCbVirtAddr = NULL;
        pstChn->u32CbPhyAddr = 0;
		
        return HI_ERR_PCIV_NOMEM;
    }
	
	 /*Initialed shared memery of recovery bank*/
    s32Ret = Simple_CB_Init(&pstChn->stRecycleCb, pstChn->u32CbPhyAddr+(u32CbSize/2), u32CbSize/2);
    if (HI_SUCCESS != s32Ret)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Channel %d init buffer fail\n", pcivChn);
        (HI_VOID)Simple_CB_DeInit(&pstChn->stPicCb);
        (HI_VOID)Simple_CB_DeInit(&pstChn->stRecycleCb);
		
        CMPI_MmzFree(pstChn->u32CbPhyAddr, pstChn->pCbVirtAddr);
		pstChn->pCbVirtAddr = NULL;
        pstChn->u32CbPhyAddr = 0;
		
        return HI_ERR_PCIV_NOMEM;
    }

    PCIV_FMW_TRACE(HI_DBG_INFO, "CbInit ok, Pic:0x%x, Recycle:0x%x, size:%d \n", 
        pstChn->stPicCb.u32PhyAddr, pstChn->stRecycleCb.u32PhyAddr, (u32CbSize/2));

    return HI_SUCCESS;
}

static HI_S32 PcivFwmm_CbExit(PCIV_CHN pcivChn)
{
    PCIV_FWM_DCCM_CHN_S *pstChn;

    pstChn = &s_astFwmDccmChn[pcivChn];

    Simple_CB_DeInit(&pstChn->stRecycleCb);
    Simple_CB_DeInit(&pstChn->stPicCb);
    
    CMPI_MmzFree(pstChn->u32CbPhyAddr, pstChn->pCbVirtAddr);
	pstChn->pCbVirtAddr = NULL;
    pstChn->u32CbPhyAddr = 0;
    
    PCIV_FMW_TRACE(HI_DBG_INFO, "CbExit ok\n");
    return HI_SUCCESS;
}


/********************************************************************************************/
HI_S32 PCIV_FirmWareCreate(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr, HI_S32 s32LocalId)
{
    HI_S32 s32Ret;    
    PCIVFMW_MSG_S stMsg;
    PCIVFMW_MSG_CREATE_S stCreateMsg;    
    HI_U32 u32CbSize = (sizeof(PCIV_PIC_S) * pAttr->u32Count) * 2;    /* Two CB */

    PCIVFMW_CHECK_CHNID(pcivChn);

	/*Initialed shared buffer*/
    s32Ret = PcivFmwm_CbInit(pcivChn, u32CbSize);
    if (s32Ret != HI_SUCCESS)
    {
        return s32Ret;
    }

	if (0 == s32LocalId)
	{
		s_astFwmDccmChn[pcivChn].bMaster = HI_TRUE;
	}
	else
	{
		s_astFwmDccmChn[pcivChn].bMaster = HI_FALSE;
	}

	/*share buffer info should send to slave arm*/
    stCreateMsg.u32CbSize  = u32CbSize;
    stCreateMsg.u32PhyAddr = s_astFwmDccmChn[pcivChn].stPicCb.u32PhyAddr;
	stCreateMsg.s32LocalId = s32LocalId;
    memcpy(&stCreateMsg.stPcivAttr, pAttr, sizeof(PCIV_ATTR_S));
        
    stMsg.stHead.pcivChn   = pcivChn;
    stMsg.stHead.u32MsgLen = sizeof(PCIVFMW_MSG_CREATE_S);
    stMsg.stHead.enMsgType = PCIVFMW_MSGTYPE_CREATE;
    memcpy(stMsg.cMsgBody, &stCreateMsg, sizeof(PCIVFMW_MSG_CREATE_S));
    
    s32Ret = PcivFmwm_SendMsg(&stMsg);
    if (s32Ret)
    {
        PcivFwmm_CbExit(pcivChn);
        return s32Ret;
    }
    
    s_astFwmDccmChn[pcivChn].bCreate = HI_TRUE;
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareDestroy(PCIV_CHN pcivChn)
{
    HI_S32 s32Ret;    
    PCIVFMW_MSG_S stMsg;
    PCIV_FWM_DCCM_CHN_S *pstChn;
    
    PCIVFMW_CHECK_CHNID(pcivChn);

    pstChn = &s_astFwmDccmChn[pcivChn];

	pstChn->bCreate = HI_FALSE;
    
	/*send message to notice slave destroy its channel*/
    stMsg.stHead.pcivChn   = pcivChn;
    stMsg.stHead.u32MsgLen = 0;
    stMsg.stHead.enMsgType = PCIVFMW_MSGTYPE_DESTROY;
    s32Ret = PcivFmwm_SendMsg(&stMsg);
    if (s32Ret)
    {
        return s32Ret;
    }

	/*Deinitiale the shared memery between two nucleus*/
    PcivFwmm_CbExit(pcivChn);
    
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareSetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr, HI_S32 s32LocalId)
{
    HI_S32 s32Ret;    
    PCIVFMW_MSG_S stMsg;
	PCIVFMW_MSG_SETATTR_S stSetAttrMsg;

    PCIVFMW_CHECK_CHNID(pcivChn);
    PCIVFMW_CHECK_PTR(pAttr);
    
    stMsg.stHead.pcivChn   = pcivChn;    
    stMsg.stHead.u32MsgLen = sizeof(PCIVFMW_MSG_SETATTR_S);
    stMsg.stHead.enMsgType = PCIVFMW_MSGTYPE_SETATTR;

	stSetAttrMsg.s32LocalId = s32LocalId;
	memcpy(&(stSetAttrMsg.stAttr), pAttr, sizeof(PCIV_ATTR_S));
	
    memcpy(stMsg.cMsgBody, &stSetAttrMsg, sizeof(PCIVFMW_MSG_SETATTR_S));
    
    s32Ret = PcivFmwm_SendMsg(&stMsg);
    if (s32Ret)
    {
        return s32Ret;
    }
        
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareSetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    HI_S32 s32Ret;    
    PCIVFMW_MSG_S stMsg;

    PCIVFMW_CHECK_CHNID(pcivChn);
    
    stMsg.stHead.pcivChn   = pcivChn;
    stMsg.stHead.u32MsgLen = sizeof(PCIV_PREPROC_CFG_S);
    stMsg.stHead.enMsgType = PCIVFMW_MSGTYPE_SETVPPCFG;
    memcpy(stMsg.cMsgBody, pAttr, sizeof(PCIV_PREPROC_CFG_S));
    
    s32Ret = PcivFmwm_SendMsg(&stMsg);
    if (s32Ret)
    {
        return s32Ret;
    }
        
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareGetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    HI_S32 s32Ret;    
    PCIVFMW_MSG_S stMsg;

    PCIVFMW_CHECK_CHNID(pcivChn);
    
    stMsg.stHead.pcivChn   = pcivChn;
    stMsg.stHead.u32MsgLen = sizeof(PCIV_PREPROC_CFG_S);
    stMsg.stHead.enMsgType = PCIVFMW_MSGTYPE_GETVPPCFG;
    memcpy(stMsg.cMsgBody, pAttr, sizeof(PCIV_PREPROC_CFG_S));

    s32Ret = PcivFmwm_SendMsg(&stMsg);
    if (s32Ret)
    {
        return s32Ret;
    }

    memcpy(pAttr, stMsg.cMsgBody, sizeof(PCIV_PREPROC_CFG_S));
    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareStart(PCIV_CHN pcivChn)
{
    HI_S32 s32Ret;    
    PCIVFMW_MSG_S stMsg;

    PCIVFMW_CHECK_CHNID(pcivChn);

    s_astFwmDccmChn[pcivChn].bStart = HI_TRUE;
    
    stMsg.stHead.pcivChn   = pcivChn;
    stMsg.stHead.u32MsgLen = 0;
    stMsg.stHead.enMsgType = PCIVFMW_MSGTYPE_START;
    s32Ret = PcivFmwm_SendMsg(&stMsg);
    if (s32Ret)
    {
        return s32Ret;
    }
        
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareStop(PCIV_CHN pcivChn)
{
    HI_S32 s32Ret;    
    PCIVFMW_MSG_S stMsg;

    PCIVFMW_CHECK_CHNID(pcivChn);
    
    s_astFwmDccmChn[pcivChn].bStart = HI_FALSE;
    
    stMsg.stHead.pcivChn   = pcivChn;
    stMsg.stHead.u32MsgLen = 0;
    stMsg.stHead.enMsgType = PCIVFMW_MSGTYPE_STOP;
    s32Ret = PcivFmwm_SendMsg(&stMsg);
    if (s32Ret)
    {
        s_astFwmDccmChn[pcivChn].bStart = HI_TRUE;
        return s32Ret;
    }    

    return HI_SUCCESS;
}

	
HI_S32 PCIV_FirmWareWindowVbCreate(PCIV_WINVBCFG_S *pCfg)
{
	HI_S32	s32Ret, i;
	HI_U32	u32PoolId;
	HI_CHAR *pBufName = "PcivVbFromWindow";

	if( g_stDccmVbPool.u32PoolCount != 0)
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
			PCIV_FMW_TRACE(HI_DBG_ALERT, "Create pool(Index=%d, Cnt=%d, Size=%d) fail\n",
								  i, pCfg->u32BlkCount[i], pCfg->u32BlkSize[i]);
			break;
		}
		g_stDccmVbPool.u32PoolCount = i + 1;
		g_stDccmVbPool.u32PoolId[i] = u32PoolId;
		g_stDccmVbPool.u32Size[i]	= pCfg->u32BlkSize[i];
	}

	/*if one pool alloc failed, then rolling back*/
	if ( g_stDccmVbPool.u32PoolCount != pCfg->u32PoolCount)
	{
		for(i=0; i < g_stDccmVbPool.u32PoolCount; i++)
		{
			(HI_VOID)VB_DestroyPool(g_stDccmVbPool.u32PoolId[i]);
			g_stDccmVbPool.u32PoolId[i] = VB_INVALID_POOLID;
		}

		g_stDccmVbPool.u32PoolCount = 0;

		return HI_ERR_PCIV_NOMEM;
	}

	return HI_SUCCESS;
}

	
HI_S32 PCIV_FirmWareWindowVbDestroy(HI_VOID)
{
	HI_S32 i;

	for(i=0; i < g_stDccmVbPool.u32PoolCount; i++)
	{
		(HI_VOID)VB_DestroyPool(g_stDccmVbPool.u32PoolId[i]);
		g_stDccmVbPool.u32PoolId[i] = VB_INVALID_POOLID;
	}

	g_stDccmVbPool.u32PoolCount = 0;
	return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareMalloc(HI_U32 u32Size, HI_S32 s32LocalId, HI_U32 *pPhyAddr)
{
	HI_S32		 i;
	HI_S32 		 s32Ret;
	VB_BLKHANDLE handle = VB_INVALID_HANDLE;

	PCIVFMW_CHECK_PTR(pPhyAddr);

	if(s32LocalId == 0)
	{
		PCIVFMW_MSG_S stMsg;    
		PCIVFMW_MSG_MALLOC_S stMallocArg;    
		PCIVFMW_MSG_MALLOC_S *pstMallocArgOut;        
		            
		stMallocArg.u32Size 	= u32Size;    
		stMallocArg.s32LocalId 	= s32LocalId;  
		stMallocArg.u32PhyAddr  = 0;
		stMsg.stHead.pcivChn   	= 0;    
		stMsg.stHead.u32MsgLen 	= sizeof(PCIVFMW_MSG_MALLOC_S);    
		stMsg.stHead.enMsgType 	= PCIVFMW_MSGTYPE_MALLOC;    
		memcpy(stMsg.cMsgBody, &stMallocArg, sizeof(PCIVFMW_MSG_MALLOC_S));        
		s32Ret = PcivFmwm_SendMsg(&stMsg);    
		if (s32Ret)    
		{    
			PCIV_FMW_TRACE(HI_DBG_ERR,"PCIVFMW_MSGTYPE_MALLOC fail,s32Ret:0x%x!\n", s32Ret);
			return s32Ret;    
		}    
		
		/*Get output return value*/
		pstMallocArgOut = (PCIVFMW_MSG_MALLOC_S*)(stMsg.cMsgBody);    
		*pPhyAddr = pstMallocArgOut->u32PhyAddr; 		

		return HI_SUCCESS;
	}

	/* if the chip is slave, the alloc memery from special VB(Window)*/
	for(i=0; i < g_stDccmVbPool.u32PoolCount; i++)
	{
		if(u32Size > g_stDccmVbPool.u32Size[i]) 
		{
			continue;
		}

		handle = VB_GetBlkByPoolId(g_stDccmVbPool.u32PoolId[i], VB_UID_PCIV);
		if(VB_INVALID_HANDLE != handle) 
		{
			break;
		}
	}

	if(VB_INVALID_HANDLE == handle)
	{
		PCIV_FMW_TRACE(HI_DBG_ERR,"VB_GetBlkBySize fail,size:%d!\n", u32Size);
		return HI_ERR_PCIV_NOBUF;
	}

	*pPhyAddr = VB_Handle2Phys(handle);
	return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareFree(HI_S32 s32LocalId, HI_U32 u32PhyAddr)
{
	HI_S32        s32Ret;
    VB_BLKHANDLE  vbHandle;

	if (0 == s32LocalId)
	{    
		PCIVFMW_MSG_S stMsg;    
		PCIVFMW_MSG_FREE_S stFreeArg;   
		stFreeArg.s32LocalId 	= s32LocalId;
		stFreeArg.u32PhyAddr 	= u32PhyAddr;            
		stMsg.stHead.pcivChn   	= 0;        
		stMsg.stHead.u32MsgLen 	= sizeof(PCIVFMW_MSG_FREE_S);    
		stMsg.stHead.enMsgType 	= PCIVFMW_MSGTYPE_FREE;    
		memcpy(stMsg.cMsgBody, &stFreeArg, sizeof(PCIVFMW_MSG_FREE_S));        
		s32Ret = PcivFmwm_SendMsg(&stMsg);    
		if (s32Ret)    
		{     
			PCIV_FMW_TRACE(HI_DBG_ERR,"PCIVFMW_MSGTYPE_MALLOC fail,s32Ret:0x%x!\n", s32Ret);
			return s32Ret;    
		}            

		return HI_SUCCESS;
	}
	
    vbHandle = VB_Phy2Handle(u32PhyAddr);
    if(VB_INVALID_HANDLE == vbHandle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Invalid Physical Address 0x%08x\n", u32PhyAddr);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    s32Ret = VB_UserSub(VB_Handle2PoolId(vbHandle), u32PhyAddr, VB_UID_PCIV);
	
	return s32Ret;
}


HI_S32 PCIV_FirmWareMallocChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_U32 u32Size, HI_S32 s32LocalId, HI_U32 *pPhyAddr)
{
	HI_S32 s32Ret;    
    PCIVFMW_MSG_S stMsg;
    PCIVFMW_MSG_CHN_MALLOC_S stChnMallocArg;
    PCIVFMW_MSG_CHN_MALLOC_S *pstChnMallocArgOut;
    
    PCIVFMW_CHECK_PTR(pPhyAddr);

	stChnMallocArg.u32Index 	= u32Index;
    stChnMallocArg.u32Size 		= u32Size;
    stChnMallocArg.s32LocalId 	= s32LocalId;
	stChnMallocArg.u32PhyAddr   = 0;
	
    stMsg.stHead.pcivChn   = pcivChn;
    stMsg.stHead.u32MsgLen = sizeof(PCIVFMW_MSG_CHN_MALLOC_S);
    stMsg.stHead.enMsgType = PCIVFMW_MSGTYPE_CHNMALLOC;
    memcpy(stMsg.cMsgBody, &stChnMallocArg, sizeof(PCIVFMW_MSG_CHN_MALLOC_S));
    
    s32Ret = PcivFmwm_SendMsg(&stMsg);
    if (s32Ret)
    {
        return s32Ret;
    }

	/*Get output return value*/
    pstChnMallocArgOut = (PCIVFMW_MSG_CHN_MALLOC_S*)(stMsg.cMsgBody);
    *pPhyAddr = pstChnMallocArgOut->u32PhyAddr;    
        
    return HI_SUCCESS;

}


HI_S32 PCIV_FirmWareFreeChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_S32 s32LocalId)
{
	HI_S32 s32Ret;    
    PCIVFMW_MSG_S stMsg;
    PCIVFMW_MSG_CHN_FREE_S stChnFreeArg;
    
    stChnFreeArg.u32Index 	= u32Index;
	stChnFreeArg.s32LocalId = s32LocalId;
        
    stMsg.stHead.pcivChn   = pcivChn;    
    stMsg.stHead.u32MsgLen = sizeof(PCIVFMW_MSG_CHN_FREE_S);
    stMsg.stHead.enMsgType = PCIVFMW_MSGTYPE_CHNFREE;
    memcpy(stMsg.cMsgBody, &stChnFreeArg, sizeof(PCIVFMW_MSG_CHN_FREE_S));
    
    s32Ret = PcivFmwm_SendMsg(&stMsg);
    if (s32Ret)
    {
        return s32Ret;
    }
        
    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareRegisterFunc(PCIVFMW_CALLBACK_S *pstCallBack)
{
    PCIVFMW_CHECK_PTR(pstCallBack);
    PCIVFMW_CHECK_PTR(pstCallBack->pfSrcSendPic);
    PCIVFMW_CHECK_PTR(pstCallBack->pfRecvPicFree);
	PCIVFMW_CHECK_PTR(pstCallBack->pfQueryPcivChnShareBufState);
	
    memcpy(&g_stPcivFmwDccmCallBack, pstCallBack, sizeof(PCIVFMW_CALLBACK_S));

    return HI_SUCCESS;
}

/********************************************************************************************/

typedef struct hiPCIV_FWMM_IPCM_CTL_S
{
    HI_BOOL             bHaveEcho;
    struct semaphore    stSem;
    wait_queue_head_t   stWait;
    HI_S32              s32RetVal;
    HI_S32              s32RetMsgLen;
    HI_U8               cMsgBody[PCIVFMW_MSG_MAXLEN];
} PCIV_FWMM_IPCM_CTL_S;

static hios_ipcm_handle_t 	*g_hPcivMIpcmHnd;
PCIV_FWMM_IPCM_CTL_S 		g_stPcivIpcm;


/* 
 * Send messeg to slave-ARM, and wait the echo from remote object
 * the return value is  the value from slave-ARM,pstMs->cMsgBody contain output value(if exist)
 */
static HI_S32 PcivFmwm_SendMsg(PCIVFMW_MSG_S *pstMsg)
{
    HI_S32 s32Ret;

	/*check the legal of the message len*/
    if (pstMsg->stHead.u32MsgLen >= PCIVFMW_MSG_MAXLEN)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "msg len %d is larger than %d\n", pstMsg->stHead.u32MsgLen, PCIVFMW_MSG_MAXLEN);
        return HI_FAILURE;
    }
    
	/*must use sem to protect, can insure the message send serial*/
    if (down_interruptible(&g_stPcivIpcm.stSem))
    {
        return -ERESTARTSYS;
    }
    
    pstMsg->stHead.u32Magic = PCIVFMW_MSG_MAGIC;

	/*send message to slave-ARM*/
    s32Ret = hios_ipcm_sendto(g_hPcivMIpcmHnd, pstMsg, pstMsg->stHead.u32MsgLen + sizeof(PCIVFMW_MSGHEAD_S));
    if (s32Ret < 0)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, " hios_ipcm_sendto err 0x%x !!! \n", s32Ret);
        up(&g_stPcivIpcm.stSem);
        return s32Ret;
    }
    PCIV_FMW_TRACE(HI_DBG_NOTICE, "send msg --> type:%s, chn:%d, len:%d \n",
        STR_PCIVFMW_MSGTYPE(pstMsg->stHead.enMsgType), pstMsg->stHead.pcivChn, pstMsg->stHead.u32MsgLen);

	/*Waiting until wakeup by the interrupt from echo message */
    s32Ret = wait_event_interruptible_timeout(g_stPcivIpcm.stWait,(g_stPcivIpcm.bHaveEcho), (timeout*HZ));
    if (s32Ret <= 0) 
    {
        printk("%s time out,ret:0x%x \n", __FUNCTION__, s32Ret);
        up(&g_stPcivIpcm.stSem);
        return HI_FAILURE;
    }

	/*Get output return value*/
    s32Ret = g_stPcivIpcm.s32RetVal;
    	
	/*Get output parameter*/
    if (g_stPcivIpcm.s32RetMsgLen > 0)
    {
        memcpy(pstMsg->cMsgBody, g_stPcivIpcm.cMsgBody, g_stPcivIpcm.s32RetMsgLen);
    }

    PCIV_FMW_TRACE(HI_DBG_INFO, "receive ret:0x%x, datalen:%d \n", s32Ret, g_stPcivIpcm.s32RetMsgLen);
    
    g_stPcivIpcm.bHaveEcho = HI_FALSE;
    
    up(&g_stPcivIpcm.stSem);
    return s32Ret;
}

static int PcivFmwm_IpcmISR(void *handle, void *buf, unsigned int len)
{
    PCIVFMW_MSG_S *pstMsg;

    HI_ASSERT(!((int)buf&0x3));
    
    pstMsg = (PCIVFMW_MSG_S*)buf;

    if (pstMsg->stHead.u32Magic != PCIVFMW_MSG_MAGIC || pstMsg->stHead.enMsgType != PCIVFMW_MSGTYPE_CMDECHO)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "err, magic:0x%x, msgtype:%s\n",
            pstMsg->stHead.u32Magic,STR_PCIVFMW_MSGTYPE(pstMsg->stHead.enMsgType));
        return -1;
    }

	/*set return value*/
    g_stPcivIpcm.s32RetVal = pstMsg->stHead.s32RetVal;

	/*set output parameter*/
    g_stPcivIpcm.s32RetMsgLen = len - sizeof(PCIVFMW_MSGHEAD_S);
    memcpy(g_stPcivIpcm.cMsgBody, pstMsg->cMsgBody, g_stPcivIpcm.s32RetMsgLen);

    /* wakeup  process*/
    g_stPcivIpcm.bHaveEcho = HI_TRUE;
    wake_up_interruptible(&g_stPcivIpcm.stWait);
    
    return HI_SUCCESS;
}

static int PcivFmwm_IpcmInit(HI_VOID)
{
    struct hi_ipcm_handle_attr attr;
    hios_ipcm_handle_opt_t opt;

    attr.target_id = 1;//DCCS_CPU_ID;
    attr.priority = 2;
    attr.port = 1;//DCC_PCIV_PORT;
    g_hPcivMIpcmHnd = hios_ipcm_open(&attr);
    if (!g_hPcivMIpcmHnd)
    {
        printk("hios_ipcm_open fail\n");
        return HI_FAILURE;
    }
        
    opt.recvfrom_notify = PcivFmwm_IpcmISR;
    opt.data = 0;
    if (hios_ipcm_setopt(g_hPcivMIpcmHnd, &opt))
    {
        printk("hios_ipcm_setopt fail\n");
        hios_ipcm_close(g_hPcivMIpcmHnd);
        return HI_FAILURE;
    }
    
    return HI_SUCCESS;
}

static int PcivFmwm_IpcmExit(void)
{
    hios_ipcm_close(g_hPcivMIpcmHnd);
    
    return HI_SUCCESS;
}

static int __init PCIV_Fmwm_ModInit(void)
{   
    HI_S32 i, s32Ret;

    s32Ret = PcivFmwm_IpcmInit();
    if (s32Ret != HI_SUCCESS)
    {
        return s32Ret;
    }

	spin_lock_init(&g_PcivFmwmLock);
	sema_init(&g_stPcivIpcm.stSem, 1);
    init_waitqueue_head(&g_stPcivIpcm.stWait);

	/*initial the timer use to query shared buffer data*/
	init_timer(&g_timerPcivDccmData);
	g_timerPcivDccmData.expires  = jiffies + msecs_to_jiffies(PCIV_TIMER_EXPIRES);
	g_timerPcivDccmData.function = PcivFmwm_TimerFunc;
	g_timerPcivDccmData.data     = 0;
	add_timer(&g_timerPcivDccmData);
		
    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
        memset(&s_astFwmDccmChn[i], 0, sizeof(PCIV_FWM_DCCM_CHN_S));
        s_astFwmDccmChn[i].bCreate = HI_FALSE;
        s_astFwmDccmChn[i].enBindType = PCIV_BIND_BUTT;
        s_astFwmDccmChn[i].stPicCb.u32PhyAddr = 0;
        s_astFwmDccmChn[i].stRecycleCb.u32PhyAddr = 0;
    }

	g_stDccmVbPool.u32PoolCount = 0;
	
    return HI_SUCCESS;
}

static void __exit PCIV_Fmwm_ModExit(void)
{
    PcivFmwm_IpcmExit();
    
    del_timer_sync(&g_timerPcivDccmData);
    
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


module_init(PCIV_Fmwm_ModInit);
module_exit(PCIV_Fmwm_ModExit);

MODULE_AUTHOR(MPP_AUTHOR);
MODULE_LICENSE(MPP_LICENSE);
MODULE_VERSION(MPP_VERSION);

