/**
 *  @ingroup  dcplaya_devel
 *  @file     mutex.h
 *  @author   benjamin gerard
 *  @date     2002/10/23
 *  @brief    Implements recursive mutex (mutual exclusion) over spinlock.
 */

#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <arch/spinlock.h>

#include <kos/mutex.h>

#if 1
#define mutex_t dcpmutex_t
#define mutex_init dcpmutex_init
#define mutex_trylock dcpmutex_trylock
#define mutex_lockcount dcpmutex_lockcount
#define mutex_lock dcpmutex_lock
#define mutex_unlock dcpmutex_unlock

/** @defgroup dcplaya_mutex Rescursive mutex
 *  @ingroup  dcplaya_devel
 *  @brief    rescursive mutex.
 *  @author   benjamin gerard
 *
 *  Implements recursive mutex (mutual exclusion) over spinlock.
 *
 *  @{
 */

/** Mutex object. */
typedef struct {
  volatile kthread_t * owner; /**< Owner thread. 0 if orphelan.     */
  volatile int count;         /**< Number of lock.                  */
  spinlock_t lock;            /**< Mutex of mutex object.           */
} mutex_t;

/** Initialize mutex object.
 *
 *     The mutex_init() function initializes the mutex object. If count value
 *     is zero, the mutex is not owned. In other case, the mutex is owned by
 *     the caller thread and the number of lock is set to count value.
 *
 *  @param  mutex  Pointer to mutex object.
 *  @param  count  Number of lock after init.
 */
static void mutex_init(mutex_t *mutex, int count)
{
  if (thd_mode) {
	spinlock_init(&mutex->lock);
	mutex->count = count;
	mutex->owner = count ? thd_current : 0;
  }
}

/** Check if a mutex is lockable.
 *
 *    The mutex_trylock() functions checks if the current thread could lock
 *    the mutex. In other way, it checks if the lock count is 0 or if the
 *    mutex is owned by the caller thread. In these case the function returns
 *    1, in the contrary it returns 0.
 *
 *  @param  mutex  Pointer to the mutex object to test.
 *
 *  @retval  1  Mutex is directly lockable by caller thread.
 *  @retval  0  Mutex is not directly lockable by caller thread.
 *
 * @warning This function is useful the state of a mutex, but it should not
 *          be use followed by a mutex_lock() function because the mutex could
 *          be locked beetween mutex_trylock() and mutex_lock() calls. Wise man
 *          use it with care.
 */
static int mutex_trylock(mutex_t * mutex)
{
  int ret = 1;
  if (thd_mode) {
	spinlock_lock(&mutex->lock);
	ret = !mutex->count || mutex->owner == thd_current;
	spinlock_unlock(&mutex->lock);
  }
  return ret;
}

/** Returns mutex lock counter.
 */
inline static int mutex_lockcount(const mutex_t * mutex)
{
  return mutex->count; /** $$$ Atomic op. */
}

/** Lock a mutex.
 *
 *    If the mutex is currently unlocked or owned by the caller thread,
 *    the lock counter is incremented, it becomes owned (if it was not) and
 *    the function returns immediately.
 *
 *    In other case, the thread is suspended until the mutex becomes unlocked.
 *    Then it behaves identically as if it were unlocked when the function
 *    was called (see above).
 */
inline static void mutex_lock(mutex_t *mutex)
{
  if (thd_mode) {
	do {
	  int me, cnt;
	  spinlock_lock(&mutex->lock);
	  cnt = mutex->count;
	  if (!cnt) {
		mutex->owner = thd_current;
		me = 1;
	  } else {
		me =  (thd_current == mutex->owner);
	  }
	  cnt += me;
	  mutex->count = cnt;
	  spinlock_unlock(&mutex->lock);
	  if (me) {
		break;
	  }
	  thd_pass();
	} while(1);
  }
}

/** Unlock a mutex.
 *
 *    The mutex is assumed to be locked and owned by the calling thread on
 *    entrance to mutex_unlock() function. It decrements the locking count
 *    of the mutex (number of mutex_lock() operations performed on it by
 *    the calling thread), and only when this count reaches zero is the mutex
 *    actually unlocked.
 */
inline static void mutex_unlock(mutex_t *mutex)
{
  if (thd_mode) {
	spinlock_lock(&mutex->lock);
	if (!--mutex->count) {
	  mutex->owner = 0;
	}
	spinlock_unlock(&mutex->lock);
  }
}

/**@}*/

#endif

#endif /* #define _MUTEX_H_ */
