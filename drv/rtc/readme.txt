/*********************** chinese ************************/
1.支持分频系数配置
  1）0x51、0x52默认值分别为0x8、0x1b；读取命令如下：
     ./test -r 0x51;
     ./test -r 0x52;
  2）0x51、0x52写方法：下划线为待写入值，以下两个命令必须连续操作，否则无法配进去。
     ./test -w 0x51 __;
     ./test -w 0x52 __;
2.支持低压监测功能
  1）打开电池低压监测功能
     ./test -b ON
  2）关闭电池低压监测功能
     ./test -b OFF
//////////////////////////////////////////////////////////

/*********************** english ************************/
1.Frequency-Division is supported.
  1)The default velue of 0x51 & 0x52 registers is 0x8 & 0x1b;They can be read through these commands:
     ./test -r 0x51;
     ./test -r 0x52;
  2)Write commands of 0x51 & 0x52 registers:replace the "__"with the value you want to write,and the two commands should be executed continuously,otherwise the registers can not be set successfully.
     ./test -w 0x51 __;
     ./test -w 0x52 __;  
2.Battery monitor is supported.
  1)Open bm
     ./test -b ON
  2)Close bm
     ./test -b OFF