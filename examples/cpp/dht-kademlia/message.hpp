/* Copyright (c) 2012-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _KADEMLIA_TASK_HPP_
#define _KADEMLIA_TASK_HPP_
#include "s4u-dht-kademlia.hpp"
#include "simgrid/s4u.hpp"

#include <memory>
#include <string>

namespace kademlia {

class Message {
public:
  unsigned int sender_id_             = 0;       // Id of the guy who sent the task
  unsigned int destination_id_        = 0;       // Id we are trying to find, if needed.
  std::unique_ptr<Answer> answer_     = nullptr; // Answer to the request made, if needed.
  simgrid::s4u::Mailbox* answer_to_   = nullptr; // mailbox to send the answer to (if not an answer).
  std::string issuer_host_name_;                 // used for logging

  explicit Message(unsigned int sender_id, unsigned int destination_id, std::unique_ptr<Answer> answer,
                   simgrid::s4u::Mailbox* mailbox, const char* hostname)
      : sender_id_(sender_id)
      , destination_id_(destination_id)
      , answer_(std::move(answer))
      , answer_to_(mailbox)
      , issuer_host_name_(hostname)
  {
  }
  explicit Message(unsigned int sender_id, unsigned int destination_id, simgrid::s4u::Mailbox* mailbox,
                   const char* hostname)
      : Message(sender_id, destination_id, nullptr, mailbox, hostname)
  {
  }
  Message(const Message&) = delete;
  Message& operator=(const Message&) = delete;
};
} // namespace kademlia
#endif
