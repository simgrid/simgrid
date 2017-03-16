/* xbt/replay_reader.h -- Tools to parse a replay file                */

/* Copyright (c) 2010, 2012-2015, 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_REPLAY_HPP
#define XBT_REPLAY_HPP

#include "xbt/dict.h"
#ifdef __cplusplus
#include <fstream>

namespace simgrid {
namespace xbt {
/* To split the file if a unique one is given (specific variable for the other case live in runner()) */
XBT_PUBLIC_DATA(std::ifstream*) action_fs;
XBT_PUBLIC(bool) replay_is_active();
XBT_PUBLIC(void) replay_init();
XBT_PUBLIC(void) replay_exit();
XBT_PUBLIC(int) replay_runner(int argc, char* argv[]);
}
}
#endif

SG_BEGIN_DECL()

typedef void (*action_fun)(const char* const* args);
XBT_PUBLIC_DATA(xbt_dict_t) xbt_action_queues;
XBT_PUBLIC(void) xbt_replay_action_register(const char* action_name, action_fun function);

SG_END_DECL()

#endif
