/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : vou_mst.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/05/22
  Description   :
  History       :
  1.Date        : 2013/05/22
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
#include "assert.h"

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vda.h"
#include "hi_comm_region.h"
#include "hi_comm_adec.h"
#include "hi_comm_aenc.h"
#include "hi_comm_ai.h"
#include "hi_comm_ao.h"
#include "hi_comm_aio.h"
#include "hi_comm_hdmi.h"
#include "hi_defines.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vpss.h"
#include "mpi_vdec.h"
#include "mpi_vda.h"
#include "mpi_region.h"
#include "mpi_adec.h"
#include "mpi_aenc.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_hdmi.h"



#define SAMPLE_AUDIO_PTNUMPERFRM   1024
#define SAMPLE_AUDIO_HDMI_AO_DEV 1
#define SAMPLE_AUDIO_AI_DEV 0
#define SAMPLE_AUDIO_AO_DEV 0

#ifdef __cplusplus
#if __cplusplus
    extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define TW2865_FILE "/dev/tw2865dev"
#define TW2960_FILE "/dev/tw2960dev"
#define TLV320_FILE "/dev/tlv320aic31"

#define VO_DEV_HD_END	1
#define VO_DEV_SD_STA	2
#define VO_DEV_SD_END	2

#define VO_BKGRD_RED      0xFF0000    /* red back groud color */
#define VO_BKGRD_GREEN    0x00FF00    /* green back groud color */
#define VO_BKGRD_BLUE     0x0000FF    /* blue back groud color */
#define VO_BKGRD_BLACK    0x000000    /* black back groud color */
#define VO_BKGRD_YELLOW   0xFFFF00    /* yellow back groud color */

typedef enum hiEN_HDMI_AV_E
{
    HDMI_VIDEO_PARAM_CHANGE  = 0,
    HDMI_AUDIO_PARAM_CHANGE,

    HDMI_PARAM_BUTT
}EN_HDMI_AV_E;

typedef struct PTHREAD_VENC
{
    VENC_CHN VeChnId;
    //VENC_GRP VeGrpID;
    
    HI_BOOL  bByPack;
    HI_S32   s32FrmCnt;
    HI_S32   s32ThreadId;
    HI_U32   u32PicWidth;
    HI_U32   u32PicHeight;

    HI_BOOL  bSendVdec;
    VDEC_CHN VdChnId;
    HI_CHAR  acFilePath[100];
    FILE     *pstream;
}VENC_PTHREAD_INFO_S;

typedef struct stPTHREAD_GET_MULTICHN_STREAM
{
    VENC_PTHREAD_INFO_S astThreadInfo[VENC_MAX_CHN_NUM];
    HI_U32    u32ChnNum;
    HI_BOOL   bSaveStream;
    pthread_mutex_t   stMutex;
}VENC_GET_MULTICHN_STREAM;

typedef enum hiAudioCodecType
{
    AUDIO_CODEC_INNER = 0,
    AUDIO_CODEC_TLV320,
    AUDIO_CODEC_HDMI,
    AUDIO_CODEC_TW2865,
    AUDIO_CODEC_BUTT
}AudioCodecType;

#define ACODEC_FILE     "/dev/acodec"

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */

typedef struct tagSAMPLE_AENC_S
{
    HI_BOOL bStart;
    pthread_t stAencPid;
    HI_S32  AeChn;
    HI_S32  AdChn;
    FILE    *pfd;
    HI_BOOL bSendAdChn;
} SAMPLE_AENC_S;

typedef struct tagSAMPLE_AI_S
{
    HI_BOOL bStart;
    HI_S32  AiDev;
    HI_S32  AiChn;
    HI_S32  AencChn;
    HI_S32  AoDev;
    HI_S32  AoChn;
    HI_BOOL bSendAenc;
    HI_BOOL bSendAo;
    pthread_t stAiPid;
} SAMPLE_AI_S;

typedef struct tagSAMPLE_ADEC_S
{
    HI_BOOL bStart;
    HI_S32 AdChn; 
    FILE *pfd;
    pthread_t stAdPid;
} SAMPLE_ADEC_S;

typedef struct tagSAMPLE_AO_S
{
	AUDIO_DEV AoDev;
	HI_BOOL bStart;
	pthread_t stAoPid;
}SAMPLE_AO_S;

#define ALIGN_BACK(x, a)              ((a) * (((x) / (a))))

#define HDMI_CHECK_WITHOUT_RET(express,name)\
do{\
    HI_S32 s32Ret;\
    s32Ret = express;\
    if (HI_SUCCESS != s32Ret)\
    {\
        printf("%s failed at %s: LINE: %d with %#x!\n", name, __FUNCTION__, __LINE__, s32Ret);\
        global_variable_reset();\
        return;\
    }\
}while(0)

#define HDMI_CHECK_RET(express,name)\
    do{\
        HI_S32 Ret;\
        Ret = express;\
        if (HI_SUCCESS != Ret)\
        {\
            printf("%s failed at %s: LINE: %d with %#x!\n", name, __FUNCTION__, __LINE__, Ret);\
            HDMI_MST_NOTPASS();\
        }\
    }while(0)


#define HDMI_PASS()\
do{\
    printf("\033[0;32mtest case <%s> passed\033[0;39m\n",__FUNCTION__);\
}while(0)

#define HDMI_NOT_PASS()\
do\
{\
    printf("\033[0;31mtest case <%s>not pass at line:%d.\033[0;39m\n",__FUNCTION__,__LINE__);\
}while(0)

#define HDMI_MST_NOTPASS() \
do{\
    HDMI_NOT_PASS();\
    global_variable_reset();\
    HDMI_MST_ExitMpp();\
    return ;\
}while(0)

#define HDMI_MST_PASS() \
do{\
    HDMI_PASS();\
    global_variable_reset();\
    HDMI_MST_ExitMpp();\
    return 1;\
}while(0)

HI_VOID HDMI_MST_SYS_Init();
HI_VOID HDMI_MST_SYS_Exit();
HI_S32 HDMI_MST_BindVpssVo(VO_LAYER VoLayer, VO_CHN VoChn, VPSS_GRP VpssGrp, VPSS_CHN VpssChn);
HI_S32 HDMI_MST_UnBindVoVpss(VO_LAYER VoLayer, VO_CHN VoChn);
HI_S32 HDMI_MST_BindVdecVpss(VDEC_CHN VdecChn, VPSS_GRP VpssGrpv);
HI_S32 HDMI_MST_UnBindVdecVpss(VPSS_GRP VpssGrp);
HI_VOID HDMI_MST_GetDefVoAttr(VO_DEV VoDev, VO_INTF_SYNC_E enIntfSync, 
            VO_PUB_ATTR_S *pstPubAttr, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr,
            HI_S32 s32SquareSort, VO_CHN_ATTR_S *astChnAttr);
HI_VOID HDMI_MST_StartVO(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr,
    VO_VIDEO_LAYER_ATTR_S *astLayerAttr, VO_CHN_ATTR_S *astChnAttr,
    HI_S32 s32ChnNum, HI_BOOL bVgsBypass, HI_BOOL bSupportCompress);
HI_VOID HDMI_MST_ChangeVOFormat(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr,
    VO_VIDEO_LAYER_ATTR_S *astLayerAttr, VO_CHN_ATTR_S *astChnAttr,
    HI_S32 s32ChnNum, HI_BOOL bVgsBypass, HI_BOOL bSupportCompress);
HI_VOID HDMI_MST_StopVO(VO_DEV VoDev, HI_S32 s32ChnNum);
HI_VOID HDMI_MST_StopVideo(VO_DEV VoDev, HI_S32 s32ChnNum);
HI_VOID HDMI_MST_StartVdec(HI_S32 s32ChnNum, HI_U32 u32Width,HI_U32 u32Height,HI_CHAR *cpFileName);
HI_VOID HDMI_MST_StopVdec(HI_S32 s32ChnNum);

HI_VOID HDMI_MST_StartVpss(HI_S32 s32GrpNum, HI_BOOL bBorderEn);
HI_VOID HDMI_MST_StopVpss(HI_S32 s32GrpNum);

HI_VOID HDMI_MST_HdmiStart(VO_INTF_SYNC_E enIntfSync);
HI_VOID HDMI_MST_StopHdmi(HI_VOID);
HI_VOID HDMI_MST_ChangeHdmiFomat(VO_INTF_SYNC_E enIntfSync);
HI_VOID HDMI_MST_HdmiStop(HI_VOID);
HI_VOID * VDEC_MST_PTHREAD_SendSTREAM(HI_VOID *pArgs);
HI_VOID global_variable_reset();
int HDMI_MST_ExitMpp();
HI_S32 HDMI_MST_StartAudio(HI_CHAR  *pSrcPath, HI_U32 u32Samplerate, HI_U32 u32ChnlNum);
HI_S32 HDMI_MST_StopAudio();
HI_S32 HDMI_MST_AUDIO_SetSampleRate(HI_U32 u32SampleRate);
HI_S32 HDMI_MST_AUDIO_SetChnlNum(HI_U32 u32ChnlNum);
HI_S32 HDMI_MST_AUDIO_Reset();


        

