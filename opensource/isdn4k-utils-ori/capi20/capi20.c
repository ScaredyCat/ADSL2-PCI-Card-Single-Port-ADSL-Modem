/*
 * $Id: capi20.c,v 1.19 2001/03/01 14:59:11 paul Exp $
 * 
 * $Log: capi20.c,v $
 * Revision 1.19  2001/03/01 14:59:11  paul
 * Various patches to fix errors when using the newest glibc,
 * replaced use of insecure tempnam() function
 * and to remove warnings etc.
 *
 * Revision 1.18  2000/11/12 16:06:41  kai
 * fix backwards compatibility in capi20 library, small other changes
 *
 * Revision 1.17  2000/06/26 15:00:43  calle
 * - Will also compile with 2.0 Kernelheaders.
 *
 * Revision 1.16  2000/05/18 15:02:26  calle
 * Updated _cmsg handling added new functions need by "capiconn".
 *
 * Revision 1.15  2000/04/10 09:08:06  calle
 * capi20_wait_for_message will now return CapiReceiveQueueEmpty on
 * timeout and error.
 *
 * Revision 1.14  2000/04/07 16:06:09  calle
 * Bugfix: without devfs open where without NONBLOCK, ahhh.
 *
 * Revision 1.13  2000/04/03 14:27:15  calle
 * non CAPI2.0 standard functions now named capi20ext not capi20.
 * Extentionfunctions will work with actual driver version.
 *
 * Revision 1.12  2000/03/03 15:56:14  calle
 * - now uses cloning device /dev/capi20.
 * - middleware extentions prepared.
 *
 * Revision 1.11  1999/12/22 17:46:21  calle
 * - Last byte in serial number now always 0.
 * - Last byte of manufacturer now always 0.
 * - returncode in capi20_isinstalled corrected.
 *
 * Revision 1.10  1999/11/11 09:24:07  calle
 * add shared lib destructor, to close "capi_fd" on unload with dlclose ..
 *
 * Revision 1.9  1999/10/20 16:43:17  calle
 * - The CAPI20 library is now a shared library.
 * - Arguments of function capi20_put_message swapped, to match capi spec.
 * - All capi20 related subdirs converted to use automake.
 * - Removed dependency to CONFIG_KERNELDIR where not needed.
 *
 * Revision 1.8  1999/09/15 08:10:44  calle
 * Bugfix: error in 64Bit extention.
 *
 * Revision 1.7  1999/09/10 17:20:33  calle
 * Last changes for proposed standards (CAPI 2.0):
 * - AK1-148 "Linux Extention"
 * - AK1-155 "Support of 64-bit Applications"
 *
 * Revision 1.6  1999/09/06 17:40:07  calle
 * Changes for CAPI 2.0 Spec.
 *
 * Revision 1.5  1999/04/20 19:52:19  calle
 * Bugfix in capi20_get_profile: wrong size in memcpy from
 * Kai Germaschewski <kai@thphy.uni-duesseldorf.de>
 *
 * Revision 1.4  1998/11/18 17:05:44  paul
 * fixed a (harmless) warning
 *
 * Revision 1.3  1998/08/30 09:57:14  calle
 * I hope it is know readable for everybody.
 *
 * Revision 1.1  1998/08/25 16:33:16  calle
 * Added CAPI2.0 library. First Version.
 *
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/capi.h>
#include "capi20.h"

#ifndef CAPI_GET_FLAGS
#define CAPI_GET_FLAGS		_IOR('C',0x23, unsigned)
#endif
#ifndef CAPI_SET_FLAGS
#define CAPI_SET_FLAGS		_IOR('C',0x24, unsigned)
#endif
#ifndef CAPI_CLR_FLAGS
#define CAPI_CLR_FLAGS		_IOR('C',0x25, unsigned)
#endif
#ifndef CAPI_NCCI_OPENCOUNT
#define CAPI_NCCI_OPENCOUNT	_IOR('C',0x26, unsigned)
#endif
#ifndef CAPI_NCCI_GETUNIT
#define CAPI_NCCI_GETUNIT	_IOR('C',0x27, unsigned)
#endif

static char capidevname[] = "/dev/capi20";
static char capidevnamenew[] = "/dev/isdn/capi20";

static int                  capi_fd = -1;
static capi_ioctl_struct    ioctl_data;
static unsigned char        rcvbuf[128+2048];   /* message + data */
static unsigned char        sndbuf[128+2048];   /* message + data */

unsigned capi20_isinstalled (void)
{
    if (capi_fd >= 0)
        return CapiNoError;

    /*----- open managment link -----*/
    if ((capi_fd = open(capidevname, O_RDWR, 0666)) < 0 && errno == ENOENT)
       capi_fd = open(capidevnamenew, O_RDWR, 0666);
    if (capi_fd < 0)
       return CapiRegNotInstalled;

    if (ioctl(capi_fd, CAPI_INSTALLED, 0) == 0)
	return CapiNoError;
    return CapiRegNotInstalled;
}

/*
 * managment of application ids
 */

#define MAX_APPL 1024

static int applidmap[MAX_APPL];

static inline int remember_applid(unsigned applid, int fd)
{
   if (applid >= MAX_APPL)
	return -1;
   applidmap[applid] = fd;
   return 0;
}

static inline unsigned alloc_applid(int fd)
{
   unsigned applid;
   for (applid=1; applid < MAX_APPL; applid++) {
       if (applidmap[applid] < 0) {
          applidmap[applid] = fd;
          return applid;
       }
   }
   return 0;
}

static inline void freeapplid(unsigned applid)
{
    if (applid < MAX_APPL)
       applidmap[applid] = -1;
}

static inline int validapplid(unsigned applid)
{
    return applid > 0 && applid < MAX_APPL && applidmap[applid] >= 0;
}

static inline int applid2fd(unsigned applid)
{
    if (applid < MAX_APPL)
	    return applidmap[applid];
    return -1;
}

/* 
 * CAPI2.0 functions
 */

unsigned
capi20_register (unsigned MaxB3Connection,
		 unsigned MaxB3Blks,
		 unsigned MaxSizeB3,
		 unsigned *ApplID)
{
    int applid = 0;
    char buf[PATH_MAX];
    int i, fd = -1;

    *ApplID = 0;

    if (capi20_isinstalled() != CapiNoError)
       return CapiRegNotInstalled;

    if ((fd = open(capidevname, O_RDWR|O_NONBLOCK, 0666)) < 0 && errno == ENOENT)
         fd = open(capidevnamenew, O_RDWR|O_NONBLOCK, 0666);

    if (fd < 0)
	return CapiRegOSResourceErr;

    ioctl_data.rparams.level3cnt = MaxB3Connection;
    ioctl_data.rparams.datablkcnt = MaxB3Blks;
    ioctl_data.rparams.datablklen = MaxSizeB3;

    if ((applid = ioctl(fd, CAPI_REGISTER, &ioctl_data)) < 0) {
        if (errno == EIO) {
            if (ioctl(fd, CAPI_GET_ERRCODE, &ioctl_data) < 0) {
		close (fd);
                return CapiRegOSResourceErr;
	    }
	    close (fd);
            return (unsigned)ioctl_data.errcode;

        } else if (errno == EINVAL) { // old kernel driver
	    close (fd);
	    fd = -1;
	    for (i=0; fd < 0; i++) {
		/*----- open pseudo-clone device -----*/
		sprintf(buf, "/dev/capi20.%02d", i);
		if ((fd = open(buf, O_RDWR|O_NONBLOCK, 0666)) < 0) {
		    switch (errno) {
		    case EEXIST:
			break;
		    default:
			return CapiRegOSResourceErr;
		    }
		}
	    }
	    if (fd < 0)
		return CapiRegOSResourceErr;

	    ioctl_data.rparams.level3cnt = MaxB3Connection;
	    ioctl_data.rparams.datablkcnt = MaxB3Blks;
	    ioctl_data.rparams.datablklen = MaxSizeB3;

	    if ((applid = ioctl(fd, CAPI_REGISTER, &ioctl_data)) < 0) {
		if (errno == EIO) {
		    if (ioctl(fd, CAPI_GET_ERRCODE, &ioctl_data) < 0) {
			close(fd);
			return CapiRegOSResourceErr;
		    }
		    close(fd);
		    return (unsigned)ioctl_data.errcode;
		}
		close(fd);
		return CapiRegOSResourceErr;
	    }
	    applid = alloc_applid(fd);
	} // end old driver compatibility
    }
    if (remember_applid(applid, fd) < 0) {
       close(fd);
       return CapiRegOSResourceErr;
    }
    *ApplID = applid;
    return CapiNoError;
}

unsigned
capi20_release (unsigned ApplID)
{
    if (capi20_isinstalled() != CapiNoError)
       return CapiRegNotInstalled;
    if (!validapplid(ApplID))
        return CapiIllAppNr;
    (void)close(applid2fd(ApplID));
    freeapplid(ApplID);
    return CapiNoError;
}

unsigned
capi20_put_message (unsigned ApplID, unsigned char *Msg)
{
    unsigned ret;
    int len = (Msg[0] | (Msg[1] << 8));
    int cmd = Msg[4];
    int subcmd = Msg[5];
    int rc;
    int fd;

    if (capi20_isinstalled() != CapiNoError)
       return CapiRegNotInstalled;

    if (!validapplid(ApplID))
        return CapiIllAppNr;

    fd = applid2fd(ApplID);

    memcpy(sndbuf, Msg, len);

    if (cmd == CAPI_DATA_B3 && subcmd == CAPI_REQ) {
        int datalen = (Msg[16] | (Msg[17] << 8));
        void *dataptr;
        if (sizeof(void *) != 4) {
	    if (len >= 30) { /* 64Bit CAPI-extention */
	       u_int64_t data64;
	       memcpy(&data64,Msg+22, sizeof(u_int64_t));
	       if (data64 != 0) dataptr = (void *)(unsigned long)data64;
	       else dataptr = Msg + len; /* Assume data after message */
	    } else {
               dataptr = Msg + len; /* Assume data after message */
	    }
        } else {
            u_int32_t data;
            memcpy(&data,Msg+12, sizeof(u_int32_t));
            if (data != 0) dataptr = (void *)(unsigned long)data;
            else dataptr = Msg + len; /* Assume data after message */
	}
        memcpy(sndbuf+len, dataptr, datalen);
        len += datalen;
    }

    ret = CapiNoError;
    errno = 0;

    if ((rc = write(fd, sndbuf, len)) != len) {
        switch (errno) {
            case EFAULT:
            case EINVAL:
                ret = CapiIllCmdOrSubcmdOrMsgToSmall;
                break;
            case EBADF:
                ret = CapiIllAppNr;
                break;
            case EIO:
                if (ioctl(fd, CAPI_GET_ERRCODE, &ioctl_data) < 0)
                    ret = CapiMsgOSResourceErr;
                else ret = (unsigned)ioctl_data.errcode;
                break;
          default:
                ret = CapiMsgOSResourceErr;
                break;
       }
    }

    return ret;
}

unsigned
capi20_get_message (unsigned ApplID, unsigned char **Buf)
{
    unsigned ret;
    int rc, fd;

    if (capi20_isinstalled() != CapiNoError)
       return CapiRegNotInstalled;

    if (!validapplid(ApplID))
        return CapiIllAppNr;

    fd = applid2fd(ApplID);

    *Buf = rcvbuf;
    if ((rc = read(fd, rcvbuf, sizeof(rcvbuf))) > 0) {
	CAPIMSG_SETAPPID(rcvbuf, ApplID); // workaround for old driver
        if (   CAPIMSG_COMMAND(rcvbuf) == CAPI_DATA_B3
	    && CAPIMSG_SUBCOMMAND(rcvbuf) == CAPI_IND) {
           if (sizeof(void *) == 4) {
	       u_int32_t data = (u_int32_t)rcvbuf + CAPIMSG_LEN(rcvbuf);
	       rcvbuf[12] = data & 0xff;
	       rcvbuf[13] = (data >> 8) & 0xff;
	       rcvbuf[14] = (data >> 16) & 0xff;
	       rcvbuf[15] = (data >> 24) & 0xff;
           } else {
	       u_int64_t data;
	       if (CAPIMSG_LEN(rcvbuf) < 30) {
		  /*
		   * grr, 64bit arch, but no data64 included,
	           * seems to be old driver
		   */
	          memmove(rcvbuf+30, rcvbuf+CAPIMSG_LEN(rcvbuf),
		          CAPIMSG_DATALEN(rcvbuf));
	          rcvbuf[0] = 30;
	          rcvbuf[1] = 0;
	       }
	       data = (u_int64_t)rcvbuf + CAPIMSG_LEN(rcvbuf);
	       rcvbuf[12] = rcvbuf[13] = rcvbuf[14] = rcvbuf[15] = 0;
	       rcvbuf[22] = data & 0xff;
	       rcvbuf[23] = (data >> 8) & 0xff;
	       rcvbuf[24] = (data >> 16) & 0xff;
	       rcvbuf[25] = (data >> 24) & 0xff;
	       rcvbuf[26] = (data >> 32) & 0xff;
	       rcvbuf[27] = (data >> 40) & 0xff;
	       rcvbuf[28] = (data >> 48) & 0xff;
	       rcvbuf[29] = (data >> 56) & 0xff;
	   }
	}
        return CapiNoError;
    }

    if (rc == 0)
        return CapiReceiveQueueEmpty;

    switch (errno) {
        case EMSGSIZE:
            ret = CapiIllCmdOrSubcmdOrMsgToSmall;
            break;
        case EAGAIN:
            return CapiReceiveQueueEmpty;
        default:
            ret = CapiMsgOSResourceErr;
            break;
    }

    return ret;
}

unsigned char *
capi20_get_manufacturer(unsigned Ctrl, unsigned char *Buf)
{
    if (capi20_isinstalled() != CapiNoError)
       return 0;
    ioctl_data.contr = Ctrl;
    if (ioctl(capi_fd, CAPI_GET_MANUFACTURER, &ioctl_data) < 0)
       return 0;
    memcpy(Buf, ioctl_data.manufacturer, CAPI_MANUFACTURER_LEN);
    Buf[CAPI_MANUFACTURER_LEN-1] = 0;
    return Buf;
}

unsigned char *
capi20_get_version(unsigned Ctrl, unsigned char *Buf)
{
    if (capi20_isinstalled() != CapiNoError)
        return 0;
    ioctl_data.contr = Ctrl;
    if (ioctl(capi_fd, CAPI_GET_VERSION, &ioctl_data) < 0)
        return 0;
    memcpy(Buf, &ioctl_data.version, sizeof(capi_version));
    return Buf;
}

unsigned char * 
capi20_get_serial_number(unsigned Ctrl, unsigned char *Buf)
{
    if (capi20_isinstalled() != CapiNoError)
        return 0;
    ioctl_data.contr = Ctrl;
    if (ioctl(capi_fd, CAPI_GET_SERIAL, &ioctl_data) < 0)
        return 0;
    memcpy(Buf, &ioctl_data.serial, CAPI_SERIAL_LEN);
    Buf[CAPI_SERIAL_LEN-1] = 0;
    return Buf;
}

unsigned
capi20_get_profile(unsigned Ctrl, unsigned char *Buf)
{
    if (capi20_isinstalled() != CapiNoError)
        return CapiMsgNotInstalled;

    ioctl_data.contr = Ctrl;
    if (ioctl(capi_fd, CAPI_GET_PROFILE, &ioctl_data) < 0) {
        if (errno != EIO)
            return CapiMsgOSResourceErr;
        if (ioctl(capi_fd, CAPI_GET_ERRCODE, &ioctl_data) < 0)
            return CapiMsgOSResourceErr;
        return (unsigned)ioctl_data.errcode;
    }
    if (Ctrl)
        memcpy(Buf, &ioctl_data.profile, sizeof(struct capi_profile));
    else
        memcpy(Buf, &ioctl_data.profile.ncontroller,
                       sizeof(ioctl_data.profile.ncontroller));
    return CapiNoError;
}
/*
 * functions added to the CAPI2.0 spec
 */

unsigned
capi20_waitformessage(unsigned ApplID, struct timeval *TimeOut)
{
  int fd;
  fd_set rfds;

  FD_ZERO(&rfds);

  if (capi20_isinstalled() != CapiNoError)
    return CapiRegNotInstalled;

  if(!validapplid(ApplID))
    return CapiIllAppNr;
  
  fd = applid2fd(ApplID);

  FD_SET(fd, &rfds);
  
  if (select(fd + 1, &rfds, NULL, NULL, TimeOut) < 1)
	return CapiReceiveQueueEmpty;
  
  return CapiNoError;
}

int
capi20_fileno(unsigned ApplID)
{
   return applid2fd(ApplID);
}

/*
 * Extensions for middleware
 */

int
capi20ext_get_flags(unsigned ApplID, unsigned *flagsptr)
{
   if (ioctl(applid2fd(ApplID), CAPI_GET_FLAGS, flagsptr) < 0)
      return CapiMsgOSResourceErr;
   return CapiNoError;
}

int
capi20ext_set_flags(unsigned ApplID, unsigned flags)
{
   if (ioctl(applid2fd(ApplID), CAPI_SET_FLAGS, &flags) < 0)
      return CapiMsgOSResourceErr;
   return CapiNoError;
}

int
capi20ext_clr_flags(unsigned ApplID, unsigned flags)
{
   if (ioctl(applid2fd(ApplID), CAPI_CLR_FLAGS, &flags) < 0)
      return CapiMsgOSResourceErr;
   return CapiNoError;
}

char *
capi20ext_get_tty_devname(unsigned applid, unsigned ncci, char *buf, size_t size)
{
	int unit;
        unit = ioctl(applid2fd(applid), CAPI_NCCI_GETUNIT, &ncci);
        if (unit < 0)
		return 0;
	snprintf(buf, size, "/dev/capi/%d", unit);
	return buf;
}

char *
capi20ext_get_raw_devname(unsigned applid, unsigned ncci, char *buf, size_t size)
{
	int unit;
        unit = ioctl(applid2fd(applid), CAPI_NCCI_GETUNIT, &ncci);
        if (unit < 0)
		return 0;
	snprintf(buf, size, "/dev/capi/r%d", unit);
	return buf;
}

int capi20ext_ncci_opencount(unsigned applid, unsigned ncci)
{
   return ioctl(applid2fd(applid), CAPI_NCCI_OPENCOUNT, &ncci);
}

static void initlib(void) __attribute__((constructor));
static void exitlib(void) __attribute__((destructor));

static void initlib(void)
{
   int i;
   for (i=0; i < MAX_APPL; i++)
	applidmap[i] = -1;
}

static void exitlib(void)
{
    if (capi_fd >= 0) {
       close(capi_fd);
       capi_fd = -1;
    }
}
