/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_explo, mc, "Generic exploration algorithm of the model-checker");

namespace simgrid::mc {

Exploration::Exploration(const std::vector<char*>& args) : remote_app_(std::make_unique<RemoteApp>(args))
{
  mc_model_checker->set_exploration(this);
}

void Exploration::log_state()
{
  if (not _sg_mc_dot_output_file.get().empty()) {
    mc_model_checker->dot_output("}\n");
    mc_model_checker->dot_output_close();
  }
  if (getenv("SIMGRID_MC_SYSTEM_STATISTICS")) {
    int ret = system("free");
    if (ret != 0)
      XBT_WARN("Call to system(free) did not return 0, but %d", ret);
  }
}

}; // namespace simgrid::mc
