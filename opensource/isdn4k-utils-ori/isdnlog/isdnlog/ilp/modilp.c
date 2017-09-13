/* modilp.c -  isdnlog procfs entry /proc/isdnlog 
 *
 * Copyright 2000 by Leopold Toetsch <lt@toetsch.at>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Changes:
 *
 * 0.10 15.12.2000 lt Initial Version
 * 0.11 21.12.2000 lt calculate duration
 * 0.12 05.04.2002 lt adapted for 2.4.x
 * 0.13 05.04.2002 lt 2. try, thanks Achim Steinmetz
 * 0.14 08.04.2002 lt fixed module unload
 * 0.15 11.04.2002 lt make it pager compatible for 2.4.x
 */

/* based on code found in lkmpg/node17.html, which is: */
/* Copyright (C) 1998-1999 by Ori Pomerantz */


/* The necessary header files */

/* Standard in kernel modules */
#include <linux/module.h>	/* Specifically, a module */
#include <linux/kernel.h>	/* We're doing kernel work */
/* Necessary because we use proc fs */
#include <linux/proc_fs.h>
#include <linux/time.h>	/* get time */

#define MODULE_VERSION "0.15"
#define MODULE_NAME "modilp"

/* In 2.2.3 /usr/include/linux/version.h includes a 
 * macro for this, but 2.0.35 doesn't - so I add it 
 * here if necessary. */
#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)*65536+(b)*256+(c))
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
#include <linux/init.h>		/* __init macros */
#define get_fast_time do_gettimeofday
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,4)
#include <asm/uaccess.h>	/* for copy_user */

#else
static inline unsigned long 
copy_from_user (void *to, const void *from, unsigned long n)
{
  int i;
  if ((i = verify_area (VERIFY_READ, from, n)) != 0)
    return i;
  memcpy_fromfs (to, from, n);
  return 0;
}

static inline unsigned long 
copy_to_user (void *to, const void *from, unsigned long n)
{
  int i;
  if ((i = verify_area (VERIFY_WRITE, to, n)) != 0)
    return i;
  memcpy_tofs (to, from, n);
  return 0;
}

#define get_fast_time do_gettimeofday
#endif


/* Here we keep the last message received */

#define MESSAGE_LENGTH 80
#define N_CHANS 2

static struct mes_t {
  char text[MESSAGE_LENGTH];
  time_t start;
} message[N_CHANS];

/* string in Connect-messages */
#define Connect "CON"

/* store duration d as text at p */

static void calc_diff(ulong d, char *p) {
  int h,m;
  char s[10];

  h = d / 3600;
  d %= 3600;
  m = d / 60;
  d %= 60;
  if (h > 99) /* forgotten connection ? */
    h = 99;
  sprintf(s, "%02d:%02d:%02d", h, m, (int)d);
  memcpy(p, s, 8);
}

/* The module's file functions ********************** */


/* Since we use the file operations struct, we can't 
 * use the special proc output provisions - we have to 
 * use a standard read function, which is this function */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static int proc_calc_metrics(char *page, char **start, off_t off,
				 int count, int *eof, int len)
{
	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count) len = count;
	if (len<0) len = 0;
	return len;
}

/* new proc-fs interface */
static int 
read_func(
		char *page,	/* data in user_space */
		char **start,	/* where to write, ignored */
		off_t off,	/* offset ignored */
		int count,	/* maximum */
		int *eof,	/* eof marker */
		void *data) 	/* N/U */
{
	int i;
	int len;
	struct timeval tv;
	get_fast_time(&tv);
	for (i=0 ; i<N_CHANS; i++)
		if (message[i].text && 
				strlen(message[i].text) >= 70 && 
				strstr(message[i].text, Connect))
			calc_diff(tv.tv_sec-message[i].start, message[i].text+62);
	len = sprintf (page,
			/*2345678901234567890123456789012345678901234567890123456789012345678901234567890 */
			/*        1         2         3         4         5         6         7          */
			"Ch State   Msn - Number                    Alias              Duration Cost\n%s%s",
			message[0].text, message[1].text);

	/* BUG_ON(len > count); ? */
#if 1
	return proc_calc_metrics(page, start, off, count, eof, len);
#else
	*eof = 1;
	return len;
#endif
}

#else	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static ssize_t 
module_output (
		struct file *file,	/* The file read */
		char *buf,	/* The buffer to put data to (in the
				 * user segment) */
		size_t len,	/* The length of the buffer */
		loff_t * offset)	/* Offset in the file - ignore */
#else
static int 
module_output (
		struct inode *inode,	/* The inode read */
		struct file *file,	/* The file read */
		char *buf,	/* The buffer to put data to (in the
				 * user segment) */
		int len)	/* The length of the buffer */
#endif
{
  static int finished = 0;
  int i;
  char output[MESSAGE_LENGTH * (N_CHANS + 1)];
  char *p;
  struct timeval tv;

  /* We return 0 to indicate end of file, that we have 
   * no more information. Otherwise, processes will 
   * continue to read from us in an endless loop. */
  if (finished)
    {
      finished = 0;
      return 0;
    }
  get_fast_time(&tv);
  for (i=0 ; i<N_CHANS; i++)
    if (message[i].text && 
	strlen(message[i].text) >= 70 && 
	strstr(message[i].text, Connect))
      calc_diff(tv.tv_sec-message[i].start, message[i].text+62);

  sprintf (output,
/*2345678901234567890123456789012345678901234567890123456789012345678901234567890 */
/*        1         2         3         4         5         6         7          */
"Ch State   Msn - Number                    Alias              Duration Cost\n%s%s",
	   message[0].text, message[1].text);
  for (p = output, i = 1; *p && i < len; p++, i++)
    ;
  len = i;
  copy_to_user (buf, output, len);

  /* Notice, we assume here that the size of the message 
   * is below len, or it will be received cut. In a real 
   * life situation, if the size of the message is less 
   * than len then we'd return len and on the second call 
   * start filling the buffer with the len+1'th byte of 
   * the message. */
  finished = 1;

  return len;			/* Return the number of bytes "read" */
}
#endif

/* This function receives input from the user when the 
 * user writes to the /proc file. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static ssize_t 
module_input (
	       struct file *file,	/* The file itself */
	       const char *buf,	/* The buffer with input */
	       size_t length,	/* The buffer's length */
	       loff_t * offset)	/* offset to file - ignore */
#else
static int 
module_input (
	       struct inode *inode,	/* The file's inode */
	       struct file *file,	/* The file itself */
	       const char *buf,	/* The buffer with the input */
	       int length)	/* The buffer's length */
#endif
{
  int n;
  char c;
  struct timeval tv;
  
  /* get prefix "1" or "2" */
  copy_from_user (&c, buf, 1);
  length--;
  if (c == '1' || c == '2')
    {
      n = c - '1';
      /* Put the input into Message[n], where module_output 
       * will later be able to use it */
      if (length > MESSAGE_LENGTH - 1)
	length = MESSAGE_LENGTH - 1;
      copy_from_user (message[n].text, buf+1, length);
      message[n].text[length] = '\0';
      /* remember connect time */
      if (strstr(message[n].text, Connect)) {
	get_fast_time(&tv);
	message[n].start = tv.tv_sec;
      }
      return length + 1;
    }
  return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static int 
write_func(
	struct file *file,	/* ignored */
	const char *buffer,	/* data */
	unsigned long count,	/* len */
	void *data)		/* ignored */
{
	return module_input(file, buffer, count, 0);
}	
#endif	

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static struct proc_dir_entry *modilp_file;

static int __init init_modilp(void)
{
	modilp_file = create_proc_entry("isdnlog", 0644, NULL);
	if (modilp_file == NULL)
		return -ENOMEM;
	modilp_file->read_proc = read_func; 	
	modilp_file->write_proc = write_func; 	
	modilp_file->owner = THIS_MODULE;
	return 0;
}

static void __exit exit_modilp(void) 
{
	remove_proc_entry("isdnlog", NULL);
}

module_init(init_modilp);
module_exit(exit_modilp);
MODULE_AUTHOR("Leopold Totsch");
MODULE_DESCRIPTION("line status for isdnlog");
MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

#else

/* This function decides whether to allow an operation 
 * (return zero) or not allow it (return a non-zero 
 * which indicates why it is not allowed).
 *
 * The operation can be one of the following values:
 * 0 - Execute (run the "file" - meaningless in our case)
 * 2 - Write (input to the kernel module)
 * 4 - Read (output from the kernel module)
 *
 * This is the real function that checks file 
 * permissions. The permissions returned by ls -l are 
 * for referece only, and can be overridden here. 
 */
static int 
module_permission (struct inode *inode, int op)
{
  /* We allow everybody to read from our module, but 
   * only root (uid 0) may write to it */
  if (op == 4 || (op == 2 && current->euid == 0))
    return 0;

  /* If it's anything else, access is denied */
  return -EACCES;
}


/* The file is opened - we don't really care about 
 * that, but it does mean we need to increment the 
 * module's reference count. */
int 
module_open (struct inode *inode, struct file *file)
{
  MOD_INC_USE_COUNT;

  return 0;
}

/* The file is closed - again, interesting only because 
 * of the reference count. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int 
module_close (struct inode *inode, struct file *file)
#else
void 
module_close (struct inode *inode, struct file *file)
#endif
{
  MOD_DEC_USE_COUNT;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
  return 0;			/* success */
#endif
}


/* Structures to register as the /proc file, with 
 * pointers to all the relevant functions. ********** */


/* File operations for our proc file. This is where we 
 * place pointers to all the functions called when 
 * somebody tries to do something to our file. NULL 
 * means we don't want to deal with something. */
static struct file_operations File_Ops_4_Our_Proc_File =
{
  NULL,				/* lseek */
  module_output,		/* "read" from the file */
  module_input,			/* "write" to the file */
  NULL,				/* readdir */
  NULL,				/* select */
  NULL,				/* ioctl */
  NULL,				/* mmap */
  module_open,			/* Somebody opened the file */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
  NULL,				/* flush, added here in version 2.2 */
#endif
  module_close,			/* Somebody closed the file */
    /* etc. etc. etc. (they are all given in 
     * /usr/include/linux/fs.h). Since we don't put 
     * anything here, the system will keep the default
     * data, which in Unix is zeros (NULLs when taken as 
     * pointers). */
};


/* Inode operations for our proc file. We need it so 
 * we'll have some place to specify the file operations 
 * structure we want to use, and the function we use for 
 * permissions. It's also possible to specify functions 
 * to be called for anything else which could be done to 
 * an inode (although we don't bother, we just put 
 * NULL). */
static struct inode_operations Inode_Ops_4_Our_Proc_File =
{
  &File_Ops_4_Our_Proc_File,
  NULL,				/* create */
  NULL,				/* lookup */
  NULL,				/* link */
  NULL,				/* unlink */
  NULL,				/* symlink */
  NULL,				/* mkdir */
  NULL,				/* rmdir */
  NULL,				/* mknod */
  NULL,				/* rename */
  NULL,				/* readlink */
  NULL,				/* follow_link */
  NULL,				/* readpage */
  NULL,				/* writepage */
  NULL,				/* bmap */
  NULL,				/* truncate */
  module_permission		/* check for permissions */
};


/* Directory entry */
static struct proc_dir_entry Our_Proc_File =
{
  0,				/* Inode number - ignore, it will be filled by 
				 * proc_register[_dynamic] */
  7,				/* Length of the file name */
  "isdnlog",			/* The file name */
  S_IFREG | S_IRUGO | S_IWUSR,
    /* File mode - this is a regular file which 
     * can be read by its owner, its group, and everybody
     * else. Also, its owner can write to it.
     *
     * Actually, this field is just for reference, it's
     * module_permission that does the actual check. It 
     * could use this field, but in our implementation it
     * doesn't, for simplicity. */
  1,				/* Number of links (directories where the 
				 * file is referenced) */
  0, 0,				/* The uid and gid for the file - 
				 * we give it to root */
  80,				/* The size of the file reported by ls. */
  &Inode_Ops_4_Our_Proc_File,
    /* A pointer to the inode structure for
     * the file, if we need it. In our case we
     * do, because we need a write function. */
  NULL
    /* The read function for the file. Irrelevant, 
     * because we put it in the inode structure above */
};



/* Module initialization and cleanup ******************* */

/* Initialize the module - register the proc file */
int 
init_module ()
{
  /* Success if proc_register[_dynamic] is a success, 
   * failure otherwise */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
  /* In version 2.2, proc_register assign a dynamic 
   * inode number automatically if it is zero in the 
   * structure , so there's no more need for 
   * proc_register_dynamic
   */
  return proc_register (&proc_root, &Our_Proc_File);
#else
  return proc_register_dynamic (&proc_root, &Our_Proc_File);
#endif
}


/* Cleanup - unregister our file from /proc */
void 
cleanup_module ()
{
  proc_unregister (&proc_root, Our_Proc_File.low_ino);
}
#endif

