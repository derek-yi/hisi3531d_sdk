ifeq ($(PARAM_FILE), )
        PARAM_FILE:=../../../mpp/Makefile.param
        include $(PARAM_FILE)
endif
default:
	$(CC) -Wall -O2 hiir_test.c -o hiir_test -static

clean:
	rm -rf hiir_test *.o
