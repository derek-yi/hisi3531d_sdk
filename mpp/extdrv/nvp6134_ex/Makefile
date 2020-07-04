#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../Makefile.param
		include $(PARAM_FILE)
endif
obj-m := nvp6134_ex.o
nvp6134_ex-objs := nvp6134_drv.o coax.o eq.o eq_recovery.o eq_common.o acp.o acp_firmup.o video.o motion.o audio.o

EXTRA_CFLAGS += -Wno-date-time  

ifeq ($(HIARCH),hi3520D)
EXTRA_CFLAGS += -DHI_GPIO_I2C
EXTRA_CFLAGS += -I$(PWD)/../gpio_i2c
EXTRA_CFLAGS += -DSUPPORT_HI3520D
endif
ifeq ($(HIARCH),hi3521a)
EXTRA_CFLAGS += -DHI_CHIP_HI3521A
#EXTRA_CFLAGS += -DI2C_INTERNAL 
EXTRA_CFLAGS += -DHI_I2C
EXTRA_CFLAGS += -I$(REL_INC)
EXTRA_CFLAGS += -I$(PWD)/../hi_i2c
EXTRA_CFLAGS += $(DRV_CFLAGS)
endif
ifeq ($(HIARCH),hi3531)
EXTRA_CFLAGS += -DHI_GPIO_I2C
EXTRA_CFLAGS += -I$(PWD)/../gpio_i2c_8b
endif
ifeq ($(HIARCH),hi3531d)
EXTRA_CFLAGS += -DHI_CHIP_HI3531D
EXTRA_CFLAGS += -DI2C_INTERNAL
EXTRA_CFLAGS += -I$(REL_INC)
EXTRA_CFLAGS += $(DRV_CFLAGS)
endif

# add Show/hide NVP6134(C) function (2016-09-10, sat)
EXTRA_CFLAGS += -D__DEC_HIDE_SHOW_FUNCTION

default:
	@make -C $(LINUX_ROOT) M=$(PWD) modules 
ifeq ($(HIARCH),hi3520D)
	cp *.ko /nfs/hi3520d/app/ko/extdrv/
endif
ifeq ($(HIARCH),hi3521a)
	#cp *.ko /nfs/hi3521a/app/ko/extdrv/
	cp *.ko $(REL_KO)/extdrv/
endif
ifeq ($(HIARCH),hi3531)
	cp *.ko /nfs/hi3531/ko/extdrv/
endif
ifeq ($(HIARCH),hi3531a)
	#cp *.ko /nfs/hi3531a/app/ko/extdrv/
	cp *.ko $(REL_KO)/extdrv/
endif
ifeq ($(HIARCH),hi3531d)
	#cp *.ko /nfs/hi3531a/app/ko/extdrv/
	cp *.ko $(REL_KO)/extdrv/
endif
clean: 
	@make -C $(LINUX_ROOT) M=$(PWD) clean 


