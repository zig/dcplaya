/* KallistiOS 1.1.5

os/elf.h
(c)2000-2001 Dan Potter

$Id: lef.h,v 1.1.1.1 2002-08-26 14:15:00 ben Exp $

*/

#ifndef __LEF_H
#define __LEF_H

#include <arch/types.h>
#include <sys/queue.h>

  typedef struct {

    void       * addr;
    char         type;
    const char * name;

  } symbol_t;






/* ELF file header */
struct lef_hdr_t {
  unsigned char	ident[16];	/* For elf32-shl, 0x7f+"ELF"+1+1 */
  uint16		type;		/* 0x02 for ET_EXEC */
  uint16		machine;	/* 0x2a for elf32-shl */
  uint32		version;
  uint32		entry;		/* Entry point */
  uint32		phoff;		/* Program header offset */
  uint32		shoff;		/* Section header offset */
  uint32		flags;		/* Processor flags */
  uint16		ehsize;		/* ELF header size in bytes */
  uint16		phentsize;	/* Program header entry size */
  uint16		phnum;		/* Program header entry count */
  uint16		shentsize;	/* Section header entry size */
  uint16		shnum;		/* Section header entry count */
  uint16		shstrndx;	/* String table section index */
};

/* Section header types */
#define SHT_NULL	0		/* Inactive */
#define SHT_PROGBITS	1		/* Program code/data */
#define SHT_SYMTAB	2		/* Full symbol table */
#define SHT_STRTAB	3		/* String table */
#define SHT_RELA	4		/* Relocation table */
#define SHT_HASH	5		/* Sym tab hashtable */
#define SHT_DYNAMIC	6		/* Dynamic linking info */
#define SHT_NOTE	7		/* Notes */
#define SHT_NOBITS	8		/* Occupies no space in the file */
#define SHT_REL		9		/* Relocation table */
#define SHT_SHLIB	10		/* Invalid.. hehe */
#define SHT_DYNSYM	11		/* Dynamic-only sym tab */
#define SHT_LOPROC	0x70000000	/* Processor specific */
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000	/* Program specific */
#define SHT_HIUSER	0xffffffff

#define SHT_INIT_ARRAY  14             /* Array of pointers to
					   initialization functions */
#define SHT_FINI_ARRAY    15       /* Array of pointers to termination
				      functions */

#define SHT_PREINIT_ARRAY 0x10     /* Array of pointers to functions that are
				      invoked before all other initialization
				      functions */
#define SHT_GROUP         17       /* This section defines a section group */ 
#define SHT_SYMTAB_SHNDX  18       /* Associated with a section of type
				      SHT_SYMTAB and is required if any of
				      the section header indexes referenced
				      by that symbol table contain the
				      escape value SHN_XINDEX */

#define SHF_WRITE	1		/* Writable data */
#define SHF_ALLOC	2		/* Resident */
#define SHF_EXECINSTR	4		/* Executable instructions */
#define SHF_MASKPROC	0xf0000000	/* Processor specific */

#define SHF_MERGE            0x10 /* Section may be merged to eliminate
				     duplication */
#define SHF_STRINGS          0x20 /* Null-terminated character strings */
#define SHF_INFO_LINK        0x40 /* sh_info field of this section header
				     holds a section header table index */
#define SHF_LINK_ORDER       0x80 /* Special ordering requirements for link
				     editors */
#define SHF_OS_NONCONFORMING 0x100 /* section requires special OS-specific
				      processing */
#define SHF_GROUP            0x200 /* Section is a member of a section group */


/* Section header */
struct lef_shdr_t {
  uint32		name;		/* Index into string table */
  uint32		type;		/* See constants above */
  uint32		flags;
  uint32		addr;		/* In-memory offset */
  uint32		offset;		/* On-disk offset */
  uint32		size;		/* Size (if SHT_NOBITS, zero file len */
  uint32		link;		/* See below */
  uint32		info;		/* See below */
  uint32		addralign;	/* Alignment constraints */
  uint32		entsize;	/* Fixed-size table entry sizes */
};
/* Link and info fields:

switch (sh_type) {
case SHT_DYNAMIC:
link = section header index of the string table used by
the entries in this section
info = 0
case SHT_HASH:
ilnk = section header index of the string table to which
this info applies
info = 0
case SHT_REL, SHT_RELA:
link = section header index of associated symbol table
info = section header index of section to which reloc applies
case SHT_SYMTAB, SHT_DYNSYM:
link = section header index of associated string table
info = one greater than the symbol table index of the last
local symbol (binding STB_LOCAL)
}

*/

/* Symbol table entry */
/* Symbol type */
#define STT_NOTYPE  0
#define STT_OBJECT  1 /* Associated with data object (variable, array...) */
#define STT_FUNC    2 /* Associated with function or other executable code */
#define STT_SECTION 3 /* For relocation. Should have STB_LOCAL binding. */
#define STT_FILE    4 /* Gives the name of the source file associated with
			 the object file. A file symbol has STB_LOCAL binding,
			 its section index is SHN_ABS, and it precedes the
			 other STB_LOCAL symbols for the file, if it is
			 present. */

#define ELF32_ST_BIND(i)   ((i)>>4)
#define ELF32_ST_TYPE(i)   ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

/* Symbol Binding */
#define STB_LOCAL  0   /* Static */
#define STB_GLOBAL 1   /* Extern */
#define STB_WEAK   2   /* Extern lower precedence */
#define STB_LOPROC 13  /* Inclusive range are reserved for */
#define STB_HIPROC 15  /* processor-specific semantics.    */


/* Symbol section index */
#define SHN_LORESERVE 0xff00 /* Specifies the range of reserved */
#define SHN_HIRESERVE 0xffff /* index */
#define SHN_LOOS      0xff20 /* Specifies the range of os */
#define SHN_HIOS      0xff3f /* specific semantics */
#define SHN_LOPROC 0xff00    /* Inclusive range are reserved for */
#define SHN_HIPROC 0xff1f    /* processor-specific semantics.    */
#define SHN_ABS 0xfff1    /* The symbol has an absolute value that will not
			     change because of relocation. */

#define SHN_COMMON 0xfff2 /* The symbol labels a common block that has not
			     yet been allocated. The symbol's value gives
			     alignment constraints, similar to a section's
			     sh_addralign member. The link editor will allocate
			     the storage for the symbol at an address that is
			     a multiple of st_value. The symbol's size tells
			     how many bytes are required. */
#define SHN_UNDEF 0      /* This section table index means the symbol is
			    undefined. When the link editor combines this
			    object file with another that defines the
			    indicated symbol, this file's references to the
			    symbol will be linked to the actual definition. */

#define SHN_XINDEX 0xffff /* This value is an escape value. It indicates
			     that the actual section header index is too
			     large to fit in the containing field and is to
			     be found in another location
			     (specific to the structure where it appears). */
struct lef_sym_t {
  uint32        name;		/* Index into stringtab */
  uint32	value;
  uint32	size;
  uint8		info;		/* type == info & 0x0f */
  uint8		other;		
  uint16	shndx;		/* Section index */
};

/* Relocation-A Entries */
#define R_SH_DIR32		1
struct lef_rela_t {
  uint32	offset;		/* Offset within section */
  uint32	info;		/* Symbol and type */
  int32		addend;		/* "A" constant */
};
#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((uint8)(i))


/* Kernel-specific definition of a loaded ELF binary */
typedef struct lef_prog {
  void	 *data;		/* Data block containing the program */
  uint32 size;		/* Memory image size */
  int	 argc;		/* Arguments */
  char	 **argv;
  int	 (*ko_main)(int argc, char **argv);	/* Program entry point */
  //int	(*ko_notify)(uint32 event);		/* Event notification */
  int    ref_count;                              /* Reference counter */
  char	 fn[256];				/* Program filename */

  //	LIST_ENTRY(lef_prog)	pslist;
} lef_prog_t;

/* For use only for debugging */
//extern LIST_HEAD(pslist, lef_prog) ps_list;

/* Load an ELF binary and return the relevant data in an lef_prog_t structure. */
lef_prog_t *lef_load(uint32 fd);

/* Free a loaded ELF program */
void lef_free(lef_prog_t *prog);

#endif	/* __LEF_H */

