/******************************************************************************
  A simple program of Hisilicon Hi35xx video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "sample_comm.h"

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VENC_Usage(char* sPrgNm)
{
    printf("Usage : %s <index>  eg: ./sample_venc 0 \n", sPrgNm);
    printf("index:\n");
    printf("\t 0) NormalP:H.265@1080p@30fps+H.264@1080p@30fps\n");
    printf("\t 1) SmartP: H.265@1080p@30fps+H.264@1080p@30fps\n");
    printf("\t 2) DualP:  H.265@1080p@30fps@BigSmallP+H.264@1080p@30fps@BigSmallP\n");   
	printf("\t 3) DualP2: H.265@1080p@30fps@SmallP+H.264@1080p@30fps@SmallP\n");
    printf("\t 4) BgModel: H.265@EnableBg+H.265@DisableBg.\n");
	printf("\t 5) IntraRefresh: H.265@1080p@30fps@IntraRefresh+H.264@1080p@30fps@IntraRefresh\n");
    printf("\t 6) Jpege:1*1080p mjpeg encode + 1*1080p jpeg\n");
	printf("\t 7) RoiBgFrameRate:H.264@1080p@30fps@RoiBgFrameRate + H.265@1080p@30fps@RoiBgFrameRate\n");
	printf("\t 8) svc-t :H.264@1080p@30fps@svc-t\n");
    return;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_VENC_HandleSig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_VENC_StopGetStream();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

/******************************************************************************
* function : to process abnormal case - the case of stream venc
******************************************************************************/
void SAMPLE_VENC_StreamHandleSig(HI_S32 signo)
{

    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}



/******************************************************************************
* function :  H.264@1080p@30fps+H.264@VGA@30fps


******************************************************************************/
HI_S32 SAMPLE_VENC_NORMALP_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
	HI_U32 u32Profile = 0;
	
    VB_CONF_S stVbConf;
	SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_1080P;
    HI_S32 s32VpssGrpCnt = 8;
    
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};

    
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
	
    HI_S32 s32ChnNum=2;
    
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
	
    stVbConf.u32MaxPoolCnt = 128;
    /*video buffer*/ 
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[0].u32BlkCnt = 32;
	}
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt  =32;
	}


    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode,gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
    
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    if(s32ChnNum >= 1)
    {
	    VpssGrp = 0;
	   	memset(&stVpssGrpAttr,0,sizeof(VPSS_GRP_ATTR_S));
		stVpssGrpAttr.u32MaxW 	= stSize.u32Width;
		stVpssGrpAttr.u32MaxH 	= stSize.u32Height;
		stVpssGrpAttr.bNrEn 	= HI_TRUE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stVpssGrpAttr.enPixFmt 	= SAMPLE_PIXEL_FORMAT;
		s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize,s32ChnNum , &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_2;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Vi bind Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_3;
	    }

    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch(c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;			
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_3;
    }

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;
		
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;	
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}
    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();
    
END_VENC_1080P_CLASSIC_4:
	
    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 2:
			VpssChn = 1;   
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;
			
	}
	
END_VENC_1080P_CLASSIC_3:    //unbind vpss and vi     
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt,s32ChnNum);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;    
}



/******************************************************************************
* function :  Gop_Mode_SmartP for H.265@1080P@normalP +H.265@1080P@smartP  


******************************************************************************/
HI_S32 SAMPLE_VENC_SMARTP_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};	
	VENC_GOP_ATTR_S stGopAttr;
	VENC_GOP_MODE_E enGopMode[2] = {VENC_GOPMODE_SMARTP,VENC_GOPMODE_SMARTP};
	HI_U32 u32Profile = 0;
	
	VB_CONF_S stVbConf;
	
	SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_1080P;
    HI_S32 s32VpssGrpCnt = 8;
    
	
	
	VPSS_GRP VpssGrp;
	VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};

	
	VENC_CHN VencChn;
	SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
	
    HI_S32 s32ChnNum=2;
	
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32BlkSize;
	SIZE_S stSize;
	char c;


	/******************************************
	 step  1: init sys variable 
	******************************************/
	memset(&stVbConf,0,sizeof(VB_CONF_S));
	
	stVbConf.u32MaxPoolCnt = 128;
//	printf("s32ChnNum:%d,Sensor Size:%d\n",s32ChnNum,enSize[0]);
    /*video buffer*/ 
	if(s32ChnNum >= 1)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
		stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[0].u32BlkCnt = 32;
	}
	if(s32ChnNum >= 2)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
		stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[1].u32BlkCnt =32;
	}


	/******************************************
	 step 2: mpp system init. 
	******************************************/
	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("system init failed with %d!\n", s32Ret);
		goto END_VENC_1080P_CLASSIC_0;
	}

	/******************************************
	 step 3: start vi dev & chn to capture
	******************************************/
	s32Ret = SAMPLE_COMM_VI_Start(enViMode,gs_enNorm);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("start vi failed!\n");
		goto END_VENC_1080P_CLASSIC_1;
	}
	
	/******************************************
	 step 4: start vpss and vi bind vpss
	******************************************/
	s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		goto END_VENC_1080P_CLASSIC_1;
	}

	if(s32ChnNum >= 1)
	{
		memset(&stVpssGrpAttr,0,sizeof(VPSS_GRP_ATTR_S));
		stVpssGrpAttr.u32MaxW 	= stSize.u32Width;
		stVpssGrpAttr.u32MaxH 	= stSize.u32Height;
		stVpssGrpAttr.bNrEn 	= HI_TRUE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stVpssGrpAttr.enPixFmt 	= SAMPLE_PIXEL_FORMAT;
		s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize,s32ChnNum , &stVpssGrpAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Vpss failed!\n");
			goto END_VENC_1080P_CLASSIC_1;
		}

		s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Vi bind Vpss failed!\n");
			goto END_VENC_1080P_CLASSIC_2;
		}


	}

	
	/******************************************
	 step 5: start stream venc
	******************************************/
	/*** HD1080P **/
	printf("\t c) cbr.\n");
	printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
	printf("\t f) fixQp\n");
	printf("please input choose rc mode!\n");
	c = (char)getchar();
	switch(c)
	{
		case 'c':
			enRcMode = SAMPLE_RC_CBR;
			break;
		case 'v':
			enRcMode = SAMPLE_RC_VBR;
			break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;			
		case 'f':
			enRcMode = SAMPLE_RC_FIXQP;
			break;
		default:
			printf("rc mode! is invaild!\n");
			goto END_VENC_1080P_CLASSIC_3;
	}

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
		VpssChn = 0;
	    VencChn = 0;
		
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[0],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }		
		
	    s32Ret = SAMPLE_COMM_VENC_StartEx(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile,&stGopAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
		
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[1],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }		
		
	    s32Ret = SAMPLE_COMM_VENC_StartEx(VencChn, enPayLoad[1],\
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile,&stGopAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}

	
	
	/******************************************
	 step 6: stream venc process -- get stream, then save it to file. 
	  ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
		if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		goto END_VENC_1080P_CLASSIC_4;
	}
	
	printf("please press ENTER to exit this sample\n");
	getchar();
	getchar();
	
	/******************************************
	 step 7: exit process
	******************************************/
	
	SAMPLE_COMM_VENC_StopGetStream();
	
END_VENC_1080P_CLASSIC_4:
	VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 2:
			VpssChn = 1;   
			VencChn = 1;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
			VencChn = 0;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
			break;
	}	
END_VENC_1080P_CLASSIC_3:    //unbind vpss and vi       
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt,s32ChnNum);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;    
}


/******************************************************************************
* function :   function DUALP :  H.265@1080@30fps+H.264@1080@30fps

******************************************************************************/
HI_S32 SAMPLE_VENC_DUALP_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265,PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
	VENC_GOP_ATTR_S stGopAttr;
	VENC_GOP_MODE_E enGopMode[2] = {VENC_GOPMODE_DUALP,VENC_GOPMODE_DUALP};
	HI_U32 u32Profile = 0;
	
    VB_CONF_S stVbConf;
	SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_1080P;
    HI_S32 s32VpssGrpCnt = 8;

    
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
	
    HI_S32 s32ChnNum=2;
    
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    stVbConf.u32MaxPoolCnt = 128;
//	printf("s32ChnNum = %d\n",s32ChnNum);
    /*video buffer*/ 
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[0].u32BlkCnt = 32;
	}
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt  = 32;
	}


    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
	s32Ret = SAMPLE_COMM_VI_Start(enViMode,gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
    
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    if(s32ChnNum >= 1)
    {
	    memset(&stVpssGrpAttr,0,sizeof(VPSS_GRP_ATTR_S));
		stVpssGrpAttr.u32MaxW 	= stSize.u32Width;
		stVpssGrpAttr.u32MaxH 	= stSize.u32Height;
		stVpssGrpAttr.bNrEn 	= HI_TRUE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stVpssGrpAttr.enPixFmt 	= SAMPLE_PIXEL_FORMAT;
		s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize,s32ChnNum , &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_1;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Vi bind Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_2;
	    }
    }
    
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");	
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch(c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;			
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_3;
    }

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;
		
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[0],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }		
		
	    s32Ret = SAMPLE_COMM_VENC_StartEx(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[1],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
		
	    s32Ret = SAMPLE_COMM_VENC_StartEx(VencChn, enPayLoad[1],\
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}
	
    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();
    
END_VENC_1080P_CLASSIC_4:
	
    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 2:
			VpssChn = 1;   
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;
			
	}



END_VENC_1080P_CLASSIC_3:    //unbind vpss and vi       
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt,s32ChnNum);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;    
}


/******************************************************************************
* function :   function DUALP :  H.265@1080@30fps+H.264@1080@30fps

******************************************************************************/
HI_S32 SAMPLE_VENC_DUALPP_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265,PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
	VENC_GOP_ATTR_S stGopAttr;
	VENC_GOP_MODE_E enGopMode[2] = {VENC_GOPMODE_DUALP,VENC_GOPMODE_DUALP};
	HI_U32 u32Profile = 0;
	
    VB_CONF_S stVbConf;
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_1080P;
    HI_S32 s32VpssGrpCnt = 8;
    
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
	
    HI_S32 s32ChnNum=2;
    
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    stVbConf.u32MaxPoolCnt = 128;
//	printf("s32ChnNum = %d\n",s32ChnNum);
    /*video buffer*/ 
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[0].u32BlkCnt = 32;
	}
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =32;
	}

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
	s32Ret = SAMPLE_COMM_VI_Start(enViMode,gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
    
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    if(s32ChnNum >= 1)
    {
	    memset(&stVpssGrpAttr,0,sizeof(VPSS_GRP_ATTR_S));
		stVpssGrpAttr.u32MaxW 	= stSize.u32Width;
		stVpssGrpAttr.u32MaxH 	= stSize.u32Height;
		stVpssGrpAttr.bNrEn 	= HI_TRUE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stVpssGrpAttr.enPixFmt 	= SAMPLE_PIXEL_FORMAT;
		s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize,s32ChnNum , &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_1;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Vi bind Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_2;
	    }

	    
    }
    
    
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch(c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;			
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_3;
    }

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;
		
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[0],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }		
		stGopAttr.stDualP.u32SPInterval = 0;
	    s32Ret = SAMPLE_COMM_VENC_StartEx(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[1],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
		stGopAttr.stDualP.u32SPInterval = 0;
	    s32Ret = SAMPLE_COMM_VENC_StartEx(VencChn, enPayLoad[1],\
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}
	
    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();
    
END_VENC_1080P_CLASSIC_4:
	
    VpssGrp = 0;
	switch(s32ChnNum)
	{		case 2:
			VpssChn = 1;   
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;
			
	}

END_VENC_1080P_CLASSIC_3:    //unbind vpss and vi       
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt,s32ChnNum);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;    
}



/******************************************************************************
* function BgModel :  H.265@1080@30fps+H.265@1080@30fps

******************************************************************************/
HI_S32 SAMPLE_VENC_BgModel_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H265};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};	
	VENC_GOP_ATTR_S stGopAttr;
	VENC_GOP_MODE_E enGopMode[2] = {VENC_GOPMODE_SMARTP,VENC_GOPMODE_SMARTP};
	HI_U32 u32Profile = 0;
	
	VB_CONF_S stVbConf;
	
	HI_BOOL bEnableEnSmartP=HI_TRUE;
	SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_1080P;
    HI_S32 s32VpssGrpCnt = 8;
    
	
	
	VPSS_GRP VpssGrp;
	VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};

	
	VENC_CHN VencChn;
	SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
	
    HI_S32 s32ChnNum=2;
	
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32BlkSize;
	SIZE_S stSize;
	char c;


	/******************************************
	 step  1: init sys variable 
	******************************************/
	memset(&stVbConf,0,sizeof(VB_CONF_S));
	
	stVbConf.u32MaxPoolCnt = 128;
//	printf("s32ChnNum:%d,Sensor Size:%d\n",s32ChnNum,enSize[0]);
    /*video buffer*/ 
	if(s32ChnNum >= 1)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
		stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[0].u32BlkCnt = 32;
	}
	if(s32ChnNum >= 2)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
		stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[1].u32BlkCnt =32;
	}


	/******************************************
	 step 2: mpp system init. 
	******************************************/
	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("system init failed with %d!\n", s32Ret);
		goto END_VENC_1080P_CLASSIC_0;
	}

	/******************************************
	 step 3: start vi dev & chn to capture
	******************************************/
	s32Ret = SAMPLE_COMM_VI_Start(enViMode,gs_enNorm);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("start vi failed!\n");
		goto END_VENC_1080P_CLASSIC_1;
	}
	
	/******************************************
	 step 4: start vpss and vi bind vpss
	******************************************/
	s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		goto END_VENC_1080P_CLASSIC_1;
	}

	if(s32ChnNum >= 1)
	{
		memset(&stVpssGrpAttr,0,sizeof(VPSS_GRP_ATTR_S));
		stVpssGrpAttr.u32MaxW 	= stSize.u32Width;
		stVpssGrpAttr.u32MaxH 	= stSize.u32Height;
		stVpssGrpAttr.bNrEn 	= HI_TRUE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stVpssGrpAttr.enPixFmt 	= SAMPLE_PIXEL_FORMAT;
		s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize,s32ChnNum , &stVpssGrpAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Vpss failed!\n");
			goto END_VENC_1080P_CLASSIC_1;
		}

		s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Vi bind Vpss failed!\n");
			goto END_VENC_1080P_CLASSIC_2;
		}


	}

	
	/******************************************
	 step 5: start stream venc
	******************************************/
	/*** HD1080P **/
	printf("\t c) cbr.\n");
	printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
	printf("\t f) fixQp\n");
	printf("please input choose rc mode!\n");
	c = (char)getchar();
	switch(c)
	{
		case 'c':
			enRcMode = SAMPLE_RC_CBR;
			break;
		case 'v':
			enRcMode = SAMPLE_RC_VBR;
			break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;			
		case 'f':
			enRcMode = SAMPLE_RC_FIXQP;
			break;
		default:
			printf("rc mode! is invaild!\n");
			goto END_VENC_1080P_CLASSIC_3;
	}

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
		VpssChn = 0;
	    VencChn = 0;
		
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[0],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }		
		
	    s32Ret = SAMPLE_COMM_VENC_StartEx(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile,&stGopAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

		
		bEnableEnSmartP = HI_TRUE;
		s32Ret = HI_MPI_VENC_EnableAdvSmartP(VencChn,bEnableEnSmartP);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Enable EnhancedSmartP failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
		
		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
		
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[1],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }		
		
	    s32Ret = SAMPLE_COMM_VENC_StartEx(VencChn, enPayLoad[1],\
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile,&stGopAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}

	
	
	/******************************************
	 step 6: stream venc process -- get stream, then save it to file. 
	  ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
		if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		goto END_VENC_1080P_CLASSIC_4;
	}
	
	printf("please press ENTER to exit this sample\n");
	getchar();
	getchar();
	
	/******************************************
	 step 7: exit process
	******************************************/
	
	SAMPLE_COMM_VENC_StopGetStream();
	
END_VENC_1080P_CLASSIC_4:
	VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 2:
			VpssChn = 1;   
			VencChn = 1;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
			VencChn = 0;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
			break;
	}	
END_VENC_1080P_CLASSIC_3:    //unbind vpss and vi       
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt,s32ChnNum);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;    
}



/******************************************************************************
* function :  H264 P refurbish I macro block/I slice,normal@1080+I macro block@1080+Islice @1080
******************************************************************************/
HI_S32 SAMPLE_VENC_IntraRefresh_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};	
	HI_U32 u32Profile = 0;
	
    VB_CONF_S stVbConf;
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_1080P;
    HI_S32 s32VpssGrpCnt = 8;
    
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};

    
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
	VENC_PARAM_INTRA_REFRESH_S stIntraRefresh;
    HI_S32 s32ChnNum=2;
    
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
	
    stVbConf.u32MaxPoolCnt = 128;
    //printf("s32ChnNum:%d,Sensor Size:%d\n",s32ChnNum,enSize[0]);
    /*video buffer*/ 
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[0].u32BlkCnt = 32;
	}
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =32;
	}


    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode,gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
    
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    if(s32ChnNum >= 1)
    {
	    memset(&stVpssGrpAttr,0,sizeof(VPSS_GRP_ATTR_S));
		stVpssGrpAttr.u32MaxW 	= stSize.u32Width;
		stVpssGrpAttr.u32MaxH 	= stSize.u32Height;
		stVpssGrpAttr.bNrEn 	= HI_TRUE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stVpssGrpAttr.enPixFmt 	= SAMPLE_PIXEL_FORMAT;
		s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize,s32ChnNum , &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_1;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Vi bind Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_2;
	    }
    }
   
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch(c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;			
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_3;
    }

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;
		
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
		
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
			SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
		s32Ret = HI_MPI_VENC_GetIntraRefresh(VencChn,&stIntraRefresh);
		stIntraRefresh.bISliceEnable  = HI_TRUE;
		stIntraRefresh.bRefreshEnable = HI_TRUE;
		stIntraRefresh.u32RefreshLineNum = (stSize.u32Height + 63)>>8;
		stIntraRefresh.u32ReqIQp = 30;

		s32Ret = HI_MPI_VENC_SetIntraRefresh(VencChn,&stIntraRefresh);		
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
		
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1],\
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }	

		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
			SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

		s32Ret = HI_MPI_VENC_GetIntraRefresh(VencChn,&stIntraRefresh);
		stIntraRefresh.bISliceEnable = HI_TRUE;
		stIntraRefresh.bRefreshEnable = HI_TRUE;
		stIntraRefresh.u32RefreshLineNum = (stSize.u32Height + 15)>>6;
		stIntraRefresh.u32ReqIQp = 30;
		s32Ret = HI_MPI_VENC_SetIntraRefresh(VencChn,&stIntraRefresh);
		
	}
	
    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();
    
END_VENC_1080P_CLASSIC_4:
	
    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 2:
			VpssChn = 1;   
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;
			
	}
	
END_VENC_1080P_CLASSIC_3:    //unbind vpss and vi       
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt,s32ChnNum);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;    
}

/******************************************************************************
* function :  1 chn 1080p MJPEG encode and 1 chn 1080p JPEG snap
******************************************************************************/
HI_S32 SAMPLE_VENC_1080P_MJPEG_JPEG(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad = PT_MJPEG;
    PIC_SIZE_E enSize[2] = {PIC_HD1080,PIC_HD1080};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_1080P;
    HI_S32 s32VpssGrpCnt = 8;


    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 2;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    HI_S32 i = 0;
    char ch;

    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    {
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[0].u32BlkCnt = 32;
	}

    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =32;
	}



    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_MJPEG_JPEG_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
	s32Ret = SAMPLE_COMM_VI_Start(enViMode,gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_MJPEG_JPEG_0;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_MJPEG_JPEG_1;
    }

    memset(&stVpssGrpAttr,0,sizeof(VPSS_GRP_ATTR_S));
	stVpssGrpAttr.u32MaxW 	= stSize.u32Width;
	stVpssGrpAttr.u32MaxH 	= stSize.u32Height;
	stVpssGrpAttr.bNrEn 	= HI_TRUE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	stVpssGrpAttr.enPixFmt 	= SAMPLE_PIXEL_FORMAT;
	s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize,s32ChnNum , &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_MJPEG_JPEG_1;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_MJPEG_JPEG_2;
    }


    /******************************************
     step 5: start stream venc
    ******************************************/
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad, \
                                    gs_enNorm, enSize[0], enRcMode, u32Profile);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_3;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_4;
    }

	
	s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		goto END_VENC_MJPEG_JPEG_4;
	}

    VpssGrp = 0;
    VpssChn = 1;
    VencChn = 1;
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        goto END_VENC_MJPEG_JPEG_4;
    }


    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_4;
    }

    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum-1);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_4;
    }

    printf("press 'q' to exit sample!\npress ENTER to capture one picture to file\n");
    i = 0;
    while ((ch = (char)getchar()) != 'q')
    {
        s32Ret = SAMPLE_COMM_VENC_SnapProcess(VencChn);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: sanp process failed!\n", __FUNCTION__);
            break;
        }
        printf("snap %d success!\n", i);
        i++;
        printf("press 'q' to exit sample!\npress ENTER to capture one picture to file\n");
    }

    /******************************************
     step 8: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_MJPEG_JPEG_4:
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);

    VpssChn = 1;
    VencChn = 1;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);
END_VENC_MJPEG_JPEG_3:    //unbind vpss and venc
	SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_MJPEG_JPEG_2:    //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt,s32ChnNum);
END_VENC_MJPEG_JPEG_1:    //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_MJPEG_JPEG_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}



HI_S32 SAMPLE_VENC_ROIBG_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2] = {PT_H264,PT_H265};
    PIC_SIZE_E enSize[3] = {PIC_HD1080, PIC_HD1080, PIC_D1};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_1080P;
    HI_S32 s32VpssGrpCnt = 8;

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};

    VENC_ROI_CFG_S  stVencRoiCfg;
    VENC_ROIBG_FRAME_RATE_S stRoiBgFrameRate;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 2;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));;

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 32;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 32;



    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
	s32Ret = SAMPLE_COMM_VI_Start(enViMode,gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
	if(s32ChnNum >= 1)
	{
	    memset(&stVpssGrpAttr,0,sizeof(VPSS_GRP_ATTR_S));
		stVpssGrpAttr.u32MaxW 	= stSize.u32Width;
		stVpssGrpAttr.u32MaxH 	= stSize.u32Height;
		stVpssGrpAttr.bNrEn 	= HI_TRUE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stVpssGrpAttr.enPixFmt 	= SAMPLE_PIXEL_FORMAT;
		s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize,s32ChnNum , &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_1;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Vi bind Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_2;
	    }
	}

	
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch (c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;			
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_3;
    }
	if(s32ChnNum >= 1)
	{
	    VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0], \
	                                    gs_enNorm, enSize[0], enRcMode, u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	    stVencRoiCfg.bAbsQp   = HI_TRUE;
	    stVencRoiCfg.bEnable  = HI_TRUE;
	    stVencRoiCfg.s32Qp    = 30;
	    stVencRoiCfg.u32Index = 0;
	    stVencRoiCfg.stRect.s32X = 64;
	    stVencRoiCfg.stRect.s32Y = 64;
	    stVencRoiCfg.stRect.u32Height = 256;
	    stVencRoiCfg.stRect.u32Width = 256;
	    s32Ret = HI_MPI_VENC_SetRoiCfg(VencChn, &stVencRoiCfg);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }

	    s32Ret = HI_MPI_VENC_GetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VENC_GetRoiBgFrameRate failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	    stRoiBgFrameRate.s32SrcFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 25 : 30;
	    stRoiBgFrameRate.s32DstFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 5 : 15;

	    s32Ret = HI_MPI_VENC_SetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VENC_SetRoiBgFrameRate!\n");
	        goto END_VENC_1080P_CLASSIC_4;
		}
	}
	if(s32ChnNum >= 2)
	{
		VpssGrp = 0;
	    VpssChn = 1;
	    VencChn = 1;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
	                                    gs_enNorm, enSize[1], enRcMode, u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	    stVencRoiCfg.bAbsQp   = HI_TRUE;
	    stVencRoiCfg.bEnable  = HI_TRUE;
	    stVencRoiCfg.s32Qp    = 30;
	    stVencRoiCfg.u32Index = 0;
	    stVencRoiCfg.stRect.s32X = 64;
	    stVencRoiCfg.stRect.s32Y = 64;
	    stVencRoiCfg.stRect.u32Height = 256;
	    stVencRoiCfg.stRect.u32Width = 256;
	    s32Ret = HI_MPI_VENC_SetRoiCfg(VencChn, &stVencRoiCfg);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	    s32Ret = HI_MPI_VENC_GetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VENC_GetRoiBgFrameRate failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	    stRoiBgFrameRate.s32SrcFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 25 : 30;
	    stRoiBgFrameRate.s32DstFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 5 : 15;
	    s32Ret = HI_MPI_VENC_SetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VENC_SetRoiBgFrameRate!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_4:
    VpssGrp = 0;

    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);



    VpssChn = 1;
    VencChn = 1;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);
END_VENC_1080P_CLASSIC_3:    //unbind vpss and vi
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt,s32ChnNum);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

HI_S32 SAMPLE_VENC_SVC_H264(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    PIC_SIZE_E enSize[3] = {PIC_HD1080, PIC_HD1080, PIC_HD1080};
    HI_U32 u32Profile = 3;/* Svc-t */

    VB_CONF_S stVbConf;
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_1080P;
    HI_S32 s32VpssGrpCnt = 8;


    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};


    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 1;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));


    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 32;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH,COMPRESS_MODE_SEG);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 32;




    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
	s32Ret = SAMPLE_COMM_VI_Start(enViMode,gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    memset(&stVpssGrpAttr,0,sizeof(VPSS_GRP_ATTR_S));
	stVpssGrpAttr.u32MaxW 	= stSize.u32Width;
	stVpssGrpAttr.u32MaxH 	= stSize.u32Height;
	stVpssGrpAttr.bNrEn 	= HI_TRUE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	stVpssGrpAttr.enPixFmt 	= SAMPLE_PIXEL_FORMAT;
	s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize,s32ChnNum , &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_2;
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch (c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;			
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_3;
    }
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;

    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad, \
                                    gs_enNorm, enSize[0], enRcMode, u32Profile);


    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
	{
		 SAMPLE_PRT("bind  Venc and vpss failed!\n");
		 goto END_VENC_1080P_CLASSIC_4;
	 }



    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream_Svc_t(s32ChnNum);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_4:
    VpssGrp = 0;

    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);



END_VENC_1080P_CLASSIC_3:	 //unbind vpss and venc
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1080P_CLASSIC_2:	 //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt,s32ChnNum);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}



/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/
int main(int argc, char *argv[])
{
    HI_S32 s32Ret;
    if ( (argc < 2) || (1 != strlen(argv[1])))
    {
        SAMPLE_VENC_Usage(argv[0]);
        return HI_FAILURE;
    }
	

    signal(SIGINT, SAMPLE_VENC_HandleSig);
    signal(SIGTERM, SAMPLE_VENC_HandleSig);


    switch (*argv[1])
    {
        case '0':/* H.264@NormalP@1080p@30fps+H.265NormalP@1080p@30fps*/
            s32Ret = SAMPLE_VENC_NORMALP_CLASSIC();
            break;
			
		case '1':/*H.264@DualP@1080p@30fps + H.265@SmartP@1080p@30fps*/
    		s32Ret = SAMPLE_VENC_SMARTP_CLASSIC();
            break;
			
		case '2':/*H.264@SmartP@1080p@30fps + H.265@DualP@1080p@30fps*/
			s32Ret = SAMPLE_VENC_DUALP_CLASSIC();
			break;
			
		case '3':/* H.265@1080@30fps+H.265@1080@30fps */
            s32Ret = SAMPLE_VENC_DUALPP_CLASSIC();
            break;
        case '4':/* H.264@NormalP@1080p@30fps@P_Islice + H.265@NormalP@1080p@30fps@P_Islice*/
            s32Ret = SAMPLE_VENC_BgModel_CLASSIC();
            break;				
        case '5':/* H.264@NormalP@1080p@30fps@P_Islice + H.265@NormalP@1080p@30fps@P_Islice*/
            s32Ret = SAMPLE_VENC_IntraRefresh_CLASSIC();
            break;
			
		case '6':/* 1*1080p mjpeg encode + 1*1080p jpeg  */
            s32Ret = SAMPLE_VENC_1080P_MJPEG_JPEG();
            break;
        case '7':/* H.264@NormalP@1080p@30fps@RoiBgFrameRate+ H.265@NormalP@1080p@30fps@RoiBgFrameRate*/
            s32Ret = SAMPLE_VENC_ROIBG_CLASSIC();
            break;
		case '8':/* H.264 Svc-t */
            s32Ret = SAMPLE_VENC_SVC_H264();
            break;	
        default:
            printf("the index is invaild!\n");
            SAMPLE_VENC_Usage(argv[0]);
            return HI_FAILURE;
    }

    if (HI_SUCCESS == s32Ret)
    { printf("program exit normally!\n"); }
    else
    { printf("program exit abnormally!\n"); }
    exit(s32Ret);

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
