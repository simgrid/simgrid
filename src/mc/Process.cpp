/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <sys/mman.h> // PROT_*

#include <pthread.h>

#include <libgen.h>

#include <libunwind.h>
#include <libunwind-ptrace.h>

#include <xbt/mmalloc.h>

#include "src/mc/mc_object_info.h"
#include "src/mc/mc_unw.h"
#include "src/mc/mc_snapshot.h"
#include "src/mc/mc_ignore.h"
#include "src/mc/mc_smx.h"

#include "src/mc/Process.hpp"
#include "src/mc/AddressSpace.hpp"
#include "src/mc/ObjectInformation.hpp"
#include "src/mc/Variable.hpp"

using simgrid::mc::remote;

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_process, mc,
                                "MC process information");

// ***** Helper stuff

#define SO_RE "\\.so[\\.0-9]*$"
#define VERSION_RE "-[\\.0-9-]*$"

static const char *const FILTERED_LIBS[] = {
  "ld",
  "libbz2",
  "libboost_chrono",
  "libboost_context",
  "libboost_system",
  "libboost_thread",
  "libc",
  "libc++",
  "libcdt",
  "libcgraph",
  "libdl",
  "libdw",
  "libelf",
  "libgcc_s",
  "liblua5.1",
  "liblua5.3",
  "liblzma",
  "libm",
  "libpthread",
  "librt",
  "libsigc",
  "libstdc++",
  "libunwind",
  "libunwind-x86_64",
  "libunwind-x86",
  "libunwind-ptrace",
  "libz"
};

static bool MC_is_simgrid_lib(const char* libname)
{
  return !strcmp(libname, "libsimgrid");
}

static bool MC_is_filtered_lib(const char* libname)
{
  for (const char* filtered_lib : FILTERED_LIBS)
    if (strcmp(libname, filtered_lib)==0)
      return true;
  return false;
}

struct s_mc_memory_map_re {
  regex_t so_re;
  regex_t version_re;
};

static char* MC_get_lib_name(const char* pathname, struct s_mc_memory_map_re* res)
{
  const char* map_basename = xbt_basename((char*) pathname);

  regmatch_t match;
  if(regexec(&res->so_re, map_basename, 1, &match, 0))
    return NULL;

  char* libname = strndup(map_basename, match.rm_so);

  // Strip the version suffix:
  if(libname && !regexec(&res->version_re, libname, 1, &match, 0)) {
    char* temp = libname;
    libname = strndup(temp, match.rm_so);
    free(temp);
  }

  return libname;
}

static ssize_t pread_whole(int fd, void *buf, size_t count, std::uint64_t offset)
{
  char* buffer = (char*) buf;
  ssize_t real_count = count;
  while (count) {
    ssize_t res = pread(fd, buffer, count, (std::int64_t) offset);
    if (res > 0) {
      count  -= res;
      buffer += res;
      offset += res;
    } else if (res==0) {
      return -1;
    } else if (errno != EINTR) {
      perror("pread_whole");
      return -1;
    }
  }
  return real_count;
}

static ssize_t pwrite_whole(int fd, const void *buf, size_t count, off_t offset)
{
  const char* buffer = (const char*) buf;
  ssize_t real_count = count;
  while (count) {
    ssize_t res = pwrite(fd, buffer, count, offset);
    if (res > 0) {
      count  -= res;
      buffer += res;
      offset += res;
    } else if (res==0) {
      return -1;
    } else if (errno != EINTR) {
      return -1;
    }
  }
  return real_count;
}

static pthread_once_t zero_buffer_flag = PTHREAD_ONCE_INIT;
static const void* zero_buffer;
static const size_t zero_buffer_size = 10 * 4096;

static void MC_zero_buffer_init(void)
{
  int fd = open("/dev/zero", O_RDONLY);
  if (fd<0)
    xbt_die("Could not open /dev/zero");
  zero_buffer = mmap(NULL, zero_buffer_size, PROT_READ, MAP_SHARED, fd, 0);
  if (zero_buffer == MAP_FAILED)
    xbt_die("Could not map the zero buffer");
  close(fd);
}

static
int open_process_file(pid_t pid, const char* file, int flags)
{
  char buff[50];
  snprintf(buff, sizeof(buff), "/proc/%li/%s", (long) pid, file);
  return open(buff, flags);
}

}

namespace simgrid {
namespace mc {

int open_vm(pid_t pid, int flags)
{
  const size_t buffer_size = 30;
  char buffer[buffer_size];
  int res = snprintf(buffer, buffer_size, "/proc/%lli/mem", (long long) pid);
  if (res < 0 || (size_t) res >= buffer_size) {
    errno = ENAMETOOLONG;
    return -1;
  }
  return open(buffer, flags);
}

  
}
}

// ***** Process

namespace simgrid {
namespace mc {

Process::Process(pid_t pid, int sockfd) :
  AddressSpace(this), socket_(sockfd), pid_(pid), running_(true)
{}

void Process::init()
{
  this->memory_map_ = simgrid::xbt::get_memory_map(this->pid_);
  this->init_memory_map_info();

  int fd = open_vm(this->pid_, O_RDWR);
  if (fd<0)
    xbt_die("Could not open file for process virtual address space");
  this->memory_file = fd;

  // Read std_heap (is a struct mdesc*):
  simgrid::mc::Variable* std_heap_var = this->find_variable("__mmalloc_default_mdp");
  if (!std_heap_var)
    xbt_die("No heap information in the target process");
  if(!std_heap_var->address)
    xbt_die("No constant address for this variable");
  this->read_bytes(&this->heap_address, sizeof(struct mdesc*),
    remote(std_heap_var->address),
    simgrid::mc::ProcessIndexDisabled);

  this->smx_process_infos = MC_smx_process_info_list_new();
  this->smx_old_process_infos = MC_smx_process_info_list_new();
  this->unw_addr_space = unw_create_addr_space(&mc_unw_accessors  , __BYTE_ORDER);
  this->unw_underlying_addr_space = unw_create_addr_space(&mc_unw_vmread_accessors, __BYTE_ORDER);
  this->unw_underlying_context = _UPT_create(this->pid_);
}

Process::~Process()
{
  if (this->socket_ >= 0 && close(this->socket_) < 0)
    xbt_die("Could not close communication socket");

  this->maestro_stack_start_ = nullptr;
  this->maestro_stack_end_ = nullptr;

  xbt_dynar_free(&this->smx_process_infos);
  xbt_dynar_free(&this->smx_old_process_infos);

  if (this->memory_file >= 0) {
    close(this->memory_file);
  }

  if (this->unw_underlying_addr_space != unw_local_addr_space) {
    unw_destroy_addr_space(this->unw_underlying_addr_space);
    _UPT_destroy(this->unw_underlying_context);
  }
  this->unw_underlying_context = NULL;
  this->unw_underlying_addr_space = NULL;

  unw_destroy_addr_space(this->unw_addr_space);
  this->unw_addr_space = NULL;

  this->cache_flags = MC_PROCESS_CACHE_FLAG_NONE;

  if (this->clear_refs_fd_ >= 0)
    close(this->clear_refs_fd_);
  if (this->pagemap_fd_ >= 0)
    close(this->pagemap_fd_);
}

/** Refresh the information about the process
 *
 *  Do not use direclty, this is used by the getters when appropriate
 *  in order to have fresh data.
 */
void Process::refresh_heap()
{
  xbt_assert(mc_mode == MC_MODE_SERVER);
  // Read/dereference/refresh the std_heap pointer:
  if (!this->heap)
    this->heap = std::unique_ptr<s_xbt_mheap_t>(new s_xbt_mheap_t());
  this->read_bytes(this->heap.get(), sizeof(struct mdesc),
    remote(this->heap_address), simgrid::mc::ProcessIndexDisabled);
  this->cache_flags |= MC_PROCESS_CACHE_FLAG_HEAP;
}

/** Refresh the information about the process
 *
 *  Do not use direclty, this is used by the getters when appropriate
 *  in order to have fresh data.
 * */
void Process::refresh_malloc_info()
{
  xbt_assert(mc_mode == MC_MODE_SERVER);
  if (!(this->cache_flags & MC_PROCESS_CACHE_FLAG_HEAP))
    this->refresh_heap();
  // Refresh process->heapinfo:
  size_t count = this->heap->heaplimit + 1;
  if (this->heap_info.size() < count)
    this->heap_info.resize(count);
  this->read_bytes(this->heap_info.data(), count * sizeof(malloc_info),
    remote(this->heap->heapinfo), simgrid::mc::ProcessIndexDisabled);
  this->cache_flags |= MC_PROCESS_CACHE_FLAG_MALLOC_INFO;
}

/** @brief Finds the range of the different memory segments and binary paths */
void Process::init_memory_map_info()
{
  XBT_DEBUG("Get debug information ...");
  this->maestro_stack_start_ = nullptr;
  this->maestro_stack_end_ = nullptr;
  this->object_infos.resize(0);
  this->binary_info = NULL;
  this->libsimgrid_info = NULL;

  struct s_mc_memory_map_re res;

  if(regcomp(&res.so_re, SO_RE, 0) || regcomp(&res.version_re, VERSION_RE, 0))
    xbt_die(".so regexp did not compile");

  std::vector<simgrid::xbt::VmMap> const& maps = this->memory_map_;

  const char* current_name = NULL;

  this->object_infos.resize(0);

  for (size_t i=0; i < maps.size(); i++) {
    simgrid::xbt::VmMap const& reg = maps[i];
    const char* pathname = maps[i].pathname.c_str();

    // Nothing to do
    if (maps[i].pathname.empty()) {
      current_name = NULL;
      continue;
    }

    // [stack], [vvar], [vsyscall], [vdso] ...
    if (pathname[0] == '[') {
      if ((reg.prot & PROT_WRITE) && !memcmp(pathname, "[stack]", 7)) {
        this->maestro_stack_start_ = remote(reg.start_addr);
        this->maestro_stack_end_ = remote(reg.end_addr);
      }
      current_name = NULL;
      continue;
    }

    if (current_name && strcmp(current_name, pathname)==0)
      continue;

    current_name = pathname;
    if (!(reg.prot & PROT_READ) && (reg.prot & PROT_EXEC))
      continue;

    const bool is_executable = !i;
    char* libname = NULL;
    if (!is_executable) {
      libname = MC_get_lib_name(pathname, &res);
      if(!libname)
        continue;
      if (MC_is_filtered_lib(libname)) {
        free(libname);
        continue;
      }
    }

    std::shared_ptr<simgrid::mc::ObjectInformation> info =
      MC_find_object_info(this->memory_map_, pathname);
    this->object_infos.push_back(info);
    if (is_executable)
      this->binary_info = info;
    else if (libname && MC_is_simgrid_lib(libname))
      this->libsimgrid_info = info;
    free(libname);
  }

  regfree(&res.so_re);
  regfree(&res.version_re);

  // Resolve time (including accross differents objects):
  for (auto const& object_info : this->object_infos)
    MC_post_process_object_info(this, object_info.get());

  xbt_assert(this->maestro_stack_start_, "Did not find maestro_stack_start");
  xbt_assert(this->maestro_stack_end_, "Did not find maestro_stack_end");

  XBT_DEBUG("Get debug information done !");
}

std::shared_ptr<simgrid::mc::ObjectInformation> Process::find_object_info(remote_ptr<void> addr) const
{
  for (auto const& object_info : this->object_infos) {
    if (addr.address() >= (std::uint64_t)object_info->start
        && addr.address() <= (std::uint64_t)object_info->end) {
      return object_info;
    }
  }
  return NULL;
}

std::shared_ptr<ObjectInformation> Process::find_object_info_exec(remote_ptr<void> addr) const
{
  for (std::shared_ptr<ObjectInformation> const& info : this->object_infos) {
    if (addr.address() >= (std::uint64_t) info->start_exec
        && addr.address() <= (std::uint64_t) info->end_exec) {
      return info;
    }
  }
  return nullptr;
}

std::shared_ptr<ObjectInformation> Process::find_object_info_rw(remote_ptr<void> addr) const
{
  for (std::shared_ptr<ObjectInformation> const& info : this->object_infos) {
    if (addr.address() >= (std::uint64_t)info->start_rw
        && addr.address() <= (std::uint64_t)info->end_rw) {
      return info;
    }
  }
  return nullptr;
}

simgrid::mc::Frame* Process::find_function(remote_ptr<void> ip) const
{
  std::shared_ptr<simgrid::mc::ObjectInformation> info = this->find_object_info_exec(ip);
  return info ? info->find_function((void*) ip.address()) : nullptr;
}

/** Find (one occurence of) the named variable definition
 */
simgrid::mc::Variable* Process::find_variable(const char* name) const
{
  // First lookup the variable in the executable shared object.
  // A global variable used directly by the executable code from a library
  // is reinstanciated in the executable memory .data/.bss.
  // We need to look up the variable in the execvutable first.
  if (this->binary_info) {
    std::shared_ptr<simgrid::mc::ObjectInformation> const& info = this->binary_info;
    simgrid::mc::Variable* var = info->find_variable(name);
    if (var)
      return var;
  }

  for (std::shared_ptr<simgrid::mc::ObjectInformation> const& info : this->object_infos) {
    simgrid::mc::Variable* var = info->find_variable(name);
    if (var)
      return var;
  }

  return NULL;
}

void Process::read_variable(const char* name, void* target, size_t size) const
{
  simgrid::mc::Variable* var = this->find_variable(name);
  if (!var->address)
    xbt_die("No simple location for this variable");
  if (!var->type->full_type)
    xbt_die("Partial type for %s, cannot check size", name);
  if ((size_t) var->type->full_type->byte_size != size)
    xbt_die("Unexpected size for %s (expected %zi, was %zi)",
      name, size, (size_t) var->type->full_type->byte_size);
  this->read_bytes(target, size, remote(var->address));
}

char* Process::read_string(remote_ptr<void> address) const
{
  if (!address)
    return NULL;

  off_t len = 128;
  char* res = (char*) malloc(len);
  off_t off = 0;

  while (1) {
    ssize_t c = pread(this->memory_file, res + off, len - off, (off_t) address.address() + off);
    if (c == -1) {
      if (errno == EINTR)
        continue;
      else
        xbt_die("Could not read from from remote process");
    }
    if (c==0)
      xbt_die("Could not read string from remote process");

    void* p = memchr(res + off, '\0', c);
    if (p)
      return res;

    off += c;
    if (off == len) {
      len *= 2;
      res = (char*) realloc(res, len);
    }
  }
}

const void *Process::read_bytes(void* buffer, std::size_t size,
  remote_ptr<void> address, int process_index,
  ReadOptions options) const
{
  if (process_index != simgrid::mc::ProcessIndexDisabled) {
    std::shared_ptr<simgrid::mc::ObjectInformation> const& info =
      this->find_object_info_rw((void*)address.address());
    // Segment overlap is not handled.
#ifdef HAVE_SMPI
    if (info.get() && this->privatized(*info)) {
      if (process_index < 0)
        xbt_die("Missing process index");
      if (process_index >= (int) MC_smpi_process_count())
        xbt_die("Invalid process index");

      // Read smpi_privatisation_regions from MCed:
      smpi_privatisation_region_t remote_smpi_privatisation_regions =
        mc_model_checker->process().read_variable<smpi_privatisation_region_t>(
          "smpi_privatisation_regions");

      s_smpi_privatisation_region_t privatisation_region =
        mc_model_checker->process().read<s_smpi_privatisation_region_t>(
          remote(remote_smpi_privatisation_regions + process_index));

      // Address translation in the privaization segment:
      size_t offset = address.address() - (std::uint64_t)info->start_rw;
      address = remote((char*)privatisation_region.address + offset);
    }
#endif
  }

  if (pread_whole(this->memory_file, buffer, size, address.address()) < 0)
    xbt_die("Read from process %lli failed", (long long) this->pid_);
  return buffer;
}

/** Write data to a process memory
 *
 *  @param process the process
 *  @param local   local memory address (source)
 *  @param remote  target process memory address (target)
 *  @param len     data size
 */
void Process::write_bytes(const void* buffer, size_t len, remote_ptr<void> address)
{
  if (pwrite_whole(this->memory_file, buffer, len, address.address()) < 0)
    xbt_die("Write to process %lli failed", (long long) this->pid_);
}

void Process::clear_bytes(remote_ptr<void> address, size_t len)
{
  pthread_once(&zero_buffer_flag, MC_zero_buffer_init);
  while (len) {
    size_t s = len > zero_buffer_size ? zero_buffer_size : len;
    this->write_bytes(zero_buffer, s, address);
    address = remote((char*) address.address() + s);
    len -= s;
  }
}

void Process::ignore_region(std::uint64_t addr, std::size_t size)
{
  IgnoredRegion region;
  region.addr = addr;
  region.size = size;

  if (ignored_regions_.empty()) {
    ignored_regions_.push_back(region);
    return;
  }

  unsigned int cursor = 0;
  IgnoredRegion* current_region = nullptr;

  int start = 0;
  int end = ignored_regions_.size() - 1;
  while (start <= end) {
    cursor = (start + end) / 2;
    current_region = &ignored_regions_[cursor];
    if (current_region->addr == addr) {
      if (current_region->size == size)
        return;
      else if (current_region->size < size)
        start = cursor + 1;
      else
        end = cursor - 1;
    } else if (current_region->addr < addr)
      start = cursor + 1;
    else
      end = cursor - 1;
  }

  std::size_t position;
  if (current_region->addr == addr) {
    if (current_region->size < size) {
      position = cursor + 1;
    } else {
      position = cursor;
    }
  } else if (current_region->addr < addr) {
    position = cursor + 1;
  } else {
    position = cursor;
  }
  ignored_regions_.insert(
    ignored_regions_.begin() + position, region);
}

void Process::reset_soft_dirty()
{
  if (this->clear_refs_fd_ < 0) {
    this->clear_refs_fd_ = open_process_file(pid_, "clear_refs", O_WRONLY|O_CLOEXEC);
    if (this->clear_refs_fd_ < 0)
      xbt_die("Could not open clear_refs file for soft-dirty tracking. Run as root?");
  }
  if(::write(this->clear_refs_fd_, "4\n", 2) != 2)
    xbt_die("Could not reset softdirty bits");
}

void Process::read_pagemap(uint64_t* pagemap, size_t page_start, size_t page_count)
{
  if (pagemap_fd_ < 0) {
    pagemap_fd_ = open_process_file(pid_, "pagemap", O_RDONLY|O_CLOEXEC);
    if (pagemap_fd_ < 0)
      xbt_die("Could not open pagemap file for soft-dirty tracking. Run as root?");
  }
  ssize_t bytesize = sizeof(uint64_t) * page_count;
  off_t offset = sizeof(uint64_t) * page_start;
  if (pread_whole(pagemap_fd_, pagemap, bytesize, offset) != bytesize)
    xbt_die("Could not read pagemap");
}

void Process::ignore_heap(IgnoredHeapRegion const& region)
{
  if (ignored_heap_.empty()) {
    ignored_heap_.push_back(std::move(region));
    return;
  }

  typedef std::vector<IgnoredHeapRegion>::size_type size_type;

  size_type start = 0;
  size_type end = ignored_heap_.size() - 1;

  // Binary search the position of insertion:
  size_type cursor;
  while (start <= end) {
    cursor = start + (end - start) / 2;
    auto& current_region = ignored_heap_[cursor];
    if (current_region.address == region.address)
      return;
    else if (current_region.address < region.address)
      start = cursor + 1;
    else if (cursor != 0)
      end = cursor - 1;
    // Avoid underflow:
    else
      break;
  }

  // Insert it mc_heap_ignore_region_t:
  if (ignored_heap_[cursor].address < region.address)
    ++cursor;
  ignored_heap_.insert( ignored_heap_.begin() + cursor, region);
}

void Process::unignore_heap(void *address, size_t size)
{
  typedef std::vector<IgnoredHeapRegion>::size_type size_type;

  size_type start = 0;
  size_type end = ignored_heap_.size() - 1;

  // Binary search:
  size_type cursor;
  while (start <= end) {
    cursor = (start + end) / 2;
    auto& region = ignored_heap_[cursor];
    if (region.address == address) {
      ignored_heap_.erase(ignored_heap_.begin() + cursor);
      return;
    } else if (region.address < address)
      start = cursor + 1;
    else if ((char *) region.address <= ((char *) address + size)) {
      ignored_heap_.erase(ignored_heap_.begin() + cursor);
      return;
    } else if (cursor != 0)
      end = cursor - 1;
    // Avoid underflow:
    else
      break;
  }
}

void Process::ignore_local_variable(const char *var_name, const char *frame_name)
{
  if (frame_name != nullptr && strcmp(frame_name, "*") == 0)
    frame_name = nullptr;
  for (std::shared_ptr<simgrid::mc::ObjectInformation> const& info :
      this->object_infos)
    info->remove_local_variable(var_name, frame_name);
}

}
}
