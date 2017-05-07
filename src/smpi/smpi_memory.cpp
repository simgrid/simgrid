/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdint>
#include <climits>
#include <cstring>

#include <vector>

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef WIN32
#include <sys/mman.h>
#include <unistd.h>

#include "src/xbt/memory_map.hpp"

#include "private.h"
#include "private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_memory, smpi, "Memory layout support for SMPI");

int smpi_loaded_page = -1;
char* smpi_start_data_exe = nullptr;
int smpi_size_data_exe = 0;
int smpi_privatize_global_variables;

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
      smpi_start_data_exe = (char*) i->start_addr;
      smpi_size_data_exe = i->end_addr - i->start_addr;
      ++i;
      /* Here we are making the assumption that a suitable empty region
         following the rw- area is the end of the data segment. It would
         be better to check with the size of the data segment. */
      if (i != map.end() && i->pathname.empty() && (i->prot & PROT_RWX) == PROT_RW
          && (char*)i->start_addr ==  smpi_start_data_exe + smpi_size_data_exe) {
        smpi_size_data_exe = (char*)i->end_addr - smpi_start_data_exe;
      }
      return;
    }
  }
  xbt_die("Did not find my data segment.");
}
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
  if(smpi_size_data_exe == 0)//no need to switch
    return;

#if HAVE_PRIVATIZATION
  if(smpi_loaded_page==-1){//initial switch, do the copy from the real page here
    for (int i=0; i< smpi_process_count(); i++){
      memcpy(smpi_privatisation_regions[i].address, TOPAGE(smpi_start_data_exe), smpi_size_data_exe);
    }
  }

  // FIXME, cross-process support (mmap across process when necessary)
  int current = smpi_privatisation_regions[dest].file_descriptor;
  XBT_DEBUG("Switching data frame to the one of process %d", dest);
  void* tmp =
      mmap(TOPAGE(smpi_start_data_exe), smpi_size_data_exe, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, current, 0);
  if (tmp != TOPAGE(smpi_start_data_exe))
    xbt_die("Couldn't map the new region (errno %d): %s", errno, strerror(errno));
  smpi_loaded_page = dest;
#endif
}

int smpi_is_privatisation_file(char* file)
{
  return strncmp("/dev/shm/my-buffer-", file, std::strlen("/dev/shm/my-buffer-")) == 0;
}

void smpi_initialize_global_memory_segments()
{

#if !HAVE_PRIVATIZATION
  smpi_privatize_global_variables=false;
  xbt_die("You are trying to use privatization on a system that does not support it. Don't.");
  return;
#else

  smpi_get_executable_global_size();

  XBT_DEBUG ("bss+data segment found : size %d starting at %p", smpi_size_data_exe, smpi_start_data_exe );

  if (smpi_size_data_exe == 0){//no need to switch
    smpi_privatize_global_variables=false;
    return;
  }

  smpi_privatisation_regions = static_cast<smpi_privatisation_region_t>(
      xbt_malloc(smpi_process_count() * sizeof(struct s_smpi_privatisation_region)));

  for (int i=0; i< smpi_process_count(); i++){
    // create SIMIX_process_count() mappings of this size with the same data inside
    int file_descriptor;
    void* address = nullptr;
    char path[24];
    int status;

    do {
      snprintf(path, sizeof(path), "/smpi-buffer-%06x", rand() % 0xffffff);
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

    status = ftruncate(file_descriptor, smpi_size_data_exe);
    if (status)
      xbt_die("Impossible to set the size of the temporary file for memory mapping");

    /* Ask for a free region */
    address = mmap(nullptr, smpi_size_data_exe, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
    if (address == MAP_FAILED)
      xbt_die("Couldn't find a free region for memory mapping");

    status = shm_unlink(path);
    if (status)
      xbt_die("Impossible to unlink temporary file for memory mapping");

    // initialize the values
    memcpy(address, TOPAGE(smpi_start_data_exe), smpi_size_data_exe);

    // store the address of the mapping for further switches
    smpi_privatisation_regions[i].file_descriptor = file_descriptor;
    smpi_privatisation_regions[i].address         = address;
  }
#endif
}

void smpi_destroy_global_memory_segments(){
  if (smpi_size_data_exe == 0)//no need to switch
    return;
#if HAVE_PRIVATIZATION
  for (int i=0; i< smpi_process_count(); i++) {
    if (munmap(smpi_privatisation_regions[i].address, smpi_size_data_exe) < 0)
      XBT_WARN("Unmapping of fd %d failed: %s", smpi_privatisation_regions[i].file_descriptor, strerror(errno));
    close(smpi_privatisation_regions[i].file_descriptor);
  }
  xbt_free(smpi_privatisation_regions);
#endif
}

