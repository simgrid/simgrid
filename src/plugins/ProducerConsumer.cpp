/* Copyright (c) 2021-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/ProducerConsumer.hpp>

XBT_LOG_NEW_CATEGORY(producer_consumer, "Producer-Consumer plugin logging category");

namespace simgrid::plugin {
unsigned long ProducerConsumerId::pc_id = 0;
}
