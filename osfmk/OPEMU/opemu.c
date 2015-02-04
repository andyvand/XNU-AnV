/*   ** SINETEK **
 * This is called the Opcode Emulator: it traps invalid opcode exceptions
 *   and modifies the state of the running thread accordingly.
 * There are two entry points, one for user space exceptions, and another for
 *   exceptions coming from kernel space.
 *
 * STATUS
 *  . SSE3 is implemented.
 *  . SSSE3 is implemented.
 *  . SYSENTER is implemented.
 *  . SYSEXIT is implemented.
 *  . RDMSR is implemented.
 *  . Vector register save and restore is implemented.
 *
 * This is a new version of AnV Software based on the AMD SSEPlus project
 * It runs much more reliable and much faster
 */

#define __SSEPLUS_NATIVE_SSE2_H__ 1
#define __SSEPLUS_EMULATION_SSE2_H__ 1
#define __SSEPLUS_ARITHMETIC_SSE2_H__ 1

#include <stdint.h>
#include "opemu.h"
#include "opemu_math.h"

#include <SSEPlus/SSEPlus_Base.h>
#include <SSEPlus/SSEPlus_REF.h>
#include <SSEPlus/SSEPlus_SSSE3.h>

#ifndef TESTCASE
#include <kern/sched_prim.h>

//#define EMULATION_FAILED -1

// forward declaration for syscall handlers of mach/bsd (32+64 bit);
extern void mach_call_munger(x86_saved_state_t *state);
extern void unix_syscall(x86_saved_state_t *);
extern void mach_call_munger64(x86_saved_state_t *state);
extern void unix_syscall64(x86_saved_state_t *);
// forward declaration of panic handler for kernel traps;
extern void panic_trap(x86_saved_state64_t *regs);

// AnV - Implemented i386 version
#ifdef __x86_64__
unsigned char opemu_ktrap(x86_saved_state_t *state)
{
    x86_saved_state64_t *saved_state = saved_state64(state);
    uint8_t *code_buffer = (uint8_t *)saved_state->isf.rip;
    unsigned int bytes_skip = 0;
    int longmode;
    longmode = is_saved_state64(state);

    bytes_skip = ssse3_run(code_buffer, state, longmode, 1);
    
    if (!bytes_skip)
    {
        bytes_skip = sse3_run_a(code_buffer, state, longmode, 1);
    }
    
    if (!bytes_skip)
    {
        bytes_skip = sse3_run_b(code_buffer, state, longmode, 1);
    }
    
    if (!bytes_skip)
    {
        bytes_skip = sse3_run_c(code_buffer, state, longmode, 1);
    }
    
    if (!bytes_skip)
    {
        bytes_skip = fisttp_run(code_buffer, state, longmode, 1);
    }
    
    if (!bytes_skip)
    {
        bytes_skip = monitor_mwait_run(code_buffer, state, longmode, 1);
    }
    
    if(!bytes_skip)
    {
        /* since this is ring0, it could be an invalid MSR read.
         * Instead of crashing the whole machine, report on it and keep running. */
        if((code_buffer[0]==0x0f) && (code_buffer[1]==0x32))
        {
            printf("[MSR] unknown location 0x%016llx\r\n", saved_state->rcx);
            // best we can do is return 0;
            saved_state->rdx = saved_state->rax = 0;
            bytes_skip = 2;
        }
    }
    
    saved_state->isf.rip += bytes_skip;
    
    if(!bytes_skip)
    {
        uint8_t *ripptr = (uint8_t *)&(saved_state->isf.rip);
        printf("invalid kernel opcode (64-bit): ");
        print_bytes(ripptr, 16);
        /* Fall through to trap */
        return 0;
    }

    return 1;
}
#else
unsigned char opemu_ktrap(x86_saved_state_t *state)
{
    x86_saved_state32_t *saved_state = saved_state32(state);
    uint64_t op = saved_state->eip;
    uint8_t *code_buffer = (uint8_t*)op ;
    unsigned int bytes_skip = 0;
    int longmode;
    longmode = is_saved_state32(state);

    bytes_skip = ssse3_run(code_buffer, state, longmode, 1);
    
    if (!bytes_skip)
    {
        bytes_skip = sse3_run_a(code_buffer, state, longmode, 1);
    }
    
    if (!bytes_skip)
    {
        bytes_skip = sse3_run_b(code_buffer, state, longmode, 1);
    }
    
    if (!bytes_skip)
    {
        bytes_skip = sse3_run_c(code_buffer, state, longmode, 1);
    }
    
    if (!bytes_skip)
    {
        bytes_skip = fisttp_run(code_buffer, state, longmode, 1);
    }
    
    if (!bytes_skip)
    {
        bytes_skip = monitor_mwait_run(code_buffer, state, longmode, 1);
    }
    
    if(!bytes_skip)
    {
        /* since this is ring0, it could be an invalid MSR read.
         * Instead of crashing the whole machine, report on it and keep running. */
        if(code_buffer[0]==0x0f && code_buffer[1]==0x32)
        {
            printf("[MSR] unknown location 0x%016llx\r\n", saved_state->ecx);
            // best we can do is return 0;
            saved_state->edx = saved_state->eax = 0;
            bytes_skip = 2;
        }
    }

    saved_state->eip += bytes_skip;
    
    if(!bytes_skip)
    {
        uint8_t *eipptr = (uint8_t *)&(saved_state->eip);
        printf("invalid kernel opcode (32-bit): ");
        print_bytes(eipptr, 16);
        /* Fall through to trap */
        return 0;
    }
    return 1;
}
#endif

void opemu_utrap(x86_saved_state_t *state)
{
    
    int longmode;
    unsigned int bytes_skip = 0;
    vm_offset_t addr;

    if ((longmode = is_saved_state64(state)))
    {
        x86_saved_state64_t *saved_state = saved_state64(state);
        uint8_t *code_buffer = (uint8_t*)saved_state->isf.rip;
        addr = saved_state->isf.rip;
        uint16_t opcode;
        opcode = *(uint16_t *) addr;
        x86_saved_state64_t *regs;
        regs = saved_state64(state);

		//sysenter
        if (opcode == 0x340f)
        {
            regs->isf.rip = regs->rdx;
            regs->isf.rsp = regs->rcx;

            if((signed int)regs->rax < 0) {
                //printf("mach call 64\n");
                mach_call_munger64(state);
            } else {
                //printf("unix call 64\n");
                unix_syscall64(state);
            }
            return;
        }

		//sysexit
        if (opcode == 0x350f)
        {
            regs->isf.rip = regs->rdx;
            regs->isf.rsp = regs->rcx;
            thread_exception_return();
            /*if (kernel_trap)
            {
                addr = regs->rcx;
                return 0x7FFF;
            }
            else
            {
                thread_exception_return();
            }*/
            return;
        }

        bytes_skip = ssse3_run(code_buffer, state, longmode, 0);
        
        if (!bytes_skip)
        {
            bytes_skip = sse3_run_a(code_buffer, state, longmode, 0);
        }
        
        if (!bytes_skip)
        {
            bytes_skip = sse3_run_b(code_buffer, state, longmode, 0);
        }
        
        if (!bytes_skip)
        {
            bytes_skip = sse3_run_c(code_buffer, state, longmode, 0);
        }
        
        if (!bytes_skip)
        {
            bytes_skip = fisttp_run(code_buffer, state, longmode, 0);
        }
        
        if (!bytes_skip)
        {
            bytes_skip = monitor_mwait_run(code_buffer, state, longmode, 0);
        }
        
        regs->isf.rip += bytes_skip;
        
        if(!bytes_skip)
        {
            uint8_t *ripptr = (uint8_t *)&(regs->isf.rip);
            printf("invalid user opcode 64: ");
            print_bytes(ripptr, 16);
            /* Fall through to trap */
            return;
        }
    }
    else
    {
        x86_saved_state32_t *saved_state = saved_state32(state);
        uint64_t op = saved_state->eip;
        uint8_t *code_buffer = (uint8_t*)op;
        addr = saved_state->eip;
        uint16_t opcode;
        opcode = *(uint16_t *) addr;
        x86_saved_state32_t *regs;
        regs = saved_state32(state);

        //sysenter
        if (opcode == 0x340f)
        {
            regs->eip = regs->edx;
            regs->uesp = regs->ecx;

            if((signed int)regs->eax < 0) {
                mach_call_munger(state);
            } else {
                unix_syscall(state);
            }
            return;
        }

        //sysexit
        if (opcode == 0x350f)
        {
            regs->eip = regs->edx;
            regs->uesp = regs->ecx;
            thread_exception_return();
            return;
        }

        bytes_skip = ssse3_run(code_buffer, state, longmode, 0);
        
        if (!bytes_skip)
        {
            bytes_skip = sse3_run_a(code_buffer, state, longmode, 0);
        }
        
        if (!bytes_skip)
        {
            bytes_skip = sse3_run_b(code_buffer, state, longmode, 0);
        }
        
        if (!bytes_skip)
        {
            bytes_skip = sse3_run_c(code_buffer, state, longmode, 0);
        }
        
        if (!bytes_skip)
        {
            bytes_skip = fisttp_run(code_buffer, state, longmode, 0);
        }
        
        if (!bytes_skip)
        {
            bytes_skip = monitor_mwait_run(code_buffer, state, longmode, 0);
        }
        
        regs->eip += bytes_skip;
        
        if(!bytes_skip)
        {
            uint8_t *eipptr = (uint8_t *)&(regs->eip);
            printf("invalid user opcode 32: ");
            print_bytes(eipptr, 16);
            /* Fall through to trap */
            return;
        }
    }
    thread_exception_return();
    /*** NOTREACHED ***/
    //EMULATION_FAILED;
}

/** Runs the sse3 emulator. returns the number of bytes consumed.
 **/
int sse3_run_a(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap)
{
    uint8_t *bytep = instruction;
    int ins_size = 0;
    ssp_m128 xmmsrc, xmmdst, xmmres;
    int src_higher = 0, dst_higher = 0;
    
    if(*bytep != 0xF2) return 0;
    
    bytep++;
    ins_size++;
    
    if(*bytep != 0x0f) return 0;
    bytep++;
    ins_size++;
    
    uint8_t *modrm = &bytep[1];
    ins_size += 1;
    int consumed = fetchoperands(modrm, src_higher, dst_higher, &xmmsrc, &xmmdst, longmode, state, kernel_trap, 1);
    ins_size += consumed;

    switch (*bytep)
    {
        case 0x12:
            //movddup(&xmmsrc,&xmmres);
            xmmres.d = ssp_movedup_pd_REF(xmmsrc.d);
            break;

        case 0x7C:
            //haddps(&xmmsrc,&xmmdst,&xmmres);
            xmmres.f = ssp_hadd_ps_REF(xmmdst.f, xmmsrc.f);
            break;

        case 0x7D:
            //hsubps(&xmmsrc,&xmmdst,&xmmres);
            xmmres.f = ssp_hsub_ps_REF(xmmdst.f, xmmsrc.f);
            break;

        case 0xD0:
            //addsubps(&xmmsrc,&xmmdst,&xmmres);
            xmmres.f = ssp_addsub_ps_REF(xmmdst.f, xmmsrc.f);
            break;

        case 0xF0:
            //lddqu(&xmmsrc,&xmmres);
            xmmres.i = ssp_lddqu_si128_REF(&xmmsrc.i);
            break;

        default:
            return 0;
    }
    
    storeresult128(*modrm, dst_higher, xmmres);
    
    return ins_size;
}

int sse3_run_b(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap)
{
    uint8_t *bytep = instruction;
    int ins_size = 0;
    ssp_m128 xmmsrc, xmmdst, xmmres;
    int src_higher = 0, dst_higher = 0;

    if(*bytep != 0xF3) return 0;

    bytep++;
    ins_size++;

    if(*bytep != 0x0f) return 0;
    bytep++;
    ins_size++;
    
    uint8_t *modrm = &bytep[1];
    ins_size += 1;
    int consumed = fetchoperands(modrm, src_higher, dst_higher, &xmmsrc, &xmmdst, longmode, state, kernel_trap, 1);
    ins_size += consumed;

    switch (*bytep)
    {
        case 0x12:
            //movsldup(&xmmsrc,&xmmres);
            xmmres.f = ssp_moveldup_ps_REF(xmmsrc.f);
            break;

        case 0x16:
            //movshdup(&xmmsrc,&xmmres);
            xmmres.f = ssp_movehdup_ps_REF(xmmsrc.f);
            break;

        default:
            return 0;
    }

    storeresult128(*modrm, dst_higher, xmmres);

    return ins_size;
}

int sse3_run_c(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap)
{
    uint8_t *bytep = instruction;
    int ins_size = 0;
    ssp_m128 xmmsrc, xmmdst, xmmres;
    int src_higher = 0, dst_higher = 0;
    
    if(*bytep != 0x66) return 0;
    
    bytep++;
    ins_size++;
    
    if(*bytep != 0x0f) return 0;
    bytep++;
    ins_size++;
    
    uint8_t *modrm = &bytep[1];
    ins_size += 1;
    int consumed = fetchoperands(modrm, src_higher, dst_higher, &xmmsrc, &xmmdst, longmode, state, kernel_trap, 1);
    ins_size += consumed;

    switch (*bytep)
    {
        case 0x7C:
            //haddpd(&xmmsrc,&xmmdst,&xmmres);
            xmmres.d = ssp_hadd_pd_REF(xmmdst.d, xmmsrc.d);
            break;

        case 0x7D:
            //hsubpd(&xmmsrc,&xmmdst,&xmmres);
            xmmres.d = ssp_hsub_pd_REF(xmmdst.d, xmmsrc.d);
            break;

        case 0xD0:
            //addsubpd(&xmmsrc,&xmmdst,&xmmres);
            xmmres.d = ssp_addsub_pd_REF(xmmdst.d, xmmsrc.d);
            break;

        default:
            return 0;
    }

    storeresult128(*modrm, dst_higher, xmmres);

    return ins_size;
}

int fisttp_run(uint8_t *instruction, x86_saved_state_t *state, int longmode, int __unused kernel_trap)
{
    uint8_t *bytep = instruction;
    int ins_size = 0;
    uint8_t base = 0;
    uint8_t mod = 0;
    int8_t add = 0;
    uint8_t modrm = 0;
    uint64_t address = 0;
    uint64_t reg_sel[8];

    if (longmode)
    {
        x86_saved_state64_t* r64 = saved_state64(state);
        reg_sel[0] = r64->rax;
        reg_sel[1] = r64->rcx;
        reg_sel[2] = r64->rdx;
        reg_sel[3] = r64->rbx;
        reg_sel[4] = r64->isf.rsp;
        reg_sel[5] = r64->rbp;
        reg_sel[6] = r64->rsi;
        reg_sel[7] = r64->rdi;
    } else {
        x86_saved_state32_t* r32 = saved_state32(state);
        reg_sel[0] = r32->eax;
        reg_sel[1] = r32->ecx;
        reg_sel[2] = r32->edx;
        reg_sel[3] = r32->ebx;
        reg_sel[4] = r32->uesp;
        reg_sel[5] = r32->ebp;
        reg_sel[6] = r32->esi;
        reg_sel[7] = r32->edi;
    }

    if (*bytep == 0x66)
    {
        bytep++;
        ins_size++;
    }

    switch (*bytep)
    {
        case 0xDB: //fist dword
        {
            bytep++;
            ins_size++;

            modrm = *bytep;
            base = modrm & 0x7;
            mod = (modrm & 0xC0) >> 6;

            if (mod == 0)
            {
                address = reg_sel[base];
            } else if (mod == 1) {
                bytep++;
                ins_size++;

                add = *bytep;
                address = reg_sel[base] + add;
            } else {
                return 0;
            }

            *(int *)address = fisttpl((double *)address);

            ins_size++;

            return(ins_size);
        }

        case 0xDD: //fst qword
        {
            bytep++;
            ins_size++;

            modrm = *bytep;
            base = modrm & 0x7;
            mod = (modrm & 0xC0) >> 6;

            if (mod == 0)
            {
                address = reg_sel[base];
            } else if (mod == 1) {
                bytep++;
                ins_size++;

                add = *bytep;
                address = reg_sel[base] + add;
            } else {
                return 0;
            }

            *(long long *)address = fisttpq((long double *)address);

            ins_size++;

            return(ins_size);
        }

        case 0xDF: //fist word
        {
            bytep++;
            ins_size++;

            modrm = *bytep;
            base = modrm & 0x7;
            mod = (modrm & 0xC0) >> 6;

            if (mod == 0)
            {
                address = reg_sel[base];
            } else if (mod == 1) {
                bytep++;
                ins_size++;

                add = *bytep;
                address = reg_sel[base] + add;
            } else {
                return 0;
            }

            *(short *)address = fisttps((float *)address);

            ins_size++;

            return(ins_size);
        }
    }
    return 0;
}

int monitor_mwait_run(uint8_t *instruction, __unused x86_saved_state_t *  state, int __unused longmode, int __unused kernel_trap)
{
    uint8_t *bytep = instruction;

    if (*bytep != 0x0F)
    {
        return 0;
    }

    bytep++;

    if (*bytep != 0x01)
    {
        return 0;
    }

    bytep++;

    switch(*bytep)
    {
        case 0xC8: //monitor : 0x0f,0x01,0xc8 sidt eax
        case 0xC9: //mwait :   0x0f,0x01,0xc9 sidt ecx
            return 3;
    }
    return 0;
}

/** Runs the ssse3 emulator. returns the number of bytes consumed.
 **/
int ssse3_run(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap)
{
    // pointer to the current byte we're working on
    uint8_t *bytep = instruction;
    int ins_size = 0;
    int is_128 = 0, src_higher = 0, dst_higher = 0;

    ssp_m128 xmmsrc, xmmdst, xmmres;
    ssp_m64 mmsrc,mmdst, mmres;

    /* We can get a few prefixes, in any order:
     * 66 throws into 128-bit xmm mode.
     */
    if(*bytep == 0x66)
    {
        is_128 = 1;
        bytep++;
        ins_size++;
    }

    /* 40->4f use higher xmm registers.*/
    if((*bytep & 0xF0) == 0x40)
    {
        if(*bytep & 1) src_higher = 1;
        if(*bytep & 4) dst_higher = 1;
        bytep++;
        ins_size++;
    }

    if(*bytep != 0x0f) return 0;

    bytep++;
    ins_size++;

    /* Two SSSE3 instruction prefixes. */
    if((*bytep == 0x38 && bytep[1] != 0x0f) || (*bytep == 0x3a && bytep[1] == 0x0f))
    {
        uint8_t opcode = bytep[1];
        uint8_t *modrm = &bytep[2];
        uint8_t imm;
        uint8_t mod = *modrm >> 6;
        uint8_t num_src = *modrm & 0x7;
        uint8_t base = modrm[1] & 0x7;
        ins_size += 2; // not counting modRM byte or anything after.

        if (mod == 0)
        {
            if (num_src == 4)
                if(base == 5) imm = bytep[8];//modrm offset + 6
                else imm = bytep[4];//modrm offset + 2
            else if (num_src == 5) imm = bytep[7];//modrm offset + 5
            else imm = bytep[3];//modrm offset + 1
        }

        if (mod == 1)
        {
            if(num_src == 4) imm = bytep[5];//modrm offset + 3
            else imm = bytep[4];//modrm offset + 2
        }

        if (mod == 2)
        {
            if(num_src == 4) imm = bytep[8];//modrm offset + 6
            else imm = bytep[7];//modrm offset + 5
        }

        if (mod == 3) imm = bytep[3];//modrm offset + 1

        if(is_128)
        {
            int consumed = fetchoperands(modrm, src_higher, dst_higher, &xmmsrc, &xmmdst, longmode, state, kernel_trap, 1);
            //operand = bytep[2 + consumed];
            ins_size += consumed;

            switch(opcode)
            {
                case 0x00: //pshufb128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_shuffle_epi8_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x01: //phaddw128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_hadd_epi16_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x02: //phaddd128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_hadd_epi32_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x03: //phaddsw128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_hadds_epi16_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x04: //pmaddubsw128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_maddubs_epi16_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x05: //phsubw128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_hsub_epi16_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x06: //phsubd128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_hsub_epi32_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x07: //phsubsw128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_hsubs_epi16_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x08: //psignb128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_sign_epi8_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x09: //psignw128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_sign_epi16_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x0A: //psignd128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_sign_epi32_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x0B: //pmulhrsw128(&xmmsrc,&xmmdst,&xmmres); break;
                    xmmres.i = ssp_mulhrs_epi16_SSSE3 (xmmdst.i, xmmsrc.i); break;
                case 0x0F: //palignr128(&xmmsrc,&xmmdst,&xmmres,imm); ins_size++; break;
                    xmmres.i = ssp_alignr_epi8_SSSE3 (xmmdst.i, xmmsrc.i, imm); ins_size++; break;
                case 0x1C: //pabsb128(&xmmsrc,&xmmres); break;
                    xmmres.i = ssp_abs_epi8_SSSE3 (xmmsrc.i); break;
                case 0x1D: //pabsw128(&xmmsrc,&xmmres); break;
                    xmmres.i = ssp_abs_epi16_SSSE3 (xmmsrc.i); break;
                case 0x1E: //pabsd128(&xmmsrc,&xmmres); break;
                    xmmres.i = ssp_abs_epi32_SSSE3 (xmmsrc.i); break;
                default: return 0;
            }

            storeresult128(*modrm, dst_higher, xmmres);
        }
        else
        {
            int consumed = fetchoperands(modrm, src_higher, dst_higher, &mmsrc, &mmdst, longmode, state, kernel_trap, 0);
            //operand = bytep[2 + consumed];
            ins_size += consumed;

            switch(opcode)
            {
                case 0x00: //pshufb64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_shuffle_pi8_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x01: //phaddw64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_hadd_pi16_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x02: //phaddd64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_hadd_pi32_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x03: //phaddsw64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_hadds_pi16_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x04: //pmaddubsw64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_maddubs_pi16_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x05: //phsubw64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_hsub_pi16_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x06: //phsubd64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_hsub_pi32_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x07: //phsubsw64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_hsubs_pi16_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x08: //psignb64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_sign_pi8_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x09: //psignw64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_sign_pi16_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x0A: //psignd64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_sign_pi32_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x0B: //pmulhrsw64(&mmsrc,&mmdst,&mmres); break;
                    mmres.m64 = ssp_mulhrs_pi16_SSSE3 (mmdst.m64, mmsrc.m64); break;
                case 0x0F: //palignr64(&mmsrc,&mmdst,&mmres, imm); ins_size++; break;
                    mmres.m64 = ssp_alignr_pi8_SSSE3 (mmdst.m64, mmsrc.m64, imm); ins_size++; break;
                case 0x1C: //pabsb64(&mmsrc,&mmres); break;
                    mmres.m64 = ssp_abs_pi8_SSSE3 (mmsrc.m64); break;
                case 0x1D: //pabsw64(&mmsrc,&mmres); break;
                    mmres.m64 = ssp_abs_pi16_SSSE3 (mmsrc.m64); break;
                case 0x1E: //pabsd64(&mmsrc,&mmres); break;
                    mmres.m64 = ssp_abs_pi32_SSSE3 (mmsrc.m64); break;
                default: return 0;
            }

            storeresult64(*modrm, dst_higher, mmres);
        }

    }
    else
    {
        // opcode wasn't handled here
        return 0;
    }

    return ins_size;
}

void print_bytes(uint8_t *from, int size)
{
    int i;
    for(i = 0; i < size; ++i)
    {
        printf("%02x ", from[i]);
    }
    printf("\n");
}

/** Fetch SSSE3 operands (except immediate values, which are fetched elsewhere).
 * We depend on memory copies, which differs depending on whether we're in kernel space
 * or not. For this reason, need to pass in a lot of information, including the state of
 * registers.
 *
 * The return value is the number of bytes used, including the ModRM byte,
 * and displacement values, as well as SIB if used.
 */
int fetchoperands(uint8_t *ModRM, unsigned int hsrc, unsigned int hdst, void *src, void *dst, unsigned int longmode, x86_saved_state_t *saved_state, int kernel_trap, int size_128)
{
    unsigned int num_src = *ModRM & 0x7; // 1 byte + 1 regs loop
    unsigned int num_dst = (*ModRM >> 3) & 0x7; //8 byte + 1 xmm loop
    unsigned int mod = *ModRM >> 6; // 40 byte + 1
    int consumed = 1; //modrm + 1 byte
    uint8_t bytelong = 0x00;

    if(hsrc) num_src += 8;
    if(hdst) num_dst += 8;

    if(size_128)
        getxmm((ssp_m128*)dst, num_dst);
    else
        getmm((ssp_m64*)dst, num_dst);

    if(mod == 3)
    {
        if(size_128)
            getxmm((ssp_m128*)src, num_src);
        else
            getmm((ssp_m64*)src, num_src);
    }

    // Implemented 64-bit fetch
    else if ((longmode = is_saved_state64(saved_state)))
    {
        uint64_t address;
        // DST is always an XMM register. decode for SRC.
        x86_saved_state64_t *r64 = saved_state64(saved_state);
        __uint64_t reg_sel[8] = {r64->rax, r64->rcx, r64->rdx,
            r64->rbx, r64->isf.rsp, r64->rbp,
            r64->rsi, r64->rdi};

        if(hsrc) printf("opemu error: high reg ssse\n"); // FIXME

        if (mod == 0)
        {
            if (num_src == 4)
            {
                uint8_t scale = ModRM[1] >> 6; //40 byte + 1
                uint8_t base = ModRM[1] & 0x7; //1 byte + 1 regs loop
                uint8_t index = (ModRM[1] >> 3) & 0x7; //8 byte + 1 regs loop

                if(base == 5)
                {
                    //ModRM              0  1  2  3  4  5
                    //byte   0  1  2  3  4  5  6  7  8  9
                    //OPCPDE 66 0F 38 01 04 05 04 03 02 01
                    //INS    phaddw xmm0, xmmword [ds:0x1020304+rax]
                    bytelong = 0x0a;
                    address = (r64->isf.rip + bytelong + *((int32_t*)&ModRM[2])) + reg_sel[index];
                    consumed += 5;
                }
                else
                {
                    if (index == 4)
                    {
                        //ModRM              0  1
                        //byte   0  1  2  3  4  5
                        //OPCPDE 66 0F 38 01 04 61
                        //INS    phaddw xmm0, xmmword [ds:rcx]
                        address = reg_sel[base];
                        consumed++;
                    }
                    else
                    {
                        //ModRM              0  1
                        //byte   0  1  2  3  4  5
                        //OPCPDE 66 0F 38 01 04 44
                        //INS    phaddw xmm0, xmmword ptr [rsp+rax*2]
                        address = reg_sel[base] + (reg_sel[index] * (1<<scale));
                        consumed++;
                    }
                }
            }
            else if (num_src == 5)
            {
                //ModRM              0  1  2  3  4
                //BYTE   0  1  2  3  4  5  6  7  8
                //OPCPDE 66 0F 38 01 05 01 02 03 04
                //INS    phaddw xmm0, xmmword ptr [cs:04030201h]
                bytelong = 0x09;
                address = r64->isf.rip + bytelong + *((int32_t*)&ModRM[1]);
                consumed += 4;
                /***DEBUG***/
                //int32_t ds = *((int32_t*)&ModRM[1]);
                //printf("DEBUG-MSG: byte=%x, RIP=%llx ,DS=%d, PTR = %llx\n", bytelong , r64->isf.rip, ds, address);
                /***DEBUG***/
            }
            else
            {
                //ModRM              0
                //BYTE   0  1  2  3  4
                //OPCPDE 66 0F 38 01 03
                //INS    phaddw xmm0, xmmword [ds:rbx]
                address = reg_sel[num_src];
            }
        }

        if (mod == 1)
        {
            if(num_src == 4)
            {
                uint8_t scale = ModRM[1] >> 6; //40 byte + 1
                uint8_t base = ModRM[1] & 0x7; //1 byte + 1 reg loop
                uint8_t index = (ModRM[1] >> 3) & 0x7; //8 byte + 1 reg loop

                if (index == 4)
                {
                    //ModRM              0  1  2
                    //BYTE   0  1  2  3  4  5  6
                    //OPCPDE 66 0F 38 01 44 27 80
                    //INS    phaddw xmm0, xmmword ptr [rdi-80h]
                    address = reg_sel[base] + (int8_t)ModRM[2];
                    consumed+= 2;
                }
                else
                {
                    //ModRM              0  1  2
                    //BYTE   0  1  2  3  4  5  6
                    //OPCPDE 66 0F 38 01 44 44 80
                    //INS    phaddw xmm0, xmmword ptr [rsp+rax*2-80h]
                    address = reg_sel[base] + (reg_sel[index] * (1<<scale)) + (int8_t)ModRM[2];
                    consumed+= 2;
                }
            }
            else
            {
                //ModRM              0  1
                //BYTE   0  1  2  3  4  5
                //OPCPDE 66 0F 38 01 43 01
                //INS    phaddw xmm0, xmmword [ds:rbx+0x1]
                address = reg_sel[num_src] + (int8_t)ModRM[1];
                consumed++;
            }
        }

        if (mod == 2)
        {
            if(num_src == 4)
            {
                uint8_t scale = ModRM[1] >> 6; //40 byte + 1
                uint8_t base = ModRM[1] & 0x7; //1 byte + 1 reg loop
                uint8_t index = (ModRM[1] >> 3) & 0x7; //8 byte + 1 reg loop

                if (index == 4)
                {
                    //ModRM              0  1  2  3  4  5
                    //BYTE   0  1  2  3  4  5  6  7  8  9
                    //OPCPDE 66 0F 38 01 84 20 04 03 02 01
                    //INS    phaddw xmm0, xmmword ptr [rax+1020304h]
                    bytelong = 0x0a;
                    address = reg_sel[base] + (r64->isf.rip + bytelong + *((int32_t*)&ModRM[2]));
                    consumed += 5;
                }
                else
                {
                    //ModRM              0  1  2  3  4  5
                    //BYTE   0  1  2  3  4  5  6  7  8  9
                    //OPCPDE 66 0F 38 01 84 44 04 03 02 01
                    //INS    phaddw xmm0, xmmword ptr [rsp+rax*2+1020304h]
                    bytelong = 0x0a;
                    address = reg_sel[base] + (reg_sel[index] * (1<<scale)) + (r64->isf.rip + bytelong + *((int32_t*)&ModRM[2]));
                    consumed += 5;
                }

            }
            else
            {
                //ModRM              0  1  2  3  4
                //BYTE   0  1  2  3  4  5  6  7  8
                //OPCPDE 66 0F 38 01 83 01 02 03 04
                //INS    phaddw xmm0, xmmword [ds:rbx+0x4030201]
                bytelong = 0x09;
                address = reg_sel[num_src] + (r64->isf.rip + bytelong + *((int32_t*)&ModRM[1]));
                consumed += 4;
            }
        }

        // address is good now, do read and store operands.
        if(kernel_trap)
        {
            if(size_128) ((ssp_m128*)src)->ui = *((__uint128_t*)address);
            else ((ssp_m64*)src)->u64 = *((uint64_t*)address);
        }
        else
        {
            //printf("XNU: PTR = %llx, RIP=%llx, RSP=%llx\n", address, r64->isf.rip, reg_sel[4]);
            if(size_128) copyin(address, (char*)& ((ssp_m128*)src)->ui, 16);
            else copyin(address, (char*)& ((ssp_m64*)src)->u64, 8);
        }
    }
    // AnV - Implemented 32-bit fetch
    else
    {
        uint32_t address;
        // DST is always an XMM register. decode for SRC.
        x86_saved_state32_t* r32 = saved_state32(saved_state);
        uint32_t reg_sel[8] = {r32->eax, r32->ecx, r32->edx,
            r32->ebx, r32->uesp, r32->ebp,
            r32->esi, r32->edi};

        if(hsrc) printf("opemu error: high reg ssse\n"); // FIXME

        if (mod == 0)
        {
            if (num_src == 4)
            {
                uint8_t scale = ModRM[1] >> 6; //40 byte + 1
                uint8_t base = ModRM[1] & 0x7; //1 byte + 1 reg loop
                uint8_t index = (ModRM[1] >> 3) & 0x7; //8 byte + 1 reg loop

                if(base == 5)
                {
                    //ModRM           0  1  2  3  4  5
                    //byte   0  1  2  3  4  5  6  7  8
                    //OPCPDE 0F 38 01 04 05 04 03 02 01
                    //INS    phaddw mm0, qword [ds:0x1020304+rax]
                    address = *((uint32_t*)&ModRM[2]) + reg_sel[index];
                    consumed += 5;
                }
                else
                {
                    if (index == 4)
                    {
                        //ModRM           0  1
                        //byte   0  1  2  3  4
                        //OPCPDE 0F 38 01 04 61
                        //INS    phaddw mm0, qword [ds:rcx]
                        address = reg_sel[base];
                        consumed++;
                    }
                    else
                    {
                        //ModRM           0  1
                        //byte   0  1  2  3  4
                        //OPCPDE 0F 38 01 04 44
                        //INS    phaddw mm0, qword ptr [rsp+rax*2]
                        address = reg_sel[base] + (reg_sel[index] * (1<<scale));
                        consumed++;
                    }
                }
            }
            else if (num_src == 5)
            {
                //ModRM           0  1  2  3  4
                //BYTE   0  1  2  3  4  5  6  7
                //OPCPDE 0F 38 01 05 01 02 03 04
                //INS    phaddw mm0, qword ptr [cs:04030201h]
                address = *((uint32_t*)&ModRM[1]);
                consumed += 4;
            }
            else
            {
                //ModRM           0
                //BYTE   0  1  2  3
                //OPCPDE 0F 38 01 03
                //INS    phaddw mm0, qword [ds:rbx]
                address = reg_sel[num_src];
            }
        }

        if (mod == 1)
        {
            if(num_src == 4)
            {
                uint8_t scale = ModRM[1] >> 6; //40 byte + 1
                uint8_t base = ModRM[1] & 0x7; //1 byte + 1 reg loop
                uint8_t index = (ModRM[1] >> 3) & 0x7; //8 byte + 1 reg loop

                if (index == 4)
                {
                    //ModRM           0  1  2
                    //BYTE   0  1  2  3  4  5
                    //OPCPDE 0F 38 01 44 27 80
                    //INS    phaddw mm0, qword ptr [rdi-80h]
                    address = reg_sel[base] + (int8_t)ModRM[2];
                    consumed+= 2;
                }
                else
                {
                    //ModRM           0  1  2
                    //BYTE   0  1  2  3  4  5
                    //OPCPDE 0F 38 01 44 44 80
                    //INS    phaddw mm0, qword ptr [rsp+rax*2-80h]
                    address = reg_sel[base] + (reg_sel[index] * (1<<scale)) + (int8_t)ModRM[2];
                    consumed+= 2;
                }
            }
            else
            {
                //ModRM           0  1
                //BYTE   0  1  2  3  4
                //OPCPDE 0F 38 01 43 01
                //INS    phaddw mm0, qword [ds:rbx+0x1]
                address = reg_sel[num_src] + (int8_t)ModRM[1];
                consumed++;
            }
        }

        if (mod == 2)
        {
            if(num_src == 4)
            {
                uint8_t scale = ModRM[1] >> 6; //40 byte + 1
                uint8_t base = ModRM[1] & 0x7; //1 byte + 1 reg loop
                uint8_t index = (ModRM[1] >> 3) & 0x7; //8 byte + 1 reg loop

                if (index == 4)
                {
                    //ModRM           0  1  2  3  4  5
                    //BYTE   0  1  2  3  4  5  6  7  8
                    //OPCPDE 0F 38 01 84 20 04 03 02 01
                    //INS    phaddw mm0, qword ptr [rax+1020304h]
                    address = reg_sel[base] + *((uint32_t*)&ModRM[2]);
                    consumed += 5;
                }
                else
                {
                    //ModRM           0  1  2  3  4  5
                    //BYTE   0  1  2  3  4  5  6  7  8
                    //OPCPDE 0F 38 01 84 44 04 03 02 01
                    //INS    phaddw mm0, qword ptr [rsp+rax*2+1020304h]
                    address = reg_sel[base] + (reg_sel[index] * (1<<scale)) + *((uint32_t*)&ModRM[2]);
                    consumed += 5;
                }

            }
            else
            {
                //ModRM           0  1  2  3  4
                //BYTE   0  1  2  3  4  5  6  7
                //OPCPDE 0F 38 01 83 01 02 03 04
                //INS    phaddw mm0, qword [ds:rbx+0x4030201]
                address = reg_sel[num_src] + *((uint32_t*)&ModRM[1]);
                consumed += 4;
            }
        }

        // address is good now, do read and store operands.
        uint64_t addr = address;

        if(kernel_trap)
        {
            if(size_128) ((ssp_m128*)src)->ui = *((__uint128_t*)addr);
            else ((ssp_m64*)src)->u64 = *((uint64_t*)addr);
        }
        else
        {
            //printf("xnu: da = %llx, rsp=%llx,  rip=%llx\n", address, reg_sel[4], r32->eip);
            if(size_128) copyin(addr, (char*) &((ssp_m128*)src)->ui, 16);
            else copyin(addr, (char*) &((ssp_m64*)src)->u64, 8);
        }
    }
    //AnV - Implemented 32-bit fetch END
    return consumed;
}

void storeresult128(uint8_t ModRM, unsigned int hdst, ssp_m128 res)
{
    unsigned int num_dst = (ModRM >> 3) & 0x7;
    if(hdst) num_dst += 8;
    movxmm(&res, num_dst);
}
void storeresult64(uint8_t ModRM, unsigned int __unused hdst, ssp_m64 res)
{
    unsigned int num_dst = (ModRM >> 3) & 0x7;
    movmm(&res, num_dst);
}

#endif /* TESTCASE */

/* get value from the xmm register i */
void getxmm(ssp_m128 *v, unsigned int i)
{
    switch(i) {
        case 0:
            asm __volatile__ ("movdqu %%xmm0, %0" : "=m" (*v->s8));
            break;
        case 1:
            asm __volatile__ ("movdqu %%xmm1, %0" : "=m" (*v->s8));
            break;
        case 2:
            asm __volatile__ ("movdqu %%xmm2, %0" : "=m" (*v->s8));
            break;
        case 3:
            asm __volatile__ ("movdqu %%xmm3, %0" : "=m" (*v->s8));
            break;
        case 4:
            asm __volatile__ ("movdqu %%xmm4, %0" : "=m" (*v->s8));
            break;
        case 5:
            asm __volatile__ ("movdqu %%xmm5, %0" : "=m" (*v->s8));
            break;
        case 6:
            asm __volatile__ ("movdqu %%xmm6, %0" : "=m" (*v->s8));
            break;
        case 7:
            asm __volatile__ ("movdqu %%xmm7, %0" : "=m" (*v->s8));
            break;
#ifdef __x86_64__
        case 8:
            asm __volatile__ ("movdqu %%xmm8, %0" : "=m" (*v->s8));
            break;
        case 9:
            asm __volatile__ ("movdqu %%xmm9, %0" : "=m" (*v->s8));
            break;
        case 10:
            asm __volatile__ ("movdqu %%xmm10, %0" : "=m" (*v->s8));
            break;
        case 11:
            asm __volatile__ ("movdqu %%xmm11, %0" : "=m" (*v->s8));
            break;
        case 12:
            asm __volatile__ ("movdqu %%xmm12, %0" : "=m" (*v->s8));
            break;
        case 13:
            asm __volatile__ ("movdqu %%xmm13, %0" : "=m" (*v->s8));
            break;
        case 14:
            asm __volatile__ ("movdqu %%xmm14, %0" : "=m" (*v->s8));
            break;
        case 15:
            asm __volatile__ ("movdqu %%xmm15, %0" : "=m" (*v->s8));
            break;
#endif
    }
}

/* get value from the mm register i  */
void getmm(ssp_m64 *v, unsigned int i)
{
    switch(i) {
        case 0:
            asm __volatile__ ("movq %%mm0, %0" : "=m" (*v->s8));
            break;
        case 1:
            asm __volatile__ ("movq %%mm1, %0" : "=m" (*v->s8));
            break;
        case 2:
            asm __volatile__ ("movq %%mm2, %0" : "=m" (*v->s8));
            break;
        case 3:
            asm __volatile__ ("movq %%mm3, %0" : "=m" (*v->s8));
            break;
        case 4:
            asm __volatile__ ("movq %%mm4, %0" : "=m" (*v->s8));
            break;
        case 5:
            asm __volatile__ ("movq %%mm5, %0" : "=m" (*v->s8));
            break;
        case 6:
            asm __volatile__ ("movq %%mm6, %0" : "=m" (*v->s8));
            break;
        case 7:
            asm __volatile__ ("movq %%mm7, %0" : "=m" (*v->s8));
            break;
    }
}

/* move value over to xmm register i */
void movxmm(ssp_m128 *v, unsigned int i)
{
    switch(i) {
        case 0:
            asm __volatile__ ("movdqu %0, %%xmm0" :: "m" (*v->s8) );
            break;
        case 1:
            asm __volatile__ ("movdqu %0, %%xmm1" :: "m" (*v->s8) );
            break;
        case 2:
            asm __volatile__ ("movdqu %0, %%xmm2" :: "m" (*v->s8) );
            break;
        case 3:
            asm __volatile__ ("movdqu %0, %%xmm3" :: "m" (*v->s8) );
            break;
        case 4:
            asm __volatile__ ("movdqu %0, %%xmm4" :: "m" (*v->s8) );
            break;
        case 5:
            asm __volatile__ ("movdqu %0, %%xmm5" :: "m" (*v->s8) );
            break;
        case 6:
            asm __volatile__ ("movdqu %0, %%xmm6" :: "m" (*v->s8) );
            break;
        case 7:
            asm __volatile__ ("movdqu %0, %%xmm7" :: "m" (*v->s8) );
            break;
#ifdef __x86_64__
        case 8:
            asm __volatile__ ("movdqu %0, %%xmm8" :: "m" (*v->s8) );
            break;
        case 9:
            asm __volatile__ ("movdqu %0, %%xmm9" :: "m" (*v->s8) );
            break;
        case 10:
            asm __volatile__ ("movdqu %0, %%xmm10" :: "m" (*v->s8) );
            break;
        case 11:
            asm __volatile__ ("movdqu %0, %%xmm11" :: "m" (*v->s8) );
            break;
        case 12:
            asm __volatile__ ("movdqu %0, %%xmm12" :: "m" (*v->s8) );
            break;
        case 13:
            asm __volatile__ ("movdqu %0, %%xmm13" :: "m" (*v->s8) );
            break;
        case 14:
            asm __volatile__ ("movdqu %0, %%xmm14" :: "m" (*v->s8) );
            break;
        case 15:
            asm __volatile__ ("movdqu %0, %%xmm15" :: "m" (*v->s8) );
            break;
#endif
    }
}

/* move value over to mm register i */
void movmm(ssp_m64 *v, unsigned int i)
{
    switch(i) {
        case 0:
            asm __volatile__ ("movq %0, %%mm0" :: "m" (*v->s8) );
            break;
        case 1:
            asm __volatile__ ("movq %0, %%mm1" :: "m" (*v->s8) );
            break;
        case 2:
            asm __volatile__ ("movq %0, %%mm2" :: "m" (*v->s8) );
            break;
        case 3:
            asm __volatile__ ("movq %0, %%mm3" :: "m" (*v->s8) );
            break;
        case 4:
            asm __volatile__ ("movq %0, %%mm4" :: "m" (*v->s8) );
            break;
        case 5:
            asm __volatile__ ("movq %0, %%mm5" :: "m" (*v->s8) );
            break;
        case 6:
            asm __volatile__ ("movq %0, %%mm6" :: "m" (*v->s8) );
            break;
        case 7:
            asm __volatile__ ("movq %0, %%mm7" :: "m" (*v->s8) );
            break;
    }
}

short fisttps(float *res)
{
    float value = opemu_truncf(*res);
    __asm__ ("fistps %0" : : "m" (value));
    *res = value;
    return (short)(res);
}

int fisttpl(double *res)
{
    double value = opemu_trunc(*res);
    __asm__ ("fistpl %0" : : "m" (value));
    *res = value;
    return (int)res;
}

long long fisttpq(long double *res)
{
    long double value = *res;
    __asm__ ("fistpq %0" : : "m" (value));
    *res = value;
    return (long long)res;
}
