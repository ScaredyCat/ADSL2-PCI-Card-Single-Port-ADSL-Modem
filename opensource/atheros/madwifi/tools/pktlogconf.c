#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include "pktlog_fmt.h"


int pktlog_enable(char *sysctl_name, unsigned filter)
{
    FILE *fp;

    fp = fopen(sysctl_name, "w");

    if (fp != NULL) {
        fprintf(fp, "%i", filter);
        fclose(fp);
        return 0;
    }
    return -1;
}

int 
pktlog_options(char *sysctl_options, unsigned long options)
{
    FILE *fp;

    fp = fopen(sysctl_options, "w");
    if (fp == NULL) 
        return -1;

    fprintf(fp, "%lu", options);
    fclose(fp);
    return 0;
}

int pktlog_size(char *sysctl_size, char *sysctl_enable, int size)
{
    FILE *fp;

    /* Make sure logging is disabled before changing size */
    fp = fopen(sysctl_enable, "w");
    
    if (fp == NULL) {
        fprintf(stderr, "Open failed on enable sysctl\n");
        return -1;
    }
    fprintf(fp, "%i", 0);
    fclose(fp);

    fp = fopen(sysctl_size, "w");

    if (fp != NULL) {
        fprintf(fp, "%i", size);
        fclose(fp);
        return 0;
    }
    return -1;
}

void usage()
{
    fprintf(stderr,
            "Packet log configuration\n"
            "usage: pktlogconf [-a adapter] [-e[event-list]] [-d] [-s log-size] [-t]\n"
            "    -h    show this usage\n"
            "    -a    configures packet logging for specific 'adapter';\n"
            "          configures system-wide logging if this option is\n"
            "          not specified\n"
            "    -d    disable packet logging\n"
            "    -e    enable logging events listed in the 'event-list'\n"
            "          event-list is an optional comma separated list of one or more\n"
            "          of the following: rx tx rcf rcu ani (eg., pktlogdump -e rx,rcu)\n"
            "    -s    change the size of log-buffer to \"log-size\" bytes\n"
            "    -t    enable logging of TCP headers\n");

    exit(-1);
}


main(int argc, char *argv[])
{
    int c;
    int size = -1;
    unsigned long filter = 0, options = 0;
    char fstr[24];
    char ad_name[24];
    char sysctl_size[128];
    char sysctl_enable[128];
    char sysctl_options[128];
    int opt_a=0, opt_d = 0, opt_e = 0, fflag=0;


    for (;;) {
        c = getopt(argc, argv, "s:e::a:dt");

        if (c < 0)
            break;

        switch (c) {
            case 't':
                options |= ATH_PKTLOG_PROTO;
                break;
        case 's':
            size = atoi(optarg);
            break;
        case 'e':
            if (opt_d) { 
                usage();
                exit(-1);
            }
            opt_e = 1;
            if (optarg) {
                fflag = 1;
                strncpy(fstr, optarg, sizeof(fstr));
            }
            break;
        case 'a':
            opt_a = 1;
            strncpy(ad_name, optarg, sizeof(ad_name));
            break;
        case 'd':
            if (opt_e) { 
                usage();
                exit(-1);
            }
            opt_d = 1;
            break;
        default:
            usage();
        }
    }

    if (opt_a) {
        sprintf(sysctl_enable, "/proc/sys/" PKTLOG_PROC_DIR "/%s/enable", ad_name);
        sprintf(sysctl_size, "/proc/sys/" PKTLOG_PROC_DIR "/%s/size", ad_name);
        sprintf(sysctl_options, "/proc/sys/" PKTLOG_PROC_DIR "/%s/options", 
                ad_name);
    } else {
        sprintf(sysctl_enable, "/proc/sys/" PKTLOG_PROC_DIR "/" PKTLOG_PROC_SYSTEM "/enable");
        sprintf(sysctl_size, "/proc/sys/" PKTLOG_PROC_DIR "/" PKTLOG_PROC_SYSTEM "/size");
        sprintf(sysctl_options, "/proc/sys/" PKTLOG_PROC_DIR "/" PKTLOG_PROC_SYSTEM "/options");
    }

    if (opt_d) {
        pktlog_options(sysctl_options, 0);
        pktlog_enable(sysctl_enable, 0);
        return 0;
    }

    if (fflag) {
        if (strstr(fstr, "rx"))
            filter |= ATH_PKTLOG_RX;
        if (strstr(fstr, "tx"))
            filter |= ATH_PKTLOG_TX;
        if (strstr(fstr, "rcf"))
            filter |= ATH_PKTLOG_RCFIND;
        if (strstr(fstr, "rcu"))
            filter |= ATH_PKTLOG_RCUPDATE;
        if (strstr(fstr, "ani"))
            filter |= ATH_PKTLOG_ANI;

        if (filter == 0)
            usage();
    } else {
        filter = ATH_PKTLOG_ANI | ATH_PKTLOG_RCUPDATE | ATH_PKTLOG_RCFIND |
            ATH_PKTLOG_RX | ATH_PKTLOG_TX;
    }

    if (size >= 0)
        if (pktlog_size(sysctl_size, sysctl_enable, size) != 0) {
            fprintf(stderr, "pktlogconf: log size setting failed\n");
            exit(-1);
        }

    if (opt_e) {
        if (pktlog_enable(sysctl_enable, filter) != 0) {
            fprintf(stderr, "pktlogconf: log filter setting failed\n");
            exit(-1);
        }
        if (pktlog_options(sysctl_options, options) != 0) {
            fprintf(stderr, "pktlogconf: options setting failed\n");
            exit(-1);
        }
    }
}
