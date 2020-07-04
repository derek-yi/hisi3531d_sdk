
#ifndef __PCIV_FIRMWARE_DCC_H__
#define __PCIV_FIRMWARE_DCC_H__

#include "hi_comm_pciv.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


typedef enum hiPCIVFMW_MSGTYPE_E
{
    PCIVFMW_MSGTYPE_CREATE = 0,
    PCIVFMW_MSGTYPE_DESTROY,
    PCIVFMW_MSGTYPE_START,
    PCIVFMW_MSGTYPE_STOP,
    PCIVFMW_MSGTYPE_SETATTR,
    PCIVFMW_MSGTYPE_MALLOC,
    PCIVFMW_MSGTYPE_FREE,
    PCIVFMW_MSGTYPE_CHNMALLOC,
    PCIVFMW_MSGTYPE_CHNFREE,
    PCIVFMW_MSGTYPE_VBCREATE,
    PCIVFMW_MSGTYPE_VBDESTROY,
    PCIVFMW_MSGTYPE_SETVPPCFG,
    PCIVFMW_MSGTYPE_GETVPPCFG,
    PCIVFMW_MSGTYPE_CMDECHO,
    
    PCIVFMW_MSGTYPE_BUTT
} PCIVFMW_MSGTYPE_E;


#define PCIVFMW_MSG_MAXLEN  (768)
#define PCIVFMW_MSG_MAGIC   (0x1234)

typedef struct hiPCIVFMW_MSGHEAD_S
{
    HI_U32            u32Magic;     					/* magic number of pciv firmware message */
    PCIVFMW_MSGTYPE_E enMsgType;    					/* message type */
    PCIV_CHN          pcivChn;      					/* pciv channel */
    HI_U32            u32MsgLen;    					/* message length, not include message head */
    HI_S32            s32RetVal;    					/* value of message receiver returned */
} PCIVFMW_MSGHEAD_S;

typedef struct hiPCIVFMW_MSG_S
{
    
    PCIVFMW_MSGHEAD_S   stHead; 						/* message head */
    
    
    HI_U8               cMsgBody[PCIVFMW_MSG_MAXLEN];	/* message body */
    
} PCIVFMW_MSG_S;

typedef struct hiPCIVFMW_MSG_CREATE_S
{
    HI_U32      u32PhyAddr;
    HI_U32      u32CbSize;
	HI_S32 		s32LocalId;
    PCIV_ATTR_S stPcivAttr;
} PCIVFMW_MSG_CREATE_S;

/* attribution of pciv chn */
typedef struct hiPCIVFMW_MSG_SETATTR_S
{
    PCIV_ATTR_S       stAttr;
	HI_S32            s32LocalId;
} PCIVFMW_MSG_SETATTR_S;

typedef struct hiPCIVFMW_MSG_MALLOC_S
{
    HI_U32 u32Size;
    HI_S32 s32LocalId;
    HI_U32 u32PhyAddr;
} PCIVFMW_MSG_MALLOC_S;

typedef struct hiPCIVFMW_MSG_CHN_MALLOC_S
{
	HI_U32 			u32Index;
    HI_U32 			u32Size;
    HI_S32 			s32LocalId;
    HI_U32 			u32PhyAddr;
} PCIVFMW_MSG_CHN_MALLOC_S;

typedef struct hiPCIVFMW_MSG_FREE_S
{
	HI_S32 s32LocalId;
    HI_U32 u32PhyAddr;
} PCIVFMW_MSG_FREE_S;

typedef struct hiPCIVFMW_MSG_CHN_FREE_S
{
	HI_U32 			u32Index;
    HI_S32 			s32LocalId;
} PCIVFMW_MSG_CHN_FREE_S;

static inline HI_CHAR* STR_PCIVFMW_MSGTYPE(PCIVFMW_MSGTYPE_E enMsgType)
{
    switch (enMsgType)
    {
        case PCIVFMW_MSGTYPE_CREATE:    	return "create";
        case PCIVFMW_MSGTYPE_DESTROY:   	return "destroy";
        case PCIVFMW_MSGTYPE_START:     	return "start";
        case PCIVFMW_MSGTYPE_STOP:      	return "stop";
        case PCIVFMW_MSGTYPE_SETATTR:   	return "set";
        case PCIVFMW_MSGTYPE_MALLOC:    	return "malloc";
        case PCIVFMW_MSGTYPE_FREE:      	return "free";
		case PCIVFMW_MSGTYPE_CHNMALLOC: 	return "chnmalloc";
    	case PCIVFMW_MSGTYPE_CHNFREE:   	return "chnfree";
        case PCIVFMW_MSGTYPE_VBCREATE:  	return "vbcreate";
        case PCIVFMW_MSGTYPE_VBDESTROY: 	return "vbdestroy";
        case PCIVFMW_MSGTYPE_SETVPPCFG: 	return "setvpp";
        case PCIVFMW_MSGTYPE_GETVPPCFG: 	return "getvpp";
		case PCIVFMW_MSGTYPE_CMDECHO:   	return "echo";
        default:    						return "invalid";
    }
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __PCIV_FIRMWARE_DCC_H__ */

