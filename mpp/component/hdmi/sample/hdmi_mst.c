/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : vou_mst.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/5/22
  Description   :
  History       :
  1.Date        : 2013/5/22
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <signal.h>
#include <math.h>
#include <linux/fb.h>
#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vi.h"
#include "hi_defines.h"
#include "hi_comm_hdmi.h"
#include "hi_comm_venc.h"
#include "hi_comm_vdec.h"
#include "hi_comm_region.h"

#include "mpi_vpss.h"
#include "mpi_vo.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_hdmi.h"
#include "mpi_venc.h"
#include "mpi_vdec.h"
#include "mpi_region.h"

#include "hdmi_mst.h"

#ifdef __cplusplus
#if __cplusplus
    extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum hiVdecThreadCtrlSignal_E
{
    VDEC_CTRL_START,
    VDEC_CTRL_PAUSE,
    VDEC_CTRL_STOP,
}VdecThreadCtrlSignal_E;

typedef struct hiVdecThreadParam
{
    HI_S32 s32ChnId;
	HI_CHAR cFileName[128];
	HI_BOOL bFileOpt;

    PAYLOAD_TYPE_E enType;
	HI_S32 s32StreamMode;
	HI_S32 s32MilliSec;
	HI_S32 s32MinBufSize;
	HI_S32 s32IntervalTime;
	VdecThreadCtrlSignal_E	 eCtrlSinal;
	HI_BOOL bErrorGen;
	HI_S32 s32ErrInterval;
	HI_S32 s32ErrLength;
    HI_U64  u64PtsInit;
    HI_U64  u64PtsInc;
    HI_BOOL bCheckPts;
    HI_BOOL bSendMannual;
    HI_BOOL bOutPutByDecodeOrder;
    HI_BOOL bSave2File;
    HI_BOOL bInstant;
    HI_BOOL bCircleSend;

    HI_U32 u32StartCode[3];
    HI_BOOL bPtsTest;
}VdecThreadParam;

typedef struct hiPthreadInfo
{
    VdecThreadParam  *pstVdecSend;
    HI_BOOL          bReverse;
    HI_U64           u64PtsInterval;
    HI_U32           u32FrameRate;
    HI_U32           u32ChnNum;
}PthreadInfo;

typedef struct hiHDMI_ARGS_S
{
    HI_HDMI_ID_E  enHdmi;
}HDMI_ARGS_S;

VIDEO_DISPLAY_MODE_E enDisplayMode = VIDEO_DISPLAY_MODE_PLAYBACK;//VIDEO_DISPLAY_MODE_PREVIEW;//

#if defined(CHIP_HI3536D)
PIXEL_FORMAT_E   g_enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_422;
#else
PIXEL_FORMAT_E   g_enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
#endif

PIXEL_FORMAT_E   g_enVpssPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

pthread_t        g_VdecThread[VDEC_MAX_CHN_NUM*3];
VdecThreadParam  g_stVdecSend[VDEC_MAX_CHN_NUM];
VdecThreadParam  g_stVdecGetPic[VDEC_MAX_CHN_NUM];

VO_PART_MODE_E   enVoPartitionMode = VO_PART_MODE_MULTI;
HI_HDMI_CALLBACK_FUNC_S g_stCallbackFunc;
HDMI_ARGS_S      g_stHdmiArgs;

#define ALIGN_UP(x, a)              ((x+a-1)&(~(a-1)))

#define CHECK_POINTER(p) \
do{                      \
    if(HI_NULL == p){\
        printf("The pointer is null\n");       \
        return HI_FAILURE;\
    } \
}while(0)

#define CHECK_POINTER_NO_RET(p) \
do{                      \
    if(HI_NULL == p){\
        printf("The pointer is null\n");       \
        return; \
    } \
}while(0)

#define CHECK_RET_SUCCESS(val) \
do{                      \
    if(HI_SUCCESS != val){\
        printf("return value is not HI_SUCCESS, 0x%x!\n", val);       \
        return val; \
    } \
}while(0)

#define CHECK_RET_SUCCESS_NO_RET(val) \
do{                      \
    if(HI_SUCCESS != val){\
        printf("return value is not HI_SUCCESS, 0x%x!\n", val);       \
        return ; \
    } \
}while(0)

HI_VOID * VDEC_MST_PTHREAD_SendSTREAM(HI_VOID *pArgs)
{
	VdecThreadParam *pstVdecThreadParam =(VdecThreadParam *)pArgs;
	FILE *fpStrm=NULL;
	HI_U8 *pu8Buf = NULL;
	VDEC_STREAM_S stStream;
	HI_BOOL bFindStart, bFindEnd;
	HI_S32 s32Ret,  i,  start = 0;
    HI_S32 s32UsedBytes = 0, s32ReadLen;
    HI_U64 u64pts = 0;
    char c;
    HI_S32 len;

	memset(&stStream, 0, sizeof(VDEC_STREAM_S) );

	if(pstVdecThreadParam->cFileName!=0)
	{
		fpStrm=fopen(pstVdecThreadParam->cFileName,"rb");
		if(fpStrm==NULL)
		{
			printf("can't open file %s in send stream thread:%d\n",pstVdecThreadParam->cFileName,pstVdecThreadParam->s32ChnId);
			return (HI_VOID *)(HI_FAILURE);
		}
	}
	pu8Buf=malloc(pstVdecThreadParam->s32MinBufSize);
	if(pu8Buf==NULL)
	{
		printf("can't alloc %d in send stream thread:%d\n",pstVdecThreadParam->s32MinBufSize,pstVdecThreadParam->s32ChnId);
		fclose(fpStrm);
		return (HI_VOID *)(HI_FAILURE);
	}

    pstVdecThreadParam->u64PtsInit = 0x0LL;
    pstVdecThreadParam->u64PtsInc = 0x0LL;

	fflush(stdout);

	while (1)
    {
        if (pstVdecThreadParam->eCtrlSinal == VDEC_CTRL_STOP)
        {
            break;
        }

        if (pstVdecThreadParam->eCtrlSinal == VDEC_CTRL_PAUSE)
        {
            sleep(pstVdecThreadParam->s32IntervalTime);
            continue;
        }

        if (pstVdecThreadParam->s32StreamMode==VIDEO_MODE_FRAME
            && pstVdecThreadParam->enType == PT_MP4VIDEO)
        {
            bFindStart = HI_FALSE;
            bFindEnd   = HI_FALSE;
            fseek(fpStrm, s32UsedBytes, SEEK_SET);
            s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);

            if (s32ReadLen == 0)
            {
                if (pstVdecThreadParam->bCircleSend == HI_TRUE)
                {
                    s32UsedBytes = 0;
                    fseek(fpStrm, 0, SEEK_SET);
                    s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
                }
                else
                {
                    memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
                    stStream.bEndOfStream = HI_TRUE;
                    HI_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, -1);
                    break;
                }
            }
            for (i=0; i<s32ReadLen-4; i++)
            {
                if (pu8Buf[i  ] == 0 && pu8Buf[i+1] == 0 &&
                    pu8Buf[i+2] == 1 && pu8Buf[i+3] == 0xB6)
                {
                    bFindStart = HI_TRUE;
                    i += 4;
                    break;
                }
            }

            for (; i<s32ReadLen-4; i++)
            {
                if (pu8Buf[i  ] == 0 && pu8Buf[i+1] == 0 &&
                    pu8Buf[i+2] == 1 && pu8Buf[i+3] == 0xB6)
                {
                    bFindEnd = HI_TRUE;
                    break;
                }
            }

            s32ReadLen = i;
            if (bFindStart == HI_FALSE)
            {
                printf("can not find start code! %d \n",
                    pstVdecThreadParam->s32ChnId);
            }
            else if (bFindEnd == HI_FALSE)
            {
                s32ReadLen = i+4;
            }

        }
        else if (pstVdecThreadParam->s32StreamMode==VIDEO_MODE_FRAME
            && pstVdecThreadParam->enType == PT_H264)
        {
            bFindStart = HI_FALSE;
            bFindEnd   = HI_FALSE;
            fseek(fpStrm, s32UsedBytes, SEEK_SET);
            s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);

            if (s32ReadLen == 0)
            {
                if (pstVdecThreadParam->bCircleSend == HI_TRUE)
                {
                    s32UsedBytes = 0;
                    fseek(fpStrm, 0, SEEK_SET);
                    s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
                }
                else
                {
                    /* End of stream send */
                    memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
                    stStream.bEndOfStream = HI_TRUE;
                    HI_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, -1);
                    break;
                }
            }

            for (i=0; i<s32ReadLen-5; i++)
            {
                if (  pu8Buf[i  ] == 0 && pu8Buf[i+1] == 0 && pu8Buf[i+2] == 1 &&
                    ((pu8Buf[i+3]&0x1F) == 0x5 || (pu8Buf[i+3]&0x1F) == 0x1) &&
                    ((pu8Buf[i+4]&0x80) == 0x80)
                   )
                {
                    bFindStart = HI_TRUE;
                    i += 4;
                    break;
                }
            }

            for (; i<s32ReadLen-5; i++)
            {
                if (  pu8Buf[i  ] == 0 && pu8Buf[i+1] == 0 && pu8Buf[i+2] == 1 &&
                    ((pu8Buf[i+3]&0x1F) == 0x5 || (pu8Buf[i+3]&0x1F) == 0x1) &&
                    ((pu8Buf[i+4]&0x80) == 0x80)
                   )
                {
                    bFindEnd = HI_TRUE;
                    break;
                }
            }

            s32ReadLen = i;
            if (bFindStart == HI_FALSE)
            {
                printf("can not find start code! %d \n", pstVdecThreadParam->s32ChnId);
            }
            else if (bFindEnd == HI_FALSE)
            {
                s32ReadLen = i+5;
            }

        }
        else if (pstVdecThreadParam->enType == PT_MJPEG || pstVdecThreadParam->enType == PT_JPEG)
        {
            // jpeg change
            bFindStart = HI_FALSE;
            bFindEnd   = HI_FALSE;


            fseek(fpStrm, s32UsedBytes, SEEK_SET);
            s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
            if (s32ReadLen == 0)
            {
                if (pstVdecThreadParam->bCircleSend == HI_TRUE)
                {
                    s32UsedBytes = 0;
                    fseek(fpStrm, 0, SEEK_SET);
                    s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
                }
                else
                {
                    /* End of stream send */
                    memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
                    stStream.bEndOfStream = HI_TRUE;
                    HI_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, -1);
                    break;
                }
            }

            for (i=0; i<s32ReadLen-2; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i+1] == 0xD8)
                {
                    start = i;
                    bFindStart = HI_TRUE;
                    i = i + 2;
                    break;
                }
            }

            for (; i<s32ReadLen-4; i++)
            {
                if ((pu8Buf[i] == 0xFF) && (pu8Buf[i+1]& 0xF0) == 0xE0)
                {
                     len = (pu8Buf[i+2]<<8) + pu8Buf[i+3];
                     i += 1 + len;
                }
                else
                {
                    break;
                }
            }

            for (; i<s32ReadLen-2; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i+1] == 0xD8)
                {
                    bFindEnd = HI_TRUE;
                    break;
                }
            }
            s32ReadLen = i;
            if (bFindStart == HI_FALSE)
            {
                printf("can not find start code! %d\n",
                    pstVdecThreadParam->s32ChnId);
            }
            else if (bFindEnd == HI_FALSE)
            {
                s32ReadLen = i+2;
            }
        }
        else
        {
            fseek(fpStrm, s32UsedBytes, SEEK_SET);
            s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);

            if (s32ReadLen == 0)
            {
                if (pstVdecThreadParam->bCircleSend == HI_TRUE)
                {
                    s32UsedBytes = 0;
                    fseek(fpStrm, 0, SEEK_SET);
                    s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
                }
                else
                {
                    /* End of stream send */
                    memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
                    stStream.bEndOfStream = HI_TRUE;
                    HI_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, -1);
                    break;
                }
            }
        }

        if (HI_TRUE == pstVdecThreadParam->bSendMannual)
        {
            printf("chn:%d,enter any key to send one frame(c to resum play)...\n",pstVdecThreadParam->s32ChnId);
			printf("ready to send %d Bytes\n",stStream.u32Len);
			c = getchar();
            if ('c' == c)
            {
               pstVdecThreadParam->bSendMannual = HI_FALSE;
            }
        }

        stStream.u64PTS  = u64pts;
		stStream.pu8Addr = pu8Buf + start;
		stStream.u32Len  = s32ReadLen;
		stStream.bEndOfFrame = HI_FALSE;
		stStream.bEndOfStream = HI_FALSE;

        s32Ret=HI_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, pstVdecThreadParam->s32MilliSec);

        if (HI_SUCCESS != s32Ret)
        {
			sleep(pstVdecThreadParam->s32IntervalTime);
        }
        else
        {
            s32UsedBytes = s32UsedBytes +s32ReadLen + start;
        }
		usleep(20000);
	}

	printf("send steam thread %d return ...\n", pstVdecThreadParam->s32ChnId);
	fflush(stdout);
	if (pu8Buf != HI_NULL)
        free(pu8Buf);
	fclose(fpStrm);
	return (HI_VOID *)HI_SUCCESS;
}

HI_VOID HDMI_MST_SYS_Init()
{
    VB_CONF_S stVbConf = {0};
    MPP_SYS_CONF_S struSysConf = {0};

	HI_U8 u8VpssRef = 2;
	HI_U8 u8WaveLevel = 1;
    HI_U8 u8VbCnt = 256+u8VpssRef;
	HI_U8 u8VdaCnt = 0;
    HI_S32 i = 0;

    HI_MPI_SYS_Exit();

    for (i=0; i<VB_MAX_USER; i++)
    {
         HI_MPI_VB_ExitModCommPool(i);
    }
    for (i=0; i<VB_MAX_POOLS; i++)
    {
         HI_MPI_VB_DestroyPool(i);
    }
    HI_MPI_VB_Exit();


    stVbConf.u32MaxPoolCnt = 64;

	// 1080p
    stVbConf.astCommPool[0].u32BlkSize = 1920*1080*2;
    stVbConf.astCommPool[0].u32BlkCnt  = 20;

	// d1
    stVbConf.astCommPool[1].u32BlkSize = 720*576*2;
    stVbConf.astCommPool[1].u32BlkCnt  = (HI_U32)(u8VbCnt) + u8VdaCnt + u8VdaCnt + u8WaveLevel;
    // cif
    stVbConf.astCommPool[2].u32BlkSize = 360*288*2;
    stVbConf.astCommPool[2].u32BlkCnt  = (HI_U32)u8VbCnt + u8WaveLevel;

    HDMI_CHECK_WITHOUT_RET(HI_MPI_VB_SetConf(&stVbConf), "SetVbConf");

    HDMI_CHECK_WITHOUT_RET(HI_MPI_VB_Init(), "VbInit");

    struSysConf.u32AlignWidth = 64;
    HDMI_CHECK_WITHOUT_RET(HI_MPI_SYS_SetConf(&struSysConf), "SetSysConf");

    HDMI_CHECK_WITHOUT_RET(HI_MPI_SYS_Init(), "SysInit");

    return;
}

HI_VOID HDMI_MST_SYS_Exit()
{
    HDMI_CHECK_WITHOUT_RET(HI_MPI_SYS_Exit(), "SysExit");

    HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
    HDMI_CHECK_WITHOUT_RET(HI_MPI_VB_Exit(), "VbExit");
}

HI_S32 HDMI_MST_BindVpssVo(VO_LAYER VoLayer, VO_CHN VoChn, VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId    = HI_ID_VPSS;
    stSrcChn.s32DevId   = VpssGrp;
    stSrcChn.s32ChnId   = VpssChn;

    stDestChn.enModId   = HI_ID_VOU;
    stDestChn.s32ChnId  = VoChn;
    stDestChn.s32DevId  = VoLayer;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

HI_S32 HDMI_MST_UnBindVoVpss(VO_LAYER VoLayer, VO_CHN VoChn)
{
    MPP_CHN_S stDestChn;

    stDestChn.enModId   = HI_ID_VOU;
    stDestChn.s32DevId  = VoLayer;
    stDestChn.s32ChnId  = VoChn;

    return HI_MPI_SYS_UnBind(NULL, &stDestChn);
}


HI_S32 HDMI_MST_BindVdecVpss(VDEC_CHN VdecChn, VPSS_GRP VpssGrpv)
{
    MPP_CHN_S stBindSrc;
    MPP_CHN_S stBindDest;
    HI_S32 s32Ret;

    stBindDest.enModId = HI_ID_VPSS;
    stBindDest.s32DevId = VpssGrpv;
    stBindDest.s32ChnId = 0;

    stBindSrc.enModId = HI_ID_VDEC;
    stBindSrc.s32ChnId = VdecChn;

    s32Ret = HI_MPI_SYS_Bind(&stBindSrc, &stBindDest);
    if (s32Ret)
    {
        printf("%s: HI_MPI_SYS_Bind: VdecChn (%d) Vpss(%d) faild with %#x!\n",
            __FUNCTION__, VdecChn, VpssGrpv, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 HDMI_MST_UnBindVdecVpss(VPSS_GRP VpssGrp)
{
    HI_S32 s32Ret;
    MPP_CHN_S stDestChn;

    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = VpssGrp;
    stDestChn.s32ChnId = 0;

    s32Ret = HI_MPI_SYS_UnBind(NULL, &stDestChn);
    if (s32Ret)
    {
        printf("%s: HI_MPI_SYS_UnBind: Vpss(%d) faild with %#x!\n",
            __FUNCTION__,  VpssGrp, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_VOID HDMI_MST_GetDefVoAttr(VO_DEV VoDev, VO_INTF_SYNC_E enIntfSync,
        VO_PUB_ATTR_S *pstPubAttr, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr,
        HI_S32 s32SquareSort, VO_CHN_ATTR_S *astChnAttr)
{
    HI_U32 u32Frmt, u32Width, u32Height, j;
    HI_U32 VoLayer;

    switch (enIntfSync)
    {
        case VO_OUTPUT_PAL      :  u32Width = 720;  u32Height = 576;  u32Frmt = 25; break;
        case VO_OUTPUT_NTSC     :  u32Width = 704;  u32Height = 480;  u32Frmt = 30; break;
        case VO_OUTPUT_1080P24  :  u32Width = 1920; u32Height = 1080; u32Frmt = 24; break;
        case VO_OUTPUT_1080P25  :  u32Width = 1920; u32Height = 1080; u32Frmt = 25; break;
        case VO_OUTPUT_1080P30  :  u32Width = 1920; u32Height = 1080; u32Frmt = 30; break;
        case VO_OUTPUT_720P50   :  u32Width = 1280; u32Height = 720;  u32Frmt = 50; break;
        case VO_OUTPUT_720P60   :  u32Width = 1280; u32Height = 720;  u32Frmt = 60; break;
        case VO_OUTPUT_1080I50  :  u32Width = 1920; u32Height = 1080; u32Frmt = 50; break;
        case VO_OUTPUT_1080I60  :  u32Width = 1920; u32Height = 1080; u32Frmt = 60; break;
        case VO_OUTPUT_1080P50  :  u32Width = 1920; u32Height = 1080; u32Frmt = 50; break;
        case VO_OUTPUT_1080P60  :  u32Width = 1920; u32Height = 1080; u32Frmt = 60; break;
        case VO_OUTPUT_576P50   :  u32Width = 720;  u32Height = 576;  u32Frmt = 50; break;
        case VO_OUTPUT_480P60   :  u32Width = 720;  u32Height = 480;  u32Frmt = 60; break;
        case VO_OUTPUT_800x600_60: u32Width = 800;  u32Height = 600;  u32Frmt = 60; break;
        case VO_OUTPUT_1024x768_60:u32Width = 1024; u32Height = 768;  u32Frmt = 60; break;
        case VO_OUTPUT_1280x1024_60:u32Width =1280; u32Height = 1024; u32Frmt = 60; break;
        case VO_OUTPUT_1366x768_60:u32Width = 1366; u32Height = 768;  u32Frmt = 60; break;
        case VO_OUTPUT_1440x900_60:u32Width = 1440; u32Height = 900;  u32Frmt = 60; break;
        case VO_OUTPUT_1280x800_60:u32Width = 1280; u32Height = 800;  u32Frmt = 60; break;
        case VO_OUTPUT_1600x1200_60:u32Width = 1600; u32Height = 1200;  u32Frmt = 60; break;
        case VO_OUTPUT_1680x1050_60:u32Width = 1680; u32Height = 1050;  u32Frmt = 60; break;
        case VO_OUTPUT_1920x1200_60:u32Width = 1920; u32Height = 1200;  u32Frmt = 60; break;
        case VO_OUTPUT_1920x2160_30:u32Width = 1920; u32Height = 2160;  u32Frmt = 30; break;
        case VO_OUTPUT_2560x1440_30:u32Width = 2560; u32Height = 1440;  u32Frmt = 30; break;
        case VO_OUTPUT_2560x1600_60:u32Width = 2560; u32Height = 1600;  u32Frmt = 60; break;
        case VO_OUTPUT_3840x2160_30:u32Width = 3840; u32Height = 2160;  u32Frmt = 30; break;
        case VO_OUTPUT_3840x2160_60:u32Width = 3840; u32Height = 2160;  u32Frmt = 60; break;
        case VO_OUTPUT_640x480_60  :u32Width = 640; u32Height = 480;  u32Frmt = 60; break;
        case VO_OUTPUT_960H_PAL:u32Width = 960; u32Height = 576;  u32Frmt = 25; break;
        case VO_OUTPUT_960H_NTSC:u32Width = 960; u32Height = 480;  u32Frmt = 30; break;
        case VO_OUTPUT_USER    :   u32Width = 720;  u32Height = 576;  u32Frmt = 25; break;
        case VO_OUTPUT_2560x1440_60:u32Width = 2560; u32Height = 1440;  u32Frmt = 60; break;
        case VO_OUTPUT_3840x2160_25:u32Width = 3840; u32Height = 2160;  u32Frmt = 25; break;
        case VO_OUTPUT_3840x2160_50:u32Width = 3840; u32Height = 2160;  u32Frmt = 50; break;
        default: return;
    }
    HDMI_CHECK_RET(HI_MPI_VO_GetPubAttr(VoDev, pstPubAttr), "HI_MPI_VO_GetPubAttr");
    if (NULL != pstPubAttr)
    {
        pstPubAttr->enIntfSync = enIntfSync;
        pstPubAttr->u32BgColor = VO_BKGRD_BLUE;
        if (0 == VoDev)
        {
            pstPubAttr->enIntfType = VO_INTF_HDMI;
        }
        else if (VO_DEV_HD_END == VoDev)
        {
            pstPubAttr->enIntfType = VO_INTF_BT1120;
        }
        else if (VoDev >= VO_DEV_SD_STA && VoDev <= VO_DEV_SD_END)
        {
            pstPubAttr->enIntfType = VO_INTF_CVBS;
        }
    }
    if(VoDev >= VO_DEV_SD_STA)
    {
        VoLayer = VoDev + 1;
    }
    else
    {
        VoLayer = VoDev;
    }

    HDMI_CHECK_RET(HI_MPI_VO_GetVideoLayerAttr(VoLayer, pstLayerAttr), "HI_MPI_VO_GetVideoLayerAttr");
    HDMI_CHECK_RET(HI_MPI_VO_GetVideoLayerPartitionMode(VoLayer, &enVoPartitionMode), "HI_MPI_VO_GetVideoLayerPartitionMode");
    if (NULL != pstLayerAttr)
    {
        pstLayerAttr->stDispRect.s32X       = 0;
        pstLayerAttr->stDispRect.s32Y       = 0;
        pstLayerAttr->stDispRect.u32Width   = u32Width;
        pstLayerAttr->stDispRect.u32Height  = u32Height;
        pstLayerAttr->stImageSize.u32Width  = u32Width;
        pstLayerAttr->stImageSize.u32Height = u32Height;
        pstLayerAttr->u32DispFrmRt          = u32Frmt;
        pstLayerAttr->enPixFormat           = g_enPixFormat;
        pstLayerAttr->bDoubleFrame          = HI_FALSE;
        pstLayerAttr->bClusterMode          = HI_FALSE;
    }

    if (NULL != astChnAttr)
    {
        for (j=0; j<(s32SquareSort * s32SquareSort); j++)
        {
            astChnAttr[j].stRect.s32X       = ALIGN_BACK((u32Width/s32SquareSort) * (j%s32SquareSort), 2);
            astChnAttr[j].stRect.s32Y       = ALIGN_BACK((u32Height/s32SquareSort) * (j/s32SquareSort), 2);
            astChnAttr[j].stRect.u32Width   = ALIGN_BACK(u32Width/s32SquareSort, 2);
            astChnAttr[j].stRect.u32Height  = ALIGN_BACK(u32Height/s32SquareSort, 2);
            astChnAttr[j].u32Priority       = 0;
#if defined(CHIP_HI3536D)
            astChnAttr[j].bDeflicker        = HI_FALSE;
#else
            astChnAttr[j].bDeflicker        = HI_TRUE;
#endif
        }
    }

    return ;
}

static HI_VOID HDMI_MST_HdmiConvertSync(VO_INTF_SYNC_E enIntfSync,
    HI_HDMI_VIDEO_FMT_E *penVideoFmt)
{
    switch (enIntfSync)
    {
        case VO_OUTPUT_PAL:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_PAL;
            break;
        case VO_OUTPUT_NTSC:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_NTSC;
            break;
        case VO_OUTPUT_1080P24:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_24;
            break;
        case VO_OUTPUT_1080P25:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_25;
            break;
        case VO_OUTPUT_1080P30:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_30;
            break;
        case VO_OUTPUT_720P50:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_720P_50;
            break;
        case VO_OUTPUT_720P60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_720P_60;
            break;
        case VO_OUTPUT_1080I50:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080i_50;
            break;
        case VO_OUTPUT_1080I60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080i_60;
            break;
        case VO_OUTPUT_1080P50:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_50;
            break;
        case VO_OUTPUT_1080P60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_60;
            break;
        case VO_OUTPUT_576P50:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_576P_50;
            break;
        case VO_OUTPUT_480P60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_480P_60;
            break;
        case VO_OUTPUT_640x480_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_861D_640X480_60;
            break;
        case VO_OUTPUT_800x600_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_800X600_60;
            break;
        case VO_OUTPUT_1024x768_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1024X768_60;
            break;
        case VO_OUTPUT_1280x1024_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1280X1024_60;
            break;
        case VO_OUTPUT_1366x768_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1366X768_60;
            break;
        case VO_OUTPUT_1440x900_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1440X900_60;
            break;
        case VO_OUTPUT_1280x800_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1280X800_60;
            break;
        case VO_OUTPUT_1680x1050_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1680X1050_60;
            break;
        case VO_OUTPUT_1920x2160_30:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1920x2160_30;
            break;
        case VO_OUTPUT_1600x1200_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1600X1200_60;
            break;
        case VO_OUTPUT_1920x1200_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1920X1200_60;
            break;
        case VO_OUTPUT_2560x1440_30:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_2560x1440_30;
            break;
        case VO_OUTPUT_2560x1440_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_2560x1440_60;
            break;
        case VO_OUTPUT_2560x1600_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_2560x1600_60;
            break;
        case VO_OUTPUT_3840x2160_25:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_3840X2160P_25;
            break;
        case VO_OUTPUT_3840x2160_30:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_3840X2160P_30;
            break;
        case VO_OUTPUT_3840x2160_50:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_3840X2160P_50;
            break;
        case VO_OUTPUT_3840x2160_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_3840X2160P_60;
            break;
        default:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_CUSTOMER_DEFINE;
            break;
    }

    return;
}

static HI_VOID HDMI_HotPlug_Proc(HI_VOID *pPrivateData)
{
    HDMI_ARGS_S    stArgs;
    HI_S32         s32Ret = HI_FAILURE;
    HI_HDMI_ATTR_S stAttr;
    HI_HDMI_SINK_CAPABILITY_S stCaps;

    printf("EVENT: HPD\n");

    CHECK_POINTER_NO_RET(pPrivateData);

    memset(&stAttr, 0, sizeof(HI_HDMI_ATTR_S));
    memset(&stArgs, 0, sizeof(HDMI_ARGS_S));
    memcpy(&stArgs, pPrivateData, sizeof(HDMI_ARGS_S));
    memset(&stCaps, 0, sizeof(HI_HDMI_SINK_CAPABILITY_S));


    s32Ret = HI_MPI_HDMI_GetSinkCapability(stArgs.enHdmi, &stCaps);
    if(s32Ret != HI_SUCCESS)
    {
        printf("get sink caps failed!\n");
    }
    else
    {
        printf("get sink caps success!\n");
    }

    s32Ret = HI_MPI_HDMI_GetAttr(stArgs.enHdmi, &stAttr);
    CHECK_RET_SUCCESS_NO_RET(s32Ret);

    s32Ret = HI_MPI_HDMI_SetAttr(stArgs.enHdmi, &stAttr);
    CHECK_RET_SUCCESS_NO_RET(s32Ret);

    HI_MPI_HDMI_Start(stArgs.enHdmi);

    return;
}

static HI_VOID HDMI_UnPlug_Proc(HI_VOID *pPrivateData)
{
    HDMI_ARGS_S  stArgs;

    printf("EVENT: UN-HPD\n");

    CHECK_POINTER_NO_RET(pPrivateData);

    memset(&stArgs, 0, sizeof(HDMI_ARGS_S));
    memcpy(&stArgs, pPrivateData, sizeof(HDMI_ARGS_S));

    HI_MPI_HDMI_Stop(stArgs.enHdmi);

    return;
}

static HI_VOID HDMI_EventCallBack(HI_HDMI_EVENT_TYPE_E event, HI_VOID *pPrivateData)
{
    switch ( event )
    {
        case HI_HDMI_EVENT_HOTPLUG:
            HDMI_HotPlug_Proc(pPrivateData);
            break;
        case HI_HDMI_EVENT_NO_PLUG:
            HDMI_UnPlug_Proc(pPrivateData);
            break;
        default:
            break;
    }

    return;
}

HI_VOID HDMI_MST_HdmiStart(VO_INTF_SYNC_E enIntfSync)
{
    HI_HDMI_ATTR_S      stAttr;
    HI_HDMI_VIDEO_FMT_E enVideoFmt;

    HDMI_MST_HdmiConvertSync(enIntfSync, &enVideoFmt);
    HDMI_CHECK_RET(HI_MPI_HDMI_Init(), "HI_MPI_HDMI_Init");

	g_stHdmiArgs.enHdmi = HI_HDMI_ID_0;
	g_stCallbackFunc.pfnHdmiEventCallback = HDMI_EventCallBack;
    g_stCallbackFunc.pPrivateData = &g_stHdmiArgs;

    HI_MPI_HDMI_Open(HI_HDMI_ID_0);

    HI_MPI_HDMI_RegCallbackFunc(HI_HDMI_ID_0, &g_stCallbackFunc);

    HDMI_CHECK_RET(HI_MPI_HDMI_GetAttr(0, &stAttr), "HI_MPI_HDMI_GetAttr");
    stAttr.bEnableHdmi = HI_TRUE;

    stAttr.bEnableVideo = HI_TRUE;
    stAttr.enVideoFmt = enVideoFmt;
    stAttr.enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444;
    stAttr.enDeepColorMode = HI_HDMI_DEEP_COLOR_OFF;
    stAttr.bxvYCCMode = HI_FALSE;

    stAttr.bEnableAudio = HI_FALSE;
    stAttr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S;
    stAttr.bIsMultiChannel = HI_FALSE;

    stAttr.enBitDepth = HI_HDMI_BIT_DEPTH_16;

    stAttr.bEnableSpdInfoFrame = HI_FALSE;
    stAttr.bEnableMpegInfoFrame = HI_FALSE;

    stAttr.bDebugFlag = HI_FALSE;
    stAttr.bHDCPEnable = HI_FALSE;

    stAttr.b3DEnable = HI_FALSE;

    if (enIntfSync > 12)
    {
        stAttr.bEnableHdmi = HI_FALSE;
        stAttr.enVidOutMode = HI_HDMI_VIDEO_MODE_RGB444;
    }

    stAttr.bEnableAudio = HI_TRUE;                  /**< if enable audio */
    stAttr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S; /**< source of HDMI audio, HI_HDMI_SND_INTERFACE_I2S suggested.the parameter must be consistent with the input of AO*/

    stAttr.u8DownSampleParm = HI_FALSE;             /**< parameter of downsampling  rate of PCM audio, default: 0 */
    stAttr.u8I2SCtlVbit = 0;                        /**< reserved, should be 0, I2S control (0x7A:0x1D) */

    stAttr.bEnableAviInfoFrame = HI_TRUE;           /**< if enable AVI InfoFrame*/
    stAttr.bEnableAudInfoFrame = HI_TRUE;           /**< if enable AUDIO InfoFrame*/

    HDMI_CHECK_RET(HI_MPI_HDMI_SetAttr(0, &stAttr), "HI_MPI_HDMI_SetAttr");
    return ;
}


HI_VOID HDMI_MST_StopHdmi(HI_VOID)
{
    HI_MPI_HDMI_Stop(0);

    return;
}


HI_VOID HDMI_MST_ChangeHdmiFomat(VO_INTF_SYNC_E enIntfSync)
{
    HI_HDMI_ATTR_S      stAttr;
    HI_HDMI_VIDEO_FMT_E enVideoFmt;

    HDMI_MST_HdmiConvertSync(enIntfSync, &enVideoFmt);
    HDMI_CHECK_RET(HI_MPI_HDMI_GetAttr(0, &stAttr), "HI_MPI_HDMI_GetAttr");

    stAttr.enVideoFmt = enVideoFmt;

    HDMI_CHECK_RET(HI_MPI_HDMI_SetAttr(0, &stAttr), "HI_MPI_HDMI_SetAttr");
    HI_MPI_HDMI_Start(0);
    return ;
}

HI_VOID HDMI_MST_HdmiStop(HI_VOID)
{
    HI_MPI_HDMI_Stop(0);
    HI_MPI_HDMI_Close(0);
    HI_MPI_HDMI_DeInit();
    printf("HI_MPI_HDMI_Stop Out\n");
    return ;
}

int hdmi_vo = -1;
HI_VOID HDMI_MST_StartVO(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr,
    VO_VIDEO_LAYER_ATTR_S *astLayerAttr, VO_CHN_ATTR_S *astChnAttr,
    HI_S32 s32ChnNum, HI_BOOL bVgsBypass, HI_BOOL bSupportCompress)
{
    HI_S32 i,VoLayer;
    VO_BORDER_S  stBorder;
    VO_COMPRESS_ATTR_S stCompressAttr;

    stBorder.bBorderEn = HI_TRUE;
    stBorder.stBorder.u32TopWidth = 2;
    stBorder.stBorder.u32BottomWidth = 2;
    stBorder.stBorder.u32LeftWidth = 2;
    stBorder.stBorder.u32RightWidth = 2;
    stBorder.stBorder.u32Color = 0x00ff00;

    if(VoDev >= VO_DEV_SD_STA)
    {
        VoLayer = VoDev + 1;
    }
    else
    {
        VoLayer = VoDev;
    }

    HDMI_CHECK_RET(HI_MPI_VO_SetPubAttr(VoDev, pstPubAttr), "HI_MPI_VO_SetPubAttr");
    HDMI_CHECK_RET(HI_MPI_VO_Enable(VoDev), "HI_MPI_VO_Enable");
    HDMI_CHECK_RET(HI_MPI_VO_SetVideoLayerPartitionMode(VoLayer, enVoPartitionMode), "HI_MPI_VO_SetVideoLayerPartitionMode");
    HDMI_CHECK_RET(HI_MPI_VO_SetVideoLayerAttr(VoLayer, &astLayerAttr[0]), "HI_MPI_VO_SetVideoLayerAttr");
    if (bSupportCompress)
    {
        stCompressAttr.bSupportCompress = HI_TRUE;
        HDMI_CHECK_RET(HI_MPI_VO_SetVideoLayerCompressAttr(VoLayer, &stCompressAttr), "HI_MPI_VO_SetVideoLayerCompressAttr");
    }

    HDMI_CHECK_RET(HI_MPI_VO_EnableVideoLayer(VoLayer), "HI_MPI_VO_EnableVideoLayer");
    if ((1 == VoDev) && (s32ChnNum > 32))
    {
        s32ChnNum = 32;
    }

    for (i = 0; i < s32ChnNum; i++)
    {
        HDMI_CHECK_RET(HI_MPI_VO_SetChnAttr(VoLayer, i, &astChnAttr[i]), "HI_MPI_VO_SetChnAttr");
        if (VO_PART_MODE_SINGLE == enVoPartitionMode)
        {
            HDMI_CHECK_RET(HI_MPI_VO_SetChnBorder(VoLayer,i,&stBorder), "HI_MPI_VO_SetChnBorder");
        }

        HDMI_CHECK_RET(HI_MPI_VO_EnableChn(VoLayer, i), "HI_MPI_VO_EnableChn");

    }
    if (pstPubAttr->enIntfType & VO_INTF_HDMI)
    {
        HDMI_MST_HdmiStart(pstPubAttr->enIntfSync);
        hdmi_vo = VoDev;
    }
    return;
}


HI_VOID HDMI_MST_ChangeVOFormat(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr,
    VO_VIDEO_LAYER_ATTR_S *astLayerAttr, VO_CHN_ATTR_S *astChnAttr,
    HI_S32 s32ChnNum, HI_BOOL bVgsBypass, HI_BOOL bSupportCompress)
{
    HI_S32 i,VoLayer;
    VO_BORDER_S  stBorder;
    VO_COMPRESS_ATTR_S stCompressAttr;

    stBorder.bBorderEn = HI_TRUE;
    stBorder.stBorder.u32TopWidth = 2;
    stBorder.stBorder.u32BottomWidth = 2;
    stBorder.stBorder.u32LeftWidth = 2;
    stBorder.stBorder.u32RightWidth = 2;
    stBorder.stBorder.u32Color = 0x00ff00;

    if(VoDev >= VO_DEV_SD_STA)
    {
        VoLayer = VoDev + 1;
    }
    else
    {
        VoLayer = VoDev;
    }

    HDMI_CHECK_RET(HI_MPI_VO_SetPubAttr(VoDev, pstPubAttr), "HI_MPI_VO_SetPubAttr");
    HDMI_CHECK_RET(HI_MPI_VO_Enable(VoDev), "HI_MPI_VO_Enable");
    HDMI_CHECK_RET(HI_MPI_VO_SetVideoLayerPartitionMode(VoLayer, enVoPartitionMode), "HI_MPI_VO_SetVideoLayerPartitionMode");
    printf("##############VoDev %d VoLayer %d##############\n", VoDev, VoLayer);
    HDMI_CHECK_RET(HI_MPI_VO_SetVideoLayerAttr(VoLayer, &astLayerAttr[0]), "HI_MPI_VO_SetVideoLayerAttr");
    if (bSupportCompress)
    {
        stCompressAttr.bSupportCompress = HI_TRUE;
        HDMI_CHECK_RET(HI_MPI_VO_SetVideoLayerCompressAttr(VoLayer, &stCompressAttr), "HI_MPI_VO_SetVideoLayerCompressAttr");
    }

    HDMI_CHECK_RET(HI_MPI_VO_EnableVideoLayer(VoLayer), "HI_MPI_VO_EnableVideoLayer");
    if ((1 == VoDev) && (s32ChnNum > 32))
    {
        s32ChnNum = 32;
    }

    for (i = 0; i < s32ChnNum; i++)
    {
        HDMI_CHECK_RET(HI_MPI_VO_SetChnAttr(VoLayer, i, &astChnAttr[i]), "HI_MPI_VO_SetChnAttr");
        if (VO_PART_MODE_SINGLE == enVoPartitionMode)
        {
            HDMI_CHECK_RET(HI_MPI_VO_SetChnBorder(VoLayer,i,&stBorder), "HI_MPI_VO_SetChnBorder");
        }

        HDMI_CHECK_RET(HI_MPI_VO_EnableChn(VoLayer, i), "HI_MPI_VO_EnableChn");
    }

    return;
}


HI_VOID HDMI_MST_StopVO(VO_DEV VoDev, HI_S32 s32ChnNum)
{
    HI_S32 i, s32Ret;
    HI_S32 VoLayer;

    if (hdmi_vo == VoDev)
    {
        HDMI_MST_HdmiStop();
    }

    if(VoDev >= VO_DEV_SD_STA)
    {
        VoLayer = VoDev + 1;
    }
    else
    {
        VoLayer = VoDev;
    }
    for (i=0; i<s32ChnNum; i++)
    {
        s32Ret = HI_MPI_VO_DisableChn(VoLayer, i);
        HDMI_CHECK_RET(s32Ret, "HI_MPI_VO_DisableChn");
    }

    s32Ret = HI_MPI_VO_DisableVideoLayer(VoLayer);
    HDMI_CHECK_RET(s32Ret, "HI_MPI_VO_DisableVideoLayer");

    s32Ret = HI_MPI_VO_Disable(VoDev);
    HDMI_CHECK_RET(s32Ret, "HI_MPI_VO_Disable");

    return ;
}

HI_VOID HDMI_MST_StopVideo(VO_DEV VoDev, HI_S32 s32ChnNum)
{
    HI_S32 i, s32Ret;
    HI_S32 VoLayer;

    if(VoDev >= VO_DEV_SD_STA)
    {
        VoLayer = VoDev + 1;
    }
    else
    {
        VoLayer = VoDev;
    }
    for (i=0; i<s32ChnNum; i++)
    {
        s32Ret = HI_MPI_VO_DisableChn(VoLayer, i);
        HDMI_CHECK_RET(s32Ret, "HI_MPI_VO_DisableChn");
    }

    s32Ret = HI_MPI_VO_DisableVideoLayer(VoLayer);
    HDMI_CHECK_RET(s32Ret, "HI_MPI_VO_DisableVideoLayer");

    s32Ret = HI_MPI_VO_Disable(VoDev);
    HDMI_CHECK_RET(s32Ret, "HI_MPI_VO_Disable");

    return ;
}


void global_variable_reset()
{
    g_enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    enDisplayMode = VIDEO_DISPLAY_MODE_PLAYBACK;

    return;
}

int HDMI_MST_ExitMpp()
{
    HI_S32 i = 0;

    HI_MPI_SYS_Exit();
    for (i=0; i<VB_MAX_USER; i++)
    {
         HI_MPI_VB_ExitModCommPool(i);
    }

    for (i=0; i<VB_MAX_POOLS; i++)
    {
         HI_MPI_VB_DestroyPool(i);
    }
    HI_MPI_VB_Exit();

    return 0;
}

HI_VOID HDMI_MST_StartVpss(HI_S32 s32GrpNum, HI_BOOL bBorderEn)
{
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 0;
    VPSS_GRP_ATTR_S stGrpVpssAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;

    /* Create group */
    stGrpVpssAttr.bHistEn   = 0;
    stGrpVpssAttr.bIeEn     = 0;
    stGrpVpssAttr.bNrEn     = 0;
    stGrpVpssAttr.bDciEn    = 0;
    stGrpVpssAttr.bEsEn     = 0;
    stGrpVpssAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stGrpVpssAttr.enPixFmt  = g_enVpssPixFormat;
    stGrpVpssAttr.u32MaxW   = 3840;
    stGrpVpssAttr.u32MaxH   = 2160;

    /* set channal attribute */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    stVpssChnAttr.bBorderEn = bBorderEn;
    stVpssChnAttr.stBorder.u32Color = VO_BKGRD_GREEN;
    stVpssChnAttr.stBorder.u32BottomWidth= 2;
    stVpssChnAttr.stBorder.u32LeftWidth = 2;
    stVpssChnAttr.stBorder.u32RightWidth = 2;
    stVpssChnAttr.stBorder.u32TopWidth = 2;
    stVpssChnAttr.bSpEn = 0;

    for(VpssGrp = 0;VpssGrp < s32GrpNum;VpssGrp++)
    {
        HDMI_CHECK_RET(HI_MPI_VPSS_CreateGrp(VpssGrp, &stGrpVpssAttr),"HI_MPI_VPSS_CreateGrp!\n");

        for (VpssChn = 0; VpssChn < VPSS_MAX_PHY_CHN_NUM; VpssChn++)
        {
            if (3 == VpssChn)
            {
                continue;
            }
            HDMI_CHECK_RET(HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn,&stVpssChnAttr),"HI_MPI_VPSS_SetChnAttr\n");

            HDMI_CHECK_RET(HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn),"HI_MPI_VPSS_EnableChn\n");
        }
        HDMI_CHECK_RET(HI_MPI_VPSS_StartGrp(VpssGrp), "HI_MPI_VPSS_StartGrp!\n");
    }

}

HI_VOID HDMI_MST_StopVpss(HI_S32 s32GrpNum)
{
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 0;

    for(VpssGrp = 0;VpssGrp < s32GrpNum;VpssGrp++)
    {
        HDMI_CHECK_RET(HI_MPI_VPSS_StopGrp(VpssGrp), "HI_MPI_VPSS_StopGrp");

         for (VpssChn = 0; VpssChn < VPSS_MAX_PHY_CHN_NUM; VpssChn++)
         {
             HDMI_CHECK_RET(HI_MPI_VPSS_DisableChn(VpssGrp,VpssChn), "HI_MPI_VPSS_DisableChn");
         }

         HDMI_CHECK_RET(HI_MPI_VPSS_DestroyGrp(VpssGrp), "HI_MPI_VPSS_DestroyGrp");
    }
}

HI_VOID HDMI_MST_StartVdec(HI_S32 s32ChnNum, HI_U32 u32Width,HI_U32 u32Height,HI_CHAR *cpFileName)
{
    HI_S32 i, s32Ret;
    VDEC_CHN_ATTR_S stVdecAttr;
    HI_CHAR cFileName[128];
    VDEC_CHN_PARAM_S stChnParam;
    VDEC_PRTCL_PARAM_S stPrtclParam;
    VB_CONF_S stVbConf = {0};
    HI_S32 s32PicSize, s32PmvSize;


    /******************VENC VB init************************************/
     HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
     memset(&stVbConf,0,sizeof(VB_CONF_S));
     stVbConf.u32MaxPoolCnt = 2;

     VB_PIC_BLK_SIZE(u32Width, u32Height, PT_H264, s32PicSize);
     stVbConf.astCommPool[0].u32BlkSize   = s32PicSize;
     stVbConf.astCommPool[0].u32BlkCnt    = 5 * s32ChnNum;
     VB_PMV_BLK_SIZE(u32Width, u32Height, PT_H264, s32PmvSize);
     stVbConf.astCommPool[1].u32BlkSize   = s32PmvSize;
     stVbConf.astCommPool[1].u32BlkCnt    = 5 * s32ChnNum;
     HI_MPI_VB_SetModPoolConf(VB_UID_VDEC, &stVbConf);
     HI_MPI_VB_InitModCommPool(VB_UID_VDEC);
    /******************************************************/
    stVdecAttr.enType=PT_H264;
	stVdecAttr.u32BufSize=2*u32Width*u32Height;
	stVdecAttr.u32Priority=5;
	stVdecAttr.u32PicWidth=u32Width;
	stVdecAttr.u32PicHeight=u32Height;
	stVdecAttr.stVdecVideoAttr.enMode=VIDEO_MODE_STREAM;
    stVdecAttr.stVdecVideoAttr.u32RefFrameNum=1;
    stVdecAttr.stVdecVideoAttr.bTemporalMvpEnable = 1;

    sprintf(cFileName,cpFileName);
    for (i=0; i<s32ChnNum; i++)
    {
        s32Ret = HI_MPI_VDEC_CreateChn(i, &stVdecAttr);
        HDMI_CHECK_RET(s32Ret, "HI_MPI_VDEC_CreateChn");
        s32Ret = HI_MPI_VDEC_SetDisplayMode(i,enDisplayMode);
        HDMI_CHECK_RET(s32Ret, "HI_MPI_VDEC_SetDisplayMode");
    	g_stVdecSend[i].s32ChnId = i;
        sprintf(g_stVdecSend[i].cFileName, cFileName);

    	g_stVdecSend[i].s32IntervalTime = 1;
    	g_stVdecSend[i].s32MinBufSize = u32Width*u32Height;
#ifdef VDEC_FRM_MODE
        g_stVdecSend[i].s32StreamMode = VIDEO_MODE_FRAME;
#else
    	g_stVdecSend[i].s32StreamMode = VIDEO_MODE_STREAM;
#endif
    	g_stVdecSend[i].eCtrlSinal = VDEC_CTRL_START;
    	g_stVdecSend[i].u32StartCode[0]=0x41010000;
		g_stVdecSend[i].u32StartCode[1]=0x65010000;
		g_stVdecSend[i].u32StartCode[2]=0x01010000;
    	g_stVdecSend[i].s32MinBufSize = u32Width*u32Height;
    	g_stVdecSend[i].bCircleSend = 1;
        g_stVdecSend[i].s32MilliSec = 0;
        //g_stVdecSend[i].enType = PT_H264;

        g_stVdecGetPic[i].s32ChnId = i;
        g_stVdecGetPic[i].eCtrlSinal = VDEC_CTRL_START;
        g_stVdecGetPic[i].s32MilliSec = 0;
    	g_stVdecGetPic[i].s32IntervalTime = 1;
        sprintf(g_stVdecGetPic[i].cFileName,"Vo_test_chn%d.yuv",g_stVdecGetPic[i].s32ChnId);

        s32Ret = HI_MPI_VDEC_GetProtocolParam(g_stVdecSend[i].s32ChnId, &stPrtclParam);
        HDMI_CHECK_RET(s32Ret, "HI_MPI_VDEC_GetProtocolParam");

        stPrtclParam.enType = PT_H264;
        stPrtclParam.stH264PrtclParam.s32MaxSpsNum   = 32;
        stPrtclParam.stH264PrtclParam.s32MaxPpsNum   = 256;
        stPrtclParam.stH264PrtclParam.s32MaxSliceNum = 136;
        s32Ret = HI_MPI_VDEC_SetProtocolParam(g_stVdecSend[i].s32ChnId, &stPrtclParam);
        HDMI_CHECK_RET(s32Ret, "HI_MPI_VDEC_SetProtocolParam");

        if ((HI_FALSE == g_stVdecSend[i].bOutPutByDecodeOrder)&&
              ((stVdecAttr.enType == PT_H264) ||(stVdecAttr.enType == PT_MP4VIDEO)))
        {
            s32Ret = HI_MPI_VDEC_GetChnParam(g_stVdecSend[i].s32ChnId,&stChnParam);
            HDMI_CHECK_RET(s32Ret, "HI_MPI_VDEC_GetChnParam");

            stChnParam.s32DecOrderOutput = 0;
            stChnParam.s32DecMode = 0;
            stChnParam.enCompressMode = COMPRESS_MODE_NONE;
            s32Ret = HI_MPI_VDEC_SetChnParam(g_stVdecSend[i].s32ChnId,&stChnParam);
            HDMI_CHECK_RET(s32Ret, "HI_MPI_VDEC_SetChnParam");
        }

        s32Ret=HI_MPI_VDEC_StartRecvStream(g_stVdecSend[i].s32ChnId);
        HDMI_CHECK_RET(s32Ret, "HI_MPI_VDEC_StartRecvStream");

    	pthread_create(&g_VdecThread[i], 0, VDEC_MST_PTHREAD_SendSTREAM, (HI_VOID *)&g_stVdecSend[i]);
    }

    return ;
}

HI_VOID HDMI_MST_StopVdec(HI_S32 s32ChnNum)
{
    HI_S32 i, sRet;
    for (i=0; i<s32ChnNum; i++)
    {
        g_stVdecSend[i].eCtrlSinal = VDEC_CTRL_STOP;
        g_stVdecGetPic[i].eCtrlSinal = VDEC_CTRL_STOP;
        HDMI_CHECK_RET(HI_MPI_VDEC_StopRecvStream(i),"HI_MPI_VDEC_StopRecvStream");
        pthread_join(g_VdecThread[i],0);
        HDMI_CHECK_RET(HI_MPI_VDEC_DestroyChn(i),"HI_MPI_VDEC_DestroyChn");
    }
    sRet = HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
    if (HI_SUCCESS != sRet)
    {
        printf("HI_MPI_VB_ExitModCommPool sRet = 0x%x\n", sRet);
    }

    return ;
}

static PAYLOAD_TYPE_E gs_enPayloadType = PT_ADPCMA;

static SAMPLE_ADEC_S gs_stSampleAdec[ADEC_MAX_CHN_NUM];
static AUDIO_SOUND_MODE_E gs_enAudioSndMode = AUDIO_SOUND_MODE_MONO;
static AUDIO_SAMPLE_RATE_E gs_enSteamSmpleRate = AUDIO_SAMPLE_RATE_BUTT;
static HI_BOOL gs_bResampleEn = HI_FALSE;

typedef struct
 {
    HI_CHAR     *pSrcPath;
    HI_U32      u32SampleRate;
    HI_U32      u32ChnlNum;

    HI_CHAR     *pSrcPath_New;
    HI_U32      u32SampleRate_New;
    HI_U32      u32ChnlNum_New;
    AUDIO_DEV   AoDev;
 }ADUIO_INPUT_S;

static ADUIO_INPUT_S gs_AudioInput;
static HI_BOOL gs_bAudioInputModified = HI_FALSE;

HI_S32 HDMI_MST_AUDIO_SetSampleRate(HI_U32 u32SampleRate)
{
    printf("Update u32SampleRate:%d---->%d\n", gs_AudioInput.u32SampleRate, u32SampleRate);
    gs_bAudioInputModified = HI_FALSE;
    gs_AudioInput.u32SampleRate_New = gs_AudioInput.u32SampleRate;
    gs_AudioInput.u32ChnlNum_New = gs_AudioInput.u32ChnlNum;
    gs_AudioInput.pSrcPath_New = gs_AudioInput.pSrcPath;
    if (u32SampleRate != gs_AudioInput.u32SampleRate)
    {
        gs_AudioInput.u32SampleRate_New = u32SampleRate;
        gs_bAudioInputModified = HI_TRUE;
    }


    return HI_SUCCESS;
}

HI_S32 HDMI_MST_AUDIO_SetChnlNum(HI_U32 u32ChnlNum)
{
    printf("Update u32ChnlNum:%d---->%d\n", gs_AudioInput.u32ChnlNum, u32ChnlNum);
    gs_bAudioInputModified = HI_FALSE;
    gs_AudioInput.u32SampleRate_New = gs_AudioInput.u32SampleRate;
    gs_AudioInput.u32ChnlNum_New = gs_AudioInput.u32ChnlNum;
    gs_AudioInput.pSrcPath_New = gs_AudioInput.pSrcPath;
    if (u32ChnlNum != gs_AudioInput.u32ChnlNum)
    {
        gs_AudioInput.u32ChnlNum_New = u32ChnlNum;
        gs_bAudioInputModified = HI_TRUE;
    }

    return HI_SUCCESS;
}

HI_VOID HDMI_MST_AUDIO_StartHdmi(AIO_ATTR_S *pstAioAttr)
{
    HI_HDMI_ATTR_S stHdmiAttr;
    HI_HDMI_ID_E enHdmi = HI_HDMI_ID_0;

    HDMI_CHECK_RET(HI_MPI_HDMI_SetAVMute(enHdmi, HI_TRUE), "HDMI_SetAVMute");

    HDMI_CHECK_RET(HI_MPI_HDMI_GetAttr(enHdmi, &stHdmiAttr), "HDMI_GetAttr");

    stHdmiAttr.bEnableAudio = HI_TRUE;                  /**< if enable audio */
    stHdmiAttr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S; /**< source of HDMI audio, HI_HDMI_SND_INTERFACE_I2S suggested.the parameter must be consistent with the input of AO*/
    stHdmiAttr.enSampleRate = pstAioAttr->enSamplerate; /**< sampling rate of PCM audio,the parameter must be consistent with the input of AO */
    stHdmiAttr.u8DownSampleParm = HI_FALSE;             /**< parameter of downsampling rate of PCM audio, default :0 */

    stHdmiAttr.enBitDepth = 8 * (pstAioAttr->enBitwidth+1);   /**< bitwidth of audio,default :16,the parameter must be consistent with the config of AO */
    stHdmiAttr.u8I2SCtlVbit = 0;                        /**< reserved, should be 0, I2S control (0x7A:0x1D) */

    stHdmiAttr.bEnableAviInfoFrame = HI_TRUE;           /**< if enable AVI InfoFrame*/
    stHdmiAttr.bEnableAudInfoFrame = HI_TRUE;           /**< if enable AUDIO InfoFrame*/

    HDMI_CHECK_RET(HI_MPI_HDMI_Stop(enHdmi), "HDMI_Stop");


    HDMI_CHECK_RET(HI_MPI_HDMI_SetAttr(enHdmi, &stHdmiAttr), "HDMI_SetAttr");


    HDMI_CHECK_RET(HI_MPI_HDMI_Start(enHdmi), "HDMI_Start");


    HDMI_CHECK_RET(HI_MPI_HDMI_SetAVMute(enHdmi, HI_FALSE), "HDMI_SetAVMute");

    return;
}

/******************************************************************************
* function : Start Adec
******************************************************************************/
HI_S32 HDMI_MST_StartAdec(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    HI_S32 s32Ret;
    ADEC_CHN_ATTR_S stAdecAttr;
    ADEC_ATTR_ADPCM_S stAdecAdpcm;

    stAdecAttr.enType = enType;
    stAdecAttr.u32BufSize = 20;
    stAdecAttr.enMode = ADEC_MODE_STREAM;/* propose use pack mode in your app */

    if (PT_ADPCMA == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecAdpcm;
        stAdecAdpcm.enADPCMType = ADPCM_TYPE_DVI4;
    }
    else
    {
        printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAdecAttr.enType);
        return HI_FAILURE;
    }

    /* create adec chn*/
    s32Ret = HI_MPI_ADEC_CreateChn(AdChn, &stAdecAttr);
    if (s32Ret)
    {
        printf("%s: HI_MPI_ADEC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,\
               AdChn,s32Ret);
        return s32Ret;
    }
    return 0;
}

HI_S32 HDMI_MST_COMM_StartAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt,
                                 AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enInSampleRate, HI_BOOL bResampleEn)
{
    HI_S32 i;
    HI_S32 s32Ret;

    if (pstAioAttr->u32ClkChnCnt == 0)
    {
        pstAioAttr->u32ClkChnCnt = pstAioAttr->u32ChnCnt;
    }

    s32Ret = HI_MPI_AO_SetPubAttr(AoDevId, pstAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_AO_SetPubAttr(%d) failed with %#x!\n", __FUNCTION__, \
               AoDevId, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_AO_Enable(AoDevId);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_AO_Enable(%d) failed with %#x!\n", __FUNCTION__, AoDevId, s32Ret);
        return HI_FAILURE;
    }

    for (i = 0; i < s32AoChnCnt; i++)
    {
        s32Ret = HI_MPI_AO_EnableChn(AoDevId, i);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_AO_EnableChn(%d) failed with %#x!\n", __FUNCTION__, i, s32Ret);
            return HI_FAILURE;
        }

        if (HI_TRUE == bResampleEn && pstAioAttr->enSoundmode == AUDIO_SOUND_MODE_MONO)
        {
            s32Ret = HI_MPI_AO_DisableReSmp(AoDevId, i);
            s32Ret |= HI_MPI_AO_EnableReSmp(AoDevId, i, enInSampleRate);
            if (HI_SUCCESS != s32Ret)
            {
                printf("%s: HI_MPI_AO_EnableReSmp(%d,%d) failed with %#x!\n", __FUNCTION__, AoDevId, i, s32Ret);
                return HI_FAILURE;
            }
        }
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Ao bind Adec
******************************************************************************/
HI_S32 HDMI_MST_COMM_AoBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = AdChn;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

/******************************************************************************
* function : Ao unbind Adec
******************************************************************************/
HI_S32 HDMI_MST_COMM_AoUnbindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32ChnId = AdChn;
    stSrcChn.s32DevId = 0;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}

/******************************************************************************
* function : get stream from file, and send it  to Adec
******************************************************************************/
void *HDMI_MST_COMM_AdecProc(void *parg)
{
    HI_S32 s32Ret;
    AUDIO_STREAM_S stAudioStream;
    HI_U32 u32Len = 1024;
    HI_U32 u32ReadLen;
    HI_S32 s32AdecChn;
    HI_U8 *pu8AudioStream = NULL;
    SAMPLE_ADEC_S *pstAdecCtl = (SAMPLE_ADEC_S *)parg;
    FILE *pfd = pstAdecCtl->pfd;
    s32AdecChn = pstAdecCtl->AdChn;

    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (NULL == pu8AudioStream)
    {
        printf("%s: malloc failed!\n", __FUNCTION__);
        return NULL;
    }

    while (HI_TRUE == pstAdecCtl->bStart)
    {
        /* read from file */
        stAudioStream.pStream = pu8AudioStream;
        u32ReadLen = fread(stAudioStream.pStream, 1, u32Len, pfd);
        if (u32ReadLen <= 0)
        {
            fseek(pfd, 0, SEEK_SET);/*read file again*/
            continue;
        }

        /* here only demo adec streaming sending mode, but pack sending mode is commended */
        stAudioStream.u32Len = u32ReadLen;
        s32Ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
        if(HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_ADEC_SendStream(%d) failed with %#x!\n",\
                   __FUNCTION__, s32AdecChn, s32Ret);
            break;
        }
    }

    free(pu8AudioStream);
    pu8AudioStream = NULL;
    fclose(pfd);
    pstAdecCtl->bStart = HI_FALSE;
    return NULL;
}


/******************************************************************************
* function : Create the thread to get stream from file and send to adec
******************************************************************************/
HI_S32 HDMI_MST_COMM_CreatTrdFileAdec(ADEC_CHN AdChn, HI_CHAR  *pSrcPath)
{
    SAMPLE_ADEC_S *pstAdec = NULL;

    if (NULL == pSrcPath)
    {
        return HI_FAILURE;
    }

    pstAdec = &gs_stSampleAdec[AdChn];
    pstAdec->AdChn = AdChn;

    pstAdec->pfd = fopen(pSrcPath, "rb");
    if (NULL == pstAdec->pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, pSrcPath);
        return HI_FAILURE;
    }
    printf("open stream file:\"%s\" for adec(%d) ok\n", pSrcPath, AdChn);

    pstAdec->bStart = HI_TRUE;
    pthread_create(&pstAdec->stAdPid, 0, HDMI_MST_COMM_AdecProc, pstAdec);

    return HI_SUCCESS;
}


/******************************************************************************
* function : file -> ADec -> Ao
******************************************************************************/
HI_VOID HDMI_MST_AdecAo(AIO_ATTR_S *pstAioAttr, HI_CHAR  *pSrcPath)
{
	HI_S32 		s32AoChnCnt;
    HI_U32      i;

    gs_AudioInput.AoDev = 1;

//hi3531d_start
#if defined(CHIP_HI3531D)
    gs_AudioInput.AoDev = 2;
#endif
//hi3531d_end

    if (NULL == pstAioAttr)
    {
        printf("%s: input piont is invalid!\n", __FUNCTION__);
        return ;
    }

    HDMI_MST_AUDIO_StartHdmi(pstAioAttr);

    s32AoChnCnt = pstAioAttr->u32ChnCnt>>pstAioAttr->enSoundmode;
    for (i = 0; i < s32AoChnCnt; i ++)
    {
        HDMI_CHECK_RET(HDMI_MST_StartAdec(i, gs_enPayloadType), "StartAdec");
    }

    HDMI_CHECK_RET(HDMI_MST_COMM_StartAo(gs_AudioInput.AoDev, s32AoChnCnt, pstAioAttr, gs_enSteamSmpleRate, gs_bResampleEn), "StartAo");

    for (i = 0; i < s32AoChnCnt; i ++)
    {
        HDMI_CHECK_RET(HDMI_MST_COMM_AoBindAdec(gs_AudioInput.AoDev, i, i), "AoBindAdec");

        HDMI_CHECK_RET(HDMI_MST_COMM_CreatTrdFileAdec(i, pSrcPath), "CreatTrdFileAdec");

        printf("bind adec:%d to ao(%d,%d) ok \n", i, gs_AudioInput.AoDev, i);
    }

    return ;
}



HI_S32 HDMI_MST_StartAudio(HI_CHAR  *pSrcPath, HI_U32 u32Samplerate, HI_U32 u32ChnlNum)
{
    AIO_ATTR_S stAioAttr;
    HI_CHAR format[7] = {0};
    HI_U32 u32Len = strlen(pSrcPath);
    HI_U32 i = 0;
    HI_U32 u32StrmSmprate = 0;
    HI_U32 u32StrmPointNum = 0;

    if (u32Len <= 6)
    {
        printf("Wrong audio source file: %s!\n", pSrcPath);
        return HI_FAILURE;
    }

    for (i = 0; i < 6; i ++)
    {
        format[i] = pSrcPath[u32Len-6+i];
    }
    format[6] = '\0';

    if (strcmp(format, ".adpcm") == 0)
    {
        gs_enPayloadType = PT_ADPCMA;
    }
    else
    {
        printf("Wrong audio source file: %s with format \"%s\"!\n", pSrcPath, format);
        return HI_FAILURE;
    }

/* init stAio. all of cases will use it */
    memset(&stAioAttr, 0, sizeof(AIO_ATTR_S));
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag = 0;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32ChnCnt = u32ChnlNum;
    stAioAttr.u32ClkSel = 0;
    stAioAttr.enSamplerate = u32Samplerate;

    if (PT_ADPCMA == gs_enPayloadType)
    {
        u32StrmSmprate = AUDIO_SAMPLE_RATE_8000;
        u32StrmPointNum = 320;
    }
    else
    {
        printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, gs_enPayloadType);
        return HI_FAILURE;
    }

    stAioAttr.u32PtNumPerFrm = u32StrmPointNum * stAioAttr.enSamplerate / u32StrmSmprate;
    if (stAioAttr.u32PtNumPerFrm > MAX_AO_POINT_NUM || stAioAttr.u32PtNumPerFrm < MIN_AUDIO_POINT_NUM)
    {
        printf("%s: invalid u32PtNumPerFrm:%d\n", __FUNCTION__, stAioAttr.u32PtNumPerFrm);
        return HI_FAILURE;
    }

    gs_bResampleEn = HI_FALSE;
    gs_enSteamSmpleRate = AUDIO_SAMPLE_RATE_BUTT;
    if (stAioAttr.u32PtNumPerFrm != u32StrmPointNum)
    {
        gs_bResampleEn = HI_TRUE;
        gs_enSteamSmpleRate = u32StrmSmprate;
    }


    HDMI_MST_AdecAo(&stAioAttr,pSrcPath);

    gs_enAudioSndMode = stAioAttr.enSoundmode;

    gs_AudioInput.pSrcPath = pSrcPath;
    gs_AudioInput.u32ChnlNum = u32ChnlNum;
    gs_AudioInput.u32SampleRate = u32Samplerate;

    return HI_SUCCESS;
}

/******************************************************************************
* function : Destory the thread to get stream from file and send to adec
******************************************************************************/
HI_S32 HDMI_MST_COMM_DestoryTrdFileAdec(ADEC_CHN AdChn)
{
    SAMPLE_ADEC_S *pstAdec = NULL;

    pstAdec = &gs_stSampleAdec[AdChn];

    if (pstAdec->bStart)
    {
        pstAdec->bStart = HI_FALSE;
        pthread_join(pstAdec->stAdPid, 0);
    }

    return HI_SUCCESS;
}

HI_S32 HDMI_MST_COMM_StopAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt, HI_BOOL bResampleEn)
{
    HI_S32 i;
    HI_S32 s32Ret;

    for (i = 0; i < s32AoChnCnt; i++)
    {
        if (HI_TRUE == bResampleEn)
        {
            s32Ret = HI_MPI_AO_DisableReSmp(AoDevId, i);
            if (HI_SUCCESS != s32Ret)
            {
                printf("%s: HI_MPI_AO_DisableReSmp failed with %#x!\n", __FUNCTION__, s32Ret);
                return s32Ret;
            }
        }

        s32Ret = HI_MPI_AO_DisableChn(AoDevId, i);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_AO_DisableChn failed with %#x!\n", __FUNCTION__, s32Ret);
            return s32Ret;
        }
    }

    s32Ret = HI_MPI_AO_Disable(AoDevId);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_AO_Disable failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Stop Adec
******************************************************************************/
HI_S32 HDMI_MST_COMM_StopAdec(ADEC_CHN AdChn)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_ADEC_DestroyChn(AdChn);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_ADEC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
               AdChn, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 HDMI_MST_StopAudio()
{
    HI_U32 u32AoChnCnt = gs_AudioInput.u32ChnlNum>>gs_enAudioSndMode;
    HI_U32 i;

    for (i = 0; i < u32AoChnCnt; i ++)
    {
        HDMI_MST_COMM_DestoryTrdFileAdec(i);
        HDMI_MST_COMM_AoUnbindAdec(gs_AudioInput.AoDev, i, i);
    }

    HDMI_MST_COMM_StopAo(gs_AudioInput.AoDev, u32AoChnCnt, gs_bResampleEn);
    for (i = 0; i < u32AoChnCnt; i ++)
    {
        HDMI_MST_COMM_StopAdec(i);
    }

    return HI_SUCCESS;
}

HI_S32 HDMI_MST_AUDIO_Reset()
{
    if (gs_bAudioInputModified)
    {
        HDMI_MST_StopAudio();
        HDMI_MST_StartAudio(gs_AudioInput.pSrcPath_New, gs_AudioInput.u32SampleRate_New, gs_AudioInput.u32ChnlNum_New);
    }

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


