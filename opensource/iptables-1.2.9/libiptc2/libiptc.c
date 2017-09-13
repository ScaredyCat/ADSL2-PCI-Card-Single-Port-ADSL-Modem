/* Library which manipulates firewall rules.  Version $Revision: 1.41 $ */

/* Architecture of firewall rules is as follows:
 *
 * Chains go INPUT, FORWARD, OUTPUT then user chains.
 * Each user chain starts with an ERROR node.
 * Every chain ends with an unconditional jump: a RETURN for user chains,
 * and a POLICY for built-ins.
 */

/* (C) 1999 Paul ``Rusty'' Russell - Placed under the GNU GPL (See
 * COPYING for details). 
 * (C) 2000-2003 by the Netfilter Core Team <coreteam@netfilter.org>
 *
 * 2003-Jun-20: Harald Welte <laforge@netfilter.org>:
 *	- Reimplementation of chain cache to use offsets instead of entries
 * 2003-Jun-23: Harald Welte <laforge@netfilter.org>:
 * 	- speed optimization, sponsored by Astaro AG (http://www.astaro.com/)
 * 	  don't rebuild the chain cache after every operation, instead fix it
 * 	  up after a ruleset change.  
 * 2003-Jun-30: Harald Welte <laforge@netfilter.org>:
 * 	- reimplementation from scratch. *sigh*.  I hope nobody has to touch 
 * 	  this code ever again.
 */
#include "linux_listhelp.h"

#ifndef IPT_LIB_DIR
#define IPT_LIB_DIR "/usr/local/lib/iptables"
#endif

static int sockfd = -1;
static void *iptc_fn = NULL;

static const char *hooknames[]
= { [HOOK_PRE_ROUTING]  "PREROUTING",
    [HOOK_LOCAL_IN]     "INPUT",
    [HOOK_FORWARD]      "FORWARD",
    [HOOK_LOCAL_OUT]    "OUTPUT",
    [HOOK_POST_ROUTING] "POSTROUTING",
#ifdef HOOK_DROPPING
    [HOOK_DROPPING]	"DROPPING"
#endif
};

struct counter_map
{
	enum {
		COUNTER_MAP_NOMAP,
		COUNTER_MAP_NORMAL_MAP,
		COUNTER_MAP_ZEROED,
		COUNTER_MAP_SET
	} maptype;
	unsigned int mappos;
};

/* Convenience structures */
struct ipt_error_target
{
	STRUCT_ENTRY_TARGET t;
	char error[TABLE_MAXNAMELEN];
};

struct rule_head
{
	struct list_head list;		/* list of rules in chain */
	
	struct chain_head *chain;	/* we're part of this chain */

	struct chain_head *jumpto;	/* target of this rule, in case
					   it is a jump rule */

	struct counter_map counter_map;

	unsigned int size;		/* size of rule */
	STRUCT_ENTRY *entry_blob;	/* pointer to entry in blob */
	STRUCT_ENTRY entry[0];
};

struct chain_head
{
	struct list_head list;

	char name[TABLE_MAXNAMELEN];
	unsigned int hooknum;
	struct list_head rules;
	struct rule_head *firstrule; 	/* first (ERROR) rule */
	struct rule_head *lastrule;	/* last (RETURN) rule */
};

STRUCT_TC_HANDLE
{
	/* Have changes been made? */
	int changed;

	/* linked list of chains in this table */
	struct list_head chains;
	
	/* current position of first_chain() / next_chain() */
	struct chain_head *chain_iterator_cur;

	/* current position of first_rule() / next_rule() */
	struct rule_head *rule_iterator_cur;

	/* the structure we receive from getsockopt() */
	STRUCT_GETINFO info;

	/* Array of hook names */
	const char **hooknames;
#if 0
	/* Size in here reflects original state. */


	/* Cached position of chain heads (NULL = no cache). */
	unsigned int cache_num_chains;
	unsigned int cache_num_builtins;
	struct chain_cache *cache_chain_heads;

	/* Chain iterator: current chain cache entry. */
	struct chain_cache *cache_chain_iteration;

	/* Rule iterator: terminal rule */
	STRUCT_ENTRY *cache_rule_end;

	/* Number in here reflects current state. */
	unsigned int new_number;
#endif
	STRUCT_GET_ENTRIES entries;
};

static void
set_changed(TC_HANDLE_T h)
{
	h->changed = 1;
}

#ifdef IPTC_DEBUG
static void do_check(TC_HANDLE_T h, unsigned int line);
#define CHECK(h) do { if (!getenv("IPTC_NO_CHECK")) do_check((h), __LINE__); } while(0)
#else
#define CHECK(h)
#endif

static struct rule_head *ruleh_alloc(unsigned int size)
{
	struct rule_head *ruleh = malloc(sizeof(*ruleh)+size);
	if (!ruleh)
		return NULL;
	
	memset(ruleh, 0, sizeof(*ruleh)+size);
	ruleh->size = size;

	return ruleh;
}

static void ruleh_free(struct rule_head *ruleh)
{
	list_del(&ruleh->list);
	free(ruleh);
}

static struct chain_head *chainh_alloc(TC_HANDLE_T h, const char *name)
{
	struct chain_head *chainh = malloc(sizeof(*chainh));
	if (!chainh)
		return NULL;

	memset(chainh, 0, sizeof(*chainh));
	strncpy(chainh->name, name, sizeof(&chainh->name));
	list_append(&chainh->list, &h->chains);

	return chainh;
}

static void
chainh_clean(struct chain_head *chainh)
{
	/* FIXME */
	struct list_head *cur_item, *item2;

	list_for_each_safe(cur_item, item2, &chainh->rules) {
		struct rule_head *ruleh = list_entry(cur_item, 
						     struct rule_head,
						    list);
		ruleh_free(ruleh);
	}
}

static void 
chainh_free(struct chain_head *chainh)
{
	chainh_clean(chainh);
	list_del(&chainh->list);
}

static struct chain_head *
chainh_find(TC_HANDLE_T h, const IPT_CHAINLABEL name)
{
	struct list_head *cur;

	list_for_each(cur, &h->chains) {
		struct chain_head *ch = list_entry(cur, struct chain_head, 
						   list);
		if (!strcmp(name, ch->name))
			return ch;
	}
	return NULL;
}

/* Returns chain head if found, otherwise NULL. */
static struct chain_head *
find_label(const char *name, TC_HANDLE_T handle)
{
	return chainh_find(handle, name);
}


/* 
 * functions that directly operate on the blob 
 */

static inline unsigned long
entry2offset(const TC_HANDLE_T h, const STRUCT_ENTRY *e)
{
	return (void *)e - (void *)h->entries.entrytable;
}

static inline STRUCT_ENTRY *
get_entry(TC_HANDLE_T h, unsigned int offset)
{
	return (STRUCT_ENTRY *)((char *)h->entries.entrytable + offset);
}

/* needed by entry2index */
static inline int
get_number(const STRUCT_ENTRY *i,
	   const STRUCT_ENTRY *seek,
	   unsigned int *pos)
{
	if (i == seek)
		return 1;
	(*pos)++;
	return 0;
}

static unsigned int
entry2index(const TC_HANDLE_T h, const STRUCT_ENTRY *seek)
{
	unsigned int pos = 0;

	if (ENTRY_ITERATE(h->entries.entrytable, h->entries.size,
			  get_number, seek, &pos) == 0) {
		fprintf(stderr, "ERROR: offset %i not an entry!\n",
			(char *)seek - (char *)h->entries.entrytable);
		abort();
	}
	return pos;
}

static inline int
get_entry_n(STRUCT_ENTRY *i,
	    unsigned int number,
	    unsigned int *pos,
	    STRUCT_ENTRY **pe)
{
	if (*pos == number) {
		*pe = i;
		return 1;
	}
	(*pos)++;
	return 0;
}

static STRUCT_ENTRY *
index2entry(TC_HANDLE_T h, unsigned int index)
{
	unsigned int pos = 0;
	STRUCT_ENTRY *ret = NULL;

	ENTRY_ITERATE(h->entries.entrytable, h->entries.size,
		      get_entry_n, index, &pos, &ret);

	return ret;
}

static inline unsigned long
index2offset(TC_HANDLE_T h, unsigned int index)
{
	return entry2offset(h, index2entry(h, index));
}

static char *
get_errorlabel(TC_HANDLE_T h, unsigned int offset)
{
	STRUCT_ENTRY *e;

	e = get_entry(h, offset);
	if (strcmp(GET_TARGET(e)->u.user.name, ERROR_TARGET) != 0) {
		fprintf(stderr, "ERROR: offset %u not an error node!\n",
			offset);
		abort();
	}

	return (char *)GET_TARGET(e)->data;
}

#if 0
static inline STRUCT_ENTRY *
offset2entry(TC_HANDLE_T h, unsigned int offset)
{
	return (STRUCT_ENTRY *) ((void *)h->entries.entrytable+offset);
}

static inline unsigned int
offset2index(const TC_HANDLE_T h, unsigned int offset)
{
	return entry2index(h, offset2entry(h, offset));
}


#endif

/* Allocate handle of given size */
static TC_HANDLE_T
alloc_tc_handle(const char *tablename, unsigned int size, 
		unsigned int num_rules)
{
	size_t len;
	TC_HANDLE_T h;

	len = sizeof(STRUCT_TC_HANDLE)
		+ size
		+ num_rules * sizeof(struct counter_map);

	if ((h = malloc(len)) == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	h->changed = 0;

	strcpy(h->info.name, tablename);
	strcpy(h->entries.name, tablename);
	INIT_LIST_HEAD(&h->chains);

	return h;
}

/* get the name of the chain that we jump to */
static char *
parse_jumptarget(const STRUCT_ENTRY *e, TC_HANDLE_T h)
{
	STRUCT_ENTRY *jumpto;
	int spos, labelidx;

	if (strcmp(GET_TARGET(e)->u.user.name, STANDARD_TARGET) != 0) {
		/* called for non-standard target */
		return "__FIXME";
	}
	/* Standard target: evaluate */
	spos = *(int *)GET_TARGET(e)->data;
	if (spos < 0) {
		return "__FIXME";
	}

	jumpto = get_entry(h, spos);

	/* Fall through rule */
	if (jumpto == (void *)e + e->next_offset)
		return "";

	/* Must point to head of a chain: ie. after error rule */
	/* FIXME: this needs to deal with internal jump targets */
	labelidx = entry2index(h, jumpto) - 1;
	return get_errorlabel(h, index2offset(h, labelidx));
}

/* parser functions */

struct rule_head *
append_entrycopy(const STRUCT_ENTRY *e, struct rule_head *prev)
{
	struct rule_head *ruleh = ruleh_alloc(e->next_offset);
	if (!ruleh)
		return NULL;
	
	memcpy(&ruleh->entry, e, e->next_offset);
	ruleh->chain = prev->chain;
	ruleh->entry_blob = e;
	list_append(&ruleh->list, &prev->list);

	return ruleh;
}

/* have to return 0 on success, bcf ENTRY_ITERATE */
static inline int 
parse_entry(const STRUCT_ENTRY *e, TC_HANDLE_T h, struct chain_head **curchain)
{
	int i;
	union tgt_u {
		STRUCT_ENTRY_TARGET ent;
		STRUCT_STANDARD_TARGET std;
		struct ipt_error_target err;
	} *tgt;

	struct rule_head *lastrule = list_entry((*curchain)->rules.prev,
						 struct rule_head, list);
	struct rule_head *newrule;

	tgt = (union tgt_u *) GET_TARGET(e);

	if (e->target_offset == sizeof(STRUCT_ENTRY)
	    && (strcmp(tgt->ent.u.user.name, IPT_STANDARD_TARGET) == 0)) {
		/* jump to somewhere else */
		char *targname;
		struct chain_head *chainh;

		newrule = append_entrycopy(e, lastrule);

		targname = parse_jumptarget(e, h);
		if (!(chainh = find_label(targname, h))) {
			chainh = chainh_alloc(h, targname);
		}
		if (!chainh) {
			errno = ENOMEM;
			return 1;
		}
		newrule->jumpto = chainh;

	} else if (e->target_offset == sizeof(STRUCT_ENTRY)
		   && e->next_offset == sizeof(STRUCT_ENTRY)
		   			+ ALIGN(sizeof(struct ipt_error_target))
		   && !strcmp(tgt->ent.u.user.name, ERROR_TARGET)) {
		/* chain head */
		*curchain = chainh_find(h, tgt->err.error);
		if (!(*curchain)) {
			*curchain = chainh_alloc(h, tgt->err.error);
			/* FIXME: error handling */
		}
		newrule = append_entrycopy(e, lastrule);
		(*curchain)->firstrule = newrule;

	} else if (e->target_offset == sizeof(STRUCT_ENTRY)
		   && e->next_offset == sizeof(STRUCT_ENTRY)
		   			+ ALIGN(sizeof(STRUCT_STANDARD_TARGET))
		   && tgt->std.verdict == RETURN) {
		/* chain end */
		newrule = append_entrycopy(e, lastrule);
		(*curchain)->lastrule = newrule;
		*curchain = NULL;
	} else {
		/* normal rule */
		newrule = append_entrycopy(e, lastrule);
	}

	/* create counter map entry */
	newrule->counter_map.maptype = COUNTER_MAP_NORMAL_MAP;
	newrule->counter_map.mappos = entry2index(h, e);

	/* iterate over hook_entries, needed to connect builtin
	 * chains with hook numbers */
	for (i = 0; i < NUMHOOKS; i++) {
		if (!(h->info.valid_hooks & (1 << i)))
			continue;
		if (h->info.hook_entry[i] == entry2offset(h, e)) {
			/* found hook entry point */
			if (*curchain)
				(*curchain)->hooknum = i;
		}
		if (h->info.underflow[i] == entry2offset(h, e)) {
			/* found underflow point */
		}
	}

	return 0;
}

static int parse_ruleset(TC_HANDLE_T h)
{
	struct chain_head *curchain;
	
	/* iterate over ruleset; create linked list of rule_head/chain_head */
	if (ENTRY_ITERATE(h->entries.entrytable, h->entries.size, 
		      parse_entry, h, &curchain)) {
		/* some error happened while iterating */
		return 0;
	}

	return 1;
}

TC_HANDLE_T
TC_INIT(const char *tablename)
{
	TC_HANDLE_T h;
	STRUCT_GETINFO info;
	int tmp;
	socklen_t s;

	iptc_fn = TC_INIT;

	if (sockfd != -1) {
		close(sockfd);
		sockfd = -1;
	}

	if (strlen(tablename) >= TABLE_MAXNAMELEN) {
		errno = EINVAL;
		return NULL;
	}
	
	sockfd = socket(TC_AF, SOCK_RAW, IPPROTO_RAW);
	if (sockfd < 0)
		return NULL;

	s = sizeof(info);

	strcpy(info.name, tablename);
	if (getsockopt(sockfd, TC_IPPROTO, SO_GET_INFO, &info, &s) < 0)
		return NULL;

	if ((h = alloc_tc_handle(info.name, info.size, info.num_entries))
	    == NULL) {
		close(sockfd);
		sockfd = -1;
		return NULL;
	}

/* Too hard --RR */
#if 0
	sprintf(pathname, "%s/%s", IPT_LIB_DIR, info.name);
	dynlib = dlopen(pathname, RTLD_NOW);
	if (!dynlib) {
		errno = ENOENT;
		return NULL;
	}
	h->hooknames = dlsym(dynlib, "hooknames");
	if (!h->hooknames) {
		errno = ENOENT;
		return NULL;
	}
#else
	h->hooknames = hooknames;
#endif

	/* Initialize current state */
	h->info = info;
	//h->new_number = h->info.num_entries;
	//
	h->entries.size = h->info.size;

	tmp = sizeof(STRUCT_GET_ENTRIES) + h->info.size;

	if (getsockopt(sockfd, TC_IPPROTO, SO_GET_ENTRIES, &h->entries,
		       &tmp) < 0) {
		close(sockfd);
		sockfd = -1;
		free(h);
		return NULL;
	}

	CHECK(h);
	parse_ruleset(h);

	return h;
}

void
TC_FREE(TC_HANDLE_T *h)
{
	struct list_head *cur_item, *item2;

	close(sockfd);
	sockfd = -1;

	/* free all chains */
	list_for_each_safe(cur_item, item2, &(*h)->chains) {
		struct chain_head *chead = list_entry(cur_item,
						      struct chain_head,
						      list);
		chainh_free(chead);
	}

	/* FIXME: free all other ressources we might be using */

	free(*h);
	*h = NULL;
}

static inline int
print_match(const STRUCT_ENTRY_MATCH *m)
{
	printf("Match name: `%s'\n", m->u.user.name);
	return 0;
}

static int dump_entry(STRUCT_ENTRY *e, const TC_HANDLE_T handle);
 
#if 0
void
TC_DUMP_ENTRIES(const TC_HANDLE_T handle)
{
	CHECK(handle);

	printf("libiptc v%s.  %u entries, %u bytes.\n",
	       IPTABLES_VERSION,
	       handle->new_number, handle->entries.size);
	printf("Table `%s'\n", handle->info.name);
	printf("Hooks: pre/in/fwd/out/post = %u/%u/%u/%u/%u\n",
	       handle->info.hook_entry[HOOK_PRE_ROUTING],
	       handle->info.hook_entry[HOOK_LOCAL_IN],
	       handle->info.hook_entry[HOOK_FORWARD],
	       handle->info.hook_entry[HOOK_LOCAL_OUT],
	       handle->info.hook_entry[HOOK_POST_ROUTING]);
	printf("Underflows: pre/in/fwd/out/post = %u/%u/%u/%u/%u\n",
	       handle->info.underflow[HOOK_PRE_ROUTING],
	       handle->info.underflow[HOOK_LOCAL_IN],
	       handle->info.underflow[HOOK_FORWARD],
	       handle->info.underflow[HOOK_LOCAL_OUT],
	       handle->info.underflow[HOOK_POST_ROUTING]);

	ENTRY_ITERATE(handle->entries.entrytable, handle->entries.size,
		      dump_entry, handle);
}

/* Returns 0 if not hook entry, else hooknumber + 1 */
static inline unsigned int
is_hook_entry(STRUCT_ENTRY *e, TC_HANDLE_T h)
{
	unsigned int i;

	for (i = 0; i < NUMHOOKS; i++) {
		if ((h->info.valid_hooks & (1 << i))
		    && get_entry(h, h->info.hook_entry[i]) == e)
			return i+1;
	}
	return 0;
}


static int alphasort(const void *a, const void *b)
{
	return strcmp(((struct chain_cache *)a)->name,
		      ((struct chain_cache *)b)->name);
}
#endif

/* Does this chain exist? */
int TC_IS_CHAIN(const char *chain, const TC_HANDLE_T handle)
{
	return find_label(chain, handle) != NULL;
}

#if 0
/* Returns the position of the final (ie. unconditional) element. */
static unsigned int
get_chain_end(const TC_HANDLE_T handle, unsigned int start)
{
	unsigned int last_off, off;
	STRUCT_ENTRY *e;

	last_off = start;
	e = get_entry(handle, start);

	/* Terminate when we meet a error label or a hook entry. */
	for (off = start + e->next_offset;
	     off < handle->entries.size;
	     last_off = off, off += e->next_offset) {
		STRUCT_ENTRY_TARGET *t;
		unsigned int i;

		e = get_entry(handle, off);

		/* We hit an entry point. */
		for (i = 0; i < NUMHOOKS; i++) {
			if ((handle->info.valid_hooks & (1 << i))
			    && off == handle->info.hook_entry[i])
				return last_off;
		}

		/* We hit a user chain label */
		t = GET_TARGET(e);
		if (strcmp(t->u.user.name, ERROR_TARGET) == 0)
			return last_off;
	}
	/* SHOULD NEVER HAPPEN */
	fprintf(stderr, "ERROR: Off end (%u) of chain from %u!\n",
		handle->entries.size, off);
	abort();
}
#endif

/* Iterator functions to run through the chains. */
const char *
TC_FIRST_CHAIN(TC_HANDLE_T *handle)
{
	struct chain_head *firsthead = list_entry((*handle)->chains.next,
						   struct chain_head, list);
	(*handle)->chain_iterator_cur = firsthead;

	return firsthead->name;
}

/* Iterator functions to run through the chains.  Returns NULL at end. */
const char *
TC_NEXT_CHAIN(TC_HANDLE_T *handle)
{
	struct chain_head *next = list_entry(&(*handle)->chain_iterator_cur->list.next, struct chain_head, list);
	(*handle)->chain_iterator_cur = next;

	if (&next->list == &(*handle)->chains)
		return NULL;

	return next->name;
}

/* Get first rule in the given chain: NULL for empty chain. */
const STRUCT_ENTRY *
TC_FIRST_RULE(const char *chain, TC_HANDLE_T *handle)
{
	struct chain_head *c;
	struct rule_head *r;

	c = find_label(chain, *handle);
	if (!c) {
		errno = ENOENT;
		return NULL;
	}

	/* Empty chain: single return/policy rule */
	if (list_empty(&c->rules))
		return NULL;

	r = list_entry(c->rules.next, struct rule_head, list);
	(*handle)->rule_iterator_cur = r;

	return r->entry;
}

/* Returns NULL when rules run out. */
const STRUCT_ENTRY *
TC_NEXT_RULE(const STRUCT_ENTRY *prev, TC_HANDLE_T *handle)
{
	struct rule_head *r = list_entry((*handle)->rule_iterator_cur->list.next, struct rule_head, list);

	if (&r->list == &r->chain->rules)
		return NULL;

	/* NOTE: prev is without any influence ! */
	return r->entry;
}

#if 0
/* How many rules in this chain? */
unsigned int
TC_NUM_RULES(const char *chain, TC_HANDLE_T *handle)
{
	unsigned int off = 0;
	STRUCT_ENTRY *start, *end;

	CHECK(*handle);
	if (!find_label(&off, chain, *handle)) {
		errno = ENOENT;
		return (unsigned int)-1;
	}

	start = get_entry(*handle, off);
	end = get_entry(*handle, get_chain_end(*handle, off));

	return entry2index(*handle, end) - entry2index(*handle, start);
}

/* Get n'th rule in this chain. */
const STRUCT_ENTRY *TC_GET_RULE(const char *chain,
				unsigned int n,
				TC_HANDLE_T *handle)
{
	unsigned int pos = 0, chainindex;

	CHECK(*handle);
	if (!find_label(&pos, chain, *handle)) {
		errno = ENOENT;
		return NULL;
	}

	chainindex = entry2index(*handle, get_entry(*handle, pos));

	return index2entry(*handle, chainindex + n);
}
#endif

static const char *
target_name(TC_HANDLE_T handle, const STRUCT_ENTRY *ce)
{
	int spos;

	/* To avoid const warnings */
	STRUCT_ENTRY *e = (STRUCT_ENTRY *)ce;

	if (strcmp(GET_TARGET(e)->u.user.name, STANDARD_TARGET) != 0)
		return GET_TARGET(e)->u.user.name;

	/* Standard target: evaluate */
	spos = *(int *)GET_TARGET(e)->data;
	if (spos < 0) {
		if (spos == RETURN)
			return LABEL_RETURN;
		else if (spos == -NF_ACCEPT-1)
			return LABEL_ACCEPT;
		else if (spos == -NF_DROP-1)
			return LABEL_DROP;
		else if (spos == -NF_QUEUE-1)
			return LABEL_QUEUE;

		fprintf(stderr, "ERROR: entry %p not a valid target (%d)\n",
			e, spos);
		abort();
	}

#if 0
//	jumpto = get_entry(handle, spos);

	/* Fall through rule */
	if (jumpto == (void *)e + e->next_offset)
		return "";

	/* Must point to head of a chain: ie. after error rule */
	/* FIXME: this needs to deal with internal jump targets */
	labelidx = entry2index(handle, jumpto) - 1;
	return get_errorlabel(handle, index2offset(handle, labelidx));
#endif
	return "";
}

/* Returns a pointer to the target name of this position. */
const char *TC_GET_TARGET(const STRUCT_ENTRY *e,
			  TC_HANDLE_T *handle)
{
	return target_name(*handle, e);
}

/* Is this a built-in chain?  Actually returns hook + 1. */
int
TC_BUILTIN(const char *chain, const TC_HANDLE_T handle)
{
	unsigned int i;

	for (i = 0; i < NUMHOOKS; i++) {
		if ((handle->info.valid_hooks & (1 << i))
		    && handle->hooknames[i]
		    && strcmp(handle->hooknames[i], chain) == 0)
			return i+1;
	}
	return 0;
}

/* Get the policy of a given built-in chain */
const char *
TC_GET_POLICY(const char *chain,
	      STRUCT_COUNTERS *counters,
	      TC_HANDLE_T *handle)
{
	STRUCT_ENTRY *e;
	struct chain_head *chainh;
	struct rule_head *ruleh;
	int hook;

	hook = TC_BUILTIN(chain, *handle);
	if (hook == 0)
		return NULL;

	chainh = find_label(chain, *handle);
	if (!chainh) {
		errno = ENOENT;
		return NULL;
	}

	ruleh = chainh->lastrule;

	e = ruleh->entry;
	*counters = e->counters;

	return target_name(*handle, e);
}

#if 0
static int
correct_verdict(STRUCT_ENTRY *e,
		char *base,
		unsigned int offset, int delta_offset)
{
	STRUCT_STANDARD_TARGET *t = (void *)GET_TARGET(e);
	unsigned int curr = (char *)e - base;

	/* Trap: insert of fall-through rule.  Don't change fall-through
	   verdict to jump-over-next-rule. */
	if (strcmp(t->target.u.user.name, STANDARD_TARGET) == 0
	    && t->verdict > (int)offset
	    && !(curr == offset &&
		 t->verdict == curr + e->next_offset)) {
		t->verdict += delta_offset;
	}

	return 0;
}

/* Adjusts standard verdict jump positions after an insertion/deletion. */
static int
set_verdict(unsigned int offset, int delta_offset, TC_HANDLE_T *handle)
{
	ENTRY_ITERATE((*handle)->entries.entrytable,
		      (*handle)->entries.size,
		      correct_verdict, (char *)(*handle)->entries.entrytable,
		      offset, delta_offset);

	set_changed(*handle);
	return 1;
}
#endif



static int
standard_map(STRUCT_ENTRY *e, int verdict)
{
	STRUCT_STANDARD_TARGET *t;

	t = (STRUCT_STANDARD_TARGET *)GET_TARGET(e);

	if (t->target.u.target_size
	    != ALIGN(sizeof(STRUCT_STANDARD_TARGET))) {
		errno = EINVAL;
		return 0;
	}
	/* memset for memcmp convenience on delete/replace */
	memset(t->target.u.user.name, 0, FUNCTION_MAXNAMELEN);
	strcpy(t->target.u.user.name, STANDARD_TARGET);
	t->verdict = verdict;

	return 1;
}

static int
map_target(const TC_HANDLE_T handle,
	   STRUCT_ENTRY *e,
	   unsigned int offset,
	   STRUCT_ENTRY_TARGET *old)
{
	STRUCT_ENTRY_TARGET *t = (STRUCT_ENTRY_TARGET *)GET_TARGET(e);

	/* Save old target (except data, which we don't change, except for
	   standard case, where we don't care). */
	*old = *t;

	/* Maybe it's empty (=> fall through) */
	if (strcmp(t->u.user.name, "") == 0)
		return standard_map(e, offset + e->next_offset);
	/* Maybe it's a standard target name... */
	else if (strcmp(t->u.user.name, LABEL_ACCEPT) == 0)
		return standard_map(e, -NF_ACCEPT - 1);
	else if (strcmp(t->u.user.name, LABEL_DROP) == 0)
		return standard_map(e, -NF_DROP - 1);
	else if (strcmp(t->u.user.name, LABEL_QUEUE) == 0)
		return standard_map(e, -NF_QUEUE - 1);
	else if (strcmp(t->u.user.name, LABEL_RETURN) == 0)
		return standard_map(e, RETURN);
	else if (TC_BUILTIN(t->u.user.name, handle)) {
		/* Can't jump to builtins. */
		errno = EINVAL;
		return 0;
	} else {
		/* Maybe it's an existing chain name. */
		struct chain_head *c;

#if 0
		/* FIXME */
		c = find_label(t->u.user.name, handle);
		if (c)
			return standard_map(e, c->start_off);
#endif
	}

	/* Must be a module?  If not, kernel will reject... */
	/* memset to all 0 for your memcmp convenience. */
	memset(t->u.user.name + strlen(t->u.user.name),
	       0,
	       FUNCTION_MAXNAMELEN - strlen(t->u.user.name));
	return 1;
}

static void
unmap_target(STRUCT_ENTRY *e, STRUCT_ENTRY_TARGET *old)
{
	STRUCT_ENTRY_TARGET *t = GET_TARGET(e);

	/* Save old target (except data, which we don't change, except for
	   standard case, where we don't care). */
	*t = *old;
}

static struct rule_head *
ruleh_get_n(struct chain_head *chead, int rulenum) 
{
	int i = 0;
	struct list_head *list;

	
	list_for_each(list, &chead->rules) {
		struct rule_head *rhead = list_entry(list, struct rule_head, 
							list);
		i++;
		if (i == rulenum)
			return rhead;
	}
	return NULL;
}

/* Insert the entry `e' in chain `chain' into position `rulenum'. */
int
TC_INSERT_ENTRY(const IPT_CHAINLABEL chain,
		const STRUCT_ENTRY *e,
		unsigned int rulenum,
		TC_HANDLE_T *handle)
{
	struct chain_head *c;
	struct rule_head *prev;

	iptc_fn = TC_INSERT_ENTRY;
	if (!(c = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	prev = ruleh_get_n(c, rulenum-1);
	if (!prev) {
		errno = E2BIG;
		return 0;
	}

	if (append_entrycopy(e, prev))
		return 1;

	return 0;
}

/* Atomically replace rule `rulenum' in `chain' with `fw'. */
int
TC_REPLACE_ENTRY(const IPT_CHAINLABEL chain,
		 const STRUCT_ENTRY *e,
		 unsigned int rulenum,
		 TC_HANDLE_T *handle)
{
	struct chain_head *c;
	struct rule_head *repl;

	iptc_fn = TC_REPLACE_ENTRY;

	if (!(c = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	repl = ruleh_get_n(c, rulenum);
	if (!repl) {
		errno = E2BIG;
		return 0;
	}

	if (!append_entrycopy(e, repl)) {
		errno = ENOMEM;
		return 0;
	}

	ruleh_free(repl);
	return 1;
}

/* Append entry `e' to chain `chain'.  Equivalent to insert with
   rulenum = length of chain. */
int
TC_APPEND_ENTRY(const IPT_CHAINLABEL chain,
		const STRUCT_ENTRY *e,
		TC_HANDLE_T *handle)
{
	struct chain_head *c;
	struct rule_head *rhead;

	iptc_fn = TC_APPEND_ENTRY;

	if (!(c = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	rhead = list_entry(c->rules.prev, struct rule_head, list);
	if(append_entrycopy(e, rhead))
		return 1;
	
	return 0;
}

static inline int
match_different(const STRUCT_ENTRY_MATCH *a,
		const unsigned char *a_elems,
		const unsigned char *b_elems,
		unsigned char **maskptr)
{
	const STRUCT_ENTRY_MATCH *b;
	unsigned int i;

	/* Offset of b is the same as a. */
	b = (void *)b_elems + ((unsigned char *)a - a_elems);

	if (a->u.match_size != b->u.match_size)
		return 1;

	if (strcmp(a->u.user.name, b->u.user.name) != 0)
		return 1;

	*maskptr += ALIGN(sizeof(*a));

	for (i = 0; i < a->u.match_size - ALIGN(sizeof(*a)); i++)
		if (((a->data[i] ^ b->data[i]) & (*maskptr)[i]) != 0)
			return 1;
	*maskptr += i;
	return 0;
}

static inline int
target_different(const unsigned char *a_targdata,
		 const unsigned char *b_targdata,
		 unsigned int tdatasize,
		 const unsigned char *mask)
{
	unsigned int i;
	for (i = 0; i < tdatasize; i++)
		if (((a_targdata[i] ^ b_targdata[i]) & mask[i]) != 0)
			return 1;

	return 0;
}

static int
is_same(const STRUCT_ENTRY *a,
	const STRUCT_ENTRY *b,
	unsigned char *matchmask);

/* Delete the first rule in `chain' which matches `origfw'. */
int
TC_DELETE_ENTRY(const IPT_CHAINLABEL chain,
		const STRUCT_ENTRY *origfw,
		unsigned char *matchmask,
		TC_HANDLE_T *handle)
{
	struct chain_head *c;
	struct list_head *cur, *cur2;

	iptc_fn = TC_DELETE_ENTRY;
	if (!(c = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	list_for_each_safe(cur, cur2, &c->rules) {
		struct rule_head *rhead = list_entry(cur, struct rule_head, 
							list);
		if (is_same(rhead->entry, origfw, matchmask)) {
			ruleh_free(rhead);
			return 1;
		}
	}

	errno = ENOENT;
	return 0;
}

/* Delete the rule in position `rulenum' in `chain'. */
int
TC_DELETE_NUM_ENTRY(const IPT_CHAINLABEL chain,
		    unsigned int rulenum,
		    TC_HANDLE_T *handle)
{
	struct chain_head *chainh;
	struct rule_head *rhead;

	iptc_fn = TC_DELETE_NUM_ENTRY;

	if (!(chainh = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	rhead = ruleh_get_n(chainh, rulenum);
	if (!rhead) {
		errno = E2BIG;
		return 0;
	}

	ruleh_free(rhead);

	return 1;
}

/* Check the packet `fw' on chain `chain'.  Returns the verdict, or
   NULL and sets errno. */
const char *
TC_CHECK_PACKET(const IPT_CHAINLABEL chain,
		STRUCT_ENTRY *entry,
		TC_HANDLE_T *handle)
{
	errno = ENOSYS;
	return NULL;
}

/* Flushes the entries in the given chain (ie. empties chain). */
int
TC_FLUSH_ENTRIES(const IPT_CHAINLABEL chain, TC_HANDLE_T *handle)
{
	struct list_head *cur, *cur2;
	struct chain_head *chainh;

	if (!(chainh = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	list_for_each_safe(cur, cur2, &chainh->rules) {
		struct rule_head *ruleh = list_entry(cur, struct rule_head, 
							list);
		/* don't free the entry and policy/return entries */
		if (ruleh != chainh->firstrule && ruleh != chainh->lastrule)
			ruleh_free(ruleh);
	}
	return 1;
}

/* Zeroes the counters in a chain. */
int
TC_ZERO_ENTRIES(const IPT_CHAINLABEL chain, TC_HANDLE_T *handle)
{
	struct chain_head *c;
	struct list_head *cur;

	if (!(c = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	list_for_each(cur, c->rules.next) {
		struct rule_head *r = list_entry(cur, struct rule_head, list);
		if (r->counter_map.maptype == COUNTER_MAP_NORMAL_MAP)
			r->counter_map.maptype = COUNTER_MAP_ZEROED;
	}
	set_changed(*handle);

	return 1;
}

STRUCT_COUNTERS *
TC_READ_COUNTER(const IPT_CHAINLABEL chain,
		unsigned int rulenum,
		TC_HANDLE_T *handle)
{
	STRUCT_ENTRY *e;
	struct chain_head *c;
	struct rule_head *r;

	iptc_fn = TC_READ_COUNTER;
	CHECK(*handle);

	if (!(c = find_label(chain, *handle) )
	      || !(r = ruleh_get_n(c, rulenum))) {
		errno = ENOENT;
		return NULL;
	}

	return &r->entry->counters;
}

int
TC_ZERO_COUNTER(const IPT_CHAINLABEL chain,
		unsigned int rulenum,
		TC_HANDLE_T *handle)
{
	STRUCT_ENTRY *e;
	struct chain_head *c;
	struct rule_head *r;
	
	iptc_fn = TC_ZERO_COUNTER;
	CHECK(*handle);

	if (!(c = find_label(chain, *handle))
	      || !(r = ruleh_get_n(c, rulenum))) {
		errno = ENOENT;
		return 0;
	}

	if (r->counter_map.maptype == COUNTER_MAP_NORMAL_MAP)
		r->counter_map.maptype = COUNTER_MAP_ZEROED;

	set_changed(*handle);

	return 1;
}

int 
TC_SET_COUNTER(const IPT_CHAINLABEL chain,
	       unsigned int rulenum,
	       STRUCT_COUNTERS *counters,
	       TC_HANDLE_T *handle)
{
	STRUCT_ENTRY *e;
	struct chain_head *c;
	struct rule_head *r;

	iptc_fn = TC_SET_COUNTER;
	CHECK(*handle);

	if (!(c = find_label(chain, *handle))
	      || !(r = ruleh_get_n(c, rulenum))) {
		errno = ENOENT;
		return 0;
	}
	
	r->counter_map.maptype = COUNTER_MAP_SET;
	memcpy(&r->entry->counters, counters, sizeof(STRUCT_COUNTERS));

	set_changed(*handle);

	return 1;
}

/* Creates a new chain. */
/* To create a chain, create two rules: error node and unconditional
 * return. */
int
TC_CREATE_CHAIN(const IPT_CHAINLABEL chain, TC_HANDLE_T *handle)
{
	int ret;
	struct chainstart {
		STRUCT_ENTRY head;
		struct ipt_error_target name;
	} *newc1;
	struct chainend {
		STRUCT_ENTRY ret;
		STRUCT_STANDARD_TARGET target;
	} *newc2;
	struct rule_head *newr1, *newr2;
	struct chain_head *chead;

	iptc_fn = TC_CREATE_CHAIN;

	/* find_label doesn't cover built-in targets: DROP, ACCEPT,
           QUEUE, RETURN. */
	if (find_label(chain, *handle)
	    || strcmp(chain, LABEL_DROP) == 0
	    || strcmp(chain, LABEL_ACCEPT) == 0
	    || strcmp(chain, LABEL_QUEUE) == 0
	    || strcmp(chain, LABEL_RETURN) == 0) {
		errno = EEXIST;
		return 0;
	}

	if (strlen(chain)+1 > sizeof(IPT_CHAINLABEL)) {
		errno = EINVAL;
		return 0;
	}

	chead = chainh_alloc(*handle, chain);
	if (!chead) {
		errno = ENOMEM;
		return 0;
	}
	
	newr1 = ruleh_alloc(sizeof(*newc1));
	if (!newr1) {
		chainh_free(chead);
		return 0;
	}
	newc1 = (struct chainstart *) newr1->entry;

	newr2 = ruleh_alloc(sizeof(*newc2));
	if (!newr2) {
		chainh_free(chead);
		ruleh_free(newr1);
		return 0;
	}
	newc2 = (struct chainend *) newr2->entry;

	newc1->head.target_offset = sizeof(STRUCT_ENTRY);
	newc1->head.next_offset
		= sizeof(STRUCT_ENTRY)
		+ ALIGN(sizeof(struct ipt_error_target));
	strcpy(newc1->name.t.u.user.name, ERROR_TARGET);
	newc1->name.t.u.target_size = ALIGN(sizeof(struct ipt_error_target));
	strcpy(newc1->name.error, chain);

	newc2->ret.target_offset = sizeof(STRUCT_ENTRY);
	newc2->ret.next_offset
		= sizeof(STRUCT_ENTRY)
		+ ALIGN(sizeof(STRUCT_STANDARD_TARGET));
	strcpy(newc2->target.target.u.user.name, STANDARD_TARGET);
	newc2->target.target.u.target_size
		= ALIGN(sizeof(STRUCT_STANDARD_TARGET));
	newc2->target.verdict = RETURN;

	list_prepend(&newr1->list, &chead->rules);
	chead->firstrule = newr1;
	list_append(&newr2->list, &chead->rules);
	chead->lastrule = newr2;

	return 1;
}

#if 0
static int
count_ref(STRUCT_ENTRY *e, unsigned int offset, unsigned int *ref)
{
	STRUCT_STANDARD_TARGET *t;

	if (strcmp(GET_TARGET(e)->u.user.name, STANDARD_TARGET) == 0) {
		t = (STRUCT_STANDARD_TARGET *)GET_TARGET(e);

		if (t->verdict == offset)
			(*ref)++;
	}

	return 0;
}

/* Get the number of references to this chain. */
int
TC_GET_REFERENCES(unsigned int *ref, const IPT_CHAINLABEL chain,
		  TC_HANDLE_T *handle)
{
	struct chain_cache *c;

	if (!(c = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	*ref = 0;
	ENTRY_ITERATE((*handle)->entries.entrytable,
		      (*handle)->entries.size,
		      count_ref, c->start_off, ref);
	return 1;
}
#endif

static unsigned int
count_rules(struct chain_head *chainh)
{
	unsigned int numrules = 0;
	struct list_head *cur;

	list_for_each(cur, &chainh->rules) {
		numrules++;
	}

	if (numrules <=2)
		return 0;
	else
		return numrules-2;
}

/* Deletes a chain. */
int
TC_DELETE_CHAIN(const IPT_CHAINLABEL chain, TC_HANDLE_T *handle)
{
	unsigned int references;
	struct chain_head *chainh;

#if 0
	if (!TC_GET_REFERENCES(&references, chain, handle))
		return 0;

	iptc_fn = TC_DELETE_CHAIN;

	if (TC_BUILTIN(chain, *handle)) {
		errno = EINVAL;
		return 0;
	}

	if (references > 0) {
		errno = EMLINK;
		return 0;
	}
#endif 

	if (!(chainh = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	if (!(count_rules(chainh) == 0)) {
		errno = ENOTEMPTY;
		return 0;
	}

	chainh_free(chainh);
	return 1;
}

/* Renames a chain. */
int TC_RENAME_CHAIN(const IPT_CHAINLABEL oldname,
		    const IPT_CHAINLABEL newname,
		    TC_HANDLE_T *handle)
{
	struct chain_head *c;
	struct rule_head *ruleh;
	struct ipt_error_target *t;

	iptc_fn = TC_RENAME_CHAIN;

	/* find_label doesn't cover built-in targets: DROP, ACCEPT,
           QUEUE, RETURN. */
	if (find_label(newname, *handle)
	    || strcmp(newname, LABEL_DROP) == 0
	    || strcmp(newname, LABEL_ACCEPT) == 0
	    || strcmp(newname, LABEL_QUEUE) == 0
	    || strcmp(newname, LABEL_RETURN) == 0) {
		errno = EEXIST;
		return 0;
	}

	if (!(c = find_label(oldname, *handle))
	    || TC_BUILTIN(oldname, *handle)) {
		errno = ENOENT;
		return 0;
	}

	if (strlen(newname)+1 > sizeof(IPT_CHAINLABEL)) {
		errno = EINVAL;
		return 0;
	}

	ruleh = list_entry(&c->rules.next, struct rule_head, list);

	t = (struct ipt_error_target *)
		GET_TARGET(ruleh->entry);

	memset(t->error, 0, sizeof(t->error));
	strcpy(t->error, newname);

	return 1;
}

/* Sets the policy on a built-in chain. */
int
TC_SET_POLICY(const IPT_CHAINLABEL chain,
	      const IPT_CHAINLABEL policy,
	      STRUCT_COUNTERS *counters,
	      TC_HANDLE_T *handle)
{
	int ctrindex;
	unsigned int hook;
	struct chain_head *chainh;
	struct rule_head *policyrh;
	STRUCT_ENTRY *e;
	STRUCT_STANDARD_TARGET *t;

	iptc_fn = TC_SET_POLICY;
	/* Figure out which chain. */
	hook = TC_BUILTIN(chain, *handle);
	if (hook == 0) {
		errno = ENOENT;
		return 0;
	} else
		hook--;

	if (!(chainh = find_label(chain, *handle))) {
		errno = ENOENT;
		return 0;
	}

	policyrh = chainh->lastrule;
	if (policyrh) {
		printf("ERROR: Policy for `%s' non-existant", chain);
		return 0;
	}

	t = (STRUCT_STANDARD_TARGET *)GET_TARGET(policyrh->entry);

	if (strcmp(policy, LABEL_ACCEPT) == 0)
		t->verdict = -NF_ACCEPT - 1;
	else if (strcmp(policy, LABEL_DROP) == 0)
		t->verdict = -NF_DROP - 1;
	else {
		errno = EINVAL;
		return 0;
	}

	ctrindex = entry2index(*handle, e);

	if (counters) {
		/* set byte and packet counters */
		memcpy(&e->counters, counters, sizeof(STRUCT_COUNTERS));

		policyrh->counter_map.maptype = COUNTER_MAP_SET;

	} else {
		policyrh->counter_map.maptype = COUNTER_MAP_NOMAP;
		policyrh->counter_map.mappos = 0;
	}

	set_changed(*handle);

	return 1;
}

/* Without this, on gcc 2.7.2.3, we get:
   libiptc.c: In function `TC_COMMIT':
   libiptc.c:833: fixed or forbidden register was spilled.
   This may be due to a compiler bug or to impossible asm
   statements or clauses.
*/
static void
subtract_counters(STRUCT_COUNTERS *answer,
		  const STRUCT_COUNTERS *a,
		  const STRUCT_COUNTERS *b)
{
	answer->pcnt = a->pcnt - b->pcnt;
	answer->bcnt = a->bcnt - b->bcnt;
}

int
TC_COMMIT(TC_HANDLE_T *handle)
{
	/* Replace, then map back the counters. */
	STRUCT_REPLACE *repl;
	STRUCT_COUNTERS_INFO *newcounters;
	unsigned int i;
	size_t counterlen;

	CHECK(*handle);

	counterlen = sizeof(STRUCT_COUNTERS_INFO)
			+ sizeof(STRUCT_COUNTERS) * (*handle)->new_number;

#if 0
	TC_DUMP_ENTRIES(*handle);
#endif

	/* Don't commit if nothing changed. */
	if (!(*handle)->changed)
		goto finished;

	repl = malloc(sizeof(*repl) + (*handle)->entries.size);
	if (!repl) {
		errno = ENOMEM;
		return 0;
	}

	/* These are the old counters we will get from kernel */
	repl->counters = malloc(sizeof(STRUCT_COUNTERS)
				* (*handle)->info.num_entries);
	if (!repl->counters) {
		free(repl);
		errno = ENOMEM;
		return 0;
	}

	/* These are the counters we're going to put back, later. */
	newcounters = malloc(counterlen);
	if (!newcounters) {
		free(repl->counters);
		free(repl);
		errno = ENOMEM;
		return 0;
	}

	strcpy(repl->name, (*handle)->info.name);
	repl->num_entries = (*handle)->new_number;
	repl->size = (*handle)->entries.size;
	memcpy(repl->hook_entry, (*handle)->info.hook_entry,
	       sizeof(repl->hook_entry));
	memcpy(repl->underflow, (*handle)->info.underflow,
	       sizeof(repl->underflow));
	repl->num_counters = (*handle)->info.num_entries;
	repl->valid_hooks = (*handle)->info.valid_hooks;
	memcpy(repl->entries, (*handle)->entries.entrytable,
	       (*handle)->entries.size);

	if (setsockopt(sockfd, TC_IPPROTO, SO_SET_REPLACE, repl,
		       sizeof(*repl) + (*handle)->entries.size) < 0) {
		free(repl->counters);
		free(repl);
		free(newcounters);
		return 0;
	}

	/* Put counters back. */
	strcpy(newcounters->name, (*handle)->info.name);
	newcounters->num_counters = (*handle)->new_number;
	for (i = 0; i < (*handle)->new_number; i++) {
		unsigned int mappos = (*handle)->counter_map[i].mappos;
		switch ((*handle)->counter_map[i].maptype) {
		case COUNTER_MAP_NOMAP:
			newcounters->counters[i]
				= ((STRUCT_COUNTERS){ 0, 0 });
			break;

		case COUNTER_MAP_NORMAL_MAP:
			/* Original read: X.
			 * Atomic read on replacement: X + Y.
			 * Currently in kernel: Z.
			 * Want in kernel: X + Y + Z.
			 * => Add in X + Y
			 * => Add in replacement read.
			 */
			newcounters->counters[i] = repl->counters[mappos];
			break;

		case COUNTER_MAP_ZEROED:
			/* Original read: X.
			 * Atomic read on replacement: X + Y.
			 * Currently in kernel: Z.
			 * Want in kernel: Y + Z.
			 * => Add in Y.
			 * => Add in (replacement read - original read).
			 */
			subtract_counters(&newcounters->counters[i],
					  &repl->counters[mappos],
					  &index2entry(*handle, i)->counters);
			break;

		case COUNTER_MAP_SET:
			/* Want to set counter (iptables-restore) */

			memcpy(&newcounters->counters[i],
			       &index2entry(*handle, i)->counters,
			       sizeof(STRUCT_COUNTERS));

			break;
		}
	}

#ifdef KERNEL_64_USERSPACE_32
	{
		/* Kernel will think that pointer should be 64-bits, and get
		   padding.  So we accomodate here (assumption: alignment of
		   `counters' is on 64-bit boundary). */
		u_int64_t *kernptr = (u_int64_t *)&newcounters->counters;
		if ((unsigned long)&newcounters->counters % 8 != 0) {
			fprintf(stderr,
				"counters alignment incorrect! Mail rusty!\n");
			abort();
		}
		*kernptr = newcounters->counters;
	}
#endif /* KERNEL_64_USERSPACE_32 */

	if (setsockopt(sockfd, TC_IPPROTO, SO_SET_ADD_COUNTERS,
		       newcounters, counterlen) < 0) {
		free(repl->counters);
		free(repl);
		free(newcounters);
		return 0;
	}

	free(repl->counters);
	free(repl);
	free(newcounters);

 finished:
	TC_FREE(handle);
	return 1;
}

/* Get raw socket. */
int
TC_GET_RAW_SOCKET()
{
	return sockfd;
}

/* Translates errno numbers into more human-readable form than strerror. */
const char *
TC_STRERROR(int err)
{
	unsigned int i;
	struct table_struct {
		void *fn;
		int err;
		const char *message;
	} table [] =
	  { { TC_INIT, EPERM, "Permission denied (you must be root)" },
	    { TC_INIT, EINVAL, "Module is wrong version" },
	    { TC_INIT, ENOENT, 
		    "Table does not exist (do you need to insmod?)" },
	    { TC_DELETE_CHAIN, ENOTEMPTY, "Chain is not empty" },
	    { TC_DELETE_CHAIN, EINVAL, "Can't delete built-in chain" },
	    { TC_DELETE_CHAIN, EMLINK,
	      "Can't delete chain with references left" },
	    { TC_CREATE_CHAIN, EEXIST, "Chain already exists" },
	    { TC_INSERT_ENTRY, E2BIG, "Index of insertion too big" },
	    { TC_REPLACE_ENTRY, E2BIG, "Index of replacement too big" },
	    { TC_DELETE_NUM_ENTRY, E2BIG, "Index of deletion too big" },
	    { TC_READ_COUNTER, E2BIG, "Index of counter too big" },
	    { TC_ZERO_COUNTER, E2BIG, "Index of counter too big" },
	    { TC_INSERT_ENTRY, ELOOP, "Loop found in table" },
	    { TC_INSERT_ENTRY, EINVAL, "Target problem" },
	    /* EINVAL for CHECK probably means bad interface. */
	    { TC_CHECK_PACKET, EINVAL,
	      "Bad arguments (does that interface exist?)" },
	    { TC_CHECK_PACKET, ENOSYS,
	      "Checking will most likely never get implemented" },
	    /* ENOENT for DELETE probably means no matching rule */
	    { TC_DELETE_ENTRY, ENOENT,
	      "Bad rule (does a matching rule exist in that chain?)" },
	    { TC_SET_POLICY, ENOENT,
	      "Bad built-in chain name" },
	    { TC_SET_POLICY, EINVAL,
	      "Bad policy name" },

	    { NULL, 0, "Incompatible with this kernel" },
	    { NULL, ENOPROTOOPT, "iptables who? (do you need to insmod?)" },
	    { NULL, ENOSYS, "Will be implemented real soon.  I promise ;)" },
	    { NULL, ENOMEM, "Memory allocation problem" },
	    { NULL, ENOENT, "No chain/target/match by that name" },
	  };

	for (i = 0; i < sizeof(table)/sizeof(struct table_struct); i++) {
		if ((!table[i].fn || table[i].fn == iptc_fn)
		    && table[i].err == err)
			return table[i].message;
	}

	return strerror(err);
}
