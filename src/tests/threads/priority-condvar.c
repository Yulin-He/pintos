/* Tests that cond_signal() wakes up the highest-priority thread
   waiting in cond_wait(). */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static thread_func priority_condvar_thread;
static struct lock lock;
static struct condition condition;
/*condition的waiters队列是优先级队列*/
void test_priority_condvar(void)
{
  int i;

  /* This test does not work with the MLFQS. */
  ASSERT(!thread_mlfqs);

  lock_init(&lock);
  cond_init(&condition);

  thread_set_priority(PRI_MIN);
  for (i = 0; i < 10; i++)
  {
    int priority = PRI_DEFAULT - (i + 7) % 10 - 1;
    char name[16];
    snprintf(name, sizeof name, "priority %d", priority);
    thread_create(name, priority, priority_condvar_thread, NULL);
  }

  for (i = 0; i < 10; i++)
  {
    lock_acquire(&lock);
    msg("Signaling...");
    cond_signal(&condition, &lock); // 在信号量条件被满足时唤醒一个等待在condition上的线程
    lock_release(&lock);
  }
}

static void
priority_condvar_thread(void *aux UNUSED)
{
  msg("Thread %s starting.", thread_name());
  lock_acquire(&lock);          // 获取锁
  cond_wait(&condition, &lock); // 释放掉锁， 等待signal唤醒， 然后再重新获取锁
  msg("Thread %s woke up.", thread_name());
  lock_release(&lock);
}
