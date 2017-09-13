/************************************************************************
 *
 *    Copyright (c) 2005
 *    Infineon Technologies AG
 *    St. Martin Strasse 53; 81669 Muenchen; Germany
 *
 *    THE DELIVERY OF THIS SOFTWARE AS WELL AS THE HEREBY GRANTED
 *    NON-EXCLUSIVE, WORLDWIDE LICENSE TO USE, COPY, MODIFY, DISTRIBUTE
 *    AND SUBLICENSE THIS SOFTWARE IS FREE OF CHARGE.
 *
 *    THE LICENSED SOFTWARE IS PROVIDED "AS IS" AND INFINEON EXPRESSLY
 *    DISCLAIMS ALL REPRESENTATIONS AND WARRANTIES, WHETHER EXPRESS OR
 *    IMPLIED, INCLUDING WITHOUT LIMI?TATION, WAR?RANTIES OR REPRESENTATIONS
 *    OF WORKMANSHIP, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 *    DURABILITY, THAT THE OPERATING OF THE LICENSED SOFTWARE WILL BE ERROR
 *    FREE OR FREE OF NY THIRD PARTY CALIMS, INCLUDING WITHOUT LIMITATION
 *    CLAIMS OF THIRD PARTY INTELLECTUAL PROPERTY INFRINGEMENT.
 *
 *    EXCEPT FOR ANY LIABILITY DUE TO WILFUL ACTS OR GROSS NEGLIGENCE AND
 *    EXCEPT FOR ANY PERSONAL INJURY INFINEON SHALL IN NO EVENT BE LIABLE
 *    FOR ANY CLAIM OR DAMAGES OF ANY KIND, WHETHER IN AN ACTION OF CONTRACT,
 *    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *    --------------------------------------------------------------------
 *
 *        Project : Common Platform
 *        Block   :
 *        Creator : Jeffrey Huang
 *        File    : ip_turbo_nat_symb.c
 *        Abstract: Export Symbol for TurboNAT
 *        Date    : 2006/02/08
 *
 *    Modification History:
 *           By              Date     Ver.   Modification Description
 *           --------------- -------- -----  -----------------------------
 *
 ************************************************************************/




#include <linux/config.h>
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <net/checksum.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/brlock.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include <linux/netfilter_ipv4/ip_nat.h>
#include <linux/netfilter_ipv4/ip_nat_rule.h>
#include <linux/netfilter_ipv4/ip_nat_protocol.h>
#include <linux/netfilter_ipv4/ip_nat_core.h>
#include <linux/netfilter_ipv4/ip_nat_helper.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_conntrack_core.h>
#include <linux/ip_turbonat.h>

#ifdef CONFIG_ADM8668_HW_NAT
#include <asm/adm8668/hw_nat.h>
extern int HwNatEntryFunc(struct ip_conntrack *link, int oper);
extern int CheckHwNatEnable(void);
#endif

turbonat_hook_cb turbonat_hook_fn=NULL;
addroute_hook_cb addroute_hook_fn=NULL;
extern inline int ip_finish_output2(struct sk_buff *skb);

static int __init init(void)
{
        return 0;
}

static void __exit fini(void)
{
        return;
}


module_init(init);
module_exit(fini);

EXPORT_SYMBOL(do_bindings);
EXPORT_SYMBOL(turbonat_hook_fn);
EXPORT_SYMBOL(addroute_hook_fn);
#ifdef CONFIG_ADM8668_HW_NAT
EXPORT_SYMBOL(HwNatEntryFunc);
EXPORT_SYMBOL(CheckHwNatEnable);
#endif
EXPORT_SYMBOL(ip_finish_output2);
EXPORT_SYMBOL(ip_forward);
MODULE_LICENSE("GPL");
