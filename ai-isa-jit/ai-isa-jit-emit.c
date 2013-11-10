#include <stdlib.h>
#include <stdio.h>
#include "ai-isa-jit/ai-isa-jit-emit.h"


void
aiisa_code_buffer_init(struct AIISA_CodeBuffer *buf)
{
    buf->cur = 0;
    buf->size = 1024;
    buf->buffer = malloc(buf->size * sizeof(uint32_t));
}

void
aiisa_code_buffer_reset(struct AIISA_CodeBuffer *buf)
{
    buf->cur = 0;
}

void
aiisa_code_buffer_fini(struct AIISA_CodeBuffer *buf)
{
    free(buf->buffer);
    buf->buffer = NULL;
    buf->size = 0;
}

void
aiisa_emit4(struct AIISA_CodeBuffer *buf, uint32_t val)
{
    if (buf->cur == buf->size) {
        buf->size *= 2;
        buf->buffer = realloc(buf->buffer, buf->size * sizeof(uint32_t));
    }

    //printf("%08x: 0x%08x\n", buf->cur*4, val);
    buf->buffer[buf->cur++] = val;
}

void
aiisa_emit4x2(struct AIISA_CodeBuffer *buf, uint32_t val0, uint32_t val1)
{
    if ((buf->cur+1) >= buf->size) {
        buf->size *= 2;
        buf->buffer = realloc(buf->buffer, buf->size * sizeof(uint32_t));
    }

    //printf("%08x: 0x%08x 0x%08x\n", (int)(buf->cur*4), (int)val0, (int)val1);

    buf->buffer[buf->cur++] = val0;
    buf->buffer[buf->cur++] = val1;
}
