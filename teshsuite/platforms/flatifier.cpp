/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/xbt_os_time.h>

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

XBT_LOG_NEW_DEFAULT_CATEGORY(flatifier, "Logging specific to this platform parsing tool");

namespace sg4 = simgrid::s4u;

static bool parse_cmdline(bool* timings, char** platformFile, int argc, char** argv)
{
  bool parse_ok = true;
  for (int i = 1; i < argc; i++) {
    if (std::strlen(argv[i]) > 1 && argv[i][0] == '-' && argv[i][1] == '-') {
      if (not std::strcmp(argv[i], "--timings")) {
        *timings = true;
      } else {
        parse_ok = false;
        break;
      }
    } else {
      *platformFile = argv[i];
    }
  }
  return parse_ok && platformFile != nullptr;
}

int main(int argc, char** argv)
{
  char* platformFile = nullptr;
  bool timings       = false;

  xbt_os_timer_t parse_time = xbt_os_timer_new();

  sg4::Engine e(&argc, argv);

  xbt_assert(parse_cmdline(&timings, &platformFile, argc, argv),
             "Invalid command line arguments: expected [--timings] platformFile");

  xbt_os_cputimer_start(parse_time);
  e.load_platform(platformFile);
  e.seal_platform();
  xbt_os_cputimer_stop(parse_time);

  if (timings) {
    XBT_INFO("Parsing time: %fs (%zu hosts, %zu links)", xbt_os_timer_elapsed(parse_time), e.get_host_count(),
             e.get_link_count());
  } else {
    std::printf("%s", e.flatify_platform().c_str());
  }

  xbt_os_timer_free(parse_time);

  return 0;
}
