
ifeq ($(PARAM_FILE), )
		PARAM_FILE:=../../Makefile.param
		include $(PARAM_FILE)
endif

obj-m := tp2853.o
tp_2802-y += tp2853.o

#EXTRA_CFLAGS += -D$(HI_FPGA)
#EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c -I$(PWD)/../gpio-i2c1
#EXTRA_CFLAGS += -I$(PWD)/../gpio_i2c_8b
EXTRA_CFLAGS += -I$(REL_INC)
EXTRA_CFLAGS += $(DRV_CFLAGS)

EXTRA_CFLAGS += -I$(REL_INC)
EXTRA_CFLAGS += $(DRV_CFLAGS)

default:
	make -C $(LINUX_ROOT) M=$(PWD) modules 
	#cp tp2802.h $(REL_INC)
	cp *.ko  $(REL_KO)/extdrv/
clean: 
	make -C $(LINUX_ROOT) M=$(PWD) clean


