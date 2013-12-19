#include <pthread.h>
#include <stdio.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include "internal/pthread_pool.h"

#define NTHREADS 20
static volatile __thread int me;
static volatile pthread_t threads[NTHREADS];
static volatile int count[NTHREADS];
static int nthreads;
static pthread_t main_thread;

static void handler(int sig)
{
  printf("signal received on pthread %d, id = %d\n", (int)pthread_self(), me);
  count[me]++;
  while (count[me] < 2);
}

static void register_signal_handler(void (*handler)(int))
{
  struct sigaction act;
  act.sa_handler = handler;
  act.sa_flags = SA_NODEFER;
  sigemptyset(&act.sa_mask);
  sigaction(SIGUSR1, &act, NULL);
}

static void *func(void *arg)
{
  register_signal_handler(handler);

  me = nthreads++;
  threads[me] = pthread_self();
  while (count[me] < 2);
  return NULL;
}

int main()
{
  int i;
  main_thread = pthread_self();
  for (i = 0; i < NTHREADS; i++) {
    threads[i] = main_thread;
    pooled_pthread_start(&func, NULL + i);
    while (pthread_equal(threads[i], main_thread));
  }
  for (i = 0; i < NTHREADS; i++) {
    printf("signaling thread %d\n", (int)threads[i]);
    pthread_kill(threads[i], SIGUSR1);
  }
  for (i = 0; i < NTHREADS; i++) {
    while (count[i] < 1);
  }
  for (i = 0; i < NTHREADS; i++) {
    printf("signaling thread %d\n", (int)threads[i]);
    pthread_kill(threads[i], SIGUSR1);
  }
  for (i = 0; i < NTHREADS; i++) {
    while (count[i] < 2);
  }

  return 0;
}
