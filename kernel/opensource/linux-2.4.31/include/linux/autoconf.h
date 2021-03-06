/*
 * Automatically generated C config: don't edit
 */
#define AUTOCONF_INCLUDED
#define CONFIG_MIPS 1
#define CONFIG_MIPS32 1
#undef  CONFIG_MIPS64

/*
 * Code maturity level options
 */
#define CONFIG_EXPERIMENTAL 1
#define CONFIG_IFX 1

/*
 * Loadable module support
 */
#define CONFIG_MODULES 1
#undef  CONFIG_MODVERSIONS
#undef  CONFIG_KMOD

/*
 * Machine selection
 */
#undef  CONFIG_ACER_PICA_61
#undef  CONFIG_MIPS_BOSPORUS
#undef  CONFIG_MIPS_FICMMP
#undef  CONFIG_MIPS_MIRAGE
#undef  CONFIG_MIPS_DB1000
#undef  CONFIG_MIPS_DB1100
#undef  CONFIG_MIPS_DB1500
#undef  CONFIG_MIPS_DB1550
#undef  CONFIG_MIPS_DB1200
#undef  CONFIG_MIPS_PB1000
#undef  CONFIG_MIPS_PB1100
#undef  CONFIG_MIPS_PB1500
#undef  CONFIG_MIPS_PB1550
#undef  CONFIG_MIPS_PB1200
#undef  CONFIG_MIPS_HYDROGEN3
#undef  CONFIG_MIPS_XXS1500
#undef  CONFIG_MIPS_MTX1
#undef  CONFIG_COGENT_CSB250
#undef  CONFIG_BAGET_MIPS
#undef  CONFIG_CASIO_E55
#undef  CONFIG_MIPS_COBALT
#undef  CONFIG_DECSTATION
#undef  CONFIG_MIPS_EV64120
#undef  CONFIG_MIPS_EV96100
#undef  CONFIG_MIPS_IVR
#undef  CONFIG_HP_LASERJET
#undef  CONFIG_IBM_WORKPAD
#undef  CONFIG_LASAT
#undef  CONFIG_AMAZON
#define CONFIG_DANUBE 1
#undef  CONFIG_USE_EMULATOR
#undef  CONFIG_DANUBE_CHIP_A11
#define CONFIG_DANUBE_PCI 1
#define CONFIG_DANUBE_PCI_HW_SWAP 1
#undef  CONFIG_DANUBE_MPS
#undef  CONFIG_DANUBE_MPS_PROC_DEBUG
#undef  CONFIG_DEBUG_MINI_BOOT

/*
 * Danube Multiplexer Setup
 */
#undef  CONFIG_AMAZON_SE
#undef  CONFIG_MIPS_ITE8172
#undef  CONFIG_MIPS_ATLAS
#undef  CONFIG_MIPS_MAGNUM_4000
#undef  CONFIG_MIPS_MALTA
#undef  CONFIG_MIPS_SEAD
#undef  CONFIG_MOMENCO_OCELOT
#undef  CONFIG_MOMENCO_OCELOT_G
#undef  CONFIG_MOMENCO_OCELOT_C
#undef  CONFIG_MOMENCO_JAGUAR_ATX
#undef  CONFIG_PMC_BIG_SUR
#undef  CONFIG_PMC_STRETCH
#undef  CONFIG_PMC_YOSEMITE
#undef  CONFIG_DDB5074
#undef  CONFIG_DDB5476
#undef  CONFIG_DDB5477
#undef  CONFIG_NEC_OSPREY
#undef  CONFIG_NEC_EAGLE
#undef  CONFIG_OLIVETTI_M700
#undef  CONFIG_NINO
#undef  CONFIG_SGI_IP22
#undef  CONFIG_SGI_IP27
#undef  CONFIG_SIBYTE_SB1xxx_SOC
#undef  CONFIG_SNI_RM200_PCI
#undef  CONFIG_TANBAC_TB0226
#undef  CONFIG_TANBAC_TB0229
#undef  CONFIG_TOSHIBA_JMR3927
#undef  CONFIG_TOSHIBA_RBTX4927
#undef  CONFIG_VICTOR_MPC30X
#undef  CONFIG_ZAO_CAPCELLA
#undef  CONFIG_HIGHMEM
#define CONFIG_RWSEM_GENERIC_SPINLOCK 1
#undef  CONFIG_RWSEM_XCHGADD_ALGORITHM
#define CONFIG_NEW_IRQ 1
#define CONFIG_NONCOHERENT_IO 1
#define CONFIG_NEW_TIME_C 1
#define CONFIG_NONCOHERENT_IO 1
#define CONFIG_PCI 1
#define CONFIG_PCI_AUTO 1
#define CONFIG_SWAP_IO_SPACE_W 1
#define CONFIG_SWAP_IO_SPACE_L 1
#undef  CONFIG_MIPS_AU1000

/*
 * CPU selection
 */
#define CONFIG_CPU_MIPS32 1
#undef  CONFIG_CPU_MIPS64
#undef  CONFIG_CPU_R3000
#undef  CONFIG_CPU_TX39XX
#undef  CONFIG_CPU_VR41XX
#undef  CONFIG_CPU_R4300
#undef  CONFIG_CPU_R4X00
#undef  CONFIG_CPU_TX49XX
#undef  CONFIG_CPU_R5000
#undef  CONFIG_CPU_R5432
#undef  CONFIG_CPU_R6000
#undef  CONFIG_CPU_NEVADA
#undef  CONFIG_CPU_R8000
#undef  CONFIG_CPU_R10000
#undef  CONFIG_CPU_RM7000
#undef  CONFIG_CPU_RM9000
#undef  CONFIG_CPU_SB1
#define CONFIG_PAGE_SIZE_4KB 1
#undef  CONFIG_PAGE_SIZE_16KB
#undef  CONFIG_PAGE_SIZE_64KB
#undef  CONFIG_CPU_HAS_PREFETCH
#undef  CONFIG_VTAG_ICACHE
#undef  CONFIG_64BIT_PHYS_ADDR
#undef  CONFIG_CPU_ADVANCED
#define CONFIG_CPU_HAS_LLSC 1
#undef  CONFIG_CPU_HAS_LLDSCD
#undef  CONFIG_CPU_HAS_WB
#define CONFIG_CPU_HAS_SYNC 1

/*
 * General setup
 */
#undef  CONFIG_CPU_LITTLE_ENDIAN
#undef  CONFIG_BUILD_ELF64
#undef  CONFIG_BINFMT_IRIX
#define CONFIG_NET 1
#undef  CONFIG_NET_IFX_EXTENSION
#define CONFIG_NET_IFX_EXTENSION_MODULE 1
#define CONFIG_PCI 1
#undef  CONFIG_PCI_NEW
#define CONFIG_PCI_AUTO 1
#undef  CONFIG_PCI_NAMES
#undef  CONFIG_ISA
#undef  CONFIG_TC
#undef  CONFIG_MCA
#undef  CONFIG_SBUS
#undef  CONFIG_HOTPLUG
#undef  CONFIG_PCMCIA
#undef  CONFIG_HOTPLUG_PCI
#define CONFIG_SYSVIPC 1
#undef  CONFIG_BSD_PROCESS_ACCT
#define CONFIG_SYSCTL 1
#define CONFIG_MAX_USER_RT_PRIO (100)
#define CONFIG_MAX_RT_PRIO (0)
#define CONFIG_KCORE_ELF 1
#undef  CONFIG_KCORE_AOUT
#undef  CONFIG_BINFMT_AOUT
#define CONFIG_BINFMT_ELF 1
#undef  CONFIG_MIPS32_COMPAT
#undef  CONFIG_MIPS32_O32
#undef  CONFIG_MIPS32_N32
#undef  CONFIG_BINFMT_ELF32
#undef  CONFIG_ILATENCY
#undef  CONFIG_BINFMT_MISC
#undef  CONFIG_OOM_KILLER
#undef  CONFIG_HIGH_RES_TIMERS
#undef  CONFIG_CMDLINE_BOOL
#undef  CONFIG_IKCONFIG
#undef  CONFIG_IKCONFIG_PROC

/*
 * Memory Technology Devices (MTD)
 */
#define CONFIG_MTD 1
#undef  CONFIG_MTD_DEBUG
#define CONFIG_MTD_PARTITIONS 1
#undef  CONFIG_MTD_CONCAT
#undef  CONFIG_MTD_REDBOOT_PARTS
#undef  CONFIG_MTD_CMDLINE_PARTS

/*
 * User Modules And Translation Layers
 */
#define CONFIG_MTD_CHAR 1
#define CONFIG_MTD_BLOCK 1
#undef  CONFIG_FTL
#undef  CONFIG_NFTL
#undef  CONFIG_INFTL

/*
 * RAM/ROM/Flash chip drivers
 */
#define CONFIG_MTD_CFI 1
#undef  CONFIG_MTD_JEDECPROBE
#define CONFIG_MTD_GEN_PROBE 1
#undef  CONFIG_MTD_CFI_ADV_OPTIONS
#define CONFIG_MTD_CFI_INTELEXT 1
#define CONFIG_MTD_CFI_AMDSTD 1
#undef  CONFIG_MTD_CFI_STAA
#undef  CONFIG_MTD_RAM
#undef  CONFIG_MTD_ROM
#undef  CONFIG_MTD_ABSENT
#undef  CONFIG_MTD_OBSOLETE_CHIPS
#undef  CONFIG_MTD_AMDSTD
#undef  CONFIG_MTD_SHARP
#undef  CONFIG_MTD_JEDEC

/*
 * Mapping drivers for chip access
 */
#undef  CONFIG_MTD_PHYSMAP
#undef  CONFIG_MTD_PB1000
#undef  CONFIG_MTD_PB1500
#undef  CONFIG_MTD_PB1100
#undef  CONFIG_MTD_BOSPORUS
#undef  CONFIG_MTD_XXS1500
#undef  CONFIG_MTD_MTX1
#undef  CONFIG_MTD_DB1X00
#undef  CONFIG_MTD_PB1550
#undef  CONFIG_MTD_HYDROGEN3
#undef  CONFIG_MTD_MIRAGE
#undef  CONFIG_MTD_CSTM_MIPS_IXX
#undef  CONFIG_MTD_OCELOT
#undef  CONFIG_MTD_LASAT
#undef  CONFIG_MTD_AMAZON
#define CONFIG_MTD_DANUBE 1
#define CONFIG_MTD_DANUBE_FLASH_SIZE (4)
#undef  CONFIG_MTD_PCI
#undef  CONFIG_MTD_PCMCIA

/*
 * Self-contained MTD device drivers
 */
#undef  CONFIG_MTD_PMC551
#undef  CONFIG_MTD_SLRAM
#undef  CONFIG_MTD_MTDRAM
#undef  CONFIG_MTD_BLKMTD

/*
 * Disk-On-Chip Device Drivers
 */
#undef  CONFIG_MTD_DOC1000
#undef  CONFIG_MTD_DOC2000
#undef  CONFIG_MTD_DOC2001
#undef  CONFIG_MTD_DOCPROBE

/*
 * NAND Flash Device Drivers
 */
#undef  CONFIG_MTD_NAND
#undef  CONFIG_MTD_DANUBE_NAND

/*
 * Parallel port support
 */
#undef  CONFIG_PARPORT

/*
 * Plug and Play configuration
 */
#undef  CONFIG_PNP
#undef  CONFIG_ISAPNP

/*
 * Block devices
 */
#undef  CONFIG_BLK_DEV_FD
#undef  CONFIG_BLK_DEV_XD
#undef  CONFIG_PARIDE
#undef  CONFIG_BLK_CPQ_DA
#undef  CONFIG_BLK_CPQ_CISS_DA
#undef  CONFIG_CISS_SCSI_TAPE
#undef  CONFIG_CISS_MONITOR_THREAD
#undef  CONFIG_BLK_DEV_DAC960
#undef  CONFIG_BLK_DEV_UMEM
#undef  CONFIG_BLK_DEV_SX8
#define CONFIG_BLK_DEV_LOOP 1
#undef  CONFIG_BLK_DEV_NBD
#undef  CONFIG_BLK_DEV_RAM
#undef  CONFIG_BLK_DEV_INITRD
#undef  CONFIG_BLK_STATS

/*
 * Multi-device support (RAID and LVM)
 */
#undef  CONFIG_MD
#undef  CONFIG_BLK_DEV_MD
#undef  CONFIG_MD_LINEAR
#undef  CONFIG_MD_RAID0
#undef  CONFIG_MD_RAID1
#undef  CONFIG_MD_RAID5
#undef  CONFIG_MD_MULTIPATH
#undef  CONFIG_BLK_DEV_LVM

/*
 * Cryptographic options
 */
#undef  CONFIG_CRYPTO

/*
 * Networking options
 */
#define CONFIG_PACKET 1
#undef  CONFIG_PACKET_MMAP
#undef  CONFIG_NETLINK_DEV
#define CONFIG_NETFILTER 1
#undef  CONFIG_NETFILTER_DEBUG
#define CONFIG_IFX_NETFILTER_PROCFS 1
#undef  CONFIG_FILTER
#define CONFIG_UNIX 1
#define CONFIG_INET 1
#undef  CONFIG_IFX_IGMP_PROXY
#define CONFIG_IFX_IGMP_PROXY_MODULE 1
#undef  CONFIG_IFX_TURBONAT
#define CONFIG_IFX_TURBONAT_MODULE 1
#define CONFIG_IP_MULTICAST 1
#define CONFIG_IP_ADVANCED_ROUTER 1
#define CONFIG_IP_MULTIPLE_TABLES 1
#define CONFIG_IP_ROUTE_FWMARK 1
#define CONFIG_IP_ROUTE_NAT 1
#undef  CONFIG_IP_ROUTE_MULTIPATH
#define CONFIG_IP_ROUTE_TOS 1
#undef  CONFIG_IP_ROUTE_VERBOSE
#undef  CONFIG_IP_PNP
#undef  CONFIG_NET_IPIP
#undef  CONFIG_NET_IPGRE
#define CONFIG_IP_MROUTE 1
#undef  CONFIG_IP_PIMSM_V1
#undef  CONFIG_IP_PIMSM_V2
#undef  CONFIG_ARPD
#undef  CONFIG_INET_ECN
#undef  CONFIG_SYN_COOKIES

/*
 *   IP: Netfilter Configuration
 */
#define CONFIG_IFX_NF_ADDONS 1
#define CONFIG_IFX_NF_MISC 1
#define CONFIG_IP_NF_CONNTRACK 1
#define CONFIG_IP_NF_FTP 1
#define CONFIG_IP_NF_AMANDA 1
#undef  CONFIG_IP_NF_TFTP
#undef  CONFIG_IP_NF_IRC
#undef  CONFIG_IP_NF_TALK
#define CONFIG_IP_NF_H323 1
#define CONFIG_IP_NF_CT_PROTO_GRE 1
#define CONFIG_IP_NF_PPTP 1
#define CONFIG_IP_NF_MMS 1
#undef  CONFIG_IP_NF_CUSEEME
#define CONFIG_IP_NF_RTSP 1
#undef  CONFIG_IP_NF_QUEUE
#define CONFIG_IP_NF_IPTABLES 1
#define CONFIG_IP_NF_MATCH_LIMIT 1
#define CONFIG_IP_NF_SET 1
#define CONFIG_IP_NF_SET_MAX (256)
#undef  CONFIG_IP_NF_SET_IPMAP
#define CONFIG_IP_NF_SET_IPMAP_MODULE 1
#undef  CONFIG_IP_NF_SET_PORTMAP
#define CONFIG_IP_NF_SET_PORTMAP_MODULE 1
#undef  CONFIG_IP_NF_SET_MACIPMAP
#define CONFIG_IP_NF_SET_MACIPMAP_MODULE 1
#undef  CONFIG_IP_NF_SET_IPHASH
#define CONFIG_IP_NF_SET_IPHASH_MODULE 1
#undef  CONFIG_IP_NF_MATCH_IPRANGE
#define CONFIG_IP_NF_MATCH_IPRANGE_MODULE 1
#define CONFIG_IP_NF_MATCH_DSTLIMIT 1
#define CONFIG_IP_NF_MATCH_MAC 1
#undef  CONFIG_IP_NF_MATCH_PKTTYPE
#define CONFIG_IP_NF_MATCH_MARK 1
#undef  CONFIG_IP_NF_MATCH_MULTIPORT
#undef  CONFIG_IP_NF_MATCH_MPORT
#define CONFIG_IP_NF_MATCH_MPORT_MODULE 1
#define CONFIG_IP_NF_MATCH_TOS 1
#undef  CONFIG_IP_NF_MATCH_TIME
#define CONFIG_IP_NF_MATCH_TIME_MODULE 1
#undef  CONFIG_IP_NF_MATCH_FUZZY
#define CONFIG_IP_NF_MATCH_FUZZY_MODULE 1
#undef  CONFIG_IP_NF_MATCH_PSD
#undef  CONFIG_IP_NF_MATCH_IPV4OPTIONS
#undef  CONFIG_IP_NF_MATCH_IFXATTACK
#undef  CONFIG_IP_NF_MATCH_ADDRTYPE
#undef  CONFIG_IP_NF_MATCH_SYNFLOOD
#undef  CONFIG_IP_NF_MATCH_RECENT
#undef  CONFIG_IP_NF_MATCH_ECN
#define CONFIG_IP_NF_MATCH_DSCP 1
#define CONFIG_IP_NF_MATCH_AH_ESP 1
#define CONFIG_IP_NF_MATCH_LENGTH 1
#undef  CONFIG_IP_NF_MATCH_U32
#define CONFIG_IP_NF_MATCH_U32_MODULE 1
#undef  CONFIG_IP_NF_MATCH_TTL
#define CONFIG_IP_NF_MATCH_TCPMSS 1
#define CONFIG_IP_NF_MATCH_HELPER 1
#define CONFIG_IP_NF_MATCH_STATE 1
#define CONFIG_IP_NF_MATCH_CONNLIMIT 1
#define CONFIG_IP_NF_MATCH_CONNTRACK 1
#undef  CONFIG_IP_NF_MATCH_UNCLEAN
#define CONFIG_IP_NF_MATCH_STRING 1
#undef  CONFIG_IP_NF_MATCH_OWNER
#define CONFIG_IP_NF_FILTER 1
#define CONFIG_IP_NF_TARGET_REJECT 1
#undef  CONFIG_IP_NF_TARGET_MIRROR
#define CONFIG_IP_NF_NAT 1
#define CONFIG_IP_NF_NAT_NEEDED 1
#define CONFIG_IP_NF_TURBONAT 1
#define CONFIG_IP_NF_TARGET_MASQUERADE 1
#undef  CONFIG_IP_NF_TARGET_REDIRECT
#define CONFIG_IP_NF_NAT_H323 1
#define CONFIG_IP_NF_NAT_PPTP 1
#define CONFIG_IP_NF_NAT_PROTO_GRE 1
#define CONFIG_IP_NF_NAT_AMANDA 1
#undef  CONFIG_IP_NF_NAT_SNMP_BASIC
#define CONFIG_IP_NF_NAT_MMS 1
#define CONFIG_IP_NF_NAT_RTSP 1
#define CONFIG_IP_NF_NAT_FTP 1
#define CONFIG_IP_NF_MANGLE 1
#define CONFIG_IP_NF_TARGET_TOS 1
#define CONFIG_IP_NF_TARGET_PTOS 1
#undef  CONFIG_IP_NF_TARGET_ROUTE
#define CONFIG_IP_NF_TARGET_ECN 1
#define CONFIG_IP_NF_TARGET_DSCP 1
#define CONFIG_IP_NF_TARGET_MARK 1
#define CONFIG_IP_NF_TARGET_IMQ 1
#define CONFIG_IP_NF_TARGET_LOG 1
#undef  CONFIG_IP_NF_TARGET_ULOG
#define CONFIG_IP_NF_TARGET_TCPMSS 1
#undef  CONFIG_IP_NF_ARPTABLES

/*
 *   IP: Virtual Server Configuration
 */
#undef  CONFIG_IP_VS
#undef  CONFIG_IPV6
#undef  CONFIG_KHTTPD

/*
 *    SCTP Configuration (EXPERIMENTAL)
 */
#undef  CONFIG_IP_SCTP
#define CONFIG_ATM 1
#undef  CONFIG_ATM_CLIP
#undef  CONFIG_ATM_LANE
#define CONFIG_ATM_BR2684 1
#undef  CONFIG_ATM_BR2684_IPFILTER
#undef  CONFIG_IFX_LLC_MUX
#undef  CONFIG_VLAN_8021Q

/*
 *  
 */
#undef  CONFIG_IPX
#undef  CONFIG_ATALK
#undef  CONFIG_DECNET
#define CONFIG_BRIDGE 1
#undef  CONFIG_IFX_NFEXT_VBRIDGE
#define CONFIG_IFX_NFEXT_VBRIDGE_MODULE 1
#define CONFIG_IFX_BR_OPT 1
#define CONFIG_IFX_BR_WAN_ISOLATION 1
#undef  CONFIG_BRIDGE_NF_EBTABLES
#undef  CONFIG_BRIDGE_EBT_T_FILTER
#undef  CONFIG_BRIDGE_EBT_T_NAT
#undef  CONFIG_BRIDGE_EBT_BROUTE
#undef  CONFIG_BRIDGE_EBT_LOG
#undef  CONFIG_BRIDGE_EBT_LOG
#undef  CONFIG_BRIDGE_EBT_IPF
#undef  CONFIG_BRIDGE_EBT_ARPF
#undef  CONFIG_BRIDGE_EBT_AMONG
#undef  CONFIG_BRIDGE_EBT_LIMIT
#undef  CONFIG_BRIDGE_EBT_VLANF
#undef  CONFIG_BRIDGE_EBT_802_3
#undef  CONFIG_BRIDGE_EBT_PKTTYPE
#undef  CONFIG_BRIDGE_EBT_STP
#undef  CONFIG_BRIDGE_EBT_MARKF
#undef  CONFIG_BRIDGE_EBT_ARPREPLY
#undef  CONFIG_BRIDGE_EBT_SNAT
#undef  CONFIG_BRIDGE_EBT_DNAT
#undef  CONFIG_BRIDGE_EBT_REDIRECT
#undef  CONFIG_BRIDGE_EBT_MARK_T
#undef  CONFIG_X25
#undef  CONFIG_LAPB
#undef  CONFIG_LLC
#undef  CONFIG_NET_DIVERT
#undef  CONFIG_ECONET
#undef  CONFIG_WAN_ROUTER
#undef  CONFIG_NET_FASTROUTE
#define CONFIG_NET_HW_FLOWCONTROL 1

/*
 * QoS and/or fair queueing
 */
#define CONFIG_NET_SCHED 1
#undef  CONFIG_IFX_ALG_QOS
#undef  CONFIG_NET_SCH_CBQ
#define CONFIG_NET_SCH_HTB 1
#undef  CONFIG_NET_SCH_CSZ
#undef  CONFIG_NET_SCH_HFSC
#undef  CONFIG_NET_SCH_ATM
#define CONFIG_NET_SCH_PRIO 1
#undef  CONFIG_NET_SCH_RED
#undef  CONFIG_NET_SCH_SFQ
#undef  CONFIG_NET_SCH_TEQL
#undef  CONFIG_NET_SCH_TBF
#undef  CONFIG_NET_SCH_GRED
#undef  CONFIG_NET_SCH_NETEM
#define CONFIG_NET_SCH_DSMARK 1
#define CONFIG_NET_SCH_INGRESS 1
#define CONFIG_NET_QOS 1
#define CONFIG_NET_ESTIMATOR 1
#define CONFIG_NET_CLS 1
#define CONFIG_NET_CLS_TCINDEX 1
#define CONFIG_NET_CLS_ROUTE4 1
#define CONFIG_NET_CLS_ROUTE 1
#define CONFIG_NET_CLS_FW 1
#define CONFIG_NET_CLS_U32 1
#undef  CONFIG_NET_CLS_RSVP
#undef  CONFIG_NET_CLS_RSVP6
#define CONFIG_NET_CLS_POLICE 1

/*
 * Network testing
 */
#undef  CONFIG_NET_PKTGEN

/*
 * Telephony Support
 */
#undef  CONFIG_PHONE
#undef  CONFIG_PHONE_IXJ
#undef  CONFIG_PHONE_IXJ_PCMCIA

/*
 * ATA/IDE/MFM/RLL support
 */
#undef  CONFIG_IDE
#undef  CONFIG_BLK_DEV_HD

/*
 * SCSI support
 */
#define CONFIG_SCSI 1

/*
 * SCSI support type (disk, tape, CD-ROM)
 */
#define CONFIG_BLK_DEV_SD 1
#define CONFIG_SD_EXTRA_DEVS (5)
#undef  CONFIG_CHR_DEV_ST
#undef  CONFIG_CHR_DEV_OSST
#undef  CONFIG_BLK_DEV_SR
#undef  CONFIG_CHR_DEV_SG

/*
 * Some SCSI devices (e.g. CD jukebox) support multiple LUNs
 */
#undef  CONFIG_SCSI_DEBUG_QUEUES
#undef  CONFIG_SCSI_MULTI_LUN
#undef  CONFIG_SCSI_CONSTANTS
#undef  CONFIG_SCSI_LOGGING

/*
 * SCSI low-level drivers
 */
#undef  CONFIG_BLK_DEV_3W_XXXX_RAID
#undef  CONFIG_SCSI_7000FASST
#undef  CONFIG_SCSI_ACARD
#undef  CONFIG_SCSI_AHA152X
#undef  CONFIG_SCSI_AHA1542
#undef  CONFIG_SCSI_AHA1740
#undef  CONFIG_SCSI_AACRAID
#undef  CONFIG_SCSI_AIC7XXX
#undef  CONFIG_SCSI_AIC79XX
#undef  CONFIG_SCSI_AIC7XXX_OLD
#undef  CONFIG_SCSI_DPT_I2O
#undef  CONFIG_SCSI_ADVANSYS
#undef  CONFIG_SCSI_IN2000
#undef  CONFIG_SCSI_AM53C974
#undef  CONFIG_SCSI_MEGARAID
#undef  CONFIG_SCSI_MEGARAID2
#undef  CONFIG_SCSI_SATA
#undef  CONFIG_SCSI_SATA_AHCI
#undef  CONFIG_SCSI_SATA_SVW
#undef  CONFIG_SCSI_ATA_PIIX
#undef  CONFIG_SCSI_SATA_NV
#undef  CONFIG_SCSI_SATA_QSTOR
#undef  CONFIG_SCSI_SATA_PROMISE
#undef  CONFIG_SCSI_SATA_SX4
#undef  CONFIG_SCSI_SATA_SIL
#undef  CONFIG_SCSI_SATA_SIS
#undef  CONFIG_SCSI_SATA_ULI
#undef  CONFIG_SCSI_SATA_VIA
#undef  CONFIG_SCSI_SATA_VITESSE
#undef  CONFIG_SCSI_BUSLOGIC
#undef  CONFIG_SCSI_CPQFCTS
#undef  CONFIG_SCSI_DMX3191D
#undef  CONFIG_SCSI_DTC3280
#undef  CONFIG_SCSI_EATA
#undef  CONFIG_SCSI_EATA_DMA
#undef  CONFIG_SCSI_EATA_PIO
#undef  CONFIG_SCSI_FUTURE_DOMAIN
#undef  CONFIG_SCSI_GDTH
#undef  CONFIG_SCSI_GENERIC_NCR5380
#undef  CONFIG_SCSI_INITIO
#undef  CONFIG_SCSI_INIA100
#undef  CONFIG_SCSI_NCR53C406A
#undef  CONFIG_SCSI_NCR53C7xx
#undef  CONFIG_SCSI_SYM53C8XX_2
#undef  CONFIG_SCSI_NCR53C8XX
#undef  CONFIG_SCSI_SYM53C8XX
#undef  CONFIG_SCSI_PAS16
#undef  CONFIG_SCSI_PCI2000
#undef  CONFIG_SCSI_PCI2220I
#undef  CONFIG_SCSI_PSI240I
#undef  CONFIG_SCSI_QLOGIC_FAS
#undef  CONFIG_SCSI_QLOGIC_ISP
#undef  CONFIG_SCSI_QLOGIC_FC
#undef  CONFIG_SCSI_QLOGIC_1280
#undef  CONFIG_SCSI_SIM710
#undef  CONFIG_SCSI_SYM53C416
#undef  CONFIG_SCSI_DC390T
#undef  CONFIG_SCSI_T128
#undef  CONFIG_SCSI_U14_34F
#undef  CONFIG_SCSI_NSP32
#undef  CONFIG_SCSI_DEBUG

/*
 * Fusion MPT device support
 */
#undef  CONFIG_FUSION
#undef  CONFIG_FUSION_BOOT
#undef  CONFIG_FUSION_ISENSE
#undef  CONFIG_FUSION_CTL
#undef  CONFIG_FUSION_LAN

/*
 * IEEE 1394 (FireWire) support (EXPERIMENTAL)
 */
#undef  CONFIG_IEEE1394

/*
 * I2O device support
 */
#undef  CONFIG_I2O
#undef  CONFIG_I2O_PCI
#undef  CONFIG_I2O_BLOCK
#undef  CONFIG_I2O_LAN
#undef  CONFIG_I2O_SCSI
#undef  CONFIG_I2O_PROC

/*
 * Network device support
 */
#define CONFIG_NETDEVICES 1

/*
 * ARCnet devices
 */
#undef  CONFIG_ARCNET
#undef  CONFIG_DUMMY
#undef  CONFIG_BONDING
#undef  CONFIG_EQUALIZER
#undef  CONFIG_IMQ
#define CONFIG_IMQ_MODULE 1
#undef  CONFIG_TUN
#undef  CONFIG_ETHERTAP

/*
 * Ethernet (10 or 100Mbit)
 */
#define CONFIG_NET_ETHERNET 1
#define CONFIG_DANUBE_ETHERNET 1
#undef  CONFIG_ADM6996_SUPPORT
#define CONFIG_ADM6996_SUPPORT_MODULE 1

/*
 *       hardware chip type
 */
#define CONFIG_SWITCH_ADM6996I 1
#undef  CONFIG_SWITCH_ADM6996LC

/*
 *       hardware control method
 */
#define CONFIG_SWITCH_ADM6996_MDIO 1
#undef  CONFIG_SWITCH_ADM6996_EEPROM
#undef  CONFIG_IFX_NFEXT_SWITCH_PHYPORT
#define CONFIG_IFX_NFEXT_SWITCH_PHYPORT_MODULE 1
#undef  CONFIG_SUNLANCE
#undef  CONFIG_HAPPYMEAL
#undef  CONFIG_SUNBMAC
#undef  CONFIG_SUNQE
#undef  CONFIG_SUNGEM
#undef  CONFIG_NET_VENDOR_3COM
#undef  CONFIG_LANCE
#undef  CONFIG_NET_VENDOR_SMC
#undef  CONFIG_NET_VENDOR_RACAL
#undef  CONFIG_HP100
#undef  CONFIG_NET_ISA
#undef  CONFIG_NET_PCI
#undef  CONFIG_NET_POCKET

/*
 * Ethernet (1000 Mbit)
 */
#undef  CONFIG_ACENIC
#undef  CONFIG_DL2K
#undef  CONFIG_E1000
#undef  CONFIG_MYRI_SBUS
#undef  CONFIG_NS83820
#undef  CONFIG_HAMACHI
#undef  CONFIG_YELLOWFIN
#undef  CONFIG_R8169
#undef  CONFIG_SK98LIN
#undef  CONFIG_TIGON3
#undef  CONFIG_FDDI
#undef  CONFIG_HIPPI
#undef  CONFIG_PLIP
#define CONFIG_PPP 1
#undef  CONFIG_PPP_MULTILINK
#undef  CONFIG_PPP_FILTER
#undef  CONFIG_PPP_IFX_IDLETIME_EXTENSION
#define CONFIG_PPP_IFX_IDLETIME_EXTENSION_MODULE 1
#undef  CONFIG_PPP_ASYNC
#undef  CONFIG_PPP_SYNC_TTY
#undef  CONFIG_PPP_DEFLATE
#undef  CONFIG_PPP_BSDCOMP
#undef  CONFIG_PPP_MPPE
#define CONFIG_PPPOE 1
#define CONFIG_PPPOATM 1
#undef  CONFIG_SLIP

/*
 * Wireless LAN (non-hamradio)
 */
#define CONFIG_NET_RADIO 1
#undef  CONFIG_STRIP
#undef  CONFIG_WAVELAN
#undef  CONFIG_ARLAN
#undef  CONFIG_AIRONET4500
#undef  CONFIG_AIRONET4500_NONCS
#undef  CONFIG_AIRONET4500_PROC
#undef  CONFIG_AIRO
#undef  CONFIG_HERMES
#undef  CONFIG_PLX_HERMES
#undef  CONFIG_TMD_HERMES
#undef  CONFIG_PCI_HERMES
#undef  CONFIG_NET_WIRELESS_SPURS

/*
 * Prism54 PCI/PCMCIA GT/Duette Driver - 802.11(a/b/g)
 */
#undef  CONFIG_PRISM54
#define CONFIG_NET_WIRELESS 1

/*
 * Token Ring devices
 */
#undef  CONFIG_TR
#undef  CONFIG_NET_FC
#undef  CONFIG_RCPCI
#undef  CONFIG_SHAPER

/*
 * Wan interfaces
 */
#undef  CONFIG_WAN

/*
 * ATM drivers
 */
#undef  CONFIG_ATM_TCP
#define CONFIG_ATM_DANUBE 1
#define CONFIG_IFX_ATM_OAM 1
#undef  CONFIG_ATM_LANAI
#undef  CONFIG_ATM_ENI
#undef  CONFIG_ATM_FIRESTREAM
#undef  CONFIG_ATM_ZATM
#undef  CONFIG_ATM_NICSTAR
#undef  CONFIG_ATM_IDT77252
#undef  CONFIG_ATM_AMBASSADOR
#undef  CONFIG_ATM_HORIZON
#undef  CONFIG_ATM_IA
#undef  CONFIG_ATM_FORE200E_MAYBE
#undef  CONFIG_ATM_HE

/*
 * Amateur Radio support
 */
#undef  CONFIG_HAMRADIO

/*
 * IrDA (infrared) support
 */
#undef  CONFIG_IRDA

/*
 * ISDN subsystem
 */
#undef  CONFIG_ISDN

/*
 * Input core support
 */
#undef  CONFIG_INPUT
#undef  CONFIG_INPUT_KEYBDEV
#undef  CONFIG_INPUT_MOUSEDEV
#undef  CONFIG_INPUT_JOYDEV
#undef  CONFIG_INPUT_EVDEV
#undef  CONFIG_INPUT_UINPUT

/*
 * Character devices
 */
#undef  CONFIG_VT
#undef  CONFIG_SERIAL
#undef  CONFIG_SERIAL_EXTENDED
#undef  CONFIG_SERIAL_NONSTANDARD
#define CONFIG_UNIX98_PTYS 1
#define CONFIG_UNIX98_PTY_COUNT (32)

/*
 * I2C support
 */
#undef  CONFIG_I2C

/*
 * Mice
 */
#undef  CONFIG_BUSMOUSE
#undef  CONFIG_MOUSE

/*
 * Joysticks
 */
#undef  CONFIG_INPUT_GAMEPORT

/*
 * Input core support is needed for gameports
 */

/*
 * Input core support is needed for joysticks
 */
#undef  CONFIG_QIC02_TAPE
#undef  CONFIG_IPMI_HANDLER
#undef  CONFIG_IPMI_PANIC_EVENT
#undef  CONFIG_IPMI_DEVICE_INTERFACE
#undef  CONFIG_IPMI_KCS
#undef  CONFIG_IPMI_WATCHDOG

/*
 * Watchdog Cards
 */
#undef  CONFIG_WATCHDOG
#undef  CONFIG_SCx200
#undef  CONFIG_SCx200_GPIO
#undef  CONFIG_AMD_PM768
#undef  CONFIG_NVRAM
#undef  CONFIG_RTC
#undef  CONFIG_DTLK
#undef  CONFIG_R3964
#undef  CONFIG_APPLICOM

/*
 * Ftape, the floppy tape device driver
 */
#undef  CONFIG_FTAPE
#undef  CONFIG_AGP

/*
 * Direct Rendering Manager (XFree86 DRI support)
 */
#undef  CONFIG_DRM
#undef  CONFIG_SERIAL_AMAZONASC
#undef  CONFIG_SERIAL_AMAZONASC_CONSOLE
#undef  CONFIG_SERIAL_DANUBEASC
#undef  CONFIG_SERIAL_DANUBEASC_CONSOLE
#define CONFIG_SERIAL_IFX_ASC 1
#define CONFIG_SERIAL_IFX_ASC_CONSOLE 1
#define CONFIG_IFX_ASC_DEFAULT_BAUDRATE (115200)
#undef  CONFIG_IFX_ASC_CONSOLE_ASC0
#define CONFIG_IFX_ASC_CONSOLE_ASC1 1
#define CONFIG_SERIAL_CORE 1
#define CONFIG_SERIAL_CORE_CONSOLE 1
#define CONFIG_DANUBE_SSC 1
#undef  CONFIG_DANUBE_EEPROM
#define CONFIG_DANUBE_WDT 1
#define CONFIG_DANUBE_MEI 1
#define CONFIG_DANUBE_MEI_MIB 1
#undef  CONFIG_DANUBE_MEI_BSP
#define CONFIG_DANUBE_BCU 1
#define CONFIG_DANUBE_PMU 1
#define CONFIG_DANUBE_RCU 1
#define CONFIG_DANUBE_CGU 1
#define CONFIG_DANUBE_GPTU 1
#define CONFIG_DANUBE_LED 1
#undef  CONFIG_DANUBE_SDIO
#undef  CONFIG_DANUBE_DUSLIC
#define CONFIG_DANUBE_MEMCOPY 1

/*
 * File systems
 */
#undef  CONFIG_QUOTA
#undef  CONFIG_QFMT_V2
#undef  CONFIG_AUTOFS_FS
#undef  CONFIG_AUTOFS4_FS
#undef  CONFIG_REISERFS_FS
#undef  CONFIG_REISERFS_CHECK
#undef  CONFIG_REISERFS_PROC_INFO
#undef  CONFIG_ADFS_FS
#undef  CONFIG_ADFS_FS_RW
#undef  CONFIG_AFFS_FS
#undef  CONFIG_HFS_FS
#undef  CONFIG_HFSPLUS_FS
#undef  CONFIG_BEFS_FS
#undef  CONFIG_BEFS_DEBUG
#undef  CONFIG_BFS_FS
#undef  CONFIG_EXT3_FS
#undef  CONFIG_JBD
#undef  CONFIG_JBD_DEBUG
#define CONFIG_FAT_FS 1
#undef  CONFIG_MSDOS_FS
#undef  CONFIG_UMSDOS_FS
#define CONFIG_VFAT_FS 1
#undef  CONFIG_EFS_FS
#undef  CONFIG_JFFS_FS
#undef  CONFIG_JFFS2_FS
#define CONFIG_SQUASHFS 1
#undef  CONFIG_SQUASHFS_EMBEDDED
#undef  CONFIG_SQUASHFS_LZMA
#undef  CONFIG_CRAMFS
#undef  CONFIG_TMPFS
#define CONFIG_RAMFS 1
#undef  CONFIG_ISO9660_FS
#undef  CONFIG_JOLIET
#undef  CONFIG_ZISOFS
#undef  CONFIG_JFS_FS
#undef  CONFIG_JFS_DEBUG
#undef  CONFIG_JFS_STATISTICS
#undef  CONFIG_MINIX_FS
#undef  CONFIG_VXFS_FS
#undef  CONFIG_NTFS_FS
#undef  CONFIG_NTFS_RW
#undef  CONFIG_HPFS_FS
#define CONFIG_PROC_FS 1
#undef  CONFIG_DEVFS_FS
#undef  CONFIG_DEVFS_MOUNT
#undef  CONFIG_DEVFS_DEBUG
#define CONFIG_DEVPTS_FS 1
#undef  CONFIG_QNX4FS_FS
#undef  CONFIG_QNX4FS_RW
#undef  CONFIG_ROMFS_FS
#undef  CONFIG_EXT2_FS
#undef  CONFIG_SYSV_FS
#undef  CONFIG_UDF_FS
#undef  CONFIG_UDF_RW
#undef  CONFIG_UFS_FS
#undef  CONFIG_UFS_FS_WRITE
#undef  CONFIG_XFS_FS
#undef  CONFIG_XFS_QUOTA
#undef  CONFIG_XFS_RT
#undef  CONFIG_XFS_TRACE
#undef  CONFIG_XFS_DEBUG

/*
 * Network File Systems
 */
#undef  CONFIG_CODA_FS
#undef  CONFIG_INTERMEZZO_FS
#undef  CONFIG_NFS_FS
#undef  CONFIG_NFS_V3
#undef  CONFIG_NFS_DIRECTIO
#undef  CONFIG_ROOT_NFS
#undef  CONFIG_NFSD
#undef  CONFIG_NFSD_V3
#undef  CONFIG_NFSD_TCP
#undef  CONFIG_SUNRPC
#undef  CONFIG_LOCKD
#undef  CONFIG_SMB_FS
#undef  CONFIG_NCP_FS
#undef  CONFIG_NCPFS_PACKET_SIGNING
#undef  CONFIG_NCPFS_IOCTL_LOCKING
#undef  CONFIG_NCPFS_STRONG
#undef  CONFIG_NCPFS_NFS_NS
#undef  CONFIG_NCPFS_OS2_NS
#undef  CONFIG_NCPFS_SMALLDOS
#undef  CONFIG_NCPFS_NLS
#undef  CONFIG_NCPFS_EXTRAS
#undef  CONFIG_ZISOFS_FS

/*
 * Partition Types
 */
#undef  CONFIG_PARTITION_ADVANCED
#define CONFIG_MSDOS_PARTITION 1
#undef  CONFIG_SMB_NLS
#define CONFIG_NLS 1

/*
 * Native Language Support
 */
#define CONFIG_NLS_DEFAULT "iso8859-1"
#undef  CONFIG_NLS_CODEPAGE_437
#undef  CONFIG_NLS_CODEPAGE_737
#undef  CONFIG_NLS_CODEPAGE_775
#undef  CONFIG_NLS_CODEPAGE_850
#undef  CONFIG_NLS_CODEPAGE_852
#undef  CONFIG_NLS_CODEPAGE_855
#undef  CONFIG_NLS_CODEPAGE_857
#undef  CONFIG_NLS_CODEPAGE_860
#undef  CONFIG_NLS_CODEPAGE_861
#undef  CONFIG_NLS_CODEPAGE_862
#undef  CONFIG_NLS_CODEPAGE_863
#undef  CONFIG_NLS_CODEPAGE_864
#undef  CONFIG_NLS_CODEPAGE_865
#undef  CONFIG_NLS_CODEPAGE_866
#undef  CONFIG_NLS_CODEPAGE_869
#undef  CONFIG_NLS_CODEPAGE_936
#undef  CONFIG_NLS_CODEPAGE_950
#undef  CONFIG_NLS_CODEPAGE_932
#undef  CONFIG_NLS_CODEPAGE_949
#undef  CONFIG_NLS_CODEPAGE_874
#undef  CONFIG_NLS_ISO8859_8
#undef  CONFIG_NLS_CODEPAGE_1250
#undef  CONFIG_NLS_CODEPAGE_1251
#undef  CONFIG_NLS_ISO8859_1
#undef  CONFIG_NLS_ISO8859_2
#undef  CONFIG_NLS_ISO8859_3
#undef  CONFIG_NLS_ISO8859_4
#undef  CONFIG_NLS_ISO8859_5
#undef  CONFIG_NLS_ISO8859_6
#undef  CONFIG_NLS_ISO8859_7
#undef  CONFIG_NLS_ISO8859_9
#undef  CONFIG_NLS_ISO8859_13
#undef  CONFIG_NLS_ISO8859_14
#undef  CONFIG_NLS_ISO8859_15
#undef  CONFIG_NLS_KOI8_R
#undef  CONFIG_NLS_KOI8_U
#undef  CONFIG_NLS_UTF8

/*
 * Multimedia devices
 */
#undef  CONFIG_VIDEO_DEV

/*
 * Sound
 */
#undef  CONFIG_SOUND

/*
 * USB support
 */
#undef  CONFIG_USB

/*
 * Support for USB gadgets
 */
#undef  CONFIG_USB_GADGET

/*
 * Bluetooth support
 */
#undef  CONFIG_BLUEZ

/*
 * Kernel tracing
 */
#undef  CONFIG_TRACE

/*
 * Kernel hacking
 */
#define CONFIG_CROSSCOMPILE 1
#undef  CONFIG_RUNTIME_DEBUG
#undef  CONFIG_KGDB
#undef  CONFIG_GDB_CONSOLE
#undef  CONFIG_DEBUG_INFO
#undef  CONFIG_MAGIC_SYSRQ
#undef  CONFIG_MIPS_UNCACHED
#define CONFIG_LOG_BUF_SHIFT (0)

/*
 * Cryptographic options
 */
#undef  CONFIG_CRYPTO

/*
 * Library routines
 */
#undef  CONFIG_CRC32
#define CONFIG_ZLIB_INFLATE 1
#define CONFIG_ZLIB_DEFLATE 1
