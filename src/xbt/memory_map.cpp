/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/types.h>
#ifdef __linux__
# include <sys/mman.h>
#endif

#include <xbt/sysdep.h>
#include <xbt/base.h>
#include <xbt/file.h>
#include <xbt/log.h>

#include "memory_map.hpp"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_memory_map, xbt, "Logging specific to algorithms for memory_map");
}

namespace simgrid {
namespace xbt {

XBT_PRIVATE std::vector<VmMap> get_memory_map(pid_t pid)
{
#ifdef __linux__
  /* Open the actual process's proc maps file and create the memory_map_t */
  /* to be returned. */
  char* path = bprintf("/proc/%i/maps", (int) pid);
  FILE *fp = std::fopen(path, "r");
  if(fp == NULL)
    std::perror("fopen failed");
  xbt_assert(fp, "Cannot open %s to investigate the memory map of the process.", path);
  free(path);
  setbuf(fp, NULL);

  std::vector<VmMap> ret;

  /* Read one line at the time, parse it and add it to the memory map to be returned */
  ssize_t read; /* Number of bytes readed */
  char* line = NULL;
  std::size_t n = 0; /* Amount of bytes to read by xbt_getline */
  while ((read = xbt_getline(&line, &n, fp)) != -1) {

    //fprintf(stderr,"%s", line);

    /* Wipeout the new line character */
    line[read - 1] = '\0';

    /* Tokenize the line using spaces as delimiters and store each token in lfields array. We expect 5 tokens/fields */
    char* lfields[6];
    lfields[0] = strtok(line, " ");

    int i;
    for (i = 1; i < 6 && lfields[i - 1] != NULL; i++) {
      lfields[i] = std::strtok(NULL, " ");
    }

    /* Check to see if we got the expected amount of columns */
    if (i < 6)
      xbt_abort();

    /* Ok we are good enough to try to get the info we need */
    /* First get the start and the end address of the map   */
    char *tok = std::strtok(lfields[0], "-");
    if (tok == NULL)
      xbt_abort();

    VmMap memreg;
    char *endptr;
    memreg.start_addr = std::strtoull(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    tok = std::strtok(NULL, "-");
    if (tok == NULL)
      xbt_abort();

    memreg.end_addr = std::strtoull(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    /* Get the permissions flags */
    if (std::strlen(lfields[1]) < 4)
      xbt_abort();

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

    if (lfields[1][4] == 'p')
      memreg.flags |= MAP_PRIVATE;
    else if (lfields[1][4] == 's')
      memreg.flags |= MAP_SHARED;

    /* Get the offset value */
    memreg.offset = std::strtoull(lfields[2], &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    /* Get the device major:minor bytes */
    tok = std::strtok(lfields[3], ":");
    if (tok == NULL)
      xbt_abort();

    memreg.dev_major = (char) strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    tok = std::strtok(NULL, ":");
    if (tok == NULL)
      xbt_abort();

    memreg.dev_minor = (char) std::strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    /* Get the inode number and make sure that the entire string was a long int */
    memreg.inode = strtoul(lfields[4], &endptr, 10);
    if (*endptr != '\0')
      xbt_abort();

    /* And finally get the pathname */
    if (lfields[5])
      memreg.pathname = lfields[5];

    /* Create space for a new map region in the region's array and copy the */
    /* parsed stuff from the temporal memreg variable */
    XBT_DEBUG("Found region for %s", !memreg.pathname.empty() ? memreg.pathname.c_str() : "(null)");

    ret.push_back(std::move(memreg));
  }

  std::free(line);
  std::fclose(fp);
  return ret;
#else
  /* On FreeBSD, kinfo_getvmmap() could be used but mmap() support is disabled anyway. */
  xbt_die("Could not get memory map from process %lli", (long long int) pid);
#endif
}

}
}
