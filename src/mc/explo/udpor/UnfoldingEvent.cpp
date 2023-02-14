/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/UnfoldingEvent.hpp"

namespace simgrid::mc::udpor {

UnfoldingEvent::UnfoldingEvent(unsigned int nb_events, std::string const& trans_tag, EventSet const& immediate_causes)
    : UnfoldingEvent(nb_events, trans_tag, immediate_causes, 0)
{
  // TODO: Implement this correctly
}

UnfoldingEvent::UnfoldingEvent(unsigned int nb_events, std::string const& trans_tag, EventSet const& immediate_causes,
                               StateHandle sid)
{
  // TODO: Implement this
}

} // namespace simgrid::mc::udpor
