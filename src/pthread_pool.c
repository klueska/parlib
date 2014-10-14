#include "internal/parlib.h"
#include "internal/pthread_pool.h"
#include "internal/futex.h"
#include "slab.h"
#include "spinlock.h"
#include <limits.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/queue.h>

struct job {
  SIMPLEQ_ENTRY(job) link;
  void *(*func)(void*);
  void *arg;
};
SIMPLEQ_HEAD(job_queue, job);
static struct job_queue job_queue = SIMPLEQ_HEAD_INITIALIZER(job_queue);
static spinlock_t lock = SPINLOCK_INITIALIZER;

static struct slab_cache *job_slab;
static pthread_attr_t attr;
static int num_avail = 0;
static int num_enqueued = 0;
static int total_enqueued = 0;

static void __attribute__((constructor)) pthread_pool_init()
{
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  job_slab = slab_cache_create("job_slab",
    sizeof(struct job), __alignof__(struct job), 0, NULL, NULL);
}

static struct job *__dequeue_job(struct job_queue *q)
{
  struct job *j = SIMPLEQ_FIRST(q);
  if (j) {
    SIMPLEQ_REMOVE_HEAD(q, link);
    num_enqueued--;
  }
  return j;
}

static void *__thread_wrapper(void *arg)
{
  int prev_total_enqueued = 0;
  struct job_queue *q = (struct job_queue*)arg;
  struct job job, *jobp = NULL;

  while(1) {
    spinlock_lock(&lock);
      if (jobp != NULL)
        num_avail++;

      if ((jobp = __dequeue_job(q)) == NULL)
        prev_total_enqueued = total_enqueued;
      else {
        job = *jobp;
        slab_cache_free(job_slab, jobp);
        num_avail--;
      }
    spinlock_unlock(&lock);

    if (jobp)
      job.func(job.arg);
    else
      futex_wait(&total_enqueued, prev_total_enqueued);
  }

  return 0;
}

void EXPORT_SYMBOL pooled_pthread_start(void *(*func)(void*), void *arg)
{
  spinlock_lock(&lock);
    struct job *j = slab_cache_alloc(job_slab, 0);
    assert(j);
    j->func = func;
    j->arg = arg;

    SIMPLEQ_INSERT_HEAD(&job_queue, j, link);
    num_enqueued++;
    total_enqueued++;

    if (num_avail < num_enqueued) {
      assert(num_avail == num_enqueued-1);
      pthread_t handle;
      pthread_create(&handle, &attr, __thread_wrapper, &job_queue);
      num_avail++;
    }
  spinlock_unlock(&lock);

  futex_wakeup_one(&total_enqueued);
}
