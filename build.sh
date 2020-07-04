#!/bin/bash

export WORKSPACE="${PWD}"

[ ! -d ${WORKSPACE}/output ] && mkdir ${WORKSPACE}/output
export DEPLOY_DIR="${WORKSPACE}/output"

[ ! -d ${DEPLOY_DIR}/rootfs ] && mkdir ${DEPLOY_DIR}/rootfs
export DEPLOY_ROOTFS_DIR="${DEPLOY_DIR}/rootfs"

[ ! -d ${DEPLOY_DIR}/driver ] && mkdir ${DEPLOY_DIR}/driver
export DEPLOY_DRIVER_DIR="${DEPLOY_DIR}/driver"

CROSS_COMPILER_DIR=/home/derek/share/toolchain/arm-hisiv500-linux/target/bin
CROSS_COMPILE_TOOL=${CROSS_COMPILER_DIR}/arm-hisiv500-linux-
export PATH=${PATH}:${CROSS_COMPILER_DIR}

ARCH=arm
CROSS_COMPILE=${CROSS_COMPILE_TOOL}

UBOOT_DEFCONFIG=hi3531d_nand_config
#UBOOT_DEFCONFIG=hi3531d_mvr_card_config

#KERNEL_DEFCONFIG=hi3531d_nand_defconfig
KERNEL_DEFCONFIG=hi3531d_defconfig

export ARCH CROSS_COMPILE KERNEL UBOOT UBOOT_DEFCONFIG KERNEL_DEFCONFIG
export HSERIES_MVR_CARD_HI3531D=1

OP_SHOW_HELP=0
OSDRV_DIR=${WORKSPACE}/osdrv
MPP_KO_DIR=${WORKSPACE}/mpp/ko
SAMPLE_APP_DIR=${WORKSPACE}/mpp/sample
export OSAL_DIR=${WORKSPACE}/osal/source/kernel
export APP_DIR=${WORKSPACE}/app
export OTHER_DRVS_DIR=${WORKSPACE}/drv
export OTHER_DRVS_COM_HEADER_DIR=${OTHER_DRVS_DIR}/export/mame
export FLASH_TYPE=nand
export PCI_MODE=master
export UBOOT_CONFIG=${UBOOT_DEFCONFIG}
export KERNEL_CFG=${KERNEL_DEFCONFIG}
export OSDRV_CROSS=arm-hisiv500-linux
#export TARGET_XLSM=Hi3531D-MVR-uboot-DDR3_1866M_64bit_2GB-A9_1400M-BUS_324M-FLYBY.xlsm
export TARGET_XLSM=Hi3531D-DEMB-uboot-DDR3_1866M_64bit_2GB-A9_1400M-BUS_324M-FLYBY.xlsm
export BUSYBOX_CFG=config_v500_a9_softfp_neon
export ROOTFS_PATCH_PATH=${WORKSPACE}/rootfs_patch/scripts

if [ $# -ne 2 ];then
	OP_SHOW_HELP=1
fi

uboot_build() {
	cd ${OSDRV_DIR}
	make hiboot
	cp ${OSDRV_DIR}/pub/${PUB_IMAGE}/image_uclibc/u-boot-hi3531d.bin ${DEPLOY_DIR} 
	exit 0
}

uboot_clean() {
	cd ${OSDRV_DIR}
	make hiboot_clean
	rm ${DEPLOY_DIR}/u-boot-hi3531d.bin
	exit 0
}

kernel_build() {
	cd ${OSDRV_DIR}
	make hikernel
	cp ${OSDRV_DIR}/pub/${PUB_IMAGE}/image_uclibc/uImage_hi3531d ${DEPLOY_DIR} 
	exit 0
}

kernel_clean() {
	cd ${OSDRV_DIR}
	make hikernel_clean
	rm ${DEPLOY_DIR}/uImage_hi3531d
	exit 0
}

other_drv_build() {
	if [ ! -d ${OTHER_DRVS_COM_HEADER_DIR} ];then
		mkdir ${OTHER_DRVS_COM_HEADER_DIR} -p
	fi

	pushd ${OTHER_DRVS_COM_HEADER_DIR};ln -s ${OTHER_DRVS_DIR}/stub/comifimx.h ./;popd
	pushd ${OTHER_DRVS_COM_HEADER_DIR};ln -s ${OTHER_DRVS_DIR}/comif/comif_dev.h ./;popd
	pushd ${OTHER_DRVS_COM_HEADER_DIR};ln -s ${OTHER_DRVS_DIR}/comif/comifio.h ./;popd
	pushd ${OTHER_DRVS_COM_HEADER_DIR};ln -s ${OTHER_DRVS_DIR}/sysinit/sysinit.h ./;popd
	pushd ${OTHER_DRVS_COM_HEADER_DIR};ln -s ${OTHER_DRVS_DIR}/spidev/spidevio.h ./;popd
	pushd ${OTHER_DRVS_COM_HEADER_DIR};ln -s ${OTHER_DRVS_DIR}/i2cdev/i2cdevio.h ./;popd

	cd ${OTHER_DRVS_DIR}

	cd cipher && make && cd ..
	cd hisi-irda && make && cd ..
	cd rtc && make all && cd ..
	cd sys_config && make all && cd ..
	cd wtdg && make && cd ..
	#cd stub && make && cd ..
	#cd comif && make && cd ..
	#cd sysinit && make && cd ..
	#cd spidev && make && cd ..
	#cd i2cdev && make && cd ..
	#cd gpio && make && cd ..
	#cd sil9136 && make && cd ..

	find ${OTHER_DRVS_DIR} -name "*.ko" -exec cp {} ${DEPLOY_DRIVER_DIR} \;
	#cp ${OTHER_DRVS_DIR}/fpga_config/fpga_load ${DEPLOY_ROOTFS_DIR}/bin/
}

other_drv_clean() {
	cd ${OTHER_DRVS_DIR}

	cd cipher && make clean && cd ..
	cd hisi-irda && make clean && cd ..
	cd rtc && make clean && cd ..
	cd sys_config && make clean && cd ..
	cd wtdg && make clean && cd ..
	#cd stub && make clean && cd ..
	#cd comif && make clean && cd ..
	#cd sysinit && make clean && cd ..
	#cd spidev && make clean && cd ..
	#cd i2cdev && make clean && cd ..
	#cd gpio && make clean && cd ..
	#cd sil9136 && make clean && cd ..

	rm -rf ${OTHER_DRVS_COM_HEADER_DIR}/*
}

driver_build() {
	# osal
	cd ${OSAL_DIR}
	make all 
	
	# mpp
	cp ${MPP_KO_DIR}/*.ko ${DEPLOY_DRIVER_DIR}
	#cp ${MPP_KO_DIR}/load3531d_mvr_card ${DEPLOY_DRIVER_DIR}
	#chmod a+x ${DEPLOY_DRIVER_DIR}/load3531d_mvr_card

	# drv
	other_drv_build
	exit 0
}

driver_clean() {
	#app
	rm ${DEPLOY_DRIVER_DIR}/*

	# osal
	cd ${OSAL_DIR}
	make clean

	# drv
	other_drv_clean
	exit 0
}

app_build() {
	# mpp sample
	cd ${SAMPLE_APP_DIR}
	make rel
	cd -

	cd ${APP_DIR}
	cd tools/mrecv && make && cd -

	cd ${WORKSPACE}
	exit 0
}

app_clean() {
	cd ${SAMPLE_APP_DIR}
	make cleanall

	cd ${APP_DIR}
	cd tools/mrecv && make clean && cd -

	cd ${WORKSPACE}
	exit 0
}

rootfs_build() {
	cd ${OSDRV_DIR}
	make hirootfs_build
	cp ${OSDRV_DIR}/pub/image_uclibc/rootfs* ${DEPLOY_DIR}
	rm -rf ${DEPLOY_ROOTFS_DIR}/*
	cp -rf ${OSDRV_DIR}/pub/rootfs_uclibc/* ${DEPLOY_ROOTFS_DIR}/
	exit 0
}

rootfs_clean() {
	cd ${OSDRV_DIR}
	make hirootfs_clean
	rm ${DEPLOY_ROOTFS_DIR}/*
	exit 0
}

all_build() {
	cd ${OSDRV_DIR}
	make all
	cp ${OSDRV_DIR}/pub/${PUB_IMAGE}/image_uclibc/u-boot-hi3531d.bin ${DEPLOY_DIR}
	cp ${OSDRV_DIR}/pub/${PUB_IMAGE}/image_uclibc/uImage_hi3531d ${DEPLOY_DIR}
	cp ${OSDRV_DIR}/pub/${PUB_IMAGE}/image_uclibc/rootfs* ${DEPLOY_DIR}
	cp ${OSDRV_DIR}/pub/${PUB_IMAGE}/rootfs_uclibc/* ${DEPLOY_DIR}
	exit 0
}

all_clean() {
	cd ${OSDRV_DIR}
	make distclean
	exit 0
}

case $1 in
	all)
		echo "starting to compile all"
		all_build
	;;
	boot | uboot | u-boot)
		echo "starting to compile u-boot"
		uboot_build
	;;
	kernel)
		echo "starting to compile kernel"
		kernel_build
	;;
	drv)
		echo "starting to compile driver"
		driver_build
	;;
	app)
		echo "starting to compile app"
		app_build
	;;
	rootfs)
		echo "starting to compile driver"
		rootfs_build
	;;
	boot-clean)
		echo "starting to clean u-boot"
		uboot_clean
	;;
	kernel-clean)
		echo "starting to clean kernel"
		kernel_clean
	;;
	drv-clean)
		echo "starting to clean driver"
		driver_clean
	;;
	app-clean)
		echo "starting to clean app"
		app_clean
	;;
	rootfs-clean)
		echo "starting to clean rootfs"
		rootfs_clean
	;;
	clean)
		echo "starting to clean all"
		all_clean
	;;
	*)
		echo "invalid parameter"
		OP_SHOW_HELP=1
	;;
esac

if [ ${OP_SHOW_HELP} = 1 ]; then
	echo "usage: build <TARGET_IMAGE>"
	echo ""
	echo "  <TARGET_IMAGE>"
	echo "    all"
	echo "    boot"
	echo "    kernel"
	echo "    rootfs"
	echo "    app"
	echo "    drv"
	echo "    boot-clean"
	echo "    kernel-clean"
	echo "    drv-clean"
	echo "    rootfs-clean"
	echo "    app-clean"
	echo "    clean"
	exit 1
fi

exit 0
