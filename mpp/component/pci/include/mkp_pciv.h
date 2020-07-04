/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mkp_pciv.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/04/05
  Description   : 
  History       :
  1.Date        : 2008/06/18
    Author      : zhourui50825
    Modification: Created file

******************************************************************************/
#ifndef __MKP_PCIV_H__
#define __MKP_PCIV_H__

#include "hi_common.h"
#include "hi_comm_pciv.h"
#include "mkp_ioctl.h"
#include "hi_comm_vdec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct hiPCIV_IOCTL_ENUMCHIP_S
{    
    HI_S32 s32ChipArray[PCIV_MAX_CHIPNUM];
} PCIV_IOCTL_ENUMCHIP_S;

typedef struct hiPCIV_IOCTL_MALLOC_S
{
    HI_U32 u32PhyAddr;
    HI_U32 u32Size;
} PCIV_IOCTL_MALLOC_S;

typedef struct hiPCIV_IOCTL_MALLOC_CHN_BUF_S
{
	HI_U32 u32ChnId;
	HI_U32 u32Index;
    HI_U32 u32PhyAddr;
    HI_U32 u32Size;
} PCIV_IOCTL_MALLOC_CHN_BUF_S;


typedef enum hiIOC_NR_PCIV_E
{
	IOC_NR_PCIV_CREATE = 0,
	IOC_NR_PCIV_DESTROY,	
	IOC_NR_PCIV_SETATTR,
	IOC_NR_PCIV_GETATTR,	
	IOC_NR_PCIV_START,
	IOC_NR_PCIV_STOP,	
    IOC_NR_PCIV_MALLOC,
    IOC_NR_PCIV_FREE,  
    IOC_NR_PCIV_MALLOC_CHN_BUF,
    IOC_NR_PCIV_FREE_CHN_BUF,
    IOC_NR_PCIV_BIND,
    IOC_NR_PCIV_UNBIND,  
    IOC_NR_PCIV_ENUMBINDOBJ,  
	IOC_NR_PCIV_GETBASEWINDOW,
	IOC_NR_PCIV_GETLOCALID,
	IOC_NR_PCIV_ENUMCHIPID,
	IOC_NR_PCIV_DMATASK,
	IOC_NR_PCIV_BINDCHN2FD,
	IOC_NR_PCIV_WINVBCREATE,
	IOC_NR_PCIV_WINVBDESTROY,
	IOC_NR_PCIV_SHOW,
	IOC_NR_PCIV_HIDE,
	IOC_NR_PCIV_SETVPPCFG,
	IOC_NR_PCIV_GETVPPCFG,
} IOC_NR_PCIV_E;

#define PCIV_CREATE_CTRL         _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_CREATE, PCIV_ATTR_S)
#define PCIV_DESTROY_CTRL        _IO  (IOC_TYPE_PCIV, IOC_NR_PCIV_DESTROY)
#define PCIV_SETATTR_CTRL        _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_SETATTR, PCIV_ATTR_S)
#define PCIV_GETATTR_CTRL        _IOR (IOC_TYPE_PCIV, IOC_NR_PCIV_GETATTR, PCIV_ATTR_S)
#define PCIV_START_CTRL          _IO  (IOC_TYPE_PCIV, IOC_NR_PCIV_START)
#define PCIV_STOP_CTRL           _IO  (IOC_TYPE_PCIV, IOC_NR_PCIV_STOP)
#define PCIV_MALLOC_CTRL         _IOWR(IOC_TYPE_PCIV, IOC_NR_PCIV_MALLOC, PCIV_IOCTL_MALLOC_S)
#define PCIV_FREE_CTRL           _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_FREE, HI_U32)
#define PCIV_MALLOC_CHN_BUF_CTRL _IOWR(IOC_TYPE_PCIV, IOC_NR_PCIV_MALLOC_CHN_BUF, PCIV_IOCTL_MALLOC_CHN_BUF_S)
#define PCIV_FREE_CHN_BUF_CTRL   _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_FREE_CHN_BUF, HI_U32)
#define PCIV_BIND_CTRL           _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_BIND, PCIV_BIND_OBJ_S)
#define PCIV_UNBIND_CTRL         _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_UNBIND, PCIV_BIND_OBJ_S)
#define PCIV_DMATASK_CTRL        _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_DMATASK, PCIV_DMA_TASK_S)
#define PCIV_GETBASEWINDOW_CTRL  _IOWR(IOC_TYPE_PCIV, IOC_NR_PCIV_GETBASEWINDOW, PCIV_BASEWINDOW_S)
#define PCIV_GETLOCALID_CTRL     _IOR (IOC_TYPE_PCIV, IOC_NR_PCIV_GETLOCALID, HI_S32)
#define PCIV_ENUMCHIPID_CTRL     _IOR (IOC_TYPE_PCIV, IOC_NR_PCIV_ENUMCHIPID, PCIV_IOCTL_ENUMCHIP_S)
#define PCIV_BINDCHN2FD_CTRL     _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_BINDCHN2FD, PCIV_CHN)
#define PCIV_WINVBCREATE_CTRL    _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_WINVBCREATE,  PCIV_WINVBCFG_S)
#define PCIV_WINVBDESTROY_CTRL   _IO  (IOC_TYPE_PCIV, IOC_NR_PCIV_WINVBDESTROY)
#define PCIV_SHOW_CTRL           _IO  (IOC_TYPE_PCIV, IOC_NR_PCIV_SHOW)
#define PCIV_HIDE_CTRL           _IO  (IOC_TYPE_PCIV, IOC_NR_PCIV_HIDE)
#define PCIV_SETVPPCFG_CTRL      _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_SETVPPCFG, PCIV_PREPROC_CFG_S)
#define PCIV_GETVPPCFG_CTRL      _IOR (IOC_TYPE_PCIV, IOC_NR_PCIV_GETVPPCFG, PCIV_PREPROC_CFG_S)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MPI_PRIV_PCIV_H__ */



