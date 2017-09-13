#ifndef _IFX_ADSL_CEOC_H
#define _IFX_ADSL_CEOC_H
extern MEI_ERROR IFX_ADSL_CEOC_IRQHandler(ifx_adsl_device_t *dev,u32 message);
extern int IFX_ADSL_CEOC_Ioctl(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon);
extern int IFX_ADSL_CEOC_ModuleInit(void);
extern int IFX_ADSL_CEOC_ModuleCleanup(void);
extern int IFX_ADSL_CEOC_Init(void);
#endif //_IFX_ADSL_CEOC_H
