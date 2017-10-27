
#ifndef CS15XX_H
#define CS15XX_H

#include <linux/types.h>


#define SPI_CPHA		0x01
#define SPI_CPOL		0x02

#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

#define SPI_CS_HIGH		0x04
#define SPI_LSB_FIRST	0x08
#define SPI_3WIRE		0x10
#define SPI_LOOP		0x20
#define SPI_NO_CS		0x40
#define SPI_READY		0x80
#define GPIO_FINGER_RST  86
/*---------------------------------------------------------------------------*/
#define KEY_KICK1S       KEY_F13
#define KEY_KICK2S 			 KEY_F14
#define KEY_LONGTOUCH  	 KEY_F15
#define NAV_UP 	         KEY_F16 
#define NAV_DOWN 	       KEY_F17		 
#define NAV_LEFT 	       KEY_F18	 
#define NAV_RIGHT  	     KEY_F19		 
  
#define SPI_IOC_MAGIC			'k'

#define 	_HOME_KEY  0
#define 	_MENU_KEY 1
#define 	_BACK_KEY 2
#define 	_PWR_KEY  3
#define 	_F11_KEY  4 
#define 	_F5_KEY   5 
#define 	_F6_KEY   6 

//
struct spi_ioc_transfer {
	__u64		tx_buf;
	__u64		rx_buf;

	__u32		len;
	__u32		speed_hz;
	__u16		delay_usecs;
	__u8		bits_per_word;
	__u8		cs_change;
	__u32		pad;

};


 
/**************************debug******************************/
#define DEFAULT_DEBUG   (0)
#define SUSPEND_DEBUG   (1)
#define SPI_DEBUG       (2)
#define TIME_DEBUG      (3)
#define FLOW_DEBUG      (4)
#define cs_debug_level 	FLOW_DEBUG  //DEFAULT_DEBUG;

#define CS_DEBUG 	0
#if CS_DEBUG
		#define cs_debug(level, fmt, args...) do{ \
		    if(cs_debug_level >= level) {\
			printk("[ChipSailing]:"fmt"\n", ##args); \
		    } \
		}while(0)
#else
		#define cs_debug(fmt,args...)
#endif
#define cs_error(fmt,arg...)          printk("<<cs_error>> "fmt"\n",##arg)  
#define FUNC_ENTRY()  cs_debug(FLOW_DEBUG,"%s, entry\n", __func__)
#define FUNC_EXIT()   cs_debug(FLOW_DEBUG,"%s, exit\n", __func__)



//
#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				          | SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				          | SPI_NO_CS | SPI_READY)


/* not all platforms use <asm-generic/ioctl.h> or _IOC_TYPECHECK() ... */
#define SPI_MSGSIZE(N) \
	((((N)*(sizeof (struct spi_ioc_transfer))) < (1 << _IOC_SIZEBITS)) \
		? ((N)*(sizeof (struct spi_ioc_transfer))) : 0)

#define SPI_IOC_MESSAGE(N) _IOW(SPI_IOC_MAGIC, 0, char[SPI_MSGSIZE(N)])

/* Read / Write of SPI mode (SPI_MODE_0..SPI_MODE_3) */
#define SPI_IOC_RD_MODE			_IOR(SPI_IOC_MAGIC, 1, __u8)
#define SPI_IOC_WR_MODE			_IOW(SPI_IOC_MAGIC, 1, __u8)



/* Read / Write SPI bit justification */
#define SPI_IOC_RD_LSB_FIRST		_IOR(SPI_IOC_MAGIC, 2, __u8)
#define SPI_IOC_WR_LSB_FIRST		_IOW(SPI_IOC_MAGIC, 2, __u8)

/* Read / Write SPI device word length (1..N) */
#define SPI_IOC_RD_BITS_PER_WORD	_IOR(SPI_IOC_MAGIC, 3, __u8)
#define SPI_IOC_WR_BITS_PER_WORD	_IOW(SPI_IOC_MAGIC, 3, __u8)

/* Read / Write SPI device default max speed hz */
#define SPI_IOC_RD_MAX_SPEED_HZ		_IOR(SPI_IOC_MAGIC, 4, __u32)
#define SPI_IOC_WR_MAX_SPEED_HZ		_IOW(SPI_IOC_MAGIC, 4, __u32)

#define SPI_IOC_RD_SENSOR_ID		_IOWR(SPI_IOC_MAGIC, 5, __u8*) 
#define SPI_IOC_SCAN_IMAGE		_IOWR(SPI_IOC_MAGIC, 6, __u8*) 
#define SPI_IOC_RD_INT_FLAG		_IOR(SPI_IOC_MAGIC, 7, __u8) 
#define SPI_IOC_SENSOR_RESET		_IO(SPI_IOC_MAGIC, 8) 

#define SPI_IOC_KEYMODE_ON		_IO(SPI_IOC_MAGIC, 12) 
#define SPI_IOC_KEYMODE_OFF		_IO(SPI_IOC_MAGIC, 13) 

#define SPI_IOC_NAV_ON		_IO(SPI_IOC_MAGIC, 14) 
#define SPI_IOC_NAV_OFF		_IO(SPI_IOC_MAGIC, 15)

#define SPI_IOC_FINGER_STATUS		_IOWR(SPI_IOC_MAGIC, 16, __u32)


#endif /* CS15XX_H */
 