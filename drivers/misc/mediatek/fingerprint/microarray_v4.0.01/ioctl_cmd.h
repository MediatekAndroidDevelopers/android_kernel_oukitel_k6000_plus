/* Copyright (C) MicroArray
 * MicroArray Fprint Driver Code
 * madev.h
 * Date: 2016-10-12
 * Version: v4.0.01 
 * Author: guq
 * Contact: guq@microarray.com.cn
 */

#ifndef __IOCTL_CMD_H_
#define __IOCTL_CMD_H_

#define MA_DRV_VERSION  (0x00004004)

//user for the ioctl,new version
//#define MA_IOC_MAGIC    (('M'<<24)|('A'<<16)|('F'<<8)|'P')
#define MA_IOC_MAGIC 	'M'
//#define MA_IOC_INIT   _IOR(MA_IOC_MAGIC, 0, unsigned char)
#define MA_IOC_DELK     _IO(MA_IOC_MAGIC,  1)					//dealy lock
#define MA_IOC_SLEP     _IO(MA_IOC_MAGIC,  2)					//sleep, remove the process out of the runqueue
#define MA_IOC_WKUP     _IO(MA_IOC_MAGIC,  3)					//wake up, sechdule the process into the runqueeue
#define MA_IOC_ENCK     _IO(MA_IOC_MAGIC,  4)					//only use in tee while the spi clock is not enable
#define MA_IOC_DICK		_IO(MA_IOC_MAGIC,  5)					//disable spi clock
#define MA_IOC_EINT		_IO(MA_IOC_MAGIC,  6)					//enable irq
#define MA_IOC_DINT		_IO(MA_IOC_MAGIC,  7)					//disable irq
#define MA_IOC_TPDW		_IO(MA_IOC_MAGIC,  8)					//tap DOWN
#define MA_IOC_TPUP		_IO(MA_IOC_MAGIC,  9)					//tap UP
#define MA_IOC_SGTP		_IO(MA_IOC_MAGIC,  11)					//single tap
#define MA_IOC_DBTP		_IO(MA_IOC_MAGIC,  12)					//double tap
#define MA_IOC_LGTP		_IO(MA_IOC_MAGIC,  13)					//log tap

#define MA_IOC_VTIM		_IOR(MA_IOC_MAGIC,  14, unsigned char)					//version time	not use
#define MA_IOC_CNUM		_IOR(MA_IOC_MAGIC,  15, unsigned char)					//cover num	not use
#define MA_IOC_SNUM		_IOR(MA_IOC_MAGIC,  16, unsigned char)					//sensor type   not use

#define MA_IOC_UKRP		_IOW(MA_IOC_MAGIC,  17, unsigned char)					//user define the report key
#define MA_IOC_NAVW		_IO(MA_IOC_MAGIC,   18)			//navigation   up
#define MA_IOC_NAVA		_IO(MA_IOC_MAGIC,   19)			//navigation   left
#define MA_IOC_NAVS		_IO(MA_IOC_MAGIC,   20)			//navigation   down
#define MA_IOC_NAVD		_IO(MA_IOC_MAGIC,   21)			//navigation   right 
//the navigation cmd was referencing from e-game

#define MA_IOC_EIRQ		_IO(MA_IOC_MAGIC,   31)			//request irq
#define MA_IOC_DIRQ		_IO(MA_IOC_MAGIC,   32)			//free irq
#define MA_IOC_SPAR		_IOW(MA_IOC_MAGIC,   33, unsigned int)		//register write reserve
#define MA_IOC_GPAR		_IOR(MA_IOC_MAGIC,   34, unsigned int)		//register read reserve
#define MA_IOC_GVER		_IOR(MA_IOC_MAGIC,   35, unsigned int)		//get the driver version,the version mapping in the u32 is the final  4+4+8,as ******** ******* ****(major verson number) ****(minor version number) ********(revised version number), the front 16 byte is reserved.
#define MA_IOC_PWOF             _IO(MA_IOC_MAGIC,    36)
#define MA_IOC_PWON             _IO(MA_IOC_MAGIC,    37)
#define MA_IOC_STSP             _IOW(MA_IOC_MAGIC,   38, unsigned int)		//set the spi speed
#define MA_IOC_FD_WAIT_CMD     _IO(MA_IOC_MAGIC, 39)						//use for fingerprintd to wait the factory apk call
#define MA_IOC_TEST_WAKE_FD    _IO(MA_IOC_MAGIC, 40)						//use factory apk wake up fingerprintd
#define MA_IOC_TEST_WAIT_FD_RET     _IOR(MA_IOC_MAGIC, 41, unsigned int)	//use factory to sleep and wait the fingerprintd complete to send back the result
#define MA_IOC_FD_WAKE_TEST_RET     _IOW(MA_IOC_MAGIC, 42, unsigned int)	//fingerprintd send the result and wake up the factory apk		
#define MA_IOC_GET_SCREEN 			_IOR(MA_IOC_MAGIC, 43, unsigned int)	//get the system's screen 
#define MA_IOC_GET_INT_STATE		_IOR(MA_IOC_MAGIC, 44, unsigned int)	//get the int pin state high or low
#define MA_IOC_PWRST                _IO(MA_IOC_MAGIC, 45) 					//power reset
//ioctl end

#endif

