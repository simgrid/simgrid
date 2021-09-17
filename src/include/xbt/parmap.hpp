/* A thread pool (C++ version).                                             */

/* Copyright (c) 2004-2021 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_PARMAP_HPP
#define XBT_PARMAP_HPP

#include "src/internal_config.h" // HAVE_FUTEX_H
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/context/Context.hpp"

#include <boost/optional.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

#if HAVE_FUTEX_H
#include <linux/futex.h>
#include <sys/syscall.h>
#endif

#if HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif

XBT_LOG_EXTERNAL_CATEGORY(xbt_parmap);

namespace simgrid {
namespace xbt {

/** @addtogroup XBT_parmap
 * @ingroup XBT_misc
 * @brief Parallel map class
 * @{
 */
template <typename T> class Parmap {
public:
  Parmap(unsigned num_workers, e_xbt_parmap_mode_t mode);
  Parmap(const Parmap&) = delete;
  Parmap& operator=(const Parmap&) = delete;
  ~Parmap();
  void apply(std::function<void(T)>&& fun, const std::vector<T>& data);
  boost::optional<T> next();

private:
  /**
   * @brief Thread data transmission structure
   */
  class ThreadData {
  public:
    ThreadData(Parmap<T>& parmap, int id) : parmap(parmap), worker_id(id) {}
    Parmap<T>& parmap;
    int worker_id;
  };

  /**
   * @brief Synchronization object (different specializations).
   */
  class Synchro {
  public:
    explicit Synchro(Parmap<T>& parmap) : parmap(parmap) {}
    virtual ~Synchro() = default;
    /**
     * @brief Wakes all workers and waits for them to finish the tasks.
     *
     * This function is called by the controller thread.
     */
    virtual void master_signal() = 0;
    /**
     * @brief Starts the parmap: waits for all workers to be ready and returns.
     *
     * This function is called by the controller thread.
     */
    virtual void master_wait() = 0;
    /**
     * @brief Ends the parmap: wakes the controller thread when all workers terminate.
     *
     * This function is called by all worker threads when they end (not including the controller).
     */
    virtual void worker_signal() = 0;
    /**
     * @brief Waits for some work to process.
     *
     * This function is called by each worker thread (not including the controller) when it has no more work to do.
     *
     * @param round  the expected round number
     */
    virtual void worker_wait(unsigned) = 0;

    Parmap<T>& parmap;
  };

  class PosixSynchro : public Synchro {
  public:
    explicit PosixSynchro(Parmap<T>& parmap) : Synchro(parmap) {}
    void master_signal() override;
    void master_wait() override;
    void worker_signal() override;
    void worker_wait(unsigned round) override;

  private:
    std::condition_variable ready_cond;
    std::mutex ready_mutex;
    std::condition_variable done_cond;
    std::mutex done_mutex;
  };

#if HAVE_FUTEX_H
  class FutexSynchro : public Synchro {
  public:
    explicit FutexSynchro(Parmap<T>& parmap) : Synchro(parmap) {}
    void master_signal() override;
    void master_wait() override;
    void worker_signal() override;
    void worker_wait(unsigned) override;

  private:
    static void futex_wait(std::atomic_uint* uaddr, unsigned val);
    static void futex_wake(std::atomic_uint* uaddr, unsigned val);
  };
#endif

  class BusyWaitSynchro : public Synchro {
  public:
    explicit BusyWaitSynchro(Parmap<T>& parmap) : Synchro(parmap) {}
    void master_signal() override;
    void master_wait() override;
    void worker_signal() override;
    void worker_wait(unsigned) override;
  };

  static void worker_main(ThreadData* data);
  Synchro* new_synchro(e_xbt_parmap_mode_t mode);
  void work();

  bool destroying = false;           /**< is the parmap being destroyed? */
  std::atomic_uint work_round{0};    /**< index of the current round */
  std::vector<std::thread*> workers; /**< worker thread handlers */
  unsigned num_workers;     /**< total number of worker threads including the controller */
  Synchro* synchro;         /**< synchronization object */

  std::atomic_uint thread_counter{0};   /**< number of workers that have done the work */
  std::function<void(T)> fun;           /**< function to run in parallel on each element of data */
  const std::vector<T>* data = nullptr; /**< parameters to pass to fun in parallel */
  std::atomic_uint index{0};            /**< index of the next element of data to pick */
};

/**
 * @brief Creates a parallel map object
 * @param num_workers number of worker threads to create
 * @param mode how to synchronize the worker threads
 */
template <typename T> Parmap<T>::Parmap(unsigned num_workers, e_xbt_parmap_mode_t mode)
{
  XBT_CDEBUG(xbt_parmap, "Create new parmap (%u workers)", num_workers);

  /* Initialize the thread pool data structure */
  this->workers.resize(num_workers);
  this->num_workers = num_workers;
  this->synchro     = new_synchro(mode);

  /* Create the pool of worker threads (the caller of apply() will be worker[0]) */
  this->workers[0] = nullptr;

  for (unsigned i = 1; i < num_workers; i++) {
    auto* data       = new ThreadData(*this, i);
    this->workers[i] = new std::thread(worker_main, data);

    /* Bind the worker to a core if possible */
#if HAVE_PTHREAD_SETAFFINITY
#if HAVE_PTHREAD_NP_H /* FreeBSD ? */
    cpuset_t cpuset;
    size_t size = sizeof(cpuset_t);
#else /* Linux ? */
    cpu_set_t cpuset;
    size_t size = sizeof(cpu_set_t);
#endif
    pthread_t pthread = this->workers[i]->native_handle();
    int core_bind     = (i - 1) % std::thread::hardware_concurrency();
    CPU_ZERO(&cpuset);
    CPU_SET(core_bind, &cpuset);
    pthread_setaffinity_np(pthread, size, &cpuset);
#endif
  }
}

/**
 * @brief Destroys a parmap
 */
template <typename T> Parmap<T>::~Parmap()
{
  destroying = true;
  synchro->master_signal();

  for (unsigned i = 1; i < num_workers; i++) {
    workers[i]->join();
    delete workers[i];
  }
  delete synchro;
}

/**
 * @brief Applies a list of tasks in parallel.
 * @param fun the function to call in parallel
 * @param data each element of this vector will be passed as an argument to fun
 */
template <typename T> void Parmap<T>::apply(std::function<void(T)>&& fun, const std::vector<T>& data)
{
  /* Assign resources to worker threads (we are maestro here)*/
  this->fun   = std::move(fun);
  this->data  = &data;
  this->index = 0;
  this->synchro->master_signal(); // maestro runs futex_wake to wake all the minions (the working threads)
  this->work();                   // maestro works with its minions
  this->synchro->master_wait();   // When there is no more work to do, then maestro waits for the last minion to stop
  XBT_CDEBUG(xbt_parmap, "Job done"); //   ... and proceeds
}

/**
 * @brief Returns a next task to process.
 *
 * Worker threads call this function to get more work.
 *
 * @return the next task to process, or throws a std::out_of_range exception if there is no more work
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
 * @brief Main work loop: applies fun to elements in turn.
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
 * @param mode the synchronization mode
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

/** @brief Main function of a worker thread */
template <typename T> void Parmap<T>::worker_main(ThreadData* data)
{
  auto engine                       = simgrid::kernel::EngineImpl::get_instance();
  Parmap<T>& parmap     = data->parmap;
  unsigned round        = 0;
  kernel::context::Context* context = engine->get_context_factory()->create_context(std::function<void()>(), nullptr);
  kernel::context::Context::set_current(context);

  XBT_CDEBUG(xbt_parmap, "New worker thread created");

  /* Worker's main loop */
  while (true) {
    round++; // New scheduling round
    parmap.synchro->worker_wait(round);
    if (parmap.destroying)
      break;

    XBT_CDEBUG(xbt_parmap, "Worker %d got a job", data->worker_id);
    parmap.work();
    parmap.synchro->worker_signal();
    XBT_CDEBUG(xbt_parmap, "Worker %d has finished", data->worker_id);
  }
  /* We are destroying the parmap */
  delete context;
  delete data;
}

template <typename T> void Parmap<T>::PosixSynchro::master_signal()
{
  std::unique_lock<std::mutex> lk(ready_mutex);
  this->parmap.thread_counter = 1;
  this->parmap.work_round++;
  /* wake all workers */
  ready_cond.notify_all();
}

template <typename T> void Parmap<T>::PosixSynchro::master_wait()
{
  std::unique_lock<std::mutex> lk(done_mutex);
  /* wait for all workers to be ready */
  done_cond.wait(lk, [this]() { return this->parmap.thread_counter >= this->parmap.num_workers; });
}

template <typename T> void Parmap<T>::PosixSynchro::worker_signal()
{
  std::unique_lock<std::mutex> lk(done_mutex);
  this->parmap.thread_counter++;
  if (this->parmap.thread_counter == this->parmap.num_workers) {
    /* all workers have finished, wake the controller */
    done_cond.notify_one();
  }
}

template <typename T> void Parmap<T>::PosixSynchro::worker_wait(unsigned round)
{
  std::unique_lock<std::mutex> lk(ready_mutex);
  /* wait for more work */
  ready_cond.wait(lk, [this, round]() { return this->parmap.work_round == round; });
}

#if HAVE_FUTEX_H
template <typename T> inline void Parmap<T>::FutexSynchro::futex_wait(std::atomic_uint* uaddr, unsigned val)
{
  XBT_CVERB(xbt_parmap, "Waiting on futex %p", uaddr);
  syscall(SYS_futex, uaddr, FUTEX_WAIT_PRIVATE, val, nullptr, nullptr, 0);
}

template <typename T> inline void Parmap<T>::FutexSynchro::futex_wake(std::atomic_uint* uaddr, unsigned val)
{
  XBT_CVERB(xbt_parmap, "Waking futex %p", uaddr);
  syscall(SYS_futex, uaddr, FUTEX_WAKE_PRIVATE, val, nullptr, nullptr, 0);
}

template <typename T> void Parmap<T>::FutexSynchro::master_signal()
{
  this->parmap.thread_counter.store(1);
  this->parmap.work_round.fetch_add(1);
  /* wake all workers */
  futex_wake(&this->parmap.work_round, std::numeric_limits<int>::max());
}

template <typename T> void Parmap<T>::FutexSynchro::master_wait()
{
  unsigned count = this->parmap.thread_counter.load();
  while (count < this->parmap.num_workers) {
    /* wait for all workers to be ready */
    futex_wait(&this->parmap.thread_counter, count);
    count = this->parmap.thread_counter.load();
  }
}

template <typename T> void Parmap<T>::FutexSynchro::worker_signal()
{
  unsigned count = this->parmap.thread_counter.fetch_add(1) + 1;
  if (count == this->parmap.num_workers) {
    /* all workers have finished, wake the controller */
    futex_wake(&this->parmap.thread_counter, std::numeric_limits<int>::max());
  }
}

template <typename T> void Parmap<T>::FutexSynchro::worker_wait(unsigned round)
{
  unsigned work_round = this->parmap.work_round.load();
  /* wait for more work */
  while (work_round != round) {
    futex_wait(&this->parmap.work_round, work_round);
    work_round = this->parmap.work_round.load();
  }
}
#endif

template <typename T> void Parmap<T>::BusyWaitSynchro::master_signal()
{
  this->parmap.thread_counter.store(1);
  this->parmap.work_round.fetch_add(1);
}

template <typename T> void Parmap<T>::BusyWaitSynchro::master_wait()
{
  while (this->parmap.thread_counter.load() < this->parmap.num_workers) {
    std::this_thread::yield();
  }
}

template <typename T> void Parmap<T>::BusyWaitSynchro::worker_signal()
{
  this->parmap.thread_counter.fetch_add(1);
}

template <typename T> void Parmap<T>::BusyWaitSynchro::worker_wait(unsigned round)
{
  /* wait for more work */
  while (this->parmap.work_round.load() != round) {
    std::this_thread::yield();
  }
}

/** @} */
}
}

#endif
