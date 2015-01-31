/*
 * Copyright (c) 2009 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

TRAP(0x00,idt64_zero_div)
TRAP_SPC(0x01,idt64_debug)
TRAP_IST2(0x02,idt64_nmi)
USER_TRAP(0x03,idt64_int3)
USER_TRAP(0x04,idt64_into)
USER_TRAP(0x05,idt64_bounds)
TRAP(0x06,idt64_invop)
TRAP(0x07,idt64_nofpu)
TRAP_IST1(0x08,idt64_double_fault)
TRAP(0x09,idt64_fpu_over)
TRAP_ERR(0x0a,idt64_inv_tss)
TRAP_IST1(0x0b,idt64_segnp)
TRAP_IST1(0x0c,idt64_stack_fault)
TRAP_IST1(0x0d,idt64_gen_prot)
TRAP_SPC(0x0e,idt64_page_fault)
TRAP(0x0f,idt64_trap_0f)
TRAP(0x10,idt64_fpu_err)
TRAP_ERR(0x11,idt64_alignment_check)
TRAP_IST1(0x12,idt64_mc)
TRAP(0x13,idt64_sse_err)
TRAP(0x14,idt64_trap_14)
TRAP(0x15,idt64_trap_15)
TRAP(0x16,idt64_trap_16)
TRAP(0x17,idt64_trap_17)
TRAP(0x18,idt64_trap_18)
TRAP(0x19,idt64_trap_19)
TRAP(0x1a,idt64_trap_1a)
TRAP(0x1b,idt64_trap_1b)
TRAP(0x1c,idt64_trap_1c)
TRAP(0x1d,idt64_trap_1d)
TRAP(0x1e,idt64_trap_1e)
TRAP(0x1f,idt64_trap_1f)

INTERRUPT(0x20)
INTERRUPT(0x21)
INTERRUPT(0x22)
INTERRUPT(0x23)
INTERRUPT(0x24)
INTERRUPT(0x25)
INTERRUPT(0x26)
INTERRUPT(0x27)
INTERRUPT(0x28)
INTERRUPT(0x29)
INTERRUPT(0x2a)
INTERRUPT(0x2b)
INTERRUPT(0x2c)
INTERRUPT(0x2d)
INTERRUPT(0x2e)
INTERRUPT(0x2f)

INTERRUPT(0x30)
INTERRUPT(0x31)
INTERRUPT(0x32)
INTERRUPT(0x33)
INTERRUPT(0x34)
INTERRUPT(0x35)
INTERRUPT(0x36)
INTERRUPT(0x37)
INTERRUPT(0x38)
INTERRUPT(0x39)
INTERRUPT(0x3a)
INTERRUPT(0x3b)
INTERRUPT(0x3c)
INTERRUPT(0x3d)
INTERRUPT(0x3e)
INTERRUPT(0x3f)

INTERRUPT(0x40)
INTERRUPT(0x41)
INTERRUPT(0x42)
INTERRUPT(0x43)
INTERRUPT(0x44)
INTERRUPT(0x45)
INTERRUPT(0x46)
INTERRUPT(0x47)
INTERRUPT(0x48)
INTERRUPT(0x49)
INTERRUPT(0x4a)
INTERRUPT(0x4b)
INTERRUPT(0x4c)
INTERRUPT(0x4d)
INTERRUPT(0x4e)
INTERRUPT(0x4f)

INTERRUPT(0x50)
INTERRUPT(0x51)
INTERRUPT(0x52)
INTERRUPT(0x53)
INTERRUPT(0x54)
INTERRUPT(0x55)
INTERRUPT(0x56)
INTERRUPT(0x57)
INTERRUPT(0x58)
INTERRUPT(0x59)
INTERRUPT(0x5a)
INTERRUPT(0x5b)
INTERRUPT(0x5c)
INTERRUPT(0x5d)
INTERRUPT(0x5e)
INTERRUPT(0x5f)

INTERRUPT(0x60)
INTERRUPT(0x61)
INTERRUPT(0x62)
INTERRUPT(0x63)
INTERRUPT(0x64)
INTERRUPT(0x65)
INTERRUPT(0x66)
INTERRUPT(0x67)
INTERRUPT(0x68)
INTERRUPT(0x69)
INTERRUPT(0x6a)
INTERRUPT(0x6b)
INTERRUPT(0x6c)
INTERRUPT(0x6d)
INTERRUPT(0x6e)
INTERRUPT(0x6f)

INTERRUPT(0x70)
INTERRUPT(0x71)
INTERRUPT(0x72)
INTERRUPT(0x73)
INTERRUPT(0x74)
INTERRUPT(0x75)
INTERRUPT(0x76)
INTERRUPT(0x77)
INTERRUPT(0x78)
INTERRUPT(0x79)
INTERRUPT(0x7a)
INTERRUPT(0x7b)
INTERRUPT(0x7c)
INTERRUPT(0x7d)
INTERRUPT(0x7e)
USER_TRAP(0x7f, idt64_dtrace_ret) /* Required by dtrace "fasttrap" */

USER_TRAP_SPC(0x80,idt64_unix_scall)
USER_TRAP_SPC(0x81,idt64_mach_scall)
USER_TRAP_SPC(0x82,idt64_mdep_scall)

INTERRUPT(0x83)
INTERRUPT(0x84)
INTERRUPT(0x85)
INTERRUPT(0x86)
INTERRUPT(0x87)
INTERRUPT(0x88)
INTERRUPT(0x89)
INTERRUPT(0x8a)
INTERRUPT(0x8b)
INTERRUPT(0x8c)
INTERRUPT(0x8d)
INTERRUPT(0x8e)
INTERRUPT(0x8f)

INTERRUPT(0x90)
INTERRUPT(0x91)
INTERRUPT(0x92)
INTERRUPT(0x93)
INTERRUPT(0x94)
INTERRUPT(0x95)
INTERRUPT(0x96)
INTERRUPT(0x97)
INTERRUPT(0x98)
INTERRUPT(0x99)
INTERRUPT(0x9a)
INTERRUPT(0x9b)
INTERRUPT(0x9c)
INTERRUPT(0x9d)
INTERRUPT(0x9e)
INTERRUPT(0x9f)

INTERRUPT(0xa0)
INTERRUPT(0xa1)
INTERRUPT(0xa2)
INTERRUPT(0xa3)
INTERRUPT(0xa4)
INTERRUPT(0xa5)
INTERRUPT(0xa6)
INTERRUPT(0xa7)
INTERRUPT(0xa8)
INTERRUPT(0xa9)
INTERRUPT(0xaa)
INTERRUPT(0xab)
INTERRUPT(0xac)
INTERRUPT(0xad)
INTERRUPT(0xae)
INTERRUPT(0xaf)

INTERRUPT(0xb0)
INTERRUPT(0xb1)
INTERRUPT(0xb2)
INTERRUPT(0xb3)
INTERRUPT(0xb4)
INTERRUPT(0xb5)
INTERRUPT(0xb6)
INTERRUPT(0xb7)
INTERRUPT(0xb8)
INTERRUPT(0xb9)
INTERRUPT(0xba)
INTERRUPT(0xbb)
INTERRUPT(0xbc)
INTERRUPT(0xbd)
INTERRUPT(0xbe)
INTERRUPT(0xbf)

INTERRUPT(0xc0)
INTERRUPT(0xc1)
INTERRUPT(0xc2)
INTERRUPT(0xc3)
INTERRUPT(0xc4)
INTERRUPT(0xc5)
INTERRUPT(0xc6)
INTERRUPT(0xc7)
INTERRUPT(0xc8)
INTERRUPT(0xc9)
INTERRUPT(0xca)
INTERRUPT(0xcb)
INTERRUPT(0xcc)
INTERRUPT(0xcd)
INTERRUPT(0xce)
INTERRUPT(0xcf)

/* Local APIC interrupt vectors */
INTERRUPT(0xd0)
INTERRUPT(0xd1)
INTERRUPT(0xd2)
INTERRUPT(0xd3)
INTERRUPT(0xd4)
INTERRUPT(0xd5)
INTERRUPT(0xd6)
INTERRUPT(0xd7)
INTERRUPT(0xd8)
INTERRUPT(0xd9)
INTERRUPT(0xda)
INTERRUPT(0xdb)
INTERRUPT(0xdc)
INTERRUPT(0xdd)
INTERRUPT(0xde)
INTERRUPT(0xdf)

INTERRUPT(0xe0)
INTERRUPT(0xe1)
INTERRUPT(0xe2)
INTERRUPT(0xe3)
INTERRUPT(0xe4)
INTERRUPT(0xe5)
INTERRUPT(0xe6)
INTERRUPT(0xe7)
INTERRUPT(0xe8)
INTERRUPT(0xe9)
INTERRUPT(0xea)
INTERRUPT(0xeb)
INTERRUPT(0xec)
INTERRUPT(0xed)
INTERRUPT(0xee)
INTERRUPT(0xef)

INTERRUPT(0xf0)
INTERRUPT(0xf1)
INTERRUPT(0xf2)
INTERRUPT(0xf3)
INTERRUPT(0xf4)
INTERRUPT(0xf5)
INTERRUPT(0xf6)
INTERRUPT(0xf7)
INTERRUPT(0xf8)
INTERRUPT(0xf9)
INTERRUPT(0xfa)
USER_TRAP_SPC(0xfb, idt64_fake_cpuid)
USER_TRAP_SPC(0xfc,idt64_fake_sysenter)
INTERRUPT(0xfd)
INTERRUPT(0xfe)
TRAP(0xff,idt64_preempt)
