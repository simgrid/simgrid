/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MAILBOX_HPP
#define SIMGRID_S4U_MAILBOX_HPP

#include <xbt/string.hpp>
#include <simgrid/s4u/Actor.hpp>

#include <string>

namespace simgrid {
namespace s4u {

/** @brief Mailboxes: Network rendez-vous points. */
class XBT_PUBLIC Mailbox {
  friend simgrid::s4u::Comm;
  friend simgrid::kernel::activity::MailboxImpl;

  simgrid::kernel::activity::MailboxImpl* const pimpl_;

  explicit Mailbox(kernel::activity::MailboxImpl * mbox) : pimpl_(mbox) {}

  /** private function to manage the mailboxes' lifetime (see @ref s4u_raii) */
  friend void intrusive_ptr_add_ref(Mailbox*) {}
  /** private function to manage the mailboxes' lifetime (see @ref s4u_raii) */
  friend void intrusive_ptr_release(Mailbox*) {}

public:
  /** private function, do not use. FIXME: make me protected */
  kernel::activity::MailboxImpl* get_impl() { return pimpl_; }

  /** @brief Retrieves the name of that mailbox as a C++ string */
  const simgrid::xbt::string& get_name() const;
  /** @brief Retrieves the name of that mailbox as a C string */
  const char* get_cname() const;

  /** Retrieve the mailbox associated to the given name */
  static MailboxPtr by_name(const std::string& name);

  /** Returns whether the mailbox contains queued communications */
  bool empty();

  /** Check if there is a communication going on in a mailbox. */
  bool listen();

  /** Check if there is a communication ready to be consumed from a mailbox. */
  bool ready();

  /** Gets the first element in the queue (without dequeuing it), or nullptr if none is there */
  kernel::activity::CommImplPtr front();

  /** Declare that the specified actor is a permanent receiver on that mailbox
   *
   * It means that the communications sent to this mailbox will start flowing to
   * its host even before it does a get(). This models the real behavior of TCP
   * and MPI communications, amongst other. It will improve the accuracy of
   * predictions, in particular if your application exhibits swarms of small messages.
   *
   * SimGrid does not enforces any kind of ownership over the mailbox. Even if a receiver
   * was declared, any other actors can still get() data from the mailbox. The timings
   * will then probably be off tracks, so you should strive on your side to not get data
   * from someone else's mailbox.
   *
   * Note that being permanent receivers of a mailbox prevents actors to be garbage-collected.
   * If your simulation creates many short-lived actors that marked as permanent receiver, you
   * should call mailbox->set_receiver(nullptr) by the end of the actors so that their memory gets
   * properly reclaimed. This call should be at the end of the actor's function, not in a on_exit
   * callback.
   */
  void set_receiver(ActorPtr actor);

  /** Return the actor declared as permanent receiver, or nullptr if none **/
  ActorPtr get_receiver();

  /** Creates (but don't start) a data transmission to that mailbox */
  CommPtr put_init();
  /** Creates (but don't start) a data transmission to that mailbox */
  CommPtr put_init(void* data, uint64_t simulated_size_in_bytes);
  /** Creates and start a data transmission to that mailbox */
  CommPtr put_async(void* data, uint64_t simulated_size_in_bytes);

  smx_activity_t iprobe(int type, int (*match_fun)(void*, void*, kernel::activity::CommImpl*), void* data);
  /** Blocking data transmission */
  void put(void* payload, uint64_t simulated_size_in_bytes);
  /** Blocking data transmission with timeout */
  void put(void* payload, uint64_t simulated_size_in_bytes, double timeout);

  /** Creates (but don't start) a data reception onto that mailbox */
  CommPtr get_init();
  /** Creates and start an async data reception to that mailbox */
  CommPtr get_async(void** data);

  /** Blocking data reception */
  void* get(); // FIXME: make a typed template version
  /** Blocking data reception with timeout */
  void* get(double timeout);

  // Deprecated functions
#ifndef DOXYGEN
  /** @deprecated Mailbox::set_receiver() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Mailbox::set_receiver()") void setReceiver(ActorPtr actor)
  {
    set_receiver(actor);
  }
  /** @deprecated Mailbox::get_receiver() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Mailbox::get_receiver()") ActorPtr getReceiver() { return get_receiver(); }
  /** @deprecated Mailbox::get_name() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Mailbox::get_name()") const simgrid::xbt::string& getName() const
  {
    return get_name();
  }
  /** @deprecated Mailbox::get_cname() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Mailbox::get_cname()") const char* getCname() const { return get_cname(); }
  /** @deprecated Mailbox::get_impl() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Mailbox::get_impl()") kernel::activity::MailboxImpl* getImpl()
  {
    return get_impl();
  }
  /** @deprecated Mailbox::by_name() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Mailbox::by_name()") static MailboxPtr byName(const char* name)
  {
    return by_name(name);
  }
  /** @deprecated Mailbox::by_name() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Mailbox::by_name()") static MailboxPtr byName(const std::string& name)
  {
    return by_name(name);
  }
#endif
};

}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_MAILBOX_HPP */
