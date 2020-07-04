# $(ROOT)/mpp/code/component/pci/pciv_fmw/Makefile

ifeq ($(PARAM_FILE), ) 
    PARAM_FILE:=../../../Makefile.param
    include $(PARAM_FILE)
endif

MPP_PATH := $(SDK_PATH)/mpp

INC_PATH := -I$(MPP_PATH)/component/pci/include -I$(MPP_PATH)/component/pci/pciv/kernel -I$(REL_INC)

ifeq ($(CPU_TYPE),single)
obj-m += $(HIARCH)_pciv_fmw.o
$(HIARCH)_pciv_fmw-y := pciv_firmware.o pciv_pic_queue.o

else ifeq ($(CPU_TYPE),master)
obj-m += $(HIARCH)_pciv_fmwm.o
$(HIARCH)_pciv_fmwm-y := pciv_firmware_dccm.o

else
obj-m += $(HIARCH)_pciv_fmws.o
$(HIARCH)_pciv_fmws-y := pciv_firmware_dccs.o

obj-m += $(HIARCH)_pciv_fmw.o
$(HIARCH)_pciv_fmw-y := pciv_firmware.o pciv_pic_queue.o
endif

EXTRA_CFLAGS += $(INC_PATH) -I.
ifeq ($(HIGDB),HI_GDB)
EXTRA_CFLAGS += -g
endif

EXTRA_CFLAGS += $(MPP_CFLAGS)
EXTRA_CFLAGS += -Wall -D$(HIARCH) -DHICHIP=$(HICHIP) -D$(HIDBG) -D$(HI_FPGA)

all:
	
ifeq ($(CPU_TYPE),single)
	make -C $(LINUX_ROOT) M=$(PWD) modules
	cp $(HIARCH)_pciv_fmw.ko $(REL_KO)/
else ifeq ($(CPU_TYPE),master)
	make -C $(LINUX_ROOT) M=$(PWD) modules
	cp $(HIARCH)_pciv_fmwm.ko $(REL_KO)/
else	
	make -C $(LINUX_ROOT) M=$(PWD) modules
	cp $(HIARCH)_pciv_fmws.ko $(REL_KO)/
	
	make -C $(LINUX_ROOT) M=$(PWD) modules
	cp $(HIARCH)_pciv_fmw.ko $(REL_KO)/
endif


clean:
	make -C $(LINUX_ROOT) M=$(PWD) clean
	rm -f $(REL_KO)/$(HIARCH)_pciv_fmw*.ko
