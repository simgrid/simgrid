/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>
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
#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_memory, smpi, "Memory layout support for SMPI");

int smpi_loaded_page      = -1;
char* smpi_data_exe_start = nullptr;
int smpi_data_exe_size    = 0;
SmpiPrivStrategies smpi_privatize_global_variables;
static void* smpi_data_exe_copy;

// Initialized by smpi_prepare_global_memory_segment().
static std::vector<simgrid::xbt::VmMap> initial_vm_map;

// We keep a copy of all the privatization regions: We can then delete everything easily by iterating over this
// collection and nothing can be leaked. We could also iterate over all actors but we would have to be diligent when two
// actors use the same privatization region (so, smart pointers would have to be used etc.)
// Use a std::deque so that pointers remain valid after push_back().
static std::deque<s_smpi_privatization_region_t> smpi_privatization_regions;

static constexpr int PROT_RWX = PROT_READ | PROT_WRITE | PROT_EXEC;
static constexpr int PROT_RW  = PROT_READ | PROT_WRITE;

/** Take a snapshot of the process' memory map.
 */
void smpi_prepare_global_memory_segment()
{
  initial_vm_map = simgrid::xbt::get_memory_map(getpid());
}

static void smpi_get_executable_global_size()
{
  char buffer[PATH_MAX];
  const char* full_name = realpath(simgrid::xbt::binary_name.c_str(), buffer);
  xbt_assert(full_name != nullptr, "Could not resolve real path of binary file '%s'",
             simgrid::xbt::binary_name.c_str());

  std::vector<simgrid::xbt::VmMap> map = simgrid::xbt::get_memory_map(getpid());
  for (auto i = map.begin(); i != map.end() ; ++i) {
    // TODO, In practice, this implementation would not detect a completely
    // anonymous data segment. This does not happen in practice, however.

    // File backed RW entry:
    if (i->pathname == full_name && (i->prot & PROT_RWX) == PROT_RW) {
      smpi_data_exe_start = (char*)i->start_addr;
      smpi_data_exe_size  = i->end_addr - i->start_addr;
      /* Here we are making the assumption that a suitable empty region
         following the rw- area is the end of the data segment. It would
         be better to check with the size of the data segment. */
      ++i;
      if (i != map.end() && i->pathname.empty() && (i->prot & PROT_RWX) == PROT_RW &&
          (char*)i->start_addr == smpi_data_exe_start + smpi_data_exe_size) {
        // Only count the portion of this region not present in the initial map.
        auto found = std::find_if(initial_vm_map.begin(), initial_vm_map.end(), [&i](const simgrid::xbt::VmMap& m) {
          return i->start_addr <= m.start_addr && m.start_addr < i->end_addr;
        });
        auto end_addr      = (found == initial_vm_map.end() ? i->end_addr : found->start_addr);
        smpi_data_exe_size = (char*)end_addr - smpi_data_exe_start;
      }
      return;
    }
  }
  xbt_die("Did not find my data segment.");
}
#endif

#if HAVE_SANITIZER_ADDRESS
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
#define asan_safe_memcpy(dest, src, n) memcpy((dest), (src), (n))
#endif

/**
 * @brief Uses shm_open to get a temporary shm, and returns its file descriptor.
 */
int smpi_temp_shm_get()
{
  constexpr unsigned VAL_MASK = 0xffffffffUL;
  static unsigned prev_val    = VAL_MASK;
  char shmname[32]; // cannot be longer than PSHMNAMLEN = 31 on macOS (shm_open raises ENAMETOOLONG otherwise)
  int fd;

  for (unsigned i = (prev_val + 1) & VAL_MASK; i != prev_val; i = (i + 1) & VAL_MASK) {
    snprintf(shmname, sizeof(shmname), "/smpi-buffer-%016x", i);
    fd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd != -1 || errno != EEXIST) {
      prev_val = i;
      break;
    }
  }
  if (fd < 0) {
    if (errno == EMFILE) {
      xbt_die("Impossible to create temporary file for memory mapping: %s\n\
The shm_open() system call failed with the EMFILE error code (too many files). \n\n\
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
    xbt_die("Impossible to create temporary file for memory mapping. shm_open: %s", strerror(errno));
  }
  XBT_DEBUG("Got temporary shm %s (fd = %d)", shmname, fd);
  if (shm_unlink(shmname) < 0)
    XBT_WARN("Could not early unlink %s. shm_unlink: %s", shmname, strerror(errno));
  return fd;
}

/**
 * @brief Mmap a region of size bytes from temporary shm with file descriptor fd.
 */
void* smpi_temp_shm_mmap(int fd, size_t size)
{
  struct stat st;
  if (fstat(fd, &st) != 0)
    xbt_die("Could not stat fd %d: %s", fd, strerror(errno));
  if (static_cast<off_t>(size) > st.st_size && ftruncate(fd, static_cast<off_t>(size)) != 0)
    xbt_die("Could not truncate fd %d to %zu: %s", fd, size, strerror(errno));
  void* mem = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mem == MAP_FAILED) {
    xbt_die("Failed to map fd %d with size %zu: %s\n"
            "If you are running a lot of ranks, you may be exceeding the amount of mappings allowed per process.\n"
            "On Linux systems, change this value with sudo sysctl -w vm.max_map_count=newvalue (default value: 65536)\n"
            "Please see "
            "https://simgrid.org/doc/latest/Configuring_SimGrid.html#configuring-the-user-code-virtualization for more "
            "information.",
            fd, size, strerror(errno));
  }
  return mem;
}

/** Map a given SMPI privatization segment (make a SMPI process active)
 *
 *  When doing a state restoration, the state of the restored variables  might not be consistent with the state of the
 *  virtual memory. In this case, we to change the data segment.
 */
void smpi_switch_data_segment(simgrid::s4u::ActorPtr actor)
{
  if (smpi_loaded_page == actor->get_pid()) // no need to switch, we've already loaded the one we want
    return;

  if (smpi_data_exe_size == 0) // no need to switch
    return;

#if HAVE_PRIVATIZATION
  // FIXME, cross-process support (mmap across process when necessary)
  XBT_DEBUG("Switching data frame to the one of process %ld", actor->get_pid());
  const simgrid::smpi::ActorExt* process = smpi_process_remote(actor);
  int current                     = process->privatized_region()->file_descriptor;
  const void* tmp = mmap(TOPAGE(smpi_data_exe_start), smpi_data_exe_size, PROT_RW, MAP_FIXED | MAP_SHARED, current, 0);
  if (tmp != TOPAGE(smpi_data_exe_start))
    xbt_die("Couldn't map the new region (errno %d): %s", errno, strerror(errno));
  smpi_loaded_page = actor->get_pid();
#endif
}

/**
 * @brief Makes a backup of the segment in memory that stores the global variables of a process.
 *        This backup is then used to initialize the global variables for every single
 *        process that is added, regardless of the progress of the simulation.
 */
void smpi_backup_global_memory_segment()
{
#if HAVE_PRIVATIZATION
  smpi_get_executable_global_size();
  initial_vm_map.clear();
  initial_vm_map.shrink_to_fit();

  XBT_DEBUG("bss+data segment found : size %d starting at %p", smpi_data_exe_size, smpi_data_exe_start);

  if (smpi_data_exe_size == 0) { // no need to do anything as global variables don't exist
    smpi_privatize_global_variables = SmpiPrivStrategies::NONE;
    return;
  }

  smpi_data_exe_copy = ::operator new(smpi_data_exe_size);
  // Make a copy of the data segment. This clean copy is retained over the whole runtime
  // of the simulation and can be used to initialize a dynamically added, new process.
  asan_safe_memcpy(smpi_data_exe_copy, TOPAGE(smpi_data_exe_start), smpi_data_exe_size);
#else /* ! HAVE_PRIVATIZATION */
  xbt_die("You are trying to use privatization on a system that does not support it. Don't.");
#endif
}

// Initializes the memory mapping for a single process and returns the privatization region
smpi_privatization_region_t smpi_init_global_memory_segment_process()
{
  int file_descriptor = smpi_temp_shm_get();

  // ask for a free region
  void* address = smpi_temp_shm_mmap(file_descriptor, smpi_data_exe_size);

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

static std::vector<unsigned char> sendbuffer;
static std::vector<unsigned char> recvbuffer;

//allocate a single buffer for all sends, growing it if needed
unsigned char* smpi_get_tmp_sendbuffer(size_t size)
{
  if (not smpi_process()->replaying())
    return new unsigned char[size];
  // FIXME: a resize() may invalidate a previous pointer. Maybe we need to handle a queue of buffers with a reference
  // counter. The same holds for smpi_get_tmp_recvbuffer.
  if (sendbuffer.size() < size)
    sendbuffer.resize(size);
  return sendbuffer.data();
}

//allocate a single buffer for all recv
unsigned char* smpi_get_tmp_recvbuffer(size_t size)
{
  if (not smpi_process()->replaying())
    return new unsigned char[size];
  if (recvbuffer.size() < size)
    recvbuffer.resize(size);
  return recvbuffer.data();
}

void smpi_free_tmp_buffer(const unsigned char* buf)
{
  if (not smpi_process()->replaying())
    delete[] buf;
}

void smpi_free_replay_tmp_buffers()
{
  std::vector<unsigned char>().swap(sendbuffer);
  std::vector<unsigned char>().swap(recvbuffer);
}
