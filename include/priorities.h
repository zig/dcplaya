/**
 * @ingroup  dcplaya_priorities
 * @file     priorities.h
 * @author   vincent penne
 * @date     2003/01/19
 * @brief    Thread priority settings.
 * 
 * $Id: priorities.h,v 1.2 2003-04-21 16:26:36 vincentp Exp $
 */


/** @defgroup dcplaya_priorities Priorities
 *  @ingroup  dcplaya_devel
 *  @brief    Priority settings
 *  @author   vincent penne
 *  @{
 */

/**
   Each thread in dcplaya can have a different priority. We use prio2
   setting of kos thread structure. The value in prio2 tells to the 
   task timer scheduler how many jiffies need to elapse before the current
   task may switch. The higher prio2 is set on one specific task, the 
   more CPU time is allocated to this task.

   In dcplaya, the task that has the highest priority is the main task,
   this is the one that in particular handle all rendering and animations,
   we set high priority so that animations keeps smooth most of the time.

   The second highest priority is allocated to the lua task and  the
   maple task. Lua because it also handles some animations, and
   the maple task because it is better not to interrupt too much transfer
   on the maple bus (anyway it does not takes too much CPU time). 

   The sndstream task also need quite good scheduling so it is
   boosted too. But it does not use a lot of CPU.

   Then all other tasks have priority 1 (smallest). The playa decoder is a 
   special case, is has smallest priority except when the samples fifo gets
   almost empty, in this case we boost the playa decoder task in order to be
   sure that the sound streaming won't be interrupted.

   Note that one jiffy is equal to 1/1200th of a second (see HZ defined in
   arch.h)
*/

/** Main thread priority */
#define MAIN_THREAD_PRIORITY 7

/** Lua thread priority */
#define LUA_THREAD_PRIORITY 3

/** Playa sndstream thread priority */
#define PLAYA_SNDSTREAM_THREAD_PRIORITY 3

/** Playa decoder thread normal state priority */
#define PLAYA_DECODER_THREAD_PRIORITY 1

/** Playa decoder thread boost priority */
#define PLAYA_DECODER_THREAD_BOOST_PRIORITY 3

/** Maple thread priority */
#define MAPLE_THREAD_PRIORITY 3

/**@}*/
