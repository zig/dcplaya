/**
 * @ingroup    dcplaya
 * @file       exceptions.h
 * @author     vincent penne <ziggy@sashipa.com>
 * @date       2002/11/09
 * @brief      Exceptions and guardians handling
 *
 * @version    $Id: exceptions.h,v 1.4 2002-11-25 16:51:05 ben Exp $
 */


#ifndef _EXECEPTIONS_H_
#define _EXECEPTIONS_H_

//#include <kos.h>
#include <kos/thread.h>
#include "setjmp.h"

void expt_init();

void expt_shutdown();

int expt_guard_begin();

// moved to kos/thread.h
//#define EXPT_GUARD_STACK_SIZE 8

/*extern int expt_guard_stack_pos;
extern jmp_buf expt_jump_stack[EXPT_GUARD_STACK_SIZE];*/


extern void irq_dump_regs(int , int);


#define EXPT_GUARD_BEGIN                                  \
  if (1) {                                                \
    thd_current->expt_guard_stack_pos++;                               \
    if (thd_current->expt_guard_stack_pos >= EXPT_GUARD_STACK_SIZE)    \
      * (int *) 1 = 0xdeadbeef ;                          \
    if (!setjmp(thd_current->expt_jump_stack[thd_current->expt_guard_stack_pos])) { 
//    if (!expt_guard_begin()) {

#define EXPT_GUARD_CATCH                                  \
    } else {                                              \
      printf("CATCHING EXCEPTION IN %s:%s (%d)\n", __FILE__, __FUNCTION__, __LINE__); \
      irq_dump_regs(0, 0);

#define EXPT_GUARD_END                                    \
    }                                                     \
    thd_current->expt_guard_stack_pos--;                  \
  } else

#endif // #ifndef _EXECEPTIONS_H_

