/* Adapted from MIT res_search.c - Ritesh */
#include <pthread.h>
#include <resolv.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#define HOSTALIASES	"/etc/hosts"  /* guessing ??? Ritesh */


#define RES_INIT        0x00000001      /* address initialized */
#define RES_DEBUG       0x00000002      /* print debug messages */
#define RES_AAONLY      0x00000004      /* authoritative answers only (!IMPL)*/
#define RES_USEVC       0x00000008      /* use virtual circuit */
#define RES_PRIMARY     0x00000010      /* query primary server only (!IMPL) */
#define RES_IGNTC       0x00000020      /* ignore trucation errors */
#define RES_RECURSE     0x00000040      /* recursion desired */
#define RES_DEFNAMES    0x00000080      /* use default domain name */
#define RES_STAYOPEN    0x00000100      /* Keep TCP socket open */
#define RES_DNSRCH      0x00000200      /* search up local domain tree */
#define RES_INSECURE1   0x00000400      /* type 1 security disabled */
#define RES_INSECURE2   0x00000800      /* type 2 security disabled */
#define RES_NOALIASES   0x00001000      /* shuts off HOSTALIASES feature */
#define RES_USE_INET6   0x00002000      /* use/map IPv6 in gethostbyname() */
#define RES_ROTATE      0x00004000      /* rotate ns list after each query */
#define RES_NOCHECKNAME 0x00008000      /* do not check names for sanity. */
#define RES_KEEPTSIG    0x00010000      /* do not strip TSIG records */
#define RES_BLAST       0x00020000      /* blast all recursive servers */

static int dummy_resolver_options = 0;

static char *search_aliases(const char *name, char *buf, int bufsize);

int res_search(const char *name, int class, int type, unsigned char *answer,
	       int anslen)
{
    const char *p;
    int num_dots, len, result, no_data = 0, error;
    char buf[2 * MAXDNAME + 2], *domain, **dptr, *alias;


    /* Count the dots in name, and get a pointer to the end of name. */
    num_dots = 0;
    for (p = name; *p; p++) {
	if (*p == '.')
	    num_dots++;
    }
    len = p - name;

    /* If there aren't any dots, check to see if name is an alias for
     * another host.  If so, try the resolved alias as a fully-qualified
     * name. */
    alias = search_aliases(name, buf, sizeof(buf));
    if (alias != NULL)
	return res_query(alias, class, type, answer, anslen);

    /* If there's a trailing dot, try to strip it off and query the name. */
    if (len > 0 && p[-1] == '.') {
	if (len > sizeof(buf)) {
	    /* It's too long; just query the original name. */
	    return res_query(name, class, type, answer, anslen);
	} else {
	    /* Copy the name without the trailing dot and query. */
	    memcpy(buf, name, len - 1);
	    buf[len] = 0;
	    return res_query(buf, class, type, answer, anslen);
	}
    }

#if 0 /* [ uClibc has no support for this whatever --> hack it for now */
    if (dummy_resolver_options & RES_DNSRCH) {
	/* If RES_DNSRCH is set, query all the domains until we get a
	 * definitive answer. */
	for (dptr = data->status.dnsrch; *dptr; dptr++) {
	    domain = *dptr;
	    sprintf(buf, "%.*s.%.%s", MAXDNAME, name, MAXDNAME, domain);
	    result = res_query(buf, class, type, answer, anslen);
	    if (result > 0)
		return result;
	    if (data->errval == NO_DATA)
		no_data = 1;
	    else if (data->errval != HOST_NOT_FOUND)
		break;
	}
    } else if (num_dots == 0 && data->status.options & RES_DEFNAMES) {
	/* If RES_DEFNAMES is set and there is no dot, query the default
	 * domain. */
	domain = data->status.defdname;
	sprintf(buf, "%.*s.%.%s", MAXDNAME, name, MAXDNAME, domain);
	result = res_query(buf, class, type, answer, anslen);
	if (result > 0)
	    return result;
	if (data->errval == NO_DATA)
	    no_data = 1;
    }
#endif

    /* If all the domain queries failed, try the name as fully-qualified.
     * Only do this if there is at least one dot in the name. */
    if (num_dots > 0) {
	result = res_query(name, class, type, answer, anslen);
	if (result > 0)
	    return result;
    }

    return -1;
}

static char *search_aliases(const char *name, char *buf, int bufsize)
{
    FILE *fp;
    char *filename, *p;
    int len;

    filename = getenv("HOSTALIASES");
    if (filename == NULL)
	return NULL;

    fp = fopen(filename, "r");
    if (fp == NULL)
	return NULL;

    len = strlen(name);
    while (fgets(buf, bufsize, fp)) {

	/* Get the first word from the buffer. */
	p = buf;
	while (*p && !isspace(*p))
	    p++;
	if (!*p)
	    break;

	/* Null-terminate the first word and compare it with the name. */
	*p = 0;
	if (strcasecmp(buf, name) != 0)
	    continue;

	p++;
	while (isspace(*p))
	    p++;
	fclose(fp);
	return (*p) ? p : NULL;
    }

    fclose(fp);
    return NULL;
}

