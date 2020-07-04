#include "stdlib.h"
#include <sys/time.h>
#include "hdmi_test_cmd.h"
#include "string.h"
#include "hdmi_mst.h"
#include "hi_type.h"

#define HDMI_MAX_ARGC       20
#define HDMI_MAX_PAMRM_SIZE 30

/* Variable */
static HI_U8 args[HDMI_MAX_ARGC][HDMI_MAX_PAMRM_SIZE];
static HDMI_INPUT_PARAM_S Hdmi_V_Timing[] =
{
    {VO_OUTPUT_1080P24,   "1080P_24",},      /**<1080p24 HZ*/
    {VO_OUTPUT_1080P25,   "1080P_25",},      /**<1080p25 HZ*/
    {VO_OUTPUT_1080P30,   "1080P_30",},      /**<1080p30 HZ*/
    {VO_OUTPUT_720P50,     "720P_50",},      /**<720p50 HZ */
    {VO_OUTPUT_720P60,    "720P_60",},       /**<720p60 HZ */
    {VO_OUTPUT_1080I50,   "1080i_50",},      /**<1080i50 HZ */
    {VO_OUTPUT_1080I60,   "1080i_60",},      /**<1080i60 HZ */
    {VO_OUTPUT_1080P50,   "1080P_50",},      /**<1080p50 HZ*/
    {VO_OUTPUT_1080P60,   "1080P_60",},      /**<1080p60 HZ*/

    {VO_OUTPUT_576P50,    "576P_50",},       /**<576p50 HZ */
    {VO_OUTPUT_480P60,    "480P_60",},       /**<480p60 HZ */
    {VO_OUTPUT_640x480_60, "640x480p_60",},  /* 640x480        */
    {VO_OUTPUT_800x600_60, "800x600",},        /* 800x600       */
    {VO_OUTPUT_1024x768_60, "1024X768",},      /* 1024X768       */
    {VO_OUTPUT_1280x1024_60, "1280X1024",},    /* 1280X1024      */
    {VO_OUTPUT_1366x768_60, "1366X768",},      /* 1366X768       */
    {VO_OUTPUT_1440x900_60, "1440X900",},      /* 1440X900       */
    {VO_OUTPUT_1280x800_60, "1280X800",},      /* 1280X800    */
    {VO_OUTPUT_1680x1050_60, "1680X1050",},    /* 1680X1050      */
    {VO_OUTPUT_1920x2160_30, "1920X2160",},    /* 1920X2160      */
    {VO_OUTPUT_1600x1200_60, "1600X1200",},    /* 1600X1200      */
    {VO_OUTPUT_1920x1200_60, "1920X1200",},    /* 1920X1200      */
    {VO_OUTPUT_2560x1440_30, "2560X1440_30",},    /* 2560X1440      */
    {VO_OUTPUT_2560x1600_60, "2560X1600",},    /* 2560X1600      */
    {VO_OUTPUT_3840x2160_30, "3840X2160_30",},    /* 4Kx2K_30      */
    {VO_OUTPUT_3840x2160_60, "3840X2160_60",},    /* 4Kx2K_60      */
    {VO_OUTPUT_2560x1440_60, "2560X1440_60"},
    {VO_OUTPUT_3840x2160_30, "3840X2160_25",},    /* 4Kx2K_25      */
    {VO_OUTPUT_3840x2160_50, "3840X2160_50",},    /* 4Kx2K_50      */
    {VO_OUTPUT_BUTT,       "BUTT",},          /* MAX */
};

#define HDMI_MAX_COLOR_MODE  4
static HDMI_INPUT_PARAM_S Hdmi_V_ColorMode[HDMI_MAX_COLOR_MODE] =
{
    {0,   "RGB444",},        /* RGB444 Mode */
    {1,   "YCbCr422",},      /* YCbCr422 Mode */
    {2,   "YCbCr444",},      /* YCbCr444 Mode */
    {3,   "YCbCr420",},      /* YCbCr420 Mode */
};

/*
** Add aspect ratio @2015.08.03
*/
#define HDMI_MAX_ASPECTRATIO  4
static HDMI_INPUT_PARAM_S Hdmi_V_AspectRatio[HDMI_MAX_ASPECTRATIO] =
{
    {1,   "4:3"},       /* RGB444 Mode   */
    {2,   "16:9"},      /* YCbCr422 Mode */
    {3,   "64:27"},      /* 64:27 Mode    */
    {4,   "256:135",},    /* 256:135 Mode  */
};

static HI_HDMI_ID_E enTest_Hdmi_ID = HI_HDMI_ID_0;

extern VO_DEV g_VoDev;
extern VO_LAYER g_VoLayer;
extern HI_S32 g_s32ChnNum;

#define HDMI_MAX_AUDIO_FREQNCE    3
//#define HDMI_MAX_AUDIO_FREQNCE          48000

static HDMI_INPUT_PARAM_S Hdmi_A_Freq[HDMI_MAX_AUDIO_FREQNCE] =
{
    {0, "32000"},
    {1, "44100"},
    {2, "48000"},
};

static HI_U8  Hdmi_AdjustString(HI_CHAR *ptr);
static HI_U32 Hdmi_ParseArg(HI_U32 argc);
static HI_U32 hdmi_hdmi_force(void);
static HI_U32 hdmi_dvi_force(void);
HI_U32 hdmi_video_timing(HI_U32 u32TimingIndex);
static HI_U32 hdmi_color_mode(HI_U32 u32ColorIndex);
static HI_U32 hdmi_aspectratio(HI_U32 u32apectrateIndex);
static HI_U32 hdmi_authmode(HI_BOOL bauthmodeEnable);
HI_U32 hdmi_deepcolor(HI_U32 u32DeepColorFlag);
static HI_U32 hdmi_a_freq(HI_U8 u8AudioFreqIndex);


static HI_U32 Hdmi_String_to_Integer(HI_U8 *ptr)
{
    HI_U32 index, stringLen = 0;

    if (NULL == ptr)
    {
        return 0xFFFFFFFF;
    }
    stringLen = strlen((char *)ptr);

    if(stringLen > 16)
    {
        return 0xFFFFFFFF;
    }

    for (index = 0; index < stringLen; index ++)
    {
        if ( (ptr[index] >= '0') && (ptr[index] <= '9') )
        {
            continue;
        }
        return 0xFFFFFFFF;
    }

    return (atoi((char *)ptr));
}

/*adjust input parameter*/
static HI_U8 Hdmi_AdjustString(HI_CHAR *ptr)
{
    HI_U32 i;

    /* Search the first charater char */
    while(*ptr==' ' && *ptr++ != '\0')
    {
        ;
    }

    /* change to little case charater */
    for(i=strlen((char *)ptr); i>0; i--)
    {
        if ((*(ptr+i-1) >= 'A') && (*(ptr+i-1) <= 'Z'))
        {
            *(ptr+i-1) = (*(ptr+i-1) - 'A') + 'a';
        }
    }

    for(i=strlen((char *)ptr);i>0;i--)
    {
        if (*(ptr+i-1) == 0x0a || *(ptr+i-1) == ' ')
        {
            *(ptr+i-1) = '\0';
        }
        else
        {
            break;
        }
    }

    for(i=0; i<HDMI_MAX_ARGC; i++)
    {
        memset(args[i], 0, HDMI_MAX_PAMRM_SIZE);
    }
    /* fill args[][] with input string */
    for(i=0; i<HDMI_MAX_ARGC; i++)
    {
        HI_U8 j = 0;
        while(*ptr==' ' && *ptr++ != '\0')
            ;

        while((*ptr !=' ') && (*ptr!='\0'))
        {
            args[i][j++] = *ptr++;
        }

        args[i][j] = '\0';
        if ('\0' == *ptr)
        {
            i++;
            break;
        }
        args[i][j] = '\0';
    }

    return i;
}

static HI_U32 Hdmi_ParseArg(HI_U32 argc)
{
    HI_U32 i, index;
    HI_U32 RetError = HI_SUCCESS;

    printf("input parameter Num:%d ", argc);
    for (i = 0 ; i < argc; i++)
    {
        printf("argv[%d]:%s, ", i, args[i]);
    }
    printf("\n");

    /* Parse HDMI input command */
    if ( (0 == strcmp("help", (char *)args[0])) || (0 == strcmp("h", (char *)args[0])) )
    {
        printf("\t List all testtool command\n");
        printf("\t help                 list all command we provide\n");
        printf("\t q                    quit sample test\n");
        printf("\t hdmi_hdmi_force      force to hdmi output\n");
        printf("\t hdmi_dvi_force       force to enter dvi output mode\n");
        printf("\t hdmi_deepcolor       set video deepcolor mode\n");
        printf("\t hdmi_video_timing    set video output timing format\n");
        printf("\t hdmi_color_mode      set video color output(RGB/YCbCr)\n");
        printf("\t hdmi_aspectratio     set video aspectratio\n");
        printf("\t hdmi_a_freq          set audio output frequence\n");

        printf("\t hdmi_authmode        authmode enable or disable\n");
    }
    else if (0 == strcmp("hdmi_hdmi_force", (char *)args[0]))
    {
        /* force to Output hdmi signal */
        RetError = hdmi_hdmi_force();
    }
    else if (0 == strcmp("hdmi_dvi_force", (char *)args[0]))
    {
        /* force to Output dvi signal */
        RetError = hdmi_dvi_force();
    }
    else if (0 == strcmp("hdmi_deepcolor", (char *)args[0]))
    {
        HI_U8 u8DeepColorIndex;
        /* set video Deep color */
        if (1 >= argc)
        {
            printf("Usage:hdmi_deepcolor DeepColorflag\n");
            printf("DeepColorflag: 0:Deep Color 24bit\n");
            printf("DeepColorflag: 1:Deep Color 30bit\n");
            printf("DeepColorflag: 2:Deep Color 36bit\n");
            printf("DeepColorflag: 3:Deep Color Off\n");
            return RetError;
        }
        u8DeepColorIndex = (HI_U8)Hdmi_String_to_Integer(args[1]);
        if (u8DeepColorIndex > HI_HDMI_DEEP_COLOR_36BIT + 1)
        {
            printf("input deep color mode is larger than Max index:%d\n", HI_HDMI_DEEP_COLOR_36BIT + 1);
            return HI_FAILURE;
        }
        if(u8DeepColorIndex == 3)
        {
            u8DeepColorIndex = HI_HDMI_DEEP_COLOR_OFF;
        }
        RetError = hdmi_deepcolor(u8DeepColorIndex);
    }
    else if (0 == strcmp("hdmi_video_timing", (char *)args[0]))
    {
        HI_U32 timing_index;
        HI_U32 u32Len = (sizeof(Hdmi_V_Timing) / sizeof(HDMI_INPUT_PARAM_S))-1;
        /* change video timing format: 720p@60, 1080i@50, ... */
        if (2 > argc)
        {
            printf("Usage:hdmi_video_timing timeingmode\n");
            for(index = 0; index < u32Len; index++)
            {
                printf("timemode:%02d   %s\n", index, Hdmi_V_Timing[index].Index_String);
            }

            return RetError;
        }
        timing_index = (HI_U8)Hdmi_String_to_Integer(args[1]);
        if (timing_index > (u32Len - 1))
        {
            printf("input timing mode is larger than Max index:%d\n", (u32Len - 1));
            return HI_FAILURE;
        }

        if(timing_index == 0)
        {
            printf("1080P24\n");
            timing_index = VO_OUTPUT_1080P24;
        }
        else if(timing_index == 1)
        {
            printf("1080P25\n");
            timing_index = VO_OUTPUT_1080P25;
        }
        else if(timing_index == 2)
        {
            printf("1080P30\n");
            timing_index = VO_OUTPUT_1080P30;
        }
        else if(timing_index == 3)
        {
            printf("720P50\n");
            timing_index = VO_OUTPUT_720P50;
        }
        else if(timing_index == 4)
        {
            printf("720P60\n");
            timing_index = VO_OUTPUT_720P60;
        }
        else if(timing_index == 5)
        {
            printf("1080I50\n");
            timing_index = VO_OUTPUT_1080I50;
        }
        else if(timing_index == 6)
        {
            printf("1080I60\n");
            timing_index = VO_OUTPUT_1080I60;
        }
        else if(timing_index == 7)
        {
            printf("1080P50\n");
            timing_index = VO_OUTPUT_1080P50;
        }
        else if(timing_index == 8)
        {
            printf("1080P60\n");
            timing_index = VO_OUTPUT_1080P60;
        }
        else if(timing_index == 9)
        {
            printf("576P50\n");
            timing_index = VO_OUTPUT_576P50;
        }
        else if(timing_index == 10)
        {
            printf("480P60\n");
            timing_index = VO_OUTPUT_480P60;
        }
        else if(timing_index == 11)
        {
            printf("640X480\n");
            timing_index = VO_OUTPUT_640x480_60;
        }
        else if(timing_index == 12)
        {
            printf("800X600\n");
            timing_index = VO_OUTPUT_800x600_60;
        }
        else if(timing_index == 13)
        {
            printf("1024X768\n");
            timing_index = VO_OUTPUT_1024x768_60;
        }
        else if(timing_index == 14)
        {
            printf("1280X1024\n");
            timing_index = VO_OUTPUT_1280x1024_60;
        }
        else if(timing_index == 15)
        {
            printf("1366X768\n");
            timing_index = VO_OUTPUT_1366x768_60;
        }
        else if(timing_index == 16)
        {
            printf("1440X900\n");
            timing_index = VO_OUTPUT_1440x900_60;
        }
        else if(timing_index == 17)
        {
            printf("1280X800\n");
            timing_index = VO_OUTPUT_1280x800_60;
        }
        else if(timing_index == 18)
        {
            printf("1680X1050\n");
            timing_index = VO_OUTPUT_1680x1050_60;
        }
        else if(timing_index == 19)
        {
            printf("1920X2160\n");
            timing_index = VO_OUTPUT_1920x2160_30;
        }
        else if(timing_index == 20)
        {
            printf("1600X1200\n");
            timing_index = VO_OUTPUT_1600x1200_60;
        }
        else if(timing_index == 21)
        {
            printf("1920X1200\n");
            timing_index = VO_OUTPUT_1920x1200_60;
        }
        else if(timing_index == 22)
        {
            printf("2560X1440_30\n");
            timing_index = VO_OUTPUT_2560x1440_30;
        }
        else if(timing_index == 23)
        {
            printf("2560X1600\n");
            timing_index = VO_OUTPUT_2560x1600_60;
        }
        else if(timing_index == 24)
        {
            printf("3840X2160_30\n");
            timing_index = VO_OUTPUT_3840x2160_30;
        }
        else if(timing_index == 25)
        {
            printf("3840X2160_60\n");
            timing_index = VO_OUTPUT_3840x2160_60;
        }
        else if(timing_index == 26)
        {
            printf("2560X1440_60\n");
            timing_index = VO_OUTPUT_2560x1440_60;
        }
        else if(timing_index == 27)
        {
            printf("3840X2160_25\n");
            timing_index = VO_OUTPUT_3840x2160_25;
        }
        else if(timing_index == 28)
        {
            printf("3840X2160_50\n");
            timing_index = VO_OUTPUT_3840x2160_50;
        }
        else if(timing_index == 35)
        {
            printf("user defined timing\n");
        }

        RetError = hdmi_video_timing(timing_index);
    }
    else if (0 == strcmp("hdmi_color_mode", (char *)args[0]))
    {
        HI_U8 color_index;
        /* change video color mode: RGB444, YCbCr422, YCbCr444, ... */
        if (1 >= argc)
        {
            printf("Usage:hdmi_color_mode colormode\n");
            for(index = 0; index < HDMI_MAX_COLOR_MODE; index ++)
            {
                printf("colormode:%02d   %s\n", index, Hdmi_V_ColorMode[index].Index_String);
            }
            return RetError;
        }
        color_index = (HI_U8)Hdmi_String_to_Integer(args[1]);
        if (color_index >= HDMI_MAX_COLOR_MODE)
        {
            printf("input color mode is larger than Max index:%d\n", HDMI_MAX_COLOR_MODE - 1);
            return HI_FAILURE;
        }
        RetError = hdmi_color_mode(color_index);
    }
    else if (0 == strcmp("hdmi_aspectratio", (char *)args[0]))
    {
        HI_U8 aspectratio_index;
        /* change video aspect ratio: 4:3/16:9 */
        if (1 >= argc)
        {
            printf("Usage:hdmi_aspectratio aspectratio\n");
            for(index = 0; index < HDMI_MAX_ASPECTRATIO; index ++)
            {
                printf("apectratio_index:%02d   %s\n", Hdmi_V_AspectRatio[index].Index, Hdmi_V_AspectRatio[index].Index_String);
            }
            return RetError;
        }
        aspectratio_index = (HI_U8)Hdmi_String_to_Integer(args[1]);
        if ((aspectratio_index > HDMI_MAX_ASPECTRATIO) || (aspectratio_index == 0))
        {
            printf("input apectratio mode:%d is invalid!\n", aspectratio_index);
            return HI_FAILURE;
        }
        RetError = hdmi_aspectratio(aspectratio_index);
    }
    else if (0 == strcmp("hdmi_authmode", (char *)args[0]))
    {
        HI_U8 authmode;
        if (1 >= argc)
        {
            printf("Usage:hdmi_authmode mode\n");
            printf("hdmi_authmode: 0  disable authmode\n");
            printf("hdmi_authmode: 1  enable authmode\n");
            return RetError;
        }
        authmode = (HI_U8)Hdmi_String_to_Integer(args[1]);

        RetError = hdmi_authmode(authmode);
    }
    else if (0 == strcmp("hdmi_a_freq", (char *)args[0]))
    {
        HI_U8 AFreq_index;
        /* change audio frequency: 32kHz,44.1kHz,48kHz,88.2kHz,96kHz,176.4kHz or 192kHz ... */
        if (1 >= argc)
        {
            printf("Usage:hdmi_a_freq frequncy\n");
            printf("frequncy: 32000,44100,48000\n");
            for(index = 0; index < HDMI_MAX_AUDIO_FREQNCE; index ++)
            {
                printf("Audio Freq:%02d   %s\n", index, Hdmi_A_Freq[index].Index_String);
            }
            return RetError;
        }
        AFreq_index = (HI_U8)Hdmi_String_to_Integer(args[1]);
        if (AFreq_index >= HDMI_MAX_AUDIO_FREQNCE)
        {
            printf("input Audio freq index is larger than Max index:%d\n", HDMI_MAX_AUDIO_FREQNCE - 1);
            return RetError;
        }
        RetError = hdmi_a_freq(AFreq_index);
    }

    return RetError;
}

static HI_U32 hdmi_hdmi_force(void)
{
    HI_U32          RetError = HI_SUCCESS;
    HI_HDMI_ATTR_S  attr;

    printf("hdmi_hdmi_force  with HI_MPI_HDMI_SetAttr\n");
    HI_MPI_HDMI_Stop(enTest_Hdmi_ID);


    HI_MPI_HDMI_GetAttr(enTest_Hdmi_ID, &attr);
    attr.bEnableHdmi = HI_TRUE;
    attr.enVidOutMode = HI_HDMI_VIDEO_MODE_RGB444;
    attr.bEnableAudio = HI_TRUE;
    attr.bEnableAviInfoFrame = HI_TRUE;
    attr.bEnableAudInfoFrame = HI_TRUE;
    HI_MPI_HDMI_SetAttr(enTest_Hdmi_ID, &attr);

    HI_MPI_HDMI_Start(enTest_Hdmi_ID);

    return RetError;
}

static HI_U32 hdmi_dvi_force(void)
{
    HI_U32          RetError = HI_SUCCESS;
    HI_HDMI_ATTR_S  attr;

    printf("hdmi_dvi_force with HI_HDMI_SetAttr\n");
    HI_MPI_HDMI_Stop(enTest_Hdmi_ID);
    HI_MPI_HDMI_GetAttr(enTest_Hdmi_ID, &attr);
    attr.bEnableHdmi = HI_FALSE;
    attr.enVidOutMode = HI_HDMI_VIDEO_MODE_RGB444;
    attr.bEnableAudio = HI_FALSE;
    attr.bEnableAviInfoFrame = HI_FALSE;
    attr.bEnableAudInfoFrame = HI_FALSE;
    HI_MPI_HDMI_SetAttr(enTest_Hdmi_ID, &attr);
    HI_MPI_HDMI_Start(enTest_Hdmi_ID);

    return RetError;
}
HI_U32 hdmi_deepcolor(HI_U32 u32DeepColorFlag)
{
    HI_U32 RetError = HI_SUCCESS;
    HI_HDMI_SINK_CAPABILITY_S       sinkCap;

    RetError = HI_MPI_HDMI_GetSinkCapability(enTest_Hdmi_ID, &sinkCap);

    if(RetError == HI_SUCCESS)
    {
        if ((u32DeepColorFlag == HI_HDMI_DEEP_COLOR_24BIT) || (u32DeepColorFlag == HI_HDMI_DEEP_COLOR_OFF))
        {
            //OK
        }
        else if (u32DeepColorFlag == HI_HDMI_DEEP_COLOR_30BIT)
        {
            if (HI_TRUE != sinkCap.bSupportDeepColor30Bit)
            {
                printf("HI_HDMI_DEEP_COLOR_30BIT do not support in Sink Device\n");
            }
        }
        else if (u32DeepColorFlag == HI_HDMI_DEEP_COLOR_36BIT)
        {
            if (HI_TRUE != sinkCap.bSupportDeepColor36Bit)
            {
                printf("HI_HDMI_DEEP_COLOR_36BIT do not support in Sink Device\n");
            }
        }
        else
        {
            printf("DeepColor:0x%x do not support in current environment\n", u32DeepColorFlag);
        }
    }
    else
    {
        printf("can't get capability, force set Attr \n");
    }

    printf("hdmi_deepcolor u32DeepColorFlag:%d\n", u32DeepColorFlag);
    HI_MPI_HDMI_Stop(enTest_Hdmi_ID);
    HI_MPI_HDMI_SetDeepColor(enTest_Hdmi_ID, u32DeepColorFlag);

    HI_MPI_HDMI_Start(enTest_Hdmi_ID);

    /* We should fill General Control packet, hdmi chip do it in driver level */
    return RetError;
}

HI_U32 hdmi_video_timing(HI_U32 u32TimingIndex)
{
    HI_U32  RetError = HI_SUCCESS;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_CHN_ATTR_S stChnAttr[32];

    printf("hdmi_video_timing u32TimingIndex:%d\n", u32TimingIndex);
    /* Output all debug message */
    printf("change DISP Timing to u32TimingIndex:%d\n", u32TimingIndex);

    if(VO_OUTPUT_1080P24 == u32TimingIndex)
    {
        printf("Set 1920X1080P_24000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_1080P25 == u32TimingIndex)
    {
        printf("Set 1920X1080P_25000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_1080P30 == u32TimingIndex)
    {
        printf("Set 1920X1080P_30000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_720P50 == u32TimingIndex)
    {
        printf("Set 1280X720P_50000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_720P60 == u32TimingIndex)
    {
        printf("Set 1280X720P_60000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_1080I50 == u32TimingIndex)
    {
        printf("Set 1920X1080i_50000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_1080I60 == u32TimingIndex)
    {
        printf("Set 1920X1080i_60000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_1080P50 == u32TimingIndex)
    {
        printf("Set 1920X1080P_50000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_1080P60 == u32TimingIndex)
    {
        printf("Set 1920X1080P_60000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_576P50 == u32TimingIndex)
    {
        printf("Set 720X576P_50000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_480P60 == u32TimingIndex)
    {
        printf("Set 720X480P_60000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if(VO_OUTPUT_640x480_60 == u32TimingIndex)
    {
        printf("Set 640X480P_60000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_800x600_60 == u32TimingIndex)
    {
        printf("Set 800x600_60000 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_1024x768_60 == u32TimingIndex)
    {
        printf("Set 1024X768_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_1280x1024_60 == u32TimingIndex)
    {
        printf("Set 1280X1024_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_1366x768_60 == u32TimingIndex)
    {
        printf("Set 1366X768_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_1440x900_60 == u32TimingIndex)
    {
        printf("Set 1440X900_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_1280x800_60 == u32TimingIndex)
    {
        printf("Set 1280X800_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_1680x1050_60 == u32TimingIndex)
    {
        printf("Set 1680X1050_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_1920x2160_30 == u32TimingIndex)
    {
        printf("Set 1920X2160_30 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_1600x1200_60 == u32TimingIndex)
    {
        printf("Set 1600x1200_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_1920x1200_60 == u32TimingIndex)
    {
        printf("Set 1920X1200_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_2560x1440_30 == u32TimingIndex)
    {
        printf("Set 2560X1440_30 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_2560x1440_60 == u32TimingIndex)
    {
        printf("Set 2560X1440_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_2560x1600_60 == u32TimingIndex)
    {
        printf("Set 2560X1600_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_3840x2160_25 == u32TimingIndex)
    {
        printf("Set 3840X2160_25 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_3840x2160_30 == u32TimingIndex)
    {
        printf("Set 3840X2160_30 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_3840x2160_50 == u32TimingIndex)
    {
        printf("Set 3840X2160_50 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else if (VO_OUTPUT_3840x2160_60 == u32TimingIndex)
    {
        printf("Set 3840X2160_60 u32TimingIndex:0x%x\n", u32TimingIndex);
    }
    else
    {
        printf("Input u32TimingIndex(%d) error,force set 720P60.\n", u32TimingIndex);
        u32TimingIndex = VO_OUTPUT_720P60;
    }

#if defined(CHIP_HI3536D)
    if(   VO_OUTPUT_3840x2160_25 == u32TimingIndex
       || VO_OUTPUT_3840x2160_30 == u32TimingIndex
       || VO_OUTPUT_2560x1600_60 == u32TimingIndex
       || VO_OUTPUT_2560x1440_60 == u32TimingIndex)
    {
        printf("This chip is not support this format(%d).\n", u32TimingIndex);
        return HI_FAILURE;
    }
#endif

#if defined(HDMI_SUPPORT_1_4)

    if(   VO_OUTPUT_3840x2160_50 == u32TimingIndex
       || VO_OUTPUT_3840x2160_60 == u32TimingIndex )
    {
        printf("This chip is not support this format(%d).\n", u32TimingIndex);
        return HI_FAILURE;
    }

#endif

    HDMI_MST_StopHdmi();
    HDMI_MST_StopVideo(g_VoDev, g_s32ChnNum);
    HDMI_MST_GetDefVoAttr(g_VoDev, u32TimingIndex, &stPubAttr, &stLayerAttr, g_s32ChnNum, stChnAttr);
    HDMI_MST_ChangeVOFormat(g_VoDev, &stPubAttr, &stLayerAttr, stChnAttr, g_s32ChnNum, HI_FALSE, HI_FALSE);
    HDMI_MST_ChangeHdmiFomat(u32TimingIndex);

    return RetError;
}

static HI_U32 hdmi_color_mode(HI_U32 u32ColorIndex)
{
    HI_U32                        RetError = HI_SUCCESS;
    HI_HDMI_VIDEO_MODE_E          enVidOutMode;
    HI_HDMI_ATTR_S                attr;
    HI_HDMI_SINK_CAPABILITY_S     sinkCap;

    printf("hdmi_color_mode u32ColorIndex:%d\n", u32ColorIndex);

    switch(u32ColorIndex)
    {
    case 1:
        enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR422;
        break;
    case 2:
        enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444;
        break;
    case 3:
        enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR420;
        break;
    case 0:
    default:
        enVidOutMode = HI_HDMI_VIDEO_MODE_RGB444;
        break;
    }

    RetError = HI_MPI_HDMI_GetSinkCapability(enTest_Hdmi_ID, &sinkCap);

    if(RetError == HI_SUCCESS)
    {
        if (HI_TRUE != sinkCap.bSupportYCbCr)
        {
            printf("Warn: sink device do not support YCbCr \n");
//            return HI_FAILURE;
        }

        if( enVidOutMode == HI_HDMI_VIDEO_MODE_YCBCR420 )
        {
            printf("Warn: please check sink device whether it is support YCbCr420 \n");
//            return HI_FAILURE;
        }
    }
    else
    {
        printf("can't get capability, force set Attr \n");
    }

    HI_MPI_HDMI_Stop(enTest_Hdmi_ID);

    printf("hdmi_color_mode enVidOutMode:%d with HI_MPI_HDMI_SetAttr\n", enVidOutMode);
    HI_MPI_HDMI_GetAttr(enTest_Hdmi_ID, &attr);

    attr.enVidOutMode = enVidOutMode;
    RetError = HI_MPI_HDMI_SetAttr(enTest_Hdmi_ID, &attr);
    if(RetError != HI_SUCCESS)
    {
        printf("HI_MPI_HDMI_SetAttr Err! \n");
    }

    HI_MPI_HDMI_Start(enTest_Hdmi_ID);

    return RetError;
}

static HI_U32 hdmi_aspectratio(HI_U32 u32aspectratioIndex)
{
    HI_U32      RetError = HI_FAILURE;
    HI_HDMI_INFOFRAME_S           stInfoFrame;
    HI_HDMI_AVI_INFOFRAME_VER2_S *pstVIDInfoframe =  HI_NULL;

    // modified @2015.09.09
    HI_MPI_HDMI_Stop(enTest_Hdmi_ID);

    HI_MPI_HDMI_GetInfoFrame(enTest_Hdmi_ID, HI_INFOFRAME_TYPE_AVI, &stInfoFrame);

    pstVIDInfoframe = &(stInfoFrame.unInforUnit.stAVIInfoFrame);

    switch(u32aspectratioIndex)
    {
        case 1:
            printf("4:3\n");
            pstVIDInfoframe->enAspectRatio       = HI_HDMI_PIC_ASP_RATIO_4TO3;
            pstVIDInfoframe->enActiveAspectRatio = HI_HDMI_ACT_ASP_RATIO_SAME_PIC;
            break;
        case 3:
            printf("64:27\n");
            pstVIDInfoframe->enAspectRatio       = HI_HDMI_PIC_ASP_RATIO_64TO27;
            pstVIDInfoframe->enActiveAspectRatio = HI_HDMI_ACT_ASP_RATIO_SAME_PIC;
            break;
        case 4:
            printf("256:135\n");
            pstVIDInfoframe->enAspectRatio       = HI_HDMI_PIC_ASP_RATIO_256TO135;
            pstVIDInfoframe->enActiveAspectRatio = HI_HDMI_ACT_ASP_RATIO_SAME_PIC;
            break;
        default:
            printf("16:9\n");
            pstVIDInfoframe->enAspectRatio       = HI_HDMI_PIC_ASP_RATIO_16TO9;
            pstVIDInfoframe->enActiveAspectRatio = HI_HDMI_ACT_ASP_RATIO_SAME_PIC;
            break;
        }

    HI_MPI_HDMI_SetInfoFrame(enTest_Hdmi_ID, &stInfoFrame);
    // modified  @2015.09.15
    HI_MPI_HDMI_Start(enTest_Hdmi_ID);

    return RetError;
}

static HI_U32 hdmi_authmode(HI_BOOL bauthmodeEnable)
{
    HI_U32      RetError = HI_SUCCESS;
    HI_HDMI_ATTR_S stAttr;

    // modified @2015.09.16
    HI_MPI_HDMI_Stop(enTest_Hdmi_ID);

    HI_MPI_HDMI_GetAttr(enTest_Hdmi_ID, &stAttr);
    stAttr.bAuthMode = bauthmodeEnable;
    HI_MPI_HDMI_SetAttr(enTest_Hdmi_ID, &stAttr);

    // modified @2015.09.16
    HI_MPI_HDMI_Start(enTest_Hdmi_ID);

    return RetError;
}

static HI_U32 hdmi_a_freq(HI_U8 u8AudioFreqIndex)
{
    HI_U32 u32SampleRate = 0;
    switch (u8AudioFreqIndex)
    {
        case 0:
            u32SampleRate = 32000;
            break;
        case 1:
            u32SampleRate = 44100;
            break;
        case 2:
            u32SampleRate = 48000;
            break;
        default:
            printf("Wrong input AudioFreq index:%d!\n", u8AudioFreqIndex);
            return HI_FAILURE;
            break;
    }

    HDMI_MST_AUDIO_SetSampleRate(u32SampleRate);
    HDMI_MST_AUDIO_Reset();

    return HI_SUCCESS;
}

HI_S32 HIADP_HDMI_DeInit(HI_HDMI_ID_E enHDMIId)
{
    HI_MPI_HDMI_Stop(enHDMIId);

    HI_MPI_HDMI_Close(enHDMIId);

    HI_MPI_HDMI_DeInit();

    return HI_SUCCESS;
}

HI_S32 HIADP_HDMI_Init(HI_HDMI_ID_E enHDMIId)
{
    HI_S32 Ret = HI_FAILURE;

    Ret = HI_MPI_HDMI_Init();
    if (HI_SUCCESS != Ret)
    {
        printf("HI_MPI_HDMI_Init failed:%#x\n",Ret);
        return HI_FAILURE;
    }


    Ret = HI_MPI_HDMI_Open(enHDMIId);
    if (Ret != HI_SUCCESS)
    {
        printf("HI_MPI_HDMI_Open failed:%#x\n",Ret);
        HI_MPI_HDMI_DeInit();
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_U32 HDMI_Test_CMD(HI_CHAR * u8String)
{
    HI_U8 argc = 0;

    if (NULL == u8String)
    {
        return HI_FAILURE;
    }
    memset(args, 0, sizeof(args));
    printf("u8String %s\n", u8String);
    argc = Hdmi_AdjustString(u8String);

    if ( (0 == strcmp("q", (char *)args[0])) || (0 == strcmp("Q", (char *)args[0]))
      || (0 == strcmp("quit", (char *)args[0])) || (0 == strcmp("QUIT", (char *)args[0])) )
    {
        printf("quit the program, use extran interface to quit\n");
        return HI_FAILURE;
    }

    return Hdmi_ParseArg(argc);

}

