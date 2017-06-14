/* Copyright (c) 2016-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_FORWARD_HPP
#define SIMGRID_S4U_FORWARD_HPP

#include <boost/intrusive_ptr.hpp>
#include <xbt/base.h>

namespace simgrid {
namespace s4u {

class Actor;
using ActorPtr = boost::intrusive_ptr<Actor>;

class Activity;
class Comm;
using CommPtr = boost::intrusive_ptr<Comm>;
class Engine;
class Host;
class Link;
class Mailbox;
using MailboxPtr = boost::intrusive_ptr<Mailbox>;
class Mutex;
class NetZone;

class File;
class Storage;

XBT_PUBLIC(void) intrusive_ptr_release(Comm* c);
XBT_PUBLIC(void) intrusive_ptr_add_ref(Comm* c);
}
}

#endif
