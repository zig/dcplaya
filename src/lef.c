/**
 * @file    lef.c
 * @author  Benjamin Gerard <ben@sashipa.com>
 * @author  Vincent Penne
 * @author  Dan Potter
 * @brief   ELF library loader - Based on elf.c from KallistiOS 1.1.5 
 *
 * @version $Id: lef.c,v 1.16 2003-04-20 02:23:20 vincentp Exp $
 */

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <kos/fs.h>

#include "dcplaya/config.h"
#include "lef.h"
#include "sysdebug.h"
#include "gzip.h"

// VP : define this to have full debug logging informations
//#define FULL_DEBUG

extern symbol_t main_symtab[];
extern int main_symtab_size;

static int verbose = 0;

static char * section_str(int type)
{
  switch(type & SHT_TYPE) {
  case SHT_NULL:          return "Inactive..........";
  case SHT_PROGBITS:      return "Program-code/data.";
  case SHT_SYMTAB:        return "Full-symbol-table.";
  case SHT_STRTAB:        return "String-table......";
  case SHT_RELA:          return "Relocation-table..";
  case SHT_HASH:          return "Sym-tab-hashtable.";
  case SHT_DYNAMIC:       return "Dynamic-link-info.";
  case SHT_NOTE:          return "Notes.............";
  case SHT_NOBITS:        return "NoBits............";
  case SHT_REL:           return "Relocation table..";
  case SHT_SHLIB:         return "Sharedlib.Invalid.";
  case SHT_DYNSYM:        return "Dynamic-only-symb.";
  case SHT_INIT_ARRAY:    return "Init functions....";
  case SHT_FINI_ARRAY:    return "Term functions....";
  case SHT_PREINIT_ARRAY: return "Pre-init functions";
  case SHT_GROUP:         return "Group.............";
  case SHT_SYMTAB_SHNDX:  return "Symbol table index";
  default:                return "??????????????????";
  }
}

/* Display stringtable */
static void display_stringtable(char *stringtab, int stringsize, int n)
{
  char *s;
  int i;
  
  SDDEBUG("*************** STRING TABLE %d **************\n", n);
  for (i=0, s=stringtab; s<stringtab+stringsize; ++i) {
    SDDEBUG("[%4d] [%08x] [%s]\n",
	    i, s-stringtab, s);
    while (*s++);
  }
  SDDEBUG("**********************************************\n");
}

static int bind_chr(unsigned int bind) {
  char bind_tab[] = { 'L', 'G', 'W', '.',
		      '.', '.', '.', '.',
		      '.', '.', '.', '.',
		      '.', 'P', 'P', 'P' };
  return (bind < 16) ? bind_tab[bind] : '?';
}

static void display_symb(struct lef_sym_t *symb, char *strtab, int tag)
{
  int lvl = tag ? sysdbg_info : sysdbg_debug;

  char *type_str[] = { "UDF", "OBJ", "FCT", "SEC",
		       "FIL", "???", "???", "???",
                       "???", "???", "???", "???",
                       "???", "???", "???", "???"};
  char section[8];

  lvl = lvl;

  switch (symb->shndx) {
  case SHN_ABS:
    strcpy(section,"ABS");
    break;
  case SHN_COMMON:
    strcpy(section,"COM");
    break;
  case SHN_UNDEF:
    strcpy(section,"UND");
    break;
  case SHN_XINDEX:
    strcpy(section,"IDX");
    break;

  default:
    if (symb->shndx<SHN_LORESERVE) {
      sprintf(section,"#%02x",symb->shndx); 
    } else if (symb->shndx >= SHN_LOPROC && symb->shndx <= SHN_HIPROC) {
      strcpy(section,"PRO");
    } else if (symb->shndx >= SHN_LOOS && symb->shndx <= SHN_HIOS) {
      strcpy(section,"OS.");
    } else if (symb->shndx >= SHN_LORESERVE && symb->shndx <= SHN_HIRESERVE) {
      strcpy(section,"RES");
    } else {
      strcpy(section,"???");
    }
  }
  section[3] = ' ';
  section[4] = bind_chr(ELF32_ST_BIND(symb->info));
  section[5] = 0;

  SDMSG(lvl, "%s[%s] val:[%08x] sz:[%06x] %s [%s]\n",
	tag ? "SYM: " : "",
	type_str[ELF32_ST_TYPE(symb->info)],
	symb->value,
	symb->size,
	section,
	symb->name + strtab);
}

static void display_section(int i, struct lef_shdr_t *shdrs, int tag)
{
  SDMSG(tag ? sysdbg_info : sysdbg_debug,
	"%s#%02X "
	"@%08x of:%06x sz:%06x al:%02x "
	"fl:%08x li:%08x in:%08x "
	"%02X-[%s] [%s]\n",
	tag ? "SEC: " : "", i,
	shdrs->addr, shdrs->offset, shdrs->size, shdrs->addralign,
	shdrs->flags, shdrs->link, shdrs->info,
	shdrs->type, section_str(shdrs->type),
	shdrs->name);
}

static void display_sections(struct lef_hdr_t *hdr, struct lef_shdr_t *shdrs,
			     int tag)
{
  int i;
  int lvl = tag ? sysdbg_info : sysdbg_debug;

  lvl = lvl;
  SDMSG(lvl, "--------------------------------------------------\n");
  SDMSG(lvl, "SECTIONS [%d]\n", hdr->shnum);
  SDMSG(lvl, "--------------------------------------------------\n");
  for (i=0; i<hdr->shnum; ++i) {
    display_section(i, shdrs+i, tag);
  }
  SDMSG(lvl, "--------------------------------------------------\n");
}

static void display_symbtable(struct lef_sym_t * stab,
			      struct lef_shdr_t *shdrs, char *stringtab,
			      int tag)
{
  int lvl = tag ? sysdbg_info : sysdbg_debug;
  int stabsz, j;

  lvl = lvl;
  stabsz = shdrs->size / sizeof(struct lef_sym_t);
  SDMSG(lvl, "--------------------------------------------------\n");
  SDMSG(lvl, "SYMBOL-TABLE [%s] [%s] (%d)\n",
	section_str(shdrs->type), shdrs->name, stabsz);
  SDMSG(lvl, "--------------------------------------------------\n");
  for (j=0; j<stabsz; ++j) {
    display_symb(stab + j, stringtab, tag);
  }
  SDMSG(lvl, "--------------------------------------------------\n");
}

/* Finds a given symbol in a relocated ELF symbol table */
static void * find_main_sym(char *name) {
  int i;

  if (!*name) return 0;

  for (i=0; i<main_symtab_size; i++) {
    if (!strcmp((char*)main_symtab[i].name, name))
      return main_symtab[i].addr;
  }

  return 0;
}


/* Finds a given symbol in a relocated ELF symbol table */
static int find_sym(char *name, struct lef_sym_t* table, int tablelen) {
  int i;

  for (i=0; i<tablelen; i++) {
    if (!strcmp((char*)table[i].name, name))
      return i;
  }
  return -1;
}

static int loadfile(int fd, char * img, int sz)
{
  //const int block = (16<<10);
  const int block = (1<<10);

  while (sz) {
    int n = sz;
    if (n > block) {
      n = block;
    }
    if (fs_read(fd, img, n) != n) {
      SDERROR("Read error\n");
      return -1;
    }
    SDDEBUG(".");
    sz -= n;
    img += n;
  }
  SDDEBUG("\nLoad completed\n");
  return 0;
}
/* Pass in a file descriptor from the virtual file system, and the
   result will be NULL if the file cannot be loaded, or a pointer to
   the loaded and relocated executable otherwise. The second variable
   will be set to the entry point. */
/* There's a lot of shit in here that's not documented or very poorly
   documented by Intel.. I hope that this works for future compilers. */
lef_prog_t *lef_load(const char * fname)
{
  lef_prog_t		*out = 0;
  char			*img = 0, *imgout = 0, *buf = 0; //, * mmap = 0;
  int			sz, i, j, sect; 
  struct lef_hdr_t	*hdr = 0;
  struct lef_shdr_t	*shdrs = 0, *symtabhdr = 0;
  struct lef_sym_t	*symtab = 0;
  int			symtabsize;
  struct lef_rela_t	*reltab = 0;
  int			reltabsize;
  char			*stringtab = 0;
  int                   stringsize;
  char                  *sectionname = 0;
  int                   errors = 0;
  int                   warnings = 0;
  //  int                   flen = 0;
  //  int                   inflate_len = 0;
  int                   lef_size;

  const int align_lef=256;

  SDDEBUG(">>%s(%s)\n", __FUNCTION__, fname);
  SDINDENT;

  img = gzip_load(fname, &sz);
  if (!img) {
    goto error;
  }

  /* Header is at the front */
  hdr = (struct lef_hdr_t *)(img+0);
  if (hdr->ident[0] != 0x7f || strncmp(hdr->ident+1, "ELF", 3)) {
    SDERROR("Not a valid ELF\n");
    hdr->ident[4] = 0;
    SDERROR("hdr->ident is %d/%s\n",
	    hdr->ident[0], hdr->ident+1);
    goto error;
  }
  if (hdr->ident[4] != 1 || hdr->ident[5] != 1) {
    SDERROR("Invalid architecture flags\n");
    goto error;
  }
  if (hdr->machine != 0x2a) {
    SDERROR("Invalid machine flags\n");
    goto error;
  }

  shdrs = (struct lef_shdr_t *)(img + hdr->shoff);
  sectionname = (char*)(img + shdrs[hdr->shstrndx].offset);

  /* Reloc section name names */
  for (i=0; i<hdr->shnum; ++i) {
    shdrs[i].name = (uint32)(sectionname + shdrs[i].name);
  }

  /* Display string tables */
  /*
  for (i=0; i<hdr->shnum; i++) {
    if (shdrs[i].type == SHT_STRTAB) {
      display_stringtable((char*)(img + shdrs[i].offset), shdrs[i].size, i);
    }
  }
  */

  /* Locate the string table; SH elf files ought to have
     two string tables, one for section names and one for object
     string names. We'll look for the latter. */
  stringtab = NULL;
  {
    int cnt = 0;
    for (i=0; i<hdr->shnum; i++) {
      if (shdrs[i].type == SHT_STRTAB && i != hdr->shstrndx) {
	++cnt;
	stringtab = (char*)(img + shdrs[i].offset);
	stringsize = shdrs[i].size;
      }
    }
    if (cnt > 1) {
      SDERROR("Too many string table [%d]\n", cnt);
      goto error;
    }
  }
  if (!stringtab) {
    SDERROR("No string table\n");
    goto error;
  }

  /* Display symbol tables */
  /*
  if (verbose) {
    for (i=0; i<hdr->shnum; i++) {
      if (shdrs[i].type != SHT_SYMTAB && shdrs[i].type != SHT_DYNSYM) {
	continue;
      }
      display_symbtable((struct lef_sym_t *)(img + shdrs[i].offset),
			shdrs+i, stringtab,0);
    }
  }
  */

  /* Locate the symbol table */
  {
    int cnt = 0;

    symtabhdr = 0;
    for (i=0; i<hdr->shnum; i++) {
      if (shdrs[i].type == SHT_SYMTAB || shdrs[i].type == SHT_DYNSYM) {
	++cnt;
	symtabhdr = shdrs+i;
      }
    }
    if (cnt > 1) {
      SDERROR("Too many symbol tables [%d]\n", cnt);
      goto error;
    }
  }

  if (!symtabhdr) {
    SDERROR("No symbol table\n");
    goto error;
  }

  if (symtabhdr->entsize != sizeof(struct lef_sym_t)) {
    SDERROR("Symbol table entry size (%d) != struct lef_sym_t (%d)\n",
	    symtabhdr->entsize, sizeof(struct lef_sym_t));
    goto error;
  }
  symtab = (struct lef_sym_t *)(img + symtabhdr->offset);
  symtabsize = symtabhdr->size / sizeof(struct lef_sym_t);
  if (symtabhdr->size % sizeof(struct lef_sym_t)) {
    SDERROR("Not multiple of symbol struct\n");
    goto error;
  }

  /* Build the final memory image */
  sz = 0;
  for (i=0; i<hdr->shnum; i++) {
    if (shdrs[i].addralign > 1) {
      /* $$$ Align this section or next one ? */
      sz = (sz + shdrs[i].addralign - 1) & ~(shdrs[i].addralign-1);
    }
    shdrs[i].addr = sz;
    if (shdrs[i].flags & SHF_ALLOC) {
      sz += shdrs[i].size;
    }
  }

  /* Alloc final memory image */
  lef_size = sizeof(lef_prog_t) + sz + align_lef - 1;
  SDDEBUG("lef image size : %d\n", lef_size);
  out = calloc(1, lef_size);
  if (!out) {
    SDERROR("Out image alloc error\n");
    goto error;
  }
  out->ref_count = 1;
  out->data = (void *)(((unsigned int)&out[1] + align_lef - 1) & -align_lef);
  out->size = sz;
  imgout = out->data;

  /* Set section real addres, and copy */
  if (verbose) {
    SDDEBUG( "------------------------------------------------\n");
    SDDEBUG( "CREATE IMAGE\n");
    SDDEBUG( "------------------------------------------------\n");
  }
  for (i=0; i<hdr->shnum; i++) {
    shdrs[i].addr += (unsigned int)imgout;
    if (shdrs[i].flags & SHF_ALLOC) {
      if (shdrs[i].type == SHT_NOBITS) {
	memset((void *)shdrs[i].addr, 0, shdrs[i].size);
	if (verbose) {
	  SDDEBUG( "CLEARED ");
	}
      }
      else {
	memcpy((void *)shdrs[i].addr, img+shdrs[i].offset, shdrs[i].size);
	if (verbose) {
	  SDDEBUG( "COPIED  ");
	}
      }
    } else {
      //section_copied &= ~(1<<i);
      if (verbose) {
	SDDEBUG(   "SKIPPED ");
      }
    }
    if (verbose) {
      display_section(i, shdrs+i, 0);
    }
  }

  /* Relocate symtab entries for quick access */
  for (i=0; i<symtabsize; i++) {
    symtab[i].name = (uint32)(stringtab + symtab[i].name);
    if (symtab[i].shndx < hdr->shnum) {
      symtab[i].value += shdrs[symtab[i].shndx].addr;
    }
  }

  if (verbose) {
    display_sections(hdr,shdrs,1);
    display_symbtable(symtab, symtabhdr, 0, 1);
  }

  /* Process the relocations */
  reltab = 0;
  for (i=0; i<hdr->shnum; i++) {
    int info = shdrs[i].info;
    int link = shdrs[i].link;

    if (shdrs[i].type != SHT_RELA) {
#ifdef FULL_DEBUG
      SDDEBUG("Section [%s] skipped\n", section_str(shdrs[i].type));
#endif
      continue;
    }
    reltab = (struct lef_rela_t *)(img + shdrs[i].offset);

    if ( shdrs[i].entsize != sizeof(struct lef_rela_t)) {
      SDERROR("Relocation entry size (%d) != lef_rela_t (%d)\n",
	      shdrs[i].entsize,sizeof(struct lef_rela_t));
      ++errors;
      continue;
    }
    reltabsize = shdrs[i].size / sizeof(struct lef_rela_t);
    if (shdrs[i].size % sizeof(struct lef_rela_t)) {
      SDERROR("Weird relatif table size : not multiple : skip\n");
      ++errors;
      continue;
    }

    /*
    if (!(shdrs[i].flags & SHF_INFO_LINK)) {
      dbglog(DBG_ERROR, "!! " __FUNCTION__
	     " : Section header info is not an section idx!!\n");
      display_section(sect, shdrs+i, 0);
      ++warnings;
      continue;
    }
    */

    sect = shdrs[i].info;
    if (sect >= hdr->shnum) {
      SDERROR("Bad section %x\n",sect);
      ++errors;
    }
    
    if (! (shdrs[sect].flags & SHF_ALLOC)) {
      /* This is no more considerate as an Error because it could be debug
	 info. A Warning should be OK. */
      SDWARNING("Can't reloc uncopied section : ");
      display_section(sect, shdrs+sect, 0);
      ++warnings;
      continue;
    }

    /*
link = section header index of associated symbol table
info = section header index of section to which reloc applies
    */
   
    if (verbose) {
      SDINFO( "------------------------------------------------\n");
      SDINFO( "RELOCATON "); display_section(i,shdrs+i,0); 
      SDINFO( "SECTION   "); display_section(info,shdrs+info,0); 
      SDINFO( "SYMBOL    "); display_section(link,shdrs+link,0); 
      SDINFO( "------------------------------------------------\n");
    }

    for (j=0; j<reltabsize; j++) {
      int sym;
      void * main_addr;
      char * name;

      if (ELF32_R_TYPE(reltab[j].info) != R_SH_DIR32) {
	SDERROR("Unknown RELA type %02x\n",
		ELF32_R_TYPE(reltab[j].info));
	++errors;
	continue;
      }

      /* Get symbol idx */
      sym = ELF32_R_SYM(reltab[j].info);
      if (sym >= symtabsize) {
	SDERROR("Symbol idx out of range [%d >= %d]\n",
		sym, symtabsize);
	++errors;
	continue;
      }

      /* Check symbol section */
      if (symtab[sym].shndx >= hdr->shnum
	  /*&& symtab[sym].shndx != SHN_COMMON*/) {
	SDERROR("Invalid symbol section index : %04x\n", symtab[sym].shndx);
	++errors;
	continue;
      }

      /* Get name */
      name = (char *)symtab[sym].name;
      main_addr = 0;

      /* Check for undefined symbol */ 
      if (symtab[sym].shndx == SHN_UNDEF) {
	/* Undefined symbol must be searched in the main symbol table */
	main_addr = find_main_sym(name);
	if (!main_addr) {
	  /*SDERROR("Undefined symbol [%s]\n", name);*/
	  printf("Undefined symbol [%s]\n", name);
	  display_symb(symtab + sym, 0, 0);
	  ++errors;
	  continue;	  
	}
      }

      switch(symtab[sym].info & 15) {
      case STT_NOTYPE:
      case STT_FUNC:
      case STT_SECTION:
      case STT_OBJECT:

	break;
      case STT_FILE:
      default:
	SDWARNING("What to do with this symbol ?\n");
	display_symb(symtab + sym, 0, 0);
	++warnings;
	continue;
      }

      {
	uint32 * addr;
	uint32 off;
	uint32 add;
	uint32 rel;
	uint32 symbolbase;

	addr = (uint32 *)(shdrs[sect].addr + reltab[j].offset);

	if (main_addr) {
	  symbolbase  = (uint32)main_addr;
	} else {
	  symbolbase  = symtab[sym].value;
	  if (symtab[sym].shndx == SHN_COMMON) {
	    symbolbase += shdrs[info].addr;
	  }
	}
	add = symbolbase + reltab[j].addend;
	off = *addr;
	rel = off + add;

	/* Test address to relocate */
	if ((uint32)addr<(uint32)imgout || (uint32)addr>=(uint32)imgout+sz) {
	  SDERROR("Relocation patch @%p out of range [%p-%p]\n",
		  addr,imgout,imgout+sz);
	  ++errors;
	  continue;
	}
	/* Test relocated address */
	if (!main_addr &&
	    ((uint32)rel<(uint32)imgout || (uint32)rel>=(uint32)imgout+sz)) {
	  SDERROR("Relocated @%p out of range [%p-%p]\n",
		  rel,imgout,imgout+sz);
	  ++errors;
	  continue;
	}

	/* Could named symbol have offset ??? */
/* 	if (*name && off) { */
/* 	  dbglog(DBG_WARNING, "!! " __FUNCTION__ */
/* 		 " : Named symbol with offset\n"); */
/* 	  ++warnings; */
/* 	} */

	if (verbose) {
	  SDINFO("%c %c @%08x [=%08x] <= [%08x] [+%08x] [%s]\n" ,
		 main_addr ? 'M' : 'L',
		 bind_chr(ELF32_ST_BIND(symtab[sym].info)),
		 addr, off, 
		 rel, add,  name);
	}

	*addr = rel;

      }
    }
  }
  if (reltab == NULL) {
    SDWARNING("No RELA sections found.\n");
    ++warnings;
  }

  /* Find/Call c-tor init : */
  if (!errors) {
/*     SDDEBUG("Searching ctor sections\n"); */
    for (i=0; i<hdr->shnum; i++) {
      if (!strcmp((char *)shdrs[i].name, ".ctors")) {
/* 	SDDEBUG( "CTOR "); display_section(i,shdrs+i,0);  */
	for (j=0; j<shdrs[i].size; j+=4) {
	  uint32 * rout = *(uint32 **)shdrs[i].addr; 
	  SDDEBUG("CTOR -->%p [%p [%08x]]\n", shdrs[i].addr, rout, *rout);
	  ((void (*)(void))rout)();
	}
      }
    }
  }
  
  /* Look for the kernel negotiation symbols and deal with that */
  {
    int mainsym /*, getsvcsym, notifysym*/;

    mainsym = find_sym("_lef_main", symtab, symtabsize);
    if (mainsym < 0) {
      SDERROR( "ELF contains no _lef_main\n");
      goto error;
    }
    out->main = (int (*)(int,char**))(symtab[mainsym].value);
  }

  if (errors) {
    goto error;
  }

  if (warnings) {
    SDWARNING(" Warnings:%d\n", warnings);
  }

  SDDEBUG("image [@%08x, @%08x, %08x] entry [%08x]\n",
	  out->data, imgout, out->size, out->main);
  goto end;

 error:
  if (out) {
    free(out);
    out = 0;
  }
  SDINFO(" Errors:%d Warnings:%d\n", errors, warnings);
  SDINFO(" : Failed\n");

 end:
/*   if (fd) {     */
/*     fs_close(fd); */
/*   } */
  if (img) {
    free(img);
  }
  if (buf && buf != img) {
    free(buf);
  }
  SDUNINDENT;
  return out;
}

/* Free a loaded ELF program */
void lef_free(lef_prog_t *prog) {
/*  SDDEBUG("%s(%p)\n", __FUNCTION__, prog);
  SDINDENT; */
  if (!prog) {
    SDERROR("[%s] : null pointer\n", __FUNCTION__);
  } else if (--prog->ref_count<=0) {
    if (prog->ref_count < 0) {
      SDWARNING("[%s] : minus refcount [%d]\n", __FUNCTION__, prog->ref_count);
    }
    free(prog);
    SDDEBUG("[%s] : removed\n", __FUNCTION__);
  } else {
/*    SDDEBUG("remainding lef instance: %d\n", prog->ref_count); */
  }
/*  SDUNINDENT; */
}
