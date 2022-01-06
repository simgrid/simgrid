/* Copyright (c) 2012-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _S4U_KADEMLIA_HPP
#define _S4U_KADEMLIA_HPP
#include <algorithm>
#include <simgrid/s4u.hpp>

namespace kademlia {
class Answer;
class Message;
} // namespace kademlia

constexpr double FIND_NODE_TIMEOUT        = 10.0;
constexpr double FIND_NODE_GLOBAL_TIMEOUT = 50.0;

constexpr unsigned KADEMLIA_ALPHA = 3;
constexpr unsigned BUCKET_SIZE    = 20;

constexpr int IDENTIFIER_SIZE = 32;

constexpr double RANDOM_LOOKUP_INTERVAL = 100.0;

constexpr unsigned MAX_STEPS = 10;

constexpr unsigned JOIN_BUCKETS_QUERIES = 5;

constexpr unsigned RANDOM_LOOKUP_NODE = 0;

#endif
