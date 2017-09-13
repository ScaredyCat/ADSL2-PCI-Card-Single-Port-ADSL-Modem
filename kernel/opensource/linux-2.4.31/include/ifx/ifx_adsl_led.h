#ifndef _IFX_ADSL_LED_H
#define _IFX_ADSL_LED_H
extern int IFX_ADSL_LED_ModuleInit(void);
extern int IFX_ADSL_LED_Init(void);
extern int IFX_ADSL_LED_ModuleCleanup(void);
extern int IFX_ATM_LED_Callback_Register( int (*adsl_led_cb)(void));
extern int IFX_ATM_LED_Callback_Unregister( int (*adsl_led_cb)(void));
#endif //_IFX_ADSL_LED_H
