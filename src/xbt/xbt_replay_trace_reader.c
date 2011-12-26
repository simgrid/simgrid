/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "simgrid_config.h" //For getline, keep that include first

#include "gras_config.h"
#include <errno.h>
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/replay_trace_reader.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(replay,xbt,"Replay trace reader");

typedef struct s_replay_trace_reader {
  FILE *fp;
  char *line;
  size_t line_len;
  char *position; /* stable storage */
  char *filename; int linenum;
} s_xbt_replay_trace_reader_t;

xbt_replay_trace_reader_t xbt_replay_trace_reader_new(const char *filename)
{
  xbt_replay_trace_reader_t res = xbt_new0(s_xbt_replay_trace_reader_t,1);
  res->fp = fopen(filename, "r");
  xbt_assert(res->fp != NULL, "Cannot open %s: %s", filename,
      strerror(errno));
  res->filename = xbt_strdup(filename);
  return res;
}

const char *xbt_replay_trace_reader_position(xbt_replay_trace_reader_t reader)
{
  free(reader->position);
  reader->position = bprintf("%s:%d",reader->filename,reader->linenum);
  return reader->position;
}

const char **xbt_replay_trace_reader_get(xbt_replay_trace_reader_t reader)
{
  ssize_t read;
  xbt_dynar_t d;
  read = getline(&reader->line, &reader->line_len, reader->fp);
  //XBT_INFO("got from trace: %s",reader->line);
  reader->linenum++;
  if (read==-1)
    return NULL; /* end of file */
  char *comment = strchr(reader->line, '#');
  if (comment != NULL)
    *comment = '\0';
  xbt_str_trim(reader->line, NULL);
  if (reader->line[0] == '\0')
    return xbt_replay_trace_reader_get(reader); /* Get next line */

  d=xbt_str_split_quoted_in_place(reader->line);
  if (xbt_dynar_is_empty(d)) {
    xbt_dynar_free(&d);
    return xbt_replay_trace_reader_get(reader); /* Get next line */
  }
  return xbt_dynar_to_array(d);
}

void xbt_replay_trace_reader_free(xbt_replay_trace_reader_t *reader)
{
  free((*reader)->filename);
  free((*reader)->position);
  fclose((*reader)->fp);
  free((*reader)->line);
  free(*reader);
  *reader=NULL;
}
