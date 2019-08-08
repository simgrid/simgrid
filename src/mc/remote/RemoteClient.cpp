/* Copyright (c) 2014-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define _FILE_OFFSET_BITS 64 /* needed for pread_whole to work as expected on 32bits */

#include "src/mc/remote/RemoteClient.hpp"

#include "src/mc/mc_smx.hpp"
#include "src/mc/sosp/Snapshot.hpp"
#include "xbt/file.hpp"
#include "xbt/log.h"

#include <fcntl.h>
#include <libunwind-ptrace.h>
#include <sys/mman.h> // PROT_*

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_process, mc, "MC process information");

namespace simgrid {
namespace mc {

// ***** Helper stuff

// List of library which memory segments are not considered:
static const std::vector<std::string> filtered_libraries = {
#ifdef __linux__
    "ld",
#elif defined __FreeBSD__
    "ld-elf",
    "ld-elf32",
    "libkvm",      /* kernel data access library */
    "libprocstat", /* process and file information retrieval */
    "libthr",      /* thread library */
    "libutil",
#endif
    "libargp", /* workarounds for glibc-less systems */
    "libasan", /* gcc sanitizers */
    "libboost_chrono",
    "libboost_context",
    "libboost_context-mt",
    "libboost_stacktrace_addr2line",
    "libboost_stacktrace_backtrace",
    "libboost_system",
    "libboost_thread",
    "libboost_timer",
    "libbz2",
    "libc",
    "libc++",
    "libcdt",
    "libcgraph",
    "libcrypto",
    "libcxxrt",
    "libdl",
    "libdw",
    "libelf",
    "libevent",
    "libexecinfo",
    "libflang",
    "libflangrti",
    "libgcc_s",
    "libgfortran",
    "libimf",
    "libintlc",
    "libirng",
    "liblua5.1",
    "liblua5.3",
    "liblzma",
    "libm",
    "libomp",
    "libpapi",
    "libpfm",
    "libpgmath",
    "libpthread",
    "libquadmath",
    "librt",
    "libstdc++",
    "libsvml",
    "libtsan",  /* gcc sanitizers */
    "libubsan", /* gcc sanitizers */
    "libunwind",
    "libunwind-ptrace",
    "libunwind-x86",
    "libunwind-x86_64",
    "libz"};

static bool is_simgrid_lib(const std::string& libname)
{
  return libname == "libsimgrid";
}

static bool is_filtered_lib(const std::string& libname)
{
  return std::find(begin(filtered_libraries), end(filtered_libraries), libname) != end(filtered_libraries);
}

static std::string get_lib_name(const std::string& pathname)
{
  std::string map_basename = simgrid::xbt::Path(pathname).get_base_name();
  std::string libname;

  size_t pos = map_basename.rfind(".so");
  if (pos != std::string::npos) {
    // strip the extension (matching regex "\.so.*$")
    libname.assign(map_basename, 0, pos);

    // strip the version suffix (matching regex "-[.0-9-]*$")
    while (true) {
      pos = libname.rfind('-');
      if (pos == std::string::npos || libname.find_first_not_of(".0123456789", pos + 1) != std::string::npos)
        break;
      libname.erase(pos);
    }
  }

  return libname;
}

static ssize_t pread_whole(int fd, void* buf, size_t count, off_t offset)
{
  char* buffer       = (char*)buf;
  ssize_t real_count = count;
  while (count) {
    ssize_t res = pread(fd, buffer, count, offset);
    if (res > 0) {
      count -= res;
      buffer += res;
      offset += res;
    } else if (res == 0)
      return -1;
    else if (errno != EINTR) {
      perror("pread_whole");
      return -1;
    }
  }
  return real_count;
}

static ssize_t pwrite_whole(int fd, const void* buf, size_t count, off_t offset)
{
  const char* buffer = (const char*)buf;
  ssize_t real_count = count;
  while (count) {
    ssize_t res = pwrite(fd, buffer, count, offset);
    if (res > 0) {
      count -= res;
      buffer += res;
      offset += res;
    } else if (res == 0)
      return -1;
    else if (errno != EINTR)
      return -1;
  }
  return real_count;
}

static pthread_once_t zero_buffer_flag = PTHREAD_ONCE_INIT;
static const void* zero_buffer;
static const size_t zero_buffer_size = 10 * 4096;

static void zero_buffer_init()
{
  int fd = open("/dev/zero", O_RDONLY);
  if (fd < 0)
    xbt_die("Could not open /dev/zero");
  zero_buffer = mmap(nullptr, zero_buffer_size, PROT_READ, MAP_SHARED, fd, 0);
  if (zero_buffer == MAP_FAILED)
    xbt_die("Could not map the zero buffer");
  close(fd);
}

int open_vm(pid_t pid, int flags)
{
  const size_t buffer_size = 30;
  char buffer[buffer_size];
  int res = snprintf(buffer, buffer_size, "/proc/%lli/mem", (long long)pid);
  if (res < 0 || (size_t)res >= buffer_size) {
    errno = ENAMETOOLONG;
    return -1;
  }
  return open(buffer, flags);
}

// ***** Process

RemoteClient::RemoteClient(pid_t pid, int sockfd) : AddressSpace(this), pid_(pid), channel_(sockfd), running_(true)
{
}

void RemoteClient::init()
{
  this->memory_map_ = simgrid::xbt::get_memory_map(this->pid_);
  this->init_memory_map_info();

  int fd = open_vm(this->pid_, O_RDWR);
  if (fd < 0)
    xbt_die("Could not open file for process virtual address space");
  this->memory_file = fd;

  // Read std_heap (is a struct mdesc*):
  const simgrid::mc::Variable* std_heap_var = this->find_variable("__mmalloc_default_mdp");
  if (not std_heap_var)
    xbt_die("No heap information in the target process");
  if (not std_heap_var->address)
    xbt_die("No constant address for this variable");
  this->read_bytes(&this->heap_address, sizeof(mdesc*), remote(std_heap_var->address));

  this->smx_actors_infos.clear();
  this->smx_dead_actors_infos.clear();
  this->unw_addr_space            = simgrid::mc::UnwindContext::createUnwindAddressSpace();
  this->unw_underlying_addr_space = simgrid::unw::create_addr_space();
  this->unw_underlying_context    = simgrid::unw::create_context(this->unw_underlying_addr_space, this->pid_);
}

RemoteClient::~RemoteClient()
{
  if (this->memory_file >= 0)
    close(this->memory_file);

  if (this->unw_underlying_addr_space != unw_local_addr_space) {
    if (this->unw_underlying_addr_space)
      unw_destroy_addr_space(this->unw_underlying_addr_space);
    if (this->unw_underlying_context)
      _UPT_destroy(this->unw_underlying_context);
  }

  unw_destroy_addr_space(this->unw_addr_space);
}

/** Refresh the information about the process
 *
 *  Do not use directly, this is used by the getters when appropriate
 *  in order to have fresh data.
 */
void RemoteClient::refresh_heap()
{
  // Read/dereference/refresh the std_heap pointer:
  if (not this->heap)
    this->heap.reset(new s_xbt_mheap_t());
  this->read_bytes(this->heap.get(), sizeof(mdesc), remote(this->heap_address));
  this->cache_flags_ |= RemoteClient::cache_heap;
}

/** Refresh the information about the process
 *
 *  Do not use direclty, this is used by the getters when appropriate
 *  in order to have fresh data.
 * */
void RemoteClient::refresh_malloc_info()
{
  // Refresh process->heapinfo:
  if (this->cache_flags_ & RemoteClient::cache_malloc)
    return;
  size_t count = this->heap->heaplimit + 1;
  if (this->heap_info.size() < count)
    this->heap_info.resize(count);
  this->read_bytes(this->heap_info.data(), count * sizeof(malloc_info), remote(this->heap->heapinfo));
  this->cache_flags_ |= RemoteClient::cache_malloc;
}

/** @brief Finds the range of the different memory segments and binary paths */
void RemoteClient::init_memory_map_info()
{
  XBT_DEBUG("Get debug information ...");
  this->maestro_stack_start_ = nullptr;
  this->maestro_stack_end_   = nullptr;
  this->object_infos.resize(0);
  this->binary_info     = nullptr;
  this->libsimgrid_info = nullptr;

  std::vector<simgrid::xbt::VmMap> const& maps = this->memory_map_;

  const char* current_name = nullptr;

  this->object_infos.clear();

  for (size_t i = 0; i < maps.size(); i++) {
    simgrid::xbt::VmMap const& reg = maps[i];
    const char* pathname           = maps[i].pathname.c_str();

    // Nothing to do
    if (maps[i].pathname.empty()) {
      current_name = nullptr;
      continue;
    }

    // [stack], [vvar], [vsyscall], [vdso] ...
    if (pathname[0] == '[') {
      if ((reg.prot & PROT_WRITE) && not memcmp(pathname, "[stack]", 7)) {
        this->maestro_stack_start_ = remote(reg.start_addr);
        this->maestro_stack_end_   = remote(reg.end_addr);
      }
      current_name = nullptr;
      continue;
    }

    if (current_name && strcmp(current_name, pathname) == 0)
      continue;

    current_name = pathname;
    if (not(reg.prot & PROT_READ) && (reg.prot & PROT_EXEC))
      continue;

    const bool is_executable = not i;
    std::string libname;
    if (not is_executable) {
      libname = get_lib_name(pathname);
      if (is_filtered_lib(libname)) {
        continue;
      }
    }

    std::shared_ptr<simgrid::mc::ObjectInformation> info =
        simgrid::mc::createObjectInformation(this->memory_map_, pathname);
    this->object_infos.push_back(info);
    if (is_executable)
      this->binary_info = info;
    else if (is_simgrid_lib(libname))
      this->libsimgrid_info = info;
  }

  // Resolve time (including across different objects):
  for (auto const& object_info : this->object_infos)
    postProcessObjectInformation(this, object_info.get());

  xbt_assert(this->maestro_stack_start_, "Did not find maestro_stack_start");
  xbt_assert(this->maestro_stack_end_, "Did not find maestro_stack_end");

  XBT_DEBUG("Get debug information done !");
}

std::shared_ptr<simgrid::mc::ObjectInformation> RemoteClient::find_object_info(RemotePtr<void> addr) const
{
  for (auto const& object_info : this->object_infos)
    if (addr.address() >= (std::uint64_t)object_info->start && addr.address() <= (std::uint64_t)object_info->end)
      return object_info;
  return nullptr;
}

std::shared_ptr<ObjectInformation> RemoteClient::find_object_info_exec(RemotePtr<void> addr) const
{
  for (std::shared_ptr<ObjectInformation> const& info : this->object_infos)
    if (addr.address() >= (std::uint64_t)info->start_exec && addr.address() <= (std::uint64_t)info->end_exec)
      return info;
  return nullptr;
}

std::shared_ptr<ObjectInformation> RemoteClient::find_object_info_rw(RemotePtr<void> addr) const
{
  for (std::shared_ptr<ObjectInformation> const& info : this->object_infos)
    if (addr.address() >= (std::uint64_t)info->start_rw && addr.address() <= (std::uint64_t)info->end_rw)
      return info;
  return nullptr;
}

simgrid::mc::Frame* RemoteClient::find_function(RemotePtr<void> ip) const
{
  std::shared_ptr<simgrid::mc::ObjectInformation> info = this->find_object_info_exec(ip);
  return info ? info->find_function((void*)ip.address()) : nullptr;
}

/** Find (one occurrence of) the named variable definition
 */
const simgrid::mc::Variable* RemoteClient::find_variable(const char* name) const
{
  // First lookup the variable in the executable shared object.
  // A global variable used directly by the executable code from a library
  // is reinstanciated in the executable memory .data/.bss.
  // We need to look up the variable in the executable first.
  if (this->binary_info) {
    std::shared_ptr<simgrid::mc::ObjectInformation> const& info = this->binary_info;
    const simgrid::mc::Variable* var                            = info->find_variable(name);
    if (var)
      return var;
  }

  for (std::shared_ptr<simgrid::mc::ObjectInformation> const& info : this->object_infos) {
    const simgrid::mc::Variable* var = info->find_variable(name);
    if (var)
      return var;
  }

  return nullptr;
}

void RemoteClient::read_variable(const char* name, void* target, size_t size) const
{
  const simgrid::mc::Variable* var = this->find_variable(name);
  xbt_assert(var, "Variable %s not found", name);
  xbt_assert(var->address, "No simple location for this variable");
  xbt_assert(var->type->full_type, "Partial type for %s, cannot check size", name);
  xbt_assert((size_t)var->type->full_type->byte_size == size, "Unexpected size for %s (expected %zu, was %zu)", name,
             size, (size_t)var->type->full_type->byte_size);
  this->read_bytes(target, size, remote(var->address));
}

std::string RemoteClient::read_string(RemotePtr<char> address) const
{
  if (not address)
    return {};

  std::vector<char> res(128);
  off_t off = 0;

  while (1) {
    ssize_t c = pread(this->memory_file, res.data() + off, res.size() - off, (off_t)address.address() + off);
    if (c == -1) {
      if (errno == EINTR)
        continue;
      else
        xbt_die("Could not read from from remote process");
    }
    if (c == 0)
      xbt_die("Could not read string from remote process");

    void* p = memchr(res.data() + off, '\0', c);
    if (p)
      return std::string(res.data());

    off += c;
    if (off == (off_t)res.size())
      res.resize(res.size() * 2);
  }
}

void* RemoteClient::read_bytes(void* buffer, std::size_t size, RemotePtr<void> address, ReadOptions /*options*/) const
{
  if (pread_whole(this->memory_file, buffer, size, (size_t)address.address()) < 0)
    xbt_die("Read at %p from process %lli failed", (void*)address.address(), (long long)this->pid_);
  return buffer;
}

/** Write data to a process memory
 *
 *  @param buffer   local memory address (source)
 *  @param len      data size
 *  @param address  target process memory address (target)
 */
void RemoteClient::write_bytes(const void* buffer, size_t len, RemotePtr<void> address)
{
  if (pwrite_whole(this->memory_file, buffer, len, (size_t)address.address()) < 0)
    xbt_die("Write to process %lli failed", (long long)this->pid_);
}

void RemoteClient::clear_bytes(RemotePtr<void> address, size_t len)
{
  pthread_once(&zero_buffer_flag, zero_buffer_init);
  while (len) {
    size_t s = len > zero_buffer_size ? zero_buffer_size : len;
    this->write_bytes(zero_buffer, s, address);
    address = remote((char*)address.address() + s);
    len -= s;
  }
}

void RemoteClient::ignore_region(std::uint64_t addr, std::size_t size)
{
  IgnoredRegion region;
  region.addr = addr;
  region.size = size;

  if (ignored_regions_.empty()) {
    ignored_regions_.push_back(region);
    return;
  }

  unsigned int cursor           = 0;
  IgnoredRegion* current_region = nullptr;

  int start = 0;
  int end   = ignored_regions_.size() - 1;
  while (start <= end) {
    cursor         = (start + end) / 2;
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
    if (current_region->size < size)
      position = cursor + 1;
    else
      position = cursor;
  } else if (current_region->addr < addr)
    position = cursor + 1;
  else
    position = cursor;
  ignored_regions_.insert(ignored_regions_.begin() + position, region);
}

void RemoteClient::ignore_heap(IgnoredHeapRegion const& region)
{
  if (ignored_heap_.empty()) {
    ignored_heap_.push_back(std::move(region));
    return;
  }

  typedef std::vector<IgnoredHeapRegion>::size_type size_type;

  size_type start = 0;
  size_type end   = ignored_heap_.size() - 1;

  // Binary search the position of insertion:
  size_type cursor;
  while (start <= end) {
    cursor               = start + (end - start) / 2;
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
  ignored_heap_.insert(ignored_heap_.begin() + cursor, region);
}

void RemoteClient::unignore_heap(void* address, size_t size)
{
  typedef std::vector<IgnoredHeapRegion>::size_type size_type;

  size_type start = 0;
  size_type end   = ignored_heap_.size() - 1;

  // Binary search:
  size_type cursor;
  while (start <= end) {
    cursor       = (start + end) / 2;
    auto& region = ignored_heap_[cursor];
    if (region.address < address)
      start = cursor + 1;
    else if ((char*)region.address <= ((char*)address + size)) {
      ignored_heap_.erase(ignored_heap_.begin() + cursor);
      return;
    } else if (cursor != 0)
      end = cursor - 1;
    // Avoid underflow:
    else
      break;
  }
}

void RemoteClient::ignore_local_variable(const char* var_name, const char* frame_name)
{
  if (frame_name != nullptr && strcmp(frame_name, "*") == 0)
    frame_name = nullptr;
  for (std::shared_ptr<simgrid::mc::ObjectInformation> const& info : this->object_infos)
    info->remove_local_variable(var_name, frame_name);
}

std::vector<simgrid::mc::ActorInformation>& RemoteClient::actors()
{
  this->refresh_simix();
  return smx_actors_infos;
}

std::vector<simgrid::mc::ActorInformation>& RemoteClient::dead_actors()
{
  this->refresh_simix();
  return smx_dead_actors_infos;
}

void RemoteClient::dump_stack()
{
  unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, BYTE_ORDER);
  if (as == nullptr) {
    XBT_ERROR("Could not initialize ptrace address space");
    return;
  }

  void* context = _UPT_create(this->pid_);
  if (context == nullptr) {
    unw_destroy_addr_space(as);
    XBT_ERROR("Could not initialize ptrace context");
    return;
  }

  unw_cursor_t cursor;
  if (unw_init_remote(&cursor, as, context) != 0) {
    _UPT_destroy(context);
    unw_destroy_addr_space(as);
    XBT_ERROR("Could not initialiez ptrace cursor");
    return;
  }

  simgrid::mc::dumpStack(stderr, std::move(cursor));

  _UPT_destroy(context);
  unw_destroy_addr_space(as);
}

bool RemoteClient::actor_is_enabled(aid_t pid)
{
  s_mc_message_actor_enabled_t msg{MC_MESSAGE_ACTOR_ENABLED, pid};
  process()->get_channel().send(msg);
  char buff[MC_MESSAGE_LENGTH];
  ssize_t received = process()->get_channel().receive(buff, MC_MESSAGE_LENGTH, true);
  xbt_assert(received == sizeof(s_mc_message_int_t), "Unexpected size in answer to ACTOR_ENABLED");
  return ((s_mc_message_int_t*)buff)->value;
}
}
}
