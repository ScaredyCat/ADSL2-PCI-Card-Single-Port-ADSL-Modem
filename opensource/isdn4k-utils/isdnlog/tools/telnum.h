/* telnum.h
  (c) 1999 by Leopold Toetsch <lt@toetsch.at>
*/  
#ifndef _TELNUM_H_
#define _TELNUM_H_
#ifndef STANDALONE
#include "isdnlog.h"
#include "tools/zone.h"
#endif

/* print_msg */
#define PRT_V PRT_INFO /* verbose */
#define PRT_A PRT_ERR /* always on stderr */

/* string lens */

#define TN_MAX_VBN_LEN 4
#define TN_MAX_PROVIDER_LEN 12
#define TN_MAX_COUNTRY_LEN 8
#define TN_MAX_SCOUNTRY_LEN 40
#define TN_MAX_COUNTRY_LEN 8
#define TN_MAX_AREA_LEN 10
#define TN_MAX_SAREA_LEN 40
#define TN_MAX_MSN_LEN 10
#define TN_MAX_NUM_LEN (TN_MAX_PROVIDER_LEN+TN_MAX_COUNTRY_LEN+TN_MAX_AREA_LEN+TN_MAX_MSN_LEN+4)

/* for number 1002 0043 1 2345 or 1002 01 2345 it gives */
typedef struct {
  char vbn[TN_MAX_VBN_LEN];		/* "10" */
  char provider[TN_MAX_PROVIDER_LEN];	/* "UTA" */
  int nprovider;			/* 2 */
  char scountry[TN_MAX_SCOUNTRY_LEN];	/* "Austria" */
  char country[TN_MAX_COUNTRY_LEN];	/* "+43" */
  char keys[TN_MAX_SCOUNTRY_LEN];	/* "VIA/AT" */
  char tld[3];				/* "AT" */
  int ncountry;				/* 43 */
  char area[TN_MAX_AREA_LEN];		/* "1" */
  int narea;				/* 1 */
  char sarea[TN_MAX_SAREA_LEN];		/* "Wien" */
  char msn[TN_MAX_MSN_LEN];		/* "2345" */
} TELNUM;

/* flags */
#define TN_PROVIDER 1
#define TN_COUNTRY 	2
#define TN_AREA		4
#define TN_MSN 		8
#define TN_ALL 		15
#define TN_NO_PROVIDER 14
#define TN_NOCLEAR 0x80

/* functions */
void initTelNum(void);
int normalizeNumber(char *target, TELNUM *num, int flag);
char * formatNumber(char* format, TELNUM* num);
char *prefix2provider(int prefix, char*prov);
int provider2prefix(char *p, int *prefix); 
void clearNum(TELNUM *num);
void initNum(TELNUM *num);
TELNUM * getMynum(void);
#endif
