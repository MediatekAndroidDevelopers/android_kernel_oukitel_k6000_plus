

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include "hct_devices.h"
//#include "Cust_Memory_type.h"

#define HCT_DEVICE "hct_device"
#define HCT_VERSION "hct6755-66-n0-mp7-v1"

typedef int (*hct_dev_print)(struct seq_file *se_f);

typedef int (*hct_dev_set_used)(char * module_name, int pdata);

struct hct_lcm_device_idnfo
{
    struct list_head hct_list;  // list in hct_devices_info
    char lcm_module_descrip[50];
    struct list_head lcm_list;  // list in hct_lcm_info
    LCM_DRIVER* pdata;
    campatible_type type;
};

struct hct_camera_device_dinfo
{
    struct list_head hct_list;
    char camera_module_descrip[20];    
    struct list_head camera_list;
    int  camera_id;
    ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT* pdata;
    campatible_type type;
    CAMERA_DUAL_CAMERA_SENSOR_ENUM  cam_type;
};

struct hct_accsensor_device_dinfo
{
    struct list_head hct_list;
    char sensor_descrip[20];
    struct list_head sensor_list;
    int dev_addr;
    int ps_direction;   // ps is throthold, als/msensor is diriction
    campatible_type type;
    struct acc_init_info * pdata;
};

//MSENSOR
struct hct_msensor_device_dinfo
{
    struct list_head hct_list;
    char sensor_descrip[20];
    struct list_head sensor_list;
    int dev_addr;
    int ps_direction;   // ps is throthold, als/msensor is diriction
    campatible_type type;
    struct mag_init_info * pdata;
};

//ALSPSSENSOR
struct hct_alspssensor_device_dinfo
{
    struct list_head hct_list;
    char sensor_descrip[20];
    struct list_head sensor_list;
    int dev_addr;
    int ps_direction;   // ps is throthold, als/alspssensor is diriction
    campatible_type type;
    struct alsps_init_info * pdata;
};

struct hct_touchpanel_device_dinfo
{
    struct list_head hct_list;     // list in hct_devices_info
    char touch_descrip[20];
    struct list_head touch_list;
    int dev_addr;
    struct tpd_driver_t * pdata;
//    int ps_direction;   // ps is throthold, als/msensor is diriction
    campatible_type type;
};

struct hct_type_info
{
    struct list_head hct_device;  // list in hct_devices_info
    struct list_head hct_dinfo;    // list in hct_touch_device_dinfo
    hct_dev_print type_print;
    hct_dev_set_used set_used;
    int dev_type;
    void * current_used_dinfo;
    struct mutex	info_mutex;
};

/* ------Jay_new_add mis info display --- start*/
struct list_head hctdisp_info_list;

struct hct_mis_disp_info
{
    struct list_head mis_info_list;  // list in hct_devices_info
    void * current_used_dinfo;
};


/* ------Jay_new_add mis info display---end*/

struct hct_devices_info
{
         struct list_head  type_list;   // hct_type_info list
         struct list_head  dev_list;    // all_list

        struct mutex	de_mutex;
};

struct hct_devices_info * hct_devices;
static int hct_camera_info_print(struct seq_file *se_f);
static int hct_touchpanel_info_print(struct seq_file *se_f);
static int hct_accsensor_info_print(struct seq_file *se_f);
static int hct_msensor_info_print(struct seq_file *se_f);
static int hct_alspssensor_info_print(struct seq_file *se_f);

static int hct_lcm_set_used(char * module_name,int pdata);
static int hct_camera_set_used(char * module_name, int pdata);
static int hct_touchpanel_set_used(char * module_name, int pdata);
static int hct_accsensor_set_used(char * module_name, int pdata);
static int hct_msensor_set_used(char * module_name, int pdata);
static int hct_alspssensor_set_used(char * module_name, int pdata);

static int hct_lcm_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * lcm_type_info;    
    struct hct_lcm_device_idnfo * plcm_device;
    int flag = -1;

    seq_printf(se_f, "---------HCT LCM USAGE--------\t \n");
    
    list_for_each_entry(lcm_type_info,&hct_devices->type_list,hct_device){
            if(lcm_type_info->dev_type == ID_LCM_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(plcm_device,&lcm_type_info->hct_dinfo,lcm_list){
                seq_printf(se_f, "      %s\t:   ",plcm_device->lcm_module_descrip);
                if(plcm_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                    seq_printf(se_f, "  used \n");
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    return 0;
}

static void init_device_type_info(struct hct_type_info * pdevice, void *used_info, int type)
{
       memset(pdevice,0, sizeof(struct hct_type_info));
       INIT_LIST_HEAD(&pdevice->hct_device);
       INIT_LIST_HEAD(&pdevice->hct_dinfo);
       pdevice ->dev_type = type;
       if(used_info)
          pdevice->current_used_dinfo=used_info;
       
       if(type == ID_LCM_TYPE)
       {
          pdevice->type_print = hct_lcm_info_print;
          pdevice->set_used= hct_lcm_set_used;
       }
       else if(type == ID_CAMERA_TYPE)
       {
           pdevice->type_print = hct_camera_info_print;
           pdevice->set_used = hct_camera_set_used;
       }
       else if(type == ID_TOUCH_TYPE)
       {
           pdevice->type_print = hct_touchpanel_info_print;
           pdevice->set_used = hct_touchpanel_set_used;
       }
       else if(type == ID_ACCSENSOR_TYPE)
       {
           pdevice->type_print = hct_accsensor_info_print;
           pdevice->set_used = hct_accsensor_set_used;
       }
       else if(type == ID_MSENSOR_TYPE)
       {
           pdevice->type_print = hct_msensor_info_print;
           pdevice->set_used = hct_msensor_set_used;
       }
       else if(type == ID_ALSPSSENSOR_TYPE)
       {
           pdevice->type_print = hct_alspssensor_info_print;
           pdevice->set_used = hct_alspssensor_set_used;
       }

       mutex_init(&pdevice->info_mutex);
       list_add(&pdevice->hct_device,&hct_devices->type_list);
}

static void init_lcm_device(struct hct_lcm_device_idnfo *pdevice, LCM_DRIVER* nLcm)
{
    memset(pdevice,0, sizeof(struct hct_lcm_device_idnfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->lcm_list);
    strcpy(pdevice->lcm_module_descrip,nLcm->name);
    pdevice->pdata=nLcm;
    
}

int hct_lcm_set_used(char * module_name, int pdata)
{
    struct hct_type_info * lcm_type_info;    
    struct hct_lcm_device_idnfo * plcm_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(lcm_type_info,&hct_devices->type_list,hct_device){
             if(lcm_type_info->dev_type == ID_LCM_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(plcm_device,&lcm_type_info->hct_dinfo,lcm_list){
           if(!strcmp(module_name,plcm_device->lcm_module_descrip))
           {
               plcm_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

int hct_lcm_device_add(LCM_DRIVER* nLcm, campatible_type isUsed)
{
    struct hct_type_info * lcm_type_info;    
    struct hct_lcm_device_idnfo * plcm_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(lcm_type_info,&hct_devices->type_list,hct_device){
             if(lcm_type_info->dev_type == ID_LCM_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           lcm_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(lcm_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
           
           if(isUsed)
             init_device_type_info(lcm_type_info, nLcm, ID_LCM_TYPE);
           else
             init_device_type_info(lcm_type_info, NULL, ID_LCM_TYPE);
       }
       else
       {
           if(isUsed &&(lcm_type_info->current_used_dinfo!=NULL))
             printk("~~~~add lcm error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(plcm_device,&lcm_type_info->hct_dinfo,lcm_list){
              if(!strcmp(nLcm->name,plcm_device->lcm_module_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             printk("error ___ lcm type is duplicated \n");
             goto duplicated_faild;
       }
       else
       {
           plcm_device = kmalloc(sizeof(struct hct_lcm_device_idnfo), GFP_KERNEL);
            if(plcm_device == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto devicemalloc_faid;
            }
            
           init_lcm_device(plcm_device,nLcm);
           plcm_device->type=isUsed;
           
           
           list_add(&plcm_device->hct_list, &hct_devices->dev_list);
           list_add(&plcm_device->lcm_list,&lcm_type_info->hct_dinfo);
       }
 
     
     return 0;
duplicated_faild:
devicemalloc_faid:
    kfree(plcm_device);
malloc_faid:
    
    printk("%s: error return: %x: ---\n",__func__,reterror);
    return reterror;
    
}


static int hct_camera_set_used(char * module_name, int pdata)
{
    struct hct_type_info * camera_type_info;    
    struct hct_camera_device_dinfo * pcamera_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(camera_type_info,&hct_devices->type_list,hct_device){
             if(camera_type_info->dev_type == ID_CAMERA_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(pcamera_device,&camera_type_info->hct_dinfo,camera_list){
           if(!strcmp(module_name,pcamera_device->camera_module_descrip))
           {
               pcamera_device->type = DEVICE_USED;
               pcamera_device->cam_type = pdata;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static int hct_camera_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * camera_type_info;    
    struct hct_camera_device_dinfo * pcamera_device;
    int flag = -1;

    seq_printf(se_f, "---------HCT CAMERA USAGE-------- \n");

    list_for_each_entry(camera_type_info,&hct_devices->type_list,hct_device){
            if(camera_type_info->dev_type == ID_CAMERA_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(pcamera_device,&camera_type_info->hct_dinfo,camera_list){
                seq_printf(se_f, "      %20s\t\t:   ",pcamera_device->camera_module_descrip);
                if(pcamera_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                {
                    seq_printf(se_f, "  used     \t");
                    if(pcamera_device->cam_type ==DUAL_CAMERA_MAIN_SENSOR)
                        seq_printf(se_f, "  main camera \n");
                    else if(pcamera_device->cam_type ==DUAL_CAMERA_SUB_SENSOR)
                        seq_printf(se_f, "  sub camera \n");
                    else
                        seq_printf(se_f, " camera unsupportd type \n");
                }
                
                
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    return 0;
}

static void init_camera_device(struct hct_camera_device_dinfo *pdevice, ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT* mCamera)
{
    memset(pdevice,0, sizeof(struct hct_camera_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->camera_list);
    strcpy(pdevice->camera_module_descrip,mCamera->drvname);
    pdevice->pdata=mCamera;
    
}


int hct_camera_device_add(ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT* mCamera, campatible_type isUsed)
{
    struct hct_type_info * camera_type_info;    
    struct hct_camera_device_dinfo * pcamera_device;
    int flag = -1;
    int reterror=0;

     list_for_each_entry(camera_type_info,&hct_devices->type_list,hct_device){
             if(camera_type_info->dev_type == ID_CAMERA_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           camera_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);
           
           if(camera_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
       
           if(isUsed)
             init_device_type_info(camera_type_info, mCamera, ID_CAMERA_TYPE);
           else
             init_device_type_info(camera_type_info, NULL, ID_CAMERA_TYPE);
       }
       else
       {
           if(isUsed &&(camera_type_info->current_used_dinfo!=NULL))
             printk("~~~~add camera error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(pcamera_device,&camera_type_info->hct_dinfo,camera_list){
              if(!strcmp(mCamera->drvname,pcamera_device->camera_module_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             goto duplicated_faild;
       }
       else
       {
           pcamera_device = kmalloc(sizeof(struct hct_camera_device_dinfo), GFP_KERNEL);
            if(camera_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto malloc_faid;
            }
           init_camera_device(pcamera_device,mCamera);
           pcamera_device->type=isUsed;
           
           list_add(&pcamera_device->hct_list, &hct_devices->dev_list);
           list_add(&pcamera_device->camera_list,&camera_type_info->hct_dinfo);
       }
 
          
          return 0;
          
     duplicated_faild:
         kfree(pcamera_device);
     malloc_faid:
         
         printk("%s: error return: %x: ---\n",__func__,reterror);
         return reterror;
    
}


static int hct_touchpanel_set_used(char * module_name, int pdata)
{
    struct hct_type_info * touchpanel_type_info;    
    struct hct_touchpanel_device_dinfo * ptouchpanel_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(touchpanel_type_info,&hct_devices->type_list,hct_device){
             if(touchpanel_type_info->dev_type == ID_TOUCH_TYPE)
            {
                printk("touch type has find break !!\n");
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(ptouchpanel_device,&touchpanel_type_info->hct_dinfo,touch_list){
           if(!strcmp(module_name,ptouchpanel_device->touch_descrip))
           {
               ptouchpanel_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static int hct_touchpanel_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * touchpanel_type_info;    
    struct hct_touchpanel_device_dinfo * ptouchpanel_device;
    int flag = -1;

    seq_printf(se_f, "--------HCT TOUCHPANEL USAGE-------\t \n");

    list_for_each_entry(touchpanel_type_info,&hct_devices->type_list,hct_device){
            if(touchpanel_type_info->dev_type == ID_TOUCH_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(ptouchpanel_device,&touchpanel_type_info->hct_dinfo,touch_list){
                seq_printf(se_f, "      %20s\t:   ",ptouchpanel_device->touch_descrip);
                if(ptouchpanel_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                {
                    seq_printf(se_f, "  used \t");
                    seq_printf(se_f, "    %20s\t \n", ptouchpanel_device->pdata->descript);
                }
                
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    return 0;
    
}

static void init_touchpanel_device(struct hct_touchpanel_device_dinfo *pdevice, struct tpd_driver_t * mTouch)
{
    memset(pdevice,0, sizeof(struct hct_touchpanel_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->touch_list);
    strcpy(pdevice->touch_descrip,mTouch->tpd_device_name);
    pdevice->pdata=mTouch;
    
}


int hct_touchpanel_device_add(struct tpd_driver_t* mTouch, campatible_type isUsed)
{
    struct hct_type_info * touchpanel_type_info;    
    struct hct_touchpanel_device_dinfo * ptouchpanel_device;
    int flag = -1;
    int reterror=0;

     list_for_each_entry(touchpanel_type_info,&hct_devices->type_list,hct_device){
             if(touchpanel_type_info->dev_type == ID_TOUCH_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           touchpanel_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(touchpanel_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
       
           if(isUsed)
             init_device_type_info(touchpanel_type_info, mTouch, ID_TOUCH_TYPE);
           else
             init_device_type_info(touchpanel_type_info, NULL, ID_TOUCH_TYPE);
       }
       else
       {
           if(isUsed &&(touchpanel_type_info->current_used_dinfo!=NULL))
             printk("~~~~add lcm error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(ptouchpanel_device,&touchpanel_type_info->hct_dinfo,touch_list){
              if(!strcmp(mTouch->tpd_device_name,ptouchpanel_device->touch_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             goto duplicated_faild;
       }
       else
       {
           ptouchpanel_device = kmalloc(sizeof(struct hct_touchpanel_device_dinfo), GFP_KERNEL);
            if(ptouchpanel_device == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto malloc_faid;
            }
           init_touchpanel_device(ptouchpanel_device,mTouch);
           ptouchpanel_device->type=isUsed;
           
           list_add(&ptouchpanel_device->hct_list, &hct_devices->dev_list);
           list_add(&ptouchpanel_device->touch_list,&touchpanel_type_info->hct_dinfo);
       }
 
          
          return 0;
          
     duplicated_faild:
         kfree(ptouchpanel_device);
     malloc_faid:
         
         printk("%s: error return: %x: ---\n",__func__,reterror);
         return reterror;
    
}


static int hct_accsensor_set_used(char * module_name, int pdata)
{
    struct hct_type_info * accsensor_type_info;    
    struct hct_accsensor_device_dinfo * paccsensor_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(accsensor_type_info,&hct_devices->type_list,hct_device){
             if(accsensor_type_info->dev_type == ID_ACCSENSOR_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(paccsensor_device,&accsensor_type_info->hct_dinfo,sensor_list){
           if(!strcmp(module_name,paccsensor_device->sensor_descrip))
           {
               paccsensor_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static int hct_accsensor_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * accsensor_type_info;    
    struct hct_accsensor_device_dinfo * paccsensor_device;
    int flag = -1;

    seq_printf(se_f, "--------HCT accsensor USAGE-------\t \n");
    
    list_for_each_entry(accsensor_type_info,&hct_devices->type_list,hct_device){
            if(accsensor_type_info->dev_type == ID_ACCSENSOR_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(paccsensor_device,&accsensor_type_info->hct_dinfo,sensor_list){
                seq_printf(se_f, "      %20s\t\t:   ",paccsensor_device->sensor_descrip);
                if(paccsensor_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                {
                    seq_printf(se_f, "  used \t\n");
                }
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    return 0;
    
}

static void init_accsensor_device(struct hct_accsensor_device_dinfo *pdevice, struct acc_init_info * maccsensor)
{
    memset(pdevice,0, sizeof(struct hct_accsensor_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->sensor_list);
    strcpy(pdevice->sensor_descrip,maccsensor->name);
    pdevice->pdata=maccsensor;
    
}

int hct_accsensor_device_add(struct acc_init_info* maccsensor, campatible_type isUsed)
{
    struct hct_type_info * accsensor_type_info;    
    struct hct_accsensor_device_dinfo * paccsensor_device;
    struct acc_init_info * pacc_sensor;
    int flag = -1;
    int reterror=0;
    
    pacc_sensor = maccsensor;

     list_for_each_entry(accsensor_type_info,&hct_devices->type_list,hct_device){
             if(accsensor_type_info->dev_type == ID_ACCSENSOR_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           accsensor_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(accsensor_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
       
           if(isUsed)
             init_device_type_info(accsensor_type_info, maccsensor, ID_ACCSENSOR_TYPE);
           else
             init_device_type_info(accsensor_type_info, NULL, ID_ACCSENSOR_TYPE);
       }
       else
       {
           if(isUsed &&(accsensor_type_info->current_used_dinfo!=NULL))
             printk("~~~~add lcm error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(paccsensor_device,&accsensor_type_info->hct_dinfo,sensor_list){
              if(!strcmp(maccsensor->name,paccsensor_device->sensor_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             goto duplicated_faild;
       }
       else
       {
           paccsensor_device = (struct hct_accsensor_device_dinfo *)kmalloc(sizeof(struct hct_accsensor_device_dinfo), GFP_KERNEL);
            if(paccsensor_device == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto malloc_faid;
            }
           init_accsensor_device(paccsensor_device , pacc_sensor);

           paccsensor_device->type=isUsed;
           
           list_add(&paccsensor_device->hct_list, &hct_devices->dev_list);
           list_add(&paccsensor_device->sensor_list,&accsensor_type_info->hct_dinfo);
       }
 
          
          return 0;
          
     duplicated_faild:
         kfree(paccsensor_device);
     malloc_faid:
         
         printk("%s: error return: %x: ---\n",__func__,reterror);
         return reterror;
    
}

//MSENSOR START
static int hct_msensor_set_used(char * module_name, int pdata)
{
    struct hct_type_info * msensor_type_info;    
    struct hct_msensor_device_dinfo * pmsensor_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(msensor_type_info,&hct_devices->type_list,hct_device){
             if(msensor_type_info->dev_type == ID_MSENSOR_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(pmsensor_device,&msensor_type_info->hct_dinfo,sensor_list){
           if(!strcmp(module_name,pmsensor_device->sensor_descrip))
           {
               pmsensor_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static int hct_msensor_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * msensor_type_info;    
    struct hct_msensor_device_dinfo * pmsensor_device;
    int flag = -1;

    seq_printf(se_f, "--------HCT msensor USAGE-------\t \n");
    
    list_for_each_entry(msensor_type_info,&hct_devices->type_list,hct_device){
            if(msensor_type_info->dev_type == ID_MSENSOR_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(pmsensor_device,&msensor_type_info->hct_dinfo,sensor_list){
                seq_printf(se_f, "      %20s\t\t:   ",pmsensor_device->sensor_descrip);
                if(pmsensor_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                {
                    seq_printf(se_f, "  used \t\n");
                }
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    
    return 0;
}

static void init_msensor_device(struct hct_msensor_device_dinfo *pdevice, struct mag_init_info * mmsensor)
{
    memset(pdevice,0, sizeof(struct hct_msensor_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->sensor_list);
    strcpy(pdevice->sensor_descrip,mmsensor->name);
    pdevice->pdata=mmsensor;
    
}

int hct_msensor_device_add(struct mag_init_info* mmsensor, campatible_type isUsed)
{
    struct hct_type_info * msensor_type_info;    
    struct hct_msensor_device_dinfo * pmsensor_device;
    int flag = -1;
    int reterror=0;

     list_for_each_entry(msensor_type_info,&hct_devices->type_list,hct_device){
             if(msensor_type_info->dev_type == ID_MSENSOR_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           msensor_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(msensor_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
       
           if(isUsed)
             init_device_type_info(msensor_type_info, mmsensor, ID_MSENSOR_TYPE);
           else
             init_device_type_info(msensor_type_info, NULL, ID_MSENSOR_TYPE);
       }
       else
       {
           if(isUsed &&(msensor_type_info->current_used_dinfo!=NULL))
             printk("~~~~add lcm error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(pmsensor_device,&msensor_type_info->hct_dinfo,sensor_list){
              if(!strcmp(mmsensor->name,pmsensor_device->sensor_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             goto duplicated_faild;
       }
       else
       {
           pmsensor_device = kmalloc(sizeof(struct hct_msensor_device_dinfo), GFP_KERNEL);
            if(pmsensor_device == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto malloc_faid;
            }
           init_msensor_device(pmsensor_device,mmsensor);
           pmsensor_device->type=isUsed;
           
           list_add(&pmsensor_device->hct_list, &hct_devices->dev_list);
           list_add(&pmsensor_device->sensor_list,&msensor_type_info->hct_dinfo);
       }
 
          
          return 0;
          
     duplicated_faild:
         kfree(pmsensor_device);
     malloc_faid:
         
         printk("%s: error return: %x: ---\n",__func__,reterror);
         return reterror;
    
}
//MSENSOR END

//ALSPSSENSOR START
static int hct_alspssensor_set_used(char * module_name, int pdata)
{
    struct hct_type_info * alspssensor_type_info;    
    struct hct_alspssensor_device_dinfo * palspssensor_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(alspssensor_type_info,&hct_devices->type_list,hct_device){
             if(alspssensor_type_info->dev_type == ID_ALSPSSENSOR_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(palspssensor_device,&alspssensor_type_info->hct_dinfo,sensor_list){
           if(!strcmp(module_name,palspssensor_device->sensor_descrip))
           {
               palspssensor_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static int hct_alspssensor_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * alspssensor_type_info;    
    struct hct_alspssensor_device_dinfo * palspssensor_device;
    int flag = -1;

    seq_printf(se_f, "--------HCT alspssensor USAGE-------\t \n");
    
    list_for_each_entry(alspssensor_type_info,&hct_devices->type_list,hct_device){
            if(alspssensor_type_info->dev_type == ID_ALSPSSENSOR_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(palspssensor_device,&alspssensor_type_info->hct_dinfo,sensor_list){
                seq_printf(se_f, "      %20s\t\t:   ",palspssensor_device->sensor_descrip);
                if(palspssensor_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                {
                    seq_printf(se_f, "  used \t\n");
                }
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    return 0;
}

static void init_alspssensor_device(struct hct_alspssensor_device_dinfo *pdevice, struct alsps_init_info * malspssensor)
{
    memset(pdevice,0, sizeof(struct hct_alspssensor_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->sensor_list);
    strcpy(pdevice->sensor_descrip,malspssensor->name);
    pdevice->pdata=malspssensor;
    
}


int hct_alspssensor_device_add(struct alsps_init_info* malspssensor, campatible_type isUsed)
{
    struct hct_type_info * alspssensor_type_info;    
    struct hct_alspssensor_device_dinfo * palspssensor_device;
    int flag = -1;
    int reterror=0;

     list_for_each_entry(alspssensor_type_info,&hct_devices->type_list,hct_device){
             if(alspssensor_type_info->dev_type == ID_ALSPSSENSOR_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           alspssensor_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(alspssensor_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
       
           if(isUsed)
             init_device_type_info(alspssensor_type_info, malspssensor, ID_ALSPSSENSOR_TYPE);
           else
             init_device_type_info(alspssensor_type_info, NULL, ID_ALSPSSENSOR_TYPE);
       }
       else
       {
           if(isUsed &&(alspssensor_type_info->current_used_dinfo!=NULL))
             printk("~~~~add lcm error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(palspssensor_device,&alspssensor_type_info->hct_dinfo,sensor_list){
              if(!strcmp(malspssensor->name,palspssensor_device->sensor_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             goto duplicated_faild;
       }
       else
       {
           palspssensor_device = kmalloc(sizeof(struct hct_alspssensor_device_dinfo), GFP_KERNEL);
            if(palspssensor_device == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto malloc_faid;
            }
           init_alspssensor_device(palspssensor_device,malspssensor);
           palspssensor_device->type=isUsed;
           
           list_add(&palspssensor_device->hct_list, &hct_devices->dev_list);
           list_add(&palspssensor_device->sensor_list,&alspssensor_type_info->hct_dinfo);
       }
 
          
          return 0;
          
     duplicated_faild:
         kfree(palspssensor_device);
     malloc_faid:
         
         printk("%s: error return: %x: ---\n",__func__,reterror);
         return reterror;
    
}
//ALSPSSENSOR END

static int hct_set_device_used(int dev_type, char * module_name, int pdata)
{
    struct hct_type_info * type_info;    
    int ret_val = 0;
    
    list_for_each_entry(type_info,&hct_devices->type_list,hct_device){
        if(type_info->dev_type == dev_type)
        {
            if(type_info->set_used!=NULL)
                type_info->set_used(module_name,pdata);
        }
    }
    
    return ret_val;
}

int hct_set_touch_device_used(char * module_name, int pdata)
{
    int ret_val = 0;
    ret_val = hct_set_device_used(ID_TOUCH_TYPE,module_name,pdata);
    return ret_val;
}


int hct_set_camera_device_used(char * module_name, int pdata)
{
    int ret_val = 0;
    ret_val = hct_set_device_used(ID_CAMERA_TYPE,module_name,pdata);
    return ret_val;
}

int hct_set_accsensor_device_used(char * module_name, int pdata)
{
    int ret_val = 0;
    ret_val = hct_set_device_used(ID_ACCSENSOR_TYPE,module_name,pdata);
    return ret_val;
}

//MSENSOR
int hct_set_msensor_device_used(char * module_name, int pdata)
{
    int ret_val = 0;
    ret_val = hct_set_device_used(ID_MSENSOR_TYPE,module_name,pdata);
    return ret_val;
}

//ALSPSSENSOR
int hct_set_alspssensor_device_used(char * module_name, int pdata)
{
    int ret_val = 0;
    ret_val = hct_set_device_used(ID_ALSPSSENSOR_TYPE,module_name,pdata);
    return ret_val;
}

extern LCM_DRIVER* lcm_driver_list[];
extern unsigned int lcm_count;
extern int  proc_hctinfo_init(void);
extern ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT kdSensorList[];
extern LCM_DRIVER * DISP_GetLcmDrv(void);
static LCM_DRIVER *lcm_drv = NULL;

static void init_lcm(void)
{
    int temp;
	lcm_drv =  DISP_GetLcmDrv();

    if(lcm_count ==1)
    {
        hct_lcm_device_add(lcm_driver_list[0], DEVICE_USED);
    }
    else
    {
        for(temp= 0;temp< lcm_count;temp++)
        {
            if(lcm_drv == lcm_driver_list[temp])                    
                hct_lcm_device_add(lcm_driver_list[temp], DEVICE_USED);
            else
                hct_lcm_device_add(lcm_driver_list[temp], DEVICE_SUPPORTED);
        }
    }
}

/* ------Jay_new_add mis info display --- start*/


void hct_add_display_info(char * buffer, int lenth)
{
    struct hct_mis_disp_info * new_display_info;
    char * buffer_info=NULL;

#if 0
   if(*(buffer+lenth)!="\0")
   {
         printk("display info error\n");
         return;
   }
#endif    
    new_display_info = kmalloc(sizeof(struct hct_mis_disp_info), GFP_KERNEL );
    list_add(&new_display_info->mis_info_list , &hctdisp_info_list);
    buffer_info = kmalloc(lenth , GFP_KERNEL );
    new_display_info->current_used_dinfo = buffer_info;
    memcpy(buffer_info, buffer,lenth );
    
}

void hct_display_info_dump(struct seq_file *m)
{
    struct hct_mis_disp_info * pdisp_info=NULL;

    seq_printf(m, "----HCT INFO DUMP---\t \n");

    list_for_each_entry(pdisp_info,&hctdisp_info_list ,mis_info_list){
        seq_printf(m, "Info:%s\n",(char *)pdisp_info->current_used_dinfo);        
    }
    
}

#ifdef EMMC_COMPATIBLE_NUM

extern char *saved_command_line;


int get_lpddr_emmc_used_index(void)
{
     char *ptr;
     int lpddr_index=0;
     ptr=strstr(saved_command_line,"lpddr_used_index=");
     if(ptr==NULL)
	     return -1;
     ptr+=strlen("lpddr_used_index=");
     lpddr_index=simple_strtol(ptr,NULL,10);
     return lpddr_index;
} 

int get_lpddr_emmc_flash_type(void)
{
     char *ptr;
     int flash_type=0;
     ptr=strstr(saved_command_line,"flash_type=");
     if(ptr==NULL)
	     return 0xFF;
     ptr+=strlen("flash_type=");
     flash_type=simple_strtol(ptr,NULL,10);
     return flash_type;
} 

#endif

#include <accdet.h>
extern struct head_dts_data accdet_dts_data;
#ifndef CONFIG_MTK_SPEAKER
    extern int extamp_sound_mode;
#endif

int hct_device_dump(struct seq_file *m)
{
    static int ilcm_init = 0;
    struct hct_type_info * type_info;    
    
    if(0 == ilcm_init)    
    {
        init_lcm();
        ilcm_init = 1;
    }

    seq_printf(m,"base:%s\n",HCT_VERSION);
    seq_printf(m," \t\t HCT DEVICE DUMP--begin\n");
    list_for_each_entry(type_info,&hct_devices->type_list,hct_device)
    {
	    if(type_info->type_print == NULL)
	    {
	        printk(" error !!! this type [%d] has no dump fun \n",type_info->dev_type);
	    }
	    else
	        type_info->type_print(m);
    }
    
    seq_printf(m,"---------HCT MIC MODE-------- \n");
//    seq_printf(m,"    phone mic mode       :    %d\n",__HCT_PHONE_MIC_MODE__);
    seq_printf(m,"    accdet mic mode      :    %d\n",accdet_dts_data.accdet_mic_mode);
#ifndef CONFIG_MTK_SPEAKER
    seq_printf(m,"   HCT_EXTAMP_HP_MODE   :    %d\n",extamp_sound_mode);
#endif
	
    #ifdef EMMC_COMPATIBLE_NUM
    {
        int tem = 0;
        int flash_idex=0;
        flash_idex = get_lpddr_emmc_used_index();
        seq_printf(m, "----HCT Memory USAGE---\t \n");
        seq_printf(m," EMMC compatible num =%d,type=%x  \n", EMMC_COMPATIBLE_NUM,get_lpddr_emmc_flash_type());
        for(;tem<EMMC_COMPATIBLE_NUM;tem++)
        {
            if(flash_idex==tem)
                seq_printf(m," EMMC COMP[%d] =%s\t --used\n", tem, Cust_emmc_support[tem]);
            else
                seq_printf(m," EMMC COMP[%d] =%s\t--support\n", tem, Cust_emmc_support[tem]);
        }
        
        seq_printf(m," \t\t HCT Memory DUMP--end\n\n");
	
        seq_printf(m, "modem: %s\n", CUSTOM_MODEM);
    }
    #endif
    hct_display_info_dump(m);
    
    return 0;
    
}

//#define HCT_DEVICES_IOCTL_SUPPORT
#ifdef HCT_DEVICES_IOCTL_SUPPORT
#include "hct_devices_ioctl.h"

static int  open_flag=1;
static spinlock_t lock;

static int hct_devices_open (struct inode *ind,struct file * fl){
	spin_lock(&lock);
	if(1 != open_flag){
		spin_unlock(&lock);
		printk("hct_devices_open already open\n");
		return -EBUSY;
	}

	open_flag --;
	spin_unlock(&lock);
	return 0;
}

static	int hct_devices_release (struct inode *inode, struct file *file){
	printk("hct_devices_release close\n");
	open_flag ++;
	return 0;
}
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);


static long hct_devices_ioctl (struct file *file, unsigned int cmd, unsigned long arg){
	int ret = -1;
	int data = -1;
	int adcdata[4] = {0};
	int rawdata = 0;
	switch (cmd) {
	case HCTIOC_GET_EXTAMP_SOUND_MODE :
		ret = copy_to_user((int *)arg, &extamp_sound_mode ,sizeof(int));
		printk("HCTIOC_GET_EXTAMP_SOUND_MODE ret:%d,extamp_sound_mode:%d\n",ret,extamp_sound_mode);
		break;
	case HCTIOC_SET_EXTAMP_SOUND_MODE :
		ret = copy_from_user(&data ,(int *)arg,sizeof(int));
		if (data < 1 || data > 5){
			printk("HCTIOC_SET_EXTAMP_SOUND_MODE err,mode:%d \n",data);
			break;
		}else
			extamp_sound_mode = data;
		ret = copy_to_user((int *)arg, &extamp_sound_mode ,sizeof(int));
		break;
	case HCTIOC_GET_ACCDET_MIC_MODE :
	
		ret = copy_to_user((unsigned int *)arg, & accdet_dts_data.accdet_mic_mode ,sizeof(int));
		break;
	case HCTIOC_SET_ACCDET_MIC_MODE :

		break;
	case HCTIOC_GET_ADC_VALUE:
		ret = copy_from_user(&data ,(int *)arg,sizeof(int));
		if (ret != 0) break;
		if( data < 0 || data > 14 ){
			printk("HCTIOC_GET_ADC_VALUE data err,data:%d \n",data);
		}
		ret = IMM_GetOneChannelValue(data, adcdata, &rawdata);
		printk("hct_adc_read Channel:%d rawdata= %x adcdata= %x %x, ret=%d \n",data,rawdata, adcdata[0], adcdata[1],ret);
		if (ret != 0) break;
		ret = copy_to_user((int *)arg, &rawdata ,sizeof(int));
		break;
	default:
		printk("hct_devices ioctl is not exits\n");	
		break;
	}
	if (ret < 0){
		printk("hct_devices ioctl failed\n");
	}
	return ret;
}


static const struct file_operations hct_devices_fops = {
    .owner =    THIS_MODULE,
//    .write =    hct_devices_write,
//    .read =     hct_devices_read,
    .unlocked_ioctl = hct_devices_ioctl,
//    .compat_ioctl = hct_devices_compat_ioctl,
    .open =     hct_devices_open,
    .release =  hct_devices_release,
    .llseek =   no_llseek,
};


static struct miscdevice hct_devices_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hct_devices",
    .fops = &hct_devices_fops,
};

#endif 

static int hct_devices_probe(struct platform_device *pdev) 
{
        int temp;
        int err =0;
        
        err = proc_hctinfo_init();
        if(err<0)
            goto proc_error;
        
        hct_devices = kmalloc(sizeof(struct hct_devices_info), GFP_KERNEL);
        
        if(hct_devices == NULL)
        {
             printk("%s: error probe becase of mem\n",__func__);
             return -1;
        }
        
        INIT_LIST_HEAD(&hct_devices->type_list);
        INIT_LIST_HEAD(&hct_devices->dev_list);
        mutex_init(&hct_devices->de_mutex);

        INIT_LIST_HEAD(&hctdisp_info_list);
        

       for(temp=0;;temp++)
       {
           if(kdSensorList[temp].SensorInit!=NULL)
           {
               hct_camera_device_add(&kdSensorList[temp], DEVICE_SUPPORTED);
           }
           else
            break;
       }
#ifdef HCT_DEVICES_IOCTL_SUPPORT
	err = misc_register(&hct_devices_misc);
	if (err < 0) {
            	printk("%s: misc_register error!\n",__func__);
		goto  proc_error;
	}


	spin_lock_init(&lock);
	
#endif
	return 0;
 proc_error:
       return err;
}
/*----------------------------------------------------------------------------*/
static int hct_devices_remove(struct platform_device *pdev)
{
#ifdef HCT_DEVICES_IOCTL_SUPPORT
	misc_deregister(&hct_devices_misc);
#endif
	return 0;
}
#ifdef CONFIG_OF
struct of_device_id hct_device_of_match[] = {
	{ .compatible = "mediatek,hct_devices", },
	{},
};
#endif

/*----------------------------------------------------------------------------*/
static struct platform_driver hct_devices_driver = {
	.probe      = hct_devices_probe,
	.remove     = hct_devices_remove,    
      .driver = {
              .name = HCT_DEVICE,
              .owner = THIS_MODULE,
#ifdef CONFIG_OF
              .of_match_table = hct_device_of_match,
#endif
      },

};

#ifndef CONFIG_OF
static struct platform_device hct_dev = {
	.name		  = "hct_devices",
	.id		  = -1,
};
#endif

/*----------------------------------------------------------------------------*/
static int __init hct_devices_init(void)
{

#ifndef CONFIG_OF
    retval = platform_device_register(&hct_dev);
    if (retval != 0){
        return retval;
    }
#endif

    if(platform_driver_register(&hct_devices_driver))
    {
    	printk("failed to register driver");
    	return -ENODEV;
    }
    
    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit hct_devices_exit(void)
{
	platform_driver_unregister(&hct_devices_driver);
}
/*----------------------------------------------------------------------------*/
rootfs_initcall(hct_devices_init);
module_exit(hct_devices_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Jay Zhou");
MODULE_DESCRIPTION("HCT DEVICE INFO");
MODULE_LICENSE("GPL");




