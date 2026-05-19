/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/Aid.hpp"
#include "src/mc/api/Clock.hpp"
#include "src/mc/smemory/smemory_config.hpp"

simgrid::mc::Clock simgrid::mc::Clock::INVALID_VALUE = static_cast<uint64_t>(~uint64_t(0));
simgrid::mc::Aid simgrid::mc::Aid::INVALID_VALUE{simgrid::mc::smemory::config::max_threads - 1, false};
