/*
 * (C) Copyright 2010
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __BOARD_PM_H__
#define __BOARD_PM_H__

#include <linux/suspend.h>

struct board_pm_fns {
	int  (*valid)		(suspend_state_t state);	/* before driver suspend */
	int  (*begin)		(suspend_state_t state);	/* before driver suspend */
	int  (*prepare)		(void);						/* after driver suspend */
	int  (*enter) 		(suspend_state_t state);	/* enter suspend */
	int  (*poweroff)	(void);						/* before cpu power off */
	void (*poweron)		(void);						/* after cpu power on */
	void (*finish)		(void);						/* before driver resume */
	void (*end)			(void);						/* after driver resume */
};

void cpu_register_board_pm(struct board_pm_fns *fns);
void cpu_register_board_shutdown(void *fns);

#endif /* __BOARD_PM_H__ */