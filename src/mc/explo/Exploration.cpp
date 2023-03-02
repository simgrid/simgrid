/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_explo, mc, "Generic exploration algorithm of the model-checker");

namespace simgrid::mc {

static simgrid::config::Flag<std::string> cfg_dot_output_file{
    "model-check/dot-output", "Name of dot output file corresponding to graph state", ""};

Exploration::Exploration(const std::vector<char*>& args) : remote_app_(std::make_unique<RemoteApp>(args))
{
  mc_model_checker->set_exploration(this);

  if (not cfg_dot_output_file.get().empty()) {
    dot_output_ = fopen(cfg_dot_output_file.get().c_str(), "w");
    xbt_assert(dot_output_ != nullptr, "Error open dot output file: %s", strerror(errno));

    fprintf(dot_output_, "digraph graphname{\n fixedsize=true; rankdir=TB; ranksep=.25; edge [fontsize=12]; node "
                         "[fontsize=10, shape=circle,width=.5 ]; graph [resolution=20, fontsize=10];\n");
  }
}

Exploration::~Exploration()
{
  if (dot_output_ != nullptr)
    fclose(dot_output_);
}

void Exploration::dot_output(const char* fmt, ...)
{
  if (dot_output_ != nullptr) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(dot_output_, fmt, ap);
    va_end(ap);
    fflush(dot_output_);
  }
}

void Exploration::log_state()
{
  if (not cfg_dot_output_file.get().empty()) {
    dot_output("}\n");
    fclose(dot_output_);
  }
  if (getenv("SIMGRID_MC_SYSTEM_STATISTICS")) {
    int ret = system("free");
    if (ret != 0)
      XBT_WARN("Call to system(free) did not return 0, but %d", ret);
  }
}

}; // namespace simgrid::mc
