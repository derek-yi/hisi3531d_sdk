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

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
HI_U32 gs_u32ViFrmRate = 0;

//SAMPLE_VDEC_SENDPARAM_S gs_SendParam[SAMPLE_MAX_VDEC_CHN_CNT];

pthread_t   gs_SendThread[VDEC_MAX_CHN_NUM];
HI_S32 gs_s32VdecCnt = 0;

VdecThreadParam gs_stVdecSend[VDEC_MAX_CHN_NUM];    

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_CAS_Slave_Usage(char *sPrgNm)
{
    printf("\n\n/************************************/\n");
    printf("please choose the case which you want to run:\n");
    printf("\t0:  Cascade slave chip's master mode,VDEC:1*1080P; VO:single cascade output.\n");
    printf("\t1:  Cascade slave chip's slave mode,VDEC:1*1080P; VO:single cascade output.\n");
    printf("\tq:  quit the whole sample\n");
    printf("sample command:");
			
    return;
}

/******************************************************************************
* function : to process abnormal case                                         
******************************************************************************/
void SAMPLE_CAS_Slave_HandleSig(HI_S32 signo)
{
    int i;
    if (SIGINT == signo || SIGTSTP == signo)
    {
        
        for (i=0; i<gs_s32VdecCnt; i++)
        {    
            HI_MPI_VDEC_StopRecvStream(i);
            gs_stVdecSend[i].eCtrlSinal=VDEC_CTRL_STOP;	
            if(gs_SendThread[i] != 0)
            {
                pthread_join(gs_SendThread[i], HI_NULL);
            }
        }          
        
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

/******************************************************************************
* function : cascade process  VDEC->VPSS->VO cascade output
*          
******************************************************************************/
HI_S32 SAMPLE_CAS_Slave_SingleProcess(HI_S32 s32StartChn,
    HI_S32 s32EndChn, SAMPLE_VO_MODE_E enVoMode, HI_BOOL bSlave)
{
    VDEC_CHN VdChn;
    HI_S32 s32Ret = HI_FAILURE , i;
    VB_CONF_S stVbConf,stModVbConf;
    //VdecThreadParam stVdecSend[VDEC_MAX_CHN_NUM];    
    SIZE_S stSize;
    VPSS_GRP VpssGrp;
    VDEC_CHN_ATTR_S stVdecChnAttr[VDEC_MAX_CHN_NUM];
    
    VO_CHN VoChn;
    VO_LAYER VoCasLayer = VO_CAS_DEV_1+1;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_CHN_ATTR_S stChnAttr;
    VO_CAS_ATTR_S stCasAttr;
    HI_U32 u32WndNum, u32BlkSize, u32Pos, u32Pattern;   
    HI_U32 u32VdCnt ,u32GrpNum, u32VoSqChnNum;
    u32VdCnt = gs_s32VdecCnt;
    u32GrpNum = gs_s32VdecCnt;
 
    /******************************************
     step 1: init varaible.
    ******************************************/
    switch (enVoMode)
    {
        case VO_MODE_1MUX:
            u32WndNum = 1;
            u32Pattern = 1;
            u32VoSqChnNum = 1;
            break;
        case VO_MODE_4MUX:
            u32WndNum = 4;
            u32Pattern = 4;
            u32VoSqChnNum = 2;
            break;
        case VO_MODE_9MUX:
            u32WndNum = 9;
            u32Pattern = 9;
            u32VoSqChnNum = 3;
            break;
        case VO_MODE_16MUX:
            u32WndNum = 16;
            u32Pattern = 16;
            u32VoSqChnNum = 4;
            break;
        default:
            SAMPLE_PRT("failed with %#x!\n", s32Ret);
            return HI_FAILURE;
    }
    
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL== gs_enNorm)?25:30;
        
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

    /************************************************
    step2:  init mod common VB
    *************************************************/
    stSize.u32Width = HD_WIDTH;
    stSize.u32Height = HD_HEIGHT;
    SAMPLE_COMM_VDEC_ModCommPoolConf(&stModVbConf, PT_H264, &stSize, u32VdCnt, HI_FALSE); 
    s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stModVbConf);    
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VDEC_InitModCommVb failed with %d!\n", s32Ret);
        goto END_0;
    }   
    /************************************************
    step3:  start VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ChnAttr(u32VdCnt, &stVdecChnAttr[0], PT_H264, &stSize);
    s32Ret = SAMPLE_COMM_VDEC_Start(u32VdCnt, &stVdecChnAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {	
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END_0;
    }  
    /******************************************
     step 4: start vpss, and bind vdec to vpss.
    ******************************************/
    s32Ret = SAMPLE_COMM_VPSS_Start(u32GrpNum, &stSize, VPSS_MAX_CHN_NUM/2, NULL);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("vpss start failed!\n");
        goto END_1;
    }
    /******************************************
     step 5: start vdec & bind  to vpss
    ******************************************/
    for (i=0; i<u32VdCnt; i++)
    {       
        VdChn = i;       
        /*** bind vdec to vpss ***/        
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VDEC_BindVpss(VdChn, VpssGrp);        
        if (HI_SUCCESS !=s32Ret)
        {
            SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
            goto END_2;
        }
    }

    /******************************************
     step 6: open file & video decoder
    ******************************************/
    SAMPLE_COMM_VDEC_ThreadParam(u32VdCnt, &gs_stVdecSend[0], &stVdecChnAttr[0], SAMPLE_1080P_H264_PATH); 
    SAMPLE_COMM_VDEC_StartSendStream(u32VdCnt, &gs_stVdecSend[0], &gs_SendThread[0]);   
 
    /******************************************
     step 7: start vo, enable cascade
    ******************************************/
    stCasAttr.bSlave = bSlave;
    stCasAttr.enCasMode = VO_CAS_MODE_SINGLE;
    stCasAttr.enCasRgn = VO_CAS_64_RGN;
    stCasAttr.enCasDataTranMode = VO_CAS_DATA_SINGLE_TRAN_MODE;

    /* set cascade attr */
    s32Ret = HI_MPI_VO_SetCascadeAttr(&stCasAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Set HI_MPI_VO_SetCascadeAttr failed!\n");
        goto END_3;
    }    
    
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30;    
    stVoPubAttr.enIntfType = VO_INTF_BT1120;
    stVoPubAttr.u32BgColor = 0x000000ff;
    
    /* start hd0 */
    s32Ret = HI_MPI_VO_SetPubAttr(SAMPLE_VO_DEV_DHD0, &stVoPubAttr);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto END_3;
    }

    s32Ret = HI_MPI_VO_Enable(SAMPLE_VO_DEV_DHD0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto END_3;
    }

    memset(&stLayerAttr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));
    stLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
    stLayerAttr.u32DispFrmRt = 30;
    stLayerAttr.stDispRect.s32X       = 0;
    stLayerAttr.stDispRect.s32Y       = 0;
    stLayerAttr.stDispRect.u32Width   = 1920;
    stLayerAttr.stDispRect.u32Height  = 1080;
    stLayerAttr.stImageSize.u32Width  = 1920;
    stLayerAttr.stImageSize.u32Height = 1080;
    s32Ret = HI_MPI_VO_SetVideoLayerAttr(SAMPLE_VO_DEV_DHD0, &stLayerAttr);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto END_4;
    }

    s32Ret = HI_MPI_VO_EnableVideoLayer(SAMPLE_VO_DEV_DHD0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto END_4;
    }

    /* start cascade dev */
    s32Ret = HI_MPI_VO_EnableCascadeDev(VO_CAS_DEV_1);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start VO_CAS_DEV_1 failed!\n");
        goto END_5;
    }

    /* start cascade dev's chn */
    for (i=0; i<u32WndNum; i++)
    {
        stChnAttr.stRect.s32X       = ALIGN_BACK((stLayerAttr.stImageSize.u32Width/u32VoSqChnNum) * (i%u32VoSqChnNum), 2);
        stChnAttr.stRect.s32Y       = ALIGN_BACK((stLayerAttr.stImageSize.u32Height/u32VoSqChnNum) * (i/u32VoSqChnNum), 2);
        stChnAttr.stRect.u32Width   = ALIGN_BACK(stLayerAttr.stImageSize.u32Width/u32VoSqChnNum, 2);
        stChnAttr.stRect.u32Height  = ALIGN_BACK(stLayerAttr.stImageSize.u32Height/u32VoSqChnNum, 2);
        stChnAttr.u32Priority       = 0;
        stChnAttr.bDeflicker        = HI_FALSE;

        s32Ret = HI_MPI_VO_SetChnAttr(VoCasLayer, i, &stChnAttr);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start SAMPLE_COMM_VO_StartChn failed!\n");
            goto END_5;
        }   
        s32Ret = HI_MPI_VO_EnableChn(VoCasLayer, i);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start SAMPLE_COMM_VO_StartChn failed!\n");
            goto END_5;
        }   
    }

    /* bind vpss to cascade dev's chn, bind cascade pos */
    for(i=0; i<u32WndNum; i++)
    {
        if (i<s32StartChn || i >s32EndChn)
        {
            HI_MPI_VO_DisableChn(VoCasLayer, i);
            continue;
        }
        VoChn = i;        
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoCasLayer,VoChn,VpssGrp-s32StartChn,VPSS_CHN0);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
            goto END_6;
        }

        u32Pos = i;
        s32Ret = HI_MPI_VO_CascadePosBindChn(u32Pos, VO_CAS_DEV_1, VoChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VO_CascadePosBindChn failed!\n");
            goto END_7;
        }
    }

    /* set pattern; start cascade output */
    u32Pattern  = 1;
    s32Ret = HI_MPI_VO_SetCascadePattern(VO_CAS_DEV_1, u32Pattern);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VO_SetCascadePattern failed!\n");
        goto END_7;
    }

    s32Ret = HI_MPI_VO_EnableCascade();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VO_EnableCascade failed!\n");
        goto END_7;
    }
    
    printf("press two enter to quit!\n");
    getchar();
    getchar();
    /******************************************************
     step 8: stop vo,disable cascade, unbind vo and vpss
    ******************************************************/

    /* stop cascade */
    HI_MPI_VO_DisableCascade();

END_7:
    /* unbind pos, unbind vo and vpss */
    for(i=0; i<u32WndNum; i++)
    {
        if (i<s32StartChn || i >s32EndChn)
        {
            continue;
        }
        VoChn = i;        
        VpssGrp = i;
        u32Pos = i;
        HI_MPI_VO_CascadePosUnBindChn(u32Pos, VO_CAS_DEV_1, VoChn);
        /* unbind vo and vpss */
        SAMPLE_COMM_VO_UnBindVpss(VoCasLayer, VoChn, VpssGrp-s32StartChn, VPSS_CHN0);
    }

END_6: 
    /* stop cascade layer's chn */
    SAMPLE_COMM_VO_StopChn(VoCasLayer, enVoMode);
    /* stop cascade dev */
    HI_MPI_VO_DisableCascadeDev(VO_CAS_DEV_1);
END_5:
    /* disable hd0 */
    SAMPLE_COMM_VO_StopLayer(SAMPLE_VO_DEV_DHD0);
END_4: 
    SAMPLE_COMM_VO_StopDev(SAMPLE_VO_DEV_DHD0);
    /******************************************
     step 9:  stop send stream
    ******************************************/
END_3:
    SAMPLE_COMM_VDEC_StopSendStream(u32VdCnt, &gs_stVdecSend[0], &gs_SendThread[0]);
    /******************************************
     step 10: unbind vdec and vpss
    ******************************************/
     /* unbind vdec and vpss */
    for (i=0; i<u32VdCnt; i++)
    {
        VdChn = i;       
        /*** unbind vdec to vpss ***/        
        VpssGrp = i;        
        SAMPLE_COMM_VDEC_UnBindVpss(VdChn, VpssGrp);
    }
END_2:
    /******************************************
    step 11: stop vpss
    ******************************************/   
    SAMPLE_COMM_VPSS_Stop(u32GrpNum, VPSS_MAX_CHN_NUM);    
END_1:
    /******************************************
    step 12: stop vdec
    ******************************************/   
    SAMPLE_COMM_VDEC_Stop(u32VdCnt);
END_0:
    /******************************************
     step 13: exit mpp system
    ******************************************/
    SAMPLE_COMM_SYS_Exit();

    return HI_SUCCESS;
}

/****************************************************************************
* function: main
****************************************************************************/
int main(int argc, char* argv[])
{
    HI_S32 s32StartChn, s32EndChn;
    HI_CHAR ch;
    HI_BOOL bExit = HI_FALSE;
    
    signal(SIGINT, SAMPLE_CAS_Slave_HandleSig);
    signal(SIGTERM, SAMPLE_CAS_Slave_HandleSig);

    while (1)
    {
        SAMPLE_CAS_Slave_Usage(argv[0]);
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
                gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
                s32StartChn = 0;
                s32EndChn = 0;
                gs_s32VdecCnt = 1;
                SAMPLE_CAS_Slave_SingleProcess(s32StartChn, s32EndChn, VO_MODE_4MUX, HI_FALSE);
                break;
            }
            case '1':
            {
                gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
                s32StartChn = 1;
                s32EndChn = 1;
                gs_s32VdecCnt = 1;
                SAMPLE_CAS_Slave_SingleProcess(s32StartChn, s32EndChn, VO_MODE_4MUX, HI_TRUE);
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
                SAMPLE_CAS_Slave_Usage(argv[0]);
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




