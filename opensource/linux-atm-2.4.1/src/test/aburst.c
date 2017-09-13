/*
#######################################################################
#
# (C) Copyright 2001
# Alex Zeffertt, Cambridge Broadband Ltd, <EMAIL: PROTECTED>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#######################################################################
# Notes:
# 
# Based loosely on aping by Werner Almesberger
#
# run without args to get usage.
#
#
#######################################################################
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <atm.h>


#define DFLT_MAX_IN_AIR 10
#define MIN_DATALEN 4 // because of the 4 byte sequence number

static int parse_cmdline(int argc,char **argv, int *aal,
                         int *cbr, int *pcr, int *datalen, int *random__,
int *ping, int *max_in_air,
                         struct sockaddr_atmpvc *addr)
{
    int i;
    int got_traf_type = 0;
    int got_datalen = 0;
    int got_addr = 0;
    int got_random = 0;
    int got_pcr = 0;
    int got_ping = 0;
    int got_aal = 0;
    int got_max_in_air = 0;

    /* NOTE: the order in which these parameters are retrieved is IMPORTANT */

    /* Get address of PVC */
    for (i = 1; i < argc-1; i++)
    {
        if (strcmp("--addr", argv[i]) == 0)
        {
            memset(addr,0,sizeof(*addr));
            if (text2atm(argv[i+1],(struct sockaddr *) addr,sizeof(*addr),
T2A_PVC | T2A_UNSPEC | T2A_WILDCARD) < 0)
            {
                got_addr = 0;
                break;
            }
            else
            {
                got_addr = 1;
                break;
            }
        }
    }
    if (!got_addr)
        goto usage;

    /* Get traffic type */
    for (i = 1; i < argc-1; i++)
    {
        if (strcmp("--tt", argv[i]) == 0)
        {
            if (strcmp("cbr", argv[i+1]) == 0)
            {
                *cbr = 1;
                got_traf_type = 1;
                break;
            }
            else if (strcmp("ubr", argv[i+1]) == 0)
            {
                *cbr = 0;
                got_traf_type = 1;
                break;
            }
            else
            {
                got_traf_type = 0;
                break;
            }
        }
    }
    if (!got_traf_type)
        goto usage;

    /* Get aal */
    for (i = 1; i < argc-1; i++)
    {
        if (strcmp("--aal", argv[i]) == 0)
        {
            *aal = atoi(argv[i+1]);
            if (*aal == 0 || *aal == 5)
                got_aal = 1;
            break;
        }
    }
    if (!got_aal)
        goto usage;
   

    /* Get datalen */
    for (i = 1; i < argc-1; i++)
    {
        if (strcmp("--datalen", argv[i]) == 0)
        {
            *datalen = atoi(argv[i+1]);
            got_datalen = 1;
            break;
        }
    }
    if ((*aal == 5) && (!got_datalen))
        goto usage;

    /* Get data pattern */
    for (i = 1; i < argc; i++)
    {
        if (strcmp("--random", argv[i]) == 0)
        {
            *random__ = 1;
            got_random = 1;
        }
    }
    if (!got_random)
        *random__ = 0;
    
    /* Get mode */
    for (i = 1; i < argc; i++)
    {
        if (strcmp("--ping", argv[i]) == 0)
        {
            *ping = 1;
            got_ping = 1;
        }
    }
    if (!got_ping)
        *ping = 0;
    
    /* Get PCR */
    for (i = 1; i < argc-1; i++)
    {
        if (strcmp("--pcr", argv[i]) == 0)
        {
            *pcr = atoi(argv[i+1]);
            got_pcr = 1;
            break;
        }
    }
    if (*cbr && (!got_pcr))
        goto usage;

    /* Get max frames in air */
    for (i = 1; i < argc-1; i++)
    {
        if (strcmp("--max-in-air", argv[i]) == 0)
        {
            *max_in_air = atoi(argv[i+1]);
            got_max_in_air = 1;
            break;
        }
    }
    if(!got_max_in_air)
        *max_in_air = DFLT_MAX_IN_AIR;
    
    return 0;

 usage:
    fprintf(stderr,
            "USAGE:\n"
            "\t%s [--ping] --addr itf.vpi.vci\n"
            "\t--tt {ubr|cbr} --aal {0|5} [--datalen datalen]\n"
            "\t[--pcr pcr] [--random] [--max-in-air nframes];\n"
            "\n"
            "\tNotes:\n"
            "\t\tpcr is required if cbr is selected\n"
            "\t\tdatalen is required if aal 5 is selected\n", argv[0]);

    return -1;
}

int main(int argc,char **argv)
{
    struct timeval timeout;
    struct atm_qos qos;
    int sent = 0;
    int recvd = 0;
    int lost = 0;
    int s;
    char *rxbuf = NULL;
    char *txbuf = NULL;
    int i;
    fd_set set;

    /* Get these from command line */
    struct sockaddr_atmpvc addr;
    int cbr = 0;                 /* 1 to use cbr; 0 to use ubr */
    int pcr = 0;                 /* pcr (only required if cbr == 1) */
    int datalen = 0;             /* length of aal5 data */
    int random__ = 0;            /* 1 to randomise data on per packet basis */
    int ping = 0;                /* 1 to run as aping; 0 to just transmit */
    int aal = 0;
    int max_in_air = 0;          /* value of sent-recv before we do a read() */
    

    int seq_next_send = 0;      /* The sequence number we send in next cell/frame */
    int seq_next_recv = 0;      /* The sequence number we expect in next cell/frame */

    int had_timeout = 0;        /* gets set when timeout occurs */

    /* Parse command line */
    if (parse_cmdline(argc, argv, &aal, &cbr, &pcr, &datalen, &random__,
&ping, &max_in_air, &addr))
        exit(1);

    /* Modify datalen if aal0 */
    if (aal == 0)
        datalen = 52;
    else
        datalen = (datalen > MIN_DATALEN) ? datalen : MIN_DATALEN;

    if ((s = socket(PF_ATMPVC,SOCK_DGRAM,0)) < 0)
    {
        perror("socket");
        return 1;
    }

    /* Set up QoS */
    memset(&qos,0,sizeof(qos));
    qos.aal = (aal == 0)?ATM_AAL0:ATM_AAL5;

    if (cbr)
    {
        qos.txtp.traffic_class = ATM_CBR;
        qos.rxtp.traffic_class = ATM_CBR;
        qos.txtp.max_pcr = pcr;
    }
    else 
    {
        /* UBR */
        qos.txtp.traffic_class = ATM_UBR;
        qos.rxtp.traffic_class = ATM_UBR;
    }
    memcpy(&qos.rxtp, &qos.txtp, sizeof(qos.rxtp));
    
    if (setsockopt(s,SOL_ATM,SO_ATMQOS,&qos,sizeof(qos)) < 0)
    {
        perror("setsockopt SO_ATMQOS");
        return 1;
    }

    /* Set up address */
    if (bind(s,(struct sockaddr *) &addr,sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }
    

    /* set up rx and tx buffs */
    //txbuf = malloc(datalen );
	txbuf = malloc(datalen + 10); // Neeraj


    if (!txbuf)
    {
        perror("malloc");
        return 1;
    }
    if (ping)
    {
        //rxbuf = malloc(datalen+1);
	rxbuf = malloc(datalen+1+10);// Neeraj to adjust 4 bytes
        if (!rxbuf)
        {
            perror("malloc");
            free(txbuf);
            return 1;
        }
    }

    /* initialise tx data (redundant if random__ == 1) */
    if (aal == 5)
    {
        for (i = 4; i < datalen; i++)      /* skip seq */
            txbuf[i] = (char)((i-4)&0xff);
    }
    else
    {
        txbuf[3] = 0xff & (addr.sap_addr.vci<4);
        txbuf[2] = 0xff & (addr.sap_addr.vci>4);
        txbuf[1] = (0xff & (addr.sap_addr.vci>12)) | (0xff & (addr.sap_addr.vpi<4));
        txbuf[0] = 0xff & (addr.sap_addr.vpi>4);
        for (i = 8; i < datalen; i++)      /* skip seq and header */
            txbuf[i] = (char)((i-8)&0xff);
    }

    /* seed the random generator if we're using random data */
    if(random__)
        srand((int)time(NULL));

    while (ping)
    {
        /* We're in ping mode */
        /* Keep max_in_air frames in the air at all times */
        if (sent > recvd + lost)
        {
            int print = 0;  /* do we print out now? */
            int seq;        /* read from payload */

            FD_ZERO(&set);
            FD_SET(s,&set);
         
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;
                
            select(s+1,&set,NULL,NULL,&timeout);
                
            if (FD_ISSET(s,&set)) 
            {
                int size = read(s,rxbuf,datalen+1+10);//Neeraj
                if (size < 0)
                {
                    perror("read");
                    return 1;
                }
                if (size != datalen)
                {
                    fprintf(stderr,"bad RX (%d)\n",size);
		    printf("\n szie != datalen size is %d and data len is %d",size,datalen);
                    if (size > 1)

                        return 1;
                }

                /* retrieve sequence number from payload */
                seq = (aal == 0) ? (*(int*)&rxbuf[4]) : (*(int*)&rxbuf[0]);

                /* did we lose any cells? */
                if (seq == seq_next_recv)
                {
                    
		    /* no */
                    recvd++;
                    print = ((recvd & (1024-1)) == 0); /* print status
once every 1024 receptions */
                }
                else
                {
			printf("\n seq is %d and seq_next_recv is %d",seq,seq_next_recv);
                    /* yes */
                    recvd++;
                    lost += seq - seq_next_recv;
                    seq_next_recv = seq;
                    print = 1;
                }
                seq_next_recv++;

                /* printout values */
                if (print)
                {
                    static char buf[80];
                    if (aal == 5)
                        sprintf(buf, "\rframes recvd=0x%x lost=0x%x", recvd, lost);
                    else
                        sprintf(buf, "\rcells recvd=0x%x lost=0x%x", recvd,
lost);                        
                    write(1,buf,strlen(buf));
                }
            }
            else
            {
                static int recvd_old = 0;
                static char buf[80];
                static int timeouts = 0;
                
                /* tell xmitter we've had a timeout */
                had_timeout = 1;
                
                /* Have we recvd anything since we were last here */
                if (recvd_old == recvd)
                    sprintf(buf, "\rrx timeouts=%d", ++timeouts); /* No new line */
                else
                {
                    timeouts = 1;
                    sprintf(buf, "\nrx timeouts=%d", timeouts); /* Use new line */
                }
                write(1,buf,strlen(buf));
                recvd_old = recvd;
            }
        }

        while ((sent < max_in_air + recvd + lost) || had_timeout)
        {
            /* create the data to send */
            if (random__)
            {
                char *ptr = &txbuf[4]; /* skip seq */
                char *end = &(txbuf[datalen]);
                if (aal == 0)
                    ptr += 4;          /* skip header */
                while (ptr < end)
                    *ptr++ = (char) rand();
            }

            /* write seq */
            *(int *)&txbuf[(aal == 0)?4:0] = seq_next_send++;

            if (write(s,txbuf,datalen) != datalen)
            {
                printf("write error");
                perror("");
                sleep(1);
            }
            else
            {
                sent++;
            }

            had_timeout = 0;
        }
    }

    while (!ping)
    {
        /* We're not in ping mode, so just send as frequently 
         * as possible and keep the user updated with the amount of 
         * frames sent
         */
        /* create the data to send */
        if (random__)
        {
            char *ptr = &txbuf[4]; /* skip seq */
            char *end = &(txbuf[datalen]);
            if (aal == 0)
                ptr += 4;          /* skip header */
            while (ptr < end)
                *ptr++ = (char) rand();
        }
        
        /* write seq */
        *(int *)&txbuf[(aal == 0)?4:0] = seq_next_send++;

        if (write(s,txbuf,datalen) != datalen)
        {
            printf("write error");
            perror("");
            sleep(1);
        }
        else
        {
            sent++;
        }

        /* print status once every 1024 transmissions */
        if ((sent & (1024-1)) == 0)
        {
            static char buf[80];
            if (aal == 5)
                sprintf(buf, "\rframes sent=%08x", sent);
            else
                sprintf(buf, "\rcells sent=%08x", sent);                    
            write(1,buf,strlen(buf));
        }
    }

    return 0;
}
