/* Copyright (c) 2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGIN_PRODUCERCONSUMER_HPP
#define SIMGRID_PLUGIN_PRODUCERCONSUMER_HPP

#include <simgrid/s4u/ConditionVariable.hpp>
#include <simgrid/s4u/Mailbox.hpp>
#include <simgrid/s4u/Mutex.hpp>
#include <xbt/asserts.h>
#include <xbt/log.h>

#include <atomic>
#include <climits>
#include <queue>
#include <string>

XBT_LOG_NEW_CATEGORY(producer_consumer, "Producer-Consumer plugin logging category");

/** Stock implementation of a generic monitored queue to solve the producer-consumer problem */

namespace simgrid {
namespace plugin {

template <typename T> class ProducerConsumer;
template <typename T> using ProducerConsumerPtr = boost::intrusive_ptr<ProducerConsumer<T>>;

static unsigned long pc_id = 0;

template <typename T> class ProducerConsumer {
public:
  /** This ProducerConsumer plugin can use two different transfer modes:
   *   - TransferMode::MAILBOX: this mode induces a s4u::Comm between the actors doing the calls to put() and get().
   *     If these actors are on the same host, this communication goes through the host's loopback and can thus be
   *     seen as a memory copy. Otherwise, data goes over the network.
   *   - TransferMode::QUEUE: data is internally stored in a std::queue. Putting and getting data to and from this
   *     data structure has a zero-cost in terms of simulated time.
   *  Both modes guarantee that the data is consumed in the order it has been produced. However, when data goes
   *  through the network, s4u::Comm are started in the right order, but may complete in a different order depending
   *  the characteristics of the different interconnections between host pairs.
   */
  enum class TransferMode { MAILBOX = 0, QUEUE };

private:
  std::string id;

  /* Implementation of a Monitor to handle the data exchanges */
  s4u::MutexPtr mutex_;
  s4u::ConditionVariablePtr can_put_;
  s4u::ConditionVariablePtr can_get_;

  /* data containers for each of the transfer modes */
  s4u::Mailbox* mbox_ = nullptr;
  std::queue<T*> queue_;

  unsigned int max_queue_size_ = 1;
  TransferMode tmode_          = TransferMode::MAILBOX;

  /* Refcounting management */
  std::atomic_int_fast32_t refcount_{0};
  friend void intrusive_ptr_add_ref(ProducerConsumer* pc) { pc->refcount_.fetch_add(1, std::memory_order_acq_rel); }

  friend void intrusive_ptr_release(ProducerConsumer* pc)
  {
    if (pc->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete pc;
    }
  }

  explicit ProducerConsumer(unsigned int max_queue_size) : max_queue_size_(max_queue_size)
  {
    xbt_assert(max_queue_size > 0, "Max queue size of 0 is not allowed");

    id = std::string("ProducerConsumer") + std::to_string(pc_id);
    pc_id++;

    mutex_   = s4u::Mutex::create();
    can_put_ = s4u::ConditionVariable::create();
    can_get_ = s4u::ConditionVariable::create();

    if (tmode_ == TransferMode::MAILBOX)
      mbox_ = s4u::Mailbox::by_name(id);
  }
  ~ProducerConsumer() = default;

public:
  /** Creation of the monitored queue. Its size can be bounded by passing a strictly positive value to 'max_queue_size'
   *  as parameter. Calling 'create()' means that the queue size is (virtually) infinite.
   */
  static ProducerConsumerPtr<T> create(unsigned int max_queue_size = UINT_MAX)
  {
    return ProducerConsumerPtr<T>(new ProducerConsumer<T>(max_queue_size));
  }

  /** This method is intended more to set the maximum queue size in a fluent way than changing the size during the
   *  utilization of the ProducerConsumer. Hence, the modification occurs in a critical section to prevent
   *  inconsistencies.
   */
  ProducerConsumer* set_max_queue_size(unsigned int max_queue_size)
  {
    std::unique_lock<s4u::Mutex> lock(*mutex_);
    max_queue_size_ = max_queue_size;
    return this;
  }

  unsigned int get_max_queue_size() const { return max_queue_size_; }

  /** The underlying data container (and transfer mode) can only be modified when the queue is empty.*/
  ProducerConsumer* set_transfer_mode(TransferMode new_mode)
  {
    if (tmode_ == new_mode) /* No change, do nothing */
      return this;

    xbt_assert(empty(), "cannot change transfer mode when some data is in queue");
    if (new_mode == TransferMode::MAILBOX) {
      mbox_ = s4u::Mailbox::by_name(id);
    } else {
      mbox_ = nullptr;
    }
    tmode_ = new_mode;
    return this;
  }
  std::string get_transfer_mode() const { return tmode_ == TransferMode::MAILBOX ? "mailbox" : "queue"; }

  /** Container-agnostic size() method */
  unsigned int size() { return tmode_ == TransferMode::MAILBOX ? mbox_->size() : queue_.size(); }

  /** Container-agnostic empty() method */
  bool empty() { return tmode_ == TransferMode::MAILBOX ? mbox_->empty() : queue_.empty(); }

  /** Asynchronous put() of a data item of a given size
   *  - TransferMode::MAILBOX: if put_async is called directly from user code, it can be considered to be done in a
   *    fire-and-forget mode. No need to save the s4u::CommPtr.
   *  - TransferMode::QUEUE: the data is simply pushed into the queue.
   */
  s4u::CommPtr put_async(T* data, size_t simulated_size_in_bytes)
  {
    std::unique_lock<s4u::Mutex> lock(*mutex_);
    s4u::CommPtr comm = nullptr;
    XBT_CVERB(producer_consumer, (size() < max_queue_size_) ? "can put" : "must wait");

    while (size() >= max_queue_size_)
      can_put_->wait(lock);
    if (tmode_ == TransferMode::MAILBOX) {
      comm = mbox_->put_async(data, simulated_size_in_bytes);
    } else
      queue_.push(data);
    can_get_->notify_all();
    return comm;
  }

  /** Synchronous put() of a data item of a given size
   *  - TransferMode::MAILBOX: the caller must wait for the induced communication with the getter of the data to be
   *    complete to continue with its execution. This wait is done outside of the monitor to prevent serialization.
   *  - TransferMode::QUEUE: the behavior is exactly the same as put_async: data is simply pushed into the queue.
   */
  void put(T* data, size_t simulated_size_in_bytes)
  {
    s4u::CommPtr comm = put_async(data, simulated_size_in_bytes);
    if (comm) {
      XBT_CDEBUG(producer_consumer, "Waiting for the data to be consumed");
      comm->wait();
    }
  }

  /** Asynchronous get() of a 'data'
   *  - TransferMode::MAILBOX: the caller is returned a s4u::CommPtr onto which it can wait when the data is really
   *    needed.
   *  - TransferMode::QUEUE: the data is simply popped from the queue and directly available. Better to call get() in
   *    this transfer mode.
   */
  s4u::CommPtr get_async(T** data)
  {
    std::unique_lock<s4u::Mutex> lock(*mutex_);
    s4u::CommPtr comm = nullptr;
    XBT_CVERB(producer_consumer, empty() ? "must wait" : "can get");
    while (empty())
      can_get_->wait(lock);
    if (tmode_ == TransferMode::MAILBOX)
      comm = mbox_->get_async<T>(data);
    else {
      *data = queue_.front();
      queue_.pop();
    }
    can_put_->notify_all();

    return comm;
  }

  /** Synchronous get() of a 'data'
   *  - TransferMode::MAILBOX: the caller waits (outside the monitor to prevent serialization) for the induced
   *    communication to be complete to continue with its execution.
   *  - TransferMode::QUEUE: the behavior is exactly the same as get_async: data is simply popped from the queue and
   *    directly available to the caller.
   */
  T* get()
  {
    T* data;
    s4u::CommPtr comm = get_async(&data);
    if (comm) {
      XBT_CDEBUG(producer_consumer, "Waiting for the data to arrive");
      comm->wait();
    }
    XBT_CDEBUG(producer_consumer, "data is available");
    return data;
  }
};

} // namespace plugin
} // namespace simgrid

#endif // SIMGRID_PLUGIN_PRODUCERCONSUMER_HPP
