/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/EngineImpl.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/simix/smx_private.hpp"
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

  for (auto const& kv : links_)
    if (kv.second)
      kv.second->destroy();
}

void EngineImpl::load_deployment(const std::string& file) const
{
  sg_platf_exit();
  sg_platf_init();

  surf_parse_open(file);
  surf_parse();
  surf_parse_close();
}

void EngineImpl::register_function(const std::string& name, const actor::ActorCodeFactory& code)
{
  registered_functions[name] = code;
}
void EngineImpl::register_default(const actor::ActorCodeFactory& code)
{
  default_function = code;
}

void EngineImpl::add_model(resource::Model::Type type, std::shared_ptr<resource::Model> model, bool is_default)
{
  if (is_default)
    models_by_type_[type].insert(models_by_type_[type].begin(), model.get());
  else
    models_by_type_[type].push_back(model.get());

  models_.push_back(std::move(model));
}

resource::Model* EngineImpl::get_default_model(resource::Model::Type type) const
{
  resource::Model* model = nullptr;
  if (models_by_type_.find(type) != models_by_type_.end() and models_by_type_.at(type).size() > 0)
    return models_by_type_.at(type)[0];
  return model;
}

const std::vector<resource::Model*>& EngineImpl::get_model_list(resource::Model::Type type)
{
  return models_by_type_[type];
}

} // namespace kernel
} // namespace simgrid
