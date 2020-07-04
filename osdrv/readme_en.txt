1.How to use Makefile of directory osdrv:
Either one of the two toolchains(arm-hisiv500-linux and arm-hisiv600-linux) could be used to compile the source code under current directory. So a parameter to specify the right toolchain should be given while compiling osdrv. The arm-hisiv500-linux toolchain is for uclibc, and the arm-hisiv600-linux toolchain for glibc. 

(1)Compile the entire osdrv directory:
	Notes: The linux_v3.18.20 source file is not released default. Please download it from www.kernel.org. 
               1) Go to the website: www.kernel.org
               2) Select HTTP protocol resources https://www.kernel.org/pub/ option,
                  enter the sub-page
               3) Select linux/ menu item, enter the sub-page
               4) Select the kernel/ menu item, enter the sub-page
               5) Select v3.x/ menu item, enter sub-page
               6) Download linux-3.18.20.tar.gz (or linux-3.18.20.tar.xz) to osdrv/opensource/kernel

	make OSDRV_CROSS=arm-hisiv500-linux PCI_MODE=XXX FLASH_TYPE=XXX all
	or:
	make OSDRV_CROSS=arm-hisiv600-linux PCI_MODE=XXX FLASH_TYPE=XXX all

Parameter explain:
FLASH_TYPE:include nand and spi. Use 'nand' when you want to use nand flash. Use 'spi nor' or 'spi nand' when you want to use spi nor flash.

(2)Clear all compiled files under osdrv directory:
	make OSDRV_CROSS=arm-hisiv500-linux clean
	or:
	make OSDRV_CROSS=arm-hisiv600-linux clean
(3)Completely remove all compiled files under osdrv directory, and the generated images:
	make OSDRV_CROSS=arm-hisiv500-linux distclean
	or:
	make OSDRV_CROSS=arm-hisiv600-linux distclean
(4)Separately compile kernel:
	Notes: before separately compile kernel, please read the readme_en.txt at osdrv/opensource/kernel.

	Enter the top directory the kernel source code, do the following:
	cp arch/arm/configs/hi3531d_full_defconfig .config
	make ARCH=arm CROSS_COMPILE=arm-hisiv500-linux- menuconfig
	make ARCH=arm CROSS_COMPILE=arm-hisiv500-linux- uImage
	or:
	cp arch/arm/configs/hi3531d_full_defconfig .config
	make ARCH=arm CROSS_COMPILE=arm-hisiv600-linux- menuconfig
	make ARCH=arm CROSS_COMPILE=arm-hisiv600-linux- uImage

hi3531d_xxx_defconfig parameter explain：
	hi3531d_full_defconfig: use SPI Nor Flash or SPI Nand Flash，PCIE master
mode；
	hi3531d_full_slave_defconfig: use SPI Nor Flash or SPI Nand Flash，PCIE
slave mode；
	hi3531d_nand_defconfig: use Nand Flash，PCIE master mode；
	hi3531d_nand_slave_defconfig: use Nand Flash，PCIE slave mode；

(5)Separately compile uboot:
	Enter the top directory of boot source code, do the following:
	make ARCH=arm CROSS_COMPILE=arm-hisiv500-linux- hi3531d_xxx_config
	make ARCH=arm CROSS_COMPILE=arm-hisiv500-linux-
	or:
	make ARCH=arm CROSS_COMPILE=arm-hisiv600-linux- hi3531d_xxx_config
	make ARCH=arm CROSS_COMPILE=arm-hisiv600-linux-

	The generated u-boot.bin is copied to osdrv/tools/pc/uboot_tools/ directory
	./mkboot.sh reg_info_hi3531d.bin u-boot_hi3531d.bin
	The generated u-boot_hi3531d.bin is available for u-boot image

Parameter explain:
	hi3531d_xxx_config sets as hi3531d_config when you want to use SPI Nor Flash
or SPI Nand Flash；
	hi3531d_xxx_config sets as hi3531d_nand_config when you want to use Nand
Flash.

(6)Build file system image:
        A compiled file system has already been in osdrv/pub/, so no need to re-build file system. What you need to do is to build the right file system image according to the flash specification of the single-board. 
	Filesystem images of jffs2 format is available for spi flash. While making jffs2 image or squashfs image, you need to specify the spi flash block size. flash block size will be printed when uboot runs. run mkfs.jffs2 first to get the right parameters from it's printed information. Here the block size is 256KB, for example:

	jffs2 filesystem image:
	osdrv/pub/bin/pc/mkfs.jffs2 -d osdrv/pub/rootfs_uclibc -l -e 0x40000 -o osdrv/pub/rootfs_uclibc_256k.jffs2
	or:
	osdrv/pub/bin/pc/mkfs.jffs2 -d osdrv/pub/rootfs_glibc -l -e 0x40000 -o osdrv/pub/rootfs_glibc_256k.jffs2
	
	Filesystem image of yaffs2 format is available for nand flash and spi nand flash. While making yaffs2 image, you need to specify it's page size and ECC. This information will be printed when uboot runs. run mkyaffs2image first to get the right parameters from it's printed information. 
       Here to 2KB pagesize, 4bit ecc, for example:
	
	If making Nand yaffs2 filesystem image, please use mkyaffs2image610
	osdrv/pub/bin/pc/mkyaffs2image610 osdrv/pub/rootfs_uclibc osdrv/pub/rootfs_uclibc_2k_4bit.yaffs2 1 2
	or:
	osdrv/pub/bin/pc/mkyaffs2image610 osdrv/pub/rootfs_glibc osdrv/pub/rootfs_glibc_2k_4bit.yaffs2 1 2
	If making SPI Nand yaffs2 filesystem image, please use mkyaffs2image100
	osdrv/pub/bin/pc/mkyaffs2image100 osdrv/pub/rootfs_uclibc osdrv/pub/rootfs_uclibc_2k_4bit.yaffs2 1 2
	or	
	osdrv/pub/bin/pc/mkyaffs2image100 osdrv/pub/rootfs_glibc osdrv/pub/rootfs_uclibc_2k_4bit.yaffs2 1 2

    UBIFS is available for nand flash. The mkubiimg.sh tool is specialized for making UBIFS image at
the path: osdrv/tools/pc/ubi_sh.
    Here to 2KB pagesize,128KiB blocksize, and 32MB mtdpartition for example:
		mkubiimg.sh hi3531d 2k 128k osdrv/pub/rootfs 32M osdrv/pub/bin/pc
	osdrv/pub/rootfs is the route of rootfs;
	osdrv/pub/bin/pc is the tool path of making UBI iamge;
	rootfs_hi3531d_2k_128k_32M.ubifs is the UBIFS image for burning;	

2. Output directory description
All compiled images, rootfs are located in directory osdrv/pub/.
pub
├── bin
│   ├── board_uclibc --------------------------------------------tools used on board with hisiv500
│   │   ├── ethtool
│   │   ├── flashcp
│   │   ├── flash_erase
│   │   ├── flash_otp_dump
│   │   ├── flash_otp_info
│   │   ├── gdb-arm-hisiv500-linux
│   │   ├── mtd_debug
│   │   ├── mtdinfo
│   │   ├── nanddump
│   │   ├── nandtest
│   │   ├── nandwrite
│   │   ├── sumtool
│   │   ├── ubiattach
│   │   ├── ubicrc32
│   │   ├── ubidetach
│   │   ├── ubiformat
│   │   ├── ubimkvol
│   │   ├── ubinfo
│   │   ├── ubinize
│   │   ├── ubirename
│   │   ├── ubirmvol
│   │   ├── ubirsvol
│   │   └── ubiupdatevol
│   ├── board_glibc -------------------------------------------- tools used on board with hisiv600
│   │   ├── ethtool
│   │   ├── flashcp
│   │   ├── flash_erase
│   │   ├── flash_otp_dump
│   │   ├── flash_otp_info
│   │   ├── gdb-arm-hisiv600-linux
│   │   ├── mtd_debug
│   │   ├── mtdinfo
│   │   ├── nanddump
│   │   ├── nandtest
│   │   ├── nandwrite
│   │   ├── sumtool
│   │   ├── ubiattach
│   │   ├── ubicrc32
│   │   ├── ubidetach
│   │   ├── ubiformat
│   │   ├── ubimkvol
│   │   ├── ubinfo
│   │   ├── ubinize
│   │   ├── ubirename
│   │   ├── ubirmvol
│   │   ├── ubirsvol
│   │   └── ubiupdatevol
│   └── pc
│       ├── lzma
│       ├── mkfs.cramfs
│       ├── mkfs.jffs2
│       ├── mkfs.ubifs
│       ├── mkimage
│       ├── mksquashfs
│       ├── mkyaffs2image100
│       ├── ubi.cfg
│       └── ubinize
├─image_uclibc -------------------------------------------- Images compiled with hisiv500
│   ├── rootfs_hi3531d_4k_24bit.yaffs2-------------------------- yaffs2 rootfs image(SPI nand-flash pagesize=4K ecc=24bit)
│   ├── rootfs_hi3531d_4k_4bit.yaffs2 -------------------------- yaffs2 rootfs image(SPI nand-flash pagesize=4K ecc=4bit)
│   ├── rootfs_hi3531d_2k_24bit.yaffs2-------------------------- yaffs2 rootfs image(SPI nand-flash pagesize=2K ecc=24bit)
│   ├── rootfs_hi3531d_2k_4bit.yaffs2 -------------------------- yaffs2 rootfs image(SPI nand-flash pagesize=2K ecc=4bit)
│   ├── rootfs_hi3531d_2k_128K_32M.ubifs------------------------ ubifs rootfs image(SPI nand-flash pagesize=2K blocksize=128K)
│   ├── rootfs_hi3531d_4k_256K_50M.ubifs------------------ ubifs rootfs image(nand-flash pagesize=4K blocksize=256K)
│   ├── rootfs_hi3531d_64k.jffs2 ------------------------------- jffs2 rootfs image(SPI NOR flash blocksize = 64K)
│   ├── u-boot-hi3531d.bin ------------------------------------- u-boot image
│   └── uImage_hi3531d ----------------------------------------- kernel image
├─image_glibc --------------------------------------------- Images compiled with hisiv600
│   ├── rootfs_hi3531d_4k_24bit.yaffs2-------------------------- yaffs2 rootfs image(SPI nand-flash pagesize=4K ecc=24bit)
│   ├── rootfs_hi3531d_4k_4bit.yaffs2 -------------------------- yaffs2 rootfs image(SPI nand-flash pagesize=4K ecc=4bit)
│   ├── rootfs_hi3531d_2k_24bit.yaffs2-------------------------- yaffs2 rootfs image(SPI nand-flash pagesize=2K ecc=24bit)
│   ├── rootfs_hi3531d_2k_4bit.yaffs2 -------------------------- yaffs2 rootfs image(SPI nand-flash pagesize=2K ecc=4bit)
│   ├── rootfs_hi3531d_2k_128K_32M.ubifs------------------------ ubifs rootfs image(SPI nand-flash pagesize=2K blocksize=128K)
│   ├── rootfs_hi3531d_4k_256K_50M.ubifs------------------ ubifs rootfs image(nand-flash pagesize=4K blocksize=256K)
│   ├── rootfs_hi3531d_64k.jffs2 ------------------------------- jffs2 rootfs image(SPI NOR flash blocksize = 64K)
│   ├── u-boot-hi3531d.bin ------------------------------------- u-boot image
│   └── uImage_hi3531d ----------------------------------------- kernel image
├── rootfs_glibc.tgz  ------------------------------------------- rootfs compiled with hisiv600
└── rootfs_uclibc.tgz  ------------------------------------------ rootfs compiled with hisiv500

3.osdrv directory structure：
osdrv
├─Makefile ------------------------------ osdrv compile script
├─tools --------------------------------- directory of all tools
│  ├─board ------------------------------ A variety of single-board tools
│  │  ├─reg-tools-1.0.0 ----------------- tools for accessing memory space and io space
│  │  ├─udev-167 ------------------------ udev toolset
│  │  ├─mtd-utils ----------------------- tool to read and write flash nude
│  │  ├─gdb ----------------------------- gdb tools
│  │  ├─ethtools -------------------------ethtools tools
│  │  └─e2fsprogs ----------------------- e2fsprogs tools
│  └─pc ------------------------------------- tools used on PC
│      ├─jffs2_tool-------------------------- tools for making jffs2 file system
│      ├─cramfs_tool ------------------------ tools for making cramfs file system
│      ├─squashfs4.3 ------------------------ tools for making mksquashfs file system
│      ├─nand_production -------------------- nand Production tools
│      ├─lzma_tool -------------------------- lzma compress tool
│      ├─zlib --------------------------------zlib tool
│      ├─mkyaffs2image -- ------------------- tools for making yaffs2 file system
│      ├─ubi_sh ------------------------------tools for making ubifs
│      └─uboot_tools ------------------------ tools for creating uboot image,xls files,ddr initialization script and reg_info.bin making tool
├─pub --------------------------------------- output directory
│  ├─image_uclibc --------------------------- images compiled with hisiv500: uboot,uImage and images of filesystem
│  ├─image_glibc ---------------------------- images compiled with hisiv600: uboot,uImage and images of filesystem
│  ├─bin ------------------------------------ tools not placed in the rootfs
│  │  ├─pc ---------------------------------- tools used on the PC
│  │  ├─board_uclibc ------------------------ board tools compiled with hisiv500
│  │  └─board_glibc ------------------------- board tools compiled with hisiv600
│  ├─rootfs_uclibc.tgz ---------------------- rootfs compiled with hisiv500
│  └─rootfs_glibc.tgz ----------------------- rootfs compiled with hisiv600
├─opensource--------------------------------- A variety of opensource code
│  ├─busybox -------------------------------- busybox source code
│  ├─uboot ---------------------------------- uboot source code
│  └─kernel --------------------------------- kernel source code
└─rootfs_scripts ---------------------------- scripts to generate rootfs directory

4.Note
(1)Executable files in Linux may become non-executable after copying them to somewhere else under Windows, and result in souce code cannot be compiled. Many symbolic link files will be generated in the souce package after compiling the u-boot or the kernel. The volume of the source package will become very big, because all the symbolic link files are turning out to be real files in Windows. So, DO NOT copy source package in Windows.
(2)If a tool chain needs to be replaced by the other, remove the original compiled files compiled with the former tool chain, and then replace the compiler tool chain with the other. 
(3) compile single-board code
    a.The Hi3531D has floating-point unit and neon. The library provided by the file system is a library with hard soft floating-points and neon. Therefore, add the following options in Makefile when compiling the single-board code.
    b.The tool chain arm-hisiv500-linux- and arm-hisiv600-linux- are based on GCC4.9. From GCC4.7, when compiling for ARMv6 (but not ARMv6-M), ARMv7-A, ARMv7-R, or ARMv7-M, the new option -munaligned-access is active by default, which for some sources generates code that accesses memory on unaligned addresses. This requires the kernel of those systems to enable such accesses. Hi3531d don't support unaligned accesses, so single-board code has to be compiled with -mno-unaligned-access. (http://gcc.gnu.org/gcc-4.7/changes.html)
    c.After the GCC4.8, it uses a more aggressive analysis to derive an upper bound for the number of iterations of loops. This may cause non-conforming programs to no longer work as expected. Recommended that single-board code should be compiled with -fno-aggressive-loop-optimizations to disable this aggressive analysis. (http://gcc.gnu.org/gcc-4.8/changes.html)
For example:
       CFLAGS += -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations
	     CXXFlAGS +=-mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations
Among these, CXXFlAGS may be different according to the specific name in user's Makefile. For example, it may be changed to CPPFLAGS.
