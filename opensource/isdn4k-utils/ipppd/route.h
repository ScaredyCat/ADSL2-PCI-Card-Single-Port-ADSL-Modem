/* Quick and dirty patch: We need those structs and constants, but we can't
   include <linux/route.h> directly, because it's incompatible with with
   glibc...
   So, simply copy the relevant parts... */

#ifndef IPPPD_ROUTE_H
#define IPPPD_ROUTE_H

#define RTF_UP          0x0001          /* route usable                 */
#define RTF_GATEWAY     0x0002          /* destination is a gateway     */
#define RTF_HOST        0x0004          /* host entry (net otherwise)   */
#define RTF_REINSTATE   0x0008          /* reinstate route after tmout  */
#define RTF_DYNAMIC     0x0010          /* created dyn. (by redirect)   */
#define RTF_MODIFIED    0x0020          /* modified dyn. (by redirect)  */
#define RTF_MTU         0x0040          /* specific MTU for this route  */
#define RTF_MSS         RTF_MTU         /* Compatibility :-(            */
#define RTF_WINDOW      0x0080          /* per route window clamping    */
#define RTF_IRTT        0x0100          /* Initial round trip time      */
#define RTF_REJECT      0x0200          /* Reject route                 */
#define RTF_STATIC      0x0400          /* Manually injected route      */
#define RTF_XRESOLVE    0x0800          /* External resolver            */
#define RTF_NOFORWARD   0x1000          /* Forwarding inhibited         */
#define RTF_THROW       0x2000          /* Go to next class             */
#define RTF_NOPMTUDISC  0x4000          /* Do not send packets with DF  */
#if 0
#define RTF_MAGIC       0x8000          /* Route added/deleted authomatically,
                                         * when interface changes its state. */
#endif

#endif
