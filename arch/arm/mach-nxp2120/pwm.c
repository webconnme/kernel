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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/hardirq.h>

/* nexell soc headers */
#include <mach/platform.h>
#include <mach/soc.h>

#if (0)
#define DBGOUT(msg...)		{ printk("pwm: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

/*-----------------------------------------------------------------------------*/
struct pwm_device{
	u_int 	ch;			/* 0 ~ 1,  2 channels */
	u_int 	duty;		/* unit % 0% ~ 100% */
	u_int 	freq;		/* unit Hz 237Hz ~ 30KHz, default = 1000 */
	u_int 	enable;
	u_int	suspend_ch;
	u_int 	io;
	u_int 	iofunc;
};

static struct pwm_device gpwm[] = {
	{ 0,	0,	0,	0,	0, (PAD_GPIO_B +  2),	NX_GPIO_PADFUNC_1	},
	{ 1,	0, 	0,	0, 	0, (PAD_GPIO_B +  3), 	NX_GPIO_PADFUNC_1	},
	{ 2,	0, 	0,	0, 	0, (PAD_GPIO_C + 18), 	NX_GPIO_PADFUNC_3	},
};
#define	PWM_CHANNEL_NUM		(sizeof(gpwm)/sizeof(gpwm[0]))

/*-----------------------------------------------------------------------------
 *		|-----------------------------------------------|
 *	MIN Frequency			|
 *		|					|						MAX Frequency
 *	scale max (128) ~~~	scale min (1)				scale min (1)
 *	period max(1000		period max(1000) ~~~~~~~~~	period min(2)
 */
#define PWM_PERIOD_MIN		2
#define PWM_PERIOD_MAX		1000
#define PWM_PRESCL_MIN		1
#define PWM_PRESCL_MAX		128

#define PWM_CLOCK_SOURCE	(CFG_PWM_CLK_SOURCE)

#define TMP_DIVISOR			((CFG_PWM_CLK_FREQ + 80000000 - 1)/80000000)	/* peri max clock is 80Mhz */
#if (TMP_DIVISOR & 0x1)														/* support only even value */
#define PWM_CLOCK_DIVISOR	(TMP_DIVISOR + 1)
#else
#define PWM_CLOCK_DIVISOR	(TMP_DIVISOR)
#endif

#define PWM_FREQUENCY_MAX	(CFG_PWM_CLK_FREQ/PWM_CLOCK_DIVISOR)
#define PWM_FREQUENCY_MIN	(CFG_PWM_CLK_FREQ/PWM_PRESCL_MAX/PWM_PERIOD_MAX)
/*-----------------------------------------------------------------------------*/

static DEFINE_MUTEX(mutex);
static u_int g_clk_enb = 0;

#define	PWM_LOCK(id)		{ if (! preempt_count() && ! in_interrupt()) mutex_lock(&mutex);   }
#define	PWM_UNLOCK(id)		{ if (mutex_is_locked(&mutex)) mutex_unlock(&mutex); }

/*-----------------------------------------------------------------------------*/
void soc_pwm_init(void)
{
#if !defined (CFG_PWM_NO_INIT)
	int i = 0, io = 0;
#endif
	DBGOUT("%s (total ch:%d)\n", __func__, PWM_CHANNEL_NUM);
	NX_PWM_SetBaseAddress((U32)IO_ADDRESS(NX_PWM_GetPhysicalAddress()));
	NX_PWM_OpenModule();

	/* check bootloader pwm config */
	if (NX_PWM_GetClockDivisorEnable() &&
		PWM_CLOCK_DIVISOR != NX_PWM_GetClockDivisor(0))
	{
		NX_PWM_SetClockDivisorEnable(CFALSE);
		NX_PWM_SetClockSource	(0, PWM_CLOCK_SOURCE);
		NX_PWM_SetClockDivisor	(0, PWM_CLOCK_DIVISOR);
		NX_PWM_SetClockPClkMode	(NX_PCLKMODE_DYNAMIC);
		NX_PWM_SetClockDivisorEnable(CTRUE);
	}

#if !defined (CFG_PWM_NO_INIT)
	NX_PWM_SetClockPClkMode	(NX_PCLKMODE_DYNAMIC);
	NX_PWM_SetClockDivisorEnable(CFALSE);

	for (i = 0; PWM_CHANNEL_NUM > i; i++) {
		io = gpwm[i].io;
		soc_gpio_set_out_value(io, 0);
		soc_gpio_set_io_dir(io, 1);
		soc_gpio_set_io_pullup(io, 0);
		soc_gpio_set_io_func(io, NX_GPIO_PADFUNC_GPIO);
	}
#endif
	printk(KERN_INFO "pwm : %lu hz ~ %lu hz, pll:%lu, div:%lu\n",
		PWM_FREQUENCY_MIN, PWM_FREQUENCY_MAX, CFG_PWM_CLK_FREQ, PWM_CLOCK_DIVISOR);
}

static int set_pwm_freq(int ch, u_int freq, u_int duty)
{
	struct pwm_device * pwm = &gpwm[ch];
	u_int io = pwm->io;
	u_int fn = pwm->iofunc;
	int clkenb  = 0, i = 0;
	int r_scale = 0, r_period = 0, r_duty = 0;

	DBGOUT("%s (ch:%d, freq:%d, duty:%d)\n", __func__, ch, freq, duty);

	if (freq > PWM_FREQUENCY_MAX || PWM_FREQUENCY_MIN > freq) {
		printk("Invalid pwm[%d], freq:%d hz (%lu ~ %lu) \n",
			ch, freq, PWM_FREQUENCY_MIN, PWM_FREQUENCY_MAX);
		return (-1);
	}

	if (duty > 100 || 0 > duty) {
		printk("Invalid pwm[%d], duty:%d percent (0 ~ 100)\n", ch, duty);
		return (-1);
	}

	/* lock */
	PWM_LOCK(0);

	if (freq > PWM_FREQUENCY_MAX/PWM_PERIOD_MAX) {
		r_scale  = PWM_PRESCL_MIN;
		r_period = (PWM_FREQUENCY_MAX/r_scale)/freq;
	} else {
		r_period = PWM_PERIOD_MAX;
		r_scale  = (PWM_FREQUENCY_MAX/r_period)/freq;
	}
	r_duty = (r_period * duty) / 100;

	NX_PWM_SetPreScale	(ch, r_scale);
	NX_PWM_SetPeriod	(ch, r_period);
	NX_PWM_SetDutyCycle	(ch, r_duty);

	printk("pwm ch[%d]: freq:%lu hz, duty:%2d%% (reg: scale=%2d, period=%4d, duty=%3d) \n",
		ch, (PWM_FREQUENCY_MAX/r_period)/r_scale, duty, r_scale, r_period, r_duty);

	if (r_period >= r_duty && r_duty > 0)
		pwm->enable = 1;
	else
		pwm->enable = 0;

	/* check pwm status */
	for (i = 0; PWM_CHANNEL_NUM > i; i++) {
		DBGOUT("pwm ch[%d] - on:%d\n", i, gpwm[i].enable);
		if (gpwm[i].enable)
			clkenb = 1;
	}
	DBGOUT("clock %d, current clock:%d \n", clkenb, g_clk_enb);

	/* pwm clock */
	if (clkenb && ! g_clk_enb) {
		NX_PWM_SetClockSource	(0, PWM_CLOCK_SOURCE);
		NX_PWM_SetClockDivisor	(0, PWM_CLOCK_DIVISOR);
		NX_PWM_SetClockPClkMode	(NX_PCLKMODE_ALWAYS);
		NX_PWM_SetClockDivisorEnable(CTRUE);
	}

	if (!clkenb)	{
		DBGOUT("disable pwm clock ...\n");
		NX_PWM_SetClockPClkMode(NX_PCLKMODE_DYNAMIC);
		NX_PWM_SetClockDivisorEnable(CFALSE);
	}

	/* pwm pad */
	if (pwm->enable) {
		soc_gpio_set_io_func(io, fn);
	} else {
		soc_gpio_set_out_value(io, 0);
		soc_gpio_set_io_dir(io, 1);
		soc_gpio_set_io_pullup(io, 0);
		soc_gpio_set_io_func(io, NX_GPIO_PADFUNC_GPIO);
	}

	/* clock status */
	g_clk_enb = clkenb;

	/* unlock */
	PWM_UNLOCK(0);
	return 0;
}

/*------------------------------------------------------------------------------
 * 	Description	: set pwm frequncy, and save frequency info
 *	In[ch]		: pwm channel, 0 ~ 3
 *	In[freq]	: pwm frequency, unit (Hz), : 30Hz ~ 237KHz
 *	In[duty]	: pwm pulse width, unit (%), 0 % ~ 100 %
 *				: if 0, pulse is low out.
 *	Return 		: none.
 */
void soc_pwm_set_frequency(int ch, u_int freq, u_int duty)
{
	struct pwm_device * pwm = NULL;
	u_int  org_freq, org_duty;

	DBGOUT("%s\n", __func__);

	NX_ASSERT(PWM_CHANNEL_NUM > ch && ch >= 0);

	pwm = &gpwm[ch];

	org_freq  = pwm->freq;
	org_duty  = pwm->duty;
	pwm->freq = freq;
	pwm->duty = duty;

	if (0 > set_pwm_freq(ch, freq, duty)) {
		pwm->freq = org_freq;
		pwm->duty = org_duty;
		return;
	}
}
EXPORT_SYMBOL_GPL(soc_pwm_set_frequency);

/*------------------------------------------------------------------------------
 * 	Description	: get pwm frequncy info
 *	In[ch]		: pwm channel, 0 ~ 3
 *	out[freq]	: pwm frequency, unit (Hz), : 30Hz ~ 237KHz
 *	out[duty]	: pwm pulse width, unit (%), 0 % ~ 100 %
 *	Return 		: none.
 */
void soc_pwm_get_frequency(int ch, uint *freq, uint *duty)
{
	struct pwm_device * pwm = NULL;

	DBGOUT("%s\n", __func__);

	NX_ASSERT(PWM_CHANNEL_NUM > ch && ch >= 0);

	pwm   = &gpwm[ch];
	*freq = pwm->freq;
	*duty = pwm->duty;

	DBGOUT("%s (ch:%d, freq:%d, duty:%d)\n", __func__, ch, *freq, *duty);
}
EXPORT_SYMBOL_GPL(soc_pwm_get_frequency);

/*------------------------------------------------------------------------------
 * 	Description	: set suspend mode,
 *	Return 		: none.
 */
void soc_pwm_set_suspend(void)
{
	PM_DBGOUT("+%s\n", __func__);
	/* clock status */
	g_clk_enb = 0;

	PM_DBGOUT("-%s\n", __func__);
}
EXPORT_SYMBOL_GPL(soc_pwm_set_suspend);

/*------------------------------------------------------------------------------
 * 	Description	: resume mode, restore saved pwm frequency.
 *	In[ch]		: pwm channel, 0 ~ 3
 *	Return 		: none.
 */
void soc_pwm_set_resume(void)
{
	struct pwm_device * pwm = NULL;
	int ch = 0;

	PM_DBGOUT("+%s\n", __func__);

	for (; PWM_CHANNEL_NUM > ch; ch++) {
		pwm = &gpwm[ch];
		if (! pwm->suspend_ch)
			set_pwm_freq(ch, pwm->freq, pwm->duty);
	}
	PM_DBGOUT("-%s\n", __func__);
}
EXPORT_SYMBOL_GPL(soc_pwm_set_resume);

/*------------------------------------------------------------------------------
 * 	Description	: ch suspend mode, it's not save frequency info
 *	In[ch]		: pwm channel, 0 ~ 3
 *	In[freq]	: pwm frequency, unit (Hz), : 30Hz ~ 237KHz
 *	In[duty]	: pwm pulse width, unit (%), 0 % ~ 100 %
 *				: if 0, pulse is low out.
 *	Return 		: none.
 */
void soc_pwm_set_suspend_ch(int ch, u_int freq, u_int duty)
{
	struct pwm_device * pwm = NULL;

	PM_DBGOUT("+%s\n", __func__);

	NX_ASSERT(PWM_CHANNEL_NUM > ch && ch >= 0);

	pwm = &gpwm[ch];
	pwm->suspend_ch = 1;

	set_pwm_freq(ch, freq, duty);

	/* clock status */
	g_clk_enb = 0;

	PM_DBGOUT("-%s\n", __func__);
}
EXPORT_SYMBOL_GPL(soc_pwm_set_suspend_ch);

/*------------------------------------------------------------------------------
 * 	Description	: ch resume mode, restore saved pwm frequency.
 *	In[ch]		: pwm channel, 0 ~ 3
 *	Return 		: none.
 */
void soc_pwm_set_resume_ch(int ch)
{
	struct pwm_device * pwm = NULL;

	PM_DBGOUT("+%s\n", __func__);

	NX_ASSERT(PWM_CHANNEL_NUM > ch && ch >= 0);

	pwm = &gpwm[ch];
	pwm->suspend_ch = 0;

	set_pwm_freq(ch, pwm->freq, pwm->duty);

	PM_DBGOUT("-%s\n", __func__);
}
EXPORT_SYMBOL_GPL(soc_pwm_set_resume_ch);

