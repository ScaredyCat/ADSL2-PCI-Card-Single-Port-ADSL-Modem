#ifndef _KOAM_H
#define _KOAM_H
#define AMAZON_OAM 1

typedef struct _oam_cell{
	u8  cell[52];
	u32 pid;
}IFX_CELL;

#define OAM_CELL_SIZE sizeof(IFX_CELL)

struct amazon_atm_cell_header {
#ifdef CONFIG_CPU_LITTLE_ENDIAN
	struct{
		u32 clp 	:1;	// Cell Loss Priority
		u32 pti		:3;	// Payload Type Identifier
		u32 vci 	:16;	// Virtual Channel Identifier
		u32 vpi		:8;	// Vitual Path Identifier
		u32 gfc 	:4;	// Generic Flow Control
		}bit;
#else
	struct{
		u32 gfc 	:4;	// Generic Flow Control
		u32 vpi		:8;	// Vitual Path Identifier
		u32 vci 	:16;	// Virtual Channel Identifier
		u32 pti		:3;	// Payload Type Identifier
		u32 clp 	:1;	// Cell Loss Priority
		}bit;
#endif

};
#endif
