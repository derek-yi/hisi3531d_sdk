/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/16
  Description   :
  History       :
  1.Date        : 2009/07/16
    Author      : Z44949
    Modification: Created file
  2.Date        : 2009/09/12
    Author      : P00123320
    Modification: Porting to Hi3520, fix some bugs and modify some func
  3.Date        : 2009/10/10
    Author      : P00123320
    Modification: Add function for hide/show pciv chn
  4.Date        : 2009/10/20
    Author      : P00123320
    Modification: When destroy pciv chn,first wait finish all transfers
  5.Date        : 2009/11/2
    Author      : P00123320
    Modification: Modify function PCIV_Create().
  6.Date        : 2010/2/11
    Author      : P00123320
    Modification: Some code in func:PcivCheckAttr remove to pciv_firmware
  7.Date        : 2010/2/24
    Author      : P00123320
    Modification: Add interfaces about set/get pre-process cfg
  8.Date        : 2010/5/29
    Author      : P00123320
    Modification: Modify struct PCIV_NOTIFY_PICEND_S, transfer enFiled additionaly.
******************************************************************************/
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "pciv.h"
#include "pciv_fmwext.h"

#define PCIV_MAX_DMATASK 32

typedef struct hiPCIV_CHANNEL_S
{
    volatile HI_BOOL bCreate;
    volatile HI_BOOL bStart;
    volatile HI_BOOL bConfig;   /* the flag of config or not */
    volatile HI_BOOL bHide;     /* the flag of hide or not */

    volatile HI_U32  u32GetCnt;   /*record the times of reading stRecycleCb or stPicCb*/
    volatile HI_U32  u32SendCnt;  /*record the times of writing stRecycleCb or stPicCb*/
    volatile HI_U32  u32RespCnt;  /*record the times of finishing response interrupt*/
    volatile HI_U32  u32LostCnt;  /*record the times of dropping the Image for no idle Buffer */
    volatile HI_U32  u32NotifyCnt;  /*the times of notify(receive the message of ReadDone or WriteDone)*/

    PCIV_ATTR_S      stPcivAttr; /*record the dest Image Info and the opposite end dev info*/
    volatile HI_BOOL bBufFree[PCIV_MAX_BUF_NUM];  /* Used by sender. */
    volatile HI_U32  u32BufUseCnt[PCIV_MAX_BUF_NUM];  /* Used by sender. */
    volatile HI_BOOL bCanRecv;    /*the flag of memory info synchronous, used in the receiver end*/

    struct semaphore pcivMutex;
} PCIV_CHANNEL_S;

typedef struct hiPCIV_USERDMADONE_S
{
    struct list_head list;

    wait_queue_head_t stwqDmaDone;
    HI_BOOL           bDmaDone;
} PCIV_USERDMANODE_S;

static PCIV_CHANNEL_S     g_stPcivChn[PCIV_MAX_CHN_NUM];
static PCIV_USERDMANODE_S g_stUserDmaPool[PCIV_MAX_DMATASK];
static struct list_head   g_listHeadUserDma;

extern HI_U32 g_u64RdoneTime[PCIV_MAX_CHN_NUM];
extern HI_U32 g_u32RdoneGap[PCIV_MAX_CHN_NUM];
extern HI_U32 g_u32MaxRdoneGap[PCIV_MAX_CHN_NUM];
extern HI_U32 g_u32MinRdoneGap[PCIV_MAX_CHN_NUM];
extern HI_U32 g_u64WdoneTime[PCIV_MAX_CHN_NUM];
extern HI_U32 g_u32WdoneGap[PCIV_MAX_CHN_NUM];
extern HI_U32 g_u32MaxWdoneGap[PCIV_MAX_CHN_NUM];
extern HI_U32 g_u32MinWdoneGap[PCIV_MAX_CHN_NUM];
extern HI_S32 g_proc_all;

static spinlock_t        g_PcivLock;
#define PCIV_SPIN_LOCK   spin_lock_irqsave(&g_PcivLock,flags)
#define PCIV_SPIN_UNLOCK spin_unlock_irqrestore(&g_PcivLock,flags)

#define PCIV_MUTEX_DOWN(sema)\
    do {\
        if (down_interruptible(&(sema)))\
            return -ERESTARTSYS;\
    } while(0)

#define PCIV_MUTEX_UP(sema) up(&(sema))

HI_S32 PcivCheckAttr(PCIV_ATTR_S *pAttr)
{
    HI_S32 i;

	/*check the number of the buffer is valide*/
    if((pAttr->u32Count < 2) || (pAttr->u32Count > PCIV_MAX_BUF_NUM))
    {
        PCIV_TRACE(HI_DBG_ERR, "Buffer count(%u) not invalid,should in [2,%u]\n", pAttr->u32Count, PCIV_MAX_BUF_NUM);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

	/*check the physical address*/
    for(i=0; i<pAttr->u32Count; i++)
    {
        if(!pAttr->u32PhyAddr[i])
        {
            PCIV_TRACE(HI_DBG_ERR, "pAttr->u32PhyAddr[%d]:0x%x invalid\n", i, pAttr->u32PhyAddr[i]);
            return HI_ERR_PCIV_ILLEGAL_PARAM;
        }
    }

	/*check the valid of the remote device */
    if(  (pAttr->stRemoteObj.s32ChipId < 0)
       ||(pAttr->stRemoteObj.s32ChipId > PCIV_MAX_CHIPNUM)
       ||(pAttr->stRemoteObj.pcivChn < 0)
       ||(pAttr->stRemoteObj.pcivChn >= PCIV_MAX_CHN_NUM)
       )
     {
        PCIV_TRACE(HI_DBG_ERR, "Invalid remote object(%d,%d)\n",
                   pAttr->stRemoteObj.s32ChipId, pAttr->stRemoteObj.pcivChn);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
     }

     return HI_SUCCESS;
}

HI_S32 PCIV_Create(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr)
{
    PCIV_CHANNEL_S *pChn = NULL;
    HI_S32          s32Ret, i;
    HI_S32          s32LocalId = 0;
    unsigned long   flags;

    PCIV_CHECK_CHNID(pcivChn);

    PCIV_MUTEX_DOWN(g_stPcivChn[pcivChn].pcivMutex);

    PCIV_SPIN_LOCK;
    pChn = &g_stPcivChn[pcivChn];
    if(HI_TRUE == pChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_ERR, "Channel %d has been created\n", pcivChn);
        PCIV_SPIN_UNLOCK;
        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_ERR_PCIV_EXIST;
    }

    if(HI_SUCCESS != PcivCheckAttr(pAttr))
    {
        PCIV_SPIN_UNLOCK;
        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    s32LocalId = PCIV_DrvAdp_GetLocalId();
    PCIV_SPIN_UNLOCK;

    s32Ret = PCIV_FirmWareCreate(pcivChn, pAttr, s32LocalId);
    if(HI_SUCCESS != s32Ret)
    {
        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return s32Ret;
    }

    PCIV_SPIN_LOCK;
    pChn->bStart   = HI_FALSE;
    pChn->bConfig  = HI_TRUE;
    pChn->bCreate  = HI_TRUE;
    pChn->bCanRecv = HI_FALSE;
    pChn->bHide    = HI_FALSE;   /* reset to show */
    pChn->u32GetCnt = pChn->u32SendCnt = pChn->u32RespCnt = pChn->u32LostCnt = pChn->u32NotifyCnt = 0;

    for (i = 0; i < PCIV_MAX_BUF_NUM; i++)
    {
        pChn->bBufFree[i] = HI_FALSE;
    }

    for (i = 0; i < pAttr->u32Count; i++)
    {
        if (0 != pAttr->u32PhyAddr[i])
        {
            pChn->bBufFree[i] = HI_TRUE;
        }
    }

	/*DTS2016060809388--Slave's different chn bind Host's same chn*/
	if(0 != s32LocalId )
	{
		for(i=0;i<PCIV_MAX_CHN_NUM;i++)
		{
			if((pAttr->stRemoteObj.pcivChn == g_stPcivChn[i].stPcivAttr.stRemoteObj.pcivChn)&&(pcivChn != i))
			{
				PCIV_TRACE(HI_DBG_ERR, "Two slave's Chns Bind the host's same  pciv chn,pcivChn:%d, pAttr->stRemoteObj.pcivChn:%d\n",pcivChn,pAttr->stRemoteObj.pcivChn);
				PCIV_SPIN_UNLOCK;
				PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
				return HI_ERR_PCIV_NOT_SUPPORT;
			}
		}
	}

	/*DTS2016060809164  DTS2016060809307--Host and slave stRemoteObj.s32ChipId cannot be itsself*/
	if(pAttr->stRemoteObj.s32ChipId == s32LocalId )
	{
		PCIV_TRACE(HI_DBG_ERR, "stRemoteObj ChipId Can not be itself! stRemoteObj.s32ChipId:%d s32LocalId:%d\n",
			pAttr->stRemoteObj.s32ChipId,s32LocalId);
		PCIV_SPIN_UNLOCK;
		PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
		return HI_ERR_PCIV_NOT_SUPPORT;

	}

    memcpy(&pChn->stPcivAttr, pAttr, sizeof(PCIV_ATTR_S));

    g_u64RdoneTime[pcivChn]   = 0;
    g_u32RdoneGap[pcivChn]    = 0;
    g_u32MaxRdoneGap[pcivChn] = 0;
    g_u32MinRdoneGap[pcivChn] = 0;
    g_u64WdoneTime[pcivChn]   = 0;
    g_u32WdoneGap[pcivChn]    = 0;
    g_u32MaxWdoneGap[pcivChn] = 0;
    g_u32MinWdoneGap[pcivChn] = 0;

    PCIV_SPIN_UNLOCK;

    PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);

    PCIV_TRACE(HI_DBG_INFO, "pciv chn %d create ok\n", pcivChn);
    return s32Ret;
}

HI_S32 PCIV_Destroy(PCIV_CHN pcivChn)
{
    PCIV_CHANNEL_S *pChn = NULL;
    unsigned long   flags;

    PCIV_CHECK_CHNID(pcivChn);

    PCIV_MUTEX_DOWN(g_stPcivChn[pcivChn].pcivMutex);
    PCIV_SPIN_LOCK;
    pChn = &g_stPcivChn[pcivChn];
    if(HI_FALSE == pChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_NOTICE, "Channel %d has not been created\n", pcivChn);

        PCIV_SPIN_UNLOCK;
        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_SUCCESS;
    }
    if(HI_TRUE == pChn->bStart)
    {
        PCIV_TRACE(HI_DBG_ERR, "pciv chn%d should stop first then destroy \n", pcivChn);

        PCIV_SPIN_UNLOCK;
        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_ERR_PCIV_NOT_PERM;
    }

    PCIV_SPIN_UNLOCK;

    (HI_VOID)PCIV_FirmWareDestroy(pcivChn);

    PCIV_SPIN_LOCK;
    pChn->bCreate       = HI_FALSE;
    pChn->bConfig       = HI_FALSE;
    PCIV_SPIN_UNLOCK;

    PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);

    PCIV_TRACE(HI_DBG_INFO, "pciv chn %d destroy ok\n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PCIV_SetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr)
{
    PCIV_CHANNEL_S *pChn = NULL;
    HI_S32          s32Ret;
    unsigned long   flags;
	HI_S32          s32LocalId = 0;

    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_PTR(pAttr);

    PCIV_MUTEX_DOWN(g_stPcivChn[pcivChn].pcivMutex);

    pChn = &g_stPcivChn[pcivChn];
    if(HI_FALSE == pChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_ERR, "Channel %d has not been created\n", pcivChn);

        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_ERR_PCIV_UNEXIST;
    }

    if(HI_TRUE == pChn->bStart)
    {
        PCIV_TRACE(HI_DBG_ERR, "Channel %d is running\n", pcivChn);

        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_ERR_PCIV_NOT_PERM;
    }

    if(HI_SUCCESS != PcivCheckAttr(pAttr))
    {
        PCIV_TRACE(HI_DBG_ERR, "Channel %d attribute error\n", pcivChn);

        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

	s32LocalId = PCIV_DrvAdp_GetLocalId();
    s32Ret = PCIV_FirmWareSetAttr(pcivChn, pAttr, s32LocalId);
    if (HI_SUCCESS != s32Ret)
    {
        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return s32Ret;
    }

    PCIV_SPIN_LOCK;
    memcpy(&pChn->stPcivAttr, pAttr, sizeof(PCIV_ATTR_S));
    pChn->bConfig = HI_TRUE;
    PCIV_SPIN_UNLOCK;

    PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);

    PCIV_TRACE(HI_DBG_INFO, "pciv chn %d set attr ok\n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PCIV_GetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr)
{
    PCIV_CHANNEL_S *pChn = NULL;

    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_PTR(pAttr);

    pChn = &g_stPcivChn[pcivChn];
    if(HI_TRUE != pChn->bConfig)
    {
        PCIV_TRACE(HI_DBG_WARN, "attr of Channel %d has not been setted\n", pcivChn);

        return HI_ERR_PCIV_NOT_CONFIG;
    }

    memcpy(pAttr, &pChn->stPcivAttr, sizeof(PCIV_ATTR_S));

    return HI_SUCCESS;
}

HI_S32 PCIV_SetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    HI_S32          s32Ret;
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_PTR(pAttr);

    PCIV_MUTEX_DOWN(g_stPcivChn[pcivChn].pcivMutex);

    s32Ret = PCIV_FirmWareSetPreProcCfg(pcivChn, pAttr);

    PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);

    return s32Ret;
}

HI_S32 PCIV_GetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_PTR(pAttr);

    return PCIV_FirmWareGetPreProcCfg(pcivChn, pAttr);
}


HI_S32 PCIV_Start(PCIV_CHN pcivChn)
{
    HI_U32            	i;
    PCIV_CHANNEL_S    	*pChn = NULL;
    HI_S32            	s32Ret;
    HI_S32            	s32LocalId = 0;
    PCIV_REMOTE_OBJ_S 	stRemoteObj;
    PCIV_PIC_S    		stRecvPic;
    unsigned long     	flags;

    PCIV_CHECK_CHNID(pcivChn);

    PCIV_MUTEX_DOWN(g_stPcivChn[pcivChn].pcivMutex);
    PCIV_SPIN_LOCK;

    pChn = &g_stPcivChn[pcivChn];
    if(HI_TRUE != pChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_ERR, "Channel %d not create\n", pcivChn);

        PCIV_SPIN_UNLOCK;
        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_ERR_PCIV_UNEXIST;
    }

    if(HI_TRUE == pChn->bStart)
    {
        PCIV_TRACE(HI_DBG_INFO, "Channel %d is running\n", pcivChn);

        PCIV_SPIN_UNLOCK;
        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_SUCCESS;
    }

    stRemoteObj.pcivChn = pChn->stPcivAttr.stRemoteObj.pcivChn;
    stRemoteObj.s32ChipId = 0;
    stRecvPic.enFiled     = -1;
    s32LocalId = PCIV_DrvAdp_GetLocalId();
    if (0 != s32LocalId)
    {
        for (i = 0; i < pChn->stPcivAttr.u32Count; i++)
        {
            pChn->bBufFree[i] = HI_TRUE;
        }
        s32Ret = PcivDrvAdpSendMsg(&stRemoteObj, PCIV_MSGTYPE_FREE, &stRecvPic);
    }

    PCIV_SPIN_UNLOCK;
    s32Ret = PCIV_FirmWareStart(pcivChn);
    PCIV_SPIN_LOCK;
    pChn->bStart = HI_TRUE;

    PCIV_SPIN_UNLOCK;
    PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);

    PCIV_TRACE(HI_DBG_INFO, "pciv chn %d start ok\n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PCIV_Stop(PCIV_CHN pcivChn)
{
    PCIV_CHANNEL_S *pChn = NULL;
    HI_S32          s32Ret;
    unsigned long   flags;
    HI_S32          s32LocalId = 0;

    PCIV_CHECK_CHNID(pcivChn);

    PCIV_MUTEX_DOWN(g_stPcivChn[pcivChn].pcivMutex);

    pChn = &g_stPcivChn[pcivChn];

    if(HI_TRUE != pChn->bStart)
    {
        PCIV_TRACE(HI_DBG_INFO, "Channel %d has stoped\n", pcivChn);

        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_SUCCESS;
    }

    PCIV_SPIN_LOCK;
	/*First Set the stop flag*/
    pChn->bStart = HI_FALSE;
    PCIV_SPIN_UNLOCK;

	/*Wait for the PCI task finished*/
    s32LocalId = PCIV_DrvAdp_GetLocalId();
    if(0 != s32LocalId)
    {
        while(pChn->u32SendCnt != pChn->u32RespCnt)
        {
            set_current_state(TASK_INTERRUPTIBLE);
            (HI_VOID)schedule_timeout(2);
            continue;
        }

    }

	/*Then stop the media releated work*/
    s32Ret = PCIV_FirmWareStop(pcivChn);
    if (s32Ret)
    {
        PCIV_SPIN_LOCK;
        pChn->bStart = HI_TRUE;
        PCIV_SPIN_UNLOCK;

        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return s32Ret;
    }

    PCIV_SPIN_LOCK;
    pChn->bCanRecv      = HI_FALSE;
    PCIV_SPIN_UNLOCK;

    PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);

    PCIV_TRACE(HI_DBG_INFO, "pciv chn %d stop ok\n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PCIV_Hide(PCIV_CHN pcivChn, HI_BOOL bHide)
{
    PCIV_CHANNEL_S *pChn = NULL;

    PCIV_CHECK_CHNID(pcivChn);

    PCIV_MUTEX_DOWN(g_stPcivChn[pcivChn].pcivMutex);

    pChn = &g_stPcivChn[pcivChn];
    if(HI_TRUE != pChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_ERR, "Channel %d not created\n", pcivChn);
        PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);
        return HI_ERR_PCIV_UNEXIST;
    }

    pChn->bHide = bHide;

    PCIV_MUTEX_UP(g_stPcivChn[pcivChn].pcivMutex);

    PCIV_TRACE(HI_DBG_INFO, "pciv chn %d hide%d ok\n", pcivChn, bHide);
    return HI_SUCCESS;
}

/*Only When  the slave chip reciver data or Image, We shoud config the Window Vb*/
HI_S32 PCIV_WindowVbCreate(PCIV_WINVBCFG_S *pCfg)
{
    PCIV_WINVBCFG_S stVbCfg;
    HI_U32 i, j, u32Size, u32Count;
    HI_S32 s32LocalId = PCIV_DrvAdp_GetLocalId();

	/*On the host chip, the action creating the special region is not supported*/
    if(0 == s32LocalId)
    {
        return HI_ERR_PCIV_NOT_SUPPORT;
    }

    if (pCfg->u32PoolCount > PCIV_MAX_VBCOUNT)
    {
        PCIV_TRACE(HI_DBG_ERR, "u32PoolCount is more than MAX_VBCOUNT:%d\n", pCfg->u32PoolCount, PCIV_MAX_VBCOUNT);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    memcpy(&stVbCfg, pCfg, sizeof(stVbCfg));
    for(i = 0; i < pCfg->u32PoolCount - 1; i++)
    {
        for(j = i+1; j < pCfg->u32PoolCount; j++)
        {
            if(stVbCfg.u32BlkSize[j] < stVbCfg.u32BlkSize[i])
            {
                u32Size  = stVbCfg.u32BlkSize[i];
                u32Count = stVbCfg.u32BlkCount[i];

                stVbCfg.u32BlkSize[i]  = stVbCfg.u32BlkSize[j];
                stVbCfg.u32BlkCount[i] = stVbCfg.u32BlkCount[j];

                stVbCfg.u32BlkSize[j]  = u32Size;
                stVbCfg.u32BlkCount[j] = u32Count;
            }
        }
    }

    return PCIV_FirmWareWindowVbCreate(&stVbCfg);
}

HI_S32 PCIV_WindowVbDestroy(HI_VOID)
{
    HI_S32 s32LocalId = PCIV_DrvAdp_GetLocalId();

	/*In the master chip,not support destroy the special area*/
    if(0 == s32LocalId)
    {
        return HI_ERR_PCIV_NOT_SUPPORT;
    }

    return PCIV_FirmWareWindowVbDestroy();
}


HI_S32 PCIV_Malloc(HI_U32 u32Size, HI_U32 *pPhyAddr)
{
    HI_S32 s32LocalId = PCIV_DrvAdp_GetLocalId();

    return PCIV_FirmWareMalloc(u32Size, s32LocalId, pPhyAddr);
}

HI_S32 PCIV_Free(HI_U32 u32PhyAddr)
{
	HI_S32 s32LocalId = PCIV_DrvAdp_GetLocalId();

    return PCIV_FirmWareFree(s32LocalId, u32PhyAddr);
}

HI_S32 PCIV_MallocChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_U32 u32Size, HI_U32 *pPhyAddr)
{
    HI_S32 s32LocalId = PCIV_DrvAdp_GetLocalId();

    return PCIV_FirmWareMallocChnBuffer(pcivChn, u32Index, u32Size, s32LocalId, pPhyAddr);
}

HI_S32 PCIV_FreeChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index)
{
	HI_S32 s32LocalId = PCIV_DrvAdp_GetLocalId();

    return PCIV_FirmWareFreeChnBuffer(pcivChn, u32Index, s32LocalId);
}

HI_VOID  PcivUserDmaDone(PCIV_SENDTASK_S *pstTask)
{
    PCIV_USERDMANODE_S *pUserDmaNode = NULL;

	/*assert the finished DMA task is the last one*/
    HI_ASSERT((pstTask->u32PrvData[0] + 1) == pstTask->u32PrvData[1]);

    pUserDmaNode = (PCIV_USERDMANODE_S *)pstTask->u32PrvData[2];
    pUserDmaNode->bDmaDone = HI_TRUE;
    wake_up(&pUserDmaNode->stwqDmaDone);
}

HI_S32 PCIV_UserDmaTask(PCIV_DMA_TASK_S *pTask)
{
    HI_S32              i;
    HI_S32              s32Ret = HI_SUCCESS;
    unsigned long       flags;
    PCIV_SENDTASK_S     stPciTask;
    PCIV_USERDMANODE_S *pUserDmaNode = NULL;
    HI_S32              s32LocalId = 0;

    if(list_empty(&g_listHeadUserDma))
    {
        return HI_ERR_PCIV_BUSY;
    }

    if((pTask->bRead != HI_TRUE)&&(pTask->bRead != HI_FALSE))
    {
        PCIV_TRACE(HI_DBG_ERR, "DMA Size is illeage! bRead:%d\n",  pTask->bRead);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    s32LocalId = PCIV_DrvAdp_GetLocalId();
    if(0 == s32LocalId)
    {
        for(i=0;i<pTask->u32Count;i++)
        {
            if(pTask->pBlock[i].u32BlkSize > 0x700000)
            {
                PCIV_TRACE(HI_DBG_ERR, "DMA Size is illeage! pTask->pBlock[%d].u32BlkSize:0x%x\n",  i, pTask->pBlock[i].u32BlkSize);
                return HI_ERR_PCIV_ILLEGAL_PARAM;
            }
        }
    }

    for (i = 0; i < pTask->u32Count; i++)
    {
        if ((pTask->pBlock[i].u32SrcAddr == 0)||(pTask->pBlock[i].u32DstAddr == 0)||((pTask->pBlock[i].u32SrcAddr&0x3) != 0)||((pTask->pBlock[i].u32DstAddr&0x3) != 0))
        {
            PCIV_TRACE(HI_DBG_ERR," u32SrcAddr:0x%x u32DstAddr:0x%x is illeage! \n", pTask->pBlock[i].u32SrcAddr,pTask->pBlock[i].u32DstAddr);
            return HI_ERR_PCIV_ILLEGAL_PARAM;
        }
    }

    PCIV_SPIN_LOCK;
    pUserDmaNode = list_entry(g_listHeadUserDma.next, PCIV_USERDMANODE_S, list);
    list_del(g_listHeadUserDma.next);
    PCIV_SPIN_UNLOCK;

    pUserDmaNode->bDmaDone = HI_FALSE;

    for(i=0; i<pTask->u32Count; i++)
    {
        stPciTask.u32SrcPhyAddr = pTask->pBlock[i].u32SrcAddr;
        stPciTask.u32DstPhyAddr = pTask->pBlock[i].u32DstAddr;
        stPciTask.u32Len        = pTask->pBlock[i].u32BlkSize;
        stPciTask.bRead         = pTask->bRead;
        stPciTask.u32PrvData[0] = i;
        stPciTask.u32PrvData[1] = pTask->u32Count;
        stPciTask.u32PrvData[2] = (HI_U32)pUserDmaNode;
        stPciTask.pCallBack     = NULL;

        /* If this is the last task node, we set the callback */
        if( (i+1) == pTask->u32Count)
        {
            stPciTask.pCallBack = PcivUserDmaDone;
        }

        if(HI_SUCCESS != PCIV_DrvAdp_AddDmaTask(&stPciTask))
        {
            s32Ret     = HI_ERR_PCIV_NOMEM;
            break;
        }
    }

    if(HI_SUCCESS == s32Ret)
    {
        HI_S32 s32TimeLeft;
        s32TimeLeft = wait_event_timeout(pUserDmaNode->stwqDmaDone,
                                      (pUserDmaNode->bDmaDone == HI_TRUE), 200);
        if(0 == s32TimeLeft)
        {
            s32Ret = HI_ERR_PCIV_TIMEOUT;
        }
    }

    PCIV_SPIN_LOCK;
    list_add_tail(&pUserDmaNode->list, &g_listHeadUserDma);
    PCIV_SPIN_UNLOCK;

    return s32Ret;
}

static HI_VOID PcivCheckNotifyCnt(PCIV_CHN pcivChn, HI_U32 u32Index, HI_U32 u32Count)
{
    PCIV_CHANNEL_S *pstPcivChn = &g_stPcivChn[pcivChn];

    if(0 == u32Count)
    {
        pstPcivChn->u32NotifyCnt = 0;
    }
    else
    {
        pstPcivChn->u32NotifyCnt ++;
        if(u32Count != pstPcivChn->u32NotifyCnt)
        {
            PCIV_TRACE(HI_DBG_WARN, "Warnning: PcivChn:%d, ReadDone MsgSeq -> (%u,%u),bufindex:%u \n",
                pcivChn, u32Count, pstPcivChn->u32NotifyCnt, u32Index);
            //HI_ASSERT(0);
        }
    }
}

/*When recieved the message of release the shared memory, This interface is been called to set the memory flag to idle*/
HI_S32 PCIV_FreeShareBuf(PCIV_CHN pcivChn, HI_U32 u32Index, HI_U32 u32Count)
{
    PCIV_CHANNEL_S    *pstPcivChn;
    unsigned long flags;

    PCIV_CHECK_CHNID(pcivChn);

    pstPcivChn = &g_stPcivChn[pcivChn];

    PCIV_SPIN_LOCK;
    if(HI_TRUE != pstPcivChn->bStart)
    {
        PCIV_TRACE(HI_DBG_ERR, "Pciv channel %d not start!\n", pcivChn);
        PCIV_SPIN_UNLOCK;
        return HI_ERR_PCIV_UNEXIST;
    }

    if(u32Index >= PCIV_MAX_BUF_NUM)
    {
        PCIV_TRACE(HI_DBG_ERR, "Buffer index %u is too larger!\n", u32Index);
        PCIV_SPIN_UNLOCK;
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    HI_ASSERT(u32Index < pstPcivChn->stPcivAttr.u32Count);

	/*check the serial number of the message is same or not with the loacal serial number*/
    PcivCheckNotifyCnt(pcivChn, u32Index, u32Count);

	/*Set the buffer flag to idle*/
    pstPcivChn->bBufFree[u32Index] = HI_TRUE;
    PCIV_SPIN_UNLOCK;

    return HI_SUCCESS;
}

HI_S32 PCIV_FreeAllBuf(PCIV_CHN pcivChn)
{
    HI_U32        i;
    unsigned long flags;

    PCIV_SPIN_LOCK;
    if (HI_TRUE != g_stPcivChn[pcivChn].bCreate)
    {
        PCIV_SPIN_UNLOCK;
        return HI_SUCCESS;
    }

    for (i = 0; i < g_stPcivChn[pcivChn].stPcivAttr.u32Count; i++)
    {
        g_stPcivChn[pcivChn].bBufFree[i] = HI_TRUE;
    }

    g_stPcivChn[pcivChn].bCanRecv = HI_TRUE;
    PCIV_SPIN_UNLOCK;
    return HI_SUCCESS;
}


/*when start DMA transmission, the interface must be called first to get a valid shared buffer */
static HI_S32 PCIV_GetShareBuf(PCIV_CHN pcivChn, HI_U32 *pCurIndex)
{
    HI_U32          i;
    PCIV_CHANNEL_S *pstPcivChn;

    pstPcivChn = &g_stPcivChn[pcivChn];
    for(i=0; i<pstPcivChn->stPcivAttr.u32Count; i++)
    {
        if(pstPcivChn->bBufFree[i])
        {
            *pCurIndex = i;
            return HI_SUCCESS;
        }
    }

    return HI_FAILURE;
}

static HI_S32 PCIV_GetShareBufState(PCIV_CHN pcivChn)
{
    HI_U32          i;
    PCIV_CHANNEL_S *pstPcivChn;

    pstPcivChn = &g_stPcivChn[pcivChn];
    for(i=0; i<pstPcivChn->stPcivAttr.u32Count; i++)
    {
        if(pstPcivChn->bBufFree[i])
        {
            return HI_SUCCESS;
        }
    }

    return HI_FAILURE;
}


HI_VOID PcivSrcPicSendDone(PCIV_SENDTASK_S *pTask)
{
    PCIV_CHN        pcivChn;
    PCIV_CHANNEL_S 	*pstChn;
    PCIV_PIC_S  	stRecvPic;
    PCIV_PIC_S   	stSrcPic;

    pcivChn = pTask->u32PrvData[0];
    HI_ASSERT((pcivChn >= 0) && (pcivChn < PCIV_MAX_CHN_NUM));

    pstChn = &g_stPcivChn[pcivChn];

    memcpy(&stRecvPic, (PCIV_PIC_S*)pTask->pvPrvData, sizeof(PCIV_PIC_S));
    kfree(pTask->pvPrvData);

    (void)PCIV_DrvAdp_DmaEndNotify(&pstChn->stPcivAttr.stRemoteObj, &stRecvPic);

    stSrcPic.u32PhyAddr = pTask->u32SrcPhyAddr;
    stSrcPic.u32PoolId  = pTask->u32PrvData[1];
    (HI_VOID)PCIV_SrcPicFree(pcivChn, &stSrcPic);

    pstChn->u32RespCnt++;

    return;
}

/*After dealing with the source Image,the interface is auto called to send the Image to the PCI target by PCI DMA mode*/
HI_S32 PCIV_SrcPicSend(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic)
{
    PCIV_CHANNEL_S 	*pChn = &g_stPcivChn[pcivChn];
    PCIV_SENDTASK_S stPciTask;
    HI_U32          u32CurIndex;
    HI_S32          s32Ret;
    PCIV_PIC_S  	*pstRecvPic = NULL;
    unsigned long   flags;

	/*Pay attention to the possibility of the pciv and vfwm called each othe*/
    PCIV_SPIN_LOCK;

    if(HI_TRUE != pChn->bStart)
    {
        PCIV_SPIN_UNLOCK;
        PCIV_TRACE(HI_DBG_ERR, "pciv chn: %d is not enable!\n", pcivChn);
        return HI_FAILURE;
    }

    HI_ASSERT(pSrcPic->enSrcType < PCIV_BIND_BUTT);
    HI_ASSERT(pSrcPic->enFiled < VIDEO_FIELD_BUTT);

    pChn->u32GetCnt++;

    s32Ret = PCIV_GetShareBuf(pcivChn, &u32CurIndex);
    if(HI_SUCCESS != s32Ret)
    {
        if((PCIV_BIND_VI == pSrcPic->enSrcType) || (PCIV_BIND_VO == pSrcPic->enSrcType))
        {
            pChn->u32LostCnt ++;
        }
        PCIV_SPIN_UNLOCK;
		PCIV_TRACE(HI_DBG_ERR, "No Free Buf, chn%d\n", pcivChn);
        return HI_ERR_PCIV_NOBUF;
    }

    pstRecvPic = (PCIV_PIC_S*)kmalloc(sizeof(PCIV_PIC_S), GFP_ATOMIC);
    if (NULL == pstRecvPic)
    {
        PCIV_TRACE(HI_DBG_EMERG, "kmalloc PCIV_RECVPIC_S err! chn%d\n", pcivChn);
        pChn->u32LostCnt ++;
        PCIV_SPIN_UNLOCK;
        return HI_ERR_PCIV_NOMEM;
    }

    pstRecvPic->u32Index                    = u32CurIndex;
    pstRecvPic->u32Count                    = pChn->u32SendCnt;
    pstRecvPic->u64Pts                      = pSrcPic->u64Pts;
    pstRecvPic->u32TimeRef                  = pSrcPic->u32TimeRef;
    pstRecvPic->enSrcType                   = pSrcPic->enSrcType;
    pstRecvPic->enFiled                     = pSrcPic->enFiled;
    pstRecvPic->stMixCapState.bHasDownScale = pSrcPic->stMixCapState.bHasDownScale;
    pstRecvPic->stMixCapState.bMixCapMode   = pSrcPic->stMixCapState.bMixCapMode;
    pstRecvPic->bBlock                      = pSrcPic->bBlock;

	/*Hide the channel,that is the PCIV channel will not send the Image to the target by DMA mode, only go on message-based communication*/
    stPciTask.u32Len        = (!pChn->bHide) ? (pChn->stPcivAttr.u32BlkSize) : 0;
    stPciTask.u32SrcPhyAddr = pSrcPic->u32PhyAddr;
    stPciTask.u32DstPhyAddr = pChn->stPcivAttr.u32PhyAddr[u32CurIndex];
    stPciTask.bRead         = HI_FALSE;
    stPciTask.u32PrvData[0] = pcivChn;              /* channel num */
    stPciTask.u32PrvData[1] = pSrcPic->u32PoolId;   /* src Image PoolID */

    stPciTask.pvPrvData     = (HI_VOID*)pstRecvPic;
    stPciTask.pCallBack     = PcivSrcPicSendDone;   /* register PCI DMA finished callback */
    s32Ret = PCIV_DrvAdp_AddDmaTask(&stPciTask);
    if(HI_SUCCESS != s32Ret)
    {
        PCIV_TRACE(HI_DBG_EMERG, "DMA task err! chn%d\n", pcivChn);
        kfree(pstRecvPic);
        pChn->u32LostCnt ++;
        PCIV_SPIN_UNLOCK;
        return s32Ret;
    }

	/*Set the serial number of the buffer not idle state*/
    HI_ASSERT(HI_TRUE == pChn->bBufFree[u32CurIndex]);
    pChn->bBufFree[u32CurIndex] = HI_FALSE;

    pChn->u32SendCnt++;

    PCIV_SPIN_UNLOCK;

    return HI_SUCCESS;
}

/*After the PCV DMA task finished,this interface is been called to release the Image buffer*/
HI_S32 PCIV_SrcPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic)
{
    return PCIV_FirmWareSrcPicFree(pcivChn, pSrcPic);
}

/*After recieving the Image through thePCIV channel, this interface is been called to send the Image
    to VO for display or VPSS for using or VENC for coding*/
HI_S32 PCIV_ReceivePic(PCIV_CHN pcivChn, PCIV_PIC_S *pRecvPic)
{
    HI_S32 s32Ret;
    unsigned long flags;

    HI_ASSERT(pcivChn < PCIV_MAX_CHN_NUM);
    HI_ASSERT(pRecvPic->enFiled < VIDEO_FIELD_BUTT);
    HI_ASSERT(pRecvPic->enSrcType < PCIV_BIND_BUTT);
    HI_ASSERT(pRecvPic->u32Index < g_stPcivChn[pcivChn].stPcivAttr.u32Count);

    PCIV_SPIN_LOCK;

   /************************************************************************************
     *  [HSCP201308020003]  l00181524,2013.08.16,when the master chip and slave chip re-creat and re-destroy, it is possibile the message is store in the buffer memory,
     * it will lead that when master chip is not booted or just created, at the same time, it recieved the slave chip Image before destroying, it will occupy the buffer, but the
     * buffer-index idle-flag is true, In this situation, will appear that it will used the buffer-index after the slave chip  re-creat, but the  master buffer-index is been occupied,
     * then an assert will occur,
     * So we introduce the bCanRecv flag, when the slave chip is re-start, it will send a message to the master chip,notice the master chip to release all vb to keep synchronous
     * with the slave chip, Because the mechanism of recieving message is trig by software interrupt, but 3531 and 3536 is double core, bCanRecv is needed to do mutual exclusion
     * on the double core system.
     **************************************************************************************/
    if ((HI_TRUE != g_stPcivChn[pcivChn].bStart) || (HI_TRUE != g_stPcivChn[pcivChn].bCanRecv))
    {
    	g_stPcivChn[pcivChn].bBufFree[pRecvPic->u32Index] = HI_FALSE;
        PCIV_SPIN_UNLOCK;
		pRecvPic->u64Pts = 0;
		PCIV_RecvPicFree(pcivChn, pRecvPic);
        PCIV_TRACE(HI_DBG_ERR, "pcivChn:%d hasn't be ready to receive pic, bStart: %d, bCanRecv: %d\n",
        pcivChn, g_stPcivChn[pcivChn].bStart, g_stPcivChn[pcivChn].bCanRecv);
        return HI_ERR_PCIV_SYS_NOTREADY;/*The vdec satuation is must in our consideration*/
    }

    g_stPcivChn[pcivChn].u32GetCnt++;

	/*Before this, the Image buffer-flag is idle, Pay attention to this situation,
	If the called order is not reasonable, and the upper do not assure that master chip and slave chip re-creat when return the HI_SUCCESS,at this time ,it is possible occur assert  */
    HI_ASSERT(HI_TRUE == g_stPcivChn[pcivChn].bBufFree[pRecvPic->u32Index]);

	/*In spite of sending to vo display success or not, here must set the buffer-flag to idle*/
    g_stPcivChn[pcivChn].bBufFree[pRecvPic->u32Index] = HI_FALSE;
    PCIV_SPIN_UNLOCK;

	/*The Firmware interface is been called to send the Image to VO display*/
    s32Ret = PCIV_FirmWareRecvPicAndSend(pcivChn, pRecvPic);
    if(HI_SUCCESS != s32Ret)
    {
        g_stPcivChn[pcivChn].u32LostCnt++;
        PCIV_TRACE(HI_DBG_ERR, "PCIV_FirmWareRecvPicAndSend Err,chn:%d, return value: 0x%x \n", pcivChn, s32Ret);
        return s32Ret;
    }

    g_stPcivChn[pcivChn].u32SendCnt++;
    return HI_SUCCESS;
}

/*After Used by VO or VPSS or VENC, In the Firmware, this interface is been called auto to return the Image buffer*/
HI_S32 PCIV_RecvPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pRecvPic)
{
    HI_S32 s32Ret = HI_SUCCESS;
    PCIV_CHANNEL_S *pChn = NULL;

    PCIV_CHECK_CHNID(pcivChn);

    pChn = &g_stPcivChn[pcivChn];

    HI_ASSERT(pRecvPic->u32Index < pChn->stPcivAttr.u32Count);
    HI_ASSERT(pRecvPic->u64Pts == 0);

	/*Only when the buffer state is been setted to used, the buffer release action  occur*/
    if(HI_TRUE != pChn->bBufFree[pRecvPic->u32Index])
    {
		/*Buffer state is set to idle*/
        pChn->bBufFree[pRecvPic->u32Index] = HI_TRUE;

        pRecvPic->u32Count = pChn->u32RespCnt;

		/*the READDONE message is send to the sender to notice that do free source releated action */
        s32Ret = PCIV_DrvAdp_BufFreeNotify(&pChn->stPcivAttr.stRemoteObj, pRecvPic);
        if(HI_SUCCESS != s32Ret)
        {
            PCIV_TRACE(HI_DBG_ERR, "PCIV_DrvAdp_BufFreeNotify err,chn%d\n", pcivChn);
            return s32Ret;
        }

		if (HI_TRUE == pChn->bCanRecv && HI_TRUE == pChn->bStart)
		{
        	pChn->u32RespCnt++;
		}
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_Init(void)
{
    HI_S32 i;
    PCIVFMW_CALLBACK_S stFirmWareCallBack;

    spin_lock_init(&g_PcivLock);

    INIT_LIST_HEAD(&g_listHeadUserDma);
    for(i=0; i<PCIV_MAX_DMATASK; i++)
    {
        init_waitqueue_head(&g_stUserDmaPool[i].stwqDmaDone);
        g_stUserDmaPool[i].bDmaDone = HI_TRUE;
        list_add_tail(&g_stUserDmaPool[i].list, &g_listHeadUserDma);
    }

    memset(g_stPcivChn, 0, sizeof(g_stPcivChn));
    for(i = 0; i < PCIV_MAX_CHN_NUM; i++)
    {
        g_stPcivChn[i].bCreate = HI_FALSE;
        g_stPcivChn[i].stPcivAttr.stRemoteObj.s32ChipId = -1;

		//DTS2016060809388-l00346266
		g_stPcivChn[i].stPcivAttr.stRemoteObj.pcivChn= -1;

        sema_init(&g_stPcivChn[i].pcivMutex, 1);
    }

    stFirmWareCallBack.pfSrcSendPic                = PCIV_SrcPicSend;
    stFirmWareCallBack.pfRecvPicFree               = PCIV_RecvPicFree;
    stFirmWareCallBack.pfQueryPcivChnShareBufState = PCIV_GetShareBufState;
    (HI_VOID)PCIV_FirmWareRegisterFunc(&stFirmWareCallBack);

    PCIV_DrvAdp_Init();

    return HI_SUCCESS;
}

HI_VOID  PCIV_Exit(void)
{
    HI_S32 i, s32Ret;

    for (i = 0; i < PCIV_MAX_CHN_NUM; i++)
    {
        if (HI_TRUE != g_stPcivChn[i].bCreate)
        {
            msleep(10);
            continue;
        }

        s32Ret = PCIV_Stop(i);
        if(HI_SUCCESS != s32Ret)
        {
            PCIV_TRACE(HI_DBG_ERR, "PCIV_Stop err,chn%d\n", i);
            return;
        }

        s32Ret = PCIV_Destroy(i);
        if(HI_SUCCESS != s32Ret)
        {
            PCIV_TRACE(HI_DBG_ERR, "PCIV_Destroy err,chn%d\n", i);
            return;
        }

    }

    PCIV_DrvAdp_Exit();
    return;
}

inline static HI_CHAR* PRINT_PIXFORMAT(PIXEL_FORMAT_E enPixFormt)
{
    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixFormt)
    {
        return "sp420";
    }
    else if (PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixFormt)
    {
        return "sp422";
    }
    else if (PIXEL_FORMAT_YUV_PLANAR_420 == enPixFormt)
    {
        return "p420";
    }
    else if (PIXEL_FORMAT_YUV_PLANAR_422 == enPixFormt)
    {
        return "p422";
    }
    else if (PIXEL_FORMAT_UYVY_PACKAGE_422 == enPixFormt)
    {
        return "uyvy422";
    }
    else if (PIXEL_FORMAT_YUYV_PACKAGE_422 == enPixFormt)
    {
        return "yuyv422";
    }
    else if (PIXEL_FORMAT_VYUY_PACKAGE_422 == enPixFormt)
    {
        return "vyuy422";
    }
    else
    {
        return NULL;
    }
}
HI_CHAR* PRINT_FIELD(VIDEO_FIELD_E enField)
{
    if (VIDEO_FIELD_TOP == enField)
    {
        return "top";
    }
    else if (VIDEO_FIELD_BOTTOM == enField)
    {
        return "bot";
    }
    else if (VIDEO_FIELD_FRAME == enField)
    {
        return "frm";
    }
    else if (VIDEO_FIELD_INTERLACED == enField)
    {
        return "intl";
    }
    else
    {
        return NULL;
    }
}


HI_S32 PCIV_ProcShow(struct osal_proc_dir_entry *s)
{
    HI_S32 i;
    PCIV_CHANNEL_S  *pstPcivChn;
    PCIV_ATTR_S     *pstAttr;
    PCIV_CHN         pcivChn;
    HI_CHAR          cString[PCIV_MAX_BUF_NUM*2+1] = {0};

    osal_seq_printf(s, "\n[PCIV] Version: ["MPP_VERSION"], Build Time:["__DATE__", "__TIME__"]\n");

    osal_seq_printf(s, "\n-----PCIV CHN ATTR--------------------------------------------------------------\n");
    osal_seq_printf(s, "%6s"    "%8s"   "%8s"    "%8s"     "%8s"   "%8s"    "%8s"    "%10s"     "%10s" "\n"
                 ,"PciChn","Width","Height","Stride0","Field","PixFmt","BufCnt","BufSize","PhyAddr0");
    for (pcivChn = 0; pcivChn < PCIV_MAX_CHN_NUM; pcivChn++)
    {
        pstPcivChn = &g_stPcivChn[pcivChn];

        if (HI_FALSE == pstPcivChn->bCreate) continue;

        pstAttr = &pstPcivChn->stPcivAttr;
        osal_seq_printf(s, "%6d" "%8d" "%8d" "%8d" "%8s" "%8s" "%8d" "%10d" "%10x" "\n",
            pcivChn,
            pstAttr->stPicAttr.u32Width,
            pstAttr->stPicAttr.u32Height,
            pstAttr->stPicAttr.u32Stride[0],
            PRINT_FIELD(pstAttr->stPicAttr.u32Field),
            PRINT_PIXFORMAT(pstAttr->stPicAttr.enPixelFormat),
            pstAttr->u32Count,
            pstAttr->u32BlkSize,
            pstAttr->u32PhyAddr[0]);
    }

    osal_seq_printf(s, "\n-----PCIV CHN STATUS------------------------------------------------------------\n");
    osal_seq_printf(s, "%6s"    "%8s"     "%8s"     "%12s"    "%12s"    "%12s"    "%12s"    "%12s"    "%12s"    "\n"
                 ,"PciChn","RemtChp","RemtChn","GetCnt","SendCnt","RespCnt","LostCnt","NtfyCnt","BufBusy");
    for (pcivChn = 0; pcivChn < PCIV_MAX_CHN_NUM; pcivChn++)
    {
        pstPcivChn = &g_stPcivChn[pcivChn];
        if (HI_FALSE == pstPcivChn->bCreate) continue;

        for(i=0; i<pstPcivChn->stPcivAttr.u32Count; i++)
        {
            osal_snprintf(&cString[i*2], 2, "%2d", !pstPcivChn->bBufFree[i]);
        }
        osal_seq_printf(s, "%6d" "%8d" "%8d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12s" "\n",
            pcivChn,
            pstPcivChn->stPcivAttr.stRemoteObj.s32ChipId,
            pstPcivChn->stPcivAttr.stRemoteObj.pcivChn,
            pstPcivChn->u32GetCnt,
            pstPcivChn->u32SendCnt,
            pstPcivChn->u32RespCnt,
            pstPcivChn->u32LostCnt,
            pstPcivChn->u32NotifyCnt,
            cString);
    }

	if (657676 == g_proc_all)
	{
	    osal_seq_printf(s, "\n-----PCIV MSG STATUS------------------------------------------------------------\n");
	    osal_seq_printf(s, "%6s"    "%10s"     "%10s"     "%10s"     "%10s"     "%10s"     "%10s"    "\n"
	                 ,"PciChn","RdoneGap","MaxRDGap","MinRDGap","WdoneGap","MaxWDGap","MinWDGap");
	    for (pcivChn = 0; pcivChn < PCIV_MAX_CHN_NUM; pcivChn++)
	    {
	        pstPcivChn = &g_stPcivChn[pcivChn];
	        if (HI_FALSE == pstPcivChn->bCreate) continue;

	        osal_seq_printf(s, "%6d" "%10d" "%10d" "%10d" "%10d" "%10d" "%10d" "\n",
	            pcivChn,
	            g_u32RdoneGap[pcivChn],
	            g_u32MaxRdoneGap[pcivChn],
	            g_u32MinRdoneGap[pcivChn],
	            g_u32WdoneGap[pcivChn],
	            g_u32MaxWdoneGap[pcivChn],
	            g_u32MinWdoneGap[pcivChn]);
	    }
	}

    return 0;
}


