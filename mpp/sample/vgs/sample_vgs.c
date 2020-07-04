/******************************************************************************
  A simple program of Hisilicon mpp implementation.
  Copyright (C), 2012-2020, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2013-7 Created
******************************************************************************/

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
#include "mpi_vgs.h"


SAMPLE_MEMBUF_S g_stMem;
VIDEO_FRAME_INFO_S g_stFrameInfo;
HI_BOOL g_bUserVdecBuf = HI_FALSE;
HI_BOOL g_bUserCommVb = HI_FALSE;

/******************************************************************************
* function : to process abnormal case                                         
******************************************************************************/
void SAMPLE_VGS_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        if (g_bUserVdecBuf)
        {
            HI_MPI_VDEC_ReleaseImage(0, &g_stFrameInfo);
    		g_bUserVdecBuf = HI_FALSE;
        }
		if (g_bUserCommVb)
		{
		    HI_MPI_VB_ReleaseBlock(g_stMem.hBlock);
		    g_bUserCommVb = HI_FALSE;
		}
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

HI_VOID SAMPLE_VGS_Usage(HI_VOID)
{
    printf("\n\n/************************************/\n");
    printf("please choose the case which you want to run:\n");
    printf("\t0:  Decompress tile picture from vdec\n");
    printf("\tq:  quit the whole sample\n");
    printf("sample command:");
}

HI_VOID SAMPLE_VGS_SaveSP42XToPlanar(FILE *pfile, VIDEO_FRAME_S *pVBuf)
{
    unsigned int w, h;
    char * pVBufVirt_Y;
    char * pVBufVirt_C;
    char * pMemContent;
    unsigned char *TmpBuff;
    HI_U32 size;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    HI_U32 u32UvHeight;/* the width for saved UV*/
	
    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat)
    {
        size = (pVBuf->u32Stride[0])*(pVBuf->u32Height)*3/2;    
        u32UvHeight = pVBuf->u32Height/2;
    }
    else
    {
        size = (pVBuf->u32Stride[0])*(pVBuf->u32Height)*2;   
        u32UvHeight = pVBuf->u32Height;
    }

    pVBufVirt_Y = pVBuf->pVirAddr[0]; 
    pVBufVirt_C = pVBufVirt_Y + (pVBuf->u32Stride[0])*(pVBuf->u32Height);

    TmpBuff = (unsigned char *)malloc(size);
    if(NULL == TmpBuff)
    {
        printf("Func:%s line:%d -- unable alloc %dB memory for tmp buffer\n", 
            __FUNCTION__, __LINE__, size);
        return;
    }

    /* save Y ----------------------------------------------------------------*/

    for(h=0; h<pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h*pVBuf->u32Stride[0];
        fwrite(pMemContent, 1,pVBuf->u32Width,pfile);
    }

    /* save U ----------------------------------------------------------------*/
    for(h=0; h<u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        pMemContent += 1;

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, 1,pVBuf->u32Width/2,pfile);
    }

    /* save V ----------------------------------------------------------------*/
    for(h=0; h<u32UvHeight; h++)    
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, 1,pVBuf->u32Width/2,pfile);
    }
    
    free(TmpBuff);

    return;
}


HI_S32 VGS_SAMPLE_LoadBmp(const char *filename, BITMAP_S *pstBitmap, HI_BOOL bFil, HI_U32 u16FilColor,PIXEL_FORMAT_E enPixelFormat)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;
    HI_S32 s32BytesPerPix = 2;

    if(GetBmpInfo(filename,&bmpFileHeader,&bmpInfo) < 0)
    {
        printf("GetBmpInfo err!\n");
        return HI_FAILURE;
    }

    if(enPixelFormat == PIXEL_FORMAT_RGB_4444)
    {
        Surface.enColorFmt =OSD_COLOR_FMT_RGB4444;
    }
    else if(enPixelFormat == PIXEL_FORMAT_RGB_1555)
    {
        Surface.enColorFmt =OSD_COLOR_FMT_RGB1555;
    }
    else if(enPixelFormat == PIXEL_FORMAT_RGB_8888)
    {
        Surface.enColorFmt =OSD_COLOR_FMT_RGB8888;
        s32BytesPerPix = 4;
    }
    else
    {
        printf("enPixelFormat err %d \n",enPixelFormat);
        return HI_FAILURE;
    }
    
    pstBitmap->pData = malloc(s32BytesPerPix*(bmpInfo.bmiHeader.biWidth)*(bmpInfo.bmiHeader.biHeight));
    
    if(NULL == pstBitmap->pData)
    {
        printf("malloc osd memroy err!\n");        
        return HI_FAILURE;
    }
    //printf("line:%d\n",__LINE__);
    CreateSurfaceByBitMap(filename,&Surface,(HI_U8*)(pstBitmap->pData));
    
    pstBitmap->u32Width = Surface.u16Width;
    pstBitmap->u32Height = Surface.u16Height;
    
    pstBitmap->enPixelFormat = enPixelFormat;
   
    

    int i,j;
    HI_U16 *pu16Temp;
    pu16Temp = (HI_U16*)pstBitmap->pData;
    
    if (bFil)
    {
        for (i=0; i<pstBitmap->u32Height; i++)
        {
            for (j=0; j<pstBitmap->u32Width; j++)
            {
                if (u16FilColor == *pu16Temp)
                {
                    *pu16Temp &= 0x7FFF;
                }

                pu16Temp++;
            }
        }

    }
    
    return HI_SUCCESS;
}


HI_S32 SAMPLE_VGS_Decompress_TilePicture(HI_VOID)
{
    VB_CONF_S stVbConf, stModVbConf;
    HI_S32 i = 0, s32Ret = HI_SUCCESS;
    VDEC_CHN_ATTR_S stVdecChnAttr[2];
    VdecThreadParam stVdecSend[2];
    SIZE_S stSize;
    HI_U32  u32BlkSize;
    VB_POOL hPool  = VB_INVALID_POOLID;
    pthread_t   VdecThread[2*VDEC_MAX_CHN_NUM];
    FILE *fpYuv = NULL;
    HI_U32 u32PicLStride            = 0;
    HI_U32 u32PicCStride            = 0;
    HI_U32 u32LumaSize              = 0;
    HI_U32 u32ChrmSize              = 0;
    HI_U32 u32OutWidth              = 1280;
    HI_U32 u32OutHeight             = 720;
    HI_CHAR OutFilename[100]        = {0};
	VGS_COVER_S stVgsAddCover[4];
	VGS_OSD_S 	stVgsAddOsd[3];
	VGS_LINE_S  stVgsLine[5];
    RGN_ATTR_S stRgnAttr;
    RGN_HANDLE Handle = 0;
	const char *filename;
    PIXEL_FORMAT_E enPixelFormat = PIXEL_FORMAT_RGB_1555;
    BITMAP_S stBitmap;
	RGN_CANVAS_INFO_S stCanvasInfo;
	
    stSize.u32Width  = HD_WIDTH;
    stSize.u32Height = HD_HEIGHT;

    u32BlkSize = u32OutWidth*u32OutHeight*3>>1;

    snprintf(OutFilename, 100, "Sample_VGS_%d_%d_%s.yuv", 
            u32OutWidth, u32OutHeight,  "420");
    fpYuv = fopen(OutFilename, "wb");
    if(fpYuv == NULL)
    {
        printf("SAMPLE_TEST:can't open file %s to save yuv\n","Decompress.yuv");
        return HI_FAILURE;
    }
    
    /************************************************
    step1:  init SYS and common VB 
    *************************************************/
    SAMPLE_COMM_VDEC_Sysconf(&stVbConf, &stSize);
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        goto END1;
    }
	
    /************************************************
    step2:  init mod common VB
    *************************************************/
    SAMPLE_COMM_VDEC_ModCommPoolConf(&stModVbConf, PT_H264, &stSize, 1,0);//not compress
    s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stModVbConf);
    if(s32Ret != HI_SUCCESS)
    {	    	
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        goto END1;
    }


    /******************************************
    step 3: create private pool on ddr0
    ******************************************/
    hPool   = HI_MPI_VB_CreatePool( u32BlkSize, 10,NULL);
    if (hPool == VB_INVALID_POOLID)
    {
        SAMPLE_PRT("HI_MPI_VB_CreatePool failed! \n");
        goto END1;
    }

   
    /************************************************
    step4:  start VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ChnAttr(1, &stVdecChnAttr[0], PT_H264, &stSize);
    s32Ret = SAMPLE_COMM_VDEC_Start(1, &stVdecChnAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {	
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END2;
    }

    /************************************************
    step4:  send stream to VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ThreadParam(1, &stVdecSend[0], &stVdecChnAttr[0], SAMPLE_1080P_H264_PATH);	
    SAMPLE_COMM_VDEC_StartSendStream(1, &stVdecSend[0], &VdecThread[0]);


    u32PicLStride = CEILING_2_POWER(u32OutWidth, SAMPLE_SYS_ALIGN_WIDTH);
    u32PicCStride = CEILING_2_POWER(u32OutWidth, SAMPLE_SYS_ALIGN_WIDTH);
    u32LumaSize = (u32PicLStride * u32OutHeight);
    u32ChrmSize = (u32PicCStride * u32OutHeight) >> 2;

    stRgnAttr.enType = OVERLAYEX_RGN;

    stRgnAttr.unAttr.stOverlayEx.enPixelFmt = enPixelFormat;
    stRgnAttr.unAttr.stOverlayEx.stSize.u32Width  = 320;
    stRgnAttr.unAttr.stOverlayEx.stSize.u32Height = 320;
    stRgnAttr.unAttr.stOverlayEx.u32BgColor = 0x80fc;

    s32Ret = HI_MPI_RGN_Create(Handle, &stRgnAttr);
    if(s32Ret != HI_SUCCESS)
    {
       	SAMPLE_PRT("HI_MPI_RGN_Create failed\n");
        goto END2;
    }
    
    
    filename = "mm.bmp";

    VGS_SAMPLE_LoadBmp(filename, &stBitmap, HI_FALSE, 0x0,enPixelFormat);
	if(s32Ret != HI_SUCCESS)
    {
       	SAMPLE_PRT("VGS_SAMPLE_LoadBmp failed\n");
		free(stBitmap.pData);
        goto END2;
    }
		
    stBitmap.enPixelFormat = enPixelFormat;
    HI_MPI_RGN_SetBitMap(Handle, &stBitmap);
	if(s32Ret != HI_SUCCESS)
    {
       	SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed\n");
		free(stBitmap.pData);
        goto END2;
    }

    free(stBitmap.pData);
    stBitmap.pData = HI_NULL;

	s32Ret = HI_MPI_RGN_GetCanvasInfo(Handle, &stCanvasInfo);
	if(s32Ret != HI_SUCCESS)
    {
       	SAMPLE_PRT("HI_MPI_RGN_AttachToChn failed\n");
        goto END2;
    }
	

    for(i =0;i<10;i++)
    {
        //SAMPLE_MEMBUF_S g_stMem = {0};
        VIDEO_FRAME_INFO_S stFrmInfo;
        VGS_HANDLE hHandle;
        g_stMem.hPool = hPool;
        //VIDEO_FRAME_INFO_S stFrameInfo;
        VGS_TASK_ATTR_S stTask;
        s32Ret = HI_MPI_VDEC_GetImage(0, &g_stFrameInfo, -1);
        if(s32Ret != HI_SUCCESS)
        {	
            SAMPLE_PRT("get vdec image failed\n");
            goto END3;
        }

		g_bUserVdecBuf = HI_TRUE;
		
        while((g_stMem.hBlock = HI_MPI_VB_GetBlock(g_stMem.hPool, u32BlkSize,NULL)) == VB_INVALID_HANDLE)
        {
            ;
        }

		g_bUserCommVb = HI_TRUE;
  
        g_stMem.u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(g_stMem.hBlock);

        g_stMem.pVirAddr = (HI_U8 *) HI_MPI_SYS_Mmap( g_stMem.u32PhyAddr, u32BlkSize );
        if(g_stMem.pVirAddr == NULL)
        {
            SAMPLE_PRT("Mem dev may not open\n");
            HI_MPI_VB_ReleaseBlock(g_stMem.hBlock);
			g_bUserCommVb = HI_FALSE;
            HI_MPI_VDEC_ReleaseImage(0, &g_stFrameInfo);
			g_bUserVdecBuf = HI_FALSE;
            goto END3;
        }
   
        memset(&stFrmInfo.stVFrame, 0, sizeof(VIDEO_FRAME_S));
        stFrmInfo.stVFrame.u32PhyAddr[0] = g_stMem.u32PhyAddr;
        stFrmInfo.stVFrame.u32PhyAddr[1] = stFrmInfo.stVFrame.u32PhyAddr[0] + u32LumaSize;
        stFrmInfo.stVFrame.u32PhyAddr[2] = stFrmInfo.stVFrame.u32PhyAddr[1] + u32ChrmSize;


        stFrmInfo.stVFrame.pVirAddr[0] = g_stMem.pVirAddr;
        stFrmInfo.stVFrame.pVirAddr[1] = (HI_U8 *) stFrmInfo.stVFrame.pVirAddr[0] + u32LumaSize;
        stFrmInfo.stVFrame.pVirAddr[2] = (HI_U8 *) stFrmInfo.stVFrame.pVirAddr[1] + u32ChrmSize;
 
        stFrmInfo.stVFrame.u32Width     = u32OutWidth;
        stFrmInfo.stVFrame.u32Height    = u32OutHeight;
        stFrmInfo.stVFrame.u32Stride[0] = u32PicLStride;
        stFrmInfo.stVFrame.u32Stride[1] = u32PicLStride;
        stFrmInfo.stVFrame.u32Stride[2] = u32PicLStride;

        stFrmInfo.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
        stFrmInfo.stVFrame.enPixelFormat  = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        stFrmInfo.stVFrame.enVideoFormat  = VIDEO_FORMAT_LINEAR;

        stFrmInfo.stVFrame.u64pts     = (i * 40);
        stFrmInfo.stVFrame.u32TimeRef = (i * 2);

        stFrmInfo.u32PoolId = hPool;

        s32Ret = HI_MPI_VGS_BeginJob(&hHandle);
        if(s32Ret != HI_SUCCESS)
    	{	
    	    SAMPLE_PRT("HI_MPI_VGS_BeginJob failed %#x\n", s32Ret);
            HI_MPI_VB_ReleaseBlock(g_stMem.hBlock);
			g_bUserCommVb = HI_FALSE;
            HI_MPI_VDEC_ReleaseImage(0, &g_stFrameInfo);
			g_bUserVdecBuf = HI_FALSE;
    	    goto END3;
    	}
   
        memcpy(&stTask.stImgIn,&g_stFrameInfo,sizeof(VIDEO_FRAME_INFO_S));
        
        memcpy(&stTask.stImgOut ,&stFrmInfo,sizeof(VIDEO_FRAME_INFO_S));
        s32Ret = HI_MPI_VGS_AddScaleTask(hHandle, &stTask);
        if(s32Ret != HI_SUCCESS)
        {	
            SAMPLE_PRT("HI_MPI_VGS_AddScaleTask failed\n");
            HI_MPI_VGS_CancelJob(hHandle);
            HI_MPI_VB_ReleaseBlock(g_stMem.hBlock);
			g_bUserCommVb = HI_FALSE;
            HI_MPI_VDEC_ReleaseImage(0, &g_stFrameInfo);
			g_bUserVdecBuf = HI_FALSE;
            goto END3;
        }

		
		memcpy(&stTask.stImgIn, &stTask.stImgOut, sizeof(VIDEO_FRAME_INFO_S));
		stVgsAddCover[0].enCoverType = COVER_RECT;
		stVgsAddCover[0].stDstRect.s32X = 0;
		stVgsAddCover[0].stDstRect.s32Y = 0;
		stVgsAddCover[0].stDstRect.u32Width  = 100;
		stVgsAddCover[0].stDstRect.u32Height = 100;
		stVgsAddCover[0].u32Color = 0xff;

		stVgsAddCover[1].enCoverType = COVER_RECT;
		stVgsAddCover[1].stDstRect.s32X = 100;
		stVgsAddCover[1].stDstRect.s32Y = 100;
		stVgsAddCover[1].stDstRect.u32Width  = 100;
		stVgsAddCover[1].stDstRect.u32Height = 100;
		stVgsAddCover[1].u32Color = 0xff00;

		stVgsAddCover[2].enCoverType = COVER_RECT;
		stVgsAddCover[2].stDstRect.s32X = 200;
		stVgsAddCover[2].stDstRect.s32Y = 200;
		stVgsAddCover[2].stDstRect.u32Width  = 100;
		stVgsAddCover[2].stDstRect.u32Height = 100;
		stVgsAddCover[2].u32Color = 0xff0000;

		stVgsAddCover[3].enCoverType = COVER_RECT;
		stVgsAddCover[3].stDstRect.s32X = 300;
		stVgsAddCover[3].stDstRect.s32Y = 300;
		stVgsAddCover[3].stDstRect.u32Width  = 400;
		stVgsAddCover[3].stDstRect.u32Height = 400;
		stVgsAddCover[3].u32Color = 0xff0;

		s32Ret = HI_MPI_VGS_AddCoverTask(hHandle, &stTask, stVgsAddCover, 4);
        if(s32Ret != HI_SUCCESS)
        {	
            SAMPLE_PRT("HI_MPI_VGS_AddCoverTask failed\n");
            HI_MPI_VGS_CancelJob(hHandle);
            HI_MPI_VB_ReleaseBlock(g_stMem.hBlock);
			g_bUserCommVb = HI_FALSE;
            HI_MPI_VDEC_ReleaseImage(0, &g_stFrameInfo);
			g_bUserVdecBuf = HI_FALSE;
            goto END3;
        }
		
		stVgsAddOsd[0].stRect.s32X = 300;
		stVgsAddOsd[0].stRect.s32Y = 300;
		stVgsAddOsd[0].stRect.u32Width = 200;
		stVgsAddOsd[0].stRect.u32Height = 200;
		stVgsAddOsd[0].u32BgAlpha = 255;
		stVgsAddOsd[0].u32FgAlpha = 255;
		stVgsAddOsd[0].u32BgColor = 0x80fc;
		stVgsAddOsd[0].u32PhyAddr = stCanvasInfo.u32PhyAddr;
		stVgsAddOsd[0].u32Stride  = stCanvasInfo.u32Stride;
		stVgsAddOsd[0].enPixelFmt = enPixelFormat;

		stVgsAddOsd[1].stRect.s32X = 500;
		stVgsAddOsd[1].stRect.s32Y = 500;
		stVgsAddOsd[1].stRect.u32Width = 200;
		stVgsAddOsd[1].stRect.u32Height = 200;
		stVgsAddOsd[1].u32BgAlpha = 255;
		stVgsAddOsd[1].u32FgAlpha = 255;
		stVgsAddOsd[1].u32BgColor = 0x8fc0;
		stVgsAddOsd[1].u32PhyAddr = stCanvasInfo.u32PhyAddr;
		stVgsAddOsd[1].u32Stride  = stCanvasInfo.u32Stride;
		stVgsAddOsd[1].enPixelFmt = enPixelFormat;

		stVgsAddOsd[2].stRect.s32X = 800;
		stVgsAddOsd[2].stRect.s32Y = 400;
		stVgsAddOsd[2].stRect.u32Width = 320;
		stVgsAddOsd[2].stRect.u32Height = 320;
		stVgsAddOsd[2].u32BgAlpha = 255;
		stVgsAddOsd[2].u32FgAlpha = 255;
		stVgsAddOsd[2].u32BgColor = 0x8ffc;
		stVgsAddOsd[2].u32PhyAddr = stCanvasInfo.u32PhyAddr;
		stVgsAddOsd[2].u32Stride  = stCanvasInfo.u32Stride;
		stVgsAddOsd[2].enPixelFmt = enPixelFormat;

		s32Ret = HI_MPI_VGS_AddOsdTask(hHandle, &stTask, stVgsAddOsd, 3);
		if(s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VGS_AddOsdTask failed\n");
            HI_MPI_VGS_CancelJob(hHandle);
            HI_MPI_VB_ReleaseBlock(g_stMem.hBlock);
			g_bUserCommVb = HI_FALSE;
            HI_MPI_VDEC_ReleaseImage(0, &g_stFrameInfo);
			g_bUserVdecBuf = HI_FALSE;
            goto END3;
	    }

		stVgsLine[0].stStartPoint.s32X = 800;
		stVgsLine[0].stStartPoint.s32Y = 100;
		stVgsLine[0].stEndPoint.s32X = 800;
		stVgsLine[0].stEndPoint.s32Y = 500;
		stVgsLine[0].u32Color = 0xff0000;
		stVgsLine[0].u32Thick = 12;

		stVgsLine[1].stStartPoint.s32X = 900;
		stVgsLine[1].stStartPoint.s32Y = 100;
		stVgsLine[1].stEndPoint.s32X = 900;
		stVgsLine[1].stEndPoint.s32Y = 500;
		stVgsLine[1].u32Color = 0xff0000;
		stVgsLine[1].u32Thick = 12;

		stVgsLine[2].stStartPoint.s32X = 800;
		stVgsLine[2].stStartPoint.s32Y = 300;
		stVgsLine[2].stEndPoint.s32X = 900;
		stVgsLine[2].stEndPoint.s32Y = 300;
		stVgsLine[2].u32Color = 0xff0000;
		stVgsLine[2].u32Thick = 12;

		stVgsLine[3].stStartPoint.s32X = 1000;
		stVgsLine[3].stStartPoint.s32Y = 100;
		stVgsLine[3].stEndPoint.s32X = 1000;
		stVgsLine[3].stEndPoint.s32Y = 120;
		stVgsLine[3].u32Color = 0xff0000;
		stVgsLine[3].u32Thick = 12;

		stVgsLine[4].stStartPoint.s32X = 1000;
		stVgsLine[4].stStartPoint.s32Y = 200;
		stVgsLine[4].stEndPoint.s32X = 1000;
		stVgsLine[4].stEndPoint.s32Y = 500;
		stVgsLine[4].u32Color = 0xff0000;
		stVgsLine[4].u32Thick = 12;
		
		s32Ret = HI_MPI_VGS_AddDrawLineTask(hHandle, &stTask, stVgsLine, 5);
		if(s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VGS_AddDrawLineTask failed\n");
            HI_MPI_VGS_CancelJob(hHandle);
            HI_MPI_VB_ReleaseBlock(g_stMem.hBlock);
			g_bUserCommVb = HI_FALSE;
            HI_MPI_VDEC_ReleaseImage(0, &g_stFrameInfo);
			g_bUserVdecBuf = HI_FALSE;
            goto END3;
	    }
		

        s32Ret = HI_MPI_VGS_EndJob(hHandle);
        if(s32Ret != HI_SUCCESS)
        {	
            SAMPLE_PRT("HI_MPI_VGS_EndJob failed\n");
            HI_MPI_VGS_CancelJob(hHandle);
            HI_MPI_VB_ReleaseBlock(g_stMem.hBlock);
			g_bUserCommVb = HI_FALSE;
            HI_MPI_VDEC_ReleaseImage(0, &g_stFrameInfo);
			g_bUserVdecBuf = HI_FALSE;
            goto END3;
        }

        /*Save the yuv*/
        SAMPLE_VGS_SaveSP42XToPlanar(fpYuv, &stFrmInfo.stVFrame);
        fflush(fpYuv);
        HI_MPI_SYS_Munmap(g_stMem.pVirAddr,u32BlkSize);
        HI_MPI_VB_ReleaseBlock(g_stMem.hBlock);
		g_bUserCommVb = HI_FALSE;
        HI_MPI_VDEC_ReleaseImage(0, &g_stFrameInfo);
		g_bUserVdecBuf = HI_FALSE;
        printf("\rfinish saving picure : %d. ", i+1);
        fflush(stdout);   

    }
    
    SAMPLE_COMM_VDEC_StopSendStream(1, &stVdecSend[0], &VdecThread[0]);

    
END3:
    SAMPLE_COMM_VDEC_Stop(1);		

END2:
    HI_MPI_VB_DestroyPool( hPool );

END1:
    SAMPLE_COMM_SYS_Exit();	
    
    fclose(fpYuv);
    
    return s32Ret;
}


/******************************************************************************
* function    : main()
* Description : vgs sample
******************************************************************************/
int main(int argc, char *argv[])
{
    HI_S32 s32Ret = HI_SUCCESS;
    int ch;
    HI_BOOL bExit = HI_FALSE;
    signal(SIGINT, SAMPLE_VGS_HandleSig);
    signal(SIGTERM, SAMPLE_VGS_HandleSig);
  
    /******************************************
    1 choose the case
    ******************************************/
    while (1)
    {
        SAMPLE_VGS_Usage();
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
                SAMPLE_VGS_Decompress_TilePicture();
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
                printf("input invaild! please try again.\n");
                break;
            }
        }
        
        if (bExit)
        {
            break;
        }
    }

    return s32Ret;
}

