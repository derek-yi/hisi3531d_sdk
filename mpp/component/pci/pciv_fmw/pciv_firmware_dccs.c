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
  2.Date        : 2009/10/15
    Author      : P00123320 
    Modification:Add synchronization  way  between VDEC/VO
  3.Date        : 2009/10/20
    Author      : P00123320 
    Modification: release source when destroy channel
  4.Date        : 2009/10/24
    Author      : P00123320 
    Modification: all messeage deal function is changed to to in kernel state, and use fuction-hander
******************************************************************************/

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "hi_comm_pciv.h"
#include "hi_comm_vo.h"
#include "pciv_firmware_dcc.h"
#include "pciv_fmwext.h"
#include "simple_cb.h"
#include "hi_ipcm_usrdev.h"
#include "proc_ext.h"
#include "mod_ext.h"

typedef struct hiPCIV_FWM_DCCS_CHN_S
{
    HI_BOOL 			bCreate;
    HI_BOOL 			bStart;
	HI_BOOL 			bMaster;

    SIMPLE_CB_S 		stPicCb;
    SIMPLE_CB_S 		stRecycleCb;
} PCIV_FWM_DCCS_CHN_S;

static struct timer_list 	g_timerPcivDccsData;
static PCIV_FWM_DCCS_CHN_S  s_astFwmDccsChn[PCIVFMW_MAX_CHN_NUM];


/********************************************************************************************
The Function Below
********************************************************************************************/


/* this functio is register to  PcivFmw module,  in the callback of vgs the function is execute, write the Image info to shared memery */
HI_S32 PcivFmws_SrcPicSend(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic)
{
    HI_U32 				u32FreeLen;
    PCIV_FWM_DCCS_CHN_S *pstChn;
    PCIV_PIC_S 			stSrcPic;

    PCIV_CHECK_PTR(pSrcPic);
    
    pstChn = &s_astFwmDccsChn[pcivChn];
        
    if (!pstChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_NOTICE,"chn %d have stoped\n",pcivChn);
        return HI_ERR_PCIV_UNEXIST;
    }
    
    u32FreeLen = Simple_CB_QueryFree(&pstChn->stPicCb);
    if (u32FreeLen < sizeof(PCIV_PIC_S))
    {
        if (pSrcPic->enSrcType==PCIV_BIND_VI) 
        {
            PCIV_FMW_TRACE(HI_DBG_ERR,"chn %d buf full,len:%d\n",pcivChn,u32FreeLen);
        }
        return HI_ERR_PCIV_BUF_FULL;
    }

    memcpy(&stSrcPic, pSrcPic, sizeof(PCIV_PIC_S));
    Simple_CB_Write(&pstChn->stPicCb, (HI_U8*)&stSrcPic, sizeof(stSrcPic));

    return HI_SUCCESS;
}


HI_S32 PcivFmws_RecvPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pRecvPic)
{
    HI_U32 u32FreeLen;
    PCIV_FWM_DCCS_CHN_S *pstChn;

    PCIV_CHECK_PTR(pRecvPic);
    
    pstChn = &s_astFwmDccsChn[pcivChn];
        
    if (!pstChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR,"chn %d have stoped\n",pcivChn);
        return HI_ERR_PCIV_UNEXIST;
    }
    
    u32FreeLen = Simple_CB_QueryFree(&pstChn->stRecycleCb);
    if (u32FreeLen < sizeof(PCIV_PIC_S))
    {
        //PCIV_FMW_TRACE(HI_DBG_ERR,"pcivfmw chn%d cb buf full, when free recv pic\n",pcivChn);
        return HI_ERR_PCIV_BUF_FULL;
    }

    Simple_CB_Write(&pstChn->stRecycleCb, (HI_U8*)pRecvPic, sizeof(PCIV_PIC_S));

    return HI_SUCCESS;
}

HI_S32 PcivFmws_QueryPcivChnShareBufState(PCIV_CHN pcivChn)
{
	HI_U32				u32FreeLen = 0;
	PCIV_FWM_DCCS_CHN_S *pstChn;
	
	pstChn = &s_astFwmDccsChn[pcivChn];
	
	u32FreeLen = Simple_CB_QueryFree(&pstChn->stPicCb);
    if (u32FreeLen < sizeof(PCIV_PIC_S))
    {
        return HI_FAILURE;
    }
	
	return HI_SUCCESS;
}


/*the slave and master cpu should deal separate,slave cpu in slave chip should drive by timer to release the image 
finished used, the slave cpu in mater chip should send date to VO/VENC/VPSS at the back*/
HI_VOID PcivFmws_TimerFunc(unsigned long data)
{
    HI_U32 				i;
    PCIV_CHN 			pcivChn;
	PCIV_PIC_S 			stSrcPic  = {0};
	PCIV_PIC_S 			stRecvPic = {0};
    PCIV_FWM_DCCS_CHN_S *pstChn;    

    g_timerPcivDccsData.expires  = jiffies + msecs_to_jiffies(PCIV_TIMER_EXPIRES);
    g_timerPcivDccsData.function = PcivFmws_TimerFunc;
    g_timerPcivDccsData.data     = 0;
    if (!timer_pending(&g_timerPcivDccsData))
    {
        add_timer(&g_timerPcivDccsData);
    }

    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
        pcivChn = i;
        pstChn = &s_astFwmDccsChn[pcivChn];

		if (HI_TRUE == pstChn->bMaster)
		{
			if (HI_TRUE != pstChn->bStart)
	        {
	            continue;
	        }
		
			/*Query the shared memery has image or not to read*/
	      	while(Simple_CB_QueryBusy(&pstChn->stPicCb) >= sizeof(stRecvPic))
	       	{
		        Simple_CB_Read(&pstChn->stPicCb, (HI_U8*)&stRecvPic, sizeof(stRecvPic));
				
				/* send Image info to deal int VO/VPSS/VENC*/
		        PCIV_FirmWareRecvPicAndSend(pcivChn, &stRecvPic);		
	       	}
		}
		else
		{
			if (pstChn->bCreate)
			{

				/*Query the recycle shared memery has image or not to read*/
				while(Simple_CB_QueryBusy(&pstChn->stRecycleCb) >= sizeof(stSrcPic))
				{
			        Simple_CB_Read(&pstChn->stRecycleCb, (HI_U8*)&stSrcPic, sizeof(stSrcPic));
					/*Release source image buffer*/
			        PCIV_FirmWareSrcPicFree(pcivChn, &stSrcPic);
				}
			}
		}
        
    }
}

static HI_S32 PcivFwms_CbInit(PCIV_CHN pcivChn, PCIVFMW_MSG_CREATE_S *pstCreateArg)
{
    HI_S32 s32Ret;
    PCIV_FWM_DCCS_CHN_S *pstChn;
    HI_U32   u32CbSize;   
    HI_U32   u32PhyAddr;

    pstChn = &s_astFwmDccsChn[pcivChn];
    u32CbSize = pstCreateArg->u32CbSize;

	/*initial  the Image  shared memery*/
    u32PhyAddr = pstCreateArg->u32PhyAddr;
    s32Ret = Simple_CB_Init(&pstChn->stPicCb, u32PhyAddr, (u32CbSize)/2);
		 
	/*initial the Image  shared memery of recycle pool*/
    u32PhyAddr += (u32CbSize)/2;
    s32Ret |= Simple_CB_Init(&pstChn->stRecycleCb, u32PhyAddr, (u32CbSize)/2);
	
    if(HI_SUCCESS != s32Ret)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Channel %d init buffer fail\n", pcivChn);
        (HI_VOID)Simple_CB_DeInit(&pstChn->stPicCb);
        (HI_VOID)Simple_CB_DeInit(&pstChn->stRecycleCb);
        return HI_ERR_PCIV_NOMEM;
    }
    
    PCIV_FMW_TRACE(HI_DBG_INFO, "PicCb:0x%x, RecycleCb:0x%x \n",
        pstChn->stPicCb.u32PhyAddr, pstChn->stRecycleCb.u32PhyAddr);
    
    return HI_SUCCESS;
}

static HI_S32 PcivFwms_CbExit(PCIV_CHN pcivChn)
{
    PCIV_FWM_DCCS_CHN_S *pstChn;

    pstChn = &s_astFwmDccsChn[pcivChn];

    Simple_CB_DeInit(&pstChn->stRecycleCb);
    Simple_CB_DeInit(&pstChn->stPicCb);

    return HI_SUCCESS;
}

/********************************************************************************************
The  below is message deal function
********************************************************************************************/

HI_S32 PcivFmwDccsCreate(PCIV_CHN pcivChn, PCIVFMW_MSG_CREATE_S *pstCreateArg)
{
    HI_S32 s32Ret;        

	if (0 == pstCreateArg->s32LocalId)
	{
		s_astFwmDccsChn[pcivChn].bMaster = HI_TRUE;
	}
	else
	{
		s_astFwmDccsChn[pcivChn].bMaster = HI_FALSE;
	}
	
    s32Ret = PCIV_FirmWareCreate(pcivChn, &pstCreateArg->stPcivAttr, pstCreateArg->s32LocalId);
    if (s32Ret != HI_SUCCESS)
    {
        return s32Ret;
    }
    
    s32Ret = PcivFwms_CbInit(pcivChn, pstCreateArg);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FirmWareDestroy(pcivChn);
        return s32Ret;
    }
    
    s_astFwmDccsChn[pcivChn].bCreate = HI_TRUE;
    return HI_SUCCESS;
}

HI_S32 PcivFmwDccsDestroy(PCIV_CHN pcivChn, HI_VOID *pArg)
{
    HI_S32 s32Ret;
	PCIV_PIC_S 	stSrcPic  = {0};
	PCIV_PIC_S 	stRecvPic = {0};
	
    s32Ret = PCIV_FirmWareDestroy(pcivChn);
    if (s32Ret != HI_SUCCESS)
    {
        return s32Ret;
    }
	
	s_astFwmDccsChn[pcivChn].bCreate = HI_FALSE;
	

	/*l00346266-Dynamic destroy and creat channel, exist the situation of VB un-released ,when destroy shared buffer ,
	destroy the vb to avoid this situation*/
	if(s_astFwmDccsChn[pcivChn].bMaster == HI_FALSE)
	{
		while(Simple_CB_QueryBusy(&s_astFwmDccsChn[pcivChn].stPicCb) >= sizeof(stRecvPic))
       	{
	        Simple_CB_Read(&s_astFwmDccsChn[pcivChn].stPicCb, (HI_U8*)&stRecvPic, sizeof(stRecvPic));
			
	        /* release the source  Image buffer */            
	        PCIV_FirmWareSrcPicFree(pcivChn, &stRecvPic);
       	}
	
		while(Simple_CB_QueryBusy(&s_astFwmDccsChn[pcivChn].stRecycleCb) >= sizeof(stSrcPic))
		{
	        Simple_CB_Read(&s_astFwmDccsChn[pcivChn].stRecycleCb, (HI_U8*)&stSrcPic, sizeof(stSrcPic));
			/* release the source  Image buffer */	   
	        PCIV_FirmWareSrcPicFree(pcivChn, &stSrcPic);
		}	
	}

    PcivFwms_CbExit(pcivChn);
	
    return HI_SUCCESS;
}
HI_S32 PcivFmwDccsSetAttr(PCIV_CHN pcivChn, PCIVFMW_MSG_SETATTR_S *pAttr)
{
    return PCIV_FirmWareSetAttr(pcivChn, &(pAttr->stAttr), pAttr->s32LocalId);
}
HI_S32 PcivFmwDccsSetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    return PCIV_FirmWareSetPreProcCfg(pcivChn, pAttr);
}
HI_S32 PcivFmwDccsGetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    return PCIV_FirmWareGetPreProcCfg(pcivChn, pAttr);
}
HI_S32 PcivFmwDccsStart(PCIV_CHN pcivChn, HI_VOID *pArg)
{
    HI_S32 s32Ret;
    s32Ret = PCIV_FirmWareStart(pcivChn);
    if (s32Ret)
    {
        return s32Ret;
    }
    s_astFwmDccsChn[pcivChn].bStart = HI_TRUE;
    return HI_SUCCESS;
}
HI_S32 PcivFmwDccsStop(PCIV_CHN pcivChn, HI_VOID *pArg)
{
    HI_S32 s32Ret;
    s_astFwmDccsChn[pcivChn].bStart = HI_FALSE;
    s32Ret = PCIV_FirmWareStop(pcivChn);
    if (s32Ret)
    {
        s_astFwmDccsChn[pcivChn].bStart = HI_TRUE;
        return s32Ret;
    }
	
    PCIV_FMW_TRACE(HI_DBG_INFO, "pcivfmws chn%d stop ok \n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PcivFmwDccsWindowVbCreate(PCIV_CHN pcivChn, PCIV_WINVBCFG_S * pCfg)
{
    return PCIV_FirmWareWindowVbCreate(pCfg);
}

HI_S32 PcivFmwDccsWindowVbDestroy(PCIV_CHN pcivChn, HI_VOID *pArg)
{
    return PCIV_FirmWareWindowVbDestroy();
}

HI_S32 PcivFmwDccsMalloc(PCIV_CHN pcivChn, PCIVFMW_MSG_MALLOC_S *pstMalloc)
{
    return PCIV_FirmWareMalloc(pstMalloc->u32Size, pstMalloc->s32LocalId, &pstMalloc->u32PhyAddr);
}

HI_S32 PcivFmwDccsFree(PCIV_CHN pcivChn, PCIVFMW_MSG_FREE_S *pstFreeArg)
{
    return PCIV_FirmWareFree(pstFreeArg->s32LocalId, pstFreeArg->u32PhyAddr);
}

HI_S32 PCIVFirmDccsMallocChnBuffer(PCIV_CHN pcivChn, PCIVFMW_MSG_CHN_MALLOC_S *pstChnMalloc)
{
    return PCIV_FirmWareMallocChnBuffer(pcivChn, pstChnMalloc->u32Index, pstChnMalloc->u32Size, pstChnMalloc->s32LocalId, &pstChnMalloc->u32PhyAddr);
}

HI_S32 PCIVFirmDccsFreeChnBuffer(PCIV_CHN pcivChn, PCIVFMW_MSG_CHN_FREE_S *pstFreeArg)
{
    return PCIV_FirmWareFreeChnBuffer(pcivChn, pstFreeArg->u32Index, pstFreeArg->s32LocalId);
}


/********************************************************************************************/

/* the contexe of messege communication*/
typedef struct hiPCIV_FWM_DCCS_MSG_CTX_S
{
    PCIV_CHN 			pcivChn;
    HI_BOOL             bHaveRev;
    PCIVFMW_MSGTYPE_E   enMsgType; 
    HI_U32              u32MsgLen;
    HI_U8               cMsgBody[PCIVFMW_MSG_MAXLEN];
    wait_queue_head_t   stWait;
    struct task_struct  *pstThread;
} PCIV_FWM_DCCS_MSG_CTX_S;

/* the message-deal function definition */
typedef HI_S32 FN_PCIV_FMWS_MSGHND(PCIV_CHN pcivChn, HI_VOID *pArg);

/*the conctrl struct of message deal*/
typedef struct hiPCIV_FMW_DCCS_MSG_S
{
    PCIVFMW_MSGTYPE_E   enMsgType;      /* the message type */          
    FN_PCIV_FMWS_MSGHND *pfnMsgHandle;  /* the message deal function handler */
    HI_S32              u32OutArgSize;  /* the parameter  length message deal function */ 
} PCIV_FMW_DCCS_MSG_S;

/*the handle r of IPCM */
static hios_ipcm_handle_t 		*g_hPcivSIpcmHnd;
/*then message contex of dual nucleus(current is one)*/
static PCIV_FWM_DCCS_MSG_CTX_S 	g_stFmwsMsgCtx;
/*message deal chart(the arrary subscript should equal to message type)*/
static PCIV_FMW_DCCS_MSG_S 		g_astFmwsMsgHndTab[PCIVFMW_MSGTYPE_BUTT];


static int PcivFmws_EchoMsg(HI_S32 s32RetVal, HI_U8 *u8MsgBody, HI_U32 u32Len)
{    
    HI_S32 s32Ret;
    PCIVFMW_MSG_S stMsg;

    stMsg.stHead.u32Magic   = PCIVFMW_MSG_MAGIC;
    stMsg.stHead.s32RetVal  = s32RetVal;
    stMsg.stHead.u32MsgLen  = u32Len;
    stMsg.stHead.enMsgType  = PCIVFMW_MSGTYPE_CMDECHO;
    
    HI_ASSERT(u32Len < PCIVFMW_MSG_MAXLEN);
    memcpy(stMsg.cMsgBody, u8MsgBody, u32Len);

    s32Ret = hios_ipcm_sendto(g_hPcivSIpcmHnd, &stMsg, sizeof(PCIVFMW_MSGHEAD_S) + u32Len);
    if (s32Ret < 0)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR,"echo msg to master ARM fail, 0x%x \n", s32Ret);
        return s32Ret;
    }
    
    PCIV_FMW_TRACE(HI_DBG_INFO,"echo msg to master ARM, ret:0x%x \n", s32RetVal);
    return HI_SUCCESS;
}


/*deal message in nucleus parameter */
static int PcivFmws_DealMsg(PCIV_FWM_DCCS_MSG_CTX_S *pstMsgCtl)
{
    HI_S32 s32Ret = HI_FAILURE;
    HI_U32 u32OutDataLen = 0;
    FN_PCIV_FMWS_MSGHND *pfnMsgHandleFunc = NULL;

    PCIV_FMW_TRACE(HI_DBG_INFO, "deal msg in kernel thread, msg type:%s\n", STR_PCIVFMW_MSGTYPE(pstMsgCtl->enMsgType));

	/*Get deal-function and output parameter length accord message type*/
    pfnMsgHandleFunc = g_astFmwsMsgHndTab[pstMsgCtl->enMsgType].pfnMsgHandle;
    u32OutDataLen = g_astFmwsMsgHndTab[pstMsgCtl->enMsgType].u32OutArgSize;
    
    if (pfnMsgHandleFunc != NULL)
    {
        s32Ret = pfnMsgHandleFunc(pstMsgCtl->pcivChn, (HI_VOID*)pstMsgCtl->cMsgBody);
    }
            
	/*return the slave return-value and  output datat to master arm*/
    if (PcivFmws_EchoMsg(s32Ret, pstMsgCtl->cMsgBody, u32OutDataLen))
    {
        printk("echo msg to master arm fail \n");
        return HI_FAILURE;
    }    

    return HI_SUCCESS;
}

/*deal the specail message in the kernel thread(cannot excute in the context of interrupt)*/
static int PcivFmws_MsgThread(void *p)
{
    HI_S32 s32Ret;
    PCIV_FWM_DCCS_MSG_CTX_S *pstMsgCtl;
    
    do {
        pstMsgCtl = &g_stFmwsMsgCtx;
        
		/*wating to wakeup*/
		s32Ret = wait_event_interruptible(pstMsgCtl->stWait, pstMsgCtl->bHaveRev);
        if (s32Ret)
        {
            printk("wait_event_interruptible err 0x%x\n", s32Ret);
            return -ERESTARTSYS;
        }
		/*update flag when wakeup*/
        pstMsgCtl->bHaveRev = HI_FALSE;
        
		/*deal message*/
        PcivFmws_DealMsg(pstMsgCtl);
        
	} while (!kthread_should_stop());
    
	return HI_SUCCESS;
}

static int PcivFmws_WakeThread(PCIV_CHN PcivChn, PCIVFMW_MSGTYPE_E enMsgType, void *buf, unsigned int len)
{         
    PCIV_FWM_DCCS_MSG_CTX_S *pstMsgCtl = &g_stFmwsMsgCtx;
    
	/*set the info and data in the message*/
    pstMsgCtl->pcivChn = PcivChn;
    pstMsgCtl->enMsgType = enMsgType;
    pstMsgCtl->u32MsgLen = len;
    HI_ASSERT(len < PCIVFMW_MSG_MAXLEN);
    memcpy(pstMsgCtl->cMsgBody, (HI_U8*)buf, len);

	/*wakeup the kernel thread of message to deal*/
    pstMsgCtl->bHaveRev = HI_TRUE;
    wake_up_interruptible(&pstMsgCtl->stWait);

    return HI_SUCCESS;
}


/*Receive the callback-function of IPCM message*/
static int PcivFwms_IpcmISR(void *handle, void *buf, unsigned int len)
{
    PCIVFMW_MSG_S *pstMsg;

    HI_ASSERT(!((int)buf&0x3));
    
    pstMsg = (PCIVFMW_MSG_S*)buf;
        
    PCIV_FMW_TRACE(HI_DBG_INFO, "receave ipcm msg from master ARM, chn:%d,type:%s\n", 
        pstMsg->stHead.pcivChn, STR_PCIVFMW_MSGTYPE(pstMsg->stHead.enMsgType));

    if (pstMsg->stHead.enMsgType >= PCIVFMW_MSGTYPE_BUTT)
    {
        printk("invalid msg type : %d \n", pstMsg->stHead.enMsgType);
        return HI_FAILURE; 
    }

	/*wakeup the kernel thread of message-deal*/
    PcivFmws_WakeThread(pstMsg->stHead.pcivChn, pstMsg->stHead.enMsgType, 
        pstMsg->cMsgBody, pstMsg->stHead.u32MsgLen);

    return  HI_SUCCESS;
}

/*the chart of  soures message*/
static PCIV_FMW_DCCS_MSG_S g_astMsgHndOrgTab[PCIVFMW_MSGTYPE_BUTT] = 
{            
    {PCIVFMW_MSGTYPE_CREATE ,   	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsCreate,          	0},
    {PCIVFMW_MSGTYPE_DESTROY,   	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsDestroy,         	0},
    {PCIVFMW_MSGTYPE_START,     	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsStart,           	0},
    {PCIVFMW_MSGTYPE_STOP,      	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsStop,            	0},
    {PCIVFMW_MSGTYPE_SETATTR,   	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsSetAttr,         	0},
    {PCIVFMW_MSGTYPE_MALLOC,    	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsMalloc,          	sizeof(PCIVFMW_MSG_MALLOC_S)},
    {PCIVFMW_MSGTYPE_FREE,      	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsFree,            	0},
    {PCIVFMW_MSGTYPE_CHNMALLOC, 	(FN_PCIV_FMWS_MSGHND*)PCIVFirmDccsMallocChnBuffer,	sizeof(PCIVFMW_MSG_CHN_MALLOC_S)},
    {PCIVFMW_MSGTYPE_CHNFREE,   	(FN_PCIV_FMWS_MSGHND*)PCIVFirmDccsFreeChnBuffer,    0},
    {PCIVFMW_MSGTYPE_VBCREATE,  	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsWindowVbCreate,  	0},
    {PCIVFMW_MSGTYPE_VBDESTROY, 	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsWindowVbDestroy, 	0},
	{PCIVFMW_MSGTYPE_SETVPPCFG, 	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsSetPreProcCfg,   	0},
    {PCIVFMW_MSGTYPE_GETVPPCFG, 	(FN_PCIV_FMWS_MSGHND*)PcivFmwDccsGetPreProcCfg,   	sizeof(PCIV_PREPROC_CFG_S)},
	{PCIVFMW_MSGTYPE_CMDECHO,   	(FN_PCIV_FMWS_MSGHND*)NULL,                       	0},
};

/*initial the chart of  message(raank to assure the subscript of array egueal to message type)*/
static HI_S32 PcivFmws_MsgHndInit(HI_VOID)
{
    HI_S32 i, j;

    for (i=0; i<PCIVFMW_MSGTYPE_BUTT; i++)
    {
        for (j=0; j<PCIVFMW_MSGTYPE_BUTT; j++)
        {
            HI_ASSERT(g_astMsgHndOrgTab[j].enMsgType < PCIVFMW_MSGTYPE_BUTT);
            if (i == g_astMsgHndOrgTab[j].enMsgType)
            {
                memcpy(&g_astFmwsMsgHndTab[i], &g_astMsgHndOrgTab[j], sizeof(PCIV_FMW_DCCS_MSG_S));
                break;
            }
        }
        if (PCIVFMW_MSGTYPE_BUTT == j)
        {
            printk("PcivFmws_MsgHndInit err, i=%d\n", i);
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

static int PcivFmws_IpcmInit(void)
{
    struct hi_ipcm_handle_attr attr;
    hios_ipcm_handle_opt_t opt;

    attr.target_id = 0;//DCCM_CPU_ID;
    attr.priority = 2;
    attr.port = 1;//DCC_PCIV_PORT;
    g_hPcivSIpcmHnd = hios_ipcm_open(&attr);
    if (!g_hPcivSIpcmHnd)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "hios_ipcm_open fail\n");
        return HI_FAILURE;
    }
        
    opt.recvfrom_notify = PcivFwms_IpcmISR;
    opt.data = 0;
    if (hios_ipcm_setopt(g_hPcivSIpcmHnd, &opt))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR,"hios_ipcm_setopt fail\n");
        hios_ipcm_close(g_hPcivSIpcmHnd);
        return HI_FAILURE;
    }
    
    return HI_SUCCESS;
}

static int PcivFmws_IpcmExit(void)
{
    hios_ipcm_close(g_hPcivSIpcmHnd);
    
    return HI_SUCCESS;
}



HI_S32 PcivFmws_Init(void *p)
{
	return HI_SUCCESS;
}

HI_VOID PcivFmws_Exit(HI_VOID)
{
	return;
}

static HI_VOID PcivFmws_Notify(MOD_NOTICE_ID_E enNotice)
{
    return;
}

static HI_VOID PcivFmws_QueryState(MOD_STATE_E *pstState)
{
    *pstState = MOD_STATE_FREE;
    return;
}

static HI_U32 PcivFmws_GetVerMagic(HI_VOID)
{
	return VERSION_MAGIC;
}


static UMAP_MODULE_S s_stPcivFmwsModule = {
    .pstOwner 		= THIS_MODULE,
    .enModId  		= HI_ID_PCIV,
	.aModName   	= "pciv",
	
    .pfnInit        = PcivFmws_Init,
    .pfnExit        = PcivFmws_Exit,
    .pfnQueryState  = PcivFmws_QueryState,
    .pfnNotify      = PcivFmws_Notify,
    .pfnVerChecker  = PcivFmws_GetVerMagic,
    
    .pstExportFuncs = HI_NULL,
    .pData          = HI_NULL,
};


static int __init PCIV_Fmws_ModInit(void)
{
    HI_S32 i;
    PCIVFMW_CALLBACK_S stFirmWareCallBack;

	if (CMPI_RegisterModule(&s_stPcivFmwsModule))
    {
		printk("Regist pciv firmware slave module err.\n");
        return HI_FAILURE;
    }

	/*initial chart of message-deal*/
    if (PcivFmws_MsgHndInit())
    {
        return HI_FAILURE;
    }
    
	/*initial communication of two-nucleus*/
    if (PcivFmws_IpcmInit())
    {
        return HI_FAILURE;
    }
        
    /* 1)send source image send callback function */
    stFirmWareCallBack.pfSrcSendPic = PcivFmws_SrcPicSend;
	/* 2)the function of releasing VO display image info*/
    stFirmWareCallBack.pfRecvPicFree = PcivFmws_RecvPicFree;

	stFirmWareCallBack.pfQueryPcivChnShareBufState = PcivFmws_QueryPcivChnShareBufState;
	
    /* register to FirmWare module */
    (HI_VOID)PCIV_FirmWareRegisterFunc(&stFirmWareCallBack);
    
        
	/*initial timer use to query  data in shared buffer*/
	init_timer(&g_timerPcivDccsData);
	g_timerPcivDccsData.expires  = jiffies + msecs_to_jiffies(PCIV_TIMER_EXPIRES);
	g_timerPcivDccsData.function = PcivFmws_TimerFunc;
	g_timerPcivDccsData.data     = 0;
	add_timer(&g_timerPcivDccsData);
		
    /* initial wait-queue */
    init_waitqueue_head(&g_stFmwsMsgCtx.stWait);
    
	/* start kernel thread to deal some function interface cannot deal in interrupt context*/
    g_stFmwsMsgCtx.pstThread = kthread_run(PcivFmws_MsgThread, NULL, "PcivFmwDccs");
    if (IS_ERR(g_stFmwsMsgCtx.pstThread))	
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "create kthread fail\n");
        PcivFmws_IpcmExit();
        del_timer(&g_timerPcivDccsData);
        return HI_FAILURE;
    }

    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
        s_astFwmDccsChn[i].bCreate = HI_FALSE;
        s_astFwmDccsChn[i].stPicCb.u32PhyAddr = 0;
        s_astFwmDccsChn[i].stRecycleCb.u32PhyAddr = 0;
    }        
    	
    return HI_SUCCESS;
}

static void __exit PCIV_Fmws_ModExit(void)
{	
    kthread_stop(g_stFmwsMsgCtx.pstThread);
    
	del_timer_sync(&g_timerPcivDccsData);

    PcivFmws_IpcmExit();

	CMPI_UnRegisterModule(HI_ID_PCIV);
	
    return;
}

module_init(PCIV_Fmws_ModInit);
module_exit(PCIV_Fmws_ModExit);

MODULE_AUTHOR(MPP_AUTHOR);
MODULE_LICENSE(MPP_LICENSE);
MODULE_VERSION(MPP_VERSION);

