/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef _KADEMLIA_EXAMPLES_COMMON
#define _KADEMLIA_EXAMPLES_COMMON
#define max_join_trials 4

#define RECEIVE_TIMEOUT 1

#define ping_timeout 55
#define find_node_timeout 10;
#define find_node_global_timeout 50

#define kademlia_alpha 3
#define bucket_size 20

#define identifier_size 32
#define max_answers_to_ask 20

#define random_lookup_interval 100

#define MAILBOX_NAME_SIZE identifier_size / 4 + 1
#define IDENTIFIER_HEX_SIZE identifier_size / 4 + 1

#define COMM_SIZE 1
#define COMP_SIZE 0

#define MAX_STEPS 10

#define JOIN_BUCKETS_QUERIES 5

#define RANDOM_LOOKUP_NODE 0


#endif /* _KADEMLIA_EXAMPLES_COMMON */
