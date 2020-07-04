#for svn test folder use
#ifeq ($(PARAM_FILE), )
#     PARAM_FILE:=../../../mpp/Makefile.param
#     include $(PARAM_FILE)
#endif

#for component folder use
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../../Makefile.param
     include $(PARAM_FILE)
endif

CC   := $(CROSS)gcc
AR   := $(CROSS)ar

INC_FLAGS := -I$(REL_INC)

CFLAGS := -Wall -g $(INC_FLAGS) -D$(HIARCH) -DHICHIP=$(HICHIP) -D$(HIDBG) -D$(HI_FPGA) -ldl -lpthread -lm

MPI_LIBS := $(REL_LIB)/libmpi.a $(REL_LIB)/libhdmi.a 

################ select audio codec type for your sample ################
INNER_HDMI ?= y

HDMI_VERSION := HDMI_SUPPORT_1_4

#hi3531d_start
ifneq ($(findstring $(HIARCH), hi3531d),)
CFLAGS += -DCHIP_HI3531D
HDMI_VERSION := HDMI_SUPPORT_2_0
endif
#hi3531d_end

ifneq ($(findstring $(HIARCH), hi3536dv100),)
CFLAGS += -DCHIP_HI3536D
endif

ifeq ($(FPGA), y)
	CFLAGS += -DHDMI_FPGA_TEST
endif

CFLAGS += -D$(HDMI_VERSION)

MPP_DIR = $(shell pwd)/../../..

ifeq ($(INNER_HDMI), y)
	CFLAGS += -DHI_ACODEC_TYPE_HDMI
	
	CFLAGS += -I$(MPP_DIR)/code/include/
	CFLAGS += -I$(MPP_DIR)/code/shelf/audio/acodec/V500
	CFLAGS += -I$(MPP_DIR)/code/arch/$(HIARCH)/include/
	CFLAGS += -I$(MPP_DIR)/release/$(HIARCH)/include/
	CFLAGS += -I$(MPP_DIR)/component/hdmi2.0/src/include/
endif

SRC  := $(wildcard *.c)

INC_FLAGS += $(wildcard *.h)
OBJ  := $(SRC:%.c=%.o)

.PHONY : clean all sample_hdmi_display 
all: sample_hdmi_display
	@echo 
sample_hdmi_display:	$(OBJ) 
	@$(CC) $(CFLAGS) -o $@ $^ $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA)
	
clean:
	@rm -f $(LIBA) $(LIBS)
	@rm -f $(OBJ)
	@rm -f sample_hdmi_display

