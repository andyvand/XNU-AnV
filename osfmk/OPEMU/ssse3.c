/*
             .d8888b.   .d8888b.   .d8888b.  8888888888 .d8888b.  
            d88P  Y88b d88P  Y88b d88P  Y88b 888       d88P  Y88b 
            Y88b.      Y88b.      Y88b.      888            .d88P 
             "Y888b.    "Y888b.    "Y888b.   8888888       8888"  
                "Y88b.     "Y88b.     "Y88b. 888            "Y8b. 
                  "888       "888       "888 888       888    888 
            Y88b  d88P Y88b  d88P Y88b  d88P 888       Y88b  d88P 
             "Y8888P"   "Y8888P"   "Y8888P"  8888888888 "Y8888P"  
*/

#include "opemu.h"
#include "ssse3_priv.h"

/**
 * Load operands from memory/register, store in obj.
 * @return: 0 if success
 */
int ssse3_grab_operands(ssse3_t *ssse3_obj)
{
    const unsigned char *bytep = (const unsigned char *)ud_insn_hex((ud_t *)&ssse3_obj->op_obj->ud_obj);
    uint8_t modrm = *bytep;
    uint8_t base = modrm & 0x7;
    uint8_t mod = (modrm & 0xC0) >> 6;
    uint64_t reg_sel[8];
    
    if (ssse3_obj->op_obj->state_flavor == SAVEDSTATE_64)
    {
        reg_sel[0] = ssse3_obj->op_obj->state64->rax;
        reg_sel[1] = ssse3_obj->op_obj->state64->rcx;
        reg_sel[2] = ssse3_obj->op_obj->state64->rdx;
        reg_sel[3] = ssse3_obj->op_obj->state64->rbx;
        reg_sel[4] = ssse3_obj->op_obj->state64->isf.rsp;
        reg_sel[5] = ssse3_obj->op_obj->state64->rbp;
        reg_sel[6] = ssse3_obj->op_obj->state64->rsi;
        reg_sel[7] = ssse3_obj->op_obj->state64->rdi;
    } else {
        reg_sel[0] = ssse3_obj->op_obj->state32->eax;
        reg_sel[1] = ssse3_obj->op_obj->state32->ecx;
        reg_sel[2] = ssse3_obj->op_obj->state32->edx;
        reg_sel[3] = ssse3_obj->op_obj->state32->ebx;
        reg_sel[4] = ssse3_obj->op_obj->state32->uesp;
        reg_sel[5] = ssse3_obj->op_obj->state32->ebp;
        reg_sel[6] = ssse3_obj->op_obj->state32->esi;
        reg_sel[7] = ssse3_obj->op_obj->state32->edi;
    }

	if (ssse3_obj->islegacy) {
		_store_mmx (ssse3_obj->udo_dst->base - UD_R_MM0, &ssse3_obj->dst.u64[0]);
		if (ssse3_obj->udo_src->type == UD_OP_REG) {
			_store_mmx (ssse3_obj->udo_src->base - UD_R_MM0, &ssse3_obj->src.u64[0]);
		} else {
			// m64 load
			int64_t disp = 0;
			uint8_t disp_size = ssse3_obj->udo_src->offset;
			uint64_t address;
            int8_t add8;
            int32_t add32;
			
            // disp8 / disp32
            if (ssse3_obj->udo_src->scale)
            {
                if (mod == 0)
                {
                    address = reg_sel[base];
                } else if (mod == 1) {
                    bytep++;
                    
                    add8 = *bytep;
                    address = reg_sel[base] + add8;
                } else {
                    bytep++;
                    
                    add32 = *(int32_t *)bytep;
                    address = reg_sel[base] + add32;
                }
            } else {
                if (retrieve_reg (ssse3_obj->op_obj->state,
                                  ssse3_obj->udo_src->base, &address) != 0)
                    return -1;
                
                switch (disp_size) {
                    case 8: disp = ssse3_obj->udo_src->lval.sbyte; break;
                    case 16: disp = ssse3_obj->udo_src->lval.sword; break;
                    case 32: disp = ssse3_obj->udo_src->lval.sdword; break;
                    case 64: disp = ssse3_obj->udo_src->lval.sqword; break;
                }
                
                address += disp;
            }

			if (ssse3_obj->op_obj->ring0)
				ssse3_obj->src.u64[0] = * ((uint64_t*) (address));
			else copyin (address, (char*) &ssse3_obj->src.u64[0], 8);
		}
	} else {
		_store_xmm (ssse3_obj->udo_dst->base - UD_R_XMM0, &ssse3_obj->dst.ui);
		if (ssse3_obj->udo_src->type == UD_OP_REG) {
			_store_xmm (ssse3_obj->udo_src->base - UD_R_XMM0, &ssse3_obj->src.ui);
		} else {
			// m128 load
			int64_t disp = 0;
			uint8_t disp_size = ssse3_obj->udo_src->offset;
			uint64_t address;
            int8_t add8;
            int32_t add32;

            // disp8 / disp32
            if (ssse3_obj->udo_src->scale)
            {
                if (mod == 0)
                {
                    address = reg_sel[base];
                } else if (mod == 1) {
                    bytep++;
                    
                    add8 = *bytep;
                    address = reg_sel[base] + add8;
                } else {
                    bytep++;
                    
                    add32 = *(int32_t *)bytep;
                    address = reg_sel[base] + add32;
                }
            } else {
                if (retrieve_reg (ssse3_obj->op_obj->state,
                                  ssse3_obj->udo_src->base, &address) != 0)
                    return -1;
                
                switch (disp_size) {
                    case 8: disp = ssse3_obj->udo_src->lval.sbyte; break;
                    case 16: disp = ssse3_obj->udo_src->lval.sword; break;
                    case 32: disp = ssse3_obj->udo_src->lval.sdword; break;
                    case 64: disp = ssse3_obj->udo_src->lval.sqword; break;
                }
                
                address += disp;
            }

			if (ssse3_obj->op_obj->ring0)
				ssse3_obj->src.ui = * ((__uint128_t*) (address));
			else copyin (address, (char*) &ssse3_obj->src.ui, 16);
		}
	}

    return 0;
}

/**
 * Store operands from obj to memory/register.
 * @return: 0 if success
 */
int ssse3_commit_results(const ssse3_t *ssse3_obj)
{
	if (ssse3_obj->islegacy) {
		_load_mmx (ssse3_obj->udo_dst->base - UD_R_MM0, (void*) &ssse3_obj->res.u64[0]);
	} else {
		_load_xmm (ssse3_obj->udo_dst->base - UD_R_XMM0, (void*) &ssse3_obj->res.ui);
	}

    return 0;
}

/**
 * Main function for the ssse3 portion. Check if the offending
 * opcode is part of the ssse3 instruction set, if not, quit early.
 * if so, then we build the appropriate context, and jump to the right function.
 * @param op_obj: opemu object
 * @return: zero if an instruction was emulated properly
 */
int op_sse3x_run(const op_t *op_obj)
{
	ssse3_t ssse3_obj;
	ssse3_obj.op_obj = op_obj;
	const uint32_t mnemonic = ud_insn_mnemonic(ssse3_obj.op_obj->ud_obj);
	ssse3_func opf;

	switch (mnemonic) {
    /* SSE 4.2 */
    case UD_Ipcmpestri:	opf = pcmpestri; goto common;
	case UD_Ipcmpestrm:	opf = pcmpestrm; goto common;
	case UD_Ipcmpistri:	opf = pcmpistri; goto common;
    case UD_Ipcmpistrm: opf = pcmpistrm; goto common;
    case UD_Ipcmpgtq:	opf = pcmpgtq;   goto common;

    /* SSSE3 */
	case UD_Ipsignb:	opf = psignb;	goto common;
	case UD_Ipsignw:	opf = psignw;	goto common;
	case UD_Ipsignd:	opf = psignd;	goto common;

	case UD_Ipabsb:		opf = pabsb;	goto common;
	case UD_Ipabsw:		opf = pabsw;	goto common;
	case UD_Ipabsd:		opf = pabsd;	goto common;

	case UD_Ipalignr:	opf = palignr;	goto common;

	case UD_Ipshufb:	opf = pshufb;	goto common;

	case UD_Ipmulhrsw:	opf = pmulhrsw;	goto common;

	case UD_Ipmaddubsw:	opf = pmaddubsw;	goto common;

	case UD_Iphsubw:	opf = phsubw;	goto common;
	case UD_Iphsubd:	opf = phsubd;	goto common;

	case UD_Iphsubsw:	opf = phsubsw;	goto common;

	case UD_Iphaddw:	opf = phaddw;	goto common;
	case UD_Iphaddd:	opf = phaddd;	goto common;

	case UD_Iphaddsw:	opf = phaddsw;	goto common;

common:
	ssse3_obj.udo_src = ud_insn_opr (op_obj->ud_obj, 1);
	ssse3_obj.udo_dst = ud_insn_opr (op_obj->ud_obj, 0);
	ssse3_obj.udo_imm = ud_insn_opr (op_obj->ud_obj, 2);

	// run some sanity checks,
	if (ssse3_obj.udo_dst->type != UD_OP_REG)
        break;

	if ((ssse3_obj.udo_src->type != UD_OP_REG)
		&& (ssse3_obj.udo_src->type != UD_OP_MEM))
        break;

	// i'd like to know if this instruction is legacy mmx
	if ((ssse3_obj.udo_dst->base >= UD_R_MM0)
		&& (ssse3_obj.udo_dst->base <= UD_R_MM7)) {
		ssse3_obj.islegacy = 1;
	} else ssse3_obj.islegacy = 0;
	
	if (ssse3_grab_operands(&ssse3_obj) != 0)
        break;

	opf(&ssse3_obj);

	if (ssse3_commit_results(&ssse3_obj))
        break;

    return 0;

	default:
        break;
	}

    // Only reached if bad
	return -1;
}

/**
 * Negate/zero/preserve
 */
void psignb (ssse3_t *this)
{
	if (this->islegacy)
    {
        this->res.m64[0] = ssp_sign_pi8_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_sign_epi8_REF(this->dst.i, this->src.i);
    }
}

/**
 * Negate/zero/preserve
 */
void psignw (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_sign_pi16_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_sign_epi16_REF(this->dst.i, this->src.i);
    }
}

/**
 * Negate/zero/preserve
 */
void psignd (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_sign_pi32_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_sign_epi32_REF(this->dst.i, this->src.i);
    }
}

/**
 * Absolute value
 */
void pabsb (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_abs_pi8_REF(this->src.m64[0]);
    } else {
        this->res.i = ssp_abs_epi8_REF(this->src.i);
    }
}

/**
 * Absolute value
 */
void pabsw (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_abs_pi16_REF(this->src.m64[0]);
    } else {
        this->res.i = ssp_abs_epi16_REF(this->src.i);
    }
}

/**
 * Absolute value
 */
void pabsd (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_abs_pi32_REF(this->src.m64[0]);
    } else {
        this->res.i = ssp_abs_epi32_REF(this->src.i);
    }
}

const int palignr_getimm128(ssse3_t *this)
{
    int rv = 0;
    const unsigned char *bytep = (const unsigned char *)ud_insn_hex((ud_t *)&this->op_obj->ud_obj);
    uint8_t modrm = bytep[4];

    if (modrm < 0x40)
    {
        rv = (int)bytep[5];
    } else if (modrm < 0x80) {
        rv = (int)bytep[6];
    } else if (modrm < 0xC0) {
        rv = (int)bytep[9];
    } else {
        rv = (int)bytep[5];
    }

    return (const int)rv;
}

const int palignr_getimm64(ssse3_t *this)
{
    int rv = 0;
    const unsigned char *bytep = (const unsigned char *)ud_insn_hex((ud_t *)&this->op_obj->ud_obj);
    uint8_t modrm = bytep[3];

    if (modrm < 0x40)
    {
        rv = (int)bytep[4];
    } else if (modrm < 0x80) {
        rv = (int)bytep[5];
    } else if (modrm < 0xC0) {
        rv = (int)bytep[8];
    } else {
        rv = (int)bytep[4];
    }
    
    return (const int)rv;
}

/**
 * Concatenate and shift
 */ 
void palignr (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_alignr_pi8_REF(this->dst.m64[0], this->src.m64[0], palignr_getimm64(this));
    } else {
        this->res.i = ssp_alignr_epi8_REF(this->dst.i, this->src.i, palignr_getimm128(this  ));
    }
}

/**
 * Shuffle Bytes
 */
void pshufb (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_shuffle_pi8_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_shuffle_epi8_REF(this->dst.i, this->src.i);
    }
}

/**
 * Multiply high with round and scale
 */
void pmulhrsw (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_mulhrs_pi16_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_mulhrs_epi16_REF(this->dst.i, this->src.i);
    }
}

/**
 * Multiply and add
 */
void pmaddubsw (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_maddubs_pi16_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_maddubs_epi16_REF(this->dst.i, this->src.i);
    }
}

/**
 * Horizontal subtract
 */
void phsubw (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_hsub_pi16_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_hsub_epi16_REF(this->dst.i, this->src.i);
    }
}

/**
 * Horizontal subtract
 */
void phsubd (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_hsub_pi32_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_hsub_epi32_REF(this->dst.i, this->src.i);
    }
}

/**
 * Horizontal subtract and saturate
 */
void phsubsw (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_hsubs_pi16_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_hsubs_epi16_REF(this->dst.i, this->src.i);
    }
}

/**
 * Horizontal add
 */
void phaddw (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_hadd_pi16_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_hadd_epi16_REF(this->dst.i, this->src.i);
    }
}

/**
 * Horizontal add
 */
void phaddd (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_hadd_pi32_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_hadd_epi32_REF(this->dst.i, this->src.i);
    }
}

/**
 * Horizontal add and saturate
 */
void phaddsw (ssse3_t *this)
{
    if (this->islegacy)
    {
        this->res.m64[0] = ssp_hadds_pi16_REF(this->dst.m64[0], this->src.m64[0]);
    } else {
        this->res.i = ssp_hadds_epi16_REF(this->dst.i, this->src.i);
    }
}

