/* Verifies that lowering a thread's priority so that it is no
   longer the highest-priority thread in the system causes it to
   yield immediately. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/thread.h"

static thread_func changing_thread;

void test_priority_change(void)
{
  /* This test does not work with the MLFQS. */
  ASSERT(!thread_mlfqs);

  msg("Creating a high-priority thread 2.");
  thread_create("thread 2", PRI_DEFAULT + 1, changing_thread, NULL); // 测试线程(称为thread1)创建了一个PRI_DEFAULT+1优先级的内核线程thread2
  msg("Thread 2 should have just lowered its priority.");
  thread_set_priority(PRI_DEFAULT - 2); // thread1优先级改成PRI_DEFAULT-2
  msg("Thread 2 should have just exited.");
}

static void
changing_thread(void *aux UNUSED)
{
  msg("Thread 2 now lowering priority.");
  thread_set_priority(PRI_DEFAULT - 1); // thread2把自身优先级调为PRI_DEFAULT-1
  msg("Thread 2 exiting.");
}
