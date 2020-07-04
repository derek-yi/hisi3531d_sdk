/******************************************************************************

  Copyright (C), 2016-2017, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hdmi_test_cmd.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2016/08/31
  Description   :
  History       :
  1.Date        : 2016/08/31
    Author      :    
    Modification: Created file

******************************************************************************/

#ifndef __HDMI_TEST_CMD_H__
#define __HDMI_TEST_CMD_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "hi_type.h"


typedef struct
{
    HI_U32 Index;
    HI_U8  Index_String[20];
}HDMI_INPUT_PARAM_S;



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __HDMI_TEST_CMD_H__ */
