/*****************************************************************************
 *
 * Filename:
 * ---------
 *     ar1335_otp.c
 *
 * Base / Project:
 * --------
 *     hct6755-66-n0-mp7-v1 / t976n-oq-s38-hd-512g32g-63m-bom7-n
 *
 * Description:
 * ------------
 *     Source code of Image Sensor for OTP
 *
 * Author / Mail:
 * ------------
 *     zhouhongbin@hctmobile.com
 *
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ar1335mipi_Sensor.h"


#define PFX "ar1335_camera_sensor"
#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)


#define SENSOR_OTP_SWITCH       1   // OTP开关，1开  0关

#if SENSOR_OTP_SWITCH

#define SENSOR_OTP_DATA_LENGTH  2   // Sensor模组厂提供的OTP文档中
#define SENSOR_OTP_I2C_ADDR     0x6C    // Sensor模组厂提供的OTP文档中 , 该ar1335 otp I2C地址为0x6C

#define OTP_AWB_SWITCH          1
#define OTP_LSC_SWITCH          1

#define GOLDEN_RG_DATA          0x24a  // Golden1(33):0x247  Golden2(99):0x24c
#define GOLDEN_BG_DATA          0x294  // Golden1(33):0x28f  Golden2(99):0x298
#define GOLDEN_GBGR_DATA        0x400  // Golden1(33):0x3fa  Golden2(99):0x406

#if (SENSOR_OTP_DATA_LENGTH == 2)
#define READ_CMOS_SENSOR read_cmos_sensor_2byte
#define WRITE_CMOS_SENSOR write_cmos_sensor_2byte

static kal_uint16 read_cmos_sensor_2byte(kal_uint32 addr)
{
  kal_uint16 get_byte=0;
	kal_uint16 tmp = 0;

  char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
  iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 2, SENSOR_OTP_I2C_ADDR);

	tmp = get_byte >> 8;
	get_byte = ((get_byte & 0x00ff) << 8) | tmp;

  return get_byte;
}

static void write_cmos_sensor_2byte(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF)};
    iWriteRegI2C(pu_send_cmd, 4, SENSOR_OTP_I2C_ADDR);
}

#else
#define READ_CMOS_SENSOR read_cmos_sensor
#define WRITE_CMOS_SENSOR write_cmos_sensor

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
  /*  MTK 参考代码
  kal_uint16 get_byte = 0;
  iReadReg((u16)addr, (u8)&get_byte, SENSOR_OTP_ID);
  return get_byte;
  */
  kal_uint16 get_byte=0;

  char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
  iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, SENSOR_OTP_I2C_ADDR);

  return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
    iWriteRegI2C(pu_send_cmd, 3, SENSOR_OTP_I2C_ADDR);
}

#endif  // SENSOR_OTP_DATA_LENGTH == 2

static kal_uint16 awb_R_Gr_ratio  = 0x247;  // 1024 Golden1(33):0x247   Golden2(99):0x24c
static kal_uint16 awb_B_Gb_ratio  = 0x28f;  // 1024 Golden1(33):0x28f   Golden2(99):0x298
static kal_uint16 awb_Gb_Gr_ratio = 0x3fa;  // 1024 Golden1(33):0x3fa   Golden2(99):0x406

static void sensor_otp_type_choice(kal_uint16 otp_type)
{   // here type 1100 或者是其他type 3700
	kal_uint16 flag_end=0;
	int i=0;
	/// mclk=12 mhz
	WRITE_CMOS_SENSOR(0x301A,0x0001);  // (0x301A,0x001D)
  mdelay(10);
	WRITE_CMOS_SENSOR(0x301A,0x0218);
	WRITE_CMOS_SENSOR(0x304C,(otp_type&0xff)<<8);  //
	WRITE_CMOS_SENSOR(0x3054,0x0400);
	WRITE_CMOS_SENSOR(0x304A,0x0210);

	do
  {
		flag_end = (READ_CMOS_SENSOR(0x304A));
		if((flag_end&0x60) == 0x60)
    {
			LOG_INF("AR1335 read otp successful 0x304A = 0x%x\n",flag_end);
			break;
		}
		mdelay(10);
		i++;
		LOG_INF("AR1335 read otp error 0x304A = 0x%x,i=%d\n",flag_end,i);
	}while(i<3);
}

#if OTP_LSC_SWITCH
void AR1335_OTP_AUTO_LOAD_LSC(void)
{
	kal_uint16 temp;
  kal_uint32 checksum = 0, otp_checksum = 0;
  kal_uint16 otp_lsc_data[256] = {0};
  int i, addr;

  sensor_otp_type_choice(0x38); //checksum lsc data
  otp_checksum = READ_CMOS_SENSOR(0x3804);

  sensor_otp_type_choice(0x11);
  for(i = 0, addr = 0x3800; addr <= 0x38e2; i++, addr += 2)
  {
    otp_lsc_data[i] = READ_CMOS_SENSOR(addr);
    checksum += otp_lsc_data[i];
  }
  checksum = checksum % 65535 + 1;

  if(checksum == otp_checksum)
  {
	  temp = READ_CMOS_SENSOR(0x3780);
	  WRITE_CMOS_SENSOR(0x3780,temp|0x8000);
    LOG_INF("AR1335_OTP_AUTO_LOAD_LSC successful  checksum = 0x%x , otp_checksum = 0x%x\n", checksum, otp_checksum);
  }
  else
  {
    LOG_INF("AR1335_OTP_AUTO_LOAD_LSC failed!!!  checksum = 0x%x , otp_checksum = 0x%x\n", checksum, otp_checksum);
  }
}
#endif // OTP_LSC_SWITCH

#if OTP_AWB_SWITCH
static int sensor_otp_read_awb(void)
{
	kal_uint16 temp;
	kal_uint16 type37_R3800;
	kal_uint16 type37_R3802;
	kal_uint16 type37_R3804;
	kal_uint16 type37_R3806;
	kal_uint32 Checksum1;
	kal_uint32 Checksum;
	sensor_otp_type_choice(0x30);
	temp = READ_CMOS_SENSOR(0x3800); /// check awb whether exist bit[3]AWB DATA EXIST bit: 0->no exist AWB data; 1->awb data exist
	if((temp&0x0008) == 0x0008)
	{
		LOG_INF("AR1335 read otp AWB exist !!!!!\n");

		sensor_otp_type_choice(0x37); /// read AWB data;
		temp = READ_CMOS_SENSOR(0x3806);  // read the type37 flag
		//if((temp ==0xFFFF)&&((0x304A&0x0040)==0x0040))  // if the flag == 0xffff&0x304A[6]==1 then read AWB
    if(temp == 0xFFFF)
		{
			type37_R3800 = READ_CMOS_SENSOR(0x3800);
			type37_R3802 = READ_CMOS_SENSOR(0x3802);
			type37_R3804 = READ_CMOS_SENSOR(0x3804);
			type37_R3806 = READ_CMOS_SENSOR(0x3806);
			Checksum1 = (type37_R3800+type37_R3802+type37_R3804+type37_R3806)%65535+1;
			sensor_otp_type_choice(0x38); /// read awb checksum data;
			Checksum = READ_CMOS_SENSOR(0x3800);
      LOG_INF("type37_R3800 = 0x%x, type37_R3802 = 0x%x, type37_R3804 = 0x%x, type37_R3806 = 0x%x, Checksum1 = 0x%x, Checksum = 0x%x\n", type37_R3800, type37_R3802, type37_R3804, type37_R3806, Checksum1, Checksum);

			if(Checksum1==Checksum)
			{
				awb_R_Gr_ratio   = type37_R3800;
				awb_B_Gb_ratio   = type37_R3802;
				awb_Gb_Gr_ratio  = type37_R3804;

        LOG_INF("awb_R_Gr_ratio = 0x%x , awb_B_Gb_ratio = 0x%x , awb_Gb_Gr_ratio = 0x%x\n", awb_R_Gr_ratio, awb_B_Gb_ratio, awb_Gb_Gr_ratio);
        return 0;
			}
		}
	}

  return -1;
}

void ar1335_write_back_otp_awb(void) // 把OTP读取出来的awb值，写回寄存器
{
  kal_uint16 R_gain = 0, B_gain = 0, Gb_gain = 0, Gr_gain = 0;
  int ret;
  kal_uint16 temp;
  kal_uint16 RG_Ratio_Typical=0, BG_Ratio_Typical=0,GbGr_ratio_Typical=0;
  kal_uint16 Base_gain=0;

  ret = sensor_otp_read_awb();
  if(ret == -1)
  {
    LOG_INF("ar1335_write_back_otp_awb failed!!!\n");
    return;
  }

  RG_Ratio_Typical = GOLDEN_RG_DATA;
  BG_Ratio_Typical = GOLDEN_BG_DATA;
  GbGr_ratio_Typical = GOLDEN_GBGR_DATA;

  R_gain = RG_Ratio_Typical*1000 / awb_R_Gr_ratio;
  B_gain = BG_Ratio_Typical*1000 / awb_B_Gb_ratio;
  Gb_gain = GbGr_ratio_Typical*1000 / awb_Gb_Gr_ratio;
  Gr_gain = 1000;

  Base_gain = R_gain;
  if(Base_gain>B_gain)  Base_gain = B_gain;
  if(Base_gain>Gb_gain) Base_gain = Gb_gain;
  if(Base_gain>Gr_gain) Base_gain = Gr_gain;

  R_gain = (0x400 * R_gain / (Base_gain))>>6;
  B_gain = (0x400 * B_gain / (Base_gain))>>6;
  Gb_gain = (0x400 * Gb_gain / (Base_gain))>>6;

  temp = READ_CMOS_SENSOR(0x3056);
  WRITE_CMOS_SENSOR(0x3056, ((Gb_gain << 8) | temp));

  temp = READ_CMOS_SENSOR(0x3058);
  WRITE_CMOS_SENSOR(0x3058, ((B_gain << 8) | temp));

  temp = READ_CMOS_SENSOR(0x305A);
  WRITE_CMOS_SENSOR(0x305A, ((R_gain << 8) | temp));

  temp = READ_CMOS_SENSOR(0x305C);
  WRITE_CMOS_SENSOR(0x305C, ((Gb_gain << 8) | temp));

}

#endif // OTP_AWB_SWITCH

#endif  // SENSOR_OTP_SWITCH
