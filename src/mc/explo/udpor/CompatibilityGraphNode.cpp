/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/CompatibilityGraphNode.hpp"

namespace simgrid::mc::udpor {

CompatibilityGraphNode::CompatibilityGraphNode(std::unordered_set<CompatibilityGraphNode*> conflicts, EventSet events_)
    : conflicts(std::move(conflicts)), events_(std::move(events_))
{
}

} // namespace simgrid::mc::udpor
