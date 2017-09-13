/* 
 *
 * Gary Jennejohn <gj@denx.de>
 * Copyright (C) 2003 Gary Jennejohn
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * Reset the DANUBE board.
 *
 */

#include <linux/kernel.h>
#include <asm/reboot.h>
#include <asm/system.h>
#include <asm/danube/danube.h>

static void danube_machine_restart(char *command);
static void danube_machine_halt(void);
static void danube_machine_power_off(void);

static void danube_machine_restart(char *command)
{
	local_irq_disable();

	DANUBE_REG32(DANUBE_RCU_REQ) |= DANUBE_RST_ALL;
	for (;;) ;
}

static void danube_machine_halt(void)
{
	/* Disable interrupts and loop forever */
	printk(KERN_NOTICE "System halted.\n");
	local_irq_disable();
	for (;;) ;
}

static void danube_machine_power_off(void)
{
	/* We can't power off without the user's assistance */
	printk(KERN_NOTICE "Please turn off the power now.\n");
	local_irq_disable();
	for (;;) ;
}

void danube_reboot_setup(void)
{
	_machine_restart = danube_machine_restart;
	_machine_halt = danube_machine_halt;
	_machine_power_off = danube_machine_power_off;
}
