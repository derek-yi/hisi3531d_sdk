static void TP2827_output(unsigned char chip)
{
    unsigned int tmp;

    tp28xx_byte_write(chip, 0xF1, 0x03);
    tp28xx_byte_write(chip, 0x4D, 0x0f);
    tp28xx_byte_write(chip, 0x4E, 0x0f);

    if( SDR_1CH == output[chip] )
    {
        tp28xx_byte_write(chip, 0xFA, 0x88);
        tp28xx_byte_write(chip, 0xFB, 0x88);
        tp28xx_byte_write(chip, 0xF6, 0x00);
        tp28xx_byte_write(chip, 0xF7, 0x11);
        tp28xx_byte_write(chip, 0xF8, 0x22);
        tp28xx_byte_write(chip, 0xF9, 0x33);
        tp28xx_byte_write(chip, 0x50, 0x00); //
        tp28xx_byte_write(chip, 0x51, 0x00); //
        tp28xx_byte_write(chip, 0x52, 0x00); //
        tp28xx_byte_write(chip, 0x53, 0x00); //
        tp28xx_byte_write(chip, 0xF3, 0x77);
        tp28xx_byte_write(chip, 0xF2, 0x77);
        if(TP2802_720P25V2 == mode || TP2802_720P30V2 == mode || TP2802_PAL == mode || TP2802_NTSC == mode )
        {
            tmp = tp28xx_byte_read(chip, 0xFA);
            tmp &= 0x88;
            tmp |= 0x11;
            tp28xx_byte_write(chip, 0xFA, tmp);
            tmp = tp28xx_byte_read(chip, 0xFB);
            tmp &= 0x88;
            tmp |= 0x11;
            tp28xx_byte_write(chip, 0xFB, tmp);
        }
        else if(FLAG_HALF_MODE == (mode & FLAG_HALF_MODE) )
        {
            tmp = tp28xx_byte_read(chip, 0xFA);
            tmp &= 0x88;
            tmp |= 0x43;
            tp28xx_byte_write(chip, 0xFA, tmp);
            tmp = tp28xx_byte_read(chip, 0xFB);
            tmp &= 0x88;
            tmp |= 0x65;
            tp28xx_byte_write(chip, 0xFB, tmp);
        }
    }
    else if(SDR_2CH == output[chip])
    {
        tp28xx_byte_write(chip, 0xFA, 0x88);
        tp28xx_byte_write(chip, 0xFB, 0x88);
        tp28xx_byte_write(chip, 0xF6, 0x10);
        tp28xx_byte_write(chip, 0xF7, 0x23);
        tp28xx_byte_write(chip, 0xF8, 0x10);
        tp28xx_byte_write(chip, 0xF9, 0x23);
        tp28xx_byte_write(chip, 0x50, 0x00); //
        tp28xx_byte_write(chip, 0x51, 0x00); //
        tp28xx_byte_write(chip, 0x52, 0x00); //
        tp28xx_byte_write(chip, 0x53, 0x00); //
        tp28xx_byte_write(chip, 0xF3, 0x77);
        tp28xx_byte_write(chip, 0xF2, 0x77);
    }
    else if(DDR_2CH == output[chip])
    {
        tp28xx_byte_write(chip, 0xFA, 0x88);
        tp28xx_byte_write(chip, 0xFB, 0x00);
        tp28xx_byte_write(chip, 0x45, 0x54); //PLL 297M
        tp28xx_byte_write(chip, 0xF4, 0xA0); //output clock 148.5M
        tp28xx_byte_write(chip, 0xF6, 0x10); //
        tp28xx_byte_write(chip, 0xF7, 0x23); //
        tp28xx_byte_write(chip, 0xF8, 0x10); //
        tp28xx_byte_write(chip, 0xF9, 0x23); //
        tp28xx_byte_write(chip, 0x50, 0x00); //
        tp28xx_byte_write(chip, 0x51, 0x00); //
        tp28xx_byte_write(chip, 0x52, 0x00); //
        tp28xx_byte_write(chip, 0x53, 0x00); //
        tp28xx_byte_write(chip, 0xF3, 0x77);
        tp28xx_byte_write(chip, 0xF2, 0x77);

    }
    else if(DDR_4CH == output[chip])
    {
        tp28xx_byte_write(chip, 0xFA, 0x88);
        tp28xx_byte_write(chip, 0xFB, 0x00);
        tp28xx_byte_write(chip, 0x45, 0x54); //PLL 297M
        tp28xx_byte_write(chip, 0xF4, 0xA0); //output clock 148.5M
        tp28xx_byte_write(chip, 0xF6, 0x10); //
        tp28xx_byte_write(chip, 0xF7, 0x10); //
        tp28xx_byte_write(chip, 0xF8, 0x10); //
        tp28xx_byte_write(chip, 0xF9, 0x10); //
        tp28xx_byte_write(chip, 0x50, 0xB2); //
        tp28xx_byte_write(chip, 0x51, 0xB2); //
        tp28xx_byte_write(chip, 0x52, 0xB2); //
        tp28xx_byte_write(chip, 0x53, 0xB2); //
        tp28xx_byte_write(chip, 0xF3, 0x77);
        tp28xx_byte_write(chip, 0xF2, 0x77);
    }
    else if( DDR_1CH == output[chip] )
    {
        tp28xx_byte_write(chip, 0xFA, 0x88);
        tp28xx_byte_write(chip, 0xFB, 0x88);
        tp28xx_byte_write(chip, 0x45, 0x54); //PLL 297M
        tp28xx_byte_write(chip, 0xF4, 0xA0); //output clock 148.5M
        tp28xx_byte_write(chip, 0xF6, 0x40);
        tp28xx_byte_write(chip, 0xF7, 0x51);
        tp28xx_byte_write(chip, 0xF8, 0x62);
        tp28xx_byte_write(chip, 0xF9, 0x73);
        tp28xx_byte_write(chip, 0x50, 0x00); //
        tp28xx_byte_write(chip, 0x51, 0x00); //
        tp28xx_byte_write(chip, 0x52, 0x00); //
        tp28xx_byte_write(chip, 0x53, 0x00); //
        tp28xx_byte_write(chip, 0xF3, 0x77);
        tp28xx_byte_write(chip, 0xF2, 0x77);
    }
}
static void TP2827_AQHDP30_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
            tp28xx_byte_write(chip, 0x20, 0x90);
            tp28xx_byte_write(chip, 0x21, 0x86);
            tp28xx_byte_write(chip, 0x22, 0x36);

            tp28xx_byte_write(chip, 0x25, 0xff);
            tp28xx_byte_write(chip, 0x26, 0x01);
            tp28xx_byte_write(chip, 0x27, 0xad);

            tp28xx_byte_write(chip, 0x2b, 0x60);
            tp28xx_byte_write(chip, 0x2d, 0xA0);
            tp28xx_byte_write(chip, 0x2e, 0x40);

            tp28xx_byte_write(chip, 0x30, 0x48);
            tp28xx_byte_write(chip, 0x31, 0x6a);
            tp28xx_byte_write(chip, 0x32, 0xbe);
            tp28xx_byte_write(chip, 0x33, 0x80);
            tp28xx_byte_write(chip, 0x35, 0x15);
            tp28xx_byte_write(chip, 0x39, 0x4c);
}
static void TP2827_AQHDP25_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
            tp28xx_byte_write(chip, 0x20, 0x90);
            tp28xx_byte_write(chip, 0x21, 0x86);
            tp28xx_byte_write(chip, 0x22, 0x36);

            tp28xx_byte_write(chip, 0x25, 0xff);
            tp28xx_byte_write(chip, 0x26, 0x01);
            tp28xx_byte_write(chip, 0x27, 0xad);

            tp28xx_byte_write(chip, 0x2b, 0x60);
            tp28xx_byte_write(chip, 0x2d, 0xA0);
            tp28xx_byte_write(chip, 0x2e, 0x40);

            tp28xx_byte_write(chip, 0x30, 0x48);
            tp28xx_byte_write(chip, 0x31, 0x6f);
            tp28xx_byte_write(chip, 0x32, 0xb5);
            tp28xx_byte_write(chip, 0x33, 0x80);
            tp28xx_byte_write(chip, 0x35, 0x15);
            tp28xx_byte_write(chip, 0x39, 0x4c);
}
static void TP2827_AQXGAP30_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
            tp28xx_byte_write(chip, 0x20, 0x90);
            tp28xx_byte_write(chip, 0x21, 0x86);
            tp28xx_byte_write(chip, 0x22, 0x36);

            tp28xx_byte_write(chip, 0x25, 0xff);
            tp28xx_byte_write(chip, 0x26, 0x01);
            tp28xx_byte_write(chip, 0x27, 0xad);

            tp28xx_byte_write(chip, 0x2b, 0x60);
            tp28xx_byte_write(chip, 0x2d, 0xA0);
            tp28xx_byte_write(chip, 0x2e, 0x40);

            tp28xx_byte_write(chip, 0x30, 0x48);
            tp28xx_byte_write(chip, 0x31, 0x68);
            tp28xx_byte_write(chip, 0x32, 0xbe);
            tp28xx_byte_write(chip, 0x33, 0x80);
            tp28xx_byte_write(chip, 0x39, 0x4c);
}
static void TP2827_AQXGAP25_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
            tp28xx_byte_write(chip, 0x20, 0x90);
            tp28xx_byte_write(chip, 0x21, 0x86);
            tp28xx_byte_write(chip, 0x22, 0x36);

            tp28xx_byte_write(chip, 0x25, 0xff);
            tp28xx_byte_write(chip, 0x26, 0x01);
            tp28xx_byte_write(chip, 0x27, 0xad);

            tp28xx_byte_write(chip, 0x2b, 0x60);
            tp28xx_byte_write(chip, 0x2d, 0xA0);
            tp28xx_byte_write(chip, 0x2e, 0x40);

            tp28xx_byte_write(chip, 0x30, 0x48);
            tp28xx_byte_write(chip, 0x31, 0x6c);
            tp28xx_byte_write(chip, 0x32, 0xbe);
            tp28xx_byte_write(chip, 0x33, 0x80);
            tp28xx_byte_write(chip, 0x39, 0x4c);
}
static void TP2827_AQXGAH30_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
                        tp28xx_byte_write(chip, 0x20, 0x48);
                        tp28xx_byte_write(chip, 0x21, 0x86);
                        tp28xx_byte_write(chip, 0x22, 0x36);

                        tp28xx_byte_write(chip, 0x25, 0xff);
                        tp28xx_byte_write(chip, 0x26, 0x01);
                        tp28xx_byte_write(chip, 0x27, 0x2d);

                        tp28xx_byte_write(chip, 0x2b, 0x60);
                        tp28xx_byte_write(chip, 0x2d, 0x50);
                        tp28xx_byte_write(chip, 0x2e, 0x50);

                        tp28xx_byte_write(chip, 0x30, 0x48);
                        tp28xx_byte_write(chip, 0x31, 0x68);
                        tp28xx_byte_write(chip, 0x32, 0xbe);
                        tp28xx_byte_write(chip, 0x33, 0x80);
}
static void TP2827_AQXGAH25_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
                        tp28xx_byte_write(chip, 0x20, 0x48);
                        tp28xx_byte_write(chip, 0x21, 0x86);
                        tp28xx_byte_write(chip, 0x22, 0x36);

                        tp28xx_byte_write(chip, 0x25, 0xff);
                        tp28xx_byte_write(chip, 0x26, 0x01);
                        tp28xx_byte_write(chip, 0x27, 0x2d);

                        tp28xx_byte_write(chip, 0x2b, 0x60);
                        tp28xx_byte_write(chip, 0x2d, 0x50);
                        tp28xx_byte_write(chip, 0x2e, 0x40);

                        tp28xx_byte_write(chip, 0x30, 0x48);
                        tp28xx_byte_write(chip, 0x31, 0x6c);
                        tp28xx_byte_write(chip, 0x32, 0xb5);
                        tp28xx_byte_write(chip, 0x33, 0x80);
}
static void TP2827_AQHDH25_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
                        tp28xx_byte_write(chip, 0x20, 0x48);
                        tp28xx_byte_write(chip, 0x21, 0x86);
                        tp28xx_byte_write(chip, 0x22, 0x36);

                        tp28xx_byte_write(chip, 0x25, 0xff);
                        tp28xx_byte_write(chip, 0x26, 0x01);
                        tp28xx_byte_write(chip, 0x27, 0x2d);

                        tp28xx_byte_write(chip, 0x2b, 0x60);
                        tp28xx_byte_write(chip, 0x2d, 0x50);
                        tp28xx_byte_write(chip, 0x2e, 0x40);

                        tp28xx_byte_write(chip, 0x30, 0x48);
                        tp28xx_byte_write(chip, 0x31, 0x6f);
                        tp28xx_byte_write(chip, 0x32, 0xb5);
                        tp28xx_byte_write(chip, 0x33, 0x80);
                        tp28xx_byte_write(chip, 0x35, 0x15);
                        //tp28xx_byte_write(chip, 0x39, 0x1c);
}
static void TP2827_AQHDH30_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
                        tp28xx_byte_write(chip, 0x20, 0x48);
                        tp28xx_byte_write(chip, 0x21, 0x86);
                        tp28xx_byte_write(chip, 0x22, 0x36);

                        tp28xx_byte_write(chip, 0x25, 0xff);
                        tp28xx_byte_write(chip, 0x26, 0x01);
                        tp28xx_byte_write(chip, 0x27, 0x2d);

                        tp28xx_byte_write(chip, 0x2b, 0x60);
                        tp28xx_byte_write(chip, 0x2d, 0x50);
                        tp28xx_byte_write(chip, 0x2e, 0x50);

                        tp28xx_byte_write(chip, 0x30, 0x48);
                        tp28xx_byte_write(chip, 0x31, 0x6a);
                        tp28xx_byte_write(chip, 0x32, 0xbe);
                        tp28xx_byte_write(chip, 0x33, 0x80);
                        tp28xx_byte_write(chip, 0x35, 0x15);
                        //tp28xx_byte_write(chip, 0x39, 0x1c);
}
static void TP2827_CQHDH30_DataSet(unsigned char chip)
{
                        tp28xx_byte_write(chip, 0x20, 0x48);
                        tp28xx_byte_write(chip, 0x26, 0x01);
                        tp28xx_byte_write(chip, 0x27, 0x5a);

                        tp28xx_byte_write(chip, 0x2d, 0x30);
                        tp28xx_byte_write(chip, 0x2e, 0x40);

                        tp28xx_byte_write(chip, 0x30, 0x75);
                        tp28xx_byte_write(chip, 0x31, 0x39);
                        tp28xx_byte_write(chip, 0x32, 0xc0);
                        tp28xx_byte_write(chip, 0x33, 0x30);
                        tp28xx_byte_write(chip, 0x35, 0x95);
                        //tp28xx_byte_write(chip, 0x39, 0x0c);
}
static void TP2827_CQHDH25_DataSet(unsigned char chip)
{
                        tp28xx_byte_write(chip, 0x20, 0x48);
                        tp28xx_byte_write(chip, 0x26, 0x01);
                        tp28xx_byte_write(chip, 0x27, 0x5a);

                        tp28xx_byte_write(chip, 0x2d, 0x30);
                        tp28xx_byte_write(chip, 0x2e, 0x40);

                        tp28xx_byte_write(chip, 0x30, 0x75);
                        tp28xx_byte_write(chip, 0x31, 0x39);
                        tp28xx_byte_write(chip, 0x32, 0xc0);
                        tp28xx_byte_write(chip, 0x33, 0x30);
                        tp28xx_byte_write(chip, 0x35, 0x95);
                        //tp28xx_byte_write(chip, 0x39, 0x0c);

}
static void TP2827_CQHDP30_DataSet(unsigned char chip)
{
                        tp28xx_byte_write(chip, 0x20, 0x90);
                        tp28xx_byte_write(chip, 0x26, 0x01);
                        tp28xx_byte_write(chip, 0x27, 0xda);

                        tp28xx_byte_write(chip, 0x2d, 0x60);
                        tp28xx_byte_write(chip, 0x2e, 0x40);

                        tp28xx_byte_write(chip, 0x30, 0x75);
                        tp28xx_byte_write(chip, 0x31, 0x39);
                        tp28xx_byte_write(chip, 0x32, 0xc0);
                        tp28xx_byte_write(chip, 0x33, 0x30);
                        tp28xx_byte_write(chip, 0x35, 0x95);
                        //tp28xx_byte_write(chip, 0x39, 0x0c);
}
static void TP2827_CQHDP25_DataSet(unsigned char chip)
{
                        tp28xx_byte_write(chip, 0x20, 0x90);
                        tp28xx_byte_write(chip, 0x26, 0x01);
                        tp28xx_byte_write(chip, 0x27, 0xda);

                        tp28xx_byte_write(chip, 0x2d, 0x60);
                        tp28xx_byte_write(chip, 0x2e, 0x40);

                        tp28xx_byte_write(chip, 0x30, 0x75);
                        tp28xx_byte_write(chip, 0x31, 0x39);
                        tp28xx_byte_write(chip, 0x32, 0xc0);
                        tp28xx_byte_write(chip, 0x33, 0x30);
                        tp28xx_byte_write(chip, 0x35, 0x95);
                        //tp28xx_byte_write(chip, 0x39, 0x0c);

}
static void TP2827_AQHDP15_DataSet(unsigned char chip)
{
    unsigned char tmp;
                        tmp = tp28xx_byte_read(chip, 0x14);
                        tmp |= 0x60;
                        tp28xx_byte_write(chip, 0x14, tmp);

                        tp28xx_byte_write(chip, 0x20, 0x38);
                        tp28xx_byte_write(chip, 0x21, 0x46);

                        tp28xx_byte_write(chip, 0x25, 0xfe);
                        tp28xx_byte_write(chip, 0x26, 0x01);

                        tp28xx_byte_write(chip, 0x2d, 0x44);
                        tp28xx_byte_write(chip, 0x2e, 0x40);

                        tp28xx_byte_write(chip, 0x30, 0x29);
                        tp28xx_byte_write(chip, 0x31, 0x68);
                        tp28xx_byte_write(chip, 0x32, 0x78);
                        tp28xx_byte_write(chip, 0x33, 0x10);

                        tp28xx_byte_write(chip, 0x3B, 0x05);

}
static void TP2827_AQXGAP18_DataSet(unsigned char chip)
{
    unsigned char tmp;
                        tmp = tp28xx_byte_read(chip, 0x14);
                        tmp |= 0x60;
                        tp28xx_byte_write(chip, 0x14, tmp);
                        tp28xx_byte_write(chip, 0x15, 0x13);
                        tp28xx_byte_write(chip, 0x16, 0x8c);
                        tp28xx_byte_write(chip, 0x18, 0x68);

                        tp28xx_byte_write(chip, 0x20, 0x48);
                        tp28xx_byte_write(chip, 0x21, 0x46);
                        tp28xx_byte_write(chip, 0x25, 0xfe);
                        tp28xx_byte_write(chip, 0x26, 0x01);

                        tp28xx_byte_write(chip, 0x2b, 0x60);
                        tp28xx_byte_write(chip, 0x2d, 0x52);
                        tp28xx_byte_write(chip, 0x2e, 0x40);

                        tp28xx_byte_write(chip, 0x30, 0x29);
                        tp28xx_byte_write(chip, 0x31, 0x65);
                        tp28xx_byte_write(chip, 0x32, 0x2b);
                        tp28xx_byte_write(chip, 0x33, 0xd0);

}
static void TP2827_V3_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tp28xx_byte_write(chip, 0x0c, 0x03);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x20, 0x32);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xad);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0x46);
    tp28xx_byte_write(chip, 0x2e, 0x70);

    tp28xx_byte_write(chip, 0x30, 0x74);
    tp28xx_byte_write(chip, 0x31, 0x9b);
    tp28xx_byte_write(chip, 0x32, 0xa5);
    tp28xx_byte_write(chip, 0x33, 0xe0);
    //tp28xx_byte_write(chip, 0x35, 0x05);
    tp28xx_byte_write(chip, 0x39, 0x4c);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);
}
