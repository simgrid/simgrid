/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/Aid.hpp"
#include "src/mc/api/Clock.hpp"

simgrid::mc::Clock simgrid::mc::Clock::INVALID = Clock::INVALID_VALUE;
simgrid::mc::Aid simgrid::mc::Aid::INVALID{Aid::INVALID_VALUE, false};