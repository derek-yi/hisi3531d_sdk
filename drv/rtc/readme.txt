/*********************** chinese ************************/
1.֧�ַ�Ƶϵ������
  1��0x51��0x52Ĭ��ֵ�ֱ�Ϊ0x8��0x1b����ȡ�������£�
     ./test -r 0x51;
     ./test -r 0x52;
  2��0x51��0x52д�������»���Ϊ��д��ֵ��������������������������������޷����ȥ��
     ./test -w 0x51 __;
     ./test -w 0x52 __;
2.֧�ֵ�ѹ��⹦��
  1���򿪵�ص�ѹ��⹦��
     ./test -b ON
  2���رյ�ص�ѹ��⹦��
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