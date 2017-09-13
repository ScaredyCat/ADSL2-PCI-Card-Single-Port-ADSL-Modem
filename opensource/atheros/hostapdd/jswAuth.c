/*
 *  Copyright (c) 2004 Atheros Communications Inc.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "eloop.h"
#include "hostapd.h"
#include "ieee802_1x.h"
#include "ieee802_11.h"
#include "accounting.h"
#include "eapol_sm.h"
#include "iapp.h"
#include "ap.h"
#include "sta_info.h"
#include "driver.h"
#include "radius_client.h"
#include "radius_server.h"
#include "wpa.h"
#include "ctrl_iface.h"
#include "tls.h"
#include "eap_sim_db.h"
#include "version.h"

#include "openssl/bn.h"
#include "openssl/dh.h"
#include "openssl/sha.h"
#include "openssl/rand.h"
#include "eap_js.h"
#include "jswproto.h"
#include "jswAuth.h"

STATIC struct jsw_session *jswAlloc(struct hostapd_data *hapd,
				    struct sta_info *sta);
STATIC int jswDhInit(struct jsw_session *js);
STATIC void jswLedStart();

STATIC void jswTimeout(void *eloop_ctx, void *timeout_ctx)
{
	struct sta_info *sta = timeout_ctx;
	struct jsw_session *js = sta->js_session;
	
	smSendEvent(js, JSW_EVENT_TIMEOUT);

	/* XXX Divy. Take care of global JS P1 timer */
	return;
}

void jswTimerStop(struct jsw_session *js)
{
	struct hostapd_data *hapd;
	struct sta_info *sta;
	
	hapd = js->hapd;
	sta = js->sta;
	eloop_cancel_timeout(jswTimeout, js->hapd, sta); 
}

void jswTimerStart(struct jsw_session *js, int timer)
{
	struct hostapd_data *hapd;
	struct sta_info *sta;
	
	hapd = js->hapd;
	sta = js->sta;	 
	eloop_register_timeout(timer, 0, jswTimeout, hapd, sta);
}

STATIC struct jsw_session * 
jswAlloc(struct hostapd_data *hapd, struct sta_info *sta)
{
	struct jsw_session *js;

	js = malloc(sizeof(struct jsw_session));
	if (!js) {
		wpa_printf(MSG_DEBUG, "Cant allocate a jsw session\n");
		return NULL;
	}
	memset(js, 0, sizeof(*js));
	js->hapd = hapd;
	js->sta = sta;
	sta->js_session = js;
	
	HOSTAPD_DEBUG(HOSTAPD_DEBUG_MINIMAL, 
		      "JUMPSTART: " MACSTR " %s\n",
		      MAC2STR(sta->addr), __func__);
		       
	if (hostapd_get_rand(js->key_replay_counter, sizeof(u64)) < 0) {
		HOSTAPD_DEBUG(HOSTAPD_DEBUG_MINIMAL, 
			      "JUMPSTART: " MACSTR 
			      " %s: Cant get a random number for replay\n",
			      MAC2STR(sta->addr), __func__);
		goto error;
	}
	if (hostapd_get_rand((unsigned char *)js->nonce, JSW_NONCE_SIZE) < 0) {
		HOSTAPD_DEBUG(HOSTAPD_DEBUG_MINIMAL, 
			      "JUMPSTART: " MACSTR 
			      " %s: Cant get a random nonce\n",
			      MAC2STR(sta->addr), __func__);
		goto error;
	}
	if (jswDhInit(js) != 0) {
		goto error;
	}
	return js;

error:
	free(js);
	return NULL;
}

void js_free_station(struct sta_info *sta)
{
	struct jsw_session *js = sta->js_session;
	if (js == NULL) 
		return;

	eloop_cancel_timeout(jswTimeout, js->hapd, sta);
	DH_free(js->dh);
	free(js);
	sta->js_session = NULL;
}

void js_p1_new_station(struct hostapd_data *hapd, struct sta_info *sta)
{
	struct jsw_session *js;
	HOSTAPD_DEBUG(HOSTAPD_DEBUG_MINIMAL, 
		      "JUMPSTART: " MACSTR " %s\n",
		      MAC2STR(sta->addr), __func__); 
		      
	if ((js = jswAlloc(hapd, sta)) == NULL) {
		wpa_printf(MSG_DEBUG, "js_p1_new_station failed\n");
		return;
	}
	smSetState(js, jswP1Start);
}

void js_p2_new_station(struct hostapd_data *hapd, struct sta_info *sta)
{
	struct jsw_session *js;

	if ((js = jswAlloc(hapd, sta)) == NULL) {
		wpa_printf(MSG_DEBUG, "js_p1_new_station failed\n");
		return;
	}
	smSetState(js, jswP2Start);
}

STATIC int jswDhInit(struct jsw_session *js)
{	
	wpa_printf(MSG_DEBUG, 
		   "JUMPSTART: " MACSTR
		   " %s: Generating DH parameters ...\n",
		   MAC2STR(js->sta->addr), __func__);
	if ((js->dh = DH_generate_parameters(JSW_PRIME_BITS, DH_GENERATOR_2,
					     NULL, 
					     NULL)) == NULL) {
		wpa_printf(MSG_DEBUG,
			   "JUMPSTART: " MACSTR
			   " %s: DH parameters generation failed\n",
			   MAC2STR(js->sta->addr), __func__);
		goto error;
	}
	wpa_printf(MSG_DEBUG, 
		      "JUMPSTART: " MACSTR
		      " %s: Generating key ...\n",
		      MAC2STR(js->sta->addr), __func__);
	if (0 == DH_generate_key(js->dh)) {
		wpa_printf(MSG_DEBUG,
			   "JUMPSTART: " MACSTR
			   " %s: DH parameters generation failed\n",
			   MAC2STR(js->sta->addr), __func__);
		goto error;
	}
	
	wpa_printf(MSG_DEBUG,
		   "JUMPSTART: " MACSTR
		   " %s: DH keys generated\n",
		   MAC2STR(js->sta->addr), __func__);
	return 0;

error:
	wpa_printf(MSG_DEBUG,
		      "JUMPSTART: " MACSTR
		      " %s: DH Error!\n",
		      MAC2STR(js->sta->addr), __func__);
	if (js->dh) {
		DH_free(js->dh);
	}
	return -1;
}

/* XXX Divy. LED control TBD */
STATIC void jswLedStart()
{
    jswSetLEDPattern(&ready);
}

int jsw_init(struct hostapd_data *hapd)
{
	struct jsw_profile *jp;
	
	/* Allocate the global profile */	
	hapd->jsw_profile = malloc(sizeof(struct jsw_profile));
	if (hapd->jsw_profile == NULL)
		return -1;
	
	memset(hapd->jsw_profile, 0, sizeof(struct jsw_profile));
	jp = hapd->jsw_profile;
	jp->version = JSW_VERSION;
	
	if (hapd->conf->js_p1) {
		jswLedStart();
	}

	if (hapd->conf->js_p2) {
		memcpy(jp->passHash, hapd->conf->js_passHash, JSW_SHA1_LEN);
		memcpy(jp->salt, hapd->conf->js_salt, JSW_NONCE_SIZE);
		jswLedStart();
	}

	return 0;
}

void jsw_deinit(struct hostapd_data *hapd)
{	
	if (hapd->jsw_profile) {
		free(hapd->jsw_profile);
		hapd->jsw_profile = NULL;
	}
}

void
jswP2GetMicKey(struct jsw_session *js, u8 *micKey)
{
	struct jsw_profile *jp;
	int len;
	u8 buf[JSW_SHA1_LEN + JSW_NONCE_SIZE];
	struct hostapd_data *hapd;
	
	hapd = js->hapd;
	jp = hapd->jsw_profile;
	len = JSW_SHA1_LEN;
	memcpy(buf, jp->passHash, len);
	memcpy(&buf[len], js->nonce, JSW_NONCE_SIZE);
	len += JSW_NONCE_SIZE;

	SHA1(buf, len, micKey);
}

void
jswP2GetMic2Key(struct jsw_session *js, u8 *micKey)
{
	int len = 0;
	u8 buf[2*JSW_MICKEY_SIZE];
	u8 mic1Key[JSW_SHA1_LEN];

	jswP2GetMicKey(js, mic1Key);
	memcpy( buf, mic1Key, JSW_MICKEY_SIZE);
	len += JSW_MICKEY_SIZE;

	memcpy(&buf[len], js->secret.micKey, JSW_MICKEY_SIZE);
	len += JSW_MICKEY_SIZE;

	SHA1(buf, len, micKey);
}

void
jswSessionDestroy()
{
}

void
js_create_ssid(struct hostapd_data *hapd, const char *pref)
{
	u8  shaBuf[JSW_SHA1_LEN];
	u8 *p;
	int i;
	
	memset(hapd->conf->ssid, 0, sizeof(hapd->conf->ssid));
	p = hapd->conf->ssid;
	strcpy(p, pref);
	p += strlen(pref);
	SHA1((u8 *)hapd->own_addr, sizeof(hapd->own_addr), shaBuf);
	for(i = 0; i < JSW_SSID_SUFF_LEN; i++) {
		sprintf(p, "%02x", *(shaBuf + i));
        	p += 2;
	}
	*p   = '\0';
	hapd->conf->ssid_len = strlen(hapd->conf->ssid);

}

/* set up the P2 conf file at the same level as the P1 conf file */
STATIC char *
jsw_set_path(const char *abs_fname, const char *rel_fname, 
			   struct jsw_session *js)
{
	int abs_path_len = strlen(abs_fname);
	int len = 0;
	char *buf = NULL;
	int ctr = 0, found = 0;
	int path_len = 0;
	
	/* Get the last '/' in the path of P1 conf file */
	while ((!found) && (ctr < abs_path_len)) {
		found = (abs_fname[abs_path_len - ctr - 1] == '/');
		ctr += (found ? 0 : 1);
	}
	
	if (!found) {
		hostapd_logger(js->hapd, js->sta->addr, HOSTAPD_MODULE_JS,
				HOSTAPD_LEVEL_WARNING, 
				"could not get path for %s",
				abs_fname);
		return NULL;
	}
	
	/* Allocate a buffer to hold the absolute path name for P2 conf file */
	path_len = abs_path_len - ctr;
	len = path_len + strlen(rel_fname) + 1;
	buf = (char *)malloc(len);
	if (buf == NULL) {
		hostapd_logger(js->hapd, js->sta->addr, HOSTAPD_MODULE_JS,
				HOSTAPD_LEVEL_WARNING,
				"could not allocate buf for path %s",
				abs_fname);
		return NULL;
	}
	
	memcpy(buf, abs_fname, path_len);
	memcpy(&buf[path_len], rel_fname, strlen(rel_fname) + 1);

	return buf;
}

void
jsw_create_psk_conf(struct hostapd_data *hapd, struct jsw_session *js)
{
	struct jsw_profile *jp;
	int i;
	FILE *f_p1, *f_p2;
	char *fname_p1,*fname_p2;
	u8 psk[2 * PMK_LEN + 1];
	u8 passHash[2 * JSW_SHA1_LEN + 1];
	u8 salt[2 * JSW_NONCE_SIZE + 1];
	u8 *p;
	char bufLine[512];
	char buf[256], *pos;
	jp = hapd->jsw_profile;
	int line = 0;
	
	p = psk;
	for (i = 0; i < PMK_LEN; i++) {
		sprintf(p, "%02x", *(jp->secret.PMK + i));
		p += 2;
	}
	*p = '\0';
	
	p = passHash;
	for (i = 0; i < JSW_SHA1_LEN; i++) {
		sprintf(p, "%02x", *(jp->passHash + i));
		p += 2;
	}
	*p = '\0';

	p = salt;
	for (i = 0; i < JSW_NONCE_SIZE; i++) {
		sprintf(p, "%02x", *(jp->salt + i));
		p += 2;
	}
	*p = '\0';
	
	fname_p1 = hapd->config_fname;
	fname_p2 = jsw_set_path(fname_p1, "js_p2.conf", js);
	if (!fname_p2)
		return;

	f_p1 = fopen(fname_p1, "r");
	if (f_p1 == NULL) {
		hostapd_logger(js->hapd, js->sta->addr, HOSTAPD_MODULE_JS,
				HOSTAPD_LEVEL_WARNING, "could not open %s: %s",
				fname_p1, strerror(errno));
	}
	
	f_p2 = fopen(fname_p2, "w");
	if (f_p2 == NULL) {
		hostapd_logger(hapd, js->sta->addr, HOSTAPD_MODULE_JS,
				HOSTAPD_LEVEL_WARNING, "could not open %s: %s",
				fname_p2, strerror(errno));
	}
	
	if (f_p1 == NULL || f_p2 == NULL)
		return;
	
	while (fgets(buf, sizeof(buf), f_p1)) {
		line++;
		if (buf[0] == '#')
			continue;
		pos = buf;
		while (*pos != '\0') {
			if (*pos == '\n') {
				*pos = '\0';
				break;
			}
			pos++;
		}
		if (buf[0] == '\0')
			continue;

		pos = strchr(buf, '=');
		if (pos == NULL) {
			printf("Line %d: invalid line '%s'\n", line, buf);
			continue;
		}
		*pos = '\0';
		pos++;
		
		if (strcmp(buf, "wpa_psk") == 0) {
			snprintf(bufLine, sizeof(bufLine), "%s=%s",
				 buf, psk);
			fprintf(f_p2, "%s\n", bufLine);
		} else if (strcmp(buf, "jumpstart_p1") == 0) {
			/* Set P2 flag */
			snprintf(bufLine, sizeof(bufLine), "jumpstart_p2=1");
			fprintf(f_p2, "%s\n", bufLine);
			
			/* Copy password hash */
			snprintf(bufLine, sizeof(bufLine),
				 "jumpstart_passHash=%s",
				 passHash);
			fprintf(f_p2, "%s\n", bufLine);

			/* Copy salt */
			snprintf(bufLine, sizeof(bufLine),
				 "jumpstart_salt=%s",
				 salt);
			fprintf(f_p2, "%s\n", bufLine);
			
		} else {
			/* Duplicate lines unrelated to Jumpstart */
			snprintf(bufLine, sizeof(bufLine), "%s=%s",
				 buf, pos);
			fprintf(f_p2, "%s\n", bufLine);
		}
		
	}
	fclose(f_p1);
	fclose(f_p2);
	free(hapd->config_fname);
	hapd->config_fname = strdup(fname_p2);
}
