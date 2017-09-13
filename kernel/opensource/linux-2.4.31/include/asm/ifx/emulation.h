#ifndef EMULATION_H
#define EMULATION_H
#ifdef CONFIG_USE_EMULATOR

#define CONFIG_USE_VENUS	//Faster, 10M CPU and 192k baudrate
#ifdef CONFIG_USE_VENUS
#define EMULATOR_CPU_SPEED		2500000
#else
#define EMULATOR_CPU_SPEED		25000	//VSTATION is NOT supported yet
#endif

#endif //CONFIG_USE_EMULATOR
#endif //EMULATION_H
