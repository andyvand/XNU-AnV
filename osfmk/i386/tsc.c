/*
 * Copyright (c) 2005-2007 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */

/*
 *	File:		i386/tsc.c
 *	Purpose:	Initializes the TSC and the various conversion
 *			factors needed by other parts of the system.
 */


#include <mach/mach_types.h>

#include <kern/cpu_data.h>
#include <kern/cpu_number.h>
#include <kern/clock.h>
#include <kern/host_notify.h>
#include <kern/macro_help.h>
#include <kern/misc_protos.h>
#include <kern/spl.h>
#include <kern/assert.h>
#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>		/* for kernel_map */
#include <architecture/i386/pio.h>
#include <i386/machine_cpu.h>
#include <i386/cpuid.h>
#include <i386/mp.h>
#include <i386/machine_routines.h>
#include <i386/proc_reg.h>
#include <i386/tsc.h>
#include <i386/misc_protos.h>
#include <pexpert/pexpert.h>
#include <machine/limits.h>
#include <machine/commpage.h>
#include <sys/kdebug.h>
#include <pexpert/device_tree.h>

uint64_t	busFCvtt2n = 0;
uint64_t	busFCvtn2t = 0;
uint64_t	tscFreq = 0;
uint64_t	tscFCvtt2n = 0;
uint64_t	tscFCvtn2t = 0;
uint64_t	tscGranularity = 0;
uint64_t	bus2tsc = 0;
uint64_t	busFreq = 0;
uint32_t	flex_ratio = 0;
uint32_t	flex_ratio_min = 0;
uint32_t	flex_ratio_max = 0;

uint64_t	tsc_at_boot = 0;

#define bit(n)		(1ULL << (n))
#define bitmask(h,l)	((bit(h)|(bit(h)-1)) & ~(bit(l)-1))
#define bitfield(x,h,l)	(((x) & bitmask(h,l)) >> l)

/* Decimal powers: */
#define kilo (1000ULL)
#define Mega (kilo * kilo)
#define Giga (kilo * Mega)
#define Tera (kilo * Giga)
#define Peta (kilo * Tera)

#define CPU_FAMILY_PENTIUM_M	(0x6)

static const char	FSB_Frequency_prop[] = "FSBFrequency";
static const char   	FSB_CPUFrequency_prop[] = "CPUFrequency";
static const char	TSC_at_boot_prop[]   = "InitialTSC";
/*
 * This routine extracts the bus frequency in Hz from the device tree.
 * Also reads any initial TSC value at boot from the device tree.
 */
static uint64_t
EFI_FSB_frequency(void)
{
	uint64_t	frequency = 0;
	DTEntry		entry;
	void		*value;
	unsigned int	size;

	if (DTLookupEntry(0, "/efi/platform", &entry) != kSuccess) {
		kprintf("EFI_FSB_frequency: didn't find /efi/platform\n");
		return 0;
	}
	if (DTGetProperty(entry,FSB_Frequency_prop,&value,&size) != kSuccess) {
		kprintf("EFI_FSB_frequency: property %s not found\n",
			FSB_Frequency_prop);
		return 0;
	}
	if (size == sizeof(uint64_t)) {
		frequency = *(uint64_t *) value;
		kprintf("EFI_FSB_frequency: read %s value: %llu\n",
			FSB_Frequency_prop, frequency);
		if (!(90*Mega < frequency && frequency < 10*Giga)) {
			kprintf("EFI_FSB_frequency: value out of range\n");
			frequency = 0;
		}
	} else {
		kprintf("EFI_FSB_frequency: unexpected size %d\n", size);
	}

	/*
	 * While we're here, see if EFI published an initial TSC value.
	 */
	if (DTGetProperty(entry,TSC_at_boot_prop,&value,&size) == kSuccess) {
		if (size == sizeof(uint64_t)) {
			tsc_at_boot = *(uint64_t *) value;
			kprintf("EFI_FSB_frequency: read %s value: %llu\n",
				TSC_at_boot_prop, tsc_at_boot);
		}
	}

	return frequency;
}

static uint64_t
EFI_CPU_Frequency(void)
{
   	uint64_t	frequency = 0;
    DTEntry		entry;
    void		*value;
    unsigned int	size;

    if (DTLookupEntry(0, "/efi/platform", &entry) != kSuccess) {
        kprintf("EFI_CPU_Frequency: didn't find /efi/platform\n");
        return 0;
    }
    if (DTGetProperty(entry,FSB_CPUFrequency_prop,&value,&size) != kSuccess) {
        kprintf("EFI_CPU_Frequency: property %s not found\n",
                FSB_Frequency_prop);
        return 0;
    }
    if (size == sizeof(uint64_t)) {
        frequency = *(uint64_t *) value;
        kprintf("EFI_CPU_Frequency: read %s value: %llu\n",
                FSB_Frequency_prop, frequency);
        if (!(10*Mega < frequency && frequency < 50*Giga)) {
            kprintf("EFI_Fake_MSR: value out of range\n");
            frequency = 0;
        }
    } else {
        kprintf("EFI_CPU_Frequency: unexpected size %d\n", size);
    }
    return frequency;
}

/*
 * Convert the cpu frequency info into a 'fake' MSR198h in Intel format
 */
static uint64_t
getFakeMSR(uint64_t frequency, uint64_t bFreq) {
    uint64_t fakeMSR = 0ull;
    uint64_t multi = 0;

    if (frequency == 0 || bFreq == 0)
        return 0;

    multi = frequency / (bFreq / 1000); // = multi*1000
    // divide by 1000, rounding up if it was x.75 or more
    // Example: 12900 will get rounded to 13150/1000 = 13
    //          but 12480 will be 12730/1000 = 12
    fakeMSR = (multi + 250) / 1000;
    fakeMSR <<= 40; // push multiplier into bits 44 to 40

    // If fractional part was within (0.25, 0.75), set N/2
    if ((multi % 1000 > 250) && (multi % 1000 < 750))
        fakeMSR |= (1ull << 46);

    return fakeMSR;
}

static uint64_t GetCpuMultiplayer()
{
    uint64_t prfsts;
    uint64_t cpuFreq;
    busFreq = 200 * Mega;
    // prfsts		= rdmsr64(AMD_COFVID_STS);
    cpuFreq = EFI_CPU_Frequency();
    prfsts = getFakeMSR(cpuFreq, busFreq);
    tscGranularity = bitfield(prfsts, 44, 40);

    return tscGranularity;
}

static uint64_t
Detect_FSB_frequency(void)
{
    //char **argv;
    // int voltage = 1;
    //voltage = atoi(argv[1]);

    //busFreq = EFI_FSB_frequency();
    int  res;
    uint64_t cpuMult;
    uint64_t cpuFreq;

    //If fsb boot parameter exists
    if (PE_parse_boot_argn("fsb", &res,sizeof(res))) return res * Mega;

    //Else try to autodetect
    cpuMult	= GetCpuMultiplayer();
    cpuFreq = EFI_CPU_Frequency();//Detect_CPU_Frequency();

    printf("FSB Detection: calculated Mult %lld, cpuFreq %lld \n", cpuMult, cpuFreq);

    if ((cpuMult == 0) || (cpuFreq == 0))
    {
        //  if (DetectFSB_NonClocked()) return DetectFSB_NonClocked() * Mega;
        return 200 * Mega;
    }

    uint64_t freq = cpuFreq / cpuMult;

    if (freq)
        return freq;

    return 200 * Mega;
}

/*
 * Initialize the various conversion factors needed by code referencing
 * the TSC.
 */
void
tsc_init(void)
{
	boolean_t	N_by_2_bus_ratio = FALSE;

    if (IsIntelCPU())
    {
	if (cpuid_vmm_present()) {
		kprintf("VMM vendor %u TSC frequency %u KHz bus frequency %u KHz\n",
				cpuid_vmm_info()->cpuid_vmm_family,
				cpuid_vmm_info()->cpuid_vmm_tsc_frequency,
				cpuid_vmm_info()->cpuid_vmm_bus_frequency);

		if (cpuid_vmm_info()->cpuid_vmm_tsc_frequency &&
			cpuid_vmm_info()->cpuid_vmm_bus_frequency) {

			busFreq = (uint64_t)cpuid_vmm_info()->cpuid_vmm_bus_frequency * kilo;
			busFCvtt2n = ((1 * Giga) << 32) / busFreq;
			busFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / busFCvtt2n;
			
			tscFreq = (uint64_t)cpuid_vmm_info()->cpuid_vmm_tsc_frequency * kilo;
			tscFCvtt2n = ((1 * Giga) << 32) / tscFreq;
			tscFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / tscFCvtt2n;
			
			tscGranularity = tscFreq / busFreq;
			
			bus2tsc = tmrCvt(busFCvtt2n, tscFCvtn2t);

			return;
		}
	}
    }

	/*
	 * Get the FSB frequency and conversion factors from EFI.
	 */
	busFreq = EFI_FSB_frequency();

    /* AnV - Adapted for manual boot ratio specification */
    /* Intel CPU Start */
    if (IsIntelCPU())
    {
        switch (cpuid_cpufamily())
        {
            case CPUFAMILY_INTEL_HASWELL:
            case CPUFAMILY_INTEL_IVYBRIDGE:
            case CPUFAMILY_INTEL_SANDYBRIDGE:
            case CPUFAMILY_INTEL_WESTMERE:
            case CPUFAMILY_INTEL_NEHALEM:
            {
                uint64_t msr_flex_ratio;
                uint64_t msr_platform_info;

                /* See if FLEX_RATIO is being used */
                msr_flex_ratio = rdmsr64(MSR_FLEX_RATIO);
                msr_platform_info = rdmsr64(MSR_PLATFORM_INFO);
                flex_ratio_min = (uint32_t)bitfield(msr_platform_info, 47, 40);
                flex_ratio_max = (uint32_t)bitfield(msr_platform_info, 15, 8);
                /* No BIOS-programed flex ratio. Use hardware max as default */
                tscGranularity = flex_ratio_max;
                if (msr_flex_ratio & bit(16))
                {
                    /* Flex Enabled: Use this MSR if less than max */
                    flex_ratio = (uint32_t)bitfield(msr_flex_ratio, 15, 8);
                    if (flex_ratio < flex_ratio_max)
                        tscGranularity = flex_ratio;
                }

                /* If EFI isn't configured correctly, use a constant 
                * value. See 6036811.
                */
                if (busFreq == 0)
                    busFreq = BASE_NHM_CLOCK_SOURCE;

                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; // Scale it back to normal
                }
                /****** Addon END ******/
                break;
            }

            /* AnV - Adapted from old code */
            case CPU_FAMILY_PENTIUM_4:
            {
                uint64_t cpuFreq;
                uint64_t prfsts;

                if (cpuid_info()->cpuid_model < 2)
                {
                    /* This uses the CPU frequency exported into EFI by the bootloader */
                    cpuFreq = EFI_CPU_Frequency();
                    prfsts  = getFakeMSR(cpuFreq, busFreq);
                    tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                    N_by_2_bus_ratio = (boolean_t)(prfsts & bit(46));
                }
                else if (cpuid_info()->cpuid_model == 2)
                {
                    prfsts      = rdmsr64(0x2C); // TODO: Add to header
                    tscGranularity  = bitfield(prfsts, 31, 24);
                }
                else
                {
                    prfsts = rdmsr64(IA32_PERF_STS);
                    tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                    N_by_2_bus_ratio = (prfsts & bit(46)) != 0;
                }
                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; // Scale it back to normal
                }
                /****** boot Argn END ******/
                break;
            }

            /* AnV - Adapted from old code */
            case CPU_FAMILY_PENTIUM_M:
            {
                uint64_t cpuFreq;
                uint64_t prfsts;

                if (cpuid_info()->cpuid_model >= 0xD)
                {
                    /* Pentium M or Core and above can use Apple method*/
                    prfsts = rdmsr64(IA32_PERF_STS);
                    tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                    N_by_2_bus_ratio = (prfsts & bit(46)) != 0;
                }
                else
                {
                    /* Other Pentium class CPU, use safest option */
                    /* This uses the CPU frequency exported into EFI by the bootloader */
                    cpuFreq = EFI_CPU_Frequency();
                    prfsts  = getFakeMSR(cpuFreq, busFreq);
                    tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                    N_by_2_bus_ratio = (boolean_t)(prfsts & bit(46));
                }
                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; // Scale it back to normal
                }
                /****** boot Argn END ******/
                break;
            }

            default:
            {
                uint64_t    prfsts;

                prfsts = rdmsr64(IA32_PERF_STS);
                tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                N_by_2_bus_ratio = (prfsts & bit(46)) != 0;
                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; // Scale it back to normal
                }
                /****** boot Argn END ******/
            }
        }
    }
    /* Intel CPU END */

    /* AMD CPU Start */
    else
    {
        switch (cpuid_info()->cpuid_family)
        {
            case 15: /*** AMD K8 Ext.Family 0x0f=15  ***/
            {
                uint64_t prfsts = 0;
                uint64_t cpuFreq = 0;
                busFreq = Detect_FSB_frequency();
                prfsts      = rdmsr64(AMD_PERF_STS);
                printf("rtclock_init: Athlon's MSR 0x%x \n", AMD_PERF_STS);

                cpuFreq = EFI_CPU_Frequency();
                prfsts = getFakeMSR(cpuFreq, busFreq);
                tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                N_by_2_bus_ratio = (prfsts & bit(0))!=0; // FIXME: This is experimental!

                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; /* Scale it back to normal */
                }
                /****** Addon END ******/
            }
            break;

            case 16: /*** AMD K10 Ext.Family 0x10=16  ***/
            {
                uint64_t prfsts = 0;
                uint64_t cpuFreq = 0;
                busFreq = Detect_FSB_frequency();
                prfsts      = rdmsr64(AMD_COFVID_STS);
                printf("rtclock_init: Phenom MSR 0x%x \n ", AMD_COFVID_STS);

                cpuFreq = EFI_CPU_Frequency();
                prfsts = getFakeMSR(cpuFreq, busFreq);
                tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                N_by_2_bus_ratio = (prfsts & bit(46))!= 0;

                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; /* Scale it back to normal */
                }
                /****** Addon END ******/
            }
            break;

            case 18:
            {
                uint64_t prfsts = 0;
                uint64_t cpuFreq = 0;
                busFreq = Detect_FSB_frequency();
                prfsts      = rdmsr64(AMD_COFVID_STS);
                printf("rtclock_init: Athlon's MSR 0x%x \n", AMD_PERF_STS);

                cpuFreq = EFI_CPU_Frequency();
                prfsts = getFakeMSR(cpuFreq, busFreq);
                tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                N_by_2_bus_ratio= (prfsts & bit(46))!=0;

                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; /* Scale it back to normal */
                }
                /****** Addon END ******/
            }
            break;

            case 20: /*** AMD APU Ext.Family 0x14=20 ***/
            {
                uint64_t prfsts = 0;
                uint64_t cpuFreq = 0;
                uint64_t cpu_mhz;

                busFreq = Detect_FSB_frequency();
                prfsts      = rdmsr64(AMD_COFVID_STS);
                printf("rtclock_init: Athlon's MSR 0x%x \n ", AMD_COFVID_STS);

                cpuFreq = EFI_CPU_Frequency();
                prfsts = getFakeMSR(cpuFreq, busFreq);
                tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                N_by_2_bus_ratio= (prfsts & bit(46))!=0;

                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; /* Scale it back to normal */
                }
                /****** Addon END ******/
                cpu_mhz = tscGranularity * EFI_FSB_frequency();
            }
            break;

            case 21: /*** AMD A8/A10 Ext.Family 0x15=21 ***/
            {
                uint64_t prfsts = 0;
                uint64_t cpuFreq = 0;
                uint64_t cpu_mhz;

                busFreq = Detect_FSB_frequency();
                prfsts      = rdmsr64(AMD_COFVID_STS);
                printf("rtclock_init: Athlon's MSR 0x%x \n", AMD_PERF_STS);

                cpuFreq = EFI_CPU_Frequency();
                prfsts = getFakeMSR(cpuFreq, busFreq);
                tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
                N_by_2_bus_ratio= (prfsts & bit(46))!=0;

                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; /* Scale it back to normal */
                }
                /****** Addon END ******/
                cpu_mhz = tscGranularity * EFI_FSB_frequency();
            }
            break;

            default:
            {
                uint64_t prfsts;
                uint64_t cpuFreq = 0;

                busFreq = EFI_FSB_frequency();

                cpuFreq = EFI_CPU_Frequency();
                prfsts = getFakeMSR(cpuFreq, busFreq);
                tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);

                /****** Addon boot Argn ******/
                if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
                {
                    if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
                    N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
                    if (N_by_2_bus_ratio) tscGranularity /= 10; // Scale it back to normal
                }
                /****** Addon END ******/
            }
        }
    }
    /* AMD CPU END */

	if (busFreq != 0) {
		busFCvtt2n = ((1 * Giga) << 32) / busFreq;
		busFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / busFCvtt2n;
	} else {
 		/* Instead of panicking, set a default FSB frequency */
	        /* AnV - Modded to 200 Mhz default... (for AMD) */
   		busFreq = 200*Mega;
  		kprintf("rtclock_init: Setting fsb to %u MHz\n", (uint32_t) (busFreq/Mega));
	}

	printf(" BUS: Frequency = %6d.%06dMHz, "
		"cvtt2n = %08X.%08X, cvtn2t = %08X.%08X\n",
		(uint32_t)(busFreq / Mega),
		(uint32_t)(busFreq % Mega), 
		(uint32_t)(busFCvtt2n >> 32), (uint32_t)busFCvtt2n,
		(uint32_t)(busFCvtn2t >> 32), (uint32_t)busFCvtn2t);

	/*
	 * Get the TSC increment.  The TSC is incremented by this
	 * on every bus tick.  Calculate the TSC conversion factors
	 * to and from nano-seconds.
	 * The tsc granularity is also called the "bus ratio". If the N/2 bit
	 * is set this indicates the bus ration is 0.5 more than this - i.e.
	 * that the true bus ratio is (2*tscGranularity + 1)/2. If we cannot
	 * determine the TSC conversion, assume it ticks at the bus frequency.
	 */
	if (tscGranularity == 0)
		tscGranularity = 1;

	if (N_by_2_bus_ratio)
		tscFCvtt2n = busFCvtt2n * 2 / (1 + 2*tscGranularity);
	else
		tscFCvtt2n = busFCvtt2n / tscGranularity;

	tscFreq = ((1 * Giga)  << 32) / tscFCvtt2n;
	tscFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / tscFCvtt2n;

	printf(" TSC: Frequency = %6d.%06dMHz, "
		"cvtt2n = %08X.%08X, cvtn2t = %08X.%08X, gran = %lld%s\n",
		(uint32_t)(tscFreq / Mega),
		(uint32_t)(tscFreq % Mega), 
		(uint32_t)(tscFCvtt2n >> 32), (uint32_t)tscFCvtt2n,
		(uint32_t)(tscFCvtn2t >> 32), (uint32_t)tscFCvtn2t,
		tscGranularity, N_by_2_bus_ratio ? " (N/2)" : "");

	/*
	 * Calculate conversion from BUS to TSC
	 */
	bus2tsc = tmrCvt(busFCvtt2n, tscFCvtn2t);
}

void
tsc_get_info(tscInfo_t *info)
{
	info->busFCvtt2n     = busFCvtt2n;
	info->busFCvtn2t     = busFCvtn2t;
	info->tscFreq        = tscFreq;
	info->tscFCvtt2n     = tscFCvtt2n;
	info->tscFCvtn2t     = tscFCvtn2t;
	info->tscGranularity = tscGranularity;
	info->bus2tsc        = bus2tsc;
	info->busFreq        = busFreq;
	info->flex_ratio     = flex_ratio;
	info->flex_ratio_min = flex_ratio_min;
	info->flex_ratio_max = flex_ratio_max;
}
