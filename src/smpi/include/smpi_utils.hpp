/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_UTILS_HPP
#define SMPI_UTILS_HPP
#include <xbt/base.h>

#include "smpi_f2c.hpp"
#include "smpi_comm.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Methods used to parse and store the values for timing injections in smpi

namespace simgrid::smpi::utils {

XBT_PUBLIC void add_benched_time(double time);
XBT_PUBLIC void account_malloc_size(size_t size, std::string_view file, int line, const void* ptr);
XBT_PUBLIC void account_shared_size(size_t size);
XBT_PUBLIC void print_time_analysis(double time);
XBT_PUBLIC void print_buffer_info();
XBT_PUBLIC void print_memory_analysis();
XBT_PUBLIC void print_current_handle();
XBT_PUBLIC void set_current_handle(F2C* handle);
XBT_PUBLIC void set_current_buffer(int i, const char* name, const void* handle);
XBT_PUBLIC size_t get_buffer_size(const void* ptr);
XBT_PUBLIC void account_free(const void* ptr);
/* Check that all ranks of comm call the same collectives in the same order. If sends and recvs are given, they hold
 * the bytes this rank sends to and receives from each rank of comm, and are checked against the other ranks. */
XBT_PUBLIC int check_collectives_ordering(MPI_Comm comm, const std::string& call, std::vector<size_t> sends = {},
                                          std::vector<size_t> recvs = {});
/* Two overlapping blocks of a v-collective buffer, or {-1, -1}. Only contiguous datatypes are checked, as the blocks of
 * other datatypes may legally interleave. */
XBT_PUBLIC std::pair<int, int> overlapping_blocks(int n, const int* counts, const int* displs, MPI_Datatype datatype);
/* Bytes exchanged with each of the n ranks, when only peer is concerned */
XBT_PUBLIC std::vector<size_t> bytes_with(int n, int peer, size_t bytes);
/* Bytes exchanged with each of the n ranks, given as counts of datatype */
XBT_PUBLIC std::vector<size_t> bytes_with_each(int n, const int* counts, MPI_Datatype datatype);

} // namespace simgrid::smpi::utils
#endif
