/* this implements ioctl(IIOCNETGPN) for 2.0 Kernels in user space
 *
 * int iiocnetgpn(int isdnctrl_desc, isdn_net_ioctl_phone *phone);
 *
 *
 * Copyright 2001 by Leopold Toetsch <lt@toetsch.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *
 *
 * for standalone testing compile with:
 * cc iiocnetgpn -I/usr/src/linux/include -DSTANDALONE -V2.7.2.3 -o iiocnetgpn
 *    (the compilerversion may be different, look at your kernel Makefile)
 *
 * referenced files are all in /usr/src/linux/isdn/drivers/isdn
 */

// #include <errno.h>
// #include <unistd.h>
//
extern int errno; /* above generates a lot of errors due to mixing of headers */

#define __KERNEL__
#include <linux/isdn.h>
#undef __KERNEL__

#define SEEK_SET 0

typedef unsigned char uchar;
static int mem_fd;
static isdn_dev *dev;

/* based on debugvar.c */
#if USE_MM        /* MM is not working on /dev/kmem */
#include <sys/mman.h> 
static ulong msize(long size) {
	ulong ps = getpagesize();
	return (size / ps + 1) * ps;
}

static uchar * mapmem(ulong location, long size)
{
	ulong mmseg;
	ulong mmsize;
	uchar* addr=0;
	ulong offset;

	mmseg = location & ~(ps-1);
	mmsize = msize(size);
	offset = location  - mmseg;
	addr = (uchar*) mmap(0, mmsize, PROT_READ, MAP_SHARED, mem_fd, mmseg);
	if ((int) addr == -1) {
		return 0;
	}
	return ((uchar *) (addr + offset));
}
#else
void *malloc(long);
void free(void*);

static uchar
* mapmem(ulong location, long size)
{
	uchar *buffer = 0;

	if ((buffer = malloc(size))) {
		lseek(mem_fd, location, SEEK_SET);
		read(mem_fd, buffer, size);
	} 
	return buffer;
}
#endif

/* based on isdn_common.c */
static int
isdn_dc2minor(int di, int ch)
{
	int i;
	for (i = 0; i < ISDN_MAX_CHANNELS; i++)
		if (dev->chanmap[i] == ch && dev->drvmap[i] == di)
			return i;
	return -1;
}
 
/* based on isdn_net.c */
static isdn_net_dev *
isdn_net_findif(char *name)
{
	ulong addr = (ulong)dev->netdev;

	while (addr) {
		isdn_net_dev *p = (isdn_net_dev *) mapmem(addr, sizeof(isdn_net_dev));
		if (!strcmp(p->local.name, name))
			return p;
		addr = (ulong) p->next;
#if USE_MM
		munmap(p, msize(sizeof(isdn_net_dev)));
#else		
		free(p);
#endif		
	}
	return (isdn_net_dev *) NULL;
}

static int getpeer(isdn_net_ioctl_phone *phone)
{
	isdn_net_dev *p = isdn_net_findif(phone->name);
	int ch, dv, idx;
	int ret = 0;
	
	if (!p) return -ENODEV;
	ch = p->local.isdn_channel;
	dv = p->local.isdn_device;
	if(ch<0 && dv<0) return -ENOTCONN;
	idx = isdn_dc2minor(dv, ch);
	if (idx<0) {
	    ret = -ENODEV;
	    goto err;
	}    
	/* for pre-bound channels, we need this extra check */
	if ( strncmp(dev->num[idx],"???",3) == 0 ) {
	    ret= -ENOTCONN;
	    goto err;
	}    
	strncpy(phone->phone,dev->num[idx],ISDN_MSNLEN);
	phone->outgoing=USG_OUTGOING(dev->usage[idx]);
err:	
#if USE_MM
	munmap(p, msize(sizeof(isdn_net_dev)));
#else		
	free(p);
#endif		
	return ret;
}

/* based on debugvar.c */

int iiocnetgpn(int isdnctrl_desc, isdn_net_ioctl_phone *phone) {
	ulong kaddr;
	int ret;
   
        errno=0;
	if (ioctl(isdnctrl_desc, IIOCDBGVAR, &kaddr)) {
	        errno=EINVAL;
		return(-1);
	}
	if ((mem_fd = open("/dev/kmem", O_RDONLY)) < 0) {
	        errno=EINVAL;
		return(-1);
	}
	dev = (isdn_dev *) mapmem(kaddr, sizeof(isdn_dev));
	ret = getpeer(phone);
#if USE_MM
	munmap(dev, msize(sizeof(isdn_dev)));
#else		
	free(dev);
#endif		
	close(mem_fd);
	return ret;
}

#ifdef STANDALONE
int main(int argc, char *argv[]) {
    int isdnctrl_desc = open("/dev/isdnctrl",O_RDONLY);
    isdn_net_ioctl_phone phone;
    int ret;
    
    strcpy(phone.name, argv[1] && *argv[1] ? argv[1] : "ippp0");
    if (!(ret=iiocnetgpn(isdnctrl_desc, &phone)))
	printf("Ok: '%s'\n", phone.phone);
    else
	printf("Err %d\n", ret);	
    close(isdnctrl_desc);
    return ret;
}
#endif
