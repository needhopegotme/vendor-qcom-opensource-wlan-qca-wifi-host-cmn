/*
 * Copyright (c) 2015-2017 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

#ifndef __HIF_NAPI_H__
#define __HIF_NAPI_H__

/**
 * DOC: hif_napi.h
 *
 * Interface to HIF implemented functions of NAPI.
 * These are used by hdd_napi.
 */


/* CLD headers */
#include <hif.h> /* struct hif_opaque_softc; */

/**
 * common stuff
 * The declarations until #ifdef FEATURE_NAPI below
 * are valid whether or not FEATURE_NAPI has been
 * defined.
 */

/**
 * NAPI manages the following states:
 * NAPI state: per NAPI instance, ENABLED/DISABLED
 * CPU  state: per CPU,           DOWN/UP
 * TPUT state: global,            LOW/HI
 *
 * "Dynamic" changes to state of various NAPI structures are
 * managed by NAPI events. The events may be produced by
 * various detection points. With each event, some data is
 * sent. The main event handler in hif_napi handles and makes
 * the state changes.
 *
 * event          : data             : generated
 * ---------------:------------------:------------------
 * EVT_INI_FILE   : cfg->napi_enable : after ini file processed
 * EVT_CMD_STATE  : cmd arg          : by the vendor cmd
 * EVT_INT_STATE  : 0                : internal - shut off/disable
 * EVT_CPU_STATE  : (cpu << 16)|state: CPU hotplug events
 * EVT_TPUT_STATE : (high/low)       : tput trigger
 * EVT_USR_SERIAL : num-serial_calls : WMA/ROAMING-START/IND
 * EVT_USR_NORMAL : N/A              : WMA/ROAMING-END
 */
enum qca_napi_event {
	NAPI_EVT_INVALID,
	NAPI_EVT_INI_FILE,
	NAPI_EVT_CMD_STATE,
	NAPI_EVT_INT_STATE,
	NAPI_EVT_CPU_STATE,
	NAPI_EVT_TPUT_STATE,
	NAPI_EVT_USR_SERIAL,
	NAPI_EVT_USR_NORMAL
};
/**
 * Following are some of NAPI related features controlled using feature flag
 * These flags need to be enabled in the qca_napi_data->flags variable for the
 * feature to kick in.
.* QCA_NAPI_FEATURE_CPU_CORRECTION   - controls CPU correction logic
.* QCA_NAPI_FEATURE_IRQ_BLACKLISTING - controls call to  irq_blacklist_on API
.* QCA_NAPI_FEATURE_CORE_CTL_BOOST   - controls call to core_ctl_set_boost API
 */
#define QCA_NAPI_FEATURE_CPU_CORRECTION            BIT(1)
#define QCA_NAPI_FEATURE_IRQ_BLACKLISTING          BIT(2)
#define QCA_NAPI_FEATURE_CORE_CTL_BOOST            BIT(3)

/**
 * Macros to map ids -returned by ...create()- to pipes and vice versa
 */
#define NAPI_ID2PIPE(i) ((i)-1)
#define NAPI_PIPE2ID(p) ((p)+1)

int hif_napi_lro_flush_cb_register(struct hif_opaque_softc *hif_hdl,
				   void (lro_flush_handler)(void *),
				   void *(lro_init_handler)(void));

void hif_napi_lro_flush_cb_deregister(struct hif_opaque_softc *hif_hdl,
				      void (lro_deinit_cb)(void *));

void *hif_napi_get_lro_info(struct hif_opaque_softc *hif_hdl, int napi_id);
#ifdef FEATURE_NAPI

/**
 * NAPI HIF API
 *
 * the declarations below only apply to the case
 * where FEATURE_NAPI is defined
 */

int hif_napi_create(struct hif_opaque_softc   *hif,
		    int (*poll)(struct napi_struct *, int),
		    int                budget,
		    int                scale,
		    uint8_t            flags);
int hif_napi_destroy(struct hif_opaque_softc  *hif,
		     uint8_t           id,
		     int               force);

struct qca_napi_data *hif_napi_get_all(struct hif_opaque_softc   *hif);

int hif_napi_event(struct hif_opaque_softc     *hif,
		   enum  qca_napi_event event,
		   void                *data);

/* called from the ISR within hif, so, ce is known */
int hif_napi_enabled(struct hif_opaque_softc *hif, int ce);

/* called from hdd (napi_poll), using napi id as a selector */
void hif_napi_enable_irq(struct hif_opaque_softc *hif, int id);

/* called by ce_tasklet.c::ce_dispatch_interrupt*/
int hif_napi_schedule(struct hif_opaque_softc *scn, int ce_id);

/* called by hdd_napi, which is called by kernel */
int hif_napi_poll(struct hif_opaque_softc *hif_ctx,
			struct napi_struct *napi, int budget);
#ifdef HELIUMPLUS
/* called to retrieve NAPI CPU statistics */
void hif_napi_stats(struct qca_napi_data *napid);
void hif_napi_update_yield_stats(struct CE_state *ce_state,
				 bool time_limit_reached,
				 bool rxpkt_thresh_reached);
#else
static inline void hif_napi_stats(struct qca_napi_data *napid) { }

static inline void hif_napi_update_yield_stats(struct CE_state *ce_state,
				 bool time_limit_reached,
				 bool rxpkt_thresh_reached) { }

#endif

#ifdef FEATURE_NAPI_DEBUG
#define NAPI_DEBUG(fmt, ...)			\
	qdf_print("wlan: NAPI: %s:%d "fmt, __func__, __LINE__, ##__VA_ARGS__);
#else
#define NAPI_DEBUG(fmt, ...) /* NO-OP */
#endif /* FEATURE NAPI_DEBUG */

#define HNC_ANY_CPU (-1)
#define HNC_ACT_RELOCATE (0)
#define HNC_ACT_COLLAPSE (1)
#define HNC_ACT_DISPERSE (-1)

enum qca_blacklist_op {
	BLACKLIST_QUERY,
	BLACKLIST_OFF,
	BLACKLIST_ON
};

int hif_napi_cpu_blacklist(struct qca_napi_data *napid, enum qca_blacklist_op op);
/**
 * Local interface to HIF implemented functions of NAPI CPU affinity management.
 * Note:
 * 1- The symbols in this file are NOT supposed to be used by any
 *    entity other than hif_napi.c
 * 2- The symbols are valid only if HELIUMPLUS is defined. They are otherwise
 *    mere wrappers.
 *
 */
#ifndef HELIUMPLUS
/**
 * stub functions
 */
/* fw-declare to make compiler happy */
struct qca_napi_data;
static inline int hif_napi_cpu_init(struct hif_opaque_softc *hif)
{ return 0; }

static inline int hif_napi_cpu_deinit(struct hif_opaque_softc *hif)
{ return 0; }

static inline int hif_napi_serialize(struct hif_opaque_softc *hif, int is_on)
{ return -EPERM; }
#else /* HELIUMPLUS - NAPI CPU symbols are valid */

/*
 * prototype signatures
 */
int hif_napi_cpu_init(struct hif_opaque_softc *hif);
int hif_napi_cpu_deinit(struct hif_opaque_softc *hif);

int hif_napi_cpu_migrate(struct qca_napi_data *napid, int cpu, int action);
int hif_napi_serialize(struct hif_opaque_softc *hif, int is_on);

#endif /* HELIUMPLUS */

#else /* ! defined(FEATURE_NAPI) */

/**
 * Stub API
 *
 * The declarations in this section are valid only
 * when FEATURE_NAPI has *not* been defined.
 */

#define NAPI_DEBUG(fmt, ...) /* NO-OP */

static inline int hif_napi_create(struct hif_opaque_softc   *hif,
				  uint8_t            pipe_id,
				  int (*poll)(struct napi_struct *, int),
				  int                budget,
				  int                scale,
				  uint8_t            flags)
{ return -EPERM; }

static inline int hif_napi_destroy(struct hif_opaque_softc  *hif,
				   uint8_t           id,
				   int               force)
{ return -EPERM; }

static inline struct qca_napi_data *hif_napi_get_all(
				struct hif_opaque_softc *hif)
{ return NULL; }

static inline int hif_napi_event(struct hif_opaque_softc     *hif,
				 enum  qca_napi_event event,
				 void                *data)
{ return -EPERM; }

/* called from the ISR within hif, so, ce is known */
static inline int hif_napi_enabled(struct hif_opaque_softc *hif, int ce)
{ return 0; }

/* called from hdd (napi_poll), using napi id as a selector */
static inline void hif_napi_enable_irq(struct hif_opaque_softc *hif, int id)
{ return; }

static inline int hif_napi_schedule(struct hif_opaque_softc *hif, int ce_id)
{ return 0; }

static inline int hif_napi_poll(struct napi_struct *napi, int budget)
{ return -EPERM; }

static inline void hif_napi_stats(struct qca_napi_data *napid) { }

#endif /* FEATURE_NAPI */

#endif /* __HIF_NAPI_H__ */
