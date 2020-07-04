ifeq ($(PARAM_FILE), )
    PARAM_FILE:=../../mpp/Makefile.param
    include $(PARAM_FILE)
endif

TARGET := sys_config

.PHONY: all clean
all: $(OSTYPE)_build
clean: $(OSTYPE)_clean

#######################################
$(TARGET)-objs = $(SRCS:%.c=%.o)
ifneq ($(KERNELRELEASE),)
    obj-m += $(TARGET).o
    
endif
linux_build:
	@echo -e "\e[0;32;1m--Compiling '$(TARGET)'... Configs as follow:\e[0;36;1m"
	@@echo -e "\e[0m"
	@$(MAKE) -C $(KERNEL_ROOT) M=$(PWD) modules
	@mkdir -p $(REL_KO) && cp $(TARGET).ko $(REL_KO)

linux_clean: 
	@rm -f *.o *.ko *.mod.c
	@rm -f *.symvers *.order
	@rm -rf .*.ko.cmd .*.o.cmd .tmp_versions
	@rm -f $(REL_KO)/$(TARGET).ko
#============================================
