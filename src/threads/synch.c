/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void sema_init(struct semaphore *sema, unsigned value)
{
  ASSERT(sema != NULL);

  sema->value = value;       // 将信号量的初始值设置为参数 value。信号量是一个非负整数，它表示某种资源的数量或可用性。
  list_init(&sema->waiters); // 始化了信号量内部的等待队列 waiters。等待队列是一个链表，用于存储等待某个条件的线程。在这里，将等待队列初始化为空，表示当前没有线程在等待该信号量。
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void sema_down(struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT(sema != NULL);
  ASSERT(!intr_context());

  old_level = intr_disable();
  while (sema->value == 0) // 锁不可用
  {
    list_insert_ordered(&sema->waiters, &thread_current()->elem, thread_cmp_priority, NULL);
    // list_push_back(&sema->waiters, &thread_current()->elem); // 把线程丢到这个信号量的队列waiters里
    thread_block(); // 阻塞该线程等待唤醒
  }
  sema->value--;
  intr_set_level(old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool sema_try_down(struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT(sema != NULL);

  old_level = intr_disable();
  if (sema->value > 0)
  {
    sema->value--;
    success = true;
  }
  else
    success = false;
  intr_set_level(old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void sema_up(struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT(sema != NULL);

  old_level = intr_disable();
  if (!list_empty(&sema->waiters))
  {
    list_sort(&sema->waiters, thread_cmp_priority, NULL);
    thread_unblock(list_entry(list_pop_front(&sema->waiters), struct thread, elem));
  }
  // if (!list_empty(&sema->waiters)) // 如果等待队列 waiters 不为空（即有线程等待该信号量）
  //   thread_unblock(list_entry(list_pop_front(&sema->waiters),
  //                             struct thread, elem)); // 从等待队列中取出一个线程，并通过 thread_unblock() 函数唤醒它。
  sema->value++;
  thread_yield(); // add
  intr_set_level(old_level);
}

static void sema_test_helper(void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void sema_self_test(void)
{
  struct semaphore sema[2];
  int i;

  printf("Testing semaphores...");
  sema_init(&sema[0], 0);
  sema_init(&sema[1], 0);
  thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
  {
    sema_up(&sema[0]);
    sema_down(&sema[1]);
  }
  printf("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper(void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
  {
    sema_down(&sema[0]);
    sema_up(&sema[1]);
  }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void lock_init(struct lock *lock)
{ // 一个锁在任何给定时间最多只能被一个线程持有
  // 我们的锁不是“递归”的，也就是说，当前持有锁的线程尝试再次获取相同的锁会导致错误。这种设计避免了死锁
  //  锁是信号量的一种特殊形式，其初始值为 1。
  // 信号量是一个非负整数，具有两个原子操作：down（或 P）和 up（或 V）。down 操作会等待信号量的值变为正数，然后将其减一；up 操作会增加信号量的值，并唤醒等待该信号量的线程之一。
  ASSERT(lock != NULL);

  lock->holder = NULL; // 在初始化时，锁还没有被任何线程持有
  sema_init(&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void lock_acquire(struct lock *lock)
{
  struct thread *current_thread = thread_current();
  struct lock *l;
  enum intr_level old_level;

  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(!lock_held_by_current_thread(lock));

  if (lock->holder != NULL && !thread_mlfqs) // 如果锁已经被持有，并且不是在多级反馈队列调度（MLFQS）模式下，那么当前线程需要进行优先级捐赠
  {
    current_thread->lock_waiting = lock;
    l = lock;
    while (l && current_thread->priority > l->max_priority) // 当前线程的优先级高于持有锁链上的锁的最高优先级时，执行优先级捐赠
    {
      l->max_priority = current_thread->priority; // 更新锁的最高优先级
      thread_donate_priority(l->holder);          // 对锁的持有者执行优先级捐赠
      l = l->holder->lock_waiting;                // 如果锁的持有者还持有别的锁，嵌套执行
    }
  }

  sema_down(&lock->semaphore); // 等待锁可用：如果信号量的值为 0 或负数（表示锁当前被其他线程持有），当前线程将进入睡眠状态，直到锁变为可用为止。

  old_level = intr_disable();

  current_thread = thread_current();
  if (!thread_mlfqs)
  {
    current_thread->lock_waiting = NULL;           // 当前线程已经不再等待任何锁
    lock->max_priority = current_thread->priority; // 设置当前锁的最高优先级为当前线程的优先级
    thread_hold_the_lock(lock);                    // 记录当前线程持有的锁
  }
  lock->holder = current_thread;

  intr_set_level(old_level);
}
// void lock_acquire(struct lock *lock)
// {
//   ASSERT(lock != NULL);
//   ASSERT(!intr_context());
//   ASSERT(!lock_held_by_current_thread(lock));

//   sema_down(&lock->semaphore); // 等待锁可用：如果信号量的值为 0 或负数（表示锁当前被其他线程持有），当前线程将进入睡眠状态，直到锁变为可用为止。
//   lock->holder = thread_current();
// }

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool lock_try_acquire(struct lock *lock)
{
  bool success;

  ASSERT(lock != NULL);
  ASSERT(!lock_held_by_current_thread(lock));

  success = sema_try_down(&lock->semaphore);
  if (success)
    lock->holder = thread_current();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void lock_release(struct lock *lock)
{
  ASSERT(lock != NULL);
  ASSERT(lock_held_by_current_thread(lock));

  if (!thread_mlfqs)
    thread_remove_lock(lock);

  lock->holder = NULL;
  sema_up(&lock->semaphore); // 将锁中的信号量的值增加 1。这表示锁现在不再被任何线程持有，其他等待该锁的线程可以通过sema_down获取锁并继续执行。
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool lock_held_by_current_thread(const struct lock *lock)
{
  ASSERT(lock != NULL);

  return lock->holder == thread_current();
}

/* One semaphore in a list. */
struct semaphore_elem
{
  struct list_elem elem;      /* List element. */
  struct semaphore semaphore; /* This semaphore. */
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void cond_init(struct condition *cond)
{
  ASSERT(cond != NULL);

  list_init(&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void cond_wait(struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  sema_init(&waiter.semaphore, 0);
  list_push_back(&cond->waiters, &waiter.elem); // 放入在条件变量上的队列
  lock_release(lock);                           // 释放锁
  sema_down(&waiter.semaphore);                 // P操作
  lock_acquire(lock);                           // 获取锁
}

/* cond sema comparation function */
bool cond_sema_cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  struct semaphore_elem *sa = list_entry(a, struct semaphore_elem, elem);
  struct semaphore_elem *sb = list_entry(b, struct semaphore_elem, elem);
  return list_entry(list_front(&sa->semaphore.waiters), struct thread, elem)->priority > list_entry(list_front(&sb->semaphore.waiters), struct thread, elem)->priority;
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_signal(struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  if (!list_empty(&cond->waiters)) // 检查等待在条件变量上的线程队列是否为空
  {
    list_sort(&cond->waiters, cond_sema_cmp_priority, NULL);
    sema_up(&list_entry(list_pop_front(&cond->waiters), struct semaphore_elem, elem)->semaphore); // 从等待队列中弹出一个等待线程，然后通过信号量的 sema_up 操作来唤醒该线程
  }
  // if (!list_empty(&cond->waiters))
  //   sema_up(&list_entry(list_pop_front(&cond->waiters),
  //                       struct semaphore_elem, elem)
  //                ->semaphore);
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_broadcast(struct condition *cond, struct lock *lock)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);

  while (!list_empty(&cond->waiters))
    cond_signal(cond, lock);
}

//----------------------------------------------add functions
/* lock comparation function */
bool lock_cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  return list_entry(a, struct lock, elem)->max_priority > list_entry(b, struct lock, elem)->max_priority;
}

/* Let thread hold a lock */
void thread_hold_the_lock(struct lock *lock)
{
  enum intr_level old_level = intr_disable();
  list_insert_ordered(&thread_current()->locks, &lock->elem, lock_cmp_priority, NULL); // 将当前线程持有的锁按照优先级有序地插入到线程的锁列表中

  if (lock->max_priority > thread_current()->priority)
  {                                                  // 如果锁的最高优先级大于当前线程的优先级
    thread_current()->priority = lock->max_priority; // 将当前线程的优先级提升到锁的最高优先级
    thread_yield();                                  // 由于优先级发生了改变，需要重新调度以确保高优先级的线程能够立即执行
  }

  intr_set_level(old_level);
}

/* Remove a lock. */
void thread_remove_lock(struct lock *lock)
{
  enum intr_level old_level = intr_disable();
  list_remove(&lock->elem);
  thread_update_priority(thread_current());
  intr_set_level(old_level);
}

/* Update priority. */
void thread_update_priority(struct thread *t)
{ // 当释放掉一个锁的时候， 当前线程的优先级可能发生变化
  enum intr_level old_level = intr_disable();
  int max_priority = t->base_priority;
  int lock_priority;

  if (!list_empty(&t->locks))
  {                                                                                     // 如果这个线程还有锁
    list_sort(&t->locks, lock_cmp_priority, NULL);                                      // 当前线程持有的锁按优先级排序
    lock_priority = list_entry(list_front(&t->locks), struct lock, elem)->max_priority; // 先获取这个线程拥有锁的最大优先级（可能被更高级线程捐赠）
    if (lock_priority > max_priority)                                                   // 如果这个优先级比base_priority大的话更新的应该是被捐赠的优先级
      max_priority = lock_priority;
  }

  t->priority = max_priority;
  intr_set_level(old_level);
}