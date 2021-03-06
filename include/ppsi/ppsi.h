/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

#ifndef __PPSI_PPSI_H__
#define __PPSI_PPSI_H__

#include <stdint.h>
#include <stdarg.h>
#include <arch/arch.h> /* ntohs and so on */
#include <ppsi/lib.h>
#include <ppsi/ieee1588_types.h>
#include <ppsi/constants.h>
#include <ppsi/jiffies.h>

#include <ppsi/pp-instance.h>
#include <ppsi/diag-macros.h>

/* We can't include pp-printf.h when building freestading, so have it here */
extern int pp_printf(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));
extern int pp_vprintf(const char *fmt, va_list args)
	__attribute__((format(printf, 1, 0)));
extern int pp_sprintf(char *s, const char *fmt, ...)
	__attribute__((format(printf,2,3)));
extern int pp_vsprintf(char *buf, const char *, va_list)
	__attribute__ ((format (printf, 2, 0)));


/* We use data sets a lot, so have these helpers */
static inline struct pp_globals *GLBS(struct pp_instance *ppi)
{
	return ppi->glbs;
}

static inline struct pp_runtime_opts *GOPTS(struct pp_globals *ppg)
{
	return ppg->rt_opts;
}

static inline struct pp_runtime_opts *OPTS(struct pp_instance *ppi)
{
	return GOPTS(GLBS(ppi));
}

static inline struct DSDefault *GDSDEF(struct pp_globals *ppg)
{
	return ppg->defaultDS;
}

static inline struct DSDefault *DSDEF(struct pp_instance *ppi)
{
	return GDSDEF(GLBS(ppi));
}

static inline struct DSCurrent *DSCUR(struct pp_instance *ppi)
{
	return GLBS(ppi)->currentDS;
}

static inline struct DSParent *DSPAR(struct pp_instance *ppi)
{
	return GLBS(ppi)->parentDS;
}

static inline struct DSPort *DSPOR(struct pp_instance *ppi)
{
	return ppi->portDS;
}

static inline struct DSTimeProperties *DSPRO(struct pp_instance *ppi)
{
	return GLBS(ppi)->timePropertiesDS;
}

static inline struct pp_net_path *NP(struct pp_instance *ppi)
{
	return &ppi->np;
}

static inline struct pp_servo *SRV(struct pp_instance *ppi)
{
	return GLBS(ppi)->servo;
}


/* Sometimes (e.g., raw ethernet frames), we need to consider an offset */
static inline void *pp_get_header(struct pp_instance *ppi, void *ptp_payload)
{
	return ptp_payload - NP(ppi)->ptp_offset;
}

static inline void *pp_get_payload(struct pp_instance *ppi, void *frame_ptr)
{
	return frame_ptr + NP(ppi)->ptp_offset;
}

extern void pp_prepare_pointers(struct pp_instance *ppi);


/*
 * Each extension should fill this structure that is used to augment
 * the standard states and avoid code duplications. Please remember
 * that proto-standard functions are picked as a fall-back when non
 * extension-specific code is provided. The set of hooks here is designed
 * based on what White Rabbit does. If you add more please remember to
 * allow NULL pointers.
 */
struct pp_ext_hooks {
	int (*init)(struct pp_instance *ppg, unsigned char *pkt, int plen);
	int (*open)(struct pp_globals *ppi, struct pp_runtime_opts *rt_opts);
	int (*close)(struct pp_globals *ppg);
	int (*listening)(struct pp_instance *ppi, unsigned char *pkt, int plen);
	int (*master_msg)(struct pp_instance *ppi, unsigned char *pkt,
			  int plen, int msgtype);
	int (*new_slave)(struct pp_instance *ppi, unsigned char *pkt, int plen);
	int (*update_delay)(struct pp_instance *ppi);
	void (*s1)(struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann);
	int (*execute_slave)(struct pp_instance *ppi);
	void (*handle_announce)(struct pp_instance *ppi);
	int (*handle_followup)(struct pp_instance *ppi, TimeInternal *orig,
			       TimeInternal *correction_field);
	int (*pack_announce)(struct pp_instance *ppi);
	void (*unpack_announce)(void *buf, MsgAnnounce *ann);
};

extern struct pp_ext_hooks pp_hooks; /* The one for the extension we build */


/*
 * Network methods are encapsulated in a structure, so each arch only needs
 * to provide that structure. This should simplify management overall.
 */
struct pp_network_operations {
	int (*init)(struct pp_instance *ppi);
	int (*exit)(struct pp_instance *ppi);
	int (*recv)(struct pp_instance *ppi, void *pkt, int len,
		    TimeInternal *t);
	/* chtype here is PP_NP_GEN or PP_NP_EVT -- use_pdelay must be 0 */
	int (*send)(struct pp_instance *ppi, void *pkt, int len,
		    TimeInternal *t, int chtype, int use_pdelay_addr);
};

/* This is the struct pp_network_operations to be provided by time- dir */
extern struct pp_network_operations DEFAULT_NET_OPS;

/* These can be liked and used as fallback by a different timing engine */
extern struct pp_network_operations unix_net_ops;


/*
 * Time operations, like network operations above, are encapsulated.
 * They may live in their own time-<name> subdirectory.
 *
 * Maybe this structure will need updating, to pass ppi as well
 */
struct pp_time_operations {
	int (*get)(struct pp_instance *ppi, TimeInternal *t);
	int (*set)(struct pp_instance *ppi, TimeInternal *t);
	/* freq_ppm is "scaled-ppm" like the argument of adjtimex(2) */
	int (*adjust)(struct pp_instance *ppi, long offset_ns, long freq_ppm);
	int (*adjust_offset)(struct pp_instance *ppi, long offset_ns);
	int (*adjust_freq)(struct pp_instance *ppi, long freq_ppm);
	/* calc_timeout cannot return zero */
	unsigned long (*calc_timeout)(struct pp_instance *ppi, int millisec);
};

/* This is the struct pp_time_operations to be provided by time- dir */
extern struct pp_time_operations DEFAULT_TIME_OPS;

/* These can be liked and used as fallback by a different timing engine */
extern struct pp_time_operations unix_time_ops;


/* FIXME this define is no more used; check whether it should be
 * introduced again */
#define  PP_ADJ_NS_MAX		(500*1000)

/* FIXME Restored to value of ptpd. What does this stand for, exactly? */
#define  PP_ADJ_FREQ_MAX	512000

/*
 * Timeouts. I renamed from "timer" to "timeout" to avoid
 * misread/miswrite with the time operations above. A timeout, actually,
 * is just a number that must be compared with the current counter.
 * So we don't need struct operations, as it is one function only,
 * which is folded into the "pp_time_operations" above.
 */

static inline void pp_timeout_set(struct pp_instance *ppi, int index,
				  int millisec)
{
	ppi->timeouts[index] = ppi->t_ops->calc_timeout(ppi, millisec);
}

extern void pp_timeout_rand(struct pp_instance *ppi, int index, int logval);

static inline void pp_timeout_clr(struct pp_instance *ppi, int index)
{
	ppi->timeouts[index] = 0;
}

extern void pp_timeout_log(struct pp_instance *ppi, int index);

static inline int pp_timeout(struct pp_instance *ppi, int index)
{
	int ret = ppi->timeouts[index] &&
		time_after_eq(ppi->t_ops->calc_timeout(ppi, 0),
			      ppi->timeouts[index]);

	if (ret)
		pp_timeout_log(ppi, index);
	return ret;
}

static inline int pp_timeout_z(struct pp_instance *ppi, int index)
{
	int ret = pp_timeout(ppi, index);

	if (ret)
		pp_timeout_clr(ppi, index);
	return ret;
}

/* how many ms to wait for the timeout to happen, for ppi->next_delay */
static inline int pp_ms_to_timeout(struct pp_instance *ppi, int index)
{
	signed long ret;

	if (!ppi->timeouts[index]) /* not pending, nothing to wait for */
		return 0;

	ret = ppi->timeouts[index] - ppi->t_ops->calc_timeout(ppi, 0);
	return ret <= 0 ? 0 : ret;
}

/* called several times, only sets a timeout, so inline it here */
static inline void pp_timeout_restart_annrec(struct pp_instance *ppi)
{
	/* This timeout is a number of the announce interval lapses */
	pp_timeout_set(ppi, PP_TO_ANN_RECEIPT,
		       ((DSPOR(ppi)->announceReceiptTimeout) <<
			DSPOR(ppi)->logAnnounceInterval) * 1000);
}


/* The channel for an instance must be created and possibly destroyed. */
extern int pp_open_globals(struct pp_globals *ppg);

extern int pp_close_globals(struct pp_globals *ppg);

extern int pp_parse_cmdline(struct pp_globals *ppg, int argc, char **argv);

extern int pp_parse_conf(struct pp_globals *ppg, char *conf, int len);

/* Servo */
extern void pp_init_clock(struct pp_instance *ppi);
extern void pp_update_delay(struct pp_instance *ppi,
			    TimeInternal *correction_field);
extern void pp_update_offset(struct pp_instance *ppi,
			     TimeInternal *correctionField);
extern void pp_update_clock(struct pp_instance *ppi);


/* bmc.c */
extern void m1(struct pp_instance *ppi);
extern void s1(struct pp_instance *ppi, MsgHeader *header, MsgAnnounce *ann);
extern int bmc(struct pp_instance *ppi);

/* msg.c */
extern void msg_pack_header(struct pp_instance *ppi, void *buf);
extern int __attribute__((warn_unused_result))
	msg_unpack_header(struct pp_instance *ppi, void *buf, int plen);
void *msg_copy_header(MsgHeader *dest, MsgHeader *src);
extern void msg_pack_sync(struct pp_instance *ppi, Timestamp *orig_tstamp);
extern void msg_unpack_sync(void *buf, MsgSync *sync);
extern int msg_pack_announce(struct pp_instance *ppi);
extern void msg_unpack_announce(void *buf, MsgAnnounce *ann);
extern void msg_pack_follow_up(struct pp_instance *ppi,
			       Timestamp *prec_orig_tstamp);
extern void msg_unpack_follow_up(void *buf, MsgFollowUp *flwup);
extern void msg_pack_delay_req(struct pp_instance *ppi,
			       Timestamp *orig_tstamp);
extern void msg_unpack_delay_req(void *buf, MsgDelayReq *delay_req);
extern void msg_pack_delay_resp(struct pp_instance *ppi,
				MsgHeader *hdr, Timestamp *rcv_tstamp);
extern void msg_unpack_delay_resp(void *buf, MsgDelayResp *resp);

/* each of them returns 0 if no error and -1 in case of error in send */
extern int msg_issue_announce(struct pp_instance *ppi);
extern int msg_issue_sync(struct pp_instance *ppi);
extern int msg_issue_followup(struct pp_instance *ppi, TimeInternal *time);
extern int msg_issue_delay_req(struct pp_instance *ppi);
extern int msg_issue_delay_resp(struct pp_instance *ppi, TimeInternal *time);


/* Functions for timestamp handling (internal to protocol format conversion*/
/* FIXME: add prefix in function name? */
extern void cField_to_TimeInternal(TimeInternal *internal, Integer64 bigint);
extern int from_TimeInternal(TimeInternal *internal, Timestamp *external);
extern int to_TimeInternal(TimeInternal *internal, Timestamp *external);
extern void add_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y);
extern void sub_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y);
extern void display_TimeInternal(const char *label, TimeInternal *t);
extern void div2_TimeInternal(TimeInternal *r);

/*
 * The state machine itself is an array of these structures.
 */

/* Use a typedef, to avoid long prototypes */
typedef int pp_action(struct pp_instance *ppi, uint8_t *packet, int plen);

struct pp_state_table_item {
	int state;
	char *name;
	pp_action *f1;
};

extern struct pp_state_table_item pp_state_table[]; /* 0-terminated */

/* Standard state-machine functions */
extern pp_action pp_initializing, pp_faulty, pp_disabled, pp_listening,
		 pp_pre_master, pp_master, pp_passive, pp_uncalibrated,
		 pp_slave;

/* The engine */
extern int pp_state_machine(struct pp_instance *ppi, uint8_t *packet, int plen);

#endif /* __PPSI_PPSI_H__ */
