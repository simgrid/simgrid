/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define _FILE_OFFSET_BITS 64 /* needed for pread_whole to work as expected on 32bits */

#include "src/mc/remote/RemoteProcess.hpp"

#include "src/mc/sosp/Snapshot.hpp"
#include "xbt/file.hpp"
#include "xbt/log.h"

#include <fcntl.h>
#include <libunwind-ptrace.h>
#include <sys/mman.h> // PROT_*

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <memory>
#include <string>

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
    "libasn1",
    "libboost_chrono",
    "libboost_context",
    "libboost_context-mt",
    "libboost_stacktrace_addr2line",
    "libboost_stacktrace_backtrace",
    "libboost_system",
    "libboost_thread",
    "libboost_timer",
    "libbrotlicommon",
    "libbrotlidec",
    "libbz2",
    "libc",
    "libc++",
    "libcdt",
    "libcgraph",
    "libcom_err",
    "libcrypt",
    "libcrypto",
    "libcurl",
    "libcurl-gnutls",
    "libcxxrt",
    "libdebuginfod",
    "libdl",
    "libdw",
    "libelf",
    "libevent",
    "libexecinfo",
    "libffi",
    "libflang",
    "libflangrti",
    "libgcc_s",
    "libgmp",
    "libgnutls",
    "libgcrypt",
    "libgfortran",
    "libgpg-error",
    "libgssapi",
    "libgssapi_krb5",
    "libhcrypto",
    "libheimbase",
    "libheimntlm",
    "libhx509",
    "libhogweed",
    "libidn2",
    "libimf",
    "libintlc",
    "libirng",
    "libk5crypto",
    "libkeyutils",
    "libkrb5",
    "libkrb5support", /*odd behaviour on fedora rawhide ... remove these when fixed*/
    "liblber",
    "libldap",
    "libldap_r",
    "liblua5.1",
    "liblua5.3",
    "liblzma",
    "libm",
    "libmd",
    "libnettle",
    "libnghttp2",
    "libomp",
    "libp11-kit",
    "libpapi",
    "libpcre2",
    "libpfm",
    "libpgmath",
    "libpsl",
    "libpthread",
    "libquadmath",
    "libresolv",
    "libroken",
    "librt",
    "librtmp",
    "libsasl2",
    "libselinux",
    "libsqlite3",
    "libssh",
    "libssh2",
    "libssl",
    "libstdc++",
    "libsvml",
    "libtasn1",
    "libtsan",  /* gcc sanitizers */
    "libubsan", /* gcc sanitizers */
    "libunistring",
    "libunwind",
    "libunwind-ptrace",
    "libunwind-x86",
    "libunwind-x86_64",
    "libwind",
    "libz",
    "libzstd"};

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
  auto* buffer       = static_cast<char*>(buf);
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
      XBT_ERROR("pread_whole: %s", strerror(errno));
      return -1;
    }
  }
  return real_count;
}

static ssize_t pwrite_whole(int fd, const void* buf, size_t count, off_t offset)
{
  const auto* buffer = static_cast<const char*>(buf);
  ssize_t real_count = count;
  while (count) {
    ssize_t res = pwrite(fd, buffer, count, offset);
    if (res > 0) {
      count -= res;
      buffer += res;
      offset += res;
    } else if (res == 0)
      return -1;
    else if (errno != EINTR) {
      XBT_ERROR("pwrite_whole: %s", strerror(errno));
      return -1;
    }
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
  std::string buffer = "/proc/" + std::to_string(pid) + "/mem";
  return open(buffer.c_str(), flags);
}

// ***** RemoteProcess

RemoteProcess::RemoteProcess(pid_t pid) : AddressSpace(this), pid_(pid), running_(true) {}

void RemoteProcess::init(void* mmalloc_default_mdp, void* maxpid, void* actors, void* dead_actors)
{
  this->heap_address      = mmalloc_default_mdp;
  this->maxpid_addr_      = maxpid;
  this->actors_addr_      = actors;
  this->dead_actors_addr_ = dead_actors;

  this->memory_map_ = simgrid::xbt::get_memory_map(this->pid_);
  this->init_memory_map_info();

  int fd = open_vm(this->pid_, O_RDWR);
  xbt_assert(fd >= 0, "Could not open file for process virtual address space");
  this->memory_file = fd;

  this->smx_actors_infos.clear();
  this->smx_dead_actors_infos.clear();
  this->unw_addr_space            = simgrid::mc::UnwindContext::createUnwindAddressSpace();
  this->unw_underlying_addr_space = simgrid::unw::create_addr_space();
  this->unw_underlying_context    = simgrid::unw::create_context(this->unw_underlying_addr_space, this->pid_);
}

RemoteProcess::~RemoteProcess()
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
void RemoteProcess::refresh_heap()
{
  // Read/dereference/refresh the std_heap pointer:
  if (not this->heap)
    this->heap = std::make_unique<s_xbt_mheap_t>();
  this->read_bytes(this->heap.get(), sizeof(mdesc), remote(this->heap_address));
  this->cache_flags_ |= RemoteProcess::cache_heap;
}

/** Refresh the information about the process
 *
 *  Do not use directly, this is used by the getters when appropriate
 *  in order to have fresh data.
 * */
void RemoteProcess::refresh_malloc_info()
{
  // Refresh process->heapinfo:
  if (this->cache_flags_ & RemoteProcess::cache_malloc)
    return;
  size_t count = this->heap->heaplimit + 1;
  if (this->heap_info.size() < count)
    this->heap_info.resize(count);
  this->read_bytes(this->heap_info.data(), count * sizeof(malloc_info), remote(this->heap->heapinfo));
  this->cache_flags_ |= RemoteProcess::cache_malloc;
}

/** @brief Finds the range of the different memory segments and binary paths */
void RemoteProcess::init_memory_map_info()
{
  XBT_DEBUG("Get debug information ...");
  this->maestro_stack_start_ = nullptr;
  this->maestro_stack_end_   = nullptr;
  this->object_infos.resize(0);
  this->binary_info = nullptr;

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
  }

  xbt_assert(this->maestro_stack_start_, "Did not find maestro_stack_start");
  xbt_assert(this->maestro_stack_end_, "Did not find maestro_stack_end");

  XBT_DEBUG("Get debug information done !");
}

std::shared_ptr<simgrid::mc::ObjectInformation> RemoteProcess::find_object_info(RemotePtr<void> addr) const
{
  for (auto const& object_info : this->object_infos)
    if (addr.address() >= (std::uint64_t)object_info->start && addr.address() <= (std::uint64_t)object_info->end)
      return object_info;
  return nullptr;
}

std::shared_ptr<ObjectInformation> RemoteProcess::find_object_info_exec(RemotePtr<void> addr) const
{
  for (std::shared_ptr<ObjectInformation> const& info : this->object_infos)
    if (addr.address() >= (std::uint64_t)info->start_exec && addr.address() <= (std::uint64_t)info->end_exec)
      return info;
  return nullptr;
}

std::shared_ptr<ObjectInformation> RemoteProcess::find_object_info_rw(RemotePtr<void> addr) const
{
  for (std::shared_ptr<ObjectInformation> const& info : this->object_infos)
    if (addr.address() >= (std::uint64_t)info->start_rw && addr.address() <= (std::uint64_t)info->end_rw)
      return info;
  return nullptr;
}

simgrid::mc::Frame* RemoteProcess::find_function(RemotePtr<void> ip) const
{
  std::shared_ptr<simgrid::mc::ObjectInformation> info = this->find_object_info_exec(ip);
  return info ? info->find_function((void*)ip.address()) : nullptr;
}

/** Find (one occurrence of) the named variable definition
 */
const simgrid::mc::Variable* RemoteProcess::find_variable(const char* name) const
{
  // First lookup the variable in the executable shared object.
  // A global variable used directly by the executable code from a library
  // is reinstantiated in the executable memory .data/.bss.
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

void RemoteProcess::read_variable(const char* name, void* target, size_t size) const
{
  const simgrid::mc::Variable* var = this->find_variable(name);
  xbt_assert(var, "Variable %s not found", name);
  xbt_assert(var->address, "No simple location for this variable");

  if (not var->type->full_type) // Try to resolve this type. The needed ObjectInfo was maybe (lazily) loaded recently
    for (auto const& object_info : this->object_infos)
      postProcessObjectInformation(this, object_info.get());
  xbt_assert(var->type->full_type, "Partial type for %s (even after re-resolving types), cannot retrieve its size.",
             name);
  xbt_assert((size_t)var->type->full_type->byte_size == size, "Unexpected size for %s (expected %zu, received %zu).",
             name, size, (size_t)var->type->full_type->byte_size);
  this->read_bytes(target, size, remote(var->address));
}

std::string RemoteProcess::read_string(RemotePtr<char> address) const
{
  if (not address)
    return {};

  std::vector<char> res(128);
  off_t off = 0;

  while (true) {
    ssize_t c = pread(this->memory_file, res.data() + off, res.size() - off, (off_t)address.address() + off);
    if (c == -1 && errno == EINTR)
      continue;
    xbt_assert(c > 0, "Could not read string from remote process");

    const void* p = memchr(res.data() + off, '\0', c);
    if (p)
      return std::string(res.data());

    off += c;
    if (off == (off_t)res.size())
      res.resize(res.size() * 2);
  }
}

void* RemoteProcess::read_bytes(void* buffer, std::size_t size, RemotePtr<void> address, ReadOptions /*options*/) const
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
void RemoteProcess::write_bytes(const void* buffer, size_t len, RemotePtr<void> address) const
{
  if (pwrite_whole(this->memory_file, buffer, len, (size_t)address.address()) < 0)
    xbt_die("Write to process %lli failed", (long long)this->pid_);
}

void RemoteProcess::clear_bytes(RemotePtr<void> address, size_t len) const
{
  pthread_once(&zero_buffer_flag, zero_buffer_init);
  while (len) {
    size_t s = len > zero_buffer_size ? zero_buffer_size : len;
    this->write_bytes(zero_buffer, s, address);
    address = remote((char*)address.address() + s);
    len -= s;
  }
}

void RemoteProcess::ignore_region(std::uint64_t addr, std::size_t size)
{
  IgnoredRegion region;
  region.addr = addr;
  region.size = size;

  auto pos = std::lower_bound(ignored_regions_.begin(), ignored_regions_.end(), region,
                              [](auto const& reg1, auto const& reg2) {
                                return reg1.addr < reg2.addr || (reg1.addr == reg2.addr && reg1.size < reg2.size);
                              });
  if (pos == ignored_regions_.end() || pos->addr != addr || pos->size != size)
    ignored_regions_.insert(pos, region);
}

void RemoteProcess::ignore_heap(IgnoredHeapRegion const& region)
{
  // Binary search the position of insertion:
  auto pos = std::lower_bound(ignored_heap_.begin(), ignored_heap_.end(), region.address,
                              [](auto const& reg, auto const* addr) { return reg.address < addr; });
  if (pos == ignored_heap_.end() || pos->address != region.address) {
    // Insert it:
    ignored_heap_.insert(pos, region);
  }
}

void RemoteProcess::unignore_heap(void* address, size_t size)
{
  // Binary search:
  auto pos = std::lower_bound(ignored_heap_.begin(), ignored_heap_.end(), address,
                              [](auto const& reg, auto const* addr) { return reg.address < addr; });
  if (pos != ignored_heap_.end() && static_cast<char*>(pos->address) <= static_cast<char*>(address) + size)
    ignored_heap_.erase(pos);
}

void RemoteProcess::ignore_local_variable(const char* var_name, const char* frame_name) const
{
  if (frame_name != nullptr && strcmp(frame_name, "*") == 0)
    frame_name = nullptr;
  for (std::shared_ptr<simgrid::mc::ObjectInformation> const& info : this->object_infos)
    info->remove_local_variable(var_name, frame_name);
}

std::vector<simgrid::mc::ActorInformation>& RemoteProcess::actors()
{
  this->refresh_simix();
  return smx_actors_infos;
}

std::vector<simgrid::mc::ActorInformation>& RemoteProcess::dead_actors()
{
  this->refresh_simix();
  return smx_dead_actors_infos;
}

void RemoteProcess::dump_stack() const
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

  simgrid::mc::dumpStack(stderr, &cursor);

  _UPT_destroy(context);
  unw_destroy_addr_space(as);
}

unsigned long RemoteProcess::get_maxpid() const
{
  unsigned long maxpid;
  this->read_bytes(&maxpid, sizeof(unsigned long), remote(maxpid_addr_));
  return maxpid;
}

void RemoteProcess::get_actor_vectors(RemotePtr<s_xbt_dynar_t>& actors, RemotePtr<s_xbt_dynar_t>& dead_actors)
{
  actors      = remote(static_cast<s_xbt_dynar_t*>(actors_addr_));
  dead_actors = remote(static_cast<s_xbt_dynar_t*>(dead_actors_addr_));
}
} // namespace mc
} // namespace simgrid
