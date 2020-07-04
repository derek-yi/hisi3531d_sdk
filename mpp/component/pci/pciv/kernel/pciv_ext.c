/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv_ext.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/06/16
  Description   :
  History       :
  1.Date        : 2008/06/16
    Author      :
    Modification: Created file

  2.Date        : 2008/11/20
    Author      : z44949
    Modification: Modify to support VDEC

******************************************************************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/version.h>

#include "hi_osal.h"

#include "hi_errno.h"
#include "hi_debug.h"
#include "mod_ext.h"
#include "dev_ext.h"
#include "proc_ext.h"
#include "pciv.h"
#include "mkp_pciv.h"

typedef enum hiPCIV_STATE_E
{
    PCIV_STATE_STARTED  = 0,
    PCIV_STATE_STOPPING = 1,
    PCIV_STATE_STOPPED  = 2
} PCIV_STATE_E;

static osal_dev_t * g_stPcivumapd;
static PCIV_STATE_E s_enPcivState   = PCIV_STATE_STOPPED;
static osal_atomic_t     s_stPcivUserRef = OSAL_ATOMIC_INIT(0);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
    static DECLARE_MUTEX(g_PcivIoctlMutex);
#else
    static DEFINE_SEMAPHORE(g_PcivIoctlMutex);
#endif

#define PCIV_IOCTL_DWON() \
do{\
    HI_S32 s32Tmp;\
    s32Tmp = down_interruptible(&g_PcivIoctlMutex);\
    if(s32Tmp)\
    {\
        osal_atomic_dec_return(&s_stPcivUserRef);\
        return s32Tmp;\
    }\
}while(0)

#define PCIV_IOCTL_UP() up(&g_PcivIoctlMutex)

static int pcivOpen(void *private_data)
{
   // private_data = (void*)(PCIV_MAX_CHN_NUM);
    //*((HI_U32*)private_data) = DEFAULT_PRIVATE_DATA;
    return 0;
}

static int pcivClose(void *private_data)
{
    return 0;
}
static long PcivDevIoctl(unsigned int  cmd, unsigned long arg,void *private_data)
{
    HI_BOOL  bDown     = HI_TRUE;
	HI_S32   s32Ret    = -ENOIOCTLCMD;
    void  * pArg = (void  *)arg;
    PCIV_CHN pcivChn   = UMAP_GET_CHN(private_data);

	/*if the system has received the notice to stop or the system has been stopped*/
    if (PCIV_STATE_STARTED != s_enPcivState)
    {
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

    PCIV_IOCTL_DWON();

    switch(cmd)
    {
        case PCIV_BINDCHN2FD_CTRL:
        {
			UMAP_SET_CHN(private_data, *((HI_U32 *)arg));
			s32Ret = HI_SUCCESS;
            break;
        }
        case PCIV_CREATE_CTRL:
        {
			PCIV_ATTR_S * pstAttr = (PCIV_ATTR_S *)pArg;
			s32Ret = PCIV_Create(pcivChn,pstAttr);
            break;
        }
        case PCIV_DESTROY_CTRL:
        {
			s32Ret =  PCIV_Destroy(pcivChn);
            break;
        }
        case PCIV_SETATTR_CTRL:
        {
			PCIV_ATTR_S *pstAttr = (PCIV_ATTR_S *)pArg;
    		s32Ret = PCIV_SetAttr(pcivChn, pstAttr);
			break;
        }
		case PCIV_GETATTR_CTRL:
		{
			PCIV_ATTR_S * pstAttr = (PCIV_ATTR_S *)pArg;
			s32Ret = PCIV_GetAttr(pcivChn, pstAttr);
			break;
		}
		case PCIV_START_CTRL:
		{
			s32Ret =  PCIV_Start(pcivChn);
			break;
		}
        case PCIV_STOP_CTRL:
        {
			s32Ret =  PCIV_Stop(pcivChn);
			break;
        }
		case PCIV_MALLOC_CTRL:
		{
			PCIV_IOCTL_MALLOC_S *pstMalloc = (PCIV_IOCTL_MALLOC_S *)pArg;
			s32Ret = PCIV_Malloc(pstMalloc->u32Size, &(pstMalloc->u32PhyAddr));
			if(HI_SUCCESS != s32Ret)
			{
				(HI_VOID)PCIV_Free(pstMalloc->u32PhyAddr);
			}
			break;
		}
		case PCIV_FREE_CTRL:
		{
			HI_U32 *pu32PhyAddr = (HI_U32 *)pArg;
			s32Ret =  PCIV_Free(*pu32PhyAddr);
			break;
		}
		case PCIV_MALLOC_CHN_BUF_CTRL:
		{
			PCIV_IOCTL_MALLOC_CHN_BUF_S * pstMalloc = (PCIV_IOCTL_MALLOC_CHN_BUF_S *)pArg;

			s32Ret = PCIV_MallocChnBuffer(pstMalloc->u32ChnId, pstMalloc->u32Index, pstMalloc->u32Size, &(pstMalloc->u32PhyAddr));
			if(HI_SUCCESS != s32Ret)
			{
				(HI_VOID)PCIV_FreeChnBuffer(pcivChn, pstMalloc->u32Index);
			}
			break;
		}
		case PCIV_FREE_CHN_BUF_CTRL:
		{
			HI_U32 *pu32Index = (HI_U32 *)pArg;
			s32Ret =  PCIV_FreeChnBuffer(pcivChn, *pu32Index);
			break;
		}
		case PCIV_GETBASEWINDOW_CTRL:
		{
			PCIV_BASEWINDOW_S *pstBase = (PCIV_BASEWINDOW_S *)pArg;

			s32Ret = PCIV_DrvAdp_GetBaseWindow(pstBase);
            if(HI_SUCCESS != s32Ret)
            {
            	osal_printk("PCIV_GETBASEWINDOW_CTRL ioctl err! s32Ret:0x%x\n\n",s32Ret);
				s32Ret = -EFAULT;
            }

		    break;
		}
		case PCIV_DMATASK_CTRL:
		{
			HI_U32 u32CpySize;
			PCIV_DMA_TASK_S  *pstTask;
			static PCIV_DMA_BLOCK_S stDmaBlk[PCIV_MAX_DMABLK];
			pstTask = (PCIV_DMA_TASK_S *)pArg;

            if (HI_NULL == pstTask->pBlock)
            {
                osal_printk("pstTask->pBlock is NULL!\n\n");
                s32Ret = HI_ERR_PCIV_NULL_PTR;
                break;
            }

            u32CpySize = sizeof(PCIV_DMA_BLOCK_S) * pstTask->u32Count;

            if (pstTask->u32Count > PCIV_MAX_DMABLK)
			{
			    osal_printk("u32Count:%d!\n",pstTask->u32Count);
				s32Ret = HI_ERR_PCIV_NOBUF;
                break;
            }

            if (osal_copy_from_user(stDmaBlk, pstTask->pBlock, u32CpySize))
            {

                osal_printk("Copy from user failed!\n");
                s32Ret = HI_ERR_PCIV_ILLEGAL_PARAM;
                break;
            }

            pstTask->pBlock = stDmaBlk;

            if (0 == pstTask->pBlock->u32BlkSize)
            {
                osal_printk("u32BlkSize:%d is illegal!\n\n", pstTask->pBlock->u32BlkSize);
                s32Ret = HI_ERR_PCIV_ILLEGAL_PARAM;
                break;
            }

			s32Ret =  PCIV_UserDmaTask(pstTask);
		    break;
		}
		case PCIV_GETLOCALID_CTRL:
		{
			HI_S32 *ps32LocalId = (HI_S32 *)pArg;
			*ps32LocalId = PCIV_DrvAdp_GetLocalId();
			s32Ret = HI_SUCCESS;
		    break;
		}
		case PCIV_ENUMCHIPID_CTRL:
		{
			PCIV_IOCTL_ENUMCHIP_S *pstChip = (PCIV_IOCTL_ENUMCHIP_S *)pArg;

			s32Ret = PCIV_DrvAdp_EunmChip(pstChip->s32ChipArray);
            if(HI_SUCCESS != s32Ret) break;
		    break;
		}
		case PCIV_WINVBCREATE_CTRL:
		{
			PCIV_WINVBCFG_S *pstConfig = (PCIV_WINVBCFG_S *)pArg;
			s32Ret = PCIV_WindowVbCreate(pstConfig);
		    break;
		}
		case PCIV_WINVBDESTROY_CTRL:
		{
			s32Ret = PCIV_WindowVbDestroy();
		    break;
		}
        case PCIV_SHOW_CTRL:
		{
			s32Ret = PCIV_Hide(pcivChn, HI_FALSE);
		    break;
		}
        case PCIV_HIDE_CTRL:
		{
			s32Ret = PCIV_Hide(pcivChn, HI_TRUE);
		    break;
		}
        case PCIV_SETVPPCFG_CTRL:
        {
			PCIV_PREPROC_CFG_S *pstVppCfg = (PCIV_PREPROC_CFG_S *)pArg;
			s32Ret = PCIV_SetPreProcCfg(pcivChn, pstVppCfg);
            break;
        }
        case PCIV_GETVPPCFG_CTRL:
        {
			PCIV_PREPROC_CFG_S *pstVppCfg = (PCIV_PREPROC_CFG_S *)pArg;
			s32Ret = PCIV_GetPreProcCfg(pcivChn, pstVppCfg);
			if(s32Ret != HI_SUCCESS)
			{
			    break;
		    }

			break;
        }

        default:
			s32Ret = -ENOIOCTLCMD;
        break;
    }

    if(bDown == HI_TRUE)
	{
		PCIV_IOCTL_UP();
	}

    return s32Ret;
}

static long pcivIoctl(unsigned int cmd, unsigned long arg, void *private_data)
{
    int ret;

    osal_atomic_inc_return(&s_stPcivUserRef);

	ret = PcivDevIoctl(cmd, arg, private_data);
    osal_atomic_dec_return(&s_stPcivUserRef);

    return ret;
}


HI_S32 PCIV_ExtInit(void *p)
{
	/*As long as it is not in stop state,it will not need to initialize,only retrun success*/
    if(s_enPcivState != PCIV_STATE_STOPPED)
    {
        return HI_SUCCESS;
    }

    if(HI_SUCCESS != PCIV_Init())
	{
        HI_TRACE(HI_DBG_ERR,HI_ID_PCIV,"PcivInit failed\n");
    	return HI_FAILURE;
	}

    HI_TRACE(HI_DBG_INFO,HI_ID_PCIV,"PcivInit success\n");
    s_enPcivState = PCIV_STATE_STARTED;

	return HI_SUCCESS;
}


HI_VOID PCIV_ExtExit(HI_VOID)
{
	/*if it is stopped ,retrun success,else the exit function is been called */
    if(s_enPcivState == PCIV_STATE_STOPPED)
    {
        return ;
    }
    PCIV_Exit();
    s_enPcivState = PCIV_STATE_STOPPED;
}

static HI_VOID PCIV_Notify(MOD_NOTICE_ID_E enNotice)
{
    s_enPcivState = PCIV_STATE_STOPPING;  /*The new IOCT is not continue received*/
	/*Pay attention to wake all user*/
    return;
}

static HI_VOID PCIV_QueryState(MOD_STATE_E *pstState)
{
    if (0 == osal_atomic_read(&s_stPcivUserRef))
    {
        *pstState = MOD_STATE_FREE;
    }
    else
    {
        *pstState = MOD_STATE_BUSY;
    }
    return;
}

static HI_U32 PCIV_GetVerMagic(HI_VOID)
{
	return VERSION_MAGIC;
}

static HI_S32 PCIV_Freeze(osal_dev_t *pdev)
{
    HI_PRINT(  "%s %d\n", __func__, __LINE__);
    return HI_SUCCESS;
}

static HI_S32 PCIV_Restore(osal_dev_t *pdev)
{
    HI_PRINT(  "%s %d\n", __func__, __LINE__);
    return HI_SUCCESS;
}

static UMAP_MODULE_S s_stPcivModule = {
    //.pstOwner 		= THIS_MODULE,
    .enModId  		= HI_ID_PCIV,
	.aModName   	= "pciv",

    .pfnInit        = PCIV_ExtInit,
    .pfnExit        = PCIV_ExtExit,
    .pfnQueryState  = PCIV_QueryState,
    .pfnNotify      = PCIV_Notify,
    .pfnVerChecker  = PCIV_GetVerMagic,
    .pData          = HI_NULL,
};


static struct osal_fileops pciv_fops = {
	.open		= pcivOpen,
    .unlocked_ioctl = pcivIoctl,

	.release    = pcivClose,
};

struct osal_pmops stPcivDrvOps = {
    .pm_freeze  = PCIV_Freeze,
    .pm_restore = PCIV_Restore,
};

static int __init PCIV_ModInit(void)
{
    HI_S32 s32Ret = HI_SUCCESS;
#ifndef DISABLE_DEBUG_INFO
    osal_proc_entry_t *proc;
#endif

    char buf[20];

	osal_snprintf(buf, 20, "%s", UMAP_DEVNAME_PCIV_BASE);
	g_stPcivumapd = osal_createdev(UMAP_DEVNAME_PCIV_BASE);
    if(!g_stPcivumapd)
	{
        osal_printk("create device err. \n");
        return HI_FAILURE;
	}
	g_stPcivumapd->fops = &pciv_fops;
    g_stPcivumapd->minor = UMAP_PCIV_MINOR_BASE;
	g_stPcivumapd->osal_pmops = &stPcivDrvOps;
    //g_stPcivumapd.owner = THIS_MODULE;

    /*register all pciv device*/
    if(osal_registerdevice(g_stPcivumapd)<0)
    {
        osal_printk("Regist device err. \n");
		osal_destroydev(g_stPcivumapd);
        return HI_FAILURE;
    }

    if (CMPI_RegisterModule(&s_stPcivModule))
    {
		osal_deregisterdevice(g_stPcivumapd);
		osal_destroydev(g_stPcivumapd);
		osal_printk("Regist pciv module err.\n");
        return HI_FAILURE;
    }
	
#ifndef DISABLE_DEBUG_INFO
	proc = osal_create_proc_entry(PROC_ENTRY_PCIV, NULL);
    if(proc==HI_NULL)
    {
        CMPI_UnRegisterModule(HI_ID_PCIV);
		osal_deregisterdevice(g_stPcivumapd);
		osal_destroydev(g_stPcivumapd);
        HI_PRINT(  "Vpss module create proc failed\n");
        return HI_FAILURE;
    }
	proc->read = PCIV_ProcShow;
#endif
	s32Ret = osal_atomic_init(&s_stPcivUserRef);
    if (HI_SUCCESS != s32Ret)
    {
        HI_PRINT("osal_atomic_init failed\n");
        return -1;
    }

    osal_atomic_set(&s_stPcivUserRef, 0);
	
    osal_printk("load pciv.ko ...OK!\n");
    return HI_SUCCESS;
}


static void __exit PCIV_ModExit(void)
{
    osal_remove_proc_entry(PROC_ENTRY_PCIV, HI_NULL);
    CMPI_UnRegisterModule(HI_ID_PCIV);
    osal_deregisterdevice(g_stPcivumapd);
	osal_destroydev(g_stPcivumapd);
	osal_printk("unload pciv.ko ...OK!\n");
    return ;
}

module_init(PCIV_ModInit);
module_exit(PCIV_ModExit);

MODULE_AUTHOR(MPP_AUTHOR);
MODULE_LICENSE(MPP_LICENSE);
MODULE_VERSION(MPP_VERSION);


