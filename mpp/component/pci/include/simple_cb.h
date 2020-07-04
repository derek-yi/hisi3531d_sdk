/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : simple_cb.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/03
  Description   : 
  History       :
  1.Date        : 2015/01/15
    Author      : c00191088
    Modification: Created file

******************************************************************************/
#ifndef __SIMPLE_CB_H__
#define __SIMPLE_CB_H__

#include <asm/io.h>

#include "hi_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define SIMPLE_CB_GAP 1

typedef struct hiSIMPLE_CBHEAD_S
{
    HI_U32 u32Write;  				/* record write offset  */
    HI_U32 u32Read;   				/* record read offset  */
} SIMPLE_CBHEAD_S;

typedef struct hiSIMPLE_CB_S
{
    SIMPLE_CBHEAD_S *pCbHead;    	/* virtual address of the circle buffer */
    HI_U32           u32PhyAddr; 	/* physical address of the circle buffer */

    HI_U8           *pu8Data;    	/* start address of data buffer */
    HI_U32           u32Lenght;  	/* length of data buffer */
	HI_BOOL          bDataLeft;
} SIMPLE_CB_S;

__inline static HI_S32 Simple_CB_Init(SIMPLE_CB_S *pCb, HI_U32 u32PhyAddr, HI_U32 u32Len)
{
    HI_ASSERT(NULL != pCb);
    HI_ASSERT(u32Len > sizeof(SIMPLE_CB_S));
    HI_ASSERT( (u32Len & 0x03) == 0);
    HI_ASSERT( (u32PhyAddr & 0x03) == 0);

    pCb->pCbHead    = (SIMPLE_CBHEAD_S *)ioremap(u32PhyAddr, u32Len);
    if(NULL == pCb->pCbHead)
    {
        return HI_FAILURE;
    }
    
    pCb->pCbHead->u32Read = 0;
    pCb->pCbHead->u32Write= 0;
    pCb->bDataLeft        = 0;
	
    pCb->u32PhyAddr = u32PhyAddr;
    pCb->u32Lenght  = u32Len - sizeof(SIMPLE_CBHEAD_S);
    pCb->pu8Data    = (HI_U8 *)((HI_U32)pCb->pCbHead + sizeof(SIMPLE_CBHEAD_S));

	
    return HI_SUCCESS;
}

__inline static HI_S32 Simple_CB_DeInit(SIMPLE_CB_S *pCb)
{
    HI_ASSERT(NULL != pCb);

    if(pCb->pCbHead != NULL) 
	{
		iounmap(pCb->pCbHead);
	}
	
    pCb->pCbHead    = NULL;
    pCb->pu8Data    = NULL;
    pCb->u32PhyAddr = 0;
    pCb->u32Lenght  = 0;
	pCb->bDataLeft  = 0;
	
    return HI_SUCCESS;
}

__inline static HI_U32 Simple_CB_QueryBusy(SIMPLE_CB_S *pCb)
{
    HI_U32  u32ReadPos, u32WritePos, u32BusyLen=0;

    HI_ASSERT(NULL != pCb);
    HI_ASSERT(NULL != pCb->pCbHead);

    /* fecth write offset and read offset from the circle buffer head */
    u32ReadPos  = pCb->pCbHead->u32Read;
    u32WritePos = pCb->pCbHead->u32Write;

    if (u32WritePos > u32ReadPos)
    {
        u32BusyLen = u32WritePos - u32ReadPos;
    }
    else if (u32WritePos < u32ReadPos)
    {
        u32BusyLen = pCb->u32Lenght - (u32ReadPos - u32WritePos);
    }
	else
	{
		if (pCb->bDataLeft)
		{
			u32BusyLen = pCb->u32Lenght;
		}
		else
		{
			u32BusyLen = 0;
		}
	}
    
    return u32BusyLen;
}

__inline static HI_U32 Simple_CB_QueryFree(SIMPLE_CB_S *pCb)
{
    HI_U32  u32ReadPos, u32WritePos, u32FreeLen=0;

    HI_ASSERT(NULL != pCb);
    HI_ASSERT(NULL != pCb->pCbHead);

    /* fecth write offset and read offset from the circle buffer head */
    u32ReadPos  = pCb->pCbHead->u32Read;
    u32WritePos = pCb->pCbHead->u32Write;

    if (u32WritePos > u32ReadPos)
    {
        u32FreeLen = (pCb->u32Lenght - (u32WritePos - u32ReadPos));
    }
    else if (u32WritePos < u32ReadPos)
    {
        u32FreeLen = (u32ReadPos - u32WritePos);
    }
	else
	{
		if (pCb->bDataLeft)
		{
			u32FreeLen = 0;
		}
		else
		{
			u32FreeLen = pCb->u32Lenght;
		}
	}

    u32FreeLen = (u32FreeLen <= SIMPLE_CB_GAP) ? 0 : u32FreeLen - SIMPLE_CB_GAP;
    
    return u32FreeLen;
}

__inline static HI_S32 Simple_CB_Read(SIMPLE_CB_S *pCb, HI_U8 *pData, HI_U32 u32Len)
{
    HI_U8  *pVirAddr   = NULL;
    HI_U32  u32RdLen[2]={0,0}, u32RdPos[2]={0,0}, i;
    HI_U32  u32ReadPos, u32WritePos;

    HI_ASSERT(NULL != pCb);
    HI_ASSERT(NULL != pData);

    /* fecth write offset and read offset from the circle buffer head */
    u32ReadPos  = pCb->pCbHead->u32Read;
    u32WritePos = pCb->pCbHead->u32Write;
    
    u32RdPos[0] = u32ReadPos;
    if (u32WritePos > u32ReadPos)
    {
        if(u32WritePos >= u32ReadPos + u32Len)
        {
            u32RdLen[0] = u32Len;
        }
        else
        {
            u32RdLen[0] = u32WritePos - u32ReadPos;
        }
    }
    else if (u32WritePos < u32ReadPos)
    {
        if( u32ReadPos + u32Len <= pCb->u32Lenght)
        {
            u32RdLen[0] = u32Len;
        }
        else
        {
            u32RdLen[0] = pCb->u32Lenght - u32ReadPos;

            u32RdPos[1] = 0;
            u32RdLen[1] = u32Len - u32RdLen[0];
            if(u32WritePos < u32RdLen[1])
            {
                u32RdLen[1] = u32WritePos;
            }
        }
    }
	else
	{
		if (pCb->bDataLeft)
		{
			if(u32ReadPos + u32Len <= pCb->u32Lenght)
	        {
	            u32RdLen[0] = u32Len;
	        }
	        else
	        {
	            u32RdLen[0] = pCb->u32Lenght - u32ReadPos;

	            u32RdPos[1] = 0;
	            u32RdLen[1] = u32Len - u32RdLen[0];
	            if(u32WritePos < u32RdLen[1])
	            {
	                u32RdLen[1] = u32WritePos;
	            }
	        }
		}
	}

    for(i=0; ( i < 2 ) && (u32RdLen[i] != 0); i++)
    {
        pVirAddr = (HI_U8 *)((HI_U32)pCb->pu8Data + u32RdPos[i]);
        memcpy(pData, pVirAddr, u32RdLen[i]);
        pData += u32RdLen[i];
        
        u32ReadPos = u32RdPos[i] + u32RdLen[i];
    }

     /* update read offset  of circle buffer */
    if(u32ReadPos == pCb->u32Lenght) 
	{
		u32ReadPos = 0;
    }
	
    pCb->pCbHead->u32Read = u32ReadPos;

    if (pCb->pCbHead->u32Read == pCb->pCbHead->u32Write)
    {
        pCb->bDataLeft = 0;
	}


    /* return read length */
    return u32RdLen[0] + u32RdLen[1];
}

__inline static HI_S32 Simple_CB_Write(SIMPLE_CB_S *pCb, HI_U8 *pData, HI_U32 u32Len)
{
    HI_U8  *pVirAddr   = NULL;
    HI_U32  u32WtLen[2]={0,0}, u32WtPos[2]={0,0}, i;
    HI_U32  u32ReadPos, u32WritePos;

    HI_ASSERT(NULL != pCb);
    HI_ASSERT(NULL != pData);
    
    /* fecth write offset and read offset from the circle buffer head */
    u32ReadPos  = pCb->pCbHead->u32Read;
    u32WritePos = pCb->pCbHead->u32Write;
    
    u32WtPos[0] = u32WritePos;
    if (u32WritePos > u32ReadPos)
    {
        if(pCb->u32Lenght >= (u32WritePos + u32Len))
        {
            u32WtLen[0] = u32Len;
        }
        else
        {
            u32WtLen[0] = pCb->u32Lenght - u32WritePos;
            u32WtLen[1] = u32Len - u32WtLen[0];
            
            u32WtPos[1] = 0;
        }
    }
    else if (u32WritePos < u32ReadPos)
    {
        u32WtLen[0] = u32Len;
    }
	else
	{
		if (0 == pCb->bDataLeft)
		{
			if(pCb->u32Lenght >= (u32WritePos + u32Len))
	        {
	            u32WtLen[0] = u32Len;
	        }
	        else
	        {
	            u32WtLen[0] = pCb->u32Lenght - u32WritePos;
	            u32WtLen[1] = u32Len - u32WtLen[0];
	            
	            u32WtPos[1] = 0;
	        }
		}
	}

    for(i=0; ( i < 2 ) && (u32WtLen[i] != 0); i++)
    {
        pVirAddr = (HI_U8 *)((HI_U32)pCb->pu8Data + u32WtPos[i]);
        memcpy(pVirAddr, pData, u32WtLen[i]);
        pData += u32WtLen[i];
        
        u32WritePos = u32WtPos[i] + u32WtLen[i];
    }

    /* update write offset  of circle buffer */
    if(u32WritePos == pCb->u32Lenght) 
	{
		u32WritePos = 0;
    }
	
    pCb->pCbHead->u32Write = u32WritePos;

    if (pCb->pCbHead->u32Write == pCb->pCbHead->u32Read)
    {
        pCb->bDataLeft = 1;		
	}

    /* return write length */
    return u32WtLen[0] + u32WtLen[1];
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __SIMPLE_CB_H__*/
