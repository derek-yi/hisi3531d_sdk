# Hisilicon Hi35xx sample Makefile

include ../Makefile.param
#ifeq ($(SAMPLE_PARAM_FILE), )
#     SAMPLE_PARAM_FILE:=../Makefile.param
#     include $(SAMPLE_PARAM_FILE)
#endif

# target source
SRC  := $(wildcard *.c)

SRC += $(wildcard ./adp/*.c)

OBJ  := $(SRC:%.c=%.o)

#TARGET := $(OBJ:%.o=%)
TARGET := sample_audio

.PHONY : clean all

all: $(TARGET)

MPI_LIBS := $(REL_LIB)/libmpi.a
MPI_LIBS += $(REL_LIB)/libhdmi.a

AAC_LIBS := $(REL_LIB)/libaacdec.a
AAC_LIBS += $(REL_LIB)/libaacenc.a

$(TARGET):%:$(OBJ) $(COMM_OBJ)
	$(CC) $(CFLAGS) -lpthread -lm -o $@ $^ $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) $(AAC_LIBS) -lpthread -ldl -lm

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
	@rm -f $(COMM_OBJ)

cleanstream:
	@rm -f *.h264
	@rm -f *.jpg
	@rm -f *.mjp
	@rm -f *.mp4
