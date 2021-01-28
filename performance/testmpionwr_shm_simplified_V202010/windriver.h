#ifndef __WINDRIVER_H__
#define __WINDRIVER_H__

#include <linux/types.h>
#ifdef __cplusplus
extern "C" {
#endif

struct ether_hw_addr {
	uint8_t addr_bytes[6]; /**< in transmission order */
} __attribute__((__packed__));

struct hw_ether_hdr {
	struct ether_hw_addr d_addr; /**< Destination address. */
	struct ether_hw_addr s_addr; /**< Source address. */
	uint16_t ether_type;      /**< Frame type. */
} __attribute__((__packed__));

#define pktmbuf_mtod(m, t) ((t)((m)->pkt.data))
#define pktmbuf_pkt_len(m) ((m)->pkt.pkt_len)
#define pktmbuf_data_len(m) ((m)->pkt.data_len)

int log_msg(const char *fmt, ...);
int log_out();
int log_init();
int log_exit();
int log_release();

unsigned long long get_nanosecond(void);

extern rt_init(int level);
extern void *rt_malloc(size_t size);
extern void rt_free(void* pv);
extern void *rt_calloc(size_t n, size_t size);
extern void *rt_realloc(void *mem_address, size_t size);
extern void initcmem(void);

#define TAILQ_ENTRY(type)                                               \
struct {                                                                \
        struct type *tqe_next;  /* next element */                      \
        struct type **tqe_prev; /* address of previous next element */  \
}

/*
 * Tail queue declarations.
 */
#define TAILQ_HEAD(name, type)                                          \
struct name {                                                           \
        struct type *tqh_first; /* first element */                     \
        struct type **tqh_last; /* addr of last next element */         \
}


#define MAX_PKT_BURST 128
#define FIBER_NET_CARD			1

unsigned int get_local_core_id();
unsigned long long get_rtcore_status(void);

#ifdef __cplusplus
}
#endif

#endif
