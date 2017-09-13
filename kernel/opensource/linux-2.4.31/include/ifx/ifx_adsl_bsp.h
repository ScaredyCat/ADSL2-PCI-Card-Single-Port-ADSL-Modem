#ifndef         _IFX_ADSL_BSP_H
#define        	_IFX_ADSL_BSP_H

#define IFX_ADSL_BSP_IOC_IS_MODEM_READY		100

#define IFX_ADSL_BSP_IOC_RESET			IFX_ADSL_IOC_RESET
#define IFX_ADSL_BSP_IOC_REBOOT			IFX_ADSL_IOC_REBOOT
#define IFX_ADSL_BSP_IOC_HALT			IFX_ADSL_IOC_HALT
#define IFX_ADSL_BSP_IOC_RUN			IFX_ADSL_IOC_RUN
#define IFX_ADSL_BSP_IOC_BOOTDOWNLOAD		IFX_ADSL_IOC_DOWNLOAD
#define IFX_ADSL_BSP_IOC_JTAG_ENABLE		IFX_ADSL_IOC_JTAG_ENABLE
#define IFX_ADSL_BSP_IOC_REMOTE			IFX_ADSL_IOC_REMOTE
#define IFX_ADSL_BSP_IOC_DSLSTART		IFX_ADSL_IOC_START
#define IFX_ADSL_BSP_IOC_DEBUG_READ		IFX_ADSL_IOC_READDEBUG
#define IFX_ADSL_BSP_IOC_DEBUG_WRITE		IFX_ADSL_IOC_WRITEDEBUG

typedef enum {
	IFX_ADSL_BSP_EVENT_DYING_GASP = 0,
	IFX_ADSL_BSP_EVENT_CEOC_IRQ,
} ifx_adsl_event_id_t;

typedef union ifx_adsl_cbparam
{
	unsigned long irq_message;
}ifx_adsl_cbparam_t;

typedef struct ifx_adsl_cb_event 
{
	unsigned long ID;
	struct ifx_adsl_device *pDev;
	ifx_adsl_cbparam_t *param;
}ifx_adsl_cb_event_t;

typedef enum {
IFX_ADSL_LED_LINK_ID=0,
IFX_ADSL_LED_DATA_ID,
} ifx_adsl_led_id_t;

typedef enum {
IFX_ADSL_LED_LINK_TYPE=0,
IFX_ADSL_LED_DATA_TYPE,
} ifx_adsl_led_type_t;

typedef enum {
IFX_ADSL_LED_ON=0,
IFX_ADSL_LED_OFF,
IFX_ADSL_LED_FLASH,
} ifx_adsl_led_mode_t;

typedef enum {
IFX_ADSL_CPU_HALT=0,
IFX_ADSL_CPU_RUN,
IFX_ADSL_CPU_RESET,
} ifx_adsl_cpu_mode_t;

typedef enum {
IFX_ADSL_MEMORY_READ=0,
IFX_ADSL_MEMORY_WRITE,
} ifx_adsl_memory_access_type_t;

typedef enum {
	IFX_ADSL_LED_HD_CPU=0,
	IFX_ADSL_LED_HD_FW,
} ifx_adsl_led_handler_t;

typedef struct ifx_adsl_device
{
   int nInUse;	// modem state, update by bsp driver, 
   void *priv;
   unsigned long base_address; // mei base address
   int nIrq;	// irq number
   meidebug lop_debugwr;				//dying gasp
  
}ifx_adsl_device_t;

extern MEI_ERROR IFX_ADSL_BSP_SendCMV(struct ifx_adsl_device *dev,u16 * request, int reply, u16 *response);
extern MEI_ERROR IFX_ADSL_BSP_FWDownload(struct ifx_adsl_device *dev,char * buf, unsigned long size, long * loff, long * current_off);
extern MEI_ERROR IFX_ADSL_BSP_Showtime(struct ifx_adsl_device *dev, u32 rate_fast,u32 rate_intl);
extern int IFX_ADSL_BSP_KernelIoctls(ifx_adsl_device_t *pDev,unsigned int command, unsigned long lon);
extern ifx_adsl_device_t *IFX_ADSL_BSP_DriverHandleGet(int maj,int num);
extern int IFX_ADSL_BSP_EventCBRegister( int (*ifx_adsl_callback)(ifx_adsl_cb_event_t *param));
extern int IFX_ADSL_BSP_EventCBUnregister( int (*ifx_adsl_callback)(ifx_adsl_cb_event_t *param));
extern int IFX_ADSL_BSP_DriverHandleDelete(ifx_adsl_device_t *nHandle);
extern MEI_ERROR IFX_ADSL_BSP_AdslLedInit(struct ifx_adsl_device *dev,ifx_adsl_led_id_t led_number,ifx_adsl_led_type_t type,ifx_adsl_led_handler_t handler);
extern int IFX_ADSL_BSP_ATMLedCBRegister( int (*ifx_adsl_ledcallback)(void));
extern int IFX_ADSL_BSP_ATMLedCBUnregister( int (*ifx_adsl_ledcallback)(void));
extern MEI_ERROR IFX_ADSL_BSP_MemoryDebugAccess(struct ifx_adsl_device *dev,ifx_adsl_memory_access_type_t type, u32 srcaddr, u32 *databuff, u32 databuffsize); 

extern volatile ifx_adsl_device_t *adsl_dev;
#endif //_IFX_ADSL_BSP_H
