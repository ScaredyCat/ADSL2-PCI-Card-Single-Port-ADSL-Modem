#ifndef _PKTLOG_H_
#define _PKTLOG_H_

#include "pktlog_fmt.h"
#include <linux/sysctl.h>
#include "ah_desc.h"

#define PKTLOG_TAG "ATH_PKTLOG" 

#define PKTLOG_MODE_SYSTEM   1
#define PKTLOG_MODE_ADAPTER  2

#define MAX_WLANDEV  10

#define PKTLOG_DEFAULT_BUFSIZE (512 * 1024)

#define PKTLOG_SYSCTL_SIZE      8

/* Permissions for creating proc entries */
#define PKTLOG_PROC_PERM        0444
#define PKTLOG_PROCSYS_DIR_PERM 0555
#define PKTLOG_PROCSYS_PERM     0644

#define PKTLOG_DEVNAME_SIZE 32
/* Packet log state information */
struct ath_pktlog_info {
    struct ath_pktlog_buf *buf;
    u_int32_t log_state;
    u_int32_t saved_state;
    u_int32_t options;
    int32_t buf_size;           /* Size of buffer in bytes */
    struct ctl_table sysctls[PKTLOG_SYSCTL_SIZE];
    struct proc_dir_entry *proc_entry;
    struct ctl_table_header *sysctl_header;
};

/* Parameter types for packet logging driver hooks */

struct log_tx {
    struct ath_desc *firstds;
    struct ath_desc *lastds;
    struct ath_buf *bf;
    int32_t misc[8];            /* Can be used for HT specific or other misc info */
    /* TBD: Add other required information */
};

struct log_rx {
    struct ath_desc *ds;
    struct ath_rx_status *status;
    int32_t misc[8];            /* Can be used for HT specific or other misc info */
    /* TBD: Add other required information */
};

struct ath_pktlog_funcs {
    int (*pktlog_attach) (struct ath_softc *);
    void (*pktlog_detach) (struct ath_softc *);
    void (*pktlog_txctl) (struct ath_softc *, struct log_tx *, u_int16_t);
    void (*pktlog_txstatus) (struct ath_softc *, struct log_tx *, u_int16_t);
    void (*pktlog_rx) (struct ath_softc *, struct log_rx *, u_int16_t);
};

#define ath_log_txctl(_sc, _log_data, _flags)                          \
        do {                                                        \
            if(g_pktlog_funcs) {                                    \
                g_pktlog_funcs->pktlog_txctl(_sc, _log_data, _flags);  \
            }                                                       \
        }while(0)

#define ath_log_txstatus(_sc, _log_data, _flags)                          \
        do {                                                        \
            if(g_pktlog_funcs) {                                    \
                g_pktlog_funcs->pktlog_txstatus(_sc, _log_data, _flags);  \
            }                                                       \
        }while(0)

#define ath_log_rx(_sc, _log_data, _flags)                          \
        do {                                                        \
            if(g_pktlog_funcs) {                                    \
                g_pktlog_funcs->pktlog_rx(_sc, _log_data, _flags);  \
            }                                                       \
        }while(0)

#define ath_pktlog_attach(_sc)                                      \
        do {                                                        \
            if(g_pktlog_funcs) {                                    \
                g_pktlog_funcs->pktlog_attach(_sc);                 \
            }                                                       \
        }while(0)

#define ath_pktlog_detach(_sc)                                      \
        do {                                                        \
            if(g_pktlog_funcs) {                                    \
                g_pktlog_funcs->pktlog_detach(_sc);                 \
            }                                                       \
        }while(0)

#endif                          /* _PKTLOG_H_ */
