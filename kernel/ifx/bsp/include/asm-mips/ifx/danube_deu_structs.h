#ifndef CLC_T
#define CLC_T
struct clc_controlr_t {
	u32 Res:26;
	u32 FSOE:1;
	u32 SBWE:1;
	u32 EDIS:1;
	u32 SPEN:1;
	u32 DISS:1;
	u32 DISR:1;

};

#endif

#ifndef DES_T
#define DES_T
struct des_t {
	struct des_controlr {	//10h
		u32 GRE:1;
		u32 reserved1:5;
		u32 GO:1;
		u32 STP:1;
		u32 Res2:10;
		u32 F:3;
		u32 O:3;
		u32 BUS:1;
		u32 DAU:1;
		u32 ARS:1;
		u32 SM:1;
		u32 E_D:1;
		u32 M:3;

	} controlr;
	u32 IHR;		//14h
	u32 ILR;		//18h
	u32 K1HR;		//1c
	u32 K1LR;		//
	u32 K2HR;
	u32 K2LR;
	u32 K3HR;
	u32 K3LR;		//30h
	u32 IVHR;		//34h
	u32 IVLR;		//38
	u32 OHR;		//3c
	u32 OLR;		//40
};

#endif

#ifndef AES_T
#define AES_T
struct aes_t {
	struct aes_controlr {

		u32 GRE:1;
		u32 reserved1:4;
		u32 PNK:1;
		u32 GO:1;
		u32 STP:1;
		u32 reserved2:10;
		u32 F:3;	//fbs
		u32 O:3;	//om
		u32 BUS:1;	//bsy
		u32 DAU:1;
		u32 ARS:1;
		u32 SM:1;
		u32 E_D:1;
		u32 KV:1;
		u32 K:2;	//KL

	} controlr;
	u32 ID3R;		//80h
	u32 ID2R;		//84h
	u32 ID1R;		//88h
	u32 ID0R;		//8Ch
	u32 K7R;		//90h
	u32 K6R;		//94h
	u32 K5R;		//98h
	u32 K4R;		//9Ch
	u32 K3R;		//A0h
	u32 K2R;		//A4h
	u32 K1R;		//A8h
	u32 K0R;		//ACh
	u32 IV3R;		//B0h
	u32 IV2R;		//B4h
	u32 IV1R;		//B8h
	u32 IV0R;		//BCh
	u32 OD3R;		//D4h
	u32 OD2R;		//D8h
	u32 OD1R;		//DCh
	u32 OD0R;		//E0h
};
#endif

#ifndef HASH_T
#define HASH_T
struct deu_hash_t {
	struct hash_controlr {
		u32 reserved1:6;
		u32 GO:1;
		u32 INIT:1;
		u32 reserved2:16;
		u32 BSY:1;
		u32 reserved3:2;
		u32 SM:1;
		u32 reserved4:3;
		u32 ALGO:1;

	} controlr;
	u32 MR;			//B4h
	u32 D1R;		//B8h
	u32 D2R;		//BCh
	u32 D3R;		//C0h
	u32 D4R;		//C4h
	u32 D5R;		//C8h
};
#endif

#ifndef DEU_DMA_T
#define DEU_DMA_T
struct deu_dma_t {
	struct dma_controlr {
		u32 reserved1:22;
		u32 BS:2;
		u32 BSY:1;
		u32 reserved2:1;
		u32 ALGO:2;
		u32 RXCLS:2;
		u32 reserved3:1;
		u32 EN:1;

	} controlr;
};
#endif
