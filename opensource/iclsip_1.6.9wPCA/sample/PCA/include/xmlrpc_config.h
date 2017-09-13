/* xmlrpc_config.h.  Generated automatically by configure.  */
/* xmlrpc_config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if va_list is actually an array. */
/* #undef VA_LIST_IS_ARRAY */

/* Define if we're using a copy of libwww with built-in SSL support. */
/* #undef HAVE_LIBWWW_SSL */

/* We use this to mark unused variables under GCC. */
#define ATTR_UNUSED __attribute__((__unused__))

/* Define this if your C library provides reasonably complete and correct
** Unicode wchar_t support. */
#define HAVE_UNICODE_WCHAR 1

/* The kind of system we're allegedly running on.  Used for diagnostics. */
#define XMLRPC_HOST_TYPE "i686-pc-linux-gnu"

/* The path separator used by the host operating system. */
#define PATH_SEPARATOR "/"

/* Define if we're building the libwww xmlrpc_client. */
/* #undef BUILD_LIBWWW_CLIENT */

/* Define if we're building the libwww xmlrpc_client. */
#define BUILD_CURL_CLIENT 1

/* Define if we're building the libwww xmlrpc_client. */
/* #undef BUILD_WININET_CLIENT */

/* Define the default transport */
#define XMLRPCDEFAULTTRANSPORT "curl"

/* Define if you have the setgroups function.  */
#define HAVE_SETGROUPS 1

/* Define if you have the wcsncmp function.  */
#define HAVE_WCSNCMP 1

/* Define if you have the <dlfcn.h> header file.  */
#define HAVE_DLFCN_H 1

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <sys/filio.h> header file.  */
/* #undef HAVE_SYS_FILIO_H */

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <wchar.h> header file.  */
#define HAVE_WCHAR_H 1

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Name of package */
#define PACKAGE "xmlrpc-c"

/* Version number of package */
#define VERSION "0.9.9"

