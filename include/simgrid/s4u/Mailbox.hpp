/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MAILBOX_HPP
#define SIMGRID_S4U_MAILBOX_HPP

#include <string>

#include <xbt/base.h>
#include <xbt/string.hpp>

#include <simgrid/s4u/forward.hpp>
#include <simgrid/s4u/Actor.hpp>

namespace simgrid {
namespace s4u {

/** @brief Mailboxes: Network rendez-vous points.
 *  @ingroup s4u_api
 *
 * @tableofcontents
 *
 * @section s4u_mb_what What are mailboxes
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
 * URLs through which the clients can connect to the servers.
 *
 * One big difference with most of these systems is that usually, no
 * actor is the exclusive owner of a mailbox, neither in sending nor
 * in receiving. Many actors can send into and/or receive from the
 * same mailbox.  This is a big difference to the socket ports for
 * example, that are definitely exclusive in receiving.
 *
 * A big difference with twitter hashtags is that SimGrid does not
 * offer easy support to broadcast a given message to many
 * receivers. So that would be like a twitter tag where each message
 * is consumed by the first coming receiver.
 *
 * The mailboxes are not located on the network, and you can access
 * them without any latency. The network delay are only related to the
 * location of the sender and receiver once the match between them is
 * done on the mailbox. This is just like the phone number that you
 * can use locally, and the geographical distance only comes into play
 * once you start the communication by dialling this number.
 *
 * @section s4u_mb_howto How to use mailboxes?
 *
 * Any existing mailbox can be retrieve from its name (which are
 * unique strings, just like with twitter tags). This results in a
 * versatile mechanism that can be used to build many different
 * situations.
 *
 * For something close to classical socket communications, use
 * "hostname:port" as mailbox names, and make sure that only one actor
 * reads into that mailbox. It's hard to build a prefectly realistic
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
 * @section s4u_mb_matching How are sends and receives matched?
 *
 * The matching algorithm is as simple as a first come, first
 * serve. When a new send arrives, it matches the oldest enqueued
 * receive. If no receive is currently enqueued, then the incomming
 * send is enqueued. As you can see, the mailbox cannot contain both
 * send and receive requests: all enqueued requests must be of the
 * same sort.
 *
 * @section s4u_mb_receiver Declaring a receiving actor
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
 * @section s4u_mb_api The API
 */
XBT_PUBLIC_CLASS Mailbox {
  friend Comm;
  friend simgrid::kernel::activity::MailboxImpl;

  simgrid::kernel::activity::MailboxImpl* pimpl_;

  explicit Mailbox(kernel::activity::MailboxImpl * mbox) : pimpl_(mbox) {}

  /** private function to manage the mailboxes' lifetime (see @ref s4u_raii) */
  friend void intrusive_ptr_add_ref(Mailbox*) {}
  /** private function to manage the mailboxes' lifetime (see @ref s4u_raii) */
  friend void intrusive_ptr_release(Mailbox*) {}
public:
  /** private function, do not use. FIXME: make me protected */
  kernel::activity::MailboxImpl* getImpl() { return pimpl_; }

  /** @brief Retrieves the name of that mailbox as a C++ string */
  const simgrid::xbt::string& getName() const;
  /** @brief Retrieves the name of that mailbox as a C string */
  const char* getCname() const;

  /** Retrieve the mailbox associated to the given C string */
  static MailboxPtr byName(const char *name);

  /** Retrieve the mailbox associated to the given C++ string */
  static MailboxPtr byName(std::string name);

  /** Returns whether the mailbox contains queued communications */
  bool empty();

  /** Check if there is a communication going on in a mailbox. */
  bool listen();

  /** Gets the first element in the queue (without dequeuing it), or nullptr if none is there */
  smx_activity_t front();

  /** Declare that the specified actor is a permanent receiver on that mailbox
   *
   * It means that the communications sent to this mailbox will start flowing to
   * its host even before he does a recv(). This models the real behavior of TCP
   * and MPI communications, amongst other.
   */
  void setReceiver(ActorPtr actor);

  /** Return the actor declared as permanent receiver, or nullptr if none **/
  ActorPtr getReceiver();

  /** Creates (but don't start) a data emission to that mailbox */
  CommPtr put_init();
  /** Creates (but don't start) a data emission to that mailbox */
  CommPtr put_init(void* data, uint64_t simulatedSizeInBytes);
  /** Creates and start a data emission to that mailbox */
  CommPtr put_async(void* data, uint64_t simulatedSizeInBytes);

  /** Blocking data emission */
  void put(void* payload, uint64_t simulatedSizeInBytes);
  /** Blocking data emission with timeout */
  void put(void* payload, uint64_t simulatedSizeInBytes, double timeout);

  /** Creates (but don't start) a data reception onto that mailbox */
  CommPtr get_init();
  /** Creates and start an async data reception to that mailbox */
  CommPtr get_async(void** data);

  /** Blocking data reception */
  void* get();
  /** Blocking data reception with timeout */
  void* get(double timeout);
};

}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_MAILBOX_HPP */
