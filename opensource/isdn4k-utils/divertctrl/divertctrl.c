/* 
 * $Id: divertctrl.c,v 1.5 2002/07/04 10:36:54 paul Exp $
 *
 * Control program for the dss1 diversion supplementary services. (User side)
 *
 * Copyright 1999       by Werner Cornelius (werner@ikt.de)
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
 * $Log: divertctrl.c,v $
 * Revision 1.5  2002/07/04 10:36:54  paul
 * Include stdlib.h for prototypes;
 * Make some things depend on NETWORK_DIAL define.
 *
 * Revision 1.4  2001/06/10 17:38:25  werner
 *
 * Updated tool and manpage with new remote dial feature
 *
 * Revision 1.3  2001/01/09 19:27:55  werner
 *
 * Added Output for interrogate and description for services.
 * Thanks for help from Wolfram Joost.
 *
 * Revision 1.2  1999/09/02 13:24:14  paul
 * fixed some compile warnings
 *
 * Revision 1.1  1999/05/07 21:33:01  werner
 * Initial release of divertctrl
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "isdn_divert.h"

static char number_types[7] = {
       'U',
       'I',
       'N',
       'S',
       'M',
       '?',
       'A' };

static char cf_types[4][5] = {
        "CFU", "CFB", "CFNR", "?" };

static struct {
       unsigned   type;
       char       *name;
} service_types[18] = {
       {  0, "all services" },
       {  1, "speech" },
       {  2, "unrestricted digital information" },
       {  3, "audio 3.1 kHz" },
       {  4, "unrestricted digital information wtaa" },
       {  5, "multirate" },
       { 32, "telephony 3.1 kHz" },
       { 33, "teletex" },
       { 34, "telefax group 4 class 1" },
       { 35, "videotex syntax based" },
       { 36, "videotelephony" },
       { 37, "telefax group 2/3" },
       { 38, "telephony 7 kHz" },
       { 39, "eurofiletransfer" },
       { 40, "filetransfer and access management" },
       { 41, "videoconference" },
       { 42, "audio graphic conference" } };

char *progname; /* name of program */ 
ulong  drvid; /* ids of driver */
int  fd; /* file descriptor for read and ioctl */
divert_ioctl dioctl; /* structure exchanging ioctl data */
char **argp; /* pointer to actual parameter */
int argrest; /* remaining args */
int waitreq; /* wait requested */
int exitcode;/* exitcode for program */

/****************************/
/* print a short usage help */
/****************************/
void usage(void)
{ close(fd);
  fprintf(stderr,"usage: %s [wait] <Command> [parms]\n\n",progname);
  fprintf(stderr,"[wait]    : wait optional keyword until -> wait until command executed \n");
  fprintf(stderr,"<Command> : desired command to execute\n");
  fprintf(stderr,"   activate    <drivers> <cfu,cfb,cfnr> <msn> <service/0> <dest_no>\n");  
  fprintf(stderr,"   deactivate  <drivers> <cfu,cfb,cfnr> <msn> <service/0> \n");  
  fprintf(stderr,"   interrogate <drivers> <cfu,cfb,cfnr> <msn> <service/0> \n");  
  fprintf(stderr,"   listrules   <drivers> -> list all rules for drivers\n");
  fprintf(stderr,"   ignore      <callid> [uus1_string] -> ignore call with callid/send uus1\n");
  fprintf(stderr,"   alert       <callid> [uus1_string] -> alert call with callid/send uus1\n");
  fprintf(stderr,"   deflect     <callid> [to_nr] -> deflect call with callid to to_nr \n");
  fprintf(stderr,"   appendrule  <drivers> -> append a rule at list end\n");
  fprintf(stderr,"   insertrule  <drivers> -> insert a rule at list start\n");
  fprintf(stderr,"   flushrules  [drivers] -> flush all rules for specified drivers\n");
  fprintf(stderr,"   append- and insertrule take the following arguments:\n");
#ifdef NETWORK_DIAL
  fprintf(stderr,"   <drivers> <action> [msn] [si1] [si2] [caller] [screen] [delay] [callopt] [destnr/if]\n");
#else
  fprintf(stderr,"   <drivers> <action> [msn] [si1] [si2] [caller] [screen] [delay] [callopt] [destnr]\n");
#endif
  fprintf(stderr,"   drivers -> driver names for D-chans, %% separated, - = all D-chan\n"); 
  fprintf(stderr,"   action -> 0/ignore 1/report 2/proceed 3/alert 4/reject\n");
  fprintf(stderr,"   msn -> - = wildcard or explicit selected msn.subaddress\n");
  fprintf(stderr,"   when action = 5 then if specifies a network interface to dial\n");
  fprintf(stderr,"   si1 -> 0 = all services, else bitmask for services 1..7\n");
  fprintf(stderr,"   si2 -> 0 = ignore, else specified value\n");
  fprintf(stderr,"   caller -> - = wildcard, 0 = unknown, else number.subaddress\n");
  fprintf(stderr,"             a - trailing number digits is handled as wildcard\n");
  fprintf(stderr,"   screen -> 0 = noscreen 1 = divertonly 2 = divertto\n");
  fprintf(stderr,"   delay -> time in seconds until hangup or divert in auto mode\n");
  fprintf(stderr,"   callopt -> 0 = all calls, 1 = only non waiting, 2 = only waiting\n");
  fprintf(stderr,"   destnr -> destination number and subaddress\n");
  fprintf(stderr,"   defaults: allmsn, si1=0, si2=0, all callers, noscreen, delay=10\n");
  fprintf(stderr,"\n");  
  exit(-1);
} /* usage */


/******************************************/
/* get the driver parameter and set drvid */
/******************************************/
void getdrvid(void)
{ char *p,*p1;

  argp++; argrest--;

  if (argrest < 1) 
   { drvid = DEFLECT_ALL_IDS;
     return;
   }

  if ((*argp)[0] == '-')
    drvid = DEFLECT_ALL_IDS;
   else
    { drvid = 0; /* first no device */
      p = *argp; 
      while (p)
       { p1 = p;
         p = strchr(p1,'%');
         if (p) *p++ = '\0'; 
         strcpy(dioctl.getid.drvnam,p1); /* set string to query */
         if (ioctl(fd,IIOCGETDRV,&dioctl))
          { fprintf(stderr,"driver %s not found \n",p1);
            close(fd);
            exit(-1);
          } 
         drvid |= (1L << dioctl.getid.drvid);
       }
    }
  argp++; argrest--;
} /* getdrvid */ 

/***********************************/
/* set the rule pars from cmd line */
/***********************************/
void setrulepar(void)
{ divert_rule *dr = &dioctl.getsetrule.rule;
  unsigned u;

  /* first set the defaults */
  dr->drvid = 0; /* no driver */
  dr->action= DEFLECT_IGNORE; 
  strcpy(dr->my_msn,"0");
  dr->si1   = 0;
  dr->si2   = 0;
  strcpy(dr->caller,"-");
  dr->screen= 0; /* no screening */
  dr->waittime = 0;
  dr->callopt = 0;
  dr->to_nr[0] = '\0';

  if (!argrest) return; 

  /* action */
  if ((sscanf(*(argp),"%u",&u) <= 0) || (u > 5))
   { fprintf(stderr,"invalid action value %s\n",*argp);
     usage();
   }
  dr->action = u & 0xFF;
#ifdef NETWORK_DIAL
  if (((dr->action == DEFLECT_ALERT) || (dr->action == NETWORK_DIAL)) && (argrest != 9))
#else
  if ((dr->action == DEFLECT_ALERT) && (argrest != 9))
#endif
   { fprintf(stderr,"alerting/dialing action must be supplied with all parms\n");
     usage();
   }    
  if (!--argrest) return;

  /* copy msn */
  strcpy(dr->my_msn, *(++argp));
  if (!--argrest) return; 

  /* si1 */
  if (sscanf(*(++argp),"%u",&u) <= 0)
   { fprintf(stderr,"invalid si1 value %s\n",*argp);
     usage();
   }
  dr->si1 = u & 0xFF;   
  if (!--argrest) return;
  
  /* si2 */
  if (sscanf(*(++argp),"%u",&u) <= 0)
   { fprintf(stderr,"invalid si2 value %s\n",*argp);
     usage();
   }
  dr->si2 = u & 0xFF;   
  if (!--argrest) return;
  
  /* caller */
  strcpy(dr->caller,*(++argp));
  if (!--argrest) return;
  
  /* screen */
  if ((sscanf(*(++argp),"%u",&u) <= 0) || (u > 2))
   { fprintf(stderr,"invalid screen value %s\n",*argp);
     usage();
   }
  dr->screen = u & 0xFF;   
  if (!--argrest) return;

  /* delay */
  if ((sscanf(*(++argp),"%u",&u) <= 0) || (u > 99))
   { fprintf(stderr,"invalid delay value %s\n",*argp);
     usage();
   }
  dr->waittime = u & 0xFF;   
  if (!--argrest) return;

  /* callopt */
  if ((sscanf(*(++argp),"%u",&u) <= 0) || (u > 2))
   { fprintf(stderr,"invalid callopt value %s\n",*argp);
     usage();
   }
  dr->callopt = u & 0xFF;   
  if (!--argrest) return;

  /* to_nr */
  strcpy(dr->to_nr,*(++argp));
  if (!--argrest) return;  
  
  fprintf(stderr,"to many cmd line parms\n");
  usage();
} /* setrulepar */

/************************************************/
/* getcallid fetches the callid for id commands */
/************************************************/
void getcallid(void)
{
  argp++; argrest--;

  if (argrest < 1) 
   { fprintf(stderr,"callid missing\n");
     usage();  
   }

  if (sscanf(*argp,"%lu",&dioctl.fwd_ctrl.callid) != 1)
    { fprintf(stderr,"invalid callid value %s\n",*argp);
      usage();
    }
  argp++; argrest--;
} /* getcallid */

/***************************************************************************/
/* getcfpar fetches the parms for de/activating and interrogating services */
/***************************************************************************/
#define CFP dioctl.cf_ctrl
void getcfpar(void)
{ unsigned u;

  /* first set the defaults */
  CFP.service = 0; /* all services */
  CFP.msn[0] = '\0'; /* all msns */
  CFP.fwd_nr[0] = '\0';
  
  if (!argrest)
   { fprintf(stderr,"no procedure specified \n");
     usage();
   } 
  if (!strcmp(*argp,"cfu")) 
    CFP.cfproc = 0;
     else 
      if (!strcmp(*argp,"cfb"))
        CFP.cfproc = 1;
         else
          if (!strcmp(*argp,"cfnr"))
            CFP.cfproc = 2;
             else
	      { fprintf(stderr,"invalid procedure %s\n",*argp);
                usage();
              }
  if (waitreq) CFP.cfproc |= 0x80;
  argp++; argrest--; 

  strcpy(CFP.msn,*(argp++));
  if (!(--argrest)) return;

  if ((sscanf(*argp,"%u",&u) != 1) ||
      (u > 255))
   { fprintf(stderr,"invalid service [0-255] %s\n",*argp);
     usage();
   }    
  CFP.service = u & 0xFF;
  argrest--; argp++;
} /* getcfpar */ 
#undef CFP

/********************/
/* execute commands */
/********************/
static void do_command(void)
{ divert_rule *dr = &dioctl.getsetrule.rule;
  divert_ioctl ioc;
  int i;
  ulong u,cmd;
  fd_set rdfs;
  struct timeval tv;
  char read_buf[512];
  ssize_t read_len;
  char *read_ptr;
  ulong read_procid;
  ulong read_counter;
  char read_driver[30];
  u_long read_msn_type;
  char read_msn[35];
  u_long read_cfproc;
  u_long read_service;
  u_long read_tonr_type;
  char read_tonr[35];
  int read_char_count;
  char read_end;
  char read_found;

  if (!strcmp(*argp,"listrules"))
    { getdrvid();
      dioctl.getsetrule.ruleidx = 0; /* get first rule */ 
      while (1)
       { /* fetch all rules */
         if (ioctl(fd,IIOCGETRULE,&dioctl)) return; /* no more rules */
         i = 0; /* first interface */
         dr->drvid = drvid & dr->drvid; /* only requested bits */ 
         while (dr->drvid)  
	  { if (!(dr->drvid & 1))
             { dr->drvid >>= 1; /* next entry */
               i++;
               continue;
             } 
            if (dr->drvid == DEFLECT_ALL_IDS)
             { printf("%10s","-"); /* all if */
               dr->drvid = 0; /* no further output */
             }  
             else
              { ioc.getid.drvid = i++; /* set para */
                dr->drvid >>= 1;
                if (ioctl(fd,IIOCGETNAM,&ioc)) continue; /* invalid driver */
                if (!ioc.getid.drvnam[0]) continue; /* no name */
                printf("%10s ",ioc.getid.drvnam);
              }
            printf("%15s %20s %2d %3d %d %2d %d %s\n",
                   dr->my_msn,  /* msn */ 
                   dr->caller,  /* caller id */
                   dr->si1,     /* si1 0 = all */
                   dr->si2,     /* si2 0 = all */
                   dr->action,  /* desired action */
                   dr->waittime,/* wait until action */
                   dr->callopt, /* option for waiting calls */
                   dr->to_nr);  /* deflect to */
          } /* interface match */ 
         dioctl.getsetrule.ruleidx++; /* next rule */ 
       } /* fetch all rules */
    } /* listrules */
  else
  if (!strcmp(*argp,"deflect"))
    { getcallid();
      dioctl.fwd_ctrl.subcmd = 2;
      if (argrest)
       { strcpy(dioctl.fwd_ctrl.to_nr,*argp);
         argp++; argrest--;
       }
      else
         dioctl.fwd_ctrl.to_nr[0] = '\0';
      exitcode = ioctl(fd,IIOCDODFACT,&dioctl);
    } /* deflect */
  else
  if (!strcmp(*argp,"alert"))
    { getcallid();
      dioctl.fwd_ctrl.subcmd = 1;
      if (argrest)
       { strcpy(dioctl.fwd_ctrl.to_nr,*argp);
         argp++; argrest--;
       }
      else
         dioctl.fwd_ctrl.to_nr[0] = '\0';
      exitcode = ioctl(fd,IIOCDODFACT,&dioctl);
    } /* alert */
  else
  if (!strcmp(*argp,"ignore"))
    { getcallid();
      dioctl.fwd_ctrl.subcmd = 0;
      if (argrest)
       { strcpy(dioctl.fwd_ctrl.to_nr,*argp);
         argp++; argrest--;
       }
      else
         dioctl.fwd_ctrl.to_nr[0] = '\0';
      exitcode = ioctl(fd,IIOCDODFACT,&dioctl);
    } /* ignore */
  else
  if (!strcmp(*argp,"insertrule"))
    { getdrvid();
      setrulepar();
      dioctl.getsetrule.ruleidx = 0; /* start of list */
      dr->drvid = drvid; /* driver id */
      ioctl(fd,IIOCINSRULE,&dioctl);
    } /* insertrule */
  else
  if (!strcmp(*argp,"appendrule"))
    { getdrvid();
      setrulepar();
      dioctl.getsetrule.ruleidx = -1; /* end of list */
      dr->drvid = drvid; /* driver id */
      ioctl(fd,IIOCINSRULE,&dioctl);
    } /* appendrule */
  else
  if (!strcmp(*argp,"flushrules"))
    { getdrvid(); 
      dioctl.getsetrule.ruleidx = 0; /* start of list */
      while (1)
       { /* fetch all rules */
         if (ioctl(fd,IIOCGETRULE,&dioctl)) return; /* no more rules */
         if (drvid & dr->drvid)
	  { dr->drvid &= ~drvid; /* delete drivers */
            if (dr->drvid)
	     { ioctl(fd,IIOCMODRULE,&dioctl);
               dioctl.getsetrule.ruleidx++;
             }       
            else
             ioctl(fd,IIOCDELRULE,&dioctl); 
          } 
         else
          dioctl.getsetrule.ruleidx++;
       } /* fetch all rules */
    } /* flushrules */
  else
  if ((!strcmp(*argp,"activate")) || (!strcmp(*argp,"deactivate"))
      || (!strcmp(*argp,"interrogate")))
    { if (*argp[0] == 'a') 
        cmd = IIOCDOCFACT;
       else if (*argp[0] == 'i')
         cmd = IIOCDOCFINT; 
       else
         cmd = IIOCDOCFDIS; 
      getdrvid();
      getcfpar(); 
      dioctl.cf_ctrl.fwd_nr[0] = '\0';
      if (cmd == IIOCDOCFACT)
       { if (!argrest)
          { fprintf(stderr,"missing diverted to number \n");
            usage();
          }
         strcpy(dioctl.cf_ctrl.fwd_nr,*(argp++));
         argrest--;
       } /* only activate */
      dioctl.cf_ctrl.drvid = 0;
      u = drvid; /* list of drivers */
      while (u)
       { if (u & 1)
	 { if ((exitcode = ioctl(fd,cmd,&dioctl)) > 0) 
              return; /* error executing diversion */
            else
             if ((exitcode < 0) && (drvid != DEFLECT_ALL_IDS))
               return;
                        read_end = 0;
            read_found = 0;
            while ( !read_end && (cmd == IIOCDOCFINT) )
             { FD_ZERO(&rdfs);
               FD_SET(fd,&rdfs);
               tv.tv_sec = 4;
               tv.tv_usec = 0;
               if (select(fd+1,&rdfs,NULL,NULL,&tv) != 1)
                { break;
                }
               read_len = read(fd,read_buf,sizeof(read_buf)-1);
               if (read_len)
               { read_buf[read_len] = 0;
                 read_ptr = read_buf;
                 while (*read_ptr)
                  { if ( ((*read_ptr != (DIVERT_REPORT | 0x30)) ||
                        (read_ptr[1] != ' ')) && strncmp(read_ptr,"128 ",4) )
                    { while (*read_ptr && (*read_ptr != '\r'))
                        read_ptr++;
                      if (*read_ptr)
                        read_ptr++;
                      continue;
                    }
                    if ( !strncmp(read_ptr,"128 ",4)) {
                       sscanf(read_ptr + 4,"%lx",&read_procid);
                       if (read_procid == dioctl.cf_ctrl.procid) {
                          puts("Error. :-(");
                          read_end = 1;
                          break;
                       }
                    }
                    else {
                       read_ptr += 2;
                       sscanf(read_ptr,"%lx %lu %s %lu %s %lx %lu %n",
                              &read_procid,&read_counter,read_driver,
                              &read_msn_type,read_msn,&read_service,&read_cfproc,
                              &read_char_count);
                       read_ptr += read_char_count;
                       read_end = (read_procid == dioctl.cf_ctrl.procid) && !read_counter;
                       if (read_end)
                        { if (!read_found)
                            puts("Nothing active.");
                          break;
                        }
                       if ( read_procid == dioctl.cf_ctrl.procid )
                        { sscanf(read_ptr,"%lu %s",&read_tonr_type,read_tonr);
                          if (read_msn_type > 6)
                             read_msn_type = 5;
                          if (read_tonr_type > 6)
                             read_tonr_type = 5;
                          if (read_cfproc > 3)
                             read_cfproc = 3;
                          read_found = 1;
                          printf("%-8s %c %-8s %-4s %c %-10s ",read_driver,
                                 number_types[read_msn_type],read_msn,
                                 cf_types[read_cfproc],
                                 number_types[read_tonr_type],read_tonr);
                          for (read_char_count = 0; read_char_count < 18; read_char_count++)
                           { if (service_types[read_char_count].type == read_service)
                              { puts(service_types[read_char_count].name);
                                break;
                              }
                           }
                          if (read_char_count == 18)
                             puts("unkown");
                        }
                     }
                     while (*read_ptr && (*read_ptr != '\r'))
                        read_ptr++;
                     if (*read_ptr)
                        read_ptr++;
                 }
               }
             }
            exitcode = 0;
          }
         u >>= 1; /* next driver */  
         dioctl.cf_ctrl.drvid++;    
       } /* for all drivers */
    } /* activate */
  else
    { fprintf(stderr,"unknown command: %s\n",*argp);
      usage();
    }
} /* do_command */


int main(int argc, char *argv[])
{

  exitcode = 0; /* default no error */

  /* first get our own program name */
  if ((progname = strrchr(argv[0], '/')))
    progname++;
  else
    progname = argv[0];

  /* now open the interface */
  if ((fd = open("/proc/net/isdn/divert", O_RDWR)) < 0)
    { if ((fd = open("/dev/isdndivert", O_RDWR)) < 0)
        { fprintf(stderr,"error opening device (dss1_divert module not loaded ?)\n");
          exit(-1);
        } 
    } 
  
  /* check the interface version */
  dioctl.drv_version  = -1; /* invalidate */
  if (ioctl(fd,IIOCGETVER,&dioctl))
    { fprintf(stderr,"could not execute ioctl call \n");
      close(fd);
      exit(-1);
    }  
  if ( dioctl.drv_version != DIVERT_IIOC_VERSION)
    { fprintf(stderr,"versions of %s and driver incompatible\n",progname);
      close(fd);
      exit(-1);
    } 

  argrest = argc - 1; /* remaining number of parameters */
  argp = argv + 1; /* pointer to next parameter/command */

  waitreq = 0; /* if no wait desired */
  if (argrest >= 1)
   if (!strcmp(*argp,"wait"))
    { waitreq = 1; 
      argp++;
      argrest--;
    }  
  /* a minimum of 1 additional cmd line parameters is needed */
  if (argrest < 1) usage();
  
  do_command(); 
  close(fd);
  exit(exitcode);
} /* main */


