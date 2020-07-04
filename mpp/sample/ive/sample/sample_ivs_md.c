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
#define SAMPLE_IVS_MD_IMG_NUM 2

typedef struct hiSAMPLE_IVS_MD_S
{
	IVE_SRC_IMAGE_S astImg[SAMPLE_IVS_MD_IMG_NUM];	
	IVE_DST_MEM_INFO_S stBlob;
	MD_ATTR_S stMdAttr;	
	
	SAMPLE_RECT_ARRAY_S stRegion;

	HI_S32 s32CurIdx;
}SAMPLE_IVS_MD_S;

typedef struct hiSAMPLE_IVS_MD_THREAD_PARAM_S
{	
	HI_BOOL bStop;
	HI_BOOL bEncode;
	HI_BOOL bVo;

	HI_BOOL bFirstFrm;
	HI_U32  u32FrmCnt;
	
	HI_U32 u32VpssGrpCnt;
	HI_U32 u32VpssChnNum;
	VPSS_CHN aVpssChn[VPSS_CHN_NUM];

	VENC_CHN VeH264Chn;	
	VO_LAYER VoLayer;
	VO_CHN   VoChn;
    VO_DEV   VoDev;
	
	SAMPLE_IVS_MD_S *pstMd;
}SAMPLE_IVS_MD_THREAD_PARAM_S;

static pthread_t s_IvsThread = 0;
static pthread_t s_VdecThread = 0;
static HI_U32 s_u32VdecChnCnt = 1;
static SAMPLE_IVS_MD_S s_stMd;
static SAMPLE_IVS_MD_THREAD_PARAM_S s_stMdThreadParam;
static VdecThreadParam s_stVdecSendParam;
static SAMPLE_VO_MODE_E s_enSampleVoMode = VO_MODE_1MUX;

static HI_VOID SAMPLE_IVS_MD_Uninit(SAMPLE_IVS_MD_S *pstMd)
{
	HI_S32 i;
	HI_S32 s32Ret = HI_SUCCESS;

    if (NULL == pstMd)
    {
        SAMPLE_PRT("NULL PTR!\n");
        return;
    }
    
	for (i = 0; i < SAMPLE_IVS_MD_IMG_NUM; i++)
	{	
    	IVE_MMZ_FREE(pstMd->astImg[i].u32PhyAddr[0],pstMd->astImg[i].pu8VirAddr[0]);
	}
	
    IVE_MMZ_FREE(pstMd->stBlob.u32PhyAddr,pstMd->stBlob.pu8VirAddr);
	
	s32Ret = HI_IVS_MD_Exit();
	if(HI_SUCCESS!=s32Ret)
	{
		SAMPLE_PRT("HI_IVS_MD_Exit fail,Error(%#x)\n",s32Ret);	
		return;
	}

    return;
}

static HI_S32 SAMPLE_IVS_MD_Init(SAMPLE_IVS_MD_S *pstMd,HI_U16 u16Width,HI_U16 u16Height)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 i ;
	HI_U32 u32Size;
	HI_U8 u8WndSz;
	
   	memset(pstMd,0,sizeof(SAMPLE_IVS_MD_S));
	for (i = 0;i < SAMPLE_IVS_MD_IMG_NUM;i++)
	{
		s32Ret = SAMPLE_COMM_IVE_CreateImage(&pstMd->astImg[i],IVE_IMAGE_TYPE_U8C1,u16Width,u16Height);
		SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, MD_INIT_FAIL,"SAMPLE_COMM_IVE_CreateImage fail\n");
	}
	
	u32Size = sizeof(IVE_CCBLOB_S);
	s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&pstMd->stBlob,u32Size);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, MD_INIT_FAIL,"SAMPLE_COMM_IVE_CreateMemInfo fail\n");
	
	//Set MD attribute
	pstMd->stMdAttr.enAlgMode = MD_ALG_MODE_BG;
	pstMd->stMdAttr.enSadMode = IVE_SAD_MODE_MB_4X4;
	pstMd->stMdAttr.u16SadThr = 100 * (1 << 1);//100 * (1 << 2);
	pstMd->stMdAttr.u16Width = u16Width;
	pstMd->stMdAttr.u16Height = u16Height;
	pstMd->stMdAttr.stAddCtrl.u0q16X = 32768;
	pstMd->stMdAttr.stAddCtrl.u0q16Y = 32768;
	u8WndSz = ( 1 << (2 + pstMd->stMdAttr.enSadMode));
	pstMd->stMdAttr.stCclCtrl.u16InitAreaThr = u8WndSz * u8WndSz;
	pstMd->stMdAttr.stCclCtrl.u16Step = u8WndSz;

	pstMd->s32CurIdx = 0;

	s32Ret = HI_IVS_MD_Init();
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, MD_INIT_FAIL,"HI_IVS_MD_Init fail,Error(%#x)\n",s32Ret);

MD_INIT_FAIL:
	
    if(HI_SUCCESS != s32Ret)
	{
        SAMPLE_IVS_MD_Uninit(pstMd);
	}
	return s32Ret;    
    
}

static HI_S32 SAMPLE_IVS_MD_DmaImage(VIDEO_FRAME_INFO_S *pstFrameInfo,IVE_DST_IMAGE_S *pstDst,HI_BOOL bInstant)
{
	HI_S32 s32Ret;
	IVE_HANDLE hIveHandle;
	IVE_SRC_DATA_S stSrcData;
	IVE_DST_DATA_S stDstData;
	IVE_DMA_CTRL_S stCtrl = {IVE_DMA_MODE_DIRECT_COPY,0};
	HI_BOOL bFinish = HI_FALSE;
	HI_BOOL bBlock = HI_TRUE;

	//fill src
	stSrcData.pu8VirAddr = (HI_U8*)pstFrameInfo->stVFrame.pVirAddr[0];
	stSrcData.u32PhyAddr = pstFrameInfo->stVFrame.u32PhyAddr[0];
	stSrcData.u16Width   = (HI_U16)pstFrameInfo->stVFrame.u32Width;
	stSrcData.u16Height  = (HI_U16)pstFrameInfo->stVFrame.u32Height;
	stSrcData.u16Stride  = (HI_U16)pstFrameInfo->stVFrame.u32Stride[0];

	//fill dst
	stDstData.pu8VirAddr = pstDst->pu8VirAddr[0];
	stDstData.u32PhyAddr = pstDst->u32PhyAddr[0];
	stDstData.u16Width   = pstDst->u16Width;
	stDstData.u16Height  = pstDst->u16Height;
	stDstData.u16Stride  = pstDst->u16Stride[0];

	s32Ret = HI_MPI_IVE_DMA(&hIveHandle,&stSrcData,&stDstData,&stCtrl,bInstant);	
	if (HI_SUCCESS != s32Ret)
	{
        SAMPLE_PRT("HI_MPI_IVE_DMA fail,Error(%#x)\n",s32Ret);
       return s32Ret;
    }
	
	if (HI_TRUE == bInstant)
	{
		s32Ret = HI_MPI_IVE_Query(hIveHandle,&bFinish,bBlock);			
		while(HI_ERR_IVE_QUERY_TIMEOUT == s32Ret)
		{
			usleep(100);					
			s32Ret = HI_MPI_IVE_Query(hIveHandle,&bFinish,bBlock);	
		}
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("HI_MPI_IVE_Query fail,Error(%#x)\n",s32Ret);
		   return s32Ret;
		}
	}

	return HI_SUCCESS;
}


static HI_VOID * SAMPLE_IVS_MD_Process(HI_VOID * pArgs)
{
	HI_S32 s32Ret = HI_SUCCESS;
	SAMPLE_IVS_MD_THREAD_PARAM_S *pstThreadParam;

	HI_U32 u32ObjNum, i;
	
	HI_U32 u32VpssGrpCnt;
	VPSS_CHN aVpssChn[VPSS_MAX_CHN_NUM];
	VENC_CHN VeH264Chn;	
	VO_LAYER VoLayer;
	VO_CHN   VoChn;	
	SAMPLE_IVS_MD_S *pstMd;
    HI_BOOL bInstant = HI_TRUE;
	MD_CHN MdChn = 0;
	MD_ATTR_S *pstMdAttr;
	
	HI_FLOAT f32SclX, f32SclY;
    
    prctl(PR_SET_NAME, "hi_MD_Process", 0, 0, 0);

	pstThreadParam = (SAMPLE_IVS_MD_THREAD_PARAM_S*)pArgs;
	u32VpssGrpCnt = pstThreadParam->u32VpssGrpCnt;
	aVpssChn[0]	  = pstThreadParam->aVpssChn[0];
	aVpssChn[1]	  = pstThreadParam->aVpssChn[1];
	VeH264Chn     = pstThreadParam->VeH264Chn;
	VoLayer       = pstThreadParam->VoLayer;
	VoChn		  = pstThreadParam->VoChn;
	pstMd	  	  = pstThreadParam->pstMd;
	pstMdAttr = &(pstMd->stMdAttr);
	
	//Create chn
	s32Ret = HI_IVS_MD_CreateChn(MdChn, pstMdAttr);	
	SAMPLE_CHECK_EXPR_RET(HI_SUCCESS != s32Ret,NULL,"HI_IVS_MD_CreateChn fail,Error(%#x)\n",s32Ret);

	while(HI_TRUE!=pstThreadParam->bStop)
	{				
		HI_S32 s32MilliSec = 2000;
		VIDEO_FRAME_INFO_S stBigFrm, stSmallFrm;
		
		for(i=0; i<u32VpssGrpCnt; i++)
		{
			VPSS_GRP VpssGrp = i;
			
			s32Ret = HI_MPI_VPSS_GetChnFrame(VpssGrp, aVpssChn[1], &stSmallFrm, s32MilliSec);
			if(HI_SUCCESS!=s32Ret)
			{
				SAMPLE_PRT("HI_MPI_VPSS_GetChnFrame small frame failed, VPSS_GRP(%d), VPSS_CHN(%d), errno: %#x!",
					VpssGrp, aVpssChn[1], s32Ret);
				continue;
			}
			
			s32Ret = HI_MPI_VPSS_GetChnFrame(VpssGrp, aVpssChn[0], &stBigFrm, s32MilliSec);
			SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, FREE_BIG_FRM,
				"HI_MPI_VPSS_GetChnFrame big frame failed, VPSS_GRP(%d), VPSS_CHN(%d), errno: %#x!",
				VpssGrp, aVpssChn[0], s32Ret);
			
			if (HI_TRUE != pstThreadParam->bFirstFrm)
			{
				s32Ret = SAMPLE_IVS_MD_DmaImage(&stSmallFrm, &pstMd->astImg[pstMd->s32CurIdx],bInstant);
				SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, FREE_SMALL_FRM,
					"SAMPLE_IVS_MD_DmaImage small frame failed, errno: %#x!", s32Ret);				
				pstThreadParam->u32FrmCnt++;
			}
			else
			{	
				s32Ret = SAMPLE_IVS_MD_DmaImage(&stSmallFrm, &pstMd->astImg[1 - pstMd->s32CurIdx], bInstant);	
				SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, FREE_SMALL_FRM,
					"SAMPLE_IVS_MD_DmaImage small frame failed, errno: %#x!", s32Ret);
				pstThreadParam->bFirstFrm = HI_FALSE;
				pstThreadParam->u32FrmCnt++;
				goto FREE_SMALL_FRM;
			}
			
			s32Ret = HI_IVS_MD_Process(MdChn,&pstMd->astImg[pstMd->s32CurIdx],
				&pstMd->astImg[1 - pstMd->s32CurIdx],&pstMd->stBlob);
			SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, FREE_SMALL_FRM,
					"HI_IVS_MD_Process failed, errno: %#x!", s32Ret);

			f32SclX = (HI_FLOAT)stBigFrm.stVFrame.u32Width/stSmallFrm.stVFrame.u32Width;
			f32SclY = (HI_FLOAT)stBigFrm.stVFrame.u32Height/stSmallFrm.stVFrame.u32Height;
			SAMPLE_COMM_IVE_BlobToRect((IVE_CCBLOB_S *)pstMd->stBlob.pu8VirAddr,
				&(pstMd->stRegion), MAX_OBJ_NUM,CCL_AREA_THR_STEP,f32SclX,f32SclY);
                                            
			//Draw rect
			s32Ret = SAMPLE_COMM_VGS_FillRect(&stBigFrm, &pstMd->stRegion, 0x0000FF00);
			SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS!=s32Ret, FREE_SMALL_FRM,
					"SAMPLE_COMM_VGS_FillRect failed, errno: %#x!", s32Ret);

			u32ObjNum = pstMd->stRegion.u16Num;
			printf("%dth frame, %d %s detected!\n", pstThreadParam->u32FrmCnt, u32ObjNum, u32ObjNum>1?"objects":"object");
			
			//Send to venc
			 if(pstThreadParam->bEncode)
			 {
				s32Ret = HI_MPI_VENC_SendFrame(VeH264Chn, &stBigFrm, s32MilliSec);
				if(HI_SUCCESS != s32Ret)
				{
					SAMPLE_PRT("HI_MPI_VENC_SendFrame fail with errno: %#x\n",s32Ret);
				} 
			 }
			 //Send to vo
			 if(pstThreadParam->bVo)
			 {			  
				 s32Ret = HI_MPI_VO_SendFrame(VoLayer, VoChn, &stBigFrm, s32MilliSec);
				 if (HI_SUCCESS != s32Ret)
				 {
					 SAMPLE_PRT("HI_MPI_VO_SendFrame,Error(%#x)\n",s32Ret);
				 }
			 }
			 
		FREE_SMALL_FRM:	 
			 s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, aVpssChn[1], &stSmallFrm);
			 if (HI_SUCCESS != s32Ret)
			 {
				 SAMPLE_PRT("HI_MPI_VPSS_ReleaseChnFrame failed,VpssGrp(%d),VpssChn(%d),Error(%#x)\n",VpssGrp,aVpssChn[1],s32Ret);
			 }
			 
		FREE_BIG_FRM:
			s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, aVpssChn[1], &stBigFrm);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("HI_MPI_VPSS_ReleaseChnFrame failed,VpssGrp(%d),VpssChn(%d),Error(%#x)\n",VpssGrp,aVpssChn[1],s32Ret);
			}
			
			//change reference and current frame index
			pstMd->s32CurIdx = 1 - pstMd->s32CurIdx;
		}
	
		usleep(20000);
	}

	//destroy 
	s32Ret = HI_IVS_MD_DestroyChn(MdChn);	
	SAMPLE_CHECK_EXPR_RET(HI_SUCCESS != s32Ret,NULL,"HI_IVS_MD_DestroyChn fail,Error(%#x)\n",s32Ret);
	
    return (HI_VOID *)HI_SUCCESS;
	
}

HI_VOID SAMPLE_IVS_MD_ThreadParam(SAMPLE_IVS_MD_THREAD_PARAM_S *pstThreadParam, HI_BOOL bEncode,
	HI_BOOL bVo, HI_U32 u32VpssGrpCnt, HI_U32 u32VpssChnNum, VPSS_CHN aVpssChn[],
	VENC_CHN VeH264Chn, VO_DEV VoDev, VO_LAYER VoLayer, VO_CHN VoChn, SAMPLE_IVS_MD_S *pstMd)
{
	HI_S32 i;

	HI_ASSERT(HI_NULL!=pstThreadParam);
	HI_ASSERT(u32VpssChnNum<VPSS_MAX_CHN_NUM);
	HI_ASSERT(HI_NULL!=pstMd);

	memset(pstThreadParam, 0, sizeof(SAMPLE_IVS_MD_THREAD_PARAM_S));

	pstThreadParam->bStop = HI_FALSE;
	pstThreadParam->bEncode = bEncode;
	pstThreadParam->bVo     = bVo;
	pstThreadParam->bFirstFrm = HI_TRUE;
	pstThreadParam->u32FrmCnt = 0;
	
	pstThreadParam->u32VpssGrpCnt = u32VpssGrpCnt;
	pstThreadParam->u32VpssChnNum = u32VpssChnNum;
	for(i=0; i<u32VpssChnNum; i++)
		pstThreadParam->aVpssChn[i] = aVpssChn[i];
	
	pstThreadParam->VeH264Chn = VeH264Chn;
    pstThreadParam->VoDev     = VoDev;
	pstThreadParam->VoLayer   = VoLayer;
	pstThreadParam->VoChn     = VoChn;
	pstThreadParam->pstMd 	  = pstMd;

}

HI_VOID SAMPLE_IVS_MD_ThreadProc(SAMPLE_IVS_MD_THREAD_PARAM_S *pstThreadParam, 
											pthread_t *pIvsThread)
{
	pthread_create(pIvsThread, 0, SAMPLE_IVS_MD_Process, (HI_VOID *)pstThreadParam);
}

HI_VOID SAMPLE_IVS_MD_CmdCtrl(SAMPLE_IVS_MD_THREAD_PARAM_S *pstThreadParam, pthread_t IvsThread)
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
			pstThreadParam->bStop = HI_TRUE;
			pthread_join(IvsThread,HI_NULL);
            break;
        }
    }
}

HI_VOID SAMPLE_IVS_MD(HI_BOOL bEncode, HI_BOOL bVo)
{
	HI_S32 s32Ret = HI_SUCCESS, i;
	
	VB_CONF_S stVbConf,stModVbConf;
	SIZE_S astSize[VPSS_CHN_NUM] = {{HD_WIDTH, HD_HEIGHT}, {VGA_WIDTH, VGA_HEIGHT}};
	HI_U32 u32Profile = 1; /*0: baseline; 1:MP; 2:HP 3:svc-t */
	PAYLOAD_TYPE_E enStreamType = PT_H264;
	
	VDEC_CHN_ATTR_S stVdecChnAttr;
	
	VPSS_CHN aVpssChn[VPSS_CHN_NUM] = {VPSS_CHN0, VPSS_CHN3};
	VPSS_GRP_ATTR_S stVpssGrpAttr;
	HI_U32 u32VpssGrpCnt = 1;
	
	VENC_CHN VeH264Chn = 0;
	SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
	PIC_SIZE_E enPicSize = PIC_HD1080;
	VIDEO_NORM_E enVideoNorm = VIDEO_ENCODING_MODE_PAL;
	HI_U32 u32VencChnCnt = 1;
		
	VO_DEV	 VoDev	 = SAMPLE_VO_DEV_DHD0;
	VO_CHN	 VoChn	 = 0;
	VO_LAYER VoLayer = SAMPLE_VO_LAYER_VHD0;
	VO_PUB_ATTR_S stVoPubAttr;
	VO_VIDEO_LAYER_ATTR_S stVoLayerAttr;

	HI_CHAR *pchStreamName = "./data/input/ive_perimeter_scene_1080p_01.h264";

    memset(&s_stMdThreadParam, 0, sizeof(s_stMdThreadParam));
    memset(&s_stVdecSendParam, 0, sizeof(s_stVdecSendParam));
	/******************************************
	 step  1: Init SYS and common VB
	******************************************/
	SAMPLE_COMM_IVS_SysConf(&stVbConf, astSize, VPSS_CHN_NUM);
	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_SYS_VB, "init SYS and common VB failed!\n");
	
	/******************************************
	 step 2: Init VDEC mod VB. 
	*****************************************/		
	SAMPLE_COMM_VDEC_ModCommPoolConf(&stModVbConf, enStreamType, &astSize[0], s_u32VdecChnCnt, HI_FALSE); 
	s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stModVbConf);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_SYS_VB, "init VDEC mod VB failed!\n");
	
	/******************************************
	 step  3: Init MD
	******************************************/
	s32Ret = SAMPLE_IVS_MD_Init(&s_stMd, astSize[1].u32Width,astSize[1].u32Height);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_MD_INIT, "SAMPLE_IVS_MD_Init failed\n");
	
	/******************************************
	 step 4:  start VDEC.
	*****************************************/
	SAMPLE_COMM_VDEC_ChnAttr(s_u32VdecChnCnt, &stVdecChnAttr, enStreamType, &astSize[0]);
	s32Ret =SAMPLE_COMM_VDEC_Start(s_u32VdecChnCnt, &stVdecChnAttr);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret,END_VDEC_START, "start VDEC failed with!\n");
    for(i=0;i<s_u32VdecChnCnt;i++)
    {
    	s32Ret = HI_MPI_VDEC_SetDisplayMode(i, VIDEO_DISPLAY_MODE_PREVIEW);
		SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VDEC_START, "VDEC_SetDisplayMode failed!\n");
    }

	/******************************************
	 step 5:  start VPSS and bind to VDEC.
	*****************************************/
	SAMPLE_COMM_IVS_VpssGrpAttr(u32VpssGrpCnt, &stVpssGrpAttr, &astSize[0]);
	s32Ret = SAMPLE_COMM_IVS_VpssStart(u32VpssGrpCnt, astSize, aVpssChn, VPSS_CHN_NUM, &stVpssGrpAttr);
	SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VPSS_START, "start VPSS failed!\n");
	
	for(i=0;i<u32VpssGrpCnt;i++)
	{	
		s32Ret = SAMPLE_COMM_VDEC_BindVpss(i,i);
		SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VPSS_BIND_VDEC, "SAMPLE_COMM_VDEC_BindVpss failed!\n");
	}
	
	/************************************************
	step 6:  start VO
	*************************************************/
	if(bVo)
	{		
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
		
	}

	/************************************************
	step 7:  start VENC
	*************************************************/
	if(bEncode)
	{
		s32Ret = SAMPLE_COMM_VENC_Start(VeH264Chn, enStreamType,enVideoNorm,enPicSize,enRcMode,u32Profile);
		SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VENC_START, "SAMPLE_COMM_VENC_Start(h264 Venc) failed!\n");
	}

	/******************************************
	step 8: VDEC start send stream. 
	******************************************/
	SAMPLE_COMM_VDEC_ThreadParam(s_u32VdecChnCnt, &s_stVdecSendParam, &stVdecChnAttr, pchStreamName);
	SAMPLE_COMM_VDEC_StartSendStream(s_u32VdecChnCnt, &s_stVdecSendParam, &s_VdecThread);

	/******************************************
	step 9: stream VENC process -- get stream, then save it to file. 
	******************************************/
	if(bEncode)
	{
		s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32VencChnCnt);
		SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_VENC_GET_STREAM, "SAMPLE_COMM_VENC_StartGetStream failed!\n"); 
	}
	
	/******************************************
	step 10: GMM process -- get stream, process and send to VO or VENC. 
	******************************************/
	SAMPLE_IVS_MD_ThreadParam(&s_stMdThreadParam,bEncode,bVo,u32VpssGrpCnt,VPSS_CHN_NUM,
							  aVpssChn,VeH264Chn, VoDev, VoLayer,VoChn, &s_stMd);
	SAMPLE_IVS_MD_ThreadProc(&s_stMdThreadParam, &s_IvsThread);
   
	/******************************************
	step 11: exit process
	******************************************/
	SAMPLE_IVS_MD_CmdCtrl(&s_stMdThreadParam, s_IvsThread);
	
END_VENC_GET_STREAM:
	if(bEncode)
	{
		SAMPLE_COMM_VENC_StopGetStream();
	}
	SAMPLE_COMM_VDEC_StopSendStream(s_u32VdecChnCnt,&s_stVdecSendParam,&s_VdecThread);

	if(bEncode)
	{		
	END_VENC_START:
		SAMPLE_COMM_VENC_Stop(VeH264Chn);
	}
	
	if(bVo)
	{
	END_VO_CHN: 
		SAMPLE_COMM_VO_StopChn(VoLayer, s_enSampleVoMode);
   
	END_LAYER_START: 
		SAMPLE_COMM_VO_StopLayer(VoLayer);
   
	END_HDMI_START: 
		SAMPLE_COMM_VO_HdmiStop();
	
	END_VO_START:
		SAMPLE_COMM_VO_StopDev(VoDev);
	}

END_VPSS_BIND_VDEC:
	for(i = 0;i < s_u32VdecChnCnt;i++)
	{
		SAMPLE_COMM_VDEC_UnBindVpss(i,i);
	}
	
END_VPSS_START:
	SAMPLE_COMM_IVS_VpssStop(u32VpssGrpCnt,VPSS_MAX_CHN_NUM);	
	
END_VDEC_START:
	SAMPLE_COMM_VDEC_Stop(s_u32VdecChnCnt);
	
END_MD_INIT:
	SAMPLE_IVS_MD_Uninit(&s_stMd);
	
END_SYS_VB:
	SAMPLE_COMM_SYS_Exit();
}

/******************************************************************************
* function : Md sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_Md_HandleSig(HI_VOID)
{
    HI_U32 i;
    
	s_stMdThreadParam.bStop = HI_TRUE;
	if (0 != s_IvsThread)
	{
		pthread_join(s_IvsThread, HI_NULL);	
		s_IvsThread = 0;
	}
    if(s_stMdThreadParam.bEncode)
	{
		SAMPLE_COMM_VENC_StopGetStream();
	}
	SAMPLE_COMM_VDEC_StopSendStream(s_u32VdecChnCnt,&s_stVdecSendParam,&s_VdecThread);

	if(s_stMdThreadParam.bEncode)
	{		
		SAMPLE_COMM_VENC_Stop(s_stMdThreadParam.VeH264Chn);
	}
	
	if(s_stMdThreadParam.bVo)
	{
		SAMPLE_COMM_VO_StopChn(s_stMdThreadParam.VoLayer, s_enSampleVoMode);
		SAMPLE_COMM_VO_StopLayer(s_stMdThreadParam.VoLayer); 
		SAMPLE_COMM_VO_HdmiStop();
		SAMPLE_COMM_VO_StopDev(s_stMdThreadParam.VoDev);
	}

	for(i = 0;i < s_u32VdecChnCnt; i++)
	{
		SAMPLE_COMM_VDEC_UnBindVpss(i,i);
	}
	
	SAMPLE_COMM_IVS_VpssStop(s_stMdThreadParam.u32VpssGrpCnt,VPSS_MAX_CHN_NUM);	
	SAMPLE_COMM_VDEC_Stop(s_u32VdecChnCnt);
	SAMPLE_IVS_MD_Uninit(&s_stMd);
	SAMPLE_COMM_SYS_Exit();
}


