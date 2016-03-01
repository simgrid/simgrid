/* xbt/replay_reader.h -- Tools to parse a replay file                */

/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_REPLAY_H
#define XBT_REPLAY_H
#include "xbt/misc.h"           /* SG_BEGIN_DECL */

SG_BEGIN_DECL()

typedef struct s_replay_reader *xbt_replay_reader_t;
typedef void (*action_fun) (const char *const *args);

XBT_PUBLIC_DATA(xbt_dict_t) xbt_action_funs;
XBT_PUBLIC_DATA(xbt_dict_t) xbt_action_queues;

/* To split the file if a unique one is given (specific variable for the other case live in runner()) */
XBT_PUBLIC_DATA(FILE *) xbt_action_fp;

XBT_PUBLIC(xbt_replay_reader_t) xbt_replay_reader_new(const char*filename);
XBT_PUBLIC(const char **) xbt_replay_reader_get(xbt_replay_reader_t reader);
XBT_PUBLIC(void) xbt_replay_reader_free(xbt_replay_reader_t *reader);

XBT_PUBLIC(void) xbt_replay_action_register(const char *action_name, action_fun function);
XBT_PUBLIC(int) xbt_replay_action_runner(int argc, char *argv[]);

XBT_PUBLIC(int) _xbt_replay_is_active(void);
XBT_PUBLIC(int) _xbt_replay_action_init(void);
XBT_PUBLIC(void) _xbt_replay_action_exit(void);

SG_END_DECL()

#endif /* XBT_REPLAY_H */
