/**
 * @ingroup    dcplaya
 * @file       exceptions.c
 * @author     vincent penne <ziggy@sashipa.com>
 * @date       2002/11/09
 * @brief      Exceptions and guardians handling
 *
 * @version    $Id: exceptions.c,v 1.7 2004-07-31 22:55:19 vincentp Exp $
 */

#include <kos.h>
#include <arch/irq.h>

#include "dcplaya/config.h"
#include "exceptions.h"
#include "setjmp.h"


/*
 * Working with KOS 1.1.5, may change with other versions !!
 *
 * This is the table where context is saved when an exception occure
 * After the exception is handled, context will be restored and 
 * an RTE instruction will be issued to come back to the user code.
 * Modifying the content of the table BEFORE returning from the handler
 * of an exception let us do interesting tricks :)
 *
 * 0x00 .. 0x3c : Registers R0 to R15
 * 0x40         : SPC (return adress of RTE)
 * 0x58         : SSR (saved SR, restituted after RTE)
 * 
 *
 * MODIFIED : using the structure definition from kos kernel include file, lol !
 *
 */
//extern void * * irq_srt_addr;
irq_context_t * irq_srt_addr;

/* This is the list of exceptions code we want to handle */
static struct {
  int code;
  char * name;
} exceptions_code[] = {
  EXC_USER_BREAK_PRE	,"  EXC_USER_BREAK_PRE"	, // 0x01e0	/* User break before instruction */
  EXC_INSTR_ADDRESS	,"  EXC_INSTR_ADDRESS"	, // 0x00e0	/* Instruction address */
  EXC_ITLB_MISS		,"  EXC_ITLB_MISS"	, // 0x00a0	/* Instruction TLB miss */
  EXC_ITLB_PV		,"  EXC_ITLB_PV"	, // 0x00a0	/* Instruction TLB protection violation */
  EXC_ILLEGAL_INSTR	,"  EXC_ILLEGAL_INSTR"	, // 0x0180	/* Illegal instruction */
  EXC_GENERAL_FPU		,"  EXC_GENERAL_FPU", // 0x0800	/* General FPU exception */
  EXC_SLOT_FPU		,"  EXC_SLOT_FPU"	, // 0x0820	/* Slot FPU exception */
  EXC_DATA_ADDRESS_READ	,"  EXC_DATA_ADDRESS_READ", // 0x00e0	/* Data address (read) */
  EXC_DATA_ADDRESS_WRITE	,"  EXC_DATA_ADDRESS_WRITE", // 0x0100	/* Data address (write) */
  EXC_DTLB_MISS_READ	,"  EXC_DTLB_MISS_READ"	, // 0x0040	/* Data TLB miss (read) */
  EXC_DTLB_MISS_WRITE	,"  EXC_DTLB_MISS_WRITE", // 0x0060	/* Data TLB miss (write) */
  EXC_DTLB_PV_READ	,"  EXC_DTLB_PV_READ"	, // 0x00a0	/* Data TLB P.V. (read) */
  EXC_DTLB_PV_WRITE	,"  EXC_DTLB_PV_WRITE"	, // 0x00c0	/* Data TLB P.V. (write) */
  EXC_FPU			,"  EXC_FPU"	, // 0x0120	/* FPU exception */
  EXC_INITIAL_PAGE_WRITE	,"  EXC_INITIAL_PAGE_WRITE"	, // 0x0120	/* Initial page write exception */
  0
};


#include "lef.h"
extern symbol_t main_symtab[];
extern int main_symtab_size;

static int stack_cb(void *ctx, void *dummy)
{
  uint32 PC = _Unwind_GetIP(ctx);
  symbol_t *symb = lef_closest_symbol(PC);
  if (symb) {
    printf("STACK 0x%8x (%s + 0x%x)\n", 
	   PC, symb->name, PC - ((uint32)symb->addr));
  }
  return 0;
}

static void guard_irq_handler(irq_t source, irq_context_t *context)
{
  if (source == EXC_FPU) {
    /* Display user friendly informations */
    symbol_t * symb;
    int i;

    symb = lef_closest_symbol(irq_srt_addr->pc);
    if (symb) {
      printf("FPU EXCEPTION PC = %s + 0x%x (0x%x)\n", 
	     symb->name, 
	     ((int)irq_srt_addr->pc) - ((int)symb->addr), irq_srt_addr->pc);
    }

    /* skip the offending FPU instruction */
    int * ptr = (int *) &irq_srt_addr->r[0x40/4];
    *ptr += 4;

    return;
  }

  {
    /* Display user friendly informations */
    symbol_t * symb;
    int i;
    uint * stack = (uint *) irq_srt_addr->r[15];

#if 1
    for (i=15; i>=0; i--) {
      symb = lef_closest_symbol(stack[i]);
      if (symb) {
	printf("STACK#%2d = 0x%8x (%s + 0x%x)\n", i, stack[i],
	       symb->name, 
	       stack[i] - ((int)symb->addr));
      }
    }
#else
    _Unwind_Backtrace(stack_cb, NULL);
#endif

    for (i=0; exceptions_code[i].code; i++)
      if (exceptions_code[i].code == source) {
	printf("EVENT = %s (0x%x)\n", exceptions_code[i].name, source);
	break;
      }

    symb = lef_closest_symbol(irq_srt_addr->pc);
    if (symb) {
      printf("PC = %s + 0x%x (0x%x)  ", 
	     symb->name, 
	     ((int)irq_srt_addr->pc) - ((int)symb->addr), irq_srt_addr->pc);
    }

    symb = lef_closest_symbol(irq_srt_addr->pr);
    if (symb) {
      printf("PR = %s + 0x%x (0x%x)\n", 
	     symb->name, 
	     ((int)irq_srt_addr->pr) - ((int)symb->addr), irq_srt_addr->pr);
    }

  }

  //printf("CATCHING EXCEPTION IN SHELL\n");

  //irq_dump_regs(0, source);

  if (thd_current->expt_guard_stack_pos >= 0) {
    // Simulate a call to longjmp by directly changing stored 
    // context of the exception
    //irq_srt_addr->r[0x40/4] = longjmp;
    irq_srt_addr->pc = longjmp;
    irq_srt_addr->r[4] = thd_current->expt_jump_stack[thd_current->expt_guard_stack_pos];
    irq_srt_addr->r[5] = (void *) -1;
  } else {
    malloc_stats();
    texture_memstats();

    /* not handled --> panic !! */
    irq_dump_regs(0, source);
    panic("DCPLAYA unhandled IRQ/Exception");
  }

}


void expt_init()
{
  int i;

  // TODO : save old values
  for (i=0; exceptions_code[i].code; i++)
    irq_set_handler(exceptions_code[i].code, guard_irq_handler);

  //expt_guard_stack_pos = -1;
}

void expt_shutdown()
{
  int i;

  for (i=0; exceptions_code[i].code; i++)
    irq_set_handler(exceptions_code[i].code, 0);
}

int expt_guard_begin()
{
  return setjmp(thd_current->expt_jump_stack[thd_current->expt_guard_stack_pos]);
}




/* uint __fixunssfsi(float a) */
/* { */
/*   printf("***********FIXUNSSFSI************* : %g\n", a); */
/*   return a; */
/* } */
