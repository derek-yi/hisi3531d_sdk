#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>

#include "sample_comm.h"
#include "sample_comm_ivs.h"
#include "sample_ive_main.h"

#define VPSS_CHN_NUM 2

typedef struct hiSAMPLE_IVE_OD_S
{
	IVE_SRC_IMAGE_S stSrc;
	IVE_DST_IMAGE_S stInteg;	
	IVE_INTEG_CTRL_S stIntegCtrl;
	HI_U32 u32W;
	HI_U32 u32H;
}SAMPLE_IVE_OD_S;

typedef struct hiSAMPLE_IVE_OD_THREAD_PARAM_S
{	
	HI_BOOL bFdStop;
	HI_U32  u32FrmCnt;
	
	HI_U32 u32VpssGrpCnt;
	HI_U32 u32VpssChnNum;
	VPSS_CHN aVpssChn[VPSS_MAX_CHN_NUM];

	VO_LAYER VoLayer;
	VO_CHN   VoChn;
    VO_DEV   VoDev;
	
	SAMPLE_IVE_OD_S *pstOd;
}SAMPLE_IVE_OD_THREAD_PARAM_S;

static pthread_t s_IvsThread = 0;
static pthread_t s_VdecThread = 0;
static HI_U32 s_u32VdecChnCnt = 1;
static SAMPLE_IVE_OD_S s_stOd;
static SAMPLE_IVE_OD_THREAD_PARAM_S s_stOdThreadParam;
static VdecThreadParam s_stVdecSendParam;
static SAMPLE_VO_MODE_E s_enSampleVoMode = VO_MODE_1MUX;

static HI_VOID SAMPLE_IVE_OD_Uninit(SAMPLE_IVE_OD_S *pstOd)
{    
    if (NULL == pstOd)
    {
        return;
    }
    
    IVE_MMZ_FREE(pstOd->stInteg.u32PhyAddr[0],pstOd->stInteg.pu8VirAddr[0]);
   	memset(pstOd,0,sizeof(SAMPLE_IVE_OD_S));

    return;
}

static HI_S32 SAMPLE_IVE_OD_Init(SAMPLE_IVE_OD_S *pstOd,HI_U16 u16Width,HI_U16 u16Height)
{
    HI_S32 s32Ret = HI_SUCCESS;
    
   	memset(pstOd,0,sizeof(SAMPLE_IVE_OD_S));
	
	s32Ret = SAMPLE_COMM_IVE_CreateImage(&pstOd->stInteg,IVE_IMAGE_TYPE_U64C1,u16Width,u16Height);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, DETECT_INIT_FAIL, "SAMPLE_COMM_IVE_CreateImage fail\n");
    
    pstOd->stIntegCtrl.enOutCtrl = IVE_INTEG_OUT_CTRL_COMBINE;

    pstOd->u32W = u16Width/IVE_CHAR_CALW;
    pstOd->u32H = u16Height/IVE_CHAR_CALH;


DETECT_INIT_FAIL:    
    if(HI_SUCCESS != s32Ret)
	{
        SAMPLE_IVE_OD_Uninit(pstOd);
	}
	return s32Ret;    
    
}

static HI_S32 SAMPLE_IVE_OD_Linear2DClassifer(POINT_S *pstChar, HI_S32 s32CharNum, 
                                                        POINT_S *pstLinearPoint, HI_S32 s32Linearnum)
{
	HI_S32 s32ResultNum;
	HI_S32 i,j;
	HI_BOOL bTestFlag;
	POINT_S *pstNextLinearPoint;
	
	s32ResultNum = 0;
	pstNextLinearPoint = &pstLinearPoint[1];	
	for(i = 0;i < s32CharNum;i++)
	{
		bTestFlag = HI_TRUE;
		for(j = 0;j < (s32Linearnum-1);j++)
		{  
			if( ( (pstChar[i].s32Y - pstLinearPoint[j].s32Y) * (pstNextLinearPoint[j].s32X - pstLinearPoint[j].s32X)>
				  (pstChar[i].s32X - pstLinearPoint[j].s32X) * (pstNextLinearPoint[j].s32Y - pstLinearPoint[j].s32Y) 
				   && (pstNextLinearPoint[j].s32X != pstLinearPoint[j].s32X))
			   || ( (pstChar[i].s32X>pstLinearPoint[j].s32X) && (pstNextLinearPoint[j].s32X == pstLinearPoint[j].s32X) ))	
			{
				bTestFlag = HI_FALSE;
				break;
			}          
           
		}
		if(HI_TRUE == bTestFlag)
		{
			s32ResultNum++;
		}
	}
	return s32ResultNum;
}


HI_S32 SAMPLE_IVE_OD_Detect(VIDEO_FRAME_INFO_S *pstFrameInfo, SAMPLE_IVE_OD_S * pstOd, HI_BOOL *pbOcclu)
{
    HI_S32 s32Ret = HI_SUCCESS;
	
	HI_U32 i,j;
	
    IVE_HANDLE IveHandle;
    HI_BOOL bFinish = HI_FALSE;
    HI_BOOL bBlock = HI_TRUE;
    HI_BOOL bInstant = HI_TRUE;
	 
    POINT_S stChar[IVE_CHAR_NUM];
    POINT_S astPoints[10] = {{0,0}};
	IVE_LINEAR_DATA_S stIveLinerData;

	IVE_IMAGE_S *pstSrc		= HI_NULL;
	IVE_IMAGE_S *pstInteg 	= HI_NULL;
	IVE_INTEG_CTRL_S *pstIntegCtrl = HI_NULL;
    HI_U64 *pu64VirData = HI_NULL;

	HI_U64 u64TopLeft, u64TopRight, u64BtmLeft, u64BtmRight;
	HI_U64 *pu64TopRow, *pu64BtmRow;
	HI_U64 u64BlockSum,u64BlockSq;
	HI_FLOAT f32SqVar;

    if((pstFrameInfo->stVFrame.u32Width!=pstOd->stInteg.u16Width) || (pstFrameInfo->stVFrame.u32Height!=pstOd->stInteg.u16Height))
        return HI_FAILURE;
	
	pstSrc = &pstOd->stSrc;
	pstInteg = &pstOd->stInteg;
	pstIntegCtrl = &pstOd->stIntegCtrl;
	
    stIveLinerData.pstLinearPoint = &astPoints[0];
    stIveLinerData.s32LinearNum = 2;
	stIveLinerData.s32ThreshNum = IVE_CHAR_NUM/2;
	stIveLinerData.pstLinearPoint[0].s32X = 80;
	stIveLinerData.pstLinearPoint[0].s32Y = 0;
	stIveLinerData.pstLinearPoint[1].s32X = 80;
	stIveLinerData.pstLinearPoint[1].s32Y = 20;

	pstSrc->enType 	      = IVE_IMAGE_TYPE_U8C1;
	pstSrc->u16Width   	  = (HI_U16)pstFrameInfo->stVFrame.u32Width;
	pstSrc->u16Height     = (HI_U16)pstFrameInfo->stVFrame.u32Height;
	pstSrc->u16Stride[0]  = (HI_U16)pstFrameInfo->stVFrame.u32Stride[0];
	pstSrc->pu8VirAddr[0] = (HI_U8*)pstFrameInfo->stVFrame.pVirAddr[0];
	pstSrc->u32PhyAddr[0] = pstFrameInfo->stVFrame.u32PhyAddr[0];

	
    s32Ret = HI_MPI_IVE_Integ(&IveHandle, pstSrc, pstInteg, pstIntegCtrl, bInstant);
	SAMPLE_CHECK_EXPR_RET(HI_SUCCESS != s32Ret, s32Ret, "HI_MPI_IVE_Integ fail,Error(%#x)\n",s32Ret);

    s32Ret = HI_MPI_IVE_Query(IveHandle, &bFinish, bBlock);		
	while(HI_ERR_IVE_QUERY_TIMEOUT == s32Ret)
	{
		usleep(100);					
		s32Ret = HI_MPI_IVE_Query(IveHandle,&bFinish,bBlock);	
	}
	SAMPLE_CHECK_EXPR_RET(HI_SUCCESS!=s32Ret,s32Ret,"HI_MPI_IVE_Query fail,Error(%#x)\n",s32Ret);
   
    pu64VirData = (HI_U64 *)pstInteg->pu8VirAddr[0];
    for(j = 0; j < IVE_CHAR_CALH; j++)
	{
        pu64TopRow = (0 == j) ? (pu64VirData) : ( pu64VirData + (j * pstOd->u32H -1) * pstOd->stInteg.u16Stride[0]);
        pu64BtmRow = pu64VirData + ((j + 1) * pstOd->u32H - 1) * pstOd->stInteg.u16Stride[0];
        
		for(i = 0;i < IVE_CHAR_CALW; i++)
		{      
            u64TopLeft  = (0 == j) ? (0) :((0 == i) ? (0) : (pu64TopRow[i * pstOd->u32W-1]));
            u64TopRight = (0 == j) ? (0) : (pu64TopRow[(i + 1) * pstOd->u32W - 1]);
            u64BtmLeft  = (0 == i) ? (0) : (pu64BtmRow[i * pstOd->u32W - 1]);
            u64BtmRight = pu64BtmRow[(i + 1) * pstOd->u32W -1];

            u64BlockSum = (u64TopLeft & 0xfffffffLL) + (u64BtmRight & 0xfffffffLL)
                        - (u64BtmLeft & 0xfffffffLL) - (u64TopRight & 0xfffffffLL);

            u64BlockSq  = (u64TopLeft >> 28) + (u64BtmRight >> 28)
                        - (u64BtmLeft >> 28) - (u64TopRight >> 28);

           // mean
			stChar[j * IVE_CHAR_CALW + i].s32X = u64BlockSum/(pstOd->u32W*pstOd->u32H);
           // sigma=sqrt(1/(w*h)*sum((x(i,j)-mean)^2)= sqrt(sum(x(i,j)^2)/(w*h)-mean^2)
           f32SqVar = u64BlockSq/(pstOd->u32W*pstOd->u32H) - stChar[j * IVE_CHAR_CALW + i].s32X * stChar[j * IVE_CHAR_CALW + i].s32X;
		   stChar[j * IVE_CHAR_CALW + i].s32Y = (HI_U32)sqrt(f32SqVar);
		}
	}

    s32Ret = SAMPLE_IVE_OD_Linear2DClassifer(&stChar[0],IVE_CHAR_NUM,
                stIveLinerData.pstLinearPoint,stIveLinerData.s32LinearNum);         
    if(s32Ret > stIveLinerData.s32ThreshNum)
    {
		*pbOcclu = HI_TRUE;
    }
    else
    {
		*pbOcclu = HI_FALSE;
    }       

     return HI_SUCCESS;
}

HI_VOID * SAMPLE_IVE_OD_Process(HI_VOID *pArgs)
{	
	HI_S32 s32Ret = HI_SUCCESS;
	SAMPLE_IVE_OD_THREAD_PARAM_S *pstThreadParam;

	HI_U32 i;
	HI_BOOL bOcclu = HI_FALSE;
	
	HI_U32 u32VpssGrpCnt;
	VPSS_CHN aVpssChn[VPSS_MAX_CHN_NUM];
	SAMPLE_IVE_OD_S *pstOd;

    prctl(PR_SET_NAME, "hi_OD_Process", 0, 0, 0);

	pstThreadParam = (SAMPLE_IVE_OD_THREAD_PARAM_S*)pArgs;
	u32VpssGrpCnt = pstThreadParam->u32VpssGrpCnt;
	aVpssChn[0]	  = pstThreadParam->aVpssChn[0];
	aVpssChn[1]	  = pstThreadParam->aVpssChn[1];
	pstOd	  	  = pstThreadParam->pstOd;
	
	while(HI_TRUE!=pstThreadParam->bFdStop)
	{				
		HI_S32 s32MilliSec = 2000;
		VIDEO_FRAME_INFO_S stSmallFrm;
		
		for(i=0; i<u32VpssGrpCnt; i++)
		{
			VPSS_GRP VpssGrp = i;
			
			s32Ret = HI_MPI_VPSS_GetChnFrame(VpssGrp, aVpssChn[1], &stSmallFrm, s32MilliSec);
			if(HI_SUCCESS!=s32Ret)
			{
				SAMPLE_PRT("HI_MPI_VPSS_GetChnFrame small frame failed, VPSS_GRP(%d), VPSS_CHN(%d), errno: %#x!\n",
					VpssGrp, aVpssChn[1], s32Ret);
				continue;
			}
			
			s32Ret = SAMPLE_IVE_OD_Detect(&stSmallFrm, pstOd, &bOcclu);
			SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, FAIL_OD, "SAMPLE_IVE_OD_Detect failed with errno: %#x!\n", s32Ret);

			pstThreadParam->u32FrmCnt++;
			if(bOcclu)
			{
				//printf("\033[0;31m%dth frame, occlusion detected!\033[0;39m\n", pstThreadParam->u32FrmCnt);
			}
			else
			{
				//printf("%dth frame, normal frame!\n",pstThreadParam->u32FrmCnt);
			}

		FAIL_OD:
			 //Release small frame
			 s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, aVpssChn[1], &stSmallFrm);
			 if (HI_SUCCESS != s32Ret)
			 {
				 SAMPLE_PRT("HI_MPI_VPSS_ReleaseChnFrame failed,VpssGrp(%d),VpssChn(%d),Error(%#x)\n",VpssGrp,aVpssChn[1],s32Ret);
			 }
	
		}
	
		usleep(50000);
	}
	
    return (HI_VOID *)HI_SUCCESS;
}


HI_VOID SAMPLE_IVE_OD_ThreadParam(SAMPLE_IVE_OD_THREAD_PARAM_S *pstThreadParam,
    HI_U32 u32VpssGrpCnt, HI_U32 u32VpssChnNum, VPSS_CHN aVpssChn[], 
	VO_DEV VoDev, VO_LAYER VoLayer, VO_CHN VoChn, SAMPLE_IVE_OD_S *pstOd)
{
	HI_S32 i;

	HI_ASSERT(HI_NULL!=pstThreadParam);
	HI_ASSERT(u32VpssChnNum<VPSS_MAX_CHN_NUM);
	HI_ASSERT(HI_NULL!=pstOd);

	memset(pstThreadParam, 0, sizeof(SAMPLE_IVE_OD_THREAD_PARAM_S));

	pstThreadParam->bFdStop   = HI_FALSE;
	pstThreadParam->u32FrmCnt = 0;
	
	pstThreadParam->u32VpssGrpCnt = u32VpssGrpCnt;
	pstThreadParam->u32VpssChnNum = u32VpssChnNum;
	for(i=0; i<u32VpssChnNum; i++)
		pstThreadParam->aVpssChn[i] = aVpssChn[i];

    pstThreadParam->VoDev = VoDev;
    pstThreadParam->VoLayer = VoLayer;
    pstThreadParam->VoChn = VoChn;
    
	pstThreadParam->pstOd = pstOd;
}

HI_VOID SAMPLE_IVE_OD_ThreadProc(SAMPLE_IVE_OD_THREAD_PARAM_S *pstThreadParam, 
											pthread_t *pIveThread)
{
	pthread_create(pIveThread, 0, SAMPLE_IVE_OD_Process, (HI_VOID *)pstThreadParam);
}

HI_VOID SAMPLE_IVE_OD_CmdCtrl(SAMPLE_IVE_OD_THREAD_PARAM_S *pstThreadParam, pthread_t IveThread)
{
    char c=0;

    printf("\nSAMPLE_TEST: press 'e' to exit \n\n"); 

    while(1)    
    {
        c = getchar();
        if (10 == c)
        {
            continue;
        }
        getchar();
        if (c == 'e' || c == 'E')
        {
			pstThreadParam->bFdStop = HI_TRUE;
			pthread_join(IveThread,HI_NULL);
            break;
        }
    }
}


/******************************************************************************
* function : show Occlusion detection sample
******************************************************************************/
HI_S32 SAMPLE_IVE_OD(HI_VOID)
{
    HI_S32 s32Ret = HI_SUCCESS, i;
	
    VB_CONF_S stVbConf,stModVbConf;
	SIZE_S astSize[VPSS_CHN_NUM] = {{HD_WIDTH, HD_HEIGHT}, {VGA_WIDTH, VGA_HEIGHT}};
    PAYLOAD_TYPE_E enStreamType = PT_H264;
	
    VDEC_CHN_ATTR_S stVdecChnAttr;
	
    VPSS_CHN aVpssChn[VPSS_CHN_NUM] = {VPSS_CHN0, VPSS_CHN3};
    VPSS_GRP_ATTR_S stVpssGrpAttr;
	HI_U32 u32VpssGrpCnt = 1;	
		
    VO_DEV   VoDev	 = SAMPLE_VO_DEV_DHD0;
	VO_CHN   VoChn   = 0;
    VO_LAYER VoLayer = SAMPLE_VO_LAYER_VHD0;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr;

	HI_CHAR *pchStreamName = "./data/input/od/od_1080p_01.h264";

    memset(&s_stOdThreadParam, 0, sizeof(s_stOdThreadParam));
    memset(&s_stVdecSendParam, 0, sizeof(s_stVdecSendParam));
    /******************************************
     step 1: Init SYS and common VB
    ******************************************/
	SAMPLE_COMM_IVS_SysConf(&stVbConf, astSize, VPSS_CHN_NUM);
	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_SYS_VB, "init SYS and common VB failed!\n");
	
    /******************************************
     step 2: Init VDEC mod VB. 
    *****************************************/    	
    SAMPLE_COMM_VDEC_ModCommPoolConf(&stModVbConf, enStreamType, &astSize[0],
        s_u32VdecChnCnt, HI_FALSE);	
    s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stModVbConf);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_SYS_VB, "init VDEC mod VB failed!\n");
	
    /******************************************
     step 3: Init OD
    ******************************************/
    s32Ret = SAMPLE_IVE_OD_Init(&s_stOd, astSize[1].u32Width,astSize[1].u32Height);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_OD_INIT, "SAMPLE_IVE_OD_Init failed\n");
	
    /******************************************
     step 4:  start VDEC.
    *****************************************/
    SAMPLE_COMM_VDEC_ChnAttr(s_u32VdecChnCnt, &stVdecChnAttr, enStreamType, &astSize[0]);
	s32Ret =SAMPLE_COMM_VDEC_Start(s_u32VdecChnCnt, &stVdecChnAttr);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret,END_VDEC_START, "start VDEC failed with!\n");
    for(i=0;i<s_u32VdecChnCnt;i++)
	{
		CHECK_CHN_RET(HI_MPI_VDEC_SetDisplayMode(i, VIDEO_DISPLAY_MODE_PREVIEW), i, "HI_MPI_VDEC_SetDisplayMode");
	}


    /******************************************
     step 5:  start VPSS and bind to VDEC.
    *****************************************/
    SAMPLE_COMM_IVS_VpssGrpAttr(u32VpssGrpCnt, &stVpssGrpAttr, &astSize[0]);
    s32Ret = SAMPLE_COMM_IVS_VpssStart2(u32VpssGrpCnt, astSize, aVpssChn, VPSS_CHN_NUM, &stVpssGrpAttr);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VPSS_START, "start VPSS failed!\n");
	
    for(i=0;i<u32VpssGrpCnt;i++)
    {	
        s32Ret = SAMPLE_COMM_VDEC_BindVpss(i,i);
		SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VPSS_BIND_VDEC, "SAMPLE_COMM_VDEC_BindVpss failed!\n");
    }
	
    /************************************************
    step 6:  start VO and bind to VPSS
    *************************************************/
	stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30;
	stVoPubAttr.enIntfType = VO_INTF_HDMI | VO_INTF_VGA;
	stVoPubAttr.u32BgColor = 0x0000FF;
	
	stVoLayerAttr.bClusterMode = HI_FALSE;
	stVoLayerAttr.bDoubleFrame = HI_FALSE;
	stVoLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
	
	s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync, \
		&stVoLayerAttr.stDispRect.u32Width, &stVoLayerAttr.stDispRect.u32Height, &stVoLayerAttr.u32DispFrmRt);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VPSS_BIND_VDEC, "SAMPLE_COMM_VO_GetWH failed!\n");
	
	stVoLayerAttr.stDispRect.s32X = 0;
	stVoLayerAttr.stDispRect.s32Y = 0;
	stVoLayerAttr.stImageSize.u32Width = stVoLayerAttr.stDispRect.u32Width;
	stVoLayerAttr.stImageSize.u32Height = stVoLayerAttr.stDispRect.u32Height;

	s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VO_START, "SAMPLE_COMM_VO_StartDev failed!\n");
	
	s32Ret = SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_HDMI_START, "SAMPLE_COMM_VO_HdmiStart failed!\n");
	
	s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stVoLayerAttr);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_LAYER_START, "SAMPLE_COMM_VO_StartLayer failed!\n");
	
	s32Ret = SAMPLE_COMM_VO_StartChn(VoLayer, s_enSampleVoMode);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VO_CHN, "SAMPLE_COMM_VO_StartChn failed!\n");

    for(i=0;i<u32VpssGrpCnt;i++)
    {	
		s32Ret = SAMPLE_COMM_VO_BindVpss(VoLayer, VoChn, i, aVpssChn[0]);
		SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VO_BIND_VPSS, "SAMPLE_COMM_VO_BindVpss failed!\n");
    }

    /******************************************
    step 7: VDEC start send stream. 
    ******************************************/
    SAMPLE_COMM_VDEC_ThreadParam(s_u32VdecChnCnt, &s_stVdecSendParam, &stVdecChnAttr, pchStreamName);
    SAMPLE_COMM_VDEC_StartSendStream(s_u32VdecChnCnt, &s_stVdecSendParam, &s_VdecThread);
	
    /******************************************
    step 8: OD process. 
    ******************************************/
    SAMPLE_IVE_OD_ThreadParam(&s_stOdThreadParam, u32VpssGrpCnt, VPSS_CHN_NUM, aVpssChn, 
                                VoDev, VoLayer, VoChn, &s_stOd);
    SAMPLE_IVE_OD_ThreadProc(&s_stOdThreadParam, &s_IvsThread);
   
    /******************************************
    step 9: exit process
    ******************************************/
	SAMPLE_IVE_OD_CmdCtrl(&s_stOdThreadParam, s_IvsThread);
    SAMPLE_COMM_VDEC_StopSendStream(s_u32VdecChnCnt,&s_stVdecSendParam,&s_VdecThread);
	
	
END_VO_BIND_VPSS:
	for(i=0; i<u32VpssGrpCnt; i++)
	{
		SAMPLE_COMM_VO_UnBindVpss(VoLayer,VoChn,i,aVpssChn[0]);
	}		

END_VO_CHN: 
	SAMPLE_COMM_VO_StopChn(VoLayer, s_enSampleVoMode);

END_LAYER_START: 
	SAMPLE_COMM_VO_StopLayer(VoLayer);

END_HDMI_START: 
	SAMPLE_COMM_VO_HdmiStop();

END_VO_START:
	SAMPLE_COMM_VO_StopDev(VoDev);

END_VPSS_BIND_VDEC:
    for(i = 0;i < s_u32VdecChnCnt; i++)
    {
        SAMPLE_COMM_VDEC_UnBindVpss(i,i);
    }
    
END_VPSS_START:
    SAMPLE_COMM_IVS_VpssStop(u32VpssGrpCnt,VPSS_MAX_CHN_NUM);	

END_VDEC_START:
    SAMPLE_COMM_VDEC_Stop(s_u32VdecChnCnt);
	
END_OD_INIT:
	SAMPLE_IVE_OD_Uninit(&s_stOd);
	
END_SYS_VB:
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}

/******************************************************************************
* function : Od sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_Od_HandleSig(HI_VOID)
{
    HI_U32 i;
    
	s_stOdThreadParam.bFdStop = HI_TRUE;
	if (0 != s_IvsThread)
	{
		pthread_join(s_IvsThread, HI_NULL);	
		s_IvsThread = 0;
	}
    SAMPLE_COMM_VDEC_StopSendStream(s_u32VdecChnCnt,&s_stVdecSendParam,&s_VdecThread);

	for(i = 0; i < s_stOdThreadParam.u32VpssGrpCnt; i++)
	{
		SAMPLE_COMM_VO_UnBindVpss(s_stOdThreadParam.VoLayer,s_stOdThreadParam.VoChn,i,s_stOdThreadParam.aVpssChn[0]);
	}		

	SAMPLE_COMM_VO_StopChn(s_stOdThreadParam.VoLayer, s_enSampleVoMode);
	SAMPLE_COMM_VO_StopLayer(s_stOdThreadParam.VoLayer);
	SAMPLE_COMM_VO_HdmiStop();
	SAMPLE_COMM_VO_StopDev(s_stOdThreadParam.VoDev);
    for(i = 0;i < s_u32VdecChnCnt; i++)
    {
        SAMPLE_COMM_VDEC_UnBindVpss(i,i);
    }
    
    SAMPLE_COMM_IVS_VpssStop(s_stOdThreadParam.u32VpssGrpCnt,VPSS_MAX_CHN_NUM);	
    SAMPLE_COMM_VDEC_Stop(s_u32VdecChnCnt);
	SAMPLE_IVE_OD_Uninit(&s_stOd);
    SAMPLE_COMM_SYS_Exit();
}

