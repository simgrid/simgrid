#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <sys/mman.h> // PROT_*

#include <libgen.h>

#include "mc_process.h"
#include "mc_object_info.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_process, mc,
                                "MC process information");

static void MC_process_init_memory_map_info(mc_process_t process);
static void MC_process_open_memory_file(mc_process_t process);


void MC_process_init(mc_process_t process, pid_t pid)
{
  process->process_flags = MC_PROCESS_NO_FLAG;
  process->pid = pid;
  if (pid==getpid())
    process->process_flags |= MC_PROCESS_SELF_FLAG;
  process->memory_map = MC_get_memory_map(pid);
  process->memory_file = -1;
  MC_process_init_memory_map_info(process);
  MC_process_open_memory_file(process);
}

void MC_process_clear(mc_process_t process)
{
  process->process_flags = MC_PROCESS_NO_FLAG;
  process->pid = 0;

  MC_free_memory_map(process->memory_map);
  process->memory_map = NULL;

  process->maestro_stack_start = NULL;
  process->maestro_stack_end = NULL;

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
  XBT_INFO("Get debug information ...");
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

  xbt_assert(process->maestro_stack_start, "maestro_stack_start");
  xbt_assert(process->maestro_stack_end, "maestro_stack_end");

  XBT_INFO("Get debug information done !");
}

mc_object_info_t MC_process_find_object_info(mc_process_t process, void *ip)
{
  size_t i;
  for (i = 0; i != process->object_infos_size; ++i) {
    if (ip >= (void *) process->object_infos[i]->start_exec
        && ip <= (void *) process->object_infos[i]->end_exec) {
      return process->object_infos[i];
    }
  }
  return NULL;
}

dw_frame_t MC_process_find_function(mc_process_t process, void *ip)
{
  mc_object_info_t info = MC_process_find_object_info(process, ip);
  if (info == NULL)
    return NULL;
  else
    return MC_file_object_info_find_function(info, ip);
}

// ***** Memory access

static void MC_process_open_memory_file(mc_process_t process)
{
  if (MC_process_is_self(process) || process->memory_file >= 0)
    return;

  const size_t buffer_size = 30;
  char buffer[buffer_size];
  int res = snprintf(buffer, buffer_size, "/proc/%lli/mem", (long long) process->pid);
  if (res < 0 || res>= buffer_size) {
    XBT_ERROR("Could not open memory file descriptor for process %lli",
      (long long) process->pid);
    return;
  }

  int fd = open(buffer, O_RDWR);
  if (fd<0)
    xbt_die("Could not initialise memory access for remote process");
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

void MC_process_read(mc_process_t process, void* local, const void* remote, size_t len)
{
  if (MC_process_is_self(process)) {
    memcpy(local, remote, len);
  } else {
    if (pread_whole(process->memory_file, local, len, (off_t) remote) < 0)
      xbt_die("Read from process %lli failed", (long long) process->pid);
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
