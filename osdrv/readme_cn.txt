1.osdrv 顶层 Makefile 使用说明
本目录下的编译脚本支持选用下文提到的两种工具链中的任何一种进行编译，因此编译时需要带上一个编译参数以指定对应的工具链 -- arm-hisiv500-linux 和 arm-hisiv600-linux。其中，arm-hisiv500-linux工具链对应uclibc库，arm-hisiv600-linux工具链对应glibc库。具体命令如下

(1)编译整个osdrv目录：
	注意：默认不发布内核源码包，只发布补丁文件。内核源码包需自行从开源社区上下载。
	      从linux开源社区下载v3.18.20版本的内核：
     	      1)进入网站：www.kernel.org
     	      2)选择HTTP协议资源的https://www.kernel.org/pub/选项,进入子页面
     	      3)选择linux/菜单项，进入子页面
     	      4)选择kernel/菜单项，进入子页面
     	      5)选择v3.x/菜单项，进入子页面
     	      6)下载linux-3.18.20.tar.gz (或者linux-3.18.20.tar.xz)到osdrv/opensource/kernel目录下                                                                                                                                            

	make OSDRV_CROSS=arm-hisiv500-linux PCI_MODE=XXX FLASH_TYPE=XXX all
	或者
	make OSDRV_CROSS=arm-hisiv600-linux PCI_MODE=XXX FLASH_TYPE=XXX all

参数说明：
	FLASH_TYPE:设为nand参数时，使用Nand Flash;设为spi参数时，使用SPI Nor Flash或SPI Nand Flash。
	PCI_MODE：共有master、slave两个参数。编译PCIE主片版本时使用master参数，编译PCIE从片使用slave参数，默认是master参数。

(2)清除整个osdrv目录的编译文件：
	make OSDRV_CROSS=arm-hisiv500-linux clean
	或者
	make OSDRV_CROSS=arm-hisiv600-linux clean
(3)彻底清除整个osdrv目录的编译文件，除清除编译文件外，还删除已编译好的镜像：
	make OSDRV_CROSS=arm-hisiv500-linux distclean
	或者
	make OSDRV_CROSS=arm-hisiv600-linux distclean

(4)单独编译kernel：
	注意：单独编译内核之前请先阅读osdrv/opensource/kernel下的readme_cn.txt说明。

	待进入内核源代码目录后，执行以下操作
	cp arch/arm/configs/hi3531d_xxx_defconfig .config
	make ARCH=arm CROSS_COMPILE=arm-hisiv500-linux- menuconfig
	make ARCH=arm CROSS_COMPILE=arm-hisiv500-linux- uImage
	或者
	cp arch/arm/configs/hi3531d_xxx_defconfig .config
	make ARCH=arm CROSS_COMPILE=arm-hisiv600-linux- menuconfig
	make ARCH=arm CROSS_COMPILE=arm-hisiv600-linux- uImage

hi3531d_xxx_defconfig参数设置说明：
	为hi3531d_full_defconfig时,使用SPI Nor Flash或SPI Nand Flash，PCIE选择主片模式；
	为hi3531d_full_slave_defconfig时,使用SPI Nor Flash或SPI Nand
Flash，PCIE选择从片模式；
	为hi3531d_nand_defconfig时,使用Nand Flash，PCIE选择主片模式；
	为hi3531d_nand_slave_defconfig时,使用Nand Flash，PCIE选择从片模式；

(5)单独编译uboot：
	待进入boot源代码目录后，执行以下操作

	make ARCH=arm CROSS_COMPILE=arm-hisiv500-linux- hi3531d_xxx_config
	make ARCH=arm CROSS_COMPILE=arm-hisiv500-linux-
	或者
	make ARCH=arm CROSS_COMPILE=arm-hisiv600-linux- hi3531d_xxx_config
	make ARCH=arm CROSS_COMPILE=arm-hisiv600-linux-

	将生成的 u-boot.bin 复制到 osdrv/tools/pc/uboot_tools/ 目录
	./mkboot.sh reg_info_hi3531d.bin u-boot_hi3531d.bin
	将生成可用的 u-boot_hi3531d.bin 镜像文件

参数说明：
	hi3531d_xxx_config设为hi3531d_config时，使用SPI Nor Flash或SPI Nand Flash；
	hi3531d_xxx_config设为hi3531d_nand_config时，使用Nand Flash；

(6)制作文件系统镜像：
	在osdrv/pub/中有已经编译好的文件系统，因此无需再重复编译文件系统，只需要根据单板上flash的规格型号制作文件系统镜像即可。

	SPI Nor Flash使用jffs2格式的镜像，制作jffs2镜像时，需要用到SPI Nor
Flash的块大小。这些信息会在uboot启动时会打印出来。建议使用时先直接运行mkfs.jffs2工具，根据打印信息填写相关参数。下面以块大小为64KB为例：
	osdrv/pub/bin/pc/mkfs.jffs2 -d osdrv/pub/rootfs_uclibc -l -e 0x10000 -o osdrv/pub/rootfs_uclibc_64k.jffs2
	或者
	osdrv/pub/bin/pc/mkfs.jffs2 -d osdrv/pub/rootfs_glibc -l -e 0x10000 -o osdrv/pub/rootfs_glibc_64k.jffs2

	Nand Flash和SPI Nand Flash使用yaffs2格式的镜像，制作yaffs2镜像时，需要用到Nand Flash的pagesize和ecc。这些信息会在uboot启动时会打印出来。建议使用时先直接运行mkyaffs2image工具，根据打印信息填写相关参数。
        示例：
        2KB pagesize、4bit ecc命令格式为：
如果制作Nand的镜像，则使用mkyaffs2image610工具:
	osdrv/pub/bin/pc/mkyaffs2image610 osdrv/pub/rootfs_uclibc osdrv/pub/rootfs_uclibc_2k_4bit.yaffs2 1 2
	或者
	osdrv/pub/bin/pc/mkyaffs2image610 osdrv/pub/rootfs_glibc osdrv/pub/rootfs_glibc_2k_4bit.yaffs2 1 2
	
如果制作SPI Nand的镜像，则使用mkyaffs2image100工具:
	osdrv/pub/bin/pc/mkyaffs2image100 osdrv/pub/rootfs_uclibc osdrv/pub/rootfs_uclibc_2k_4bit.yaffs2 1 2
	或者
	osdrv/pub/bin/pc/mkyaffs2image100 osdrv/pub/rootfs_gclibc osdrv/pub/rootfs_uclibc_2k_4bit.yaffs2 1 2

说明：
	当FLASH_TYPE=spi时，生成的mkyaffs2image工具是mkyaffs2image100；
	当FLASH_TYPE=nand时，生成的mkyaffs2image工具是mkyaffs2image610；
    Nand Flash使用UBI文件系统，在osdrv/tools/pc/ubi_sh下提供mkubiimg.sh工具用于制作UBI文件系统，需要用到Nand
Flash的pagesize、blocksize和UBIFS分区的大小。
	以2KB pagesize, 128KB blocksize和UBI文件系统分区大小32MB为例：
		mkubiimg.sh hi3531d 2k 128k osdrv/pub/rootfs 32M osdrv/pub/bin/pc
	osdrv/pub/rootfs是根文件系统文件夹目录
	osdrv/pub/bin/pc是制作UBI文件系统镜像的工具目录
    生成的rootfs_hi3531d_2k_128k_32M.ubifs，就是最终用于烧写的UBI文件系统镜像。

2. 镜像存放目录说明
编译完的image，rootfs等存放在osdrv/pub目录下
pub
├── bin
│   ├── board_uclibc -------------------------------------------- hisiv500编译出的单板用工具
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
│   ├── board_glibc -------------------------------------------- hisiv600编译出的单板用工具
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
├── image_uclibc ------------------------------------------------ hisiv500编译出的镜像文件
│   ├── rootfs_hi3531d_4k_24bit.yaffs2-------------------------- 4k 24bit yaffs2文件系统镜像
│   ├── rootfs_hi3531d_4k_4bit.yaffs2 -------------------------- 4k 4bit yaffs2 文件系统镜像
│   ├── rootfs_hi3531d_2k_24bit.yaffs2-------------------------- 2k 24bit yaffs2文件系统镜像
│   ├── rootfs_hi3531d_2k_4bit.yaffs2 -------------------------- 2k 4bit yaffs2 文件系统镜像
│   ├── rootfs_hi3531d_2k_128K_32M.ubifs------------------------ 2k 128k UBI 文件系统镜像
│   ├── rootfs_hi3531d_4k_256K_50M.ubifs------------------------- 4k 256k UBI 文件系统镜像
│   ├── rootfs_hi3531d_64k.jffs2 ------------------------------- 64K jffs2 文件系统镜像
│   ├── u-boot-hi3531d.bin ------------------------------------- u-boot镜像
│   └── uImage_hi3531d ----------------------------------------- kernel镜像
├── image_glibc ------------------------------------------------- hisiv600编译出的镜像文件
│   ├── rootfs_hi3531d_4k_24bit.yaffs2-------------------------- 4k 24bit yaffs2文件系统镜像
│   ├── rootfs_hi3531d_4k_4bit.yaffs2 -------------------------- 4k 4bit yaffs2 文件系统镜像
│   ├── rootfs_hi3531d_2k_24bit.yaffs2-------------------------- 2k 24bit yaffs2文件系统镜像
│   ├── rootfs_hi3531d_2k_4bit.yaffs2 -------------------------- 2k 4bit yaffs2 文件系统镜像
│   ├── rootfs_hi3531d_2k_128K_32M.ubifs------------------------ 2k 128K UBI 文件系统镜像
│   ├── rootfs_hi3531d_4k_256K_50M.ubifs------------------------- 4k 256k UBI 文件系统镜像
│   ├── rootfs_hi3531d_64k.jffs2 ------------------------------- 64K jffs2 文件系统镜像
│   ├── u-boot-hi3531d.bin ------------------------------------- u-boot镜像
│   └── uImage_hi3531d ----------------------------------------- kernel镜像
├── rootfs_glibc.tgz  ------------------------------------------- hisiv600编译出的rootfs文件系统
└── rootfs_uclibc.tgz  ------------------------------------------ hisiv500编译出的rootfs文件系统

3.osdrv目录结构说明：
osdrv
├─Makefile -------------------------------------- osdrv目录编译脚本
├─tools ----------------------------------------- 存放各种工具的目录
│  ├─board -------------------------------------- 各种单板上使用工具
│  │  ├─reg-tools-1.0.0 ------------------------- 寄存器读写工具
│  │  ├─udev-167 -------------------------------- udev工具集
│  │  ├─mtd-utils ------------------------------- flash裸读写工具集
│  │  ├─gdb ------------------------------------- gdb工具
│  │  ├─ethtools -------------------------------- ethtools工具
│  │  └─e2fsprogs ------------------------------- mkfs工具集
│  └─pc ----------------------------------------- 各种pc上使用工具
│      ├─jffs2_tool------------------------------ jffs2文件系统制作工具
│      ├─cramfs_tool ---------------------------- cramfs文件系统制作工具
│      ├─squashfs4.3 ---------------------------- squashfs文件系统制作工具
│      ├─nand_production ------------------------ nand量产工具
│      ├─lzma_tool ------------------------------ lzma压缩工具
│      ├─mkyaffs2image -------------------------- yaffs2文件系统制作工具
│      ├─ubi_sh ----------------------------------ubifs文件系统制作工具
│      ├─zlib ----------------------------------- zlib工具
│      └─uboot_tools ---------------------------- uboot镜像制作工具、xls文件及ddr初始化脚本、reg_info.bin制作工具
├─pub ------------------------------------------- 存放各种镜像的目录
│  ├─image_uclibc ------------------------------- 基于hisiv500工具链编译，可供FLASH烧写的映像文件，包括uboot、内核、文件系统
│  ├─image_glibc -------------------------------- 基于hisiv600工具链编译，可供FLASH烧写的映像文件，包括uboot、内核、文件系统
│  ├─bin ---------------------------------------- 各种未放入根文件系统的工具
│  │  ├─pc -------------------------------------- 在pc上执行的工具
│  │  ├─board_uclibc ---------------------------- 基于hisiv500工具链编译，在单板上执行的工具
│  │  └─board_glibc ----------------------------- 基于hisiv600工具链编译，在单板上执行的工具
│  ├─rootfs_uclibc.tgz -------------------------- 基于hisiv500工具链编译的根文件系统
│  └─rootfs_glibc.tgz --------------------------- 基于hisiv600工具链编译的根文件系统
├─opensource------------------------------------- 存放各种开源源码目录
│  ├─busybox ------------------------------------ 存放busybox源代码的目录
│  ├─uboot -------------------------------------- 存放uboot源代码的目录
│  └─kernel ------------------------------------- 存放kernel源代码的目录
└─rootfs_scripts -------------------------------- 存放根文件系统制作脚本的目录

4.注意事项
(1)在windows下复制源码包时，linux下的可执行文件可能变为非可执行文件，导致无法编译使用；u-boot或内核下编译后，会有很多符号链接文件，在windows下复制这些源码包, 会使源码包变的巨大，因为linux下的符号链接文件变为windows下实实在在的文件，因此源码包膨胀。因此使用时请注意不要在windows下复制源代码包。
(2)使用某一工具链编译后，如果需要更换工具链，请先将原工具链编译文件清除，然后再更换工具链编译。
(3)编译板端软件
    a.Hi3531D具有浮点运算单元和neon。文件系统中的库是采用软浮点和neon编译而成，因此请用户注意，所有Hi3531D板端代码编译时需要在Makefile里面添加选项-mcpu=cortex-a9、-mfloat-abi=softfp和-mfpu=neon。
    b.v500和v600工具链都是基于GCC4.9的。从GCC4.7开始，在编译基于ARMv6 (除了ARMv6-M), ARMv7-A, ARMv7-R, or ARMv7-M的代码时，为了支持以地址非对齐方式访问内存，默认激活了-munaligned-access选项，但这需要系统的内核支持这种访问方式。Hi3531d不支持地址非对齐的内存访问方式，所以板端代码在编译的时候需要显式加上选项-mno-unaligned-access。(http://gcc.gnu.org/gcc-4.7/changes.html)
    c.GCC4.8以上版本使用了更激进的循环上界分析策略，这有可能导致一些不相容的程序运行出错。建议板端代码在编译的时候使用-fno-aggressive-loop-optimizations关闭此优化选项，以避免代码运行时出现奇怪的错误。(http://gcc.gnu.org/gcc-4.8/changes.html)
    如：
       CFLAGS += -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations
	     CXXFlAGS +=-mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations
	其中CXXFlAGS中的XX根据用户Makefile中所使用宏的具体名称来确定，e.g:CPPFLAGS。
