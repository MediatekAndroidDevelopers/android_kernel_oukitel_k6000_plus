/*****************************************************************************
 *
 * Filename:
 * ---------
 *     ov8856_otp.c
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

#include "ov8856mipiraw_Sensor.h"

#define PFX "OV8856_2Lane"

#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)


#define SENSOR_OTP_SWITCH       1   // OTP开关，1开  0关

#if SENSOR_OTP_SWITCH

#define SENSOR_OTP_DATA_LENGTH  1   // Sensor模组厂提供的OTP文档中
#define SENSOR_OTP_I2C_ADDR     0x42    // Sensor模组厂提供的OTP文档中
#define OTP_I2C_SAMEAS_SENSOR   0   // 和sensor 公用I2C 时为1，不公用为0。目前没用到
#define OTP_AF_SWITCH           0
#define OTP_AWB_SWITCH          1
#define OTP_LSC_SWITCH          1

#define GOLDEN_RG_DATA          0x138  // Golden1(185):0x138  Golden2(136):0x139
#define GOLDEN_BG_DATA          0x190  // Golden1(185):0x191  Golden2(136):0x18e

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

static kal_uint16 awb_RG_ratio  = 1024;
static kal_uint16 awb_BG_ratio  = 1024;

#if OTP_I2C_SAMEAS_SENSOR
// 和sensor公用I2C
/*  MTK 参考代码
extern int iReadReg(u16 a_u2Addr , u8 *a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);

#define write_cmos_sensor_otp(addr,para) iWriteReg((u16)addr,(u32)para,1,SENSOR_OTP_ID)
*/

#else
// 和sensor不公用I2C
/*  MTK 参考文档代码
static int iReadReg_8(u8 a_u2Addr , u8 *a_puBuff , u16 i2cId)
{
 int i4RetValue = 0;
 char puReadCmd[1] = {(char)(a_u2Addr & 0xFF)};

 spin_lock(&kdsensor_drv_lock);
 g_pstI2Cclient->addr = (i2cId >> 1);
 g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_DMA_FLAG);
 spin_unlock(&kdsensor_drv_lock);

 i4RetValue = i2c_master_send(g_pstI2Cclient, puReadCmd, 1);
 if(i4RetValue != 1)
 {
     PK_ERR("[CAMERA SENSOR] I2C send failed!!, Addr = 0x%x, data = 0x%x\n", a_u2Addr, *a_puBuff);
     return -1;
 }

 i4RetValue = i2c_master_recv(g_pstI2Cclient, (char *)a_puBuff, a_sizeRecvData);
 if(i4RetValue != 1)
 {
     PK_ERR("[CAMERA SENSOR] I2C read failed!!\n");
     return -1;
 }

 return 0;
}

static kal_uint16 read_eeprom(kal_uint16 addr, u16 i2cid)
{
 kal_uint16 get_byte = 0;
 iReadReg_8((u8)addr, (u8*)&get_byte, i2cid);
 return get_byte;
}
*/

#endif  // OTP_I2C_SAMEAS_SENSOR

#if OTP_AF_SWITCH
/*  MTK 参考代码
static int read_otp_af(void)
{
 int temp = 0;
 kal_uint8 retry = 3;
 u8 AF[4] = {0};    // 可能是全局变量，待会看

 write_cmos_sensor_otp();  //page 9
 write_cmos_sensor_otp();  // for read

 AF[0] = (u8)read_cmos_sensor_otp();   // ar1335 otp文档没有提供
 AF[1] = (u8)read_cmos_sensor_otp();
 AF[2] = (u8)read_cmos_sensor_otp();
 AF[3] = (u8)read_cmos_sensor_otp();

 do {  // check read complete or not
   temp = read_cmos_sensor_otp();    //  这里估计是填确认位寄存器
   if(temp == 1)
   {
     LOG_INF("read_cmos_sensor_otp completed!\n")
     return 1;
   }
   retry--;
 } while(retry > 0);

 LOG_INF("read_cmos_sensor_otp failed!\n")
 return 0;
}
*/
#endif  // OTP_AF_SWITCH

static int sensor_otp_read(void)
{
  kal_uint16 temp;
  kal_uint16 otp_flag, otp_data_group;
  int i = 0;

  temp = READ_CMOS_SENSOR(0x5001);

	WRITE_CMOS_SENSOR(0x5001, (0x00&0x08) | (temp&(~0x08)));
  // read buffer
  WRITE_CMOS_SENSOR(0x3d84,0xc0);
  WRITE_CMOS_SENSOR(0x3d88,0x70);
  WRITE_CMOS_SENSOR(0x3d89,0x10);
  WRITE_CMOS_SENSOR(0x3d8a,0x72);
  WRITE_CMOS_SENSOR(0x3d8b,0x0a);
  WRITE_CMOS_SENSOR(0x3d81,0x01);
  mdelay(10);

  do{

    otp_flag = READ_CMOS_SENSOR(0x7010);
    otp_data_group = (otp_flag >> 6) & 0x01;

    // 读取flag位，判断是用哪组otp数据，ov8856提供了两组数据，优先选择第一组
    if(otp_data_group == 0x01)
    {
      LOG_INF("OTP Group1 data exist\n");
      return 1;
    }
    else
    {
      otp_data_group = (otp_flag >> 4) & 0x01;
      if(otp_data_group == 0x01)
      {
        LOG_INF("OTP Group2 data exist\n");
        return 2;
      }
    }

    i++;
    mdelay(10);

  }while(i < 3);

  LOG_INF("OTP Group1&2 data is not exist!!!\n");
  return -1;
}

#if OTP_LSC_SWITCH
void ov8856_otp_load_lsc(void)
{
  kal_uint16 addr_base = 0, addr_lsc = 0, addr_chk = 0;
  kal_uint16 temp;
  kal_uint32 checksum = 0, otp_checksum = 0;
  kal_uint16 otp_lsc_data[256] = {0};
  int i, ret;

  ret = sensor_otp_read();
  if(ret == -1)
  {
    LOG_INF("ov8856_otp_load_lsc failed!!!\n");
    return;
  }

  switch(ret)
  {
    case 1:
    {
      addr_base = 0x7011;
      addr_lsc =  0x7029;
      addr_chk =  0x7119;
      break;
    }
    case 2:
    {
      addr_base = 0x7019;
      addr_lsc =  0x711a;
      addr_chk =  0x720a;
      break;
    }
    default:
      ;
  }

  for(i = 0; i < 240; i++)
  {
    otp_lsc_data[i] = READ_CMOS_SENSOR(addr_lsc + i);
    checksum += otp_lsc_data[i];
  }

  checksum = checksum % 255 + 1;
  otp_checksum = READ_CMOS_SENSOR(addr_chk);
  if(otp_checksum != checksum)
  {
    LOG_INF("OTP Data Checksum error!!!\n");
    return;
  }

  temp = READ_CMOS_SENSOR(0x5000);
  temp = 0x20 | temp;
  WRITE_CMOS_SENSOR(0x5000,temp);

  for(i = 0; i < 240; i++)
  {
    WRITE_CMOS_SENSOR(0x5900+i,otp_lsc_data[i]);
  }
  LOG_INF("ov8856_otp_load_lsc successful\n");

}
#endif // OTP_LSC_SWITCH

#if OTP_AWB_SWITCH
static int sensor_otp_read_awb(void)
{
  kal_uint16 addr_awb_RG = 0, addr_awb_BG = 0, addr_awb_LSB = 0;
  kal_uint16 temp;
  int ret;

  ret = sensor_otp_read();
  if(ret == -1)
  {
    LOG_INF("ov8856_otp_read_awb failed!!!\n");
    return -1;
  }

  switch(ret)
  {
    case 1:
    {
      addr_awb_RG = 0x7016;
      addr_awb_BG = 0x7017;
      addr_awb_LSB = 0x7018;
      break;
    }
    case 2:
    {
      addr_awb_RG = 0x701E;
      addr_awb_BG = 0x701F;
      addr_awb_LSB = 0x7020;
      break;
    }
    default:
      ;
  }

  // 读取OTP RG BG数据
  temp = READ_CMOS_SENSOR(addr_awb_LSB);
  awb_RG_ratio = (READ_CMOS_SENSOR(addr_awb_RG) << 2) + ((temp >> 6) & 0x03);
  awb_BG_ratio = (READ_CMOS_SENSOR(addr_awb_BG) << 2) + ((temp >> 4) & 0x03);

  LOG_INF("awb_RG_ratio = 0x%x , awb_BG_ratio = 0x%x\n", awb_RG_ratio, awb_BG_ratio);
  return 0;
}

void ov8856_2lane_write_back_otp_awb(void) // 把OTP读取出来的awb值，写回寄存器
{
   unsigned int R_gain = 0, B_gain = 0, G_gain = 0, Base_gain;
   unsigned int rg, bg, RG_Ratio_Typical, BG_Ratio_Typical;
   int ret;

   ret = sensor_otp_read_awb();
   if(ret == -1)
   {
     LOG_INF("write_back_otp_awb failed!!!\n");
     return;
   }

   rg = awb_RG_ratio;
   bg = awb_BG_ratio;

   RG_Ratio_Typical = GOLDEN_RG_DATA;
   BG_Ratio_Typical = GOLDEN_BG_DATA;

   R_gain = (RG_Ratio_Typical * 1000) / rg;
   B_gain = (BG_Ratio_Typical * 1000) / bg;
   G_gain = 1000;

   if(R_gain < G_gain || B_gain < G_gain)
   {
     if(R_gain < B_gain)
        Base_gain = R_gain;
     else
        Base_gain = B_gain;
   }
   else
   {
     Base_gain = G_gain;
   }

   R_gain = 0x400 * R_gain / (Base_gain);
   B_gain = 0x400 * B_gain / (Base_gain);
   G_gain = 0x400 * G_gain / (Base_gain);

   if(R_gain > 0x400)
   {
     WRITE_CMOS_SENSOR(0x5019,R_gain >> 8);
     WRITE_CMOS_SENSOR(0x501A,R_gain & 0x00ff);
   }
   if(G_gain > 0x400)
   {
     WRITE_CMOS_SENSOR(0x501B,G_gain >> 8);
     WRITE_CMOS_SENSOR(0x501C,G_gain & 0x00ff);
   }
   if(B_gain > 0x400)
   {
     WRITE_CMOS_SENSOR(0x501D,B_gain >> 8);
     WRITE_CMOS_SENSOR(0x501E,B_gain & 0x00ff);
   }

   LOG_INF("write_back_otp_awb successful\n");

}

#endif // OTP_AWB_SWITCH

#endif  // SENSOR_OTP_SWITCH
