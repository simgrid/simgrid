/* Copyright (c) 2013-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "storage_n11.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/xml/platf.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_storage);

/*************
 * CallBacks *
 *************/
extern std::map<std::string, simgrid::kernel::resource::StorageType*> storage_types;

void check_disk_attachment()
{
  for (auto const& s : simgrid::s4u::Engine::get_instance()->get_all_storages()) {
    const simgrid::kernel::routing::NetPoint* host_elm =
        simgrid::s4u::Engine::get_instance()->netpoint_by_name_or_null(s->get_impl()->get_host());
    if (not host_elm)
      surf_parse_error(std::string("Unable to attach storage ") + s->get_cname() + ": host " +
                       s->get_impl()->get_host() + " does not exist.");
    else
      s->set_host(simgrid::s4u::Host::by_name(s->get_impl()->get_host()));
  }
}

/*********
 * Model *
 *********/

void surf_storage_model_init_default()
{
  surf_storage_model = new simgrid::kernel::resource::StorageN11Model();
}

namespace simgrid {
namespace kernel {
namespace resource {

StorageN11Model::StorageN11Model()
{
  all_existing_models.push_back(this);
}

StorageImpl* StorageN11Model::createStorage(const std::string& id, const std::string& type_id,
                                            const std::string& content_name, const std::string& attach)
{
  const StorageType* storage_type = storage_types.at(type_id);

  double Bread =
      surf_parse_get_bandwidth(storage_type->model_properties->at("Bread").c_str(), "property Bread, storage", type_id);
  double Bwrite = surf_parse_get_bandwidth(storage_type->model_properties->at("Bwrite").c_str(),
                                           "property Bwrite, storage", type_id);

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s'\n\t\tBread '%f'\n", id.c_str(), type_id.c_str(),
            Bread);

  return new StorageN11(this, id, get_maxmin_system(), Bread, Bwrite, type_id, content_name, storage_type->size,
                        attach);
}

double StorageN11Model::next_occurring_event(double now)
{
  return StorageModel::next_occurring_event_full(now);
}

void StorageN11Model::update_actions_state(double /*now*/, double delta)
{
  for (auto it = std::begin(*get_started_action_set()); it != std::end(*get_started_action_set());) {
    auto& action = *it;
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    action.update_remains(lrint(action.get_variable()->get_value() * delta));
    action.update_max_duration(delta);

    if (((action.get_remains_no_update() <= 0) && (action.get_variable()->get_penalty() > 0)) ||
        ((action.get_max_duration() != NO_MAX_DURATION) && (action.get_max_duration() <= 0))) {
      action.finish(Action::State::FINISHED);
    }
  }
}

/************
 * Resource *
 ************/

StorageN11::StorageN11(StorageModel* model, const std::string& name, lmm::System* maxminSystem, double bread,
                       double bwrite, const std::string& type_id, const std::string& content_name, sg_size_t size,
                       const std::string& attach)
    : StorageImpl(model, name, maxminSystem, bread, bwrite, type_id, content_name, size, attach)
{
  XBT_DEBUG("Create resource with Bread '%f' Bwrite '%f' and Size '%llu'", bread, bwrite, size);
  s4u::Storage::on_creation(*get_iface());
}

StorageAction* StorageN11::io_start(sg_size_t size, s4u::Io::OpType type)
{
  return new StorageN11Action(get_model(), size, not is_on(), this, type);
}

StorageAction* StorageN11::read(sg_size_t size)
{
  return new StorageN11Action(get_model(), size, not is_on(), this, s4u::Io::OpType::READ);
}

StorageAction* StorageN11::write(sg_size_t size)
{
  return new StorageN11Action(get_model(), size, not is_on(), this, s4u::Io::OpType::WRITE);
}

/**********
 * Action *
 **********/

StorageN11Action::StorageN11Action(Model* model, double cost, bool failed, StorageImpl* storage, s4u::Io::OpType type)
    : StorageAction(model, cost, failed, model->get_maxmin_system()->variable_new(this, 1.0, -1.0, 3), storage, type)
{
  XBT_IN("(%s,%g", storage->get_cname(), cost);

  // Must be less than the max bandwidth for all actions
  model->get_maxmin_system()->expand(storage->get_constraint(), get_variable(), 1.0);
  switch(type) {
    case s4u::Io::OpType::READ:
      model->get_maxmin_system()->expand(storage->constraint_read_, get_variable(), 1.0);
      break;
    case s4u::Io::OpType::WRITE:
      model->get_maxmin_system()->expand(storage->constraint_write_, get_variable(), 1.0);
      break;
    default:
      THROW_UNIMPLEMENTED;
  }
  XBT_OUT();
}

void StorageN11Action::cancel()
{
  set_state(Action::State::FAILED);
}

void StorageN11Action::suspend()
{
  XBT_IN("(%p)", this);
  if (is_running()) {
    get_model()->get_maxmin_system()->update_variable_penalty(get_variable(), 0.0);
    set_suspend_state(Action::SuspendStates::SUSPENDED);
  }
  XBT_OUT();
}

void StorageN11Action::resume()
{
  THROW_UNIMPLEMENTED;
}

void StorageN11Action::set_max_duration(double /*duration*/)
{
  THROW_UNIMPLEMENTED;
}

void StorageN11Action::set_sharing_penalty(double)
{
  THROW_UNIMPLEMENTED;
}
void StorageN11Action::update_remains_lazy(double /*now*/)
{
  THROW_IMPOSSIBLE;
}
} // namespace resource
} // namespace kernel
} // namespace simgrid
