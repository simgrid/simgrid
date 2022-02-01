/* Copyright (c) 2010-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/sg_config.hpp"
#include "src/instr/instr_private.hpp"
#include "src/instr/instr_smpi.hpp"
#include "src/smpi/include/private.hpp"
#include <fstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr, "tracing event system");

namespace simgrid {
namespace instr {
static std::vector<PajeEvent*> buffer;

double last_timestamp_to_dump = 0;
// dumps the trace file until the last_timestamp_to_dump
void dump_buffer(bool force)
{
  if (not TRACE_is_enabled())
    return;
  XBT_DEBUG("%s: dump until %f. starts", __func__, last_timestamp_to_dump);
  if (force || (trace_format == TraceFormat::Ti)){
    for (auto const& event : buffer) {
      event->print();
      delete event;
    }
    buffer.clear();
  } else {
    auto i = buffer.begin();
    for (auto const& event : buffer) {
      double head_timestamp = event->timestamp_;
      if (head_timestamp > last_timestamp_to_dump)
        break;
      event->print();
      delete event;
      ++i;
    }
    buffer.erase(buffer.begin(), i);
  }
  XBT_DEBUG("%s: ends", __func__);
}

/* internal do the instrumentation module */
void PajeEvent::insert_into_buffer()
{
  XBT_DEBUG("%s: insert event_type=%u, timestamp=%f, buffersize=%zu)", __func__, static_cast<unsigned>(eventType_),
            timestamp_, buffer.size());
  std::vector<PajeEvent*>::reverse_iterator i;
  for (i = buffer.rbegin(); i != buffer.rend(); ++i) {
    PajeEvent* e1 = *i;
    XBT_DEBUG("compare to %p is of type %u; timestamp:%f", e1, static_cast<unsigned>(e1->eventType_), e1->timestamp_);
    if (e1->timestamp_ <= timestamp_)
      break;
  }
  if (i == buffer.rend())
    XBT_DEBUG("%s: inserted at beginning", __func__);
  else if (i == buffer.rbegin())
    XBT_DEBUG("%s: inserted at end", __func__);
  else
    XBT_DEBUG("%s: inserted at pos= %zd from its end", __func__, std::distance(buffer.rbegin(), i));
  buffer.insert(i.base(), this);
}

} // namespace instr
} // namespace simgrid
