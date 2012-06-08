/* xbt/replay_reader.h -- Tools to parse a replay file                */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_REPLAY_H
#define XBT_REPLAY_H
#include "xbt/misc.h"           /* SG_BEGIN_DECL */

SG_BEGIN_DECL()

typedef struct s_replay_reader *xbt_replay_reader_t;
typedef void (*action_fun) (const char *const *args);

static xbt_dict_t action_funs;
static xbt_dict_t action_queues;

/* To split the file if a unique one is given (specific variable for the other case live in runner()) */
FILE *action_fp;


xbt_replay_reader_t xbt_replay_reader_new(const char*filename);
const char **xbt_replay_reader_get(xbt_replay_reader_t reader);
void xbt_replay_reader_free(xbt_replay_reader_t *reader);
const char *xbt_replay_reader_position(xbt_replay_reader_t reader);

int xbt_replay_action_runner(int argc, char *argv[]);

XBT_PUBLIC(void) xbt_replay_action_register(const char *action_name,
                                     			  action_fun function);
XBT_PUBLIC(void) xbt_replay_action_unregister(const char *action_name);

SG_END_DECL()

#endif /* XBT_REPLAY_H */
