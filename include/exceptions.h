/**
 * @ingroup    dcplaya_exception
 * @file       exceptions.h
 * @author     vincent penne
 * @date       2002/11/09
 * @brief      Exceptions and guardians handling
 *
 * @version    $Id: exceptions.h,v 1.7 2004-06-30 15:17:35 vincentp Exp $
 */


#ifndef _EXECEPTIONS_H_
#define _EXECEPTIONS_H_

//#include <kos.h>
#include <kos/thread.h>
#include "setjmp.h"

/** @defgroup  dcplaya_exception  Exceptions and Guardians handling
 *  @ingroup   dcplaya_devel
 *  @brief     exceptions and guardians handling
 *  @author    vincent penne
 *  @{
 */

/** Initialize the exception system. */
void expt_init(void);

/** Shutdoyn the exception system. */
void expt_shutdown(void);

/** Start a protected section.
 *  @deprecated Use EXPT_GUARD_BEGIN macro.
 */
int expt_guard_begin(void);

// moved to kos/thread.h
//#define EXPT_GUARD_STACK_SIZE 8

/*
extern int expt_guard_stack_pos;
extern jmp_buf expt_jump_stack[EXPT_GUARD_STACK_SIZE];
*/
extern void irq_dump_regs(int , int);

/** @name Protected section.
 *
 *  To protect a code from exception :
 * @code
 *  char * buffer = malloc(32);
 *  EXPT_GUARD_BEGIN;
 *  // Run code to protect here. This instruction makes a bus error or
 *  // something like that on most machine.
 *  *(int *) (buffer+1) = 0xDEADBEEF;
 *
 *  EXPT_GUARD_CATCH;
 *  // Things to do if hell happen
 *  printf("Error\n");
 *  free(buffer);
 *  return -1;
 *
 *  EXPT_GUARD_END;
 *  // Life continue ... 
 *  memset(buffer,0,32);
 *  // ...
 * @endcode
 *
 * @{ 
 */

#define NO_EXPT

#ifdef NO_EXPT


#define EXPT_GUARD_BEGIN                                  \
  if (1) {                                                \

#define EXPT_GUARD_CATCH                                  \
    } else {                                              \

#define EXPT_GUARD_END }

#define EXPT_GUARD_RETURN                                 \
  return



#else /* ifdef NO_EXPT */



/** Start a protected section.
  * @warning : it is FORBIDEN to do "return" inside a guarded section,
    use EXPT_GUARD_RETURN instead. */
#define EXPT_GUARD_BEGIN                                  \
  if (1) {                                                \
    thd_current->expt_guard_stack_pos++;                               \
    if (thd_current->expt_guard_stack_pos >= EXPT_GUARD_STACK_SIZE)    \
      * (int *) 1 = 0xdeadbeef ;                          \
    if (!setjmp(thd_current->expt_jump_stack[thd_current->expt_guard_stack_pos])) { 
//    if (!expt_guard_begin()) {

/** Catch a protected section. */
#define EXPT_GUARD_CATCH                                  \
    } else {                                              \
      printf("CATCHING EXCEPTION IN %s:%s (%d)\n", __FILE__, __FUNCTION__, __LINE__); \
      irq_dump_regs(0, 0);

/** End of protected section. */
#define EXPT_GUARD_END                                    \
    }                                                     \
    thd_current->expt_guard_stack_pos--;                  \
  } else

/** Return in middle of a guarded section. 
    @warning : to be used exclusively inbetween EXPT_GUARD_BEGIN and
    EXPT_GUARD_END. Never use normal "return" in this case. */
#define EXPT_GUARD_RETURN                                 \
  thd_current->expt_guard_stack_pos--;                    \
  return



#endif /* ifdef NO_EXPT */




/**@}*/

#endif // #ifndef _EXECEPTIONS_H_

