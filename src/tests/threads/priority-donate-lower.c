/* The main thread acquires a lock.  Then it creates a
   higher-priority thread that blocks acquiring the lock, causing
   it to donate their priorities to the main thread.  The main
   thread attempts to lower its priority, which should not take
   effect until the donation is released. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func acquire_thread_func;
/*测试的逻辑是当修改一个被捐赠的线程优先级的时候的行为正确性*/

void test_priority_donate_lower(void)
{
     struct lock lock;

     /* This test does not work with the MLFQS. */
     ASSERT(!thread_mlfqs);

     /* Make sure our priority is the default. */
     ASSERT(thread_get_priority() == PRI_DEFAULT);

     lock_init(&lock);
     lock_acquire(&lock);                                                    // 主线程获取锁lock
     thread_create("acquire", PRI_DEFAULT + 10, acquire_thread_func, &lock); // 创建线程acquire
     msg("Main thread should have priority %d.  Actual priority: %d.",
         PRI_DEFAULT + 10, thread_get_priority());

     msg("Lowering base priority...");
     thread_set_priority(PRI_DEFAULT - 10); // 主线程优先级降低（但因为优先级捐献暂时不改变，释放锁后应该降低）
     msg("Main thread should have priority %d.  Actual priority: %d.",
         PRI_DEFAULT + 10, thread_get_priority());
     lock_release(&lock); // 释放锁lock
     msg("acquire must already have finished.");
     msg("Main thread should have priority %d.  Actual priority: %d.",
         PRI_DEFAULT - 10, thread_get_priority());
}

static void
acquire_thread_func(void *lock_)
{
     struct lock *lock = lock_;

     lock_acquire(lock); // 尝试获取锁lock，被主线程占有lock，阻塞
     msg("acquire: got the lock");
     lock_release(lock);
     msg("acquire: done");
}
