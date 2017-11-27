/* A thread pool (C++ version).                                             */

/* Copyright (c) 2004-2017 The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_PARMAP_HPP
#define XBT_PARMAP_HPP

#include "src/internal_config.h" // HAVE_FUTEX_H
#include "src/kernel/context/Context.hpp"
#include <atomic>
#include <boost/optional.hpp>
#include <simgrid/simix.h>
#include <vector>
#include <xbt/log.h>
#include <xbt/parmap.h>
#include <xbt/xbt_os_thread.h>

#if HAVE_FUTEX_H
#include <limits>
#include <linux/futex.h>
#include <sys/syscall.h>
#endif

XBT_LOG_EXTERNAL_CATEGORY(xbt_parmap);

namespace simgrid {
namespace xbt {

/** \addtogroup XBT_parmap
  * \ingroup XBT_misc
  * \brief Parallel map class
  * \{
  */
template <typename T> class Parmap {
public:
  Parmap(unsigned num_workers, e_xbt_parmap_mode_t mode);
  Parmap(const Parmap&) = delete;
  ~Parmap();
  void apply(void (*fun)(T), const std::vector<T>& data);
  boost::optional<T> next();

private:
  enum Flag { PARMAP_WORK, PARMAP_DESTROY };

  /**
   * \brief Thread data transmission structure
   */
  class ThreadData {
  public:
    ThreadData(Parmap<T>& parmap, int id) : parmap(parmap), worker_id(id) {}
    Parmap<T>& parmap;
    int worker_id;
  };

  /**
   * \brief Synchronization object (different specializations).
   */
  class Synchro {
  public:
    explicit Synchro(Parmap<T>& parmap) : parmap(parmap) {}
    virtual ~Synchro() = default;
    /**
     * \brief Wakes all workers and waits for them to finish the tasks.
     *
     * This function is called by the controller thread.
     */
    virtual void master_signal() = 0;
    /**
     * \brief Starts the parmap: waits for all workers to be ready and returns.
     *
     * This function is called by the controller thread.
     */
    virtual void master_wait() = 0;
    /**
     * \brief Ends the parmap: wakes the controller thread when all workers terminate.
     *
     * This function is called by all worker threads when they end (not including the controller).
     */
    virtual void worker_signal() = 0;
    /**
     * \brief Waits for some work to process.
     *
     * This function is called by each worker thread (not including the controller) when it has no more work to do.
     *
     * \param round  the expected round number
     */
    virtual void worker_wait(unsigned) = 0;

    Parmap<T>& parmap;
  };

  class PosixSynchro : public Synchro {
  public:
    explicit PosixSynchro(Parmap<T>& parmap);
    ~PosixSynchro();
    void master_signal();
    void master_wait();
    void worker_signal();
    void worker_wait(unsigned round);

  private:
    xbt_os_cond_t ready_cond;
    xbt_os_mutex_t ready_mutex;
    xbt_os_cond_t done_cond;
    xbt_os_mutex_t done_mutex;
  };

#if HAVE_FUTEX_H
  class FutexSynchro : public Synchro {
  public:
    explicit FutexSynchro(Parmap<T>& parmap) : Synchro(parmap) {}
    void master_signal();
    void master_wait();
    void worker_signal();
    void worker_wait(unsigned);

  private:
    static void futex_wait(unsigned* uaddr, unsigned val);
    static void futex_wake(unsigned* uaddr, unsigned val);
  };
#endif

  class BusyWaitSynchro : public Synchro {
  public:
    explicit BusyWaitSynchro(Parmap<T>& parmap) : Synchro(parmap) {}
    void master_signal();
    void master_wait();
    void worker_signal();
    void worker_wait(unsigned);
  };

  static void* worker_main(void* arg);
  Synchro* new_synchro(e_xbt_parmap_mode_t mode);
  void work();

  Flag status;              /**< is the parmap active or being destroyed? */
  unsigned work_round;      /**< index of the current round */
  xbt_os_thread_t* workers; /**< worker thread handlers */
  unsigned num_workers;     /**< total number of worker threads including the controller */
  Synchro* synchro;         /**< synchronization object */

  unsigned thread_counter    = 0;       /**< number of workers that have done the work */
  void (*fun)(const T)       = nullptr; /**< function to run in parallel on each element of data */
  const std::vector<T>* data = nullptr; /**< parameters to pass to fun in parallel */
  std::atomic<unsigned> index;          /**< index of the next element of data to pick */
};

/**
 * \brief Creates a parallel map object
 * \param num_workers number of worker threads to create
 * \param mode how to synchronize the worker threads
 */
template <typename T> Parmap<T>::Parmap(unsigned num_workers, e_xbt_parmap_mode_t mode)
{
  XBT_CDEBUG(xbt_parmap, "Create new parmap (%u workers)", num_workers);

  /* Initialize the thread pool data structure */
  this->status      = PARMAP_WORK;
  this->work_round  = 0;
  this->workers     = new xbt_os_thread_t[num_workers];
  this->num_workers = num_workers;
  this->synchro     = new_synchro(mode);

  /* Create the pool of worker threads */
  this->workers[0] = nullptr;
#if HAVE_PTHREAD_SETAFFINITY
  int core_bind = 0;
#endif
  for (unsigned i = 1; i < num_workers; i++) {
    ThreadData* data = new ThreadData(*this, i);
    this->workers[i] = xbt_os_thread_create(nullptr, worker_main, data, nullptr);
#if HAVE_PTHREAD_SETAFFINITY
    xbt_os_thread_bind(this->workers[i], core_bind);
    if (core_bind != xbt_os_get_numcores() - 1)
      core_bind++;
    else
      core_bind = 0;
#endif
  }
}

/**
 * \brief Destroys a parmap
 */
template <typename T> Parmap<T>::~Parmap()
{
  status = PARMAP_DESTROY;
  synchro->master_signal();

  for (unsigned i = 1; i < num_workers; i++)
    xbt_os_thread_join(workers[i], nullptr);

  delete[] workers;
  delete synchro;
}

/**
 * \brief Applies a list of tasks in parallel.
 * \param fun the function to call in parallel
 * \param data each element of this vector will be passed as an argument to fun
 */
template <typename T> void Parmap<T>::apply(void (*fun)(T), const std::vector<T>& data)
{
  /* Assign resources to worker threads (we are maestro here)*/
  this->fun   = fun;
  this->data  = &data;
  this->index = 0;
  this->synchro->master_signal(); // maestro runs futex_wake to wake all the minions (the working threads)
  this->work();                   // maestro works with its minions
  this->synchro->master_wait();   // When there is no more work to do, then maestro waits for the last minion to stop
  XBT_CDEBUG(xbt_parmap, "Job done"); //   ... and proceeds
}

/**
 * \brief Returns a next task to process.
 *
 * Worker threads call this function to get more work.
 *
 * \return the next task to process, or throws a std::out_of_range exception if there is no more work
 */
template <typename T> boost::optional<T> Parmap<T>::next()
{
  unsigned index = this->index.fetch_add(1, std::memory_order_relaxed);
  if (index < this->data->size())
    return (*this->data)[index];
  else
    return boost::none;
}

/**
 * \brief Main work loop: applies fun to elements in turn.
 */
template <typename T> void Parmap<T>::work()
{
  unsigned length = this->data->size();
  unsigned index  = this->index.fetch_add(1, std::memory_order_relaxed);
  while (index < length) {
    this->fun((*this->data)[index]);
    index = this->index.fetch_add(1, std::memory_order_relaxed);
  }
}

/**
 * Get a synchronization object for given mode.
 * \param mode the synchronization mode
 */
template <typename T> typename Parmap<T>::Synchro* Parmap<T>::new_synchro(e_xbt_parmap_mode_t mode)
{
  if (mode == XBT_PARMAP_DEFAULT) {
#if HAVE_FUTEX_H
    mode = XBT_PARMAP_FUTEX;
#else
    mode = XBT_PARMAP_POSIX;
#endif
  }
  Synchro* res;
  switch (mode) {
    case XBT_PARMAP_POSIX:
      res = new PosixSynchro(*this);
      break;
    case XBT_PARMAP_FUTEX:
#if HAVE_FUTEX_H
      res = new FutexSynchro(*this);
#else
      xbt_die("Futex is not available on this OS.");
#endif
      break;
    case XBT_PARMAP_BUSY_WAIT:
      res = new BusyWaitSynchro(*this);
      break;
    default:
      THROW_IMPOSSIBLE;
  }
  return res;
}

/**
 * \brief Main function of a worker thread.
 */
template <typename T> void* Parmap<T>::worker_main(void* arg)
{
  ThreadData* data      = static_cast<ThreadData*>(arg);
  Parmap<T>& parmap     = data->parmap;
  unsigned round        = 0;
  smx_context_t context = SIMIX_context_new(std::function<void()>(), nullptr, nullptr);
  SIMIX_context_set_current(context);

  XBT_CDEBUG(xbt_parmap, "New worker thread created");

  /* Worker's main loop */
  while (1) {
    round++;
    parmap.synchro->worker_wait(round);
    if (parmap.status == PARMAP_DESTROY)
      break;

    XBT_CDEBUG(xbt_parmap, "Worker %d got a job", data->worker_id);
    parmap.work();
    parmap.synchro->worker_signal();
    XBT_CDEBUG(xbt_parmap, "Worker %d has finished", data->worker_id);
  }
  /* We are destroying the parmap */
  delete context;
  delete data;
  return nullptr;
}

template <typename T> Parmap<T>::PosixSynchro::PosixSynchro(Parmap<T>& parmap) : Synchro(parmap)
{
  ready_cond  = xbt_os_cond_init();
  ready_mutex = xbt_os_mutex_init();
  done_cond   = xbt_os_cond_init();
  done_mutex  = xbt_os_mutex_init();
}

template <typename T> Parmap<T>::PosixSynchro::~PosixSynchro()
{
  xbt_os_cond_destroy(ready_cond);
  xbt_os_mutex_destroy(ready_mutex);
  xbt_os_cond_destroy(done_cond);
  xbt_os_mutex_destroy(done_mutex);
}

template <typename T> void Parmap<T>::PosixSynchro::master_signal()
{
  xbt_os_mutex_acquire(ready_mutex);
  this->parmap.thread_counter = 1;
  this->parmap.work_round++;
  /* wake all workers */
  xbt_os_cond_broadcast(ready_cond);
  xbt_os_mutex_release(ready_mutex);
}

template <typename T> void Parmap<T>::PosixSynchro::master_wait()
{
  xbt_os_mutex_acquire(done_mutex);
  while (this->parmap.thread_counter < this->parmap.num_workers) {
    /* wait for all workers to be ready */
    xbt_os_cond_wait(done_cond, done_mutex);
  }
  xbt_os_mutex_release(done_mutex);
}

template <typename T> void Parmap<T>::PosixSynchro::worker_signal()
{
  xbt_os_mutex_acquire(done_mutex);
  this->parmap.thread_counter++;
  if (this->parmap.thread_counter == this->parmap.num_workers) {
    /* all workers have finished, wake the controller */
    xbt_os_cond_signal(done_cond);
  }
  xbt_os_mutex_release(done_mutex);
}

template <typename T> void Parmap<T>::PosixSynchro::worker_wait(unsigned round)
{
  xbt_os_mutex_acquire(ready_mutex);
  /* wait for more work */
  while (this->parmap.work_round != round) {
    xbt_os_cond_wait(ready_cond, ready_mutex);
  }
  xbt_os_mutex_release(ready_mutex);
}

#if HAVE_FUTEX_H
template <typename T> inline void Parmap<T>::FutexSynchro::futex_wait(unsigned* uaddr, unsigned val)
{
  XBT_CVERB(xbt_parmap, "Waiting on futex %p", uaddr);
  syscall(SYS_futex, uaddr, FUTEX_WAIT_PRIVATE, val, nullptr, nullptr, 0);
}

template <typename T> inline void Parmap<T>::FutexSynchro::futex_wake(unsigned* uaddr, unsigned val)
{
  XBT_CVERB(xbt_parmap, "Waking futex %p", uaddr);
  syscall(SYS_futex, uaddr, FUTEX_WAKE_PRIVATE, val, nullptr, nullptr, 0);
}

template <typename T> void Parmap<T>::FutexSynchro::master_signal()
{
  __atomic_store_n(&this->parmap.thread_counter, 1, __ATOMIC_SEQ_CST);
  __atomic_add_fetch(&this->parmap.work_round, 1, __ATOMIC_SEQ_CST);
  /* wake all workers */
  futex_wake(&this->parmap.work_round, std::numeric_limits<int>::max());
}

template <typename T> void Parmap<T>::FutexSynchro::master_wait()
{
  unsigned count = __atomic_load_n(&this->parmap.thread_counter, __ATOMIC_SEQ_CST);
  while (count < this->parmap.num_workers) {
    /* wait for all workers to be ready */
    futex_wait(&this->parmap.thread_counter, count);
    count = __atomic_load_n(&this->parmap.thread_counter, __ATOMIC_SEQ_CST);
  }
}

template <typename T> void Parmap<T>::FutexSynchro::worker_signal()
{
  unsigned count = __atomic_add_fetch(&this->parmap.thread_counter, 1, __ATOMIC_SEQ_CST);
  if (count == this->parmap.num_workers) {
    /* all workers have finished, wake the controller */
    futex_wake(&this->parmap.thread_counter, std::numeric_limits<int>::max());
  }
}

template <typename T> void Parmap<T>::FutexSynchro::worker_wait(unsigned round)
{
  unsigned work_round = __atomic_load_n(&this->parmap.work_round, __ATOMIC_SEQ_CST);
  /* wait for more work */
  while (work_round != round) {
    futex_wait(&this->parmap.work_round, work_round);
    work_round = __atomic_load_n(&this->parmap.work_round, __ATOMIC_SEQ_CST);
  }
}
#endif

template <typename T> void Parmap<T>::BusyWaitSynchro::master_signal()
{
  __atomic_store_n(&this->parmap.thread_counter, 1, __ATOMIC_SEQ_CST);
  __atomic_add_fetch(&this->parmap.work_round, 1, __ATOMIC_SEQ_CST);
}

template <typename T> void Parmap<T>::BusyWaitSynchro::master_wait()
{
  while (__atomic_load_n(&this->parmap.thread_counter, __ATOMIC_SEQ_CST) < this->parmap.num_workers) {
    xbt_os_thread_yield();
  }
}

template <typename T> void Parmap<T>::BusyWaitSynchro::worker_signal()
{
  __atomic_add_fetch(&this->parmap.thread_counter, 1, __ATOMIC_SEQ_CST);
}

template <typename T> void Parmap<T>::BusyWaitSynchro::worker_wait(unsigned round)
{
  /* wait for more work */
  while (__atomic_load_n(&this->parmap.work_round, __ATOMIC_SEQ_CST) != round) {
    xbt_os_thread_yield();
  }
}

/** \} */
}
}

#endif
