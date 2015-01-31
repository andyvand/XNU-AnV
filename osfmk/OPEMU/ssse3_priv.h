#pragma once

#define __SSEPLUS_NATIVE_SSE2_H__ 1
#define __SSEPLUS_EMULATION_SSE2_H__ 1
#define __SSEPLUS_ARITHMETIC_SSE2_H__ 1
#define __SSEPLUS_MEMORY_REF_H__ 1

#include "opemu.h"
#include "libudis86/extern.h"

// log function debug
#define LF	D("%s\n", __PRETTY_FUNCTION__);
#define D	printf

#include <SSEPlus/SSEPlus_Base.h>
#include <SSEPlus/SSEPlus_REF.h>

/**
 * Print said register to screen. Useful for debugging
 * @param  uint128
 */
#define print128(x)	printf("0x%016llx%016llx", x.u64[1], x.u64[0]);

/**
 * ssse3 object
 */
typedef struct ssse3 {
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
} ssse3_t;

/**
 * Instruction emulation function type.
 */
typedef void (*ssse3_func)(ssse3_t*);

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
	switch (n) {
case 0:  storeq_template(0, where); break;
case 1:  storeq_template(1, where); break;
case 2:  storeq_template(2, where); break;
case 3:  storeq_template(3, where); break;
case 4:  storeq_template(4, where); break;
case 5:  storeq_template(5, where); break;
case 6:  storeq_template(6, where); break;
case 7:  storeq_template(7, where); break;
}}

/**
 * Load mmx register from memory
 */
inline void _load_mmx (const uint8_t n, const uint64_t *where)
{
	switch (n) {
case 0:  loadq_template(0, where); break;
case 1:  loadq_template(1, where); break;
case 2:  loadq_template(2, where); break;
case 3:  loadq_template(3, where); break;
case 4:  loadq_template(4, where); break;
case 5:  loadq_template(5, where); break;
case 6:  loadq_template(6, where); break;
case 7:  loadq_template(7, where); break;
}}

inline int ssse3_grab_operands(ssse3_t*);
inline int ssse3_commit_results(const ssse3_t*);
inline int op_sse3x_run(const op_t*);

void psignb	(ssse3_t*);
void psignw	(ssse3_t*);
void psignd	(ssse3_t*);
void pabsb	(ssse3_t*);
void pabsw	(ssse3_t*);
void pabsd	(ssse3_t*);
void palignr	(ssse3_t*);
void pshufb	(ssse3_t*);
void pmulhrsw	(ssse3_t*);
void pmaddubsw	(ssse3_t*);
void phsubw	(ssse3_t*);
void phsubd	(ssse3_t*);
void phsubsw	(ssse3_t*);
void phaddw	(ssse3_t*);
void phaddd	(ssse3_t*);
void phaddsw	(ssse3_t*);

/*** SSE4.2 ***/
void pcmpistri	(ssse3_t*);
void pcmpestri	(ssse3_t*);
void pcmpestrm	(ssse3_t*);
void pcmpistrm	(ssse3_t*);
void pcmpgtq     (ssse3_t*);
