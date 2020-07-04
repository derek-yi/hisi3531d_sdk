/*
 * TP2802 Initialization I2C Tables
 */


// Video Format
unsigned char tbl_tp2802_1080p25_raster[] = {
		// Start address 0x15, Size = 9B
		//0x03, 0xD3, 0x80, 0x29, 0x38, 0x47, 0x00, 0x0A, 0x50
		0x03, 0xD3, 0x80, 0x29, 0x38, 0x48, 0x00, 0x0A, 0x50
};

unsigned char tbl_tp2802_1080p30_raster[] = {
		// Start address 0x15, Size = 9B
		0x03, 0xD3, 0x80, 0x29, 0x38, 0x47, 0x00, 0x08, 0x98
};

unsigned char tbl_tp2802_720p25_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x16, 0x00, 0x19, 0xD0, 0x25, 0x00, 0x0F, 0x78
};

unsigned char tbl_tp2802_720p30_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x16, 0x00, 0x19, 0xD0, 0x25, 0x00, 0x0C, 0xE4
};

unsigned char tbl_tp2802_720p50_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x16, 0x00, 0x19, 0xD0, 0x25, 0x00, 0x07, 0xBC
};

unsigned char tbl_tp2802_720p60_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x16, 0x00, 0x19, 0xD0, 0x25, 0x00, 0x06, 0x72
};

unsigned char tbl_tp2802_PAL_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x5f, 0xbc, 0x17, 0x20, 0x17, 0x00, 0x09, 0x48
};

unsigned char tbl_tp2802_NTSC_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x4e, 0xbc, 0x15, 0xf0, 0x07, 0x00, 0x09, 0x38
};

unsigned char tbl_tp2802_3M_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x6C, 0x00, 0x2C, 0x00, 0x68, 0x00, 0x09, 0xC4
};

unsigned char tbl_tp2802_5M_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x1f, 0x20, 0x34, 0x98, 0x7A, 0x00, 0x0B, 0x9A //5M
};
unsigned char tbl_tp2802_4M_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x1f, 0x80, 0x7d, 0xf0, 0x5A, 0x00, 0x0b, 0xb8 //4M15
};
unsigned char tbl_tp2802_3M20_raster[] = {
		// Start address 0x15, Size = 9B
		0x03, 0xa0, 0x00, 0x6e, 0x00, 0x68, 0x00, 0x08, 0xca
};
unsigned char tbl_tp2802_4M12_raster[] = {
		// Start address 0x15, Size = 9B
		0x23, 0x34, 0x80, 0x8c, 0xf0, 0x5A, 0x00, 0x0c, 0xe4 //4M12
};
unsigned char tbl_tp2802_6M10_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0xec, 0x80, 0xb0, 0x08, 0x7c, 0x00, 0x0e, 0xa6 //6M10
};
unsigned char tbl_tp2802_4MH30_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x0f, 0x00, 0x32, 0xa0, 0x55, 0x00, 0x06, 0x72 //4MH30
};
unsigned char tbl_tp2802_4MH25_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x20, 0x00, 0x20, 0xa0, 0x55, 0x00, 0x07, 0xbc //4MH25
};
unsigned char tbl_tp2802_QHD15_raster[] = {
		// Start address 0x15, Size = 9B
		0x13, 0x0f, 0x00, 0x32, 0xa0, 0x5a, 0x00, 0x0c, 0xe4 //2560x1440p15
};

// PLLs
unsigned char tbl_tp2802_common_pll[] = {
		// Start address 0x42, Size = 4B
		0x00, 0x12, 0x07, 0x49
};

// Output Enables
unsigned char tbl_tp2802_common_oe[] = {
		// Start address 0x4B, Size = 11B
		0x10, 0x32, 0x0F, 0xFF, 0x0F, 0x00, 0x0A, 0x0B, 0x1F, 0xFA, 0x27
};

// Rx Common
unsigned char tbl_tp2802_common_rx[] = {
		// Start address 0x7E, Size = 13B
		0x2F, 0x00, 0x07, 0x08, 0x04, 0x00, 0x00, 0x60, 0x10,
		0x06, 0xBE, 0x39, 0xA7
};

// IRQ Common
unsigned char tbl_tp2802_common_irq[] = {
		// Start address 0xB3, Size = 6B
		0xFA, 0x1C, 0x0F, 0xFF, 0x00, 0x00
};

