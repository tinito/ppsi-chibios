/*
 * Alessandro Rubini for CERN, 2013 -- LGPL 2.1 or later
 * (chibios code heavily based on bare code)
 */
#include <arch/jiffies.h>
#include <ppsi/ppsi.h>

#define HZ 1000

static unsigned long long jiffies_base;
static const int nsec_per_jiffy = 1000 * 1000 * 1000 / HZ;

volatile unsigned long jiffies;

static int chibios_time_get(struct pp_instance *ppi, TimeInternal *t)
{
	/* FIXME: horrible time-keeping based on jiffies */
	unsigned long long j = jiffies_base + jiffies;

	/* FIXME: Use our own do_div to avoid libgcc mess */
	t->seconds = j / HZ;
	t->nanoseconds = (j % HZ) * nsec_per_jiffy;
	if (!(pp_global_flags & PP_FLAG_NOTIMELOG))
		pp_diag(ppi, time, 2, "%s: %9li.%06li\n", __func__,
			(long)t->seconds, (long)t->nanoseconds);
	return 0;
}

static int chibios_time_set(struct pp_instance *ppi, TimeInternal *t)
{
	unsigned long long j = (long long)(t->seconds) * HZ
		+ (t->nanoseconds + nsec_per_jiffy / 2) / nsec_per_jiffy;

	jiffies_base = j - jiffies;
	pp_diag(ppi, time, 1, "%s: %9li.%06li\n", __func__,
		(long)t->seconds, (long)t->nanoseconds);
	return 0;
}

static int chibios_time_adjust(struct pp_instance *ppi, long offset_ns,
			    long freq_ppm)
{
	/* FIXME: no adj-time is present */
	pp_diag(ppi, time, 1, "%s: %li %li\n", __func__, offset_ns, freq_ppm);
	return 0;
}

int chibios_time_adjust_offset(struct pp_instance *ppi, long offset_ns)
{
	return chibios_time_adjust(ppi, offset_ns, 0);
}

int chibios_time_adjust_freq(struct pp_instance *ppi, long freq_ppm)
{
	return chibios_time_adjust(ppi, 0, freq_ppm);
}

/*
 * Unfortuanately (my own bug, it seems), the timeout is not expressed
 * in generic "jiffies", but rather in milliseconds. Currently the fix
 * is setting HZ to 1000 in chibios.
 */
#if HZ != 1000
#error "HZ must be 1000, until the code is fixed"
#endif

static unsigned long chibios_calc_timeout(struct pp_instance *ppi, int millisec)
{
	/* ms to jiffies... */
	unsigned long j = millisec * 1000 * 1000 / nsec_per_jiffy;
	return jiffies + j ?: 1 /* cannot return 0 */;
}


struct pp_time_operations chibios_time_ops = {
	.get = chibios_time_get,
	.set = chibios_time_set,
	.adjust = chibios_time_adjust,
	.adjust_offset = chibios_time_adjust_offset,
	.adjust_freq = chibios_time_adjust_freq,
	.calc_timeout = chibios_calc_timeout,
};
