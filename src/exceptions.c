/**
 * @ingroup    dcplaya
 * @file       exceptions.c
 * @author     vincent penne <ziggy@sashipa.com>
 * @date       2002/11/09
 * @brief      Exceptions and guardians handling
 *
 * @version    $Id: exceptions.c,v 1.3 2002-09-14 07:22:42 zig Exp $
 */

#include <kos.h>

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
 */
extern void * * irq_srt_addr;

/* This is the list of exceptions code we want to handle */
static int exceptions_code[] = {
  EXC_USER_BREAK_PRE	, // 0x01e0	/* User break before instruction */
  EXC_INSTR_ADDRESS	, // 0x00e0	/* Instruction address */
  EXC_ITLB_MISS		, // 0x00a0	/* Instruction TLB miss */
  EXC_ITLB_PV		, // 0x00a0	/* Instruction TLB protection violation */
  EXC_ILLEGAL_INSTR	, // 0x0180	/* Illegal instruction */
  EXC_GENERAL_FPU		, // 0x0800	/* General FPU exception */
  EXC_SLOT_FPU		, // 0x0820	/* Slot FPU exception */
  EXC_DATA_ADDRESS_READ	, // 0x00e0	/* Data address (read) */
  EXC_DATA_ADDRESS_WRITE	, // 0x0100	/* Data address (write) */
  EXC_DTLB_MISS_READ	, // 0x0040	/* Data TLB miss (read) */
  EXC_DTLB_MISS_WRITE	, // 0x0060	/* Data TLB miss (write) */
  EXC_DTLB_PV_READ	, // 0x00a0	/* Data TLB P.V. (read) */
  EXC_DTLB_PV_WRITE	, // 0x00c0	/* Data TLB P.V. (write) */
  EXC_FPU			, // 0x0120	/* FPU exception */
  EXC_INITIAL_PAGE_WRITE	, // 0x0120	/* Initial page write exception */
  0
};


static void guard_irq_handler(irq_t source, irq_context_t *context)
{
  //printf("CATCHING EXCEPTION IN SHELL\n");

  //irq_dump_regs(0, source);

  if (thd_current->expt_guard_stack_pos >= 0) {
    // Simulate a call to longjmp by directly changing stored 
    // context of the exception
    irq_srt_addr[0x40/4] = longjmp;
    irq_srt_addr[4] = thd_current->expt_jump_stack[thd_current->expt_guard_stack_pos];
    irq_srt_addr[5] = (void *) -1;
  } else {
    /* not handled --> panic !! */
    irq_dump_regs(0, source);
    panic("unhandled IRQ/Exception");
  }

}


void expt_init()
{
  int i;

  // TODO : save old values
  for (i=0; exceptions_code[i]; i++)
    irq_set_handler(exceptions_code[i], guard_irq_handler);

  //expt_guard_stack_pos = -1;
}

void expt_shutdown()
{
  int i;

  for (i=0; exceptions_code[i]; i++)
    irq_set_handler(exceptions_code[i], 0);
}

int expt_guard_begin()
{
  return setjmp(thd_current->expt_jump_stack[thd_current->expt_guard_stack_pos]);
}
