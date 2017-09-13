/* Change log:
2.00.00 02/10/2006
   First version for the separation of DSL and MEI
2.00.01 12/10/2006
   Fix some compiling warnings
 */

//#define IFX_ADSL_PORT_RTEMS 1

#if defined(IFX_ADSL_PORT_RTEMS)
#include "danube_mei_rtems.h"
#else
#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <ifx/ifx_adsl_linux.h>

#endif //!defined(IFX_ADSL_PORT_RTEMS)

#define IFX_ADSL_LED_VERSION "2.00.01" //led module version

#define DATA_LED_ON_MODE // turn on atm traffic led by default
#define DATA_LED_ADSL_FW_HANDLE // adsl data led handle by firmware

#if defined(CONFIG_IFX_ADSL_LED) && defined(CONFIG_IFX_ADSL_ATM_LED)
static int IFX_ADSL_LED_FlashTask(void);
#endif


#if defined(CONFIG_IFX_ADSL_LED) && defined(CONFIG_IFX_ADSL_ATM_LED)
int led_status_on=0; // current status of atm traffic led 
int led_need_to_flash=0;  // traffic received/sent, need led to blank
static int stop_led_module=0;	//wakeup and clean led module
static wait_queue_head_t wait_queue_led_polling;// adsl led
#endif //#if defined(CONFIG_IFX_ADSL_LED) && defined(CONFIG_IFX_ADSL_ATM_LED)

/**
 * \brief LED Initialization function
 * This function sends setup the ADSL/Board GPIO and setup Firmware to enable ADSL Link Led
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
int IFX_ADSL_LED_Init(void)
{
	u16	data=(1<<10)|(1<<9);
	u16 CMVMSG[MSG_LENGTH];
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();
	if (adsl_dev == NULL)
	{
		return -EIO;
	}	
		
	IFX_ADSL_BSP_AdslLedInit(adsl_dev,IFX_ADSL_LED_LINK_ID,IFX_ADSL_LED_LINK_TYPE,IFX_ADSL_LED_HD_FW);
	
	data=(1<<10);
#if defined(CONFIG_IFX_ADSL_ATM_LED) && defined (DATA_LED_ADSL_FW_HANDLE)
	data |= (1<<9);
#endif
	// Setup ADSL Link/Data LED
	// setup ADSL GPIO as output mode
	if (IFX_ADSL_SendCmv(H2D_CMV_WRITE, INFO, INFO_GPIO_Control, 0, 1, &data, CMVMSG)!= 0)
	{
		return -EBUSY;
	}
	// setup ADSL GPIO mask 
	if (IFX_ADSL_SendCmv(H2D_CMV_WRITE, INFO, INFO_GPIO_Control, 2, 1, &data, CMVMSG)!= 0)
	{
		return -EBUSY;
	}
	// Let FW to handle ADSL Link LED
	data=0x0a01;
	if (IFX_ADSL_SendCmv(H2D_CMV_WRITE, INFO, INFO_GPIO_Control, 4, 1, &data, CMVMSG)!= 0)
	{
		return -EBUSY;
	}

#ifdef CONFIG_IFX_ADSL_ATM_LED
#ifdef DATA_LED_ADSL_FW_HANDLE
	// Turn ADSL Data LED on
	data=0x0900;
	if (IFX_ADSL_SendCmv(H2D_CMV_WRITE, INFO, INFO_GPIO_Control, 5, 1, &data, CMVMSG)!= 0)
	{
		return -EBUSY;
	}
	IFX_ADSL_BSP_AdslLedInit(adsl_dev,IFX_ADSL_LED_DATA_ID,IFX_ADSL_LED_DATA_TYPE,IFX_ADSL_LED_HD_FW);	
#else
	IFX_ADSL_BSP_AdslLedInit(adsl_dev,IFX_ADSL_LED_DATA_ID,IFX_ADSL_LED_DATA_TYPE,IFX_ADSL_LED_HD_CPU);
#endif
#endif
	return 0;
}

#if defined(CONFIG_IFX_ADSL_ATM_LED)

/**
 * \brief Led Thread main polling function
 * This function polling the data update flag and update the led
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
static int IFX_ADSL_LED_Poll(void *unused)
{
	struct task_struct *tsk = current;

	daemonize();
	strcpy(tsk->comm, "atm_led");
	reparent_to_init();
	sigfillset(&tsk->blocked);

	stop_led_module=0;	//begin polling ...

	while(!stop_led_module){
		// led_status_on: need to check if the led need to turn off
		// led_need_to_flash: ATM Data sent/received, need to blank led
		if (led_status_on || led_need_to_flash)
		{
			IFX_ADSL_LED_FlashTask();
		}
		if (led_status_on)//sleep 250 ms to check if need to turn led off
		{
			MEI_WAIT_EVENT_TIMEOUT(wait_queue_led_polling,250);
		}else
		{
			// sleep and wait adsl_led_flash function to wakeup
			MEI_WAIT_EVENT (wait_queue_led_polling);
		}
	}
	return 0;
}


/**
 * \brief API for atm driver to notify led thread a data coming/sending
 * This function provide a API used by ATM driver, the atm driver call this function once a atm packet sent/received.
 * \return	0 success else fail
 * \ingroup	Internal
 */  
static int IFX_ADSL_LED_Flash(void)
{
	ifx_adsl_device_t *adsl_dev;

	adsl_dev = IFX_ADSL_GetAdslDevice();
	if (adsl_dev == NULL)
	{
		return -EIO;
	}		

	if (IFX_ADSL_IsModemReady(adsl_dev)!=1)	return 0;

	if (led_status_on == 0 && led_need_to_flash == 0)
	{
		MEI_WAKEUP_EVENT(wait_queue_led_polling); //wake up and clean led module
	}

	led_need_to_flash=1;//asking to flash led

	return 0;
}

/**
 * \brief Main task for led controlling.
 * This function control the atm traffic led behavior for blanking/on/off.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
static int IFX_ADSL_LED_FlashTask(void)
{
#ifdef DATA_LED_ADSL_FW_HANDLE
	u16	data=0x0600;
	u16 CMVMSG[MSG_LENGTH];
#endif
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();
	if (adsl_dev == NULL)
	{
		return -EIO;
	}	
	if (!showtime) {
		led_need_to_flash=0;
		led_status_on = 0;
		return 0;
	}

	if (led_status_on == 0 && led_need_to_flash == 1)
	{
#ifdef DATA_LED_ADSL_FW_HANDLE
		data=0x0901;//flash
		IFX_ADSL_SendCmv(H2D_CMV_WRITE, INFO, INFO_GPIO_Control, 5, 1, &data,CMVMSG);	//use GPIO9 for TR68 data led .flash.
#else
		IFX_ADSL_BSP_AdslLedSet(adsl_dev,IFX_ADSL_LED_DATA_ID,IFX_ADSL_LED_FLASH);
#endif
		led_status_on=1;


	}else if (led_status_on == 1 && led_need_to_flash==0)
	{
#ifdef DATA_LED_ADSL_FW_HANDLE
#ifdef DATA_LED_ON_MODE
		data=0x0903;//use GPIO9 for TR68 data led .turn on.
#else
		data=0x0900;//off
#endif
		IFX_ADSL_SendCmv(H2D_CMV_WRITE, INFO, INFO_GPIO_Control, 5, 1, &data,CMVMSG);	//use GPIO9 for TR68 data led .off.
#else
#ifdef DATA_LED_ON_MODE
		IFX_ADSL_BSP_AdslLedSet(adsl_dev,IFX_ADSL_LED_DATA_ID,IFX_ADSL_LED_ON);
#else
		IFX_ADSL_BSP_AdslLedSet(adsl_dev,IFX_ADSL_LED_DATA_ID,IFX_ADSL_LED_OFF);
#endif

#endif
		led_status_on=0;
	}
	led_need_to_flash = 0;
	return 0;
}
#endif //(CONFIG_IFX_ADSL_ATM_LED)


/**
 * \brief LED Module initialization 
 * This function init the LED module.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
int IFX_ADSL_LED_ModuleInit(void)
{
	printk("Infineon ADSL Led Version:%s\n",IFX_ADSL_LED_VERSION);

#ifdef CONFIG_IFX_ADSL_ATM_LED		
	IFX_ADSL_BSP_ATMLedCBRegister(IFX_ADSL_LED_Flash);		
	MEI_INIT_WAKELIST("atmled",wait_queue_led_polling);// adsl led for led function
	kernel_thread(IFX_ADSL_LED_Poll, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGNAL);
#endif	
	return 0;
}

/**
 * \brief LED Module cleanup 
 * This function clean up the LED module.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
int IFX_ADSL_LED_ModuleCleanup(void)
{
#ifdef CONFIG_IFX_ADSL_ATM_LED	
	IFX_ADSL_BSP_ATMLedCBUnregister(IFX_ADSL_LED_Flash);
	stop_led_module=1;			//wake up and clean led module
        MEI_WAKEUP_EVENT(wait_queue_led_polling);
#endif        
        return 0;
}

// export function for ifx_adsl_bacic using
EXPORT_SYMBOL(IFX_ADSL_LED_ModuleInit);
EXPORT_SYMBOL(IFX_ADSL_LED_ModuleCleanup);

