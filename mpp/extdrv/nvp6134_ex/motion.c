/********************************************************************************
*
*  Copyright (C) 2016 	NEXTCHIP Inc. All rights reserved.
*  Module		: The decoder's motion detection module
*  Description	: Now, Not used
*  Author		:
*  Date         :
*  Version		: Version 1.0
*
********************************************************************************
*  History      :
*
*
********************************************************************************/
#include <linux/string.h>
#include <linux/delay.h>

//#include "common.h"
#include "nvp6134.h"
#include "video.h"
extern unsigned int nvp6134_iic_addr[4];
extern unsigned int nvp6134_cnt;


//0x00+(ch%4)*0x07  bit0 motion onoff						default 0x0D
//0x01+(ch%4)*0x07  motionʱ�����ж�						default 0x60
//0x02+(ch%4)*0x07  motion�������ж�						default 0x23
//0x03+(ch%4)*0x07  motion�������жȣ�ֵԽ��Խ������		default 0x00
//0x04+(ch%4)*0x07  										default 0x00
//0x05+(ch%4)*0x07  sum_white								default 0xa0
//0x06+(ch%4)*0x07  sum_black								default 0xa0
void nvp6134_motion_init(unsigned char ch, unsigned char onoff)
{
	unsigned char ch_onoff = 0;
	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x02);
	ch_onoff = gpio_i2c_read(nvp6134_iic_addr[ch/4], 0x00+(ch%4)*0x07);
	if(onoff == 1)
	{
		CLE_BIT(ch_onoff, 0);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+(ch%4)*0x07, ch_onoff);
	}
	else
	{
		SET_BIT(ch_onoff, 0);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+(ch%4)*0x07, ch_onoff);
	}
	
	printk("nvp6134 ch[%d] motion %s setted...\n", ch, onoff==1?"ON":"OFF");
}

//��ȡ����motion��ͨ��
//get the motion channel  
unsigned int nvp6134_get_motion_ch(void)
{
	int i;
	unsigned int motion=0xFFFF;
	
	for(i=0;i<nvp6134_cnt;i++)
	{
		gpio_i2c_write(nvp6134_iic_addr[i], 0xFF, 0x00);
		motion|=(gpio_i2c_read(nvp6134_iic_addr[i], 0xA9)&0x0F)<<(4*i);
	}

	return motion;
}

//��motion pic��ʾ���ֳ��������ʾ�����˿飬���ڵ���motion������ʹ��ʱ����ر�.
//Open the motion PIC shows. Scene shows the Mosaic block, used for debugging motion, please shut when normal use.
void nvp6134_motion_display(unsigned char ch, unsigned char onoff)
{
	unsigned char ch_disp = 0;
	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x02);
	ch_disp = gpio_i2c_read(nvp6134_iic_addr[ch/4], 0x00+(ch%4)*0x07);
	if(onoff == 1)
	{
		SET_BIT(ch_disp, 2);
		SET_BIT(ch_disp, 3);
	}
	else
	{
		CLE_BIT(ch_disp, 2);
		CLE_BIT(ch_disp, 3);
	}
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+(ch%4)*0x07, ch_disp);
}

void nvp6134_motion_sensitivity(unsigned int sens[16])
{
}

/*******************************************************************************
motion block setting...192���飬ÿ���Ĵ���8��bit��ÿ��bit����1��block.

motion block setting...192 pieces, each register 8 bit, each 1 bit control block..
0x40~0x57  ch1
0x58~0x6F  ch2
0x70~0x87  ch3
0x88~0x9F  ch4
*******************************************************************************/
void nvp6134_motion_area_mask(unsigned char ch, unsigned int *blockset)
{
	int bcnt;
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x02);
	for(bcnt=0;bcnt<0x18;bcnt++)
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+(ch%4)*0x18, blockset[bcnt]);
	}
}

/*******************************************************************************
*	End of file
*******************************************************************************/
