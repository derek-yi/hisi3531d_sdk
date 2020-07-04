#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "mpi_sys.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "hi_comm_vo.h"
#include "mpi_vo.h"

static VIDEO_FRAME_INFO_S g_stFrame;
static char* g_pVBufVirt_Y = NULL;
static char* g_pVBufVirt_C = NULL;
static HI_U32 g_Ysize, g_Csize;
static FILE* g_pfd = NULL;
static VO_LAYER g_VoLayer = 0;



static void usage(void)
{
    printf(
        "\n"
        "*************************************************\n"
        "Usage: ./vou_screen_dump [VoLayer] [Frmcnt].\n"
        "1)VoLayer: \n"
        "   which layer to be dump\n"
        "   Default: 0\n"
        "2)FrmCnt: \n"
        "   the count of frame to be dump\n"
        "   Default: 1\n"
        "*)Example:\n"
        "   e.g : ./vou_screen_dump 0 1 (dump one YUV)\n"
        "*************************************************\n"
        "\n");
}


/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void VOU_TOOL_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {
        if (0 != g_stFrame.stVFrame.u32PhyAddr[0])
        {
            HI_MPI_VO_ReleaseScreenFrame(g_VoLayer, &g_stFrame);
            memset(&g_stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        }

        if (NULL != g_pVBufVirt_Y)
        {
            HI_MPI_SYS_Munmap(g_pVBufVirt_Y, g_Ysize);
            g_pVBufVirt_Y = NULL;
        }

        if (NULL != g_pVBufVirt_C)
        {
            HI_MPI_SYS_Munmap(g_pVBufVirt_C, g_Csize);
            g_pVBufVirt_C = NULL;
        }

        if (NULL != g_pfd)
        {
            fclose(g_pfd);
            g_pfd = NULL;
        }

        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

/* sp420 to p420 ; sp422 to p422  */
void sample_yuv_dump(VIDEO_FRAME_S * pVBuf, FILE *pfd)
{
    unsigned int w, h;
    char * pMemContent;
    unsigned char TmpBuff[4096];                
    HI_U32 phy_addr;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    HI_U32 u32UvHeight;/* uv height when saved for planar type */

    g_Ysize = (pVBuf->u32Stride[0])*(pVBuf->u32Height);
    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat)
    {
       g_Csize = (pVBuf->u32Stride[1])*(pVBuf->u32Height)/2;    
        u32UvHeight = pVBuf->u32Height/2;
    }
    else if (PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixelFormat)
    {
        g_Csize = (pVBuf->u32Stride[1]) * (pVBuf->u32Height);
        u32UvHeight = pVBuf->u32Height;
    }
    else
    {
        g_Csize = 0;
        u32UvHeight = 0;
    }
    
    phy_addr = pVBuf->u32PhyAddr[0];
    g_pVBufVirt_Y = (HI_CHAR *) HI_MPI_SYS_Mmap(phy_addr, g_Ysize);	
    if (NULL == g_pVBufVirt_Y)
    {
        return;
    }

    g_pVBufVirt_C = (HI_CHAR *) HI_MPI_SYS_Mmap(pVBuf->u32PhyAddr[1], g_Csize);
    if (NULL == g_pVBufVirt_C)
    {
        HI_MPI_SYS_Munmap(g_pVBufVirt_Y, g_Ysize); 
        return;
    }

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Y......");
    fflush(stderr);
    for(h=0; h<pVBuf->u32Height; h++)
    {
        pMemContent = g_pVBufVirt_Y + h*pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
    }
    fflush(pfd);
    
    /* save U ----------------------------------------------------------------*/
    fprintf(stderr, "U......");
    fflush(stderr);
    for(h=0; h<u32UvHeight; h++)
    {
        pMemContent = g_pVBufVirt_C + h*pVBuf->u32Stride[1];

        pMemContent += 1;

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);

    /* save V ----------------------------------------------------------------*/
    fprintf(stderr, "V......");
    fflush(stderr);
    for(h=0; h<u32UvHeight; h++)    
    {
        pMemContent = g_pVBufVirt_C + h*pVBuf->u32Stride[1];

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);
    
    //fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fprintf(stderr, "done !\n");
    fflush(stderr);
    
    HI_MPI_SYS_Munmap(g_pVBufVirt_Y, g_Ysize);
    g_pVBufVirt_Y = NULL;
	HI_MPI_SYS_Munmap(g_pVBufVirt_C, g_Csize);
	g_pVBufVirt_C = NULL;

}

HI_S32 SAMPLE_MISC_VoDump(VO_LAYER VoLayer, HI_U32 u32Cnt)
{
    HI_S32 i, s32Ret;
    HI_CHAR szYuvName[128]={0};
    HI_CHAR szPixFrm[10]={0};

   // printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    //printf("usage: ./vo_screen_dump [VoLayer] [frmcnt]. sample: ./vo_screen_dump 0 1\n\n");

    /* Get Frame to make file name*/
    s32Ret = HI_MPI_VO_GetScreenFrame(VoLayer, &g_stFrame, 0);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VO(%d)_GetScreenFrame errno %#x\n", VoLayer, s32Ret);
        return -1;
    }

    /* make file name */
    strcpy(szPixFrm,
    (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == g_stFrame.stVFrame.enPixelFormat)?"p420":"p422");
    sprintf(szYuvName, "./vo(%d)_%s_%u_%ux%u.yuv",VoLayer,
        szPixFrm,u32Cnt,g_stFrame.stVFrame.u32Width, g_stFrame.stVFrame.u32Height);
    printf("Dump YUV frame of vo(%d) to file: \"%s\"\n",VoLayer, szYuvName);

    HI_MPI_VO_ReleaseScreenFrame(VoLayer, &g_stFrame);

    /* open file */
    g_pfd = fopen(szYuvName, "wb");

    if (NULL == g_pfd)
    {
        return -1;
    }

    /* get VO frame  */
    for (i=0; i<u32Cnt; i++)
    {
        s32Ret = HI_MPI_VO_GetScreenFrame(VoLayer, &g_stFrame, 0);
        if (HI_SUCCESS != s32Ret)
        {
            printf("get vo(%d) frame err\n", VoLayer);
            printf("only get %d frame\n", i);
            break;
        }
        printf("addr 0x%x\n", g_stFrame.stVFrame.u32PhyAddr[0]);
        /* save VO frame to file */
		sample_yuv_dump(&g_stFrame.stVFrame, g_pfd);

        /* release frame after using */
        s32Ret = HI_MPI_VO_ReleaseScreenFrame(VoLayer, &g_stFrame);
        if (HI_SUCCESS != s32Ret)
        {
            printf("Release vo(%d) frame err\n", VoLayer);
            printf("only get %d frame\n", i);
            break;
        }
    }

    fclose(g_pfd);

	return 0;
}

HI_S32 main(int argc, char *argv[])
{
    HI_S32 s32FrmCnt = 1;
    HI_U32 u32FrmCnt = 1;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    #ifndef __HuaweiLite__
    printf("\tTo see more usage, please enter: ./vou_screen_dump -h\n\n");
    #else
    printf("\tTo see more usage, please enter: vou_screen_dump -h\n\n");
    #endif

    if (argc > 1)
    {
        if (!strncmp(argv[1], "-h", 2))
        {
            usage();
            exit(HI_SUCCESS);
        }
        g_VoLayer = atoi(argv[1]);
    }
	/* video layer ID*/
    if (argc > 3)
    {
        printf("Too many parameters!\n");
        return HI_FAILURE;
    }
	/* the numbers of sampling */
    if (argc > 2)
    {
        s32FrmCnt = atoi(argv[2]);
        if (s32FrmCnt <= 0)
        {
            printf("The Frmcnt(%d) is wrong!\n", s32FrmCnt);
            return HI_FAILURE;
        }

        if(s32FrmCnt >= 2147483647)
        {
            printf("frmcnt data overflow !\n");
            return HI_FAILURE;
        }
    }

    signal(SIGINT, VOU_TOOL_HandleSig);
    signal(SIGTERM, VOU_TOOL_HandleSig);
    u32FrmCnt = s32FrmCnt;
    SAMPLE_MISC_VoDump(g_VoLayer, u32FrmCnt);

	return HI_SUCCESS;
}

