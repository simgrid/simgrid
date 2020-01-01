/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/EngineImpl.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/StorageImpl.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf.hpp" // FIXME: KILLME. There must be a better way than mimicking XML here

namespace simgrid {
namespace kernel {

EngineImpl::~EngineImpl()
{
  /* Since hosts_ is a std::map, the hosts are destroyed in the lexicographic order, which ensures that the output is
   * reproducible.
   */
  while (not hosts_.empty())
    hosts_.begin()->second->destroy();

  /* Also delete the other data */
  delete netzone_root_;
  for (auto const& kv : netpoints_)
    delete kv.second;

  for (auto const& kv : storages_)
    if (kv.second)
      kv.second->destroy();

  for (auto const& kv : links_)
    if (kv.second)
      kv.second->destroy();
}

void EngineImpl::load_deployment(const std::string& file)
{
  sg_platf_exit();
  sg_platf_init();

  surf_parse_open(file);
  surf_parse();
  surf_parse_close();
}
void EngineImpl::register_function(const std::string& name, xbt_main_func_t code)
{
  simix_global->registered_functions[name] = [code](std::vector<std::string> args) {
    return xbt::wrap_main(code, std::move(args));
  };
}

void EngineImpl::register_function(const std::string& name, void (*code)(std::vector<std::string>))
{
  simix_global->registered_functions[name] = [code](std::vector<std::string> args) {
    return std::bind(std::move(code), std::move(args));
  };
}

void EngineImpl::register_default(xbt_main_func_t code)
{
  simix_global->default_function = [code](std::vector<std::string> args) {
    return xbt::wrap_main(code, std::move(args));
  };
}

} // namespace kernel
} // namespace simgrid
