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
#include "mc_address_space.h"
#include "mc_unw.h"
#include "mc_snapshot.h"
#include "mc_ignore.h"

#include "simix/smx_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_process, mc,
                                "MC process information");

static void MC_process_init_memory_map_info(mc_process_t process);
static void MC_process_open_memory_file(mc_process_t process);

// ***** Destructor callbacks

// ***** mc_address_space methods for mc_process

static mc_process_t MC_process_get_process(mc_process_t p) {
  return p;
}

static const s_mc_address_space_class_t mc_process_class = {
  .read = (void*) &MC_process_read,
  .get_process = (void*) MC_process_get_process
};

bool MC_is_process(mc_address_space_t p)
{
  return p->address_space_class == &mc_process_class;
}

// ***** mc_process

void MC_process_init(mc_process_t process, pid_t pid, int sockfd)
{
  process->address_space.address_space_class = &mc_process_class;
  process->process_flags = MC_PROCESS_NO_FLAG;
  process->socket = sockfd;
  process->pid = pid;
  if (pid==getpid())
    process->process_flags |= MC_PROCESS_SELF_FLAG;
  process->running = true;
  process->status = 0;
  process->memory_map = MC_get_memory_map(pid);
  process->memory_file = -1;
  process->cache_flags = 0;
  process->heap = NULL;
  process->heap_info = NULL;
  MC_process_init_memory_map_info(process);
  MC_process_open_memory_file(process);

  // Read std_heap (is a struct mdesc*):
  dw_variable_t std_heap_var = MC_process_find_variable_by_name(process, "std_heap");
  if (!std_heap_var)
    xbt_die("No heap information in the target process");
  if(!std_heap_var->address)
    xbt_die("No constant address for this variable");
  MC_process_read(process, MC_ADDRESS_SPACE_READ_FLAGS_NONE,
    &process->heap_address, std_heap_var->address, sizeof(struct mdesc*),
    MC_PROCESS_INDEX_DISABLED);

  process->smx_process_infos = mc_smx_process_info_list_new();
  process->smx_old_process_infos = mc_smx_process_info_list_new();

  process->checkpoint_ignore = MC_checkpoint_ignore_new();

  process->unw_addr_space = unw_create_addr_space(&mc_unw_accessors  , __BYTE_ORDER);
  if (process->process_flags & MC_PROCESS_SELF_FLAG) {
    process->unw_underlying_addr_space = unw_local_addr_space;
    process->unw_underlying_context = NULL;
  } else {
    process->unw_underlying_addr_space = unw_create_addr_space(&mc_unw_vmread_accessors, __BYTE_ORDER);
    process->unw_underlying_context = _UPT_create(pid);
  }
}

void MC_process_clear(mc_process_t process)
{
  process->address_space.address_space_class = NULL;
  process->process_flags = MC_PROCESS_NO_FLAG;
  process->pid = 0;

  MC_free_memory_map(process->memory_map);
  process->memory_map = NULL;

  process->maestro_stack_start = NULL;
  process->maestro_stack_end = NULL;

  xbt_dynar_free(&process->checkpoint_ignore);

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

  process->cache_flags = 0;

  free(process->heap);
  process->heap = NULL;

  free(process->heap_info);
  process->heap_info = NULL;
}

void MC_process_refresh_heap(mc_process_t process)
{
  assert(!MC_process_is_self(process));
  // Read/dereference/refresh the std_heap pointer:
  if (!process->heap) {
    xbt_mheap_t oldheap  = mmalloc_set_current_heap(mc_heap);
    process->heap = malloc(sizeof(struct mdesc));
    mmalloc_set_current_heap(oldheap);
  }
  MC_process_read(process, MC_ADDRESS_SPACE_READ_FLAGS_NONE,
    process->heap, process->heap_address, sizeof(struct mdesc),
    MC_PROCESS_INDEX_DISABLED
    );
}

void MC_process_refresh_malloc_info(mc_process_t process)
{
  assert(!MC_process_is_self(process));
  if (!process->cache_flags & MC_PROCESS_CACHE_FLAG_HEAP)
    MC_process_refresh_heap(process);
  // Refresh process->heapinfo:
  size_t malloc_info_bytesize = process->heap->heaplimit * sizeof(malloc_info);

  xbt_mheap_t heap  = mmalloc_set_current_heap(mc_heap);
  process->heap_info = (malloc_info*) realloc(process->heap_info,
    malloc_info_bytesize);
  mmalloc_set_current_heap(heap);

  MC_process_read(process, MC_ADDRESS_SPACE_READ_FLAGS_NONE,
    process->heap_info,
    process->heap->heapinfo, malloc_info_bytesize,
    MC_PROCESS_INDEX_DISABLED);
}

/** Load the remote swag of processes into a dynar
 *
 *  @param process     MCed process
 *  @param target      Local dynar (to be filled with copies of `s_smx_process_t`)
 *  @param remote_swag Address of the process SWAG in the remote list
 */
static void MC_process_refresh_simix_process_list(
  mc_process_t process,
  xbt_dynar_t target, xbt_swag_t remote_swag)
{
  // swag = REMOTE(*simix_global->process_list)
  s_xbt_swag_t swag;
  MC_process_read(process, MC_PROCESS_NO_FLAG, &swag, remote_swag, sizeof(swag),
    MC_PROCESS_INDEX_ANY);

  smx_process_t p;
  xbt_dynar_reset(target);

  int i = 0;
  for (p = swag.head; p; ++i) {

    s_mc_smx_process_info_t info;
    info.address = p;
    MC_process_read(process, MC_PROCESS_NO_FLAG,
      &info.copy, p, sizeof(info.copy), MC_PROCESS_INDEX_ANY);
    xbt_dynar_push(target, &info);

    // Lookup next process address:
    p = xbt_swag_getNext(&info.copy, swag.offset);
  }
  assert(i == swag.count);
}

void MC_process_refresh_simix_processes(mc_process_t process)
{
  if (process->cache_flags & MC_PROCESS_CACHE_FLAG_SIMIX_PROCESSES)
    return;

  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  // TODO, avoid to reload `&simix_global`, `simix_global`, `*simix_global`

  // simix_global_p = REMOTE(simix_global);
  smx_global_t simix_global_p;
  MC_process_read_variable(process, "simix_global", &simix_global_p, sizeof(simix_global_p));

  // simix_global = REMOTE(*simix_global)
  s_smx_global_t simix_global;
  MC_process_read(process, MC_PROCESS_NO_FLAG, &simix_global, simix_global_p, sizeof(simix_global),
    MC_PROCESS_INDEX_ANY);

  MC_process_refresh_simix_process_list(
    process, process->smx_process_infos, simix_global.process_list);
  MC_process_refresh_simix_process_list(
    process, process->smx_old_process_infos, simix_global.process_to_destroy);

  process->cache_flags |= MC_PROCESS_CACHE_FLAG_SIMIX_PROCESSES;
  mmalloc_set_current_heap(heap);
}

#define SO_RE "\\.so[\\.0-9]*$"
#define VERSION_RE "-[\\.0-9]*$"

const char* FILTERED_LIBS[] = {
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
  const size_t n = sizeof(FILTERED_LIBS) / sizeof(const char*);
  size_t i;
  for (i=0; i!=n; ++i)
    if (strcmp(libname, FILTERED_LIBS[i])==0)
      return true;
  return false;
}

struct s_mc_memory_map_re {
  regex_t so_re;
  regex_t version_re;
};

static char* MC_get_lib_name(const char* pathname, struct s_mc_memory_map_re* res) {
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

/** @brief Finds the range of the different memory segments and binary paths */
static void MC_process_init_memory_map_info(mc_process_t process)
{
  XBT_DEBUG("Get debug information ...");
  process->maestro_stack_start = NULL;
  process->maestro_stack_end = NULL;
  process->object_infos = NULL;
  process->object_infos_size = 0;
  process->binary_info = NULL;
  process->libsimgrid_info = NULL;

  struct s_mc_memory_map_re res;

  if(regcomp(&res.so_re, SO_RE, 0) || regcomp(&res.version_re, VERSION_RE, 0))
    xbt_die(".so regexp did not compile");

  memory_map_t maps = process->memory_map;

  const char* current_name = NULL;

  size_t i = 0;
  for (i=0; i < maps->mapsize; i++) {
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
        process->maestro_stack_start = reg->start_addr;
        process->maestro_stack_end = reg->end_addr;
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
      MC_find_object_info(process->memory_map, pathname, is_executable);
    process->object_infos = (mc_object_info_t*) realloc(process->object_infos,
      (process->object_infos_size+1) * sizeof(mc_object_info_t*));
    process->object_infos[process->object_infos_size] = info;
    process->object_infos_size++;
    if (is_executable)
      process->binary_info = info;
    else if (libname && MC_is_simgrid_lib(libname))
      process->libsimgrid_info = info;
    free(libname);
  }

  regfree(&res.so_re);
  regfree(&res.version_re);

  // Resolve time (including accress differents objects):
  for (i=0; i!=process->object_infos_size; ++i)
    MC_post_process_object_info(process, process->object_infos[i]);

  xbt_assert(process->maestro_stack_start, "Did not find maestro_stack_start");
  xbt_assert(process->maestro_stack_end, "Did not find maestro_stack_end");

  XBT_DEBUG("Get debug information done !");
}

mc_object_info_t MC_process_find_object_info(mc_process_t process, const void *addr)
{
  size_t i;
  for (i = 0; i != process->object_infos_size; ++i) {
    if (addr >= (void *) process->object_infos[i]->start
        && addr <= (void *) process->object_infos[i]->end) {
      return process->object_infos[i];
    }
  }
  return NULL;
}

mc_object_info_t MC_process_find_object_info_exec(mc_process_t process, const void *addr)
{
  size_t i;
  for (i = 0; i != process->object_infos_size; ++i) {
    if (addr >= (void *) process->object_infos[i]->start_exec
        && addr <= (void *) process->object_infos[i]->end_exec) {
      return process->object_infos[i];
    }
  }
  return NULL;
}

mc_object_info_t MC_process_find_object_info_rw(mc_process_t process, const void *addr)
{
  size_t i;
  for (i = 0; i != process->object_infos_size; ++i) {
    if (addr >= (void *) process->object_infos[i]->start_rw
        && addr <= (void *) process->object_infos[i]->end_rw) {
      return process->object_infos[i];
    }
  }
  return NULL;
}

// Functions, variables…

dw_frame_t MC_process_find_function(mc_process_t process, const void *ip)
{
  mc_object_info_t info = MC_process_find_object_info_exec(process, ip);
  if (info == NULL)
    return NULL;
  else
    return MC_file_object_info_find_function(info, ip);
}

dw_variable_t MC_process_find_variable_by_name(mc_process_t process, const char* name)
{
  const size_t n = process->object_infos_size;
  size_t i;

  // First lookup the variable in the executable shared object.
  // A global variable used directly by the executable code from a library
  // is reinstanciated in the executable memory .data/.bss.
  // We need to look up the variable in the execvutable first.
  if (process->binary_info) {
    mc_object_info_t info = process->binary_info;
    dw_variable_t var = MC_file_object_info_find_variable_by_name(info, name);
    if (var)
      return var;
  }

  for (i=0; i!=n; ++i) {
    mc_object_info_t info =process->object_infos[i];
    dw_variable_t var = MC_file_object_info_find_variable_by_name(info, name);
    if (var)
      return var;
  }

  return NULL;
}

void MC_process_read_variable(mc_process_t process, const char* name, void* target, size_t size)
{
  dw_variable_t var = MC_process_find_variable_by_name(process, name);
  if (!var->address)
    xbt_die("No simple location for this variable");
  MC_process_read(process, MC_PROCESS_NO_FLAG, target, var->address, size,
    MC_PROCESS_INDEX_ANY);
}

// ***** Memory access

int MC_process_vm_open(pid_t pid, int flags)
{
  const size_t buffer_size = 30;
  char buffer[buffer_size];
  int res = snprintf(buffer, buffer_size, "/proc/%lli/mem", (long long) pid);
  if (res < 0 || res >= buffer_size) {
    errno = ENAMETOOLONG;
    return -1;
  }
  return open(buffer, flags);
}

static void MC_process_open_memory_file(mc_process_t process)
{
  if (MC_process_is_self(process) || process->memory_file >= 0)
    return;

  int fd = MC_process_vm_open(process->pid, O_RDWR);
  if (fd<0)
    xbt_die("Could not open file for process virtual address space");
  process->memory_file = fd;
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

const void* MC_process_read(mc_process_t process, e_adress_space_read_flags_t flags,
  void* local, const void* remote, size_t len,
  int process_index)
{
  if (process_index != MC_PROCESS_INDEX_DISABLED) {
    mc_object_info_t info = MC_process_find_object_info_rw(process, remote);
    // Segment overlap is not handled.
    if (MC_object_info_is_privatized(info)) {
      if (process_index < 0)
        xbt_die("Missing process index");
      // Address translation in the privaization segment:
      size_t offset = (const char*) remote - info->start_rw;
      remote = (const char*) remote - offset;
    }
  }

  if (MC_process_is_self(process)) {
    if (flags & MC_ADDRESS_SPACE_READ_FLAGS_LAZY)
      return remote;
    else {
      memcpy(local, remote, len);
      return local;
    }
  } else {
    if (pread_whole(process->memory_file, local, len, (off_t) remote) < 0)
      xbt_die("Read from process %lli failed", (long long) process->pid);
    return local;
  }
}

void MC_process_write(mc_process_t process, const void* local, void* remote, size_t len)
{
  if (MC_process_is_self(process)) {
    memcpy(remote, local, len);
  } else {
    if (pwrite_whole(process->memory_file, local, len, (off_t) remote) < 0)
      xbt_die("Write to process %lli failed", (long long) process->pid);
  }
}

static pthread_once_t zero_buffer_flag = PTHREAD_ONCE_INIT;
static const void* zero_buffer;
static const int zero_buffer_size = 10 * 4096;

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

void MC_process_clear_memory(mc_process_t process, void* remote, size_t len)
{
  if (MC_process_is_self(process)) {
    memset(remote, 0, len);
  } else {
    pthread_once(&zero_buffer_flag, MC_zero_buffer_init);
    while (len) {
      size_t s = len > zero_buffer_size ? zero_buffer_size : len;
      MC_process_write(process, zero_buffer, remote, s);
      remote = (char*) remote + s;
      len -= s;
    }
  }
}

/** Get the issuer of a simcall (`req->issuer`)
 *
 *  In split-process mode, it does the black magic necessary to get an address
 *  of a (shallow) copy of the data structure the issuer SIMIX process in the local
 *  address space.
 *
 *  @param process the MCed process
 *  @param req     the simcall (copied in the local process)
 */
smx_process_t MC_process_get_issuer(mc_process_t process, smx_simcall_t req)
{
  if (MC_process_is_self(&mc_model_checker->process))
    return req->issuer;

  MC_process_refresh_simix_processes(process);

  // This is the address of the smx_process in the MCed process:
  void* address = req->issuer;

  unsigned i;
  mc_smx_process_info_t p;

  xbt_dynar_foreach_ptr(process->smx_process_infos, i, p)
    if (p->address == address)
      return &p->copy;
  xbt_dynar_foreach_ptr(process->smx_old_process_infos, i, p)
    if (p->address == address)
      return &p->copy;

  xbt_die("Issuer not found");
}

void MC_simcall_handle(smx_simcall_t req, int value)
{
  if (MC_process_is_self(&mc_model_checker->process)) {
    SIMIX_simcall_handle(req, value);
    return;
  }

  MC_process_refresh_simix_processes(&mc_model_checker->process);

  unsigned i;
  mc_smx_process_info_t pi = NULL;

  xbt_dynar_foreach_ptr(mc_model_checker->process.smx_process_infos, i, pi) {
    smx_process_t p = (smx_process_t) pi->address;
    if (req == &pi->copy.simcall) {
      smx_simcall_t real_req = &p->simcall;
      // TODO, use a remote call
      SIMIX_simcall_handle(real_req, value);
      return;
    }
  }

  // Check (remove afterwards):
  xbt_dynar_foreach_ptr(mc_model_checker->process.smx_process_infos, i, pi) {
    smx_process_t p = (smx_process_t) pi->address;
    if (req == &p->simcall)
      xbt_die("The real simcall was passed. We expected the local copy.");
  }

  xbt_die("Could not find the request");
}

void mc_smx_process_info_clear(mc_smx_process_info_t p)
{
  // Nothing yet
}
