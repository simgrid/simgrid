/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"

#include "simgrid/s4u/mailbox.hpp"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_channel,s4u,"S4U Communication Mailboxes");


using namespace simgrid;

boost::unordered_map <std::string, s4u::Mailbox *> *s4u::Mailbox::mailboxes = new boost::unordered_map<std::string, s4u::Mailbox*> ();


s4u::Mailbox::Mailbox(const char*name, smx_mailbox_t inferior) {
  inferior_ = inferior;
  name_ = name;
  mailboxes->insert({name, this});
}
const char *s4u::Mailbox::getName() {
  return name_.c_str();
}
s4u::Mailbox *s4u::Mailbox::byName(const char*name) {
  s4u::Mailbox *res;
  try {
    res = mailboxes->at(name);
  } catch (std::out_of_range& e) {
    // FIXME: there is a potential race condition here where two actors run Mailbox::byName on a non-existent mailbox
    // during the same scheduling round. Both will be interrupted in the simcall creating the underlying simix rdv.
    // Only one simix object will be created, but two S4U objects will be created.
    // Only one S4U object will be stored in the hashmap and used, and the other one will be leaked.
    new Mailbox(name,simcall_rdv_create(name));
    res = mailboxes->at(name); // Use the stored one, even if it's not the one I created myself.
  }
  return res;
}
