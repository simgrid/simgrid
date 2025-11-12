/* Copyright (c) 2002-2025. The SimGrid Team. All rights reserved.          */

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
#include <cerrno>
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

extern "C" {
// These functions are mostly useful in sthread.c but they must be defined in libsimgrid for it to compile (although
// they are not used in libsimgrid) We used to have 2 definitions of these functions, one useful in libsthread and one
// placeholder in libsimgrid, but this proved to be fragile: it broke on several CI builders in 2025 when LTO and
// optimizations were actived. That is why this code is now in this file, which is always in libsimgrid even if it's
// mostly unused from here. Only sthread_is_initialized() is used here and there to detect that sthread is or is not
// loaded in memory, to adapt the library initialization code.
static thread_local int sthread_inside_simgrid = 1;
void sthread_enable(void)
{ // Start intercepting all pthread calls
  sthread_inside_simgrid = 0;
}
void sthread_disable(void)
{ // Stop intercepting all pthread calls
  sthread_inside_simgrid = 1;
}
static int sthread_inited = 0;
void sthread_do_initialize()
{
  sthread_inited = 1;
}
int sthread_is_initialized()
{
  return sthread_inited;
}
int sthread_is_enabled(void)
{ // Returns whether sthread is currenctly active
  return sthread_inside_simgrid == 0;
}
}

XBT_LOG_NEW_DEFAULT_CATEGORY(sthread, "pthread intercepter");
namespace sg4 = simgrid::s4u;

static sg4::Host* lilibeth = nullptr;

static bool sthread_quiet = false;

int sthread_main(int argc, char** argv, char** envp, int (*raw_main)(int, char**, char**))
{
  /* Check whether we should keep silent about non-error situations */
  for (int i = 0; envp[i] != nullptr; i++)
    if (std::string_view(envp[i]).rfind("STHREAD_QUIET", 0) == 0)
      sthread_quiet = true;

  /* Do not initialize the simulation world when running from SMPI (but still intercepts pthread calls) */
  for (int i = 0; envp[i] != nullptr; i++)
    if (std::string_view(envp[i]).rfind("SMPI_GLOBAL_SIZE", 0) == 0) {
      if (not sthread_quiet)
        printf("sthread intercepts pthreads in your SMPI application.\n");
      return raw_main(argc, argv, envp);
    }

  /* Do not intercept system binaries such as valgrind step 1 */
  std::vector<std::string> binaries = {"/usr/bin/env",
                                       "/usr/bin/valgrind.bin",
                                       "/usr/bin/python3",
                                       "/bin/sh",
                                       "/bin/bash",
                                       "addr2line",
                                       "cat",
                                       "dirname",
                                       "gdb",
                                       "grep",
                                       "ls",
                                       "ltrace",
                                       "make",
                                       "md5sum",
                                       "mktemp",
                                       "rm",
                                       "sed",
                                       "sh",
                                       "strace",
                                       "simgrid-mc",
                                       "wc"};
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
    // FIXME in C++20, we could use `binary_view.ends_with(binary)`
    if (binary_view.size() >= binary.size() &&
        binary_view.compare(binary_view.size() - binary.size(), binary.size(), binary) == 0) {
      return raw_main(argc, argv, envp);
    }
  }

  if (not sthread_quiet) {
    fprintf(stderr,
            "sthread is intercepting the execution of %s. If it's not what you want, export STHREAD_IGNORE_BINARY=%s\n",
            argv[0], argv[0]);
  }

  sg4::Engine e(&argc, argv);
  auto* zone = e.get_netzone_root();
  lilibeth   = zone->add_host("Lilibeth", 1e15);
  zone->seal();

  /* If not in SMPI, the old main becomes an actor in a newly created simulation. Do not activate sthread yet: creating
   * contextes won't like it */
  sg4::ActorPtr main_actor = lilibeth->add_actor("main thread", raw_main, argc, argv, envp);
  sg4::Engine::get_instance()->run();

  if (not sthread_quiet)
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
  if (lilibeth == nullptr) {
    auto* zone = simgrid::s4u::Engine::get_instance()->get_netzone_root();
    for (auto* h : zone->get_all_hosts())
      if (h->get_name() == "Lilibeth")
        lilibeth = h;
    if (lilibeth == nullptr) { // Still not found. Let's create it
      zone->unseal();
      lilibeth = zone->add_host("Lilibeth", 1e15);
      zone->seal();
    }
    xbt_assert(lilibeth, "The host Lilibeth was not created. Something's wrong in sthread initialization.");
  }
#endif
  sg4::ActorPtr actor = 
    lilibeth->add_actor(name, [](auto* user_function, auto* param) {
#if HAVE_SMPI
        if (SMPI_is_inited())
          SMPI_thread_create();
#endif
        sthread_enable();
        auto* ret = user_function(param);
        sthread_disable();
        sthread_exit(ret);
      },
      start_routine, arg);

  intrusive_ptr_add_ref(actor.get());
  *thread = reinterpret_cast<unsigned long>(actor.get());
  return 0;
}
int sthread_detach(sthread_t thread)
{
  return 0;
}
int sthread_join(sthread_t thread, void** retval)
{
  sg4::ActorPtr actor(reinterpret_cast<sg4::Actor*>(thread));
  actor->join();
  if (retval)
    *retval = actor->get_data<void>();
  intrusive_ptr_release(actor.get());

  return 0;
}
void sthread_exit(void* retval)
{
  if (retval)
    simgrid::s4u::Actor::self()->set_data(retval);
  simgrid::s4u::this_actor::exit();
}

int sthread_mutexattr_init(sthread_mutexattr_t* attr)
{
  memset(attr, 0, sizeof(*attr));
  return 0;
}
int sthread_mutexattr_settype(sthread_mutexattr_t* attr, int type)
{
  switch (type) {
    default: // Let's assume that unknown mutex policies behave as normal ones. That's somehow true for ADAPTIVE ones
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
      break;
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
  mutex->errorcheck = attr ? attr->errorcheck : false;

  return 0;
}

int sthread_mutex_lock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  XBT_DEBUG("%s(%p)", __func__, (xbt_log_no_loc ? (void*)0xDEADBEEF : mutex));
  if (mutex->errorcheck && static_cast<sg4::Mutex*>(mutex->mutex)->get_owner() != nullptr &&
      static_cast<sg4::Mutex*>(mutex->mutex)->get_owner()->get_pid() == simgrid::s4u::this_actor::get_pid())
    return EDEADLK;
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

  XBT_DEBUG("%s(%p)", __func__, (xbt_log_no_loc ? (void*)0xDEADBEEF : mutex));
  if (mutex->errorcheck &&
      (static_cast<sg4::Mutex*>(mutex->mutex)->get_owner() == nullptr ||
       static_cast<sg4::Mutex*>(mutex->mutex)->get_owner()->get_pid() != simgrid::s4u::this_actor::get_pid()))
    return EPERM;
  static_cast<sg4::Mutex*>(mutex->mutex)->unlock();
  return 0;
}
int sthread_mutex_destroy(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  xbt_assert(static_cast<sg4::Mutex*>(mutex->mutex)->get_owner() == nullptr,
             "Destroying a mutex that is still owned is UB. See https://cwe.mitre.org/data/definitions/667.html");

  XBT_DEBUG("%s(%p)", __func__, (xbt_log_no_loc ? (void*)0xDEADBEEF : mutex));
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

  /* At least in glibc, PTHREAD_COND_INITIALIZER sets every fields to 0 */
  if (cond->cond == nullptr)
    sthread_cond_init(cond, nullptr);

  if (cond->mutex != nullptr) {
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

  /* At least in glibc, PTHREAD_COND_INITIALIZER sets every fields to 0 */
  if (cond->cond == nullptr)
    sthread_cond_init(cond, nullptr);

  if (cond->mutex != nullptr) {
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

  /* At least in glibc, PTHREAD_COND_INITIALIZER sets every fields to 0 */
  if (cond->cond == nullptr)
    sthread_cond_init(cond, nullptr);

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

  std::cv_status res = static_cast<sg4::ConditionVariable*>(cond->cond)
                           ->wait_until(static_cast<sg4::Mutex*>(mutex->mutex),
                                        abs_timeout->tv_sec + ((double)abs_timeout->tv_nsec) / 1000000);
  if (res == std::cv_status::timeout)
    return ETIMEDOUT;
  return 0;
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

time_t sthread_time(time_t*)
{
  return trunc(simgrid::s4u::Engine::get_clock());
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
