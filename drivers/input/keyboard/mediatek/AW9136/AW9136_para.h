/**************************************************************************
*  AW9136_para.h
* 
*  AW9136 Driver code version 1.0
* 
*  Create Date : 2014/02/12
* 
*  Modify Date : 
*
*  Create by   : lijunjie
* 
**************************************************************************/

//////////////////////////////////////////////////////////////////////
#define AW_AUTO_CALI
//////////////////////////////////////////////////////////////////////
// AW9136 initial register @ mobile active AW_NormalMode()
//////////////////////////////////////////////////////////////////////
#define N_LER1					0x001c			//LED enable
#define N_LER2					0x0000			//LED enable
#define N_IMAX1 				0x3300			//LED MAX light setting
#define N_IMAX2 				0x0003			//LED MAX light setting
#define N_IMAX3 				0x0000			//LED MAX light setting
#define N_IMAX4 				0x0000			//LED MAX light setting
#define N_IMAX5 				0x0000			//LED MAX light setting
#define N_LCR						0x00a1			//LED effect control
#define N_IDLECR 				0x1800			//IDLE time setting
#define N_SLPR					0x0023			// touch key enable
#define N_SCFG1					0x0084			// scan time setting
#define N_SCFG2					0x284				// bit0~3 is sense seting
#define N_OFR1					0x0000			// offset
#define N_OFR2					0x1010			// offset
#define N_OFR3					0x0000			// offset
#define N_THR0					0x2D32			//S1 press thred setting
#define N_THR1					0x2D32			//S2 press thred setting
#define N_THR2					0x2D32			//S3 press thred setting
#define N_THR3					0x2D32			//S4 press thred setting
#define N_THR4					0x2D32			//S5 press thred setting
#define N_THR5					0x2D32			//S6 press thred setting
#define N_SETCNT				0x0202			// debounce
#define N_BLCTH					0x0402			// base trace rate 
#define N_AKSR					0x001c			// AKS 
#define N_INTER					0x001c			// signel click interrupt 
#define N_GDTR					0x0000			// gesture time setting
#define N_GDCFGR				0x0000			//gesture  key select
#define N_TAPR1					0x0000			//double click 1
#define N_TAPR2					0x0000			//double click 2
#define N_TDTR					0x0000			//double click time
#define N_GIER					0x0000			//gesture and double click enable

//////////////////////////////////////////////////////////////////////
// AW9136 initial register @ mobile sleep AW_SleepMode()
//////////////////////////////////////////////////////////////////////
#define S_LER1					0x0000			//LED enable
#define S_LER2					0x0000			//LED enable
#define S_IMAX1 				0x0000			//LED MAX light setting
#define S_IMAX2 				0x0000			//LED MAX light setting
#define S_IMAX3 				0x0000			//LED MAX light setting
#define S_IMAX4 				0x0000			//LED MAX light setting
#define S_IMAX5 				0x0000			//LED MAX light setting
#define S_LCR						0x00a1			//LED effect control
#define S_IDLECR 				0x1800			//IDLE time setting
#define S_SLPR					0x0023			// touch key enable
#define S_SCFG1					0x0084			// scan time setting
#define S_SCFG2					0x287				// bit0~3 is sense seting
#define S_OFR1					0x0000			// offset
#define S_OFR2					0x0000			// offset
#define S_OFR3					0x0000			// offset
#define S_THR0					0x0a0c			//S1 press thred setting
#define S_THR1					0x0a0c			//S2 press thred setting
#define S_THR2					0x0a0c			//S3 press thred setting
#define S_THR3					0x0a0c			//S4 press thred setting
#define S_THR4					0x0a0c			//S5 press thred setting
#define S_THR5					0x0a0c			//S6 press thred setting
#define S_SETCNT				0x0202			// debounce
#define S_BLCTH					0x0402			// base trace rate
#define S_AKSR					0x001c			// AKS 
#define S_INTER					0x0000			// signel click interrupt 
#define S_GDTR					0x0000			// gesture time setting
#define S_GDCFGR				0x025f			//gesture  key select
#define S_TAPR1					0x0022			//double click 1
#define S_TAPR2					0x0000			//double click 2
#define S_TDTR					0x1932			//double click time
#define S_GIER					0x0010			//gesture and double click enable