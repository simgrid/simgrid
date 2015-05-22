/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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

#include "mc_process.h"
#include "mc_object_info.h"
#include "AddressSpace.hpp"
#include "mc_unw.h"
#include "mc_snapshot.h"
#include "mc_ignore.h"
#include "mc_smx.h"
#include "mc_server.h"

using simgrid::mc::remote;

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_process, mc,
                                "MC process information");

// ***** Helper stuff

#define SO_RE "\\.so[\\.0-9]*$"
#define VERSION_RE "-[\\.0-9]*$"

static const char *const FILTERED_LIBS[] = {
  "libstdc++",
  "libc++",
  "libm",
  "libgcc_s",
  "libpthread",
  "libunwind",
  "libunwind-x86_64",
  "libunwind-x86",
  "libunwind-ptrace",
  "libdw",
  "libdl",
  "librt",
  "liblzma",
  "libelf",
  "libbz2",
  "libz",
  "libelf",
  "libc",
  "ld"
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
  const char* map_basename = basename((char*) pathname);

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

static ssize_t pread_whole(int fd, void *buf, size_t count, off_t offset)
{
  char* buffer = (char*) buf;
  ssize_t real_count = count;
  while (count) {
    ssize_t res = pread(fd, buffer, count, offset);
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

Process::Process(pid_t pid, int sockfd)
{
  Process* process = this;

  process->process_flags = MC_PROCESS_NO_FLAG;
  process->socket = sockfd;
  process->pid = pid;
  if (pid==getpid())
    process->process_flags |= MC_PROCESS_SELF_FLAG;
  process->running = true;
  process->status = 0;
  process->memory_map = MC_get_memory_map(pid);
  process->cache_flags = MC_PROCESS_CACHE_FLAG_NONE;
  process->heap = NULL;
  process->heap_info = NULL;
  process->init_memory_map_info();

  // Open the memory file
  if (process->is_self())
    process->memory_file = -1;
  else {
    int fd = open_vm(process->pid, O_RDWR);
    if (fd<0)
      xbt_die("Could not open file for process virtual address space");
    process->memory_file = fd;
  }

  // Read std_heap (is a struct mdesc*):
  dw_variable_t std_heap_var = process->find_variable("__mmalloc_default_mdp");
  if (!std_heap_var)
    xbt_die("No heap information in the target process");
  if(!std_heap_var->address)
    xbt_die("No constant address for this variable");
  process->read_bytes(&process->heap_address, sizeof(struct mdesc*),
    remote(std_heap_var->address),
    simgrid::mc::ProcessIndexDisabled);

  process->smx_process_infos = MC_smx_process_info_list_new();
  process->smx_old_process_infos = MC_smx_process_info_list_new();

  process->unw_addr_space = unw_create_addr_space(&mc_unw_accessors  , __BYTE_ORDER);
  if (process->process_flags & MC_PROCESS_SELF_FLAG) {
    process->unw_underlying_addr_space = unw_local_addr_space;
    process->unw_underlying_context = NULL;
  } else {
    process->unw_underlying_addr_space = unw_create_addr_space(&mc_unw_vmread_accessors, __BYTE_ORDER);
    process->unw_underlying_context = _UPT_create(pid);
  }
}

Process::~Process()
{
  Process* process = this;

  process->process_flags = MC_PROCESS_NO_FLAG;
  process->pid = 0;

  MC_free_memory_map(process->memory_map);
  process->memory_map = NULL;

  process->maestro_stack_start = NULL;
  process->maestro_stack_end = NULL;

  xbt_dynar_free(&process->smx_process_infos);
  xbt_dynar_free(&process->smx_old_process_infos);

  size_t i;
  for (i=0; i!=process->object_infos_size; ++i) {
    MC_free_object_info(&process->object_infos[i]);
  }
  free(process->object_infos);
  process->object_infos = NULL;
  process->object_infos_size = 0;
  if (process->memory_file >= 0) {
    close(process->memory_file);
  }

  if (process->unw_underlying_addr_space != unw_local_addr_space) {
    unw_destroy_addr_space(process->unw_underlying_addr_space);
    _UPT_destroy(process->unw_underlying_context);
  }
  process->unw_underlying_context = NULL;
  process->unw_underlying_addr_space = NULL;

  unw_destroy_addr_space(process->unw_addr_space);
  process->unw_addr_space = NULL;

  process->cache_flags = MC_PROCESS_CACHE_FLAG_NONE;

  free(process->heap);
  process->heap = NULL;

  free(process->heap_info);
  process->heap_info = NULL;
}

/** Refresh the information about the process
 *
 *  Do not use direclty, this is used by the getters when appropriate
 *  in order to have fresh data.
 */
void Process::refresh_heap()
{
  xbt_assert(mc_mode == MC_MODE_SERVER);
  xbt_assert(!this->is_self());
  // Read/dereference/refresh the std_heap pointer:
  if (!this->heap) {
    this->heap = (struct mdesc*) malloc(sizeof(struct mdesc));
  }
  this->read_bytes(this->heap, sizeof(struct mdesc), remote(this->heap_address),
    simgrid::mc::ProcessIndexDisabled);
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
  xbt_assert(!this->is_self());
  if (!(this->cache_flags & MC_PROCESS_CACHE_FLAG_HEAP))
    this->refresh_heap();
  // Refresh process->heapinfo:
  size_t malloc_info_bytesize =
    (this->heap->heaplimit + 1) * sizeof(malloc_info);
  this->heap_info = (malloc_info*) realloc(this->heap_info, malloc_info_bytesize);
  this->read_bytes(this->heap_info, malloc_info_bytesize,
    remote(this->heap->heapinfo), simgrid::mc::ProcessIndexDisabled);
  this->cache_flags |= MC_PROCESS_CACHE_FLAG_MALLOC_INFO;
}

/** @brief Finds the range of the different memory segments and binary paths */
void Process::init_memory_map_info()
{
  XBT_DEBUG("Get debug information ...");
  this->maestro_stack_start = NULL;
  this->maestro_stack_end = NULL;
  this->object_infos = NULL;
  this->object_infos_size = 0;
  this->binary_info = NULL;
  this->libsimgrid_info = NULL;

  struct s_mc_memory_map_re res;

  if(regcomp(&res.so_re, SO_RE, 0) || regcomp(&res.version_re, VERSION_RE, 0))
    xbt_die(".so regexp did not compile");

  memory_map_t maps = this->memory_map;

  const char* current_name = NULL;

  for (ssize_t i=0; i < maps->mapsize; i++) {
    map_region_t reg = &(maps->regions[i]);
    const char* pathname = maps->regions[i].pathname;

    // Nothing to do
    if (maps->regions[i].pathname == NULL) {
      current_name = NULL;
      continue;
    }

    // [stack], [vvar], [vsyscall], [vdso] ...
    if (pathname[0] == '[') {
      if ((reg->prot & PROT_WRITE) && !memcmp(pathname, "[stack]", 7)) {
        this->maestro_stack_start = reg->start_addr;
        this->maestro_stack_end = reg->end_addr;
      }
      current_name = NULL;
      continue;
    }

    if (current_name && strcmp(current_name, pathname)==0)
      continue;

    current_name = pathname;
    if (!(reg->prot & PROT_READ) && (reg->prot & PROT_EXEC))
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

    mc_object_info_t info =
      MC_find_object_info(this->memory_map, pathname, is_executable);
    this->object_infos = (mc_object_info_t*) realloc(this->object_infos,
      (this->object_infos_size+1) * sizeof(mc_object_info_t*));
    this->object_infos[this->object_infos_size] = info;
    this->object_infos_size++;
    if (is_executable)
      this->binary_info = info;
    else if (libname && MC_is_simgrid_lib(libname))
      this->libsimgrid_info = info;
    free(libname);
  }

  regfree(&res.so_re);
  regfree(&res.version_re);

  // Resolve time (including accross differents objects):
  for (size_t i=0; i!=this->object_infos_size; ++i)
    MC_post_process_object_info(this, this->object_infos[i]);

  xbt_assert(this->maestro_stack_start, "Did not find maestro_stack_start");
  xbt_assert(this->maestro_stack_end, "Did not find maestro_stack_end");

  XBT_DEBUG("Get debug information done !");
}

mc_object_info_t Process::find_object_info(remote_ptr<void> addr) const
{
  size_t i;
  for (i = 0; i != this->object_infos_size; ++i) {
    if (addr.address() >= (std::uint64_t)this->object_infos[i]->start
        && addr.address() <= (std::uint64_t)this->object_infos[i]->end) {
      return this->object_infos[i];
    }
  }
  return NULL;
}

mc_object_info_t Process::find_object_info_exec(remote_ptr<void> addr) const
{
  size_t i;
  for (i = 0; i != this->object_infos_size; ++i) {
    if (addr.address() >= (std::uint64_t)this->object_infos[i]->start_exec
        && addr.address() <= (std::uint64_t)this->object_infos[i]->end_exec) {
      return this->object_infos[i];
    }
  }
  return NULL;
}

mc_object_info_t Process::find_object_info_rw(remote_ptr<void> addr) const
{
  size_t i;
  for (i = 0; i != this->object_infos_size; ++i) {
    if (addr.address() >= (std::uint64_t)this->object_infos[i]->start_rw
        && addr.address() <= (std::uint64_t)this->object_infos[i]->end_rw) {
      return this->object_infos[i];
    }
  }
  return NULL;
}

dw_frame_t Process::find_function(remote_ptr<void> ip) const
{
  mc_object_info_t info = this->find_object_info_exec(ip);
  if (info == NULL)
    return NULL;
  else
    return MC_file_object_info_find_function(info, (void*) ip.address());
}

/** Find (one occurence of) the named variable definition
 */
dw_variable_t Process::find_variable(const char* name) const
{
  const size_t n = this->object_infos_size;
  size_t i;

  // First lookup the variable in the executable shared object.
  // A global variable used directly by the executable code from a library
  // is reinstanciated in the executable memory .data/.bss.
  // We need to look up the variable in the execvutable first.
  if (this->binary_info) {
    mc_object_info_t info = this->binary_info;
    dw_variable_t var = MC_file_object_info_find_variable_by_name(info, name);
    if (var)
      return var;
  }

  for (i=0; i!=n; ++i) {
    mc_object_info_t info = this->object_infos[i];
    dw_variable_t var = MC_file_object_info_find_variable_by_name(info, name);
    if (var)
      return var;
  }

  return NULL;
}

void Process::read_variable(const char* name, void* target, size_t size) const
{
  dw_variable_t var = this->find_variable(name);
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
  if (this->is_self())
    return strdup((char*) address.address());

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
  AddressSpace::ReadMode mode) const
{
  if (process_index != simgrid::mc::ProcessIndexDisabled) {
    mc_object_info_t info = this->find_object_info_rw((void*)address.address());
    // Segment overlap is not handled.
    if (MC_object_info_is_privatized(info)) {
      if (process_index < 0)
        xbt_die("Missing process index");
      // Address translation in the privaization segment:
      // TODO, fix me (broken)
      size_t offset = address.address() - (std::uint64_t)info->start_rw;
      address = remote(address.address() - offset);
    }
  }

  if (this->is_self()) {
    if (mode == simgrid::mc::AddressSpace::Lazy)
      return (void*)address.address();
    else {
      memcpy(buffer, (void*)address.address(), size);
      return buffer;
    }
  } else {
    if (pread_whole(this->memory_file, buffer, size, (off_t) address.address()) < 0)
      xbt_die("Read from process %lli failed", (long long) this->pid);
    return buffer;
  }
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
  if (this->is_self()) {
    memcpy((void*)address.address(), buffer, len);
  } else {
    if (pwrite_whole(this->memory_file, buffer, len, address.address()) < 0)
      xbt_die("Write to process %lli failed", (long long) this->pid);
  }
}

void Process::clear_bytes(remote_ptr<void> address, size_t len)
{
  if (this->is_self()) {
    memset((void*)address.address(), 0, len);
  } else {
    pthread_once(&zero_buffer_flag, MC_zero_buffer_init);
    while (len) {
      size_t s = len > zero_buffer_size ? zero_buffer_size : len;
      this->write_bytes(zero_buffer, s, address);
      address = remote((char*) address.address() + s);
      len -= s;
    }
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

}
}
