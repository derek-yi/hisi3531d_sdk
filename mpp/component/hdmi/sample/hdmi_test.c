/*-------------------------------------------------------------------------

-------------------------------------------------------------------------*/
/*
 *  CUnit - A Unit testing framework library for C.
 *  Copyright (C) 2001        Anil Kumar
 *  Copyright (C) 2004, 2005  Anil Kumar, Jerry St.Clair
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vi.h"
#include "hi_defines.h"
#include "mpi_region.h"
#include "mpi_vpss.h"
#include "mpi_vo.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"

#include "hdmi_mst.h"
#include "hi_tde_api.h"
#include "hi_tde_type.h"

#ifdef __cplusplus
#if __cplusplus
    extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

extern PIXEL_FORMAT_E   g_enPixFormat;
extern HI_BOOL enDisplayMode;
extern VO_PART_MODE_E   enVoPartitionMode;
VO_DEV g_VoDev = 0;
VO_LAYER g_VoLayer = 0;
HI_S32 g_s32ChnNum = 1;
HI_BOOL g_bStartAudio = HI_FALSE;

#define SAMPLE_GET_INPUTCMD(InputCmd) fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin)

HI_VOID HdmiGetIntfSync(VO_INTF_SYNC_E *penIntfSync, char cCmd)
{
    switch(cCmd)
    {
        case '0':
            *penIntfSync = VO_OUTPUT_1080P24;
            break;
        case '1':
            *penIntfSync = VO_OUTPUT_1080P25;
            break;
        case '2':
            *penIntfSync = VO_OUTPUT_1080P30;
            break;
        case '3':
            *penIntfSync = VO_OUTPUT_720P50;
            break;
        case '4':
            *penIntfSync = VO_OUTPUT_720P60;
            break;
        case '5':
            *penIntfSync = VO_OUTPUT_1080I50;
            break;
        case '6':
            *penIntfSync = VO_OUTPUT_1080I60;
            break;
        case '7':
            *penIntfSync = VO_OUTPUT_1080P50;
            break;
        case '8':
            *penIntfSync = VO_OUTPUT_1080P60;
            break;
        case '9':
            *penIntfSync = VO_OUTPUT_576P50;
            break;
        case 'a':
            *penIntfSync = VO_OUTPUT_480P60;
            break;
        case 'b':
            *penIntfSync = VO_OUTPUT_800x600_60;
            break;
        case 'c':
            *penIntfSync = VO_OUTPUT_1024x768_60;
            break;
        case 'd':
            *penIntfSync = VO_OUTPUT_1280x1024_60;
            break;
        case 'e':
            *penIntfSync = VO_OUTPUT_1366x768_60;
            break;
        case 'f':
            *penIntfSync = VO_OUTPUT_1440x900_60;
            break;
        case 'g':
            *penIntfSync = VO_OUTPUT_1280x800_60;
            break;
        case 'h':
            *penIntfSync = VO_OUTPUT_1600x1200_60;
            break;
        case 'i':
            *penIntfSync = VO_OUTPUT_1680x1050_60;
            break;
        case 'j':
            *penIntfSync = VO_OUTPUT_1920x1200_60;
            break;
        case 'k':
            *penIntfSync = VO_OUTPUT_1920x2160_30;
            break;
        case 'l':
            *penIntfSync = VO_OUTPUT_2560x1440_30;
            break;
        case 'm':
            *penIntfSync = VO_OUTPUT_2560x1600_60;
            break;
        case 'n':
            *penIntfSync = VO_OUTPUT_3840x2160_30;
            break;
        case 'o':
            *penIntfSync = VO_OUTPUT_3840x2160_60;
            break;
        case 'p':
            *penIntfSync = VO_OUTPUT_640x480_60;
            break;
        default:
            *penIntfSync = VO_OUTPUT_1080P60;
            break;
    }
}

VO_INTF_SYNC_E HI_Disp_StrToFmt(HI_CHAR *pcString, HI_U32 *pu32Width, HI_U32 *pu32Height)
{
    VO_INTF_SYNC_E enIntfSync;

    if(0 == strcmp("1080P_24", pcString))
    {
        enIntfSync = VO_OUTPUT_1080P24;
        *pu32Width = 1920;
        *pu32Height = 1080;
    }
    else if(0 == strcmp("1080P_25", pcString))
    {
        enIntfSync = VO_OUTPUT_1080P25;
        *pu32Width = 1920;
        *pu32Height = 1080;
    }
    else if(0 == strcmp("1080P_30", pcString))
    {
        enIntfSync = VO_OUTPUT_1080P30;
        *pu32Width = 1920;
        *pu32Height = 1080;
    }
    else if(0 == strcmp("720P_50", pcString))
    {
        enIntfSync = VO_OUTPUT_720P50;
        *pu32Width = 1280;
        *pu32Height = 720;
    }
    else if(0 == strcmp("720P_60", pcString))
    {
        enIntfSync = VO_OUTPUT_720P60;
        *pu32Width = 1280;
        *pu32Height = 720;
    }
    else if(0 == strcmp("1080i_50", pcString))
    {
        enIntfSync = VO_OUTPUT_1080I50;
        *pu32Width = 1920;
        *pu32Height = 1080;
    }
    else if(0 == strcmp("1080i_60", pcString))
    {
        enIntfSync = VO_OUTPUT_1080I60;
        *pu32Width = 1920;
        *pu32Height = 1080;
    }
    else if(0 == strcmp("1080P_50", pcString))
    {
        enIntfSync = VO_OUTPUT_1080P50;
        *pu32Width = 1920;
        *pu32Height = 1080;
    }
    else if(0 == strcmp("1080P_60", pcString))
    {
        enIntfSync = VO_OUTPUT_1080P60;
        *pu32Width = 1920;
        *pu32Height = 1080;
    }
    else if(0 == strcmp("576P_50", pcString))
    {
        enIntfSync = VO_OUTPUT_576P50;
        *pu32Width = 720;
        *pu32Height = 576;
    }
    else if(0 == strcmp("480p_60", pcString))
    {
        enIntfSync = VO_OUTPUT_480P60;
        *pu32Width = 720;
        *pu32Height = 480;
    }
    else if(0 == strcmp("2160P_30", pcString))
    {
        enIntfSync = VO_OUTPUT_3840x2160_30;
        *pu32Width = 3840;
        *pu32Height = 2160;
    }
    else if(0 == strcmp("2160P_60", pcString))
    {
        enIntfSync = VO_OUTPUT_3840x2160_60;
        *pu32Width = 3840;
        *pu32Height = 2160;
    }
    else
    {
        enIntfSync = VO_OUTPUT_1080P60;
        *pu32Width = 1920;
        *pu32Height = 1080;
    }

    return enIntfSync;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void HDMI_Test_HandleSig(HI_S32 signo)
{
    HI_U32 i;
    if (SIGINT == signo || SIGTSTP == signo || SIGTERM == signo)
    {
        /* audio start */
        if (g_bStartAudio)
        {
            HDMI_MST_StopAudio();
        }
        /* audio end */

        HDMI_MST_StopVO(g_VoDev,g_s32ChnNum);
        for(i = 0;i < g_s32ChnNum;i++)
        {
            HDMI_MST_UnBindVoVpss(g_VoLayer, i);
        }
        for (i = 0; i < g_s32ChnNum; i++)
        {
            HDMI_MST_UnBindVdecVpss(i);
        }
        HDMI_MST_StopVpss(g_s32ChnNum);

        HDMI_MST_StopVdec(g_s32ChnNum);
        /* deinit */
        global_variable_reset();
        HDMI_MST_ExitMpp();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

extern HI_U32 HDMI_Test_CMD(HI_CHAR * pString);

HI_S32 main(HI_S32 argc,HI_CHAR *argv[])
{
    HI_CHAR                 InputCmd[128];
    VO_INTF_SYNC_E   enFormat = VO_OUTPUT_1080P60;
    HI_U32 u32Width = 1920;
    HI_U32 u32Height = 1080;
    HI_U32 i;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_CHN_ATTR_S stChnAttr[32];
    HI_U32 u32Samplerate = 48000;
    HI_U32 u32ChnlNum = 1;
    HI_CHAR VideoFileAttr[32] = {0};
    HI_CHAR AudioFileAttr[32] = {0};
    snprintf(VideoFileAttr, sizeof(VideoFileAttr), "%s", "1080P.h264");
    snprintf(AudioFileAttr, sizeof(AudioFileAttr), "%s", "mysecret.adpcm");
    signal(SIGINT, HDMI_Test_HandleSig);
    signal(SIGTERM, HDMI_Test_HandleSig);

    HDMI_MST_SYS_Init();
    HDMI_MST_StartVdec(g_s32ChnNum, u32Width, u32Height, VideoFileAttr);
    HDMI_MST_StartVpss(g_s32ChnNum, HI_TRUE);

    for (i = 0; i < g_s32ChnNum; i++)
    {
        HDMI_MST_BindVdecVpss(i, i);
    }

    HDMI_MST_GetDefVoAttr(g_VoDev, enFormat, &stPubAttr, &stLayerAttr, g_s32ChnNum, stChnAttr);
    HDMI_MST_StartVO(g_VoDev, &stPubAttr, &stLayerAttr, stChnAttr, g_s32ChnNum, HI_FALSE, HI_FALSE);
    for(i = 0;i < g_s32ChnNum;i++)
    {
        HDMI_MST_BindVpssVo(g_VoLayer,i,i,0);
    }

    /* audio start */
    HDMI_MST_StartAudio(AudioFileAttr, u32Samplerate, u32ChnlNum);
    g_bStartAudio = HI_TRUE;
    /* audio end */

    while(1)
    {
        printf("please input 'h' to get help or 'q' to quit!\n");
        printf("hdmi_cmd >");
        memset(InputCmd, 0, 128);
        SAMPLE_GET_INPUTCMD(InputCmd);
        if ('q' == InputCmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

        HDMI_Test_CMD(InputCmd);
    }

    /* audio start */
    if (g_bStartAudio)
    {
        HDMI_MST_StopAudio();
    }
    /* audio end */

    HDMI_MST_StopVO(g_VoDev,g_s32ChnNum);
    for(i = 0;i < g_s32ChnNum;i++)
    {
        HDMI_MST_UnBindVoVpss(g_VoLayer, i);
    }
    for (i = 0; i < g_s32ChnNum; i++)
    {
        HDMI_MST_UnBindVdecVpss(i);
    }
    HDMI_MST_StopVpss(g_s32ChnNum);

    HDMI_MST_StopVdec(g_s32ChnNum);
    /* deinit */
    HDMI_MST_PASS();

    return 1;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

