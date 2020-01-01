/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _KADEMLIA_EXAMPLES_COMMON
#define _KADEMLIA_EXAMPLES_COMMON

#define MAX_JOIN_TRIALS 4

#define FIND_NODE_TIMEOUT 10
#define FIND_NODE_GLOBAL_TIMEOUT 50

#define KADEMLIA_ALPHA 3
#define BUCKET_SIZE 20

#define IDENTIFIER_SIZE 32

#define RANDOM_LOOKUP_INTERVAL 100

#define MAILBOX_NAME_SIZE 16 /* (identifier_size / 4)  (hex encoded) */

#define COMM_SIZE 1
#define COMP_SIZE 0

#define MAX_STEPS 10

#define JOIN_BUCKETS_QUERIES 5

#define RANDOM_LOOKUP_NODE 0


#endif                          /* _KADEMLIA_EXAMPLES_COMMON */
