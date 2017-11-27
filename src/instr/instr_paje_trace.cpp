/* Copyright (c) 2010-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/sg_config.h"
#include "src/instr/instr_private.hpp"
#include "src/instr/instr_smpi.hpp"
#include "src/smpi/include/private.hpp"
#include "typeinfo"
#include "xbt/virtu.h" /* sg_cmdline */
#include <fstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr, "tracing event system");

static std::stringstream stream;
FILE *tracing_file = nullptr;

std::vector<simgrid::instr::PajeEvent*> buffer;
void buffer_debug(std::vector<simgrid::instr::PajeEvent*>* buf);

void dump_comment(std::string comment)
{
  if (comment.empty())
    return;
  fprintf(tracing_file, "# %s\n", comment.c_str());
}

void dump_comment_file(std::string filename)
{
  if (filename.empty())
    return;
  std::ifstream* fs = new std::ifstream();
  fs->open(filename.c_str(), std::ifstream::in);

  if (fs->fail()) {
    THROWF(system_error, 1, "Comment file %s could not be opened for reading.", filename.c_str());
  }
  while (not fs->eof()) {
    std::string line;
    fprintf (tracing_file, "# ");
    std::getline(*fs, line);
    fprintf(tracing_file, "%s", line.c_str());
  }
  fs->close();
}

double TRACE_last_timestamp_to_dump = 0;
//dumps the trace file until the timestamp TRACE_last_timestamp_to_dump
void TRACE_paje_dump_buffer(bool force)
{
  if (not TRACE_is_enabled())
    return;
  XBT_DEBUG("%s: dump until %f. starts", __FUNCTION__, TRACE_last_timestamp_to_dump);
  if (force){
    for (auto const& event : buffer) {
      event->print();
      delete event;
    }
    buffer.clear();
  }else{
    std::vector<simgrid::instr::PajeEvent*>::iterator i = buffer.begin();
    for (auto const& event : buffer) {
      double head_timestamp = event->timestamp_;
      if (head_timestamp > TRACE_last_timestamp_to_dump)
        break;
      event->print();
      delete event;
      ++i;
    }
    buffer.erase(buffer.begin(), i);
  }
  XBT_DEBUG("%s: ends", __FUNCTION__);
}

void buffer_debug(std::vector<simgrid::instr::PajeEvent*>* buf)
{
  if (not XBT_LOG_ISENABLED(instr_paje_trace, xbt_log_priority_debug))
    return;
  XBT_DEBUG(">>>>>> Dump the state of the buffer. %zu events", buf->size());
  for (auto const& event : *buf) {
    event->print();
    XBT_DEBUG("%p %s", event, stream.str().c_str());
    stream.str("");
    stream.clear();
  }
  XBT_DEBUG("<<<<<<");
}

/* internal do the instrumentation module */
void simgrid::instr::PajeEvent::insertIntoBuffer()
{
  if (not TRACE_buffer()) {
    print();
    delete this;
    return;
  }
  buffer_debug(&buffer);

  XBT_DEBUG("%s: insert event_type=%u, timestamp=%f, buffersize=%zu)", __FUNCTION__, eventType_, timestamp_,
            buffer.size());
  std::vector<simgrid::instr::PajeEvent*>::reverse_iterator i;
  for (i = buffer.rbegin(); i != buffer.rend(); ++i) {
    simgrid::instr::PajeEvent* e1 = *i;
    XBT_DEBUG("compare to %p is of type %u; timestamp:%f", e1, e1->eventType_, e1->timestamp_);
    if (e1->timestamp_ <= timestamp_)
      break;
  }
  if (i == buffer.rend())
    XBT_DEBUG("%s: inserted at beginning", __FUNCTION__);
  else if (i == buffer.rbegin())
    XBT_DEBUG("%s: inserted at end", __FUNCTION__);
  else
    XBT_DEBUG("%s: inserted at pos= %zd from its end", __FUNCTION__, std::distance(buffer.rbegin(), i));
  buffer.insert(i.base(), this);

  buffer_debug(&buffer);
}

void TRACE_paje_start() {
  std::string filename = TRACE_get_filename();
  tracing_file         = fopen(filename.c_str(), "w");
  if (tracing_file == nullptr){
    THROWF(system_error, 1, "Tracefile %s could not be opened for writing.", filename.c_str());
  }

  XBT_DEBUG("Filename %s is open for writing", filename.c_str());

  /* output generator version */
  fprintf (tracing_file, "#This file was generated using SimGrid-%d.%d.%d\n",
           SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR, SIMGRID_VERSION_PATCH);
  fprintf (tracing_file, "#[");
  unsigned int cpt;
  char *str;
  xbt_dynar_foreach (xbt_cmdline, cpt, str){
    fprintf(tracing_file, "%s ",str);
  }
  fprintf (tracing_file, "]\n");

  /* output one line comment */
  dump_comment (TRACE_get_comment());

  /* output comment file */
  dump_comment_file (TRACE_get_comment_file());

  /* output header */
  TRACE_header(TRACE_basic(),TRACE_display_sizes());
}

void TRACE_paje_end() {
  fclose(tracing_file);
  XBT_DEBUG("Filename %s is closed", TRACE_get_filename().c_str());
}


void TRACE_TI_start()
{
  std::string filename = TRACE_get_filename();
  tracing_file         = fopen(filename.c_str(), "w");
  if (tracing_file == nullptr) {
    THROWF(system_error, 1, "Tracefile %s could not be opened for writing.", filename.c_str());
  }

  XBT_DEBUG("Filename %s is open for writing", filename.c_str());

  /* output one line comment */
  dump_comment(TRACE_get_comment());

  /* output comment file */
  dump_comment_file(TRACE_get_comment_file());
}

void TRACE_TI_end()
{
  fclose(tracing_file);
  XBT_DEBUG("Filename %s is closed", TRACE_get_filename().c_str());
}
