#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <elf.h>
#include <string.h>
#include <stdio.h>

#include "ai-isa-jit/ai-isa-jit.h"

/*
 * セグメント4
 * セクション6
 *  に入ってる
 * 
 * セクションヘッダ:
 *   [番] 名前              タイプ          アドレス Off    サイズ ES Flg Lk Inf Al
 *   [ 0]                   NULL            00000000 000000 000000 00      0   0  0
 *   [ 1] .shstrtab         STRTAB          00000000 0000fc 000028 00      0   0  0
 *   [ 2] .text             PROGBITS        00000000 0004d8 0008a8 00      0   0  0
 *   [ 3] .data             PROGBITS        00000000 000d80 001280 1280      0   0  0
 *   [ 4] .symtab           SYMTAB          00000000 002000 000060 10      5   1  0
 *   [ 5] .strtab           STRTAB          00000000 002060 00001b 00      0   0  0
 *   [ 6] .text             PROGBITS        00000000 00262f 000068 00      0   0  0  (これ)
 *   [ 7] .data             PROGBITS        00000000 002697 001280 1280      0   0  0
 *   [ 8] .symtab           SYMTAB          00000000 003917 000060 10      9   1  0
 *   [ 9] .strtab           STRTAB          00000000 003977 00001b 00      0   0  0
 *
 *
 * やることは、
 *  - .data, .symtab, .strtab の開始位置をずらす
 *  - セグメント4のサイズを増やす
 *  - .text のサイズを拡張する
 *
 */
static void
resize_inner_text(struct AIISA_InnerELF *inner, int new_section_size)
{
    Elf32_Shdr *text = (Elf32_Shdr*)(inner->data + inner->text_shoff);
    int dsize = new_section_size - text->sh_size;
    unsigned char *new_buffer;
    int new_size;
    Elf32_Shdr *data, *symtab, *strtab;
    Elf32_Phdr *seg4;

    if (dsize <= 0) {
        return;
    }

    new_size = inner->size + dsize;
    new_buffer = malloc(new_size);

    text->sh_size += dsize;

    memcpy(new_buffer, inner->data, inner->size);

    text = (Elf32_Shdr*)(new_buffer + inner->text_shoff);
    data = (Elf32_Shdr*)(new_buffer + inner->data_shoff);
    symtab = (Elf32_Shdr*)(new_buffer + inner->symtab_shoff);
    strtab = (Elf32_Shdr*)(new_buffer + inner->strtab_shoff);
    seg4 = (Elf32_Phdr*)(new_buffer + inner->seg4_phoff);

    memcpy(new_buffer + data->sh_offset + dsize,
           inner->data + data->sh_offset, data->sh_size);
    memcpy(new_buffer + symtab->sh_offset + dsize,
           inner->data + symtab->sh_offset, symtab->sh_size);
    memcpy(new_buffer + strtab->sh_offset + dsize,
           inner->data + strtab->sh_offset, strtab->sh_size);

    data->sh_offset += dsize;
    symtab->sh_offset += dsize;
    strtab->sh_offset += dsize;

    seg4->p_memsz += dsize;
    seg4->p_filesz += dsize;

    free(inner->data);

    inner->data = new_buffer;
    inner->size = new_size;

}


static void
resize_outer_text(struct AIISA_Program *prog, int new_section_size)
{
    Elf32_Shdr *text = (Elf32_Shdr*)(prog->cl_binary + prog->outer_text_shdr_offset);
    Elf32_Shdr *comment;
    int dsize = new_section_size - text->sh_size;
    unsigned char *new_buffer;
    int new_size;

    if (dsize <= 0) {
        return;
    }

    new_size = prog->size + dsize;
    new_buffer = malloc(new_size);

    memcpy(new_buffer, prog->cl_binary, prog->size);

    text = (Elf32_Shdr*)(new_buffer + prog->outer_text_shdr_offset);
    comment = (Elf32_Shdr*)(new_buffer + prog->outer_comment_shdr_offset);

    memcpy(new_buffer + comment->sh_offset + dsize,
           prog->cl_binary + comment->sh_offset,
           comment->sh_size);


    text->sh_size = new_section_size;
    comment->sh_offset += dsize;

    free(prog->cl_binary);
    prog->cl_binary = new_buffer;
    prog->size = new_size;
}

static int
read_file(unsigned char **ret_data,
          size_t *ret_size,
          const char *path)
{
    struct stat st_buf;
    unsigned char *buffer;
    int fd;
    size_t size;
    int r;

    r = stat(path, &st_buf);
    if (r < 0) {
        return -1;
    }

    size = st_buf.st_size;
    buffer=(unsigned char*)malloc(size);
    fd = open(path, O_RDONLY);
    read(fd, buffer, size);
    close(fd);

    *ret_data = buffer;
    *ret_size = size;

    return 0;
}

static int
write_file(char *path,
           void *data,
           size_t sz)
{
    int fd;

    fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    write(fd, data, sz);
    close(fd);

    return 0;
}


int 
aiisa_build_binary(struct AIISA_Program *prog, const char *path)
{
    int r;
    unsigned char *buffer;
    size_t size;

    r = read_file(&buffer, &size, path);
    if (r < 0) {
        prog->size = 0;
        prog->cl_binary = 0;
        return r;
    }

    prog->size = size;
    prog->cl_binary = buffer;

    return 0;
}

static void
setup_inner(struct AIISA_Program *prog)
{
    unsigned char *base = prog->cl_binary;
    Elf32_Ehdr *eh = (Elf32_Ehdr*)base;
    int sh_entsize = eh->e_shentsize;
    unsigned char *text_contents;
    int sh_cur = eh->e_shoff;

    sh_cur += sh_entsize * 5;
    prog->outer_text_shdr_offset = sh_cur;
    prog->outer_comment_shdr_offset = sh_cur + sh_entsize;

    {
        Elf32_Shdr *sh = (Elf32_Shdr*)(base + prog->outer_text_shdr_offset);
        Elf32_Ehdr *inner_ehdr;
        int inner_sh_cur;
        int inner_ph_cur;

        text_contents = base + sh->sh_offset;

        write_file("out2.elf", text_contents, sh->sh_size);


        prog->inner.size = sh->sh_size;
        prog->inner.data = (unsigned char*)malloc(sh->sh_size);
        memcpy(prog->inner.data, text_contents, sh->sh_size);
        text_contents = prog->inner.data;
        inner_ehdr = (Elf32_Ehdr*)text_contents;


        /*        
         *   [ 6] .text             PROGBITS        00000000 00262f 000068 00      0   0  0  (これ)
         *   [ 7] .data             PROGBITS        00000000 002697 001280 1280      0   0  0
         *   [ 8] .symtab           SYMTAB          00000000 003917 000060 10      9   1  0
         *   [ 9] .strtab           STRTAB          00000000 003977 00001b 00      0   0  0
         */
        inner_sh_cur = inner_ehdr->e_shoff;
        inner_sh_cur += inner_ehdr->e_shentsize * 6;

        prog->inner.text_shoff = inner_sh_cur + inner_ehdr->e_shentsize * 0;
        prog->inner.data_shoff = inner_sh_cur + inner_ehdr->e_shentsize * 1;
        prog->inner.symtab_shoff = inner_sh_cur + inner_ehdr->e_shentsize * 2;
        prog->inner.strtab_shoff = inner_sh_cur + inner_ehdr->e_shentsize * 3;;

        /*
         *  00
         *  01
         *  02     .text .data .symtab .strtab
         *  03
         *  04     .text .data .symtab .strtab
         */

        inner_ph_cur = inner_ehdr->e_phoff;
        inner_ph_cur += inner_ehdr->e_phentsize * 4;

        prog->inner.seg4_phoff = inner_ph_cur;
    }
}


int
aiisa_build_binary_from_cl(struct AIISA_Program *prog_ret,
                           cl_context context,
                           cl_device_id dev,
                           const char *path)
{
    cl_int err;
    const char *src[1];
    size_t src_sz[1];
    size_t binary_sz[1];
    size_t ret_sz;
    cl_program prog;
    char *src_text;
    size_t src_text_len;
    int r;
    unsigned char *binary;

    prog_ret->size = 0;
    prog_ret->cl_binary = 0;

    r = read_file((unsigned char**)&src_text, &src_text_len, path);
    if (r < 0) {
        return r;
    }

    src[0] = src_text;
    src_sz[0] = src_text_len;
    prog = clCreateProgramWithSource(context, 1, src, src_sz, &err);

    free(src_text);

    if (err != CL_SUCCESS) {
        puts("cre pro");
        return -1;
    }

    err = clBuildProgram(prog, 1, &dev, "-fbin-exe -fno-bin-llvmir -fno-bin-source -fno-bin-amdil", NULL, NULL);

    if (err != CL_SUCCESS) {
        char log[1024];
        size_t ret;
        puts("build pro");
        err = clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, 1024, log, &ret);
        printf("%d %s\n", err, log);
        return -1;
    }

    clGetProgramInfo(prog, CL_PROGRAM_BINARY_SIZES, sizeof(binary_sz), binary_sz, &ret_sz);

    binary = malloc(binary_sz[0]);
    clGetProgramInfo(prog, CL_PROGRAM_BINARIES, sizeof(binary), &binary, &ret_sz);

    {
        write_file("out1.elf", binary, binary_sz[0]);
    }

    prog_ret->size = binary_sz[0];
    prog_ret->cl_binary = binary;

    clReleaseProgram(prog);
    //prog_ret->orig_prog = prog;

    setup_inner(prog_ret);

    return 0;
}


void
aiisa_replace_text(struct AIISA_Program *prog,
                   struct AIISA_CodeBuffer *code)
{
    Elf32_Shdr *text;
    Elf32_Shdr *inner_text;
    resize_inner_text(&prog->inner, code->cur * 4);

    inner_text = (Elf32_Shdr*)(prog->inner.data + prog->inner.text_shoff);
    memcpy(prog->inner.data + inner_text->sh_offset, code->buffer, code->cur * 4);

    resize_outer_text(prog, prog->inner.size);

    text = (Elf32_Shdr*)(prog->cl_binary + prog->outer_text_shdr_offset);
    memcpy(prog->cl_binary + text->sh_offset, prog->inner.data, prog->inner.size);

    {
        write_file("out3.elf", prog->cl_binary, prog->size);
    }
    {
        write_file("out4.elf", prog->inner.data, prog->inner.size);
    }
}


void
aiisa_fini_binary(struct AIISA_Program *prog)
{
    free(prog->cl_binary);
}

