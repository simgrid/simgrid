/* xbt/replay_trace_reader.h -- Tools to parse a replay file                */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"           /* SG_BEGIN_DECL */

SG_BEGIN_DECL()

typedef struct s_replay_trace_reader *xbt_replay_trace_reader_t;

xbt_replay_trace_reader_t xbt_replay_trace_reader_new(const char*filename);
char * const *xbt_replay_trace_reader_get(xbt_replay_trace_reader_t reader);
void xbt_replay_trace_reader_free(xbt_replay_trace_reader_t *reader);
const char *xbt_replay_trace_reader_position(xbt_replay_trace_reader_t reader);

SG_END_DECL()
