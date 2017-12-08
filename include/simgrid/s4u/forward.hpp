/* Copyright (c) 2016-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_FORWARD_HPP
#define SIMGRID_S4U_FORWARD_HPP

#include <boost/intrusive_ptr.hpp>
#include <xbt/base.h>

namespace simgrid {
namespace s4u {

class Activity;
class Actor;
using ActorPtr = boost::intrusive_ptr<Actor>;
XBT_PUBLIC(void) intrusive_ptr_release(Actor* actor);
XBT_PUBLIC(void) intrusive_ptr_add_ref(Actor* actor);

class Comm;
using CommPtr = boost::intrusive_ptr<Comm>;
XBT_PUBLIC(void) intrusive_ptr_release(Comm* c);
XBT_PUBLIC(void) intrusive_ptr_add_ref(Comm* c);

class Engine;
class Exec;
using ExecPtr = boost::intrusive_ptr<Exec>;
XBT_PUBLIC(void) intrusive_ptr_release(Exec* e);
XBT_PUBLIC(void) intrusive_ptr_add_ref(Exec* e);

class Host;
class Link;
class Mailbox;
using MailboxPtr = boost::intrusive_ptr<Mailbox>;
class Mutex;
class NetZone;
class VirtualMachine;
class File;
class Storage;
}
}

#endif
