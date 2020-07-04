/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_sys.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/03/11
  Description   :
  History       :
  1.Date        : 2009/03/11
    Author      : z4494
    Modification: Created file
    
  2.Date        : 2009/07/16
    Author      : p00123320
    Modification: add and modify some function

******************************************************************************/
#include "hi_type.h"
#include "mod_ext.h"
#include "mkp_sys.h"

#ifndef __HI_SYS_H__
#define __HI_SYS_H__

typedef enum hiSYS_FUNC_E
{
    SYS_VI_EVEN_DEV_CLK_SEL = 3,
    SYS_VI_DEV_CLK_DIV_SEL,
    SYS_VI_ODD_DEV_CLK_SEL,
    SYS_VI_DEV_CLK_SEL,

    SYS_VOU_BT1120_CLK_SEL,
    SYS_VOU_BT1120_CAS_OUT_CLK_DIV,
    SYS_VOU_BT1120_CAS_OUT_CLK_PHASIC_CTRL,
    SYS_VOU_BT1120_OUT_CLK_DIV,
    SYS_VOU_BT1120_OUT_CLK_PHASIC_CTRL,
    SYS_VOU_HD_PLL_SEL,
    SYS_VOU_HDDAC_CLK_SEL,
    SYS_VOU_HDMI_CLK_SEL,
    SYS_VOU_BT1120_CLK_EN,

    SYS_VOU_BUS_RESET_SEL,
    SYS_VOU_DEV_CLK_EN,
    SYS_VOU_SD_DAC_EN,   
    SYS_VOU_HD_DAC_EN,
    SYS_VOU_HD_DAC_CLK_PHASIC_CTRL,
    SYS_VOU_SD_DAC_CLK_PHASIC_CTRL,
    SYS_VOU_CLK_SEL,
    SYS_VOU_SD_CLK_SEL,
    SYS_VOU_SD_CLK_DIV,
    SYS_VOU_HD_CLK_DIV,
    SYS_VOU_WORK_EN,
    SYS_VOU_BUS_CLK_EN,
    
    SYS_VO_PLL_FRAC_SET,
    SYS_VO_PLL_POSTDIV1_SET,
    SYS_VO_PLL_POSTDIV2_SET,
    SYS_VO_PLL_REFDIV_SET,
    SYS_VO_PLL_FBDIV_SET,
        
    SYS_AI_SAMPLECLK_DIVSEL,
    SYS_AI_BITCKL_DIVSEL,
    SYS_AI_SYSCKL_SEL,     
    SYS_AI_MASTER,         
    SYS_AI_RESET_SEL,          
    SYS_AI_CLK_EN,             

    SYS_AO_SAMPLECLK_DIVSEL,
    SYS_AO_BITCKL_DIVSEL,
    SYS_AO_SYSCKL_SEL,     
    SYS_AO_MASTER,         
    SYS_AO_RESET_SEL,          
    SYS_AO_CLK_EN,             

    SYS_AIO_RESET_SEL,          
    SYS_AIO_CLK_EN, 
    SYS_AIO_CLK_SEL,

    SYS_VIU_BUS_RESET_SEL,       
    SYS_VIU_BUS_CLK_EN,          
    SYS_VIU_DEV_RESET_SEL,       
    SYS_VIU_DEV_CLK_EN,         
    SYS_VIU_DIV_CHN_CLK_EN,     
    SYS_VIU_CHN_CLK_EN,        
    SYS_VIU_CHN_CLK_DOUBLE,     
    SYS_VIU_CHN_CLK_PHASE,
    SYS_VIU_DEV_CLK_PHASE,
    SYS_VIU_PPC_RESET_SEL,
    SYS_VIU_PPC_CLK_EN,
    SYS_VIU_APB_CLK_EN, 
    SYS_VIU_DLL_ENABLE,
    SYS_VIU_DLL_DISABLE,

    SYS_VIU_ISP_RESET_SEL,
    SYS_VIU_ISP_CLK_EN,

    SYS_SENSOR_CLK_OUT_SEL,
 
    SYS_VEDU_RESET_SEL,       
    SYS_VEDU_CLK_EN,
    SYS_VEDU_SED_RESET_SEL,       
    SYS_VEDU_SED_CLK_EN,
    SYS_VEDU_SKIP_INIT,
    SYS_VEDU_CLK_SEL, /*the frequency of clock */

    SYS_VPSS_RESET_SEL,
    SYS_VPSS_CLK_EN,
#if 0
    SYS_VDH_RESET_SEL,
    SYS_VDH_CLK_EN,
#endif
    SYS_TDE_RESET_SEL,        
    SYS_TDE_CLK_EN,           

    SYS_JPGE_RESET_SEL,       
    SYS_JPGE_CLK_EN,  
    SYS_JPGE_SEL_CLK,
#if 0
    SYS_JPGD_RESET_SEL,
    SYS_JPGD_CLK_EN,
#endif
    SYS_MD_RESET_SEL,         
    SYS_MD_CLK_EN,            

    SYS_IVE_RESET_SEL,        
    SYS_IVE_CLK_EN,    

	SYS_FD_RESET_SEL,
	SYS_FD_CLK_EN,

#if 1
    SYS_VOIE_RESET_SEL,
    SYS_VOIE_CLK_EN,
#endif
    SYS_PCIE_RESET_SEL,
    SYS_PCIE_CLK_EN,

    SYS_CIPHER_RESET_SEL,
    SYS_CIPHER_CLK_EN,

    SYS_VGS_RESET_SEL,
    SYS_VGS_CLK_EN,

    SYS_HEVC_CLK_EN,
    SYS_HEVC_RESET_SEL, 

    SYS_HDMI_RESET_SEL,
    SYS_HDMI_PIXEL_CLK_EN,
    SYS_HDMI_BUS_CLK_EN,
    SYS_VEDU_GETRESETINFO_SEL,

    SYS_VO_PLL_CLK_DIV,

    SYS_HD_DAC_PCTRL1_SEL,
    SYS_BUTT,
} SYS_FUNC_E;

typedef struct hiSYS_SclCfg
{
    HI_U32 YhCoefPhyAddr;
    HI_U32 YvCoefPhyAddr;
    HI_U32 ChCoefPhyAddr;
    HI_U32 CvCoefPhyAddr;
    HI_U32 Yhstep;
    HI_U32 Yvstep;
    HI_U32 Chstep;
    HI_U32 Cvstep;

    HI_U32 Yhrat;
    HI_U32 Yvrat;
    HI_U32 Chrat;
    HI_U32 Cvrat;
}SclCfg;

typedef struct
{
    HI_U32 u32Yhrat;
    HI_U32 u32Yvrat;
    HI_U32 u32Chrat;
    HI_U32 u32Cvrat;
}SYS_SclRate;

typedef struct hiSYS_EXPORT_FUNC_S
{
    HI_U64  (*pfnSYSGetTimeStamp)  (HI_VOID);
    HI_VOID (*pfnSYSSyncTimeStamp) (HI_U64 u64Base, HI_BOOL bInit);
    HI_S32  (*pfnSysGetStride)     (HI_U32 u32Width, HI_U32 *pu32Stride);
    HI_U32  (*pfnSysGetChipVersion)(HI_VOID);
    HI_S32  (*pfnSysDrvIoCtrl)     (MPP_CHN_S *pstMppChn, SYS_FUNC_E enFuncId, HI_VOID *pIoArgs);    

    HI_S32  (*pfnSysRegisterSender)     (BIND_SENDER_INFO_S *pstInfo);
    HI_S32  (*pfnSysUnRegisterSender)   (MOD_ID_E enModId);
    HI_S32  (*pfnSysRegisterReceiver)   (BIND_RECEIVER_INFO_S *pstInfo);
    HI_S32  (*pfnSysUnRegisterReceiver) (MOD_ID_E enModId);
    HI_S32  (*pfnSysSendData)      (MOD_ID_E enModId, HI_S32 s32DevId, HI_S32 s32ChnId, HI_BOOL bBlock, 
        								MPP_DATA_TYPE_E enDataType, HI_VOID *pvData);

    HI_S32  (*pfnSysResetData)     (MOD_ID_E enModId, HI_S32 s32DevId, 
                                            HI_S32 s32ChnId, HI_VOID *pPrivate);
    
    HI_S32 (*pfnGetBindbySrc)(MPP_CHN_S *pstSrcChn,MPP_BIND_SRC_S *pstBindSrc);
    HI_S32 (*pfnGetBindbyDest)(MPP_CHN_S *pstDestChn,MPP_CHN_S *pstSrcChn);
    HI_S32 (*pfnGetMmzName)(MPP_CHN_S *pstChn,HI_VOID **ppMmzName);
    HI_S32 (*pfnGetMemIndex)(HI_U32 u32PhyAddr,HI_U32 *pu32MemIndex);
    HI_S32 (*pfnGetMemDdr)(MPP_CHN_S *pstChn,MPP_SYS_DDR_NAME_S *pstMemDdr);
    
    HI_U32  (*pfnSysVRegRead)  (HI_U32 u32Addr, HI_U32 u32Bytes);
    HI_S32  (*pfnSysVRegWrite)  (HI_U32 u32Addr, HI_U32 u32Value, HI_U32 u32Bytes);
    HI_U32  (*pfnSysGetVRegAddr)  (HI_VOID);
	HI_U32  (*pfnSysGetDciCoef) (MOD_ID_E enModId,  HI_U32* pu32PhyAddr);
	HI_U32  (*pfnSysGetScaleCoef) (MOD_ID_E enModId,  HI_BOOL bDeflicker, HI_VOID* pvRate, HI_VOID* pvCfg);
	HI_U64  (*pfnSYSGetSchedClock)  (HI_VOID);
	HI_S32  (*pfnSYSGetFullChipId)  (HI_U32 *pu32ChipId);
} SYS_EXPORT_FUNC_S;

#define CKFN_SYS_ENTRY()  CHECK_FUNC_ENTRY(HI_ID_SYS)

#define CKFN_SYS_GetSchedClock() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSYSGetSchedClock)
#define CALL_SYS_GetSchedClock() \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSYSGetSchedClock()

#define CKFN_SYS_GetTimeStamp() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSYSGetTimeStamp)
#define CALL_SYS_GetTimeStamp() \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSYSGetTimeStamp()

#define CKFN_SYS_SyncTimeStamp() \
        (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSYSSyncTimeStamp)
#define CALL_SYS_SyncTimeStamp(u64Base, bInit) \
        FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSYSSyncTimeStamp(u64Base, bInit)

#define CKFN_SYS_GetStride() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysGetStride)
#define CALL_SYS_GetStride(u32Width, pu32Stride) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysGetStride(u32Width, pu32Stride)

#define CKFN_SYS_GetChipVersion() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysGetChipVersion)
#define CALL_SYS_GetChipVersion() \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysGetChipVersion()


#define CKFN_SYS_DrvIoCtrl() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysDrvIoCtrl)
#define CALL_SYS_DrvIoCtrl(pstMppChn, enFuncId, pIoArgs) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysDrvIoCtrl(pstMppChn, enFuncId, pIoArgs)

#define CKFN_SYS_RegisterSender() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysRegisterSender)
#define CALL_SYS_RegisterSender(pstInfo) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysRegisterSender(pstInfo)

#define CKFN_SYS_UnRegisterSender() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysUnRegisterSender)
#define CALL_SYS_UnRegisterSender(enModId) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysUnRegisterSender(enModId)

#define CKFN_SYS_RegisterReceiver() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysRegisterReceiver)
#define CALL_SYS_RegisterReceiver(pstInfo) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysRegisterReceiver(pstInfo)

#define CKFN_SYS_UnRegisterReceiver() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysUnRegisterReceiver)
#define CALL_SYS_UnRegisterReceiver(enModId) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysUnRegisterReceiver(enModId)

#define CKFN_SYS_SendData() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysSendData)
#define CALL_SYS_SendData(enModId, s32DevId, s32ChnId, bBlock, enDataType, pvData) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysSendData(enModId, s32DevId, s32ChnId, bBlock, enDataType, pvData)

#define CKFN_SYS_ResetData() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysResetData)
#define CALL_SYS_ResetData(enModId, s32DevId, s32ChnId, pPrivate) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysResetData(enModId, s32DevId, s32ChnId, pPrivate)

#define CKFN_SYS_GetBindbySrc() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetBindbySrc)
#define CALL_SYS_GetBindbySrc(pstSrcChn,pstBindSrc) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetBindbySrc(pstSrcChn,pstBindSrc)

#define CKFN_SYS_GetBindbyDest() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetBindbyDest)
#define CALL_SYS_GetBindbyDest(pstDestChn,pstSrcChn) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetBindbyDest(pstDestChn,pstSrcChn)

#define CKFN_SYS_GetMmzName() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetMmzName)
#define CALL_SYS_GetMmzName(pstSrcChn,ppMmzName) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetMmzName(pstSrcChn,ppMmzName)

#define CKFN_SYS_GetMemIndex() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetMemIndex)
#define CALL_SYS_GetMemIndex(u32PhyAddr,pu32MemIndex) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetMemIndex(u32PhyAddr,pu32MemIndex)

#define CKFN_SYS_GetMemDdr() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetMemDdr)
#define CALL_SYS_GetMemDdr(pstSrcChn,pstMemDdr) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnGetMemDdr(pstSrcChn,pstMemDdr)

#define CKFN_SYS_GetDciCoef() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysGetDciCoef)
#define CALL_SYS_GetDciCoef(enModId,pu32PhyAddr) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysGetDciCoef(enModId,pu32PhyAddr)

#define CKFN_SYS_GetScaleCoef() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysGetScaleCoef)
#define CALL_SYS_GetScaleCoef(enModId, bDeflicker, pvRate, pvCfg) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSysGetScaleCoef(enModId, bDeflicker, pvRate, pvCfg)

#define CKFN_SYS_GetFullChipId() \
    (NULL != FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSYSGetFullChipId)
#define CALL_SYS_GetFullChipId(pu32ChipId) \
    FUNC_ENTRY(SYS_EXPORT_FUNC_S, HI_ID_SYS)->pfnSYSGetFullChipId(pu32ChipId)


#endif /* __HI_SYS_H__ */

