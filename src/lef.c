/**
 * @file    lef.c
 * @brief   ELF loader - Based on elf.c from KallistiOS 1.1.5 
 * @author  Benjamin Gerard <ben@sashipa.com>
 * @author  Vincent Penne
 * @author  Dan Potter
 *
 * $Id: lef.c,v 1.3 2002-09-04 18:54:11 ben Exp $
 */

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <kos/fs.h>

#include "lef.h"

extern symbol_t main_symtab[];
extern int main_symtab_size;

static int verbose = 0;

static char * section_str(int type)
{
  switch(type & 0x0F) {
  case SHT_NULL:       return "Inactive.........";
  case SHT_PROGBITS:   return "Program-code/data";
  case SHT_SYMTAB:     return "Full-symbol-table";
  case SHT_STRTAB:     return "String-table.....";
  case SHT_RELA:       return "Relocation-table.";
  case SHT_HASH:       return "Sym-tab-hashtable";
  case SHT_DYNAMIC:    return "Dynamic-link-info";
  case SHT_NOTE:       return "Notes............";
  case SHT_NOBITS:     return "NoBits...........";
  case SHT_REL:        return "Relocation table.";
  case SHT_SHLIB:      return "Sharedlib.Invalid";
  case SHT_DYNSYM:     return "Dynamic-only-symb";
  default:
                       return "?????????????????";
  }
}

/* Display stringtable */
void display_stringtable(char *stringtab, int stringsize, int n)
{
  char *s;
  int i;
  
  dbglog(DBG_DEBUG, "*************** STRING TABLE %d **************\n", n);
  for (i=0, s=stringtab; s<stringtab+stringsize; ++i) {
    dbglog(DBG_DEBUG, "[%4d] [%08x] [%s]\n",
	   i, s-stringtab, s);
    while (*s++);
  }
  dbglog(DBG_DEBUG, "**********************************************\n");
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
  char *type_str[] = { "UDF", "OBJ", "FCT", "SEC",
		       "FIL", "???", "???", "???",
                       "???", "???", "???", "???",
                       "???", "???", "???", "???"};
  char section[8];

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

  dbglog(DBG_DEBUG, "%s[%s] val:[%08x] sz:[%06x] %s [%s]\n",
	 tag ? "SYM: " : "",
	 type_str[ELF32_ST_TYPE(symb->info)],
	 symb->value,
	 symb->size,
	 section,
	 symb->name + strtab);
}

static display_section(int i, struct lef_shdr_t *shdrs, int tag)
{
    dbglog(DBG_DEBUG, "%s#%02X @%08x o:%06x s:%06x a:%02x f:%08x [%s] [%s]\n",
	   tag ? "SEC: " : "",
	   i, shdrs->addr,shdrs->offset, shdrs->size,shdrs->addralign,
	   shdrs->flags, section_str(shdrs->type), shdrs->name);
}

static void display_sections(struct lef_hdr_t *hdr, struct lef_shdr_t *shdrs,
			     int tag)
{
  int i;
  dbglog(DBG_DEBUG,"--------------------------------------------------\n");
  dbglog(DBG_DEBUG,"SECTIONS [%d]\n", hdr->shnum);
  dbglog(DBG_DEBUG,"--------------------------------------------------\n");
  for (i=0; i<hdr->shnum; ++i) {
    display_section(i, shdrs+i, tag);
  }
  dbglog(DBG_DEBUG,"--------------------------------------------------\n");
}

static void display_symbtable(struct lef_sym_t * stab,
			      struct lef_shdr_t *shdrs, char *stringtab,
			      int tag)
{
  int stabsz, j;

  stabsz = shdrs->size / sizeof(struct lef_sym_t);
  dbglog(DBG_DEBUG, "--------------------------------------------------\n");
  dbglog(DBG_DEBUG, "SYMBOL-TABLE [%s] [%s] (%d)\n",
	 section_str(shdrs->type), shdrs->name, stabsz);
  dbglog(DBG_DEBUG, "--------------------------------------------------\n");
  for (j=0; j<stabsz; ++j) {
    display_symb(stab + j, stringtab, tag);
  }
  dbglog(DBG_DEBUG, "--------------------------------------------------\n");
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

/* Pass in a file descriptor from the virtual file system, and the
   result will be NULL if the file cannot be loaded, or a pointer to
   the loaded and relocated executable otherwise. The second variable
   will be set to the entry point. */
/* There's a lot of shit in here that's not documented or very poorly
   documented by Intel.. I hope that this works for future compilers. */
lef_prog_t *lef_load(uint32 fd)
{
  lef_prog_t		*out = 0;
  char			*img = 0, *imgout = 0;
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
  int                   section_copied = 0;

  /* Load the file: needs to change to just load headers */
  sz = fs_total(fd);
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Loading ELF file of size %d\n", sz);
	
  img = calloc(1,sz);
  if (!img) {
    dbglog(DBG_ERROR, 
	   "!! " __FUNCTION__ " : Can't allocate %d bytes for ELF load\n", sz);
    goto error;
  }
  if (fs_read(fd, img, sz) != sz) {
    dbglog(DBG_ERROR,
	   "!! " __FUNCTION__ " : Read error\n");
    goto error;
  }
  fs_close(fd);
  fd = 0;

  /* Header is at the front */
  hdr = (struct lef_hdr_t *)(img+0);
  if (hdr->ident[0] != 0x7f || strncmp(hdr->ident+1, "ELF", 3)) {
    dbglog(DBG_ERROR, "!! " __FUNCTION__ " : Not a valid ELF\n");
    hdr->ident[4] = 0;
    dbglog(DBG_ERROR, "!! " __FUNCTION__ " : hdr->ident is %d/%s\n",
	   hdr->ident[0], hdr->ident+1);
    goto error;
  }
  if (hdr->ident[4] != 1 || hdr->ident[5] != 1) {
    dbglog(DBG_ERROR, "!! " __FUNCTION__ " : Invalid architecture flags\n");
    goto error;
  }
  if (hdr->machine != 0x2a) {
    dbglog(DBG_ERROR, "!! " __FUNCTION__ " : Invalid machine flags\n");
    goto error;
  }
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Indentification test [completed]\n");

  shdrs = (struct lef_shdr_t *)(img + hdr->shoff);
  sectionname = (char*)(img + shdrs[hdr->shstrndx].offset);

  /* Reloc section name names */
  for (i=0; i<hdr->shnum; ++i) {
    shdrs[i].name = (uint32)(sectionname + shdrs[i].name);
  }

  /* Display sections */
  if (verbose) {
    display_sections(hdr,shdrs,0);
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
      dbglog(DBG_ERROR, "!! " __FUNCTION__ 
	     " : Too many string table [%d]\n", cnt);
      goto error;
    }
  }
  if (!stringtab) {
    dbglog(DBG_ERROR, "!! " __FUNCTION__ " : No string table\n");
    goto error;
  }

  /* Display symbol tables */
  for (i=0; i<hdr->shnum; i++) {
    if (shdrs[i].type != SHT_SYMTAB && shdrs[i].type != SHT_DYNSYM) {
      continue;
    }
    if (verbose) {
      display_symbtable((struct lef_sym_t *)(img + shdrs[i].offset),
			shdrs+i, stringtab,0);
    }
  }

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
      dbglog(DBG_ERROR, "!! " __FUNCTION__
	     " : Too many symbol tables [%d]\n", cnt);
      goto error;
    }
  }

  if (!symtabhdr) {
    dbglog(DBG_ERROR, "!! " __FUNCTION__ " : No symbol table\n");
    goto error;
  }

  if (symtabhdr->entsize != sizeof(struct lef_sym_t)) {
    dbglog(DBG_ERROR, "!! " __FUNCTION__
	   " : Symbol table entry size (%d) != struct lef_sym_t (%d)\n",
	   symtabhdr->entsize, sizeof(struct lef_sym_t));
    goto error;
  }
  symtab = (struct lef_sym_t *)(img + symtabhdr->offset);
  symtabsize = symtabhdr->size / sizeof(struct lef_sym_t);
  if (symtabhdr->size % sizeof(struct lef_sym_t)) {
    dbglog(DBG_ERROR, "!! " __FUNCTION__ " : Not multiple of symbol struct\n");
    goto error;
  }

  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Symbol table test [completed]\n");

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
  out = calloc(1,sizeof(lef_prog_t));
  if (!out) {
    dbglog(DBG_ERROR,
	   "!! " __FUNCTION__ " : Can't alloc %d bytes for prg structure\n",
	   sizeof(lef_prog_t));
    goto error;
  }
  out->ref_count = 0;
  out->data = imgout = calloc(1,sz);
  if (!out->data) {
    dbglog(DBG_ERROR,
	   "!! " __FUNCTION__ "  Can't allocate %d bytes for prg image\n", sz);
    goto error;
  }
  out->size = sz;

  /* Set section real addres, and copy */
  dbglog(DBG_DEBUG, "------------------------------------------------\n");
  dbglog(DBG_DEBUG, "CREATE IMAGE\n");
  dbglog(DBG_DEBUG, "------------------------------------------------\n");
  for (i=0; i<hdr->shnum; i++) {
    shdrs[i].addr += imgout;
    if (shdrs[i].flags & SHF_ALLOC) {
      //      section_copied |= 1<<i;
      if (shdrs[i].type == SHT_NOBITS) {
	memset((void *)shdrs[i].addr, 0, shdrs[i].size);
	dbglog(DBG_DEBUG, "CLEARED ");
      }
      else {
	memcpy((void *)shdrs[i].addr, img+shdrs[i].offset, shdrs[i].size);
	dbglog(DBG_DEBUG, "COPIED  ");
      }
    } else {
      //section_copied &= ~(1<<i);
      dbglog(DBG_DEBUG,   "SKIPPED ");
    }
    display_section(i, shdrs+i,0);
  }
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Fill Image [completed]\n");

  dbglog(DBG_DEBUG,"***********************************************\n");
  dbglog(DBG_DEBUG,"***********************************************\n");
  dbglog(DBG_DEBUG,"***********************************************\n");
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
      dbglog(DBG_DEBUG,"** " __FUNCTION__
	     " : Section [%s] skipped\n", section_str(shdrs[i].type));
      continue;
    }
    reltab = (struct lef_rela_t *)(img + shdrs[i].offset);

    if ( shdrs[i].entsize != sizeof(struct lef_rela_t)) {
      dbglog(DBG_ERROR,"!! " __FUNCTION__
	     " : Relocation entry size (%d) != lef_rela_t (%d)\n",
	     shdrs[i].entsize,sizeof(struct lef_rela_t));
      ++errors;
      continue;
    }
    reltabsize = shdrs[i].size / sizeof(struct lef_rela_t);
    if (shdrs[i].size % sizeof(struct lef_rela_t)) {
      dbglog(DBG_ERROR,"!! " __FUNCTION__
	     " : Weird relatif table size : not multiple : skip\n");
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
      dbglog(DBG_ERROR, "!! " __FUNCTION__ " : Bad section %x\n",sect);
      ++errors;
    }
    
    if (! (shdrs[sect].flags & SHF_ALLOC)) {
      /* This is no more considerate as an Error because it could be debug
	 info. A Warning should be OK. */
      dbglog(DBG_WARNING, "!! " __FUNCTION__
	     " : Can't reloc uncopied section : ");
      display_section(sect, shdrs+sect, 0);
      ++warnings;
      continue;
    }

    /*
link = section header index of associated symbol table
info = section header index of section to which reloc applies
    */
   
    if (verbose) {
      dbglog(DBG_DEBUG, "------------------------------------------------\n");
      dbglog(DBG_DEBUG, "RELOCATON "); display_section(i,shdrs+i,0); 
      dbglog(DBG_DEBUG, "SECTION   "); display_section(info,shdrs+info,0); 
      dbglog(DBG_DEBUG, "SYMBOL    "); display_section(link,shdrs+link,0); 
      dbglog(DBG_DEBUG, "------------------------------------------------\n");
    }

    for (j=0; j<reltabsize; j++) {
      int sym;
      void * main_addr;
      char * name;

      if (ELF32_R_TYPE(reltab[j].info) != R_SH_DIR32) {
	dbglog(DBG_ERROR, "!! " __FUNCTION__ " : Unknown RELA type %02x\n",
	       ELF32_R_TYPE(reltab[j].info));
	++errors;
	continue;
      }

      /* Get symbol idx */
      sym = ELF32_R_SYM(reltab[j].info);
      if (sym >= symtabsize) {
	dbglog(DBG_ERROR, "!! " __FUNCTION__
	       " : Symbol idx out of range [%d >= %d]\n",
	       sym, symtabsize);
	++errors;
	continue;
      }

      /* Check symbol section */
      if (symtab[sym].shndx >= hdr->shnum
	  /*&& symtab[sym].shndx != SHN_COMMON*/) {
	dbglog(DBG_ERROR, "!! " __FUNCTION__
	       " : Invalid symbol section index : %04x\n", symtab[sym].shndx);
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
	  dbglog(DBG_ERROR, "!! " __FUNCTION__ 
		 " : Undefined symbol [%s]\n", name);
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
	dbglog(DBG_WARNING, "!!  " __FUNCTION__
	       " : What to do with this symbol ?\n");
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
	  dbglog(DBG_ERROR, "!! " __FUNCTION__
		 " : Relocation patch @%p out of range [%p-%p]\n",
		 addr,imgout,imgout+sz);
	  ++errors;
	  continue;
	}
	/* Test address to relocate */
	if (!main_addr &&
	    ((uint32)rel<(uint32)imgout || (uint32)rel>=(uint32)imgout+sz)) {
	  dbglog(DBG_ERROR, "!! " __FUNCTION__
		 " : Relocation @%p out of range [%p-%p]\n",
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
	  dbglog(DBG_DEBUG,"** " __FUNCTION__
		 " : %c %c @%08x [=%08x] <= [%08x] [+%08x] [%s]\n" ,
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
    dbglog(DBG_WARNING, "!! " __FUNCTION__ 
	   " lef_load warning: found no RELA sections; did you forget -r?\n");
    ++warnings;
  }
  
  /* Look for the kernel negotiation symbols and deal with that */
  {
    int mainsym /*, getsvcsym, notifysym*/;

    mainsym = find_sym("_ko_main", symtab, symtabsize);
    if (mainsym < 0) {
      dbglog(DBG_ERROR, "!! " __FUNCTION__ 
	     " lef_load: ELF contains no _ko_main\n");
      goto error;
    }
    out->ko_main = (int (*)(int,char**))(symtab[mainsym].value);
  }

  if (errors) {
    goto error;
  }

  free(img);
  img = 0;

  if (warnings) {
    dbglog(DBG_DEBUG,
	   "** " __FUNCTION__ " Warnings:%d\n", warnings);
  }

  dbglog(DBG_DEBUG,
	 "<< " __FUNCTION__ " image [@%08x, %08x] entry [%08x]\n",
	 out->data, out->size, out->ko_main);
  return out;

  error:
  if (fd) {    
    fs_close(fd);
  }
  if (out) {
    if (out->data) {
      free(out->data);
    }
    free(out);
  }
  if (img) {
    free(img);
  }
  dbglog(DBG_DEBUG,
	 "** " __FUNCTION__ " Errors:%d Warnings:%d\n", errors, warnings);
  dbglog(DBG_DEBUG, "<< " __FUNCTION__ " : Failed\n");
  return 0;
}

/* Free a loaded ELF program */
void lef_free(lef_prog_t *prog) {
  if (prog && --prog->ref_count<=0) {
    free(prog->data);
    free(prog);
  }
}
