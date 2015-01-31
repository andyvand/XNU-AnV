#pragma once

#define __SSEPLUS_NATIVE_SSE2_H__ 1
#define __SSEPLUS_EMULATION_SSE2_H__ 1
#define __SSEPLUS_ARITHMETIC_SSE2_H__ 1
#define __SSEPLUS_MEMORY_REF_H__ 1

#include "opemu.h"
#include "libudis86/extern.h"

#include <SSEPlus/SSEPlus_Base.h>
#include <SSEPlus/SSEPlus_REF.h>

// log function debug
#define LF	D("%s\n", __PRETTY_FUNCTION__);
#define D	printf

/**
 * Print said register to screen. Useful for debugging
 * @param  uint128
 */
#define print128(x)	printf("0x%016llx%016llx", x.u64[1], x.u64[0]);

/**
 * sse3 object
 */
typedef struct sse3 {
	uint8_t	extended;	// bool type

    ssp_m128		dst;
    ssp_m128        src;
	ssp_m128		res;

	// operands
    const ud_operand_t	*udo_src;
    const ud_operand_t	*udo_dst;
    const ud_operand_t	*udo_imm;

	// objects
	const op_t		*op_obj;

	// legacy mmx flag
	uint8_t 		islegacy;
} sse3_t;

/**
 * Instruction emulation function type.
 */
typedef void (*sse3_func)(sse3_t*);

#define storedqu_template(n, where)					\
    asm __volatile__ ("movdqu %%xmm" #n ", %0" : "=m" (*(where)));

#define loaddqu_template(n, where)					\
    asm __volatile__ ("movdqu %0, %%xmm" #n :: "m" (*(where)));

#define storeq_template(n, where)					\
    asm __volatile__ ("movq %%mm" #n ", %0" : "=m" (*(where)));

#define loadq_template(n, where)					\
    asm __volatile__ ("movq %0, %%mm" #n :: "m" (*(where)));

/**
 * Store xmm register somewhere in memory
 */
inline void _store_xmm (const uint8_t n, __uint128_t *where)
{
	switch (n)
    {
case 0:  storedqu_template(0, where); break;
case 1:  storedqu_template(1, where); break;
case 2:  storedqu_template(2, where); break;
case 3:  storedqu_template(3, where); break;
case 4:  storedqu_template(4, where); break;
case 5:  storedqu_template(5, where); break;
case 6:  storedqu_template(6, where); break;
case 7:  storedqu_template(7, where); break;

#ifdef __x86_64__
case 8:  storedqu_template(8, where); break;
case 9:  storedqu_template(9, where); break;
case 10: storedqu_template(10, where); break;
case 11: storedqu_template(11, where); break;
case 12: storedqu_template(12, where); break;
case 13: storedqu_template(13, where); break;
case 14: storedqu_template(14, where); break;
case 15: storedqu_template(15, where); break;
#endif /* __x86_64__ */
    }
}

/**
 * Load xmm register from memory
 */
inline void _load_xmm (const uint8_t n, const __uint128_t *where)
{
	switch (n)
    {
case 0:  loaddqu_template(0, where); break;
case 1:  loaddqu_template(1, where); break;
case 2:  loaddqu_template(2, where); break;
case 3:  loaddqu_template(3, where); break;
case 4:  loaddqu_template(4, where); break;
case 5:  loaddqu_template(5, where); break;
case 6:  loaddqu_template(6, where); break;
case 7:  loaddqu_template(7, where); break;

#ifdef __x86_64__
case 8:  loaddqu_template(8, where); break;
case 9:  loaddqu_template(9, where); break;
case 10: loaddqu_template(10, where); break;
case 11: loaddqu_template(11, where); break;
case 12: loaddqu_template(12, where); break;
case 13: loaddqu_template(13, where); break;
case 14: loaddqu_template(14, where); break;
case 15: loaddqu_template(15, where); break;
#endif /* __x86_64__ */
    }
}

/**
 * Store mmx register somewhere in memory
 */
inline void _store_mmx (const uint8_t n, uint64_t *where)
{
	switch (n)
    {
case 0:  storeq_template(0, where); break;
case 1:  storeq_template(1, where); break;
case 2:  storeq_template(2, where); break;
case 3:  storeq_template(3, where); break;
case 4:  storeq_template(4, where); break;
case 5:  storeq_template(5, where); break;
case 6:  storeq_template(6, where); break;
case 7:  storeq_template(7, where); break;
    }
}

/**
 * Load mmx register from memory
 */
inline void _load_mmx (const uint8_t n, const uint64_t *where)
{
	switch (n)
    {
case 0:  loadq_template(0, where); break;
case 1:  loadq_template(1, where); break;
case 2:  loadq_template(2, where); break;
case 3:  loadq_template(3, where); break;
case 4:  loadq_template(4, where); break;
case 5:  loadq_template(5, where); break;
case 6:  loadq_template(6, where); break;
case 7:  loadq_template(7, where); break;
    }
}

inline int sse3_grab_operands(sse3_t*);
inline int sse3_commit_results(const sse3_t*);
inline int op_sse3_run(const op_t*);

/** AnV - SSE3 instructions **/
inline void addsubpd   (sse3_t*);
inline void addsubps   (sse3_t*);
inline void haddpd     (sse3_t*);
inline void haddps     (sse3_t*);
inline void hsubpd     (sse3_t*);
inline void hsubps     (sse3_t*);
inline void lddqu      (sse3_t*);
inline void movddup    (sse3_t*);
inline void movshdup   (sse3_t*);
inline void movsldup   (sse3_t*);
inline void fisttp     (sse3_t*);
inline void fisttps    (float *res);
inline void fisttpl    (double *res);
inline void fisttpq    (long double *res);
