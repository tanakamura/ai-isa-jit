#ifndef AI_ISA_JIT_H
#define AI_ISA_JIT_H

#include <CL/cl.h>
#include "ai-isa-jit/ai-isa-jit-emit.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AIISA_InnerELF {
    unsigned char *data;
    int size;

    int seg4_phoff;

    int text_shoff;
    int data_shoff;
    int symtab_shoff;
    int strtab_shoff;
};

/*
 * 必要なこと
 *  outer ELF
 *   inner ELF のサイズ変わるので
 *   .text shdr のサイズ変える
 *   .comment の offset 変える
 *  inner ELF
 */
struct AIISA_Program {
    size_t size;
    unsigned char *cl_binary;
    cl_program orig_prog;

    int outer_text_shdr_offset;
    int outer_comment_shdr_offset;

    struct AIISA_InnerELF inner;
};

int aiisa_build_binary_from_cl(struct AIISA_Program *prog,
                               cl_context context,
                               cl_device_id dev,
                               const char *path);
void aiisa_fini_binary(struct AIISA_Program *prog);

void aiisa_replace_text(struct AIISA_Program *prog,
                        struct AIISA_CodeBuffer *code);

#ifdef __cplusplus
}
#endif


#endif
