/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/sg_config.hpp"
#include "src/internal_config.h"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"

#if HAVE_SMPI
#include "smpi/smpi.h"
#endif

#include <boost/tokenizer.hpp>
#include <cstring>
#include <memory>
#include <unistd.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(mc);

static simgrid::config::Flag<std::string> _sg_mc_setenv{
    "model-check/setenv", "Extra environment variables to pass to the child process (ex: 'AZE=aze;QWE=qwe').", "",
    [](std::string_view value) {
      xbt_assert(value.empty() || value.find('=', 0) != std::string_view::npos,
                 "The 'model-check/setenv' parameter must be like 'AZE=aze', but it does not contain an equal sign.");
    }};

static void mc_exec_user_code(std::vector<char*> const& argv)
{
  int i = 1;
  while (argv[i] != nullptr && argv[i][0] == '-')
    i++;

  /* Setup the tokenizer that parses the cfg:model-check/setenv parameter */
  using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
  boost::char_separator<char> semicol_sep(";");
  boost::char_separator<char> equal_sep("=");
  Tokenizer token_vars(_sg_mc_setenv.get(), semicol_sep); /* Iterate over all FOO=foo parts */
  for (const auto& token : token_vars) {
    std::vector<std::string> kv;
    Tokenizer token_kv(token, equal_sep);
    for (const auto& t : token_kv) /* Iterate over 'FOO' and then 'foo' in that 'FOO=foo' */
      kv.push_back(t);
    xbt_assert(kv.size() == 2, "Parse error on 'model-check/setenv' value %s. Does it contain an equal sign?",
               token.c_str());
    XBT_INFO("setenv '%s'='%s'", kv[0].c_str(), kv[1].c_str());
    setenv(kv[0].c_str(), kv[1].c_str(), 1);
  }
  xbt_assert(argv[i] != nullptr,
             "Unable to find a binary to exec on the command line. Did you only pass config flags?");

  execvp(argv[i], argv.data() + i);
  xbt_die("The model-checked process failed to exec(%s): %s", argv[i], strerror(errno));
}

int main(int argc, char** argv)
{
  xbt_assert(argc >= 2, "Missing arguments");

  // Currently, we need this before sg_config_init:
  _sg_do_model_check = 1;

  // The initialization function can touch argv.
  // We make a copy of argv before modifying it in order to pass the original value to the model-checked application:
  std::vector<char*> argv_copy{argv, argv + argc + 1};

  xbt_log_init(&argc, argv);
#if HAVE_SMPI
  smpi_init_options(); // that's OK to call it twice, and we need it ASAP
#endif
  sg_config_init(&argc, argv);

  auto remote_app = std::make_unique<simgrid::mc::RemoteApp>([argv_copy] { mc_exec_user_code(argv_copy); });

  simgrid::mc::Exploration* explo;
  if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
    explo = simgrid::mc::create_communication_determinism_checker(*remote_app.get());
  else if (_sg_mc_unfolding_checker)
    explo = simgrid::mc::create_udpor_checker(*remote_app.get());
  else if (_sg_mc_property_file.get().empty())
    explo = simgrid::mc::create_dfs_exploration(*remote_app.get());
  else
    explo = simgrid::mc::create_liveness_checker(*remote_app.get());

  mc_model_checker->set_exploration(explo);
  std::unique_ptr<simgrid::mc::Exploration> checker{explo};

  try {
    checker->run();
  } catch (const simgrid::mc::DeadlockError&) {
    return SIMGRID_MC_EXIT_DEADLOCK;
  } catch (const simgrid::mc::TerminationError&) {
    return SIMGRID_MC_EXIT_NON_TERMINATION;
  } catch (const simgrid::mc::LivenessError&) {
    return SIMGRID_MC_EXIT_LIVENESS;
  }
  return SIMGRID_MC_EXIT_SUCCESS;
}
