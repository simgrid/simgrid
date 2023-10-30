/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MESSAGEQUEUE_HPP
#define SIMGRID_S4U_MESSAGEQUEUE_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Mess.hpp>
#include <smpi/forward.hpp>

#include <string>

namespace simgrid::s4u {

class XBT_PUBLIC MessageQueue {
#ifndef DOXYGEN
  friend Mess;
  friend kernel::activity::MessageQueueImpl;
#endif

  kernel::activity::MessageQueueImpl* const pimpl_;

  explicit MessageQueue(kernel::activity::MessageQueueImpl * mqueue) : pimpl_(mqueue) {}
  ~MessageQueue() = default;

protected:
  kernel::activity::MessageQueueImpl* get_impl() const { return pimpl_; }

public:
  /** @brief Retrieves the name of that message queue as a C++ string */
  const std::string& get_name() const;
  /** @brief Retrieves the name of that message queue as a C string */
  const char* get_cname() const;

  /** \static Retrieve the message queye associated to the given name. Message queues are created on demand. */
  static MessageQueue* by_name(const std::string& name);

  /** Returns whether the message queue contains queued messages */
  bool empty() const;

  /* Returns the number of queued messages */
  size_t size() const;

  /** Gets the first element in the queue (without dequeuing it), or nullptr if none is there */
  kernel::activity::MessImplPtr front() const;

  /** Creates (but don't start) a data transmission to that message queue */
  MessPtr put_init();
  /** Creates (but don't start) a data transmission to that message queue.
   *
   * Please note that if you send a pointer to some data, you must ensure that your data remains live until
   * consumption, or the receiver will get a pointer to a garbled memory area.
   */
  MessPtr put_init(void* payload);
  /** Creates and start a data transmission to that mailbox.
   *
   * Please note that if you send a pointer to some data, you must ensure that your data remains live until
   * consumption, or the receiver will get a pointer to a garbled memory area.
   */
  MessPtr put_async(void* payload);

  /** Blocking data transmission.
   *
   * Please note that if you send a pointer to some data, you must ensure that your data remains live until
   * consumption, or the receiver will get a pointer to a garbled memory area.
   */
  void put(void* payload);
  /** Blocking data transmission with timeout */
  void put(void* payload, double timeout);

  /** Creates (but don't start) a data reception onto that message queue. */
  MessPtr get_init();
  /** Creates and start an async data reception to that message queue */
  template <typename T> MessPtr get_async(T** data);
  /** Creates and start an async data reception to that mailbox. Since the data location is not provided, you'll have to
   * use Mess::get_payload once the messaging operation terminates */
  MessPtr get_async();

  /** Blocking data reception */
  template <typename T> T* get();
  template <typename T> std::unique_ptr<T> get_unique() { return std::unique_ptr<T>(get<T>()); }

  /** Blocking data reception with timeout */
  template <typename T> T* get(double timeout);
  template <typename T> std::unique_ptr<T> get_unique(double timeout) { return std::unique_ptr<T>(get<T>(timeout)); }
};

template <typename T> MessPtr MessageQueue::get_async(T** data)
{
  MessPtr res = get_init()->set_dst_data(reinterpret_cast<void**>(data), sizeof(void*));
  res->start();
  return res;
}

template <typename T> T* MessageQueue::get()
{
  T* res = nullptr;
  get_async<T>(&res)->wait();
  return res;
}

template <typename T> T* MessageQueue::get(double timeout)
{
  T* res = nullptr;
  get_async<T>(&res)->wait_for(timeout);
  return res;
}
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_MESSAGEQUEUE_HPP */
