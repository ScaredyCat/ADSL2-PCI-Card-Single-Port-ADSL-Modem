
/*
 *
 * Copyright (C) Eicon Technology Corporation, 2000.
 *
 * This source file is supplied for the exclusive use with Eicon
 * Technology Corporation's range of DIVA Server Adapters.
 *
 * Eicon File Revision :    1.10  
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY OF ANY KIND WHATSOEVER INCLUDING ANY 
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */



#include    <memory.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    <fcntl.h>
#include    <unistd.h>
#include    <sys/ioctl.h>
#include    "sys.h"
#include    "pc_maint.h"
#include    "divas.h"
#include    "load.h"
#include    "diva.h"
#include    "diload.h"

#include    "linuxcfg.h"

void waitkey (void);
byte read_dec(char *str,word *val);
byte read_hex(char *str,dword *val);
byte check_oa(char *str);
byte check_osa(char *str);
byte check_spid(char *str);
void usage(char *name);

dia_config_t options;
dia_card_t card;
int load_all_cards = FALSE;
int card_idl;

byte cards[20], num_cards;
byte new_cards[20], new_num_cards = 0;
byte real_card_id[20];
byte flavours[20];
char OSA[8][MAX_ADDR];
char OAD[8][MAX_ADDR];
char SPID[8][MAX_SPID];


char *flavour_str[] = { 
			"etsi",
			"ni",
			"5ess",
			"1tr6",
			"franc",
			"japan" };

#define DMLT_PRI  0x01
#define DMLT_BRI  0x02
#define DMLT_4BRI 0x04

typedef struct  {
	byte pri;
	byte bri;
	dword multi;
	byte id;
	const char* name;
	const char*  image;
	const char*  description;
} diva_dmlt_protocols_t;

diva_dmlt_protocols_t dmlt_protocols [] = {
/* pri, bri, dmlt, id name    image     */
	{1,   1,   0,    0, "1TR6", "te_1tr6",
"Germany, old protocol for PABX"},
	{1,   1,   1,    1, "ETSI", "te_etsi",
"DSS1, Europe (Germany, ...)"},
	{1,   1,   1,    2, "FRANC", "te_etsi",
"VN3, France, old protocol for PABX"},
	{1,   1,   1,    3, "BELG",  "te_etsi",
"NET3, Belgien, old protocol for PABX"},
	{1,   0,   1,    4, "SWED",  "te_etsi",
"DSS1 with CRC4 off, Sweden, Benelux"},
	{1,   1,   1,    5, "NI",    "te_us",
"NI1, NI2, North America, National ISDN"},
	{1,   1,   1,    6, "5ESS",  "te_us",
"5ESS, North America, AT&T"},
	{1,   1,   0,    7, "JAPAN", "te_japan",
"Japan, INS-NET64"},
	{1,   1,   1,    8, "ATEL",  "te_etsi",
"ATEL, Australia, old TPH1962"},
	{0,   1,   1,    9, "US",    "te_us",
"North America, Auto Detect"/* V.6 < 5ESS Lucent, National ISDN*/},
	{1,   1,   1,    10, "ITALY", "te_etsi",
"DSS1, Italy"},
	{1,   1,   1,    11, "TWAN",  "te_etsi",
"DSS1, Taiwan"},
	{1,   1,   1,    12, "AUSTRAL", "te_etsi",
"Australia, Microlink (TPH1962), On Ramp ETSI"},
	{1,   0,   1,    13, "4ESS_SDN", "te_us",
"4ESS Software Defined Network"},
	{1,   0,   1,    14, "4ESS_SDS", "te_us",
"4ESS Switched Digital Service"},
	{1,   0,   1,    15, "4ESS_LDS", "te_us",
"4ESS Long Distance Service"},
	{1,   0,   1,    16, "4ESS_MGC", "te_us",
"4ESS Megacom"},
	{1,   0,   1,    17, "4ESS_MGI", "te_us",
"4ESS Megacom International"},
	{1,   1,   1,    18, "HONGKONG", "te_etsi",
"Hongkong"},
	{1,   0,   1,    19, "RBSCAS",  "te_dmlt",
"Robbed Bit Signaling, CAS"},
#if 0
	{1,   1,   7,    20, "CORNETN", "te_dmlt",
"Siemens CORNET-N"},
#endif
	{1,   1,   7,    21, "QSIG",    "te_dmlt",
"QSIG, Intra PABX link protocol"},
	{0,   0,   0,    0xff, 0,       0,           0}
};

static void diva_load_print_supported_protocols (const char* tab) {
	int i, s;
	char buffer[256];

	if (!tab) tab = "";

	for (i = 0; dmlt_protocols [i].name; i++) {
		s = sprintf(buffer,"%s-f ", tab);
		strcat (buffer, dmlt_protocols [i].name);
		s = strlen (buffer) - s;
		while (s++ < 9) {
			strcat(buffer, " ");
		}
		strcat (buffer, dmlt_protocols [i].description);
		s = strlen (buffer) - s;
		while (s++ < 54) {
			strcat(buffer, ".");
		}
		if (dmlt_protocols[i].pri && dmlt_protocols[i].bri) {
			strcat (buffer, " PRI & BRI");
		} else if (dmlt_protocols[i].pri) {
			strcat (buffer, " PRI");
		} else if (dmlt_protocols[i].bri) {
			strcat (buffer, "       BRI");
		} 
		printf ("%s\n", buffer);
	}
}

int selected_protocol_ordinal;
byte selected_protocol_id = 0;
char selected_protocol_image[24];
dword selected_protocol_is_dmlt = 0;
int diva_load_print_options_summary = 0;
void print_options_summary (const dia_config_t* opt);
static char protocol_code_directory [2048];
char* selected_protocol_code_directory = protocol_code_directory;
int diva_load_detect_only   = 0;
int diva_load_get_card_type = 0;
int selected_bri_code_version = 0;

/*
	Fill Above variables,
	return -1 on error
	*/
int diva_load_get_protocol_by_name (const char* name) {
	int i;
	for (i = 0; dmlt_protocols [i].name; i++) {
		if (!strcasecmp (dmlt_protocols[i].name, name)) {
			selected_protocol_ordinal = i;
			selected_protocol_id = dmlt_protocols[i].id;
			selected_protocol_is_dmlt = dmlt_protocols[i].multi;
			strcpy (selected_protocol_image, dmlt_protocols[i].image);
			return (0);
		}
	}

	return (-1);
}



int DivaALoad(char *dsp_name, dia_config_t *options, dia_card_t *card, char *msg , int adapter_instance);
static char *card_desc[] = {"DIVA Server PRI",
        			"DIVA Server BRI",
				"DIVA Server 4-BRI"};

int main( int argc, char *argv[] )
{
    int rc=0;
    char ret_msg[80];
    char disp_msg[80];
    int i,opt;
    byte stablel2;
    word tei=0xff, channel_no=0, adapter_no=0;
    char c;
    int dev_id = 0;
    byte card_i;
    char afname[10] = "";
	byte byId;
	byte *pParamStr, *pOAD, *pOSA, *pSPID;
	dia_card_list_t *card_list;

	strcpy (protocol_code_directory, DATADIR"/");

	if (argc < 2) {
		usage(argv[0]);
		return (1);
	}

	if ((argv[1][0] == '-') && (argv[1][1] == 'y')) {
		diva_load_detect_only = 1;
		diva_load_get_card_type = atoi (&argv[1][2]);
	}

	argc=0;

    opt=1;
    bzero(&options, sizeof(dia_config_t));
    bzero(&card, sizeof(dia_card_t));

		options.stable_l2 = 2;

		if (!diva_load_detect_only) {
    	printf("EICON DIVALOAD: Firmware loader for Eicon DIVA Server ISDN adapters\n");
		}
    if ((dev_id = open(DIVAS_DEVICE_DFS, O_RDONLY, 0)) == -1)
    if ((dev_id = open(DIVAS_DEVICE, O_RDONLY, 0)) == -1)
    {
				if (!diva_load_detect_only)
        	fprintf(stderr, "Couldn't open %s\n", DIVAS_DEVICE);
        return 1;
    }


    if((rc = ioctl(dev_id, DIA_IOCTL_DETECT, &cards)) == -1)
    {
				if (!diva_load_detect_only)
        	fprintf(stderr, "%s DETECT Error %d\n", DIVAS_DEVICE, rc);
        (void)close(dev_id);
        return 1;
    }

    card_list = calloc(cards[0], sizeof(dia_card_list_t));


    if((rc = ioctl(dev_id, DIA_IOCTL_GET_LIST, card_list)) == -1)
    {
				if (!diva_load_detect_only)
        	fprintf(stderr, "%s COUNT Error %d\n", DIVAS_DEVICE, rc);
        (void)close(dev_id);
        return 1;
    }

    close(dev_id);

    num_cards = cards[0];

	{
		int i;

		for (i=1; i<=num_cards;) {
	    new_cards[new_num_cards+1] = cards[i];
	    real_card_id[new_num_cards+1] = i-1;

 		 if (diva_load_detect_only &&
				 (diva_load_get_card_type == (new_num_cards+1))) {
			 printf ("%d\n", cards[i]);
       exit (0);
		 }

			if (cards[i] == 2) { /* 4BRI */
				i+=3;
			}
			new_num_cards++;
	    i++;
		}
	}

	num_cards = new_num_cards;
 if (diva_load_detect_only) {
		printf ("255\n");
		exit (1);
 }

    if (num_cards)
	{
		printf("divaload: %d %s found\n", num_cards,
				num_cards > 1 ? "adapters" : "adapter");
	}
	else
	{
		printf("divaload: no adapters found\n");
	}

    while( argv[opt] )
    { 
        if( argv[opt][0]=='-')
        {
            c=argv[opt][1];
            opt++;
            if(c>='A' &&  c<='Z')
            { 
               c+='a'-'A';
            }

            switch(c)
            {
			case '1':
			case '2':
			case '3': // For 4BRI
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
				byId = c - '1';
				opt--; /* Go back to -n??? argument to see if it was oad, osa or spid */

				pParamStr = argv[opt] + 2;	/* Set up pointer to paramater string - oad, osa, spid */

				if (strcmp(pParamStr, "oad") == 0)
				{
					pOAD = argv[opt+1];
					if (check_oa(pOAD))
					{
							strcpy((char *)OAD[byId], pOAD);
					}
					else
					{
						fprintf(stderr, "Invalid OAD - %s\n", pOAD);
                                                return (1);
					}
				}
				else if (strcmp(pParamStr, "osa") == 0)
				{
					pOSA = argv[opt+1];
					if (check_osa(pOSA))
					{
						strcpy((char *)OSA[byId], pOSA);
					}
					else
					{
						fprintf(stderr, "Invalid OSA - %s\n", pOSA);
                                                return (1);
					}
				}
				else if (strcmp(pParamStr, "spid") == 0)
				{
					pSPID = argv[opt+1];
					if (check_spid(pSPID))
					{
						strcpy((char *)SPID[byId], pSPID);
					}
					else
					{
						fprintf(stderr, "Invalid SPID - %s\n", pSPID);
                                                return (1);
					}
				}
				else
				{
					fprintf(stderr, "Invalid sub-parameter: [%s]\n", pParamStr);
					return 1;
				}
				
				opt++; opt++;
				break;

			case 'e': {
				word l1_opt = 0;
				if (read_dec(argv[opt],(word*)&l1_opt)) {
					printf("Invalid params for L1 option.\n");
					return (1);
				} else {
					if ((!l1_opt) || (l1_opt > 3)) {
						printf("Invalid L1 options\n");
						return (1);
					}
					options.crc4 = l1_opt;
					opt++;
				}
			}	break;

			case 'w': {
				if (argv[opt] && (argv[opt][0] != '-')) {
					strcpy (selected_protocol_code_directory, argv[opt]);
					opt++;
				} else {
					fprintf (stderr, "Invalid protocol code directory selected\n");
					return (1);
				}
			} break;

			case 'a': // all?
				opt--;
				if ((argv[opt][2] == 'l') && (argv[opt][3] == 'l') && (argv[opt][4] == '\0') && (options.card_id == 0))
				{
					load_all_cards = TRUE;
				}
				else if (options.card_id != 0)
				{
					fprintf(stderr, "divaload: cannot specify both -all and -c\n");
				}
				else
				{
					fprintf(stderr, "divaload: invalid parameter [%s] (Use 'all')\n", argv[opt]);
					return 1;
				}
				opt++;
				break;

			case 'c': // Card number (starting from 1)
				if(read_dec(argv[opt],(word *) &adapter_no))
				{
            		fprintf(stderr, "divaload: invalid card number specified\n");
					return (1);
				}
				else if (load_all_cards)
				{
	 	    		fprintf(stderr, "divaload: -all specified. Cannot specify both -all and -c");
		    		return 1;
				}
				else if ((adapter_no < 1) || (adapter_no > num_cards))
           		{
            		if (num_cards > 1)
					{
						fprintf(stderr, "divaload: use value between 1 and %d inclusive\n", num_cards);
						return 1;
					}
					else
					{
						fprintf(stderr, "divaload: adapter numbers start at 1\n");
						return 1;
					}
				}
				opt++;
				break;

			case 'f': /* Switch configuration */
				if (!argv[opt])
				{
					fprintf(stderr, "divaload: No switch type specified\n");
					return (1);
				}

				if (diva_load_get_protocol_by_name (&argv[opt][0])) {
					printf("divaload: Invalid switch type (%s)\n", argv[opt]);
					return (1);
				}
				strcpy(afname, selected_protocol_image);
				if (selected_protocol_id) {
					options.prot_version = (0x80 | selected_protocol_id);
				}
				opt++;
				break;

			case 'h':
				usage(argv[0]);
				return (1);
				break;

			case 'z':
				options.x_interface |= (byte)(0x01 << 2);
				break;

			case 'q': {
				word qsig_opt = 0;
				if (read_dec(argv[opt],(word*)&qsig_opt)) {
					printf("Invalid params for QSIG option.\n");
					return (1);
				} else {
					if (qsig_opt > 3) {
						printf("Invalid QSIG options\n");
						return (1);
					}
					options.x_interface |= (byte)(qsig_opt & 0x00ff);
					opt++;
				}
			}	break;

			case 'l':
				if (read_dec(argv[opt],(word*)&channel_no)) {
					printf("Invalid params for channel option.\n");
					return (1);
				} else {
					if (channel_no > 30)
					{
						printf("Invalid channel number (%d)!!\n", channel_no);
						return (1);
					}
					options.low_channel = channel_no;
					opt++;
				}
				break;

			case 'v': {
				word code_version = 0;
				if (!argv[opt] || *argv[opt]=='-') {
					printf("Invalid option for BRI code version.\n");
					return (1);
				}
				if (read_dec(argv[opt],(word*)&code_version)) {
					printf("Invalid option for BRI code version.\n");
					return (1);
				} else if (code_version != 6) {
					printf("Invalid option for BRI code version.\n");
					return (1);
				} else {
					selected_bri_code_version = 6;
					opt++;
				}
			} break;

			case 'n': {
				word didn = 0;
				options.nt2=TRUE;
				if (!argv[opt] || *argv[opt]=='-') {
						break;
				}
				if (read_dec(argv[opt],(word*)&didn)) {
					printf("Invalid options for NT2 operation.\n");
					return (1);
				} else {
					if(didn>20) {
						printf("Invalid options for NT2 operation.\n");
						return 1;
					}
					options.sig_flags = didn;
					opt++;
				}
			} break;

			case 'o':
				options.no_order_check=TRUE;
				break;

			case 'p': {
				word pmode = 1;
				if (!argv[opt] || *argv[opt]=='-') {
					pmode = 1;
				} else {
					if (read_dec(argv[opt],(word*)&pmode)) {
						printf("Invalid options for permanent mode operation.\n");
						return (1);
					} else {
						if(!pmode || pmode>2) {
							printf("Invalid options for permanent mode operation.\n");
							return 1;
						}
						opt++;
					}
				}
				options.permanent=pmode;
				options.stable_l2 = (2 | (options.stable_l2 & 0x08));
				options.nt2=TRUE;
			} break;

			case 'd':
				diva_load_print_options_summary = 1;
				break;
			
			case 'u':
				options.stable_l2 = (2 | (options.stable_l2 & 0x08));
				options.nt2=TRUE;
				options.tei = 0x01;
				break;

			case 'x':
				options.stable_l2 = 0x02 /* L2 Permanent */ | 0x08 /* NT Mode */;
				break;

			case 's':
				if (1 /* || (!(options.stable_l2 & 0x08)) && (!options.permanent) */) {
					if (!argv[opt] || *argv[opt]=='-') {
						break;
					}

					if (read_dec(argv[opt],(word*)&stablel2)) {
						printf("Invalid options for Layer2 operation.\n");
						return (1);
					} else {
						if(stablel2>3) {
							printf("Invalid options for Layer2 operation.\n");
							return 1;
						}
						options.stable_l2 = (stablel2 | (options.stable_l2 & 0x08));
						opt++;
					}
				} else {
					printf("Invalid options for Layer2 operation.\n");
					return 1;
				}
				break;

			case 't':
				if (read_dec(argv[opt],(word*)&tei)) {
					printf("Invalid params for TEI option.\n");
					return (1);
				} else {
					if (tei>63) { 
						printf("Invalid tei number !!\n");
						return (1);
					}
					options.tei = (tei << 1) | 1;
					opt++;
				}
				break;

				/* Invalid parameter (on Linux anyway) */
			default:
				fprintf(stderr, "divaload: invalid parameter [%c]\n", c);
				return -1;
			}
		}
	}

	if (strlen(afname) == 0) {
		fprintf(stderr,
					"divaload: No switch type specified ('./divaload -h' for help)\n");
		return -1;
	}


	if (load_all_cards) {
		int num_adaps, qm;

		for (card_i = 1 ; card_i <= num_cards; card_i++) {

			fprintf (stderr,
			"divaload: Loading Adapter %d (%s) with %s firmware\n",
			 card_i,
			 card_desc[new_cards[card_i]],
			 dmlt_protocols [selected_protocol_ordinal].name);
			fprintf (stderr, "          %s\n",
			 dmlt_protocols [selected_protocol_ordinal].description);
				
			options.card_id = card_i - 1;

			if (card_list[options.card_id].state == DIA_RUNNING) {
				fprintf(stderr, "divaload: adapter %d already running.\n", card_i);
				continue;
			}

			if (new_cards[card_i] == 0) {
				strcat(afname, ".pm");
				num_adaps = 1;
			} else if (new_cards[card_i] == 1) {
				strcat(afname, ".sm");
				num_adaps = 1;
			} else if (new_cards[card_i] == 2) {
				strcat(afname, ".qm ");
				num_adaps = 4;
			} else {
				printf("divaload: unknown card type %d\n", new_cards[card_i]);
				return -1;
			}
			options.card_id = real_card_id[card_i];

			for (i=options.card_id, qm = 0; num_adaps; i++, num_adaps--, qm++) {
				options.card_id = i;

				if (new_cards[card_i] == 2) { /* 4BRI */
					 afname[strlen(afname) - 1] = '0' + qm;
				}
				if (diva_load_print_options_summary) {
					print_options_summary (&options);
				}
				rc=DivaALoad(afname, &options, &card, ret_msg, num_adaps);
				if( rc != ERR_ETDD_OK ) {
					switch(rc) {
					case ERR_ETDD_ACCESS:
						 sprintf(disp_msg,"cannot access file %s",ret_msg);
						 perror(disp_msg);
						 break;
 
					case ERR_ETDD_DSP:
						 sprintf(disp_msg,"cannot access DSP binary %s",ret_msg);
						 perror(disp_msg);
						 break;

					case ERR_ETDD_IOCTL:
						 sprintf(disp_msg,"error doing ioctl");
						 perror(disp_msg);
						 break;

					case ERR_ETDD_NOMEM:
						 printf("DivaLoad:Not enough memory available.\n");
						 break;

					case ERR_ETDD_OPEN:
						 sprintf(disp_msg,"Cannot open file %s",ret_msg);
						 perror(disp_msg);
						 break;

					case ERR_ETDD_READ:
						 sprintf(disp_msg,"Cannot read from file %s",ret_msg);
						 perror(disp_msg);
						 break;

					default:
						 printf("divaload: unknown error\n");
					}/*switch*/
				}/*if*/

			}/*Next Sub-Adapter*/

			if (strchr(afname, '.')) {
				*(strchr(afname, '.')) = '\0';
			}
		}/*Next real adapter*/
	} else {
		int num_adaps = 0, qm;
		if (adapter_no == 0) { // No adapter specified
			fprintf(stderr,
				"divaload: You need to specify either an adpater (-c n) or use -all\n");
			return -1;
		}

		
		fprintf (stderr,
		"divaload: Loading Adapter %d (%s) with %s firmware\n",
		 adapter_no,
		 card_desc[new_cards[adapter_no]],
		 dmlt_protocols [selected_protocol_ordinal].name);
		fprintf (stderr, "          %s\n",
		 dmlt_protocols [selected_protocol_ordinal].description);

		options.card_id = real_card_id[adapter_no];
		if (card_list[options.card_id].state == DIA_RUNNING) {
			fprintf(stderr, "divaload: adapter %d already running.\n", adapter_no);
			return -1;
		}
		if (new_cards[adapter_no] == 0) {
			strcat(afname, ".pm");
			num_adaps = 1;
		} else if (new_cards[adapter_no] == 1) {
			strcat(afname, ".sm");
			num_adaps = 1;
		} else if (new_cards[adapter_no] == 2) {
			strcat(afname, ".qm ");
			num_adaps = 4;
		} else {
			printf("divaload: unknown adapter type %d", new_cards[adapter_no]);
			return -1;
		}
		/* Load num_adaps adapters (4BRI: num_adaps = 4) */
		for (i=options.card_id, qm = 0; num_adaps; i++, num_adaps--, qm++) {
		options.card_id = i;

		// num_adaps serves as a trigger for download of the combifile
		// Only gets done (in DivaALoad) when num_adaps == 1 (i.e. last one)

		if (new_cards[adapter_no] == 2) { // 4BRI
			afname[strlen(afname) - 1] = '0' + qm;
		}

		strcpy((char *)options.terminal[0].oad, OAD[qm*2]);
		strcpy((char *)options.terminal[1].oad, OAD[(qm*2)+1]);

		strcpy((char *)options.terminal[0].osa, OSA[qm*2]);
		strcpy((char *)options.terminal[1].osa, OSA[(qm*2)+1]);

		strcpy((char *)options.terminal[0].spid, SPID[qm*2]);
		strcpy((char *)options.terminal[1].spid, SPID[(qm*2)+1]);

		if (diva_load_print_options_summary) {
			print_options_summary (&options);
		}
    rc=DivaALoad(afname, &options, &card, ret_msg, num_adaps);
	
    if( rc != ERR_ETDD_OK ) {
			switch(rc) {
            case ERR_ETDD_ACCESS:
                 sprintf(disp_msg,"cannot access file %s",ret_msg);
                 perror(disp_msg);
                 break;
 
            case ERR_ETDD_DSP:
                 sprintf(disp_msg,"cannot access DSP binary %s",ret_msg);
                 perror(disp_msg);
                 break;

            case ERR_ETDD_IOCTL:
                 sprintf(disp_msg,"error doing ioctl");
                 perror(disp_msg);
                 break;

            case ERR_ETDD_NOMEM:
                 printf("divaload: not enough memory available.\n");
                 break;

            case ERR_ETDD_OPEN:
                 sprintf(disp_msg,"cannot open file %s",ret_msg);
                 perror(disp_msg);
                 break;

            case ERR_ETDD_READ:
                 sprintf(disp_msg,"cannot read from file %s",ret_msg);
                 perror(disp_msg);
                 break;

            default:
                 printf("divaload: unknown error\n");
            }
        }
			} // 'for' loop for loading 4BRI adapters
    }
    return(0);
}

/*
 * take in a decimal number from the keyboard
 */
byte read_dec(char *str,word *val)
{
    word c;
    *val=0;
    
    if(!str)
    {
        return TRUE;
    }

    c=*str; 
    while( *str && (c>='0' && c<='9'))
    {
        *val *= 10;
        *val += c-'0';
        str++;
        c=*str;
    }

    if( *str )
    {
        *val=0;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*
 * take in a hex number from the keyboard
 */
byte read_hex(char *str,dword *val)
{
    word c;
    *val=0;
    
    if(!str)
    {
        return TRUE;
    }
	if(!strncmp(str, "0x", 2))
	{
		str++; 
		str++; 
	}
	c=*str; 
    while( *str && ((c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F')))
    {
        *val *= 0x10;
		if (c>='0' && c<='9') 
		{
			*val += c-'0';
		}
		else
		{
	 		if (c>='a' && c<='f')
			{
				*val += c-'a';
				*val += 0xa;
			}
			else
			{
				*val += c-'A';
				*val += 0xa;
			}
		}
        str++;
        c=*str;
    }

    if( *str )
    {
        *val=0;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
 * validate the Originating Address string 
 */
byte check_oa(char *str)
{
    byte len=0;

    if(!*str)
    {
       return FALSE;
    }

    while(*str && len<MAX_ADDR && (*str>='0' && *str<='9'))
    {
        len++;
        str++;
    }
    if( *str )
    {
        return FALSE;
    }
    else
    {

        return TRUE;
    }
} 

/*
 * validate the Originating Sub-Address string 
 */
byte check_osa(char *str)
{
    byte len=0;

    if(!*str)
    {
       return FALSE;
    }

    while(*str && len<MAX_ADDR && ((*str>='0' && *str<='9')
          || *str=='P' || *str=='p' ))
    {
       len++;
       str++;
    }
    if( *str )
    {
        return FALSE;
    }
    else
    {

        return TRUE;
    }
} 

/*
 * validate the SPID string 
 */
byte check_spid(char *str)
{
    byte len=0;

    if(!*str)
    {
       return FALSE;
    }

    while(*str && len<MAX_SPID && (*str>='0' && *str<='9'))
    {
       len++;
       str++;
    }
    if( *str )
    {
        return FALSE;
    }
    else
    {

        return TRUE;
    }
} 
       
void usage_controller (void) {
	printf (" controller, mandatory:\n");
	printf ("  -c n:    select controller number n\n");
	printf ("  -all:    select all available controllers\n");
	printf ("\n");
}

void usage_protocol (void) {
	printf (" protocol, mandatory:\n");
	diva_load_print_supported_protocols ("  ");
	printf ("\n");
}


typedef struct {
	const char*			key;
	const char*     text[12];
} diva_protocol_options_t;

diva_protocol_options_t protocol_options [] = {
{"-e [1,2,3]", {"set Layer 1 framing on PRI Adapter",
                "1 - doubleframing (NO CRC4)",
					      "2 - multiframing (CRC4)",
                "3 - autodetection",
                0}
},
{"-x",          {"set PRI Adapter in NT mode",
                 "default mode ist TE",
                 0},
},
{"-z",          {"set PRI Adapter in High Impedance state",
                 "until first user application does request",
                 "interface activation",
                 0},
},
{"-l [1...30]", {"set starting channel number on PRI Adapter",
                 "By default the allocation of channels is made",
                 "on a high-to-low basis.",
                 "By specifying -l you select a low-to-high",
                 "allocation policy (in addition)",
                 0}
},
{"-q [0..3]",		 {"select QSIG options",
                  "0 - CR and CHI 2 Bytes long (default)",
                  "1 - CR 1 byte, CHI 2 bytes",
                  "2 - CR is 2 bytes, CHI is 1 byte",
                  "3 - CR and CHI 1 byte",
                  "(CR - Call Reference, CHI - B-Channel Ident.)",
                  0}
},
{"-n [0...20]",   {"select NT2 mode and default length of DIDN",
                   "(DIDN - Direct Inward Dial Number)",
                  0}
},
{"-o",           {"turn off order checking of information elements",
									0}
},
{"-p [1,2]",      {"establish a permanent connection",
                  "(e.g. leased line configuration)",
                  "1 - TE <-> TE mode, structured line",
                  "2 - NT <-> TE mode, raw line",
									0}
},
{"-s [0,1,2,3]",  {"D-Channel Layer 2 activation policy on BRI Adapter",
                  "0 - On demand",
                  "1 - Deactivation only by NT side, preferred, default",
                  "2 - Always active",
                  "3 - Always active, mode 2",
                   0},
},
{"-t [0...63]",		{"specifies a fixed TEI value",
                   "(Default is a automatic TEI assignment)",
                   0},
},
{"-u",          {"select point to point mode on BRI Adapter",
								 "uses default TEI '0'",
                 "NT2 mode is on",
                 0}
},
{"-[1...8]oad|osa|spid",
                  {"BRI Adapter B-Channel options",
                   "Specify the Originating Address (OAD)",
                   "Originating Sub-address (OSA) and/or",
                   "Service Profile Identifier (SPID)",
                   "for each B-channel (1 or 2 by BRI, 1...8 by 4BRI)",
                   "Example: -1oad 123456 -1spid 1234560001",
                   0}
},
{"-d",          {"display protocol options summary", 0}},
{"-w",          {"select alternative protocol code directory",
                 "default directory: \""DATADIR"\"",
                 0}
},
#if 0
{"-v 6",        {"protocol code version for BRI Adapter",
                 "Without this option V4 version of protocol code,"
                 "(with V.90 modem support) is used",
                 "With this option V6 version of protocol code,"
                 "(with QSIG and additional SS support) is used",
                 0}
},
#endif
{"-h or -?",    {"help", 0}},
{ 0, {0}}
};

void usage_option (const char* tab, const diva_protocol_options_t* opt);
void usage_options (const char* tab) {
	const diva_protocol_options_t* opt = &protocol_options [0];

	printf (" options, optional:\n");

	while (opt->key) {
		usage_option (tab, opt++);
	}
	printf ("\n");
}

void usage_option (const char* tab, const diva_protocol_options_t* opt) {
	char buffer[128];
	int s, j, i;

	strcpy (buffer, tab);
	strcat (buffer, opt->key);
	s = strlen (buffer);
	while (s++ < 22) {
		strcat (buffer, " ");
	}
	j = strlen (buffer) + 2;
	strcat (buffer, ": ");
	strcat (buffer, opt->text[0]);
	printf ("%s\n", buffer);

	for (i = 1; (opt->text[i] && (i < 12)); i++) {
		buffer[0] = 0;
		s = 0;
		while (s++ < j) {
			strcat (buffer, " ");
		}
		strcat (buffer, opt->text[i]);
		printf ("%s\n", buffer);
	}
}
      
/* -----------------------------------------------------------------------
			Usage function 
   ----------------------------------------------------------------------- */
void usage(char *name) {
	char* p = strstr(name, "/");
	while (p) {
		name = p+1;
		p = strstr (name, "/");
	}

	printf("Usage: %s controller protocol [options]\n\n", name);
	usage_controller ();
	waitkey ();
	usage_protocol();
	waitkey ();
	usage_options ("  ");
}

void print_options_summary (const dia_config_t* opt) {
	int i;
	printf ("Protocol options summary (RAW):\n");
	printf ("Adapter         : %d\n",		opt->card_id);
	printf (" TEI            : %02x\n", opt->tei);
	printf (" NT2            : %02x\n", opt->nt2);
	printf (" WATCHDOG       : %02x\n", opt->watchdog);
	printf (" PERMANENT      : %02x\n", opt->permanent);
	printf (" L1Z/QSIG       : %02x\n", opt->x_interface);
	printf (" NT/L2 Options  : %02x\n", opt->stable_l2);
	printf (" No Order Check : %02x\n", opt->no_order_check);
	printf (" Handset Type   : %02x\n", opt->handset_type);
	printf (" DIDN           : %02d\n", opt->sig_flags);
	printf (" Low Channel    : %02x\n", opt->low_channel);
	printf (" ProtVersion    : %02x\n", opt->prot_version);
	printf (" CRC4           : %02x\n", opt->crc4);
	for (i = 0; i < 2; i++) {
	printf (" Terminal       : %d\n", i+1);
	printf ("  OAD           : \"%s\"\n", opt->terminal[i].oad);
	printf ("  OSA           : \"%s\"\n", opt->terminal[i].osa);
	printf ("  SPID          : \"%s\"\n", opt->terminal[i].spid);
	}
}

void waitkey (void) {
/*	getchar (); */
}



