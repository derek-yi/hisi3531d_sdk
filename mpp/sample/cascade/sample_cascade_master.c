/******************************************************************************
  A simple program of Hisilicon Hi35xx video cascade implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-8 Created
******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h" 

#if 0
#define SAMPLE_MAX_VDEC_CHN_CNT 32
typedef struct sample_vdec_sendparam
{
    pthread_t Pid;
    HI_BOOL bRun;
    VDEC_CHN VdChn;    
    PAYLOAD_TYPE_E enPayload;
	HI_S32 s32MinBufSize;
    VIDEO_MODE_E enVideoMode;
}SAMPLE_VDEC_SENDPARAM_S;
#endif

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
HI_U32 gs_u32ViFrmRate = 0;
//SAMPLE_VDEC_SENDPARAM_S gs_SendParam[SAMPLE_MAX_VDEC_CHN_CNT];
pthread_t   gs_SendParam[VDEC_MAX_CHN_NUM]; 
VdecThreadParam gs_stVdecSend[VDEC_MAX_CHN_NUM];    

HI_S32 gs_s32VdecCnt;
/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_CAS_Master_Usage(char *sPrgNm)
{
    printf("\n\n/************************************/\n");
    printf("please choose the case which you want to run:\n");
    printf("\t0:  Cascade master chip,VI:cascade input; VDEC->VPSS->VO; VO:hdmi&VGA output.\n");
    printf("\tq:  quit the whole sample\n");
    printf("sample command:");
    
    return;
}

/******************************************************************************
* function : to process abnormal case                                         
******************************************************************************/
void SAMPLE_CAS_Master_HandleSig(HI_S32 signo)
{
    HI_S32 i;
    
    if (SIGINT == signo || SIGTSTP == signo)
    {
        for (i=0; i<gs_s32VdecCnt; i++)
        {           
            //pstVdecSend[i].eCtrlSinal=VDEC_CTRL_STOP;	

            HI_MPI_VDEC_StopRecvStream(i);
            gs_stVdecSend[i].eCtrlSinal=VDEC_CTRL_STOP;	
            if(gs_SendParam[i] != 0)
            {
                pthread_join(gs_SendParam[i], HI_NULL);
            }    
        }          

        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

HI_S32 SAMPLE_COMM_VI_Start_Cascade()
{
    
    VI_CHN ViChn = 32;
    VI_VBI_ARG_S stVbiArg; 

    stVbiArg.stVbiAttr[0].enLocal = VI_VBI_LOCAL_ODD_END;
    stVbiArg.stVbiAttr[0].s32X = 0x114;
    stVbiArg.stVbiAttr[0].s32Y = 0x28;
    stVbiArg.stVbiAttr[0].u32Len = 0x20;

    stVbiArg.stVbiAttr[1].enLocal = VI_VBI_LOCAL_ODD_END;
    stVbiArg.stVbiAttr[1].s32X = 0x114;
    stVbiArg.stVbiAttr[1].s32Y = 0x28;
    stVbiArg.stVbiAttr[1].u32Len = 0x20;

    CHECK_RET(HI_MPI_VI_SetVbiAttr(ViChn, &stVbiArg),"HI_MPI_VI_SetVbiAttr");

    CHECK_RET(HI_MPI_VI_EnableVbi(ViChn),"HI_MPI_VI_EnableVbi");   
    {
        CHECK_RET(HI_MPI_VI_EnableCascadeChn(VI_CAS_CHN_1),"HI_MPI_VI_EnableCascadeChn");
    }
    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_VI_Stop_Cascade(VI_CHN ViChn)
{
    HI_S32 s32Ret;
        
    s32Ret = HI_MPI_VI_DisableCascadeChn(VI_CAS_CHN_1);
    if(s32Ret != HI_SUCCESS)
    {
        return s32Ret;
    }
    s32Ret = HI_MPI_VI_DisableVbi(ViChn);
    
    if(s32Ret != HI_SUCCESS)
    {
        return s32Ret;
    }
    return HI_SUCCESS;
}

/******************************************************************************
* function : cascade process
*            vo is hd : vi cas chn -> vo
*                       vdec -> vpss -> vo
******************************************************************************/
HI_S32 SAMPLE_CAS_Master_SingleProcess(PIC_SIZE_E enVdecPicSize, HI_S32 s32VdecCnt,
    VO_DEV VoDev, SAMPLE_VO_MODE_E enVoMode)
{
    VDEC_CHN VdChn;
    HI_S32 s32Ret = HI_FAILURE;
    SIZE_S stSize;
    VB_CONF_S stVbConf,stModVbConf;
    HI_S32 i;
    VPSS_GRP VpssGrp;

    VI_DEV ViDev;
    VI_CHN ViChn;
    SIZE_S stDestSize;
    RECT_S stCapRect;

    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_ZOOM_ATTR_S stZoomAttr;
    VO_CHN_ATTR_S stChnAttr;
    HI_U32 u32WndNum, u32BlkSize, u32GrpNum;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    //VdecThreadParam stVdecSend[VDEC_MAX_CHN_NUM];    
    VDEC_CHN_ATTR_S stVdecChnAttr[VDEC_MAX_CHN_NUM];
 
    /******************************************
     step 1: init varaible.
    ******************************************/
    switch (enVoMode)
    {
        case VO_MODE_1MUX:
            u32WndNum = 1;
            break;
        case VO_MODE_4MUX:
            u32WndNum = 4;
            break;
        case VO_MODE_9MUX:
            u32WndNum = 9;
            break;
        case VO_MODE_16MUX:
            u32WndNum = 16;
            break;
        default:
            SAMPLE_PRT("failed with %#x!\n", s32Ret);
            return HI_FAILURE;
    }

    u32GrpNum = s32VdecCnt;
    
    if (s32VdecCnt > SAMPLE_MAX_VDEC_CHN_CNT)
    {
        SAMPLE_PRT("Vdec count is bigger than sample define!\n");
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enVdecPicSize, &stSize);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return HI_FAILURE;
    }
    
    /******************************************
     step 2: mpp system init.
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));    
    stVbConf.u32MaxPoolCnt = VB_MAX_POOLS;    

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                PIC_HD1080, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH, COMPRESS_MODE_NONE);
    /*ddr0 video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 20;
   
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("mpp init failed!\n");
        return HI_FAILURE;
    } 

    stSize.u32Width = HD_WIDTH;
    stSize.u32Height = HD_HEIGHT;

    /************************************************
    step3:  init mod common VB
    *************************************************/
    SAMPLE_COMM_VDEC_ModCommPoolConf(&stModVbConf, PT_H264, &stSize, s32VdecCnt, HI_FALSE); 
    s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stModVbConf);    
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VDEC_InitModCommVb failed with %d!\n", s32Ret);
        goto END_0;
    }   
    /************************************************
    step4:  start VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ChnAttr(s32VdecCnt, &stVdecChnAttr[0], PT_H264, &stSize);
    s32Ret = SAMPLE_COMM_VDEC_Start(s32VdecCnt, &stVdecChnAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {	
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END_1;
    }  
    
    /******************************************
     step 4: start vpss for vdec.
    ******************************************/    
    s32Ret = SAMPLE_COMM_VPSS_Start(u32GrpNum, &stSize, VPSS_MAX_CHN_NUM/2,NULL);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("vpss start failed!\n");
        goto END_2;
    }

    /******************************************
     step 5: start vo
    ******************************************/    
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30;    
    stVoPubAttr.enIntfType = VO_INTF_HDMI|VO_INTF_VGA;
    stVoPubAttr.u32BgColor = 0x000000ff;    
    
    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_2;
    }

    stLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
    stLayerAttr.u32DispFrmRt = 30;
    stLayerAttr.stDispRect.s32X       = 0;
    stLayerAttr.stDispRect.s32Y       = 0;
    stLayerAttr.stDispRect.u32Width   = 1920;
    stLayerAttr.stDispRect.u32Height  = 1080;
    stLayerAttr.stImageSize.u32Width  = 1920;
    stLayerAttr.stImageSize.u32Height = 1080;
    stLayerAttr.bDoubleFrame = HI_FALSE;
    stLayerAttr.bClusterMode = HI_FALSE;

    s32Ret = SAMPLE_COMM_VO_StartLayer(VoDev, &stLayerAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_3;
    }
    
    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev,  enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartChn failed!\n");
        goto END_4;
    }

    /* if it's displayed on HDMI, we should start HDMI */
    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
        {
            SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
            goto END_5;
        }
    }    

    for(i=0; i<s32VdecCnt; i++)
    {
        VoChn = (u32WndNum - s32VdecCnt) + i;
        
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev, VoChn, VpssGrp, VPSS_CHN0);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
            goto END_6;
        }
    }

    /******************************************
     step 6: start vdec & bind it to vpss
    ******************************************/
    for (i=0; i<s32VdecCnt; i++)
    {
        /*** create vdec chn ***/
        VdChn = i;
       
        /*** bind vdec to vpss ***/        
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VDEC_BindVpss(VdChn, VpssGrp);
        if (HI_SUCCESS !=s32Ret)
        {
            SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
            goto END_7;
        }
    }

    /******************************************
     step 7: open file & video decoder
     ******************************************/
    SAMPLE_COMM_VDEC_ThreadParam(s32VdecCnt, &gs_stVdecSend[0], &stVdecChnAttr[0], SAMPLE_1080P_H264_PATH); 
    SAMPLE_COMM_VDEC_StartSendStream(s32VdecCnt, &gs_stVdecSend[0], &gs_SendParam[0]);

    /******************************************
     step 8: start vi cascade
    ******************************************/
    ViDev = 8;
    ViChn = 32;
    s32Ret = SAMPLE_COMM_VI_Mode2Size(SAMPLE_VI_MODE_16_1080P, gs_enNorm, &stCapRect, &stDestSize);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("vi get size failed!\n");
        goto END_7;
    }
    
    s32Ret = SAMPLE_COMM_VI_StartDev(ViDev, SAMPLE_VI_MODE_16_1080P);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartDev failed with %#x\n", s32Ret);
        goto END_7;
    }

    s32Ret = SAMPLE_COMM_VI_StartChn(ViChn, &stCapRect, &stDestSize, SAMPLE_VI_MODE_16_1080P, VI_CHN_SET_NORMAL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("call SAMPLE_COMM_VI_StarChn failed with %#x\n", s32Ret);
        goto END_8;
    }

    s32Ret = SAMPLE_COMM_VI_Start_Cascade();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("call HI_MPI_VI_EnableCascadeChn failed with %#x\n", s32Ret);
        goto END_8;
    }

    /******************************************
     step 9: bind vi&vo ,set vo zoom in
    ******************************************/    
    stZoomAttr.enZoomType = VOU_ZOOM_IN_RECT;
    for (i=0; i<u32WndNum - s32VdecCnt; i++)
    {
        /*** bind vi to vo ***/
        VoChn = i;
        s32Ret = SAMPLE_COMM_VI_BindVo(VI_CAS_CHN_1,VoDev,VoChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("call HI_MPI_VO_SetZoomInWindow failed with %#x\n", s32Ret);
            goto END_9;
        }
        
        HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stChnAttr);
        memcpy(&stZoomAttr.stZoomRect, &stChnAttr.stRect, sizeof(RECT_S));
        stZoomAttr.stZoomRect.s32X = 0;
        stZoomAttr.stZoomRect.s32Y = 0;
        s32Ret = HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stZoomAttr);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("call HI_MPI_VO_SetZoomInWindow failed with %#x\n", s32Ret);
            goto END_10;
        }
    }
    
    printf("press two enter to quit!\n");
    getchar();
    getchar();
    /******************************************
     step 10: unbind vpss&vo, vi&vpss
    ******************************************/
END_10:
    for (i=0; i<u32WndNum - s32VdecCnt; i++)
    {
        /*** unbind vi to vpss ***/
        VpssGrp = i+s32VdecCnt;
        s32Ret = SAMPLE_COMM_VI_UnBindVo(VI_CAS_CHN_1, VoDev, i);
    }
    /******************************************
     step 11: stop vi cascade & vi 
    ******************************************/
END_9:   
    s32Ret = SAMPLE_COMM_VI_Stop_Cascade(ViChn);
END_8: 
    HI_MPI_VI_DisableChn(ViChn);
    HI_MPI_VI_DisableDev(ViDev);
    
    /******************************************
     step 12: join thread
    ******************************************/
    SAMPLE_COMM_VDEC_StopSendStream(s32VdecCnt, &gs_stVdecSend[0], &gs_SendParam[0]);

    /******************************************
     step 13: Unbind vdec to vpss & destroy vdec-chn
    ******************************************/
END_7:
    for (i=0; i<s32VdecCnt; i++)
    {
        VdChn = i;
        VpssGrp = i;
        SAMPLE_COMM_VDEC_UnBindVpss(VdChn, VpssGrp);
    }
    
    /******************************************
     step 14: Unbind vpss&vo, stop vo
    ******************************************/
END_6:
    for(i=0; i<s32VdecCnt; i++)
    {
        VoChn = (u32WndNum - s32VdecCnt) + i;        
        VpssGrp = i;
        SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,VPSS_CHN0);
    }
END_5:
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
END_4:   
    SAMPLE_COMM_VO_StopLayer(VoDev);
END_3:    
  
    SAMPLE_COMM_VO_StopDev(VoDev);    
    
    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        SAMPLE_COMM_VO_HdmiStop();
    }
END_2: 
    /******************************************
    step 15: stop vpss
    ******************************************/
    SAMPLE_COMM_VPSS_Stop(u32GrpNum, VPSS_MAX_CHN_NUM/2);
    
END_1: 

    /******************************************
    step 16:  stop vdec
    ******************************************/

    SAMPLE_COMM_VDEC_Stop(s32VdecCnt);
    /******************************************
     step 17: exit mpp system
    ******************************************/
END_0:
    SAMPLE_COMM_SYS_Exit();

    return HI_SUCCESS;
}

/****************************************************************************
* function: main
****************************************************************************/
int main(int argc, char* argv[])
{
    HI_CHAR ch;
    HI_BOOL bExit = HI_FALSE;
    
    signal(SIGINT, SAMPLE_CAS_Master_HandleSig);
    signal(SIGTERM, SAMPLE_CAS_Master_HandleSig);

    while (1)
    {
        SAMPLE_CAS_Master_Usage(argv[0]);
        ch = getchar();
        if (10 == ch)
        {
            continue;
        }
        getchar();
        switch (ch)
        {
            case '0':
            {
                gs_s32VdecCnt = 2;
                gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
                SAMPLE_CAS_Master_SingleProcess(PIC_D1, gs_s32VdecCnt, SAMPLE_VO_DEV_DHD0, VO_MODE_4MUX);
                break;
            }
            case 'q':
            case 'Q':
            {
                bExit = HI_TRUE;
                break;
            }
            default :
            {
                printf("the index is invaild!\n");
                SAMPLE_CAS_Master_Usage(argv[0]);
                break;
            }
        }

        if (bExit)
        {
            break;
        }
    }

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */



