/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#ifndef WIN32
#include <sys/mman.h>
#include <unistd.h>

#include "src/internal_config.h"
#include "src/xbt/memory_map.hpp"

#include "private.hpp"
#include "smpi_process.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_memory, smpi, "Memory layout support for SMPI");

int smpi_loaded_page      = -1;
char* smpi_data_exe_start = nullptr;
int smpi_data_exe_size    = 0;
int smpi_privatize_global_variables;
static void* smpi_data_exe_copy;

// We keep a copy of all the privatization regions: We can then delete everything easily by iterating over this
// collection and nothing can be leaked. We could also iterate over all actors but we would have to be diligent when two
// actors use the same privatization region (so, smart pointers would have to be used etc.)
// Use a std::deque so that pointers remain valid after push_back().
static std::deque<s_smpi_privatization_region_t> smpi_privatization_regions;

static const int PROT_RWX = (PROT_READ | PROT_WRITE | PROT_EXEC);
static const int PROT_RW  = (PROT_READ | PROT_WRITE );
XBT_ATTRIB_UNUSED static const int PROT_RX  = (PROT_READ | PROT_EXEC );

void smpi_get_executable_global_size()
{
  char buffer[PATH_MAX];
  char* full_name = realpath(xbt_binary_name, buffer);
  if (full_name == nullptr)
    xbt_die("Could not resolve binary file name");

  std::vector<simgrid::xbt::VmMap> map = simgrid::xbt::get_memory_map(getpid());
  for (auto i = map.begin(); i != map.end() ; ++i) {
    // TODO, In practice, this implementation would not detect a completely
    // anonymous data segment. This does not happen in practice, however.

    // File backed RW entry:
    if (i->pathname == full_name && (i->prot & PROT_RWX) == PROT_RW) {
      smpi_data_exe_start = (char*)i->start_addr;
      smpi_data_exe_size  = i->end_addr - i->start_addr;
      ++i;
      /* Here we are making the assumption that a suitable empty region
         following the rw- area is the end of the data segment. It would
         be better to check with the size of the data segment. */
      if (i != map.end() && i->pathname.empty() && (i->prot & PROT_RWX) == PROT_RW &&
          (char*)i->start_addr == smpi_data_exe_start + smpi_data_exe_size) {
        smpi_data_exe_size = (char*)i->end_addr - smpi_data_exe_start;
      }
      return;
    }
  }
  xbt_die("Did not find my data segment.");
}
#endif

#if HAVE_SANITIZE_ADDRESS
#include <sanitizer/asan_interface.h>
static void* asan_safe_memcpy(void* dest, void* src, size_t n)
{
  char* psrc  = static_cast<char*>(src);
  char* pdest = static_cast<char*>(dest);
  for (size_t i = 0; i < n;) {
    while (i < n && __asan_address_is_poisoned(psrc + i))
      ++i;
    if (i < n) {
      char* p  = static_cast<char*>(__asan_region_is_poisoned(psrc + i, n - i));
      size_t j = p ? (p - psrc) : n;
      memcpy(pdest + i, psrc + i, j - i);
      i = j;
    }
  }
  return dest;
}
#else
#define asan_safe_memcpy(dest, src, n) memcpy(dest, src, n)
#endif

/** Map a given SMPI privatization segment (make a SMPI process active) */
void smpi_switch_data_segment(int dest) {
  if (smpi_loaded_page == dest)//no need to switch, we've already loaded the one we want
    return;

  // So the job:
  smpi_really_switch_data_segment(dest);
}

/** Map a given SMPI privatization segment (make a SMPI process active)  even if SMPI thinks it is already active
 *
 *  When doing a state restoration, the state of the restored variables  might not be consistent with the state of the
 *  virtual memory. In this case, we to change the data segment.
 */
void smpi_really_switch_data_segment(int dest)
{
  if (smpi_data_exe_size == 0) // no need to switch
    return;

#if HAVE_PRIVATIZATION
  // FIXME, cross-process support (mmap across process when necessary)
  simgrid::smpi::Process* process = smpi_process_remote(dest);
  int current                     = process->privatized_region()->file_descriptor;
  XBT_DEBUG("Switching data frame to the one of process %d", dest);
  void* tmp =
      mmap(TOPAGE(smpi_data_exe_start), smpi_data_exe_size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, current, 0);
  if (tmp != TOPAGE(smpi_data_exe_start))
    xbt_die("Couldn't map the new region (errno %d): %s", errno, strerror(errno));
  smpi_loaded_page = dest;
#endif
}

int smpi_is_privatization_file(char* file)
{
  const std::string buffer_path("/dev/shm/my-buffer-");
  return buffer_path.compare(0, std::string::npos, file, buffer_path.length()) == 0;
}

// TODO: cheinrich: The behavior changed; this now only makes a backup of the
// data segment. I think the function should be renamed.
void smpi_backup_global_memory_segment()
{
#if HAVE_PRIVATIZATION
  smpi_get_executable_global_size();

  XBT_DEBUG("bss+data segment found : size %d starting at %p", smpi_data_exe_size, smpi_data_exe_start);

  if (smpi_data_exe_size == 0) { // no need to do anything as global variables don't exist
    smpi_privatize_global_variables=false;
    return;
  }

  smpi_data_exe_copy = ::operator new(smpi_data_exe_size);
  // Make a copy of the data segment. This clean copy is retained over the whole runtime
  // of the simulation and can be used to initialize a dynamically added, new process.
  asan_safe_memcpy(smpi_data_exe_copy, TOPAGE(smpi_data_exe_start), smpi_data_exe_size);
#else /* ! HAVE_PRIVATIZATION */
  smpi_privatize_global_variables = false;
  xbt_die("You are trying to use privatization on a system that does not support it. Don't.");
  return;
#endif
}

// Initializes the memory mapping for a single process and returns the privatization region
smpi_privatization_region_t smpi_init_global_memory_segment_process()
{
  int file_descriptor;
  void* address = nullptr;
  char path[24];
  int status;

  do {
    snprintf(path, sizeof(path), "/smpi-buffer-%06x", rand() % 0xffffffU);
    file_descriptor = shm_open(path, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  } while (file_descriptor == -1 && errno == EEXIST);
  if (file_descriptor < 0) {
    if (errno == EMFILE) {
      xbt_die("Impossible to create temporary file for memory mapping: %s\n\
The open() system call failed with the EMFILE error code (too many files). \n\n\
This means that you reached the system limits concerning the amount of files per process. \
This is not a surprise if you are trying to virtualize many processes on top of SMPI. \
Don't panic -- you should simply increase your system limits and try again. \n\n\
First, check what your limits are:\n\
  cat /proc/sys/fs/file-max # Gives you the system-wide limit\n\
  ulimit -Hn                # Gives you the per process hard limit\n\
  ulimit -Sn                # Gives you the per process soft limit\n\
  cat /proc/self/limits     # Displays any per-process limitation (including the one given above)\n\n\
If one of these values is less than the amount of MPI processes that you try to run, then you got the explanation of this error. \
Ask the Internet about tutorials on how to increase the files limit such as: https://rtcamp.com/tutorials/linux/increase-open-files-limit/",
              strerror(errno));
    }
    xbt_die("Impossible to create temporary file for memory mapping: %s", strerror(errno));
  }

  status = ftruncate(file_descriptor, smpi_data_exe_size);
  if (status)
    xbt_die("Impossible to set the size of the temporary file for memory mapping");

  /* Ask for a free region */
  address = mmap(nullptr, smpi_data_exe_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
  if (address == MAP_FAILED)
    xbt_die("Couldn't find a free region for memory mapping");

  status = shm_unlink(path);
  if (status)
    xbt_die("Impossible to unlink temporary file for memory mapping");

  // initialize the values
  asan_safe_memcpy(address, smpi_data_exe_copy, smpi_data_exe_size);

  // store the address of the mapping for further switches
  smpi_privatization_regions.emplace_back(s_smpi_privatization_region_t{address, file_descriptor});

  return &smpi_privatization_regions.back();
}

void smpi_destroy_global_memory_segments(){
  if (smpi_data_exe_size == 0) // no need to switch
    return;
#if HAVE_PRIVATIZATION
  for (auto const& region : smpi_privatization_regions) {
    if (munmap(region.address, smpi_data_exe_size) < 0)
      XBT_WARN("Unmapping of fd %d failed: %s", region.file_descriptor, strerror(errno));
    close(region.file_descriptor);
  }
  smpi_privatization_regions.clear();
  ::operator delete(smpi_data_exe_copy);
#endif
}

