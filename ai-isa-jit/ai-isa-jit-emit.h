#ifndef AI_ISA_JIT_EMIT_H
#define AI_ISA_JIT_EMIT_H

#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AIISA_CodeBuffer {
    int cur;
    int size;

    uint32_t *buffer;
};

void aiisa_code_buffer_init(struct AIISA_CodeBuffer *buf);
void aiisa_code_buffer_reset(struct AIISA_CodeBuffer *buf);
void aiisa_code_buffer_fini(struct AIISA_CodeBuffer *buf);

void aiisa_emit4(struct AIISA_CodeBuffer *buffer, uint32_t val);

//  buffer_store_byte  v1, v0, s[4:7], 0 offen glc            // 00000010: E0605000 80010100
//                                                                         ^left    ^right
void aiisa_emit4x2(struct AIISA_CodeBuffer *buffer, uint32_t val_left, uint32_t val_right);

static __inline void
aiisa_emit_sopp(struct AIISA_CodeBuffer *buffer, unsigned int opc, unsigned int imm16)
{
    uint32_t val = 0;
    val |= 0xbf800000;
    val |= opc << 16;
    val |= (imm16&0xffff);

    aiisa_emit4(buffer, val);
}

static __inline void
aiisa_emit_sopk(struct AIISA_CodeBuffer *buffer, unsigned int opc,
                unsigned int sdst, unsigned int imm16)
{
    uint32_t val = 0;
    val |= 0xb0000000;
    val |= opc << 23;
    val |= sdst << 16;
    val |= imm16;

    aiisa_emit4(buffer, val);
}

static __inline void
aiisa_emit_sopc(struct AIISA_CodeBuffer *buffer, unsigned int opc,
                unsigned int ssrc1, unsigned int ssrc0)
{
    uint32_t val = 0;
    val |= 0x17e << 23;
    val |= opc << 16;
    val |= ssrc1 << 8;
    val |= ssrc0 << 8;

    aiisa_emit4(buffer, val);
}

static __inline void
aiisa_emit_smrd_regoff(struct AIISA_CodeBuffer *buffer,
                       unsigned int opc,
                       unsigned int sdst,
                       unsigned int sbase,
                       unsigned int off_reg)
{
    /* sdst = [sbase + off_reg] */

    uint32_t val = 0;
    val |= 0xc0000000;
    val |= opc << 22;
    val |= sdst << 15;
    val |= sbase << 8;
    val |= off_reg;

    aiisa_emit4(buffer, val);

    assert((sbase & 1) == 0);
}

static __inline void
aiisa_emit_smrd_immoff(struct AIISA_CodeBuffer *buffer,
                       unsigned int opc,
                       unsigned int sdst,
                       unsigned int sbase,
                       unsigned int imm_off)
{
    /* sdst = [sbase + imm_off] */
    uint32_t val = 0;
    val |= 0xc0000100;
    val |= opc << 22;
    val |= sdst << 15;
    val |= sbase << 8;
    val |= imm_off;

    aiisa_emit4(buffer, val);
    assert((sbase & 1) == 0);
}

static __inline void
aiisa_emit_vop1(struct AIISA_CodeBuffer *buffer,
                unsigned int vdst,
                unsigned int opc,
                unsigned int src0)

{
    uint32_t val = 0;
    val |= 0x7e000000;
    val |= vdst << 17;
    val |= opc << 9;
    val |= src0;

    aiisa_emit4(buffer, val);
}

static __inline void
aiisa_emit_mubuf(struct AIISA_CodeBuffer *buffer,
                 unsigned int opc,
                 unsigned int flags,
                 unsigned int offset,
                 unsigned int soffset,
                 unsigned int flags2,
                 unsigned int srsrc,
                 unsigned int vdata,
                 unsigned int vaddr)
{
    uint32_t val_left = 0, val_right = 0;

    val_left |= 0xe0000000;
    val_left |= opc<<18;
    val_left |= flags<<12;
    val_left |= offset;

    val_right |= soffset << 24;
    val_right |= flags2 << 22;
    val_right |= srsrc << (16-2);
    val_right |= vdata << 8;
    val_right |= vaddr << 0;

    aiisa_emit4x2(buffer, val_left, val_right);

    assert((srsrc & 3) == 0);
}

static __inline void
aiisa_emit_mtbuf(struct AIISA_CodeBuffer *buffer,
                 unsigned int nfmt,
                 unsigned int dfmt,
                 unsigned int opc,
                 unsigned int flags,
                 unsigned int offset,
                 unsigned int soffset,
                 unsigned int flags2,
                 unsigned int srsrc,
                 unsigned int vdata,
                 unsigned int vaddr)
{
    uint32_t val_left = 0, val_right = 0;

    val_left |= 0xe8000000;
    val_left |= nfmt << 23;
    val_left |= dfmt << 19;
    val_left |= opc<<16;
    val_left |= flags<<12;
    val_left |= offset;

    val_right |= soffset << 24;
    val_right |= flags2 << 22;
    val_right |= srsrc << (16-2);
    val_right |= vdata << 8;
    val_right |= vaddr << 0;

    aiisa_emit4x2(buffer, val_left, val_right);

    assert((srsrc & 3) == 0);
}

static __inline void
aiisa_s_endpgm(struct AIISA_CodeBuffer *buffer)
{
    aiisa_emit_sopp(buffer, 1, 0);
}
static __inline void
aiisa_s_waitcnt(struct AIISA_CodeBuffer *buffer, int cnt)
{
    aiisa_emit_sopp(buffer, 12, cnt);
}
static __inline void
aiisa_s_cbranch_scc0(struct AIISA_CodeBuffer *buffer, int simm)
{
    aiisa_emit_sopp(buffer, 4, simm);
}

static __inline void
aiisa_s_cbranch_scc1(struct AIISA_CodeBuffer *buffer, int simm)
{
    aiisa_emit_sopp(buffer, 5, simm);
}


static __inline void
aiisa_v_mov_b32(struct AIISA_CodeBuffer *buffer,
                unsigned int vdst,
                unsigned int src)
{
    aiisa_emit_vop1(buffer, vdst, 1, src);
}

static __inline void
aiisa_s_addk_i32(struct AIISA_CodeBuffer *buffer,
                 unsigned int sdst,
                 unsigned int imm)
{
    aiisa_emit_sopk(buffer, 15, sdst, imm);
}

static __inline void
aiisa_s_movk_i32(struct AIISA_CodeBuffer *buffer,
                 unsigned int sdst,
                 unsigned int imm)
{
    aiisa_emit_sopk(buffer, 0, sdst, imm);
}
static __inline void
aiisa_s_cmpk_lt_i32(struct AIISA_CodeBuffer *buffer,
                    unsigned int sdst,
                    unsigned int imm)
{
    aiisa_emit_sopk(buffer, 7, sdst, imm);
}
static __inline void
aiisa_s_cmpk_le_i32(struct AIISA_CodeBuffer *buffer,
                    unsigned int sdst,
                    unsigned int imm)
{
    aiisa_emit_sopk(buffer, 8, sdst, imm);
}



static __inline void
aiisa_s_cmp_gt_i32(struct AIISA_CodeBuffer *buffer,
                   unsigned int ssrc1,
                   unsigned int ssrc0)
{
    aiisa_emit_sopc(buffer, 2, ssrc1, ssrc0);
}

static __inline void
aiisa_s_cmp_eq_i32(struct AIISA_CodeBuffer *buffer,
                   unsigned int ssrc1,
                   unsigned int ssrc0)
{
    aiisa_emit_sopc(buffer, 0, ssrc1, ssrc0);
}
static __inline void
aiisa_s_cmp_le_i32(struct AIISA_CodeBuffer *buffer,
                   unsigned int ssrc1,
                   unsigned int ssrc0)
{
    aiisa_emit_sopc(buffer, 5, ssrc1, ssrc0);
}

static __inline void
aiisa_s_buffer_load_dword_regoff(struct AIISA_CodeBuffer *buffer,
                                 unsigned int sdst,
                                 unsigned int sbase,
                                 unsigned int off_reg)
{
    aiisa_emit_smrd_regoff(buffer, 8, sdst, sbase, off_reg);
}

static __inline void
aiisa_s_buffer_load_dword_immoff(struct AIISA_CodeBuffer *buffer,
                                 unsigned int sdst,
                                 unsigned int sbase,
                                 unsigned int imm_off)
{
    aiisa_emit_smrd_immoff(buffer, 8, sdst, sbase, imm_off);
}

static __inline void
aiisa_tbuffer_store_format_x(struct AIISA_CodeBuffer *buffer,
                             unsigned int nfmt,
                             unsigned int dfmt,
                             unsigned int flags,
                             unsigned int offset,
                             unsigned int soffset,
                             unsigned int flags2,
                             unsigned int srsrc,
                             unsigned int vdata,
                             unsigned int vaddr)
{
    aiisa_emit_mtbuf(buffer, nfmt, dfmt, 4,
                     flags, offset, soffset, flags2, srsrc, vdata, vaddr);
}

#define LGKMCNT(cnt) (((cnt)<<8)|0x7f)

#define S(reg) (reg)
#define V(reg) (reg)
#define ZERO 128
#define VCCZ 251
#define EXECZ 252
#define SCC 253
                          
#define FLAG_OFFEN (1<<0)
#define FLAG_IDXEN (1<<1)
#define FLAG_GLC (1<<2)
#define FLAG_ADDR64 (1<<3)
#define FLAG_LDS (1<<4)

#define FLAG2_SLC (1<<0)
#define FLAG2_TFE (1<<1)

#define DFMT_32 (4)
#define DFMT_32_32_32_32 (14)

#define NFMT_FLOAT (7)

#ifdef __cplusplus
}
#endif

#endif
