/*
 * Copyright(c) 2006-2007, Works Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution. 
 * 3. Neither the name of the vendors nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * $Author: joinsonj $ 
 * $Date: 2007-06-08 02:14:47 $
 */


//LANDevice
int dev_LANDevice_Ethnum(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int landevice_ethnum;
	
    if (!opt_flag) // get function
    {
        //Call native function to get landevice_ethnum -- TO DO
        landevice_ethnum = 2;
        *((unsigned int *)mthdval_struct) = landevice_ethnum;
        printf("The value of LANEthernetInterfaceNumberOfEntries is %d.\n", landevice_ethnum); 
        return 0;
    }
    else
    {           // set function
    	printf("LANEthernetInterfaceNumberOfEntries is unwritable.\n");
        return -1;
    }
}
int dev_LANDevice_USBnum(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int landevice_usbnum;
	
    if (!opt_flag) // get function
    {
        //Call native function to get landevice_usbnum -- TO DO
        landevice_usbnum = 2;
        *((unsigned int *)mthdval_struct) = landevice_usbnum;
        printf("LANUSBInterfaceNumberOfEntries is %d.\n", landevice_usbnum);
        return 0;
    }
    else           // set function
    {
        printf("LANUSBInterfaceNumberOfEntries is unwritable.\n");
        return -1;
    }
}
int dev_LANDevice_WLANnum(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int landevice_wlannum;
	
    if (!opt_flag) // get function
    {
    	//Call native function to get landevice_wlannum -- TO DO
    	landevice_wlannum = 2;
    	*((unsigned int *)mthdval_struct) = landevice_wlannum;
        printf("LANWLANConfigurationNumberOfEntries is %d.\n", landevice_wlannum);
        return 0;
    }
    else           // set function
    {
    	printf("LANWLANConfigurationNumberOfEntries is unwritable.\n"); 
        return -1;
    }
}

//LANDevice.Hosts.
int dev_LAN_Hosts_Hostnum(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int lan_hosts_hostnum;
    	
    if (!opt_flag) // get function
    {
        //Call native function to get lan_hosts_hostnum -- TO DO
        lan_hosts_hostnum = 2;
    	*((unsigned int *)mthdval_struct) = lan_hosts_hostnum;
        printf("HostNumberOfEntries is %d.\n", lan_hosts_hostnum);
        return 0;
    }
    else           // set function
    {
        printf("HostNumberOfEntries is unwritable.\n");
        return -1;
    }
}

//LANDevice.Hosts.Host.
int dev_LAN_Hosts_Host_IPAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LAN_Hosts_Host_IPAddress -- TO DO	
        strcpy((char *)mthdval_struct, "192.168.0.2");
        printf("The LAN_Hosts_Host_IPAddress is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("LAN_Hosts_Host_IPAddress is unwritable.\n");
        return -1;
    }
}
int dev_LAN_Hosts_Host_AddressSource(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LAN_Hosts_Host_AddressSource -- TO DO
        strcpy((char *)mthdval_struct, "DHCP");
        printf("The LAN_Hosts_Host_AddressSource is %s.\n", (char *)mthdval_struct);	
        return 0;
    }
    else           // set function
    {
        printf("LAN_Hosts_Host_AddressSource is unwritable.\n");
        return -1;
    }
}
int dev_LAN_Hosts_Host_MACAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LAN_Hosts_Host_MACAddress -- TO DO
        strcpy((char *)mthdval_struct, "192.168.0.3");
        printf("The LAN_Hosts_Host_MACAddress is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("LAN_Hosts_Host_MACAddress is unwritable.\n");
        return -1;
    }
}
int dev_LAN_Hosts_Host_LeaseTimeRemaining(int opt_flag, void *mthdval_struct, int locate[])
{
	int lan_hosts_host_leasetimeremaining;
	
    if (!opt_flag) // get function
    {
        //Call native function to get lan_hosts_host_leasetimeremaining -- TO DO
        lan_hosts_host_leasetimeremaining = -1;
        *((int *)mthdval_struct) = lan_hosts_host_leasetimeremaining;
        printf("The LAN_Hosts_Host_MACAddress is %d.\n", (int *)mthdval_struct);	
        return 0;
    }
    else           // set function
    {
        printf("lan_hosts_host_leasetimeremaining is unwritable.\n");
        return -1;
    }
}
int dev_LAN_Hosts_Host_HostName(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LAN_Hosts_Host_HostName -- TO DO
        strcpy((char *)mthdval_struct, "ADSL-1");
        printf("The LAN_Hosts_Host_HostName is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("LAN_Hosts_Host_HostName is unwritable.\n");
        return -1;
    }
}
int dev_LAN_Hosts_Host_InterfaceType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LAN_Hosts_Host_InterfaceType -- TO DO
        strcpy((char *)mthdval_struct, "Ethernet");
        printf("The LAN_Hosts_Host_InterfaceType is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("LAN_Hosts_Host_InterfaceType is unwritable.\n");
        return -1;
    }
}
int dev_LAN_Hosts_Host_Active(int opt_flag, void *mthdval_struct, int locate[])
{
    int lan_hosts_host_active;
    
    if (!opt_flag) // get function
    {
        //Call native function to get lan_hosts_host_active -- TO DO
        lan_hosts_host_active = 0;
        *((int *)mthdval_struct) = lan_hosts_host_active;
        printf("lan_hosts_host_active is %d.\n", lan_hosts_host_active);
        return 0;
    }
    else           // set function
    {
        printf("lan_hosts_host_active is unwritable.\n");
        return -1;
    }
}

//LANDevice.LANHostConfigManagement
int dev_LANHost_DHCPServerConfigurable(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get lanhost_dhcpserverconfig -- TO DO
            *((int*)mthdval_struct)= 0;
            printf("lanhost_dhcpserverconfigurable is %d.\n");
            return 0;
    }
    else           // set function
    {
        //Call native function to set lanhost_dhcpserverconfigurable -- TO DO
            printf("Set lanhost_dhcpserverenable successfully.\n");
    }

}
int dev_LANHost_DHCPServerEnable(int opt_flag, void *mthdval_struct, int locate[])
{

    if (!opt_flag) // get function
    {
        //Call native function to get lanhost_dhcpserverenable -- TO DO
            *((int *)mthdval_struct) = 1;
            printf("lanhost_dhcpserverenable is %d.\n", *((int *)mthdval_struct));
            return 0;
    }
    else           // set function
    {
        //Call native function to set lanhost_dhcpserverenable -- TO DO
            printf("Set lanhost_dhcpserverenable successfully.\n");
            return 0;
    }
}
int dev_LANHost_DHCPRelay(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get lanhost_dhcpserverenable -- TO DO
            *((int *)mthdval_struct) = 2;
            printf("lanhost_dhcpserverenable is %d.\n", *((int *)mthdval_struct));
            return 0;
    }
    else           // set function
    {
        //Call native function to set lanhost_dhcpserverenable -- TO DO
            printf("Set lanhost_dhcpserverenable successfully.\n");
            return 0;
    }
}

int dev_LANHost_MinAddress(int opt_flag, void *mthdval_struct, int locate[])
{
                                                                                
    if (!opt_flag) // get function
    {
        //Call native function to get lanhost_dhcpserverenable -- TO DO
            strcpy(((char *)mthdval_struct), "172.31.0.122");
            printf("lanhost_startAddress is %s\n", mthdval_struct);
            return 0;
    }
    else           // set function
    {
        //Call native function to set lanhost_dhcpserverenable -- TO DO
            printf("Set lanhost_startAddress successfully.\n");
            return 0;
    }
}

int dev_LANHost_MaxAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get lanhost_dhcpserverenable -- TO DO
            strcpy(((char *)mthdval_struct), "172.31.0.123");
            printf("lanhost_endaddress is %s\n", mthdval_struct);
            return 0;
    }
    else           // set function
    {
        //Call native function to set lanhost_dhcpserverenable -- TO DO
            printf("Set lanhost_endaddress successfully.\n");
            return 0;
    }
}

int dev_LANHost_ReservedAddresses(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LANHost_ReservedAddresses -- TO DO
        strcpy((char *)mthdval_struct, "192.168.2.1");
        printf("The LANHost_ReservedAddresses is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LANHost_ReservedAddresses -- TO DO
        printf("Set LANHost_ReservedAddresses successfully.\n");
        return 0;
    }
}
int dev_LANHost_SubnetMask(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LANHost_SubnetMask -- TO DO
        strcpy((char *)mthdval_struct, "255.255.255.0");
        printf("The LANHost_SubnetMask is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LANHost_SubnetMask -- TO DO
        printf("Set LANHost_SubnetMask successfully.\n");
        return 0;
    }
}
int dev_LANHost_DNSServers(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LANHost_DNSServers -- TO DO
        strcpy((char *)mthdval_struct, "192.158.54.87");
        printf("The LANHost_DNSServers is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LANHost_DNSServers -- TO DO
        printf("Set LANHost_DNSServers successfully.\n");
        return 0;
    }
}
int dev_LANHost_DomainName(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LANHost_DomainName -- TO DO
        strcpy((char *)mthdval_struct, "Workgroup");
        printf("The LANHost_DomainName is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LANHost_DomainName -- TO DO
        printf("Set LANHost_DomainName successfully.\n");
        return 0;
    }
}
int dev_LANHost_IPRouters(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LANHost_IPRouters -- TO DO
        strcpy((char *)mthdval_struct, "10.10.10.10");
        printf("LANHost_IPRouters is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LANHost_IPRouters -- TO DO
        printf("Set LANHost_IPRouters successfully.\n");
        return 0;
    }
}
int dev_LANHost_DHCPLeaseTime(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get lanhost_dhcpserverenable -- TO DO
            *(int *)mthdval_struct = 360001;
            printf("lanhost_dhcp lease time is %d\n", mthdval_struct);
            return 0;
    }
    else           // set function
    {
        //Call native function to set lanhost_dhcpserverenable -- TO DO
            //printf("Set lanhost_leasedTime successfully.\n");
            return 0;
    }
}   
int dev_LANHost_UseAllocatedWAN(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LANHost_UseAllocatedWAN -- TO DO
        strcpy((char *)mthdval_struct, "Normal");
        printf("LANHost_UseAllocatedWAN is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LANHost_UseAllocatedWAN -- TO DO
        printf("Set LANHost_UseAllocatedWAN successfully.\n");
        return 0;
    }
}
int dev_LANHost_AssociatedConnection(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LANHost_AssociatedConnection -- TO DO
        strcpy((char *)mthdval_struct, "InternetGatewayDevice.WANDevie");
        printf("LANHost_AssociatedConnection is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LANHost_AssociatedConnection -- TO DO
        printf("Set LANHost_AssociatedConnection successfully.\n");
        return 0;
    }
}
int dev_LANHost_PassthroughLease(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int lanhost_passthroughlease;
    
    if (!opt_flag) // get function
    {
        //Call native function to get lanhost_passthroughlease -- TO DO
        lanhost_passthroughlease = -1;
        *((unsigned int *)mthdval_struct) = lanhost_passthroughlease;
        printf("lanhost_passthroughlease is %d.\n", lanhost_passthroughlease);
        return 0;
    }
    else           // set function
    {
        //Call native function to set lanhost_passthroughlease -- TO DO
        printf("Set lanhost_passthroughlease successfully.\n");
        return 0;
    }
}
int dev_LANHost_PassthroughMACAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LANHost_PassthroughMACAddres -- TO DO
        strcpy((char *)mthdval_struct, "192.168.0.6");
        printf("LANHost_PassthroughMACAddres is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LANHost_PassthroughMACAddres -- TO DO
        printf("Set LANHost_PassthroughMACAddres successfully.\n");
        return 0;
    }
}
int dev_LANHost_AllowedMACAddresses(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LANHost_AllowedMACAddresses -- TO DO
        strcpy((char *)mthdval_struct, "192.168.0.5");
        printf("LANHost_AllowedMACAddresses is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LANHost_AllowedMACAddresses -- TO DO
        printf("Set LANHost_AllowedMACAddresses successfully.\n");
        return 0;
    }
}
int dev_LANHost_IPnum(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int lanhost_ipnum;
    if (!opt_flag) // get function
    {
        //Call native function to get lanhost_ipnum -- TO DO
        lanhost_ipnum = -1;
        *((unsigned int *)mthdval_struct) = lanhost_ipnum;
        printf("lanhost_ipnum is %d.\n", lanhost_ipnum);
        return 0;
    }
    else           // set function
    {
        printf("lanhost_ipnum is unwritable.\n");
        return -1;
    }
}

//LANDevice.LANHostConfigManagement.IPInterface
int dev_LH_IPInterface_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    int lh_ipinterface_enable;
    
    if (!opt_flag) // get function
    {
        //Call native function to get lh_ipinterface_enable -- TO DO
        lh_ipinterface_enable = 0;
        *((int *)mthdval_struct) = lh_ipinterface_enable;
        printf("lh_ipinterface_enable %d.\n", lh_ipinterface_enable);
        return 0;
    }
    else           // set function
    {
        //Call native function to set lh_ipinterface_enable -- TO DO
        printf("Set lh_ipinterface_enable successfully.\n");
        return 0;
    }
}
int dev_LH_IPInterface_IPAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LH_IPInterface_IPAddress -- TO DO
            strcpy((char *)mthdval_struct, "172.31.0.6");
            printf("LH_IPInterface_IPAddress is %s.\n", (char *)mthdval_struct);
	    return 0;
    }
    else           // set function
    {
        //Call native function to set LH_IPInterface_IPAddress -- TO DO
	    //printf("Set LH_IPInterface_IPAddress successfully.\n");
            return 0;
    }
}
int dev_LH_IPInterface_SubnetMask(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LH_IPInterface_IPAddress -- TO DO
            strcpy((char *)mthdval_struct,"255.255.255.0");
            printf("LH_IPInterface_subnet mask is %s.\n", (char *)mthdval_struct);
            return 0;
    }
    else           // set function
    {
        //Call native function to set LH_IPInterface_IPAddress -- TO DO
            printf("Set LH_IPInterface_SubnetMask successfully.\n");
            return 0;
    }
}
int dev_LH_IPInterface_AddressingType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get LH_IPInterface_AddressingType -- TO DO
        strcpy((char *)mthdval_struct, "DHCP");
        printf("LH_IPInterface_AddressingType is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set LH_IPInterface_AddressingType -- TO DO
        printf("Set LH_IPInterface_AddressingType successfully.\n");
        return 0;
    }
}

//LANDevice.LANEthernetInterfaceConfig
int dev_Eth_config_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    int eth_config_enable;
    
    if (!opt_flag) // get function
    {
        //Call native function to get eth_config_enable -- TO DO
        eth_config_enable = 0;
        *((int *)mthdval_struct) = eth_config_enable;
        printf("eth_config_enable is %d.\n", eth_config_enable);
        return 0;
    }
    else           // set function
    {
        //Call native function to set eth_config_enable -- TO DO
        printf("Set eth_config_enable successfully.\n");
        return 0;
    }
}
int dev_Eth_config_Status(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get Eth_config_Status -- TO DO
        strcpy((char *)mthdval_struct, "UP");
        printf("Eth_config_Status is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("Eth_config_Status is unwritable.\n");
        return -1;
    }
}
int dev_Eth_config_MACAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get Eth_config_MACAddress -- TO DO
        strcpy((char *)mthdval_struct, "192.168.0.8");
        printf("Eth_config_MACAddress is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        printf("Eth_config_MACAddress is unwritable.\n");
        return -1;
    }
}
int dev_Eth_config_MACControlEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    int eth_config_maccontrolenabled;
    
    if (!opt_flag) // get function
    {
        //Call native function to get eth_config_maccontrolenabled -- TO DO
        eth_config_maccontrolenabled = 0;
        *((int *)mthdval_struct) = eth_config_maccontrolenabled;
        printf("eth_config_maccontrolenabled is %d.\n", eth_config_maccontrolenabled);
        return 0;
    }
    else           // set function
    {
        //Call native function to set eth_config_maccontrolenabled -- TO DO
        printf("Set eth_config_maccontrolenabled successfully.\n");
        return 0;
    }
}
int dev_Eth_config_MaxBitRate(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get Eth_config_MaxBitRate -- TO DO
        strcpy((char *)mthdval_struct, "100");
        printf("Eth_config_MaxBitRate is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set Eth_config_MaxBitRate -- TO DO
        printf("Set Eth_config_MaxBitRate successfully.\n");
        return 0;
    }
}
int dev_Eth_config_DuplexMode(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        //Call native function to get Eth_config_DuplexMode -- TO DO
        strcpy((char *)mthdval_struct, "full");
        printf("Eth_config_DuplexMode is %s.\n", (char *)mthdval_struct);
        return 0;
    }
    else           // set function
    {
        //Call native function to set Eth_config_DuplexMod -- TO DO
        printf("Set Eth_config_DuplexMod successfully.\n");
        return 0;
    }
}

//LANDevice.LANEthernetInterfaceConfig.Stats
int dev_LAN_Ethstats_BytesSent(int opt_flag, void *mthdval_struct, int locate[])
{
    unsigned int lan_ethstats_bytessent;
    if (!opt_flag) // get function
    {
        //Call native function to get lan_ethstats_bytessent -- TO DO
        lan_ethstats_bytessent = 100;
        *((unsigned int *)mthdval_struct) = lan_ethstats_bytessent;
        printf("lan_ethstats_bytessent is %d.\n", lan_ethstats_bytessent);
        return 0;
    }
    else           // set function
    {
        printf("lan_ethstats_bytessent is unwritable.\n");
        return -1;
    }
}
int dev_LAN_Ethstats_BytesReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_LAN_Ethstats_PacketsSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_LAN_Ethstats_PacketsReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}

//LANDevice.LANUSBInterfaceConfig
int dev_usb_config_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_usb_config_Status(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_usb_config_MACAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_usb_config_MACControlEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_usb_config_Standard(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_usb_config_Type(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_usb_config_Rate(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_usb_config_Power(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}

//LANDevice.LANUSBInterfaceConfig.stats
int dev_LAN_USB_BytesSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_LAN_USB_BytesReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_LAN_USB_CellsSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_LAN_USB_CellsReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//LANDevice.WLANConfigConfiguration
int dev_WLAN_Enable(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_Status(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_BSSID(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_MaxBitRate(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_Channel(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_SSID(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_BeaconType(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_MACAddressControlEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_Standard(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_WEPKeyIndex(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_KeyPassphrase(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_WEPEncryptionLevel(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_BasicEncryptionModes(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_BasicAuthenticationMode(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_WPAEncryptionModes(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_WPAAuthenticationMode(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_IEEE11iEncryptionModes(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_IEEE11iAuthenticationMode(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_PossibleChannels(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_BasicDataTransmitRates(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_OperationalDataTransmitRates(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_PossibleDataTransmitRates(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_InsecureOOBAccessEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_BeaconAdvertisementEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_RadioEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WLAN_AutoRateFallBackEnabled(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_LocationDescription(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_RegulatoryDomain(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_TotalPSKFailures(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WLAN_TotalIntegrityFailures(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_ChannelsInUse(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_DeviceOperationMode(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_DistanceFromRoot(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_PeerBSSID(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_AuthenticationServiceMode(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_TotalBytesSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_TotalBytesReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}
int dev_WLAN_TotalPacketsSent(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_TotalPacketsReceived(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_TotalAssociations(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}

//LANDevice.WLANConfigConfiguration.Associations
int dev_WLAN_Assoc_DeviceMACAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_Assoc_DeviceIPAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_Assoc_AuthenticationState(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_Assoc_LastRequestedUnicastCipher(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_Assoc_LastRequestedMulticastCipher(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_Assoc_LastPMKId(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
        return 0;
    else           // set function
        return 0;
}

//LANDevice.WLANConfigConfiguration.WEPKey
int dev_WLAN_WEPKey(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}

//LANDevice.WLANConfigConfiguration.PreSharedKey
int dev_WLAN_PreSharedKey_PreSharedKey(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_PreSharedKey_KeyPassphrase(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}
int dev_WLAN_PreSharedKey_AssociatedDeviceMACAddress(int opt_flag, void *mthdval_struct, int locate[])
{
    if (!opt_flag) // get function
    {
        return 0;
    }
    else           // set function
    {
        return 0;
    }
}



