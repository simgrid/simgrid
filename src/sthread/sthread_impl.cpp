/* SimGrid's pthread interposer. Actual implementation of the symbols (see the comment in sthread.h) */

#include "smpi/smpi.h"
#include <simgrid/actor.h>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Mutex.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <xbt/base.h>
#include <xbt/sysdep.h>

#include "src/internal_config.h"
#include "src/sthread/sthread.h"

#include <dlfcn.h>
#include <pthread.h>
#include <semaphore.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

XBT_LOG_NEW_DEFAULT_CATEGORY(sthread, "pthread intercepter");
namespace sg4 = simgrid::s4u;

static sg4::Host* lilibeth = NULL;

int sthread_main(int argc, char** argv, char** envp, int (*raw_main)(int, char**, char**))
{
  std::ostringstream id;
  id << std::this_thread::get_id();

  XBT_INFO("sthread main() is starting in thread %s", id.str().c_str());

  sg4::Engine e(&argc, argv);
  auto* zone = sg4::create_full_zone("world");
  lilibeth   = zone->create_host("Lilibeth", 1e15);
  zone->seal();

  /* Launch the user's main() on an actor */
  sthread_enable();
  sg4::ActorPtr main_actor = sg4::Actor::create("tid 0", lilibeth, raw_main, argc, argv, envp);

  XBT_INFO("sthread main() is launching the simulation");
  sg4::Engine::get_instance()->run();

  return 0;
}

struct sthread_mutex {
  s4u_Mutex* mutex;
};

static void thread_create_wrapper(void* (*user_function)(void*), void* param)
{
#if HAVE_SMPI
  if (SMPI_is_inited())
    SMPI_thread_create();
#endif
  sthread_enable();
  user_function(param);
  sthread_disable();
}

int sthread_create(unsigned long int* thread, const /*pthread_attr_t*/ void* attr, void* (*start_routine)(void*),
                   void* arg)
{
  static int TID = 0;
  TID++;
  XBT_INFO("Create thread %d", TID);
  int rank = 0;
#if HAVE_SMPI
  if (SMPI_is_inited())
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
  char name[100];
  sprintf(name, "%d:%d", rank, TID);
  sg4::ActorPtr actor = sg4::Actor::init(name, lilibeth);
  actor->start(thread_create_wrapper, start_routine, arg);

  intrusive_ptr_add_ref(actor.get());
  *thread = reinterpret_cast<unsigned long>(actor.get());
  return 0;
}
int sthread_join(sthread_t thread, void** retval)
{
  sg4::ActorPtr actor(reinterpret_cast<sg4::Actor*>(thread));
  actor->join();
  intrusive_ptr_release(actor.get());

  return 0;
}

int sthread_mutex_init(sthread_mutex_t* mutex, const /*pthread_mutexattr_t*/ void* attr)
{
  auto m = sg4::Mutex::create();
  intrusive_ptr_add_ref(m.get());

  mutex->mutex = m.get();
  return 0;
}

int sthread_mutex_lock(sthread_mutex_t* mutex)
{
  static_cast<sg4::Mutex*>(mutex->mutex)->lock();
  return 0;
}

int sthread_mutex_trylock(sthread_mutex_t* mutex)
{
  return static_cast<sg4::Mutex*>(mutex->mutex)->try_lock();
}

int sthread_mutex_unlock(sthread_mutex_t* mutex)
{
  static_cast<sg4::Mutex*>(mutex->mutex)->unlock();
  return 0;
}
int sthread_mutex_destroy(sthread_mutex_t* mutex)
{
  intrusive_ptr_release(static_cast<sg4::Mutex*>(mutex->mutex));
  return 0;
}

#if 0
int sem_init(sem_t *sem, int pshared, unsigned int value) {
	int res;

	res=raw_sem_init(sem,pshared,value);
	return res;
}

int sem_wait(sem_t *sem) {
	int res;

	res = raw_sem_wait(sem);
	return res;
}

int sem_post(sem_t *sem) {
	return raw_sem_post(sem);
}

int pthread_join(pthread_t thread, void **retval) {
	sg_actor_join(thread, -1);
    return 0;
}

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
