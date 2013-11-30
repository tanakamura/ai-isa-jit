#ifndef AI_ISA_JIT_ELF_H
#define AI_ISA_JIT_ELF_H

#ifdef _WIN32

typedef unsigned int Elf32_Word;
typedef unsigned int Elf32_Off;
typedef unsigned int Elf32_Addr;
typedef unsigned int Elf32_Size;
typedef unsigned short Elf32_Half;

typedef unsigned __int64 Elf64_Addr;
typedef unsigned int Elf64_Word;
typedef unsigned __int64 Elf64_Off;
typedef unsigned __int64 Elf64_Addr;
typedef unsigned __int64 Elf64_Size;
typedef unsigned short Elf64_Half;

#define EI_NIDENT 16

typedef struct {
    unsigned char   e_ident[EI_NIDENT];
    Elf32_Half      e_type;
    Elf32_Half      e_machine;
    Elf32_Word      e_version;
    Elf32_Addr      e_entry;
    Elf32_Off       e_phoff;
    Elf32_Off       e_shoff;
    Elf32_Word      e_flags;
    Elf32_Half      e_ehsize;
    Elf32_Half      e_phentsize;
    Elf32_Half      e_phnum;
    Elf32_Half      e_shentsize;
    Elf32_Half      e_shnum;
    Elf32_Half      e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    unsigned char   e_ident[EI_NIDENT];
    Elf64_Half      e_type;
    Elf64_Half      e_machine;
    Elf64_Word      e_version;
    Elf64_Addr      e_entry;
    Elf64_Off       e_phoff;
    Elf64_Off       e_shoff;
    Elf64_Word      e_flags;
    Elf64_Half      e_ehsize;
    Elf64_Half      e_phentsize;
    Elf64_Half      e_phnum;
    Elf64_Half      e_shentsize;
    Elf64_Half      e_shnum;
    Elf64_Half      e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    Elf32_Word      sh_name;
    Elf32_Word      sh_type;
    Elf32_Word      sh_flags;
    Elf32_Addr      sh_addr;
    Elf32_Off       sh_offset;
    Elf32_Size      sh_size;
    Elf32_Word      sh_link;
    Elf32_Word      sh_info;
    Elf32_Size      sh_addralign;
    Elf32_Size      sh_entsize;
} Elf32_Shdr;

typedef struct {
    Elf32_Word      p_type;
    Elf32_Off       p_offset;
    Elf32_Addr      p_vaddr;
    Elf32_Addr      p_paddr;
    Elf32_Size      p_filesz;
    Elf32_Size      p_memsz;
    Elf32_Word      p_flags;
    Elf32_Size      p_align;
} Elf32_Phdr;

#else
#include <elf.h>
#endif

#endif
