#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

#include <fcntl.h>
#include <dlfcn.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <math.h>

#include "/home/chandrav/iproute2/include/libnetlink.h"
#include "/home/chandrav/iproute2/include/ll_map.h"
#include "/home/chandrav/iproute2/tc/tc_util.h"
#include "/home/chandrav/iproute2/include/utils.h"


void *BODY;

static int filter_ifindex;
int show_stats =1 ;
struct qdisc_util * qdisc_list;

static int print_noqopt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	if (opt && RTA_PAYLOAD(opt))
		fprintf(f, "[Unknown qdisc, optlen=%u] ", RTA_PAYLOAD(opt));
	return 0;
}

static int parse_noqopt(struct qdisc_util *qu, int argc, char **argv, struct nlmsghdr *n)
{
        if (argc) {
                fprintf(stderr, "Unknown qdisc \"%s\", hence option \"%s\" is unparsable\n", qu->id, *argv);
                return -1;
        }
        return 0;
}



struct qdisc_util *get_qdisc_kind(char *str)
{
	void *dlh;
	char buf[256];
	struct qdisc_util *q;

	for (q = qdisc_list; q; q = q->next)
		if (strcmp(q->id, str) == 0)
			return q;

	snprintf(buf, sizeof(buf), "q_%s.so", str);
	dlh = dlopen(buf, 0x00001);
	if (dlh == NULL) {
		dlh = BODY;
		if (dlh == NULL) {
			dlh = BODY = dlopen(NULL, 0x00001);
			if (dlh == NULL)
				goto noexist;
		}
	}

	snprintf(buf, sizeof(buf), "%s_util", str);
	q = dlsym(dlh, buf);
	if (q == NULL) {
		printf("dlsym returned error - %s\n", dlerror());
		goto noexist;
    }

reg:
	q->next = qdisc_list;
	qdisc_list = q;
	return q;

noexist:
	q = malloc(sizeof(*q));
	if (q) {
		memset(q, 0, sizeof(*q));
		strncpy(q->id, str, 15);
		q->parse_qopt = parse_noqopt;
		q->print_qopt = print_noqopt;
		goto reg;
	}
	return q;
}


#if 0
int print_tc_classid(char *buf, int len, __u32 h)
{
	if (h == TC_H_ROOT)
		sprintf(buf, "root");
	else if (h == TC_H_UNSPEC)
		snprintf(buf, len, "none");
	else if (TC_H_MAJ(h) == 0)
		snprintf(buf, len, ":%x", TC_H_MIN(h));
	else if (TC_H_MIN(h) == 0)
		snprintf(buf, len, "%x:", TC_H_MAJ(h)>>16);
	else
		snprintf(buf, len, "%x:%x", TC_H_MAJ(h)>>16, TC_H_MIN(h));
	return 0;
}
#endif

int print_qdisc(struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE*)arg;
	struct tcmsg *t = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	struct rtattr * tb[TCA_MAX+1];
	struct qdisc_util *q;
	char abuf[256];

    
	if (n->nlmsg_type != RTM_NEWQDISC && n->nlmsg_type != RTM_DELQDISC) {
		fprintf(stderr, "Not a qdisc\n");
		return 0;
	}

	printf("see the handle :%d",t->tcm_parent);
	fflush(stdout);

	len -= NLMSG_LENGTH(sizeof(*t));
	if (len < 0) {
		fprintf(stderr, "Wrong len %d\n", len);
		return -1;
	}

	if (filter_ifindex && filter_ifindex != t->tcm_ifindex)
		return 0;

	memset(tb, 0, sizeof(tb));
	parse_rtattr(tb, TCA_MAX, TCA_RTA(t), len);

	if (tb[TCA_KIND] == NULL) {
		fprintf(stderr, "NULL kind\n");
		return -1;
	}
	if (n->nlmsg_type == RTM_DELQDISC)
		fprintf(fp, "deleted ");

	fprintf(fp, "qdisc %s %x: ", (char*)RTA_DATA(tb[TCA_KIND]), t->tcm_handle>>16);
	if (filter_ifindex == 0)
		fprintf(fp, "dev %s ", ll_index_to_name(t->tcm_ifindex));
	if (t->tcm_parent == TC_H_ROOT)
		fprintf(fp, "root ");
	else if (t->tcm_parent) {
		print_tc_classid(abuf, sizeof(abuf), t->tcm_parent);
		fprintf(fp, "parent %s ", abuf);
	}
	if (t->tcm_info != 1) {
		fprintf(fp, "refcnt %d ", t->tcm_info);
	}
	
	if ((q = get_qdisc_kind(RTA_DATA(tb[TCA_KIND]))) != NULL)
		q->print_qopt(q, fp, tb[TCA_OPTIONS]);
	else
		fprintf(fp, "[UNKNOWN]");

	fprintf(fp, "\n");
/*     	
	if (show_stats) {
		if (tb[TCA_STATS]) {
			if (RTA_PAYLOAD(tb[TCA_STATS]) < sizeof(struct tc_stats))
				fprintf(fp, "statistics truncated");
			else {
				struct tc_stats st;
				memcpy(&st, RTA_DATA(tb[TCA_STATS]), sizeof(st));
				print_tcstats(fp, &st);
				fprintf(fp, "\n");
			}
		}
		if (q && tb[TCA_XSTATS]) {
			q->print_xstats(q, fp, tb[TCA_XSTATS]);
			fprintf(fp, "\n");
		}
		fprintf(fp, "\n");
	}
	
*/

	fflush(fp);
	return 0;

return 0;

}


int tc_qdisc_get(char *device, char *name)
{
	struct tcmsg t;
	struct rtnl_handle rth;
	char d[16];


	memset(&t, 0, sizeof(t));
	t.tcm_family = AF_UNSPEC;
	memset(&d, 0, sizeof(d));
	
		if (strcmp(device, "dev") == 0) {
			strncpy(d, name, sizeof(d)-1);
#ifdef TC_H_INGRESS
                } else if (strcmp(device, "ingress") == 0) {
                             if (t.tcm_parent) {
                                     fprintf(stderr, "Duplicate parent ID\n");
                             }
                             t.tcm_parent = TC_H_INGRESS;
#endif
		} else if (matches(device, "help") == 0) {
			//usage();
		} else {
			return -1;
		}


	if (rtnl_open(&rth, 0) < 0) {
		fprintf(stderr, "Cannot open rtnetlink\n");
		exit(1);
	}

	ll_init_map(&rth);

	if (d[0]) {
		if ((t.tcm_ifindex = ll_name_to_index(d)) == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", d);
			exit(1);
		}
		filter_ifindex = t.tcm_ifindex;
	}

   	
	if (rtnl_dump_request(&rth, RTM_GETQDISC, &t, sizeof(t)) < 0) {
		perror("Cannot send dump request");
		exit(1);
	}
	
	
	if (rtnl_dump_filter(&rth, print_qdisc, stdout, NULL, NULL) < 0) {
		fprintf(stderr, "Dump terminated\n");
		exit(1);
	}

	rtnl_close(&rth);
	return 0;
}


int main () {
 char * device =malloc(10);
 char *de = malloc(10);
 
 strcpy(device,"dev");
 strcpy(de,"eth0");
 
 tc_qdisc_get(device, de);	
 return 0;
 		
}

