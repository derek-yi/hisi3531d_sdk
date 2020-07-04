/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv_firmware.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/16
  Description   : 
  History       :
  1.Date        : 2009/07/16
    Author      : Z44949
    Modification: Created file

******************************************************************************/

#ifndef __PCIV_FIRMWARE_H__
#define __PCIV_FIRMWARE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */
typedef struct hiPCIV_PORTMAP_NODE_S
{
    struct list_head list;
    PCIV_CHN pcivChn;
} PCIV_PORTMAP_NODE_S;

typedef struct hiPCIV_VBPOOL_S
{
    HI_U32 u32PoolCount;
    HI_U32 u32PoolId[PCIV_MAX_VBCOUNT];
    HI_U32 u32Size[PCIV_MAX_VBCOUNT];
} PCIV_VBPOOL_S;


/*get the distance of integer of 32-bit,deal with wrap state*/
__inline static HI_U32 PCIV_GetU32Span(HI_U32 u32Front, HI_U32 u32Back)
{
    return (u32Front >= u32Back) ? (u32Front-u32Back) : (u32Front + ((~0UL)-u32Back));
}

__inline static HI_S32 PcivFmwGetPicSize(VIDEO_FRAME_S *pstVFrame, HI_U32 *pu32BlkSize)
{
    switch (pstVFrame->enPixelFormat)
    {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            *pu32BlkSize = pstVFrame->u32Stride[0]*pstVFrame->u32Height*3/2;
            break;
        case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
            *pu32BlkSize = pstVFrame->u32Stride[0]*pstVFrame->u32Height*2;
            break;  
        case PIXEL_FORMAT_VYUY_PACKAGE_422:
            *pu32BlkSize = pstVFrame->u32Stride[0]*pstVFrame->u32Height;
            break;
        default:
            return -1;
    }
    return 0;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __PCIV_FIRMWARE_H__*/

