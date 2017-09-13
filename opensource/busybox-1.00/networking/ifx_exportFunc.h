#ifndef _IFX_EXPORTFUNC_H_
#define _IFX_EXPORTFUNC_H_
/* IFX_INETD_ENHANCEMENT: Nirav start */
#define IFX_HTTPD_INACTIVITY_TIMEOUT	600		/* Timeout after which httpd can exit safely */
/* IFX_INETD_ENHANCEMENT: Nirav end */

typedef int (*IFX_SENDHEADERS)(int responseNum, char *redirectURL);
typedef int (*IFX_READ)(int,void *,int);
typedef int (*IFX_WRITE)(int,const void *,int);
typedef void (*IFX_GETHEADERDATA)(const char *,char **);

struct ifx_exportFuncList
{
	IFX_SENDHEADERS ifx_sendHeaders;
	IFX_READ ifx_read;
	IFX_WRITE ifx_write;
	IFX_GETHEADERDATA ifx_getHeaderData;
	void *dlHandle;
};



// Name Of ifx_lib file
#define IFX_LIB "/lib/libifx_httpd.so"

#endif /* _IFX_EXPORTFUNC_H_ */
