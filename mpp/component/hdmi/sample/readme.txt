注意事项：
1、需要把整个文件夹拷贝到发布包的component下进行编译或者修改Makefile，放在合适的位置进行编译。
2、该用例音频测试需要做重采样，因此，在运行之前，须将发布包release目录中的libhive_RES.so拷贝到板端的/usr/lib中，并且执行 “chmod 777 ./libhive_RES.so” 修改文件权限。
3、需要把编译生成的可执行文件、音频（mysecret.adpcm）和视频（1080P.h264）码流文件放在同一目录下。
4、通过命令“./sample_hdmi_display”执行测试程序。

