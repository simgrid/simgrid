/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid's pthread interposer. Actual implementation of the symbols (see the comment in sthread.h) */

#include "smpi/smpi.h"
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
    if (std::string_view(envp[i]).rfind("SMPI_GLOBAL_SIZE", 0) == 0)
      return raw_main(argc, argv, envp);

  /* If not in SMPI, the old main becomes an actor in a newly created simulation */
  std::ostringstream id;
  id << std::this_thread::get_id();

  XBT_DEBUG("sthread main() is starting in thread %s", id.str().c_str());

  sg4::Engine e(&argc, argv);
  auto* zone = sg4::create_full_zone("world");
  lilibeth   = zone->create_host("Lilibeth", 1e15);
  zone->seal();

  /* Launch the user's main() on an actor */
  sthread_enable();
  sg4::ActorPtr main_actor = sg4::Actor::create("main thread", lilibeth, raw_main, argc, argv, envp);

  XBT_INFO("Starting the simulation.");
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

int sthread_mutex_init(sthread_mutex_t* mutex, const void* /*pthread_mutexattr_t* attr*/)
{
  auto m = sg4::Mutex::create();
  intrusive_ptr_add_ref(m.get());

  mutex->mutex = m.get();
  return 0;
}

int sthread_mutex_lock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  static_cast<sg4::Mutex*>(mutex->mutex)->lock();
  return 0;
}

int sthread_mutex_trylock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  return static_cast<sg4::Mutex*>(mutex->mutex)->try_lock();
}

int sthread_mutex_unlock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  static_cast<sg4::Mutex*>(mutex->mutex)->unlock();
  return 0;
}
int sthread_mutex_destroy(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  intrusive_ptr_release(static_cast<sg4::Mutex*>(mutex->mutex));
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

void sthread_sleep(double seconds)
{
  simgrid::s4u::this_actor::sleep_for(seconds);
}

#if 0
int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *cond_attr) {
    *cond = sg_cond_init();
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
	sg_cond_notify_one(*cond);
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
	sg_cond_notify_all(*cond);
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
	sg_cond_wait(*cond, *mutex);
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
	sg_cond_destroy(*cond);
    return 0;
}
#endif
