/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid's pthread interposer. Actual implementation of the symbols (see the comment in sthread.h) */

#include "simgrid/s4u/Barrier.hpp"
#include "simgrid/s4u/ConditionVariable.hpp"
#include "smpi/smpi.h"
#include "xbt/asserts.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/string.hpp"
#include <simgrid/actor.h>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Mutex.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/s4u/Semaphore.hpp>
#include <xbt/base.h>
#include <xbt/sysdep.h>

#include "src/internal_config.h"
#include "src/sthread/sthread.h"

#include <cmath>
#include <dlfcn.h>
#include <pthread.h>
#include <semaphore.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string_view>
#include <thread>

XBT_LOG_NEW_DEFAULT_CATEGORY(sthread, "pthread intercepter");
namespace sg4 = simgrid::s4u;

static sg4::Host* lilibeth = nullptr;

int sthread_main(int argc, char** argv, char** envp, int (*raw_main)(int, char**, char**))
{
  /* Do not intercept the main when run from SMPI: it will initialize the simulation properly */
  for (int i = 0; envp[i] != nullptr; i++)
    if (std::string_view(envp[i]).rfind("SMPI_GLOBAL_SIZE", 0) == 0) {
      printf("sthread refuses to intercept the SMPI application %s directly, as its interception is done otherwise.\n",
             argv[0]);
      return raw_main(argc, argv, envp);
    }

  /* Do not intercept system binaries such as valgrind step 1 */
  std::vector<std::string> binaries = {"/usr/bin/valgrind.bin", "/bin/sh", "/bin/bash", "gdb", "addr2line"};
  for (int i = 0; envp[i] != nullptr; i++) {
    auto view = std::string_view(envp[i]);
    /* If you want to ignore more than one binary, export STHREAD_IGNORE_BINARY1=toto STHREAD_IGNORE_BINARY2=tutu */
    /* Note that this cannot be configured with --cfg because we are before the main() */
    if (view.rfind("STHREAD_IGNORE_BINARY", 0) == 0) {
      view.remove_prefix(std::min(view.rfind("=") + 1, view.size()));
      binaries.push_back(std::string(view));
    }
  }
  auto binary_view = std::string_view(argv[0]);
  for (auto binary : binaries) {
    if (binary_view.rfind(binary) != std::string_view::npos) {
      printf("sthread refuses to intercept the execution of %s. Running the application unmodified.\n", argv[0]);
      fflush(stdout);
      return raw_main(argc, argv, envp);
    }
  }

  /* If not in SMPI, the old main becomes an actor in a newly created simulation */
  printf("sthread is intercepting the execution of %s. If it's not what you want, export STHREAD_IGNORE_BINARY=%s\n",
         argv[0], argv[0]);
  fflush(stdout);

  sg4::Engine e(&argc, argv);
  auto* zone = sg4::create_full_zone("world");
  lilibeth   = zone->create_host("Lilibeth", 1e15);
  zone->seal();

  /* Launch the user's main() on an actor */
  sthread_enable();
  sg4::ActorPtr main_actor = sg4::Actor::create("main thread", lilibeth, raw_main, argc, argv, envp);

  sg4::Engine::get_instance()->run();
  sthread_disable();
  XBT_INFO("All threads exited. Terminating the simulation.");

  return 0;
}

struct sthread_mutex {
  s4u_Mutex* mutex;
};

int sthread_create(unsigned long int* thread, const void* /*pthread_attr_t* attr*/, void* (*start_routine)(void*),
                   void* arg)
{
  static int TID = 0;
  TID++;
  XBT_VERB("Create thread %d", TID);
  std::string name = std::string("thread ") + std::to_string(TID);
#if HAVE_SMPI
  if (SMPI_is_inited()) {
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    name = simgrid::xbt::string_printf("%d:%d", rank, TID);
  }
#endif
  sg4::ActorPtr actor = sg4::Actor::create(
      name, lilibeth,
      [](auto* user_function, auto* param) {
#if HAVE_SMPI
        if (SMPI_is_inited())
          SMPI_thread_create();
#endif
        sthread_enable();
        user_function(param);
        sthread_disable();
      },
      start_routine, arg);

  intrusive_ptr_add_ref(actor.get());
  *thread = reinterpret_cast<unsigned long>(actor.get());
  return 0;
}
int sthread_join(sthread_t thread, void** /*retval*/)
{
  sg4::ActorPtr actor(reinterpret_cast<sg4::Actor*>(thread));
  actor->join();
  intrusive_ptr_release(actor.get());

  return 0;
}

int sthread_mutexattr_init(sthread_mutexattr_t* attr)
{
  memset(attr, 0, sizeof(*attr));
  return 0;
}
int sthread_mutexattr_settype(sthread_mutexattr_t* attr, int type)
{
  switch (type) {
    case PTHREAD_MUTEX_NORMAL:
      xbt_assert(not attr->recursive, "S4U does not allow to remove the recursivness of a mutex.");
      attr->recursive = 0;
      break;
    case PTHREAD_MUTEX_RECURSIVE:
      attr->recursive = 1;
      attr->errorcheck = 0; // reset
      break;
    case PTHREAD_MUTEX_ERRORCHECK:
      attr->errorcheck = 1;
      THROW_UNIMPLEMENTED;
      break;
    default:
      THROW_IMPOSSIBLE;
  }
  return 0;
}
int sthread_mutexattr_gettype(const sthread_mutexattr_t* attr, int* type)
{
  if (attr->recursive)
    *type = PTHREAD_MUTEX_RECURSIVE;
  else if (attr->errorcheck)
    *type = PTHREAD_MUTEX_ERRORCHECK;
  else
    *type = PTHREAD_MUTEX_NORMAL;
  return 0;
}
int sthread_mutexattr_getrobust(const sthread_mutexattr_t* attr, int* robustness)
{
  *robustness = attr->robust;
  return 0;
}
int sthread_mutexattr_setrobust(sthread_mutexattr_t* attr, int robustness)
{
  attr->robust = robustness;
  if (robustness)
    THROW_UNIMPLEMENTED;
  return 0;
}

int sthread_mutex_init(sthread_mutex_t* mutex, const sthread_mutexattr_t* attr)
{
  auto m = sg4::Mutex::create(attr != nullptr && attr->recursive);
  intrusive_ptr_add_ref(m.get());

  mutex->mutex = m.get();
  return 0;
}

int sthread_mutex_lock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  XBT_DEBUG("%s(%p)", __func__, mutex);
  static_cast<sg4::Mutex*>(mutex->mutex)->lock();
  return 0;
}

int sthread_mutex_trylock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  XBT_DEBUG("%s(%p)", __func__, mutex);
  if (static_cast<sg4::Mutex*>(mutex->mutex)->try_lock())
    return 0;
  return EBUSY;
}

int sthread_mutex_unlock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  XBT_DEBUG("%s(%p)", __func__, mutex);
  static_cast<sg4::Mutex*>(mutex->mutex)->unlock();
  return 0;
}
int sthread_mutex_destroy(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  XBT_DEBUG("%s(%p)", __func__, mutex);
  intrusive_ptr_release(static_cast<sg4::Mutex*>(mutex->mutex));
  return 0;
}

int sthread_barrier_init(sthread_barrier_t* barrier, const sthread_barrierattr_t* attr, unsigned count){
  auto b = sg4::Barrier::create(count);
  intrusive_ptr_add_ref(b.get());

  barrier->barrier = b.get();
  return 0;
}
int sthread_barrier_wait(sthread_barrier_t* barrier){
  XBT_DEBUG("%s(%p)", __func__, barrier);
  static_cast<sg4::Barrier*>(barrier->barrier)->wait();
  return 0;
}
int sthread_barrier_destroy(sthread_barrier_t* barrier){
  XBT_DEBUG("%s(%p)", __func__, barrier);
  intrusive_ptr_release(static_cast<sg4::Barrier*>(barrier->barrier));
  return 0;
}

int sthread_cond_init(sthread_cond_t* cond, sthread_condattr_t* attr)
{
  auto cv = sg4::ConditionVariable::create();
  intrusive_ptr_add_ref(cv.get());

  cond->cond = cv.get();
  cond->mutex = nullptr;
  return 0;
}
int sthread_cond_signal(sthread_cond_t* cond)
{
  XBT_DEBUG("%s(%p)", __func__, cond);

  if (cond->mutex == nullptr)
    XBT_WARN("No mutex was associated so far with condition variable %p. Safety checks skipped.", cond);
  else {
    auto* owner = static_cast<sg4::Mutex*>(cond->mutex)->get_owner();
    if (owner == nullptr)
      XBT_WARN("The mutex associated to condition %p is not currently owned by anyone when calling "
               "pthread_cond_signal(). The signal could get lost.",
               cond);
    else if (owner != simgrid::s4u::Actor::self())
      XBT_WARN("The mutex associated to condition %p is currently owned by %s, not by the thread currently calling "
               "calling pthread_cond_signal(). The signal could get lost.",
               cond, owner->get_cname());
  }

  static_cast<sg4::ConditionVariable*>(cond->cond)->notify_one();
  return 0;
}
int sthread_cond_broadcast(sthread_cond_t* cond)
{
  XBT_DEBUG("%s(%p)", __func__, cond);

  if (cond->mutex == nullptr)
    XBT_WARN("No mutex was associated so far with condition variable %p. Safety checks skipped.", cond);
  else {
    auto* owner = static_cast<sg4::Mutex*>(cond->mutex)->get_owner();
    if (owner == nullptr)
      XBT_WARN("The mutex associated to condition %p is not currently owned by anyone when calling "
               "pthread_cond_broadcast(). The signal could get lost.",
               cond);
    else if (owner != simgrid::s4u::Actor::self())
      XBT_WARN("The mutex associated to condition %p is currently owned by %s, not by the thread currently calling "
               "calling pthread_cond_broadcast(). The signal could get lost.",
               cond, owner->get_cname());
  }

  static_cast<sg4::ConditionVariable*>(cond->cond)->notify_all();
  return 0;
}
int sthread_cond_wait(sthread_cond_t* cond, sthread_mutex_t* mutex)
{
  XBT_DEBUG("%s(%p)", __func__, cond);

  if (cond->mutex == nullptr)
    cond->mutex = mutex->mutex;
  else if (cond->mutex != mutex->mutex)
    XBT_WARN("The condition %p is now waited with mutex %p while it was previoulsy waited with mutex %p. sthread may "
             "not work with such a dangerous code.",
             cond, cond->mutex, mutex->mutex);

  static_cast<sg4::ConditionVariable*>(cond->cond)->wait(static_cast<sg4::Mutex*>(mutex->mutex));
  return 0;
}
int sthread_cond_timedwait(sthread_cond_t* cond, sthread_mutex_t* mutex, const struct timespec* abs_timeout)
{
  XBT_DEBUG("%s(%p)", __func__, cond);

  if (cond->mutex == nullptr)
    cond->mutex = mutex->mutex;
  else if (cond->mutex != mutex->mutex)
    XBT_WARN("The condition %p is now waited with mutex %p while it was previoulsy waited with mutex %p. sthread may "
             "not work with such a dangerous code.",
             cond, cond->mutex, mutex->mutex);

  THROW_UNIMPLEMENTED;
}
int sthread_cond_destroy(sthread_cond_t* cond)
{
  XBT_DEBUG("%s(%p)", __func__, cond);
  intrusive_ptr_release(static_cast<sg4::ConditionVariable*>(cond->cond));
  return 0;
}

int sthread_sem_init(sthread_sem_t* sem, int /*pshared*/, unsigned int value)
{
  auto s = sg4::Semaphore::create(value);
  intrusive_ptr_add_ref(s.get());

  sem->sem = s.get();
  return 0;
}
int sthread_sem_destroy(sthread_sem_t* sem)
{
  intrusive_ptr_release(static_cast<sg4::Semaphore*>(sem->sem));
  return 0;
}
int sthread_sem_post(sthread_sem_t* sem)
{
  static_cast<sg4::Semaphore*>(sem->sem)->release();
  return 0;
}
int sthread_sem_wait(sthread_sem_t* sem)
{
  static_cast<sg4::Semaphore*>(sem->sem)->acquire();
  return 0;
}
int sthread_sem_trywait(sthread_sem_t* sem)
{
  auto* s = static_cast<sg4::Semaphore*>(sem->sem);
  if (s->would_block()) {
    errno = EAGAIN;
    return -1;
  }
  s->acquire();
  return 0;
}
int sthread_sem_timedwait(sthread_sem_t* sem, const struct timespec* abs_timeout)
{
  if (static_cast<sg4::Semaphore*>(sem->sem)->acquire_timeout(static_cast<double>(abs_timeout->tv_sec) +
                                                              static_cast<double>(abs_timeout->tv_nsec) / 1E9)) {
    errno = ETIMEDOUT;
    return -1;
  }
  return 0;
}

int sthread_gettimeofday(struct timeval* tv)
{
  if (tv) {
    double now   = simgrid::s4u::Engine::get_clock();
    double secs  = trunc(now);
    double usecs = (now - secs) * 1e6;
    tv->tv_sec   = static_cast<time_t>(secs);
    tv->tv_usec  = static_cast<decltype(tv->tv_usec)>(usecs); // suseconds_t
  }
  return 0;
}

unsigned int sthread_sleep(double seconds)
{
  XBT_DEBUG("sleep(%lf)", seconds);
  simgrid::s4u::this_actor::sleep_for(seconds);
  return 0;
}
int sthread_usleep(double seconds)
{
  XBT_DEBUG("sleep(%lf)", seconds);
  simgrid::s4u::this_actor::sleep_for(seconds);
  return 0;
}
