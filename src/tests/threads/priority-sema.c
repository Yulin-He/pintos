/* Tests that the highest-priority thread waiting on a semaphore
   is the first to wake up. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static thread_func priority_sema_thread;
static struct semaphore sema;
/* 信号量的等待队列是优先级队列*/
void test_priority_sema(void)
{
  int i;

  /* This test does not work with the MLFQS. */
  ASSERT(!thread_mlfqs);

  sema_init(&sema, 0);
  thread_set_priority(PRI_MIN); // 设置主线程优先级为最低
  for (i = 0; i < 10; i++)
  {
    int priority = PRI_DEFAULT - (i + 3) % 10 - 1;
    char name[16];
    snprintf(name, sizeof name, "priority %d", priority);
    thread_create(name, priority, priority_sema_thread, NULL); // 创建线程（优先级从低到高）
  }

  for (i = 0; i < 10; i++)
  {
    sema_up(&sema); // V操作，优先级高的先唤醒
    msg("Back in main thread.");
  }
}

static void
priority_sema_thread(void *aux UNUSED)
{
  sema_down(&sema); // P操作，10个线程全部阻塞
  msg("Thread %s woke up.", thread_name());
}
