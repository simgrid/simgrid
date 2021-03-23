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

void EngineImpl::add_model(std::shared_ptr<resource::Model> model, std::vector<std::string>&& dep_models)
{
  auto model_name = model->get_name();
  xbt_assert(models_prio_.find(model_name) == models_prio_.end(),
             "Model %s already exists, use model.set_name() to change its name", model_name.c_str());
  int order = -1;
  for (const auto& dep_name : dep_models) {
    xbt_assert(models_prio_.find(dep_name) != models_prio_.end(),
               "Model %s doesn't exists. Impossible to use it as dependency.", dep_name.c_str());
    if (models_prio_[dep_name].prio > order) {
      order = models_prio_[dep_name].prio;
    }
  }
  models_prio_[model_name] = {++order, std::move(model)};

  auto sorted_models = std::vector<std::pair<std::string, ModelStruct>>(models_prio_.begin(), models_prio_.end());
  std::sort(
      sorted_models.begin(), sorted_models.end(),
      [](const std::pair<std::string, ModelStruct>& first, const std::pair<std::string, ModelStruct>& second) -> bool {
        return first.second.prio < second.second.prio;
      });
  models_.clear();
  for (const auto& model : sorted_models) {
    models_.push_back(model.second.ptr.get());
  }
}

} // namespace kernel
} // namespace simgrid
