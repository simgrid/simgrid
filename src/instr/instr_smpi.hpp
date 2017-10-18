/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_SMPI_HPP
#define INSTR_SMPI_HPP

#include "smpi/smpi.h"
#include "src/instr/instr_private.hpp"
#include <string>

extern "C" {

XBT_PRIVATE void TRACE_internal_smpi_set_category(const char* category);
XBT_PRIVATE const char* TRACE_internal_smpi_get_category();
XBT_PRIVATE void TRACE_smpi_collective_in(int rank, const char* operation, instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_collective_out(int rank, const char* operation);
XBT_PRIVATE void TRACE_smpi_computing_init(int rank);
XBT_PRIVATE void TRACE_smpi_computing_out(int rank);
XBT_PRIVATE void TRACE_smpi_computing_in(int rank, instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_sleeping_init(int rank);
XBT_PRIVATE void TRACE_smpi_sleeping_out(int rank);
XBT_PRIVATE void TRACE_smpi_sleeping_in(int rank, instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_testing_out(int rank);
XBT_PRIVATE void TRACE_smpi_testing_in(int rank, instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_alloc();
XBT_PRIVATE void TRACE_smpi_release();
XBT_PRIVATE void TRACE_smpi_ptp_in(int rank, const char* operation, instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_ptp_out(int rank, int dst, const char* operation);
XBT_PRIVATE void TRACE_smpi_send(int rank, int src, int dst, int tag, int size);
XBT_PRIVATE void TRACE_smpi_recv(int src, int dst, int tag);
XBT_PRIVATE void TRACE_smpi_init(int rank);
XBT_PRIVATE void TRACE_smpi_finalize(int rank);
XBT_PRIVATE char* smpi_container(int rank, char* container, int n);

XBT_PRIVATE const char* encode_datatype(MPI_Datatype datatype, int* known);

class smpi_trace_call_location_t {
public:
  const char* filename;
  int linenumber;

  const char* previous_filename;
  int previous_linenumber;

  std::string get_composed_key()
  {
    return std::string(previous_filename) + ':' + std::to_string(previous_linenumber) + ':' + filename + ':' +
           std::to_string(linenumber);
  }
};
}

#endif
