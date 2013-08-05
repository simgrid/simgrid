/* Copyright (c) 2008-2013 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_memory_map, mc,
                                "Logging specific to algorithms for memory_map");

memory_map_t MC_get_memory_map(void)
{
  FILE *fp;                     /* File pointer to process's proc maps file */
  char *line = NULL;            /* Temporal storage for each line that is readed */
  ssize_t read;                 /* Number of bytes readed */
  size_t n = 0;                 /* Amount of bytes to read by xbt_getline */
  memory_map_t ret = NULL;      /* The memory map to return */

/* The following variables are used during the parsing of the file "maps" */
  s_map_region_t memreg;          /* temporal map region used for creating the map */
  char *lfields[6], *tok, *endptr;
  int i;

/* Open the actual process's proc maps file and create the memory_map_t */
/* to be returned. */
  fp = fopen("/proc/self/maps", "r");

  if(fp == NULL)
    perror("fopen failed");

  xbt_assert(fp,
              "Cannot open /proc/self/maps to investigate the memory map of the process. Please report this bug.");

  setbuf(fp, NULL);

  ret = xbt_new0(s_memory_map_t, 1);

  /* Read one line at the time, parse it and add it to the memory map to be returned */
  while ((read = xbt_getline(&line, &n, fp)) != -1) {

    //fprintf(stderr,"%s", line);

    /* Wipeout the new line character */
    line[read - 1] = '\0';

    /* Tokenize the line using spaces as delimiters and store each token */
    /* in lfields array. We expect 5 tokens/fields */
    lfields[0] = strtok(line, " ");

    for (i = 1; i < 6 && lfields[i - 1] != NULL; i++) {
      lfields[i] = strtok(NULL, " ");
    }

    /* Check to see if we got the expected amount of columns */
    if (i < 6)
      xbt_abort();

    /* Ok we are good enough to try to get the info we need */
    /* First get the start and the end address of the map   */
    tok = strtok(lfields[0], "-");
    if (tok == NULL)
      xbt_abort();

    memreg.start_addr = (void *) strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    tok = strtok(NULL, "-");
    if (tok == NULL)
      xbt_abort();

    memreg.end_addr = (void *) strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    /* Get the permissions flags */
    if (strlen(lfields[1]) < 4)
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
    memreg.offset = (void *) strtoul(lfields[2], &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    /* Get the device major:minor bytes */
    tok = strtok(lfields[3], ":");
    if (tok == NULL)
      xbt_abort();

    memreg.dev_major = (char) strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    tok = strtok(NULL, ":");
    if (tok == NULL)
      xbt_abort();

    memreg.dev_minor = (char) strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if (*endptr != '\0')
      xbt_abort();

    /* Get the inode number and make sure that the entire string was a long int */
    memreg.inode = strtoul(lfields[4], &endptr, 10);
    if (*endptr != '\0')
      xbt_abort();

    /* And finally get the pathname */
    memreg.pathname = xbt_strdup(lfields[5]);

    /* Create space for a new map region in the region's array and copy the */
    /* parsed stuff from the temporal memreg variable */
    ret->regions =
        xbt_realloc(ret->regions, sizeof(memreg) * (ret->mapsize + 1));
    memcpy(ret->regions + ret->mapsize, &memreg, sizeof(memreg));
    ret->mapsize++;

  }

  free(line);

  fclose(fp);

  return ret;
}

void MC_free_memory_map(memory_map_t map){

  int i;
  for(i=0; i< map->mapsize; i++){
    xbt_free(map->regions[i].pathname);
  }
  xbt_free(map->regions);
  xbt_free(map);
}
