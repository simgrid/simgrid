/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <sys/types.h>

#if defined __APPLE__
# include <dlfcn.h>
# include <mach/mach_init.h>
# include <mach/mach_traps.h>
# include <mach/mach_port.h>
# include <mach/mach_vm.h>
# include <sys/mman.h>
# include <sys/param.h>
# if __MAC_OS_X_VERSION_MIN_REQUIRED < 1050
#  define mach_vm_address_t vm_address_t
#  define mach_vm_size_t vm_size_t
#  if defined __ppc64__ || defined __x86_64__
#    define mach_vm_region vm_region64
#  else
#    define mach_vm_region vm_region
#  endif
# endif
#endif

#if defined __linux__
# include <sys/mman.h>
#endif

#if defined __FreeBSD__
# include <sys/types.h>
# include <sys/mman.h>
# include <sys/param.h>
# include <sys/queue.h>
# include <sys/socket.h>
# include <sys/sysctl.h>
# include <sys/user.h>
# include <libprocstat.h>
#endif

#include <cinttypes>

#include "memory_map.hpp"

// abort with a message if `expr' is false
#define CHECK(expr)                                                                                                    \
  if (not(expr)) {                                                                                                     \
    fprintf(stderr, "CHECK FAILED: %s:%d: %s\n", __FILE__, __LINE__, #expr);                                           \
    abort();                                                                                                           \
  } else                                                                                                               \
    ((void)0)

namespace simgrid {
namespace xbt {

/**
 * \todo This function contains many cases that do not allow for a
 *       recovery. Currently, abort() is called but we should
 *       much rather die with the specific reason so that it's easier
 *       to find out what's going on.
 */
std::vector<VmMap> get_memory_map(pid_t pid)
{
  std::vector<VmMap> ret;
#if defined __APPLE__
  vm_map_t map;

  /* Request authorization to read mappings */
  if (task_for_pid(mach_task_self(), pid, &map) != KERN_SUCCESS) {
    std::perror("task_for_pid failed");
    std::fprintf(stderr, "Cannot request authorization for kernel information access\n");
    abort();
  }

  /*
   * Darwin do not give us the number of mappings, so we read entries until
   * we get a KERN_INVALID_ADDRESS return.
   */
  mach_vm_address_t address = VM_MIN_ADDRESS;
  while (true) {
    kern_return_t kr;
    memory_object_name_t object;
    mach_vm_size_t size;
#if defined __ppc64__ || defined __x86_64__
    vm_region_flavor_t flavor = VM_REGION_BASIC_INFO_64;
    struct vm_region_basic_info_64 info;
    mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
#else
    vm_region_flavor_t flavor = VM_REGION_BASIC_INFO;
    struct vm_region_basic_info info;
    mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT;
#endif

    kr =
      mach_vm_region(
          map,
          &address,
          &size,
          flavor,
          (vm_region_info_t)&info,
          &info_count,
          &object);
    if (kr == KERN_INVALID_ADDRESS) {
      break;
    }
    else if (kr != KERN_SUCCESS) {
      std::perror("mach_vm_region failed");
      std::fprintf(stderr, "Cannot request authorization for kernel information access\n");
      abort();
    }

    VmMap memreg;

    /* Addresses */
    memreg.start_addr = address;
    memreg.end_addr = address + size;

    /* Permissions */
    memreg.prot = PROT_NONE;
    if (info.protection & VM_PROT_READ)
      memreg.prot |= PROT_READ;
    if (info.protection & VM_PROT_WRITE)
      memreg.prot |= PROT_WRITE;
    if (info.protection & VM_PROT_EXECUTE)
      memreg.prot |= PROT_EXEC;

    /* Private (copy-on-write) or shared? */
    memreg.flags = 0;
    if (info.shared)
      memreg.flags |= MAP_SHARED;
    else
      memreg.flags |= MAP_PRIVATE;

    /* Offset */
    memreg.offset = info.offset;

    /* Device : not sure this can be mapped to something outside of Linux? */
    memreg.dev_major = 0;
    memreg.dev_minor = 0;

    /* Inode */
    memreg.inode = 0;

    /* Path */
    Dl_info dlinfo;
    if (dladdr(reinterpret_cast<void*>(address), &dlinfo))
      memreg.pathname = dlinfo.dli_fname;

#if 0 /* debug */
    std::fprintf(stderr, "Region: %016" PRIx64 "-%016" PRIx64 " | %c%c%c | %s\n", memreg.start_addr, memreg.end_addr,
              (memreg.prot & PROT_READ) ? 'r' : '-', (memreg.prot & PROT_WRITE) ? 'w' : '-',
              (memreg.prot & PROT_EXEC) ? 'x' : '-', memreg.pathname.c_str());
#endif

    ret.push_back(std::move(memreg));
    address += size;
  }

  mach_port_deallocate(mach_task_self(), map);
#elif defined __linux__
  /* Open the actual process's proc maps file and create the memory_map_t */
  /* to be returned. */
  std::string path = std::string("/proc/") + std::to_string(pid) + "/maps";
  std::ifstream fp;
  fp.rdbuf()->pubsetbuf(nullptr, 0);
  fp.open(path);
  if (not fp) {
    std::perror("open failed");
    std::fprintf(stderr, "Cannot open %s to investigate the memory map of the process.\n", path.c_str());
    abort();
  }

  /* Read one line at the time, parse it and add it to the memory map to be returned */
  std::string sline;
  while (std::getline(fp, sline)) {
    /**
     * The lines that we read have this format: (This is just an example)
     * 00602000-00603000 rw-p 00002000 00:28 1837264                            <complete-path-to-file>
     */
    char* line = &sline[0];

    /* Tokenize the line using spaces as delimiters and store each token in lfields array. We expect 5 tokens for 6 fields */
    char* saveptr = nullptr; // for strtok_r()
    std::array<char*, 6> lfields;
    lfields[0] = strtok_r(line, " ", &saveptr);

    int i;
    for (i = 1; i < 6 && lfields[i - 1] != nullptr; i++) {
      lfields[i] = strtok_r(nullptr, " ", &saveptr);
    }

    /* Check to see if we got the expected amount of columns */
    if (i < 6) {
      std::fprintf(stderr, "The memory map apparently only supplied less than 6 columns. Recovery impossible.\n");
      abort();
    }

    /* Ok we are good enough to try to get the info we need */
    /* First get the start and the end address of the map   */
    const char* tok = strtok_r(lfields[0], "-", &saveptr);
    if (tok == nullptr) {
      std::fprintf(stderr,
                   "Start and end address of the map are not concatenated by a hyphen (-). Recovery impossible.\n");
      abort();
    }

    VmMap memreg;
    char *endptr;
    memreg.start_addr = std::strtoull(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    CHECK(*endptr == '\0');

    tok = strtok_r(nullptr, "-", &saveptr);
    CHECK(tok != nullptr);

    memreg.end_addr = std::strtoull(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    CHECK(*endptr == '\0');

    /* Get the permissions flags */
    CHECK(std::strlen(lfields[1]) >= 4);

    memreg.prot = 0;
    for (i = 0; i < 3; i++){
      switch(lfields[1][i]){
        case 'r':
          memreg.prot |= PROT_READ;
          break;
        case 'w':
          memreg.prot |= PROT_WRITE;
          break;
        case 'x':
          memreg.prot |= PROT_EXEC;
          break;
        default:
          break;
      }
    }
    if (memreg.prot == 0)
      memreg.prot |= PROT_NONE;

    memreg.flags = 0;
    if (lfields[1][3] == 'p') {
      memreg.flags |= MAP_PRIVATE;
    } else {
      memreg.flags |= MAP_SHARED;
      if (lfields[1][3] != 's')
        fprintf(stderr,
                "The protection is neither 'p' (private) nor 's' (shared) but '%s'. Let's assume shared, as on b0rken "
                "win-ubuntu systems.\nFull line: %s\n",
                lfields[1], line);
    }

    /* Get the offset value */
    memreg.offset = std::strtoull(lfields[2], &endptr, 16);
    /* Make sure that the entire string was an hex number */
    CHECK(*endptr == '\0');

    /* Get the device major:minor bytes */
    tok = strtok_r(lfields[3], ":", &saveptr);
    CHECK(tok != nullptr);

    memreg.dev_major = (char) strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    CHECK(*endptr == '\0');

    tok = strtok_r(nullptr, ":", &saveptr);
    CHECK(tok != nullptr);

    memreg.dev_minor = (char) std::strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    CHECK(*endptr == '\0');

    /* Get the inode number and make sure that the entire string was a long int */
    memreg.inode = strtoul(lfields[4], &endptr, 10);
    CHECK(*endptr == '\0');

    /* And finally get the pathname */
    if (lfields[5])
      memreg.pathname = lfields[5];

    /* Create space for a new map region in the region's array and copy the */
    /* parsed stuff from the temporal memreg variable */
    // std::fprintf(stderr, "Found region for %s\n", not memreg.pathname.empty() ? memreg.pathname.c_str() : "(null)");

    ret.push_back(std::move(memreg));
  }

  fp.close();
#elif defined __FreeBSD__
  struct procstat *prstat;
  struct kinfo_proc *proc;
  struct kinfo_vmentry *vmentries;
  unsigned int cnt;

  if ((prstat = procstat_open_sysctl()) == NULL) {
    std::perror("procstat_open_sysctl failed");
    std::fprintf(stderr, "Cannot access kernel state information\n");
    abort();
  }
  if ((proc = procstat_getprocs(prstat, KERN_PROC_PID, pid, &cnt)) == NULL) {
    std::perror("procstat_open_sysctl failed");
    std::fprintf(stderr, "Cannot access process information\n");
    abort();
  }
  if ((vmentries = procstat_getvmmap(prstat, proc, &cnt)) == NULL) {
    std::perror("procstat_getvmmap failed");
    std::fprintf(stderr, "Cannot access process memory mappings\n");
    abort();
  }
  for (unsigned int i = 0; i < cnt; i++) {
    VmMap memreg;

    /* Addresses */
    memreg.start_addr = vmentries[i].kve_start;
    memreg.end_addr = vmentries[i].kve_end;

    /* Permissions */
    memreg.prot = PROT_NONE;
    if (vmentries[i].kve_protection & KVME_PROT_READ)
      memreg.prot |= PROT_READ;
    if (vmentries[i].kve_protection & KVME_PROT_WRITE)
      memreg.prot |= PROT_WRITE;
    if (vmentries[i].kve_protection & KVME_PROT_EXEC)
      memreg.prot |= PROT_EXEC;

    /* Private (copy-on-write) or shared? */
    memreg.flags = 0;
    if (vmentries[i].kve_flags & KVME_FLAG_COW)
      memreg.flags |= MAP_PRIVATE;
    else
      memreg.flags |= MAP_SHARED;

    /* Offset */
    memreg.offset = vmentries[i].kve_offset;

    /* Device : not sure this can be mapped to something outside of Linux? */
    memreg.dev_major = 0;
    memreg.dev_minor = 0;

    /* Inode */
    memreg.inode = vmentries[i].kve_vn_fileid;

     /*
      * Path. Linuxize result by giving an anonymous mapping a path from
      * the previous mapping, provided previous is vnode and has a path,
      * and mark the stack.
      */
    if (vmentries[i].kve_path[0] != '\0')
      memreg.pathname = vmentries[i].kve_path;
    else if (vmentries[i].kve_type == KVME_TYPE_DEFAULT && vmentries[i - 1].kve_type == KVME_TYPE_VNODE &&
             vmentries[i - 1].kve_path[0] != '\0')
      memreg.pathname = vmentries[i-1].kve_path;
    else if (vmentries[i].kve_type == KVME_TYPE_DEFAULT
        && vmentries[i].kve_flags & KVME_FLAG_GROWS_DOWN)
      memreg.pathname = "[stack]";

    /*
     * One last dirty modification: remove write permission from shared
     * libraries private clean pages. This is necessary because simgrid
     * later identifies mappings based on the permissions that are expected
     * when running the Linux kernel.
     */
    if (vmentries[i].kve_type == KVME_TYPE_VNODE && not(vmentries[i].kve_flags & KVME_FLAG_NEEDS_COPY))
      memreg.prot &= ~PROT_WRITE;

    ret.push_back(std::move(memreg));
  }
  procstat_freevmmap(prstat, vmentries);
  procstat_freeprocs(prstat, proc);
  procstat_close(prstat);
#else
  std::fprintf(stderr, "Could not get memory map from process %lli\n", (long long int)pid);
  abort();
#endif
  return ret;
}

}
}
