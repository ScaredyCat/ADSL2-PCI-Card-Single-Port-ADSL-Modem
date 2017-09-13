/* Copyright 2000-2004 Joakim Axelsson (gozem@linux.nu)
 *                     Patrick Schaaf (bof@bof.de)
 *                     Jozsef Kadlecsik (kadlec@blackhole.kfki.hu)
 *
 * This program is free software; you can redistribute it and/or modify   
 * it under the terms of the GNU General Public License as published by   
 * the Free Software Foundation; either version 2 of the License, or      
 * (at your option) any later version.                                    
 *                                                                         
 * This program is distributed in the hope that it will be useful,        
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
 * GNU General Public License for more details.                           
 *                                                                         
 * You should have received a copy of the GNU General Public License      
 * along with this program; if not, write to the Free Software            
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* Kernel module for IP set management */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/skbuff.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/softirq.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

#define ASSERT_READ_LOCK(x)	/* dont use that */
#define ASSERT_WRITE_LOCK(x)
#include <linux/netfilter_ipv4/listhelp.h>
#include <linux/netfilter_ipv4/ip_set.h>

static struct list_head set_type_list;		/* all registred set types */
struct ip_set **ip_set_list;			/* all individual sets */
static rwlock_t list_lock = RW_LOCK_UNLOCKED;	/* protects both set_type_list and ip_set_list */
static unsigned int max_sets = 0;		/* max number of sets, */

/* Arrgh */
#ifdef MODULE
#define __MOD_INC(foo) __MOD_INC_USE_COUNT(foo)
#define __MOD_DEC(foo) __MOD_DEC_USE_COUNT(foo)
#else
#define __MOD_INC(foo) do { } while (0)
#define __MOD_DEC(foo) do { } while (0)
#endif

#define NOT_IN_CHILD_SET(fn,args...) \
	!*private \
	|| !(*private)->childsets \
	|| (set->type[i]->fn(*private,##args) < 0)

static struct ip_set_private **
ip_set_find_private(struct ip_set *set,
		    struct ip_set_private **private,
		    ip_set_ip_t *ip,
		    u_int8_t level)
{
	int i;
	ip_set_ip_t id;

	for (i = 0; i < level; i++) {
		if (NOT_IN_CHILD_SET(matchip, ip[i], &id))
			return NULL;
		private = &(*private)->childsets[id];
	}
	DP("id: %i private: %p %p", id, private, *private);
	return private;
}

/* Destroy function for the private part of the (child)sets.
 * Must be called without holding any locks.
 */
static void
ip_set_destroy_private(struct ip_set *set,
		       struct ip_set_private **private,
		       u_int8_t level)
{
	int i;
	
	DP("set %p private %p %p %p", set, private, *private, (*private)->childsets);
	if ((*private)->childsets) {
		for (i = 0; i < set->type[level]->sizeid(*private); i++)
			if ((*private)->childsets[i]) {
				DP("%i -> %p", i, (*private)->childsets[i]);
				ip_set_destroy_private(set,
						       &(*private)->childsets[i],
						       level + 1);
			}
		vfree((*private)->childsets);
	}

	set->type[level]->destroy(private);
	DP("%p %p", private, *private);
}

static void ip_set_flush_private(struct ip_set *set,
			         struct ip_set_private *private,
			         u_int8_t level,
			         u_int8_t childsets)
{
	int i;
	
	if (childsets && private->childsets)
		for (i = 0; i < set->type[level]->sizeid(private); i++)
			if (private->childsets[i])
				ip_set_flush_private(set,
						     private->childsets[i],
						     level + 1,
						     childsets);

	set->type[level]->flush(private);
	
}

/* ip_set_flush() - flush data in a set
 */
static int ip_set_flush(struct ip_set *set,
			ip_set_ip_t *ip, 
			u_int8_t level,
			u_int8_t childsets)
{
	int res = 0;
	struct ip_set_private **private;
	
	write_lock_bh(&set->lock);
	if (set->subref) {
		res = -EBUSY;
		goto unlock;
	}
	
	private = ip_set_find_private(set, &set->private, ip, level);
		
	if (private)
		ip_set_flush_private(set, *private, level, childsets);

    unlock:
	write_unlock_bh(&set->lock);
	return res;
}

int
ip_set_testip_kernel(struct ip_set *set,
		     const struct sk_buff *skb,
		     const u_int32_t *flags,
		     u_int8_t set_level,
		     u_int8_t ip_level)
{
	struct ip_set_private **private = &set->private;
	ip_set_ip_t id;
	int i, res = 0;
	
	read_lock_bh(&set->lock);
	if (set->levels < ip_level || set->subref)
		goto unlock;
	
	for (i = 0; i < set_level; i++) {
		if (NOT_IN_CHILD_SET(testip_kernel, skb,
				     flags[i] | set->type[i]->typecode, &id))
			goto unlock;
		DP("id: %i private: %p", id, *private);
		private = &(*private)->childsets[id];
	}
	for (i = set_level; private && *private && i < ip_level; i++) {
		if (set->type[i]->testip_kernel(*private, skb,
				flags[i] | set->type[i]->typecode, &id) <= 0)
			goto unlock;
		private = (*private)->childsets 
				? &(*private)->childsets[id] : NULL;
	}
	res = 1;
    unlock:
	read_unlock_bh(&set->lock);
	return res;
}

void
ip_set_addip_kernel(struct ip_set *set, 
		    const struct sk_buff *skb,
		    const u_int32_t *flags,
		    u_int8_t set_level,
		    u_int8_t ip_level)
{
	struct ip_set_private **private = &set->private;
	ip_set_ip_t id;
	int i, res;

	write_lock_bh(&set->lock);
	if (set->levels < ip_level || set->subref) {
		write_unlock_bh(&set->lock);
		return;
	}
	for (i = 0; i < set_level; i++) {
		if (NOT_IN_CHILD_SET(testip_kernel, skb,
				     flags[i] | set->type[i]->typecode, &id)) {
			write_unlock_bh(&set->lock);
			return;
		}
		private = &(*private)->childsets[id];
	}
	for (i = set_level; private && *private && i < ip_level; i++) {
		res = set->type[i]->addip_kernel(*private, skb,
			flags[i] | set->type[i]->typecode, &id);
		if (!(res == 0 || res == -EEXIST)) {
			write_unlock_bh(&set->lock);
			return;
		}
		private = (*private)->childsets 
				? &(*private)->childsets[id] : NULL;
	}
	write_unlock_bh(&set->lock);
}

void
ip_set_delip_kernel(struct ip_set *set,
		    const struct sk_buff *skb,
		    const u_int32_t *flags,
		    u_int8_t set_level,
		    u_int8_t ip_level)
{
	struct ip_set_private **private = &set->private;
	ip_set_ip_t id;
	int i, res;

	write_lock_bh(&set->lock);
	if (set->levels < ip_level || set->subref) {
		write_unlock_bh(&set->lock);
		return;
	}
	for (i = 0; i < set_level; i++) {
		if (NOT_IN_CHILD_SET(testip_kernel, skb,
				     flags[i] | set->type[i]->typecode, &id)) {
			write_unlock_bh(&set->lock);
			return;
		}
		private = &(*private)->childsets[id];
	}
	for (i = set_level; private && *private && i < ip_level; i++) {
		res = set->type[i]->delip_kernel(*private, skb,
			flags[i] | set->type[i]->typecode, &id);
		if (!(res == 0 || res == -EEXIST)) {
			write_unlock_bh(&set->lock);
			return;
		}
		private = (*private)->childsets 
				? &(*private)->childsets[id] : NULL;
	}
	write_unlock_bh(&set->lock);
}

static int
ip_set_addip(struct ip_set *set,
	     ip_set_ip_t *ip,
	     u_int8_t level,
	     const void *data,
	     size_t size)
{
	struct ip_set_private **private;
	ip_set_ip_t id;
	int res = 0;

	DP("%s %i %d", set->name, level, size);
	write_lock_bh(&set->lock);
	if (set->subref) {
		res = -EBUSY;
		goto unlock;
	}
	private = ip_set_find_private(set, &set->private, ip, level);
	DP("%s %i %d", set->name, level, size);
	while (level <= set->levels && size) {
		DP("%s %i %d", set->name, level, size);
		if (!(private && *private)) {
			res = -ENOENT;
			goto unlock;
		}
		if (size < set->type[level]->reqsize) {
			res = -EINVAL;
			goto unlock;
		}
		res = set->type[level]->addip(*private, data, 
					set->type[level]->reqsize, &id);
		if (!(res == 0 || res == -EEXIST))
			goto unlock;
		private = (*private)->childsets ? &(*private)->childsets[id] : NULL;
		data += set->type[level]->reqsize;
		size -= set->type[level++]->reqsize;
	}
	if (size)
		res = -EINVAL;
    unlock:
	write_unlock_bh(&set->lock);
	return res;
}

static int
ip_set_delip(struct ip_set *set,
	     ip_set_ip_t *ip,
	     u_int8_t level,
	     const void *data,
	     size_t size)
{
	struct ip_set_private **private;
	ip_set_ip_t id;
	int res = 0;

	write_lock_bh(&set->lock);
	if (set->subref) {
		res = -EBUSY;
		goto unlock;
	}
	private = ip_set_find_private(set, &set->private, ip, level);
	while (level <= set->levels && size) {
		if (!(private && *private)) {
			res = -ENOENT;
			goto unlock;
		}
		if (size < set->type[level]->reqsize) {
			res = -EINVAL;
			goto unlock;
		}
		res = set->type[level]->delip(*private, data, 
					set->type[level]->reqsize, &id);
		if (!(res == 0 || res == -EEXIST))
			goto unlock;
		private = (*private)->childsets ? &(*private)->childsets[id] : NULL;
		data += set->type[level]->reqsize;
		size -= set->type[level++]->reqsize;
	}
	if (size)
		res = -EINVAL;
    unlock:
	write_unlock_bh(&set->lock);
	return res;
}

static int
ip_set_testip(struct ip_set *set,
	      ip_set_ip_t *ip,
	      u_int8_t level,
	      const void *data,
	      size_t size)
{
	struct ip_set_private **private;
	ip_set_ip_t id;
	int res = 0;

	write_lock_bh(&set->lock);
	if (set->subref) {
		res = -EBUSY;
		goto unlock;
	}
	private = ip_set_find_private(set, &set->private, ip, level);
	while (level <= set->levels && size) {
		if (!(private && *private)) {
			res = -ENOENT;
			goto unlock;
		}
		if (size < set->type[level]->reqsize) {
			res = -EINVAL;
			goto unlock;
		}
		res = set->type[level]->testip(*private, data, 
					set->type[level]->reqsize, &id);
		DP("level: %i res: %i", level, res);
		if (res <= 0)
			goto unlock;
		private = (*private)->childsets ? &(*private)->childsets[id] : NULL;
		data += set->type[level]->reqsize;
		size -= set->type[level++]->reqsize;
	}
	if (size)
		res = -EINVAL;
    unlock:
	write_unlock_bh(&set->lock);
	return (res > 0);
}

static inline int
set_type_equal(const struct ip_set_type *set_type, const char *str2)
{
	DP("'%s' vs. '%s'", set_type->typename, str2);
	return !strncmp(set_type->typename, str2, IP_SET_MAXNAMELEN - 1);
}

/*
 * Always use find_setfoo() under the &list_lock.
 */
static inline struct ip_set_type *find_set_type(const char name[IP_SET_MAXNAMELEN])
{
	return LIST_FIND(&set_type_list,
			 set_type_equal,
			 struct ip_set_type *,
			 name);
}

int ip_set_register_set_type(struct ip_set_type *set_type)
{
	if (set_type->protocol_version != IP_SET_PROTOCOL_VERSION) {
		ip_set_printk("'%s' uses wrong protocol version %u (want %u)",
			      set_type->typename,
			      set_type->protocol_version,
			      IP_SET_PROTOCOL_VERSION);
		return -EINVAL;
	}

	write_lock_bh(&list_lock);
	if (find_set_type(set_type->typename)) {
		/* Duplicate! */
		write_unlock_bh(&list_lock);
		ip_set_printk("'%s' already registered!", 
			      set_type->typename);
		return -EINVAL;
	}
	MOD_INC_USE_COUNT;
	list_append(&set_type_list, set_type);
	write_unlock_bh(&list_lock);
	DP("'%s' registered.", set_type->typename);
	return 0;
}

void ip_set_unregister_set_type(struct ip_set_type *set_type)
{
	write_lock_bh(&list_lock);
	if (!find_set_type(set_type->typename)) {
		ip_set_printk("'%s' not registered?",
			      set_type->typename);
		write_unlock_bh(&list_lock);
		return;
	}
	LIST_DELETE(&set_type_list, set_type);
	write_unlock_bh(&list_lock);
	MOD_DEC_USE_COUNT;

	DP("'%s' unregistered.", set_type->typename);
}

/* Create the private part of a (child)set.
 * Must be called without holding any locks.
 */
static int
ip_set_create_private(struct ip_set_type *set_type,
		      struct ip_set_private **private,
		      const void *data,
		      size_t size,
		      u_int8_t childsets)
{
	int res = 0;
	int newbytes;

	DP("%s %p %p %i", set_type->typename, private, *private, childsets);

	if (*private)
		printk("%p: %p as private already occupied", private, *private);

	/* Call the set_type initializer. */
	res = set_type->create(private, data, size);
	if (res != 0)
		return res;

	if (!childsets) {
		(*private)->childsets = NULL;
		return res;
	}
				
	/* Create room for subsets */
	newbytes = set_type->sizeid(*private) * sizeof(struct ip_set_private *);
	DP("%s (%p) %i", set_type->typename, *private, newbytes);
	(*private)->childsets = vmalloc(newbytes);	\
	if (!(*private)->childsets) {
		set_type->destroy(private);
		return -ENOMEM;
	}
	DP("%s %p %p %p", set_type->typename, private, *private, (*private)->childsets);
	memset((*private)->childsets, 0, newbytes);
	return res;
}

static int
ip_set_create_childset(struct ip_set *set, 
		       ip_set_ip_t *ip,
		       u_int8_t level,
		       u_int8_t childsets,
		       const void *data,
		       size_t size)
{
	struct ip_set_private **private = &set->private;
	ip_set_ip_t id;
	int res;

	DP("%s (%i %i)", set->name, level, childsets);
	write_lock_bh(&set->lock);
	if (set->subref) {
		res = -EBUSY;
		goto unlock;
	}
	if (level > 1)
		private = ip_set_find_private(set, private, ip, level - 1);
	DP("%s (%i %i) %p %p", set->name, level, childsets, private, *private);
	if (!(private && *private && (*private)->childsets)) {
		res = -ENOENT;
		goto unlock;
	}
	DP("%s (%i %i) %p %p", set->name, level, childsets, private, *private);
	set->type[level - 1]->matchip(*private, ip[level - 1], &id);
	DP("%s (%i %i) %p %p %i", set->name, level, childsets, private, *private, id);
	if (id < 0) {
		res = -ENOENT;
		goto unlock;
	}
	if ((*private)->childsets[id]) {
		res = -EEXIST;
		goto unlock;
	}
	set->subref++;
	write_unlock_bh(&set->lock);
	
	/* Without holding any locks, create private part.  */
	res = ip_set_create_private(set->type[level],
				    &(*private)->childsets[id],
				    data, size, childsets);

	write_lock_bh(&set->lock);
	set->subref--;
    unlock:
	DP("%s (%p %p) res=%i", set->name, private, *private, res);
	write_unlock_bh(&set->lock);
	return res;
}

static int
ip_set_create(const char name[IP_SET_MAXNAMELEN],
	      char typename[IP_SET_LEVELS][IP_SET_MAXNAMELEN],
	      u_int8_t level,
	      const void *data,
	      size_t size)
{
	int i, id, res = 0;
	struct ip_set *set;

	DP("%s (%i): %s", typename[0], level, name);
	/*
	 * First, and without any locks, allocate and initialize
	 * a normal base set structure.
	 */
	set = kmalloc(sizeof(struct ip_set), GFP_KERNEL);
	if (!set)
		return -ENOMEM;
	set->lock = RW_LOCK_UNLOCKED;
	strncpy(set->name, name, IP_SET_MAXNAMELEN);
	set->name[IP_SET_MAXNAMELEN - 1] = '\0';
	set->ref = 0;
	set->subref = 0;
	set->levels = level;
	set->private = NULL;

	/*
	 * Next, take the &list_lock, check that we know the type,
	 * and take a reference on the type, to make sure it
	 * stays available while constructing our new set.
	 *
	 * After referencing the type, we drop the &list_lock,
	 * and let the new set construction run without locks.
	 */
	write_lock_bh(&list_lock);
	for (i = 0; i < level; i++) {
		set->type[i] = find_set_type(typename[i]);
		if (set->type[i] == NULL) {
			/* FIXME: try loading the module */
			write_unlock_bh(&list_lock);
			ip_set_printk("no set type '%s', set '%s' not created",
				      typename[i], name);
			kfree(set);
			return -EINVAL;
		}
	}
	for (i = 0; i < level; i++)
		__MOD_INC(set->type[i]->me);
	write_unlock_bh(&list_lock);

	/*
	 * Without holding any locks, create private part.
	 */
	res = ip_set_create_private(set->type[0],
				    &set->private,
				    data, size, level - 1);				
	if (res != 0) {
		for (i = 0; i <= level; i++)
			__MOD_DEC(set->type[i]->me);
		kfree(set);
		return res;
	}

	/* BTW, res==0 here. */

	/*
	 * Here, we have a valid, constructed set. &list_lock again,
	 * and check that it is not already in ip_set_list.
	 */
	write_lock_bh(&list_lock);
	id = -1;
	for (i = 0; i < max_sets; i++) {
		if (ip_set_list[i] != NULL 
		    && strncmp(ip_set_list[i]->name, set->name, 
			       IP_SET_MAXNAMELEN - 1) == 0) {
			res = -EEXIST;
			goto cleanup;
		} else if (id < 0 && ip_set_list[i] == NULL)
			id = i;
	}
	if (id < 0) {
		/* No free slot remained */
		res = -ERANGE;
		goto cleanup;
	}
	/*
	 * Finally! Append our shiny new set into the list, and be done.
	 */
	DP("create: '%s' created with id %i!", set->name, id);
	ip_set_list[id] = set;
	write_unlock_bh(&list_lock);
	return res;
	
    cleanup:
	write_unlock_bh(&list_lock);
	ip_set_destroy_private(set, &set->private, 0);
	for (i = 0; i < level; i++)
		__MOD_DEC(set->type[i]->me);
	kfree(set);
	return res;
}

static int ip_set_destroy(struct ip_set *set,
			  ip_set_ip_t *ip,
			  u_int8_t level)
{
	struct ip_set_private **private;
	int i, res = 0;

	write_lock_bh(&list_lock);
	/* there is no race, here. ->ref modification always happens
	 * under &list_lock. Fine.
	 */
	if (level == 0) {
		/* one ref from caller */
		if (set->ref > 1 || set->subref) {
			res = -EBUSY;
			goto unlock;
		}

		for (i = 0; i < max_sets; i++)
			if (ip_set_list[i] == set) {
				ip_set_list[i] = NULL;
				break;
			}
		write_unlock_bh(&list_lock);

		ip_set_destroy_private(set, &set->private, 0);
		for (i = 0; i < set->levels; i++)
			__MOD_DEC(set->type[i]->me);
		kfree(set);
		return res;
	}

	private = ip_set_find_private(set, &set->private, 
				      ip, level);
		
	if (private && *private) {
		if (set->subref) {
			res = -EBUSY;
			goto unlock;
		}
		set->subref++;
		write_unlock_bh(&list_lock);
		
		DP("%p %p", private, *private);
		ip_set_destroy_private(set, private, level);
		DP("%p %p", private, *private);

		write_lock_bh(&list_lock);
		set->subref--;
	} else
		res = -ENOENT;
		
    unlock:
	write_unlock_bh(&list_lock);
	return res;
}

/*
 * Find set by name, reference it once.  The reference makes sure the
 * thing pointed to, does not go away under our feet.  Drop the reference
 * later, using ip_set_put().
 */
struct ip_set *ip_set_get_byname(const char name[IP_SET_MAXNAMELEN],
				 int *id)
{
	struct ip_set *set = NULL;
	int i;

	read_lock_bh(&list_lock);
	for (i = 0; i < max_sets; i++) {
		set = ip_set_list[i];
		if (set != NULL
		    && strncmp(set->name, name, IP_SET_MAXNAMELEN - 1) == 0) {
			set->ref++;
			*id = i;
			break;
		}
	}
	read_unlock_bh(&list_lock);
	return set;
}

/*
 * Find set by id, reference it once.  The reference makes sure the
 * thing pointed to, does not go away under our feet.  Drop the reference
 * later, using ip_set_put().
 */
struct ip_set *ip_set_get_byid(int id)
{
	struct ip_set *set;

	if (id < 0 || id >= max_sets)
		return NULL;
		
	write_lock_bh(&list_lock);
	set = ip_set_list[id];;
	if (set)
		set->ref++;
	write_unlock_bh(&list_lock);
	return set;
}

/*
 * If the given set pointer points to a valid set, decrement
 * reference count by 1.  The caller shall not assume the pointer
 * to be valid, after calling this function.
 */
void ip_set_put(struct ip_set *set)
{
	write_lock_bh(&list_lock);
	if (set)
		set->ref--;
	write_unlock_bh(&list_lock);
}

static int ip_set_rename(struct ip_set *set, const char *name)
{
	int i, res = 0;

	write_lock_bh(&list_lock);
	for (i = 0; i < max_sets; i++) {
		if (ip_set_list[i] != NULL 
		    && strncmp(ip_set_list[i]->name, 
			       name,
			       IP_SET_MAXNAMELEN - 1) == 0) {
			res = -EEXIST;
			goto unlock;
		}
	}
	strncpy(set->name, name, IP_SET_MAXNAMELEN);
	set->name[IP_SET_MAXNAMELEN - 1] = '\0';
    unlock:
	write_unlock_bh(&list_lock);
	return res;
}

static int ip_set_swap(struct ip_set *from, struct ip_set *to)
{
	char from_name[IP_SET_MAXNAMELEN];
	unsigned from_ref;
	int i, res = 0;
	int from_id = -1, to_id = -1;

	write_lock_bh(&list_lock);
	for (i = 0; i < max_sets && (from_id < 0 || to_id < 0); i++) {
		if (ip_set_list[i] == from)
			from_id = i;
		if (ip_set_list[i] == to)
			to_id = i;
	}
	/* We must have got both sets: we hold refcounts against them! */
	if (from_id < 0 || to_id < 0) {
		res = -EINVAL;
		goto unlock;
	}
	
	strncpy(from_name, from->name, IP_SET_MAXNAMELEN);
	from_ref = from->ref;
	
	ip_set_list[from_id] = to;
	ip_set_list[to_id] = from;
	
	strncpy(from->name, to->name, IP_SET_MAXNAMELEN);
	from->ref = to->ref;
	strncpy(to->name, from_name, IP_SET_MAXNAMELEN);
	to->ref = from_ref;
    unlock:
	write_unlock_bh(&list_lock);
	return res;
}

size_t ip_set_listing_size(void)
{
	size_t size = 0;
	int id;

	read_lock_bh(&list_lock);
	for (id = 0; id < max_sets; id++) {
		if (ip_set_list[id] != NULL)
			size += sizeof(struct ip_set_req_listing);
	}
	read_unlock_bh(&list_lock);

	return size;
}

int ip_set_listing(void *data, int *len)
{
	int used = 0;
	int res = 0;		/* All OK */
	int i, id;
	struct ip_set *set;
	struct ip_set_req_listing *header = data;

	read_lock_bh(&list_lock);
	for (id = 0; id < max_sets; id++) {
		if (ip_set_list[id] == NULL)
			continue;
	
		/* Pointer to our header */
		header = (struct ip_set_req_listing *) (data + used);

		DP("used before= %d %p %p %p", used, header, data,
		   data + used);

		/* Get and ensure header size */
		if (used + sizeof(struct ip_set_req_listing) > *len)
			goto not_enough_mem;

		set = ip_set_list[id];

		/* Fill with data */
		strncpy(header->name, set->name, IP_SET_MAXNAMELEN - 1);
		for (i = 0; i < set->levels; i++)
			strncpy(header->typename[i], set->type[i]->typename,
				IP_SET_MAXNAMELEN - 1);
		header->levels = set->levels;
		header->ref = set->ref;
		header->id = id;

		used += sizeof(struct ip_set_req_listing);
		DP("used after= %d", used);
	}
	*len = used;		/* How much did we use? */
	goto unlock_and_return;

    not_enough_mem:
	DP("not enough mem, try again");
	res = -ENOMEM;

    unlock_and_return:
	read_unlock_bh(&list_lock);
	return res;
}

int ip_set_list_size(struct ip_set * set,
		     ip_set_ip_t *ip,
		     unsigned level,
		     size_t *size,
		     unsigned op)
{
	int res = 0;	/* OK */
	struct ip_set_private **private;

	DP("%d %s %d", op, set->name, level);
	read_lock_bh(&set->lock);
	if (set->subref) {
		res = -EBUSY;
		goto unlock;
	}
	private = ip_set_find_private(set, &set->private, ip, level);
	if (!(private && *private)) {
		res = -ENOENT;
		goto unlock;
	}
	switch (op) {
	case IP_SET_OP_LIST_HEADER_SIZE:
		*size = set->type[level]->list_header_size(*private);
		break;
	case IP_SET_OP_LIST_MEMBERS_SIZE:
		*size = set->type[level]->list_members_size(*private);
		break;
	case IP_SET_OP_LIST_CHILDSETS_SIZE:
		*size = (*private)->childsets == NULL ? 0
			: bitmap_bytes(0, set->type[level]->sizeid(*private) - 1);
		break;
	default:
		res = -EINVAL;
	}
   unlock:
	read_unlock_bh(&set->lock);
	DP("%d %s %d: %u", op, set->name, level, *size);

	return res;
}

static void list_childsets(const struct ip_set_private *private, 
			   void *data,
			   ip_set_ip_t sizeid)
{
	ip_set_ip_t id;
	
	memset(data, 0, bitmap_bytes(0, sizeid - 1));

	if (private->childsets == NULL)
		return;
	
	for (id = 0; id < sizeid; id++)
		if (private->childsets[id] != NULL)
			set_bit(id, data);
}

int ip_set_list_data(struct ip_set *set,
		     ip_set_ip_t *ip,
		     unsigned level,
		     void *data,
		     int *len,
		     unsigned op)
{
	int res = 0;		/* All OK */
	size_t need;
	struct ip_set_private **private;
	void (*datafn)(const struct ip_set_private *, void *);

	read_lock_bh(&set->lock);
	if (set->subref) {
		res = -EBUSY;
		goto unlock;
	}
	private = ip_set_find_private(set, &set->private, ip, level);
	if (!(private && *private)) {
		res = -ENOENT;
		goto unlock;
	}
	switch (op) {
	case IP_SET_OP_LIST_HEADER:
		need = set->type[level]->list_header_size(*private);
		datafn = set->type[level]->list_header;
		break;
	case IP_SET_OP_LIST_MEMBERS:
		need = set->type[level]->list_members_size(*private);
		datafn = set->type[level]->list_members;
		break;
	case IP_SET_OP_LIST_CHILDSETS:
		if ((*private)->childsets == NULL) {
			res = -ENOENT;
			goto unlock;
		}
		need = bitmap_bytes(0, set->type[level]->sizeid(*private) - 1);
		datafn = NULL;
		break;
	default:
		res = -EINVAL;
		goto unlock;
	}
	if (need > *len) {
		res = -ENOMEM;
		goto unlock;
	}
	*len = need;
	if (datafn)
		datafn(*private, data);
	else
		list_childsets(*private, data, set->type[level]->sizeid(*private));

    unlock:
	read_unlock_bh(&set->lock);
	return res;
}

static int
ip_set_sockfn_set(struct sock *sk, int optval, void *user, unsigned int len)
{
	void *data;
	int res = 0;		/* Assume OK */
	struct ip_set_req_base *req_base;
	struct ip_set_req_std *req_std;
	struct ip_set *set = NULL;

	DP("optval=%d, user=%p, len=%d", optval, user, len);
	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	if (optval != SO_IP_SET)
		return -EBADF;
	if (len < sizeof(struct ip_set_req_base)) {
		ip_set_printk("short userdata (want >=%d, got %d)",
			      sizeof(struct ip_set_req_base), len);
		return -EINVAL;
	}
	data = vmalloc(len);
	if (!data) {
		DP("out of mem for %d bytes", len);
		return -ENOMEM;
	}
	if (copy_from_user(data, user, len) != 0) {
		res = -EFAULT;
		goto done;
	}

	req_base = (struct ip_set_req_base *) data;

	DP("op=%x id='%x'", req_base->op, req_base->id);
	
	/* Handle set creation first - no incoming set specified */

	if (req_base->op == IP_SET_OP_CREATE) {
		struct ip_set_req_create *req_create
			= (struct ip_set_req_create *) data;
		int i;
		
		if (len < sizeof(struct ip_set_req_create)) {
			ip_set_printk("short CREATE data (want >%d, got %d)",
				      sizeof(struct ip_set_req_create), len);
			res = -EINVAL;
			goto done;
		}
		if (req_create->levels > IP_SET_LEVELS) {
			ip_set_printk("set level %d too deep (max %d)",
				      req_create->levels, IP_SET_LEVELS);
			res = -EINVAL;
			goto done;
		}
		req_create->name[IP_SET_MAXNAMELEN - 1] = '\0';
		for (i = 0; i < req_create->levels; i++)
			req_create->typename[i][IP_SET_MAXNAMELEN - 1] = '\0';
		res = ip_set_create(req_create->name,
				    req_create->typename,
				    req_create->levels,
				    data + sizeof(struct ip_set_req_create),
				    len - sizeof(struct ip_set_req_create));
		goto done;
	}

	/* All remaining requests want a set by id.
	 * We take a proper reference here, and drop it after processing.
	 * From hereon, code goes to '*put_set', not to 'done'.
	 */

	set = ip_set_get_byid(req_base->id);
	if (set == NULL) {
		res = -ESRCH;
		goto done;
	} 
	
	DP("set %s (%d) (%u)", set->name, req_base->id, set->ref);
	/* Simple requests: no subsets */
	switch (req_base->op) {
	case IP_SET_OP_RENAME:{
			struct ip_set_req_rename *req_rename
				= (struct ip_set_req_rename *) data;

			if (len != sizeof(struct ip_set_req_rename)) {
				ip_set_printk("short RENAME data (want >%d, got %d)",
					      sizeof(struct ip_set_req_rename), len);
				res = -EINVAL;
				goto put_set;
			}

			res = ip_set_rename(set, req_rename->newname);
			goto put_set;
		}

	case IP_SET_OP_SWAP:{
			struct ip_set_req_swap *req_swap
				= (struct ip_set_req_swap *) data;
			struct ip_set *to;

			if (len != sizeof(struct ip_set_req_swap)) {

				ip_set_printk("short SWAP data (want >%d, got %d)",
					      sizeof(struct ip_set_req_swap), len);
				res = -EINVAL;
				goto put_set;
			}

			to = ip_set_get_byid(req_swap->to);
			if (to == NULL) {
				res = -ESRCH;
				goto put_set;
			}
			res = ip_set_swap(set, to);
			ip_set_put(to);
			goto put_set;
		}
	default: 
		; /* Requests with possible subsets: fall trough. */
	}
	
	req_std = (struct ip_set_req_std *) data;
	if (len < sizeof(struct ip_set_req_std)) {
		ip_set_printk("short data in std request (want >%d, got %d)",
			      sizeof(struct ip_set_req_std), len);
		res = -EINVAL;
		goto put_set;
	} else if (req_std->level >= set->levels) {
		res = -EINVAL;
		goto put_set;
	}

	switch (req_base->op) {
	case IP_SET_OP_ADD_IP:{
			res = ip_set_addip(set,
					   req_std->ip, req_std->level,
					   data + sizeof(struct ip_set_req_std),
					   len - sizeof(struct ip_set_req_std));
			goto put_set;
		}
	case IP_SET_OP_DEL_IP:{
			res = ip_set_delip(set,
					   req_std->ip, req_std->level,
					   data + sizeof(struct ip_set_req_std),
					   len - sizeof(struct ip_set_req_std));
			goto put_set;
		}
	case IP_SET_OP_DESTROY:{
			res = ip_set_destroy(set, req_std->ip, req_std->level);
			if (req_std->level == 0 && res == 0)
				goto done;	/* destroyed: no ip_set_put */
			goto put_set;
		}
	case IP_SET_OP_FLUSH:{
			struct ip_set_req_sub *req_sub =
				(struct ip_set_req_sub *) data;

			if (len < sizeof(struct ip_set_req_sub)) {
				ip_set_printk("short data in flush request (want >%d, got %d)",
					      sizeof(struct ip_set_req_sub), len);
				res = -EINVAL;
				goto put_set;
			}
			res = ip_set_flush(set, req_sub->ip, req_sub->level, req_sub->childsets);
			goto put_set;
		}
	case IP_SET_OP_CREATE_CHILD:{
			struct ip_set_req_sub *req_sub
				= (struct ip_set_req_sub *) data;
		
			if (len < sizeof(struct ip_set_req_sub)) {
				ip_set_printk("short CREATE_CHILD data (want >%d, got %d)",
					      sizeof(struct ip_set_req_sub), len);
				res = -EINVAL;
				goto put_set;
			}
			if (req_sub->level < 1) {
				/* No entry supplied? */
				res = -EINVAL;
				goto put_set;
			}
			if (((req_sub->level >= set->levels  - 1) && req_sub->childsets)) {
				/* No room for subsets to be created. */
				res = -ERANGE;
				goto put_set;
			}
			res = ip_set_create_childset(set,
					    req_sub->ip, 
					    req_sub->level, 
					    req_sub->childsets,
					    data + sizeof(struct ip_set_req_sub),
					    len - sizeof(struct ip_set_req_sub));
			goto put_set;
		}
	default:{
		DP("unknown op %d", req_base->op);
		ip_set_printk("obsolete - upgrade your ipset(8) utility.");
		res = -EINVAL;
		}
	} /* end of switch(op) */

    put_set:
	if (set)
		ip_set_put(set);
    done:
	vfree(data);
	if (res > 0)
		res = 0;
	return res;
}

static int 
ip_set_sockfn_get(struct sock *sk, int optval, void *user, int *len)
{
	int res = 0;
	struct ip_set_req_base *req_base;
	struct ip_set_req_std *req_std;
	struct ip_set *set = NULL;
	void *data;
	int copylen = *len;

	DP("optval=%d, user=%p, len=%d", optval, user, *len);
	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	if (optval != SO_IP_SET)
		return -EBADF;
	if (*len < sizeof(struct ip_set_req_base)) {
		ip_set_printk("short userdata (want >=%d, got %d)",
			      sizeof(struct ip_set_req_base), *len);
		return -EINVAL;
	}
	data = vmalloc(*len);
	if (!data) {
		DP("out of mem for %d bytes", *len);
		return -ENOMEM;
	}
	if (copy_from_user(data, user, *len) != 0) {
		res = -EFAULT;
		goto done;
	}

	req_base = (struct ip_set_req_base *) data;

	DP("op=%x id='%x'", req_base->op, req_base->id);

	/* Requests without a named set. */
	switch (req_base->op) {
	case IP_SET_OP_VERSION:{
			struct ip_set_req_version *req_version =
			    (struct ip_set_req_version *) data;

			if (*len != sizeof(struct ip_set_req_version)) {
				ip_set_printk("short VERSION (want >=%d, got %d)",
					      sizeof(struct ip_set_req_version),
					      *len);
				res = -EINVAL;
				goto done;
			}

			req_version->version = IP_SET_PROTOCOL_VERSION;
			res = copy_to_user(user, req_version,
					   sizeof(struct ip_set_req_version));
			goto done;
		}
	case IP_SET_OP_LISTING_SIZE:{
			struct ip_set_req_listing_size *req_list =
			    (struct ip_set_req_listing_size *) data;

			DP("IP_SET_OP_LISTING_SIZE");

			if (*len != sizeof(struct ip_set_req_listing_size)) {
				ip_set_printk("short LISTING_SIZE (want >=%d, got %d)",
					      sizeof(struct ip_set_req_listing_size),
					      *len);
				res = -EINVAL;
				goto done;
			}

			req_list->size = ip_set_listing_size();
			DP("req_list->size = %d", req_list->size);
			res = copy_to_user(user, req_list,
					   sizeof(struct ip_set_req_listing_size));
			goto done;
		}
	case IP_SET_OP_LISTING:{
			DP("LISTING before len=%d", *len);
			res = ip_set_listing(data, len);
			DP("LISTING done len=%d", *len);
			if (res < 0)
				goto done;	/* Error */

			res = copy_to_user(user, data, *len);	/* Only copy the mem used */
			goto done;
		}
	default: 
		; /* Requests with named set: fall trought */
	}

	/* Special cases: GETSET_BYNAME/BYID */
	switch (req_base->op) {
	case IP_SET_OP_GETSET_BYNAME: {
			struct ip_set_req_get *req_get
				= (struct ip_set_req_get *) data;

			if (*len != sizeof(struct ip_set_req_get)) {
				ip_set_printk("short _BYNAME (want >=%d, got %d)",
					      sizeof(struct ip_set_req_get), *len);
				res = -EINVAL;
				goto done;
			}
			req_get->name[IP_SET_MAXNAMELEN - 1] = '\0';
			req_get->id = -1;
			set = ip_set_get_byname(req_get->name, &req_get->id);
			if (set) {
				req_get->ref = set->ref - 1;
			   	ip_set_put(set);
			}
			res = copy_to_user(user, data, copylen);
			goto done;
		}
	case IP_SET_OP_GETSET_BYID: {
			struct ip_set_req_get *req_get
				= (struct ip_set_req_get *) data;

			if (*len != sizeof(struct ip_set_req_get)) {
				ip_set_printk("short _BYID (want >=%d, got %d)",
					       sizeof(struct ip_set_req_get), *len);
				res = -EINVAL;
				goto done;
			}
			set = ip_set_get_byid(req_get->id);
			if (set) {
				req_get->ref = set->ref - 1;
				strncpy(req_get->name, set->name, IP_SET_MAXNAMELEN);
			   	ip_set_put(set);
			} else 
				req_get->id = -1;
			res = copy_to_user(user, data, copylen);
			goto done;
		}
	default: 
		; /* Requests with set id: fall trought */
	}

	/* Requests with set id: */
	if (req_base->id < 0 || req_base->id >= max_sets) {
		res = -EINVAL;
		goto done;
	}
	set = ip_set_get_byid(req_base->id);	/* Reference lock */
	if (!set) {
		res = -ENOENT;
		goto done;
	}

	DP("set %s (%d) (%u)", set->name, req_base->id, set->ref);
	req_std = (struct ip_set_req_std *) data;
	if (*len < sizeof(struct ip_set_req_std)) {
		ip_set_printk("short data in std request (want >%d, got %d)",
			      sizeof(struct ip_set_req_std), *len);
		goto put_inval;
	} else if (req_std->level >= set->levels) {
		res = -ERANGE;
		goto put_set;
	}
	
	switch (req_base->op) {
	case IP_SET_OP_TEST_IP:{
			struct ip_set_req_test *req_test =
				(struct ip_set_req_test *) data;

			if (*len < sizeof(struct ip_set_req_test)) {
				ip_set_printk("short data in testip request (want >%d, got %d)",
					      sizeof(struct ip_set_req_test), *len);
				res = -EINVAL;
				goto put_set;
			}
			req_test->reply = ip_set_testip(set,
					    req_test->ip,
					    req_test->level,
					    data + sizeof(struct ip_set_req_test),
					    *len - sizeof(struct  ip_set_req_test));

			DP("test result: %i", req_test->reply);
			*len = copylen = sizeof(struct ip_set_req_test);
			goto put_copy;
		}
	case IP_SET_OP_LIST_HEADER_SIZE:
	case IP_SET_OP_LIST_MEMBERS_SIZE:
	case IP_SET_OP_LIST_CHILDSETS_SIZE: {
			struct ip_set_req_list *req_list =
			    (struct ip_set_req_list *) data;

			if (*len != sizeof(struct ip_set_req_list)) {
				ip_set_printk("short LIST (want >=%d, got %d)",
					      sizeof(struct ip_set_req_list),
					      *len);
				goto put_inval;
			}
			res =  ip_set_list_size(set,
						req_list->ip,
						req_list->level,
						&req_list->size,
						req_base->op);
			DP("SIZEfoo size=%d", req_list->size);
			if (res < 0)
				goto put_set;	/* Error */
			goto put_copy;
		}
	case IP_SET_OP_LIST_HEADER:
	case IP_SET_OP_LIST_MEMBERS:
	case IP_SET_OP_LIST_CHILDSETS:{
			DP("LISTfoo before len=%d", *len);
			res = ip_set_list_data(set,
					       req_std->ip,
					       req_std->level,
					       data,
					       len,
					       req_base->op);
			DP("LISTfoo done len=%d", *len);

			if (res < 0)
				goto put_set;	/* Error */

			copylen = *len;		/* Only copy the mem used */
			goto put_copy;
		}
	default:{
			DP("unknown op %d", req_base->op);
			ip_set_printk("obsolete - upgrade your ipset(8) utility.");
			goto put_inval;
		}
	}	/* end of switch(op) */

    put_copy:
   	ip_set_put(set);
   	DP("set %s (%u)", set->name, set->ref);
	res = copy_to_user(user, data, copylen);
	goto done;
    put_inval:
	res = -EINVAL;
    put_set:
	ip_set_put(set);
   	DP("set %s (%u)", set->name, set->ref);
    done:
	vfree(data);
	if (res > 0)
		res = 0;
	DP("final result %d", res);
	return res;
}

static struct nf_sockopt_ops so_set = {
	.pf 		= PF_INET,
	.set_optmin 	= SO_IP_SET,
	.set_optmax 	= SO_IP_SET + 1,
	.set 		= &ip_set_sockfn_set,
	.get_optmin 	= SO_IP_SET,
	.get_optmax	= SO_IP_SET + 1,
	.get		= &ip_set_sockfn_get,
	.use		= 0
};

MODULE_PARM(max_sets, "i");
MODULE_PARM_DESC(max_sets, "maximal number of sets");

static int __init init(void)
{
	int res;

	if (max_sets <= 0)
		max_sets = CONFIG_IP_NF_SET_MAX;
	ip_set_list = vmalloc(sizeof(struct ip_set *) * max_sets);
	if (!ip_set_list) {
		printk(KERN_ERR "Unable to create ip_set_list\n");
		return -ENOMEM;
	}
	memset(ip_set_list, 0, sizeof(struct ip_set *) * max_sets);
	INIT_LIST_HEAD(&set_type_list);

	res = nf_register_sockopt(&so_set);
	if (res != 0) {
		ip_set_printk("SO_SET registry failed: %d", res);
		vfree(ip_set_list);
		return res;
	}
	return 0;
}

static void __exit fini(void)
{
	nf_unregister_sockopt(&so_set);
	vfree(ip_set_list);
	DP("these are the famous last words");
}

EXPORT_SYMBOL(ip_set_register_set_type);
EXPORT_SYMBOL(ip_set_unregister_set_type);

EXPORT_SYMBOL(ip_set_list);
EXPORT_SYMBOL(ip_set_get_byname);
EXPORT_SYMBOL(ip_set_get_byid);
EXPORT_SYMBOL(ip_set_put);

EXPORT_SYMBOL(ip_set_addip_kernel);
EXPORT_SYMBOL(ip_set_delip_kernel);
EXPORT_SYMBOL(ip_set_testip_kernel);

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
