# $(ROOT)/mpp/code/component/pci/Makefile

ifeq ($(PARAM_FILE), ) 
    PARAM_FILE:=../../Makefile.param
    include $(PARAM_FILE)
endif

DCI_HEADER := dci.h hi_defines.h
COM_HEADER := hi_comm_video.h hi_comm_pciv.h  hi_comm_vdec.h hi_comm_vb.h hi_comm_vpss.h hi_comm_vgs.h hi_comm_sys.h hi_comm_region.h hi_comm_vo.h hi_type.h hi_common.h hi_math.h hi_errno.h hi_debug.h
MKP_HEADER := mkp_ioctl.h mkp_sys.h mkp_vb.h mkp_vpss.h
EXT_HEADER := vgs_ext.h mm_ext.h mod_ext.h dev_ext.h pciv_fmwext.h proc_ext.h sys_ext.h vb_ext.h vpss_ext.h region_ext.h
IPCM_HEADER := hi_ipcm_usrdev.h
MEDA_HEADER := himedia.h
OSAL_HEADER := osal_mmz.h osal_ioctl.h hi_osal_user.h hi_osal_init.h osal_list.h hi_osal.h 

INC_DIR=$(SDK_PATH)/mpp/component/pci/include

.PHONY: default clean

default:
ifeq ($(MPP_BUILD), y)
	@-pushd $(SDK_PATH)/mpp/component/pci/include > /dev/null; rm -f $(COM_HEADER) rm -f $(MKP_HEADER); rm -f $(EXT_HEADER); popd > /dev/null
	@-pushd $(SDK_PATH)/mpp/code/mkp_inc > /dev/null;                      cp $(MKP_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(SDK_PATH)/mpp/code/mkp/include > /dev/null;                  cp $(EXT_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(SDK_PATH)/mpp/code/include > /dev/null;                      cp $(COM_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(SDK_PATH)/mpp/code/arch/$(ARCH_DIR)/include > /dev/null;     cp $(DCI_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(SDK_PATH)/mpp/code/dccs/dccs_main/include > /dev/null;       cp $(IPCM_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(OSAL_ROOT)/include > /dev/null;                              cp $(OSAL_HEADER) $(INC_DIR); popd > /dev/null	
	@-pushd $(OSAL_ROOT)/linux/kernel/himedia > /dev/null;                    cp $(MEDA_HEADER) $(INC_DIR); popd > /dev/null
endif
	@for x in `find ./ -maxdepth 2 -mindepth 2 -name "Makefile" `; \
	   do cd `dirname $$x`; if [ $$? ]; then make || exit 1; cd ../; fi; done

clean:
	@for x in `find ./ -maxdepth 2 -mindepth 2 -name "Makefile" `; \
	   do cd `dirname $$x`; if [ $$? ]; then make clean || exit 1; cd ../; fi; done

ifeq ($(MPP_BUILD), y)
	@-pushd $(SDK_PATH)/mpp/component/pci/include > /dev/null; popd > /dev/null
endif

