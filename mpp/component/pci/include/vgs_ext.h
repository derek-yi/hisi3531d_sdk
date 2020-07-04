/******************************************************************************

  Copyright (C), 2013-2023, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : vgs_ext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/04/23
  Description   :
  History       :
  1.Date        : 2013/04/23
    Author      : z00183560
    Modification: Created file
******************************************************************************/

#ifndef __VGS_EXT_H__
#define __VGS_EXT_H__

#include "hi_comm_video.h"
#include "hi_comm_vgs.h"
#include "vpss_ext.h"

#include "hi_errno.h"
#include "dci.h"
#define HI_TRACE_VGS(level, fmt...)	\
do{\
	HI_TRACE(level, HI_ID_VGS, "[%s]: %d,", __FUNCTION__, __LINE__);\
	HI_TRACE(level, HI_ID_VGS, ##fmt);\
} while(0);

#define VGS_INVALD_HANDLE          (-1UL) 		/* invalid job handle */

#define VGS_MAX_JOB_NUM             400  		/* max job number */
#define VGS_MIN_JOB_NUM             20  		/* min job number */

#define VGS_MAX_TASK_NUM            800  		/* max task number */
#define VGS_MIN_TASK_NUM            20  		/* min task number */

#define VGS_MAX_NODE_NUM            800  		/* max node number */
#define VGS_MIN_NODE_NUM            20  		/* min node number */

#define VGS_MAX_WEIGHT_THRESHOLD    100  		/* max weight threshold */
#define VGS_MIN_WEIGHT_THRESHOLD    1  			/* min weight threshold */

#define MAX_VGS_COVER               4
#define MAX_VGS_OSD                 8
#define MAX_VGS_LINE                4

#define MIN_VGS_COVER               1
#define MIN_VGS_OSD                 1
#define MIN_VGS_LINE                1

#define VGS_MAX_HSP                 100

/*the type of task operation */
typedef enum hiVGS_JOB_TYPE_E
{
    VGS_JOB_TYPE_BYPASS = 0,    				/* BYPASS job,can only contain bypass task */
    VGS_JOB_TYPE_NORMAL = 1,       				/* normal job,can contain any task except bypass task */
    VGS_JOB_TYPE_BUTT
}VGS_JOB_TYPE_E;


/*task finish status*/
typedef enum hiVGS_TASK_FNSH_STAT_E
{
    VGS_TASK_FNSH_STAT_OK = 0,   				/* task finish status: proc ok */
    VGS_TASK_FNSH_STAT_FAIL = 1, 				/* task finish status: proc failed */
    VGS_TASK_FNSH_STAT_CANCELED= 2, 			/* task finish status: canceled */
    VGS_TASK_FNSH_STAT_BUTT
}VGS_TASK_FNSH_STAT_E;


typedef struct hiVGS_TASK_DATA_S
{
	HI_BOOL                 bUserTask;      	/* current task is add by user or other mode */
    VIDEO_FRAME_INFO_S      stImgIn;        	/* input picture */
    VIDEO_FRAME_INFO_S      stImgOut;       	/* output picture */
    HI_U32                  au32privateData[4]; /* task's private data */
    void                    (*pCallBack)(MOD_ID_E enCallModId,HI_U32 u32CallDevId,const struct hiVGS_TASK_DATA_S *pTask); /* callback */
    MOD_ID_E	            enCallModId;    	/* module ID */
    HI_U32                  u32CallChnId;    	/* channel ID */
    VGS_TASK_FNSH_STAT_E    enFinishStat;     	/* output param:task finish status(ok or nok) */
    HI_U32                  reserved;       	/* save current picture's state while debug */   
}VGS_TASK_DATA_S;


typedef enum hiVGS_JOB_PRI_E
{
    VGS_JOB_PRI_HIGH = 0,      					/* high priority */
    VGS_JOB_PRI_NORMAL =1,     					/* medium priority */
    VGS_JOB_PRI_LOW =2,        					/* low priority */
    VGS_JOB_PRI_BUTT
}VGS_JOB_PRI_E;


typedef struct hiVGS_CANCEL_STAT_S
{
    HI_U32  u32JobsCanceled;    				/* job numbers of successful canceled */
    HI_U32  u32JobsLeft;        				/* left job numbers of vgs */
}VGS_CANCEL_STAT_S;


/*job finish status*/
typedef enum hiVGS_JOB_FNSH_STAT_E
{
    VGS_JOB_FNSH_STAT_OK = 1,   				/* job finish status: proc ok */
    VGS_JOB_FNSH_STAT_FAIL = 2, 				/* job finish status: proc failed */
    VGS_JOB_FNSH_STAT_CANCELED = 3, 			/* job finish status: canceled */
    VGS_JOB_FNSH_STAT_BUTT
}VGS_JOB_FNSH_STAT_E;


typedef struct hiVGS_JOB_DATA_S
{
    HI_U32                  au32PrivateData[VGS_PRIVATE_DATA_LEN]; /* job's private data */
    VGS_JOB_FNSH_STAT_E     enJobFinishStat; 	/* output param:job finish status(ok or nok) */
    VGS_JOB_TYPE_E          enJobType;
    void                    (*pJobCallBack)(MOD_ID_E enCallModId,HI_U32 u32CallDevId,struct hiVGS_JOB_DATA_S *pJobData); /* callback */
}VGS_JOB_DATA_S;


typedef struct hiVGS_ADD_OSD_S
{
    HI_U32 u32PhyAddr;							/* physical address of bitmap for osd */
    PIXEL_FORMAT_E enPixelFormat;				/* pixel format of bitmap for osd */
    HI_U32 u32Stride; 							/* stride of bitmap for osd */
    HI_BOOL bAlphaExt1555; 						/* if enable ARGB1555 alpha */
    HI_U8 u8Alpha0;        
    HI_U8 u8Alpha1;        
} VGS_ADD_OSD_S;


typedef struct hiVGS_OSD_OPT_S
{
    HI_BOOL         bOsdEn;
    HI_U8           u32GlobalAlpha;
    VGS_ADD_OSD_S   stVgsOsd;
    RECT_S          stOsdRect;
} VGS_OSD_OPT_S;

typedef struct hiVGS_LUMAINFO_OPT_S
{
    RECT_S      stRect;             			/* the regions to get lumainfo */
    HI_U32 *    pu32VirtAddrForResult;
    HI_U32      u32PhysAddrForResult;
}VGS_LUMASTAT_OPT_S;


/*Define 4 video frame*/
typedef enum HI_VGS_BORDER_WORK_E
{
    VGS_BORDER_WORK_LEFT     =  0,
    VGS_BORDER_WORK_RIGHT    =  1,
    VGS_BORDER_WORK_BOTTOM   =  2,
    VGS_BORDER_WORK_TOP      =  3,
    VGS_BORDER_WORK_BUTT
}VGS_BORDER_WORK_E;


/*Define attributes of video frame*/
typedef struct HI_VGS_FRAME_OPT_S
{
    RECT_S  stRect;
    HI_U32  u32Width; 							/* width of 4 frames,0:L,1:R,2:B,3:T */
    HI_U32  u32Color; 							/* color of 4 frames,R/G/B */
}VGS_FRAME_OPT_S;

/*Define attributes of video frame*/
typedef struct HI_VGS_BORDER_OPT_S
{
    HI_U32  u32Width[VGS_BORDER_WORK_BUTT]; 	/* width of 4 frames,0:L,1:R,2:B,3:T */
    HI_U32  u32Color; 							/* color of 4 frames,R/G/B */
}VGS_BORDER_OPT_S;


typedef struct hiVGS_ROTATE_OPT_S
{
    ROTATE_E enRotateAngle;

}VGS_ROTATE_OPT_S;


typedef struct hiVGS_COVER_OPT_S
{
    HI_BOOL     				bCoverEn;
	VGS_COVER_TYPE_E   			enCoverType;
    union
    {
        RECT_S              	stDstRect;      /* rect cover configuration */
        VGS_QUADRANGLE_COVER_S  stQuadRangle;   /* quadrangle cover configuration */
    };
    HI_U32      				u32CoverData;   /* color of cover */
}VGS_COVER_OPT_S;


typedef struct hiVGS_HSHARPEN_OPT_S
{
    HI_U32 u32LumaGain;           				/* Luma gain of sharp function */   
}VGS_HSHARPEN_OPT_S;

typedef struct hiVGS_CROP_OPT_S
{
    RECT_S stDestRect;
}VGS_CROP_OPT_S;


typedef struct hiVGS_DCI_OPT_S
{
    HI_S32      s32Strength;    				/* dci strength: [0, 64] */
    HI_S32      s32CurFrmNum;    
    HI_BOOL     bFullRange;     
    HI_BOOL     bResetDci;
    HI_BOOL     bEnSceneChange;
    HI_U32      u32HistPhyAddr;
    HI_S32 *    ps32HistVirtAddr;
    HI_U32      u32HistSize;
    DCIParam*   pstDciParam;
}VGS_DCI_OPT_S;

typedef struct hiVGS_LINE_OPT_S
{
    HI_BOOL 	bLineEn[MAX_VGS_LINE];
	VGS_LINE_S 	stVgsLine[MAX_VGS_LINE];
 }VGS_LINE_OPT_S;

typedef struct hiVGS_ASPECTRATIO_OPT_S
{
	RECT_S  stVideoRect;
	HI_U32 	u32CoverData;
 }VGS_ASPECTRATIO_OPT_S;


typedef struct hiVGS_PARAM_S
{
     
}VGS_PARAM_S;


typedef struct hiVGS_ONLINE_OPT_S
{
    HI_BOOL                 bCrop;              /* if enable crop */
    VGS_CROP_OPT_S          stCropOpt;
    HI_BOOL                 bCover;             /* if enable cover */
    VGS_COVER_OPT_S         stCoverOpt[MAX_VGS_COVER];   
    HI_BOOL                 bOsd;               /* if enable osd */
    VGS_OSD_OPT_S           stOsdOpt[MAX_VGS_OSD];  
    HI_BOOL                 bHSharpen;          /* if enable sharpen */
    VGS_HSHARPEN_OPT_S      stHSharpenOpt; 
    HI_BOOL                 bBorder;            /* if enable Border */
    VGS_BORDER_OPT_S        stBorderOpt; 
    VGS_PARAM_S             stVgsParams;
    HI_BOOL                 bDci;               /* if enable dci */
    VGS_DCI_OPT_S           stDciOpt;
	HI_BOOL    				bAspectRatio;		/* if enable aspect ratio */
	VGS_ASPECTRATIO_OPT_S 	stAspectRatioOpt;
	
    HI_BOOL                 bForceHFilt;        /* if enable horizontal filter */
    HI_BOOL                 bForceVFilt;        /* if enable vertical filter */	
    HI_BOOL                 bDeflicker;        	/* if enable deflicker */
    HI_BOOL                 bUVInvert;
}VGS_ONLINE_OPT_S;

typedef HI_S32 	    FN_VGS_Init(HI_VOID* pVrgs);

typedef HI_VOID     FN_VGS_Exit(HI_VOID);

typedef HI_S32      FN_VGS_BeginJob(VGS_HANDLE *pVgsHanle,VGS_JOB_PRI_E enPriLevel,
                                    MOD_ID_E	enCallModId,HI_U32 u32CallChnId,VGS_JOB_DATA_S *pJobData);

typedef HI_S32      FN_VGS_EndJob(VGS_HANDLE VgsHanle, HI_BOOL bSortJob,VGS_JOB_DATA_S* pJobData);


typedef HI_S32      FN_VGS_CancelJob(VGS_HANDLE hHandle);

typedef HI_S32      FN_VGS_CancelJobByModDev(MOD_ID_E	enCallModId,HI_U32 u32CallDevId,VGS_CANCEL_STAT_S*  pstVgsCancelStat);

typedef HI_S32      FN_VGS_AddFrameTask(VGS_HANDLE VgsHanle,VGS_TASK_DATA_S *pTask,VGS_FRAME_OPT_S* pstVgsFrameOpt);

typedef HI_S32      FN_VGS_AddCoverTask(VGS_HANDLE VgsHanle,VGS_TASK_DATA_S *pTask,VGS_COVER_OPT_S* pstCoverOpt);

typedef HI_S32      FN_VGS_AddRotateTask(VGS_HANDLE VgsHanle,VGS_TASK_DATA_S *pTask,VGS_ROTATE_OPT_S* pstRotateOpt);

typedef HI_S32      FN_VGS_AddOSDTask(VGS_HANDLE VgsHanle,VGS_TASK_DATA_S *pTask,VGS_OSD_OPT_S* pstOsdOpt);

typedef HI_S32      FN_VGS_AddBypassTask(VGS_HANDLE VgsHanle,VGS_TASK_DATA_S *pTask);

typedef HI_S32      FN_VGS_AddGetLumaStatTask(VGS_HANDLE VgsHanle,VGS_TASK_DATA_S *pTask,VGS_LUMASTAT_OPT_S* pstLumaInfoOpt);

typedef HI_S32      FN_VGS_AddOnlineTask(VGS_HANDLE VgsHanle,VGS_TASK_DATA_S *pTask,VGS_ONLINE_OPT_S* pstOnlineOpt); 

typedef HI_S32      FN_VGS_AddLineTask(VGS_HANDLE hHandle, VGS_TASK_DATA_S *pTask, VGS_LINE_OPT_S *pstLineOpt);

typedef HI_S32      FN_VGS_Add2ScaleTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S* pTask);

/*only for test*/
typedef HI_VOID     FN_VGS_GetMaxJobNum(HI_S32* s32MaxJobNum);
typedef HI_VOID     FN_VGS_GetMaxTaskNum(HI_S32* s32MaxTaskNum);

typedef HI_S32      FN_VGS_ProcVpssCallBack(HI_U32 u32JobHandle,  HI_U32 u32TaskId, VGS_TASK_FNSH_STAT_E enFinishState);

typedef struct hiVGS_EXPORT_FUNC_S
{
    FN_VGS_BeginJob         	*pfnVgsBeginJob;
    FN_VGS_CancelJob        	*pfnVgsCancelJob;
    FN_VGS_CancelJobByModDev    *pfnVgsCancelJobByModDev;
    FN_VGS_EndJob           	*pfnVgsEndJob;
    FN_VGS_AddFrameTask     	*pfnVgsAddFrameTask;
    FN_VGS_AddCoverTask     	*pfnVgsAddCoverTask;
    FN_VGS_AddRotateTask    	*pfnVgsAddRotateTask;
    FN_VGS_AddOSDTask       	*pfnVgsAddOSDTask;
    FN_VGS_AddBypassTask    	*pfnVgsAddBypassTask;
    FN_VGS_AddGetLumaStatTask   *pfnVgsGetLumaStatTask;
    FN_VGS_AddOnlineTask    	*pfnVgsAddOnlineTask;
	FN_VGS_AddLineTask       	*pfnVgsAddLineTask;
	
    /*only for test*/
    FN_VGS_GetMaxJobNum     	*pfnVgsGetMaxJobNum;
    FN_VGS_GetMaxTaskNum    	*pfnVgsGetMaxTaskNum;
	FN_VGS_ProcVpssCallBack 	*pfnVgsProcVpssCallBack;
	
    FN_VGS_Add2ScaleTask        *pfnVgsAdd2ScaleTask;
}VGS_EXPORT_FUNC_S;



#endif /* __VGS_H__ */

