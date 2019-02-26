/* Copyright (c) 2012-2019. The SimGrid Team.
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
}

#define find_node_timeout 10
#define find_node_global_timeout 50

#define kademlia_alpha 3
#define BUCKET_SIZE 20

#define identifier_size 32

#define random_lookup_interval 100

#define MAX_STEPS 10

#define JOIN_BUCKETS_QUERIES 5

#define RANDOM_LOOKUP_NODE 0

#endif
