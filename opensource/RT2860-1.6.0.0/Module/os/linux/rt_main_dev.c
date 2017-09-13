#include <rt_config.h>

#define FORTY_MHZ_INTOLERANT_INTERVAL	(60*1000) // 1 min
#define MAX_TX_IN_TBTT		(16)

#ifdef CONFIG_AP_SUPPORT
static RALINK_TIMER_STRUCT     PeriodicTimer;
extern VOID APDetectOverlappingExec(
			IN PVOID SystemSpecific1, 
			IN PVOID FunctionContext, 
			IN PVOID SystemSpecific2, 
			IN PVOID SystemSpecific3);
#endif // CONFIG_AP_SUPPORT //
static void rx_done_tasklet(unsigned long data);
static void mgmt_dma_done_tasklet(unsigned long data);
static void ac0_dma_done_tasklet(unsigned long data);
static void ac1_dma_done_tasklet(unsigned long data);
static void ac2_dma_done_tasklet(unsigned long data);
static void ac3_dma_done_tasklet(unsigned long data);
static void hcca_dma_done_tasklet(unsigned long data);
static void tbtt_tasklet(unsigned long data);
static void fifo_statistic_full_tasklet(unsigned long data);
#ifdef CONFIG_AP_SUPPORT
#ifdef DFS_SUPPORT
static void pulse_radar_detect_tasklet(unsigned long data);
static void width_radar_detect_tasklet(unsigned long data);
#endif // DFS_SUPPORT //
#ifdef CARRIER_DETECTION_SUPPORT
static void carrier_sense_tasklet(unsigned long data);
#endif // CARRIER_DETECTION_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

/*---------------------------------------------------------------------*/
/* Symbol & Macro Definitions                                          */
/*---------------------------------------------------------------------*/
#define RT2860_INT_RX_DLY				(1<<0)		// bit 0	
#define RT2860_INT_TX_DLY				(1<<1)		// bit 1
#define RT2860_INT_RX_DONE				(1<<2)		// bit 2
#define RT2860_INT_AC0_DMA_DONE			(1<<3)		// bit 3
#define RT2860_INT_AC1_DMA_DONE			(1<<4)		// bit 4
#define RT2860_INT_AC2_DMA_DONE			(1<<5)		// bit 5
#define RT2860_INT_AC3_DMA_DONE			(1<<6)		// bit 6
#define RT2860_INT_HCCA_DMA_DONE		(1<<7)		// bit 7
#define RT2860_INT_MGMT_DONE			(1<<8)		// bit 8

#define INT_RX			RT2860_INT_RX_DONE

#define INT_AC0_DLY		(RT2860_INT_AC0_DMA_DONE) //| RT2860_INT_TX_DLY)
#define INT_AC1_DLY		(RT2860_INT_AC1_DMA_DONE) //| RT2860_INT_TX_DLY)
#define INT_AC2_DLY		(RT2860_INT_AC2_DMA_DONE) //| RT2860_INT_TX_DLY)
#define INT_AC3_DLY		(RT2860_INT_AC3_DMA_DONE) //| RT2860_INT_TX_DLY)
#define INT_HCCA_DLY 	(RT2860_INT_HCCA_DMA_DONE) //| RT2860_INT_TX_DLY)
#define INT_MGMT_DLY	RT2860_INT_MGMT_DONE


char *mac = "";		   // default 00:00:00:00:00:00
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,12)
MODULE_PARM (mac, "s");
#else
module_param (mac, charp, 0);
#endif
MODULE_PARM_DESC (mac, "rt2860: wireless mac addr");


/*---------------------------------------------------------------------*/
/* Prototypes of External Functions                                    */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/* Prototypes of Functions Used                                        */
/*---------------------------------------------------------------------*/
/* function declarations */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
irqreturn_t
#else
void
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
rt2860_interrupt(int irq, void *dev_instance);
#else
rt2860_interrupt(int irq, void *dev_instance, struct pt_regs *regs);
#endif
static int rt2860_open(struct net_device *net_dev);
static int rt2860_close(struct net_device *net_dev);
static int rt2860_send_packets(struct sk_buff *skb, struct net_device *net_dev);
static int rt2860_init(struct net_device *net_dev);
static INT __devinit rt2860_init_one (struct pci_dev *pci_dev, const struct pci_device_id  *ent);
static VOID __devexit rt2860_remove_one(struct pci_dev *pci_dev);
static INT __devinit rt2860_probe(struct pci_dev *pci_dev, const struct pci_device_id  *ent);
void kill_thread_task(PRTMP_ADAPTER pAd);
void init_thread_task(PRTMP_ADAPTER pAd);
static void __exit rt2860_cleanup_module(void);
static int __init rt2860_init_module(void);
extern INT rt2860_ioctl(struct net_device *net_dev, struct ifreq *rq, int cmd);
static VOID rt2860_set_rx_mode(IN struct net_device *net_dev);
struct net_device_stats *rt2860_get_ether_stats(IN struct net_device *net_dev);

/*---------------------------------------------------------------------*/
/* External Variable Definitions                                       */
/*---------------------------------------------------------------------*/


//
// Ralink PCI device table, include all supported chipsets
//
static struct pci_device_id rt2860_pci_tbl[] __devinitdata =
{
	{0x1814, 0x0601, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},		//RT28602.4G
	{0x1814, 0x0681, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0x1814, 0x0701, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0x1814, 0x0781, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0,}		// terminate list
};

MODULE_DEVICE_TABLE(pci, rt2860_pci_tbl);

//
// Our PCI driver structure
//
static struct pci_driver rt2860_driver =
{
    name:       "rt2860",
    id_table:   rt2860_pci_tbl,
    probe:      rt2860_init_one,
#if LINUX_VERSION_CODE >= 0x20412
    remove:     __devexit_p(rt2860_remove_one),
#else
    remove:     __devexit(rt2860_remove_one),
#endif
};

static INT __init rt2860_init_module(VOID)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	return pci_register_driver(&rt2860_driver);
#else
    return pci_module_init(&rt2860_driver);
#endif
}

//
// Driver module unload function
//
static VOID __exit rt2860_cleanup_module(VOID)
{
    pci_unregister_driver(&rt2860_driver);
}

module_init(rt2860_init_module);
module_exit(rt2860_cleanup_module);


static INT __devinit rt2860_init_one (
    IN  struct pci_dev              *pci_dev,
    IN  const struct pci_device_id  *ent)
{
    INT rc;

    DBGPRINT(RT_DEBUG_TRACE, ("===> rt2860_init_one\n"));

    // wake up and enable device
    if (pci_enable_device (pci_dev))
    {
        rc = -EIO;
    }
    else
    {
        rc = rt2860_probe(pci_dev, ent);
    }

    DBGPRINT(RT_DEBUG_TRACE, ("<=== rt2860_init_one\n"));
    return rc;
}

static VOID __devexit rt2860_remove_one(
    IN  struct pci_dev  *pci_dev)
{
    struct net_device   *net_dev = pci_get_drvdata(pci_dev);
    RTMP_ADAPTER        *pAd = net_dev->priv;

    DBGPRINT(RT_DEBUG_TRACE, ("===> rt2860_remove_one\n"));

	if (pAd != NULL)
	{
#ifdef WSC_INCLUDED
    if (pAd->write_dat_file_pid >= 0)
    {
        int ret;
        pAd->time_to_die = 1;
        up(&(pAd->write_dat_file_semaphore));
        wmb(); // need to check
		ret = kill_proc (pAd->write_dat_file_pid, SIGTERM, 1);
		if (ret)
		{
			printk (KERN_ERR "%s: unable to signal thread\n", pAd->net_dev->name);
			return;
		}
        wait_for_completion (&pAd->write_dat_file_notify);
    }
#endif // WSC_INCLUDED //
    
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	// remove all AP-client virtual interfaces.
	RT2860_ApCli_Remove(pAd);
#endif // APCLI_SUPPORT //

#ifdef WDS_SUPPORT
	// remove all WDS virtual interfaces.
	RT2860_WDS_Remove(pAd);
#endif // WDS_SUPPORT //

#ifdef MBSS_SUPPORT
    RT2860_MBSS_Remove(pAd);
#endif // MBSS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

    // Unregister network device
    unregister_netdev(net_dev);

    // Unmap CSR base address
    iounmap((char *)(net_dev->base_addr));

	RTMPFreeAdapter(pAd);

    // release memory region
    release_mem_region(pci_resource_start(pci_dev, 0), pci_resource_len(pci_dev, 0));
	}
	else
	{
	    // Unregister network device
    	unregister_netdev(net_dev);

	    // Unmap CSR base address
    	iounmap((char *)(net_dev->base_addr));

	    // release memory region
    	release_mem_region(pci_resource_start(pci_dev, 0), pci_resource_len(pci_dev, 0));
	}

    // Free pre-allocated net_device memory
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    free_netdev(net_dev);
#else
	kfree(net_dev);
#endif
}

//
// PCI device probe & initialization function
//
static INT __devinit   rt2860_probe(
    IN  struct pci_dev              *pci_dev, 
    IN  const struct pci_device_id  *ent)
{
    struct  net_device      *net_dev;
    PRTMP_ADAPTER           pAd;
    CHAR                    *print_name;
    ULONG                   csr_addr;
    INT                     status;
	PVOID					handle;
	
//#define DRIVER_VERSION "0.1"
	
    DBGPRINT(RT_DEBUG_TRACE, ("Driver version-%s\n", DRIVER_VERSION));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    print_name = pci_dev ? pci_name(pci_dev) : "rt2860";
#else
    print_name = pci_dev ? pci_dev->slot_name : "rt2860";
#endif

#if LINUX_VERSION_CODE <= 0x20402       // Red Hat 7.1
    net_dev = alloc_netdev(sizeof(PRTMP_ADAPTER), "eth%d", ether_setup);
#else
    net_dev = alloc_etherdev(sizeof(PRTMP_ADAPTER));
#endif
    if (net_dev == NULL) 
    {
        DBGPRINT(RT_DEBUG_TRACE, ("init_ethernet failed\n"));
        goto err_out;
    }

    SET_MODULE_OWNER(net_dev);

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
/* for supporting Network Manager */
/* Set the sysfs physical device reference for the network logical device
 * if set prior to registration will cause a symlink during initialization.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
    SET_NETDEV_DEV(net_dev, &(pci_dev->dev));
#endif
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //
        
    if (pci_request_regions(pci_dev, print_name))
        goto err_out_free_netdev;

    // Interrupt IRQ number
    net_dev->irq = pci_dev->irq;

    // map physical address to virtual address for accessing register
    csr_addr = (unsigned long) ioremap(pci_resource_start(pci_dev, 0), pci_resource_len(pci_dev, 0));
    if (!csr_addr) 
    {
        DBGPRINT(RT_DEBUG_ERROR, ("ioremap failed for device %s, region 0x%lX @ 0x%lX\n",
            print_name, (ULONG)pci_resource_len(pci_dev, 0), pci_resource_start(pci_dev, 0)));
        goto err_out_free_res;
    }
	// Allocate RTMP_ADAPTER miniport adapter structure
	
	handle = kmalloc(sizeof(struct os_cookie) , GFP_KERNEL);
	
	((POS_COOKIE)handle)->pci_dev = pci_dev;

	status = RTMPAllocAdapterBlock(handle, &pAd);
	
	if (status != NDIS_STATUS_SUCCESS) 
	{
		goto err_out_free_res;
	}

	net_dev->priv = (PVOID)pAd;

    // Save CSR virtual address and irq to device structure
    net_dev->base_addr = csr_addr;
    pAd->CSRBaseAddress = (PUCHAR)net_dev->base_addr;
    pAd->net_dev = net_dev;
    // Set DMA master
    pci_set_master(pci_dev);

    // The chip-specific entries in the device structure.
    net_dev->open = rt2860_open;
    //net_dev->hard_start_xmit = rt2860_packet_xmit;
    net_dev->hard_start_xmit = rt2860_send_packets;
	net_dev->do_ioctl = rt2860_ioctl;
    net_dev->stop = rt2860_close;
    net_dev->priv_flags = INT_MAIN;
	
	net_dev->get_stats = rt2860_get_ether_stats;
	

	net_dev->set_multicast_list = rt2860_set_rx_mode;
  
    //net_dev->priv_flags = INT_MAIN;

    {// find available 
        INT     i=0;
        CHAR    slot_name[IFNAMSIZ];
        struct net_device   *device;

        for (i = 0; i < 8; i++)
        {
            sprintf(slot_name, "ra%d", i);
            
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			device = dev_get_by_name(slot_name);
			if (device != NULL) dev_put(device);
#else
            for (device = dev_base; device != NULL; device = device->next)
            {
                if (strncmp(device->name, slot_name, 4) == 0)
                {
                    break;
                }
            }
#endif
            if(device == NULL)  break;
        }
        if(i == 8)
        {
            DBGPRINT(RT_DEBUG_ERROR, ("No available slot name\n"));
            goto err_out_unmap;
        }

        sprintf(net_dev->name, "ra%d", i);
    }

    // Register this device
    status = register_netdev(net_dev);
    if (status)
        goto err_out_unmap;

    DBGPRINT(RT_DEBUG_TRACE, ("%s: at 0x%lx, VA 0x%lx, IRQ %d. \n", 
        net_dev->name, pci_resource_start(pci_dev, 0), (ULONG)csr_addr, pci_dev->irq));

    // Set driver data
    pci_set_drvdata(pci_dev, net_dev);

#ifdef WSC_INCLUDED
	InitializeWSCTLV();
    init_MUTEX_LOCKED(&(pAd->write_dat_file_semaphore));
    init_completion(&pAd->write_dat_file_notify);
    start_write_dat_file_thread(pAd);
#endif // WSC_INCLUDED //

#ifdef CONFIG_AP_SUPPORT
	pAd->ApCfg.MBSSID[MAIN_MBSSID].MSSIDDev = net_dev;
#endif // CONFIG_AP_SUPPORT //

    return 0;

err_out_unmap:
    iounmap((void *)csr_addr);
    release_mem_region(pci_resource_start(pci_dev, 0), pci_resource_len(pci_dev, 0));
err_out_free_res:
    pci_release_regions(pci_dev);
err_out_free_netdev:
    kfree (net_dev);
err_out:
    return -ENODEV;
}

static int rt2860_close(IN struct net_device *net_dev)
{
    RTMP_ADAPTER    *pAd = net_dev->priv;

    DBGPRINT(RT_DEBUG_TRACE, ("===> rt2860_close\n"));

	if (pAd == NULL)
		return 0;


	//RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);
	kill_thread_task(pAd);
    // Stop Mlme state machine
    MlmeHalt(pAd);

#ifdef CONFIG_AP_SUPPORT
{
	BOOLEAN 	  Cancelled;
	RTMPCancelTimer(&PeriodicTimer,	&Cancelled);
}

#ifdef APCLI_SUPPORT

#ifdef WSC_AP_SUPPORT
    WscStop(pAd, TRUE, &pAd->ApCfg.ApCliTab[0].WscControl);
    WSC_VFREE_KEY_MEM(pAd->ApCfg.ApCliTab[0].WscControl.pPubKeyMem, pAd->ApCfg.ApCliTab[0].WscControl.pSecKeyMem);
#endif // WSC_AP_SUPPORT //

	// remove all AP-client virtual interfaces.
	RT2860_ApCli_Close(pAd);
#endif // APCLI_SUPPORT //

#ifdef MAT_SUPPORT
	MATEngineExit();
#endif // MAT_SUPPORT //

#ifdef WDS_SUPPORT
	// remove all WDS virtual interfaces.
	RT2860_WDS_Close(pAd);
#endif // WDS_SUPPORT //

#ifdef MBSS_SUPPORT
    RT2860_MBSS_Close(pAd);
#endif // MBSS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
    // shutdown Access Point function, release all related resources 
	APShutdown(pAd);
#endif // CONFIG_AP_SUPPORT //


#ifdef WSC_INCLUDED

#ifdef CONFIG_AP_SUPPORT
    WscStop(pAd, FALSE, &pAd->ApCfg.MBSSID[0].WscControl);
#endif // CONFIG_AP_SUPPORT //


#ifdef OLD_DH_KEY
{
#ifdef CONFIG_AP_SUPPORT
    WSC_VFREE_KEY_MEM(pAd->ApCfg.MBSSID[0].WscControl.pPubKeyMem, pAd->ApCfg.MBSSID[0].WscControl.pSecKeyMem);
#endif // CONFIG_AP_SUPPORT //
}
#endif // OLD_DH_KEY //

#ifndef OLD_DH_KEY
	DH_freeall();
#endif //OLD_DH_KEY

#endif // WSC_INCLUDED //    

#ifdef CONFIG_AP_SUPPORT
	// Free BssTab & ChannelInfo tabbles.
	AutoChBssTableDestroy(pAd);
	ChannelInfoDestroy(pAd);
#endif // CONFIG_AP_SUPPORT //

    netif_stop_queue(net_dev);
    netif_carrier_off(net_dev);
    if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_ACTIVE))
    {
        NICDisableInterrupt(pAd);
    }

    // Disable Rx, register value supposed will remain after reset
	NICIssueReset(pAd);

    // Free IRQ
    if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
    {
        // Deregister interrupt function

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	synchronize_irq(net_dev->irq);
#endif
        free_irq(net_dev->irq, net_dev);
        RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE);
    }

    // Free Ring buffers
	RTMPFreeDMAMemory(pAd);

	// Free ba reorder resource
	ba_reordering_resource_release(pAd);
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    module_put(THIS_MODULE);
#else
    MOD_DEC_USE_COUNT;
#endif

    return 0;
}

#if LINUX_VERSION_CODE <= 0x20402       // Red Hat 7.1
static struct net_device *alloc_netdev(int sizeof_priv, const char *mask, void (*setup)(struct net_device *))
{
    struct net_device	*dev;
    INT					alloc_size;

    /* ensure 32-byte alignment of the private area */
    alloc_size = sizeof (*dev) + sizeof_priv + 31;

    dev = (struct net_device *) kmalloc (alloc_size, GFP_KERNEL);
    if (dev == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("alloc_netdev: Unable to allocate device memory.\n"));
        return NULL;
    }

    memset(dev, 0, alloc_size);

    if (sizeof_priv)
        dev->priv = (void *) (((long)(dev + 1) + 31) & ~31);

    setup(dev);
    strcpy(dev->name,mask);

    return dev;
}
#endif

void CfgInitHook(PRTMP_ADAPTER pAd)
{
	//OID_SET_HT_PHYMODE HTPhyMode;
	//int i, KeyIdx;
	//UCHAR CipherAlg;
	//PUCHAR	Key;


	pAd->bBroadComHT = TRUE;

	// WEP mode
#if 0
	for (i=0; i<MAX_MBSSID_NUM; i++)
	{	
		int j, keyid;

		pAd->ApCfg.MBSSID[i].AuthMode = Ndis802_11AuthModeOpen;
		pAd->ApCfg.MBSSID[i].WepStatus = Ndis802_11WEPEnabled;
		pAd->ApCfg.MBSSID[i].DefaultKeyId = keyid = i%4;

		CipherAlg = pAd->SharedKey[i][keyid].CipherAlg = CIPHER_WEP64;

		for (j=0; j<5; j++)
		{ 
			pAd->SharedKey[i][keyid].Key[j] = '0'+i;
		}

		Key = pAd->SharedKey[i][keyid].Key;

		AsicAddSharedKeyEntry(pAd, i, keyid, CipherAlg, Key, NULL, NULL);
		
	}
#endif


//	pAd->CommonCfg.BACapability.field.MpduDensity = 4;
//	pAd->CommonCfg.BACapability.field.AutoBA = TRUE;
//	pAd->CommonCfg.BACapability.field.RxBAWinLimit = 16;
//	pAd->CommonCfg.PhyMode = PHY_11A;

#if 0
	pAd->CommonCfg.PhyMode = PHY_11N;
	pAd->CommonCfg.BACapability.field.RxBAWinLimit = 16;

	HTPhyMode.BW = BW_40;
	HTPhyMode.PhyMode = PHY_11N;
	HTPhyMode.ExtOffset = EXTCHA_NONE;
	HTPhyMode.TransmitNo = 1;
	HTPhyMode.HtMode = HTMODE_GF;
	HTPhyMode.SHORTGI = GI_400; 
	HTPhyMode.STBC = STBC_NONE;
	HTPhyMode.MCS = MCS_AUTO;

	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
		RTMPSetHT(pAd, &HTPhyMode);
#endif // 0


	//NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].Ssid, "Sam_AP", 6);
	//pAd->ApCfg.MBSSID[apidx].SsidLen = 6;


#if 0
	{
		UCHAR *key = "12345678";
		UCHAR keyMaterial[40];
		int i;
		
		// set WPAPSK 
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPAPSK;

		// set TKIP
		pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11Encryption2Enabled;
	
		// Init some variable
		for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
		{
			if (pAd->MacTab.Content[i].ValidAsCLI)
			{
				pAd->MacTab.Content[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
			}
		}
		pAd->ApCfg.PortSecured = WPA_802_1X_PORT_NOT_SECURED;
    
		ApCfg.MBSSID[apidx].DefaultKeyId  = 0;
		if(pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
		{   
			pAd->ApCfg.WpaGTKState = SETKEYS;
			pAd->ApCfg.GKeyDoneStations = pAd->MacTab.Size;
			ApCfg.MBSSID[apidx].DefaultKeyId = 1;		
		}

#ifdef CONFIG_AP_SUPPORT
		// set WPAPSK key
		PasswordHash((CHAR *)key, pAd->ApCfg.MBSSID[apidx].Ssid, pAd->ApCfg.MBSSID[apidx].SsidLen, keyMaterial);
    	NdisMoveMemory(pAd->ApCfg.PMK, keyMaterial, 32);
#endif // CONFIG_AP_SUPPORT //
	
		RTMPMakeRSNIE(pAd, pAd->ApCfg.MBSSID[apidx].AuthMode, pAd->ApCfg.MBSSID[apidx].WepStatus, apidx);
	}
#endif

}

static int rt2860_init(IN struct net_device *net_dev)
{
	PRTMP_ADAPTER 			pAd = (PRTMP_ADAPTER)net_dev->priv;
	UINT					index;
	UCHAR					TmpPhy;
//	ULONG					Value=0;
	NDIS_STATUS				Status;
//#ifdef CONFIG_AP_SUPPORT
//    OID_SET_HT_PHYMODE		SetHT;
//#endif // CONFIG_AP_SUPPORT //
	WPDMA_GLO_CFG_STRUC     GloCfg;


	init_thread_task(pAd);
	
	NICInitTxRxRingAndBacklogQueue(pAd);

	/* Allocate BA Reordering memory */
	ba_reordering_resource_init(pAd, MAX_REORDERING_MPDU_NUM);
	//
	// Make sure MAC gets ready.
	//
	index = 0;
	do 
	{
		RTMP_IO_READ32(pAd, MAC_CSR0, &pAd->MACVersion);

		if ((pAd->MACVersion != 0x00) && (pAd->MACVersion != 0xFFFFFFFF))
			break;

		RTMPusecDelay(10);
	} while (index++ < 100);

	DBGPRINT(RT_DEBUG_TRACE, ("MAC_CSR0  [ Ver:Rev=0x%08lx]\n", pAd->MACVersion));

	// Disable DMA.
	RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &GloCfg.word);
	GloCfg.word &= 0xff0;
	GloCfg.field.EnTXWriteBackDDONE =1;
	RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, GloCfg.word);

	RTMP_IO_WRITE32(pAd, WPDMA_RST_IDX , 0xFFFFFFFF);
	RTMP_IO_WRITE32(pAd, PBF_SYS_CTRL, 0xe1f);
	RTMP_IO_WRITE32(pAd, PBF_SYS_CTRL, 0xe00);

	// Load 8051 firmware; 
	Status = NICLoadFirmware(pAd);
	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT_ERR(("NICLoadFirmware failed, Status[=0x%08x]\n", Status));
		goto err1;
	}

	NICLoadRateSwitchingParams(pAd);

	// Disable interrupts here which is as soon as possible
	// This statement should never be true. We might consider to remove it later
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_ACTIVE))
	{
		NICDisableInterrupt(pAd);
	}

	Status = RTMPAllocDMAMemory(pAd);
	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT_ERR(("RTMPAllocDMAMemory failed, Status[=0x%08x]\n", Status));
		goto err1;
	}

	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE);

	// initialize MLME
	//

	Status = MlmeInit(pAd);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT_ERR(("MlmeInit failed, Status[=0x%08x]\n", Status));
		goto err2;
	}

	// Initialize pAd->StaCfg, pAd->ApCfg, pAd->CommonCfg to manufacture default
	//
	UserCfgInit(pAd);
	
#ifdef CONFIG_AP_SUPPORT
	pAd->OpMode = OPMODE_AP;
#endif // CONFIG_AP_SUPPORT //
//	COPY_MAC_ADDR(pAd->ApCfg.MBSSID[apidx].Bssid, netif->hwaddr);
//	pAd->bForcePrintTX = TRUE;

	CfgInitHook(pAd);	

#ifdef CONFIG_AP_SUPPORT
	APInitialize(pAd);
#endif // CONFIG_AP_SUPPORT //	

#ifdef BLOCK_NET_IF
	initblockQueueTab(pAd);
#endif // BLOCK_NET_IF //


	//
	// Init the hardware, we need to init asic before read registry, otherwise mac register will be reset
	//
	Status = NICInitializeAdapter(pAd, TRUE);
	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT_ERR(("NICInitializeAdapter failed, Status[=0x%08x]\n", Status));
		goto err3;
	}	

	// Read parameters from Config File 
	Status = RTMPReadParametersHook(pAd);

	printk("1. Phy Mode = %d\n", pAd->CommonCfg.PhyMode);
	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT_ERR(("NICReadRegParameters failed, Status[=0x%08x]\n", Status));
		goto err4;
	}



   	//Init Ba Capability parameters.
	pAd->CommonCfg.DesiredHtPhy.MpduDensity = (UCHAR)pAd->CommonCfg.BACapability.field.MpduDensity;
	pAd->CommonCfg.DesiredHtPhy.AmsduEnable = (USHORT)pAd->CommonCfg.BACapability.field.AmsduEnable;
	pAd->CommonCfg.DesiredHtPhy.AmsduSize= (USHORT)pAd->CommonCfg.BACapability.field.AmsduSize;
	pAd->CommonCfg.DesiredHtPhy.MimoPs= (USHORT)pAd->CommonCfg.BACapability.field.MMPSmode;
	// UPdata to HT IE
	pAd->CommonCfg.HtCapability.HtCapInfo.MimoPs = (USHORT)pAd->CommonCfg.BACapability.field.MMPSmode;
	pAd->CommonCfg.HtCapability.HtCapInfo.AMsduSize = (USHORT)pAd->CommonCfg.BACapability.field.AmsduSize;
	pAd->CommonCfg.HtCapability.HtCapParm.MpduDensity = (UCHAR)pAd->CommonCfg.BACapability.field.MpduDensity;


	// after reading Registry, we now know if in AP mode or STA mode

	// Load 8051 firmware; crash when FW image not existent
	// Status = NICLoadFirmware(pAd);
	// if (Status != NDIS_STATUS_SUCCESS)
	//    break;

	printk("2. Phy Mode = %d\n", pAd->CommonCfg.PhyMode);

	// We should read EEPROM for all cases.  rt2860b
	NICReadEEPROMParameters(pAd, mac);	

	printk("3. Phy Mode = %d\n", pAd->CommonCfg.PhyMode);

	// Set PHY to appropriate mode
	TmpPhy = pAd->CommonCfg.PhyMode;
	pAd->CommonCfg.PhyMode = 0xff;
	RTMPSetPhyMode(pAd, TmpPhy);

#ifdef CONFIG_AP_SUPPORT
#if 0
	// Set HT mode
	SetHT.PhyMode = pAd->CommonCfg.PhyMode;
	SetHT.TransmitNo = ((UCHAR)pAd->Antenna.field.TxPath);
	SetHT.HtMode = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.HTMODE;
	SetHT.ExtOffset = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.EXTCHA;
	SetHT.MCS = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.MCS;
	SetHT.BW = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.BW;
	SetHT.STBC = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.STBC;
	SetHT.SHORTGI = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.ShortGI;		

	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		printk("==========>RTMPSetHT\n");
	RTMPSetHT(pAd, &SetHT);
	}
#else
	SetCommonHT(pAd);
#endif
#endif // CONFIG_AP_SUPPORT //
	
	printk("MCS Set = %02x %02x %02x %02x %02x\n", pAd->CommonCfg.HtCapability.MCSSet[0],
           pAd->CommonCfg.HtCapability.MCSSet[1], pAd->CommonCfg.HtCapability.MCSSet[2],
           pAd->CommonCfg.HtCapability.MCSSet[3], pAd->CommonCfg.HtCapability.MCSSet[4]);
	NICInitAsicFromEEPROM(pAd); //rt2860b


#if 0
		// Patch cardbus controller if EEPROM said so.
		if (pAd->bTest1 == FALSE)
			RTMPPatchCardBus(pAd);
#endif

//		APInitialize(pAd);

		//
		// Initialize RF register to default value
	//
	AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
		AsicLockChannel(pAd, pAd->CommonCfg.Channel);		

	// 8051 firmware require the signal during booting time.
	AsicSendCommandToMcu(pAd, 0x72, 0xff, 0x00, 0x00);

	if (pAd && (Status != NDIS_STATUS_SUCCESS))
	{
		BOOLEAN Cancelled;
		//
		// Undo everything if it failed
		//
#ifdef WIN_NDIS			
		if (RTMP_GET_REF(pAd) > 0)
		{
			RTMP_DEC_REF(pAd);
		}
#endif // WIN_NDIS //		

		//RTMPCancelTimer(&pAd->RfTuningTimer, &Cancelled);
		RTMPCancelTimer(&pAd->Mlme.PeriodicTimer, &Cancelled);
		RTMPCancelTimer(&pAd->Mlme.APSDPeriodicTimer, &Cancelled);

		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
		{
//			NdisMDeregisterInterrupt(&pAd->Interrupt);
			RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE);
		}
		RTMPFreeAdapter(pAd);
	}
	else if (pAd)
	{
		// Microsoft HCT require driver send a disconnect event after driver initialization.
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);
		pAd->IndicateMediaState = NdisMediaStateDisconnected;
		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_MEDIA_STATE_CHANGE);
	
		DBGPRINT(RT_DEBUG_TRACE, ("NDIS_STATUS_MEDIA_DISCONNECT Event B!\n"));

#ifdef CONFIG_AP_SUPPORT
		if (pAd->OpMode == OPMODE_AP)
		{
			if (pAd->ApCfg.bAutoChannelAtBootup || (pAd->CommonCfg.Channel == 0))
			{
				//
				// Enable Interrupt
				//
				//pAd->bStaFifoTest = TRUE;
				pAd->int_enable_reg = ((DELAYINTMASK)  | (RxINT|TxDataInt|TxMgmtInt)) & ~(0x03);
				pAd->int_disable_mask = 0;
				pAd->int_pending = 0;
			
				RTMP_IO_WRITE32(pAd, INT_SOURCE_CSR, 0xffffffff);  // clear garbage interrupts
				NICEnableInterrupt(pAd);
				// Now Enable RxTx
				//
				RTMPEnableRxTx(pAd);
				// APRxDoneInterruptHandle API will check this flag to decide accept incoming packet or not.
				// Set the flag be ready to receive Beacon frame for autochannel select.
				RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_START_UP);

				// Now we can receive the beacon and do the listen beacon
				pAd->CommonCfg.Channel = APAutoSelectChannel(pAd);
				//pAd->CommonCfg.Channel = New_ApAutoSelectChannel(pAd);
			}

{
			RTMPInitTimer(pAd, &PeriodicTimer, GET_TIMER_FUNCTION(APDetectOverlappingExec), pAd, TRUE);
			RTMPSetTimer(&PeriodicTimer, FORTY_MHZ_INTOLERANT_INTERVAL);
}

			// If phymode > PHY_11ABGN_MIXED and BW=40 check extension channel, after select channel  
			ApSelectChannelCheck(pAd);

			APStartUp(pAd);
			printk("Main bssid = %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(pAd->ApCfg.MBSSID[BSS0].Bssid));
		}
#endif // CONFIG_AP_SUPPORT //	

#ifdef WSC_INCLUDED
		NdisZeroMemory(&Wsc_Uuid_Str[0], sizeof(Wsc_Uuid_Str));
		NdisZeroMemory(&Wsc_Uuid_E[0], sizeof(Wsc_Uuid_E));
		WscGenerateUUID(pAd, &Wsc_Uuid_E[0], &Wsc_Uuid_Str[0], 0);

#ifdef CONFIG_AP_SUPPORT
        WscInit(pAd, FALSE, &pAd->ApCfg.MBSSID[BSS0].WscControl);
        if (pAd->ApCfg.MBSSID[BSS0].WscControl.WscEnrolleePinCode == 0)
            pAd->ApCfg.MBSSID[BSS0].WscControl.WscEnrolleePinCode = WscGeneratePinCode(pAd, FALSE, 0);
#endif // CONFIG_AP_SUPPORT //	


#endif // WSC_INCLUDED //

#if 0
		//
		// Enable Interrupt
		//
		RTMP_IO_WRITE32(pAd, INT_SOURCE_CSR, 0xffffffff);  // clear garbage interrupts
		NICEnableInterrupt(pAd);
		// Now Enable RxTx
		//
		RTMPEnableRxTx(pAd);
		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_START_UP);
#endif
	}// end of else

	DBGPRINT_S(Status, ("<==== RTMPInitialize, Status=%x\n", Status));

	return TRUE;


err4:
err3:
	MlmeHalt(pAd);
err2:
	RTMPFreeDMAMemory(pAd);
	RTMPFreeAdapter(pAd);
err1:

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	synchronize_irq(net_dev->irq);
#endif
	free_irq(net_dev->irq, net_dev);
	net_dev->priv = 0;

//	panic("!!! RT2860 Initialized fail !!!\n");
	printk("!!! RT2860 Initialized fail !!!\n");
	return FALSE;
}

void init_thread_task(IN PRTMP_ADAPTER pAd)
{
	POS_COOKIE pObj;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	tasklet_init(&pObj->rx_done_task, rx_done_tasklet, (unsigned long)pAd);
	tasklet_init(&pObj->mgmt_dma_done_task, mgmt_dma_done_tasklet, (unsigned long)pAd);
	tasklet_init(&pObj->ac0_dma_done_task, ac0_dma_done_tasklet, (unsigned long)pAd);
	tasklet_init(&pObj->ac1_dma_done_task, ac1_dma_done_tasklet, (unsigned long)pAd);
	tasklet_init(&pObj->ac2_dma_done_task, ac2_dma_done_tasklet, (unsigned long)pAd);
	tasklet_init(&pObj->ac3_dma_done_task, ac3_dma_done_tasklet, (unsigned long)pAd);
	tasklet_init(&pObj->hcca_dma_done_task, hcca_dma_done_tasklet, (unsigned long)pAd);
	tasklet_init(&pObj->tbtt_task, tbtt_tasklet, (unsigned long)pAd);
	tasklet_init(&pObj->fifo_statistic_full_task, fifo_statistic_full_tasklet, (unsigned long)pAd);
#ifdef CONFIG_AP_SUPPORT
#ifdef DFS_SUPPORT
	tasklet_init(&pObj->pulse_radar_detect_task, pulse_radar_detect_tasklet, (unsigned long)pAd);
	tasklet_init(&pObj->width_radar_detect_task, width_radar_detect_tasklet, (unsigned long)pAd);
#endif // DFS_SUPPORT //
#ifdef CARRIER_DETECTION_SUPPORT
	tasklet_init(&pObj->carrier_sense_task, carrier_sense_tasklet, (unsigned long)pAd);
#endif // CARRIER_DETECTION_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
}

void kill_thread_task(IN PRTMP_ADAPTER pAd)
{
	POS_COOKIE pObj;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;

	tasklet_kill(&pObj->rx_done_task);
	tasklet_kill(&pObj->mgmt_dma_done_task);
	tasklet_kill(&pObj->ac0_dma_done_task);
	tasklet_kill(&pObj->ac1_dma_done_task);
	tasklet_kill(&pObj->ac2_dma_done_task);
	tasklet_kill(&pObj->ac3_dma_done_task);
	tasklet_kill(&pObj->hcca_dma_done_task);
	tasklet_kill(&pObj->tbtt_task);
	tasklet_kill(&pObj->fifo_statistic_full_task);
#ifdef CONFIG_AP_SUPPORT
#ifdef CARRIER_DETECTION_SUPPORT
	tasklet_kill(&pObj->carrier_sense_task);
#endif // CARRIER_DETECTION_SUPPORT //
#ifdef DFS_SUPPORT
	tasklet_kill(&pObj->width_radar_detect_task);
	tasklet_kill(&pObj->pulse_radar_detect_task);
#endif // DFS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
}

int rt2860_packet_xmit(struct sk_buff *skb)
{
	struct net_device *net_dev = skb->dev;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) net_dev->priv;
	struct sk_buff *pkt = (struct sk_buff *) skb;
	int status = 0;
	PNDIS_PACKET pPacket = OSPKT_TO_RTPKT(pkt);


    // EapolStart size is 18
	if ((skb->len < 14) 
#ifdef RALINK_ATE
		|| (pAd->ate.Mode != ATE_STOP)
#endif // RALINK_ATE //
		)
	{
		//printk("bad packet size: %d\n", pkt->len);
		hex_dump("bad packet", skb->data, skb->len);
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		goto done;
	}
	
#if 0
//	if ((pkt->data[0] & 0x1) == 0)
	{
		//hex_dump(__FUNCTION__, pkt->data, pkt->len);
		printk("pPacket = %x\n", pPacket);
	}
#endif
		
	RTMP_SET_PACKET_5VT(pPacket, 0);
//	MiniportMMRequest(pAd, pkt->data, pkt->len);
#ifdef CONFIG_5VT_ENHANCE
    if (*(int*)(skb->cb) == BRIDGE_TAG) {
		RTMP_SET_PACKET_5VT(pPacket, 1);
    }
#endif


#ifdef CONFIG_AP_SUPPORT
	APSendPackets((NDIS_HANDLE)pAd, (PPNDIS_PACKET) &pPacket, 1);
#endif // CONFIG_AP_SUPPORT //


	status = 0;
done:
			   
	return status;
}

static int rt2860_open(struct net_device *net_dev)
{				 
	PRTMP_ADAPTER pAd= (PRTMP_ADAPTER)net_dev->priv;
	int retval = 0;
 	POS_COOKIE pObj;

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -1;
	}

 	pObj = (POS_COOKIE)pAd->OS_Cookie;
	
// reset Adapter flags
	RTMP_CLEAR_FLAGS(pAd);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    if (!try_module_get(THIS_MODULE))
    {
        DBGPRINT(RT_DEBUG_ERROR, ("%s: cannot reserve module\n", __FUNCTION__));
        return -1;
    }
#else
    MOD_INC_USE_COUNT;
#endif

	/* register the interrupt routine with the os */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    if ((retval = request_irq(pObj->pci_dev->irq, rt2860_interrupt, IRQF_SHARED, net_dev->name ,net_dev))) 
#else
	if ((retval = request_irq(pObj->pci_dev->irq,rt2860_interrupt, SA_SHIRQ, net_dev->name ,net_dev))) 
#endif
	{
		printk("RT2860: request_irq  ERROR(%d)\n", retval);
		return retval;
	}

	// Init BssTab & ChannelInfo tabbles for auto channel select.
#ifdef CONFIG_AP_SUPPORT	
	AutoChBssTableInit(pAd);
	ChannelInfoInit(pAd);
#endif // CONFIG_AP_SUPPORT //

	if (rt2860_init(net_dev) == FALSE)
		goto err;
	

	// Set up the Mac address
	NdisMoveMemory(net_dev->dev_addr, (void *) pAd->CurrentAddress, 6);

	//pAd->bStaFifoTest = TRUE;
	pAd->int_enable_reg = ((DELAYINTMASK)  | (RxINT|TxDataInt|TxMgmtInt)) & ~(0x03);
	pAd->int_disable_mask = 0;
	pAd->int_pending = 0;
	    // Start net interface tx /rx

#ifdef CONFIG_AP_SUPPORT
#ifdef MBSS_SUPPORT
	/* the function can not be moved to RT2860_probe() even register_netdev()
	   is changed as register_netdevice().
	   Or in some PC, kernel will panic (Fedora 4) */
    RT2860_MBSS_Init(pAd, net_dev);
#endif // MBSS_SUPPORT //

#ifdef WDS_SUPPORT
	RT2860_WDS_Init(pAd, net_dev);
#endif // WDS_SUPPORT //

#ifdef MAT_SUPPORT
	MATEngineInit();
#endif // MAT_SUPPORT //

#ifdef APCLI_SUPPORT
	RT2860_ApCli_Init(pAd, net_dev);
#endif // APCLI_SUPPORT //
#endif // CONFIG_AP_SUPPORT //


#if 1
		//
		// Enable Interrupt
		//
		RTMP_IO_WRITE32(pAd, INT_SOURCE_CSR, 0xffffffff);  // clear garbage interrupts
		NICEnableInterrupt(pAd);
		// Now Enable RxTx
		//

		RTMPEnableRxTx(pAd);
		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_START_UP);
#endif 


{
	ULONG reg;
	RTMP_IO_READ32(pAd, 0x1300, &reg);  // clear garbage interrupts
	printk("0x1300 = %08lx\n", reg);
}
    netif_start_queue(net_dev);
    netif_carrier_on(net_dev);
    netif_wake_queue(net_dev);


{
//	u32 reg;
//	u8  byte;
//	u16 tmp;

//	RTMP_IO_READ32(pAd, XIFS_TIME_CFG, &reg);

//	tmp = 0x0805;
//	reg  = (reg & 0xffff0000) | tmp;
//	RTMP_IO_WRITE32(pAd, XIFS_TIME_CFG, reg);

}

#if 0
	/* 
	 * debugging helper
	 * 		show the size of main table in Adapter structure
	 *		MacTab  -- 185K
	 *		BATable -- 137K
	 * 		Total 	-- 385K  !!!!! (5/26/2006)
	 */
	printk("sizeof(pAd->MacTab) = %ld\n", sizeof(pAd->MacTab));
	printk("sizeof(pAd->AccessControlList) = %ld\n", sizeof(pAd->AccessControlList));
	printk("sizeof(pAd->ApCfg) = %ld\n", sizeof(pAd->ApCfg));
	printk("sizeof(pAd->BATable) = %ld\n", sizeof(pAd->BATable));	
	BUG();
#endif 

	return (retval);

err:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    module_put(THIS_MODULE);
#else
    MOD_DEC_USE_COUNT;
#endif
	return (-1);
}

static void rt2860_int_enable(PRTMP_ADAPTER pAd, unsigned int mode)
{
	u32 regValue;

	pAd->int_disable_mask &= ~(mode);
	regValue = pAd->int_enable_reg & ~(pAd->int_disable_mask);		
	//if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
	{
		RTMP_IO_WRITE32(pAd, INT_MASK_CSR, regValue);     // 1:enable
	}
	//else
	//	DBGPRINT(RT_DEBUG_TRACE, ("fOP_STATUS_DOZE !\n"));

	if (regValue != 0)
		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_ACTIVE);
}


static void rt2860_int_disable(PRTMP_ADAPTER pAd, unsigned int mode)
{
	u32 regValue;

	pAd->int_disable_mask |= mode;
	regValue = 	pAd->int_enable_reg & ~(pAd->int_disable_mask);
	RTMP_IO_WRITE32(pAd, INT_MASK_CSR, regValue);     // 0: disable 
	
	if (regValue == 0)
	{
		RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_ACTIVE);		
	}
}

static void mgmt_dma_done_tasklet(unsigned long data)
{
	unsigned long flags;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
    INT_SOURCE_CSR_STRUC	IntSource;
	POS_COOKIE pObj;
	
    pObj = (POS_COOKIE) pAd->OS_Cookie;

//	printk("mgmt_dma_done_process\n");
	IntSource.word = 0;
	IntSource.field.MgmtDmaDone = 1;
	pAd->int_pending &= ~INT_MGMT_DLY;
	
	RTMPHandleMgmtRingDmaDoneInterrupt(pAd);

	RTMP_IRQ_LOCK(&pAd->irq_lock, flags);
	/*
	 * double check to avoid lose of interrupts
	 */
	if (pAd->int_pending & INT_MGMT_DLY) 
	{
		tasklet_hi_schedule(&pObj->mgmt_dma_done_task);
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
		return;
	}

	/* enable TxDataInt again */
	rt2860_int_enable(pAd, INT_MGMT_DLY);
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
}

static void rx_done_tasklet(unsigned long data)
{
	unsigned long flags;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
	BOOLEAN	bReschedule = 0;
	POS_COOKIE pObj;
	
    pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	pAd->int_pending &= ~(INT_RX);
#ifdef CONFIG_AP_SUPPORT	
//	if (pAd->OpMode == OPMODE_AP)
		bReschedule = APRxDoneInterruptHandle(pAd);
#endif // CONFIG_AP_SUPPORT //	

	RTMP_IRQ_LOCK(&pAd->irq_lock, flags);
	/*
	 * double check to avoid rotting packet 
	 */
	if (pAd->int_pending & INT_RX || bReschedule) 
	{
		tasklet_hi_schedule(&pObj->rx_done_task);
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
		return;
	}

	/* enable RxINT again */
	rt2860_int_enable(pAd, INT_RX);
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);

}

void fifo_statistic_full_tasklet(unsigned long data)
{
	unsigned long flags;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
	POS_COOKIE pObj;
	
    pObj = (POS_COOKIE) pAd->OS_Cookie;

	pAd->int_pending &= ~(FifoStaFullInt); 
	NICUpdateFifoStaCounters(pAd);
	
	RTMP_IRQ_LOCK(&pAd->irq_lock, flags);  
	/*
	 * double check to avoid rotting packet 
	 */
	if (pAd->int_pending & FifoStaFullInt) 
	{
		tasklet_hi_schedule(&pObj->fifo_statistic_full_task);
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
		return;
	}

	/* enable RxINT again */

	rt2860_int_enable(pAd, FifoStaFullInt);
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);

}

static void hcca_dma_done_tasklet(unsigned long data)
{
	unsigned long flags;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
    INT_SOURCE_CSR_STRUC	IntSource;
	POS_COOKIE pObj;
	
    pObj = (POS_COOKIE) pAd->OS_Cookie;


	IntSource.word = 0;
	IntSource.field.HccaDmaDone = 1;
	pAd->int_pending &= ~INT_HCCA_DLY;

	RTMPHandleTxRingDmaDoneInterrupt(pAd, IntSource);

	RTMP_IRQ_LOCK(&pAd->irq_lock, flags);
	/*
	 * double check to avoid lose of interrupts
	 */
	if (pAd->int_pending & INT_HCCA_DLY)
	{
		tasklet_hi_schedule(&pObj->hcca_dma_done_task);
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
		return;
	}

	/* enable TxDataInt again */
	rt2860_int_enable(pAd, INT_HCCA_DLY);
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
}

static void ac3_dma_done_tasklet(unsigned long data)
{
	unsigned long flags;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
    INT_SOURCE_CSR_STRUC	IntSource;
	POS_COOKIE pObj;
	BOOLEAN bReschedule = 0;

	
    pObj = (POS_COOKIE) pAd->OS_Cookie;

//	printk("ac0_dma_done_process\n");
	IntSource.word = 0;
	IntSource.field.Ac3DmaDone = 1;
	pAd->int_pending &= ~INT_AC3_DLY;

	bReschedule = RTMPHandleTxRingDmaDoneInterrupt(pAd, IntSource);

	RTMP_IRQ_LOCK(&pAd->irq_lock, flags);
	/*
	 * double check to avoid lose of interrupts
	 */
	if ((pAd->int_pending & INT_AC3_DLY) || bReschedule)
	{
		tasklet_hi_schedule(&pObj->ac3_dma_done_task);
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
		return;
	}

	/* enable TxDataInt again */
	rt2860_int_enable(pAd, INT_AC3_DLY);
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
}

static void ac2_dma_done_tasklet(unsigned long data)
{
	unsigned long flags;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
    INT_SOURCE_CSR_STRUC	IntSource;
	POS_COOKIE pObj;
	BOOLEAN bReschedule = 0;
	
    pObj = (POS_COOKIE) pAd->OS_Cookie;

	IntSource.word = 0;
	IntSource.field.Ac2DmaDone = 1;
	pAd->int_pending &= ~INT_AC2_DLY;

	bReschedule = RTMPHandleTxRingDmaDoneInterrupt(pAd, IntSource);

	RTMP_IRQ_LOCK(&pAd->irq_lock, flags);

	/*
	 * double check to avoid lose of interrupts
	 */
	if ((pAd->int_pending & INT_AC2_DLY) || bReschedule) 
	{
		tasklet_hi_schedule(&pObj->ac2_dma_done_task);
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
		return;
	}

	/* enable TxDataInt again */
	rt2860_int_enable(pAd, INT_AC2_DLY);
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
}

static void ac1_dma_done_tasklet(unsigned long data)
{
	unsigned long flags;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
    INT_SOURCE_CSR_STRUC	IntSource;
	POS_COOKIE pObj;
	BOOLEAN bReschedule = 0;

	
    pObj = (POS_COOKIE) pAd->OS_Cookie;

//	printk("ac0_dma_done_process\n");
	IntSource.word = 0;
	IntSource.field.Ac1DmaDone = 1;
	pAd->int_pending &= ~INT_AC1_DLY;

	bReschedule = RTMPHandleTxRingDmaDoneInterrupt(pAd, IntSource);

	RTMP_IRQ_LOCK(&pAd->irq_lock, flags);
	/*
	 * double check to avoid lose of interrupts
	 */
	if ((pAd->int_pending & INT_AC1_DLY) || bReschedule) 
	{
		tasklet_hi_schedule(&pObj->ac1_dma_done_task);
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
		return;
	}

	/* enable TxDataInt again */
	rt2860_int_enable(pAd, INT_AC1_DLY);
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
}

static void ac0_dma_done_tasklet(unsigned long data)
{
	unsigned long flags;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
    INT_SOURCE_CSR_STRUC	IntSource;
	POS_COOKIE pObj;
	BOOLEAN bReschedule = 0;

	
    pObj = (POS_COOKIE) pAd->OS_Cookie;

//	printk("ac0_dma_done_process\n");
	IntSource.word = 0;
	IntSource.field.Ac0DmaDone = 1;
	pAd->int_pending &= ~INT_AC0_DLY;

//	RTMPHandleMgmtRingDmaDoneInterrupt(pAd);
	bReschedule = RTMPHandleTxRingDmaDoneInterrupt(pAd, IntSource);
	
	RTMP_IRQ_LOCK(&pAd->irq_lock, flags);
	/*
	 * double check to avoid lose of interrupts
	 */
	if ((pAd->int_pending & INT_AC0_DLY) || bReschedule)
	{
		tasklet_hi_schedule(&pObj->ac0_dma_done_task);
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
		return;
	}

	/* enable TxDataInt again */
	rt2860_int_enable(pAd, INT_AC0_DLY);
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, flags);    
}

static void tbtt_tasklet(unsigned long data)
{
#ifdef CONFIG_AP_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;

	if (pAd->OpMode == OPMODE_AP)
	{
		//
		// step 7 - if DTIM, then move backlogged bcast/mcast frames from PSQ to TXQ whenever DtimCount==0
#if 0    
		// NOTE: This updated BEACON frame will be sent at "next" TBTT instead of at cureent TBTT. The reason is
		//       because ASIC already fetch the BEACON content down to TX FIFO before driver can make any
		//       modification. To compenstate this effect, the actual time to deilver PSQ frames will be
		//       at the time that we wrapping around DtimCount from 0 to DtimPeriod-1
		if ((pAd->ApCfg.DtimCount + 1) == pAd->ApCfg.DtimPeriod)
#else
		if (pAd->ApCfg.DtimCount == 0)
#endif
		{
			PQUEUE_ENTRY    pEntry;
			BOOLEAN			bPS = FALSE;

//			NdisAcquireSpinLock(&pAd->MacTabLock);
//			NdisAcquireSpinLock(&pAd->TxSwQueueLock);
			
			while (pAd->MacTab.McastPsQueue.Head)
			{
				bPS = TRUE;
                if (pAd->TxSwQueue[QID_AC_BE].Number <= (MAX_PACKETS_IN_QUEUE + (MAX_PACKETS_IN_MCAST_PS_QUEUE>>1))) 
				{
					pEntry = RemoveHeadQueue(&pAd->MacTab.McastPsQueue);
					if(pAd->MacTab.McastPsQueue.Number)
					{
						RTMP_SET_PACKET_MOREDATA(pEntry, TRUE);
					}
					InsertHeadQueue(&pAd->TxSwQueue[QID_AC_BE], pEntry);
				}
				else
				{
					break;
				}
			}
			DBGPRINT(RT_DEBUG_INFO, ("DTIM=%d/%d, tx mcast/bcast out...\n",pAd->ApCfg.DtimCount,pAd->ApCfg.DtimPeriod));
//			NdisReleaseSpinLock(&pAd->TxSwQueueLock);
//			NdisReleaseSpinLock(&pAd->MacTabLock);
			if (pAd->MacTab.McastPsQueue.Number == 0)
			{			
                UINT bss_index;

                /* clear MCAST/BCAST backlog bit for all BSS */
                for(bss_index=BSS0; bss_index<pAd->ApCfg.BssidNum; bss_index++)
					WLAN_MR_TIM_BCMC_CLEAR(bss_index);
                /* End of for */
			}
			pAd->MacTab.PsQIdleCount = 0;

			// Dequeue outgoing framea from TxSwQueue0..3 queue and process it
            if (bPS == TRUE) 
			{
				RTMPDeQueuePacket(pAd, TRUE, MAX_TX_IN_TBTT);
			}
		}
	}
#endif // CONFIG_AP_SUPPORT //
}



int print_int_count;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)   
irqreturn_t
#else
void 
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
rt2860_interrupt(int irq, void *dev_instance)
#else
rt2860_interrupt(int irq, void *dev_instance, struct pt_regs *regs)
#endif
{
	struct net_device *net_dev = (struct net_device *) dev_instance;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) net_dev->priv;
	INT_SOURCE_CSR_STRUC	IntSource;
	POS_COOKIE pObj;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;

	//
	// Inital the Interrupt source.
	//
	IntSource.word = 0x00000000L;
//	McuIntSource.word = 0x00000000L;



	//
	// Get the interrupt sources & saved to local variable
	//
	//RTMP_IO_READ32(pAd, where, &McuIntSource.word);
	//RTMP_IO_WRITE32(pAd, , McuIntSource.word);

	//
	// Flag fOP_STATUS_DOZE On, means ASIC put to sleep, elase means ASICK WakeUp
	// And at the same time, clock maybe turned off that say there is no DMA service.
	// when ASIC get to sleep. 
	// To prevent system hang on power saving.
	// We need to check it before handle the INT_SOURCE_CSR, ASIC must be wake up.
	//
	// RT2661 => when ASIC is sleeping, MAC register cannot be read and written.
	// RT2860 => when ASIC is sleeping, MAC register can be read and written.
//	if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
	{
		RTMP_IO_READ32(pAd, INT_SOURCE_CSR, &IntSource.word);
		RTMP_IO_WRITE32(pAd, INT_SOURCE_CSR, IntSource.word); // write 1 to clear
	}
//	else
//		DBGPRINT(RT_DEBUG_TRACE, (">>>fOP_STATUS_DOZE<<<\n"));

//	RTMP_IO_READ32(pAd, INT_SOURCE_CSR, &IsrAfterClear);
//	RTMP_IO_READ32(pAd, MCU_INT_SOURCE_CSR, &McuIsrAfterClear);
//	DBGPRINT(RT_DEBUG_INFO, ("====> RTMPHandleInterrupt(ISR=%08x,Mcu ISR=%08x, After clear ISR=%08x, MCU ISR=%08x)\n",
//			IntSource.word, McuIntSource.word, IsrAfterClear, McuIsrAfterClear));

	//
	// Handle interrupt, walk through all bits
	// Should start from highest priority interrupt
	// The priority can be adjust by altering processing if statement
	//
#if 0
{
	u32 regValue;

	RTMP_IO_READ32(pAd, MAC_SYS_CTRL, &regValue);
	printk("MAC_SYS_CTRL = %08x\n", regValue);

	RTMP_IO_READ32(pAd, 0x400, &regValue);
	printk("MCU 0x400 = %08x\n", regValue);

	RTMP_IO_READ32(pAd, 0x404, &regValue);
	printk("MCU 0x404 = %08x\n", regValue);

	RTMP_IO_READ32(pAd, 0x408, &regValue);
	printk("MCU 0x408 = %08x\n", regValue);

	RTMP_IO_READ32(pAd, 0x40C, &regValue);
	printk("MCU 0x40C = %08x\n", regValue);

	RTMP_IO_READ32(pAd, 0x410, &regValue);
	printk("MCU 0x410 = %08x\n", regValue);

	RTMP_IO_READ32(pAd, 0x414, &regValue);
	printk("MCU_INT_STA = %08x\n", regValue);
}
#endif 

#ifdef DBG
	if ((RTDebugLevel == RT_DEBUG_LOUD) && (((++print_int_count) % 100) == 0))
	{
		ULONG reg;
		int Count, free;

		RTMP_IO_READ32(pAd, INT_MASK_CSR, &reg);     // 1:enable
		printk("%d: INT_MASK_CSR = %08lx, IntSource %08x\n", print_int_count, reg, IntSource.word);
		RTMP_IO_READ32(pAd, TX_CTX_IDX0 + 0 * 0x10 , &reg);
		printk("TX_CTX_IDX0 = %08lx\n", reg);
		RTMP_IO_READ32(pAd, TX_DTX_IDX0 + 0 * 0x10 , &reg);
		printk("TX_DTX_IDX0 = %08lx\n", reg);
		RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &reg);
		printk("WPDMA_GLO_CFG = %08lx\n", reg);
		for (Count = 0; Count < 1; Count++)
		{
			if (pAd->TxRing[Count].TxSwFreeIdx> pAd->TxRing[Count].TxCpuIdx)
				free = pAd->TxRing[Count].TxSwFreeIdx - pAd->TxRing[Count].TxCpuIdx -1;
			else
				free = pAd->TxRing[Count].TxSwFreeIdx + TX_RING_SIZE - pAd->TxRing[Count].TxCpuIdx -1;
		
			printk("%d: Free = %d TxSwFreeIdx = %ld\n", Count, free, pAd->TxRing[Count].TxSwFreeIdx); 
		}
		printk("pAd->int_disable_mask = %08x\n", pAd->int_disable_mask);
		printk("pAd->int_enable_reg = %08x\n", pAd->int_enable_reg);
		printk("pAd->int_pending = %08x\n", pAd->int_pending);
		RTMP_IO_READ32(pAd, RX_DRX_IDX , &reg);
		printk("pAd->RxRing.RxSwReadIdx = %08lx, RX_DRX_IDX = %08lx\n", pAd->RxRing.RxSwReadIdx, reg);
//		NetJobAdd(APRxDoneIntProcess, pAd, 0, 0);
	}
#endif
		

	// If required spinlock, each interrupt service routine has to acquire
	// and release itself.
	//
	
	if (IntSource.word & TxCoherent)
	{
		DBGPRINT(RT_DEBUG_ERROR, (">>>TxCoherent<<<\n"));
		RTMPHandleRxCoherentInterrupt(pAd);
	}

	if (IntSource.word & RxCoherent)
	{
		DBGPRINT(RT_DEBUG_ERROR, (">>>RxCoherent<<<\n"));
		RTMPHandleRxCoherentInterrupt(pAd);
	}

	if (IntSource.word & FifoStaFullInt) 
	{
#if 1
		if ((pAd->int_disable_mask & FifoStaFullInt) == 0) 
		{
			/* mask FifoStaFullInt */
			rt2860_int_disable(pAd, FifoStaFullInt);
			tasklet_hi_schedule(&pObj->fifo_statistic_full_task);
		}
		pAd->int_pending |= FifoStaFullInt; 
#else
		NICUpdateFifoStaCounters(pAd);		
#endif
	}

	if (IntSource.word & INT_MGMT_DLY) 
	{
		if ((pAd->int_disable_mask & INT_MGMT_DLY) ==0 )
		{
			rt2860_int_disable(pAd, INT_MGMT_DLY);
			tasklet_hi_schedule(&pObj->mgmt_dma_done_task);			
		}
		//RTMPHandleMgmtRingDmaDoneInterrupt(pAd);
		pAd->int_pending |= INT_MGMT_DLY ;
	}

	if (IntSource.word & INT_RX)
	{
		if ((pAd->int_disable_mask & INT_RX) == 0) 
		{
			/* mask RxINT */
			rt2860_int_disable(pAd, INT_RX);
			tasklet_hi_schedule(&pObj->rx_done_task);
		}
		pAd->int_pending |= INT_RX; 		
	}

	if (IntSource.word & INT_HCCA_DLY)
	{

		if ((pAd->int_disable_mask & INT_HCCA_DLY) == 0) 
		{
			/* mask TxDataInt */
			rt2860_int_disable(pAd, INT_HCCA_DLY);
			tasklet_hi_schedule(&pObj->hcca_dma_done_task);
		}
		pAd->int_pending |= INT_HCCA_DLY;						
	}

	if (IntSource.word & INT_AC3_DLY)
	{

		if ((pAd->int_disable_mask & INT_AC3_DLY) == 0) 
		{
			/* mask TxDataInt */
			rt2860_int_disable(pAd, INT_AC3_DLY);
			tasklet_hi_schedule(&pObj->ac3_dma_done_task);
		}
		pAd->int_pending |= INT_AC3_DLY;						
	}

	if (IntSource.word & INT_AC2_DLY)
	{

		if ((pAd->int_disable_mask & INT_AC2_DLY) == 0) 
		{
			/* mask TxDataInt */
			rt2860_int_disable(pAd, INT_AC2_DLY);
			tasklet_hi_schedule(&pObj->ac2_dma_done_task);
		}
		pAd->int_pending |= INT_AC2_DLY;						
	}

	if (IntSource.word & INT_AC1_DLY)
	{

		pAd->int_pending |= INT_AC1_DLY;						

		if ((pAd->int_disable_mask & INT_AC1_DLY) == 0) 
		{
			/* mask TxDataInt */
			rt2860_int_disable(pAd, INT_AC1_DLY);
			tasklet_hi_schedule(&pObj->ac1_dma_done_task);
		}
		
	}

	if (IntSource.word & INT_AC0_DLY)
	{

/*
		if (IntSource.word & 0x2) {
			u32 reg;
			RTMP_IO_READ32(pAd, DELAY_INT_CFG, &reg);
			printk("IntSource.word = %08x, DELAY_REG = %08x\n", IntSource.word, reg);
		}
*/
		pAd->int_pending |= INT_AC0_DLY;

		if ((pAd->int_disable_mask & INT_AC0_DLY) == 0) 
		{
			/* mask TxDataInt */
			rt2860_int_disable(pAd, INT_AC0_DLY);
			tasklet_hi_schedule(&pObj->ac0_dma_done_task);
		}
								
	}

    if (IntSource.word & PreTBTTInt)
	{
		RTMPHandlePreTBTTInterrupt(pAd);
	}

	if (IntSource.word & TBTTInt)
	{
		RTMPHandleTBTTInterrupt(pAd);
	}

#ifdef CONFIG_AP_SUPPORT
	if (IntSource.word & McuCommand)
	{
		UINT32 McuIntSrc;

		RTMP_IO_READ32(pAd, 0x7024, &McuIntSrc);

		// clear MCU Int source register.
		RTMP_IO_WRITE32(pAd, 0x7024, 0);

#ifdef DFS_SUPPORT
		if (pAd->CommonCfg.bIEEE80211H)
		{
			// pulse radar signal Int.
			if (McuIntSrc & 0x40)
			{
				tasklet_hi_schedule(&pObj->pulse_radar_detect_task);
			}

			// width radar signal Int.
			if(((pAd->CommonCfg.RadarDetect.RDDurRegion == FCC)
					|| (pAd->CommonCfg.RadarDetect.RDDurRegion == JAP_W56))
				&& (McuIntSrc & 0x80))
			{
				tasklet_hi_schedule(&pObj->width_radar_detect_task);
			}

			// long pulse radar signal detection.
			if(((pAd->CommonCfg.RadarDetect.RDDurRegion == FCC)
					|| (pAd->CommonCfg.RadarDetect.RDDurRegion == JAP_W56)
					|| (JapRadarType(pAd) == JAP_W56))
			&& (McuIntSrc & 0x10))
			{
				tasklet_hi_schedule(&pObj->width_radar_detect_task);
			}

		}
#endif // DFS_SUPPORT //
#ifdef CARRIER_DETECTION_SUPPORT
		if ((pAd->CommonCfg.CarrierDetect.Enable)
			&& (McuIntSrc & 0x04))
		{
			tasklet_hi_schedule(&pObj->carrier_sense_task);
		}
#endif // CARRIER_DETECTION_SUPPORT //
	}
#endif // CONFIG_AP_SUPPORT //



	// Do nothing if Reset in progress
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
		RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
        return  IRQ_HANDLED;
#else
        return;
#endif
	}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    return  IRQ_HANDLED;
#endif
	
}

/*
========================================================================
Routine Description:
    Send a packet to WLAN.

Arguments:
    skb_p           points to our adapter
    dev_p           which WLAN network interface

Return Value:
    0: transmit successfully
    otherwise: transmit fail

Note:
========================================================================
*/
static INT rt2860_send_packets(
	IN struct sk_buff 		*skb_p, 
	IN struct net_device 	*net_dev)
{
	RTMP_SET_PACKET_NET_DEVICE_MBSSID(skb_p, MAIN_MBSSID);

	/* transmit the packet */
	return rt2860_packet_xmit(skb_p);
} /* End of MBSS_VirtualIF_PacketSend */



/*
    ========================================================================

    Routine Description:
        return ethernet statistics counter

    Arguments:
        net_dev                     Pointer to net_device

    Return Value:
        net_device_stats*

    Note:

    ========================================================================
*/
struct net_device_stats *rt2860_get_ether_stats(
    IN  struct net_device *net_dev)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) net_dev->priv;
    
    DBGPRINT(RT_DEBUG_INFO, ("rt2860_get_ether_stats --->\n"));

    pAd->stats.rx_packets = pAd->WlanCounters.ReceivedFragmentCount.u.LowPart;        // total packets received
    pAd->stats.tx_packets = pAd->WlanCounters.TransmittedFragmentCount.u.LowPart;     // total packets transmitted

    pAd->stats.rx_bytes = pAd->RalinkCounters.ReceivedByteCount;             // total bytes received
    pAd->stats.tx_bytes = pAd->RalinkCounters.TransmittedByteCount;         // total bytes transmitted

    pAd->stats.rx_errors = pAd->Counters8023.RxErrors;                          // bad packets received
    pAd->stats.tx_errors = pAd->Counters8023.TxErrors;                          // packet transmit problems

    pAd->stats.rx_dropped = pAd->Counters8023.RxNoBuffer;                       // no space in linux buffers
    pAd->stats.tx_dropped = pAd->WlanCounters.FailedCount.u.LowPart;                  // no space available in linux

    pAd->stats.multicast = pAd->WlanCounters.MulticastReceivedFrameCount.u.LowPart;   // multicast packets received
    pAd->stats.collisions = pAd->Counters8023.OneCollision + pAd->Counters8023.MoreCollisions;  // Collision packets

    pAd->stats.rx_length_errors = 0;
    pAd->stats.rx_over_errors = pAd->Counters8023.RxNoBuffer;                   // receiver ring buff overflow
    pAd->stats.rx_crc_errors = 0;//pAd->WlanCounters.FCSErrorCount;     // recved pkt with crc error
    pAd->stats.rx_frame_errors = pAd->Counters8023.RcvAlignmentErrors;          // recv'd frame alignment error
    pAd->stats.rx_fifo_errors = pAd->Counters8023.RxNoBuffer;                   // recv'r fifo overrun
    pAd->stats.rx_missed_errors = 0;                                            // receiver missed packet

    // detailed tx_errors
    pAd->stats.tx_aborted_errors = 0;
    pAd->stats.tx_carrier_errors = 0;
    pAd->stats.tx_fifo_errors = 0;
    pAd->stats.tx_heartbeat_errors = 0;
    pAd->stats.tx_window_errors = 0;

    // for cslip etc
    pAd->stats.rx_compressed = 0;
    pAd->stats.tx_compressed = 0;

    return &pAd->stats;
}

/*
    ========================================================================

    Routine Description:
        Set to filter multicast list

    Arguments:
        net_dev                     Pointer to net_device

    Return Value:
        VOID

    Note:

    ========================================================================
*/
static VOID rt2860_set_rx_mode(
    IN  struct net_device *net_dev)
{
	INT i;
	VIRTUAL_ADAPTER *pVirtualAd;
	RTMP_ADAPTER *pAd = net_dev->priv;
	UCHAR mfilterAddr[MAC_ADDR_LEN];

	// determine this ioctl command is comming from which interface.
	if (net_dev->priv_flags == INT_MAIN)
	{
		pAd= net_dev->priv;
	}
	else if ((net_dev->priv_flags == INT_MBSSID)
			|| (net_dev->priv_flags == INT_APCLI))
	{
		pVirtualAd = net_dev->priv;
		pAd = pVirtualAd->RtmpDev->priv;
	}

	/* Note: do not reorder, GCC is clever about common statements. */
	if ((net_dev->flags & IFF_PROMISC)
		|| (net_dev->flags & IFF_ALLMULTI))
	{
		/* Unconditionally log net taps. */
		DBGPRINT (RT_DEBUG_INFO, ("%s: Promiscuous or Accept-All_Multicase mode enabled.\n", net_dev->name));
	} else if (net_dev->flags & IFF_UP)
	{
		struct dev_mc_list *mclist;

		for (i = 0, mclist = net_dev->mc_list; mclist && i < net_dev->mc_count;
			i++, mclist = mclist->next)
		{
			COPY_MAC_ADDR(mfilterAddr, mclist->dmi_addr);

#ifdef CONFIG_AP_SUPPORT
#ifdef IGMP_SNOOP_SUPPORT
			MulticastFilterTableInsertEntry(pAd, mfilterAddr, NULL, net_dev, MCAT_FILTER_STATIC);
#endif // IGMP_SNOOP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
			DBGPRINT(RT_DEBUG_INFO, ("%s (%2x:%2x:%2x:%2x:%2x:%2x)\n",
				net_dev->name, mfilterAddr[0], mfilterAddr[1], mfilterAddr[2],
				mfilterAddr[3], mfilterAddr[4], mfilterAddr[5]));
		}
	}
	return;
}

#ifdef CONFIG_AP_SUPPORT
#ifdef DFS_SUPPORT
static void pulse_radar_detect_tasklet(unsigned long data)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
	POS_COOKIE pObj;
	
    pObj = (POS_COOKIE) pAd->OS_Cookie;

	RadarSMDetect(pAd, RADAR_PULSE);
}

static void width_radar_detect_tasklet(unsigned long data)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
	POS_COOKIE pObj;
	
    pObj = (POS_COOKIE) pAd->OS_Cookie;

	RadarSMDetect(pAd, RADAR_WIDTH);
}
#endif // DFS_SUPPORT //

#ifdef CARRIER_DETECTION_SUPPORT
static void carrier_sense_tasklet(unsigned long data)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) data;
	POS_COOKIE pObj;
	
    pObj = (POS_COOKIE) pAd->OS_Cookie;

	CarrierDetectionCheck(pAd);
}
#endif // CARRIER_DETECTION_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

