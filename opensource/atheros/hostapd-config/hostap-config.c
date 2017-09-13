#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CLI_AUTH_WPA = 1,
    CLI_AUTH_WPAPSK,
    CLI_AUTH_WPA2,
    CLI_AUTH_WPA2PSk,
    CLI_AUTH_WPA_AUTO,
    CLI_AUTH_WPA_AUTO_PSK,
    CLI_AUTH_8021X,
    CLI_MAX_AUTH,
};

enum {
    CLI_CIPHER_AUTO = 1,
    CLI_CIPHER_WEP,
    CLI_CIPHER_TKIP,
    CLI_CIPHER_AES,
    CLI_MAX_CIPHER
}; 

#define HOSTAPD_CFG_FILE  "hostapd.conf"
#define DEBUG "0"

static void cli_fputs(char *p, FILE *fp)
{
    if (fputs(p, fp) == EOF) {
	printf("Can't write %s\n", HOSTAPD_CFG_FILE);	
        fclose(fp);
    }
}


int main()
{

char SSID[32],INTERFACE[32],passphrase[64],own_ip_addr[20];
char auth_server_addr[20],auth_server_shared_secret[20];
int authType,CLI_BSS_F_HASHED_KEY,auth_server_port;
int rsn_preauth,CIPER,wpa_group_rekey;

FILE *fp;
int wpa, psk, radius, key_length;
char buf[128];
int run;

    printf("Do you want to configure Hostapd?\n<2>Use default,<1>Yes,<0>No\n");
    scanf("%d",&run);

    if(run==0)
	return 0;

    fp = fopen(HOSTAPD_CFG_FILE, "w+");
    if (fp == NULL) {
        printf("Can't create %s\n", HOSTAPD_CFG_FILE);
        return 1;
    }

    /* interface */
    if(run==2)
	strcpy(INTERFACE,"ath0");
    else
	{
	printf("Enter interface:\n");
    	scanf("%s",INTERFACE);
	}
    sprintf(buf, "interface=%s\n", INTERFACE);
    cli_fputs(buf, fp);
    sprintf(buf, "bridge=%s\n", "br0");
    cli_fputs(buf, fp);
    sprintf(buf, "driver=%s\n", "madwifi");
    cli_fputs(buf, fp);
    sprintf(buf, "logger_syslog=%s\n", "0");
    cli_fputs(buf, fp);
    sprintf(buf, "logger_syslog_level=%s\n", "0");
    cli_fputs(buf, fp);	
    sprintf(buf, "logger_stdout=%s\n", "0");
    cli_fputs(buf, fp);
    sprintf(buf, "logger_stdout_level=%s\n", "0");
    cli_fputs(buf, fp);
    sprintf(buf, "debug=%s\n", DEBUG);
    cli_fputs(buf, fp);
    /* ssid */
    if(run==2)
	strcpy(SSID,"AtherosTest");
    else
	{
    	printf("Enter ssid:\n");
    	scanf("%s",SSID);
	}
    sprintf(buf, "ssid=%s\n", SSID);
    cli_fputs(buf, fp);

    /* WPA version */ 
    if(run==2)
	authType=6;
    else
	{
    		printf("Choice athentication mathod:\n");
    		printf("<1>WPA\n");
    		printf("<2>WPA-PSK\n");
    		printf("<3>WPA2\n");
    		printf("<4>WPA2-PSK\n");
    		printf("<5>WPA/WPA2 Auto selected\n");
    		printf("<6>WPA/WPA2 PSK Auto selected)\n");
    		printf("<7>IEEE802.1x(WEP)\n");
    		scanf("%d",&authType);
	}
    wpa = 0; radius = 0; psk = 0;   
    switch (authType) {
        case CLI_AUTH_8021X: radius = 1; break;
        case CLI_AUTH_WPA: wpa = 1, radius = 1; break;              
        case CLI_AUTH_WPAPSK: wpa = 1; psk = 1; break;               
        case CLI_AUTH_WPA2: wpa = 2; radius = 1; break;
        case CLI_AUTH_WPA2PSk: wpa = 2, psk = 1; break;
        case CLI_AUTH_WPA_AUTO: wpa = 3, radius = 1; break;
        case CLI_AUTH_WPA_AUTO_PSK: wpa = 3, psk = 1; break;
        default:
            printf("Unknown auth type\n");
    }
    /* Write out WPA version */
    if (wpa) {
        sprintf(buf, "wpa=%d\n", wpa);
        cli_fputs(buf, fp);
    }
    /* write out WPAPSK info if necessary */
    if (psk) {
	if(run==2)
		CLI_BSS_F_HASHED_KEY=1;
	else
	{
		printf("Enable passphrase?\n <1>Enable <2>Disable\n");
		scanf("%d",&CLI_BSS_F_HASHED_KEY);
	}
        if (CLI_BSS_F_HASHED_KEY == 1)
	{
	      	if(run==2)
			strcpy(passphrase,"AtherosTest1234");
		else
		{
			printf("Enter passphrase key (8~63 ASCII)\n");
	      		scanf("%s",&passphrase);
		}
              	sprintf(buf, "wpa_passphrase=%s\n", passphrase);
	}
        else
	{
              	printf("Enter PSK key (64 HAX)\n");
              	scanf("%s",&passphrase);
		sprintf(buf, "wpa_psk=%s\n", passphrase);
	}
        cli_fputs(buf, fp);
        cli_fputs("wpa_key_mgmt=WPA-PSK\n", fp);
    }
    /* write out RADIUS info if necessary */
    if (radius) {
        char dst[32];
		printf("Enter Own IP for Radius server\n");
		scanf("%s",&own_ip_addr);
            	sprintf(buf, "own_ip_addr=%s\n", own_ip_addr);
            	cli_fputs(buf, fp);
                printf("Enter Radius server address\n");
                scanf("%s",&auth_server_addr);
            	sprintf(buf, "auth_server_addr=%s\n", auth_server_addr);
            	cli_fputs(buf, fp);
                printf("Enter Radius server port\n");
                scanf("%d",&auth_server_port);
            	sprintf(buf, "auth_server_port=%d\n", auth_server_port);
            	cli_fputs(buf, fp);
                printf("Enter Shared secret key\n");
                scanf("%s",&auth_server_shared_secret);
            	sprintf(buf, "auth_server_shared_secret=%s\n", auth_server_shared_secret);
            	cli_fputs(buf, fp);
        cli_fputs("ieee8021x=1\n", fp);

    	if (wpa)
    	{
        	/* WPA/WPA2 */
        	cli_fputs("wpa_key_mgmt=WPA-EAP\n", fp);

    		if(wpa==2 || wpa==3)
    		{
        		if(run==2)
        			rsn_preauth=0;
        		else
        		{
                        	printf("RSN Preauthentication <1>Enable <0>Disable\n");
                        	scanf("%d",&rsn_preauth);
        		}

            	sprintf(buf, "rsn_preauth=%d\n", rsn_preauth);
             	cli_fputs(buf, fp);
                sprintf(buf, "rsn_preauth_interfaces=%s\n", "br0");
                cli_fputs(buf, fp);
    		}
        }
        else
        {
    		printf("802.1x key length <5/13> :\n");
    		scanf("%d",&key_length);

            /* 802.1x. Always use 104 as the key length */
            sprintf(buf, "wep_key_len_unicast=%d\n", key_length);
            cli_fputs(buf, fp);
            sprintf(buf, "wep_key_len_broadcast=%d\n", key_length);
            cli_fputs(buf, fp);
        }
    }

    if (wpa) {
	if(run==2)
		CIPER=1;
	else
	{
		printf("Choice Cipher type:\n");
    		printf("<1>Auto\n");
		printf("<2>wep\n");
		printf("<3>tkip\n");
		printf("<4>ccmp\n");
		scanf("%d",&CIPER);
	}
		switch(CIPER) {
			case CLI_CIPHER_AUTO:
				sprintf(buf, "wpa_pairwise=%s\n", "CCMP TKIP" );
				break;	
			case CLI_CIPHER_WEP:
				sprintf(buf, "wpa_pairwise=%s\n", "WEP" );
				break;	
			case CLI_CIPHER_TKIP:
				sprintf(buf, "wpa_pairwise=%s\n", "TKIP" );
				break;	
			case CLI_CIPHER_AES:
				sprintf(buf, "wpa_pairwise=%s\n", "CCMP" );
				break;
			default:
				sprintf(buf, "wpa_pairwise=%s\n", "CCMP TKIP" );
			}
		cli_fputs(buf, fp);
              /* write out group key update interval */
        	if(run==2)
			wpa_group_rekey=600;
		else
		{	
			printf("Enter WPA group rekey interval:\n");
        		scanf("%d",&wpa_group_rekey);
		}
              	sprintf(buf, "wpa_group_rekey=%d\n", wpa_group_rekey);
              	cli_fputs(buf, fp);
    }
    fclose(fp);
    return 0;
}

