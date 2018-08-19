/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MAILBOX_HPP
#define SIMGRID_S4U_MAILBOX_HPP

#include <xbt/string.hpp>
#include <simgrid/s4u/Actor.hpp>

#include <string>

namespace simgrid {
namespace s4u {

/** @brief Mailboxes: Network rendez-vous points.
 *
 * <b>What are mailboxes?</b>
 *
 * Rendez-vous point for network communications, similar to URLs on
 * which you could post and retrieve data. Actually, the mailboxes are
 * not involved in the communication once it starts, but only to find
 * the contact with which you want to communicate.

 * Here are some mechanisms similar to the mailbox in other
 * communication systems: The phone number, which allows the caller to
 * find the receiver. The twitter hashtag, which help senders and
 * receivers to find each others. In TCP, the pair {host name, host
 * port} to which you can connect to find your interlocutor. In HTTP,
 * URLs through which the clients can connect to the servers. In ZeroMQ
 * and other queuing systems, the queues are used to match senders
 * and receivers.
 *
 * One big difference with most of these systems is that no actor is
 * the exclusive owner of a mailbox, neither in sending nor in
 * receiving. Many actors can send into and/or receive from the
 * same mailbox.  This is a big difference to the socket ports for
 * example, that are definitely exclusive in receiving.
 *
 * Mailboxes can optionally have a @i receiver with `simgrid::s4u::Mailbox::set_receiver()`.
 * It means that the data exchange starts as soon as the sender has
 * done the `put()`, even before the corresponding `get()`
 * (usually, it starts as soon as both  `put()` and `get()` are posted).
 * This is closer to the BSD semantic and can thus help to improve
 * the timing accuracy, but this is not mandatory at all.
 *
 * A big difference with twitter hashtags is that SimGrid does not
 * offer easy support to broadcast a given message to many
 * receivers. So that would be like a twitter tag where each message
 * is consumed by the first coming receiver.
 *
 * A big difference with the ZeroMQ queues is that you cannot filter
 * on the data you want to get from the mailbox. To model such settings
 * in SimGrid, you'd have one mailbox per potential topic, and subscribe
 * to each topic individually with a `get_async()` on each mailbox.
 * Then, use `Comm::wait_any()` to get the first message on any of the
 * mailbox you are subscribed onto.
 *
 * The mailboxes are not located on the network, and you can access
 * them without any latency. The network delay are only related to the
 * location of the sender and receiver once the match between them is
 * done on the mailbox. This is just like the phone number that you
 * can use locally, and the geographical distance only comes into play
 * once you start the communication by dialing this number.
 *
 * <b>How to use mailboxes?</b>
 *
 * Any existing mailbox can be retrieve from its name (which are
 * unique strings, just like with twitter tags). This results in a
 * versatile mechanism that can be used to build many different
 * situations.
 *
 * For something close to classical socket communications, use
 * "hostname:port" as mailbox names, and make sure that only one actor
 * reads into that mailbox. It's hard to build a perfectly realistic
 * model of the TCP sockets, but most of the time, this system is too
 * cumbersome for your simulations anyway. You probably want something
 * simpler, that turns our to be easy to build with the mailboxes.
 *
 * Many SimGrid examples use a sort of yellow page system where the
 * mailbox names are the name of the service (such as "worker",
 * "master" or "reducer"). That way, you don't have to know where your
 * peer is located to contact it. You don't even need its name. Its
 * function is enough for that. This also gives you some sort of load
 * balancing for free if more than one actor pulls from the mailbox:
 * the first relevant actor that can deal with the request will handle
 * it.
 *
 * <b>How are sends and receives matched?</b>
 *
 * The matching algorithm is as simple as a first come, first
 * serve. When a new send arrives, it matches the oldest enqueued
 * receive. If no receive is currently enqueued, then the incoming
 * send is enqueued. As you can see, the mailbox cannot contain both
 * send and receive requests: all enqueued requests must be of the
 * same sort.
 *
 * <b>Declaring a receiving actor</b>
 *
 * The last twist is that by default in the simulator, the data starts
 * to be exchanged only when both the sender and the receiver are
 * declared while in real systems (such as TCP or MPI), the data
 * starts to flow as soon as the sender posts it, even if the receiver
 * did not post its recv() yet. This can obviously lead to bad
 * simulation timings, as the simulated communications do not start at
 * the exact same time than the real ones.
 *
 * If the simulation timings are very important to you, you can
 * declare a specific receiver to a given mailbox (with the function
 * setReceiver()). That way, any send() posted to that mailbox will
 * start as soon as possible, and the data will already be there on
 * the receiver host when the receiver actor posts its receive().
 *
 * <b>The API</b>
 *
 */
class XBT_PUBLIC Mailbox {
  friend simgrid::s4u::Comm;
  friend simgrid::kernel::activity::MailboxImpl;

  simgrid::kernel::activity::MailboxImpl* pimpl_;

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
  static MailboxPtr by_name(std::string name);

  /** Returns whether the mailbox contains queued communications */
  bool empty();

  /** Check if there is a communication going on in a mailbox. */
  bool listen();

  /** Check if there is a communication ready to be consumed from a mailbox. */
  bool ready();

  /** Gets the first element in the queue (without dequeuing it), or nullptr if none is there */
  smx_activity_t front();

  /** Declare that the specified actor is a permanent receiver on that mailbox
   *
   * It means that the communications sent to this mailbox will start flowing to
   * its host even before he does a recv(). This models the real behavior of TCP
   * and MPI communications, amongst other. It will improve the accuracy of
   * predictions, in particular if your application exhibits swarms of small messages.
   *
   * SimGrid does not enforces any kind of ownership over the mailbox. Even if a receiver
   * was declared, any other actors can still get() data from the mailbox. The timings
   * will then probably be off tracks, so you should strive on your side to not get data
   * from someone else's mailbox.
   */
  void set_receiver(ActorPtr actor);

  /** Return the actor declared as permanent receiver, or nullptr if none **/
  ActorPtr get_receiver();

  /** Creates (but don't start) a data emission to that mailbox */
  CommPtr put_init();
  /** Creates (but don't start) a data emission to that mailbox */
  CommPtr put_init(void* data, uint64_t simulated_size_in_bytes);
  /** Creates and start a data emission to that mailbox */
  CommPtr put_async(void* data, uint64_t simulated_size_in_bytes);

  /** Blocking data emission */
  void put(void* payload, uint64_t simulated_size_in_bytes);
  /** Blocking data emission with timeout */
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
  XBT_ATTRIB_DEPRECATED_v323("Please use Mailbox::by_name()") static MailboxPtr byName(std::string name)
  {
    return by_name(name);
  }
};

}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_MAILBOX_HPP */
