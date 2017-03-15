/* Copyright (c) 2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_FORWARD_HPP
#define SIMGRID_S4U_FORWARD_HPP

#include <boost/intrusive_ptr.hpp>

namespace simgrid {
namespace s4u {

class Actor;
using ActorPtr = boost::intrusive_ptr<Actor>;

class Activity;
class Comm;
class Engine;
class Host;
class Mailbox;
using MailboxPtr = boost::intrusive_ptr<Mailbox>;
class Mutex;

class Storage;

}
}

#endif
